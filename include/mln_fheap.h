
/*
 * Copyright (C) Niklaus F.Schen.
 */

#ifndef __MLN_FHEAP_H
#define __MLN_FHEAP_H

#include <stdlib.h>
#include <string.h>
#include "mln_types.h"
#include "mln_func.h"

#if defined(i386) || defined(__arm__) || defined(MSVC)
#define FH_LGN 33
#else
#define FH_LGN 65
#endif

enum mln_fheap_mark {
    FHEAP_FALSE = 0,
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
};

typedef struct mln_fheap_node_s {
    void                     *key;
    struct mln_fheap_node_s  *parent;
    struct mln_fheap_node_s  *child;
    struct mln_fheap_node_s  *left;
    struct mln_fheap_node_s  *right;
    mln_size_t                degree;
    mln_u32_t                 nofree:1;
    mln_u32_t                 mark:31;
} mln_fheap_node_t;

typedef struct {
    mln_fheap_node_t         *root_list;
    mln_fheap_node_t         *min;
    fheap_cmp                 cmp;
    fheap_copy                copy;
    fheap_key_free            key_free;
    mln_size_t                num;
    void                     *min_val;
    void                     *pool;
    fheap_pool_alloc_handler  pool_alloc;
    fheap_pool_free_handler   pool_free;
    mln_fheap_node_t         *free_list;
} mln_fheap_t;

/*
 * for internal
 */
MLN_FUNC_VOID(static inline, void, mln_fheap_add_child, \
              (mln_fheap_node_t **root, mln_fheap_node_t *node), \
              (root, node), \
{
    if (*root == NULL) {
        *root = node;
        return;
    }
    node->right = *root;
    node->left = (*root)->left;
    (*root)->left = node;
    node->left->right = node;
})

MLN_FUNC_VOID(static inline, void, mln_fheap_del_child, \
              (mln_fheap_node_t **root, mln_fheap_node_t *node), \
              (root, node), \
{
    if (*root == node) {
        if (node->right == node) {
            *root = NULL;
        } else {
            *root = node->right;
            node->right->left = node->left;
            node->left->right = node->right;
        }
    } else {
        /*if (node->right == node) abort();*/
        node->right->left = node->left;
        node->left->right = node->right;
    }
    node->right = node->left = node;
})

MLN_FUNC_VOID(static inline, void, mln_fheap_splice_children, \
              (mln_fheap_node_t **root_list, mln_fheap_node_t *child_list), \
              (root_list, child_list), \
{
    if (child_list == NULL) return;
    mln_fheap_node_t *c = child_list;
    do {
        c->parent = NULL;
        c = c->right;
    } while (c != child_list);
    if (*root_list == NULL) {
        *root_list = child_list;
    } else {
        mln_fheap_node_t *rl = (*root_list)->left;
        mln_fheap_node_t *cl = child_list->left;
        rl->right = child_list;
        child_list->left = rl;
        cl->right = *root_list;
        (*root_list)->left = cl;
    }
})

MLN_FUNC_VOID(static inline, void, mln_fheap_destroy_list, \
              (mln_fheap_node_t *head, fheap_key_free kfree, void *pool, fheap_pool_free_handler pfree), \
              (head, kfree, pool, pfree), \
{
    if (head == NULL) return;
    head->left->right = NULL;
    mln_fheap_node_t *w = head, *next;
    while (w != NULL) {
        next = w->right;
        mln_fheap_destroy_list(w->child, kfree, pool, pfree);
        if (kfree != NULL && w->key != NULL)
            kfree(w->key);
        if (!w->nofree) {
            if (pool != NULL) pfree(w);
            else free(w);
        }
        w = next;
    }
})

MLN_FUNC_VOID(static inline, void, mln_fheap_link, \
              (mln_fheap_t *fh, mln_fheap_node_t *y, mln_fheap_node_t *x), \
              (fh, y, x), \
{
    mln_fheap_del_child(&(fh->root_list), y);
    mln_fheap_add_child(&(x->child), y);
    y->parent = x;
    ++(x->degree);
    y->mark = FHEAP_FALSE;
})

MLN_FUNC_VOID(static inline, void, mln_fheap_consolidate, \
              (mln_fheap_t *fh, fheap_cmp cmp), (fh, cmp), \
{
    mln_fheap_node_t *array[FH_LGN];
    memset(array, 0, sizeof(mln_fheap_node_t *) * FH_LGN);
    mln_fheap_node_t *w = fh->root_list;
    if (w == NULL) { fh->min = NULL; return; }
    mln_size_t root_count = 0;
    mln_fheap_node_t *t = w;
    do { root_count++; t = t->right; } while (t != w);
    mln_fheap_node_t *x, *y, *next, *tmp;
    mln_size_t d, i, max_d = 0;
    for (i = 0; i < root_count; i++) {
        next = w->right;
        x = w;
        x->left = x;
        x->right = x;
        d = x->degree;
        while (array[d] != NULL) {
            y = array[d];
            if (!cmp(y->key, x->key)) {
                tmp = x; x = y; y = tmp;
            }
            if (x->child == NULL) {
                x->child = y;
                y->left = y;
                y->right = y;
            } else {
                y->right = x->child;
                y->left = x->child->left;
                x->child->left->right = y;
                x->child->left = y;
            }
            y->parent = x;
            ++(x->degree);
            y->mark = FHEAP_FALSE;
            array[d] = NULL;
            ++d;
        }
        array[d] = x;
        if (d > max_d) max_d = d;
        w = next;
    }
    fh->root_list = NULL;
    fh->min = NULL;
    for (i = 0; i <= max_d; ++i) {
        if (array[i] == NULL) continue;
        mln_fheap_node_t *node = array[i];
        node->parent = NULL;
        if (fh->root_list == NULL) {
            node->left = node;
            node->right = node;
            fh->root_list = node;
            fh->min = node;
        } else {
            node->right = fh->root_list;
            node->left = fh->root_list->left;
            fh->root_list->left->right = node;
            fh->root_list->left = node;
            if (!cmp(node->key, fh->min->key))
                fh->min = node;
        }
    }
})

MLN_FUNC_VOID(static inline, void, mln_fheap_cut, \
              (mln_fheap_t *fh, mln_fheap_node_t *x, mln_fheap_node_t *y), \
              (fh, x, y), \
{
    mln_fheap_del_child(&(y->child), x);
    --(y->degree);
    mln_fheap_add_child(&(fh->root_list), x);
    x->parent = NULL;
    x->mark = FHEAP_FALSE;
})

MLN_FUNC_VOID(static inline, void, mln_fheap_cascading_cut, \
              (mln_fheap_t *fh, mln_fheap_node_t *y), (fh, y), \
{
    mln_fheap_node_t *z;
lp:
    z = y->parent;
    if (z == NULL) return;
    if (y->mark == FHEAP_FALSE)
        y->mark = FHEAP_TRUE;
    else {
        mln_fheap_cut(fh, y, z);
        y = z;
        goto lp;
    }
})

MLN_FUNC(static inline, mln_fheap_node_t *, mln_fheap_remove_child, \
         (mln_fheap_node_t **root), (root), \
{
    if (*root == NULL) return NULL;
    mln_fheap_node_t *ret = *root;
    if (ret->right == ret) {
        *root = NULL;
    } else {
        *root = ret->right;
        ret->right->left = ret->left;
        ret->left->right = ret->right;
    }
    ret->left = ret->right = ret;
    return ret;
})

#if !defined(MSVC)
#define mln_fheap_inline_insert(fh, fn, compare) ({\
    fheap_cmp cmp = (fheap_cmp)(compare);\
    if (cmp == NULL) cmp = (fh)->cmp;\
    mln_fheap_add_child(&((fh)->root_list), (fn));\
    (fn)->parent = NULL;\
    if ((fh)->min == NULL) {\
        (fh)->min = (fn);\
    } else {\
        if (!cmp((fn)->key, (fh)->min->key))\
            (fh)->min = (fn);\
    }\
    ++((fh)->num);\
})

#define mln_fheap_inline_extract_min(fh, compare) ({\
    fheap_cmp cmp = (fheap_cmp)(compare);\
    if (cmp == NULL) cmp = (fh)->cmp;\
    mln_fheap_node_t *z = (fh)->min;\
    if (z != NULL) {\
        mln_fheap_splice_children(&((fh)->root_list), z->child);\
        z->child = NULL;\
        mln_fheap_del_child(&((fh)->root_list), z);\
        if ((fh)->root_list == NULL) {\
            (fh)->min = NULL;\
        } else {\
            (fh)->min = (fh)->root_list;\
            mln_fheap_consolidate((fh), cmp);\
        }\
        --((fh)->num);\
    }\
    z;\
})

#define mln_fheap_inline_decrease_key(fh, node, k, cpy, compare) ({\
    fheap_cmp cmp = (fheap_cmp)(compare);\
    if (cmp == NULL) cmp = (fh)->cmp;\
    fheap_copy cp = (fheap_copy)(cpy);\
    if (cp == NULL) cp = (fh)->copy;\
    int r = 0;\
    if (!cmp((node)->key, (k))) {\
        r = -1;\
    } else {\
        cp((node)->key, (k));\
        mln_fheap_node_t *y = (node)->parent;\
        if (y != NULL && !cmp((node)->key, y->key)) {\
            mln_fheap_cut((fh), (node), y);\
            mln_fheap_cascading_cut((fh), y);\
        }\
        if ((node) != (fh)->min && !cmp((node)->key, (fh)->min->key))\
            (fh)->min = (node);\
    }\
    r;\
})


#define mln_fheap_inline_delete(fh, node, cpy, compare) ({\
    mln_fheap_inline_decrease_key((fh), (node), (fh)->min_val, cpy, compare);\
    mln_fheap_inline_extract_min((fh), compare);\
})

#define mln_fheap_inline_node_free(fh, fn, freer) ({\
    fheap_key_free f = (fheap_key_free)(freer);\
    if (f == NULL) f = (fh)->key_free;\
    if ((fn) != NULL) {\
        if (f != NULL && (fn)->key != NULL)\
            f((fn)->key);\
        if (!(fn)->nofree) {\
           if ((fh)->pool != NULL) (fh)->pool_free((fn));\
           else { (fn)->right = (fh)->free_list; (fh)->free_list = (fn); }\
        }\
    }\
})

#define mln_fheap_inline_free(fh, compare, freer) ({\
    if ((fh) != NULL) {\
        fheap_key_free _kf = (fheap_key_free)(freer);\
        if (_kf == NULL) _kf = (fh)->key_free;\
        mln_fheap_destroy_list((fh)->root_list, _kf, (fh)->pool, (fh)->pool_free);\
        { mln_fheap_node_t *_fl = (fh)->free_list;\
          while (_fl != NULL) {\
            mln_fheap_node_t *_fn = _fl; _fl = _fl->right;\
            if ((fh)->pool != NULL) (fh)->pool_free(_fn);\
            else free(_fn);\
          }\
        }\
        if ((fh)->pool != NULL) (fh)->pool_free((fh));\
        else free((fh));\
    }\
})

#define mln_fheap_node_init(fn, k) ({\
    (fn)->key = (k);\
    (fn)->parent = NULL;\
    (fn)->child = NULL;\
    (fn)->left = (fn);\
    (fn)->right = (fn);\
    (fn)->degree = 0;\
    (fn)->nofree = 1;\
    (fn)->mark = FHEAP_FALSE;\
    (fn);\
})
#else
extern mln_fheap_node_t *mln_fheap_node_init(mln_fheap_node_t *fn, void *k);
#endif

/*
 * external
 */
#define mln_fheap_node_key(node)   ((node)->key)
#define mln_fheap_minimum(fh)      ((fh)->min)

extern mln_fheap_t *
mln_fheap_new(void *min_val, struct mln_fheap_attr *attr) __NONNULL1(1);
extern mln_fheap_t *
mln_fheap_new_fast(void *min_val, fheap_cmp cmp, fheap_copy copy, fheap_key_free key_free);
extern void
mln_fheap_free(mln_fheap_t *fh);
extern void
mln_fheap_insert(mln_fheap_t *fh, mln_fheap_node_t *fn) __NONNULL2(1,2);
extern mln_fheap_node_t *
mln_fheap_extract_min(mln_fheap_t *fh) __NONNULL1(1);
/*
 * return value: -1 - key error   0 - on success
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

