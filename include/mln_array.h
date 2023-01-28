
/*
 * Copyright (C) Niklaus F.Schen.
 */
#ifndef __MLN_ARRAY_H
#define __MLN_ARRAY_H

#include "mln_types.h"
#include "mln_defs.h"

typedef void *(*array_pool_alloc_handler)(void *, mln_size_t);
typedef void (*array_pool_free_handler)(void *);
typedef void (*array_free)(void *);

struct mln_array_attr {
    void                     *pool;
    array_pool_alloc_handler  pool_alloc;
    array_pool_free_handler   pool_free;
    array_free                free;
    mln_size_t                size;
    mln_size_t                nalloc;
};

typedef struct {
    void                     *elts;
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

extern int mln_array_init(mln_array_t *arr, struct mln_array_attr *attr) __NONNULL2(1,2);
extern mln_array_t *mln_array_new(struct mln_array_attr *attr) __NONNULL1(1);
extern void mln_array_destroy(mln_array_t *arr);
extern void mln_array_free(mln_array_t *arr);
extern void mln_array_reset(mln_array_t *arr);
extern void *mln_array_push(mln_array_t *arr) __NONNULL1(1);
extern void *mln_array_pushn(mln_array_t *arr, mln_size_t n) __NONNULL1(1);
extern void mln_array_pop(mln_array_t *arr);
#endif

