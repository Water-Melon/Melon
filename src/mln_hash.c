
/*
 * Copyright (C) Niklaus F.Schen.
 */

#include <stdlib.h>
#include "mln_prime_generator.h"
#include "mln_hash.h"
#include "mln_log.h"

static inline mln_hash_entry_t *
mln_new_hash_entry(void *key, void *val) __NONNULL2(1,2);
static inline void
mln_free_hash_entry(mln_hash_t *h, mln_hash_entry_t *he, enum mln_hash_flag flg) __NONNULL1(1);
static inline void
mln_unchain_entry(mln_hash_mgr_t *mgr, mln_hash_entry_t *he) __NONNULL2(1,2);
static inline void
mln_hash_reduce(mln_hash_t *h) __NONNULL1(1);
static inline void
mln_hash_expand(mln_hash_t *h) __NONNULL1(1);
static inline void
mln_move_hash_entry(mln_hash_t *h, mln_hash_mgr_t *old_tbl, mln_u32_t old_len) __NONNULL2(1,2);

mln_hash_t *
mln_hash_init(struct mln_hash_attr *attr)
{
    mln_hash_t *h = (mln_hash_t *)malloc(sizeof(mln_hash_t));
    if (h == NULL) return NULL;
    h->expandable = attr->expandable;
    h->len = mln_calc_prime(attr->len_base);
    h->tbl = (mln_hash_mgr_t *)calloc(h->len, sizeof(mln_hash_mgr_t));
    if (h->tbl == NULL) {
        free(h);
        return NULL;
    }
    h->hash = attr->hash;
    h->cmp = attr->cmp;
    h->free_key = attr->free_key;
    h->free_val = attr->free_val;
    h->nr_nodes = 0;
    h->threshold = mln_calc_prime(h->len << 1);
    if (h->len == 0 || \
            h->hash == NULL || \
            h->cmp == NULL)
    {
        free(h->tbl);
        free(h);
        return NULL;
    }
    return h;
}

void
mln_hash_destroy(mln_hash_t *h, enum mln_hash_flag flg)
{
    mln_hash_entry_t *he;
    mln_hash_mgr_t *mgr;
    for (mgr = h->tbl; mgr < h->tbl+h->len; mgr++) {
        while (mgr->head != NULL) {
            he = mgr->head;
            mln_unchain_entry(mgr, he);
            h->nr_nodes--;
            mln_free_hash_entry(h, he, flg);
        }
    }
    free(h->tbl);
    free(h);
}

int mln_hash_insert(mln_hash_t *h, void *key, void *val)
{
    if (h->expandable && h->nr_nodes > h->threshold) {
        mln_hash_expand(h);
    }
    if (h->expandable && h->nr_nodes <= (h->threshold >> 3)) {
        mln_hash_reduce(h);
    }
    mln_u32_t index = h->hash(h, key);
    if (index >= h->len) {
        mln_log(error, "fatal error: index >= hash table length.\n");
        abort();
    }
    mln_hash_mgr_t *mgr = &(h->tbl[index]);
    mln_hash_entry_t *he = mln_new_hash_entry(key, val);
    if (he == NULL) return -1;
    if (mgr->head == NULL) {
        mgr->head = mgr->tail = he;
    } else {
        mgr->head->prev = he;
        he->next = mgr->head;
        mgr->head = he;
    }
    h->nr_nodes++;
    return 0;
}

static inline void mln_hash_reduce(mln_hash_t *h)
{
    if (h->len == 0) {
        mln_log(error, "fatal error: hash table length is 0.\n");
        abort();
    }
    mln_hash_mgr_t *old_tbl = h->tbl;
    mln_u32_t len = h->len;
    h->len = mln_calc_prime(h->threshold >> 2);
    h->tbl = (mln_hash_mgr_t *)calloc(h->len, sizeof(mln_hash_mgr_t));
    if (h->tbl == NULL) {
        h->tbl = old_tbl;
        h->len = len;
        return;
    }
    h->threshold = mln_calc_prime(h->threshold >> 1);
    mln_move_hash_entry(h, old_tbl, len);
    free(old_tbl);
}

static inline void mln_hash_expand(mln_hash_t *h)
{
    if (h->len == 0) {
        mln_log(error, "fatal error: hash table length is 0.\n");
        abort();
    }
    mln_hash_mgr_t *old_tbl = h->tbl;
    mln_u32_t len = h->len;
    h->len = mln_calc_prime(len + (len >> 1));
    h->tbl = (mln_hash_mgr_t *)calloc(h->len, sizeof(mln_hash_mgr_t));
    if (h->tbl == NULL) {
        h->tbl = old_tbl;
        h->len = len;
        return;
    }
    h->threshold = mln_calc_prime(h->threshold + (h->threshold >> 1));
    mln_move_hash_entry(h, old_tbl,len);
    free(old_tbl);
}

static inline void
mln_move_hash_entry(mln_hash_t *h, mln_hash_mgr_t *old_tbl, mln_u32_t old_len)
{
    mln_hash_mgr_t *mgr, *new;
    mln_hash_entry_t *he;
    mln_u32_t i, index;
    for (i = 0; i<old_len; i++) {
        mgr = &(old_tbl[i]);
        while (mgr->head != NULL) {
            he = mgr->tail;
            mln_unchain_entry(mgr, he);
            index = h->hash(h, he->key);
            if (index >= h->len) {
                mln_log(error, "fatal error: index >= hash table length.\n");
                abort();
            }
            new = &(h->tbl[index]);
            if (new->head == NULL) {
                new->head = new->tail = he;
            } else {
                new->head->prev = he;
                he->next = new->head;
                new->head = he;
            }
        }
    }
}

void *mln_hash_search(mln_hash_t *h, void *key)
{
    mln_u32_t index = h->hash(h, key);
    if (index >= h->len) {
        mln_log(error, "fatal error: index >= hash table length.\n");
        abort();
    }
    mln_hash_mgr_t *mgr = &(h->tbl[index]);
    mln_hash_entry_t *he;
    for (he = mgr->head; he != NULL; he = he->next) {
        if (h->cmp(h, key, he->key)) break;
    }
    if (he == NULL) return NULL;
    return he->val;
}

void mln_hash_remove(mln_hash_t *h, void *key, enum mln_hash_flag flg)
{
    mln_u32_t index = h->hash(h, key);
    if (index >= h->len) {
        mln_log(error, "fatal error: index >= hash table length.\n");
        abort();
    }
    mln_hash_mgr_t *mgr = &(h->tbl[index]);
    mln_hash_entry_t *he;
    for (he = mgr->head; he != NULL; he = he->next) {
        if (h->cmp(h, key, he->key)) break;
    }
    if (he == NULL) return ;
    mln_unchain_entry(mgr, he);
    h->nr_nodes--;
    mln_free_hash_entry(h, he, flg);
}

static inline void
mln_unchain_entry(mln_hash_mgr_t *mgr, mln_hash_entry_t *he)
{
    if (he == mgr->head) {
        if (he == mgr->tail) {
            mgr->head = mgr->tail = NULL;
        } else {
            mgr->head = he->next;
            he->next->prev = NULL;
        }
    } else {
        if (he == mgr->tail) {
            mgr->tail = he->prev;
            he->prev->next = NULL;
        } else {
            he->prev->next = he->next;
            he->next->prev = he->prev;
        }
    }
    he->prev = he->next = NULL;
}

static inline mln_hash_entry_t *
mln_new_hash_entry(void *key, void *val)
{
    mln_hash_entry_t *he;
    he = (mln_hash_entry_t *)malloc(sizeof(mln_hash_entry_t));
    if (he == NULL) return NULL;
    he->val = val;
    he->key = key;
    he->prev = he->next = NULL;
    return he;
}

static inline void
mln_free_hash_entry(mln_hash_t *h, mln_hash_entry_t *he, enum mln_hash_flag flg)
{
    if (he == NULL) return;
    switch (flg) {
        case hash_only_free_val:
            if (h->free_val != NULL)
                h->free_val(he->val);
            break;
        case hash_only_free_key:
            if (h->free_key != NULL)
                h->free_key(he->key);
            break;
        case hash_free_key_val:
            if (h->free_val != NULL)
                h->free_val(he->val);
            if (h->free_key != NULL)
                h->free_key(he->key);
            break;
        default: break;
    }
    free(he);
}

