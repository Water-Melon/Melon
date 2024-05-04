
/*
 * Copyright (C) Niklaus F.Schen.
 */

#include <stdio.h>
#include <stdlib.h>
#include "mln_fheap.h"

static inline void
mln_fheap_cut(mln_fheap_t *fh, mln_fheap_node_t *x, mln_fheap_node_t *y);
static inline void
mln_fheap_cascading_cut(mln_fheap_t *fh, mln_fheap_node_t *y);


MLN_FUNC(, mln_fheap_t *, mln_fheap_new, (void *min_val, struct mln_fheap_attr *attr), (min_val, attr), {
    mln_fheap_t *fh;
    if (attr != NULL && attr->pool != NULL)
        fh = (mln_fheap_t *)attr->pool_alloc(attr->pool, sizeof(mln_fheap_t));
    else
        fh = (mln_fheap_t *)malloc(sizeof(mln_fheap_t));
    if (fh == NULL) return NULL;

    if (attr != NULL) {
        fh->pool = attr->pool;
        fh->pool_alloc = attr->pool_alloc;
        fh->pool_free = attr->pool_free;
        fh->cmp = attr->cmp;
        fh->copy = attr->copy;
        fh->key_free = attr->key_free;
    } else {
        fh->pool = NULL;
        fh->pool_alloc = NULL;
        fh->pool_free = NULL;
        fh->cmp = NULL;
        fh->copy = NULL;
        fh->key_free = NULL;
    }
    fh->min_val = min_val;
    fh->min = NULL;
    fh->root_list = NULL;
    fh->num = 0;
    return fh;
})

MLN_FUNC(, mln_fheap_t *, mln_fheap_new_fast, \
         (void *min_val, fheap_cmp cmp, fheap_copy copy, fheap_key_free key_free), \
         (min_val, cmp, copy, key_free), \
{
    mln_fheap_t *fh;

    fh = (mln_fheap_t *)malloc(sizeof(mln_fheap_t));
    if (fh == NULL) return NULL;

    fh->pool = NULL;
    fh->pool_alloc = NULL;
    fh->pool_free = NULL;
    fh->cmp = cmp;
    fh->copy = copy;
    fh->key_free = key_free;
    fh->min_val = min_val;
    fh->min = NULL;
    fh->root_list = NULL;
    fh->num = 0;
    return fh;
})

MLN_FUNC_VOID(, void, mln_fheap_insert, (mln_fheap_t *fh, mln_fheap_node_t *fn), (fh, fn), {
    mln_fheap_add_child(&(fh->root_list), fn);
    fn->parent = NULL;
    if (fh->min == NULL) {
        fh->min = fn;
    } else {
        if (!fh->cmp(fn->key, fh->min->key))
            fh->min = fn;
    }
    ++(fh->num);
})

MLN_FUNC(, mln_fheap_node_t *, mln_fheap_extract_min, (mln_fheap_t *fh), (fh), {
    mln_fheap_node_t *z = fh->min;
    if (z != NULL) {
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
            mln_fheap_consolidate(fh, fh->cmp);
        }
        --((fh)->num);
    }
    return z;
})

MLN_FUNC(, int, mln_fheap_decrease_key, (mln_fheap_t *fh, mln_fheap_node_t *node, void *key), (fh, node, key), {
    if (!fh->cmp(node->key, key)) {
        return -1;
    }
    fh->copy(node->key, key);
    mln_fheap_node_t *y = node->parent;
    if (y != NULL && !fh->cmp(node->key, y->key)) {
        mln_fheap_cut(fh, node, y);
        mln_fheap_cascading_cut(fh, y);
    }
    if (node != fh->min && !fh->cmp(node->key, fh->min->key))
        fh->min = node;
    return 0;
})

MLN_FUNC_VOID(, void, mln_fheap_delete, (mln_fheap_t *fh, mln_fheap_node_t *node), (fh, node), {
    mln_fheap_decrease_key(fh, node, fh->min_val);
    mln_fheap_extract_min(fh);
})

MLN_FUNC_VOID(, void, mln_fheap_free, (mln_fheap_t *fh), (fh), {
    if (fh == NULL) return;

    mln_fheap_node_t *fn;
    while ((fn = mln_fheap_extract_min(fh)) != NULL) {
        mln_fheap_node_free(fh, fn);
    }
    if (fh->pool != NULL) fh->pool_free(fh);
    else free(fh);
})

/*mln_fheap_node_t*/
MLN_FUNC(, mln_fheap_node_t *, mln_fheap_node_new, (mln_fheap_t *fh, void *key), (fh, key), {
    mln_fheap_node_t *fn;

    if (fh->pool != NULL)
        fn = (mln_fheap_node_t *)fh->pool_alloc(fh->pool, sizeof(mln_fheap_node_t));
    else
        fn = (mln_fheap_node_t *)malloc(sizeof(mln_fheap_node_t));
    if (fn == NULL) return NULL;

    fn->key = key;
    fn->parent = NULL;
    fn->child = NULL;
    fn->left = fn;
    fn->right = fn;
    fn->degree = 0;
    fn->nofree = 0;
    fn->mark = FHEAP_FALSE;
    return fn;
})

MLN_FUNC_VOID(, void, mln_fheap_node_free, (mln_fheap_t *fh, mln_fheap_node_t *fn), (fh, fn), {
    if (fn == NULL) return;

    if (fh->key_free != NULL && fn->key != NULL)
        fh->key_free(fn->key);
    if (!fn->nofree) {
       if (fh->pool != NULL) fh->pool_free(fn);
       else free(fn);
    }
})

#if defined(MSVC)
mln_fheap_node_t *mln_fheap_node_init(mln_fheap_node_t *fn, void *k)
{
    fn->key = k;
    fn->parent = NULL;
    fn->child = NULL;
    fn->right = fn->left = fn;
    fn->degree = 0;
    fn->nofree = 1;
    fn->mark = FHEAP_FALSE;
    return fn;
}
#endif

