
/*
 * Copyright (C) Niklaus F.Schen.
 */

#ifndef __MLN_HASH_H
#define __MLN_HASH_H

#include "mln_types.h"

typedef struct mln_hash_s mln_hash_t;

typedef int (*hash_scan_handler)(void * /*key*/, void * /*val*/, void *);
typedef mln_u64_t (*hash_calc_handler)(mln_hash_t *, void *);
/*
 * cmp_handler's return value: 0 -- not matched, !0 -- matched.
 */
typedef int  (*hash_cmp_handler) (mln_hash_t *, void *, void *);
typedef void (*hash_free_handler)(void *);

typedef enum mln_hash_flag {
    M_HASH_F_NONE,
    M_HASH_F_VAL,
    M_HASH_F_KEY,
    M_HASH_F_KV
} mln_hash_flag_t;

struct mln_hash_attr {
    hash_calc_handler        hash;
    hash_cmp_handler         cmp;
    hash_free_handler        free_key;
    hash_free_handler        free_val;
    mln_u64_t                len_base;
    mln_u32_t                expandable:1;
    mln_u32_t                calc_prime:1;
    mln_u32_t                cache:1;
};

typedef struct mln_hash_entry_s {
    mln_hash_t              *hash;
    void                    *val;
    void                    *key;
    struct mln_hash_entry_s *prev;
    struct mln_hash_entry_s *next;
    struct mln_hash_entry_s *cache_prev;
    struct mln_hash_entry_s *cache_next;
} mln_hash_entry_t;

typedef struct {
    mln_hash_entry_t        *head;
    mln_hash_entry_t        *tail;
} mln_hash_mgr_t;

struct mln_hash_s {
    hash_calc_handler        hash;
    hash_cmp_handler         cmp;
    hash_free_handler        free_key;
    hash_free_handler        free_val;
    mln_hash_mgr_t          *tbl;
    mln_hash_entry_t        *cache_head;
    mln_hash_entry_t        *cache_tail;
    mln_u64_t                len;
    mln_u32_t                nr_nodes;
    mln_u32_t                threshold;
    mln_u32_t                expandable:1;
    mln_u32_t                calc_prime:1;
    mln_u32_t                cache:1;
};


extern mln_hash_t *
mln_hash_init(struct mln_hash_attr *attr) __NONNULL1(1);
extern void
mln_hash_destroy(mln_hash_t *h, mln_hash_flag_t flg) __NONNULL1(1);
extern void *
mln_hash_search(mln_hash_t *h, void *key) __NONNULL2(1,2);
extern void *
mln_hash_search_iterator(mln_hash_t *h, void *key, int **ctx) __NONNULL3(1,2,3);
/*
 * mln_hash_replace():
 * The second and third arguments are all second rank pointer variables.
 * For getting rid of the compiler's error, I have to define these two
 * types as void *.
 */
extern int
mln_hash_replace(mln_hash_t *h, void *key, void *val) __NONNULL3(1,2,3);
extern int
mln_hash_insert(mln_hash_t *h, void *key, void *val) __NONNULL2(1,2);
extern void
mln_hash_remove(mln_hash_t *h, void *key, mln_hash_flag_t flg) __NONNULL2(1,2);
extern int
mln_hash_scan_all(mln_hash_t *h, hash_scan_handler handler, void *udata) __NONNULL1(1);
extern void *
mln_hash_change_value(mln_hash_t *h, void *key, void *new_value) __NONNULL2(1,2);
extern int mln_hash_key_exist(mln_hash_t *h, void *key) __NONNULL2(1,2);
extern void mln_hash_reset(mln_hash_t *h, mln_hash_flag_t flg) __NONNULL1(1);

#endif

