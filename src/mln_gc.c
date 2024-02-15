
/*
 * Copyright (C) Niklaus F.Schen.
 */
#include "mln_gc.h"
#include "mln_utils.h"
#include "mln_func.h"
#include <stdlib.h>
#include <stdio.h>

MLN_CHAIN_FUNC_DECLARE(static inline, \
                       mln_gc_item, \
                       mln_gc_item_t, );
MLN_CHAIN_FUNC_DEFINE(static inline, \
                      mln_gc_item, \
                      mln_gc_item_t, \
                      prev, \
                      next);
MLN_CHAIN_FUNC_DECLARE(static inline, \
                       mln_gc_item_proc, \
                       mln_gc_item_t, );
MLN_CHAIN_FUNC_DEFINE(static inline, \
                      mln_gc_item_proc, \
                      mln_gc_item_t, \
                      proc_prev, \
                      proc_next);

MLN_FUNC(static inline, mln_gc_item_t *, mln_gc_item_new, (mln_gc_t *gc, void *data), (gc, data), {
    mln_gc_item_t *item;
    if ((item = (mln_gc_item_t *)mln_alloc_m(gc->pool, sizeof(mln_gc_item_t))) == NULL) {
        return NULL;
    }
    item->gc = gc;
    item->data = data;
    item->prev = item->next = NULL;
    item->proc_prev = item->proc_next = NULL;
    item->suspected = 0;
    item->credit = 0;
    item->inc = 0;
    item->visited = 0;
    return item;
})

MLN_FUNC_VOID(static inline, void, mln_gc_item_free, (mln_gc_item_t *item), (item), {
    if (item == NULL) return;
    mln_alloc_free(item);
})

/*
 * mln_gc_t
 */
MLN_FUNC(, mln_gc_t *, mln_gc_new, (struct mln_gc_attr *attr), (attr), {
    ASSERT(attr->item_getter != NULL && \
        attr->item_setter != NULL && \
        attr->item_freer != NULL && \
        attr->member_setter != NULL && \
        attr->move_handler != NULL && \
        attr->clean_searcher != NULL && \
        attr->free_handler != NULL);

    mln_gc_t *gc;
    if ((gc = (mln_gc_t *)mln_alloc_m(attr->pool, sizeof(mln_gc_t))) == NULL) {
        return NULL;
    }
    gc->pool = attr->pool;
    gc->item_head = gc->item_tail = NULL;
    gc->proc_head = gc->proc_tail = NULL;
    gc->iter = NULL;
    gc->item_getter = attr->item_getter;
    gc->item_setter = attr->item_setter;
    gc->item_freer = attr->item_freer;
    gc->member_setter = attr->member_setter;
    gc->move_handler = attr->move_handler;
    gc->root_setter = attr->root_setter;
    gc->clean_searcher = attr->clean_searcher;
    gc->free_handler = attr->free_handler;
    gc->del = 0;
    return gc;
})

MLN_FUNC_VOID(, void, mln_gc_free, (mln_gc_t *gc), (gc), {
    if (gc == NULL) return;
    mln_gc_item_t *item;
    while ((item = gc->proc_head) != NULL) {
        mln_gc_item_proc_chain_del(&(gc->proc_head), &(gc->proc_tail), item);
    }
    while ((item = gc->item_head) != NULL) {
        mln_gc_item_chain_del(&(gc->item_head), &(gc->item_tail), item);
        gc->free_handler(item->data);
        mln_gc_item_free(item);
    }
    mln_alloc_free(gc);
})

MLN_FUNC(, int, mln_gc_add, (mln_gc_t *gc, void *data), (gc, data), {
    mln_gc_item_t *item;
    if ((item = mln_gc_item_new(gc, data)) == NULL) {
        return -1;
    }
    gc->item_setter(data, item);
    mln_gc_item_chain_add(&(gc->item_head), &(gc->item_tail), item);
    return 0;
})

MLN_FUNC_VOID(, void, mln_gc_suspect, (mln_gc_t *gc, void *data), (gc, data), {
    mln_gc_item_t *item = (mln_gc_item_t *)(gc->item_getter(data));
    item->suspected = 1;
})

MLN_FUNC_VOID(, void, mln_gc_merge, (mln_gc_t *dest, mln_gc_t *src), (dest, src), {
    /*
     * GC must NOT be identical
     * Pool must be identical
     */
    ASSERT(dest != src && dest->pool == src->pool);
    mln_gc_item_t *item;
    while ((item = src->item_head) != NULL) {
        mln_gc_item_chain_del(&(src->item_head), &(src->item_tail), item);
        src->move_handler(dest, item->data);
        item->gc = dest;
        mln_gc_item_chain_add(&(dest->item_head), &(dest->item_tail), item);
    }
})

MLN_FUNC_VOID(, void, mln_gc_collect_add, (mln_gc_t *gc, void *data), (gc, data), {
    if (data == NULL) return;
    mln_gc_item_t *item = (mln_gc_item_t *)(gc->item_getter(data));
    ASSERT(item != NULL); /* 'data' has NOT been added. */
    if (item->proc_prev != NULL || \
        item->proc_next != NULL || \
        (gc->proc_head == gc->proc_tail && gc->proc_head == item))
    {
        item->credit = 1;
    } else {
        mln_gc_item_proc_chain_add(&(gc->proc_head), &(gc->proc_tail), item);
        item->inc = 1;
    }
})

MLN_FUNC(, int, mln_gc_clean_add, (mln_gc_t *gc, void *data), (gc, data), {
    mln_gc_item_t *item = (mln_gc_item_t *)(gc->item_getter(data));
    ASSERT(item != NULL); /* 'data' has NOT been added. */
    if (item->proc_prev != NULL || \
        item->proc_next != NULL || \
        (gc->proc_head == gc->proc_tail && gc->proc_head == item))
    {
        return -1;
    }
    mln_gc_item_proc_chain_add(&(gc->proc_head), &(gc->proc_tail), item);
    item->inc = 1;
    return 0;
})

MLN_FUNC_VOID(, void, mln_gc_collect, (mln_gc_t *gc, void *root_data), (gc, root_data), {
    int done = 0;
    mln_gc_item_t *item, *head = NULL, *tail = NULL;
    for (item = gc->item_head; item != NULL; item = item->next) {
        if (item->proc_prev == NULL && \
            item->proc_next == NULL && \
            gc->proc_head != item)
            mln_gc_item_proc_chain_add(&(gc->proc_head), &(gc->proc_tail), item);
    }
    if (root_data != NULL && gc->root_setter != NULL)
        gc->root_setter(gc, root_data);
    while (!done) {
        done = 1;
        for (item = gc->proc_head; item != NULL; item = item->proc_next) {
            if ((item->suspected && !item->credit) || item->visited) {
                continue;
            }
            gc->member_setter(gc, item->data);
            item->visited = 1;
            if (done) done = 0;
        }
    }

    while ((item = gc->proc_head) != NULL) {
        mln_gc_item_proc_chain_del(&(gc->proc_head), &(gc->proc_tail), item);
        if (item->inc) {
            item->inc = 0;
            item->visited = 0;
            item->credit = 0;
            continue;
        }
        if (item->credit) {
            item->credit = 0;
            item->visited = 0;
            continue;
        }
        if (!item->suspected) {
            item->visited = 0;
            item->credit = 0;
            continue;
        }
        mln_gc_item_proc_chain_add(&head, &tail, item);
    }
    gc->proc_head = head;
    gc->proc_tail = tail;

    for (gc->iter = gc->proc_head; gc->iter != NULL;) {
        if (gc->iter->visited) {
            gc->iter = gc->iter->next;
            continue;
        }
        gc->clean_searcher(gc, gc->iter->data);
        if (gc->del) {
            gc->del = 0;
            continue;
        }
        gc->iter->visited = 1;
        gc->iter = gc->iter->proc_next;
    }
    while ((item = gc->proc_head) != NULL) {
        mln_gc_item_proc_chain_del(&(gc->proc_head), &(gc->proc_tail), item);
        if (item->inc) {
            item->inc = 0;
            item->visited = 0;
            continue;
        }
        mln_gc_item_chain_del(&(gc->item_head), &(gc->item_tail), item);
        gc->item_freer(item->data);
        mln_gc_item_free(item);
    }
})

MLN_FUNC_VOID(, void, mln_gc_remove, (mln_gc_t *gc, void *data, mln_gc_t *proc_gc), (gc, data, proc_gc), {
    mln_gc_item_t *item = (mln_gc_item_t *)(gc->item_getter(data));
    ASSERT(item != NULL); /* 'data' has NOT been added. */
    if (proc_gc == NULL) proc_gc = gc;
    if (item->proc_prev != NULL || \
        item->proc_next != NULL || \
        proc_gc->proc_head == item)
    {
        if (proc_gc->iter != NULL && proc_gc->iter == item) {
            proc_gc->iter = item->proc_next;
            proc_gc->del = 1;
        }
        mln_gc_item_proc_chain_del(&(proc_gc->proc_head), &(proc_gc->proc_tail), item);
    }
    mln_gc_item_chain_del(&(gc->item_head), &(gc->item_tail), item);
    mln_gc_item_free(item);
})

