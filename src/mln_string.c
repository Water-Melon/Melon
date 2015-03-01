
/*
 * Copyright (C) Niklaus F.Schen.
 */

#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include "mln_string.h"

static inline mln_string_t *mln_assign_string(char *s, mln_s32_t len);

static inline int *compute_prefix_function(const char *pattern, int m);
static inline char *kmp_string_match(char *text, const char *pattern) __NONNULL1(1);
static mln_string_t *mln_slice_recursive(char *s, int len, char *ascii, int cnt) __NONNULL2(1,3);

static inline mln_string_t *mln_assign_string(char *s, mln_s32_t len)
{
    mln_string_t *str;
    str = (mln_string_t *)malloc(sizeof(mln_string_t));
    if (str == NULL) return NULL;
    str->str = s;
    str->len = len;
    str->is_referred = 1;
    return str;
}

mln_string_t *mln_new_string(const char *s)
{
    mln_string_t *str = (mln_string_t *)malloc(sizeof(mln_string_t));
    if (str == NULL) return NULL;
    if (s == NULL) {
        str->str = NULL;
        str->len = 0;
        str->is_referred = 0;
        return str;
    }
    mln_s32_t len = strlen(s);
    str->str = (mln_s8ptr_t)malloc(len + 1);
    if (str->str == NULL) {
        free(str);
        return NULL;
    }
    memcpy(str->str, s, len);
    str->str[len] = 0;
    str->len = len;
    str->is_referred = 0;
    return str;
}

mln_string_t *mln_dup_string(mln_string_t *str)
{
    mln_string_t *s = (mln_string_t *)malloc(sizeof(mln_string_t));
    if (s == NULL) return NULL;
    s->str = (mln_s8ptr_t)malloc(str->len + 1);
    if (s->str == NULL) {
        free(s);
        return NULL;
    }
    memcpy(s->str, str->str, str->len);
    s->str[str->len] = 0;
    s->len = str->len;
    s->is_referred = 0;
    return s;
}

mln_string_t *mln_ndup_string(mln_string_t *str, mln_s32_t size)
{
    if (size <= 0) return NULL;
    mln_string_t *s = (mln_string_t *)malloc(sizeof(mln_string_t));
    if (s == NULL) return NULL;
    mln_s32_t min = size > str->len ? str->len : size;
    s->str = (mln_s8ptr_t)malloc(min + 1);
    if (s->str == NULL) {
        free(s);
        return NULL;
    }
    memcpy(s->str, str->str, min);
    s->str[min] = 0;
    s->len = min;
    s->is_referred = 0;
    return s;
}

mln_string_t *mln_refer_string(mln_string_t *str)
{
    mln_string_t *s = (mln_string_t *)malloc(sizeof(mln_string_t));
    if (s == NULL) return NULL;
    s->str = str->str;
    s->len = str->len;
    s->is_referred = 1;
    return s;
}

mln_string_t *mln_refer_const_string(char *s)
{
    mln_string_t *str = (mln_string_t *)malloc(sizeof(mln_string_t));
    if (s == NULL) return NULL;
    str->str = (mln_s8ptr_t)s;
    str->len = strlen(s);
    str->is_referred = 1;
    return str;
}

void mln_free_string(mln_string_t *str)
{
    if (str == NULL) return;
    if (!str->is_referred && str->str != NULL)
        free(str->str);
    free(str);
}

int mln_strcmp(mln_string_t *s1, mln_string_t *s2)
{
    if (s1 == s2 || s1->str == s2->str) return 0;
    if (s1->len != s2->len) return s1->len - s2->len;
    if (s1->len > 280) return strncmp(s1->str, s2->str, s1->len);
    mln_s32_t i1 = 0, i2 = 0, j = 0, k = 0;
    mln_s8ptr_t c1 = s1->str, c2 = s2->str;
    while (j < s1->len) {
        i1 |= (c1[j]<<k);
        i2 |= (c2[j]<<k);
        k += 8;
        if (k > 24) {
            if (i1 != i2) return i1 - i2;
            i1 = i2 = k = 0;
        }
        j++;
    }
    if (i1 != i2) return i1 - i2;
    return 0;
}

int mln_const_strcmp(mln_string_t *s1, const char *s2)
{
    if (s1->str == s2) return 0;
    mln_s32_t len = strlen(s2);
    if (s1->len != len) return s1->len - len;
    if (s1->len > 280) return strncmp(s1->str, s2, len);
    mln_s32_t i1 = 0, i2 = 0, j = 0, k = 0;
    mln_s8ptr_t c1 = s1->str;
    const char *c2 = s2;
    while (j < s1->len) {
        i1 |= (c1[j]<<k);
        i2 |= (c2[j]<<k);
        k += 8;
        if (k > 24) {
            if (i1 != i2) return i1 - i2;
            i1 = i2 = k = 0;
        }
        j++;
    }
    if (i1 != i2) return i1 - i2;
    return 0;
}

int mln_strncmp(mln_string_t *s1, mln_string_t *s2, mln_u32_t n)
{
    if (s1 == s2 || s1->str == s2->str) return 0;
    if (s1->len < n || s2->len < n) return -1;
    if (n > 280) return strncmp(s1->str, s2->str, n);
    mln_s32_t i1 = 0, i2 = 0;
    mln_u32_t j = 0, k = 0;
    mln_s8ptr_t c1 = s1->str, c2 = s2->str;
    while (j < n) {
        i1 |= (c1[j]<<k);
        i2 |= (c2[j]<<k);
        k += 8;
        if (k > 24) {
            if (i1 != i2) return i1 - i2;
            i1 = i2 = k = 0;
        }
        j++;
    }
    if (i1 != i2) return i1 - i2;
    return 0;
}

int mln_const_strncmp(mln_string_t *s1, const char *s2, mln_u32_t n)
{
    if (s1->str == s2) return 0;
    mln_s32_t len = strlen(s2);
    if (s1->len < n || len < n) return -1;
    if (n > 280) return strncmp(s1->str, s2, n);
    mln_s32_t i1 = 0, i2 = 0;
    mln_u32_t j = 0, k = 0;
    mln_s8ptr_t c1 = s1->str;
    const char *c2 = s2;
    while (j < n) {
        i1 |= (c1[j]<<k);
        i2 |= (c2[j]<<k);
        k += 8;
        if (k > 24) {
            if (i1 != i2) return i1 - i2;
            i1 = i2 = k = 0;
        }
        j++;
    }
    if (i1 != i2) return i1 - i2;
    return 0;
}

int mln_strcasecmp(mln_string_t *s1, mln_string_t *s2)
{
    if (s1 == s2 || s1->str == s2->str) return 0;
    if (s1->len != s2->len) return s1->len - s2->len;
    return strncasecmp(s1->str, s2->str, s1->len);
}

int mln_strncasecmp(mln_string_t *s1, mln_string_t *s2, mln_u32_t n)
{
    if (s1 == s2 || s1->str == s2->str) return 0;
    if (s1->len < n || s2->len < n) return -1;
    return strncasecmp(s1->str, s2->str, n);
}

int mln_const_strcasecmp(mln_string_t *s1, const char *s2)
{
    if (s1->str == s2) return 0;
    mln_s32_t len = strlen(s2);
    if (s1->len != len) return s1->len - len;
    return strncasecmp(s1->str, s2, len);
}

int mln_const_strncasecmp(mln_string_t *s1, const char *s2, mln_u32_t n)
{
    if (s1->str == s2) return 0;
    mln_s32_t len = strlen(s2);
    if (s1->len < n || len < n) return -1;
    return strncasecmp(s1->str, s2, n);
}

char *mln_strstr(mln_string_t *text, mln_string_t *pattern)
{
    if (text == pattern || text->str == pattern->str)
        return text->str;
    return strstr(text->str, pattern->str);
}

char *mln_const_strstr(mln_string_t *text, const char *pattern)
{
    if (text->str == pattern)
        return text->str;
    return strstr(text->str, pattern);
}

mln_string_t *mln_str_strstr(mln_string_t *text, mln_string_t *pattern)
{
    char *p = mln_strstr(text, pattern);
    if (p == NULL) return NULL;
    return mln_assign_string(p, text->len - (p - text->str));
}

mln_string_t *mln_str_const_strstr(mln_string_t *text, const char *pattern)
{
    char *p = mln_const_strstr(text, pattern);
    if (p == NULL) return NULL;
    return mln_assign_string(p, text->len - (p - text->str));
}

char *mln_kmp_strstr(mln_string_t *text, mln_string_t *pattern)
{
    if (text == pattern || text->str == pattern->str)
        return text->str;
    return kmp_string_match(text->str, pattern->str);
}

char *mln_const_kmp_strstr(mln_string_t *text, const char *pattern)
{
    if (text->str == pattern)
        return text->str;
    return kmp_string_match(text->str, pattern);
}

mln_string_t *mln_str_kmp_strstr(mln_string_t *text, mln_string_t *pattern)
{
    char *p = mln_kmp_strstr(text, pattern);
    if (p == NULL) return NULL;
    return mln_assign_string(p, text->len - (p - text->str));
}

mln_string_t *mln_str_const_kmp_strstr(mln_string_t *text, const char *pattern)
{
    char *p = mln_const_kmp_strstr(text, pattern);
    if (p == NULL) return NULL;
    return mln_assign_string(p, text->len - (p - text->str));
}

/*
 * kmp_string_match() and compute_prefix_function() are 
 * components of KMP algorithm.
 * The longer pattern's prefix matched and existed
 * the higher performance of KMP algorithm made.
 */

static inline char *kmp_string_match(char *text, const char *pattern)
{
    int n = strlen(text);
    int m = strlen(pattern);
    int *shift = compute_prefix_function(pattern, m);
    if (shift == NULL) return NULL;
    int q = 0, i;
    for (i = 0; i<n; i++) {
        while (q > 0 && pattern[q] != text[i])
            q = shift[q - 1];
        if (pattern[q] == text[i])
            q++;
        if (q == m) {
            free(shift);
            return &text[i-m+1];
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
    for (q = 1; q<m; q++) {
        while (k > 0 && pattern[k] != pattern[q])
            k = shift[k - 1];
        if (pattern[k] == pattern[q])
            k++;
        shift[q] = k;
    }
    return shift;
}

mln_string_t *mln_slice(mln_string_t *s, const char *sep_array/*ended by \0*/)
{
    const char *ps;
    char ascii[256] = {0};
    for (ps = sep_array; *ps != 0; ps++) {
        ascii[(int)(*ps)] = 1;
    }
    return mln_slice_recursive(s->str, s->len, ascii, 1);
}

static mln_string_t *mln_slice_recursive(char *s, int len, char *ascii, int cnt)
{
    char *jmp_ascii, *end = s + len;
    for (jmp_ascii = s; jmp_ascii < end; jmp_ascii++) {
        if (!ascii[(int)(*jmp_ascii)]) break;
        *jmp_ascii = 0;
    }
    if (jmp_ascii >= end) {
        mln_string_t *ret = (mln_string_t *)malloc(sizeof(mln_string_t)*cnt);
        if (ret == NULL) return NULL;
        ret[cnt-1].str = NULL;
        ret[cnt-1].len = 0;
        ret[cnt-1].is_referred = 0;
        return ret;
    }
    cnt++;
    char *jmp_valid;
    for (jmp_valid = jmp_ascii; jmp_valid < end; jmp_valid++) {
        if (ascii[(int)(*jmp_valid)]) break;
    }
    mln_string_t *array = mln_slice_recursive(jmp_valid, len-(jmp_valid-s), ascii, cnt);
    if (array == NULL) return NULL;
    array[cnt-2].str = jmp_ascii;
    array[cnt-2].len = jmp_valid - jmp_ascii;
    array[cnt-2].is_referred = 1;
    return array;
}

void mln_slice_free(mln_string_t *array)
{
    free(array);
}

