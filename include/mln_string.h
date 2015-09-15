
/*
 * Copyright (C) Niklaus F.Schen.
 */

#ifndef __MLN_STRING_H
#define __MLN_STRING_H
#include "mln_types.h"
#include <string.h>
#include "mln_alloc.h"

typedef struct {
    mln_s8ptr_t  str;
    mln_s32_t    len;
    mln_u32_t    is_referred;
} mln_string_t;

/*
 * init & free
 */
#define mln_string(s) {s, sizeof(s)-1}
#define mln_string_set(pstring,s) \
    {\
        (pstring)->str = (mln_s8ptr_t)(s);\
        (pstring)->len = strlen(s);\
        (pstring)->is_referred = 1;\
    }

extern mln_string_t *
mln_new_string(const char *s);
extern mln_string_t *
mln_new_string_pool(mln_alloc_t *pool, const char *s);
extern mln_string_t *
mln_dup_string(mln_string_t *str) __NONNULL1(1);
extern mln_string_t *
mln_ndup_string(mln_string_t *str, mln_s32_t size) __NONNULL1(1);
extern mln_string_t *
mln_refer_string(mln_string_t *str) __NONNULL1(1);
extern mln_string_t *
mln_refer_const_string(char *s);
extern void
mln_free_string(mln_string_t *str);
extern void
mln_free_string_pool(mln_string_t *str);

/*
 * tool functions
 */
extern int
mln_strcmp(mln_string_t *s1, mln_string_t *s2) __NONNULL2(1,2);
extern int
mln_const_strcmp(mln_string_t *s1, const char *s2) __NONNULL1(1);
extern int
mln_strncmp(mln_string_t *s1, mln_string_t *s2, mln_u32_t n) __NONNULL2(1,2);
extern int
mln_const_strncmp(mln_string_t *s1, const char *s2, mln_u32_t n) __NONNULL1(1);
extern int
mln_strcasecmp(mln_string_t *s1, mln_string_t *s2) __NONNULL2(1,2);
extern int
mln_const_strcasecmp(mln_string_t *s1, const char *s2) __NONNULL1(1);
extern int
mln_const_strncasecmp(mln_string_t *s1, const char *s2, mln_u32_t n) __NONNULL1(1);
extern int
mln_strncasecmp(mln_string_t *s1, mln_string_t *s2, mln_u32_t n) __NONNULL2(1,2);
extern char *
mln_strstr(mln_string_t *text, mln_string_t *pattern) __NONNULL2(1,2);
extern char *
mln_const_strstr(mln_string_t *text, const char *pattern) __NONNULL2(1,2);
/*
 * if text and pattern are NOT matched, 
 * mln_str_strstr() & mln_str_const_strstr() will return NULL.
 */
extern mln_string_t *
mln_str_strstr(mln_string_t *text, mln_string_t *pattern) __NONNULL2(1,2);
extern mln_string_t *
mln_str_const_strstr(mln_string_t *text, const char *pattern) __NONNULL2(1,2);
/*
 * The longer pattern's prefix matched and existed
 * the higher performance of KMP algorithm made.
 */
extern char *
mln_kmp_strstr(mln_string_t *text, mln_string_t *pattern) __NONNULL2(1,2);
extern char *
mln_const_kmp_strstr(mln_string_t *text, const char *pattern) __NONNULL1(1);
/*
 * if text and pattern are NOT matched, 
 * mln_str_kmp_strstr() & mln_str_const_kmp_strstr() will return NULL.
 */
extern mln_string_t *
mln_str_kmp_strstr(mln_string_t *text, mln_string_t *pattern) __NONNULL2(1,2);
extern mln_string_t *
mln_str_const_kmp_strstr(mln_string_t *text, const char *pattern) __NONNULL2(1,2);
/*
 * mln_slice will modify s.
 * So if you want to get avoid this side-effect,
 * you can call mln_dup_string() or mln_ndup_string()
 * before you call mln_slice().
 */
extern mln_string_t *
mln_slice(mln_string_t *s, const char *sep_array/*ended by \0*/) __NONNULL2(1,2);
extern void
mln_slice_free(mln_string_t *array) __NONNULL1(1);
#endif

