
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
  0x4d, 0x45, 0x4c, 0x4f,
  0x4e, 0x53, 0x68, 0x65,
  0x6e, 0x46, 0x61, 0x6e,
  0x63, 0x68, 0x65, 0x6e
};

MLN_CHAIN_FUNC_DECLARE(mln_small, \
                       mln_alloc_blk_t, \
                       static inline void, \
                       __NONNULL3(1,2,3));
MLN_CHAIN_FUNC_DECLARE(mln_large, \
                       mln_alloc_blk_t, \
                       static inline void, \
                       __NONNULL3(1,2,3));
static inline void
mln_alloc_mgr_table_init(mln_alloc_mgr_t *tbl);
static inline mln_size_t
mln_alloc_calc_divisor(mln_size_t size, mln_size_t start);
static inline mln_alloc_mgr_t *
mln_alloc_get_mgr_by_size(mln_alloc_mgr_t *tbl, mln_size_t size);
static inline void
mln_alloc_reduce(mln_alloc_t *pool);


mln_alloc_t *mln_alloc_init(void)
{
    mln_alloc_t *pool = (mln_alloc_t *)malloc(sizeof(mln_alloc_t));
    if (pool == NULL) return pool;

    mln_alloc_mgr_table_init(pool->mgr_tbl);

    pool->large_used_head = pool->large_used_tail = NULL;
    pool->cur_size = 0;
    pool->threshold = 0;

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
        am->blk_size = blk_size;
        if (i != 0) {
            amprev = &tbl[i-1];
            amprev->free_head = amprev->free_tail = NULL;
            amprev->used_head = amprev->used_tail = NULL;
            amprev->blk_size = (am->blk_size + tbl[i-2].blk_size) >> 1;
        }
    }
}

void mln_alloc_destroy(mln_alloc_t *pool)
{
    if (pool == NULL) return;

    mln_alloc_mgr_t *am, *amend;
    amend = pool->mgr_tbl + M_ALLOC_MGR_LEN;
    mln_alloc_blk_t *ab;

    for (am = pool->mgr_tbl; am < amend; ++am) {
        while ((ab = am->free_head) != NULL) {
            mln_small_chain_del(&(am->free_head), &(am->free_tail), ab);
            free(ab);
        }

        while ((ab = am->used_head) != NULL) {
            mln_small_chain_del(&(am->used_head), &(am->used_tail), ab);
            free(ab);
        }
    }

    while ((ab = pool->large_used_head) != NULL) {
        mln_large_chain_del(&(pool->large_used_head), &(pool->large_used_tail), ab);
        free(ab);
    }

    free(pool);
}

void *mln_alloc_m(mln_alloc_t *pool, mln_size_t size)
{
    mln_alloc_blk_t *blk;
    mln_alloc_mgr_t *am;
    mln_size_t alloc_size, final_size, check_bits_len;

    am = mln_alloc_get_mgr_by_size(pool->mgr_tbl, size);
    final_size = am == NULL? size: am->blk_size;
    check_bits_len = sizeof(mln_alloc_check_bits) - 1;
    alloc_size = final_size + sizeof(mln_alloc_blk_t) + check_bits_len;

    if (am == NULL || am->free_head == NULL) {
        mln_u8ptr_t new_blk = (mln_u8ptr_t)malloc(alloc_size);
        if (new_blk == NULL) {
            if (am == NULL) return NULL;

            mln_alloc_mgr_t *amend = pool->mgr_tbl + M_ALLOC_MGR_LEN;
            for (; am < amend; ++am) {
                if (am->free_head != NULL) {
                    goto out;
                } 
            }
            return NULL;
        }
        blk = (mln_alloc_blk_t *)new_blk;
        blk->prev = blk->next = NULL;
        blk->data = new_blk + sizeof(mln_alloc_blk_t);
        blk->mgr = am;
        blk->pool = pool;
        blk->blk_size = final_size;
        blk->is_large = am == NULL? 1: 0;
        blk->in_used = 0;

        memcpy(new_blk+sizeof(mln_alloc_blk_t)+final_size, \
               mln_alloc_check_bits, \
               check_bits_len);

        if (am == NULL) {
            mln_large_chain_add(&(pool->large_used_head), &(pool->large_used_tail), blk);
            blk->in_used = 1;
            return blk->data;
        }

        pool->cur_size += alloc_size;
        if (!pool->threshold) {
            pool->threshold = pool->cur_size >> 1;
        } else {
            if (pool->cur_size > pool->threshold) {
                pool->threshold += (pool->threshold >> 1);
            } else {
                pool->threshold += (pool->threshold / \
                                    mln_alloc_calc_divisor(alloc_size, 2));
            }
        }

        mln_small_chain_add(&(am->free_head), &(am->free_tail), blk);
    }

out:
    blk = am->free_head;
    mln_small_chain_del(&(am->free_head), &(am->free_tail), blk);
    mln_small_chain_add(&(am->used_head), &(am->used_tail), blk);
    blk->in_used = 1;
    return blk->data;
}

static inline mln_size_t
mln_alloc_calc_divisor(mln_size_t size, mln_size_t start)
{
#if defined(i386) || defined(__x86_64)
    register mln_size_t off = 0;
    __asm__("bsr %1, %0":"=r"(off):"m"(size));
    ++off;
#else
    mln_size_t off = (sizeof(mln_size_t)<<3) - 1;
    while (off != 0) {
        if (((mln_size_t)1<<off) & size) break;
        --off;
    }
    ++off;
#endif
    return (sizeof(mln_size_t)<<3) - off + start - 1;
}

static inline mln_alloc_mgr_t *
mln_alloc_get_mgr_by_size(mln_alloc_mgr_t *tbl, mln_size_t size)
{
    if (size > tbl[M_ALLOC_MGR_LEN-1].blk_size)
        return NULL;

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
    if (off < M_ALLOC_BEGIN_OFF) return am;
    off = (off - M_ALLOC_BEGIN_OFF + 1) * M_ALLOC_MGR_GRAIN_SIZE;
    if (am[--off].blk_size < size) return &am[off+1];
    return &am[off];
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

    mln_alloc_blk_t *blk = (mln_alloc_blk_t *)((mln_u8ptr_t)ptr - sizeof(mln_alloc_blk_t));
    if (!blk->in_used) {
        mln_log(error, "Double free.\n");
        abort();
    }

    if (memcmp(ptr + blk->blk_size, mln_alloc_check_bits, sizeof(mln_alloc_check_bits)-1)) {
        mln_log(error, "Buffer overflow.\n");
        abort();
    }

    mln_alloc_t *pool = blk->pool;

    if (blk->is_large) {
        mln_large_chain_del(&(pool->large_used_head), &(pool->large_used_tail), blk);
        free(blk);
        return;
    }

    mln_alloc_mgr_t *am = blk->mgr;

    mln_small_chain_del(&(am->used_head), &(am->used_tail), blk);
    mln_small_chain_add(&(am->free_head), &(am->free_tail), blk);
    blk->in_used = 0;

    mln_size_t thr = pool->threshold, alloc_size;
    alloc_size = am->blk_size+sizeof(mln_alloc_blk_t)+sizeof(mln_alloc_check_bits)-1;
    pool->threshold = (thr + (thr - thr / mln_alloc_calc_divisor(alloc_size, 4096))) >> 1;

    mln_alloc_reduce(pool);
}

static inline void
mln_alloc_reduce(mln_alloc_t *pool)
{
    mln_alloc_blk_t *blk = NULL;
    mln_alloc_mgr_t *am = &pool->mgr_tbl[M_ALLOC_MGR_LEN-1], *ambeg;

    for (ambeg = pool->mgr_tbl; am >= ambeg; --am) {
        while ((blk = am->free_head) != NULL) {
            if (pool->threshold >= pool->cur_size) {
                return;
            }

            mln_small_chain_del(&(am->free_head), &(am->free_tail), blk);
            free(blk);

            pool->cur_size -= (am->blk_size + sizeof(mln_alloc_blk_t) + sizeof(mln_alloc_check_bits) - 1);
        }
    }
}

/*
 * chain
 */
MLN_CHAIN_FUNC_DEFINE(mln_small, \
                      mln_alloc_blk_t, \
                      static inline void, \
                      prev, \
                      next);

MLN_CHAIN_FUNC_DEFINE(mln_large, \
                      mln_alloc_blk_t, \
                      static inline void, \
                      prev, \
                      next);

