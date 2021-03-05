
/*
 * Copyright (C) Niklaus F.Schen.
 */

#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include "mln_string.h"


static inline int *compute_prefix_function(const char *pattern, int m);
static inline char *
kmp_string_match(char *text, const char *pattern, int text_len, int pattern_len) __NONNULL2(1,2);
static mln_string_t *mln_string_slice_recursive(char *s, mln_u64_t len, mln_u8ptr_t ascii, int cnt) __NONNULL2(1,3);
static inline mln_string_t *mln_assign_string(char *s, mln_u32_t len);

static inline mln_string_t *mln_assign_string(char *s, mln_u32_t len)
{
    mln_string_t *str;
    str = (mln_string_t *)malloc(sizeof(mln_string_t));
    if (str == NULL) return NULL;
    str->data = (mln_u8ptr_t)s;
    str->len = len;
    str->pool = 0;
    str->is_referred = 1;
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
        str->is_referred = 0;
        str->pool = 1;
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
    str->is_referred = 0;
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
        str->is_referred = 0;
        str->pool = 0;
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
    str->is_referred = 0;
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
    memcpy(s->data, str->data, str->len);
    s->data[str->len] = 0;
    s->len = str->len;
    s->is_referred = 0;
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
    memcpy(s->data, str->data, str->len);
    s->data[str->len] = 0;
    s->len = str->len;
    s->is_referred = 0;
    s->pool = 1;
    s->ref = 1;
    return s;
}

mln_string_t *mln_string_nDup(mln_string_t *str, mln_s32_t size)
{
    if (size <= 0) return NULL;
    mln_string_t *s = (mln_string_t *)malloc(sizeof(mln_string_t));
    if (s == NULL) return NULL;
    mln_s32_t min = size > str->len ? str->len : size;
    if ((s->data = (mln_u8ptr_t)malloc(min + 1)) == NULL) {
        free(s);
        return NULL;
    }
    memcpy(s->data, str->data, min);
    s->data[min] = 0;
    s->len = min;
    s->is_referred = 0;
    s->pool = 0;
    s->ref = 1;
    return s;
}

mln_string_t *mln_string_nConstDup(char *str, mln_s32_t size)
{
    if (size <= 0) return NULL;
    mln_string_t *s = (mln_string_t *)malloc(sizeof(mln_string_t));
    if (s == NULL) return NULL;
    if ((s->data = (mln_u8ptr_t)malloc(size + 1)) == NULL) {
        free(s);
        return NULL;
    }
    memcpy(s->data, str, size);
    s->data[size] = 0;
    s->len = size;
    s->is_referred = 0;
    s->pool = 0;
    s->ref = 1;
    return s;
}

mln_string_t *mln_string_refDup(mln_string_t *str)
{
    mln_string_t *s = (mln_string_t *)malloc(sizeof(mln_string_t));
    if (s == NULL) return NULL;
    s->data = str->data;
    s->len = str->len;
    s->is_referred = 1;
    s->pool = 0;
    s->ref = 1;
    return s;
}

mln_string_t *mln_string_refConstDup(char *s)
{
    mln_string_t *str = (mln_string_t *)malloc(sizeof(mln_string_t));
    if (s == NULL) return NULL;
    str->data = (mln_u8ptr_t)s;
    str->len = strlen(s);
    str->is_referred = 1;
    str->pool = 0;
    str->ref = 1;
    return str;
}

void mln_string_free(mln_string_t *str)
{
    if (str == NULL) return;
    if (str->ref > 1) {
        --str->ref;
        return;
    }
    if (!str->is_referred && str->data != NULL) {
        if (str->pool) mln_alloc_free(str->data);
        else free(str->data);
    }
    if (str->pool) mln_alloc_free(str);
    else free(str);
}

int mln_string_strcmpSeq(mln_string_t *s1, mln_string_t *s2)
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
    if (s1->len > 280) return memcmp(s1->data, s2->data, s1->len);
    mln_u32_t i1 = 0, i2 = 0, j = 0, k = 0;
    mln_u8ptr_t c1 = s1->data, c2 = s2->data;
    while (j < s1->len) {
        i1 |= (c1[j]<<k);
        i2 |= (c2[j]<<k);
        k += 8;
        if (k > 24) {
            if (i1 > i2) return 1;
            if (i1 < i2) return -1;
            i1 = i2 = k = 0;
        }
        ++j;
    }
    if (i1 > i2) return 1;
    if (i1 < i2) return -1;
    return 0;
}

int mln_string_constStrcmp(mln_string_t *s1, char *s2)
{
    if (s1->data == (mln_u8ptr_t)s2) return 0;
    mln_u32_t len = strlen(s2);
    if (s1->len > len) return 1;
    if (s1->len < len) return -1;
    if (s1->len > 280) return memcmp(s1->data, s2, len);
    mln_u32_t i1 = 0, i2 = 0, j = 0, k = 0;
    mln_u8ptr_t c1 = s1->data, c2 = (mln_u8ptr_t)s2;
    while (j < s1->len) {
        i1 |= (c1[j]<<k);
        i2 |= (c2[j]<<k);
        k += 8;
        if (k > 24) {
            if (i1 > i2) return 1;
            if (i1 < i2) return -1;
            i1 = i2 = k = 0;
        }
        ++j;
    }
    if (i1 > i2) return 1;
    if (i1 < i2) return -1;
    return 0;
}

int mln_string_strncmp(mln_string_t *s1, mln_string_t *s2, mln_u32_t n)
{
    if (s1 == s2 || s1->data == s2->data) return 0;
    if (s1->len < n || s2->len < n) return -1;
    if (n > 280) return memcmp(s1->data, s2->data, n);
    mln_u32_t i1 = 0, i2 = 0;
    mln_u32_t j = 0, k = 0;
    mln_u8ptr_t c1 = s1->data, c2 = s2->data;
    while (j < n) {
        i1 |= (c1[j]<<k);
        i2 |= (c2[j]<<k);
        k += 8;
        if (k > 24) {
            if (i1 > i2) return 1;
            if (i1 < i2) return -1;
            i1 = i2 = k = 0;
        }
        ++j;
    }
    if (i1 > i2) return 1;
    if (i1 < i2) return -1;
    return 0;
}

int mln_string_constStrncmp(mln_string_t *s1, char *s2, mln_u32_t n)
{
    if (s1->data == (mln_u8ptr_t)s2) return 0;
    mln_u32_t len = strlen(s2);
    if (s1->len < n || len < n) return -1;
    if (n > 280) return memcmp(s1->data, s2, n);
    mln_u32_t i1 = 0, i2 = 0;
    mln_u32_t j = 0, k = 0;
    mln_u8ptr_t c1 = s1->data, c2 = (mln_u8ptr_t)s2;
    while (j < n) {
        i1 |= (c1[j]<<k);
        i2 |= (c2[j]<<k);
        k += 8;
        if (k > 24) {
            if (i1 > i2) return 1;
            if (i1 < i2) return -1;
            i1 = i2 = k = 0;
        }
        ++j;
    }
    if (i1 > i2) return 1;
    if (i1 < i2) return -1;
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

int mln_string_constStrcasecmp(mln_string_t *s1, char *s2)
{
    if (s1->data == (mln_u8ptr_t)s2) return 0;
    mln_u32_t len = strlen(s2);
    if (s1->len > len) return 1;
    if (s1->len < len) return -1;
    return strncasecmp((char *)(s1->data), s2, len);
}

int mln_string_constStrncasecmp(mln_string_t *s1, char *s2, mln_u32_t n)
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

char *mln_string_constStrstr(mln_string_t *text, char *pattern)
{
    if (text->data == (mln_u8ptr_t)pattern)
        return (char *)(text->data);
    return strstr((char *)(text->data), pattern);
}

mln_string_t *mln_string_S_strstr(mln_string_t *text, mln_string_t *pattern)
{
    char *p = mln_string_strstr(text, pattern);
    if (p == NULL) return NULL;
    return mln_assign_string(p, text->len - (p - (char *)(text->data)));
}

mln_string_t *mln_string_S_constStrstr(mln_string_t *text, char *pattern)
{
    char *p = mln_string_constStrstr(text, pattern);
    if (p == NULL) return NULL;
    return mln_assign_string(p, text->len - (p - (char *)(text->data)));
}

char *mln_string_KMPStrstr(mln_string_t *text, mln_string_t *pattern)
{
    if (text == pattern || text->data == pattern->data)
        return (char *)(text->data);
    return kmp_string_match((char *)(text->data), (char *)(pattern->data), text->len, pattern->len);
}

char *mln_string_KMPConstStrstr(mln_string_t *text, char *pattern)
{
    if (text->data == (mln_u8ptr_t)pattern)
        return (char *)(text->data);
    return kmp_string_match((char *)(text->data), pattern, text->len, strlen(pattern));
}

mln_string_t *mln_string_S_KMPStrstr(mln_string_t *text, mln_string_t *pattern)
{
    char *p = mln_string_KMPStrstr(text, pattern);
    if (p == NULL) return NULL;
    return mln_assign_string(p, text->len - (p - (char *)(text->data)));
}

mln_string_t *mln_string_S_KMPConstStrstr(mln_string_t *text, char *pattern)
{
    char *p = mln_string_KMPConstStrstr(text, pattern);
    if (p == NULL) return NULL;
    return mln_assign_string(p, text->len - (p - (char *)(text->data)));
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
    mln_u8_t ascii[256] = {0};
    for (ps = sep_array; *ps != 0; ++ps) {
        ascii[(mln_u8_t)(*ps)] = 1;
    }
    return mln_string_slice_recursive((char *)(s->data), s->len, ascii, 1);
}

static mln_string_t *mln_string_slice_recursive(char *s, mln_u64_t len, mln_u8ptr_t ascii, int cnt)
{
    char *jmp_ascii, *end = s + len;
    for (jmp_ascii = s; jmp_ascii < end; ++jmp_ascii) {
        if (!ascii[(mln_u8_t)(*jmp_ascii)]) break;
        *jmp_ascii = 0;
    }
    if (jmp_ascii >= end) {
        mln_string_t *ret = (mln_string_t *)malloc(sizeof(mln_string_t)*cnt);
        if (ret == NULL) return NULL;
        ret[cnt-1].data = NULL;
        ret[cnt-1].len = 0;
        ret[cnt-1].is_referred = 0;
        ret[cnt-1].pool = 0;
        ret[cnt-1].ref = 1;
        return ret;
    }
    ++cnt;
    char *jmp_valid;
    for (jmp_valid = jmp_ascii; jmp_valid < end; ++jmp_valid) {
        if (ascii[(mln_u8_t)(*jmp_valid)]) break;
    }
    mln_string_t *array = mln_string_slice_recursive(jmp_valid, len-(jmp_valid-s), ascii, cnt);
    if (array == NULL) return NULL;
    array[cnt-2].data = (mln_u8ptr_t)jmp_ascii;
    array[cnt-2].len = jmp_valid - jmp_ascii;
    array[cnt-2].is_referred = 1;
    array[cnt-2].pool = 0;
    array[cnt-2].ref = 1;
    return array;
}

void mln_string_slice_free(mln_string_t *array)
{
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
        ret->is_referred = 0;
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
    ret->is_referred = 0;
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
        ret->is_referred = 0;
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
    ret->is_referred = 0;
    ret->pool = 1;
    ret->ref = 1;
    return ret;
}

