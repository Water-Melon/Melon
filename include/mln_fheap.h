
/*
 * Copyright (C) Niklaus F.Schen.
 */

#ifndef __MLN_FHEAP_H
#define __MLN_FHEAP_H

#include "mln_types.h"

#if defined(i386) || defined(__arm__) || defined(WIN32)
#define FH_LGN 33
#else
#define FH_LGN 65
#endif

enum mln_fheap_mark {
    FHEAP_FALSE,
    FHEAP_TRUE
};

/*
 * return value: 0 - p1 < p2   !0 - p1 >= p2
 */
typedef int (*fheap_cmp)(const void *, const void *);
/*
 * the left argument is the destination and the right
 * one is the source.
 */
typedef void (*fheap_copy)(void *, void *);
typedef void (*fheap_key_free)(void *);
typedef void *(*fheap_pool_alloc_handler)(void *, mln_size_t);
typedef void (*fheap_pool_free_handler)(void *);

struct mln_fheap_attr {
    void                     *pool;
    fheap_pool_alloc_handler  pool_alloc;
    fheap_pool_free_handler   pool_free;
    fheap_cmp                 cmp;
    fheap_copy                copy;
    fheap_key_free            key_free;
    void                     *min_val;
    mln_size_t                min_val_size;
};

typedef struct mln_fheap_node_s {
    void                     *key;
    struct mln_fheap_node_s  *parent;
    struct mln_fheap_node_s  *child;
    struct mln_fheap_node_s  *left;
    struct mln_fheap_node_s  *right;
    mln_size_t                degree;
    enum mln_fheap_mark       mark;
} mln_fheap_node_t;

typedef struct {
    void                     *pool;
    fheap_pool_alloc_handler  pool_alloc;
    fheap_pool_free_handler   pool_free;
    void                     *min_val;
    fheap_cmp                 cmp;
    fheap_copy                copy;
    fheap_key_free            key_free;
    mln_fheap_node_t         *min;
    mln_fheap_node_t         *root_list;
    mln_size_t                num;
} mln_fheap_t;


#define mln_fheap_node_key(node)   ((node)->key)

extern mln_fheap_t *
mln_fheap_new(struct mln_fheap_attr *attr) __NONNULL1(1);
extern void
mln_fheap_free(mln_fheap_t *fh);
extern void
mln_fheap_insert(mln_fheap_t *fh, mln_fheap_node_t *fn) __NONNULL2(1,2);
extern mln_fheap_node_t *
mln_fheap_minimum(mln_fheap_t *fh) __NONNULL1(1);
extern mln_fheap_node_t *
mln_fheap_extract_min(mln_fheap_t *fh) __NONNULL1(1);
/*
 * return value: -1 - key error   0 - succeed
 */
extern int
mln_fheap_decrease_key(mln_fheap_t *fh, mln_fheap_node_t *node, void *key) __NONNULL3(1,2,3);
extern void
mln_fheap_delete(mln_fheap_t *fh, mln_fheap_node_t *node) __NONNULL2(1,2);

/*mln_fheap_node_t*/
extern mln_fheap_node_t *
mln_fheap_node_new(mln_fheap_t *fh, void *key) __NONNULL2(1,2);
extern void
mln_fheap_node_free(mln_fheap_t *fh, mln_fheap_node_t *fn) __NONNULL1(1);

#endif

