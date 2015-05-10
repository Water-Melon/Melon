
/*
 * Copyright (C) Niklaus F.Schen.
 */

#ifndef __MLN_STACK_H
#define __MLN_STACK_H

#include "mln_types.h"

typedef void (*stack_free)(void *);
typedef void *(*stack_copy)(void *);

typedef struct mln_stack_node_s {
    void                    *data;
    struct mln_stack_node_s *prev;
    struct mln_stack_node_s *next;
} mln_stack_node_t;

typedef struct {
    mln_stack_node_t        *bottom;
    mln_stack_node_t        *top;
    mln_uauto_t              nr_node;
    stack_free               free_handler;
    stack_copy               copy_handler;
} mln_stack_t;

struct mln_stack_attr {
    stack_free               free_handler;
    stack_copy               copy_handler;
};


#define mln_stack_empty(s) (!(s)->nr_node)
extern mln_stack_t *
mln_stack_init(struct mln_stack_attr *attr) __NONNULL1(1);
extern void
mln_stack_destroy(mln_stack_t *st);
extern int
mln_stack_push(mln_stack_t *st, void *data) __NONNULL2(1,2);
extern void *mln_stack_pop(mln_stack_t *st) __NONNULL1(1);
extern void *mln_stack_top(mln_stack_t *st) __NONNULL1(1);
/*
 * mln_stack_dup():should be attention memory leak.
 */
extern mln_stack_t *mln_stack_dup(mln_stack_t *st) __NONNULL1(1);

#endif

