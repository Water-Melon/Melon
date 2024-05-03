
/*
 * Copyright (C) Niklaus F.Schen.
 */

#ifndef __MLN_RBTREE_H
#define __MLN_RBTREE_H
#include "mln_types.h"
#include "mln_func.h"

typedef struct mln_rbtree_node_s mln_rbtree_node_t;
/*
 * >0 -- the first argument greater than the second.
 * ==0 -- equal.
 * <0 -- less.
 */
typedef int (*rbtree_cmp)(const void *, const void *);
typedef void (*rbtree_free_data)(void *);
typedef int (*rbtree_iterate_handler)(mln_rbtree_node_t *node, void *udata);
typedef void *(*rbtree_pool_alloc_handler)(void *, mln_size_t);
typedef void (*rbtree_pool_free_handler)(void *);

enum rbtree_color {
    M_RB_RED = 0,
    M_RB_BLACK
};

struct mln_rbtree_attr {
    void                      *pool;
    rbtree_pool_alloc_handler  pool_alloc;
    rbtree_pool_free_handler   pool_free;
    rbtree_cmp                 cmp;
    rbtree_free_data           data_free;
};

struct mln_rbtree_node_s {
    void                      *data;
    struct mln_rbtree_node_s  *prev;
    struct mln_rbtree_node_s  *next;
    struct mln_rbtree_node_s  *parent;
    struct mln_rbtree_node_s  *left;
    struct mln_rbtree_node_s  *right;
    mln_u32_t                  nofree:1;
    mln_u32_t                  color:31;
};

typedef struct rbtree_s {
    void                      *pool;
    rbtree_pool_alloc_handler  pool_alloc;
    rbtree_pool_free_handler   pool_free;
    rbtree_cmp                 cmp;
    rbtree_free_data           data_free;
    mln_rbtree_node_t          nil;
    mln_rbtree_node_t         *root;
    mln_rbtree_node_t         *min;
    mln_rbtree_node_t         *head;
    mln_rbtree_node_t         *tail;
    mln_rbtree_node_t         *iter;
    mln_uauto_t                nr_node;
    mln_u32_t                  del:1;
} mln_rbtree_t;


MLN_CHAIN_FUNC_DECLARE(static inline, \
                       mln_rbtree, \
                       mln_rbtree_node_t,);
MLN_CHAIN_FUNC_DEFINE(static inline, \
                      mln_rbtree, \
                      mln_rbtree_node_t, \
                      prev, \
                      next);

/*Left rotate*/
MLN_FUNC_VOID(static inline, void, mln_rbtree_left_rotate, \
              (mln_rbtree_t *t, mln_rbtree_node_t *n), (t, n), \
{
    if (n->right == &(t->nil)) return;
    mln_rbtree_node_t *tmp = n->right;
    n->right = tmp->left;
    if (tmp->left != &(t->nil)) tmp->left->parent = n;
    tmp->parent = n->parent;
    if (n->parent == &(t->nil)) t->root = tmp;
    else if (n == n->parent->left) n->parent->left = tmp;
    else n->parent->right = tmp;
    tmp->left = n;
    n->parent = tmp;
})

/*Right rotate*/
MLN_FUNC_VOID(static inline, void, mln_rbtree_right_rotate, \
              (mln_rbtree_t *t, mln_rbtree_node_t *n), (t, n), \
{
    if (n->left == &(t->nil)) return;
    mln_rbtree_node_t *tmp = n->left;
    n->left = tmp->right;
    if (tmp->right != &(t->nil)) tmp->right->parent = n;
    tmp->parent = n->parent;
    if (n->parent == &(t->nil)) t->root = tmp;
    else if (n==n->parent->right) n->parent->right = tmp;
    else n->parent->left = tmp;
    tmp->right = n;
    n->parent = tmp;
})

/*insert fixup*/
MLN_FUNC_VOID(static inline, void, rbtree_insert_fixup, \
              (mln_rbtree_t *t, mln_rbtree_node_t *n), (t, n), \
{
    mln_rbtree_node_t *tmp;
    while (n->parent->color == M_RB_RED) {
        if (n->parent == n->parent->parent->left) {
            tmp = n->parent->parent->right;
            if (tmp->color == M_RB_RED) {
                n->parent->color = M_RB_BLACK;
                tmp->color = M_RB_BLACK;
                n->parent->parent->color = M_RB_RED;
                n = n->parent->parent;
                continue;
            } else if (n == n->parent->right) {
                n = n->parent;
                mln_rbtree_left_rotate(t, n);
            }
            n->parent->color = M_RB_BLACK;
            n->parent->parent->color = M_RB_RED;
            mln_rbtree_right_rotate(t, n->parent->parent);
        } else {
            tmp = n->parent->parent->left;
            if (tmp->color == M_RB_RED) {
                n->parent->color = M_RB_BLACK;
                tmp->color = M_RB_BLACK;
                n->parent->parent->color = M_RB_RED;
                n = n->parent->parent;
                continue;
            } else if (n == n->parent->left) {
                n = n->parent;
                mln_rbtree_right_rotate(t, n);
            }
            n->parent->color = M_RB_BLACK;
            n->parent->parent->color = M_RB_RED;
            mln_rbtree_left_rotate(t, n->parent->parent);
        }
    }
    t->root->color = M_RB_BLACK;
})

#if !defined(MSVC)
/*Insert*/
#define mln_rbtree_inline_insert(t, n, compare) ({\
    mln_rbtree_t *tree = (t);\
    mln_rbtree_node_t *y = &(tree->nil);\
    mln_rbtree_node_t *x = tree->root;\
    mln_rbtree_node_t *nil = &(tree->nil);\
    while (x != nil) {\
        y = x;\
        if (compare((n)->data, x->data) < 0) x = x->left;\
        else x = x->right;\
    }\
    (n)->parent = y;\
    if (y == nil) tree->root = (n);\
    else if (compare((n)->data, y->data) < 0) y->left = (n);\
    else y->right = (n);\
    (n)->left = (n)->right = nil;\
    (n)->color = M_RB_RED;\
    rbtree_insert_fixup(tree, (n));\
    if (tree->min == nil) tree->min = (n);\
    else if (compare((n)->data, tree->min->data) < 0) tree->min = (n);\
    ++(tree->nr_node);\
    mln_rbtree_chain_add(&(tree->head), &(tree->tail), (n));\
})



/*search*/
#define mln_rbtree_inline_root_search(t, root, key, compare) ({\
    mln_rbtree_t *tree = (t);\
    mln_rbtree_node_t *ret_node = (root);\
    int ret;\
    while ((ret_node != &(tree->nil)) && ((ret = compare(key, ret_node->data)) != 0)) {\
        if (ret < 0) ret_node = ret_node->left;\
        else ret_node = ret_node->right;\
    }\
    ret_node;\
})

#define mln_rbtree_inline_search(t, key, compare) ({\
    mln_rbtree_t *tree = (t);\
    mln_rbtree_node_t *ret_node = tree->root;\
    int ret;\
    while ((ret_node != &(tree->nil)) && ((ret = compare(key, ret_node->data)) != 0)) {\
        if (ret < 0) ret_node = ret_node->left;\
        else ret_node = ret_node->right;\
    }\
    ret_node;\
})

/*rbtree free node*/
#define mln_rbtree_inline_node_free(t, n, freer) ({\
    mln_u32_t nofree = (n)->nofree;\
    if ((n)->data != NULL)\
        freer((n)->data);\
    if (!nofree) {\
        if ((t)->pool != NULL) (t)->pool_free((n));\
        else free((n));\
    }\
})


/*rbtree_destroy*/
/*
 * Warning: mln_lang_sys.c: mln_import is very dependent on this release order.
 * This release order ensures that the resources of the dynamic extension library
 * are released first, and then the import resources are released.
 * If the import resource is released before the dynamic library resource,
 * the function in the dynamic library cannot be read when the dynamic extension
 * resource is released, and the program terminates abnormally.
 */
#define mln_rbtree_inline_free(t, freer) ({\
    mln_rbtree_t *tree = (t);\
    if (tree != NULL) {\
        mln_rbtree_node_t *fr;\
        while ((fr = tree->tail) != NULL) {\
            mln_rbtree_chain_del(&(tree->head), &(tree->tail), fr);\
            mln_rbtree_inline_node_free(tree, fr, freer);\
        }\
        if (tree->pool != NULL) tree->pool_free(tree);\
        else free(tree);\
    }\
})


#define mln_rbtree_inline_reset(t, freer) ({\
    mln_rbtree_t *tree = (t);\
    mln_rbtree_node_t *fr;\
    while ((fr = tree->tail) != NULL) {\
        mln_rbtree_chain_del(&(tree->head), &(tree->tail), fr);\
        mln_rbtree_inline_node_free(tree, fr, freer);\
    }\
    tree->root = &(tree->nil);\
    tree->min = &(tree->nil);\
    tree->iter = NULL;\
    tree->nr_node = 0;\
    tree->del = 0;\
})

#define mln_rbtree_node_init(n, ud) ({\
    (n)->data = (ud);\
    (n)->nofree = 1;\
    (n);\
})
#endif


#define mln_rbtree_node_num(ptree)            ((ptree)->nr_node)
#define mln_rbtree_null(ptr,ptree)            ((ptr)==&((ptree)->nil))
#define mln_rbtree_node_data_get(node)        ((node)->data)
#define mln_rbtree_node_data_set(node,ud)     ((node)->data = (ud))
#define mln_rbtree_root(ptree)                ((ptree)->root)

#if defined(MSVC)
extern mln_rbtree_node_t *mln_rbtree_node_init(mln_rbtree_node_t *n, void *data);
#endif
extern mln_rbtree_t *mln_rbtree_new(struct mln_rbtree_attr *attr);
extern void mln_rbtree_free(mln_rbtree_t *t);
extern void mln_rbtree_insert(mln_rbtree_t *t, mln_rbtree_node_t *node) __NONNULL2(1,2);
mln_rbtree_node_t *mln_rbtree_search(mln_rbtree_t *t, void *key) __NONNULL2(1,2);
extern void mln_rbtree_delete(mln_rbtree_t *t, mln_rbtree_node_t *n) __NONNULL2(1,2);
extern mln_rbtree_node_t *mln_rbtree_successor(mln_rbtree_t *t, mln_rbtree_node_t *n) __NONNULL2(1,2);
extern mln_rbtree_node_t *mln_rbtree_min(mln_rbtree_t *t) __NONNULL1(1);
extern void mln_rbtree_reset(mln_rbtree_t *t) __NONNULL1(1);

extern mln_rbtree_node_t *mln_rbtree_node_new(mln_rbtree_t *t, void *data) __NONNULL2(1,2);
extern void mln_rbtree_node_free(mln_rbtree_t *t, mln_rbtree_node_t *n) __NONNULL2(1,2);

extern int mln_rbtree_iterate(mln_rbtree_t *t, rbtree_iterate_handler handler, void *udata) __NONNULL2(1,2);
#endif

