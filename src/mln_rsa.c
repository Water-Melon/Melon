
/*
 * Copyright (C) Niklaus F.Schen.
 */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include "mln_rsa.h"
#include "mln_chain.h"
#include "mln_md5.h"
#include "mln_sha.h"

static inline int mln_rsa_rsaep_rsadp(mln_rsa_key_t *key, mln_bignum_t *in, mln_bignum_t *out);
static inline void mln_rsa_pub_padding(mln_u8ptr_t in, mln_size_t inlen, mln_u8ptr_t out, mln_size_t keylen);
static inline void mln_rsa_pri_padding(mln_u8ptr_t in, mln_size_t inlen, mln_u8ptr_t out, mln_size_t keylen);
static inline mln_u8ptr_t mln_rsa_antiPaddingPublic(mln_u8ptr_t in, mln_size_t len);
static inline mln_u8ptr_t mln_rsa_antiPaddingPrivate(mln_u8ptr_t in, mln_size_t len);
static inline mln_string_t *mln_EMSAPKCS1V15_encode(mln_string_t *m, mln_size_t emlen, mln_u32_t hashType);
static inline mln_string_t *mln_octet_string_encode(mln_u8ptr_t hash, mln_u64_t hlen);

static mln_u8_t EMSAPKCS1V15_HASH_MD5[] = \
{0x30, 0x20, 0x30, 0xc, 0x6, 0x8, 0x2a, 0x86, 0x48, 0x86, 0xf7, 0xd, 0x2, 0x5, 0x5, 0x0, 0x4, 0x10};
static mln_u8_t EMSAPKCS1V15_HASH_SHA1[] = \
{0x30, 0x21, 0x30, 0x9, 0x6, 0x5, 0x2b, 0xe, 0x3, 0x2, 0x1a, 0x5, 0x0, 0x4, 0x14};
static mln_u8_t EMSAPKCS1V15_HASH_SHA256[] = \
{0x30, 0x31, 0x30, 0xd, 0x6, 0x9, 0x60, 0x86, 0x48, 0x1, 0x65, 0x3, 0x4, 0x2, 0x1, 0x5, 0x0, 0x4, 0x20};
struct mln_EMSAPKCS1V15_HASH_s {
    mln_u8ptr_t digestAlgorithm;
    mln_size_t  len;
} EMSAPKCS1V15_HASH[] = {
    {EMSAPKCS1V15_HASH_MD5, sizeof(EMSAPKCS1V15_HASH_MD5)},
    {EMSAPKCS1V15_HASH_SHA1, sizeof(EMSAPKCS1V15_HASH_SHA1)},
    {EMSAPKCS1V15_HASH_SHA256, sizeof(EMSAPKCS1V15_HASH_SHA256)}
};

mln_rsa_key_t *mln_rsa_key_new(void)
{
    return (mln_rsa_key_t *)malloc(sizeof(mln_rsa_key_t));
}

mln_rsa_key_t *mln_rsa_key_pool_new(mln_alloc_t *pool)
{
    return (mln_rsa_key_t *)mln_alloc_m(pool, sizeof(mln_rsa_key_t));
}

void mln_rsa_key_free(mln_rsa_key_t *key)
{
    if (key == NULL) return;
    free(key);
}

void mln_rsa_key_pool_free(mln_rsa_key_t *key)
{
    if (key == NULL) return;
    mln_alloc_free(key);
}

int mln_rsa_keyGenerate(mln_rsa_key_t *pub, mln_rsa_key_t *pri, mln_u32_t bits)
{
    if (bits <= 88 || bits > 2048) return -1;

    mln_bignum_t p, q, n, phiN, one, d;
    mln_bignum_assign(&one, "1", 1);
    mln_bignum_t e, tmpp_1, tmpq_1;

lp:
    while (1) {
        if (mln_bignum_prime(&p, (bits>>1)-1) < 0) return -1;
        if (mln_bignum_prime(&q, (bits>>1)-1) < 0) return -1;;
        if (mln_bignum_compare(&p, &q)) break;
    }
    n = p;

    mln_bignum_mul(&n, &q);

    if (pub != NULL) {
        pub->p = p;
        pub->q = q;
        if (mln_bignum_extendEulid(&q, &p, &(pub->qinv), NULL, NULL) < 0) goto lp;
        if (mln_bignum_isNegative(&(pub->qinv))) goto lp;
    }
    if (pri != NULL) {
        pri->p = p;
        pri->q = q;
        if (mln_bignum_extendEulid(&q, &p, &(pri->qinv), NULL, NULL) < 0) goto lp;
        if (mln_bignum_isNegative(&(pri->qinv))) goto lp;
    }
    mln_bignum_sub(&p, &one);
    mln_bignum_sub(&q, &one);
    tmpp_1 = p;
    tmpq_1 = q;
    phiN = p;
    mln_bignum_mul(&phiN, &q);

    mln_bignum_assign(&e, "0x10001", 7);

    if (mln_bignum_extendEulid(&e, &phiN, &d, NULL, NULL) < 0) goto lp;
    if (mln_bignum_compare(&d, &one) <= 0) goto lp;

    if (pub != NULL) {
        pub->n = n;
        pub->ed = e;
        pub->dp = pub->dq = e;
        mln_bignum_div(&(pub->dp), &tmpp_1, NULL);
        mln_bignum_div(&(pub->dq), &tmpq_1, NULL);
    }
    if (pri != NULL) {
        pri->n = n;
        pri->ed = d;
        pri->dp = pri->dq = d;
        mln_bignum_div(&(pri->dp), &tmpp_1, NULL);
        mln_bignum_div(&(pri->dq), &tmpq_1, NULL);
    }

    return 0;
}

static inline int mln_rsa_rsaep_rsadp(mln_rsa_key_t *key, mln_bignum_t *in, mln_bignum_t *out)
{
    if (mln_bignum_absCompare(&(key->ed), &(key->p)) <= 0) {
in:
        if (mln_bignum_pwr(in, &(key->ed), &(key->n)) < 0) return -1;
        *out = *in;
    } else {
        mln_bignum_t m1, m2;
        m1 = m2 = *in;
        mln_bignum_pwr(&m1, &(key->dp), &(key->p));
        mln_bignum_pwr(&m2, &(key->dq), &(key->q));
        mln_bignum_sub(&m1, &m2);
        if (mln_bignum_isNegative(&m1)) goto in;
        mln_bignum_mul(&m1, &(key->qinv));
        mln_bignum_div(&m1, &(key->p), NULL);
        mln_bignum_mul(&m1, &(key->q));
        mln_bignum_add(&m1, &m2);
        mln_bignum_div(&m1, &(key->n), NULL);
        *out = m1;
    }
    return 0;
}

static inline void mln_rsa_pub_padding(mln_u8ptr_t in, mln_size_t inlen, mln_u8ptr_t out, mln_size_t keylen)
{
    mln_size_t j;
    struct timeval tv;
    mln_u32_t val;

    *out++ = 0;
    *out++ = 0x2;
    gettimeofday(&tv, NULL);
    val = tv.tv_sec*1000000+tv.tv_usec;
    for (j = keylen - inlen - 3; j > 0; j--) {
lp:
        val = *out = rand_r(&val);
        if (val == 0) {
            gettimeofday(&tv, NULL);
            val = tv.tv_sec*1000000+tv.tv_usec;
            goto lp;
        }
        out++;
    }
    *out++ = 0;
    memcpy(out, in, inlen);
}

static inline void mln_rsa_pri_padding(mln_u8ptr_t in, mln_size_t inlen, mln_u8ptr_t out, mln_size_t keylen)
{
    mln_size_t j;

    *out++ = 0;
    *out++ = 0x1;
    for (j = keylen - inlen - 3; j > 0 ; j--) {
        *out++ = 0xff;
    }
    *out++ = 0;
    memcpy(out, in, inlen);
}

static inline mln_u8ptr_t mln_rsa_antiPaddingPublic(mln_u8ptr_t in, mln_size_t len)
{
    if (in == NULL || len == 0 || *in != 0) return NULL;
    mln_u8ptr_t p = in + 2, end = in + len;
    if (*(in+1) != 0x2) return NULL;
    for (; *p != 0 && p < end; p++)
        ;
    if (p >= end || p+1 >= end) return NULL;
    if (*p++ != 0) return NULL;
    return p;
}

static inline mln_u8ptr_t mln_rsa_antiPaddingPrivate(mln_u8ptr_t in, mln_size_t len)
{
    if (in == NULL || len == 0 || *in != 0) return NULL;
    mln_u8ptr_t p = in + 2, end = in + len;
    if (*(in+1) == 0) {
        for (; *p == 0 && p < end; p++)
            ;
        if (p >= end) return NULL;
    } else if (*(in+1) == 1) {
        for (; *p == 0xff && p < end; p++)
            ;
        if (p >= end || p+1 >= end) return NULL;
        if (*p++ != 0) return NULL;
    } else {
        return NULL;
    }
    return p;
}

mln_string_t *mln_RSAESPKCS1V15PubEncrypt(mln_rsa_key_t *pub, mln_string_t *text)
{
    mln_size_t nlen = mln_bignum_getLength(&(pub->n)) << 2;
    mln_u8ptr_t buf, p;
    mln_u8ptr_t in = (mln_u8ptr_t)(text->str);
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

    mln_string_nSet(&tmp, buf, sum);
    ret = mln_string_dup(&tmp);
    free(buf);
    return ret;
}

mln_string_t *mln_RSAESPKCS1V15PubDecrypt(mln_rsa_key_t *pub, mln_string_t *cipher)
{
    mln_size_t nlen = mln_bignum_getLength(&(pub->n)) << 2;
    mln_u8ptr_t buf, p, pos;
    mln_u8ptr_t in = (mln_u8ptr_t)(cipher->str);
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

        if (mln_rsa_rsaep_rsadp(pub, &num, &num) < 0) {
            free(buf);
            return NULL;
        }

        if (mln_bignum_i2osp(&num, in, nlen) < 0) {
            free(buf);
            return NULL;
        }

        pos = mln_rsa_antiPaddingPrivate(in, nlen);
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

    mln_string_nSet(&tmp, buf, sum);
    ret = mln_string_dup(&tmp);
    free(buf);
    return ret;
}

mln_string_t *mln_RSAESPKCS1V15PriEncrypt(mln_rsa_key_t *pri, mln_string_t *text)
{
    mln_size_t nlen = mln_bignum_getLength(&(pri->n)) << 2;
    mln_u8ptr_t buf, p;
    mln_u8ptr_t in = (mln_u8ptr_t)(text->str);
    mln_string_t *ret = NULL, tmp;
    mln_size_t i = 0, len = text->len, sum;

    if (nlen-11 < text->len) return NULL;

    sum = (len / (nlen-11)) * nlen + (len % (nlen-11))? nlen: 0;
    buf = (mln_u8ptr_t)malloc(sum);
    if (buf == NULL) return NULL;
    p = buf;

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

    mln_string_nSet(&tmp, buf, sum);
    ret = mln_string_dup(&tmp);
    free(buf);
    return ret;
}

mln_string_t *mln_RSAESPKCS1V15PriDecrypt(mln_rsa_key_t *pri, mln_string_t *cipher)
{
    mln_size_t nlen = mln_bignum_getLength(&(pri->n)) << 2;
    mln_u8ptr_t buf, p, pos;
    mln_u8ptr_t in = (mln_u8ptr_t)(cipher->str);
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

        pos = mln_rsa_antiPaddingPublic(in, nlen);
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

    mln_string_nSet(&tmp, buf, sum);
    ret = mln_string_dup(&tmp);
    free(buf);
    return ret;
}

void mln_RSAESPKCS1V15Free(mln_string_t *s)
{
    if (s == NULL) return;
    mln_string_free(s);
}


/*
 * Sign & verify
 */
static inline mln_string_t *mln_EMSAPKCS1V15_encode(mln_string_t *m, mln_size_t emlen, mln_u32_t hashType)
{
    mln_u8ptr_t hashval = NULL, em;
    mln_size_t hlen = 0;
    mln_string_t *ret, tmp, *osh;

    /*hash*/
    switch (hashType) {
        case M_EMSAPKCS1V15_HASH_MD5:
        {
            mln_md5_t md5;
            mln_md5_init(&md5);
            mln_md5_calc(&md5, (mln_u8ptr_t)(m->str), m->len, 1);
            hashval = (mln_u8ptr_t)malloc(16);
            if (hashval == NULL) return NULL;
            hlen = 16;
            mln_md5_toBytes(&md5, hashval, hlen);
            break;
        }
        case M_EMSAPKCS1V15_HASH_SHA1:
        {
            mln_sha1_t sha1;
            mln_sha1_init(&sha1);
            mln_sha1_calc(&sha1, (mln_u8ptr_t)(m->str), m->len, 1);
            hashval = (mln_u8ptr_t)malloc(20);
            if (hashval == NULL) return NULL;
            hlen = 20;
            mln_sha1_toBytes(&sha1, hashval, hlen);
            break;
        }
        case M_EMSAPKCS1V15_HASH_SHA256:
        {
            mln_sha256_t sha256;
            mln_sha256_init(&sha256);
            mln_sha256_calc(&sha256, (mln_u8ptr_t)(m->str), m->len, 1);
            hashval = (mln_u8ptr_t)malloc(32);
            if (hashval == NULL) return NULL;
            hlen = 32;
            mln_sha256_toBytes(&sha256, hashval, hlen);
            break;
        }
        default: return NULL;
    }

    /*asn.1*/
    osh = mln_octet_string_encode(hashval, hlen);
    free(hashval);
    if (osh == NULL) return NULL;
    hashval = (mln_u8ptr_t)malloc(osh->len + EMSAPKCS1V15_HASH[hashType].len);
    if (hashval == NULL) {
        mln_string_free(osh);
        return NULL;
    }
    memcpy(hashval, EMSAPKCS1V15_HASH[hashType].digestAlgorithm, EMSAPKCS1V15_HASH[hashType].len);
    memcpy(hashval+EMSAPKCS1V15_HASH[hashType].len, osh->str, osh->len);

    /*test len*/
    if (emlen < osh->len+EMSAPKCS1V15_HASH[hashType].len+11) {
        mln_string_free(osh);
        free(hashval);
        return NULL;
    }

    /*padding*/
    em = (mln_u8ptr_t)malloc(emlen);
    if (em == NULL) {
        mln_string_free(osh);
        free(hashval);
        return NULL;
    }
    mln_rsa_pri_padding(hashval, osh->len+EMSAPKCS1V15_HASH[hashType].len, em, emlen);
    mln_string_free(osh);
    free(hashval);
    mln_string_nSet(&tmp, em, emlen);
    ret = mln_string_dup(&tmp);
    free(em);
    return ret;
}

static inline mln_string_t *mln_octet_string_encode(mln_u8ptr_t hash, mln_u64_t hlen)
{
    mln_string_t tmp, *ret;
    mln_u8ptr_t buf = NULL;
    mln_size_t len = 0;

    if (hlen <= 127) {
        len = 2 + hlen;
        buf = (mln_u8ptr_t)malloc(len);
        if (buf == NULL) return NULL;
        buf[0] = 0x4;
        buf[1] = hlen;
        memcpy(&buf[2], hash, hlen);
    } else if (hlen <= 255) {
        len = 3 + hlen;
        buf = (mln_u8ptr_t)malloc(len);
        if (buf == NULL) return NULL;
        buf[0] = 0x4;
        buf[1] = 0x81;
        buf[2] = hlen;
        memcpy(&buf[3], hash, hlen);
    } else if (hlen <= 65535) {
        len = 4 + hlen;
        buf = (mln_u8ptr_t)malloc(len);
        if (buf == NULL) return NULL;
        buf[0] = 0x4;
        buf[1] = 0x82;
        buf[2] = (hlen >> 8) & 0xff;
        buf[3] = hlen & 0xff;
        memcpy(&buf[4], hash, hlen);
    } else if (hlen <= ~((mln_u32_t)0)) {
        len = 6 + hlen;
        buf = (mln_u8ptr_t)malloc(len);
        if (buf == NULL) return NULL;
        buf[0] = 0x4;
        buf[1] = 0x84;
        buf[2] = (hlen >> 24) & 0xff;
        buf[3] = (hlen >> 16) & 0xff;
        buf[4] = (hlen >> 8) & 0xff;
        buf[5] = hlen & 0xff;
        memcpy(&buf[6], hash, hlen);
    } else {
        len = 10 + hlen;
        buf = (mln_u8ptr_t)malloc(len);
        if (buf == NULL) return NULL;
        buf[0] = 0x4;
        buf[1] = 0x88;
        buf[2] = (hlen >> 56) & 0xff;
        buf[3] = (hlen >> 48) & 0xff;
        buf[4] = (hlen >> 40) & 0xff;
        buf[5] = (hlen >> 32) & 0xff;
        buf[6] = (hlen >> 24) & 0xff;
        buf[7] = (hlen >> 16) & 0xff;
        buf[8] = (hlen >> 8) & 0xff;
        buf[9] = hlen & 0xff;
        memcpy(&buf[10], hash, hlen);
    }

    mln_string_nSet(&tmp, buf, len);
    ret = mln_string_dup(&tmp);
    free(buf);
    return ret;
}

mln_string_t *mln_RSASSAPKCS1V15SIGN(mln_rsa_key_t *pri, mln_string_t *m, mln_u32_t hashType)
{
    mln_string_t *em = mln_EMSAPKCS1V15_encode(m, (mln_bignum_getLength(&(pri->n))<<2), hashType);
    if (em == NULL) return NULL;

    if (em->len > (M_BIGNUM_SIZE<<2)) {
        mln_string_free(em);
        return NULL;
    }

    mln_bignum_t num;
    if (mln_bignum_os2ip(&num, (mln_u8ptr_t)(em->str), em->len) < 0) {
        mln_string_free(em);
        return NULL;
    }
    mln_string_free(em);

    if (mln_rsa_rsaep_rsadp(pri, &num, &num) < 0) {
        return NULL;
    }

    mln_u8ptr_t buf = (mln_u8ptr_t)malloc(mln_bignum_getLength(&(pri->n))<<2);
    if (buf == NULL) return NULL;
    if (mln_bignum_i2osp(&num, buf, mln_bignum_getLength(&(pri->n))<<2) < 0) {
        free(buf);
        return NULL;
    }
    mln_string_t tmp;
    mln_string_nSet(&tmp, buf, mln_bignum_getLength(&(pri->n))<<2);
    mln_string_t *ret = mln_string_dup(&tmp);
    free(buf);
    return ret;
}

int mln_RSASSAPKCS1V15VERIFY(mln_rsa_key_t *pub, mln_string_t *m, mln_string_t *s, mln_u32_t hashType)
{
    if (s->len != (mln_bignum_getLength(&(pub->n)) << 2)) return -1;

    mln_bignum_t num;
    if (mln_bignum_os2ip(&num, (mln_u8ptr_t)(s->str), s->len) < 0) {
        return -1;
    }

    if (mln_rsa_rsaep_rsadp(pub, &num, &num) < 0) {
        return -1;
    }

    mln_u8ptr_t buf = (mln_u8ptr_t)malloc(mln_bignum_getLength(&(pub->n)) << 2);
    if (buf == NULL) return -1;
    if (mln_bignum_i2osp(&num, buf, mln_bignum_getLength(&(pub->n)) << 2) < 0) {
        free(buf);
        return -1;
    }
    mln_string_t tmp;
    mln_string_nSet(&tmp, buf, mln_bignum_getLength(&(pub->n)) << 2);
    mln_string_t *_em = mln_string_dup(&tmp);
    free(buf);
    if (_em == NULL) return -1;

    mln_string_t *em = mln_EMSAPKCS1V15_encode(m, mln_bignum_getLength(&(pub->n))<<2, hashType);
    if (em == NULL) {
        mln_string_free(_em);
        return -1;
    }

    int ret = mln_string_strcmp(em, _em);
    mln_string_free(em);
    mln_string_free(_em);
    return ret==0? 0: -1;
}

