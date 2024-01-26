
/*
 * Copyright (C) Niklaus F.Schen.
 */
#include <stdlib.h>
#include <string.h>
#include "mln_span.h"
#include "mln_log.h"

mln_stack_t *mln_span_callstack = NULL;
mln_span_t *mln_span_root = NULL;
#if defined(WIN32)
DWORD mln_span_registered_thread;
#else
pthread_t mln_span_registered_thread;
#endif

mln_span_t *mln_span_new(mln_span_t *parent, const char *file, const char *func, int line)
{
    mln_span_t *s;
    struct mln_array_attr attr;

    if (parent != NULL) {
        s = (mln_span_t *)mln_array_push(&parent->subspans);
    } else {
        s = (mln_span_t *)malloc(sizeof(mln_span_t));
    }
    if (s == NULL) return NULL;

    memset(&s->begin, 0, sizeof(struct timeval));
    memset(&s->end, 0, sizeof(struct timeval));
    s->file = file;
    s->func = func;
    s->line = line;
    attr.pool = NULL;
    attr.pool_alloc = NULL;
    attr.pool_free = NULL;
    attr.free = (array_free)mln_span_free;
    attr.size = sizeof(mln_span_t);
    attr.nalloc = 7;
    if (mln_array_init(&s->subspans, &attr) < 0) {
        if (parent == NULL) free(s);
        return NULL;
    }
    s->parent = parent;
    return s;
}

void mln_span_free(mln_span_t *s)
{
    if (s == NULL) return;
    mln_array_destroy(&s->subspans);
    if (s->parent == NULL) free(s);
}

void mln_span_entry(const char *file, const char *func, int line)
{
    mln_span_t *span;

#if defined(WIN32)
    if (mln_span_registered_thread != GetCurrentThreadId()) return;
#else
    if (!pthread_equal(mln_span_registered_thread, pthread_self())) return;
#endif
    if ((span = mln_span_new(mln_stack_top(mln_span_callstack), file, func, line)) == NULL) {
        mln_log(warn, "new span failed\n");
        return;
    }
    if (mln_stack_push(mln_span_callstack, span) < 0) {
        mln_log(warn, "push span failed\n");
        return;
    }
    if (mln_span_root == NULL) mln_span_root = span;
    gettimeofday(&span->begin, NULL);
}

void mln_span_exit(const char *file, const char *func, int line)
{
#if defined(WIN32)
    if (mln_span_registered_thread != GetCurrentThreadId()) return;
#else
    if (!pthread_equal(mln_span_registered_thread, pthread_self())) return;
#endif
    mln_span_t *span = mln_stack_pop(mln_span_callstack);
    if (span == NULL) {
        mln_log(warn, "call stack crashed\n");
        return;
    }
    gettimeofday(&span->end, NULL);
}

static void mln_span_format_dump(mln_span_t *span, int blanks)
{
    int i;
    mln_span_t *sub;

    for (i = 0; i < blanks; ++i)
        mln_log(none, " ");
    mln_log(none, "| %s at %s:%d takes %U (us)\n", \
           span->func, span->file, span->line, \
           (span->end.tv_sec * 1000000 + span->end.tv_usec) - (span->begin.tv_sec * 1000000 + span->begin.tv_usec));

    for (i = 0; i < mln_array_nelts(&(span->subspans)); ++i) {
        sub = ((mln_span_t *)mln_array_elts(&(span->subspans))) + i;
        mln_span_format_dump(sub, blanks + 2);
    }
}

void mln_span_dump(void)
{
    if (mln_span_root != NULL)
        mln_span_format_dump(mln_span_root, 0);
}

