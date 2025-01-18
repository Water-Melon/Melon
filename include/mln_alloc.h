
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
#define M_ALLOC_INFINITE_SIZE    (~((mln_size_t)0))

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
    mln_size_t                size;
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
    mln_size_t                capacity;
    mln_size_t                in_used;
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


mln_alloc_t *mln_alloc_shm_init(mln_size_t capacity, void *locker, mln_alloc_shm_lock_cb_t lock, mln_alloc_shm_lock_cb_t unlock);
mln_alloc_t *mln_alloc_init(mln_alloc_t *parent, mln_size_t capacity);
void mln_alloc_destroy(mln_alloc_t *pool);
void *mln_alloc_m(mln_alloc_t *pool, mln_size_t size);
void *mln_alloc_c(mln_alloc_t *pool, mln_size_t size);
void *mln_alloc_re(mln_alloc_t *pool, void *ptr, mln_size_t size);
void mln_alloc_free(void *ptr);
mln_size_t mln_alloc_available_capacity(mln_alloc_t *pool);

#endif

