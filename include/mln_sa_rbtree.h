
/*
 * Copyright (C) Niklaus F.Schen.
 */

#ifndef __MLN_SA_RBTREE_H
#define __MLN_SA_RBTREE_H
#include "mln_defs.h"

/*
 * >0 -- the first argument greater than the second.
 * ==0 -- equal.
 * <0 -- less.
 */
typedef int (*sarbt_cmp)(const void *, const void *);

enum sarbt_color {
    M_SARB_RED,
    M_SARB_BLACK
};

typedef struct mln_sarbt_node_s {
    void                     *data;
    struct mln_sarbt_node_s  *parent;
    struct mln_sarbt_node_s  *left;
    struct mln_sarbt_node_s  *right;
    enum sarbt_color          color;
} mln_sarbt_node_t __cacheline_aligned;

typedef struct rbtree_s {
    mln_sarbt_node_t          nil;
    mln_sarbt_node_t         *root;
    sarbt_cmp                 cmp;
} mln_sarbt_t;

extern void
mln_sarbt_insert(mln_sarbt_t *t, mln_sarbt_node_t *n) __NONNULL2(1,2);
extern void
mln_sarbt_delete(mln_sarbt_t *t, mln_sarbt_node_t *n) __NONNULL2(1,2);
extern mln_sarbt_node_t *
mln_sarbt_successor(mln_sarbt_t *t, mln_sarbt_node_t *n) __NONNULL2(1,2);
extern void *
mln_sarbt_search(mln_sarbt_t *t, const void *key) __NONNULL2(1,2);
extern void *
mln_sarbt_maximum(mln_sarbt_t *t) __NONNULL1(1);
#endif

