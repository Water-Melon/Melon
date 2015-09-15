
/*
 * Copyright (C) Niklaus F.Schen.
 */

#ifndef __MLN_ALLOC_H
#define __MLN_ALLOC_H

#include "mln_types.h"
#include "mln_rbtree.h"

#define M_ALLOC_BEGIN_OFF      ((mln_size_t)4)
#define M_ALLOC_MGR_GRAIN_SIZE 2
#define M_ALLOC_MGR_LEN        18*M_ALLOC_MGR_GRAIN_SIZE-(M_ALLOC_MGR_GRAIN_SIZE-1)

typedef struct mln_alloc_s     mln_alloc_t;
typedef struct mln_alloc_mgr_s mln_alloc_mgr_t;

typedef struct mln_alloc_blk_s {
    struct mln_alloc_blk_s *prev;
    struct mln_alloc_blk_s *next;
    void                   *data;
    mln_alloc_mgr_t        *mgr;
    mln_alloc_t            *pool;
    mln_size_t              blk_size;
    mln_size_t              is_large:1;
    mln_size_t              in_used:1;
} mln_alloc_blk_t __cacheline_aligned;

struct mln_alloc_mgr_s {
    mln_size_t              blk_size;
    mln_alloc_blk_t        *free_head;
    mln_alloc_blk_t        *free_tail;
    mln_alloc_blk_t        *used_head;
    mln_alloc_blk_t        *used_tail;
};

struct mln_alloc_s {
    mln_alloc_mgr_t         mgr_tbl[M_ALLOC_MGR_LEN];
    mln_alloc_blk_t        *large_used_head;
    mln_alloc_blk_t        *large_used_tail;
    mln_size_t              cur_size;
    mln_size_t              threshold;
};


extern mln_alloc_t *mln_alloc_init(void);
extern void mln_alloc_destroy(mln_alloc_t *pool);
extern void *mln_alloc_m(mln_alloc_t *pool, mln_size_t size);
extern void *mln_alloc_c(mln_alloc_t *pool, mln_size_t size);
extern void *mln_alloc_re(mln_alloc_t *pool, void *ptr, mln_size_t size);
extern void mln_alloc_free(void *ptr);

#endif

