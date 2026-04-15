
/*
 * Copyright (C) Niklaus F.Schen.
 */

#include <stdlib.h>
#include "mln_prime_generator.h"
#include "mln_hash.h"
#include "mln_func.h"
#include <stdio.h>
#include <string.h>

static inline void
mln_hash_entry_free_data(mln_hash_t *h, mln_hash_entry_t *he, mln_hash_flag_t flg) __NONNULL1(1);
static inline void
mln_hash_reduce(mln_hash_t *h) __NONNULL1(1);
static inline void
mln_hash_expand(mln_hash_t *h) __NONNULL1(1);
static inline void
mln_hash_rehash(mln_hash_t *h, mln_hash_entry_t *old_tbl, mln_u64_t old_len, mln_s32_t old_iter_head) __NONNULL2(1,2);
static inline void
mln_hash_compact(mln_hash_t *h) __NONNULL1(1);
static inline void
mln_hash_iter_chain_add(mln_hash_t *h, mln_s32_t idx) __NONNULL1(1);
static inline void
mln_hash_iter_chain_del(mln_hash_t *h, mln_s32_t idx) __NONNULL1(1);

MLN_FUNC(, int, mln_hash_init, (mln_hash_t *h, struct mln_hash_attr *attr), (h, attr), {
    h->pool = attr->pool;
    h->pool_alloc = attr->pool_alloc;
    h->pool_free = attr->pool_free;
    h->hash = attr->hash;
    h->cmp = attr->cmp;
    h->key_freer = attr->key_freer;
    h->val_freer = attr->val_freer;
    h->len = attr->calc_prime? mln_prime_generate(attr->len_base): attr->len_base;
    if (h->len == 0 || h->hash == NULL || h->cmp == NULL)
        return -1;
    if (h->pool != NULL) {
        h->tbl = (mln_hash_entry_t *)h->pool_alloc(h->pool, h->len * sizeof(mln_hash_entry_t));
    } else {
        h->tbl = (mln_hash_entry_t *)malloc(h->len * sizeof(mln_hash_entry_t));
    }
    if (h->tbl == NULL) return -1;
    memset(h->tbl, 0, h->len * sizeof(mln_hash_entry_t));
    h->nr_nodes = 0;
    h->nr_deleted = 0;
    h->threshold = (mln_u32_t)(h->len * 3 / 4);
    if (h->threshold == 0) h->threshold = 1;
    h->expandable = attr->expandable;
    h->calc_prime = attr->calc_prime;
    h->iter = NULL;
    h->iter_head = h->iter_tail = -1;
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
    if (h->len == 0 || h->hash == NULL || h->cmp == NULL)
        return -1;
    h->tbl = (mln_hash_entry_t *)malloc(h->len * sizeof(mln_hash_entry_t));
    if (h->tbl == NULL) return -1;
    memset(h->tbl, 0, h->len * sizeof(mln_hash_entry_t));
    h->nr_nodes = 0;
    h->nr_deleted = 0;
    h->threshold = (mln_u32_t)(h->len * 3 / 4);
    if (h->threshold == 0) h->threshold = 1;
    h->expandable = expandable;
    h->calc_prime = calc_prime;
    h->iter = NULL;
    h->iter_head = h->iter_tail = -1;
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
    if (mln_hash_init(h, attr) < 0) {
        if (attr->pool != NULL) attr->pool_free(h);
        else free(h);
        return NULL;
    }
    return h;
})

MLN_FUNC(, mln_hash_t *, mln_hash_new_fast, \
         (hash_calc_handler hash, hash_cmp_handler cmp, hash_free_handler key_freer, \
          hash_free_handler val_freer, mln_u64_t base_len, mln_u32_t expandable, mln_u32_t calc_prime), \
         (hash, cmp, key_freer, val_freer, base_len, expandable, calc_prime), \
{
    mln_hash_t *h = (mln_hash_t *)malloc(sizeof(mln_hash_t));
    if (h == NULL) return NULL;
    if (mln_hash_init_fast(h, hash, cmp, key_freer, val_freer, base_len, expandable, calc_prime) < 0) {
        free(h);
        return NULL;
    }
    return h;
})

MLN_FUNC_VOID(, void, mln_hash_destroy, (mln_hash_t *h, mln_hash_flag_t flg), (h, flg), {
    if (h == NULL) return;
    mln_hash_entry_t *tbl = h->tbl;
    mln_u64_t i, len = h->len;
    for (i = 0; i < len; ++i) {
        if (tbl[i].state == M_HASH_STATE_OCCUPIED)
            mln_hash_entry_free_data(h, &tbl[i], flg);
    }
    if (h->pool != NULL) h->pool_free(h->tbl);
    else free(h->tbl);
})

MLN_FUNC_VOID(, void, mln_hash_free, (mln_hash_t *h, mln_hash_flag_t flg), (h, flg), {
    if (h == NULL) return;
    mln_hash_entry_t *tbl = h->tbl;
    mln_u64_t i, len = h->len;
    for (i = 0; i < len; ++i) {
        if (tbl[i].state == M_HASH_STATE_OCCUPIED)
            mln_hash_entry_free_data(h, &tbl[i], flg);
    }
    if (h->pool != NULL) h->pool_free(h->tbl);
    else free(h->tbl);
    if (h->pool != NULL) h->pool_free(h);
    else free(h);
})

MLN_FUNC(, int, mln_hash_update, (mln_hash_t *h, void *key, void *val), (h, key, val), {
    void **k = (void **)key;
    void **v = (void **)val;
    mln_u64_t idx = h->hash(h, *k) % h->len;
    mln_hash_entry_t *tbl = h->tbl;
    mln_u64_t len = h->len;
    for (;;) {
        mln_hash_entry_t *e = &tbl[idx];
        if (e->state == M_HASH_STATE_EMPTY) break;
        if (e->state == M_HASH_STATE_OCCUPIED && h->cmp(h, *k, e->key)) {
            e->removed = 0;
            void *save_key = e->key;
            void *save_val = e->val;
            e->key = *k;
            e->val = *v;
            *k = save_key;
            *v = save_val;
            return 0;
        }
        if (++idx >= len) idx = 0;
    }

    if (h->expandable && (h->nr_nodes + h->nr_deleted) > h->threshold) {
        mln_hash_expand(h);
        tbl = h->tbl;
        len = h->len;
    }
    if (h->expandable && h->nr_nodes <= (h->threshold >> 3)) {
        mln_hash_reduce(h);
        tbl = h->tbl;
        len = h->len;
    }
    if (!h->expandable && (h->nr_nodes + h->nr_deleted + 1) >= len) {
        if (h->nr_deleted > 0) {
            mln_hash_compact(h);
            tbl = h->tbl;
        } else {
            return -1;
        }
    }

    idx = h->hash(h, *k) % len;
    for (;;) {
        mln_hash_entry_t *e = &tbl[idx];
        if (e->state != M_HASH_STATE_OCCUPIED) {
            if (e->state == M_HASH_STATE_DELETED) h->nr_deleted--;
            e->key = *k;
            e->val = *v;
            e->state = M_HASH_STATE_OCCUPIED;
            e->removed = 0;
            e->remove_flag = M_HASH_F_NONE;
            mln_hash_iter_chain_add(h, (mln_s32_t)idx);
            ++(h->nr_nodes);
            *k = *v = NULL;
            return 0;
        }
        if (++idx >= len) idx = 0;
    }
})

MLN_FUNC(, int, mln_hash_insert, (mln_hash_t *h, void *key, void *val), (h, key, val), {
    if (h->expandable && (h->nr_nodes + h->nr_deleted) > h->threshold) {
        mln_hash_expand(h);
    }
    if (h->expandable && h->nr_nodes <= (h->threshold >> 3)) {
        mln_hash_reduce(h);
    }
    if (!h->expandable && (h->nr_nodes + h->nr_deleted + 1) >= h->len) {
        if (h->nr_deleted > 0) {
            mln_hash_compact(h);
        } else {
            return -1;
        }
    }
    mln_u64_t idx = h->hash(h, key) % h->len;
    mln_hash_entry_t *tbl = h->tbl;
    mln_u64_t len = h->len;
    for (;;) {
        mln_hash_entry_t *e = &tbl[idx];
        if (e->state != M_HASH_STATE_OCCUPIED) {
            if (e->state == M_HASH_STATE_DELETED) h->nr_deleted--;
            e->key = key;
            e->val = val;
            e->state = M_HASH_STATE_OCCUPIED;
            e->removed = 0;
            e->remove_flag = M_HASH_F_NONE;
            mln_hash_iter_chain_add(h, (mln_s32_t)idx);
            ++(h->nr_nodes);
            return 0;
        }
        if (++idx >= len) idx = 0;
    }
})

MLN_FUNC_VOID(static inline, void, mln_hash_reduce, (mln_hash_t *h), (h), {
    mln_hash_entry_t *old_tbl = h->tbl;
    mln_u64_t old_len = h->len;
    mln_s32_t old_iter_head = h->iter_head;
    h->len = h->calc_prime? mln_prime_generate((mln_u32_t)(old_len >> 1)): (old_len >> 1);
    if (h->len == 0) h->len = 1;
    if (h->len <= h->nr_nodes + 1) h->len = h->nr_nodes + h->nr_nodes / 3 + 2;
    if (h->pool != NULL) {
        h->tbl = (mln_hash_entry_t *)h->pool_alloc(h->pool, h->len * sizeof(mln_hash_entry_t));
    } else {
        h->tbl = (mln_hash_entry_t *)malloc(h->len * sizeof(mln_hash_entry_t));
    }
    if (h->tbl == NULL) {
        h->tbl = old_tbl;
        h->len = old_len;
        return;
    }
    memset(h->tbl, 0, h->len * sizeof(mln_hash_entry_t));
    h->threshold = (mln_u32_t)(h->len * 3 / 4);
    if (h->threshold == 0) h->threshold = 1;
    mln_hash_rehash(h, old_tbl, old_len, old_iter_head);
    if (h->pool != NULL) h->pool_free(old_tbl);
    else free(old_tbl);
})

MLN_FUNC_VOID(static inline, void, mln_hash_expand, (mln_hash_t *h), (h), {
    mln_hash_entry_t *old_tbl = h->tbl;
    mln_u64_t old_len = h->len;
    mln_s32_t old_iter_head = h->iter_head;
    h->len = h->calc_prime? mln_prime_generate((mln_u32_t)(old_len << 1)): ((old_len << 1) - 1);
    if (h->pool != NULL) {
        h->tbl = (mln_hash_entry_t *)h->pool_alloc(h->pool, h->len * sizeof(mln_hash_entry_t));
    } else {
        h->tbl = (mln_hash_entry_t *)malloc(h->len * sizeof(mln_hash_entry_t));
    }
    if (h->tbl == NULL) {
        h->tbl = old_tbl;
        h->len = old_len;
        return;
    }
    memset(h->tbl, 0, h->len * sizeof(mln_hash_entry_t));
    h->threshold = (mln_u32_t)(h->len * 3 / 4);
    if (h->threshold == 0) h->threshold = 1;
    mln_hash_rehash(h, old_tbl, old_len, old_iter_head);
    if (h->pool != NULL) h->pool_free(old_tbl);
    else free(old_tbl);
})

MLN_FUNC_VOID(static inline, void, mln_hash_rehash, \
              (mln_hash_t *h, mln_hash_entry_t *old_tbl, mln_u64_t old_len, mln_s32_t old_iter_head), \
              (h, old_tbl, old_len, old_iter_head), \
{
    mln_s32_t oi = old_iter_head;
    mln_u64_t len = h->len;
    h->iter_head = h->iter_tail = -1;
    h->nr_deleted = 0;

    while (oi >= 0) {
        mln_hash_entry_t *oe = &old_tbl[oi];
        mln_s32_t next_oi = oe->iter_next;

        if (oe->state == M_HASH_STATE_OCCUPIED && !oe->removed) {
            mln_u64_t ni = h->hash(h, oe->key) % len;
            while (h->tbl[ni].state != M_HASH_STATE_EMPTY) {
                if (++ni >= len) ni = 0;
            }
            h->tbl[ni].key = oe->key;
            h->tbl[ni].val = oe->val;
            h->tbl[ni].state = M_HASH_STATE_OCCUPIED;
            h->tbl[ni].removed = 0;
            h->tbl[ni].remove_flag = M_HASH_F_NONE;
            mln_hash_iter_chain_add(h, (mln_s32_t)ni);
        }
        oi = next_oi;
    }
})

MLN_FUNC_VOID(static inline, void, mln_hash_compact, (mln_hash_t *h), (h), {
    mln_hash_entry_t *old_tbl = h->tbl;
    mln_u64_t old_len = h->len;
    mln_s32_t old_iter_head = h->iter_head;

    if (h->pool != NULL) {
        h->tbl = (mln_hash_entry_t *)h->pool_alloc(h->pool, h->len * sizeof(mln_hash_entry_t));
    } else {
        h->tbl = (mln_hash_entry_t *)malloc(h->len * sizeof(mln_hash_entry_t));
    }
    if (h->tbl == NULL) {
        h->tbl = old_tbl;
        return;
    }
    memset(h->tbl, 0, h->len * sizeof(mln_hash_entry_t));
    mln_hash_rehash(h, old_tbl, old_len, old_iter_head);
    if (h->pool != NULL) h->pool_free(old_tbl);
    else free(old_tbl);
})

MLN_FUNC(, void *, mln_hash_change_value, \
         (mln_hash_t *h, void *key, void *new_value), (h, key, new_value), \
{
    mln_u64_t idx = h->hash(h, key) % h->len;
    mln_hash_entry_t *tbl = h->tbl;
    mln_u64_t len = h->len;
    for (;;) {
        mln_hash_entry_t *e = &tbl[idx];
        if (e->state == M_HASH_STATE_EMPTY) return NULL;
        if (e->state == M_HASH_STATE_OCCUPIED && h->cmp(h, key, e->key)) {
            mln_u8ptr_t retval = (mln_u8ptr_t)(e->val);
            e->val = new_value;
            return retval;
        }
        if (++idx >= len) idx = 0;
    }
})

MLN_FUNC(, void *, mln_hash_search, (mln_hash_t *h, void *key), (h, key), {
    mln_u64_t idx = h->hash(h, key) % h->len;
    mln_hash_entry_t *tbl = h->tbl;
    mln_u64_t len = h->len;
    for (;;) {
        mln_hash_entry_t *e = &tbl[idx];
        if (e->state == M_HASH_STATE_EMPTY) return NULL;
        if (e->state == M_HASH_STATE_OCCUPIED && !e->removed && h->cmp(h, key, e->key))
            return e->val;
        if (++idx >= len) idx = 0;
    }
})

MLN_FUNC(, void *, mln_hash_search_iterator, \
         (mln_hash_t *h, void *key, int **ctx), (h, key, ctx), \
{
    mln_u64_t idx;
    mln_hash_entry_t *tbl = h->tbl;
    mln_u64_t len = h->len;

    if (*ctx != NULL) {
        idx = (mln_u64_t)(mln_uptr_t)(*ctx) - 1;
    } else {
        idx = h->hash(h, key) % len;
    }

    for (;;) {
        mln_hash_entry_t *e = &tbl[idx];
        if (e->state == M_HASH_STATE_EMPTY) {
            *ctx = NULL;
            return NULL;
        }
        if (e->state == M_HASH_STATE_OCCUPIED && !e->removed && h->cmp(h, key, e->key)) {
            mln_u64_t next = idx + 1;
            if (next >= len) next = 0;
            *ctx = (int *)(mln_uptr_t)(next + 1);
            return e->val;
        }
        if (++idx >= len) idx = 0;
    }
})

MLN_FUNC_VOID(, void, mln_hash_remove, \
              (mln_hash_t *h, void *key, mln_hash_flag_t flg), \
              (h, key, flg), \
{
    mln_u64_t idx = h->hash(h, key) % h->len;
    mln_hash_entry_t *tbl = h->tbl;
    mln_u64_t len = h->len;

    for (;;) {
        mln_hash_entry_t *e = &tbl[idx];
        if (e->state == M_HASH_STATE_EMPTY) return;
        if (e->state == M_HASH_STATE_OCCUPIED && h->cmp(h, key, e->key)) {
            if (h->iter == e) {
                e->remove_flag = flg;
                e->removed = 1;
                return;
            }
            mln_hash_iter_chain_del(h, (mln_s32_t)idx);
            mln_hash_entry_free_data(h, e, flg);
            e->state = M_HASH_STATE_DELETED;
            e->key = NULL;
            e->val = NULL;
            --(h->nr_nodes);
            ++(h->nr_deleted);
            return;
        }
        if (++idx >= len) idx = 0;
    }
})

MLN_FUNC_VOID(static inline, void, mln_hash_entry_free_data, \
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
})

MLN_FUNC(, int, mln_hash_iterate, \
         (mln_hash_t *h, hash_iterate_handler handler, void *udata), \
         (h, handler, udata), \
{
    mln_s32_t idx = h->iter_head;

    while (idx >= 0) {
        mln_hash_entry_t *cur = &h->tbl[idx];
        h->iter = cur;
        int aborted = 0;

        if (!cur->removed) {
            if (handler != NULL && handler(h, cur->key, cur->val, udata) < 0) {
                aborted = 1;
            }
        }

        mln_s32_t next_idx = cur->iter_next;

        if (cur->removed) {
            mln_hash_flag_t rf = cur->remove_flag;
            mln_hash_iter_chain_del(h, idx);
            mln_hash_entry_free_data(h, cur, rf);
            cur->state = M_HASH_STATE_DELETED;
            cur->key = NULL;
            cur->val = NULL;
            cur->removed = 0;
            --(h->nr_nodes);
            ++(h->nr_deleted);
        }

        if (aborted) {
            h->iter = NULL;
            return -1;
        }

        idx = next_idx;
    }
    h->iter = NULL;
    return 0;
})

MLN_FUNC(, int, mln_hash_key_exist, (mln_hash_t *h, void *key), (h, key), {
    mln_u64_t idx = h->hash(h, key) % h->len;
    mln_hash_entry_t *tbl = h->tbl;
    mln_u64_t len = h->len;
    for (;;) {
        mln_hash_entry_t *e = &tbl[idx];
        if (e->state == M_HASH_STATE_EMPTY) return 0;
        if (e->state == M_HASH_STATE_OCCUPIED && !e->removed && h->cmp(h, key, e->key))
            return 1;
        if (++idx >= len) idx = 0;
    }
})

MLN_FUNC_VOID(, void, mln_hash_reset, (mln_hash_t *h, mln_hash_flag_t flg), (h, flg), {
    mln_hash_entry_t *tbl = h->tbl;
    mln_u64_t i, len = h->len;
    for (i = 0; i < len; ++i) {
        if (tbl[i].state == M_HASH_STATE_OCCUPIED)
            mln_hash_entry_free_data(h, &tbl[i], flg);
    }
    memset(h->tbl, 0, h->len * sizeof(mln_hash_entry_t));
    h->nr_nodes = 0;
    h->nr_deleted = 0;
    h->iter = NULL;
    h->iter_head = h->iter_tail = -1;
})

MLN_FUNC_VOID(static inline, void, mln_hash_iter_chain_add, \
              (mln_hash_t *h, mln_s32_t idx), (h, idx), \
{
    mln_hash_entry_t *e = &h->tbl[idx];
    e->iter_prev = h->iter_tail;
    e->iter_next = -1;
    if (h->iter_tail >= 0) {
        h->tbl[h->iter_tail].iter_next = idx;
    } else {
        h->iter_head = idx;
    }
    h->iter_tail = idx;
})

MLN_FUNC_VOID(static inline, void, mln_hash_iter_chain_del, \
              (mln_hash_t *h, mln_s32_t idx), (h, idx), \
{
    mln_hash_entry_t *e = &h->tbl[idx];
    if (e->iter_prev >= 0) {
        h->tbl[e->iter_prev].iter_next = e->iter_next;
    } else {
        h->iter_head = e->iter_next;
    }
    if (e->iter_next >= 0) {
        h->tbl[e->iter_next].iter_prev = e->iter_prev;
    } else {
        h->iter_tail = e->iter_prev;
    }
    e->iter_prev = e->iter_next = -1;
})
