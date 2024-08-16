
/*
 * Copyright (C) Niklaus F.Schen.
 */

#ifndef __MLN_STRING_H
#define __MLN_STRING_H
#include "mln_types.h"
#include <string.h>
#include "mln_alloc.h"
#include <stdlib.h>

typedef struct {
    mln_u8ptr_t  data;
    mln_u64_t    len;
    mln_uauto_t  data_ref:1;
    mln_uauto_t  pool:1;
    mln_uauto_t  ref:30;
} mln_string_t;

/*
 * init & free
 */
#define mln_string(s) {(mln_u8ptr_t)s, sizeof(s)-1, 1, 0, 1}
#if !defined(MSVC)
#define mln_string_set(pstring,s) \
    ({\
        (pstring)->data = (mln_u8ptr_t)(s);\
        (pstring)->len = strlen(s);\
        (pstring)->data_ref = 1;\
        (pstring)->pool = 0;\
        (pstring)->ref = 1;\
        (pstring);\
    })
#define mln_string_nset(pstring,s,n) \
    ({\
        (pstring)->data = (mln_u8ptr_t)(s);\
        (pstring)->len = (n);\
        (pstring)->data_ref = 1;\
        (pstring)->pool = 0;\
        (pstring)->ref = 1;\
        (pstring);\
    })
#define mln_string_ref(pstring) ({\
    mln_string_t *__s = (pstring);\
    ++__s->ref;\
    __s;\
})

#define mln_string_free(pstr) \
({\
    mln_string_t *__s = (pstr);\
    if (__s != NULL) {\
        if (__s->ref-- <= 1) {\
            if (!__s->data_ref && __s->data != NULL) {\
                if (__s->pool) mln_alloc_free(__s->data);\
                else free(__s->data);\
            }\
            if (__s->pool) mln_alloc_free(__s);\
            else free(__s);\
        }\
    }\
})
#else
extern mln_string_t *mln_string_set(mln_string_t *str, char *s);
extern mln_string_t *mln_string_nset(mln_string_t *str, char *s, mln_u64_t n);
extern mln_string_t *mln_string_ref(mln_string_t *s);
extern void mln_string_free(mln_string_t *s);
#endif


extern mln_string_t *mln_string_new(const char *s);
extern mln_string_t *mln_string_pool_new(mln_alloc_t *pool, const char *s);
/*
 * mln_string_buf_new & mln_string_buf_pool_new
 * The argument buf should be allocated from the same place: same pool or malloc
 */
extern mln_string_t *mln_string_buf_new(mln_u8ptr_t buf, mln_u64_t len);
extern mln_string_t *mln_string_buf_pool_new(mln_alloc_t *pool, mln_u8ptr_t buf, mln_u64_t len);
extern mln_string_t *mln_string_dup(mln_string_t *str) __NONNULL1(1);
extern mln_string_t *mln_string_pool_dup(mln_alloc_t *pool, mln_string_t *str) __NONNULL2(1,2);
extern mln_string_t *mln_string_alloc(mln_s32_t size);
extern mln_string_t *mln_string_pool_alloc(mln_alloc_t *pool, mln_s32_t size) __NONNULL1(1);
extern mln_string_t *mln_string_const_ndup(char *str, mln_s32_t size) __NONNULL1(1);
extern mln_string_t *mln_string_ref_dup(mln_string_t *str) __NONNULL1(1);
extern mln_string_t *mln_string_const_ref_dup(char *s);
extern mln_string_t *mln_string_concat(mln_string_t *s1, mln_string_t *s2, mln_string_t *sep);
extern mln_string_t *mln_string_pool_concat(mln_alloc_t *pool, mln_string_t *s1, mln_string_t *s2, mln_string_t *sep);

/*
 * tool functions
 */
extern int mln_string_strseqcmp(mln_string_t *s1, mln_string_t *s2) __NONNULL2(1,2);
extern int mln_string_strcmp(mln_string_t *s1, mln_string_t *s2) __NONNULL2(1,2);
extern int mln_string_const_strcmp(mln_string_t *s1, char *s2) __NONNULL1(1);
extern int mln_string_strncmp(mln_string_t *s1, mln_string_t *s2, mln_u32_t n) __NONNULL2(1,2);
extern int mln_string_const_strncmp(mln_string_t *s1, char *s2, mln_u32_t n) __NONNULL1(1);
extern int mln_string_strcasecmp(mln_string_t *s1, mln_string_t *s2) __NONNULL2(1,2);
extern int mln_string_const_strcasecmp(mln_string_t *s1, char *s2) __NONNULL1(1);
extern int mln_string_const_strncasecmp(mln_string_t *s1, char *s2, mln_u32_t n) __NONNULL1(1);
extern int mln_string_strncasecmp(mln_string_t *s1, mln_string_t *s2, mln_u32_t n) __NONNULL2(1,2);
extern char *mln_string_strstr(mln_string_t *text, mln_string_t *pattern) __NONNULL2(1,2);
extern char *mln_string_const_strstr(mln_string_t *text, char *pattern) __NONNULL2(1,2);
/*
 * if text and pattern are NOT matched, 
 * mln_string_new_strstr() & mln_string_new_const_strstr() will return NULL.
 */
extern mln_string_t *mln_string_new_strstr(mln_string_t *text, mln_string_t *pattern) __NONNULL2(1,2);
extern mln_string_t *mln_string_new_const_strstr(mln_string_t *text, char *pattern) __NONNULL2(1,2);
/*
 * The longer pattern's prefix matched and existed
 * the higher performance of KMP algorithm made.
 */
extern char *mln_string_kmp(mln_string_t *text, mln_string_t *pattern) __NONNULL2(1,2);
extern char *mln_string_const_kmp(mln_string_t *text, char *pattern) __NONNULL1(1);
/*
 * if text and pattern are NOT matched, 
 * mln_string_new_kmp() & mln_string_new_const_kmp() will return NULL.
 */
extern mln_string_t *mln_string_new_kmp(mln_string_t *text, mln_string_t *pattern) __NONNULL2(1,2);
extern mln_string_t *mln_string_new_const_kmp(mln_string_t *text, char *pattern) __NONNULL2(1,2);
/*
 * mln_string_slice will modify s.
 * So if you want to get avoid this side-effect,
 * you can call mln_string_dup() or mln_string_ndup()
 * before you call mln_string_slice().
 */
extern mln_string_t *mln_string_slice(mln_string_t *s, const char *sep_array/*ended by \0*/) __NONNULL2(1,2);
extern void mln_string_slice_free(mln_string_t *array) __NONNULL1(1);
extern mln_string_t *mln_string_strcat(mln_string_t *s1, mln_string_t *s2) __NONNULL2(1,2);
extern mln_string_t *mln_string_pool_strcat(mln_alloc_t *pool, mln_string_t *s1, mln_string_t *s2) __NONNULL2(1,2);
extern mln_string_t *mln_string_trim(mln_string_t *s, mln_string_t *mask);
extern mln_string_t *mln_string_pool_trim(mln_alloc_t *pool, mln_string_t *s, mln_string_t *mask);
extern void mln_string_upper(mln_string_t *s) __NONNULL1(1);
extern void mln_string_lower(mln_string_t *s) __NONNULL1(1);
#if defined(MLN_C99)
extern int strncasecmp(const char *s1, const char *s2, size_t n);
#endif
#endif

