
/*
 * Copyright (C) Niklaus F.Schen.
 */
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "mln_span.h"

mln_span_stack_node_t *__mln_span_stack_top = NULL;
mln_span_stack_node_t *__mln_span_stack_bottom = NULL;
mln_span_t *mln_span_root = NULL;
#if defined(MSVC)
DWORD mln_span_registered_thread;
#else
pthread_t mln_span_registered_thread;
#endif

static inline void mln_span_chain_add(mln_span_t **head, mln_span_t **tail, mln_span_t *node)
{
    if (head == NULL || tail == NULL || node == NULL) return;
    node->prev = node->next = NULL;
    if (*head == NULL) {
        *head = *tail = node;
        return;
    }
    (*tail)->next = node;
    node->prev = (*tail);
    *tail = node;
}

static inline void mln_span_chain_del(mln_span_t **head, mln_span_t **tail, mln_span_t *node)
{
    if (head == NULL || tail == NULL || node == NULL) return;
    if (*head == node) {
        if (*tail == node) {
            *head = *tail = NULL;
        } else {
            *head = node->next;
            (*head)->prev = NULL;
        }
    } else {
        if (*tail == node) {
            *tail = node->prev;
            (*tail)->next = NULL;
        } else {
            node->prev->next = node->next;
            node->next->prev = node->prev;
        }
    }
    node->prev = node->next = NULL;
}

/*
 * callstack
 */
static void mln_span_stack_node_free(mln_span_stack_node_t *s)
{
    if (s == NULL) return;
    free(s);
}

static mln_span_t *mln_span_stack_top(void)
{
    return __mln_span_stack_top == NULL? NULL: __mln_span_stack_top->span;
}

static int mln_span_stack_push(mln_span_t *span)
{
    mln_span_stack_node_t *s;

    if ((s = (mln_span_stack_node_t *)malloc(sizeof(mln_span_stack_node_t))) == NULL) {
        return -1;
    }
    s->span = span;
    s->next = __mln_span_stack_top;
    if (__mln_span_stack_top == NULL) __mln_span_stack_bottom = s;
    __mln_span_stack_top = s;
    return 0;
}

static mln_span_t *mln_span_stack_pop(void)
{
    mln_span_stack_node_t *s;
    if ((s = __mln_span_stack_top) == NULL) return NULL;
    mln_span_t *span = s->span;
    if (__mln_span_stack_bottom == __mln_span_stack_top)
        __mln_span_stack_top = __mln_span_stack_bottom = NULL;
    else
        __mln_span_stack_top = s->next;
    mln_span_stack_node_free(s);
    return span;
}

void mln_span_stack_free(void)
{
    mln_span_stack_node_t *s;

    while ((s = __mln_span_stack_top) != NULL) {
        if (__mln_span_stack_bottom == __mln_span_stack_top)
            __mln_span_stack_top = __mln_span_stack_bottom = NULL;
        else
            __mln_span_stack_top = s->next;
        mln_span_stack_node_free(s);
    }
}

/*
 * subspan
 */

/*
 * span
 */
mln_span_t *mln_span_new(mln_span_t *parent, const char *file, const char *func, int line)
{
    mln_span_t *s;

    if ((s = (mln_span_t *)malloc(sizeof(mln_span_t))) == NULL)
        return NULL;

    memset(&s->begin, 0, sizeof(struct timeval));
    memset(&s->end, 0, sizeof(struct timeval));
    s->file = file;
    s->func = func;
    s->line = line;
    s->subspans_head = s->subspans_tail = NULL;
    s->prev = s->next = NULL;

    s->parent = parent;
    if (parent != NULL) {
        mln_span_chain_add(&parent->subspans_head, &parent->subspans_tail, s);
    }
    return s;
}

void mln_span_free(mln_span_t *s)
{
    if (s == NULL) return;
    mln_span_t *sub;
    while ((sub = s->subspans_head) != NULL) {
        mln_span_chain_del(&s->subspans_head, &s->subspans_tail, sub);
        mln_span_free(sub);
    }
    free(s);
}

int mln_span_entry(void *fptr, const char *file, const char *func, int line, ...)
{
    mln_span_t *span;

#if defined(MSVC)
    if (mln_span_registered_thread != GetCurrentThreadId()) return 0;
#else
    if (!pthread_equal(mln_span_registered_thread, pthread_self())) return 0;
#endif
    if ((span = mln_span_new(mln_span_stack_top(), file, func, line)) == NULL) {
        assert(0);
        return 0;
    }
    if (mln_span_stack_push(span) < 0) {
        assert(0);
        return 0;
    }
    if (mln_span_root == NULL) mln_span_root = span;
    gettimeofday(&span->begin, NULL);

    return 0;
}

void mln_span_exit(void *fptr, const char *file, const char *func, int line, void *ret, ...)
{
#if defined(MSVC)
    if (mln_span_registered_thread != GetCurrentThreadId()) return;
#else
    if (!pthread_equal(mln_span_registered_thread, pthread_self())) return;
#endif
    mln_span_t *span = (mln_span_t *)mln_span_stack_pop();
    if (span == NULL) {
        assert(0);
        return;
    }
    gettimeofday(&span->end, NULL);
}

static void __mln_span_dump(mln_span_t *s, mln_span_dump_cb_t cb, void *data, int level)
{
    if (s == NULL || cb == NULL) return;

    mln_span_t *sub;

    cb(s, level, data);
    for (sub = s->subspans_head; sub != NULL; sub = sub->next) {
        __mln_span_dump(sub, cb, data, level + 1);
    }
}

void mln_span_dump(mln_span_dump_cb_t cb, void *data)
{
    __mln_span_dump(mln_span_root, cb, data, 0);
}

#if defined(MSVC)
void mln_span_release(void)
{
    mln_span_free(mln_span_root);
    mln_span_root = NULL;
}

mln_span_t *mln_span_move(void)
{
    mln_span_t *span = mln_span_root;
    mln_span_root = NULL;
    return span;
}

mln_u64_t mln_span_time_cost(mln_span_t *s)
{
    return (mln_u64_t)(s->end.tv_sec * 1000000 + s->end.tv_usec) - (s->begin.tv_sec * 1000000 + s->begin.tv_usec);
}
#endif

