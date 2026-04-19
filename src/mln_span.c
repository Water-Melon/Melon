
/*
 * Copyright (C) Niklaus F.Schen.
 */
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "mln_span.h"

mln_span_t *mln_span_root = NULL;
#if defined(MSVC)
DWORD mln_span_registered_thread;
#else
pthread_t mln_span_registered_thread;
#endif

/*
 * Chunked call-stack: each chunk holds a fixed-size array of span pointers.
 * Chunks are linked via ->next (pointing to the chunk below).
 * Exhausted chunks are moved to a free list for reuse, avoiding
 * any per-push/per-pop malloc/free and any realloc+copy overhead.
 */
#define MLN_SPAN_STACK_CHUNK_SIZE 512

typedef struct mln_span_stack_chunk_s {
    mln_span_t                       *entries[MLN_SPAN_STACK_CHUNK_SIZE];
    struct mln_span_stack_chunk_s    *next;
} mln_span_stack_chunk_t;

static mln_span_stack_chunk_t *__stack_cur = NULL;
static int __stack_pos = -1;
static mln_span_stack_chunk_t *__stack_free_chunks = NULL;

static inline mln_span_t *mln_span_stack_peek(void)
{
    if (__stack_cur == NULL || __stack_pos < 0) return NULL;
    return __stack_cur->entries[__stack_pos];
}

static inline int mln_span_stack_push(mln_span_t *span)
{
    if (__stack_cur == NULL || __stack_pos + 1 >= MLN_SPAN_STACK_CHUNK_SIZE) {
        mln_span_stack_chunk_t *chunk = __stack_free_chunks;
        if (chunk != NULL) {
            __stack_free_chunks = chunk->next;
        } else {
            chunk = (mln_span_stack_chunk_t *)malloc(sizeof(mln_span_stack_chunk_t));
            if (chunk == NULL) return -1;
        }
        chunk->next = __stack_cur;
        __stack_cur = chunk;
        __stack_pos = 0;
        chunk->entries[0] = span;
        return 0;
    }
    __stack_cur->entries[++__stack_pos] = span;
    return 0;
}

static inline mln_span_t *mln_span_stack_pop(void)
{
    mln_span_t *span;

    if (__stack_cur == NULL || __stack_pos < 0) return NULL;

    span = __stack_cur->entries[__stack_pos--];
    if (__stack_pos < 0 && __stack_cur->next != NULL) {
        mln_span_stack_chunk_t *old = __stack_cur;
        __stack_cur = old->next;
        old->next = __stack_free_chunks;
        __stack_free_chunks = old;
        __stack_pos = MLN_SPAN_STACK_CHUNK_SIZE - 1;
    }
    return span;
}

void mln_span_stack_free(void)
{
    while (__stack_cur != NULL) {
        mln_span_stack_chunk_t *prev = __stack_cur->next;
        __stack_cur->next = __stack_free_chunks;
        __stack_free_chunks = __stack_cur;
        __stack_cur = prev;
    }
    __stack_pos = -1;
}

/*
 * Chunked span pool: allocates mln_span_t objects in bulk (one malloc
 * per MLN_SPAN_POOL_CHUNK_SIZE spans).  Freed spans go onto a free-list
 * for O(1) reuse; the chunks themselves are freed only when
 * mln_span_pool_free() is called.
 */
#define MLN_SPAN_POOL_CHUNK_SIZE 512

typedef struct mln_span_pool_chunk_s {
    struct mln_span_pool_chunk_s *next;
    mln_span_t                    spans[MLN_SPAN_POOL_CHUNK_SIZE];
} mln_span_pool_chunk_t;

static mln_span_pool_chunk_t *__pool_head = NULL;
static int __pool_used = MLN_SPAN_POOL_CHUNK_SIZE;
static mln_span_t *__pool_free_list = NULL;

static inline mln_span_t *mln_span_pool_alloc(void)
{
    if (__pool_free_list != NULL) {
        mln_span_t *s = __pool_free_list;
        __pool_free_list = s->next;
        return s;
    }
    if (__pool_used >= MLN_SPAN_POOL_CHUNK_SIZE) {
        mln_span_pool_chunk_t *chunk;
        chunk = (mln_span_pool_chunk_t *)malloc(sizeof(mln_span_pool_chunk_t));
        if (chunk == NULL) return NULL;
        chunk->next = __pool_head;
        __pool_head = chunk;
        __pool_used = 0;
    }
    return &__pool_head->spans[__pool_used++];
}

static inline void mln_span_pool_recycle(mln_span_t *s)
{
    s->next = __pool_free_list;
    __pool_free_list = s;
}

MLN_FUNC_VOID(, void, mln_span_pool_free, (void), (), {
    mln_span_pool_chunk_t *pchunk;
    mln_span_stack_chunk_t *schunk;

    while ((pchunk = __pool_head) != NULL) {
        __pool_head = pchunk->next;
        free(pchunk);
    }
    __pool_used = MLN_SPAN_POOL_CHUNK_SIZE;
    __pool_free_list = NULL;

    while (__stack_cur != NULL) {
        schunk = __stack_cur;
        __stack_cur = schunk->next;
        free(schunk);
    }
    while ((schunk = __stack_free_chunks) != NULL) {
        __stack_free_chunks = schunk->next;
        free(schunk);
    }
    __stack_pos = -1;
})

/*
 * Subspan chain helper
 */
static inline void mln_span_chain_add(mln_span_t **head, mln_span_t **tail, mln_span_t *node)
{
    node->prev = *tail;
    node->next = NULL;
    if (*tail != NULL)
        (*tail)->next = node;
    else
        *head = node;
    *tail = node;
}

/*
 * Span creation / destruction
 */
mln_span_t *mln_span_new(mln_span_t *parent, const char *file, const char *func, int line)
{
    mln_span_t *s = mln_span_pool_alloc();
    if (s == NULL) return NULL;

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
    mln_span_t *cur, *child, *parent;
    int is_root;

    if (s == NULL) return;

    /* Iterative post-order traversal using the tree's own pointers */
    cur = s;
    for (;;) {
        if (cur->subspans_head != NULL) {
            child = cur->subspans_head;
            cur->subspans_head = child->next;
            if (child->next != NULL) child->next->prev = NULL;
            cur = child;
        } else {
            parent = cur->parent;
            is_root = (cur == s);
            mln_span_pool_recycle(cur);
            if (is_root) break;
            cur = parent;
        }
    }
}

/*
 * Entry / exit callbacks
 */
int mln_span_entry(void *fptr, const char *file, const char *func, int line, ...)
{
    mln_span_t *span;

#if defined(MSVC)
    if (mln_span_registered_thread != GetCurrentThreadId()) return 0;
#else
    if (!pthread_equal(mln_span_registered_thread, pthread_self())) return 0;
#endif
    if ((span = mln_span_new(mln_span_stack_peek(), file, func, line)) == NULL) {
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
    mln_span_t *span = mln_span_stack_pop();
    if (span == NULL) {
        assert(0);
        return;
    }
    gettimeofday(&span->end, NULL);
}

/*
 * Dump
 */
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
    mln_span_pool_free();
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
