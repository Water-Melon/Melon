
/*
 * Copyright (C) Niklaus F.Schen.
 */
#ifndef __MLN_RSA_H
#define __MLN_RSA_H

#include "mln_bignum.h"
#include "mln_alloc.h"

typedef struct {
    mln_bignum_t n;
    mln_bignum_t ed;
} mln_rsa_key_t;


extern mln_rsa_key_t *mln_rsa_key_new(void);
extern mln_rsa_key_t *mln_rsa_key_new_pool(mln_alloc_t *pool) __NONNULL1(1);
extern void mln_rsa_key_free(mln_rsa_key_t *key);
extern void mln_rsa_key_free_pool(mln_rsa_key_t *key);
extern int mln_rsa_keyGenerate(mln_rsa_key_t *pub, mln_rsa_key_t *pri, mln_u32_t bits);
extern int mln_rsa_calc(mln_rsa_key_t *key, mln_bignum_t *in, mln_bignum_t *out) __NONNULL3(1,2,3);

#endif

