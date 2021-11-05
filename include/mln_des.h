
/*
 * Copyright (C) Niklaus F.Schen.
 */
#ifndef __MLN_DES_H
#define __MLN_DES_H

/*
 * DES took me five days!!!
 * Chinese blogs of DES made me insane!
 * The DES theory is very easy to understand,
 * but there is a thing they never said in blogs.
 * A very important thing -- the sequence of bits in a byte.
 * Most implementation I saw were all use an integer array
 * to store every bit in a byte. And when they transform
 * bits to array, they changed bit sequence which is
 * contrasted with bit shift sequence at the same time.
 * For example,
 *   '1' == 0x31 == 0011 0001
 * after transformation,
 *   '1' => unsigned char arr[] = {0, 0, 1, 1, 0, 0, 0, 1}
 * and which is the No.3 bit (start from 1)?
 * the answer is 1 -- arr[3-1];
 * which is the left part of 'arr'?
 * the answer is {0, 0, 1, 1}.
 * That this is a big trap!
 * I wanted to enhance the performance of DES calculation,
 * so I used a 64-bit integer instead of an integer array.
 * That means I should understand the theory in array way
 * but implement it in integer way.
 * So you may find out my implementation is converse.
 */

#include "mln_types.h"
#include "mln_alloc.h"

#define __M_DES_SUBKEY_SIZE 16
#define __M_DES_ROR28(x,n)  ((((x) << (28 - (n))) | ((x) >> (n))) & 0xfffffff)
#define __M_DES_ROL28(x,n)  ((((x) >> (28 - (n))) | ((x) << (n))) & 0xfffffff)

typedef struct {
    mln_u64_t key;
    mln_u64_t sub_keys[__M_DES_SUBKEY_SIZE];
} mln_des_t;

extern void mln_des_init(mln_des_t *d, mln_u64_t key) __NONNULL1(1);
extern mln_des_t *mln_des_new(mln_u64_t key);
extern mln_des_t *mln_des_pool_new(mln_alloc_t *pool, mln_u64_t key) __NONNULL1(1);
extern void mln_des_free(mln_des_t *d);
extern void mln_des_pool_free(mln_des_t *d);
extern mln_u64_t mln_des(mln_des_t *d, mln_u64_t msg, mln_u32_t is_encrypt) __NONNULL1(1);
extern void mln_des_buf(mln_des_t *d, \
                        mln_u8ptr_t in, mln_uauto_t inlen, \
                        mln_u8ptr_t out, mln_uauto_t outlen, \
                        mln_u8_t fill, \
                        mln_u32_t is_encrypt) __NONNULL3(1,2,4);

typedef struct {
    mln_des_t _1key;
    mln_des_t _2key;
} mln_3des_t;

extern void mln_3des_init(mln_3des_t *tdes, mln_u64_t key1, mln_u64_t key2) __NONNULL1(1);
extern mln_3des_t *mln_3des_new(mln_u64_t key1, mln_u64_t key2);
extern mln_3des_t *mln_3des_pool_new(mln_alloc_t *pool, mln_u64_t key1, mln_u64_t key2) __NONNULL1(1);
extern void mln_3des_free(mln_3des_t *tdes);
extern void mln_3des_pool_free(mln_3des_t *tdes);
extern mln_u64_t mln_3des(mln_3des_t *tdes, mln_u64_t msg, mln_u32_t is_encrypt) __NONNULL1(1);
extern void mln_3des_buf(mln_3des_t *tdes, \
                         mln_u8ptr_t in, mln_uauto_t inlen, \
                         mln_u8ptr_t out, mln_uauto_t outlen, \
                         mln_u8_t fill, \
                         mln_u32_t is_encrypt) __NONNULL3(1,2,4);

#endif

