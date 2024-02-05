
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

MLN_FUNC_VOID(, void, mln_fheap_insert, (mln_fheap_t *fh, mln_fheap_node_t *fn), (fh, fn), {
    mln_fheap_inline_insert(fh, fn, NULL);
})

MLN_FUNC(, mln_fheap_node_t *, mln_fheap_extract_min, (mln_fheap_t *fh), (fh), {
    return mln_fheap_inline_extract_min(fh, NULL);
})

MLN_FUNC(, int, mln_fheap_decrease_key, (mln_fheap_t *fh, mln_fheap_node_t *node, void *key), (fh, node, key), {
    return mln_fheap_inline_decrease_key(fh, node, key, NULL, NULL);
})

MLN_FUNC_VOID(, void, mln_fheap_delete, (mln_fheap_t *fh, mln_fheap_node_t *node), (fh, node), {
    mln_fheap_inline_delete(fh, node, NULL, NULL);
})

MLN_FUNC_VOID(, void, mln_fheap_free, (mln_fheap_t *fh), (fh), {
    mln_fheap_inline_free(fh, NULL, NULL);
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
    mln_fheap_inline_node_free(fh, fn, NULL);
})

