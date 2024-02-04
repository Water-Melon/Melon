
/*
 * Copyright (C) Niklaus F.Schen.
 */
#include "mln_array.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mln_func.h"

static inline int mln_array_alloc(mln_array_t *arr, mln_size_t n);

MLN_FUNC(, int, mln_array_init, (mln_array_t *arr, struct mln_array_attr *attr), (arr, attr), {
    arr->elts = NULL;
    arr->size = attr->size;
    arr->nalloc = attr->nalloc;
    arr->nelts = 0;
    arr->pool = attr->pool;
    arr->pool_alloc = attr->pool_alloc;
    arr->pool_free = attr->pool_free;
    arr->free = attr->free;
    return mln_array_alloc(arr, arr->nalloc);
})

MLN_FUNC(, mln_array_t *, mln_array_new, (struct mln_array_attr *attr), (attr), {
    mln_array_t *arr;

    if (attr->pool == NULL || attr->pool_alloc == NULL || attr->pool_free == NULL) {
        arr = (mln_array_t *)malloc(sizeof(mln_array_t));
        attr->pool = NULL;
    } else {
        arr = (mln_array_t *)attr->pool_alloc(attr->pool, sizeof(mln_array_t));
    }
    if (mln_array_init(arr, attr) < 0) {
        if (attr->pool == NULL || attr->pool_alloc == NULL || attr->pool_free == NULL)
            free(arr);
        else
            attr->pool_free(arr);
        return NULL;
    }
    return arr;
})

MLN_FUNC_VOID(, void, mln_array_destroy, (mln_array_t *arr), (arr), {
    if (arr == NULL) return;

    if (arr->free != NULL && arr->nelts) {
        mln_u8ptr_t p = arr->elts, pend = arr->elts + (arr->nelts * arr->size);
        for (; p < pend; p += arr->size)
            arr->free(p);
    }

    if (arr->pool != NULL)
        arr->pool_free(arr->elts);
    else
        free(arr->elts);
})

MLN_FUNC_VOID(, void, mln_array_free, (mln_array_t *arr), (arr), {
    if (arr == NULL) return;

    if (arr->free != NULL && arr->nelts) {
        mln_u8ptr_t p = arr->elts, pend = arr->elts + (arr->nelts * arr->size);
        for (; p < pend; p += arr->size)
            arr->free(p);
    }

    if (arr->pool != NULL) {
        arr->pool_free(arr->elts);
        arr->pool_free(arr);
    } else {
        free(arr->elts);
        free(arr);
    }
})

MLN_FUNC_VOID(, void, mln_array_reset, (mln_array_t *arr), (arr), {
    if (arr == NULL) return;

    if (arr->free != NULL && arr->nelts) {
        mln_u8ptr_t p = arr->elts, pend = arr->elts + (arr->nelts * arr->size);
        for (; p < pend; p += arr->size)
            arr->free(p);
    }
    arr->nelts = 0;
})

MLN_FUNC(, void *, mln_array_push, (mln_array_t *arr), (arr), {
    if (arr->nelts >= arr->nalloc) {
        if (mln_array_alloc(arr, 1) < 0)
            return NULL;
    }
    return arr->elts + (arr->nelts++) * arr->size;
})

MLN_FUNC(, void *, mln_array_pushn, (mln_array_t *arr, mln_size_t n), (arr, n), {
    mln_u8ptr_t ptr;

    if (arr->nelts + n > arr->nalloc) {
        if (mln_array_alloc(arr, n) < 0)
            return NULL;
    }
    ptr = arr->elts + arr->nelts * arr->size;
    arr->nelts += n;
    return ptr;
})

static inline int mln_array_alloc(mln_array_t *arr, mln_size_t n)
{
    mln_u8ptr_t ptr;
    mln_size_t num = arr->nalloc;
    while (n + arr->nelts > num) {
        num <<= 1;
    }
    if (arr->pool != NULL) {
        ptr = arr->pool_alloc(arr->pool, num * arr->size);
    } else {
        ptr = malloc(num * arr->size);
    }
    if (ptr == NULL)
        return -1;

    memcpy(ptr, arr->elts, arr->nelts * arr->size);
    if (arr->pool != NULL)
        arr->pool_free(arr->elts);
    else
        free(arr->elts);
    arr->elts = ptr;
    arr->nalloc = num;

    return 0;
}

MLN_FUNC_VOID(, void, mln_array_pop, (mln_array_t *arr), (arr), {
    if (arr == NULL || !arr->nelts)
        return;

    if (arr->free != NULL)
        arr->free(arr->elts + (arr->nelts - 1) * arr->size);
    --arr->nelts;
})

