
/*
 * Copyright (C) Niklaus F.Schen.
 */

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include"mln_rbtree.h"

/*static declarations*/
static inline mln_rbtree_node_t *
rbtree_minimum(mln_rbtree_t *t, mln_rbtree_node_t *n) __NONNULL2(1,2);
static inline void
rbtree_transplant(mln_rbtree_t *t, mln_rbtree_node_t *u, mln_rbtree_node_t *v) __NONNULL3(1,2,3);
static inline void
rbtree_delete_fixup(mln_rbtree_t *t, mln_rbtree_node_t *n) __NONNULL2(1,2);

/*rbtree_init*/
mln_rbtree_t *
mln_rbtree_new(struct mln_rbtree_attr *attr)
{
    mln_rbtree_t *t;
    if (attr == NULL || attr->pool == NULL) {
        t = (mln_rbtree_t *)malloc(sizeof(mln_rbtree_t));
    } else {
        t = (mln_rbtree_t *)attr->pool_alloc(attr->pool, sizeof(mln_rbtree_t));
    }
    if (t == NULL) return NULL;
    if (attr == NULL) {
        t->pool = NULL;
        t->pool_alloc = NULL;
        t->pool_free = NULL;
        t->cmp = NULL;
        t->data_free = NULL;
    } else {
        t->pool = attr->pool;
        t->pool_alloc = attr->pool_alloc;
        t->pool_free = attr->pool_free;
        t->cmp = attr->cmp;
        t->data_free = attr->data_free;
    }
    t->nil.data = NULL;
    t->nil.parent = &(t->nil);
    t->nil.left = &(t->nil);
    t->nil.right = &(t->nil);
    t->nil.color = M_RB_BLACK;
    t->root = &(t->nil);
    t->min = &(t->nil);
    t->head = t->tail = NULL;
    t->iter = NULL;
    t->nr_node = 0;
    t->del = 0;
    return t;
}

/*rbtree free*/
void mln_rbtree_free(mln_rbtree_t *t)
{
    mln_rbtree_inline_free(t, NULL);
}

/*rbtree reset*/
void mln_rbtree_reset(mln_rbtree_t *t)
{
    mln_rbtree_inline_reset(t, NULL);
}

/*rbtree insert*/
void mln_rbtree_insert(mln_rbtree_t *t, mln_rbtree_node_t *node)
{
    mln_rbtree_inline_insert(t, node, NULL);
}

/*rbtree search*/
mln_rbtree_node_t *mln_rbtree_search(mln_rbtree_t *t, void *key)
{
    return mln_rbtree_inline_search(t, key, NULL);
}

/*rbtree successor*/
mln_rbtree_node_t *
mln_rbtree_successor(mln_rbtree_t *t, mln_rbtree_node_t *n)
{
    if (n != &(t->nil) && n->right != &(t->nil))
        return rbtree_minimum(t, n->right);
    mln_rbtree_node_t *tmp = n->parent;
    while (tmp!=&(t->nil) && n==tmp->right) {
        n = tmp; tmp = tmp->parent;
    }
    return tmp;
}

/*rbtree node new*/
mln_rbtree_node_t *
mln_rbtree_node_new(mln_rbtree_t *t, void *data)
{
    mln_rbtree_node_t *n;

    if (t->pool == NULL)
        n = (mln_rbtree_node_t *)malloc(sizeof(mln_rbtree_node_t));
    else
        n = (mln_rbtree_node_t *)t->pool_alloc(t->pool, sizeof(mln_rbtree_node_t));
    if (n == NULL) return NULL;
    n->data = data;
    return n;
}

/*rbtree node free*/
void mln_rbtree_node_free(mln_rbtree_t *t, mln_rbtree_node_t *n)
{
    mln_rbtree_node_inline_free(t, n, NULL);
}

/*Tree Minimum*/
static inline mln_rbtree_node_t *
rbtree_minimum(mln_rbtree_t *t, mln_rbtree_node_t *n)
{
    while (n->left != &(t->nil)) n = n->left;
    return n;
}

/*transplant*/
static inline void
rbtree_transplant(mln_rbtree_t *t, mln_rbtree_node_t *u, mln_rbtree_node_t *v)
{
    if (u->parent == &(t->nil)) t->root = v;
    else if (u == u->parent->left) u->parent->left = v;
    else u->parent->right = v;
    v->parent = u->parent;
}

/*rbtree_delete*/
void
mln_rbtree_delete(mln_rbtree_t *t, mln_rbtree_node_t *n)
{
    if (n == t->min)
        t->min = mln_rbtree_successor(t, n);
    enum rbtree_color y_original_color;
    mln_rbtree_node_t *x, *y;
    y = n;
    y_original_color = y->color;
    if (n->left == &(t->nil)) {
        x = n->right;
        rbtree_transplant(t, n, n->right);
    } else if (n->right == &(t->nil)) {
        x = n->left;
        rbtree_transplant(t, n, n->left);
    } else {
        y = rbtree_minimum(t, n->right);
        y_original_color = y->color;
        x = y->right;
        if (y->parent == n) x->parent = y;
        else {
            rbtree_transplant(t, y, y->right);
            y->right = n->right;
            y->right->parent = y;
        }
        rbtree_transplant(t, n, y);
        y->left = n->left;
        y->left->parent = y;
        y->color = n->color;
    }
    if (y_original_color == M_RB_BLACK) rbtree_delete_fixup(t, x);
    n->parent = n->left = n->right = &(t->nil);
    --(t->nr_node);
    if (t->iter != NULL && t->iter == n) {
        t->iter = n->next;
        t->del = 1;
    }
    mln_rbtree_chain_del(&(t->head), &(t->tail), n);
}

/*rbtree_delete_fixup*/
static inline void
rbtree_delete_fixup(mln_rbtree_t *t, mln_rbtree_node_t *n)
{
    mln_rbtree_node_t *tmp;
    while ((n != t->root) && (n->color == M_RB_BLACK)) {
        if (n == n->parent->left) {
            tmp = n->parent->right;
            if (tmp->color == M_RB_RED) {
                tmp->color = M_RB_BLACK;
                n->parent->color = M_RB_RED;
                mln_rbtree_left_rotate(t, n->parent);
                tmp = n->parent->right;
            }
            if ((tmp->left->color == M_RB_BLACK) && (tmp->right->color == M_RB_BLACK)) {
                tmp->color = M_RB_RED;
                n = n->parent;
                continue;
            } else if (tmp->right->color == M_RB_BLACK) {
                tmp->left->color = M_RB_BLACK;
                tmp->color = M_RB_RED;
                mln_rbtree_right_rotate(t, tmp);
                tmp = n->parent->right;
            }
            tmp->color = n->parent->color;
            n->parent->color = M_RB_BLACK;
            tmp->right->color = M_RB_BLACK;
            mln_rbtree_left_rotate(t, n->parent);
            n = t->root;
        } else {
            tmp = n->parent->left;
            if (tmp->color == M_RB_RED) {
                tmp->color = M_RB_BLACK;
                n->parent->color = M_RB_RED;
                mln_rbtree_right_rotate(t, n->parent);
                tmp = n->parent->left;
            }
            if ((tmp->right->color == M_RB_BLACK) && (tmp->left->color == M_RB_BLACK)) {
                tmp->color = M_RB_RED;
                n = n->parent;
                continue;
            } else if (tmp->left->color == M_RB_BLACK) {
                tmp->right->color = M_RB_BLACK;
                tmp->color = M_RB_RED;
                mln_rbtree_left_rotate(t, tmp);
                tmp = n->parent->left;
            }
            tmp->color = n->parent->color;
            n->parent->color = M_RB_BLACK;
            tmp->left->color = M_RB_BLACK;
            mln_rbtree_right_rotate(t, n->parent);
            n = t->root;
        }
    }
    n->color = M_RB_BLACK;
}

/*min*/
mln_rbtree_node_t *
mln_rbtree_min(mln_rbtree_t *t)
{
    return t->min;
}

/*iterate*/
int mln_rbtree_iterate(mln_rbtree_t *t, rbtree_iterate_handler handler, void *udata)
{
    for (t->iter = t->head; t->iter != NULL; ) {
        if (handler(t->iter, udata) < 0)
            return -1;
        if (t->del) {
            t->del = 0;
            continue;
        } else {
            t->iter = t->iter->next;
        }
    }
    return 0;
}

