
/*
 * Copyright (C) Niklaus F.Schen.
 */
#if defined(MSVC)
#define _CRT_RAND_S
#endif
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#if defined(MSVC)
#include "mln_utils.h"
#else
#include <sys/time.h>
#endif
#include "mln_rsa.h"
#include "mln_chain.h"
#include "mln_md5.h"
#include "mln_sha.h"
#include "mln_asn1.h"
#include "mln_func.h"

static inline int mln_rsa_rsaep_rsadp(mln_rsa_key_t *key, mln_bignum_t *in, mln_bignum_t *out);
static inline void mln_rsa_pub_padding(mln_u8ptr_t in, mln_size_t inlen, mln_u8ptr_t out, mln_size_t keylen);
static inline void mln_rsa_pri_padding(mln_u8ptr_t in, mln_size_t inlen, mln_u8ptr_t out, mln_size_t keylen);
static inline mln_u8ptr_t mln_rsa_anti_padding_public(mln_u8ptr_t in, mln_size_t len);
static inline mln_u8ptr_t mln_rsa_anti_padding_private(mln_u8ptr_t in, mln_size_t len);
static inline mln_string_t *mln_EMSAPKCS1V15_encode(mln_alloc_t *pool, mln_string_t *m, mln_u32_t hash_type);
static inline mln_string_t *mln_EMSAPKCS1V15_decode(mln_alloc_t *pool, mln_string_t *e, mln_u32_t *hash_type);

static mln_u8_t EMSAPKCS1V15_HASH_MD5[] = \
{0x2a, 0x86, 0x48, 0x86, 0xf7, 0xd, 0x2, 0x5};
static mln_u8_t EMSAPKCS1V15_HASH_SHA1[] = \
{0x2b, 0xe, 0x3, 0x2, 0x1a};
static mln_u8_t EMSAPKCS1V15_HASH_SHA256[] = \
{0x60, 0x86, 0x48, 0x1, 0x65, 0x3, 0x4, 0x2, 0x1};
struct mln_EMSAPKCS1V15_HASH_s {
    mln_u8ptr_t digest_algorithm;
    mln_size_t  len;
} EMSAPKCS1V15_HASH[] = {
    {EMSAPKCS1V15_HASH_MD5, sizeof(EMSAPKCS1V15_HASH_MD5)},
    {EMSAPKCS1V15_HASH_SHA1, sizeof(EMSAPKCS1V15_HASH_SHA1)},
    {EMSAPKCS1V15_HASH_SHA256, sizeof(EMSAPKCS1V15_HASH_SHA256)}
};

MLN_FUNC(, mln_rsa_key_t *, mln_rsa_key_new, (void), (), {
    return (mln_rsa_key_t *)malloc(sizeof(mln_rsa_key_t));
})

MLN_FUNC(, mln_rsa_key_t *, mln_rsa_key_pool_new, (mln_alloc_t *pool), (pool), {
    return (mln_rsa_key_t *)mln_alloc_m(pool, sizeof(mln_rsa_key_t));
})

MLN_FUNC_VOID(, void, mln_rsa_key_free, (mln_rsa_key_t *key), (key), {
    if (key == NULL) return;
    free(key);
})

MLN_FUNC_VOID(, void, mln_rsa_key_pool_free, (mln_rsa_key_t *key), (key), {
    if (key == NULL) return;
    mln_alloc_free(key);
})

MLN_FUNC(, int, mln_rsa_key_generate, \
         (mln_rsa_key_t *pub, mln_rsa_key_t *pri, mln_u32_t bits), \
         (pub, pri, bits), \
{
    if (bits <= 88 || bits > M_BIGNUM_BITS) return -1;

    mln_bignum_t p, q, n, phi_n, one, d;
    mln_bignum_assign(&one, "1", 1);
    mln_bignum_t e, tmpp_1, tmpq_1;

lp:
    while (1) {
        if (mln_bignum_prime(&p, (bits>>1)+1) < 0) return -1;
        if (mln_bignum_prime(&q, (bits>>1)) < 0) return -1;
        if (mln_bignum_compare(&p, &q)) break;
    }
    n = p;

    mln_bignum_mul(&n, &q);

    if (!mln_bignum_bit_test(&n, bits-1)) goto lp;

    if (pub != NULL) {
        pub->p = p;
        pub->q = q;
        if (mln_bignum_extend_eulid(&q, &p, &(pub->qinv), NULL) < 0) goto lp;
        if (mln_bignum_compare(&(pub->qinv), &one) < 0) goto lp;
    }
    if (pri != NULL) {
        pri->p = p;
        pri->q = q;
        if (mln_bignum_extend_eulid(&q, &p, &(pri->qinv), NULL) < 0) goto lp;
        if (mln_bignum_compare(&(pri->qinv), &one) < 0) goto lp;
    }
    mln_bignum_sub(&p, &one);
    mln_bignum_sub(&q, &one);
    tmpp_1 = p;
    tmpq_1 = q;
    phi_n = p;
    mln_bignum_mul(&phi_n, &q);

    mln_bignum_assign(&e, "0x10001", 7);

    if (mln_bignum_extend_eulid(&e, &phi_n, &d, NULL) < 0) goto lp;
    if (mln_bignum_compare(&d, &e) <= 0) goto lp;

    if (pub != NULL) {
        pub->n = n;
        pub->ed = e;
        pub->dp = pub->dq = e;
        mln_bignum_div(&(pub->dp), &tmpp_1, NULL);
        mln_bignum_div(&(pub->dq), &tmpq_1, NULL);
        pub->pwr = 0;
    }
    if (pri != NULL) {
        pri->n = n;
        pri->ed = d;
        pri->dp = pri->dq = d;
        mln_bignum_div(&(pri->dp), &tmpp_1, NULL);
        mln_bignum_div(&(pri->dq), &tmpq_1, NULL);
        pri->pwr = 0;
    }

    return 0;
})

MLN_FUNC(static inline, int, mln_rsa_rsaep_rsadp, \
         (mln_rsa_key_t *key, mln_bignum_t *in, mln_bignum_t *out), \
         (key, in, out), \
{
    if (key->pwr || mln_bignum_abs_compare(&(key->ed), &(key->p)) <= 0) {
in:
        if (mln_bignum_pwr(in, &(key->ed), &(key->n)) < 0) return -1;
        *out = *in;
    } else {
        mln_bignum_t m1, m2;
        m1 = m2 = *in;
        mln_bignum_pwr(&m1, &(key->dp), &(key->p));
        mln_bignum_pwr(&m2, &(key->dq), &(key->q));
        mln_bignum_sub(&m1, &m2);
        if (mln_bignum_is_negative(&m1)) goto in;
        mln_bignum_mul(&m1, &(key->qinv));
        mln_bignum_div(&m1, &(key->p), NULL);
        mln_bignum_mul(&m1, &(key->q));
        mln_bignum_add(&m1, &m2);
        mln_bignum_div(&m1, &(key->n), NULL);
        *out = m1;
    }
    return 0;
})

static inline void mln_rsa_pub_padding(mln_u8ptr_t in, mln_size_t inlen, mln_u8ptr_t out, mln_size_t keylen)
{
    mln_size_t j;
    struct timeval tv;
    mln_u32_t val;

    *out++ = 0;
    *out++ = 0x2;
    gettimeofday(&tv, NULL);
    val = tv.tv_sec*1000000+tv.tv_usec;
    for (j = keylen - inlen - 3; j > 0; --j) {
lp:
#if defined(MSVC)
        rand_s(&val);
        *out = val;
#else
        val = *out = rand_r(&val);
#endif
        if (val == 0) {
            gettimeofday(&tv, NULL);
            val = tv.tv_sec*1000000+tv.tv_usec;
            goto lp;
        }
        ++out;
    }
    *out++ = 0;
    memcpy(out, in, inlen);
}

MLN_FUNC_VOID(static inline, void, mln_rsa_pri_padding, \
              (mln_u8ptr_t in, mln_size_t inlen, mln_u8ptr_t out, mln_size_t keylen), \
              (in, inlen, out, keylen), \
{
    mln_size_t j;

    *out++ = 0;
    *out++ = 0x1;
    for (j = keylen - inlen - 3; j > 0 ; --j) {
        *out++ = 0xff;
    }
    *out++ = 0;
    memcpy(out, in, inlen);
})

static inline mln_u8ptr_t mln_rsa_anti_padding_public(mln_u8ptr_t in, mln_size_t len)
{
    if (in == NULL || len == 0 || *in != 0) return NULL;
    mln_u8ptr_t p = in + 2, end = in + len;
    if (*(in+1) != 0x2) return NULL;
    for (; *p != 0 && p < end; ++p)
        ;
    if (p >= end || p+1 >= end) return NULL;
    if (*p++ != 0) return NULL;
    return p;
}

static inline mln_u8ptr_t mln_rsa_anti_padding_private(mln_u8ptr_t in, mln_size_t len)
{
    if (in == NULL || len == 0 || *in != 0) return NULL;
    mln_u8ptr_t p = in + 2, end = in + len;
    if (*(in+1) == 0) {
        for (; *p == 0 && p < end; ++p)
            ;
        if (p >= end) return NULL;
    } else if (*(in+1) == 1) {
        for (; *p == 0xff && p < end; ++p)
            ;
        if (p >= end || p+1 >= end) return NULL;
        if (*p++ != 0) return NULL;
    } else {
        return NULL;
    }
    return p;
}

MLN_FUNC(, mln_string_t *, mln_RSAESPKCS1V15_public_encrypt, \
         (mln_rsa_key_t *pub, mln_string_t *text), (pub, text), \
{
    mln_size_t nlen = mln_bignum_get_length(&(pub->n)) << 2;
    mln_u8ptr_t buf, p;
    mln_u8ptr_t in = text->data;
    mln_string_t *ret = NULL, tmp;
    mln_size_t i = 0, len = text->len, sum;

    if (nlen-11 < text->len) return NULL;

    sum = (len / (nlen-11)) * nlen + (len % (nlen-11))? nlen: 0;
    buf = (mln_u8ptr_t)malloc(sum);
    if (buf == NULL) return NULL;
    p = buf;

    while (len > 0) {
        mln_bignum_t num = mln_bignum_zero();
        i = len > (nlen-11)? (nlen-11): len;

        mln_rsa_pub_padding(in, i, p, nlen);
        if (mln_bignum_os2ip(&num, p, nlen) < 0) {
            free(buf);
            return NULL;
        }
        if (mln_rsa_rsaep_rsadp(pub, &num, &num) < 0) {
            free(buf);
            return NULL;
        }
        if (mln_bignum_i2osp(&num, p, nlen) < 0) {
            free(buf);
            return NULL;
        }

        len -= i;
        in += i;
        p += nlen;
    }

    mln_string_nset(&tmp, buf, sum);
    ret = mln_string_dup(&tmp);
    free(buf);
    return ret;
})

MLN_FUNC(, mln_string_t *, mln_RSAESPKCS1V15_public_decrypt, \
         (mln_rsa_key_t *pub, mln_string_t *cipher), (pub, cipher), \
{
    mln_size_t nlen = mln_bignum_get_length(&(pub->n)) << 2;
    mln_u8ptr_t buf, p, pos;
    mln_u8ptr_t in = cipher->data;
    mln_string_t *ret = NULL, tmp;
    mln_size_t len = cipher->len, sum = 0;

    if (cipher->len < 11 || cipher->len % nlen) {
        return NULL;
    }

    if ((buf = (mln_u8ptr_t)malloc(cipher->len)) == NULL) {
        return NULL;
    }
    p = buf;

    while (len > 0) {
        mln_bignum_t num = mln_bignum_zero();

        if (mln_bignum_os2ip(&num, in, nlen) < 0) {
            free(buf);
            return NULL;
        }

        if (mln_rsa_rsaep_rsadp(pub, &num, &num) < 0) {
            free(buf);
            return NULL;
        }

        if (mln_bignum_i2osp(&num, in, nlen) < 0) {
            free(buf);
            return NULL;
        }

        pos = mln_rsa_anti_padding_private(in, nlen);
        if (pos == NULL) {
            free(buf);
            return NULL;
        }

        memcpy(p, pos, (nlen - (pos - in)));
        sum += (nlen - (pos - in));
        p += (nlen - (pos - in));
        in += nlen;
        len -= nlen;
    }

    mln_string_nset(&tmp, buf, sum);
    ret = mln_string_dup(&tmp);
    free(buf);
    return ret;
})

MLN_FUNC(, mln_string_t *, mln_RSAESPKCS1V15_private_encrypt, \
         (mln_rsa_key_t *pri, mln_string_t *text), (pri, text), \
{
    mln_size_t nlen = mln_bignum_get_length(&(pri->n)) << 2;
    mln_u8ptr_t buf, p;
    mln_u8ptr_t in = text->data;
    mln_string_t *ret = NULL, tmp;
    mln_size_t i = 0, len = text->len, sum;

    if (nlen-11 < text->len) return NULL;

    sum = (len / (nlen-11)) * nlen + (len % (nlen-11))? nlen: 0;
    if ((p = buf = (mln_u8ptr_t)malloc(sum)) == NULL) return NULL;

    while (len > 0) {
        mln_bignum_t num = mln_bignum_zero();
        i = len > (nlen-11)? nlen-11: len;

        mln_rsa_pri_padding(in, i, p, nlen);
        if (mln_bignum_os2ip(&num, p, nlen) < 0) {
            free(buf);
            return NULL;
        }
        if (mln_rsa_rsaep_rsadp(pri, &num, &num) < 0) {
            free(buf);
            return NULL;
        }
        if (mln_bignum_i2osp(&num, p, nlen) < 0) {
            free(buf);
            return NULL;
        }

        p += nlen;
        len -= i;
        in += i;
    }

    mln_string_nset(&tmp, buf, sum);
    ret = mln_string_dup(&tmp);
    free(buf);
    return ret;
})

MLN_FUNC(, mln_string_t *, mln_RSAESPKCS1V15_private_decrypt, \
         (mln_rsa_key_t *pri, mln_string_t *cipher), (pri, cipher), \
{
    mln_size_t nlen = mln_bignum_get_length(&(pri->n)) << 2;
    mln_u8ptr_t buf, p, pos;
    mln_u8ptr_t in = cipher->data;
    mln_string_t *ret = NULL, tmp;
    mln_size_t len = cipher->len, sum = 0;

    if (cipher->len < 11 || cipher->len % nlen) return NULL;

    buf = (mln_u8ptr_t)malloc(cipher->len);
    if (buf == NULL) return NULL;
    p = buf;

    while (len > 0) {
        mln_bignum_t num = mln_bignum_zero();

        if (mln_bignum_os2ip(&num, in, nlen) < 0) {
            free(buf);
            return NULL;
        }

        if (mln_rsa_rsaep_rsadp(pri, &num, &num) < 0) {
            free(buf);
            return NULL;
        }

        if (mln_bignum_i2osp(&num, in, nlen) < 0) {
            free(buf);
            return NULL;
        }

        pos = mln_rsa_anti_padding_public(in, nlen);
        if (pos == NULL) {
            free(buf);
            return NULL;
        }

        memcpy(p, pos, (nlen - (pos - in)));
        sum += (nlen - (pos - in));
        p += (nlen - (pos - in));
        in += nlen;
        len -= nlen;
    }

    mln_string_nset(&tmp, buf, sum);
    ret = mln_string_dup(&tmp);
    free(buf);
    return ret;
})

MLN_FUNC_VOID(, void, mln_RSAESPKCS1V15_free, (mln_string_t *s), (s), {
    if (s == NULL) return;
    mln_string_free(s);
})


/*
 * Sign & verify
 */
MLN_FUNC(static inline, mln_string_t *, mln_EMSAPKCS1V15_encode, \
         (mln_alloc_t *pool, mln_string_t *m, mln_u32_t hash_type), \
         (pool, m, hash_type), \
{
    mln_u8_t hashval[32] = {0};
    mln_u64_t hlen = 0;
    mln_asn1_enresult_t res, dres;
    mln_string_t *ret, tmp;

    switch (hash_type) {
        case M_EMSAPKCS1V15_HASH_MD5:
        {
            mln_md5_t md5;
            mln_md5_init(&md5);
            mln_md5_calc(&md5, m->data, m->len, 1);
            hlen = 16;
            mln_md5_tobytes(&md5, hashval, hlen);
            break;
        }
        case M_EMSAPKCS1V15_HASH_SHA1:
        {
            mln_sha1_t sha1;
            mln_sha1_init(&sha1);
            mln_sha1_calc(&sha1, m->data, m->len, 1);
            hlen = 20;
            mln_sha1_tobytes(&sha1, hashval, hlen);
            break;
        }
        case M_EMSAPKCS1V15_HASH_SHA256:
        {
            mln_sha256_t sha256;
            mln_sha256_init(&sha256);
            mln_sha256_calc(&sha256, m->data, m->len, 1);
            hlen = 32;
            mln_sha256_tobytes(&sha256, hashval, hlen);
            break;
        }
        default: return NULL;
    }

    if (mln_asn1_enresult_init(&res, pool) != M_ASN1_RET_OK) {
        return NULL;
    }
    if (mln_asn1_encode_object_identifier(&res, \
                                          EMSAPKCS1V15_HASH[hash_type].digest_algorithm, \
                                          EMSAPKCS1V15_HASH[hash_type].len) != M_ASN1_RET_OK)
    {
err:
        mln_asn1_enresult_destroy(&res);
        return NULL;
    }
    if (mln_asn1_encode_null(&res) != M_ASN1_RET_OK) {
        goto err;
    }
    if (mln_asn1_encode_sequence(&res) != M_ASN1_RET_OK) {
        goto err;
    }

    if (mln_asn1_enresult_init(&dres, pool) != M_ASN1_RET_OK) {
        goto err;
    }
    if (mln_asn1_encode_octetstring(&dres, hashval, hlen) != M_ASN1_RET_OK) {
        mln_asn1_enresult_destroy(&dres);
        goto err;
    }
    if (mln_asn1_encode_merge(&res, &dres) != M_ASN1_RET_OK) {
        mln_asn1_enresult_destroy(&dres);
        goto err;
    }
    mln_asn1_enresult_destroy(&dres);

    if (mln_asn1_encode_sequence(&res) != M_ASN1_RET_OK) {
        goto err;
    }

    if (mln_asn1_enresult_get_content(&res, 0, &(tmp.data), (mln_u64_t *)&(tmp.len)) != M_ASN1_RET_OK) {
        goto err;
    }

    if ((ret = mln_string_dup(&tmp)) == NULL) {
        goto err;
    }
    mln_asn1_enresult_destroy(&res);

    return ret;
})

MLN_FUNC(, mln_string_t *, mln_RSASSAPKCS1V15_sign, \
         (mln_alloc_t *pool, mln_rsa_key_t *pri, mln_string_t *m, mln_u32_t hash_type), \
         (pool, pri, m, hash_type), \
{
    mln_string_t *ret = NULL, tmp, *em;
    mln_u8ptr_t buf, p, q;
    mln_u64_t len = 4096, left, n, k;
    k = (mln_bignum_get_length(&(pri->n)) << 2) - 11;

    if (k < 1) return NULL;

    if ((em = mln_EMSAPKCS1V15_encode(pool, m, hash_type)) == NULL) return NULL;

    if ((buf = (mln_u8ptr_t)malloc(len)) == NULL) {
err:
        mln_string_free(em);
        return NULL;
    }
    p = buf;
    q = em->data;
    left = em->len;

    while (left > 0) {
        n = left>k? k: left; /*30 see rfc2312 - page 8 - Notes 1.*/
        left -= n;
        mln_string_nset(&tmp, q, n);
        q += n;
        if ((ret = mln_RSAESPKCS1V15_private_encrypt(pri, &tmp)) == NULL) {
            free(buf);
            goto err;
        }
        if (len-(p-buf) < ret->len) {
            len += (len >> 1);
            mln_u64_t diff = p - buf;
            mln_u8ptr_t ptr = (mln_u8ptr_t)realloc(buf, len);
            if (ptr == NULL) {
                free(buf);
                goto err;
            }
            buf = ptr;
            p = ptr + diff;
        }
        memcpy(p, ret->data, ret->len);
        p += ret->len;
        mln_string_free(ret);
    }
    mln_string_free(em);
    mln_string_nset(&tmp, buf, p-buf);
    if ((ret = mln_string_dup(&tmp)) == NULL) {
        free(buf);
        goto err;
    }
    free(buf);
    return ret;
})

MLN_FUNC(, int, mln_RSASSAPKCS1V15_verify, \
         (mln_alloc_t *pool, mln_rsa_key_t *pub, mln_string_t *m, mln_string_t *s), \
         (pool, pub, m, s), \
{
    mln_string_t tmp, *ret;
    mln_u8ptr_t buf, p, q, end;
    mln_u64_t len = 4096;
    mln_size_t n = mln_bignum_get_length(&(pub->n)) << 2;
    mln_u8_t hashval[32] = {0};
    mln_size_t hlen;
    mln_u32_t hash_type = 0;

    if (s->len % n) {
        return -1;
    }

    if ((buf = (mln_u8ptr_t)malloc(len)) == NULL) {
        return -1;
    }

    p = buf;
    q = s->data;
    end = s->data + s->len;

    while (q < end) {
        mln_string_nset(&tmp, q, n);
        if ((ret = mln_RSAESPKCS1V15_public_decrypt(pub, &tmp)) == NULL) {
            free(buf);
            return -1;
        }

        if (len-(p-buf) < ret->len) {
            mln_u8ptr_t ptr;
            mln_u64_t diff = p - buf;
            len += (len >> 1);
            if ((ptr = (mln_u8ptr_t)realloc(buf, len)) == NULL) {
                mln_string_free(ret);
                free(buf);
                return -1;
            }
            buf = ptr;
            p = ptr + diff;
        }
        memcpy(p, ret->data, ret->len);
        p += ret->len;
        q += n;
        mln_string_free(ret);
    }
    mln_string_nset(&tmp, buf, p - buf);

    if ((ret = mln_EMSAPKCS1V15_decode(pool, &tmp, &hash_type)) == NULL) {
        free(buf);
        return -1;
    }
    free(buf);

    switch (hash_type) {
        case M_EMSAPKCS1V15_HASH_MD5:
        {
            mln_md5_t md5;
            mln_md5_init(&md5);
            mln_md5_calc(&md5, m->data, m->len, 1);
            hlen = 16;
            mln_md5_tobytes(&md5, hashval, hlen);
            break;
        }
        case M_EMSAPKCS1V15_HASH_SHA1:
        {
            mln_sha1_t sha1;
            mln_sha1_init(&sha1);
            mln_sha1_calc(&sha1, m->data, m->len, 1);
            hlen = 20;
            mln_sha1_tobytes(&sha1, hashval, hlen);
            break;
        }
        case M_EMSAPKCS1V15_HASH_SHA256:
        {
            mln_sha256_t sha256;
            mln_sha256_init(&sha256);
            mln_sha256_calc(&sha256, m->data, m->len, 1);
            hlen = 32;
            mln_sha256_tobytes(&sha256, hashval, hlen);
            break;
        }
        default:
            mln_string_free(ret);
            return -1;
    }

    if (ret->len != hlen || memcmp(ret->data, hashval, hlen)) {
        mln_string_free(ret);
        return -1;
    }
    mln_string_free(ret);
    return 0;
})

MLN_FUNC(static inline, mln_string_t *, mln_EMSAPKCS1V15_decode, \
         (mln_alloc_t *pool, mln_string_t *e, mln_u32_t *hash_type), \
         (pool, e, hash_type), \
{
    mln_asn1_deresult_t *res, *sub_res, *ssub_res;
    int err = M_ASN1_RET_OK;
    mln_string_t *ret, tmp, t;
    mln_u8ptr_t code_buf;
    mln_u64_t code_len;
    struct mln_EMSAPKCS1V15_HASH_s *p, *end;

    if ((res = mln_asn1_decode_ref(e->data, e->len, &err, pool)) == NULL) {
        return NULL;
    }

    if (mln_asn1_deresult_ident_get(res) != M_ASN1_ID_SEQUENCE || \
        mln_asn1_deresult_content_num(res) != 2)
    {
err:
        mln_asn1_deresult_free(res);
        return NULL;
    }


    sub_res = mln_asn1_deresult_content_get(res, 0);
    if (mln_asn1_deresult_ident_get(sub_res) != M_ASN1_ID_SEQUENCE || \
        mln_asn1_deresult_content_num(sub_res) != 2)
    {
        goto err;
    }
    ssub_res = mln_asn1_deresult_content_get(sub_res, 0);
    if (mln_asn1_deresult_ident_get(ssub_res) != M_ASN1_ID_OBJECT_IDENTIFIER) {
        goto err;
    }
    if ((ssub_res = mln_asn1_deresult_content_get(ssub_res, 0)) == NULL) {
        goto err;
    }
    code_buf = mln_asn1_deresult_code_get(ssub_res);
    code_len = mln_asn1_deresult_code_length_get(ssub_res);
    mln_string_nset(&tmp, code_buf, code_len);
    p = EMSAPKCS1V15_HASH;
    end = EMSAPKCS1V15_HASH + sizeof(EMSAPKCS1V15_HASH)/sizeof(struct mln_EMSAPKCS1V15_HASH_s);
    for (; p < end; ++p) {
        mln_string_nset(&t, p->digest_algorithm, p->len);
        if (!mln_string_strcmp(&tmp, &t)) break;
    }
    if (p >= end) goto err;
    *hash_type = p - EMSAPKCS1V15_HASH;

    ssub_res = mln_asn1_deresult_content_get(sub_res, 1);
    if (mln_asn1_deresult_ident_get(ssub_res) != M_ASN1_ID_NULL || \
        mln_asn1_deresult_content_num(ssub_res) != 1)
    {
        goto err;
    }
    ssub_res = mln_asn1_deresult_content_get(ssub_res, 0);
    if (mln_asn1_deresult_code_length_get(ssub_res) != 0) {
        goto err;
    }
    

    sub_res = mln_asn1_deresult_content_get(res, 1);
    if (mln_asn1_deresult_ident_get(sub_res) != M_ASN1_ID_OCTET_STRING || \
        mln_asn1_deresult_content_num(sub_res) != 1)
    {
        goto err;
    }
    sub_res = mln_asn1_deresult_content_get(sub_res, 0);
    mln_string_nset(&tmp, mln_asn1_deresult_code_get(sub_res), mln_asn1_deresult_code_length_get(sub_res));
    ret = mln_string_dup(&tmp);
    mln_asn1_deresult_free(res);

    return ret;
})

