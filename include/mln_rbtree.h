
/*
 * Copyright (C) Niklaus F.Schen.
 */

#ifndef __MLN_RBTREE_H
#define __MLN_RBTREE_H
#include "mln_types.h"

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
    M_RB_RED,
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
    enum rbtree_color          color;
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


MLN_CHAIN_FUNC_DECLARE(mln_rbtree, \
                       mln_rbtree_node_t, \
                       static inline void,);
MLN_CHAIN_FUNC_DEFINE(mln_rbtree, \
                      mln_rbtree_node_t, \
                      static inline void, \
                      prev, \
                      next);

/*Left rotate*/
static inline void
mln_rbtree_left_rotate(mln_rbtree_t *t, mln_rbtree_node_t *n)
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
}

/*Right rotate*/
static inline void
mln_rbtree_right_rotate(mln_rbtree_t *t, mln_rbtree_node_t *n)
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
}

/*insert fixup*/
static inline void
rbtree_insert_fixup(mln_rbtree_t *t, mln_rbtree_node_t *n)
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
}

/*Insert*/
#define mln_rbtree_inline_insert(t, n, compare) ({\
    mln_rbtree_node_t *y = &((t)->nil);\
    mln_rbtree_node_t *x = (t)->root;\
    mln_rbtree_node_t *nil = &(t->nil);\
    rbtree_cmp cmp = (compare) == NULL? (t)->cmp: (rbtree_cmp)(compare);\
    while (x != nil) {\
        y = x;\
        if (cmp((n)->data, x->data) < 0) x = x->left;\
        else x = x->right;\
    }\
    (n)->parent = y;\
    if (y == nil) (t)->root = (n);\
    else if (cmp((n)->data, y->data) < 0) y->left = (n);\
    else y->right = (n);\
    (n)->left = (n)->right = nil;\
    (n)->color = M_RB_RED;\
    rbtree_insert_fixup((t), (n));\
    if ((t)->min == nil) (t)->min = (n);\
    else if (cmp((n)->data, (t)->min->data) < 0) (t)->min = (n);\
    ++((t)->nr_node);\
    mln_rbtree_chain_add(&((t)->head), &((t)->tail), (n));\
})



/*search*/
#define mln_rbtree_inline_root_search(t, root, key, compare) ({\
    mln_rbtree_node_t *ret_node = (root);\
    rbtree_cmp cmp = (compare) == NULL? (t)->cmp: (rbtree_cmp)(compare);\
    int ret;\
    while ((ret_node != &(t->nil)) && ((ret = cmp(key, ret_node->data)) != 0)) {\
        if (ret < 0) ret_node = ret_node->left;\
        else ret_node = ret_node->right;\
    }\
    ret_node;\
})

#define mln_rbtree_inline_search(t, key, compare) ({\
    mln_rbtree_node_t *ret_node = (t)->root;\
    rbtree_cmp cmp = (compare) == NULL? (t)->cmp: (rbtree_cmp)(compare);\
    int ret;\
    while ((ret_node != &(t->nil)) && ((ret = cmp(key, ret_node->data)) != 0)) {\
        if (ret < 0) ret_node = ret_node->left;\
        else ret_node = ret_node->right;\
    }\
    ret_node;\
})

/*rbtree free node*/
#define mln_rbtree_inline_node_free(t, n, freer) ({\
    rbtree_free_data f = (freer) == NULL? (t)->data_free: (rbtree_free_data)(freer);\
    if ((n)->data != NULL && f != NULL)\
        f((n)->data);\
    if ((t)->pool != NULL) (t)->pool_free((n));\
    else free((n));\
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
    if ((t) != NULL) {\
        mln_rbtree_node_t *fr;\
        while ((fr = (t)->tail) != NULL) {\
            mln_rbtree_chain_del(&((t)->head), &((t)->tail), fr);\
            mln_rbtree_inline_node_free((t), fr, (freer));\
        }\
        if ((t)->pool != NULL) (t)->pool_free((t));\
        else free((t));\
    }\
})


#define mln_rbtree_inline_reset(t, freer) ({\
    mln_rbtree_node_t *fr;\
    while ((fr = (t)->tail) != NULL) {\
        mln_rbtree_chain_del(&((t)->head), &((t)->tail), fr);\
        mln_rbtree_inline_node_free((t), fr, (freer));\
    }\
    (t)->root = &((t)->nil);\
    (t)->min = &((t)->nil);\
    (t)->iter = NULL;\
    (t)->nr_node = 0;\
    (t)->del = 0;\
})


#define mln_rbtree_node_num(ptree)            ((ptree)->nr_node)
#define mln_rbtree_null(ptr,ptree)            ((ptr)==&((ptree)->nil))
#define mln_rbtree_node_data_get(node)        ((node)->data)
#define mln_rbtree_node_data_set(node,ud)     ((node)->data = (ud))
#define mln_rbtree_root(ptree)                ((ptree)->root)

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

