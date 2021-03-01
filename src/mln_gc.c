
/*
 * Copyright (C) Niklaus F.Schen.
 */
#include "mln_gc.h"
#include "mln_log.h"
#include <stdlib.h>
#include <stdio.h>

MLN_CHAIN_FUNC_DECLARE(mln_gc_item, \
                       mln_gc_item_t, \
                       static inline void,);
MLN_CHAIN_FUNC_DEFINE(mln_gc_item, \
                      mln_gc_item_t, \
                      static inline void, \
                      prev, \
                      next);
MLN_CHAIN_FUNC_DECLARE(mln_gc_item_proc, \
                       mln_gc_item_t, \
                       static inline void,);
MLN_CHAIN_FUNC_DEFINE(mln_gc_item_proc, \
                      mln_gc_item_t, \
                      static inline void, \
                      proc_prev, \
                      proc_next);

static inline mln_gc_item_t *mln_gc_item_new(mln_gc_t *gc, void *data)
{
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
}

static inline void mln_gc_item_free(mln_gc_item_t *item)
{
    if (item == NULL) return;
    mln_alloc_free(item);
}

/*
 * mln_gc_t
 */
mln_gc_t *mln_gc_new(struct mln_gc_attr *attr)
{
    if (attr->itemGetter == NULL || \
        attr->itemSetter == NULL || \
        attr->itemFreer == NULL || \
        attr->memberSetter == NULL || \
        attr->moveHandler == NULL || \
        attr->cleanSearcher == NULL || \
        attr->freeHandler == NULL)
    {
        mln_log(error, "Invalid arguments.\n");
        abort();
    }

    mln_gc_t *gc;
    if ((gc = (mln_gc_t *)mln_alloc_m(attr->pool, sizeof(mln_gc_t))) == NULL) {
        return NULL;
    }
    gc->pool = attr->pool;
    gc->item_head = gc->item_tail = NULL;
    gc->proc_head = gc->proc_tail = NULL;
    gc->iter = NULL;
    gc->itemGetter = attr->itemGetter;
    gc->itemSetter = attr->itemSetter;
    gc->itemFreer = attr->itemFreer;
    gc->memberSetter = attr->memberSetter;
    gc->moveHandler = attr->moveHandler;
    gc->rootSetter = attr->rootSetter;
    gc->cleanSearcher = attr->cleanSearcher;
    gc->freeHandler = attr->freeHandler;
    gc->del = 0;
    return gc;
}

void mln_gc_free(mln_gc_t *gc)
{
    if (gc == NULL) return;
    mln_gc_item_t *item;
    while ((item = gc->proc_head) != NULL) {
        mln_gc_item_proc_chain_del(&(gc->proc_head), &(gc->proc_tail), item);
    }
    while ((item = gc->item_head) != NULL) {
        mln_gc_item_chain_del(&(gc->item_head), &(gc->item_tail), item);
        gc->freeHandler(item->data);
        mln_gc_item_free(item);
    }
    mln_alloc_free(gc);
}

int mln_gc_add(mln_gc_t *gc, void *data)
{
    mln_gc_item_t *item;
    if ((item = mln_gc_item_new(gc, data)) == NULL) {
        return -1;
    }
    gc->itemSetter(data, item);
    mln_gc_item_chain_add(&(gc->item_head), &(gc->item_tail), item);
    return 0;
}

void mln_gc_suspect(mln_gc_t *gc, void *data)
{
    mln_gc_item_t *item = (mln_gc_item_t *)(gc->itemGetter(data));
    item->suspected = 1;
}

void mln_gc_merge(mln_gc_t *dest, mln_gc_t *src)
{
    if (dest == src) {
        mln_log(error, "GC must NOT be identical.\n");
        abort();
    }
    if (dest->pool != src->pool) {
        mln_log(error, "Pool must be identical.\n");
        abort();
    }
    mln_gc_item_t *item;
    while ((item = src->item_head) != NULL) {
        mln_gc_item_chain_del(&(src->item_head), &(src->item_tail), item);
        src->moveHandler(dest, item->data);
        item->gc = dest;
        mln_gc_item_chain_add(&(dest->item_head), &(dest->item_tail), item);
    }
}

void mln_gc_addForCollect(mln_gc_t *gc, void *data)
{
    if (data == NULL) return;
    mln_gc_item_t *item = (mln_gc_item_t *)(gc->itemGetter(data));
    if (item == NULL) {
        mln_log(error, "'data' has NOT been added.\n");
        abort();
    }
    if (item->proc_prev != NULL || \
        item->proc_next != NULL || \
        (gc->proc_head == gc->proc_tail && gc->proc_head == item))
    {
        item->credit = 1;
    } else {
        mln_gc_item_proc_chain_add(&(gc->proc_head), &(gc->proc_tail), item);
        item->inc = 1;
    }
}

int mln_gc_addForClean(mln_gc_t *gc, void *data)
{
    mln_gc_item_t *item = (mln_gc_item_t *)(gc->itemGetter(data));
    if (item == NULL) {
        mln_log(error, "'data' has NOT been added.\n");
        abort();
    }
    if (item->proc_prev != NULL || \
        item->proc_next != NULL || \
        (gc->proc_head == gc->proc_tail && gc->proc_head == item))
    {
        return -1;
    }
    mln_gc_item_proc_chain_add(&(gc->proc_head), &(gc->proc_tail), item);
    item->inc = 1;
    return 0;
}

void mln_gc_collect(mln_gc_t *gc, void *rootData)
{
    int done = 0;
    mln_gc_item_t *item, *head = NULL, *tail = NULL;
    for (item = gc->item_head; item != NULL; item = item->next) {
        if (item->proc_prev == NULL && \
            item->proc_next == NULL && \
            gc->proc_head != item)
            mln_gc_item_proc_chain_add(&(gc->proc_head), &(gc->proc_tail), item);
    }
    if (rootData != NULL && gc->rootSetter != NULL)
        gc->rootSetter(gc, rootData);
    while (!done) {
        done = 1;
        for (item = gc->proc_head; item != NULL; item = item->proc_next) {
            if ((item->suspected && !item->credit) || item->visited) {
                continue;
            }
            gc->memberSetter(gc, item->data);
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
        gc->cleanSearcher(gc, gc->iter->data);
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
        gc->itemFreer(item->data);
        mln_gc_item_free(item);
    }
}

void mln_gc_remove(mln_gc_t *gc, void *data, mln_gc_t *procGC)
{
    mln_gc_item_t *item = (mln_gc_item_t *)(gc->itemGetter(data));
    if (item == NULL) {
        mln_log(error, "'data' has NOT been added.\n");
        abort();
    }
    if (procGC == NULL) procGC = gc;
    if (item->proc_prev != NULL || \
        item->proc_next != NULL || \
        procGC->proc_head == item)
    {
        if (procGC->iter != NULL && procGC->iter == item) {
            procGC->iter = item->proc_next;
            procGC->del = 1;
        }
        mln_gc_item_proc_chain_del(&(procGC->proc_head), &(procGC->proc_tail), item);
    }
    mln_gc_item_chain_del(&(gc->item_head), &(gc->item_tail), item);
    mln_gc_item_free(item);
}

