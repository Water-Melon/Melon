
/*
 * Copyright (C) Niklaus F.Schen.
 */
#ifndef __MLN_BIGNUM_H
#define __MLN_BIGNUM_H

#include "mln_types.h"
#include "mln_string.h"
#include "mln_alloc.h"

#define M_BIGNUM_POSITIVE 0
#define M_BIGNUM_NEGATIVE 1
#define M_BIGNUM_SIZE     257
#define M_BIGNUM_BITS     8224
#define M_BIGNUM_UMAX     ((mln_u64_t)0x100000000llu)
#define M_BIGNUM_SHIFT    32

typedef struct {
    mln_u32_t tag;
    mln_u32_t length;
    mln_u64_t data[M_BIGNUM_SIZE];
} mln_bignum_t;

#if defined(MSVC)
#define mln_bignum_init(bn) do {\
    bn.tag = M_BIGNUM_POSITIVE;\
    bn.length = 0;\
    memset((bn).data, 0, sizeof(mln_u64_t)*M_BIGNUM_SIZE);\
} while (0)
#else
#define mln_bignum_init(bn)         ({\
    bn.tag = M_BIGNUM_POSITIVE;\
    bn.length = 0;\
    memset((bn).data, 0, sizeof(mln_u64_t)*M_BIGNUM_SIZE);\
})
#endif
#define mln_bignum_positive(pbn)    ((pbn)->tag = M_BIGNUM_POSITIVE)
#define mln_bignum_negative(pbn)    ((pbn)->tag = M_BIGNUM_NEGATIVE)
#define mln_bignum_is_positive(pbn) ((pbn)->tag == M_BIGNUM_POSITIVE)
#define mln_bignum_is_negative(pbn) ((pbn)->tag == M_BIGNUM_NEGATIVE)
#define mln_bignum_get_length(pbn)  ((pbn)->length)
#define mln_bignum_zero()           {M_BIGNUM_POSITIVE, 0, {0}}
extern mln_bignum_t *mln_bignum_new(void);
extern mln_bignum_t *mln_bignum_pool_new(mln_alloc_t *pool) __NONNULL1(1);
extern void mln_bignum_free(mln_bignum_t *bn);
extern void mln_bignum_pool_free(mln_bignum_t *bn);
extern mln_bignum_t *mln_bignum_dup(mln_bignum_t *bn) __NONNULL1(1);
extern mln_bignum_t *mln_bignum_pool_dup(mln_alloc_t *pool, mln_bignum_t *bn) __NONNULL2(1,2);
extern int mln_bignum_assign(mln_bignum_t *bn, mln_s8ptr_t sval, mln_u32_t len) __NONNULL2(1,2);
extern void mln_bignum_add(mln_bignum_t *dest, mln_bignum_t *src) __NONNULL2(1,2);
extern void mln_bignum_sub(mln_bignum_t *dest, mln_bignum_t *src) __NONNULL2(1,2);
extern void mln_bignum_mul(mln_bignum_t *dest, mln_bignum_t *src) __NONNULL2(1,2);
extern int mln_bignum_div(mln_bignum_t *dest, mln_bignum_t *src, mln_bignum_t *quotient) __NONNULL2(1,2);
extern int mln_bignum_pwr(mln_bignum_t *dest, mln_bignum_t *exponent, mln_bignum_t *mod) __NONNULL2(1,2);
extern int mln_bignum_compare(mln_bignum_t *bn1, mln_bignum_t *bn2) __NONNULL2(1,2);
extern int mln_bignum_abs_compare(mln_bignum_t *bn1, mln_bignum_t *bn2) __NONNULL2(1,2);
extern int mln_bignum_bit_test(mln_bignum_t *bn, mln_u32_t index) __NONNULL1(1);
extern void mln_bignum_left_shift(mln_bignum_t *bn, mln_u32_t n) __NONNULL1(1);
extern void mln_bignum_right_shift(mln_bignum_t *bn, mln_u32_t n) __NONNULL1(1);
extern int mln_bignum_prime(mln_bignum_t *res, mln_u32_t bitwidth) __NONNULL1(1);
extern int mln_bignum_extend_eulid(mln_bignum_t *a, mln_bignum_t *b, mln_bignum_t *x, mln_bignum_t *y) __NONNULL2(1,2);
extern void mln_bignum_dump(mln_bignum_t *bn);
extern int mln_bignum_i2osp(mln_bignum_t *n, mln_u8ptr_t buf, mln_size_t len) __NONNULL2(1,2);
extern int mln_bignum_os2ip(mln_bignum_t *n, mln_u8ptr_t buf, mln_size_t len) __NONNULL2(1,2);
extern mln_string_t *mln_bignum_tostring(mln_bignum_t *n) __NONNULL1(1);

#endif
