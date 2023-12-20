
/*
 * Copyright (C) Niklaus F.Schen.
 */

#include <ctype.h>
#include <stdio.h>
#include <stdarg.h>
#include "mln_json.h"

static int mln_json_dump_obj_iterate_handler(mln_rbtree_node_t *node, void *data);
static inline int
mln_json_parse_json(mln_json_t *j, char *jstr, int len, mln_uauto_t index);
static inline int
mln_json_parse_obj(mln_json_t *val, char *jstr, int len, mln_uauto_t index);
static inline int
mln_json_parse_array(mln_json_t *val, char *jstr, int len, mln_uauto_t index);
static inline int
mln_json_parse_string(mln_json_t *j, char *jstr, int len, mln_uauto_t index);
static mln_u8ptr_t
mln_json_parse_string_fetch(mln_u8ptr_t jstr, int *len);
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
static inline mln_size_t mln_json_get_length(mln_json_t *j);
static int
mln_json_get_length_obj_iterate_handler(mln_rbtree_node_t *node, void *data);
static inline mln_size_t
mln_json_write_content(mln_json_t *j, mln_s8ptr_t buf);
static int
mln_json_write_content_obj_iterate_handler(mln_rbtree_node_t *node, void *data);
static inline int mln_json_parse_is_index(mln_string_t *s, mln_size_t *idx);
static inline int mln_json_obj_generate(mln_json_t *j, char **fmt, va_list *arg);
static inline int mln_json_array_generate(mln_json_t *j, char **fmt, va_list *arg);
static inline int __mln_json_obj_update(mln_json_t *j, mln_json_t *key, mln_json_t *val);
static inline int __mln_json_array_append(mln_json_t *j, mln_json_t *value);
static inline int mln_json_object_iterator(mln_rbtree_node_t *node, void *data);


static inline int mln_json_kv_cmp(const mln_json_kv_t *kv1, const mln_json_kv_t *kv2)
{
    ASSERT(mln_json_is_string(&(kv1->key)) && mln_json_is_string(&(kv2->key)));
    return mln_string_strcmp(mln_json_string_data_get(&(kv1->key)), mln_json_string_data_get(&(kv2->key)));
}

static inline void mln_json_kv_free(mln_json_kv_t *kv)
{
    if (kv == NULL) return;

    mln_json_destroy(&(kv->key));
    mln_json_destroy(&(kv->val));
    free(kv);
}


int mln_json_obj_init(mln_json_t *j)
{
    j->type = M_JSON_OBJECT;
    if ((j->data.m_j_obj = mln_rbtree_new(NULL)) == NULL) return -1;
    return 0;
}

int mln_json_obj_update(mln_json_t *j, mln_json_t *key, mln_json_t *val)
{
    return __mln_json_obj_update(j, key, val);
}

static inline int __mln_json_obj_update(mln_json_t *j, mln_json_t *key, mln_json_t *val)
{
    mln_rbtree_node_t *rn;
    mln_json_kv_t kv, *pkv;

    if (!mln_json_is_string(key) || !mln_json_is_object(j)) return -1;

    kv.key = *key;
    rn = mln_rbtree_inline_search(mln_json_object_data_get(j), &kv, mln_json_kv_cmp);
    if (mln_rbtree_null(rn, mln_json_object_data_get(j))) {
        pkv = (mln_json_kv_t *)malloc(sizeof(mln_json_kv_t));
        if (pkv == NULL) return -1;

        pkv->key = *key;
        pkv->val = *val;
        rn = mln_rbtree_node_init(&pkv->node, pkv);
        mln_rbtree_inline_insert(mln_json_object_data_get(j), rn, mln_json_kv_cmp);
    } else {
        pkv = (mln_json_kv_t *)mln_rbtree_node_data_get(rn);
        mln_json_destroy(&(pkv->key));
        mln_json_destroy(&(pkv->val));
        pkv->key = *key;
        pkv->val = *val;
    }

    return 0;
}

mln_json_t *mln_json_obj_search(mln_json_t *j, mln_string_t *key)
{
    if (!mln_json_is_object(j)) return NULL;

    mln_rbtree_node_t *rn;
    mln_json_kv_t kv;

    mln_json_string_init(&(kv.key), key);
    rn = mln_rbtree_inline_search(mln_json_object_data_get(j), &kv, mln_json_kv_cmp);
    if (mln_rbtree_null(rn, mln_json_object_data_get(j))) {
        return NULL;
    }
    return &(((mln_json_kv_t *)mln_rbtree_node_data_get(rn))->val);
}

void mln_json_obj_remove(mln_json_t *j, mln_string_t *key)
{
    if (!mln_json_is_object(j)) return;

    mln_json_kv_t kv;
    mln_rbtree_node_t *rn;

    mln_json_string_init(&(kv.key), key);
    rn = mln_rbtree_inline_search(mln_json_object_data_get(j), &kv, mln_json_kv_cmp);
    if (mln_rbtree_null(rn, mln_json_object_data_get(j))) {
        return;
    }
    mln_rbtree_delete(mln_json_object_data_get(j), rn);
    mln_rbtree_inline_node_free(mln_json_object_data_get(j), rn, mln_json_kv_free);
}


int mln_json_array_init(mln_json_t *j)
{
    struct mln_array_attr attr;

    attr.pool = NULL;
    attr.pool_alloc = NULL;
    attr.pool_free = NULL;
    attr.free = (array_free)mln_json_destroy;
    attr.size = sizeof(mln_json_t);
    attr.nalloc = M_JSON_LEN;
    if ((j->data.m_j_array = mln_array_new(&attr)) == NULL)
        return -1;
    mln_json_array_type_set(j);
    return 0;
}

mln_json_t *mln_json_array_search(mln_json_t *j, mln_uauto_t index)
{
    if (!mln_json_is_array(j)) return NULL;

    if (index >= mln_array_nelts(mln_json_array_data_get(j))) return NULL;

    return &(((mln_json_t *)mln_array_elts(mln_json_array_data_get(j)))[index]);
}

mln_uauto_t mln_json_array_length(mln_json_t *j)
{
    if (!mln_json_is_array(j)) return 0;
    return mln_array_nelts(mln_json_array_data_get(j));
}

int mln_json_array_append(mln_json_t *j, mln_json_t *value)
{
    return __mln_json_array_append(j, value);
}

static inline int __mln_json_array_append(mln_json_t *j, mln_json_t *value)
{
    mln_json_t *elem;

    if (!mln_json_is_array(j)) return -1;

    if ((elem = (mln_json_t *)mln_array_push(mln_json_array_data_get(j))) == NULL)
        return -1;
    *elem = *value;
    return 0;
}

int mln_json_array_update(mln_json_t *j, mln_json_t *value, mln_uauto_t index)
{
    if (!mln_json_is_array(j)) return -1;

    mln_array_t *arr = mln_json_array_data_get(j);
    if (index >= mln_array_nelts(arr)) return -1;

    mln_json_destroy(&(((mln_json_t *)mln_array_elts(arr))[index]));
    ((mln_json_t *)mln_array_elts(arr))[index] = *value;
    return 0;
}

void mln_json_array_remove(mln_json_t *j, mln_uauto_t index)
{
    if (j == NULL || !mln_json_is_array(j)) return;

    ASSERT(index == mln_array_nelts(mln_json_array_data_get(j)) - 1);

    mln_array_pop(mln_json_array_data_get(j));
}

void mln_json_destroy(mln_json_t *j)
{
    if (j == NULL) return;

    switch (j->type) {
        case M_JSON_OBJECT:
            mln_rbtree_inline_free(mln_json_object_data_get(j), mln_json_kv_free);
            break;
        case M_JSON_ARRAY:
            mln_array_free(mln_json_array_data_get(j));
            break;
        case M_JSON_STRING:
            if (j->data.m_j_string != NULL) {
                mln_string_free(j->data.m_j_string);
            }
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
void mln_json_dump(mln_json_t *j, int n_space, char *prefix)
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
            mln_rbtree_iterate(mln_json_object_data_get(j), mln_json_dump_obj_iterate_handler, &space);
            break;
        case M_JSON_ARRAY:
        {
            printf("type:array\n");
            mln_json_t *elem = mln_array_elts(mln_json_array_data_get(j));
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
}

static int mln_json_dump_obj_iterate_handler(mln_rbtree_node_t *node, void *data)
{
    int *space = (int *)data;
    mln_json_kv_t *kv = (mln_json_kv_t *)mln_rbtree_node_data_get(node);

    mln_json_dump(&(kv->key), *space, "Object key:");
    mln_json_dump(&(kv->val), *space, "Object value:");

    return 0;
}


/*
 * decode
 */
int mln_json_decode(mln_string_t *jstr, mln_json_t *out)
{
    if (jstr == NULL || out == NULL) {
        return -1;
    }

    mln_json_init(out);

    if (mln_json_parse_json(out, (char *)(jstr->data), jstr->len, 0) != 0) {
        mln_json_destroy(out);
        return -1;
    }
    if (!mln_json_is_object(out) && !mln_json_is_array(out)) {
        mln_json_destroy(out);
        return -1;
    }

    return 0;
}

static inline int
mln_json_parse_json(mln_json_t *j, char *jstr, int len, mln_uauto_t index)
{
    if (jstr == NULL) {
        return -1;
    }

    int left;

    mln_json_jumpoff_blank(&jstr, &len);
    if (len <= 0) {
        return len;
    }

    switch (jstr[0]) {
        case '{':
            return mln_json_parse_obj(j, jstr, len, index);
        case '[':
            return mln_json_parse_array(j, jstr, len, index);
        case '\"':
            return mln_json_parse_string(j, jstr, len, index);
        default:
            if (isdigit(jstr[0]) || jstr[0] == '-') {
                return mln_json_parse_digit(j, jstr, len, index);
            }
            left = mln_json_parse_true(j, jstr, len, index);
            if (left >= 0) return left;
            left = mln_json_parse_false(j, jstr, len, index);
            if (left >= 0) return left;
            break;
    }
    return mln_json_parse_null(j, jstr, len, index);
}

static inline int
mln_json_parse_obj(mln_json_t *val, char *jstr, int len, mln_uauto_t index)
{
    int left;
    mln_json_t key, v;

    /*jump off '{'*/
    ++jstr;
    if (--len <= 0) {
        return -1;
    }

    if (mln_json_obj_init(val) < 0) return -1;

again:
    mln_json_init(&key);
    mln_json_init(&v);
    mln_json_jumpoff_blank(&jstr, &len);
    if (jstr[0] != '}') {
        left = mln_json_parse_json(&key, jstr, len, 0);
        if (left <= 0) {
            mln_json_destroy(&key);
            mln_json_destroy(&v);
            return -1;
        }
        if (!mln_json_is_string(&key)) {
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

        mln_json_jumpoff_blank(&jstr, &len);
        if (len <= 0) {
            mln_json_destroy(&key);
            mln_json_destroy(&v);
            return -1;
        }

        /*jump off ':'*/
        if (jstr[0] != ':') {
            mln_json_destroy(&key);
            mln_json_destroy(&v);
            return -1;
        }
        ++jstr;
        if (--len <= 0) {
            mln_json_destroy(&key);
            mln_json_destroy(&v);
            return -1;
        }

        left = mln_json_parse_json(&v, jstr, len, 0);
        if (left <= 0) {
            mln_json_destroy(&key);
            mln_json_destroy(&v);
            return -1;
        }
        jstr += (len - left);
        len = left;

        mln_json_jumpoff_blank(&jstr, &len);
        if (len <= 0) {
            mln_json_destroy(&key);
            mln_json_destroy(&v);
            return -1;
        }

        if (__mln_json_obj_update(val, &key, &v) < 0) {
            mln_json_destroy(&key);
            mln_json_destroy(&v);
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
}

static inline int
mln_json_parse_array(mln_json_t *val, char *jstr, int len, mln_uauto_t index)
{
    int left;
    mln_json_t j;
    mln_uauto_t cnt = 0;

    if (mln_json_array_init(val) < 0) return -1;

    ++jstr;
    if (--len <= 0) {
        return -1;
    }

again:
    mln_json_init(&j);
    if (jstr[0] != ']') {
        left = mln_json_parse_json(&j, jstr, len, ++cnt);
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

    mln_json_jumpoff_blank(&jstr, &len);
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
}

static inline int
mln_json_parse_string(mln_json_t *j, char *jstr, int len, mln_uauto_t index)
{
    mln_u8_t *p, flag = 0;
    int plen, count = 0;
    mln_string_t *str;
    mln_u8ptr_t buf;

    ++jstr;
    if (--len <= 0) {
        return -1;
    }

    for (p = (mln_u8ptr_t)jstr, plen = len; plen > 0; ++p, ++count, --plen) {
        if (!flag && *p == (mln_u8_t)'\\') {
            flag = 1;
        } else {
            if (*p == (mln_u8_t)'\"' && (p == (mln_u8ptr_t)jstr || !flag))
                break;
            if (flag) flag = 0;
        }
    }
    if (plen <= 0) {
        return -1;
    }

    buf = mln_json_parse_string_fetch((mln_u8ptr_t)jstr, &count);
    if (buf == NULL) {
        return -1;
    }

    str = mln_string_const_ndup((char *)buf, count);
    free(buf);
    if (str == NULL) {
        return -1;
    }

    mln_json_string_init(j, str);

    return --plen; /* jump off " */
}

static mln_u8ptr_t mln_json_parse_string_fetch(mln_u8ptr_t jstr, int *len)
{
    int l = *len, c, count = 0;
    unsigned int hex = 0;
    mln_u8ptr_t p = jstr, buf, q;
    if ((buf = (mln_u8ptr_t)malloc(l)) == NULL) {
        return NULL;
    }
    q = buf;
    while (l > 0) {
        c = mln_json_get_char(&p, &l, &hex);
        if (c < 0) {
            free(buf);
            return NULL;
        } else if (c == 0) {
            mln_json_encode_utf8(hex, &q, &count);
        } else {
            *q++ = (mln_u8_t)c;
            ++count;
        }
    }
    *len = count;
    return buf;
}

static void mln_json_encode_utf8(unsigned int u, mln_u8ptr_t *b, int *count)
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
}

static inline int mln_json_char2int(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return c;
}

static inline int mln_json_get_char(mln_u8ptr_t *s, int *len, unsigned int *hex)
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
}

static int
mln_json_parse_digit(mln_json_t *j, char *jstr, int len, mln_uauto_t index)
{
    int sub_flag = 0, left;
    double val = 0;

    if (jstr[0] == '-') {
        sub_flag = 1;
        ++jstr;
        if (--len <= 0) {
            return -1;
        }
    }

    left = mln_json_digit_process(&val, jstr, len);
    if (left < 0) {
        return -1;
    }

    mln_json_number_init(j, sub_flag? -val: val);

    return left;
}

static inline int
mln_json_digit_process(double *val, char *s, int len)
{
    double f = 0;
    int i, j, dir = 1;

    if (!isdigit(*s)) return -1;

    if (s[0] == '0') {
        ++s;
        if (--len <= 0) return 0;
        if (isdigit(*s)) return -1;
    } else {
        for (; len > 0; --len, ++s) {
            if (!isdigit(*s)) break;
            (*val) *= 10;
            (*val) += (*s - '0');
        }
        if (len <= 0) return 0;
    }

    if (*s == '.') {
        ++s;
        if (--len <= 0) return -1;
        for (i = 1; len > 0; --len, ++s, ++i) {
            if (!isdigit(*s)) break;
            f = *s - '0';
            for (j = 0; j < i; ++j) {
                f /= 10;
            }
            *val += f;
        }
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
            if (!isdigit(*s)) break;
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
}

static inline int
mln_json_parse_true(mln_json_t *j, char *jstr, int len, mln_uauto_t index)
{
    if (len < 4) {
        return -1;
    }

    if (strncasecmp(jstr, "true", 4)) {
        return -1;
    }
    mln_json_true_init(j);

    return len - 4;
}

static inline int
mln_json_parse_false(mln_json_t *j, char *jstr, int len, mln_uauto_t index)
{
    if (len < 5) {
        return -1;
    }

    if (strncasecmp(jstr, "false", 5)) {
        return -1;
    }
    mln_json_false_init(j);

    return len - 5;
}

static inline int
mln_json_parse_null(mln_json_t *j, char *jstr, int len, mln_uauto_t index)
{
    if (len < 4) {
        return -1;
    }

    if (strncasecmp(jstr, "null", 4)) {
        return -1;
    }
    mln_json_null_init(j);

    return len - 4;
}

static inline void mln_json_jumpoff_blank(char **jstr, int *len)
{
    for (; \
         *len > 0 && (*jstr[0] == ' ' || *jstr[0] == '\t' || *jstr[0] == '\r' || *jstr[0] == '\n'); \
         ++(*jstr), --(*len))
        ;
}


mln_string_t *mln_json_encode(mln_json_t *j)
{
    mln_u32_t size = mln_json_get_length(j), n;
    mln_s8ptr_t buf;
    mln_string_t *s;

    buf = (mln_s8ptr_t)malloc(size);
    if (buf == NULL) return NULL;

    n = mln_json_write_content(j, buf);
    buf[n] = 0;

    s = mln_string_const_ndup(buf, n);
    free(buf);
    if (s == NULL) return NULL;

    return s;
}

static inline mln_size_t mln_json_get_length(mln_json_t *j)
{
    if (j == NULL) return 0;

    char num_str[512] = {0};
    mln_size_t length = 1;
    mln_u8ptr_t p, end;

    switch (j->type) {
        case M_JSON_OBJECT:
            length += 2;
            mln_rbtree_iterate(mln_json_object_data_get(j), mln_json_get_length_obj_iterate_handler, &length);
            break;
        case M_JSON_ARRAY:
        {
            mln_json_t *el = (mln_json_t *)mln_array_elts(mln_json_array_data_get(j));
            mln_json_t *elend = el + mln_array_nelts(mln_json_array_data_get(j));
            length += 2;
            for (; el < elend; ++el) {
                length += (mln_json_get_length(el) + 1);
            }
            break;
        }
        case M_JSON_STRING:
            length += 2;
            if (j->data.m_j_string != NULL) {
                p = j->data.m_j_string->data;
                end = p + j->data.m_j_string->len;
                for (; p < end; ++p) {
                    if (*p == '\"' || *p == '\\')
                        ++length;
                }
                length += j->data.m_j_string->len;
            }
            break;
        case M_JSON_NUM:
            length += snprintf(num_str, sizeof(num_str), "%f", j->data.m_j_number);
            break;
        case M_JSON_TRUE:
            length += 4;
            break;
        case M_JSON_FALSE:
            length += 5;
            break;
        case M_JSON_NULL:
            length += 4;
            break;
        default:
            break;
    }

    return length;
}

static int
mln_json_get_length_obj_iterate_handler(mln_rbtree_node_t *node, void *data)
{
    mln_json_kv_t *kv = (mln_json_kv_t *)mln_rbtree_node_data_get(node);
    mln_size_t *length = (mln_size_t *)data;

    (*length) += (mln_json_get_length(&(kv->key)) + mln_json_get_length(&(kv->val)) + 2);
    return 0;
}

struct mln_json_tmp_s {
    void *ptr1;
    void *ptr2;
};

static inline mln_size_t
mln_json_write_content(mln_json_t *j, mln_s8ptr_t buf)
{
    if (j == NULL) return 0;

    int n;
    mln_size_t length = 0, save;
    mln_string_t *s;
    struct mln_json_tmp_s tmp;
    mln_u8ptr_t p, end;

    switch (j->type) {
        case M_JSON_OBJECT:
            *buf++ = '{';
            ++length;
            tmp.ptr1 = &length;
            tmp.ptr2 = &buf;
            save = length;
            mln_rbtree_iterate(mln_json_object_data_get(j), mln_json_write_content_obj_iterate_handler, &tmp);
            if (save < length) {
                --buf;
                --length;
                *buf = 0;
            }
            *buf++ = '}';
            ++length;
            break;
        case M_JSON_ARRAY:
        {
            mln_json_t *el = (mln_json_t *)mln_array_elts(mln_json_array_data_get(j));
            mln_json_t *elend = el + mln_array_nelts(mln_json_array_data_get(j));

            *buf++ = '[';
            ++length;
            save = length;
            for (; el < elend; ++el) {
                n = mln_json_write_content(el, buf);
                buf += n;
                length += n;

                *buf++ = ',';
                length += 1;
            }
            if (save < length) {
                --buf;
                --length;
                *buf = 0;
            }
            *buf++ = ']';
            ++length;
            break;
        }
        case M_JSON_STRING:
            *buf++ = '\"';
            ++length;
            if ((s = j->data.m_j_string) != NULL) {
                p = j->data.m_j_string->data;
                end = p + j->data.m_j_string->len;
                for (; p < end; ) {
                    if (*p == '\"' || *p == '\\') {
                        *buf++ = '\\';
                        ++length;
                    }
                    *buf++ = *p++;
                    ++length;
                }
            }
            *buf++ = '\"';
            ++length;
            break;
        case M_JSON_NUM:
        {
            mln_s64_t i = (mln_s64_t)(j->data.m_j_number);
            if (i == j->data.m_j_number)
#if defined(WIN32) && defined(__pentiumpro__)
                n = snprintf(buf, 512, "%I64d", i);
#elif defined(WIN32) || defined(i386) || defined(__arm__) || defined(__wasm__)
                n = snprintf(buf, 512, "%lld", i);
#else
                n = snprintf(buf, 512, "%ld", i);
#endif
            else
                n = snprintf(buf, 512, "%f", j->data.m_j_number);
            buf += n;
            length += n;
            break;
        }
        case M_JSON_TRUE:
            memcpy(buf, "true", 4);
            buf += 4;
            length += 4;
            break;
        case M_JSON_FALSE:
            memcpy(buf, "false", 5);
            buf += 5;
            length += 5;
            break;
        case M_JSON_NULL:
            memcpy(buf, "null", 4);
            buf += 4;
            length += 4;
            break;
        default:
            break;
    }

    return length;
}

static int
mln_json_write_content_obj_iterate_handler(mln_rbtree_node_t *node, void *data)
{
    mln_json_kv_t *kv = (mln_json_kv_t *)mln_rbtree_node_data_get(node);
    struct mln_json_tmp_s *tmp = (struct mln_json_tmp_s *)data;
    mln_size_t *length = (mln_size_t *)(tmp->ptr1);
    mln_s8ptr_t *buf = (mln_s8ptr_t *)(tmp->ptr2);
    mln_size_t n;

    if (kv == NULL) return 0;

    n = mln_json_write_content(&(kv->key), *buf);
    (*buf) += n;
    (*length) += n;
    *(*buf)++ = ':';
    ++(*length);
    n = mln_json_write_content(&(kv->val), *buf);
    (*buf) += n;
    (*length) += n;

    *(*buf)++ = ',';
    (*length) += 1;

    return 0;
}


int mln_json_parse(mln_json_t *j, mln_string_t *exp, mln_json_iterator_t iterator, void *data)
{
    mln_size_t idx = 0;
    mln_string_t *p, *arr = mln_string_slice(exp, ".");
    if (arr == NULL) return -1;

    for (p = arr; p->len != 0; ++p) {
        if (mln_json_is_object(j)) {
            j = mln_json_obj_search(j, p);
            if (j == NULL) goto err;
        } else if (mln_json_is_array(j)) {
            if (!mln_json_parse_is_index(p, &idx)) goto err;
            j = mln_json_array_search(j, idx);
            if (j == NULL) goto err;
        } else {
err:
            mln_string_slice_free(arr);
            return -1;
        }
    }
    mln_string_slice_free(arr);
    if (iterator != NULL)
        return iterator(j, data);
    return 0;
}

static inline int mln_json_parse_is_index(mln_string_t *s, mln_size_t *idx)
{
    mln_u8ptr_t p = s->data, pend = s->data + s->len - 1;
    mln_size_t sum = 0;

    for (; pend >= p; --pend) {
        if (*pend < (mln_u8_t)'0' || *pend > (mln_u8_t)'9')
           return 0;
        sum = sum * 10 + (*pend - (mln_u8_t)'0');
    }
    *idx = sum;
    return 1;
}


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

static inline int mln_json_obj_generate(mln_json_t *j, char **fmt, va_list *arg)
{
    int rc = 0;
    mln_json_t k, v, *json;
    char *s, *f = *fmt;
    mln_string_t *str, tmp;
    mln_s32_t s32;
    mln_u32_t u32;
    mln_s64_t s64;
    mln_u64_t u64;
    double d;
    struct mln_json_call_attr *ca;

    if (mln_json_is_none(j) && mln_json_obj_init(j) < 0) return -1;
    if (!mln_json_is_object(j)) return -1;
    ++f;

again:
    mln_json_init(&k);
    mln_json_init(&v);
    switch (*f++) {
        case 's':
        {
            s = va_arg(*arg, char *);
            mln_string_set(&tmp, s);
            str = mln_string_dup(&tmp);
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
            mln_string_set(&tmp, s);
            str = mln_string_dup(&tmp);
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
}

static inline int mln_json_array_generate(mln_json_t *j, char **fmt, va_list *arg)
{
    int rc = 0;
    mln_json_t v, *json;
    char *s, *f = *fmt;
    mln_string_t *str, tmp;
    mln_s32_t s32;
    mln_u32_t u32;
    mln_s64_t s64;
    mln_u64_t u64;
    double d;
    struct mln_json_call_attr *ca;

    if (mln_json_is_none(j) && mln_json_array_init(j) < 0) return -1;
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
            mln_string_set(&tmp, s);
            str = mln_string_dup(&tmp);
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
}

int mln_json_object_iterate(mln_json_t *j, mln_json_object_iterator_t it, void *data)
{
    if (!mln_json_is_object(j)) return -1;

    struct mln_json_tmp_s tmp;
    tmp.ptr1 = it;
    tmp.ptr2 = data;
    return mln_rbtree_iterate(mln_json_object_data_get(j), mln_json_object_iterator, &tmp);
}

static inline int mln_json_object_iterator(mln_rbtree_node_t *node, void *data)
{
    struct mln_json_tmp_s *tmp = (struct mln_json_tmp_s *)data;
    mln_json_kv_t *kv = (mln_json_kv_t *)mln_rbtree_node_data_get(node);
    mln_json_object_iterator_t it = (mln_json_object_iterator_t)(tmp->ptr1);
    return it(&(kv->key), &(kv->val), tmp->ptr2);
}

