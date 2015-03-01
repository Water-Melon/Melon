
/*
 * Copyright (C) Niklaus F.Schen.
 */

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include"mln_sa_rbtree.h"

/*static declarations*/
static inline void
left_rotate(mln_sarbt_t *t, mln_sarbt_node_t *n) __NONNULL2(1,2);
static inline void
right_rotate(mln_sarbt_t *t, mln_sarbt_node_t *n) __NONNULL2(1,2);
static inline void
rbtree_insert_fixup(mln_sarbt_t *t, mln_sarbt_node_t *n) __NONNULL2(1,2);
static inline mln_sarbt_node_t *
rbtree_minimum(mln_sarbt_t *t, mln_sarbt_node_t *n) __NONNULL2(1,2);
static inline void
rbtree_transplant(mln_sarbt_t *t, mln_sarbt_node_t *u, mln_sarbt_node_t *v) __NONNULL3(1,2,3);
static inline void
rbtree_delete_fixup(mln_sarbt_t *t, mln_sarbt_node_t *n) __NONNULL2(1,2);

/*rbtree successor*/
mln_sarbt_node_t *
mln_sarbt_successor(mln_sarbt_t *t, mln_sarbt_node_t *n)
{
    if (n != &(t->nil) && n->right != &(t->nil))
        return rbtree_minimum(t, n->right);
    mln_sarbt_node_t *tmp = n->parent;
    while (tmp!=&(t->nil) && n==tmp->right) {
        n = tmp; tmp = tmp->parent;
    }
    return tmp;
}

/*Left rotate*/
static inline void
left_rotate(mln_sarbt_t *t, mln_sarbt_node_t *n)
{
    if (n->right == &(t->nil)) return;
    mln_sarbt_node_t *tmp = n->right;
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
right_rotate(mln_sarbt_t *t, mln_sarbt_node_t *n)
{
    if (n->left == &(t->nil)) return;
    mln_sarbt_node_t *tmp = n->left;
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
mln_sarbt_insert(mln_sarbt_t *t, mln_sarbt_node_t *n)
{
    mln_sarbt_node_t *y = &(t->nil);
    mln_sarbt_node_t *x = t->root;
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
    n->color = M_SARB_RED;
    rbtree_insert_fixup(t, n);
}

/*insert fixup*/
static inline void
rbtree_insert_fixup(mln_sarbt_t *t, mln_sarbt_node_t *n)
{
    mln_sarbt_node_t *tmp;
    while (n->parent->color == M_SARB_RED) {
        if (n->parent == n->parent->parent->left) {
            tmp = n->parent->parent->right;
            if (tmp->color == M_SARB_RED) {
                n->parent->color = M_SARB_BLACK;
                tmp->color = M_SARB_BLACK;
                n->parent->parent->color = M_SARB_RED;
                n = n->parent->parent;
                continue;
            } else if (n == n->parent->right) {
                n = n->parent;
                left_rotate(t, n);
            }
            n->parent->color = M_SARB_BLACK;
            n->parent->parent->color = M_SARB_RED;
            right_rotate(t, n->parent->parent);
        } else {
            tmp = n->parent->parent->left;
            if (tmp->color == M_SARB_RED) {
                n->parent->color = M_SARB_BLACK;
                tmp->color = M_SARB_BLACK;
                n->parent->parent->color = M_SARB_RED;
                n = n->parent->parent;
                continue;
            } else if (n == n->parent->left) {
                n = n->parent;
                right_rotate(t, n);
            }
            n->parent->color = M_SARB_BLACK;
            n->parent->parent->color = M_SARB_RED;
            left_rotate(t, n->parent->parent);
        }
    }
    t->root->color = M_SARB_BLACK;
}

/*Tree Minimum*/
static inline mln_sarbt_node_t *
rbtree_minimum(mln_sarbt_t *t, mln_sarbt_node_t *n)
{
    while (n->left != &(t->nil)) n = n->left;
    return n;
}

/*Tree Maximum*/
void *mln_sarbt_maximum(mln_sarbt_t *t)
{
    mln_sarbt_node_t *n = t->root;
    while (n->right != &(t->nil)) n = n->right;
    return n->data;
}

/*transplant*/
static inline void
rbtree_transplant(mln_sarbt_t *t, mln_sarbt_node_t *u, mln_sarbt_node_t *v)
{
    if (u->parent == &(t->nil)) t->root = v;
    else if (u == u->parent->left) u->parent->left = v;
    else u->parent->right = v;
    v->parent = u->parent;
}

/*rbtree_delete*/
void
mln_sarbt_delete(mln_sarbt_t *t, mln_sarbt_node_t *n)
{
    enum sarbt_color y_original_color;
    mln_sarbt_node_t *x, *y;
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
    if (y_original_color == M_SARB_BLACK) rbtree_delete_fixup(t, x);
    n->parent = n->left = n->right = &(t->nil);
}

/*rbtree_delete_fixup*/
static inline void
rbtree_delete_fixup(mln_sarbt_t *t, mln_sarbt_node_t *n)
{
    mln_sarbt_node_t *tmp;
    while ((n != t->root) && (n->color == M_SARB_BLACK)) {
        if (n == n->parent->left) {
            tmp = n->parent->right;
            if (tmp->color == M_SARB_RED) {
                tmp->color = M_SARB_BLACK;
                n->parent->color = M_SARB_RED;
                left_rotate(t, n->parent);
                tmp = n->parent->right;
            }
            if ((tmp->left->color == M_SARB_BLACK) && (tmp->right->color == M_SARB_BLACK)) {
                tmp->color = M_SARB_RED;
                n = n->parent;
                continue;
            } else if (tmp->right->color == M_SARB_BLACK) {
                tmp->left->color = M_SARB_BLACK;
                tmp->color = M_SARB_RED;
                right_rotate(t, tmp);
                tmp = n->parent->right;
            }
            tmp->color = n->parent->color;
            n->parent->color = M_SARB_BLACK;
            tmp->right->color = M_SARB_BLACK;
            left_rotate(t, n->parent);
            n = t->root;
        } else {
            tmp = n->parent->left;
            if (tmp->color == M_SARB_RED) {
                tmp->color = M_SARB_BLACK;
                n->parent->color = M_SARB_RED;
                right_rotate(t, n->parent);
                tmp = n->parent->left;
            }
            if ((tmp->right->color == M_SARB_BLACK) && (tmp->left->color == M_SARB_BLACK)) {
                tmp->color = M_SARB_RED;
                n = n->parent;
                continue;
            } else if (tmp->left->color == M_SARB_BLACK) {
                tmp->right->color = M_SARB_BLACK;
                tmp->color = M_SARB_RED;
                left_rotate(t, tmp);
                tmp = n->parent->left;
            }
            tmp->color = n->parent->color;
            n->parent->color = M_SARB_BLACK;
            tmp->left->color = M_SARB_BLACK;
            right_rotate(t, n->parent);
            n = t->root;
        }
    }
    n->color = M_SARB_BLACK;
}

/*search*/
void *mln_sarbt_search(mln_sarbt_t *t, const void *key)
{
    int ret = 1;
    mln_sarbt_node_t *root = t->root, *save = &(t->nil);
    while ((root != &(t->nil)) && ((ret = t->cmp(key, root->data)) != 0)) {
        if (ret < 0) {
            save = root;
            root = root->left;
        } else {
            root = root->right;
        }
    }
    if (!ret) save = root;
    return save->data;
}

