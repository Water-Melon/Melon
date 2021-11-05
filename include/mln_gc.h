
/*
 * Copyright (C) Niklaus F.Schen.
 */
#ifndef __MLN_GC_H
#define __MLN_GC_H

#include "mln_types.h"
#include "mln_alloc.h"

typedef struct mln_gc_s      mln_gc_t;
typedef struct mln_gc_item_s mln_gc_item_t;

typedef void *(*gc_item_getter)   (void *data);
typedef void  (*gc_item_setter)   (void *data, void *item);
typedef void  (*gc_item_freer)    (void *data);
typedef void  (*gc_member_setter) (mln_gc_t *gc, void *data);
typedef void  (*gc_move_handler)  (mln_gc_t *dest_gc, void *data);
typedef void  (*gc_root_setter)   (mln_gc_t *gc, void *data);
typedef void  (*gc_clean_searcher)(mln_gc_t *gc, void *data);
typedef void  (*gc_free_handler)  (void *data);

struct mln_gc_attr {
    mln_alloc_t            *pool;
    gc_item_getter          item_getter;
    gc_item_setter          item_setter;
    gc_item_freer           item_freer;
    gc_member_setter        member_setter;
    gc_move_handler         move_handler;
    gc_root_setter          root_setter;
    gc_clean_searcher       clean_searcher;
    gc_free_handler         free_handler;
};

struct mln_gc_item_s {
    mln_gc_t               *gc;
    void                   *data;
    struct mln_gc_item_s   *prev;
    struct mln_gc_item_s   *next;
    struct mln_gc_item_s   *proc_prev;
    struct mln_gc_item_s   *proc_next;
    mln_u32_t               suspected:1;
    mln_u32_t               credit:1;
    mln_u32_t               inc:1;
    mln_u32_t               visited:1;
};

struct mln_gc_s {
    mln_alloc_t            *pool;
    mln_gc_item_t          *item_head;
    mln_gc_item_t          *item_tail;
    mln_gc_item_t          *proc_head;
    mln_gc_item_t          *proc_tail;
    mln_gc_item_t          *iter;
    gc_item_getter          item_getter;
    gc_item_setter          item_setter;
    gc_item_freer           item_freer;
    gc_member_setter        member_setter;
    gc_move_handler         move_handler;
    gc_root_setter          root_setter;
    gc_clean_searcher       clean_searcher;
    gc_free_handler         free_handler;
    mln_u32_t               del:1;
};

extern mln_gc_t *mln_gc_new(struct mln_gc_attr *attr) __NONNULL1(1);
extern void mln_gc_free(mln_gc_t *gc);
extern int mln_gc_add(mln_gc_t *gc, void *data) __NONNULL2(1,2);
extern void mln_gc_suspect(mln_gc_t *gc, void *data) __NONNULL2(1,2);
extern void mln_gc_merge(mln_gc_t *dest, mln_gc_t *src) __NONNULL2(1,2);
extern void mln_gc_collect_add(mln_gc_t *gc, void *data) __NONNULL1(1);
extern int mln_gc_clean_add(mln_gc_t *gc, void *data) __NONNULL2(1,2);
extern void mln_gc_collect(mln_gc_t *gc, void *root_data) __NONNULL1(1);
extern void mln_gc_remove(mln_gc_t *gc, void *data, mln_gc_t *proc_gc)__NONNULL2(1,2);

#endif
