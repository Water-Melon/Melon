
/*
 * Copyright (C) Niklaus F.Schen.
 */

#ifndef __MLN_SALLOC_H
#define __MLN_SALLOC_H

#include "mln_sa_rbtree.h"
#include "mln_types.h"

#define M_SA_4K 0x1000
#define M_SA_64K 0x10000
#define M_SA_1M 0x100000
#define M_SA_2M 0x200000
#define M_SA_4M 0x400000
#define M_INDEX_OFF ((mln_size_t)4)
#define M_GRAIN_SIZE 4
#define M_INDEX_LEN 18*M_GRAIN_SIZE-3
#define M_DFL_PAGESIZE 4096
#define M_CHKBITS_LEN 16
#define M_SIZE_BLOCK (sizeof(mln_blk_t)+M_CHKBITS_LEN)
#define M_SIZE_CHUNK (sizeof(mln_chunk_t)+M_SIZE_BLOCK)
#define M_SIZE_CHUNK_HEAD sizeof(mln_chunk_t)
#define M_DFL_ROLLBACK_CNT 4

typedef struct mln_salloc_s mln_salloc_t;
typedef struct mln_blk_index_s mln_blk_index_t;
typedef struct mln_chunk_s mln_chunk_t;

typedef struct mln_blk_s {
    mln_chunk_t           *chunk;
    mln_salloc_t          *salloc;
    mln_blk_index_t       *index;
    struct mln_blk_s      *index_prev;
    struct mln_blk_s      *index_next;
    struct mln_blk_s      *chunk_prev;
    struct mln_blk_s      *chunk_next;
    mln_uauto_t            in_used;
    mln_size_t             blk_size;
} mln_blk_t;

struct mln_blk_index_s {
    mln_size_t             alloc_size;
    mln_size_t             blk_size;
    mln_blk_t             *free_head;
    mln_blk_t             *free_tail;
    mln_blk_t             *used_head;
    mln_blk_t             *used_tail;
    mln_uauto_t            idle_start_sec;
    mln_uauto_t            alloc_cnt;
}__cacheline_aligned;

struct mln_chunk_s {
    mln_size_t             chunk_size;
    mln_sarbt_node_t       large_node;
    struct mln_chunk_s    *prev;
    struct mln_chunk_s    *next;
    mln_blk_t             *blk_head;
    mln_blk_t             *blk_tail;
    mln_u32_t              is_large:1;
    mln_u32_t              padding:31;
}__cacheline_aligned;

struct mln_salloc_s {
    mln_blk_index_t        index_tbl[M_INDEX_LEN];
    mln_size_t             current_size;
    mln_size_t             threshold;
    mln_lock_t             index_lock;
    mln_lock_t             stat_lock;
    pthread_key_t          key;
};

extern void *malloc(size_t size);
extern void *calloc(size_t nmemb, size_t size);
extern void *realloc(void *ptr, size_t size);
extern void free(void *ptr);
extern void mdump(void);

#endif

