
/*
 * Copyright (C) Niklaus F.Schen.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <pthread.h>
#include "mln_alloc.h"
#include "mln_defs.h"
#include "mln_log.h"


char mln_alloc_check_bits[] = {
  0x4d, 0x61, 0x67, 0x65,
  0x62, 0x69, 0x74, 0x4e,
  0x69, 0x6b, 0x6c, 0x61,
  0x75, 0x73, 0x46, 0x53
};

MLN_CHAIN_FUNC_DECLARE(mln_chunk_blk, \
                       mln_alloc_blk_t, \
                       static inline void, \
                       );
MLN_CHAIN_FUNC_DECLARE(mln_blk, \
                       mln_alloc_blk_t, \
                       static inline void, \
                       __NONNULL3(1,2,3));
MLN_CHAIN_FUNC_DECLARE(mln_chunk, \
                       mln_alloc_chunk_t, \
                       static inline void, \
                       __NONNULL3(1,2,3));
MLN_CHAIN_FUNC_DECLARE(mln_alloc_shm, \
                       mln_alloc_shm_t, \
                       static inline void, \
                       __NONNULL3(1,2,3));
static inline void
mln_alloc_mgr_table_init(mln_alloc_mgr_t *tbl);
static inline mln_alloc_mgr_t *
mln_alloc_get_mgr_by_size(mln_alloc_mgr_t *tbl, mln_size_t size);
static inline void *mln_alloc_shm_m(mln_alloc_t *pool, mln_size_t size);
static inline void *mln_alloc_shm_large_m(mln_alloc_t *pool, mln_size_t size);
static inline int mln_alloc_shm_allowed(mln_alloc_shm_t *as, mln_off_t *Boff, mln_off_t *boff, mln_size_t size);
static inline void *mln_alloc_shm_set_bitmap(mln_alloc_shm_t *as, mln_off_t Boff, mln_off_t boff, mln_size_t size);
static inline mln_alloc_shm_t *mln_alloc_shm_new_block(mln_alloc_t *pool, mln_off_t *Boff, mln_off_t *boff, mln_size_t size);
static inline void mln_alloc_free_shm(void *ptr);

static inline mln_alloc_shm_t *mln_alloc_shm_new(mln_alloc_t *pool, mln_size_t size, int is_large)
{
    int n, i, j;
    mln_alloc_shm_t *shm;

    if ((shm = mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANON, -1, 0)) == NULL) {
        return NULL;
    }
    shm->pool = pool;
    shm->addr = shm;
    shm->size = size;
    shm->nfree = is_large ? 1: (size / M_ALLOC_SHM_BIT_SIZE);
    shm->base = shm->nfree;
    shm->large = is_large;
    shm->prev = shm->next = NULL;
    memset(shm->bitmap, 0, M_ALLOC_SHM_BITMAP_LEN);

    if (!is_large) {
        n = (sizeof(mln_alloc_shm_t)+M_ALLOC_SHM_BIT_SIZE-1) / M_ALLOC_SHM_BIT_SIZE;
        shm->nfree -= n;
        shm->base -= n;
        for (i = 0, j = 0; n > 0; --n) {
            shm->bitmap[i] |= (1 << (7-j));
            if (++j >= 8) {
                j = 0;
                ++i;
            }
        }
    }

    return shm;
}

static inline void mln_alloc_shm_free(mln_alloc_shm_t *shm)
{
    if (shm == NULL) return;
    munmap(shm->addr, shm->size);
}

mln_alloc_t *mln_alloc_shm_init(void)
{
    pthread_rwlockattr_t attr;
    mln_alloc_t *pool = (mln_alloc_t *)malloc(sizeof(mln_alloc_t));
    if (pool == NULL) return pool;
    pool->large_used_head = pool->large_used_tail = NULL;
    pool->shm_head = pool->shm_tail = NULL;
    pool->shm = 1;
    pthread_rwlockattr_init(&attr);
    pthread_rwlockattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    pthread_rwlock_init(&pool->rwlock, &attr);
    return pool;
}

mln_alloc_t *mln_alloc_init(void)
{
    mln_alloc_t *pool = (mln_alloc_t *)malloc(sizeof(mln_alloc_t));
    if (pool == NULL) return pool;
    mln_alloc_mgr_table_init(pool->mgr_tbl);
    pool->large_used_head = pool->large_used_tail = NULL;
    pool->shm_head = pool->shm_tail = NULL;
    pool->shm = 0;
    memset(&pool->rwlock, 0, sizeof(pthread_rwlock_t));
    mln_chunk_blk_chain_del(NULL, NULL, NULL);/* get rid of warning in Darwin */
    return pool;
}

static inline void
mln_alloc_mgr_table_init(mln_alloc_mgr_t *tbl)
{
    int i, j;
    mln_size_t blk_size;
    mln_alloc_mgr_t *am, *amprev;
    for (i = 0; i < M_ALLOC_MGR_LEN; i += M_ALLOC_MGR_GRAIN_SIZE) {
        blk_size = 0;
        for (j = 0; j < i/M_ALLOC_MGR_GRAIN_SIZE + M_ALLOC_BEGIN_OFF; ++j) {
             blk_size |= (((mln_size_t)1) << j);
        }
        am = &tbl[i];
        am->free_head = am->free_tail = NULL;
        am->used_head = am->used_tail = NULL;
        am->chunk_head = am->chunk_tail = NULL;
        am->blk_size = blk_size + 1;
        if (i != 0) {
            amprev = &tbl[i-1];
            amprev->free_head = amprev->free_tail = NULL;
            amprev->used_head = amprev->used_tail = NULL;
            amprev->chunk_head = amprev->chunk_tail = NULL;
            amprev->blk_size = (am->blk_size + tbl[i-2].blk_size) >> 1;
        }
    }
}

void mln_alloc_destroy(mln_alloc_t *pool)
{
    if (pool == NULL) return;

    if (!pool->shm) {
        mln_alloc_mgr_t *am, *amend;
        amend = pool->mgr_tbl + M_ALLOC_MGR_LEN;
        mln_alloc_chunk_t *ch;
        for (am = pool->mgr_tbl; am < amend; ++am) {
            while ((ch = am->chunk_head) != NULL) {
                mln_chunk_chain_del(&(am->chunk_head), &(am->chunk_tail), ch);
                free(ch);
            }
        }
        while ((ch = pool->large_used_head) != NULL) {
            mln_chunk_chain_del(&(pool->large_used_head), &(pool->large_used_tail), ch);
            free(ch);
        }
    } else {
        mln_alloc_shm_t *shm;
        while ((shm = pool->shm_head) != NULL) {
            mln_alloc_shm_chain_del(&pool->shm_head, &pool->shm_tail, shm);
            mln_alloc_shm_free(shm);
        }
        pthread_rwlock_destroy(&pool->rwlock);
    }
    free(pool);
}

void *mln_alloc_m(mln_alloc_t *pool, mln_size_t size)
{
    mln_alloc_blk_t *blk;
    mln_alloc_mgr_t *am;
    mln_alloc_chunk_t *ch;
    mln_u8ptr_t ptr;
    mln_size_t n, alloc_size;

    if (pool->shm) {
        return mln_alloc_shm_m(pool, size);
    }

    am = mln_alloc_get_mgr_by_size(pool->mgr_tbl, size);

    if (am == NULL) {
        size += (sizeof(mln_alloc_blk_t) + sizeof(mln_alloc_check_bits) + sizeof(mln_alloc_chunk_t));
        n = size / sizeof(mln_uauto_t);
        if (size % sizeof(mln_uauto_t)) ++n;
        size = n * sizeof(mln_uauto_t);
        ptr = (mln_u8ptr_t)malloc(size);
        if (ptr == NULL) return NULL;
        ch = (mln_alloc_chunk_t *)ptr;
        ch->prev = ch->next = NULL;
        ch->refer = 1;
        ch->mgr = NULL;
        ch->head = ch->tail = NULL;
        mln_chunk_chain_add(&(pool->large_used_head), &(pool->large_used_tail), ch);
        blk = (mln_alloc_blk_t *)(ptr + sizeof(mln_alloc_chunk_t));
        blk->prev = blk->next = NULL;
        blk->chunk_prev = blk->chunk_next = NULL;
        blk->data = ptr + sizeof(mln_alloc_chunk_t) + sizeof(mln_alloc_blk_t);
        blk->chunk = ch;
        blk->pool = pool;
        blk->blk_size = size - (sizeof(mln_alloc_chunk_t) + sizeof(mln_alloc_blk_t) + sizeof(mln_alloc_check_bits));
        blk->is_large = 1;
        blk->in_used = 1;
        blk->padding = 0;
        memcpy(ptr+sizeof(mln_alloc_chunk_t)+sizeof(mln_alloc_blk_t)+blk->blk_size, mln_alloc_check_bits, sizeof(mln_alloc_check_bits));
        mln_chunk_blk_chain_add(&(ch->head), &(ch->tail), blk);
        return blk->data;
    }

    if (am->free_head == NULL) {
        mln_alloc_blk_t *arr[M_ALLOC_BLK_NUM];

        size = sizeof(mln_alloc_blk_t) + sizeof(mln_alloc_check_bits) + am->blk_size;
        n = size / sizeof(mln_uauto_t);
        if (size % sizeof(mln_uauto_t)) ++n;
        size = n * sizeof(mln_uauto_t);

        alloc_size = sizeof(mln_alloc_chunk_t) + M_ALLOC_BLK_NUM * size;
        n = alloc_size / sizeof(mln_uauto_t);
        if (alloc_size % sizeof(mln_uauto_t)) ++n;
        alloc_size = n * sizeof(mln_uauto_t);;

        if ((ptr = (mln_u8ptr_t)malloc(alloc_size)) == NULL) {
            for (; am < pool->mgr_tbl + M_ALLOC_MGR_LEN; ++am) {
                if (am->free_head != NULL) goto out;
            }
            return NULL;
        }
        ch = (mln_alloc_chunk_t *)ptr;
        ch->prev = ch->next = NULL;
        ch->refer = 0;
        ch->mgr = am;
        ch->head = ch->tail = NULL;
        mln_chunk_chain_add(&(am->chunk_head), &(am->chunk_tail), ch);
        ptr += sizeof(mln_alloc_chunk_t);
        for (n = 0; n < M_ALLOC_BLK_NUM; ++n) {
            blk = (mln_alloc_blk_t *)ptr;
            blk->data = ptr + sizeof(mln_alloc_blk_t);
            blk->chunk = ch;
            blk->pool = pool;
            blk->blk_size = am->blk_size;
            blk->is_large = 0;
            blk->in_used = 0;
            blk->padding = 0;
            blk->prev = blk->next = NULL;
            blk->chunk_prev = blk->chunk_next = NULL;
            mln_chunk_blk_chain_add(&(ch->head), &(ch->tail), blk);
            memcpy(ptr+sizeof(mln_alloc_blk_t)+am->blk_size, mln_alloc_check_bits, sizeof(mln_alloc_check_bits));
            ptr += size;
            arr[n] = blk;
        }
        for (n = 0; n < M_ALLOC_BLK_NUM; ++n) {
            mln_blk_chain_add(&(am->free_head), &(am->free_tail), arr[M_ALLOC_BLK_NUM-1-n]);
        }
    }

out:
    blk = am->free_tail;
    mln_blk_chain_del(&(am->free_head), &(am->free_tail), blk);
    mln_blk_chain_add(&(am->used_head), &(am->used_tail), blk);
    blk->in_used = 1;
    ++(blk->chunk->refer);
    return blk->data;
}

static inline mln_alloc_mgr_t *
mln_alloc_get_mgr_by_size(mln_alloc_mgr_t *tbl, mln_size_t size)
{
    if (size > tbl[M_ALLOC_MGR_LEN-1].blk_size)
        return NULL;
    if (size <= tbl[0].blk_size) return &tbl[0];

    mln_alloc_mgr_t *am = tbl;
#if defined(i386) || defined(__x86_64)
    register mln_size_t off = 0;
    __asm__("bsr %1, %0":"=r"(off):"m"(size));
#else
    mln_size_t off = 0;
    int i;
    for (i = (sizeof(mln_size_t)<<3) - 1; i >= 0; --i) {
        if (size & (((mln_size_t)1) << i)) {
            off = i;
            break;
        }
    }
#endif
    off = (off - M_ALLOC_BEGIN_OFF) * M_ALLOC_MGR_GRAIN_SIZE;
    if (am[off].blk_size >= size) return &am[off];
    if (am[off+1].blk_size >= size) return &am[off+1];
    return &am[off+2];
}

void *mln_alloc_c(mln_alloc_t *pool, mln_size_t size)
{
    mln_u8ptr_t ptr = mln_alloc_m(pool, size);
    if (ptr == NULL) return NULL;
    memset(ptr, 0, size);
    return ptr;
}

void *mln_alloc_re(mln_alloc_t *pool, void *ptr, mln_size_t size)
{
    if (size == 0) {
        mln_alloc_free(ptr);
        return NULL;
    }

    mln_alloc_blk_t *old_blk = (mln_alloc_blk_t *)((mln_u8ptr_t)ptr - sizeof(mln_alloc_blk_t));
    if (old_blk->pool == pool && old_blk->blk_size >= size) {
        return ptr;
    }

    mln_u8ptr_t new_ptr = mln_alloc_m(pool, size);
    if (new_ptr == NULL) return NULL;
    memcpy(new_ptr, ptr, old_blk->blk_size);
    mln_alloc_free(ptr);
    
    return new_ptr;
}

void mln_alloc_free(void *ptr)
{
    if (ptr == NULL) {
        return;
    }

    mln_alloc_t *pool;
    mln_alloc_chunk_t *ch;
    mln_alloc_mgr_t *am;
    mln_alloc_blk_t *blk;

    blk = (mln_alloc_blk_t *)((mln_u8ptr_t)ptr - sizeof(mln_alloc_blk_t));

    if (!blk->in_used) {
        mln_log(error, "Double free.\n");
        abort();
    }

    if (memcmp(ptr + blk->blk_size, mln_alloc_check_bits, sizeof(mln_alloc_check_bits))) {
        mln_log(error, "Buffer overflow.\n");
        abort();
    }

    pool = blk->pool;
    if (pool->shm) {
        return mln_alloc_free_shm(ptr);
    }

    if (blk->is_large) {
        mln_chunk_chain_del(&(pool->large_used_head), &(pool->large_used_tail), blk->chunk);
        free(blk->chunk);
        return;
    }
    ch = blk->chunk;
    am = ch->mgr;
    blk->in_used = 0;
    mln_blk_chain_del(&(am->used_head), &(am->used_tail), blk);
    mln_blk_chain_add(&(am->free_head), &(am->free_tail), blk);
    if (!--(ch->refer)) {
        for (blk = ch->head; blk != NULL; blk = blk->chunk_next) {
            mln_blk_chain_del(&(am->free_head), &(am->free_tail), blk);
        }
        mln_chunk_chain_del(&(am->chunk_head), &(am->chunk_tail), ch);
        free(ch);
    }
}

int mln_alloc_shm_rdlock(mln_alloc_t *pool)
{
    return pthread_rwlock_rdlock(&pool->rwlock);
}

int mln_alloc_shm_tryrdlock(mln_alloc_t *pool)
{
    return pthread_rwlock_tryrdlock(&pool->rwlock);
}

int mln_alloc_shm_wrlock(mln_alloc_t *pool)
{
    return pthread_rwlock_wrlock(&pool->rwlock);
}

int mln_alloc_shm_trywrlock(mln_alloc_t *pool)
{
    return pthread_rwlock_trywrlock(&pool->rwlock);
}

int mln_alloc_shm_unlock(mln_alloc_t *pool)
{
    return pthread_rwlock_unlock(&pool->rwlock);
}

static inline void *mln_alloc_shm_m(mln_alloc_t *pool, mln_size_t size)
{
    mln_alloc_shm_t *as;
    mln_off_t Boff = -1, boff = -1;

    if (size > M_ALLOC_SHM_LARGE_SIZE) {
        return mln_alloc_shm_large_m(pool, size);
    }

    if (pool->shm_head == NULL) {
new_block:
        as = mln_alloc_shm_new_block(pool, &Boff, &boff, size);
        if (as == NULL) return NULL;
    } else {
        for (as = pool->shm_head; as != NULL; as = as->next) {
            if (mln_alloc_shm_allowed(as, &Boff, &boff, size)) {
                break;
            }
        }
        if (as == NULL) goto new_block;
    }
    return mln_alloc_shm_set_bitmap(as, Boff, boff, size);
}

static inline void *mln_alloc_shm_large_m(mln_alloc_t *pool, mln_size_t size)
{
    mln_alloc_shm_t *as;
    mln_alloc_blk_t *blk;

    if ((as = mln_alloc_shm_new(pool, size + sizeof(mln_alloc_shm_t)+sizeof(mln_alloc_blk_t)+sizeof(mln_alloc_check_bits), 1)) == NULL)
        return NULL;
    as->nfree = 0;
    mln_alloc_shm_chain_add(&pool->shm_head, &pool->shm_tail, as);
    blk = (mln_alloc_blk_t *)(as->addr+sizeof(mln_alloc_shm_t));
    memset(blk, 0, sizeof(mln_alloc_blk_t));
    blk->pool = pool;
    blk->blk_size = size;
    blk->data = as->addr + sizeof(mln_alloc_shm_t) + sizeof(mln_alloc_blk_t);
    blk->chunk = (mln_alloc_chunk_t *)as;
    blk->is_large = 1;
    blk->in_used = 1;
    memcpy(blk->data+blk->blk_size, mln_alloc_check_bits, sizeof(mln_alloc_check_bits));
    return blk->data;
}

static inline mln_alloc_shm_t *mln_alloc_shm_new_block(mln_alloc_t *pool, mln_off_t *Boff, mln_off_t *boff, mln_size_t size)
{
    mln_alloc_shm_t *ret;
    if ((ret = mln_alloc_shm_new(pool, M_ALLOC_SHM_DEFAULT_SIZE, 0)) == NULL) {
        return NULL;
    }
    mln_alloc_shm_allowed(ret, Boff, boff, size);
    mln_alloc_shm_chain_add(&pool->shm_head, &pool->shm_tail, ret);
    return ret;
}

static inline int mln_alloc_shm_allowed(mln_alloc_shm_t *as, mln_off_t *Boff, mln_off_t *boff, mln_size_t size)
{
    int i, j = -1, s = -1;
    int n = (size+sizeof(mln_alloc_blk_t)+sizeof(mln_alloc_check_bits)+M_ALLOC_SHM_BIT_SIZE-1) / M_ALLOC_SHM_BIT_SIZE;
    mln_u8ptr_t p, pend, save = NULL;

    if (n > as->nfree) return 0;

    p = as->bitmap;
    for (pend = p + M_ALLOC_SHM_BITMAP_LEN; p < pend; ++p) {
        if ((*p & 0xff) == 0xff) {
            if (save != NULL) {
                j = -1;
                s = -1;
                save = NULL;
            }
            continue;
        }

        for (i = 7; i >= 0; --i) {
            if (!(*p & ((mln_u8_t)1 << i))) {
                if (save == NULL) {
                    j = n;
                    s = i;
                    save = p;
                }
                if (--j <= 0) {
                    break;
                }
            } else if (save != NULL) {
                j = -1;
                s = -1;
                save = NULL;
            }
        }

        if (save != NULL && !j) {
            *Boff = save - as->bitmap;
            *boff = s;
            return 1;
        }
    }
    return 0;
}

static inline void *mln_alloc_shm_set_bitmap(mln_alloc_shm_t *as, mln_off_t Boff, mln_off_t boff, mln_size_t size)
{
    int i, n = (size+sizeof(mln_alloc_blk_t)+sizeof(mln_alloc_check_bits)+M_ALLOC_SHM_BIT_SIZE-1) / M_ALLOC_SHM_BIT_SIZE;
    mln_u8ptr_t p, pend, addr;
    mln_alloc_blk_t *blk;

    addr = as->addr + (Boff * 8 + (7 - boff)) * 64;
    blk = (mln_alloc_blk_t *)addr;
    memset(blk, 0, sizeof(mln_alloc_blk_t));
    blk->pool = as->pool;
    blk->data = addr + sizeof(mln_alloc_blk_t);
    blk->chunk = (mln_alloc_chunk_t *)as;
    blk->blk_size = size;
    blk->padding = ((Boff & 0xffff) << 8) | (boff & 0xff);
    blk->is_large = 0;
    blk->in_used = 1;
    memcpy(blk->data+blk->blk_size, mln_alloc_check_bits, sizeof(mln_alloc_check_bits));
    p = as->bitmap + Boff;
    pend = p + M_ALLOC_SHM_BITMAP_LEN;
    for (i = boff; p < pend;) {
        *p |= ((mln_u8_t)1 << i);
        --as->nfree;
        if (--n <= 0) break;
        if (--i < 0) {
            i = 7;
            ++p;
        }
    }

    return blk->data;
}

static inline void mln_alloc_free_shm(void *ptr)
{
    mln_alloc_blk_t *blk;
    mln_alloc_shm_t *as;
    mln_off_t Boff, boff;
    mln_u8ptr_t p, pend;
    int i, n;

    blk = (mln_alloc_blk_t *)((mln_u8ptr_t)ptr - sizeof(mln_alloc_blk_t));
    as = (mln_alloc_shm_t *)(blk->chunk);
    if (!as->large) {
        Boff = (blk->padding >> 8) & 0xffff;
        boff = blk->padding & 0xff;
        blk->in_used = 0;
        p = as->bitmap + Boff;
        n = (blk->blk_size+sizeof(mln_alloc_blk_t)+sizeof(mln_alloc_check_bits)+M_ALLOC_SHM_BIT_SIZE-1) / M_ALLOC_SHM_BIT_SIZE;
        i = boff;
        for (pend = as->bitmap+M_ALLOC_SHM_BITMAP_LEN; p < pend;) {
            *p &= (~((mln_u8_t)1 << i));
            ++as->nfree;
            if (--n <= 0) break;
            if (--i < 0) {
                i = 7;
                ++p;
            }
        }
    }
    if (as->large || as->nfree == as->base) {
        mln_alloc_shm_chain_del(&as->pool->shm_head, &as->pool->shm_tail, as);
        mln_alloc_shm_free(as);
    }
}

/*
 * chain
 */
MLN_CHAIN_FUNC_DEFINE(mln_chunk_blk, \
                      mln_alloc_blk_t, \
                      static inline void, \
                      chunk_prev, \
                      chunk_next);
MLN_CHAIN_FUNC_DEFINE(mln_blk, \
                      mln_alloc_blk_t, \
                      static inline void, \
                      prev, \
                      next);

MLN_CHAIN_FUNC_DEFINE(mln_chunk, \
                      mln_alloc_chunk_t, \
                      static inline void, \
                      prev, \
                      next);
MLN_CHAIN_FUNC_DEFINE(mln_alloc_shm, \
                      mln_alloc_shm_t, \
                      static inline void, \
                      prev, \
                      next);

