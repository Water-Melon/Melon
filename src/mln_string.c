
/*
 * Copyright (C) Niklaus F.Schen.
 */

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include "mln_string.h"


static inline int *compute_prefix_function(const char *pattern, int m);
static inline char *
kmp_string_match(char *text, const char *pattern, int text_len, int pattern_len) __NONNULL2(1,2);
static mln_string_t *mln_string_slice_recursive(char *s, mln_u64_t len, mln_u8ptr_t ascii, int cnt, mln_string_t *save) __NONNULL3(1,3,5);
static inline mln_string_t *mln_string_assign(char *s, mln_u32_t len);

static inline mln_string_t *mln_string_assign(char *s, mln_u32_t len)
{
    mln_string_t *str;
    str = (mln_string_t *)malloc(sizeof(mln_string_t));
    if (str == NULL) return NULL;
    str->data = (mln_u8ptr_t)s;
    str->len = len;
    str->pool = 0;
    str->data_ref = 1;
    str->ref = 1;
    return str;
}

mln_string_t *mln_string_buf_new(mln_u8ptr_t buf, mln_u64_t len)
{
    mln_string_t *str = (mln_string_t *)malloc(sizeof(mln_string_t));
    if (str == NULL) return NULL;

    str->data = buf;
    str->len = len;
    str->data_ref = 0;
    str->pool = 0;
    str->ref = 1;
    return str;
}

mln_string_t *mln_string_buf_pool_new(mln_alloc_t *pool, mln_u8ptr_t buf, mln_u64_t len)
{
    mln_string_t *str = (mln_string_t *)mln_alloc_m(pool, sizeof(mln_string_t));
    if (str == NULL) return NULL;

    str->data = buf;
    str->len = len;
    str->data_ref = 0;
    str->pool = 1;
    str->ref = 1;
    return str;
}

mln_string_t *mln_string_pool_new(mln_alloc_t *pool, const char *s)
{
    mln_string_t *str = (mln_string_t *)mln_alloc_m(pool, sizeof(mln_string_t));
    if (str == NULL) return NULL;
    if (s == NULL) {
        str->data = NULL;
        str->len = 0;
        str->data_ref = 0;
        str->pool = 1;
        str->ref = 1;
        return str;
    }
    mln_s32_t len = strlen(s);
    if ((str->data = (mln_u8ptr_t)mln_alloc_m(pool, len + 1)) == NULL) {
        mln_alloc_free(str);
        return NULL;
    }
    memcpy(str->data, s, len);
    str->data[len] = 0;
    str->len = len;
    str->data_ref = 0;
    str->pool = 1;
    str->ref = 1;
    return str;
}

mln_string_t *mln_string_new(const char *s)
{
    mln_string_t *str = (mln_string_t *)malloc(sizeof(mln_string_t));
    if (str == NULL) return NULL;
    if (s == NULL) {
        str->data = NULL;
        str->len = 0;
        str->data_ref = 0;
        str->pool = 0;
        str->ref = 1;
        return str;
    }
    mln_s32_t len = strlen(s);
    if ((str->data = (mln_u8ptr_t)malloc(len + 1)) == NULL) {
        free(str);
        return NULL;
    }
    memcpy(str->data, s, len);
    str->data[len] = 0;
    str->len = len;
    str->data_ref = 0;
    str->pool = 0;
    str->ref = 1;
    return str;
}

mln_string_t *mln_string_dup(mln_string_t *str)
{
    mln_string_t *s = (mln_string_t *)malloc(sizeof(mln_string_t));
    if (s == NULL) return NULL;
    if ((s->data = (mln_u8ptr_t)malloc(str->len + 1)) == NULL) {
        free(s);
        return NULL;
    }
    if (str->data != NULL)
        memcpy(s->data, str->data, str->len);
    s->data[str->len] = 0;
    s->len = str->len;
    s->data_ref = 0;
    s->pool = 0;
    s->ref = 1;
    return s;
}

mln_string_t *mln_string_pool_dup(mln_alloc_t *pool, mln_string_t *str)
{
    mln_string_t *s = (mln_string_t *)mln_alloc_m(pool, sizeof(mln_string_t));
    if (s == NULL) return NULL;
    if ((s->data = (mln_u8ptr_t)mln_alloc_m(pool, str->len + 1)) == NULL) {
        mln_alloc_free(s);
        return NULL;
    }
    if (str->data != NULL)
        memcpy(s->data, str->data, str->len);
    s->data[str->len] = 0;
    s->len = str->len;
    s->data_ref = 0;
    s->pool = 1;
    s->ref = 1;
    return s;
}

mln_string_t *mln_string_alloc(mln_s32_t size)
{
    if (size < 0) return NULL;
    mln_string_t *s = (mln_string_t *)malloc(sizeof(mln_string_t));
    if (s == NULL) return NULL;
    if ((s->data = (mln_u8ptr_t)malloc(size + 1)) == NULL) {
        free(s);
        return NULL;
    }
    s->len = size;
    s->data_ref = 0;
    s->pool = 0;
    s->ref = 1;
    return s;
}

mln_string_t *mln_string_pool_alloc(mln_alloc_t *pool, mln_s32_t size)
{
    if (size < 0) return NULL;
    mln_string_t *s = (mln_string_t *)mln_alloc_m(pool, sizeof(mln_string_t));
    if (s == NULL) return NULL;
    if ((s->data = (mln_u8ptr_t)mln_alloc_m(pool, size + 1)) == NULL) {
        mln_alloc_free(s);
        return NULL;
    }
    s->len = size;
    s->data_ref = 0;
    s->pool = 1;
    s->ref = 1;
    return s;
}

mln_string_t *mln_string_const_ndup(char *str, mln_s32_t size)
{
    if (size < 0) return NULL;
    mln_string_t *s = (mln_string_t *)malloc(sizeof(mln_string_t));
    if (s == NULL) return NULL;
    if ((s->data = (mln_u8ptr_t)malloc(size + 1)) == NULL) {
        free(s);
        return NULL;
    }
    memcpy(s->data, str, size);
    s->data[size] = 0;
    s->len = size;
    s->data_ref = 0;
    s->pool = 0;
    s->ref = 1;
    return s;
}

mln_string_t *mln_string_ref_dup(mln_string_t *str)
{
    mln_string_t *s = (mln_string_t *)malloc(sizeof(mln_string_t));
    if (s == NULL) return NULL;
    s->data = str->data;
    s->len = str->len;
    s->data_ref = 1;
    s->pool = 0;
    s->ref = 1;
    return s;
}

mln_string_t *mln_string_const_ref_dup(char *s)
{
    mln_string_t *str = (mln_string_t *)malloc(sizeof(mln_string_t));
    if (s == NULL) {
        free(str);
        return NULL;
    }
    str->data = (mln_u8ptr_t)s;
    str->len = strlen(s);
    str->data_ref = 1;
    str->pool = 0;
    str->ref = 1;
    return str;
}

int mln_string_strseqcmp(mln_string_t *s1, mln_string_t *s2)
{
    int ret;
    if (s1->len > s2->len) {
        if (s2->len == 0) return 1;
        ret = memcmp(s1->data, s2->data, s2->len);
        return ret? ret: 1;
    } else if (s1->len < s2->len) {
        if (s1->len == 0) return 1;
        ret = memcmp(s1->data, s2->data, s1->len);
        return ret? ret: -1;
    }
    if (s1->len == 0) return 0;
    return memcmp(s1->data, s2->data, s1->len);
}

int mln_string_strcmp(mln_string_t *s1, mln_string_t *s2)
{
    if (s1 == s2 || s1->data == s2->data) return 0;
    if (s1->len > s2->len) return 1;
    if (s1->len < s2->len) return -1;
    if (s1->len > 280 || (s1->len % sizeof(mln_u32_t)))
        return memcmp(s1->data, s2->data, s1->len);
    mln_u32_t *i1 = (mln_u32_t *)(s1->data), *i2 = (mln_u32_t *)(s2->data), i;
    mln_s32_t res;
    for (i = 0; i < s1->len; ) {
        if ((res = (*i1++ - *i2++)) != 0)
            return res;
        i += sizeof(mln_u32_t);
    }
    return 0;
}

int mln_string_const_strcmp(mln_string_t *s1, char *s2)
{
    if (s1->data == (mln_u8ptr_t)s2) return 0;
    mln_u32_t len = strlen(s2);
    if (s1->len > len) return 1;
    if (s1->len < len) return -1;
    if (s1->len > 280 || (len % sizeof(mln_u32_t)))
        return memcmp(s1->data, s2, len);

    mln_u32_t *i1 = (mln_u32_t *)(s1->data), *i2 = (mln_u32_t *)s2, i;
    mln_s32_t res;
    for (i = 0; i < len; ) {
        if ((res = (*i1++ - *i2++)) != 0)
            return res;
        i += sizeof(mln_u32_t);
    }
    return 0;
}

int mln_string_strncmp(mln_string_t *s1, mln_string_t *s2, mln_u32_t n)
{
    if (s1 == s2 || s1->data == s2->data) return 0;
    if (s1->len < n || s2->len < n) return -1;
    if (n > 280 || (n % sizeof(mln_u32_t)))
        return memcmp(s1->data, s2->data, n);

    mln_u32_t *i1 = (mln_u32_t *)(s1->data), *i2 = (mln_u32_t *)(s2->data), i;
    mln_s32_t res;
    for (i = 0; i < n; ) {
        if ((res = (*i1++ - *i2++)) != 0)
            return res;
        i += sizeof(mln_u32_t);
    }
    return 0;
}

int mln_string_const_strncmp(mln_string_t *s1, char *s2, mln_u32_t n)
{
    if (s1->data == (mln_u8ptr_t)s2) return 0;
    if (s1->len < n || strlen(s2) < n) return -1;
    if (n > 280 || (n % sizeof(mln_u32_t)))
        return memcmp(s1->data, s2, n);

    mln_u32_t *i1 = (mln_u32_t *)(s1->data), *i2 = (mln_u32_t *)s2, i;
    mln_s32_t res;
    for (i = 0; i < n; ) {
        if ((res = (*i1++ - *i2++)) != 0)
            return res;
        i += sizeof(mln_u32_t);
    }
    return 0;
}

int mln_string_strcasecmp(mln_string_t *s1, mln_string_t *s2)
{
    if (s1 == s2 || s1->data == s2->data) return 0;
    if (s1->len > s2->len) return 1;
    if (s1->len < s2->len) return -1;
    return strncasecmp((char *)(s1->data), (char *)(s2->data), s1->len);
}

int mln_string_strncasecmp(mln_string_t *s1, mln_string_t *s2, mln_u32_t n)
{
    if (s1 == s2 || s1->data == s2->data) return 0;
    if (s1->len < n || s2->len < n) return -1;
    return strncasecmp((char *)(s1->data), (char *)(s2->data), n);
}

int mln_string_const_strcasecmp(mln_string_t *s1, char *s2)
{
    if (s1->data == (mln_u8ptr_t)s2) return 0;
    mln_u32_t len = strlen(s2);
    if (s1->len > len) return 1;
    if (s1->len < len) return -1;
    return strncasecmp((char *)(s1->data), s2, len);
}

int mln_string_const_strncasecmp(mln_string_t *s1, char *s2, mln_u32_t n)
{
    if (s1->data == (mln_u8ptr_t)s2) return 0;
    mln_u32_t len = strlen(s2);
    if (s1->len < n || len < n) return -1;
    return strncasecmp((char *)(s1->data), s2, n);
}

char *mln_string_strstr(mln_string_t *text, mln_string_t *pattern)
{
    if (text == pattern || text->data == pattern->data)
        return (char *)(text->data);
    return strstr((char *)(text->data), (char *)(pattern->data));
}

char *mln_string_const_strstr(mln_string_t *text, char *pattern)
{
    if (text->data == (mln_u8ptr_t)pattern)
        return (char *)(text->data);
    return strstr((char *)(text->data), pattern);
}

mln_string_t *mln_string_new_strstr(mln_string_t *text, mln_string_t *pattern)
{
    char *p = mln_string_strstr(text, pattern);
    if (p == NULL) return NULL;
    return mln_string_assign(p, text->len - (p - (char *)(text->data)));
}

mln_string_t *mln_string_new_const_strstr(mln_string_t *text, char *pattern)
{
    char *p = mln_string_const_strstr(text, pattern);
    if (p == NULL) return NULL;
    return mln_string_assign(p, text->len - (p - (char *)(text->data)));
}

char *mln_string_kmp(mln_string_t *text, mln_string_t *pattern)
{
    if (text == pattern || text->data == pattern->data)
        return (char *)(text->data);
    return kmp_string_match((char *)(text->data), (char *)(pattern->data), text->len, pattern->len);
}

char *mln_string_const_kmp(mln_string_t *text, char *pattern)
{
    if (text->data == (mln_u8ptr_t)pattern)
        return (char *)(text->data);
    return kmp_string_match((char *)(text->data), pattern, text->len, strlen(pattern));
}

mln_string_t *mln_string_new_kmp(mln_string_t *text, mln_string_t *pattern)
{
    char *p = mln_string_kmp(text, pattern);
    if (p == NULL) return NULL;
    return mln_string_assign(p, text->len - (p - (char *)(text->data)));
}

mln_string_t *mln_string_new_const_kmp(mln_string_t *text, char *pattern)
{
    char *p = mln_string_const_kmp(text, pattern);
    if (p == NULL) return NULL;
    return mln_string_assign(p, text->len - (p - (char *)(text->data)));
}

/*
 * kmp_string_match() and compute_prefix_function() are 
 * components of KMP algorithm.
 * The longer pattern's prefix matched and existed
 * the higher performance of KMP algorithm made.
 */

static inline char *
kmp_string_match(char *text, const char *pattern, int text_len, int pattern_len)
{
    int *shift = compute_prefix_function(pattern, pattern_len);
    if (shift == NULL) return NULL;
    int q = 0, i;
    for (i = 0; i<text_len; ++i) {
        while (q > 0 && pattern[q] != text[i])
            q = shift[q - 1];
        if (pattern[q] == text[i])
            ++q;
        if (q == pattern_len) {
            free(shift);
            return &text[i-pattern_len+1];
          /*
           * we just return the first position.
           */
           /*q = shift[q];*/
        }
    }
    free(shift);
    return NULL;
}

static inline int *compute_prefix_function(const char *pattern, int m)
{
    int *shift = (int *)malloc(sizeof(int)*m);
    if (shift == NULL) return NULL;
    shift[0] = 0;
    int k = 0, q;
    for (q = 1; q<m; ++q) {
        while (k > 0 && pattern[k] != pattern[q])
            k = shift[k - 1];
        if (pattern[k] == pattern[q])
            ++k;
        shift[q] = k;
    }
    return shift;
}

mln_string_t *mln_string_slice(mln_string_t *s, const char *sep_array/*ended by \0*/)
{
    const char *ps;
    mln_string_t *tmp = mln_string_dup(s);
    mln_u8_t ascii[256] = {0};

    if (tmp == NULL) return NULL;

    for (ps = sep_array; *ps != 0; ++ps) {
        ascii[(mln_u8_t)(*ps)] = 1;
    }
    return mln_string_slice_recursive((char *)(tmp->data), tmp->len, ascii, 1, tmp);
}

static mln_string_t *mln_string_slice_recursive(char *s, mln_u64_t len, mln_u8ptr_t ascii, int cnt, mln_string_t *save)
{
    char *jmp_ascii, *end = s + len;
    for (jmp_ascii = s; jmp_ascii < end; ++jmp_ascii) {
        if (!ascii[(mln_u8_t)(*jmp_ascii)]) break;
        *jmp_ascii = 0;
    }
    if (jmp_ascii >= end) {
        mln_string_t *ret = (mln_string_t *)malloc(sizeof(mln_string_t)*cnt);
        if (ret == NULL) return NULL;
        ret[cnt-1].data = (mln_u8ptr_t)save;
        ret[cnt-1].len = 0;
        ret[cnt-1].data_ref = 0;
        ret[cnt-1].pool = 0;
        ret[cnt-1].ref = 1;
        return ret;
    }
    ++cnt;
    char *jmp_valid;
    for (jmp_valid = jmp_ascii; jmp_valid < end; ++jmp_valid) {
        if (ascii[(mln_u8_t)(*jmp_valid)]) break;
    }
    mln_string_t *array = mln_string_slice_recursive(jmp_valid, len-(jmp_valid-s), ascii, cnt, save);
    if (array == NULL) return NULL;
    array[cnt-2].data = (mln_u8ptr_t)jmp_ascii;
    array[cnt-2].len = jmp_valid - jmp_ascii;
    array[cnt-2].data_ref = 1;
    array[cnt-2].pool = 0;
    array[cnt-2].ref = 1;
    return array;
}

void mln_string_slice_free(mln_string_t *array)
{
    mln_string_t *s = &array[0];
    for (; s->len; ++s)
        ;
    mln_string_free((mln_string_t *)(s->data));
    free(array);
}

mln_string_t *mln_string_strcat(mln_string_t *s1, mln_string_t *s2)
{
    mln_string_t *ret = (mln_string_t *)malloc(sizeof(mln_string_t));
    if (ret == NULL) return NULL;
    mln_u64_t len = s1->len + s2->len;
    if (len == 0) {
        ret->data = NULL;
        ret->len = 0;
        ret->data_ref = 0;
        ret->pool = 0;
        ret->ref = 1;
        return ret;
    }
    if ((ret->data = (mln_u8ptr_t)malloc(len + 1)) == NULL) {
        free(ret);
        return NULL;
    }
    if (s1->len > 0) memcpy(ret->data, s1->data, s1->len);
    if (s2->len > 0) memcpy(ret->data+s1->len, s2->data, s2->len);
    ret->data[len] = 0;
    ret->len = len;
    ret->data_ref = 0;
    ret->pool = 0;
    ret->ref = 1;
    return ret;
}

mln_string_t *mln_string_pool_strcat(mln_alloc_t *pool, mln_string_t *s1, mln_string_t *s2)
{
    mln_string_t *ret = (mln_string_t *)mln_alloc_m(pool, sizeof(mln_string_t));
    if (ret == NULL) return NULL;
    mln_u64_t len = s1->len + s2->len;
    if (len == 0) {
        ret->data = NULL;
        ret->len = 0;
        ret->data_ref = 0;
        ret->pool = 1;
        ret->ref = 1;
        return ret;
    }
    if ((ret->data = (mln_u8ptr_t)mln_alloc_m(pool, len + 1)) == NULL) {
        mln_alloc_free(ret);
        return NULL;
    }
    if (s1->len > 0) memcpy(ret->data, s1->data, s1->len);
    if (s2->len > 0) memcpy(ret->data+s1->len, s2->data, s2->len);
    ret->data[len] = 0;
    ret->len = len;
    ret->data_ref = 0;
    ret->pool = 1;
    ret->ref = 1;
    return ret;
}

mln_string_t *mln_string_trim(mln_string_t *s, mln_string_t *mask)
{
    mln_u8_t chars[256] = {0};
    mln_string_t tmp;
    mln_size_t i, j;

    for (i = 0; i < mask->len; ++i) {
        chars[mask->data[i]] = 1;
    }
    for (i = 0; i < s->len; ++i) {
        if (!chars[s->data[i]]) break;
    }
    for (j = s->len; j > i; --j) {
        if (!chars[s->data[j - 1]]) break;
    }

    mln_string_nset(&tmp, &(s->data[i]), j - i);
    return mln_string_dup(&tmp);
}

mln_string_t *mln_string_pool_trim(mln_alloc_t *pool, mln_string_t *s, mln_string_t *mask)
{
    mln_u8_t chars[256] = {0};
    mln_string_t tmp;
    mln_size_t i, j;

    for (i = 0; i < mask->len; ++i) {
        chars[mask->data[i]] = 1;
    }
    for (i = 0; i < s->len; ++i) {
        if (!chars[s->data[i]]) break;
    }
    for (j = s->len; j > i; --j) {
        if (!chars[s->data[j - 1]]) break;
    }

    mln_string_nset(&tmp, &(s->data[i]), j - i);
    return mln_string_pool_dup(pool, &tmp);
}

void mln_string_upper(mln_string_t *s)
{
    mln_u8_t chars[256] = {0};
    mln_u8_t upper[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    mln_u8_t lower[] = "abcdefghijklmnopqrstuvwxyz";
    mln_size_t i;

    for (i = 0; i < sizeof(lower) - 1; ++i) {
        chars[lower[i]] = upper[i];
    }
    for (i = 0; i < s->len; ++i) {
        if (chars[s->data[i]])
            s->data[i] = chars[s->data[i]];
    }
}

void mln_string_lower(mln_string_t *s)
{
    mln_u8_t chars[256] = {0};
    mln_u8_t upper[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    mln_u8_t lower[] = "abcdefghijklmnopqrstuvwxyz";
    mln_size_t i;

    for (i = 0; i < sizeof(upper) - 1; ++i) {
        chars[upper[i]] = lower[i];
    }
    for (i = 0; i < s->len; ++i) {
        if (chars[s->data[i]])
            s->data[i] = chars[s->data[i]];
    }
}

