
/*
 * Copyright (C) Niklaus F.Schen.
 */

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include"mln_rbtree.h"

MLN_CHAIN_FUNC_DECLARE(mln_rbtree, \
                       mln_rbtree_node_t, \
                       static inline void, \
                       __NONNULL3(1,2,3));
MLN_CHAIN_FUNC_DEFINE(mln_rbtree, \
                      mln_rbtree_node_t, \
                      static inline void, \
                      prev, \
                      next);

/*static declarations*/
static inline void
left_rotate(mln_rbtree_t *t, mln_rbtree_node_t *n) __NONNULL2(1,2);
static inline void
right_rotate(mln_rbtree_t *t, mln_rbtree_node_t *n) __NONNULL2(1,2);
static inline void
rbtree_insert_fixup(mln_rbtree_t *t, mln_rbtree_node_t *n) __NONNULL2(1,2);
static inline mln_rbtree_node_t *
rbtree_minimum(mln_rbtree_t *t, mln_rbtree_node_t *n) __NONNULL2(1,2);
static inline void
rbtree_transplant(mln_rbtree_t *t, mln_rbtree_node_t *u, mln_rbtree_node_t *v) __NONNULL3(1,2,3);
static inline void
rbtree_delete_fixup(mln_rbtree_t *t, mln_rbtree_node_t *n) __NONNULL2(1,2);

/*rbtree_init*/
mln_rbtree_t *
mln_rbtree_init(struct mln_rbtree_attr *attr)
{
    mln_rbtree_t *t;
    t = (mln_rbtree_t *)malloc(sizeof(mln_rbtree_t));
    if (t == NULL) return NULL;
    t->nil.data = NULL;
    t->nil.parent = &(t->nil);
    t->nil.left = &(t->nil);
    t->nil.right = &(t->nil);
    t->nil.color = M_RB_BLACK;
    t->root = &(t->nil);
    t->min = &(t->nil);
    t->head = t->tail = NULL;
    t->iter = NULL;
    t->cmp = attr->cmp;
    t->data_free = attr->data_free;
    t->nr_node = 0;
    t->del = 0;
    return t;
}

/*rbtree_destroy*/
void
mln_rbtree_destroy(mln_rbtree_t *t)
{
    if (t == NULL) return;
    mln_rbtree_node_t *fr;
    while ((fr = t->head) != NULL) {
        mln_rbtree_chain_del(&(t->head), &(t->tail), fr);
        mln_rbtree_free_node(t, fr);
    }
    free(t);
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

/*rbtree new node*/
mln_rbtree_node_t *
mln_rbtree_new_node(mln_rbtree_t *t, void *data)
{
    mln_rbtree_node_t *n;
    n = (mln_rbtree_node_t *)malloc(sizeof(mln_rbtree_node_t));
    if (n == NULL) return NULL;
    n->data = data;
    n->prev = n->next = NULL;
    n->parent = &(t->nil);
    n->left = &(t->nil);
    n->right = &(t->nil);
    return n;
}

/*rbtree free node*/
void
mln_rbtree_free_node(mln_rbtree_t *t, mln_rbtree_node_t *n)
{
    if (n->data != NULL && t->data_free != NULL)
        t->data_free(n->data);
    free(n);
}

/*Left rotate*/
static inline void
left_rotate(mln_rbtree_t *t, mln_rbtree_node_t *n)
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
right_rotate(mln_rbtree_t *t, mln_rbtree_node_t *n)
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

/*Insert*/
void
mln_rbtree_insert(mln_rbtree_t *t, mln_rbtree_node_t *n)
{
    mln_rbtree_node_t *y = &(t->nil);
    mln_rbtree_node_t *x = t->root;
    while (x != &(t->nil)) {
        y = x;
        if (t->cmp(n->data, x->data) < 0) x = x->left;
        else x = x->right;
    }
    n->parent = y;
    if (y == &(t->nil)) t->root = n;
    else if (t->cmp(n->data, y->data) < 0) y->left = n;
    else y->right = n;
    n->left = &(t->nil);
    n->right = &(t->nil);
    n->color = M_RB_RED;
    rbtree_insert_fixup(t, n);
    if (t->min == &(t->nil)) t->min = n;
    else if (t->cmp(n->data, t->min->data) < 0) t->min = n;
    ++(t->nr_node);
    mln_rbtree_chain_add(&(t->head), &(t->tail), n);
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
                left_rotate(t, n);
            }
            n->parent->color = M_RB_BLACK;
            n->parent->parent->color = M_RB_RED;
            right_rotate(t, n->parent->parent);
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
                right_rotate(t, n);
            }
            n->parent->color = M_RB_BLACK;
            n->parent->parent->color = M_RB_RED;
            left_rotate(t, n->parent->parent);
        }
    }
    t->root->color = M_RB_BLACK;
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
                left_rotate(t, n->parent);
                tmp = n->parent->right;
            }
            if ((tmp->left->color == M_RB_BLACK) && (tmp->right->color == M_RB_BLACK)) {
                tmp->color = M_RB_RED;
                n = n->parent;
                continue;
            } else if (tmp->right->color == M_RB_BLACK) {
                tmp->left->color = M_RB_BLACK;
                tmp->color = M_RB_RED;
                right_rotate(t, tmp);
                tmp = n->parent->right;
            }
            tmp->color = n->parent->color;
            n->parent->color = M_RB_BLACK;
            tmp->right->color = M_RB_BLACK;
            left_rotate(t, n->parent);
            n = t->root;
        } else {
            tmp = n->parent->left;
            if (tmp->color == M_RB_RED) {
                tmp->color = M_RB_BLACK;
                n->parent->color = M_RB_RED;
                right_rotate(t, n->parent);
                tmp = n->parent->left;
            }
            if ((tmp->right->color == M_RB_BLACK) && (tmp->left->color == M_RB_BLACK)) {
                tmp->color = M_RB_RED;
                n = n->parent;
                continue;
            } else if (tmp->left->color == M_RB_BLACK) {
                tmp->right->color = M_RB_BLACK;
                tmp->color = M_RB_RED;
                left_rotate(t, tmp);
                tmp = n->parent->left;
            }
            tmp->color = n->parent->color;
            n->parent->color = M_RB_BLACK;
            tmp->left->color = M_RB_BLACK;
            right_rotate(t, n->parent);
            n = t->root;
        }
    }
    n->color = M_RB_BLACK;
}

/*search*/
mln_rbtree_node_t *
mln_rbtree_search(mln_rbtree_t *t, mln_rbtree_node_t *root, const void *key)
{
    int ret;
    while ((root != &(t->nil)) && ((ret = t->cmp(key, root->data)) != 0)) {
        if (ret < 0) root = root->left;
        else root = root->right;
    }
    return root;
}

/*min*/
mln_rbtree_node_t *
mln_rbtree_min(mln_rbtree_t *t)
{
    return t->min;
}

/*scan_all*/
int mln_rbtree_scan_all(mln_rbtree_t *t, rbtree_act act, void *udata)
{
    for (t->iter = t->head; t->iter != NULL; ) {
        if (act(t->iter, t->iter->data, udata) < 0)
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

