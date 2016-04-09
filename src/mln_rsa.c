
/*
 * Copyright (C) Niklaus F.Schen.
 */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "mln_rsa.h"

mln_rsa_key_t *mln_rsa_key_new(void)
{
    return (mln_rsa_key_t *)malloc(sizeof(mln_rsa_key_t));
}

mln_rsa_key_t *mln_rsa_key_new_pool(mln_alloc_t *pool)
{
    return (mln_rsa_key_t *)mln_alloc_m(pool, sizeof(mln_rsa_key_t));
}

void mln_rsa_key_free(mln_rsa_key_t *key)
{
    if (key == NULL) return;
    free(key);
}

void mln_rsa_key_free_pool(mln_rsa_key_t *key)
{
    if (key == NULL) return;
    mln_alloc_free(key);
}

int mln_rsa_keyGenerate(mln_rsa_key_t *pub, mln_rsa_key_t *pri, mln_u32_t bits)
{
    if (bits < 16) return -1;

    mln_bignum_t p, q, n, phiN, one, d;
    mln_bignum_assign(&one, "1", 1);
    mln_bignum_t e;

lp:
    while (1) {
        if (mln_bignum_prime(&p, (bits>>1)-1) < 0) return -1;
        if (mln_bignum_prime(&q, (bits>>1)-1) < 0) return -1;;
        if (mln_bignum_compare(&p, &q)) break;
    }
    memcpy(&n, &p, sizeof(n));

    mln_bignum_mul(&n, &q);

    mln_bignum_sub(&p, &one);
    mln_bignum_sub(&q, &one);
    memcpy(&phiN, &p, sizeof(phiN));
    mln_bignum_mul(&phiN, &q);

    mln_bignum_assign(&e, "0x10001", 7);

    if (mln_bignum_extendEulid(&e, &phiN, &d, NULL, NULL) < 0) goto lp;
    if (mln_bignum_compare(&d, &one) <= 0) goto lp;

    if (pub != NULL) {
        memcpy(&(pub->n), &n, sizeof(n));
        memcpy(&(pub->ed), &e, sizeof(e));
    }
    if (pri != NULL) {
        memcpy(&(pri->n), &n, sizeof(n));
        memcpy(&(pri->ed), &d, sizeof(d));
    }

    return 0;
}

int mln_rsa_calc(mln_rsa_key_t *key, mln_bignum_t *in, mln_bignum_t *out)
{
    mln_bignum_t tmp;
    memcpy(&tmp, in, sizeof(tmp));
    if (mln_bignum_pwr(&tmp, &(key->ed), &(key->n)) < 0) return -1;
    memcpy(out, &tmp, sizeof(tmp));
    return 0;
}

