
/*
 * Copyright (C) Niklaus F.Schen.
 */

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "mln_json.h"
#include "mln_func.h"

static inline int
mln_json_parse_json(mln_json_t *j, char *jstr, int len, mln_uauto_t index, mln_json_policy_t *policy, int obj_key, mln_size_t depth);
static inline int
mln_json_parse_obj(mln_json_t *val, char *jstr, int len, mln_uauto_t index, mln_json_policy_t *policy, mln_size_t depth);
static inline int
mln_json_parse_array(mln_json_t *val, char *jstr, int len, mln_uauto_t index, mln_json_policy_t *policy, mln_size_t depth);
static inline int
mln_json_parse_string(mln_json_t *j, char *jstr, int len, mln_uauto_t index, mln_json_policy_t *policy, int obj_key);
static mln_string_t *
mln_json_parse_string_alloc(mln_u8ptr_t jstr, int *len);
static void mln_json_encode_utf8(unsigned int u, mln_u8ptr_t *b, int *count);
static inline int mln_json_get_char(mln_u8ptr_t *s, int *len, unsigned int *hex);
static int
mln_json_parse_digit(mln_json_t *j, char *jstr, int len, mln_uauto_t index);
static inline int
mln_json_digit_process(double *val, char *s, int len);
static inline int
mln_json_parse_true(mln_json_t *j, char *jstr, int len, mln_uauto_t index);
static inline int
mln_json_parse_false(mln_json_t *j, char *jstr, int len, mln_uauto_t index);
static inline int
mln_json_parse_null(mln_json_t *j, char *jstr, int len, mln_uauto_t index);
static inline void mln_json_jumpoff_blank(char **jstr, int *len);
static inline int
mln_json_write_content(mln_json_t *j, mln_s8ptr_t *buf, mln_size_t *size, mln_size_t *off, mln_u32_t flags);
static inline int mln_json_parse_is_index(mln_string_t *s, mln_size_t *idx);
static inline int mln_json_obj_generate(mln_json_t *j, char **fmt, va_list *arg);
static inline int mln_json_array_generate(mln_json_t *j, char **fmt, va_list *arg);
static inline int __mln_json_obj_update(mln_json_t *j, mln_json_t *key, mln_json_t *val);
static inline int __mln_json_array_append(mln_json_t *j, mln_json_t *value);


MLN_CHAIN_FUNC_DEFINE(static inline, mln_json_kv, mln_json_kv_t, iter_prev, iter_next);

static inline mln_u32_t mln_json_obj_hash(mln_string_t *s, mln_u32_t len)
{
    mln_u64_t h = 5381;
    mln_u8ptr_t p = s->data, end = s->data + s->len;
    while (p < end)
        h = ((h << 5) + h) + *p++;
    return (mln_u32_t)(h % len);
}

MLN_FUNC(static inline, int, mln_json_kv_is_pooled, \
         (mln_json_kv_t *kv, mln_json_obj_t *obj), (kv, obj), \
{
    return (kv >= obj->pool && kv < obj->pool + obj->pool_cap);
})

MLN_FUNC_VOID(static inline, void, mln_json_kv_free, \
              (mln_json_kv_t *kv, mln_json_obj_t *obj), (kv, obj), \
{
    if (kv == NULL) return;
    mln_json_destroy(&(kv->key));
    mln_json_destroy(&(kv->val));
    if (mln_json_kv_is_pooled(kv, obj)) {
        /* Return to freelist for reuse; reuse kv->next as link */
        kv->next = obj->freelist;
        obj->freelist = kv;
    } else {
        free(kv);
    }
})


MLN_FUNC(static inline, mln_json_kv_t *, mln_json_kv_alloc, \
         (mln_json_obj_t *obj), (obj), \
{
    if (obj->freelist != NULL) {
        mln_json_kv_t *kv = obj->freelist;
        obj->freelist = kv->next;
        return kv;
    }
    if (obj->pool_used < obj->pool_cap)
        return &obj->pool[obj->pool_used++];
    return (mln_json_kv_t *)malloc(sizeof(mln_json_kv_t));
})


static inline int __mln_json_obj_init(mln_json_t *j)
{
    /*
     * Single allocation: obj struct + bucket array + KV pool.
     * Layout: [mln_json_obj_t][mln_json_kv_t *tbl[M_JSON_LEN]][mln_json_kv_t pool[M_JSON_OBJ_POOL]]
     *
     * Use malloc + targeted memset instead of calloc: only the bucket array
     * needs zeroing (NULL pointers). Pool slots are initialized on use.
     * Saves zeroing ~1200 bytes of pool memory.
     */
    mln_size_t bucket_bytes = M_JSON_LEN * sizeof(mln_json_kv_t *);
    mln_size_t pool_bytes = M_JSON_OBJ_POOL * sizeof(mln_json_kv_t);
    mln_size_t total = sizeof(mln_json_obj_t) + bucket_bytes + pool_bytes;
    mln_json_obj_t *obj = (mln_json_obj_t *)malloc(total);
    if (obj == NULL) return -1;
    obj->tbl = (mln_json_kv_t **)((mln_u8ptr_t)obj + sizeof(mln_json_obj_t));
    obj->pool = (mln_json_kv_t *)((mln_u8ptr_t)obj->tbl + bucket_bytes);
    memset(obj->tbl, 0, bucket_bytes); /* only zero the bucket array */
    obj->len = M_JSON_LEN;
    obj->head = obj->tail = NULL;
    obj->freelist = NULL;
    obj->nr_nodes = 0;
    obj->pool_used = 0;
    obj->pool_cap = M_JSON_OBJ_POOL;
    j->type = M_JSON_OBJECT;
    j->data.m_j_obj = obj;
    return 0;
}

MLN_FUNC(, int, mln_json_obj_init, (mln_json_t *j), (j), {
    return __mln_json_obj_init(j);
})

MLN_FUNC(, int, mln_json_obj_update, \
         (mln_json_t *j, mln_json_t *key, mln_json_t *val), (j, key, val), \
{
    return __mln_json_obj_update(j, key, val);
})

/*
 * Fast insert: skip search for duplicates. Used during decode where
 * keys are guaranteed unique (JSON spec allows duplicates but most
 * parsers treat them as undefined behavior, and ours overwrites).
 */
MLN_FUNC(static inline, int, __mln_json_obj_insert, \
         (mln_json_obj_t *obj, mln_json_t *key, mln_json_t *val), (obj, key, val), \
{
    mln_string_t *skey = mln_json_string_data_get(key);
    mln_u32_t idx = mln_json_obj_hash(skey, obj->len);
    mln_json_kv_t *kv = mln_json_kv_alloc(obj);
    if (kv == NULL) return -1;
    kv->key = *key;
    kv->val = *val;
    kv->next = obj->tbl[idx];
    obj->tbl[idx] = kv;
    mln_json_kv_chain_add(&obj->head, &obj->tail, kv);
    ++(obj->nr_nodes);
    return 0;
})

static inline int __mln_json_obj_update(mln_json_t *j, mln_json_t *key, mln_json_t *val)
{
    mln_json_obj_t *obj;
    mln_json_kv_t *kv, **pp;
    mln_string_t *skey;
    mln_u32_t idx;

    if (!mln_json_is_string(key) || !mln_json_is_object(j)) return -1;

    obj = mln_json_object_data_get(j);
    skey = mln_json_string_data_get(key);
    idx = mln_json_obj_hash(skey, obj->len);

    /* search existing */
    for (pp = &obj->tbl[idx]; *pp != NULL; pp = &(*pp)->next) {
        kv = *pp;
        if (mln_json_string_data_get(&(kv->key))->len == skey->len &&
            !memcmp(mln_json_string_data_get(&(kv->key))->data, skey->data, skey->len))
        {
            /* found: update in place */
            mln_json_destroy(&(kv->key));
            mln_json_destroy(&(kv->val));
            kv->key = *key;
            kv->val = *val;
            return 0;
        }
    }

    /* not found: insert new */
    kv = mln_json_kv_alloc(obj);
    if (kv == NULL) return -1;
    kv->key = *key;
    kv->val = *val;
    kv->next = obj->tbl[idx];
    obj->tbl[idx] = kv;
    mln_json_kv_chain_add(&obj->head, &obj->tail, kv);
    ++(obj->nr_nodes);
    return 0;
}

mln_json_t *mln_json_obj_search(mln_json_t *j, mln_string_t *key)
{
    mln_json_obj_t *obj;
    mln_json_kv_t *kv;
    mln_u32_t idx;

    if (!mln_json_is_object(j)) return NULL;

    obj = mln_json_object_data_get(j);
    idx = mln_json_obj_hash(key, obj->len);
    for (kv = obj->tbl[idx]; kv != NULL; kv = kv->next) {
        mln_string_t *k = mln_json_string_data_get(&(kv->key));
        if (k->len == key->len && !memcmp(k->data, key->data, key->len))
            return &(kv->val);
    }
    return NULL;
}

void mln_json_obj_remove(mln_json_t *j, mln_string_t *key)
{
    mln_json_obj_t *obj;
    mln_json_kv_t *kv, **pp;
    mln_u32_t idx;

    if (!mln_json_is_object(j)) return;

    obj = mln_json_object_data_get(j);
    idx = mln_json_obj_hash(key, obj->len);

    for (pp = &obj->tbl[idx]; *pp != NULL; pp = &(*pp)->next) {
        kv = *pp;
        mln_string_t *k = mln_json_string_data_get(&(kv->key));
        if (k->len == key->len && !memcmp(k->data, key->data, key->len)) {
            /* unlink from hash chain */
            *pp = kv->next;
            /* unlink from iteration list */
            mln_json_kv_chain_del(&obj->head, &obj->tail, kv);
            mln_json_kv_free(kv, obj);
            --(obj->nr_nodes);
            return;
        }
    }
}


MLN_FUNC(static inline, int, __mln_json_array_init, (mln_json_t *j), (j), {
    if ((j->data.m_j_array = mln_array_new((array_free)mln_json_destroy, sizeof(mln_json_t), M_JSON_ARRAY_NALLOC)) == NULL)
        return -1;
    mln_json_array_type_set(j);
    return 0;
})

MLN_FUNC(, int, mln_json_array_init, (mln_json_t *j), (j), {
    return __mln_json_array_init(j);
})

MLN_FUNC(, mln_json_t *, mln_json_array_search, (mln_json_t *j, mln_uauto_t index), (j, index), {
    if (!mln_json_is_array(j)) return NULL;

    if (index >= mln_array_nelts(mln_json_array_data_get(j))) return NULL;

    return &(((mln_json_t *)mln_array_elts(mln_json_array_data_get(j)))[index]);
})

MLN_FUNC(, mln_uauto_t, mln_json_array_length, (mln_json_t *j), (j), {
    if (!mln_json_is_array(j)) return 0;
    return mln_array_nelts(mln_json_array_data_get(j));
})

MLN_FUNC(, int, mln_json_array_append, (mln_json_t *j, mln_json_t *value), (j, value), {
    return __mln_json_array_append(j, value);
})

MLN_FUNC(static inline, int, __mln_json_array_append, (mln_json_t *j, mln_json_t *value), (j, value), {
    mln_json_t *elem;

    if (!mln_json_is_array(j)) return -1;

    if ((elem = (mln_json_t *)mln_array_push(mln_json_array_data_get(j))) == NULL)
        return -1;
    *elem = *value;
    return 0;
})

MLN_FUNC(, int, mln_json_array_update, \
         (mln_json_t *j, mln_json_t *value, mln_uauto_t index), (j, value, index), \
{
    if (!mln_json_is_array(j)) return -1;

    mln_array_t *arr = mln_json_array_data_get(j);
    if (index >= mln_array_nelts(arr)) return -1;

    mln_json_destroy(&(((mln_json_t *)mln_array_elts(arr))[index]));
    ((mln_json_t *)mln_array_elts(arr))[index] = *value;
    return 0;
})

MLN_FUNC_VOID(, void, mln_json_array_remove, (mln_json_t *j, mln_uauto_t index), (j, index), {
    if (j == NULL || !mln_json_is_array(j)) return;

    ASSERT(index == mln_array_nelts(mln_json_array_data_get(j)) - 1);

    mln_array_pop(mln_json_array_data_get(j));
})

/*
 * Inline destroy for leaf JSON types inside destroy loops.
 * Avoids full mln_json_destroy function call overhead for each KV.
 */
MLN_FUNC_VOID(static inline, void, mln_json_destroy_inline, \
              (mln_json_t *j), (j), \
{
    switch (j->type) {
        case M_JSON_STRING:
            mln_string_free(j->data.m_j_string);
            break;
        case M_JSON_OBJECT:
        case M_JSON_ARRAY:
            mln_json_destroy(j);
            break;
        default: /* NUM, TRUE, FALSE, NULL, NONE — nothing to free */
            break;
    }
})

void mln_json_destroy(mln_json_t *j)
{
    if (j == NULL) return;

    switch (j->type) {
        case M_JSON_OBJECT:
        {
            mln_json_obj_t *obj = mln_json_object_data_get(j);
            if (obj != NULL) {
                mln_json_kv_t *kv = obj->head, *next;
                while (kv != NULL) {
                    next = kv->iter_next;
                    /* Key is always a string — inline free */
                    mln_string_free(kv->key.data.m_j_string);
                    /* Value may be any type — use inline dispatch */
                    mln_json_destroy_inline(&(kv->val));
                    if (!mln_json_kv_is_pooled(kv, obj))
                        free(kv);
                    kv = next;
                }
                free(obj);
            }
            break;
        }
        case M_JSON_ARRAY:
            mln_array_free(mln_json_array_data_get(j));
            break;
        case M_JSON_STRING:
            mln_string_free(j->data.m_j_string);
            break;
        case M_JSON_NONE:
        case M_JSON_NUM:
        case M_JSON_TRUE:
        case M_JSON_FALSE:
        case M_JSON_NULL:
        default:
            break;
    }
}


/*
 * dump
 */
MLN_FUNC_VOID(, void, mln_json_dump, \
              (mln_json_t *j, int n_space, char *prefix), (j, n_space, prefix), \
{
    if (j == NULL) return;

    int i, space = n_space + 2;
    for (i = 0; i < n_space; ++i)
        printf(" ");

    if (prefix != NULL) {
        printf("%s ", prefix);
    }
    switch (j->type) {
        case M_JSON_OBJECT:
            printf("type:object\n");
            {
                mln_json_kv_t *_kv;
                for (_kv = mln_json_object_data_get(j)->head; _kv != NULL; _kv = _kv->iter_next) {
                    mln_json_dump(&(_kv->key), space, "Object key:");
                    mln_json_dump(&(_kv->val), space, "Object value:");
                }
            }
            break;
        case M_JSON_ARRAY:
        {
            printf("type:array\n");
            mln_json_t *elem = (mln_json_t *)mln_array_elts(mln_json_array_data_get(j));
            mln_json_t *end = elem + mln_array_nelts(mln_json_array_data_get(j));
            for (; elem < end; ++elem) {
                mln_json_dump(elem, space, "Array member:");
            }
            break;
        }
        case M_JSON_STRING:
            if (j->data.m_j_string != NULL && j->data.m_j_string->data != NULL)
                printf("type:string val:[%s]\n", (char *)(j->data.m_j_string->data));
            break;
        case M_JSON_NUM:
            printf("type:number val:[%f]\n", j->data.m_j_number);
            break;
        case M_JSON_TRUE:
            printf("type:true val:[true]\n");
            break;
        case M_JSON_FALSE:
            printf("type:false val:[false]\n");
            break;
        case M_JSON_NULL:
            printf("type:NULL val:[NULL]\n");
            break;
        default:
            printf("type:none\n");
            break;
    }
})


/*
 * decode
 */
MLN_FUNC(, int, mln_json_decode, (mln_string_t *jstr, mln_json_t *out, mln_json_policy_t *policy), (jstr, out, policy), {
    if (jstr == NULL || out == NULL || jstr->len == 0) {
        return -1;
    }

    mln_json_init(out);
    if (mln_json_parse_json(out, (char *)(jstr->data), jstr->len, 0, policy, 0, 0) != 0) {
        mln_json_destroy(out);
        return -1;
    }
    if (!mln_json_is_object(out) && !mln_json_is_array(out)) {
        mln_json_destroy(out);
        return -1;
    }
    return 0;
})

MLN_FUNC(static inline, int, mln_json_parse_json, \
         (mln_json_t *j, char *jstr, int len, mln_uauto_t index, mln_json_policy_t *policy, int obj_key, mln_size_t depth), \
         (j, jstr, len, index, policy, obj_key, depth), \
{
    if (jstr == NULL) {
        return -1;
    }

    mln_json_jumpoff_blank(&jstr, &len);
    if (len <= 0) {
        return len;
    }

    switch (jstr[0]) {
        case '{':
            return mln_json_parse_obj(j, jstr, len, index, policy, depth);
        case '[':
            return mln_json_parse_array(j, jstr, len, index, policy, depth);
        case '\"':
            return mln_json_parse_string(j, jstr, len, index, policy, obj_key);
        default:
            if (mln_isdigit(jstr[0]) || jstr[0] == '-') {
                return mln_json_parse_digit(j, jstr, len, index);
            }
            /* Dispatch by first char to avoid sequential tries */
            switch (jstr[0]) {
                case 't': case 'T':
                    return mln_json_parse_true(j, jstr, len, index);
                case 'f': case 'F':
                    return mln_json_parse_false(j, jstr, len, index);
                case 'n': case 'N':
                    return mln_json_parse_null(j, jstr, len, index);
                default:
                    break;
            }
            break;
    }
    return -1;
})

MLN_FUNC(static inline, int, mln_json_parse_obj, \
         (mln_json_t *val, char *jstr, int len, mln_uauto_t index, mln_json_policy_t *policy, mln_size_t depth), \
         (val, jstr, len, index, policy, depth), \
{
    int left;
    mln_json_t key, v;

    /*jump off '{'*/
    ++jstr;
    if (--len <= 0) {
        return -1;
    }

    if (__mln_json_obj_init(val) < 0) return -1;

    if (policy != NULL && policy->depth && depth + 1 > policy->depth) {
        policy->error = M_JSON_DEPTH;
        return -1;
    }

again:
    mln_json_init(&key);
    mln_json_init(&v);
    mln_json_jumpoff_blank(&jstr, &len);
    if (len <= 0) {
        mln_json_destroy(&key);
        mln_json_destroy(&v);
        return -1;
    }
    if (jstr[0] != '}') {
        /*
         * Inline key parsing: JSON object keys are always strings, so we
         * skip the full mln_json_parse_json dispatch and go directly to
         * string parsing. Also skip the redundant jumpoff_blank in
         * parse_json since we already called it above.
         */
        if (jstr[0] != '\"') {
            mln_json_destroy(&key);
            mln_json_destroy(&v);
            return -1;
        }
        left = mln_json_parse_string(&key, jstr, len, 0, policy, 1);
        if (left <= 0) {
            mln_json_destroy(&key);
            mln_json_destroy(&v);
            return -1;
        }
        jstr += (len - left);
        len = left;
        if (len <= 0) {
            mln_json_destroy(&key);
            mln_json_destroy(&v);
            return -1;
        }

        /*jump off ':' — skip whitespace only if needed */
        if (jstr[0] != ':') {
            mln_json_jumpoff_blank(&jstr, &len);
            if (len <= 0 || jstr[0] != ':') {
                mln_json_destroy(&key);
                mln_json_destroy(&v);
                return -1;
            }
        }
        ++jstr;
        if (--len <= 0) {
            mln_json_destroy(&key);
            mln_json_destroy(&v);
            return -1;
        }

        left = mln_json_parse_json(&v, jstr, len, 0, policy, 0, depth + 1);
        if (left <= 0) {
            mln_json_destroy(&key);
            mln_json_destroy(&v);
            return -1;
        }
        jstr += (len - left);
        len = left;

        /* Skip jumpoff_blank when next char is already a delimiter */
        if (len > 0 && jstr[0] != ',' && jstr[0] != '}') {
            mln_json_jumpoff_blank(&jstr, &len);
        }
        if (len <= 0) {
            mln_json_destroy(&key);
            mln_json_destroy(&v);
            return -1;
        }

        if (__mln_json_obj_insert(val->data.m_j_obj, &key, &v) < 0) {
            mln_json_destroy(&key);
            mln_json_destroy(&v);
            return -1;
        }

        if (policy != NULL && policy->obj_kv_num && val->data.m_j_obj->nr_nodes > policy->obj_kv_num) {
            policy->error = M_JSON_OBJKV;
            return -1;
        }
    }

    if (jstr[0] == ',') {
        ++jstr;
        if (--len <= 0) {
            return -1;
        }
        goto again;
    } else if (jstr[0] == '}') {
        ++jstr;
        --len;
        mln_json_jumpoff_blank(&jstr, &len);
        return len;
    }

    return -1;
})

MLN_FUNC(static inline, int, mln_json_parse_array, \
         (mln_json_t *val, char *jstr, int len, mln_uauto_t index, mln_json_policy_t *policy, mln_size_t depth), \
         (val, jstr, len, index, policy, depth), \
{
    int left;
    mln_json_t j;
    mln_uauto_t cnt = 0;

    if (__mln_json_array_init(val) < 0) return -1;

    ++jstr;
    if (--len <= 0) {
        return -1;
    }

    if (policy != NULL && policy->depth && depth + 1 > policy->depth) {
        policy->error = M_JSON_DEPTH;
        return -1;
    }

again:
    mln_json_init(&j);
    if (jstr[0] != ']') {
        left = mln_json_parse_json(&j, jstr, len, ++cnt, policy, 0, depth + 1);
        if (left <= 0) {
            mln_json_destroy(&j);
            return -1;
        }
        jstr += (len - left);
        len = left;
        if (len <= 0) {
            mln_json_destroy(&j);
            return -1;
        }
    }

    if (__mln_json_array_append(val, &j) < 0) {
        mln_json_destroy(&j);
        return -1;
    }

    if (policy != NULL && policy->arr_elem_num && mln_array_nelts(val->data.m_j_array) > policy->arr_elem_num) {
        policy->error = M_JSON_ARRELEM;
        return -1;
    }

    /* Skip jumpoff_blank when next char is already a delimiter */
    if (len > 0 && jstr[0] != ',' && jstr[0] != ']') {
        mln_json_jumpoff_blank(&jstr, &len);
    }
    if (len <= 0) {
        return -1;
    }

    if (jstr[0] == ',') {
        ++jstr;
        if (--len <= 0) {
            return -1;
        }
        goto again;
    }
    if (jstr[0] == ']') {
        ++jstr;
        --len;
        mln_json_jumpoff_blank(&jstr, &len);
        return len;
    }

    return -1;
})

MLN_FUNC(static inline, int, mln_json_parse_string, \
         (mln_json_t *j, char *jstr, int len, mln_uauto_t index, mln_json_policy_t *policy, int obj_key), \
         (j, jstr, len, index, policy, obj_key), \
{
    int count, has_escape = 0, plen;
    mln_string_t *str;

    ++jstr;
    if (--len <= 0) {
        return -1;
    }

    /*
     * memchr-accelerated string end detection.
     * For the common case (no escapes) memchr uses SIMD to find '"'
     * in ~1 instruction per 16-32 bytes instead of 1 per byte.
     */
    {
        mln_u8ptr_t base = (mln_u8ptr_t)jstr;
        mln_u8ptr_t scan = base;
        int remain = len;

        for (;;) {
            mln_u8ptr_t q = (mln_u8ptr_t)memchr(scan, '\"', remain);
            if (q == NULL) return -1;

            /* Count preceding backslashes to detect escaped quotes */
            mln_u8ptr_t r = q;
            while (r > base && *(r - 1) == '\\') --r;
            if ((q - r) & 1) {
                /* Odd number of backslashes: this quote is escaped */
                has_escape = 1;
                remain -= (int)(q - scan) + 1;
                scan = q + 1;
                if (remain <= 0) return -1;
                continue;
            }
            /* Even (or zero) backslashes: this is the real closing quote */
            count = (int)(q - base);
            plen = len - count - 1; /* bytes after the closing quote */
            if (q > base && !has_escape) {
                /* Quick check: any backslash at all? If memchr found none during scanning. */
                mln_u8ptr_t bs = (mln_u8ptr_t)memchr(base, '\\', count);
                if (bs != NULL) has_escape = 1;
            }
            break;
        }
    }

    if (!has_escape) {
        /*
         * Fast path: no escape sequences. Single-alloc string via mln_string_const_ndup.
         */
        str = mln_string_const_ndup((char *)jstr, count);
        if (str == NULL) return -1;
    } else {
        str = mln_json_parse_string_alloc((mln_u8ptr_t)jstr, &count);
        if (str == NULL) {
            return -1;
        }
    }

    if (policy != NULL) {
        if (obj_key) {
            if (policy->key_len && count > policy->key_len) {
                policy->error = M_JSON_KEYLEN;
                mln_string_free(str);
                return -1;
            }
        } else {
            if (policy->str_len && count > policy->str_len) {
                policy->error = M_JSON_STRLEN;
                mln_string_free(str);
                return -1;
            }
        }
    }

    mln_json_string_init(j, str);

    return plen;
})

/*
 * Decode JSON escape sequences into a temporary buffer, then use
 * mln_string_const_ndup to produce the final single-allocation string.
 */
MLN_FUNC(static, mln_string_t *, mln_json_parse_string_alloc, \
         (mln_u8ptr_t jstr, int *len), (jstr, len), \
{
    int l = *len, c, count = 0;
    unsigned int hex = 0;
    mln_u8ptr_t p = jstr, q;
    mln_u8ptr_t buf;
    mln_string_t *s;

    buf = (mln_u8ptr_t)malloc(l + 1);
    if (buf == NULL) return NULL;

    q = buf;
    while (l > 0) {
        c = mln_json_get_char(&p, &l, &hex);
        if (c < 0) {
            free(buf);
            return NULL;
        } else if (c == 0) {
            /* Handle surrogate pairs: high surrogate followed by \uXXXX low surrogate */
            if (hex >= 0xD800 && hex <= 0xDBFF) {
                unsigned int low = 0;
                int c2 = mln_json_get_char(&p, &l, &low);
                if (c2 != 0 || low < 0xDC00 || low > 0xDFFF) {
                    free(buf);
                    return NULL;
                }
                hex = 0x10000 + ((hex - 0xD800) << 10) + (low - 0xDC00);
            }
            mln_json_encode_utf8(hex, &q, &count);
        } else {
            *q++ = (mln_u8_t)c;
            ++count;
        }
    }
    buf[count] = 0;

    s = mln_string_const_ndup((char *)buf, count);
    free(buf);
    if (s == NULL) return NULL;

    *len = count;
    return s;
})

MLN_FUNC_VOID(static, void, mln_json_encode_utf8, \
              (unsigned int u, mln_u8ptr_t *b, int *count), \
              (u, b, count), \
{
    mln_u8ptr_t buf = *b;
    if (u <= 0x7f) {
        *buf++ = u & 0xFF;
        ++(*count);
    } else if (u <= 0x7FF) {
        *buf++ = 0xC0 | ((u >> 6) & 0xFF);
        *buf++ = 0x80 | ((u) & 0x3F);
        (*count) += 2;
    } else if (u <= 0xFFFF) {
        *buf++ = 0xE0 | ((u >> 12) & 0xFF);
        *buf++ = 0x80 | ((u >> 6) & 0x3F);
        *buf++ = 0x80 | ((u) & 0x3F);
        (*count) += 3;
    } else {
        *buf++ = 0xF0 | ((u >> 18) & 0xFF);
        *buf++ = 0x80 | ((u >> 12) & 0x3F);
        *buf++ = 0x80 | ((u >> 6) & 0x3F);
        *buf++ = 0x80 | (u & 0x3F);
        (*count) += 4;
    }
    *b = buf;
})

MLN_FUNC(static inline, int, mln_json_char2int, (char c), (c), {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return c;
})

MLN_FUNC(static inline, int, mln_json_get_char, \
         (mln_u8ptr_t *s, int *len, unsigned int *hex), (s, len, hex), \
{
    if (*len <= 0) return 0;

    if ((*s)[0] == (mln_u8_t)'\\' && *len > 1) {
        switch ((*s)[1]) {
            case (mln_u8_t)'\"':
                (*s) += 2;
                (*len) -= 2;
                return '\"';
            case (mln_u8_t)'\\':
                (*s) += 2;
                (*len) -= 2;
                return '\\';
            case (mln_u8_t)'/':
                (*s) += 2;
                (*len) -= 2;
                return '/';
            case (mln_u8_t)'b':
                (*s) += 2;
                (*len) -= 2;
                return '\b';
            case (mln_u8_t)'f':
                (*s) += 2;
                (*len) -= 2;
                return '\f';
            case (mln_u8_t)'n':
                (*s) += 2;
                (*len) -= 2;
                return '\n';
            case (mln_u8_t)'r':
                (*s) += 2;
                (*len) -= 2;
                return '\r';
            case (mln_u8_t)'t':
                (*s) += 2;
                (*len) -= 2;
                return '\t';
            case (mln_u8_t)'u': {
                (*s) += 2;
                (*len) -= 2;
                if (*len < 4) return -1;
                unsigned int h = 0;
                h = mln_json_char2int(*(*s)++);
                h <<= 4;
                h |= mln_json_char2int(*(*s)++);
                h <<= 4;
                h |= mln_json_char2int(*(*s)++);
                h <<= 4;
                h |= mln_json_char2int(*(*s)++);
                (*len) -= 4;
                *hex = h;
                return 0;
            } default:
                return -1;
        }
    }

    switch ((*s)[0]) {
        case (mln_u8_t)'\\':
            return -1;
        default:
            break;
    }
    (*len) -= 1;
    return *((*s)++);
})

MLN_FUNC(static, int, mln_json_parse_digit, \
         (mln_json_t *j, char *jstr, int len, mln_uauto_t index), \
         (j, jstr, len, index), \
{
    int sub_flag = 0, left;

    if (jstr[0] == '-') {
        sub_flag = 1;
        ++jstr;
        if (--len <= 0) {
            return -1;
        }
    }

    /*
     * Integer fast path: if the number is a plain integer (no '.', 'e', 'E'),
     * parse using integer arithmetic (much faster than double mul/add).
     * Only fall back to double parsing when we encounter '.', 'e', or 'E'.
     */
    if (mln_isdigit(jstr[0]) && jstr[0] != '0') {
        mln_s64_t ival = 0;
        char *p = jstr;
        int remaining = len;
        for (; remaining > 0 && mln_isdigit(*p); --remaining, ++p) {
            ival = ival * 10 + (*p - '0');
        }
        /* Check if it's a pure integer (no '.', 'e', 'E' follows) */
        if (remaining <= 0 || (*p != '.' && *p != 'e' && *p != 'E')) {
            mln_json_number_init(j, sub_flag ? (double)(-ival) : (double)ival);
            return remaining;
        }
        /* Fall through to double path */
    }

    {
        double val = 0;
        left = mln_json_digit_process(&val, jstr, len);
        if (left < 0) {
            return -1;
        }
        mln_json_number_init(j, sub_flag ? -val : val);
        return left;
    }
})

MLN_FUNC(static inline, int, mln_json_digit_process, (double *val, char *s, int len), (val, s, len), {
    double f = 0;
    int i, j, dir = 1;

    if (!mln_isdigit(*s)) return -1;

    if (s[0] == '0') {
        ++s;
        if (--len <= 0) return 0;
        if (mln_isdigit(*s)) return -1;
    } else {
        for (; len > 0; --len, ++s) {
            if (!mln_isdigit(*s)) break;
            (*val) *= 10;
            (*val) += (*s - '0');
        }
        if (len <= 0) return 0;
    }

    if (*s == '.') {
        double frac = 0, divisor = 1;
        ++s;
        if (--len <= 0) return -1;
        for (; len > 0; --len, ++s) {
            if (!mln_isdigit(*s)) break;
            frac = frac * 10 + (*s - '0');
            divisor *= 10;
        }
        *val += frac / divisor;
        if (len <= 0) return 0;
    }

    if (*s == 'e' || *s == 'E') {
        ++s;
        if (--len <= 0) return -1;

        if (*s == '+') {
            ++s;
            if (--len <= 0) return 0;
        } else if (*s == '-') {
            dir = 0;
            ++s;
            if (--len <= 0) return 0;
        }

        for (i = 0; len > 0; --len, ++s) {
            if (!mln_isdigit(*s)) break;
            i *= 10;
            i += (*s - '0');
        }
        if (i == 0) return len;

        for (f = 1, j = 0; j < i; ++j) {
            if (dir) f *= 10;
            else f /= 10;
        }
        (*val) *= f;
    }

    return len;
})

MLN_FUNC(static inline, int, mln_json_parse_true, \
         (mln_json_t *j, char *jstr, int len, mln_uauto_t index), \
         (j, jstr, len, index), \
{
    if (len < 4) return -1;
    /* Fast path: standard lowercase "true" (common case) */
    if (memcmp(jstr, "true", 4) != 0 && strncasecmp(jstr, "true", 4) != 0)
        return -1;
    mln_json_true_init(j);
    return len - 4;
})

MLN_FUNC(static inline, int, mln_json_parse_false, \
         (mln_json_t *j, char *jstr, int len, mln_uauto_t index), \
         (j, jstr, len, index), \
{
    if (len < 5) return -1;
    if (memcmp(jstr, "false", 5) != 0 && strncasecmp(jstr, "false", 5) != 0)
        return -1;
    mln_json_false_init(j);
    return len - 5;
})

MLN_FUNC(static inline, int, mln_json_parse_null, \
         (mln_json_t *j, char *jstr, int len, mln_uauto_t index), \
         (j, jstr, len, index), \
{
    if (len < 4) return -1;
    if (memcmp(jstr, "null", 4) != 0 && strncasecmp(jstr, "null", 4) != 0)
        return -1;
    mln_json_null_init(j);
    return len - 4;
})

MLN_FUNC_VOID(static inline, void, mln_json_jumpoff_blank, \
              (char **jstr, int *len), (jstr, len), \
{
    /* JSON whitespace is only SP/TAB/CR/LF, all <= 0x20.
     * Single comparison replaces 4 branches per byte. */
    for (; *len > 0 && (mln_u8_t)(**jstr) <= (mln_u8_t)' '; ++(*jstr), --(*len))
        ;
})


MLN_FUNC(, mln_string_t *, mln_json_encode, (mln_json_t *j, mln_u32_t flags), (j, flags), {
    mln_s8ptr_t buf;
    mln_size_t size = M_JSON_BUFLEN, pos = 0;
    mln_string_t *s;

    buf = (mln_s8ptr_t)malloc(size);
    if (buf == NULL) return NULL;

    if (mln_json_write_content(j, &buf, &size, &pos, flags) < 0) {
        free(buf);
        return NULL;
    }

    /* null-terminate for safety, reuse the buffer directly */
    if (pos >= size) {
        mln_s8ptr_t tmp = (mln_s8ptr_t)realloc(buf, pos + 1);
        if (tmp == NULL) { free(buf); return NULL; }
        buf = tmp;
    }
    buf[pos] = 0;
    s = mln_string_buf_new((mln_u8ptr_t)buf, pos);
    if (s == NULL) { free(buf); return NULL; }

    return s;
})

struct mln_json_tmp_s {
    void *ptr1;
    void *ptr2;
    void *ptr3;
};

MLN_FUNC(static, int, mln_json_write_grow, \
         (mln_s8ptr_t *buf, mln_size_t *size, mln_size_t need), (buf, size, need), \
{
    mln_size_t new_size = *size;
    mln_s8ptr_t tmp;
    while (new_size < need)
        new_size <<= 1;
    tmp = (mln_s8ptr_t)realloc(*buf, new_size);
    if (tmp == NULL) return -1;
    *buf = tmp;
    *size = new_size;
    return 0;
})

/*
 * Fast inline write: avoid function-call overhead for common small writes.
 * Only calls the grow path when buffer is actually full.
 */
#define mln_json_write_byte_inline(buf, size, off, s, n) \
    ((*(off) + (n) <= *(size)) \
     ? (memcpy(*(buf) + *(off), (s), (n)), *(off) += (n), 0) \
     : mln_json_write_byte_slow(buf, size, off, s, n))

MLN_FUNC(static inline, int, mln_json_write_byte_slow, \
         (mln_s8ptr_t *buf, mln_size_t *size, mln_size_t *off, mln_u8ptr_t s, mln_size_t n), (buf, size, off, s, n), \
{
    if (mln_json_write_grow(buf, size, *off + n) < 0) return -1;
    memcpy(*buf + *off, s, n);
    *off += n;
    return 0;
})

/*
 * Fast integer-to-string: avoids snprintf overhead for the common case.
 * Writes digits into caller-provided buffer (at least 21 bytes).
 * Returns number of characters written.
 */
MLN_FUNC(static inline, int, mln_json_i64toa, \
         (mln_s64_t val, char *buf), (val, buf), \
{
    char tmp[21];
    int i = 0, n;
    mln_u64_t uv;

    if (val < 0) {
        *buf++ = '-';
        uv = (mln_u64_t)(-(val + 1)) + 1;
        i = 1;
    } else {
        uv = (mln_u64_t)val;
    }

    n = 0;
    do {
        tmp[n++] = '0' + (char)(uv % 10);
        uv /= 10;
    } while (uv);

    /* reverse */
    for (int k = n - 1; k >= 0; --k)
        buf[k] = tmp[n - 1 - k];

    return i + n;
})

#if defined(MSVC)
static inline void mln_json_write_back(mln_size_t *off, mln_size_t n)
{
    *off = *off < n? 0: *off - n;
}
#else
#define mln_json_write_back(_off, _n) ({\
    mln_size_t o = (_off), n = (_n);\
    (_off) = o < n? 0: (o - n);\
})
#endif

static inline int
mln_json_write_content(mln_json_t *j, mln_s8ptr_t *buf, mln_size_t *size, mln_size_t *off, mln_u32_t flags)
{
    if (j == NULL) return 0;

    mln_size_t save;

    switch (j->type) {
        case M_JSON_OBJECT:
        {
            if (mln_json_write_byte_inline(buf, size, off, (mln_u8ptr_t)"{", 1) < 0) return -1;
            {
                mln_json_kv_t *_kv;
                int _first = 1;
                for (_kv = mln_json_object_data_get(j)->head; _kv != NULL; _kv = _kv->iter_next) {
                    if (!_first) {
                        if (mln_json_write_byte_inline(buf, size, off, (mln_u8ptr_t)",", 1) < 0) return -1;
                    }
                    _first = 0;
                    if (mln_json_write_content(&(_kv->key), buf, size, off, flags) < 0) return -1;
                    if (mln_json_write_byte_inline(buf, size, off, (mln_u8ptr_t)":", 1) < 0) return -1;
                    if (mln_json_write_content(&(_kv->val), buf, size, off, flags) < 0) return -1;
                }
            }
            if (mln_json_write_byte_inline(buf, size, off, (mln_u8ptr_t)"}", 1) < 0) return -1;
            break;
        }
        case M_JSON_ARRAY:
        {
            mln_json_t *el = (mln_json_t *)mln_array_elts(mln_json_array_data_get(j));
            mln_json_t *elend = el + mln_array_nelts(mln_json_array_data_get(j));

            if (mln_json_write_byte_inline(buf, size, off, (mln_u8ptr_t)"[", 1) < 0) return -1;
            save = *off;
            for (; el < elend; ++el) {
                if (mln_json_write_content(el, buf, size, off, flags) < 0) return -1;
                if (mln_json_write_byte_inline(buf, size, off, (mln_u8ptr_t)",", 1) < 0) return -1;
            }
#if defined(MSVC)
            if (save < *off) mln_json_write_back(off, 1);
#else
            if (save < *off) mln_json_write_back(*off, 1);
#endif
            if (mln_json_write_byte_inline(buf, size, off, (mln_u8ptr_t)"]", 1) < 0) return -1;
            break;
        }
        case M_JSON_STRING:
        {
            mln_string_t *s;
            if ((s = j->data.m_j_string) != NULL && s->len > 0) {
                mln_u8ptr_t p = s->data, end = p + s->len;
                int need_escape = 0;

                if (flags & M_JSON_ENCODE_UNICODE) {
                    /* Unicode mode: scan for any char needing escape */
                    for (mln_u8ptr_t t = p; t < end; ++t) {
                        if (*t == '\"' || *t == '\\' || *t >= 0x80) { need_escape = 1; break; }
                    }
                } else {
                    if (memchr(p, '\"', s->len) != NULL || memchr(p, '\\', s->len) != NULL)
                        need_escape = 1;
                }

                if (!need_escape) {
                    /* Fast path: write "string" in one shot */
                    mln_size_t total = s->len + 2;
                    if (*off + total > *size) {
                        if (mln_json_write_grow(buf, size, *off + total) < 0) return -1;
                    }
                    (*buf)[(*off)++] = '\"';
                    memcpy(*buf + *off, s->data, s->len);
                    *off += s->len;
                    (*buf)[(*off)++] = '\"';
                } else {
                    /* Slow path: character-by-character escape */
                    if (mln_json_write_byte_inline(buf, size, off, (mln_u8ptr_t)"\"", 1) < 0) return -1;
                    mln_u8ptr_t run_start = p;
                    while (p < end) {
                        if (*p == '\"' || *p == '\\') {
                            if (p > run_start) {
                                if (mln_json_write_byte_inline(buf, size, off, run_start, p - run_start) < 0) return -1;
                            }
                            if (mln_json_write_byte_inline(buf, size, off, (mln_u8ptr_t)"\\", 1) < 0) return -1;
                            if (mln_json_write_byte_inline(buf, size, off, p, 1) < 0) return -1;
                            run_start = ++p;
                        } else if ((flags & M_JSON_ENCODE_UNICODE) && *p >= 0x80) {
                            /* Decode UTF-8 to codepoint, emit \uXXXX */
                            unsigned int cp = 0;
                            int bytes = 0;
                            if (p > run_start) {
                                if (mln_json_write_byte_inline(buf, size, off, run_start, p - run_start) < 0) return -1;
                            }
                            if ((*p & 0xE0) == 0xC0)      { cp = *p & 0x1F; bytes = 2; }
                            else if ((*p & 0xF0) == 0xE0)  { cp = *p & 0x0F; bytes = 3; }
                            else if ((*p & 0xF8) == 0xF0)  { cp = *p & 0x07; bytes = 4; }
                            else { ++p; run_start = p; continue; } /* invalid, skip */
                            if (p + bytes > end) { ++p; run_start = p; continue; } /* truncated */
                            for (int i = 1; i < bytes; ++i) cp = (cp << 6) | (p[i] & 0x3F);
                            p += bytes;
                            if (cp <= 0xFFFF) {
                                char esc[6];
                                esc[0] = '\\'; esc[1] = 'u';
                                esc[2] = "0123456789abcdef"[(cp >> 12) & 0xf];
                                esc[3] = "0123456789abcdef"[(cp >> 8) & 0xf];
                                esc[4] = "0123456789abcdef"[(cp >> 4) & 0xf];
                                esc[5] = "0123456789abcdef"[cp & 0xf];
                                if (mln_json_write_byte_inline(buf, size, off, (mln_u8ptr_t)esc, 6) < 0) return -1;
                            } else {
                                /* Surrogate pair for codepoints above BMP */
                                unsigned int hi, lo;
                                char esc[12];
                                cp -= 0x10000;
                                hi = 0xD800 + (cp >> 10);
                                lo = 0xDC00 + (cp & 0x3FF);
                                esc[0] = '\\'; esc[1] = 'u';
                                esc[2] = "0123456789abcdef"[(hi >> 12) & 0xf];
                                esc[3] = "0123456789abcdef"[(hi >> 8) & 0xf];
                                esc[4] = "0123456789abcdef"[(hi >> 4) & 0xf];
                                esc[5] = "0123456789abcdef"[hi & 0xf];
                                esc[6] = '\\'; esc[7] = 'u';
                                esc[8] = "0123456789abcdef"[(lo >> 12) & 0xf];
                                esc[9] = "0123456789abcdef"[(lo >> 8) & 0xf];
                                esc[10] = "0123456789abcdef"[(lo >> 4) & 0xf];
                                esc[11] = "0123456789abcdef"[lo & 0xf];
                                if (mln_json_write_byte_inline(buf, size, off, (mln_u8ptr_t)esc, 12) < 0) return -1;
                            }
                            run_start = p;
                        } else {
                            ++p;
                        }
                    }
                    if (p > run_start) {
                        if (mln_json_write_byte_inline(buf, size, off, run_start, p - run_start) < 0) return -1;
                    }
                    if (mln_json_write_byte_inline(buf, size, off, (mln_u8ptr_t)"\"", 1) < 0) return -1;
                }
            } else {
                if (mln_json_write_byte_inline(buf, size, off, (mln_u8ptr_t)"\"\"", 2) < 0) return -1;
            }
            break;
        }
        case M_JSON_NUM:
        {
            char tmp[32];
            int n;
            mln_s64_t i = (mln_s64_t)(j->data.m_j_number);
            if (i == j->data.m_j_number) {
                n = mln_json_i64toa(i, tmp);
            } else {
                n = snprintf(tmp, sizeof(tmp) - 1, "%f", j->data.m_j_number);
            }
            if (mln_json_write_byte_inline(buf, size, off, (mln_u8ptr_t)tmp, n) < 0) return -1;
            break;
        }
        case M_JSON_TRUE:
            if (mln_json_write_byte_inline(buf, size, off, (mln_u8ptr_t)"true", 4) < 0) return -1;
            break;
        case M_JSON_FALSE:
            if (mln_json_write_byte_inline(buf, size, off, (mln_u8ptr_t)"false", 5) < 0) return -1;
            break;
        case M_JSON_NULL:
            if (mln_json_write_byte_inline(buf, size, off, (mln_u8ptr_t)"null", 4) < 0) return -1;
            break;
        default:
            break;
    }

    return 0;
}


MLN_FUNC(, int, mln_json_fetch, \
         (mln_json_t *j, mln_string_t *exp, mln_json_iterator_t iterator, void *data), \
         (j, exp, iterator, data), \
{
    mln_size_t idx;
    mln_string_t seg;
    mln_u8ptr_t p, end, seg_start;

    if (exp->len == 0) {
        if (iterator != NULL) return iterator(j, data);
        return 0;
    }

    p = exp->data;
    end = exp->data + exp->len;
    seg_start = p;

    for (;;) {
        if (p == end || *p == (mln_u8_t)'.') {
            seg.data = seg_start;
            seg.len = p - seg_start;

            if (seg.len > 0) {
                if (mln_json_is_object(j)) {
                    j = mln_json_obj_search(j, &seg);
                    if (j == NULL) return -1;
                } else if (mln_json_is_array(j)) {
                    if (!mln_json_parse_is_index(&seg, &idx)) return -1;
                    j = mln_json_array_search(j, idx);
                    if (j == NULL) return -1;
                } else {
                    return -1;
                }
            }

            if (p == end) break;
            seg_start = p + 1;
        }
        ++p;
    }

    if (iterator != NULL)
        return iterator(j, data);
    return 0;
})

MLN_FUNC(static inline, int, mln_json_parse_is_index, \
         (mln_string_t *s, mln_size_t *idx), (s, idx), \
{
    mln_u8ptr_t p = s->data, pend = s->data + s->len;
    mln_size_t sum = 0;

    if (p == pend) return 0;

    for (; p < pend; ++p) {
        if (*p < (mln_u8_t)'0' || *p > (mln_u8_t)'9')
           return 0;
        sum = sum * 10 + (*p - (mln_u8_t)'0');
    }
    *idx = sum;
    return 1;
})


int mln_json_generate(mln_json_t *j, char *fmt, ...)
{
    int rc = 0;
    va_list arg;

    va_start(arg, fmt);

    if (*fmt == '{') {
        rc = mln_json_obj_generate(j, &fmt, &arg);
    } else if (*fmt == '[') {
        rc = mln_json_array_generate(j, &fmt, &arg);
    } else {
        rc = -1;
    }

    va_end(arg);
    return rc;
}

MLN_FUNC(static inline, int, mln_json_obj_generate, \
         (mln_json_t *j, char **fmt, va_list *arg), (j, fmt, arg), \
{
    int rc = 0;
    mln_json_t k, v, *json;
    char *s, *f = *fmt;
    mln_string_t *str;
    mln_s32_t s32;
    mln_u32_t u32;
    mln_s64_t s64;
    mln_u64_t u64;
    double d;
    struct mln_json_call_attr *ca;

    if (mln_json_is_none(j) && __mln_json_obj_init(j) < 0) return -1;
    if (!mln_json_is_object(j)) return -1;
    ++f;

again:
    mln_json_init(&k);
    mln_json_init(&v);
    switch (*f++) {
        case 's':
        {
            s = va_arg(*arg, char *);
            str = mln_string_const_ndup(s, strlen(s));
            if (str == NULL) goto err;
            mln_json_string_init(&k, str);
            break;
        }
        case 'S':
        {
            str = va_arg(*arg, mln_string_t *);
            mln_json_string_init(&k, mln_string_ref(str));
            break;
        }
        case 'c':
        {
            ca = va_arg(*arg, struct mln_json_call_attr *);
            if (ca->callback(&k, ca->data) < 0) goto err;
            break;
        }
        case '}':
        {
            goto out;
        }
        default:
            goto err;
    }

    if (*f++ != ':') {
        goto err;
    }

    switch (*f++) {
        case 'j':
        {
            json = va_arg(*arg, mln_json_t *);
            v = *json;
            break;
        }
        case 'd':
        {
            s32 = va_arg(*arg, mln_s32_t);
            mln_json_number_init(&v, s32);
            break;
        }
        case 'D':
        {
            s64 = va_arg(*arg, mln_s64_t);
            mln_json_number_init(&v, s64);
            break;
        }
        case 'u':
        {
            u32 = va_arg(*arg, mln_u32_t);
            mln_json_number_init(&v, u32);
            break;
        }
        case 'U':
        {
            u64 = va_arg(*arg, mln_u64_t);
            mln_json_number_init(&v, u64);
            break;
        }
        case 'F':
        {
            d = va_arg(*arg, double);
            mln_json_number_init(&v, d);
            break;
        }
        case 't':
        {
            mln_json_true_init(&v);
            break;
        }
        case 'f':
        {
            mln_json_false_init(&v);
            break;
        }
        case 'n':
        {
            mln_json_null_init(&v);
            break;
        }
        case 's':
        {
            s = va_arg(*arg, char *);
            str = mln_string_const_ndup(s, strlen(s));
            if (str == NULL) {
                goto err;
            }
            mln_json_string_init(&v, str);
            break;
        }
        case 'S':
        {
            str = va_arg(*arg, mln_string_t *);
            mln_json_string_init(&v, mln_string_ref(str));
            break;
        }
        case 'c':
        {
            ca = va_arg(*arg, struct mln_json_call_attr *);
            if (ca->callback(&v, ca->data) < 0) goto err;
            break;
        }
        case '{':
            --f;
            if (mln_json_obj_generate(&v, &f, arg) < 0) goto err;
            break;
        case '[':
            --f;
            if (mln_json_array_generate(&v, &f, arg) < 0) goto err;
            break;
        default:
            goto err;
    }

    if (__mln_json_obj_update(j, &k, &v) < 0) {
        goto err;
    }

    if (*f == '}') {
        ++f;
        goto out;
    }
    if (*f++ != ',') {
        goto error;
    }

    goto again;

err:
    mln_json_destroy(&k);
    mln_json_destroy(&v);
error:
    rc = -1;
    mln_json_reset(j);

out:
    *fmt = f;
    return rc;
})

MLN_FUNC(static inline, int, mln_json_array_generate, \
         (mln_json_t *j, char **fmt, va_list *arg), (j, fmt, arg), \
{
    int rc = 0;
    mln_json_t v, *json;
    char *s, *f = *fmt;
    mln_string_t *str;
    mln_s32_t s32;
    mln_u32_t u32;
    mln_s64_t s64;
    mln_u64_t u64;
    double d;
    struct mln_json_call_attr *ca;

    if (mln_json_is_none(j) && __mln_json_array_init(j) < 0) return -1;
    if (!mln_json_is_array(j)) return -1;
    ++f;

again:
    mln_json_init(&v);
    switch (*f++) {
        case 'j':
        {
            json = va_arg(*arg, mln_json_t *);
            v = *json;
            break;
        }
        case 'd':
        {
            s32 = va_arg(*arg, mln_s32_t);
            mln_json_number_init(&v, s32);
            break;
        }
        case 'D':
        {
            s64 = va_arg(*arg, mln_s64_t);
            mln_json_number_init(&v, s64);
            break;
        }
        case 'u':
        {
            u32 = va_arg(*arg, mln_u32_t);
            mln_json_number_init(&v, u32);
            break;
        }
        case 'U':
        {
            u64 = va_arg(*arg, mln_u64_t);
            mln_json_number_init(&v, u64);
            break;
        }
        case 'F':
        {
            d = va_arg(*arg, double);
            mln_json_number_init(&v, d);
            break;
        }
        case 't':
        {
            mln_json_true_init(&v);
            break;
        }
        case 'f':
        {
            mln_json_false_init(&v);
            break;
        }
        case 'n':
        {
            mln_json_null_init(&v);
            break;
        }
        case 's':
        {
            s = va_arg(*arg, char *);
            str = mln_string_const_ndup(s, strlen(s));
            if (str == NULL) {
                goto err;
            }
            mln_json_string_init(&v, str);
            break;
        }
        case 'S':
        {
            str = va_arg(*arg, mln_string_t *);
            mln_json_string_init(&v, mln_string_ref(str));
            break;
        }
        case 'c':
        {
            ca = va_arg(*arg, struct mln_json_call_attr *);
            if (ca->callback(&v, ca->data) < 0) goto err;
            break;
        }
        case ']':
        {
            goto out;
        }
        case '{':
            --f;
            if (mln_json_obj_generate(&v, &f, arg) < 0) goto err;
            break;
        case '[':
            --f;
            if (mln_json_array_generate(&v, &f, arg) < 0) goto err;
            break;
        default:
            goto err;
    }

    if (__mln_json_array_append(j, &v) < 0) {
        goto err;
    }

    if (*f == ']') {
        ++f;
        goto out;
    }
    if (*f++ != ',') {
        goto error;
    }

    goto again;

err:
    mln_json_destroy(&v);
error:
    rc = -1;
    mln_json_reset(j);

out:
    *fmt = f;
    return rc;
})

MLN_FUNC(, int, mln_json_object_iterate, \
         (mln_json_t *j, mln_json_object_iterator_t it, void *data), (j, it, data), \
{
    mln_json_obj_t *obj;
    mln_json_kv_t *kv;

    if (!mln_json_is_object(j)) return -1;

    obj = mln_json_object_data_get(j);
    for (kv = obj->head; kv != NULL; kv = kv->iter_next) {
        if (it(&(kv->key), &(kv->val), data) != 0) return -1;
    }
    return 0;
})

MLN_FUNC(, mln_size_t, mln_json_obj_element_num, (mln_json_t *j), (j), {
    if (!mln_json_is_object(j)) return 0;
    return (mln_size_t)mln_json_object_data_get(j)->nr_nodes;
})

#if defined(MSVC)
void mln_json_reset(mln_json_t *j)
{
    mln_json_destroy(j);
    mln_json_none_type_set(j);
}

int mln_json_array_iterate(mln_json_t *j, mln_json_array_iterator_t it, void *data)
{
    mln_json_t *end;
    int rc = -1;
    mln_array_t *a = mln_json_array_data_get(j);
    if (mln_json_is_array(j)) {
        j = (mln_json_t *)mln_array_elts(a);
        end = j + mln_array_nelts(a);
        for (; j < end; ++j) {
            if ((rc = it(j, data))) break;
        }
    }
    return rc;
}
#endif

