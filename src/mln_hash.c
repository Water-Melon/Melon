
/*
 * Copyright (C) Niklaus F.Schen.
 */

#include <stdlib.h>
#include "mln_prime_generator.h"
#include "mln_hash.h"
#include "mln_func.h"
#include <stdio.h>
#include <string.h>

MLN_CHAIN_FUNC_DECLARE(static inline, \
                       mln_hash_entry_iter, \
                       mln_hash_entry_t, );
MLN_CHAIN_FUNC_DECLARE(static inline, \
                       mln_hash_entry, \
                       mln_hash_entry_t, );
static inline mln_hash_entry_t *
mln_hash_entry_new(mln_hash_t *h, mln_hash_mgr_t *mgr, void *key, void *val) __NONNULL4(1,2,3,4);
static inline void
mln_hash_entry_free(mln_hash_t *h, mln_hash_entry_t *he, mln_hash_flag_t flg) __NONNULL1(1);
static inline void
mln_hash_reduce(mln_hash_t *h) __NONNULL1(1);
static inline void
mln_hash_expand(mln_hash_t *h) __NONNULL1(1);
static inline void
mln_move_hash_entry(mln_hash_t *h, mln_hash_mgr_t *old_tbl, mln_u32_t old_len) __NONNULL2(1,2);

MLN_FUNC(, int, mln_hash_init, (mln_hash_t *h, struct mln_hash_attr *attr), (h, attr), {
    h->pool = attr->pool;
    h->pool_alloc = attr->pool_alloc;
    h->pool_free = attr->pool_free;
    h->hash = attr->hash;
    h->cmp = attr->cmp;
    h->key_freer = attr->key_freer;
    h->val_freer = attr->val_freer;
    h->len = attr->calc_prime? mln_prime_generate(attr->len_base): attr->len_base;
    if (h->pool != NULL) {
        h->tbl = (mln_hash_mgr_t *)h->pool_alloc(h->pool, h->len*sizeof(mln_hash_mgr_t));
        memset(h->tbl, 0, h->len*sizeof(mln_hash_mgr_t));
    } else {
        h->tbl = (mln_hash_mgr_t *)calloc(h->len, sizeof(mln_hash_mgr_t));
    }
    if (h->tbl == NULL) return -1;

    h->nr_nodes = 0;
    h->threshold = attr->calc_prime? mln_prime_generate(h->len << 1): h->len << 1;
    h->expandable = attr->expandable;
    h->calc_prime = attr->calc_prime;
    if (h->len == 0 || \
        h->hash == NULL || \
        h->cmp == NULL)
    {
        if (h->pool != NULL) {
            h->pool_free(h->tbl);
        } else {
            free(h->tbl);
        }
        return -1;
    }
    h->iter = h->iter_head = h->iter_tail = NULL;
    return 0;
})

MLN_FUNC(, int, mln_hash_init_fast, \
         (mln_hash_t *h, hash_calc_handler hash, hash_cmp_handler cmp, hash_free_handler key_freer, \
          hash_free_handler val_freer, mln_u64_t base_len, mln_u32_t expandable, mln_u32_t calc_prime), \
         (h, hash, cmp, key_freer, val_freer, base_len, expandable, calc_prime), \
{
    h->pool = NULL;
    h->pool_alloc = NULL;
    h->pool_free = NULL;
    h->hash = hash;
    h->cmp = cmp;
    h->key_freer = key_freer;
    h->val_freer = val_freer;
    h->len = calc_prime? mln_prime_generate(base_len): base_len;
    h->tbl = (mln_hash_mgr_t *)calloc(h->len, sizeof(mln_hash_mgr_t));
    if (h->tbl == NULL) return -1;

    h->nr_nodes = 0;
    h->threshold = calc_prime? mln_prime_generate(h->len << 1): h->len << 1;
    h->expandable = expandable;
    h->calc_prime = calc_prime;
    if (h->len == 0 || \
        h->hash == NULL || \
        h->cmp == NULL)
    {
        if (h->pool != NULL) {
            h->pool_free(h->tbl);
        } else {
            free(h->tbl);
        }
        return -1;
    }
    h->iter = h->iter_head = h->iter_tail = NULL;
    return 0;
})

MLN_FUNC(, mln_hash_t *, mln_hash_new, (struct mln_hash_attr *attr), (attr), {
    mln_hash_t *h;
    if (attr->pool != NULL) {
        h = (mln_hash_t *)attr->pool_alloc(attr->pool, sizeof(mln_hash_t));
    } else {
        h = (mln_hash_t *)malloc(sizeof(mln_hash_t));
    }
    if (h == NULL) return NULL;

    h->pool = attr->pool;
    h->pool_alloc = attr->pool_alloc;
    h->pool_free = attr->pool_free;
    h->hash = attr->hash;
    h->cmp = attr->cmp;
    h->key_freer = attr->key_freer;
    h->val_freer = attr->val_freer;
    h->len = attr->calc_prime? mln_prime_generate(attr->len_base): attr->len_base;
    if (h->pool != NULL) {
        h->tbl = (mln_hash_mgr_t *)h->pool_alloc(h->pool, h->len*sizeof(mln_hash_mgr_t));
        memset(h->tbl, 0, h->len*sizeof(mln_hash_mgr_t));
    } else {
        h->tbl = (mln_hash_mgr_t *)calloc(h->len, sizeof(mln_hash_mgr_t));
    }
    if (h->tbl == NULL) {
        if (h->pool != NULL) h->pool_free(h);
        else free(h);
        return NULL;
    }
    h->nr_nodes = 0;
    h->threshold = attr->calc_prime? mln_prime_generate(h->len << 1): h->len << 1;
    h->expandable = attr->expandable;
    h->calc_prime = attr->calc_prime;
    if (h->len == 0 || \
        h->hash == NULL || \
        h->cmp == NULL)
    {
        if (h->pool != NULL) {
            h->pool_free(h->tbl);
            h->pool_free(h);
        } else {
            free(h->tbl);
            free(h);
        }
        return NULL;
    }
    h->iter = h->iter_head = h->iter_tail = NULL;
    return h;
})

MLN_FUNC(, mln_hash_t *, mln_hash_new_fast, \
         (hash_calc_handler hash, hash_cmp_handler cmp, hash_free_handler key_freer, \
          hash_free_handler val_freer, mln_u64_t base_len, mln_u32_t expandable, mln_u32_t calc_prime), \
         (hash, cmp, key_freer, val_freer, base_len, expandable, calc_prime), \
{
    mln_hash_t *h;

    h = (mln_hash_t *)malloc(sizeof(mln_hash_t));
    if (h == NULL) return NULL;

    h->pool = NULL;
    h->pool_alloc = NULL;
    h->pool_free = NULL;
    h->hash = hash;
    h->cmp = cmp;
    h->key_freer = key_freer;
    h->val_freer = val_freer;
    h->len = calc_prime? mln_prime_generate(base_len): base_len;
    h->tbl = (mln_hash_mgr_t *)calloc(h->len, sizeof(mln_hash_mgr_t));
    if (h->tbl == NULL) {
        free(h);
        return NULL;
    }
    h->nr_nodes = 0;
    h->threshold = calc_prime? mln_prime_generate(h->len << 1): h->len << 1;
    h->expandable = expandable;
    h->calc_prime = calc_prime;
    if (h->len == 0 || h->hash == NULL || h->cmp == NULL) {
        free(h->tbl);
        free(h);
        return NULL;
    }
    h->iter = h->iter_head = h->iter_tail = NULL;
    return h;
})

MLN_FUNC_VOID(, void, mln_hash_destroy, (mln_hash_t *h, mln_hash_flag_t flg), (h, flg), {
    if (h == NULL) return;

    mln_hash_entry_t *he, *fr;
    mln_hash_mgr_t *mgr, *mgr_end = h->tbl + h->len;
    for (mgr = h->tbl; mgr < mgr_end; ++mgr) {
        he = mgr->head;
        while (he != NULL) {
            fr = he;
            he = he->next;
            mln_hash_entry_free(h, fr, flg);
        }
    }
    if (h->pool != NULL) h->pool_free(h->tbl);
    else free(h->tbl);
})

MLN_FUNC_VOID(, void, mln_hash_free, (mln_hash_t *h, mln_hash_flag_t flg), (h, flg), {
    if (h == NULL) return;

    mln_hash_entry_t *he, *fr;
    mln_hash_mgr_t *mgr, *mgr_end = h->tbl + h->len;
    for (mgr = h->tbl; mgr < mgr_end; ++mgr) {
        he = mgr->head;
        while (he != NULL) {
            fr = he;
            he = he->next;
            mln_hash_entry_free(h, fr, flg);
        }
    }
    if (h->pool != NULL) h->pool_free(h->tbl);
    else free(h->tbl);
    if (h->pool != NULL) h->pool_free(h);
    else free(h);
})

MLN_FUNC(, int, mln_hash_update, (mln_hash_t *h, void *key, void *val), (h, key, val), {
    void **k = (void **)key;
    void **v = (void **)val;
    mln_u32_t index = h->hash(h, *k);
    mln_hash_mgr_t *mgr = &(h->tbl[index]);
    mln_hash_entry_t *he;
    for (he = mgr->head; he != NULL; he = he->next) {
        if (h->cmp(h, *k, he->key)) break;
    }
    if (he != NULL) {
        he->removed = 0;

        void *save_key = he->key;
        void *save_val = he->val;
        he->key = *k;
        he->val = *v;
        *k = save_key;
        *v = save_val;
        return 0;
    }

    if (h->expandable && h->nr_nodes > h->threshold) {
        mln_hash_expand(h);
    }
    if (h->expandable && h->nr_nodes <= (h->threshold >> 3)) {
        mln_hash_reduce(h);
    }
    he = mln_hash_entry_new(h, mgr, *k, *v);
    if (he == NULL) return -1;
    mln_hash_entry_chain_add(&(mgr->head), &(mgr->tail), he);
    mln_hash_entry_iter_chain_add(&(h->iter_head), &(h->iter_tail), he);
    ++(h->nr_nodes);
    *k = *v = NULL;
    return 0;
})

MLN_FUNC(, int, mln_hash_insert, (mln_hash_t *h, void *key, void *val), (h, key, val), {
    if (h->expandable && h->nr_nodes > h->threshold) {
        mln_hash_expand(h);
    }
    if (h->expandable && h->nr_nodes <= (h->threshold >> 3)) {
        mln_hash_reduce(h);
    }
    mln_u32_t index = h->hash(h, key);
    mln_hash_mgr_t *mgr = &(h->tbl[index]);
    mln_hash_entry_t *he = mln_hash_entry_new(h, mgr, key, val);
    if (he == NULL) return -1;
    mln_hash_entry_chain_add(&(mgr->head), &(mgr->tail), he);
    mln_hash_entry_iter_chain_add(&(h->iter_head), &(h->iter_tail), he);
    ++(h->nr_nodes);
    return 0;
})

MLN_FUNC_VOID(static inline, void, mln_hash_reduce, (mln_hash_t *h), (h), {
    mln_hash_mgr_t *old_tbl = h->tbl;
    mln_u32_t len = h->len;
    h->len = h->calc_prime? mln_prime_generate(h->threshold >> 2): h->threshold >> 2;
    if (h->len == 0) h->len = 1;
    if (h->pool != NULL) {
        h->tbl = (mln_hash_mgr_t *)h->pool_alloc(h->pool, h->len*sizeof(mln_hash_mgr_t));
        memset(h->tbl, 0, h->len*sizeof(mln_hash_mgr_t));
    } else {
        h->tbl = (mln_hash_mgr_t *)calloc(h->len, sizeof(mln_hash_mgr_t));
    }
    if (h->tbl == NULL) {
        h->tbl = old_tbl;
        h->len = len;
        return;
    }
    h->threshold = h->calc_prime? mln_prime_generate(h->threshold >> 1): h->threshold >> 1;
    mln_move_hash_entry(h, old_tbl, len);
    if (h->pool != NULL) h->pool_free(old_tbl);
    else free(old_tbl);
})

MLN_FUNC_VOID(static inline, void, mln_hash_expand, (mln_hash_t *h), (h), {
    mln_hash_mgr_t *old_tbl = h->tbl;
    mln_u32_t len = h->len;
    h->len = h->calc_prime? mln_prime_generate(len << 1): ((len << 1) - 1);
    if (h->pool != NULL) {
        h->tbl = (mln_hash_mgr_t *)h->pool_alloc(h->pool, h->len*sizeof(mln_hash_mgr_t));
        memset(h->tbl, 0, h->len*sizeof(mln_hash_mgr_t));
    } else {
        h->tbl = (mln_hash_mgr_t *)calloc(h->len, sizeof(mln_hash_mgr_t));
    }
    if (h->tbl == NULL) {
        h->tbl = old_tbl;
        h->len = len;
        return;
    }
    h->threshold = h->calc_prime? mln_prime_generate(h->threshold << 1): \
                                  ((h->threshold << 1) - 1);
    mln_move_hash_entry(h, old_tbl, len);
    if (h->pool != NULL) h->pool_free(old_tbl);
    else free(old_tbl);
})

MLN_FUNC_VOID(static inline, void, mln_move_hash_entry, \
              (mln_hash_t *h, mln_hash_mgr_t *old_tbl, mln_u32_t old_len), \
              (h, old_tbl, old_len), \
{
    mln_hash_mgr_t *old_end = old_tbl + old_len;
    mln_hash_mgr_t *new_mgr;
    mln_hash_entry_t *he;
    mln_u32_t index;

    for (; old_tbl < old_end; ++old_tbl) {
        while ((he = old_tbl->head) != NULL) {
            mln_hash_entry_chain_del(&(old_tbl->head), &(old_tbl->tail), he);
            index = h->hash(h, he->key);
            new_mgr = &(h->tbl[index]);
            mln_hash_entry_chain_add(&(new_mgr->head), &(new_mgr->tail), he);
        }
    }
})

MLN_FUNC(, void *, mln_hash_change_value, \
         (mln_hash_t *h, void *key, void *new_value), (h, key, new_value), \
{
    mln_u32_t index = h->hash(h, key);
    mln_hash_mgr_t *mgr = &(h->tbl[index]);
    mln_hash_entry_t *he;
    for (he = mgr->head; he != NULL; he = he->next) {
        if (h->cmp(h, key, he->key)) break;
    }
    if (he == NULL) return NULL;
    mln_u8ptr_t retval = (mln_u8ptr_t)(he->val);
    he->val = new_value;
    return retval;
})

MLN_FUNC(, void *, mln_hash_search, (mln_hash_t *h, void *key), (h, key), {
    mln_u32_t index = h->hash(h, key);
    mln_hash_mgr_t *mgr = &(h->tbl[index]);
    mln_hash_entry_t *he;
    for (he = mgr->head; he != NULL; he = he->next) {
        if (h->cmp(h, key, he->key)) break;
    }
    if (he == NULL || he->removed) return NULL;
    return he->val;
})

MLN_FUNC(, void *, mln_hash_search_iterator, \
         (mln_hash_t *h, void *key, int **ctx), (h, key, ctx), \
{
    if (*ctx != NULL) {
        mln_hash_entry_t *he = *((mln_hash_entry_t **)ctx);
        for (; he != NULL; he = he->next) {
            if (h->cmp(h, key, he->key)) break;
        }
        if (he == NULL || he->removed) {
            *ctx = NULL;
            return NULL;
        }
        *ctx = (int *)(he->next);
        return he->val;
    }
    mln_u32_t index = h->hash(h, key);
    mln_hash_mgr_t *mgr = &(h->tbl[index]);
    mln_hash_entry_t *he;
    for (he = mgr->head; he != NULL; he = he->next) {
        if (h->cmp(h, key, he->key)) break;
    }
    if (he == NULL || he->removed) return NULL;
    *ctx = (int *)(he->next);
    return he->val;
})

MLN_FUNC_VOID(, void, mln_hash_remove, \
              (mln_hash_t *h, void *key, mln_hash_flag_t flg), \
              (h, key, flg), \
{
    mln_u32_t index = h->hash(h, key);
    mln_hash_mgr_t *mgr = &(h->tbl[index]);
    mln_hash_entry_t *he;

    for (he = mgr->head; he != NULL; he = he->next) {
        if (h->cmp(h, key, he->key)) break;
    }
    if (he == NULL) return;

    if (h->iter == he) {
        he->remove_flag = flg;
        he->removed = 1;
        return;
    }

    mln_hash_entry_chain_del(&(mgr->head), &(mgr->tail), he);
    mln_hash_entry_iter_chain_del(&(h->iter_head), &(h->iter_tail), he);
    --(h->nr_nodes);
    mln_hash_entry_free(h, he, flg);
})

MLN_FUNC(static inline, mln_hash_entry_t *, mln_hash_entry_new, \
         (mln_hash_t *h, mln_hash_mgr_t *mgr, void *key, void *val), \
         (h, mgr, key, val), \
{
    mln_hash_entry_t *he;
    if (h->pool != NULL) {
        he = (mln_hash_entry_t *)h->pool_alloc(h->pool, sizeof(mln_hash_entry_t));
    } else {
        he = (mln_hash_entry_t *)malloc(sizeof(mln_hash_entry_t));
    }
    if (he == NULL) return NULL;
    he->val = val;
    he->key = key;
    he->prev = he->next = NULL;
    he->iter_prev = he->iter_next = NULL;
    he->mgr = mgr;
    he->remove_flag = M_HASH_F_NONE;
    he->removed = 0;
    return he;
})

MLN_FUNC_VOID(static inline, void, mln_hash_entry_free, \
              (mln_hash_t *h, mln_hash_entry_t *he, mln_hash_flag_t flg), \
              (h, he, flg), \
{
    if (he == NULL) return;
    switch (flg) {
        case M_HASH_F_VAL:
            if (h->val_freer != NULL)
                h->val_freer(he->val);
            break;
        case M_HASH_F_KEY:
            if (h->key_freer != NULL)
                h->key_freer(he->key);
            break;
        case M_HASH_F_KV:
            if (h->val_freer != NULL)
                h->val_freer(he->val);
            if (h->key_freer != NULL)
                h->key_freer(he->key);
            break;
        default: break;
    }
    if (h->pool != NULL) h->pool_free(he);
    else free(he);
})

MLN_FUNC(, int, mln_hash_iterate, \
         (mln_hash_t *h, hash_iterate_handler handler, void *udata), \
         (h, handler, udata), \
{
    mln_hash_entry_t *he = h->iter_head, *cur;

    while (he != NULL) {
        h->iter = cur = he;
        he = he->iter_next;

        if (cur->removed) continue;

        if (handler != NULL && handler(h, cur->key, cur->val, udata) < 0) {
            h->iter = NULL;
            return -1;
        }

        if (cur->removed) {
            mln_hash_entry_chain_del(&(cur->mgr->head), &(cur->mgr->tail), cur);
            mln_hash_entry_iter_chain_del(&(h->iter_head), &(h->iter_tail), cur);
            --(h->nr_nodes);
            mln_hash_entry_free(h, cur, cur->remove_flag);
        }
    }
    h->iter = NULL;
    return 0;
})

MLN_FUNC(, int, mln_hash_key_exist, (mln_hash_t *h, void *key), (h, key), {
    mln_u32_t index = h->hash(h, key);
    mln_hash_mgr_t *mgr = &(h->tbl[index]);
    mln_hash_entry_t *he;
    for (he = mgr->head; he != NULL; he = he->next) {
        if (!he->removed && h->cmp(h, key, he->key)) return 1;
    }
    return 0;
})

MLN_FUNC_VOID(, void, mln_hash_reset, (mln_hash_t *h, mln_hash_flag_t flg), (h, flg), {
    mln_hash_mgr_t *mgr, *end;
    mgr = h->tbl;
    end = h->tbl + h->len;
    mln_hash_entry_t *he;
    for (; mgr < end; ++mgr) {
        while ((he = mgr->head) != NULL) {
            mln_hash_entry_chain_del(&(mgr->head), &(mgr->tail), he);
            mln_hash_entry_free(h, he, flg);
        }
    }

    h->nr_nodes = 0;
    h->iter = h->iter_head = h->iter_tail = NULL;
})

MLN_CHAIN_FUNC_DEFINE(static inline, \
                      mln_hash_entry, \
                      mln_hash_entry_t, \
                      prev, \
                      next);
MLN_CHAIN_FUNC_DEFINE(static inline, \
                      mln_hash_entry_iter, \
                      mln_hash_entry_t, \
                      iter_prev, \
                      iter_next);

