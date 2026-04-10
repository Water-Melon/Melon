
/*
 * Copyright (C) Niklaus F.Schen.
 */
#ifndef __MLN_ARRAY_H
#define __MLN_ARRAY_H

#include "mln_types.h"
#include "mln_utils.h"
#include <stdlib.h>
#include <string.h>

typedef void *(*array_pool_alloc_handler)(void *, mln_size_t);
typedef void (*array_pool_free_handler)(void *);
typedef void (*array_free)(void *);

typedef struct {
#if defined(MSVC)
    mln_u8ptr_t               elts;
#else
    void                     *elts;
#endif
    mln_size_t                size;
    mln_size_t                nalloc;
    mln_size_t                nelts;
    void                     *pool;
    array_pool_alloc_handler  pool_alloc;
    array_pool_free_handler   pool_free;
    array_free                free;
} mln_array_t;

#define mln_array_elts(arr)    ((arr)->elts)
#define mln_array_nelts(arr)   ((arr)->nelts)

extern int mln_array_init(mln_array_t *arr, array_free free, mln_size_t size, mln_size_t nalloc);
extern int mln_array_pool_init(mln_array_t *arr, \
                               array_free free, \
                               mln_size_t size, \
                               mln_size_t nalloc, \
                               void *pool, \
                               array_pool_alloc_handler pool_alloc, \
                               array_pool_free_handler pool_free);
extern mln_array_t *mln_array_new(array_free free, mln_size_t size, mln_size_t nalloc);
extern mln_array_t *mln_array_pool_new(array_free free, \
                                       mln_size_t size, \
                                       mln_size_t nalloc, \
                                       void *pool, \
                                       array_pool_alloc_handler pool_alloc, \
                                       array_pool_free_handler pool_free);
extern void mln_array_destroy(mln_array_t *arr);
extern void mln_array_free(mln_array_t *arr);
extern void mln_array_reset(mln_array_t *arr);
extern void *mln_array_push(mln_array_t *arr) __NONNULL1(1);
extern void *mln_array_pushn(mln_array_t *arr, mln_size_t n) __NONNULL1(1);
extern void mln_array_pop(mln_array_t *arr);
extern int mln_array_grow(mln_array_t *arr, mln_size_t n) __NONNULL1(1);

/*
 * Inline macros for hot-path operations.
 * These avoid function call overhead on the fast path (no reallocation needed).
 * Use MLN_ARRAY_PUSH/MLN_ARRAY_PUSHN/MLN_ARRAY_POP for maximum performance.
 */
#define MLN_ARRAY_PUSH(arr, ret_ptr) do { \
    mln_array_t *__arr = (arr); \
    if (__arr->nelts >= __arr->nalloc) { \
        if (mln_array_grow(__arr, 1) < 0) { (ret_ptr) = NULL; break; } \
    } \
    (ret_ptr) = (void *)((mln_u8ptr_t)__arr->elts + (__arr->nelts++) * __arr->size); \
} while (0)

#define MLN_ARRAY_PUSHN(arr, n, ret_ptr) do { \
    mln_array_t *__arr = (arr); \
    mln_size_t __n = (n); \
    if (__arr->nelts + __n > __arr->nalloc) { \
        if (mln_array_grow(__arr, __n) < 0) { (ret_ptr) = NULL; break; } \
    } \
    (ret_ptr) = (void *)((mln_u8ptr_t)__arr->elts + __arr->nelts * __arr->size); \
    __arr->nelts += __n; \
} while (0)

#define MLN_ARRAY_POP(arr) do { \
    mln_array_t *__arr = (arr); \
    if (__arr != NULL && __arr->nelts) { \
        if (__arr->free != NULL) \
            __arr->free((void *)((mln_u8ptr_t)__arr->elts + (__arr->nelts - 1) * __arr->size)); \
        --__arr->nelts; \
    } \
} while (0)

#endif

