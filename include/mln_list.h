
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

#define mln_list_null()         {NULL, NULL}
#define mln_list_init(s)        do { (s)->next = (s); (s)->prev = (s); } while (0)
#define mln_list_head(sentinel) \
    ((sentinel)->next == NULL || (sentinel)->next == (sentinel) ? NULL : (sentinel)->next)
#define mln_list_tail(sentinel) \
    ((sentinel)->prev == NULL || (sentinel)->prev == (sentinel) ? NULL : (sentinel)->prev)
#define mln_list_next(sentinel, node) \
    ((node)->next == (sentinel) ? NULL : (node)->next)
#define mln_list_prev(sentinel, node) \
    ((node)->prev == (sentinel) ? NULL : (node)->prev)

#define mln_list_for_each(node, sentinel) \
    for ((node) = (sentinel)->next; (node) != (sentinel); (node) = (node)->next)

#define mln_list_for_each_safe(node, tmp, sentinel) \
    for ((node) = (sentinel)->next, (tmp) = (node)->next; \
         (node) != (sentinel); \
         (node) = (tmp), (tmp) = (node)->next)

#define mln_list_add(_sentinel, _node) do { \
    mln_list_t *__s = (_sentinel), *__n = (_node); \
    if (__s->next == NULL) { __s->next = __s; __s->prev = __s; } \
    __n->next = __s; \
    __n->prev = __s->prev; \
    __s->prev->next = __n; \
    __s->prev = __n; \
} while (0)

#define mln_list_remove(_sentinel, _node) do { \
    mln_list_t *__n = (_node); \
    (void)(_sentinel); \
    __n->prev->next = __n->next; \
    __n->next->prev = __n->prev; \
    __n->prev = __n->next = NULL; \
} while (0)

#endif

