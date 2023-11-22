
/*
 * Copyright (C) Niklaus F.Schen.
 */

#include <stdlib.h>
#include "mln_prime_generator.h"
#include "../include/mln_hash.h"
#include <stdio.h>
#include <string.h>

MLN_CHAIN_FUNC_DECLARE(mln_hash_entry, \
                       mln_hash_entry_t, \
                       static inline void,);
static inline mln_hash_entry_t *
mln_hash_entry_new(mln_hash_t *h, void *key, void *val) __NONNULL3(1,2,3);
static inline void
mln_hash_entry_free(mln_hash_t *h, mln_hash_entry_t *he, mln_hash_flag_t flg) __NONNULL1(1);
static inline void
mln_hash_reduce(mln_hash_t *h) __NONNULL1(1);
static inline void
mln_hash_expand(mln_hash_t *h) __NONNULL1(1);
static inline void
mln_move_hash_entry(mln_hash_t *h, mln_hash_mgr_t *old_tbl, mln_u32_t old_len) __NONNULL2(1,2);

int mln_hash_init(mln_hash_t *h, struct mln_hash_attr *attr)
{
    h->pool = attr->pool;
    h->pool_alloc = attr->pool_alloc;
    h->pool_free = attr->pool_free;
    h->hash = attr->hash;
    h->cmp = attr->cmp;
    h->free_key = attr->free_key;
    h->free_val = attr->free_val;
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
    return 0;
}

mln_hash_t *
mln_hash_new(struct mln_hash_attr *attr)
{
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
    h->free_key = attr->free_key;
    h->free_val = attr->free_val;
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
    return h;
}

void mln_hash_destroy(mln_hash_t *h, mln_hash_flag_t flg)
{
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
}

void mln_hash_free(mln_hash_t *h, mln_hash_flag_t flg)
{
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
}

int mln_hash_replace(mln_hash_t *h,void *key,void *val)
{
    void **k = (void **)key;
    void **v = (void **)val;
    mln_u32_t index = h->hash(h, *k);
    mln_hash_mgr_t *mgr = &(h->tbl[index]);
    mln_hash_entry_t *he;
    for (he = mgr->head; he != NULL; he = he->next) {
        if (h->cmp(h, *k, he->key)) break;
    }
    if (he != NULL) {
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
    he = mln_hash_entry_new(h, *k, *v);
    if (he == NULL) return -1;
    mln_hash_entry_chain_add(&(mgr->head), &(mgr->tail), he);
    ++(h->nr_nodes);
    *k = *v = NULL;
    return 0;
}

int mln_hash_insert(mln_hash_t *h, mln_hash_iterator_t* it)
{
    if(it->it_belong!= h || it->it_type != M_HASH_ZOMBIE) return -1;
    if(it->key == NULL) return -1;
    if (h->expandable && h->nr_nodes > h->threshold) {
        mln_hash_expand(h);
    }
    if (h->expandable && h->nr_nodes <= (h->threshold >> 3)) {
        mln_hash_reduce(h);
    }
    it->it_type = M_HASH_ITERATOR;
    mln_u32_t index = h->hash(h, it->key);
    mln_hash_mgr_t *mgr = &(h->tbl[index]);
    mln_hash_entry_t *he = it;
    mln_hash_entry_chain_add(&(mgr->head), &(mgr->tail), he);
    ++(h->nr_nodes);
    return 0;
}

static inline void mln_hash_reduce(mln_hash_t *h)
{
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
}

static inline void mln_hash_expand(mln_hash_t *h)
{
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
}

static inline void
mln_move_hash_entry(mln_hash_t *h, mln_hash_mgr_t *old_tbl, mln_u32_t old_len)
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
}

void *mln_hash_change_value(mln_hash_t *h, void *key, void *new_value)
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
}

mln_hash_iterator_t *mln_hash_search(mln_hash_t *h, void *key)
{
    mln_u32_t index = h->hash(h, key);
    mln_hash_mgr_t *mgr = &(h->tbl[index]);
    mln_hash_entry_t *he;
    for (he = mgr->head; he != NULL; he = he->next) {
        if (h->cmp(h, key, he->key)) break;
    }
    if (he == NULL) return NULL;
    return he;
}

mln_hash_iterator_t *mln_hash_search_iterator(mln_hash_t *h, void *key, int **ctx)
{
    if (*ctx != NULL) {
        mln_hash_entry_t *he = *((mln_hash_entry_t **)ctx);
        for (; he != NULL; he = he->next) {
            if (h->cmp(h, key, he->key)) break;
        }
        if (he == NULL) {
            *ctx = NULL;
            return NULL;
        }
        *ctx = (int *)(he->next);
        return he;
    }
    mln_u32_t index = h->hash(h, key);
    mln_hash_mgr_t *mgr = &(h->tbl[index]);
    mln_hash_entry_t *he;
    for (he = mgr->head; he != NULL; he = he->next) {
        if (h->cmp(h, key, he->key)) break;
    }
    if (he == NULL) return NULL;
    *ctx = (int *)(he->next);
    return he;
}

//this function will not free it
void mln_hash_remove(mln_hash_t *h, mln_hash_iterator_t * it)
{
    if(it->it_belong!= h || it->it_type != M_HASH_ITERATOR)
        return;
    mln_u32_t index = h->hash(h, it->key);
    mln_hash_mgr_t *mgr = &(h->tbl[index]);
    mln_hash_entry_t *he = it;    
    mln_hash_entry_chain_del(&(mgr->head), &(mgr->tail), he);
    --(h->nr_nodes);
    it->it_type = M_HASH_ZOMBIE;
}

static inline mln_hash_entry_t *
mln_hash_entry_new(mln_hash_t *h, void *key, void *val)
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
    he->it_belong = h;
    he->it_type = M_HASH_ZOMBIE;
    return he;
}

static inline void
mln_hash_entry_free(mln_hash_t *h, mln_hash_entry_t *he, mln_hash_flag_t flg)
{
    if (he == NULL) return;
    
    switch (flg) {
        case M_HASH_F_VAL:
            if (h->free_val != NULL)
                h->free_val(he->val);
            break;
        case M_HASH_F_KEY:
            if (h->free_key != NULL)
                h->free_key(he->key);
            break;
        case M_HASH_F_KV:
            if (h->free_val != NULL)
                h->free_val(he->val);
            if (h->free_key != NULL)
                h->free_key(he->key);
            break;
        default: break;
    }
    if (h->pool != NULL) h->pool_free(he);
    else free(he);
    he->it_belong = NULL;
    he->it_type = M_HASH_FREE;
}

int mln_hash_iterate(mln_hash_t *h, hash_iterate_handler handler, void *udata)
{
    mln_hash_mgr_t *mgr, *end;
    mgr = h->tbl;
    end = h->tbl + h->len;
    mln_hash_entry_t *he;
    for (; mgr < end; ++mgr) {
	mln_hash_entry_t* next_it;
        for (he = mgr->head; he != NULL; he = next_it) {
	    next_it = he->next;
            if (handler != NULL && handler(he, udata) < 0)
                return -1;
        }
    }
    return 0;
}

int mln_hash_key_exist(mln_hash_t *h, void *key)
{
    mln_u32_t index = h->hash(h, key);
    mln_hash_mgr_t *mgr = &(h->tbl[index]);
    mln_hash_entry_t *he;
    for (he = mgr->head; he != NULL; he = he->next) {
        if (h->cmp(h, key, he->key)) return 1;
    }
    return 0;
}

void mln_hash_reset(mln_hash_t *h, mln_hash_flag_t flg)
{
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
}


// null error
// not null key
void *mln_hash_iterator_key_get(mln_hash_iterator_t* it)
{
    if(it->it_type != M_HASH_ITERATOR) return NULL;
    return it->key;
}

void* mln_hash_iterator_value_set(mln_hash_iterator_t* it,void *val)
{
    if(it->it_type != M_HASH_ITERATOR) return NULL;
    void* retval = it->val;
    it->val = val;
    return retval;
}

// null error
// not null value
void *mln_hash_iterator_value_get(mln_hash_iterator_t* it)
{
    if(it->it_type != M_HASH_ITERATOR) return NULL;
    return it->val;
}


mln_hash_iterator_t * mln_hash_iterator_new(mln_hash_t* h,void *key,void *val)
{
    return mln_hash_entry_new(h,key,val);
}

//free iterator
//must be removed from chain or initalize
void mln_hash_iterator_free(mln_hash_iterator_t *it,mln_hash_flag_t flg)
{
    if(it->it_type != M_HASH_ZOMBIE) return;
    mln_hash_t* h = it->it_belong;
    if(h == NULL) return;
    mln_hash_entry_free(h,it,flg);
}

MLN_CHAIN_FUNC_DEFINE(mln_hash_entry, \
                      mln_hash_entry_t, \
                      static inline void, \
                      prev, \
                      next);

