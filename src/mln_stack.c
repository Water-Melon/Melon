
/*
 * Copyright (C) Niklaus F.Schen.
 */

#include <stdlib.h>
#include <string.h>
#include "mln_stack.h"
#undef mln_stack_push
#undef mln_stack_pop
#include "mln_func.h"

#define MLN_STACK_INIT_CAP 8

static inline mln_uauto_t mln_stack_roundup_pow2(mln_uauto_t n)
{
    if (n == 0) return 1;
    --n;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    if (sizeof(mln_uauto_t) > 4)
        n |= n >> 32;
    return n + 1;
}

/*
 * stack
 */
MLN_FUNC(, mln_stack_t *, mln_stack_init, \
         (stack_free free_handler, stack_copy copy_handler), \
         (free_handler, copy_handler), \
{
    mln_stack_t *st = (mln_stack_t *)malloc(sizeof(mln_stack_t));
    if (st == NULL) return NULL;
    st->cap = MLN_STACK_INIT_CAP;
    st->buf = (void **)malloc(st->cap * sizeof(void *));
    if (st->buf == NULL) {
        free(st);
        return NULL;
    }
    st->nr_node = 0;
    st->free_handler = free_handler;
    st->copy_handler = copy_handler;
    return st;
})

MLN_FUNC_VOID(, void, mln_stack_destroy, (mln_stack_t *st), (st), {
    if (st == NULL) return;
    if (st->free_handler != NULL) {
        mln_uauto_t i;
        for (i = 0; i < st->nr_node; ++i) {
            st->free_handler(st->buf[i]);
        }
    }
    if (st->buf != NULL)
        free(st->buf);
    free(st);
})

/*
 * grow
 */
MLN_FUNC(, int, mln_stack_grow, (mln_stack_t *st), (st), {
    mln_uauto_t new_cap = mln_stack_roundup_pow2(st->cap + 1);
    void **new_buf = (void **)realloc(st->buf, new_cap * sizeof(void *));
    if (new_buf == NULL) return -1;
    st->buf = new_buf;
    st->cap = new_cap;
    return 0;
})

/*
 * push
 */
MLN_FUNC(, int, mln_stack_push, (mln_stack_t *st, void *data), (st, data), {
    if (st->nr_node >= st->cap) {
        if (mln_stack_grow(st) < 0) return -1;
    }
    st->buf[st->nr_node++] = data;
    return 0;
})

/*
 * pop
 */
MLN_FUNC(, void *, mln_stack_pop, (mln_stack_t *st), (st), {
    if (!st->nr_node) return NULL;
    return st->buf[--st->nr_node];
})

/*
 * dup
 */
MLN_FUNC(, mln_stack_t *, mln_stack_dup, (mln_stack_t *st, void *udata), (st, udata), {
    mln_stack_t *new_st = mln_stack_init(st->free_handler, st->copy_handler);
    if (new_st == NULL) return NULL;
    mln_uauto_t i;
    void *data;
    for (i = 0; i < st->nr_node; ++i) {
        if (new_st->copy_handler == NULL) {
            data = st->buf[i];
        } else {
            data = new_st->copy_handler(st->buf[i], udata);
            if (data == NULL) {
                mln_stack_destroy(new_st);
                return NULL;
            }
        }
        if (mln_stack_push(new_st, data) < 0) {
            if (new_st->free_handler != NULL)
                new_st->free_handler(data);
            mln_stack_destroy(new_st);
            return NULL;
        }
    }
    return new_st;
})

/*
 * scan
 */
MLN_FUNC(, int, mln_stack_iterate, \
         (mln_stack_t *st, stack_iterate_handler handler, void *data), \
         (st, handler, data), \
{
    mln_sauto_t i;
    for (i = (mln_sauto_t)st->nr_node - 1; i >= 0; --i) {
        if (handler(st->buf[i], data) < 0) return -1;
    }
    return 0;
})

