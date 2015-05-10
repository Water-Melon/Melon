
/*
 * Copyright (C) Niklaus F.Schen.
 */
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <sys/time.h>
#include "mln_log.h"
#include "mln_salloc.h"

/*
 * global variables
 */
mln_salloc_t *mAllocator = NULL;
mln_chunk_t *mHeapBase = NULL;
mln_chunk_t *mHeapTop = NULL;
mln_chunk_t *mMmapHead = NULL;
mln_chunk_t *mMmapTail = NULL;
mln_lock_t mHeapLock;
mln_lock_t mLargeTreeLock;
mln_lock_t mMmapLock;
mln_sarbt_t mLargeTree;
long mPageSize = 0;
char mln_check_bits[M_CHKBITS_LEN+1] = {
  0x4d, 0x45, 0x4c, 0x4f,
  0x4e, 0x53, 0x68, 0x65,
  0x6e, 0x46, 0x61, 0x6e,
  0x63, 0x68, 0x65, 0x6e
};
/*
 * thread variables
 */
__thread mln_salloc_t *tAllocator = NULL;

/*
 * declarations
 */
MLN_CHAIN_FUNC_DECLARE(mgr_q, \
                       mln_chunk_t, \
                       static inline void, \
                       __NONNULL3(1,2,3));
MLN_CHAIN_FUNC_DECLARE(chunk_q, \
                       mln_blk_t, \
                       static inline void, \
                       __NONNULL3(1,2,3));
MLN_CHAIN_FUNC_DECLARE(index_q, \
                       mln_blk_t, \
                       static inline void, \
                       __NONNULL3(1,2,3));
static int mln_salloc_init(void);
static inline int
mln_Allocator_init(mln_salloc_t **sa);
static inline void
mln_index_table_init(mln_blk_index_t *tbl);
static int
mln_large_tree_cmp(const void *k1, const void *k2);
static inline mln_blk_t *
mln_alloc_chunk(mln_size_t size);
static inline mln_blk_t *
mln_chunk_heap(mln_size_t chunk_size, mln_size_t size, int is_large);
static inline mln_blk_t *
mln_chunk_mmap(mln_size_t chunk_size, mln_size_t size, int is_large);
static inline mln_blk_t *
mln_calc_slice(mln_chunk_t *chunk, mln_size_t size, int is_large, int is_heap);
static inline mln_size_t
mln_calc_divisor(mln_size_t size, mln_size_t start);
static inline mln_blk_index_t *
mln_get_index_entry(mln_blk_index_t *tbl, mln_size_t size);
static inline void *
mln_alloc(mln_size_t size);
static void *
mln_alloc_failed(mln_blk_index_t *bi, mln_size_t size);
static inline mln_blk_t *
mln_alloc_large(mln_size_t size, int small2large);
static inline mln_blk_t *
mln_search_mAllocator(int index);
static inline mln_blk_t *mln_search_index(int index);
static inline mln_blk_t *
mln_search_index_main(int index);
static mln_blk_t *mln_merge(int index);
static void dump_salloc(mln_salloc_t *sa);
static void mln_salloc_reduce_thread(mln_salloc_t *sa);
static void mln_salloc_reduce_main(void);
static void mln_move(void *arg);
static void mln_salloc_destroy(mln_salloc_t *sa);

/*
 * Init
 */
static int mln_salloc_init(void)
{
    if (mln_Allocator_init(&mAllocator) < 0)
        return -1;
    return mln_Allocator_init(&tAllocator);
}

static inline int
mln_Allocator_init(mln_salloc_t **sa)
{
    if (*sa != NULL) return 0;
    int err = 0;
#ifdef MAP_ANONYMOUS
    *sa = (mln_salloc_t *)mmap(NULL, \
                               sizeof(mln_salloc_t), \
                               PROT_READ|PROT_WRITE, \
                               MAP_PRIVATE|MAP_ANONYMOUS, \
                               -1, 0);
#else
    *sa = (mln_salloc_t *)mmap(NULL, \
                               sizeof(mln_salloc_t), \
                               PROT_READ|PROT_WRITE, \
                               MAP_PRIVATE|MAP_ANON, \
                               -1, 0);
#endif
    if (*sa == MAP_FAILED) {
        mln_log(error, "Allocator init failed. %s\n", strerror(errno));
        return -1;
    }

    mln_index_table_init((*sa)->index_tbl);
    (*sa)->current_size = 0;
    (*sa)->threshold = 0;
    err = MLN_LOCK_INIT(&((*sa)->index_lock));
    if (err != 0) {
        mln_log(error, "index_lock init error. %s\n", strerror(err));
        goto err1;
    }
    err = MLN_LOCK_INIT(&((*sa)->stat_lock));
    if (err != 0) {
        mln_log(error, "stat_lock init error. %s\n", strerror(err));
        goto err2;
    }

    if (sa != &mAllocator) {
        if ((err = pthread_key_create(&((*sa)->key), mln_move)) != 0) {
            mln_log(error, "pthread_key_create error. %s\n", strerror(err));
            goto err3;
        }

        if ((err = pthread_setspecific((*sa)->key, (*sa))) != 0) {
            mln_log(error, "pthread_setspecific error. %s\n", strerror(err));
            pthread_key_delete((*sa)->key);
            goto err3;
        }
        return 0;
    }
    mPageSize = sysconf(_SC_PAGESIZE);
    if (mPageSize < 0) {
        mPageSize = M_DFL_PAGESIZE;
    }
    
    mLargeTree.nil.data = NULL;
    mLargeTree.nil.parent = &(mLargeTree.nil);
    mLargeTree.nil.left = &(mLargeTree.nil);
    mLargeTree.nil.right = &(mLargeTree.nil);
    mLargeTree.nil.color = M_SARB_BLACK;
    mLargeTree.root = &(mLargeTree.nil);
    mLargeTree.cmp = mln_large_tree_cmp;

    err = MLN_LOCK_INIT(&mHeapLock);
    if (err != 0) {
        mln_log(error, "mHeapLock init error. %s\n", strerror(err));
        goto err3;
    }
    err = MLN_LOCK_INIT(&mLargeTreeLock);
    if (err != 0) {
        mln_log(error, "mLargeTreeLock init error. %s\n", strerror(err));
        goto err4;
    }
    err = MLN_LOCK_INIT(&mMmapLock);
    if (err != 0) {
        mln_log(error, "mMmapLock init error. %s\n", strerror(err));
        goto err5;
    }
    return 0;

err5:
    MLN_LOCK_DESTROY(&mLargeTreeLock);
err4:
    MLN_LOCK_DESTROY(&mHeapLock);
err3:
    MLN_LOCK_DESTROY(&((*sa)->stat_lock));
err2:
    MLN_LOCK_DESTROY(&((*sa)->index_lock));
err1:
    munmap((*sa), sizeof(mln_salloc_t));
    *sa = NULL;
    return -1;
}

static int
mln_large_tree_cmp(const void *k1, const void *k2)
{
    mln_chunk_t *c1 = (mln_chunk_t *)k1;
    mln_chunk_t *c2 = (mln_chunk_t *)k2;
    if (c1->chunk_size > c2->chunk_size) return 1;
    if (c1->chunk_size < c2->chunk_size) return -1;
    return 0;
}

#define MLN_SA_INDEX_ASSIGN(index,blksize,sec); \
    if ((blksize) + M_SIZE_CHUNK < M_SA_4K) \
        tbl[(index)].alloc_size = M_SA_4K;\
    else if ((blksize) + M_SIZE_CHUNK < M_SA_64K) \
        tbl[(index)].alloc_size = M_SA_64K;\
    else if ((blksize) + M_SIZE_CHUNK < M_SA_1M) \
        tbl[(index)].alloc_size = M_SA_1M;\
    else if ((blksize) + M_SIZE_CHUNK < M_SA_2M) \
        tbl[(index)].alloc_size = M_SA_2M;\
    else \
        tbl[(index)].alloc_size = M_SA_4M;\
    tbl[(index)].blk_size = (blksize);\
    tbl[(index)].free_head = NULL;\
    tbl[(index)].free_tail = NULL;\
    tbl[(index)].used_head = NULL;\
    tbl[(index)].used_tail = NULL;\
    tbl[(index)].idle_start_sec = (sec);\
    tbl[(index)].alloc_cnt = 0;

static inline void
mln_index_table_init(mln_blk_index_t *tbl)
{
    int i, j;
    mln_size_t blk_size;
    struct timeval now;
    gettimeofday(&now, NULL);
    for (i = 0; i < M_INDEX_LEN; i += M_GRAIN_SIZE) {
        blk_size = 0;
        for (j = 0; j < i/M_GRAIN_SIZE + M_INDEX_OFF; j++)
            blk_size |= (((mln_size_t)1) << j);
        MLN_SA_INDEX_ASSIGN(i, blk_size, now.tv_sec);
        if (i != 0) {
            blk_size = (tbl[i].blk_size+tbl[i-4].blk_size)>>1;
            MLN_SA_INDEX_ASSIGN(i-2, blk_size, now.tv_sec);
            blk_size = (tbl[i].blk_size+tbl[i-2].blk_size)>>1;
            MLN_SA_INDEX_ASSIGN(i-1, blk_size, now.tv_sec);
            blk_size = (tbl[i-2].blk_size+tbl[i-4].blk_size)>>1;
            MLN_SA_INDEX_ASSIGN(i-3, blk_size, now.tv_sec);
        }
    }
}

/*
 * tool functions
 */
static inline mln_size_t
mln_calc_divisor(mln_size_t size, mln_size_t start)
{
#if defined(i386) || defined(__x86_64)
    register mln_size_t off = 0;
    __asm__("bsr %1, %0":"=r"(off):"m"(size));
    off++;
#else
    mln_size_t off = sizeof(mln_size_t)*8-1;
    while (off != 0 ) {
        if (((mln_size_t)1<<off) & size) break;
        off--;
    }
    off++;
#endif
    return sizeof(mln_size_t)*8 - off + start - 1;
}

static inline mln_blk_index_t *
mln_get_index_entry(mln_blk_index_t *tbl, mln_size_t size)
{
    if (size > tbl[M_INDEX_LEN-1].blk_size) return NULL;
    mln_blk_index_t *bi = tbl;
#if defined(i386) || defined(__x86_64)
    register mln_size_t off = 0;
    __asm__("bsr %1, %0":"=r"(off):"m"(size));
#else
    mln_size_t off = 0;
    int i;
    for (i = (sizeof(mln_size_t)<<3) - 1; i >= 0; i--) {
        if (size & (((mln_size_t)1) << i)) {
            off = i;
            break;
        }
    }
#endif
    if (off < M_INDEX_OFF) return bi;
    off = (off - M_INDEX_OFF + 1)*M_GRAIN_SIZE;
    if (bi[--off].blk_size < size) return &bi[off+1];
    if (bi[--off].blk_size < size) return &bi[off+1];
    if (bi[--off].blk_size < size) return &bi[off+1];
    return &bi[off];
}


/*
 * chunk allocation
 */
static inline mln_blk_t *
mln_alloc_chunk(mln_size_t size)
{
    mln_blk_t *blk = NULL;
    mln_blk_index_t *bi = mln_get_index_entry(mAllocator->index_tbl, size);
    if (bi == NULL) {
        mln_size_t alloc_size = size + M_SIZE_CHUNK;
        if (alloc_size % mPageSize)
            alloc_size += (mPageSize - (alloc_size % mPageSize));
        if ((blk = mln_chunk_mmap(alloc_size, size, 1)) == NULL)
            return mln_chunk_heap(alloc_size, size, 1);
        return blk;
    }
    if (bi->alloc_size <= M_SA_64K) {
        if ((blk = mln_chunk_heap(bi->alloc_size, size, 0)) == NULL)
            return mln_chunk_mmap(bi->alloc_size, size, 0);
        return blk;
    }
    if ((blk = mln_chunk_mmap(bi->alloc_size, size, 0)) == NULL)
        return mln_chunk_heap(bi->alloc_size, size, 0);
    return blk;
}

#define MLN_CHUNK_INIT(pchunk,cSize,isLarge); \
    (pchunk)->chunk_size = (cSize);\
    (pchunk)->large_node.data = (pchunk);\
    (pchunk)->large_node.parent = &(mLargeTree.nil);\
    (pchunk)->large_node.left = &(mLargeTree.nil);\
    (pchunk)->large_node.right = &(mLargeTree.nil);\
    (pchunk)->prev = NULL;\
    (pchunk)->next = NULL;\
    (pchunk)->blk_head = NULL;\
    (pchunk)->blk_tail = NULL;\
    (pchunk)->is_large = (isLarge);

static inline mln_blk_t *
mln_chunk_heap(mln_size_t chunk_size, mln_size_t size, int is_large)
{
    MLN_LOCK(&mHeapLock);
    mln_s8ptr_t ptr = (mln_s8ptr_t)sbrk(chunk_size);
    if (ptr == (void *) -1) {
        MLN_UNLOCK(&mHeapLock);
        return NULL;
    }
    mln_chunk_t *chunk = (mln_chunk_t *)ptr;
    MLN_CHUNK_INIT(chunk, chunk_size, is_large);
    MLN_UNLOCK(&mHeapLock);

    MLN_LOCK(&(mAllocator->stat_lock));
    mAllocator->current_size += chunk_size;
    if (!mAllocator->threshold) {
        mAllocator->threshold = mAllocator->current_size >> 1;
    } else {
        if (mAllocator->current_size > mAllocator->threshold)
            mAllocator->threshold += (mAllocator->threshold >> 1);
        else
            mAllocator->threshold += (mAllocator->threshold / \
                                      mln_calc_divisor(chunk_size, 2));
    }
    MLN_UNLOCK(&(mAllocator->stat_lock));

    return mln_calc_slice(chunk, size, is_large, 1);
}

static inline mln_blk_t *
mln_chunk_mmap(mln_size_t chunk_size, mln_size_t size, int is_large)
{
    mln_s8ptr_t ptr;
#ifdef MAP_ANONYMOUS
    ptr = (mln_s8ptr_t)mmap(NULL, \
                            chunk_size, \
                            PROT_READ|PROT_WRITE, \
                            MAP_PRIVATE|MAP_ANONYMOUS, \
                            -1, 0);
#else
    ptr = (mln_s8ptr_t)mmap(NULL, \
                            chunk_size, \
                            PROT_READ|PROT_WRITE, \
                            MAP_PRIVATE|MAP_ANON, \
                            -1, 0);
#endif
    if (ptr == MAP_FAILED) return NULL;

    mln_chunk_t *chunk = (mln_chunk_t *)ptr;
    MLN_CHUNK_INIT(chunk, chunk_size, is_large);

    MLN_LOCK(&(mAllocator->stat_lock));
    mAllocator->current_size += chunk_size;
    if (!mAllocator->threshold) {
        mAllocator->threshold = mAllocator->current_size >> 1;
    } else {
        if (mAllocator->current_size > mAllocator->threshold)
            mAllocator->threshold += (mAllocator->threshold >> 1);
        else
            mAllocator->threshold += (mAllocator->threshold / \
                                      mln_calc_divisor(chunk_size, 2));
    }
    MLN_UNLOCK(&(mAllocator->stat_lock));

    return mln_calc_slice(chunk, size, is_large, 0);
}

#define MLN_BLOCK_INIT(pblk,psa,pchunk,pindex,bsize); \
    (pblk)->chunk = (pchunk);\
    (pblk)->salloc = (psa);\
    (pblk)->index = (pindex);\
    (pblk)->index_prev = NULL;\
    (pblk)->index_next = NULL;\
    (pblk)->chunk_prev = NULL;\
    (pblk)->chunk_next = NULL;\
    (pblk)->in_used = 0;\
    (pblk)->blk_size = (bsize);

static inline mln_blk_t *
mln_calc_slice(mln_chunk_t *chunk, mln_size_t size, int is_large, int is_heap)
{
    struct timeval now;
    mln_size_t left_size = chunk->chunk_size;
    register mln_size_t tmp_size;
    mln_u8ptr_t ptr;
    mln_blk_t *blk;
    mln_blk_index_t *bi = NULL;
    if (is_large) {
        ptr = (mln_u8ptr_t)chunk;
        memcpy(ptr+chunk->chunk_size-M_CHKBITS_LEN, mln_check_bits, M_CHKBITS_LEN);
        ptr += M_SIZE_CHUNK_HEAD;
        blk = (mln_blk_t *)ptr;
        MLN_BLOCK_INIT(blk, mAllocator, chunk, NULL, chunk->chunk_size-M_SIZE_CHUNK);
        blk->in_used = 1;
        chunk_q_chain_add(&(chunk->blk_head), &(chunk->blk_tail), blk);
        goto larg;
    }
    gettimeofday(&now, NULL);
    bi = mln_get_index_entry(mAllocator->index_tbl, size);
    tmp_size = bi->blk_size + M_SIZE_BLOCK;
    ptr = (mln_u8ptr_t)chunk + M_SIZE_CHUNK_HEAD;
    memcpy(ptr+sizeof(mln_blk_t)+bi->blk_size, mln_check_bits, M_CHKBITS_LEN);
    blk = (mln_blk_t *)ptr;
    ptr += tmp_size;
    left_size -= (M_SIZE_CHUNK_HEAD + tmp_size);

    mln_blk_index_t *tbi = &(tAllocator->index_tbl[bi-mAllocator->index_tbl]);
    MLN_BLOCK_INIT(blk, tAllocator, chunk, tbi, tbi->blk_size);
    blk->in_used = 1;
    chunk_q_chain_add(&(chunk->blk_head), &(chunk->blk_tail), blk);
    MLN_LOCK(&(tAllocator->index_lock));
    index_q_chain_add(&(tbi->used_head), &(tbi->used_tail), blk);
    MLN_UNLOCK(&(tAllocator->index_lock));
    MLN_LOCK(&(mAllocator->index_lock));
    bi->idle_start_sec = now.tv_sec;
    bi->alloc_cnt = bi->alloc_cnt>=M_DFL_ROLLBACK_CNT?0:bi->alloc_cnt+2;

    mln_uauto_t max = 0, tmp = 0;
    mln_blk_index_t *save = NULL, *index = NULL;
    mln_blk_t *small_blk = NULL;
    while (left_size > 0) {
        if (left_size <= M_SIZE_BLOCK) break;
        tmp_size = left_size - M_SIZE_BLOCK;
        index = mln_get_index_entry(mAllocator->index_tbl, tmp_size);
        while (index->blk_size > tmp_size) {
            if (index == mAllocator->index_tbl) break;
            index--;
        }
        if (index->blk_size > tmp_size) break;

        max = 0;
        save = NULL;
        for (; index >= mAllocator->index_tbl; index--) {
            if (index->alloc_cnt) {
                index->alloc_cnt--;
                if (now.tv_sec > index->idle_start_sec)
                    tmp = index->alloc_cnt/(now.tv_sec - index->idle_start_sec);
                else if (now.tv_sec == index->idle_start_sec)
                    tmp = index->alloc_cnt;
                else tmp = 0;
            } else {
                tmp = 0;
            }
            if (max < tmp || save == NULL) {
                max = tmp;
                save = index;
            }
        }

        memcpy(ptr+sizeof(mln_blk_t)+save->blk_size, mln_check_bits, M_CHKBITS_LEN);
        small_blk = (mln_blk_t *)ptr;
        tmp_size = save->blk_size + M_SIZE_BLOCK;
        ptr += tmp_size;
        left_size -= tmp_size;
        MLN_BLOCK_INIT(small_blk, mAllocator, chunk, save, save->blk_size);
        index_q_chain_add(&(save->free_head), &(save->free_tail), small_blk);
        chunk_q_chain_add(&(chunk->blk_head), &(chunk->blk_tail), small_blk);
    }
    MLN_UNLOCK(&(mAllocator->index_lock));

larg:
    if (is_heap) {
        MLN_LOCK(&mHeapLock);
        mgr_q_chain_add(&mHeapBase, &mHeapTop, chunk);
        MLN_UNLOCK(&mHeapLock);
    } else {
        MLN_LOCK(&mMmapLock);
        mgr_q_chain_add(&mMmapHead, &mMmapTail, chunk);
        MLN_UNLOCK(&mMmapLock);
    }
    return blk;
}

static inline void *
mln_alloc(mln_size_t size)
{
    if (!size) return NULL;
    if (mln_salloc_init() < 0) return NULL;
    mln_blk_index_t *bi = mln_get_index_entry(tAllocator->index_tbl, size);
    mln_blk_t *blk = NULL;
    if (bi == NULL) {
        blk = mln_alloc_large(size, 0);
        return ((mln_u8ptr_t)blk)+sizeof(mln_blk_t);
    }
    MLN_LOCK(&(tAllocator->index_lock));
    if (bi->free_head == NULL) {
        MLN_UNLOCK(&(tAllocator->index_lock));
        return mln_alloc_failed(bi, size);
    }
    blk = bi->free_head;
    index_q_chain_del(&(bi->free_head), &(bi->free_tail), blk);
    index_q_chain_add(&(bi->used_head), &(bi->used_tail), blk);
    blk->in_used = 1;
    MLN_UNLOCK(&(tAllocator->index_lock));

    return ((mln_u8ptr_t)blk)+sizeof(mln_blk_t);
}

static void *
mln_alloc_failed(mln_blk_index_t *bi, mln_size_t size)
{
    int index = bi - tAllocator->index_tbl;
    mln_blk_t *blk;
    blk = mln_search_mAllocator(index);
    if (blk != NULL) goto out;
    blk = mln_alloc_chunk(size);
    if (blk != NULL) goto out;
    blk = mln_search_index(index);
    if (blk != NULL)
        return ((mln_u8ptr_t)blk)+sizeof(mln_blk_t);
    blk = mln_search_index_main(index);
    if (blk != NULL) goto out;
    blk = mln_merge(index);
    if (blk != NULL) goto out;
    blk = mln_alloc_large(size, 1);
    if (blk != NULL)
        return ((mln_u8ptr_t)blk)+sizeof(mln_blk_t);
    return NULL;

out:
    MLN_LOCK(&(tAllocator->stat_lock));
    tAllocator->current_size += blk->index->blk_size;
    if (!tAllocator->threshold) {
        tAllocator->threshold = tAllocator->current_size >> 1;
    } else {
        if (tAllocator->current_size > tAllocator->threshold)
            tAllocator->threshold += (tAllocator->threshold >> 1);
        else
            tAllocator->threshold += (tAllocator->threshold / \
                                      mln_calc_divisor(bi->blk_size, 2));
    }
    MLN_UNLOCK(&(tAllocator->stat_lock));
    return ((mln_u8ptr_t)blk)+sizeof(mln_blk_t);
}

static inline mln_blk_t *
mln_alloc_large(mln_size_t size, int small2large)
{
    mln_chunk_t *chunk = NULL, tmp_chunk;
    mln_blk_t *blk = NULL;
    mln_size_t require_size;
lp:
    require_size = size + M_SIZE_CHUNK;
    if (require_size % mPageSize)
        require_size += (mPageSize - (require_size % mPageSize));
    tmp_chunk.chunk_size = require_size;
    MLN_LOCK(&mLargeTreeLock);
    chunk = (mln_chunk_t *)mln_sarbt_search(&mLargeTree, &tmp_chunk);
    if (chunk != NULL) {
        if (small2large || \
            chunk->chunk_size <= (require_size<<1))
        {
            mln_sarbt_delete(&mLargeTree, &(chunk->large_node));
            blk = chunk->blk_head;
            blk->in_used = 1;
            MLN_UNLOCK(&mLargeTreeLock);
            return blk;
        }
    }
    MLN_UNLOCK(&mLargeTreeLock);
    if (!small2large) {
        blk = mln_alloc_chunk(size);
        if (blk != NULL) return blk;
        if (chunk != NULL) {
            small2large = 1;
            goto lp;
        }
    }
    return NULL;
}

static inline mln_blk_t *
mln_search_mAllocator(int index)
{
    mln_blk_t *blk = NULL;
    mln_blk_index_t *bi = &(mAllocator->index_tbl[index]);
    mln_blk_index_t *tbi = &(tAllocator->index_tbl[index]);
    MLN_LOCK(&(mAllocator->index_lock));
    if ((blk = bi->free_head) != NULL) {
        index_q_chain_del(&(bi->free_head), &(bi->free_tail), blk);
        blk->salloc = tAllocator;
        blk->index = tbi;
        blk->in_used = 1;
        MLN_UNLOCK(&(mAllocator->index_lock));
        MLN_LOCK(&(tAllocator->index_lock));
        index_q_chain_add(&(tbi->used_head), &(tbi->used_tail), blk);
        MLN_UNLOCK(&(tAllocator->index_lock));
        return blk;
    }
    MLN_UNLOCK(&(mAllocator->index_lock));
    return NULL;
}

static inline mln_blk_t *mln_search_index(int index)
{
    mln_blk_t *blk = NULL;
    mln_blk_index_t *bi = NULL;
    MLN_LOCK(&(tAllocator->index_lock));
    for (index++; index < M_INDEX_LEN; index++) {
        if (tAllocator->index_tbl[index].free_head != NULL)
            break;
    }
    if (index >= M_INDEX_LEN) {
        MLN_UNLOCK(&(tAllocator->index_lock));
        return NULL;
    }

    bi = &(tAllocator->index_tbl[index]);
    blk = bi->free_head;
    index_q_chain_del(&(bi->free_head), &(bi->free_tail), blk);
    index_q_chain_add(&(bi->used_head), &(bi->used_tail), blk);
    blk->in_used = 1;
    MLN_UNLOCK(&(tAllocator->index_lock));

    return blk;
}

static inline mln_blk_t *
mln_search_index_main(int index)
{
    mln_blk_t *blk = NULL;
    mln_blk_index_t *bi = NULL, *tbi = NULL;
    MLN_LOCK(&(mAllocator->index_lock));
    for (index++; index < M_INDEX_LEN; index++) {
        if (mAllocator->index_tbl[index].free_head != NULL)
            break;
    }
    if (index >= M_INDEX_LEN) {
        MLN_UNLOCK(&(mAllocator->index_lock));
        return NULL;
    }
    bi = &(mAllocator->index_tbl[index]);
    tbi = &(tAllocator->index_tbl[index]);
    blk = bi->free_head;
    index_q_chain_del(&(bi->free_head), &(bi->free_tail), blk);
    blk->salloc = tAllocator;
    blk->index = tbi;
    blk->in_used = 1;
    MLN_UNLOCK(&(mAllocator->index_lock));
    MLN_LOCK(&(tAllocator->index_lock));
    index_q_chain_add(&(tbi->used_head), &(tbi->used_tail), blk);
    MLN_UNLOCK(&(tAllocator->index_lock));

    return blk;
}

static mln_blk_t *mln_merge(int index)
{
    mln_size_t size = mAllocator->index_tbl[index].blk_size + M_SIZE_BLOCK;
    mln_size_t count;
    mln_chunk_t *chunk;
    register mln_blk_t *begin, *end;
    mln_blk_t *before;
    int search_heap = 0;
    mln_lock_t *plock = &mMmapLock;

    MLN_LOCK(&(mAllocator->index_lock));

again:
    MLN_LOCK(plock);
    chunk = search_heap? mHeapBase: mMmapHead;
    for (; chunk != NULL; chunk = chunk->next) {
        if (chunk->is_large) continue;
        if (chunk->chunk_size <= size) continue;
        before = NULL;
        begin = chunk->blk_head;
        count = 0;
        for (end = begin; \
             end != NULL && count < size; \
             end = end->chunk_next)
        {
            if (end->in_used || end->salloc != mAllocator) {
                begin = end->chunk_next;
                before = end;
                count = 0;
                continue;
            }
            count += (end->index->blk_size + M_SIZE_BLOCK);
        }
        if (count >= size) break;
    }
    if (chunk == NULL) {
        MLN_UNLOCK(plock);
        if (search_heap++ == 0) {
            plock = &mHeapLock;
            goto again;
        }
        MLN_UNLOCK(&(mAllocator->index_lock));
        return NULL;
    }

    mln_blk_t *blk = begin, *fr = NULL;
    while (blk != end) {
        fr = blk;
        blk = blk->chunk_next;
        chunk_q_chain_del(&(chunk->blk_head), &(chunk->blk_tail), fr);
        index_q_chain_del(&(fr->index->free_head), \
                          &(fr->index->free_tail), \
                          fr);
    }

    memcpy(((mln_s8ptr_t)begin)+size-M_CHKBITS_LEN, mln_check_bits, M_CHKBITS_LEN);
    begin->salloc = tAllocator;
    begin->index = &(tAllocator->index_tbl[index]);
    begin->in_used = 1;
    begin->blk_size = begin->index->blk_size;
    if (before == NULL) {
        begin->chunk_next = chunk->blk_head;
        if (chunk->blk_head != NULL) chunk->blk_head->chunk_prev = begin;
        chunk->blk_head = begin;
        if (chunk->blk_tail == NULL) chunk->blk_tail = begin;
    } else {
        begin->chunk_prev = before;
        begin->chunk_next = end;
        before->chunk_next = begin;
        if (end != NULL) end->chunk_prev = begin;
        else chunk->blk_tail = begin;
    }
    MLN_UNLOCK(plock);
    MLN_UNLOCK(&(mAllocator->index_lock));

    MLN_LOCK(&(tAllocator->index_lock));
    index_q_chain_add(&(begin->index->used_head), \
                      &(begin->index->used_tail), \
                      begin);
    MLN_UNLOCK(&(tAllocator->index_lock));
    return begin;
}

void free(void *ptr)
{
    if (ptr == NULL) return;
    mln_blk_t *blk = (mln_blk_t *)(ptr - sizeof(mln_blk_t));
    mln_chunk_t *chunk = blk->chunk;
    if (!blk->in_used) {
        mln_log(error, "Double free!\n");
        abort();
    }
    if (chunk->is_large) {
        mln_size_t chunk_size = chunk->chunk_size;
        if (memcmp(((mln_s8ptr_t)chunk)+chunk_size - M_CHKBITS_LEN, \
                   mln_check_bits, \
                   M_CHKBITS_LEN))
        {
            mln_log(error, "Block overflow!\n");
            abort();
        }
        MLN_LOCK(&mLargeTreeLock);
        blk->in_used = 0;
        mln_sarbt_insert(&mLargeTree, &(chunk->large_node));
        MLN_UNLOCK(&mLargeTreeLock);
        MLN_LOCK(&(mAllocator->stat_lock));
        register mln_size_t thr = mAllocator->threshold;
        mAllocator->threshold = (thr + \
                                 (thr - \
                                  thr/mln_calc_divisor(chunk_size, 4096))) >> 1;
        MLN_UNLOCK(&(mAllocator->stat_lock));
        mln_salloc_reduce_main();
        return;
    }

    MLN_LOCK(&(mAllocator->index_lock));
    mln_lock_t *pindex = NULL, *pstat = NULL;
    if (blk->salloc != mAllocator) {
        pindex = &(blk->salloc->index_lock);
        MLN_LOCK(pindex);
    }
    pstat = &(blk->salloc->stat_lock);
    mln_blk_index_t *bi = blk->index;
    if (memcmp(ptr+blk->blk_size, mln_check_bits, M_CHKBITS_LEN)) {
        mln_log(error, "Block overflow!\n");
        abort();
    }
    index_q_chain_del(&(bi->used_head), &(bi->used_tail), blk);
    index_q_chain_add(&(bi->free_head), &(bi->free_tail), blk);
    blk->in_used = 0;

    MLN_LOCK(pstat);
    register mln_size_t thr = blk->salloc->threshold;
    blk->salloc->threshold = (thr + \
                              (thr - \
                               thr/mln_calc_divisor(bi->blk_size, 4096))) >> 1;
    if (blk->salloc != mAllocator)
        mln_salloc_reduce_thread(blk->salloc);
    MLN_UNLOCK(pstat);
    if (pindex != NULL)
        MLN_UNLOCK(pindex);
    MLN_UNLOCK(&(mAllocator->index_lock));
    mln_salloc_reduce_main();
}

void *malloc(size_t size)
{
    return mln_alloc(size);
}

void *calloc(size_t nmemb, size_t size)
{
    mln_u8ptr_t ptr = mln_alloc(nmemb*size);
    if (ptr == NULL) return NULL;
    memset(ptr, 0, nmemb*size);
    return ptr;
}

void *realloc(void *ptr, size_t size)
{
    if (size == 0) {
        free(ptr);
        return NULL;
    }
    if (ptr == NULL) return mln_alloc(size);
    mln_u8ptr_t new_ptr = NULL;
    mln_blk_t *blk = (mln_blk_t *)(ptr - sizeof(mln_blk_t));
    if (blk->blk_size >= size)
        return ptr;
    new_ptr = mln_alloc(size);
    if (new_ptr == NULL) return NULL;
    memcpy(new_ptr, ptr, blk->blk_size);
    free(ptr);
    return new_ptr;
}

static void dump_salloc(mln_salloc_t *sa)
{
    int i;
    mln_blk_t *blk;
    mln_chunk_t *chunk;
    mln_size_t cur = 0, check_size = 0;
    mln_size_t free_cnt, used_cnt;
    mln_size_t sum_free = 0, sum_used = 0;

    MLN_LOCK(&(sa->index_lock));
    MLN_LOCK(&(sa->stat_lock));
    cur += sa->current_size;
    mln_log(none, "INDEX:\n");
    for (i = 0; i < M_INDEX_LEN; i++) {
        free_cnt = used_cnt = 0;
        blk = (sa->index_tbl)[i].free_head;
        for (; blk != NULL; blk = blk->index_next) {
            free_cnt++;
        }
        blk = (sa->index_tbl)[i].used_head;
        for (; blk != NULL; blk = blk->index_next) {
            used_cnt++;
        }
        mln_log(none, "blk_size:%U nr_free:%U nr_used:%U\n", \
                sa->index_tbl[i].blk_size, free_cnt, used_cnt);
        sum_free += free_cnt;
        sum_used += used_cnt;
    }
    mln_log(none, "SUM: nr_free:%U nr_used:%U\n", sum_free, sum_used);
    if (sa != mAllocator) {
        MLN_UNLOCK(&(sa->stat_lock));
        MLN_UNLOCK(&(sa->index_lock));
        return;
    }

    MLN_LOCK(&mHeapLock);
    mln_log(none, "CHUNK:\n");
    sum_free = sum_used = 0;
    for (chunk = mHeapBase; chunk != NULL; chunk = chunk->next) {
        check_size += chunk->chunk_size;
        for (blk = chunk->blk_head; blk != NULL; blk = blk->chunk_next) {
            if (blk->in_used) sum_used++;
            else sum_free++;
        }
    }
    MLN_UNLOCK(&mHeapLock);
    MLN_LOCK(&mMmapLock);
    for (chunk = mMmapHead; chunk != NULL; chunk = chunk->next) {
        check_size += chunk->chunk_size;
        for (blk = chunk->blk_head; blk != NULL; blk = blk->chunk_next) {
            if (blk->in_used) sum_used++;
            else sum_free++;
        }
    }
    MLN_UNLOCK(&mMmapLock);
    MLN_UNLOCK(&(sa->stat_lock));
    MLN_UNLOCK(&(sa->index_lock));
    mln_log(none, "Current_size:%U Check_size:%U nr_free:%U nr_used:%U\n", \
            cur, check_size, sum_free, sum_used);
}

void mdump(void)
{
    mln_log(none, "tAllocator:\n");
    dump_salloc(tAllocator);
    mln_log(none, "mAllocator:\n");
    dump_salloc(mAllocator);
}

static void mln_salloc_reduce_thread(mln_salloc_t *sa)
{
    int index;
    mln_blk_t *blk = NULL;
    mln_blk_index_t *bi, *mbi;
    MLN_LOCK(&(mAllocator->stat_lock));
    for (index = M_INDEX_LEN-1; index >= 0; index--) {
        bi = &(sa->index_tbl[index]);
        mbi = &(mAllocator->index_tbl[index]);
        while ((blk = bi->free_head) != NULL) {
            if (sa->current_size <= sa->threshold) {
                MLN_UNLOCK(&(mAllocator->stat_lock));
                return;
            }
            index_q_chain_del(&(bi->free_head), &(bi->free_tail), blk);
            blk->salloc = mAllocator;
            blk->index = mbi;
            index_q_chain_add(&(mbi->free_head), &(mbi->free_tail), blk);
            sa->current_size -= bi->blk_size;
            register mln_size_t thr = mAllocator->threshold;
            mAllocator->threshold = (thr + \
                                 (thr - \
                                  thr/mln_calc_divisor(blk->blk_size, 4096))) >> 1;
        }
    }
    MLN_UNLOCK(&(mAllocator->stat_lock));
}

static void mln_salloc_reduce_main(void)
{
    register mln_chunk_t *chunk;
    register mln_blk_t *blk;
    mln_sauto_t size;

    MLN_LOCK(&(mAllocator->index_lock));
    MLN_LOCK(&(mAllocator->stat_lock));
    MLN_LOCK(&mLargeTreeLock);
    
    /*heap*/
    MLN_LOCK(&mHeapLock);
    mln_u8ptr_t current_heaptop = sbrk(0);
    while ((chunk = mHeapTop) != NULL) {
        if (mAllocator->current_size <= mAllocator->threshold)
            break;
        size = chunk->chunk_size;
        if (((mln_u8ptr_t)chunk)+size != current_heaptop)
            break;
        if (chunk->is_large) {
            blk = chunk->blk_head;
            if (blk->in_used) break;
            mln_sarbt_delete(&mLargeTree, &(chunk->large_node));
        } else {
            for (blk = chunk->blk_head; blk != NULL; blk = blk->chunk_next) {
                if (blk->in_used || blk->salloc != mAllocator) {
                    for (blk = blk->chunk_prev; blk != NULL; blk = blk->chunk_prev)
                    {
                        index_q_chain_add(&(blk->index->free_head), \
                                          &(blk->index->free_tail), \
                                          blk);
                    }
                    goto hout;
                }
                index_q_chain_del(&(blk->index->free_head), \
                                  &(blk->index->free_tail), \
                                  blk);
            }
        }
        mgr_q_chain_del(&mHeapBase, &mHeapTop, chunk);
        mAllocator->current_size -= size;
        sbrk(-size);
    }
hout:
    MLN_UNLOCK(&mHeapLock);
    if (mAllocator->current_size <= mAllocator->threshold)
        goto out;

    /*mmap*/
    mln_chunk_t *fr;
    MLN_LOCK(&mMmapLock);
    chunk = mMmapHead;
mlp:
    while (chunk != NULL) {
        if (mAllocator->current_size <= mAllocator->threshold)
            break;
        if (chunk->is_large) {
            blk = chunk->blk_head;
            if (blk->in_used) {
                chunk = chunk->next;
                continue;
            }
            mln_sarbt_delete(&mLargeTree, &(chunk->large_node));
        } else {
            for (blk = chunk->blk_head; blk != NULL; blk = blk->chunk_next) {
                if (blk->in_used || blk->salloc != mAllocator) {
                    for (blk = blk->chunk_prev; blk != NULL; blk = blk->chunk_prev)
                    {
                        index_q_chain_add(&(blk->index->free_head), \
                                          &(blk->index->free_tail), \
                                          blk);
                    }
                    chunk = chunk->next;
                    goto mlp;
                }
                index_q_chain_del(&(blk->index->free_head), \
                                  &(blk->index->free_tail), \
                                  blk);
            }
        }
        fr = chunk;
        chunk = chunk->next;
        mgr_q_chain_del(&mMmapHead, &mMmapTail, fr);
        mAllocator->current_size -= fr->chunk_size;
        if (munmap((mln_u8ptr_t)fr, fr->chunk_size) < 0) {
            mln_log(error, "munmap failed. %s\n", strerror(errno));
            abort();
        }
    }
    MLN_UNLOCK(&mMmapLock);

out:
    MLN_UNLOCK(&mLargeTreeLock);
    MLN_UNLOCK(&(mAllocator->stat_lock));
    MLN_UNLOCK(&(mAllocator->index_lock));
}

static void mln_move(void *arg)
{
    int i;
    mln_blk_t *blk;
    mln_blk_index_t *dbi, *sbi;
    mln_salloc_t *sa = (mln_salloc_t *)arg;

    MLN_LOCK(&(mAllocator->index_lock));
    MLN_LOCK(&(sa->index_lock));
    MLN_LOCK(&(sa->stat_lock));
    MLN_LOCK(&(mAllocator->stat_lock));
    MLN_LOCK(&mLargeTreeLock);
    MLN_LOCK(&mMmapLock);
    MLN_LOCK(&mHeapLock);

    for (i = 0; i < M_INDEX_LEN; i++) {
        dbi = &(mAllocator->index_tbl[i]);
        sbi = &(sa->index_tbl[i]);
        while ((blk = sbi->free_head) != NULL) {
            index_q_chain_del(&(sbi->free_head), &(sbi->free_tail), blk);
            blk->salloc = mAllocator;
            blk->index = dbi;
            index_q_chain_add(&(dbi->free_head), &(dbi->free_tail), blk);
        }
        while ((blk = sbi->used_head) != NULL) {
            index_q_chain_del(&(sbi->used_head), &(sbi->used_tail), blk);
            blk->salloc = mAllocator;
            blk->index = dbi;
            index_q_chain_add(&(dbi->used_head), &(dbi->used_tail), blk);
        }
    }
    mAllocator->threshold >>= 1;

    MLN_UNLOCK(&mHeapLock);
    MLN_UNLOCK(&mMmapLock);
    MLN_UNLOCK(&mLargeTreeLock);
    MLN_UNLOCK(&(mAllocator->stat_lock));
    MLN_UNLOCK(&(sa->stat_lock));
    MLN_UNLOCK(&(sa->index_lock));
    MLN_UNLOCK(&(mAllocator->index_lock));

    mln_salloc_destroy(sa);

    mln_salloc_reduce_main();
}

static void
mln_salloc_destroy(mln_salloc_t *sa)
{
    pthread_key_delete(sa->key);
    MLN_LOCK_DESTROY(&(sa->index_lock));
    MLN_LOCK_DESTROY(&(sa->stat_lock));
    if (munmap(sa, sizeof(mln_salloc_t)) < 0) {
        mln_log(error, "munmap failed. %s\n", strerror(errno));
        abort();
    }
}

/*
 * chains
 */
MLN_CHAIN_FUNC_DEFINE(mgr_q, \
                      mln_chunk_t, \
                      static inline void, \
                      prev, \
                      next);
MLN_CHAIN_FUNC_DEFINE(chunk_q, \
                      mln_blk_t, \
                      static inline void, \
                      chunk_prev, \
                      chunk_next);
MLN_CHAIN_FUNC_DEFINE(index_q, \
                      mln_blk_t, \
                      static inline void, \
                      index_prev, \
                      index_next);

