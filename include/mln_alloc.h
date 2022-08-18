
/*
 * Copyright (C) Niklaus F.Schen.
 */

#ifndef __MLN_ALLOC_H
#define __MLN_ALLOC_H

#if defined(WIN32)
#include <windows.h>
#include <winbase.h>
#else
#include <sys/mman.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mln_types.h"

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

struct mln_alloc_shm_attr_s {
    mln_size_t                size;
    void                     *locker;
    mln_alloc_shm_lock_cb_t   lock;
    mln_alloc_shm_lock_cb_t   unlock;
};

/*
 * Note:
 * In Darwin, if set mln_alloc_blk_t's prev and next at the beginning of the structure,
 * program will encounter segmentation fault.
 * It seems that we can not set bit variables those summary not enough aligned bytes
 * at the end of structure.
 * But in Linux, no such kind of problem.
 */
typedef struct mln_alloc_blk_s {
    mln_alloc_t              *pool;
    void                     *data;
    mln_alloc_chunk_t        *chunk;
    mln_size_t                blk_size;
    mln_size_t                is_large:1;
    mln_size_t                in_used:1;
    mln_size_t                padding:30;
    struct mln_alloc_blk_s   *prev;
    struct mln_alloc_blk_s   *next;
} mln_alloc_blk_t __cacheline_aligned;

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
    mln_alloc_t              *pool;
    void                     *addr;
    mln_size_t                size;
    mln_u32_t                 nfree;
    mln_u32_t                 base:31;
    mln_u32_t                 large:1;
    mln_u8_t                  bitmap[M_ALLOC_SHM_BITMAP_LEN];
    struct mln_alloc_shm_s   *prev;
    struct mln_alloc_shm_s   *next;
} mln_alloc_shm_t;

struct mln_alloc_s {
    mln_alloc_mgr_t           mgr_tbl[M_ALLOC_MGR_LEN];
    struct mln_alloc_s       *parent;
    mln_alloc_chunk_t        *large_used_head;
    mln_alloc_chunk_t        *large_used_tail;
    mln_alloc_shm_t          *shm_head;
    mln_alloc_shm_t          *shm_tail;
    void                     *mem;
    mln_size_t                shm_size;
    void                     *locker;
    mln_alloc_shm_lock_cb_t   lock;
    mln_alloc_shm_lock_cb_t   unlock;
#if defined(WIN32)
    HANDLE                    map_handle;
#endif
};


#define mln_alloc_is_shm(pool) (pool->mem != NULL)

extern mln_alloc_t *mln_alloc_shm_init(struct mln_alloc_shm_attr_s *attr);
extern mln_alloc_t *mln_alloc_init(mln_alloc_t *parent);
extern void mln_alloc_destroy(mln_alloc_t *pool);
extern void *mln_alloc_m(mln_alloc_t *pool, mln_size_t size);
extern void *mln_alloc_c(mln_alloc_t *pool, mln_size_t size);
extern void *mln_alloc_re(mln_alloc_t *pool, void *ptr, mln_size_t size);
extern void mln_alloc_free(void *ptr);

#endif

