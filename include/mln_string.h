
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
#define mln_string(s) {(mln_s8ptr_t)s, sizeof(s)-1, 1}
#define mln_string_set(pstring,s); \
    {\
        (pstring)->str = (mln_s8ptr_t)(s);\
        (pstring)->len = strlen(s);\
        (pstring)->is_referred = 1;\
    }
#define mln_string_nSet(pstring,s,n); \
    {\
        (pstring)->str = (mln_s8ptr_t)(s);\
        (pstring)->len = (n);\
        (pstring)->is_referred = 1;\
    }

extern mln_string_t *
mln_string_new(const char *s);
extern mln_string_t *
mln_string_pool_new(mln_alloc_t *pool, const char *s);
extern mln_string_t *
mln_string_dup(mln_string_t *str) __NONNULL1(1);
extern mln_string_t *
mln_string_pool_dup(mln_alloc_t *pool, mln_string_t *str) __NONNULL2(1,2);
extern mln_string_t *
mln_string_nDup(mln_string_t *str, mln_s32_t size) __NONNULL1(1);
extern mln_string_t *
mln_string_nConstDup(char *str, mln_s32_t size) __NONNULL1(1);
extern mln_string_t *
mln_string_refDup(mln_string_t *str) __NONNULL1(1);
extern mln_string_t *
mln_string_refConstDup(char *s);
extern void
mln_string_free(mln_string_t *str);
extern void
mln_string_pool_free(mln_string_t *str);

/*
 * tool functions
 */
extern int
mln_string_strcmp(mln_string_t *s1, mln_string_t *s2) __NONNULL2(1,2);
extern int
mln_string_constStrcmp(mln_string_t *s1, const char *s2) __NONNULL1(1);
extern int
mln_string_strncmp(mln_string_t *s1, mln_string_t *s2, mln_u32_t n) __NONNULL2(1,2);
extern int
mln_string_constStrncmp(mln_string_t *s1, const char *s2, mln_u32_t n) __NONNULL1(1);
extern int
mln_string_strcasecmp(mln_string_t *s1, mln_string_t *s2) __NONNULL2(1,2);
extern int
mln_string_constStrcasecmp(mln_string_t *s1, const char *s2) __NONNULL1(1);
extern int
mln_string_constStrncasecmp(mln_string_t *s1, const char *s2, mln_u32_t n) __NONNULL1(1);
extern int
mln_string_strncasecmp(mln_string_t *s1, mln_string_t *s2, mln_u32_t n) __NONNULL2(1,2);
extern char *
mln_string_strstr(mln_string_t *text, mln_string_t *pattern) __NONNULL2(1,2);
extern char *
mln_string_constStrstr(mln_string_t *text, const char *pattern) __NONNULL2(1,2);
/*
 * if text and pattern are NOT matched, 
 * mln_string_S_strstr() & mln_string_S_constStrstr() will return NULL.
 */
extern mln_string_t *
mln_string_S_strstr(mln_string_t *text, mln_string_t *pattern) __NONNULL2(1,2);
extern mln_string_t *
mln_string_S_constStrstr(mln_string_t *text, const char *pattern) __NONNULL2(1,2);
/*
 * The longer pattern's prefix matched and existed
 * the higher performance of KMP algorithm made.
 */
extern char *
mln_string_KMPStrstr(mln_string_t *text, mln_string_t *pattern) __NONNULL2(1,2);
extern char *
mln_string_KMPConstStrstr(mln_string_t *text, const char *pattern) __NONNULL1(1);
/*
 * if text and pattern are NOT matched, 
 * mln_string_S_KMPStrstr() & mln_string_S_KMPConstStrstr() will return NULL.
 */
extern mln_string_t *
mln_string_S_KMPStrstr(mln_string_t *text, mln_string_t *pattern) __NONNULL2(1,2);
extern mln_string_t *
mln_string_S_KMPConstStrstr(mln_string_t *text, const char *pattern) __NONNULL2(1,2);
/*
 * mln_string_slice will modify s.
 * So if you want to get avoid this side-effect,
 * you can call mln_string_dup() or mln_string_nDup()
 * before you call mln_string_slice().
 */
extern mln_string_t *
mln_string_slice(mln_string_t *s, const char *sep_array/*ended by \0*/) __NONNULL2(1,2);
extern void
mln_string_slice_free(mln_string_t *array) __NONNULL1(1);
#endif

