
/*
 * Copyright (C) Niklaus F.Schen.
 */
#ifndef __MLN_SHA_H
#define __MLN_SHA_H

#include "mln_types.h"
#include "mln_alloc.h"

#define __M_SHA_BUFLEN 64

typedef struct {
    mln_u32_t H0;
    mln_u32_t H1;
    mln_u32_t H2;
    mln_u32_t H3;
    mln_u32_t H4;
    mln_u64_t length;
    mln_u32_t pos;
    mln_u8_t  buf[__M_SHA_BUFLEN];
} mln_sha1_t;

extern void mln_sha1_init(mln_sha1_t *s) __NONNULL1(1);
extern mln_sha1_t *mln_sha1_new(void);
extern mln_sha1_t *mln_sha1_pool_new(mln_alloc_t *pool) __NONNULL1(1);
extern void mln_sha1_free(mln_sha1_t *s);
extern void mln_sha1_pool_free(mln_sha1_t *s);
extern void mln_sha1_calc(mln_sha1_t *s, mln_u8ptr_t input, mln_uauto_t len, mln_u32_t is_last) __NONNULL1(1);
extern void mln_sha1_tobytes(mln_sha1_t *s, mln_u8ptr_t buf, mln_u32_t len) __NONNULL1(1);
extern void mln_sha1_tostring(mln_sha1_t *s, mln_s8ptr_t buf, mln_u32_t len) __NONNULL1(1);
extern void mln_sha1_dump(mln_sha1_t *s) __NONNULL1(1);


typedef struct {
    mln_u32_t H0;
    mln_u32_t H1;
    mln_u32_t H2;
    mln_u32_t H3;
    mln_u32_t H4;
    mln_u32_t H5;
    mln_u32_t H6;
    mln_u32_t H7;
    mln_u64_t length;
    mln_u32_t pos;
    mln_u8_t  buf[__M_SHA_BUFLEN];
} mln_sha256_t;

extern void mln_sha256_init(mln_sha256_t *s) __NONNULL1(1);
extern mln_sha256_t *mln_sha256_new(void);
extern mln_sha256_t *mln_sha256_pool_new(mln_alloc_t *pool) __NONNULL1(1);
extern void mln_sha256_free(mln_sha256_t *s);
extern void mln_sha256_pool_free(mln_sha256_t *s);
extern void mln_sha256_calc(mln_sha256_t *s, mln_u8ptr_t input, mln_uauto_t len, mln_u32_t is_last) __NONNULL1(1);
extern void mln_sha256_tobytes(mln_sha256_t *s, mln_u8ptr_t buf, mln_u32_t len) __NONNULL1(1);
extern void mln_sha256_tostring(mln_sha256_t *s, mln_s8ptr_t buf, mln_u32_t len) __NONNULL1(1);
extern void mln_sha256_dump(mln_sha256_t *s) __NONNULL1(1);

#endif

