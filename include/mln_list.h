
/*
 * Copyright (C) Niklaus F.Schen.
 */

#ifndef __MLN_LIST_H
#define __MLN_LIST_H

#include <stdio.h>

typedef struct mln_list_s {
    struct mln_list_s *next;
    struct mln_list_s *prev;
} mln_list_t;

#define mln_list_head(sentinel) ((sentinel)->prev)
#define mln_list_tail(sentinel) ((sentinel)->next)
#define mln_list_next(node)     ((node)->next)
#define mln_list_prev(node)     ((node)->prev)
#define mln_list_null()         {NULL, NULL}

extern void mln_list_add(mln_list_t *sentinel, mln_list_t *node);
extern void mln_list_remove(mln_list_t *sentinel, mln_list_t *node);

#endif

