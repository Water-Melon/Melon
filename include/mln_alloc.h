
/*
 * Copyright (C) Niklaus F.Schen.
 */

#ifndef __MLN_ALLOC_H
#define __MLN_ALLOC_H

#if defined(MSVC)
#include <windows.h>
#include <winbase.h>
#else
#include <sys/mman.h>
#endif
#include "mln_types.h"
#include "mln_utils.h"
#include <stdlib.h>
#include <string.h>
#if defined(MLN_C99)
#include <fcntl.h>
#endif
#include "mln_func.h"

typedef int (*mln_alloc_shm_lock_cb_t)(void *);

#define M_ALLOC_BEGIN_OFF        ((mln_size_t)4)
#define M_ALLOC_MGR_GRAIN_SIZE   2
#define M_ALLOC_MGR_LEN          18*M_ALLOC_MGR_GRAIN_SIZE-(M_ALLOC_MGR_GRAIN_SIZE-1)
#define M_ALLOC_BLK_NUM          4
#define M_ALLOC_CHUNK_COUNT      1023

#define M_ALLOC_SHM_BITMAP_LEN   4096
#define M_ALLOC_SHM_BIT_SIZE     64
#define M_ALLOC_SHM_LARGE_SIZE   (1*1024+512)*1024
#define M_ALLOC_SHM_DEFAULT_SIZE 2*1024*1024

typedef struct mln_alloc_s       mln_alloc_t;
typedef struct mln_alloc_mgr_s   mln_alloc_mgr_t;
typedef struct mln_alloc_chunk_s mln_alloc_chunk_t;

/*
 * Note:
 * In Darwin, if set mln_alloc_blk_t's prev and next at the beginning of the structure,
 * program will encounter segmentation fault.
 * It seems that we can not set bit variables those summary not enough aligned bytes
 * at the end of structure.
 * But in Linux, no such kind of problem.
 */
typedef struct mln_alloc_blk_s {
    struct mln_alloc_blk_s   *prev;
    struct mln_alloc_blk_s   *next;
    mln_alloc_t              *pool;
    void                     *data;
    mln_alloc_chunk_t        *chunk;
    mln_size_t                blk_size;
    mln_size_t                is_large:1;
    mln_size_t                in_used:1;
    mln_size_t                padding:30;
} mln_alloc_blk_t;

struct mln_alloc_chunk_s {
    struct mln_alloc_chunk_s *prev;
    struct mln_alloc_chunk_s *next;
    mln_size_t                refer;
    mln_size_t                count;
    mln_alloc_mgr_t          *mgr;
    mln_alloc_blk_t          *blks[M_ALLOC_BLK_NUM+1];
};

struct mln_alloc_mgr_s {
    mln_size_t                blk_size;
    mln_alloc_blk_t          *free_head;
    mln_alloc_blk_t          *free_tail;
    mln_alloc_blk_t          *used_head;
    mln_alloc_blk_t          *used_tail;
    mln_alloc_chunk_t        *chunk_head;
    mln_alloc_chunk_t        *chunk_tail;
};

typedef struct mln_alloc_shm_s {
    struct mln_alloc_shm_s   *prev;
    struct mln_alloc_shm_s   *next;
    mln_alloc_t              *pool;
#if defined(MSVC)
    mln_u8ptr_t               addr;
#else
    void                     *addr;
#endif
    mln_size_t                size;
    mln_u32_t                 nfree;
    mln_u32_t                 base:31;
    mln_u32_t                 large:1;
    mln_u8_t                  bitmap[M_ALLOC_SHM_BITMAP_LEN];
} mln_alloc_shm_t;

struct mln_alloc_s {
#if defined(MSVC)
    mln_u8ptr_t               mem;
#else
    void                     *mem;
#endif
    mln_size_t                shm_size;
    void                     *locker;
    mln_alloc_shm_lock_cb_t   lock;
    mln_alloc_shm_lock_cb_t   unlock;
#if defined(MSVC)
    HANDLE                    map_handle;
#endif
    struct mln_alloc_s       *parent;
    mln_alloc_mgr_t           mgr_tbl[M_ALLOC_MGR_LEN];
    mln_alloc_chunk_t        *large_used_head;
    mln_alloc_chunk_t        *large_used_tail;
    mln_alloc_shm_t          *shm_head;
    mln_alloc_shm_t          *shm_tail;
};


#define mln_alloc_is_shm(pool) (pool->mem != NULL)


static inline mln_alloc_t *mln_alloc_shm_init(mln_size_t size, void *locker, mln_alloc_shm_lock_cb_t lock, mln_alloc_shm_lock_cb_t unlock);
static inline mln_alloc_t *mln_alloc_init(mln_alloc_t *parent);
static inline void mln_alloc_destroy(mln_alloc_t *pool);
static inline void *mln_alloc_m(mln_alloc_t *pool, mln_size_t size);
static inline void *mln_alloc_c(mln_alloc_t *pool, mln_size_t size);
static inline void *mln_alloc_re(mln_alloc_t *pool, void *ptr, mln_size_t size);
static inline void mln_alloc_free(void *ptr);

MLN_CHAIN_FUNC_DECLARE(static inline, \
                       mln_blk, \
                       mln_alloc_blk_t, );
MLN_CHAIN_FUNC_DECLARE(static inline, \
                       mln_chunk, \
                       mln_alloc_chunk_t, );
MLN_CHAIN_FUNC_DECLARE(static inline, \
                       mln_alloc_shm, \
                       mln_alloc_shm_t, );
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
    mln_alloc_shm_t *shm, *tmp;
    mln_u8ptr_t p = pool->mem + sizeof(mln_alloc_t);

    for (tmp = pool->shm_head; tmp != NULL; tmp = tmp->next) {
        if ((mln_u8ptr_t)(tmp->addr) - p >= size) break;
        p = tmp->addr + tmp->size;
    }
    if (tmp == NULL && (mln_u8ptr_t)(pool->mem + pool->shm_size) - p < size) {
        return NULL;
    }

    shm = (mln_alloc_shm_t *)p;
    shm->pool = pool;
    shm->addr = p;
    shm->size = size;
    shm->nfree = is_large ? 1: (size / M_ALLOC_SHM_BIT_SIZE);
    shm->base = shm->nfree;
    shm->large = is_large;
    shm->prev = shm->next = NULL;
    if (tmp == NULL) {
        mln_alloc_shm_chain_add(&pool->shm_head, &pool->shm_tail, shm);
    } else {
        if (tmp == pool->shm_head) {
            shm->next = tmp;
            shm->prev = NULL;
            tmp->prev = shm;
            pool->shm_head = shm;
        } else {
            shm->next = tmp;
            shm->prev = tmp->prev;
            tmp->prev->next = shm;
            tmp->prev = shm;
        }
    }

    if (!is_large) {
        memset(shm->bitmap, 0, M_ALLOC_SHM_BITMAP_LEN);
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

static inline mln_alloc_t *mln_alloc_shm_init(mln_size_t size, void *locker, mln_alloc_shm_lock_cb_t lock, mln_alloc_shm_lock_cb_t unlock)
{
    mln_alloc_t *pool;
#if defined(MSVC)
    HANDLE handle;
#endif

    if (size < M_ALLOC_SHM_DEFAULT_SIZE+1024 || locker == NULL || lock == NULL || unlock == NULL)
        return NULL;

#if defined(MSVC)
    if ((handle = CreateFileMapping(INVALID_HANDLE_VALUE,
                                    NULL,
                                    PAGE_READWRITE,
#if defined(__x86_64)
                                    (u_long) (size >> 32),
#else
                                    0,
#endif
                                    (u_long) (size & 0xffffffff),
                                    NULL)) == NULL)
    {
        return NULL;
    }
    pool = (mln_alloc_t *)MapViewOfFile(handle, FILE_MAP_WRITE, 0, 0, 0);
    if (pool == NULL) {
        CloseHandle(handle);
        return NULL;
    }
    pool->map_handle = handle;
#else

#if defined(MLN_MMAP)
#if defined(MLN_C99)
    int fd = open("/dev/zero", O_RDWR);
    if (fd < 0) return NULL;
    pool = (mln_alloc_t *)mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    close(fd);
#else
    pool = (mln_alloc_t *)mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANON, -1, 0);
#endif
    if (pool == NULL) return NULL;
#else
    return NULL;
#endif
#endif
    pool->parent = NULL;
    pool->large_used_head = pool->large_used_tail = NULL;
    pool->shm_head = pool->shm_tail = NULL;
#if defined(MSVC)
    pool->mem = (mln_u8ptr_t)pool;
#else
    pool->mem = pool;
#endif
    pool->shm_size = size;
    pool->locker = locker;
    pool->lock = lock;
    pool->unlock = unlock;
    return pool;
}

static inline mln_alloc_t *mln_alloc_init(mln_alloc_t *parent)
{
    mln_alloc_t *pool;

#ifdef __DEBUG__
    (void)mln_blk_chain_add;
    (void)mln_blk_chain_del;
    (void)mln_chunk_chain_add;
    (void)mln_alloc_get_mgr_by_size;
    (void)mln_alloc_shm_m;
    (void)mln_alloc_free_shm;
#endif

    if (parent != NULL) {
        if (mln_alloc_is_shm(parent)) {
            if (parent->lock(parent->locker) != 0) return NULL;
        }
        pool = (mln_alloc_t *)mln_alloc_m(parent, sizeof(mln_alloc_t));
        if (mln_alloc_is_shm(parent)) {
            (void)parent->unlock(parent->locker);
        }
    } else {
        pool = (mln_alloc_t *)malloc(sizeof(mln_alloc_t));
    }
    if (pool == NULL) return pool;
    mln_alloc_mgr_table_init(pool->mgr_tbl);
    pool->parent = parent;
    pool->large_used_head = pool->large_used_tail = NULL;
    pool->shm_head = pool->shm_tail = NULL;
    pool->mem = NULL;
    pool->shm_size = 0;
    pool->locker = NULL;
    pool->lock = NULL;
    pool->unlock = NULL;
    return pool;
}

MLN_FUNC_VOID(static inline, void, mln_alloc_mgr_table_init, (mln_alloc_mgr_t *tbl), (tbl), {
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
})

static inline void mln_alloc_destroy(mln_alloc_t *pool)
{
    if (pool == NULL) return;

    mln_alloc_t *parent = pool->parent;
    if (parent != NULL && mln_alloc_is_shm(parent))
        if (parent->lock(parent->locker) != 0)
            return;
    if (pool->mem == NULL) {
        mln_alloc_mgr_t *am, *amend;
        amend = pool->mgr_tbl + M_ALLOC_MGR_LEN;
        mln_alloc_chunk_t *ch;
        for (am = pool->mgr_tbl; am < amend; ++am) {
            while ((ch = am->chunk_head) != NULL) {
                mln_chunk_chain_del(&(am->chunk_head), &(am->chunk_tail), ch);
                if (parent != NULL) mln_alloc_free(ch);
                else free(ch);
            }
        }
        while ((ch = pool->large_used_head) != NULL) {
            mln_chunk_chain_del(&(pool->large_used_head), &(pool->large_used_tail), ch);
            if (parent != NULL) mln_alloc_free(ch);
            else free(ch);
        }
        if (parent != NULL) mln_alloc_free(pool);
        else free(pool);
    } else {
#if defined(MSVC)
        HANDLE handle = pool->map_handle;
        UnmapViewOfFile(pool->mem);
        CloseHandle(handle);
#else
#if defined(MLN_MMAP)
        munmap(pool->mem, pool->shm_size);
#endif
#endif
    }
    if (parent != NULL && mln_alloc_is_shm(parent))
        (void)parent->unlock(parent->locker);
}

static inline void *mln_alloc_m(mln_alloc_t *pool, mln_size_t size)
{
#ifdef __DEBUG__
    return malloc(size);
#else
    mln_alloc_blk_t *blk;
    mln_alloc_mgr_t *am;
    mln_alloc_chunk_t *ch;
    mln_u8ptr_t ptr;
    mln_size_t n;

    if (pool->mem != NULL) {
        return mln_alloc_shm_m(pool, size);
    }

    am = mln_alloc_get_mgr_by_size(pool->mgr_tbl, size);

    if (am == NULL) {
        n = (size + sizeof(mln_alloc_blk_t) + sizeof(mln_alloc_chunk_t) + 3) >> 2;
        size = n << 2;
        if (pool->parent != NULL) {
            if (mln_alloc_is_shm(pool->parent)) {
                if (pool->parent->lock(pool->parent->locker) != 0)
                    return NULL;
            }
            ptr = (mln_u8ptr_t)mln_alloc_c(pool->parent, size);
            if (mln_alloc_is_shm(pool->parent)) {
                (void)pool->parent->unlock(pool->parent->locker);
            }
        } else {
            ptr = (mln_u8ptr_t)calloc(1, size);
        }
        if (ptr == NULL) return NULL;
        ch = (mln_alloc_chunk_t *)ptr;
        ch->refer = 1;
        mln_chunk_chain_add(&(pool->large_used_head), &(pool->large_used_tail), ch);
        blk = (mln_alloc_blk_t *)(ptr + sizeof(mln_alloc_chunk_t));
        blk->data = ptr + sizeof(mln_alloc_chunk_t) + sizeof(mln_alloc_blk_t);
        blk->chunk = ch;
        blk->pool = pool;
        blk->blk_size = size - (sizeof(mln_alloc_chunk_t) + sizeof(mln_alloc_blk_t));
        blk->is_large = 1;
        blk->in_used = 1;
        ch->blks[0] = blk;
        return blk->data;
    }

    if (am->free_head == NULL) {
        n = (sizeof(mln_alloc_blk_t) + am->blk_size + 3) >> 2;
        size = n << 2;

        n = (sizeof(mln_alloc_chunk_t) + M_ALLOC_BLK_NUM * size + 3) >> 2;

        if (pool->parent != NULL) {
            if (mln_alloc_is_shm(pool->parent)) {
                if (pool->parent->lock(pool->parent->locker) != 0)
                    return NULL;
            }
            ptr = (mln_u8ptr_t)mln_alloc_m(pool->parent, n << 2);
            if (mln_alloc_is_shm(pool->parent)) {
                (void)pool->parent->unlock(pool->parent->locker);
            }
        } else {
            ptr = (mln_u8ptr_t)malloc(n << 2);
        }
        if (ptr == NULL) {
            for (; am < pool->mgr_tbl + M_ALLOC_MGR_LEN; ++am) {
                if (am->free_head != NULL) goto out;
            }
            return NULL;
        }
        ch = (mln_alloc_chunk_t *)ptr;
        mln_chunk_chain_add(&(am->chunk_head), &(am->chunk_tail), ch);
        ch->refer = ch->count = 0;
        ch->mgr = am;
        ptr += sizeof(mln_alloc_chunk_t);
        for (n = 0; n < M_ALLOC_BLK_NUM; ++n) {
            blk = (mln_alloc_blk_t *)ptr;
            mln_blk_chain_add(&(am->free_head), &(am->free_tail), blk);
            blk->pool = pool;
            blk->data = ptr + sizeof(mln_alloc_blk_t);
            blk->chunk = ch;
            blk->blk_size = am->blk_size;
            blk->is_large = blk->in_used = 0;
            ch->blks[n] = blk;
            ptr += size;
        }
        ch->blks[n] = NULL;
    }

out:
    blk = am->free_tail;
    mln_blk_chain_del(&(am->free_head), &(am->free_tail), blk);
    mln_blk_chain_add(&(am->used_head), &(am->used_tail), blk);
    blk->in_used = 1;
    ++(blk->chunk->refer);
    return blk->data;
#endif
}

static inline mln_alloc_mgr_t *mln_alloc_get_mgr_by_size(mln_alloc_mgr_t *tbl, mln_size_t size)
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

static inline void *mln_alloc_c(mln_alloc_t *pool, mln_size_t size)
{
#ifdef __DEBUG__
    return calloc(1, size);
#else
    mln_u8ptr_t ptr = mln_alloc_m(pool, size);
    if (ptr == NULL) return NULL;
    memset(ptr, 0, size);
    return ptr;
#endif
}

static void *mln_alloc_re(mln_alloc_t *pool, void *ptr, mln_size_t size)
{
#ifdef __DEBUG__
    return realloc(ptr, size);
#else
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
#endif
}

static inline void mln_alloc_free(void *ptr)
{
    if (ptr == NULL) {
        return;
    }
#ifdef __DEBUG__
    return free(ptr);
#else

    mln_alloc_t *pool;
    mln_alloc_chunk_t *ch;
    mln_alloc_mgr_t *am;
    mln_alloc_blk_t *blk;

    blk = (mln_alloc_blk_t *)((mln_u8ptr_t)ptr - sizeof(mln_alloc_blk_t));

    ASSERT(blk->in_used);

    pool = blk->pool;
    if (pool->mem) {
        mln_alloc_free_shm(ptr);
        return;
    }

    if (blk->is_large) {
        mln_chunk_chain_del(&(pool->large_used_head), &(pool->large_used_tail), blk->chunk);
        if (pool->parent != NULL) {
            if (mln_alloc_is_shm(pool->parent)) {
                if (pool->parent->lock(pool->parent->locker) != 0) {
                    return;
                }
            }
            mln_alloc_free(blk->chunk);
            if (mln_alloc_is_shm(pool->parent)) {
                (void)pool->parent->unlock(pool->parent->locker);
            }
        } else
            free(blk->chunk);
        return;
    }
    ch = blk->chunk;
    am = ch->mgr;
    blk->in_used = 0;
    mln_blk_chain_del(&(am->used_head), &(am->used_tail), blk);
    mln_blk_chain_add(&(am->free_head), &(am->free_tail), blk);
    if (!--(ch->refer) && ++(ch->count) > M_ALLOC_CHUNK_COUNT) {
        mln_alloc_blk_t **blks = ch->blks;
        while (*blks != NULL) {
            mln_blk_chain_del(&(am->free_head), &(am->free_tail), *(blks++));
        }
        mln_chunk_chain_del(&(am->chunk_head), &(am->chunk_tail), ch);
        if (pool->parent != NULL) {
            if (mln_alloc_is_shm(pool->parent)) {
                if (pool->parent->lock(pool->parent->locker) != 0) {
                    return;
                }
            }
            mln_alloc_free(ch);
            if (mln_alloc_is_shm(pool->parent)) {
                (void)pool->parent->unlock(pool->parent->locker);
            }
        } else
            free(ch);
    }
#endif
}

MLN_FUNC(static inline, void *, mln_alloc_shm_m, (mln_alloc_t *pool, mln_size_t size), (pool, size), {
    mln_alloc_shm_t *as;
    mln_off_t Boff = -1, boff = -1;

    if (size > M_ALLOC_SHM_LARGE_SIZE) {
        return mln_alloc_shm_large_m(pool, size);
    }

    if (pool->shm_head == NULL) {
new_block:
        as = mln_alloc_shm_new_block(pool, &Boff, &boff, size);
        if (as == NULL) {
            return NULL;
        }
    } else {
        for (as = pool->shm_head; as != NULL; as = as->next) {
            if (mln_alloc_shm_allowed(as, &Boff, &boff, size)) {
                break;
            }
        }
        if (as == NULL) goto new_block;
    }
    return mln_alloc_shm_set_bitmap(as, Boff, boff, size);
})

static inline void *mln_alloc_shm_large_m(mln_alloc_t *pool, mln_size_t size)
{
    mln_alloc_shm_t *as;
    mln_alloc_blk_t *blk;

    if ((as = mln_alloc_shm_new(pool, size + sizeof(mln_alloc_shm_t)+sizeof(mln_alloc_blk_t), 1)) == NULL)
        return NULL;
    as->nfree = 0;
    blk = (mln_alloc_blk_t *)(as->addr+sizeof(mln_alloc_shm_t));
    memset(blk, 0, sizeof(mln_alloc_blk_t));
    blk->pool = pool;
    blk->blk_size = size;
    blk->data = as->addr + sizeof(mln_alloc_shm_t) + sizeof(mln_alloc_blk_t);
    blk->chunk = (mln_alloc_chunk_t *)as;
    blk->is_large = 1;
    blk->in_used = 1;
    return blk->data;
}

MLN_FUNC(static inline, mln_alloc_shm_t *, mln_alloc_shm_new_block, \
         (mln_alloc_t *pool, mln_off_t *Boff, mln_off_t *boff, mln_size_t size), \
         (pool, Boff, boff, size), \
{
    mln_alloc_shm_t *ret;
    if ((ret = mln_alloc_shm_new(pool, M_ALLOC_SHM_DEFAULT_SIZE, 0)) == NULL) {
        return NULL;
    }
    mln_alloc_shm_allowed(ret, Boff, boff, size);
    return ret;
})

MLN_FUNC(static inline, int, mln_alloc_shm_allowed, \
         (mln_alloc_shm_t *as, mln_off_t *Boff, mln_off_t *boff, mln_size_t size), \
         (as, Boff, boff, size), \
{
    int i, j = -1, s = -1;
    int n = (size+sizeof(mln_alloc_blk_t)+M_ALLOC_SHM_BIT_SIZE-1) / M_ALLOC_SHM_BIT_SIZE;
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
})

MLN_FUNC(static inline, void *, mln_alloc_shm_set_bitmap, \
         (mln_alloc_shm_t *as, mln_off_t Boff, mln_off_t boff, mln_size_t size), \
         (as, Boff, boff, size), \
{
    int i, n = (size+sizeof(mln_alloc_blk_t)+M_ALLOC_SHM_BIT_SIZE-1) / M_ALLOC_SHM_BIT_SIZE;
    mln_u8ptr_t p, pend, addr;
    mln_alloc_blk_t *blk;

    addr = as->addr + (Boff * 8 + (7 - boff)) * M_ALLOC_SHM_BIT_SIZE;
    blk = (mln_alloc_blk_t *)addr;
    memset(blk, 0, sizeof(mln_alloc_blk_t));
    blk->pool = as->pool;
    blk->data = addr + sizeof(mln_alloc_blk_t);
    blk->chunk = (mln_alloc_chunk_t *)as;
    blk->blk_size = size;
    blk->padding = ((Boff & 0xffff) << 8) | (boff & 0xff);
    blk->is_large = 0;
    blk->in_used = 1;
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
})

MLN_FUNC_VOID(static inline, void, mln_alloc_free_shm, (void *ptr), (ptr), {
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
        n = (blk->blk_size+sizeof(mln_alloc_blk_t)+M_ALLOC_SHM_BIT_SIZE-1) / M_ALLOC_SHM_BIT_SIZE;
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
    }
})

/*
 * chain
 */
MLN_CHAIN_FUNC_DEFINE(static inline, \
                      mln_blk, \
                      mln_alloc_blk_t, \
                      prev, \
                      next);
MLN_CHAIN_FUNC_DEFINE(static inline, \
                      mln_chunk, \
                      mln_alloc_chunk_t, \
                      prev, \
                      next);
MLN_CHAIN_FUNC_DEFINE(static inline, \
                      mln_alloc_shm, \
                      mln_alloc_shm_t, \
                      prev, \
                      next);

#endif

