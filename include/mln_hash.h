
/*
 * Copyright (C) Niklaus F.Schen.
 */

#ifndef __MLN_HASH_H
#define __MLN_HASH_H

#include "mln_types.h"

typedef struct mln_hash_s mln_hash_t;
typedef struct mln_hash_entry_s mln_hash_iterator_t;

typedef int (*hash_iterate_handler)(mln_hash_iterator_t*, void *);
typedef mln_u64_t (*hash_calc_handler)(mln_hash_t *, void *);
/*
 * cmp_handler's return value: 0 -- not matched, !0 -- matched.
 */
typedef int  (*hash_cmp_handler) (mln_hash_t *, void *, void *);
typedef void (*hash_free_handler)(void *);
typedef void *(*hash_pool_alloc_handler)(void *, mln_size_t);
typedef void (*hash_pool_free_handler)(void *);

typedef enum mln_hash_flag {
    M_HASH_F_NONE,
    M_HASH_F_VAL,
    M_HASH_F_KEY,
    M_HASH_F_KV
} mln_hash_flag_t;

typedef enum mln_hash_type_e {
    M_HASH_ITERATOR,
    M_HASH_FREE,
    M_HASH_ZOMBIE,
    
} mln_hash_type_t;

struct mln_hash_attr {
    hash_calc_handler        hash;
    hash_cmp_handler         cmp;
    hash_free_handler        free_key;
    hash_free_handler        free_val;
    mln_u64_t                len_base;
    mln_u32_t                expandable:1;
    mln_u32_t                calc_prime:1;
    void                    *pool;
    hash_pool_alloc_handler  pool_alloc;
    hash_pool_free_handler   pool_free;
};

typedef struct mln_hash_entry_s {
    struct mln_hash_entry_s *prev;
    struct mln_hash_entry_s *next;
    void                    *val;
    void                    *key;
    mln_hash_type_t          it_type;
    mln_hash_t              *it_belong;
} mln_hash_entry_t;

typedef struct {
    mln_hash_entry_t        *head;
    mln_hash_entry_t        *tail;
} mln_hash_mgr_t;

struct mln_hash_s {
    mln_hash_mgr_t          *tbl;
    mln_u64_t                len;
    hash_calc_handler        hash;
    hash_cmp_handler         cmp;
    hash_free_handler        free_key;
    hash_free_handler        free_val;
    mln_u32_t                nr_nodes;
    mln_u32_t                threshold;
    mln_u32_t                expandable:1;
    mln_u32_t                calc_prime:1;
    void                    *pool;
    hash_pool_alloc_handler  pool_alloc;
    hash_pool_free_handler   pool_free;
};


extern int
mln_hash_init(mln_hash_t *h, struct mln_hash_attr *attr) __NONNULL2(1,2);
extern void mln_hash_destroy(mln_hash_t *h, mln_hash_flag_t flg);
extern mln_hash_t *
mln_hash_new(struct mln_hash_attr *attr) __NONNULL1(1);
extern void
mln_hash_free(mln_hash_t *h, mln_hash_flag_t flg);
extern mln_hash_iterator_t *
mln_hash_search(mln_hash_t *h, void *key) __NONNULL2(1,2);
extern mln_hash_iterator_t *
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
mln_hash_insert(mln_hash_t *h,mln_hash_iterator_t *it) __NONNULL2(1,2);
extern void
mln_hash_remove(mln_hash_t *h,mln_hash_iterator_t *it) __NONNULL2(1,2);
extern int
mln_hash_iterate(mln_hash_t *h, hash_iterate_handler handler, void *udata) __NONNULL1(1);
extern void *
mln_hash_change_value(mln_hash_t *h, void *key, void *new_value) __NONNULL2(1,2);
extern int mln_hash_key_exist(mln_hash_t *h, void *key) __NONNULL2(1,2);
extern void mln_hash_reset(mln_hash_t *h, mln_hash_flag_t flg) __NONNULL1(1);

extern void * 
mln_hash_iterator_key_get(mln_hash_iterator_t *it) __NONNULL1(1);
extern void * 
mln_hash_iterator_value_get(mln_hash_iterator_t *it) __NONNULL1(1);
extern mln_hash_iterator_t *
mln_hash_iterator_new(mln_hash_t* h,void *key,void *val) __NONNULL2(1,2);
extern void
mln_hash_iterator_free(mln_hash_iterator_t *it,mln_hash_flag_t flg) __NONNULL1(1);
extern void* 
mln_hash_iterator_value_set(mln_hash_iterator_t* it,void *val) __NONNULL1(1);


#endif

