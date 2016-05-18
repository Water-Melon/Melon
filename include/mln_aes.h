
/*
 * Copyright (C) Niklaus F.Schen.
 */
#ifndef __MLN_AES_H
#define __MLN_AES_H

#include "mln_types.h"
#include "mln_alloc.h"

#define M_AES_128         0
#define M_AES_192         1
#define M_AES_256         2

#define __MLN_AES_Nb      4
#define __MLN_AES128_Nr   10
#define __MLN_AES192_Nr   12
#define __MLN_AES256_Nr   14
#define __MLN_AES128_Nk   4
#define __MLN_AES192_Nk   6
#define __MLN_AES256_Nk   8

#define __MLN_AES_XTIME(x) ((x<<1) ^ (((x>>7) & 1) * 0x1b))
#define __MLN_AES_SHIFTROW(x,n) ((((x) << (n)) | ((x) >> (32 - (n)))) & 0xffffffff)
#define __MLN_AES_MULTIB_01(x) ((x) & 0xff)
#define __MLN_AES_MULTIB_02(x) __MLN_AES_XTIME(x)
#define __MLN_AES_MULTIB_03(x) ((__MLN_AES_MULTIB_02(x) ^ (x)) & 0xff)
#define __MLN_AES_MULTIB_09(x) ((__MLN_AES_MULTIB_02(__MLN_AES_MULTIB_02(__MLN_AES_MULTIB_02(x))) ^ (x)) & 0xff)
#define __MLN_AES_MULTIB_0b(x) ((__MLN_AES_MULTIB_02(__MLN_AES_MULTIB_02(__MLN_AES_MULTIB_02(x))) ^ __MLN_AES_MULTIB_02(x) ^ (x)) & 0xff)
#define __MLN_AES_MULTIB_0d(x) ((__MLN_AES_MULTIB_02(__MLN_AES_MULTIB_02(__MLN_AES_MULTIB_02(x))) ^ __MLN_AES_MULTIB_02(__MLN_AES_MULTIB_02(x)) ^ (x)) & 0xff)
#define __MLN_AES_MULTIB_0e(x) ((__MLN_AES_MULTIB_02(__MLN_AES_MULTIB_02(__MLN_AES_MULTIB_02(x))) ^ __MLN_AES_MULTIB_02(__MLN_AES_MULTIB_02(x)) ^ __MLN_AES_MULTIB_02(x)) & 0xff)

typedef struct {
    mln_u32_t bits;
    mln_u32_t w[60];
} mln_aes_t;


extern int mln_aes_init(mln_aes_t *a, mln_u8ptr_t key, mln_u32_t bits) __NONNULL2(1,2);
extern mln_aes_t *mln_aes_new(mln_u8ptr_t key, mln_u32_t bits) __NONNULL1(1);
extern mln_aes_t *mln_aes_pool_new(mln_alloc_t *pool, mln_u8ptr_t key, mln_u32_t bits) __NONNULL2(1,2);
extern void mln_aes_free(mln_aes_t *a);
extern void mln_aes_pool_free(mln_aes_t *a);
extern int mln_aes_encrypt(mln_aes_t *a, mln_u8ptr_t text);
extern int mln_aes_decrypt(mln_aes_t *a, mln_u8ptr_t cipher);

#endif

