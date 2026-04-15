
/*
 * Copyright (C) Niklaus F.Schen.
 */

#ifndef __MLN_QUEUE_H
#define __MLN_QUEUE_H

#include "mln_types.h"
#include <stdlib.h>
#include <string.h>

typedef void (*queue_free)(void *);
typedef int (*queue_iterate_handler)(void *, void *);

typedef struct {
    void                 **queue;
    mln_uauto_t            head;
    mln_uauto_t            qlen;
    mln_uauto_t            nr_element;
    mln_uauto_t            mask;
    queue_free             free_handler;
} mln_queue_t;


#define mln_queue_empty(q) (!((q)->nr_element))
#define mln_queue_full(q) ((q)->nr_element >= (q)->qlen)
#define mln_queue_length(q) ((q)->qlen)
#define mln_queue_element(q) ((q)->nr_element)
extern mln_queue_t *mln_queue_init(mln_uauto_t qlen, queue_free free_handler);
extern void mln_queue_destroy(mln_queue_t *q);
extern int mln_queue_append(mln_queue_t *q, void *data) __NONNULL1(1);
extern void *mln_queue_get(mln_queue_t *q) __NONNULL1(1);
extern void mln_queue_remove(mln_queue_t *q) __NONNULL1(1);
/*mln_queue_free_index & mln_queue_search's index start from 0*/
extern void *mln_queue_search(mln_queue_t *q, mln_uauto_t index) __NONNULL1(1);
extern void mln_queue_free_index(mln_queue_t *q, mln_uauto_t index) __NONNULL1(1);
extern int mln_queue_iterate(mln_queue_t *q, queue_iterate_handler handler, void *udata) __NONNULL1(1);

#ifndef MLN_FUNC_FLAG
#define mln_queue_append(q, d) ({\
    mln_queue_t *_mq = (q);\
    int _mr;\
    if (_mq->nr_element >= _mq->qlen) {\
        _mr = -1;\
    } else {\
        _mq->queue[(_mq->head + _mq->nr_element) & _mq->mask] = (d);\
        ++_mq->nr_element;\
        _mr = 0;\
    }\
    _mr;\
})

#define mln_queue_get(q) ({\
    mln_queue_t *_mq = (q);\
    _mq->nr_element ? _mq->queue[_mq->head] : NULL;\
})

#define mln_queue_remove(q) ({\
    mln_queue_t *_mq = (q);\
    if (_mq->nr_element) {\
        _mq->head = (_mq->head + 1) & _mq->mask;\
        --_mq->nr_element;\
    }\
})

#define mln_queue_search(q, idx) ({\
    mln_queue_t *_mq = (q);\
    mln_uauto_t _mi = (idx);\
    _mi >= _mq->nr_element ? NULL : _mq->queue[(_mq->head + _mi) & _mq->mask];\
})
#endif

#endif

