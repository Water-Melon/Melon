
/*
 * Copyright (C) Niklaus F.Schen.
 */

#ifndef __MLN_STACK_H
#define __MLN_STACK_H

#include "mln_types.h"
#include <stdlib.h>
#include <string.h>

typedef void (*stack_free)(void *);
typedef void *(*stack_copy)(void *, void *);
typedef int (*stack_iterate_handler)(void *, void *);

typedef struct {
    void              **buf;
    mln_uauto_t         nr_node;
    mln_uauto_t         cap;
    stack_free           free_handler;
    stack_copy           copy_handler;
} mln_stack_t;

#define mln_stack_empty(s) (!(s)->nr_node)
#define mln_stack_top(st) ((st)->nr_node ? (st)->buf[(st)->nr_node - 1] : NULL)
extern mln_stack_t *
mln_stack_init(stack_free free_handler, stack_copy copy_handler);
extern void
mln_stack_destroy(mln_stack_t *st);
extern int
mln_stack_push(mln_stack_t *st, void *data) __NONNULL2(1,2);
extern int mln_stack_grow(mln_stack_t *st) __NONNULL1(1);
extern void *mln_stack_pop(mln_stack_t *st) __NONNULL1(1);
/*
 * mln_stack_dup():should be attention memory leak.
 */
extern mln_stack_t *mln_stack_dup(mln_stack_t *st, void *udata) __NONNULL1(1);
extern int mln_stack_iterate(mln_stack_t *st, stack_iterate_handler handler, void *data) __NONNULL1(1);

#ifndef MLN_FUNC_FLAG
#define mln_stack_push(st, d) ({\
    mln_stack_t *_ms = (st);\
    int _mr = 0;\
    if (_ms->nr_node >= _ms->cap)\
        _mr = mln_stack_grow(_ms);\
    if (_mr == 0)\
        _ms->buf[_ms->nr_node++] = (d);\
    _mr;\
})

#define mln_stack_pop(st) ({\
    mln_stack_t *_ms = (st);\
    _ms->nr_node ? _ms->buf[--_ms->nr_node] : NULL;\
})
#endif

#endif

