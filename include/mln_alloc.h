
/*
 * Copyright (C) Niklaus F.Schen.
 */

#ifndef __MLN_ALLOC_H
#define __MLN_ALLOC_H

#include "mln_types.h"

#define M_ALLOC_BEGIN_OFF      ((mln_size_t)4)
#define M_ALLOC_MGR_GRAIN_SIZE 2
#define M_ALLOC_MGR_LEN        18*M_ALLOC_MGR_GRAIN_SIZE-(M_ALLOC_MGR_GRAIN_SIZE-1)
#define M_ALLOC_BLK_NUM        8

typedef struct mln_alloc_s       mln_alloc_t;
typedef struct mln_alloc_mgr_s   mln_alloc_mgr_t;
typedef struct mln_alloc_chunk_s mln_alloc_chunk_t;

/*
 * Notice:
 * In Darwin, if set mln_alloc_blk_t's prev and next at the beginning of the structure,
 * program will encounter segmentation fault.
 * It seems that we can not set bit variables those summary not enough aligned bytes
 * at the end of structure.
 * But in Linux, no such kind of problem.
 */
typedef struct mln_alloc_blk_s {
    void                     *data;
    mln_alloc_chunk_t        *chunk;
    mln_alloc_t              *pool;
    mln_size_t                blk_size;
    mln_size_t                is_large:1;
    mln_size_t                in_used:1;
    struct mln_alloc_blk_s   *prev;
    struct mln_alloc_blk_s   *next;
    struct mln_alloc_blk_s   *chunk_prev;
    struct mln_alloc_blk_s   *chunk_next;
} mln_alloc_blk_t __cacheline_aligned;

struct mln_alloc_chunk_s {
    struct mln_alloc_chunk_s *prev;
    struct mln_alloc_chunk_s *next;
    mln_size_t                refer;
    mln_alloc_mgr_t          *mgr;
    mln_alloc_blk_t          *head;
    mln_alloc_blk_t          *tail;
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

struct mln_alloc_s {
    mln_alloc_mgr_t           mgr_tbl[M_ALLOC_MGR_LEN];
    mln_alloc_chunk_t        *large_used_head;
    mln_alloc_chunk_t        *large_used_tail;
};


extern mln_alloc_t *mln_alloc_init(void);
extern void mln_alloc_destroy(mln_alloc_t *pool);
extern void *mln_alloc_m(mln_alloc_t *pool, mln_size_t size);
extern void *mln_alloc_c(mln_alloc_t *pool, mln_size_t size);
extern void *mln_alloc_re(mln_alloc_t *pool, void *ptr, mln_size_t size);
extern void mln_alloc_free(void *ptr);

#endif

