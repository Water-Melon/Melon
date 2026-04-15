
/*
 * Copyright (C) Niklaus F.Schen.
 */
#include "mln_queue.h"
#undef mln_queue_append
#undef mln_queue_get
#undef mln_queue_remove
#undef mln_queue_search
#include "mln_func.h"

static inline mln_uauto_t mln_queue_roundup_pow2(mln_uauto_t n)
{
    if (n == 0) return 1;
    --n;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
#if __SIZEOF_LONG__ > 4
    n |= n >> 32;
#endif
    return n + 1;
}

MLN_FUNC(, mln_queue_t *, mln_queue_init, \
         (mln_uauto_t qlen, queue_free free_handler), (qlen, free_handler), \
{
    mln_queue_t *q = (mln_queue_t *)malloc(sizeof(mln_queue_t));
    if (q == NULL) return NULL;
    mln_uauto_t buf_size = mln_queue_roundup_pow2(qlen ? qlen : 1);
    q->qlen = qlen;
    q->nr_element = 0;
    q->mask = buf_size - 1;
    q->head = 0;
    q->free_handler = free_handler;
    q->queue = (void **)calloc(buf_size, sizeof(void *));
    if (q->queue == NULL) {
        free(q);
        return NULL;
    }
    return q;
})

MLN_FUNC_VOID(, void, mln_queue_destroy, (mln_queue_t *q), (q), {
    if (q == NULL) return;
    if (q->free_handler != NULL) {
        mln_uauto_t i;
        for (i = 0; i < q->nr_element; ++i) {
            q->free_handler(q->queue[(q->head + i) & q->mask]);
        }
    }
    if (q->queue != NULL)
        free(q->queue);
    free(q);
})

MLN_FUNC(, int, mln_queue_append, (mln_queue_t *q, void *data), (q, data), {
    if (q->nr_element >= q->qlen) return -1;
    q->queue[(q->head + q->nr_element) & q->mask] = data;
    ++q->nr_element;
    return 0;
})

MLN_FUNC(, void *, mln_queue_get, (mln_queue_t *q), (q), {
    if (!q->nr_element) return NULL;
    return q->queue[q->head];
})

MLN_FUNC_VOID(, void, mln_queue_remove, (mln_queue_t *q), (q), {
    if (!q->nr_element) return;
    q->head = (q->head + 1) & q->mask;
    --q->nr_element;
})

MLN_FUNC(, void *, mln_queue_search, (mln_queue_t *q, mln_uauto_t index), (q, index), {
    if (index >= q->nr_element) return NULL;
    return q->queue[(q->head + index) & q->mask];
})

MLN_FUNC(, int, mln_queue_iterate, \
         (mln_queue_t *q, queue_iterate_handler handler, void *udata), \
         (q, handler, udata), \
{
    if (handler == NULL) return 0;
    mln_uauto_t i, n = q->nr_element, h = q->head, m = q->mask;
    void **buf = q->queue;
    for (i = 0; i < n; ++i) {
        if (handler(buf[(h + i) & m], udata) < 0)
            return -1;
    }
    return 0;
})

MLN_FUNC_VOID(, void, mln_queue_free_index, (mln_queue_t *q, mln_uauto_t index), (q, index), {
    if (index >= q->nr_element) return;
    mln_uauto_t mask = q->mask;
    mln_uauto_t rem = (q->head + index) & mask;
    void *save = q->queue[rem];
    mln_uauto_t buf_size = mask + 1;
    mln_uauto_t count = q->nr_element - 1 - index;
    if (count > 0) {
        if (rem + count < buf_size) {
            memmove(&q->queue[rem], &q->queue[rem + 1], count * sizeof(void *));
        } else if (rem == mask) {
            q->queue[mask] = q->queue[0];
            if (count > 1)
                memmove(&q->queue[0], &q->queue[1], (count - 1) * sizeof(void *));
        } else {
            mln_uauto_t first_part = mask - rem;
            memmove(&q->queue[rem], &q->queue[rem + 1], first_part * sizeof(void *));
            q->queue[mask] = q->queue[0];
            mln_uauto_t rest = count - first_part - 1;
            if (rest > 0)
                memmove(&q->queue[0], &q->queue[1], rest * sizeof(void *));
        }
    }
    --q->nr_element;
    if (q->free_handler != NULL)
        q->free_handler(save);
})

