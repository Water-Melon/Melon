
/*
 * Copyright (C) Niklaus F.Schen.
 */

#ifndef __MLN_HASH_H
#define __MLN_HASH_H

#include "mln_types.h"

typedef struct mln_hash_s mln_hash_t;

typedef int  (*hash_calc_handler)(mln_hash_t *, void *);
/*
 * cmp_handler's return value: 0 -- not matched, !0 -- matched.
 */
typedef int  (*hash_cmp_handler) (mln_hash_t *, void *, void *);
typedef void (*hash_free_handler)(void *);

enum mln_hash_flag {
    hash_none,
    hash_only_free_val,
    hash_only_free_key,
    hash_free_key_val
};

struct mln_hash_attr {
    hash_calc_handler       hash;
    hash_cmp_handler        cmp;
    hash_free_handler       free_key;
    hash_free_handler       free_val;
    mln_u32_t               len_base;
    mln_u32_t               expandable;
};

typedef struct mln_hash_entry_s {
    void                    *val          __cacheline_aligned;
    void                    *key;
    struct mln_hash_entry_s *next;
    struct mln_hash_entry_s *prev;
} mln_hash_entry_t;

typedef struct {
    mln_hash_entry_t        *head;
    mln_hash_entry_t        *tail;
} mln_hash_mgr_t;

struct mln_hash_s {
    mln_u32_t               expandable    __cacheline_aligned;
    mln_u32_t               len;
    mln_hash_mgr_t          *tbl;
    hash_calc_handler       hash;
    hash_cmp_handler        cmp;
    hash_free_handler       free_key;
    hash_free_handler       free_val;
    mln_u32_t               nr_nodes      __cacheline_aligned;
    mln_u32_t               threshold;
};


extern mln_hash_t *
mln_hash_init(struct mln_hash_attr *attr) __NONNULL1(1);
extern void
mln_hash_destroy(mln_hash_t *h, enum mln_hash_flag flg) __NONNULL1(1);
extern void *
mln_hash_search(mln_hash_t *h, void *key) __NONNULL2(1,2);
extern int
mln_hash_insert(mln_hash_t *h, void *key, void *val) __NONNULL3(1,2,3);
extern void
mln_hash_remove(mln_hash_t *h, void *key, enum mln_hash_flag flg) __NONNULL2(1,2);

#endif

