
/*
 * Copyright (C) Niklaus F.Schen.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
static inline void
mln_alloc_mgr_table_init(mln_alloc_mgr_t *tbl);
static inline mln_alloc_mgr_t *
mln_alloc_get_mgr_by_size(mln_alloc_mgr_t *tbl, mln_size_t size);


mln_alloc_t *mln_alloc_init(void)
{
    mln_alloc_t *pool = (mln_alloc_t *)malloc(sizeof(mln_alloc_t));
    if (pool == NULL) return pool;
    mln_alloc_mgr_table_init(pool->mgr_tbl);
    pool->large_used_head = pool->large_used_tail = NULL;
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
    free(pool);
}

void *mln_alloc_m(mln_alloc_t *pool, mln_size_t size)
{
    mln_alloc_blk_t *blk;
    mln_alloc_mgr_t *am;
    mln_alloc_chunk_t *ch;
    mln_u8ptr_t ptr;
    mln_size_t n, alloc_size;

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

