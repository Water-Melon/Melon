
/*
 * Copyright (C) Niklaus F.Schen.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mln_fheap.h"

static inline mln_fheap_node_t *
mln_fheap_remove_child(mln_fheap_node_t **root);
static inline void
mln_fheap_add_child(mln_fheap_node_t **root, mln_fheap_node_t *node);
static inline void
mln_fheap_del_child(mln_fheap_node_t **root, mln_fheap_node_t *node);

static inline void
mln_fheap_consolidate(mln_fheap_t *fh);
static inline void
mln_fheap_link(mln_fheap_t *fh, mln_fheap_node_t *y, mln_fheap_node_t *x);
static inline void
mln_fheap_cut(mln_fheap_t *fh, mln_fheap_node_t *x, mln_fheap_node_t *y);
static inline void
mln_fheap_cascading_cut(mln_fheap_t *fh, mln_fheap_node_t *y);


mln_fheap_t *mln_fheap_init(struct mln_fheap_attr *attr)
{
    mln_fheap_t *fh = (mln_fheap_t *)malloc(sizeof(mln_fheap_t));
    if (fh == NULL) return NULL;
    fh->min_val = malloc(attr->min_val_size);
    if (fh->min_val == NULL) {
        free(fh);
        return NULL;
    }
    memcpy(fh->min_val, attr->min_val, attr->min_val_size);
    fh->cmp = attr->cmp;
    fh->copy = attr->copy;
    fh->key_free = attr->key_free;
    fh->min = NULL;
    fh->root_list = NULL;
    fh->num = 0;
    return fh;
}

void mln_fheap_insert(mln_fheap_t *fh, mln_fheap_node_t *fn)
{
    mln_fheap_add_child(&(fh->root_list), fn);
    fn->parent = NULL;
    if (fh->min == NULL) {
        fh->min = fn;
    } else {
        if (fh->cmp(fn->key, fh->min->key) < 0)
            fh->min = fn;
    }
    ++(fh->num);
}

mln_fheap_node_t *mln_fheap_minimum(mln_fheap_t *fh)
{
    return fh->min;
}

mln_fheap_node_t *mln_fheap_extract_min(mln_fheap_t *fh)
{
    mln_fheap_node_t *z = fh->min;
    if (z == NULL) return NULL;
    mln_fheap_node_t *child;
    while ((child = mln_fheap_remove_child(&(z->child))) != NULL) {
        mln_fheap_add_child(&(fh->root_list), child);
        child->parent = NULL;
    }
    mln_fheap_node_t *right = z->right;
    mln_fheap_del_child(&(fh->root_list), z);
    if (z == right) {
        fh->min = NULL;
    } else {
        fh->min = right;
        mln_fheap_consolidate(fh);
    }
    --(fh->num);
    return z;
}

static inline void
mln_fheap_consolidate(mln_fheap_t *fh)
{
    mln_fheap_node_t *array[FH_LGN];
    memset(array, 0, sizeof(mln_fheap_node_t *)*FH_LGN);
    mln_fheap_node_t *x, *y, *w, *tmp;
    mln_size_t d, mark = 0;
    for (w = fh->root_list; w != NULL && !(w == fh->root_list && mark);) {
        if (w == fh->root_list) ++mark;
        x = w;
        w = w->right;
        d = x->degree;
        while (array[d] != NULL) {
            y = array[d];
            if (fh->cmp(x->key, y->key) > 0) {
                tmp = x;
                x = y;
                y = tmp;
            }
            if (y == w) w = w->right;
            mln_fheap_link(fh, y, x);
            array[d] = NULL;
            ++d;
        }
        array[d] = x;
    }
    fh->min = NULL;
    mln_size_t i;
    mln_fheap_node_t *root_list = NULL;
    for (i = 0; i<FH_LGN; ++i) {
        if (array[i] == NULL) continue;
        mln_fheap_del_child(&(fh->root_list), array[i]);
        mln_fheap_add_child(&root_list, array[i]);
        array[i]->parent = NULL;
        if (fh->min == NULL) {
            fh->min = array[i];
        } else {
            if (fh->cmp(array[i]->key, fh->min->key) < 0)
                fh->min = array[i];
        }
    }
    fh->root_list = root_list;
}

static inline void
mln_fheap_link(mln_fheap_t *fh, mln_fheap_node_t *y, mln_fheap_node_t *x)
{
    mln_fheap_del_child(&(fh->root_list), y);
    mln_fheap_add_child(&(x->child), y);
    y->parent = x;
    ++(x->degree);
    y->mark = FHEAP_FALSE;
}

int mln_fheap_decrease_key(mln_fheap_t *fh, mln_fheap_node_t *node, void *key)
{
    if (fh->cmp(node->key, key) < 0) return -1;
    fh->copy(node->key, key);
    mln_fheap_node_t *y = node->parent;
    if (y != NULL && fh->cmp(node->key, y->key) < 0) {
        mln_fheap_cut(fh, node, y);
        mln_fheap_cascading_cut(fh, y);
    }
    if (node != fh->min && fh->cmp(node->key, fh->min->key) < 0)
        fh->min = node;
    return 0;
}

static inline void
mln_fheap_cut(mln_fheap_t *fh, mln_fheap_node_t *x, mln_fheap_node_t *y)
{
    mln_fheap_del_child(&(y->child), x);
    --(y->degree);
    mln_fheap_add_child(&(fh->root_list), x);
    x->parent = NULL;
    x->mark = FHEAP_FALSE;
}

static inline void
mln_fheap_cascading_cut(mln_fheap_t *fh, mln_fheap_node_t *y)
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
}

void mln_fheap_delete(mln_fheap_t *fh, mln_fheap_node_t *node)
{
    mln_fheap_decrease_key(fh, node, fh->min_val);
    mln_fheap_extract_min(fh);
}

void mln_fheap_destroy(mln_fheap_t *fh)
{
    if (fh == NULL) return;
    mln_fheap_node_t *fn;
    while ((fn = mln_fheap_extract_min(fh)) != NULL) {
        mln_fheap_node_destroy(fh, fn);
    }
    if (fh->min_val != NULL)
        free(fh->min_val);
    free(fh);
}

/*mln_fheap_node_t*/
mln_fheap_node_t *mln_fheap_node_init(mln_fheap_t *fh, void *key)
{
    mln_fheap_node_t *fn;
    fn = (mln_fheap_node_t *)malloc(sizeof(mln_fheap_node_t));
    if (fn == NULL) return NULL;
    fn->key = key;
    fn->parent = NULL;
    fn->child = NULL;
    fn->left = fn;
    fn->right = fn;
    fn->degree = 0;
    fn->mark = FHEAP_FALSE;
    return fn;
}

void mln_fheap_node_destroy(mln_fheap_t *fh, mln_fheap_node_t *fn)
{
    if (fn == NULL) return;
    if (fh->key_free != NULL && fn->key != NULL)
        fh->key_free(fn->key);
    free(fn);
}

/*chain*/
static inline mln_fheap_node_t *
mln_fheap_remove_child(mln_fheap_node_t **root)
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
}

static inline void
mln_fheap_add_child(mln_fheap_node_t **root, mln_fheap_node_t *node)
{
    if (*root == NULL) {
        *root = node;
        return;
    }
    node->right = *root;
    node->left = (*root)->left;
    (*root)->left = node;
    node->left->right = node;
}

static inline void
mln_fheap_del_child(mln_fheap_node_t **root, mln_fheap_node_t *node)
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
 //       if (node->right == node) abort();
        node->right->left = node->left;
        node->left->right = node->right;
    }
    node->right = node->left = node;
}

