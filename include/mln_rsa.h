
/*
 * Copyright (C) Niklaus F.Schen.
 */
#ifndef __MLN_RSA_H
#define __MLN_RSA_H

#include "mln_bignum.h"
#include "mln_alloc.h"

#define M_EMSAPKCS1V15_HASH_MD5    0
#define M_EMSAPKCS1V15_HASH_SHA1   1
#define M_EMSAPKCS1V15_HASH_SHA256 2

typedef struct {
    mln_bignum_t n;
    mln_bignum_t ed;
    mln_bignum_t p;
    mln_bignum_t q;
    mln_bignum_t dp;
    mln_bignum_t dq;
    mln_bignum_t qinv;
} mln_rsa_key_t;


extern mln_rsa_key_t *mln_rsa_key_new(void);
extern mln_rsa_key_t *mln_rsa_key_pool_new(mln_alloc_t *pool) __NONNULL1(1);
extern void mln_rsa_key_free(mln_rsa_key_t *key);
extern void mln_rsa_key_pool_free(mln_rsa_key_t *key);
extern int mln_rsa_keyGenerate(mln_rsa_key_t *pub, mln_rsa_key_t *pri, mln_u32_t bits);
extern mln_string_t *mln_RSAESPKCS1V15PubEncrypt(mln_rsa_key_t *pub, mln_string_t *text) __NONNULL2(1,2);
extern mln_string_t *mln_RSAESPKCS1V15PubDecrypt(mln_rsa_key_t *pub, mln_string_t *cipher) __NONNULL2(1,2);
extern mln_string_t *mln_RSAESPKCS1V15PriEncrypt(mln_rsa_key_t *pri, mln_string_t *text) __NONNULL2(1,2);
extern mln_string_t *mln_RSAESPKCS1V15PriDecrypt(mln_rsa_key_t *pri, mln_string_t *cipher) __NONNULL2(1,2);
extern void mln_RSAESPKCS1V15Free(mln_string_t *s);
extern mln_string_t *mln_RSASSAPKCS1V15SIGN(mln_rsa_key_t *pri, mln_string_t *m, mln_u32_t hashType) __NONNULL2(1,2);
extern int mln_RSASSAPKCS1V15VERIFY(mln_rsa_key_t *pub, mln_string_t *m, mln_string_t *s, mln_u32_t hashType) __NONNULL3(1,2,3);

#endif

