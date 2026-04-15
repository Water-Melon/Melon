
/*
 * Copyright (C) Niklaus F.Schen.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mln_sha.h"
#include "mln_func.h"

/*
 * sha1
 */
#define __M_SHA1_ROTATE_LEFT(x,n) (((x) << (n)) | (((x)&0xffffffff) >> (32-(n))))
#define __M_SHA1_F1(b,c,d) (((b)&(c))|((~(b))&(d)))
#define __M_SHA1_F2(b,c,d) ((b)^(c)^(d))
#define __M_SHA1_F3(b,c,d) (((b)&(c))|((d) & ((b)|(c))))
#define __M_SHA1_F4(b,c,d) ((b)^(c)^(d))
#define __M_SHA1_FF1(a,b,c,d,e,wt,kt); \
{\
    (e) += (__M_SHA1_ROTATE_LEFT((a),5)+__M_SHA1_F1((b),(c),(d))+(kt)+(wt));\
    (b) = __M_SHA1_ROTATE_LEFT((b), 30);\
}
#define __M_SHA1_FF2(a,b,c,d,e,wt,kt); \
{\
    (e) += (__M_SHA1_ROTATE_LEFT((a),5)+__M_SHA1_F2((b),(c),(d))+(kt)+(wt));\
    (b) = __M_SHA1_ROTATE_LEFT((b), 30);\
}
#define __M_SHA1_FF3(a,b,c,d,e,wt,kt); \
{\
    (e) += (__M_SHA1_ROTATE_LEFT((a),5)+__M_SHA1_F3((b),(c),(d))+(kt)+(wt));\
    (b) = __M_SHA1_ROTATE_LEFT((b), 30);\
}
#define __M_SHA1_FF4(a,b,c,d,e,wt,kt); \
{\
    (e) += (__M_SHA1_ROTATE_LEFT((a),5)+__M_SHA1_F4((b),(c),(d))+(kt)+(wt));\
    (b) = __M_SHA1_ROTATE_LEFT((b), 30);\
}
#define __M_SHA1_W(t,wt) \
((wt)[(t)&0xf] = __M_SHA1_ROTATE_LEFT((wt)[((t)-3)&0xf]^(wt)[((t)-8)&0xf]^(wt)[((t)-14)&0xf]^(wt)[(t)&0xf], 1))

#define __M_SHA_LOAD32BE(w, p) \
    ((w) = ((mln_u32_t)(p)[0] << 24) | ((mln_u32_t)(p)[1] << 16) | ((mln_u32_t)(p)[2] << 8) | (mln_u32_t)(p)[3])

static inline void mln_sha1_calc_block(mln_sha1_t *s);


MLN_FUNC(static inline, mln_s8_t, mln_sha_hex_tostring, (mln_u8_t c), (c), {
    return c < 10? ('0' + c): ('a' + (c - 10));
})

MLN_FUNC_VOID(, void, mln_sha1_init, (mln_sha1_t *s), (s), {
    s->H0 = 0x67452301;
    s->H1 = 0xefcdab89;
    s->H2 = 0x98badcfe;
    s->H3 = 0x10325476;
    s->H4 = 0xc3d2e1f0;
    s->length = 0;
    s->pos = 0;
})

MLN_FUNC(, mln_sha1_t *, mln_sha1_new, (void), (), {
    mln_sha1_t *s = (mln_sha1_t *)malloc(sizeof(mln_sha1_t));
    if (s == NULL) return NULL;
    mln_sha1_init(s);
    return s;
})

MLN_FUNC(, mln_sha1_t *, mln_sha1_pool_new, (mln_alloc_t *pool), (pool), {
    mln_sha1_t *s = (mln_sha1_t *)mln_alloc_m(pool, sizeof(mln_sha1_t));
    if (s == NULL) return NULL;
    mln_sha1_init(s);
    return s;
})

MLN_FUNC_VOID(, void, mln_sha1_free, (mln_sha1_t *s), (s), {
    if (s == NULL) return;
    free(s);
})

MLN_FUNC_VOID(, void, mln_sha1_pool_free, (mln_sha1_t *s), (s), {
    if (s == NULL) return;
    mln_alloc_free(s);
})

MLN_FUNC_VOID(, void, mln_sha1_calc, \
              (mln_sha1_t *s, mln_u8ptr_t input, mln_uauto_t len, mln_u32_t is_last), \
              (s, input, len, is_last), \
{
    mln_uauto_t size;

    s->length += len;
    while (len + s->pos > __M_SHA_BUFLEN) {
        size = __M_SHA_BUFLEN - s->pos;
        memcpy(&(s->buf[s->pos]), input, size);
        len -= size;
        input += size;
        mln_sha1_calc_block(s);
        s->pos = 0;
    }

    if (len > 0) {
        memcpy(&(s->buf[s->pos]), input, len);
        s->pos += len;
    }
    if (is_last) {
        if (s->pos < 56) {
            memset(&(s->buf[s->pos]), 0, 56-s->pos);
            s->buf[s->pos] = 1 << 7;
        } else {
            if (s->pos < __M_SHA_BUFLEN) {
                memset(&(s->buf[s->pos]), 0, __M_SHA_BUFLEN-s->pos);
                s->buf[s->pos] = 1 << 7;
                mln_sha1_calc_block(s);
                s->pos = 0;
                memset(s->buf, 0, 56);
            } else {
                mln_sha1_calc_block(s);
                s->pos = 0;
                memset(s->buf, 0, 56);
                s->buf[s->pos] = 1 << 7;
            }
        }
        s->length <<= 3;
        s->buf[56] = (s->length >> 56) & 0xff;
        s->buf[57] = (s->length >> 48) & 0xff;
        s->buf[58] = (s->length >> 40) & 0xff;
        s->buf[59] = (s->length >> 32) & 0xff;
        s->buf[60] = (s->length >> 24) & 0xff;
        s->buf[61] = (s->length >> 16) & 0xff;
        s->buf[62] = (s->length >> 8) & 0xff;
        s->buf[63] = (s->length) & 0xff;
        mln_sha1_calc_block(s);
        s->pos = 0;
    }
})

MLN_FUNC_VOID(static inline, void, mln_sha1_calc_block, (mln_sha1_t *s), (s), {
    mln_u32_t group[16];
    mln_u32_t a = s->H0, b = s->H1, c = s->H2, d = s->H3, e = s->H4;
    const mln_u8_t *p = s->buf;

    __M_SHA_LOAD32BE(group[0],  p);
    __M_SHA_LOAD32BE(group[1],  p + 4);
    __M_SHA_LOAD32BE(group[2],  p + 8);
    __M_SHA_LOAD32BE(group[3],  p + 12);
    __M_SHA_LOAD32BE(group[4],  p + 16);
    __M_SHA_LOAD32BE(group[5],  p + 20);
    __M_SHA_LOAD32BE(group[6],  p + 24);
    __M_SHA_LOAD32BE(group[7],  p + 28);
    __M_SHA_LOAD32BE(group[8],  p + 32);
    __M_SHA_LOAD32BE(group[9],  p + 36);
    __M_SHA_LOAD32BE(group[10], p + 40);
    __M_SHA_LOAD32BE(group[11], p + 44);
    __M_SHA_LOAD32BE(group[12], p + 48);
    __M_SHA_LOAD32BE(group[13], p + 52);
    __M_SHA_LOAD32BE(group[14], p + 56);
    __M_SHA_LOAD32BE(group[15], p + 60);

    __M_SHA1_FF1(a, b, c, d, e, group[0], 0x5A827999);
    __M_SHA1_FF1(e, a, b, c, d, group[1], 0x5A827999);
    __M_SHA1_FF1(d, e, a, b, c, group[2], 0x5A827999);
    __M_SHA1_FF1(c, d, e, a, b, group[3], 0x5A827999);
    __M_SHA1_FF1(b, c, d, e, a, group[4], 0x5A827999);
    __M_SHA1_FF1(a, b, c, d, e, group[5], 0x5A827999);
    __M_SHA1_FF1(e, a, b, c, d, group[6], 0x5A827999);
    __M_SHA1_FF1(d, e, a, b, c, group[7], 0x5A827999);
    __M_SHA1_FF1(c, d, e, a, b, group[8], 0x5A827999);
    __M_SHA1_FF1(b, c, d, e, a, group[9], 0x5A827999);
    __M_SHA1_FF1(a, b, c, d, e, group[10], 0x5A827999);
    __M_SHA1_FF1(e, a, b, c, d, group[11], 0x5A827999);
    __M_SHA1_FF1(d, e, a, b, c, group[12], 0x5A827999);
    __M_SHA1_FF1(c, d, e, a, b, group[13], 0x5A827999);
    __M_SHA1_FF1(b, c, d, e, a, group[14], 0x5A827999);
    __M_SHA1_FF1(a, b, c, d, e, group[15], 0x5A827999);
    __M_SHA1_FF1(e, a, b, c, d, __M_SHA1_W(16, group), 0x5A827999);
    __M_SHA1_FF1(d, e, a, b, c, __M_SHA1_W(17, group), 0x5A827999);
    __M_SHA1_FF1(c, d, e, a, b, __M_SHA1_W(18, group), 0x5A827999);
    __M_SHA1_FF1(b, c, d, e, a, __M_SHA1_W(19, group), 0x5A827999);

    __M_SHA1_FF2(a, b, c, d, e, __M_SHA1_W(20, group), 0x6ED9EBA1);
    __M_SHA1_FF2(e, a, b, c, d, __M_SHA1_W(21, group), 0x6ED9EBA1);
    __M_SHA1_FF2(d, e, a, b, c, __M_SHA1_W(22, group), 0x6ED9EBA1);
    __M_SHA1_FF2(c, d, e, a, b, __M_SHA1_W(23, group), 0x6ED9EBA1);
    __M_SHA1_FF2(b, c, d, e, a, __M_SHA1_W(24, group), 0x6ED9EBA1);
    __M_SHA1_FF2(a, b, c, d, e, __M_SHA1_W(25, group), 0x6ED9EBA1);
    __M_SHA1_FF2(e, a, b, c, d, __M_SHA1_W(26, group), 0x6ED9EBA1);
    __M_SHA1_FF2(d, e, a, b, c, __M_SHA1_W(27, group), 0x6ED9EBA1);
    __M_SHA1_FF2(c, d, e, a, b, __M_SHA1_W(28, group), 0x6ED9EBA1);
    __M_SHA1_FF2(b, c, d, e, a, __M_SHA1_W(29, group), 0x6ED9EBA1);
    __M_SHA1_FF2(a, b, c, d, e, __M_SHA1_W(30, group), 0x6ED9EBA1);
    __M_SHA1_FF2(e, a, b, c, d, __M_SHA1_W(31, group), 0x6ED9EBA1);
    __M_SHA1_FF2(d, e, a, b, c, __M_SHA1_W(32, group), 0x6ED9EBA1);
    __M_SHA1_FF2(c, d, e, a, b, __M_SHA1_W(33, group), 0x6ED9EBA1);
    __M_SHA1_FF2(b, c, d, e, a, __M_SHA1_W(34, group), 0x6ED9EBA1);
    __M_SHA1_FF2(a, b, c, d, e, __M_SHA1_W(35, group), 0x6ED9EBA1);
    __M_SHA1_FF2(e, a, b, c, d, __M_SHA1_W(36, group), 0x6ED9EBA1);
    __M_SHA1_FF2(d, e, a, b, c, __M_SHA1_W(37, group), 0x6ED9EBA1);
    __M_SHA1_FF2(c, d, e, a, b, __M_SHA1_W(38, group), 0x6ED9EBA1);
    __M_SHA1_FF2(b, c, d, e, a, __M_SHA1_W(39, group), 0x6ED9EBA1);

    __M_SHA1_FF3(a, b, c, d, e, __M_SHA1_W(40, group), 0x8F1BBCDC);
    __M_SHA1_FF3(e, a, b, c, d, __M_SHA1_W(41, group), 0x8F1BBCDC);
    __M_SHA1_FF3(d, e, a, b, c, __M_SHA1_W(42, group), 0x8F1BBCDC);
    __M_SHA1_FF3(c, d, e, a, b, __M_SHA1_W(43, group), 0x8F1BBCDC);
    __M_SHA1_FF3(b, c, d, e, a, __M_SHA1_W(44, group), 0x8F1BBCDC);
    __M_SHA1_FF3(a, b, c, d, e, __M_SHA1_W(45, group), 0x8F1BBCDC);
    __M_SHA1_FF3(e, a, b, c, d, __M_SHA1_W(46, group), 0x8F1BBCDC);
    __M_SHA1_FF3(d, e, a, b, c, __M_SHA1_W(47, group), 0x8F1BBCDC);
    __M_SHA1_FF3(c, d, e, a, b, __M_SHA1_W(48, group), 0x8F1BBCDC);
    __M_SHA1_FF3(b, c, d, e, a, __M_SHA1_W(49, group), 0x8F1BBCDC);
    __M_SHA1_FF3(a, b, c, d, e, __M_SHA1_W(50, group), 0x8F1BBCDC);
    __M_SHA1_FF3(e, a, b, c, d, __M_SHA1_W(51, group), 0x8F1BBCDC);
    __M_SHA1_FF3(d, e, a, b, c, __M_SHA1_W(52, group), 0x8F1BBCDC);
    __M_SHA1_FF3(c, d, e, a, b, __M_SHA1_W(53, group), 0x8F1BBCDC);
    __M_SHA1_FF3(b, c, d, e, a, __M_SHA1_W(54, group), 0x8F1BBCDC);
    __M_SHA1_FF3(a, b, c, d, e, __M_SHA1_W(55, group), 0x8F1BBCDC);
    __M_SHA1_FF3(e, a, b, c, d, __M_SHA1_W(56, group), 0x8F1BBCDC);
    __M_SHA1_FF3(d, e, a, b, c, __M_SHA1_W(57, group), 0x8F1BBCDC);
    __M_SHA1_FF3(c, d, e, a, b, __M_SHA1_W(58, group), 0x8F1BBCDC);
    __M_SHA1_FF3(b, c, d, e, a, __M_SHA1_W(59, group), 0x8F1BBCDC);

    __M_SHA1_FF4(a, b, c, d, e, __M_SHA1_W(60, group), 0xCA62C1D6);
    __M_SHA1_FF4(e, a, b, c, d, __M_SHA1_W(61, group), 0xCA62C1D6);
    __M_SHA1_FF4(d, e, a, b, c, __M_SHA1_W(62, group), 0xCA62C1D6);
    __M_SHA1_FF4(c, d, e, a, b, __M_SHA1_W(63, group), 0xCA62C1D6);
    __M_SHA1_FF4(b, c, d, e, a, __M_SHA1_W(64, group), 0xCA62C1D6);
    __M_SHA1_FF4(a, b, c, d, e, __M_SHA1_W(65, group), 0xCA62C1D6);
    __M_SHA1_FF4(e, a, b, c, d, __M_SHA1_W(66, group), 0xCA62C1D6);
    __M_SHA1_FF4(d, e, a, b, c, __M_SHA1_W(67, group), 0xCA62C1D6);
    __M_SHA1_FF4(c, d, e, a, b, __M_SHA1_W(68, group), 0xCA62C1D6);
    __M_SHA1_FF4(b, c, d, e, a, __M_SHA1_W(69, group), 0xCA62C1D6);
    __M_SHA1_FF4(a, b, c, d, e, __M_SHA1_W(70, group), 0xCA62C1D6);
    __M_SHA1_FF4(e, a, b, c, d, __M_SHA1_W(71, group), 0xCA62C1D6);
    __M_SHA1_FF4(d, e, a, b, c, __M_SHA1_W(72, group), 0xCA62C1D6);
    __M_SHA1_FF4(c, d, e, a, b, __M_SHA1_W(73, group), 0xCA62C1D6);
    __M_SHA1_FF4(b, c, d, e, a, __M_SHA1_W(74, group), 0xCA62C1D6);
    __M_SHA1_FF4(a, b, c, d, e, __M_SHA1_W(75, group), 0xCA62C1D6);
    __M_SHA1_FF4(e, a, b, c, d, __M_SHA1_W(76, group), 0xCA62C1D6);
    __M_SHA1_FF4(d, e, a, b, c, __M_SHA1_W(77, group), 0xCA62C1D6);
    __M_SHA1_FF4(c, d, e, a, b, __M_SHA1_W(78, group), 0xCA62C1D6);
    __M_SHA1_FF4(b, c, d, e, a, __M_SHA1_W(79, group), 0xCA62C1D6);

    s->H0 += a;
    s->H1 += b;
    s->H2 += c;
    s->H3 += d;
    s->H4 += e;
})

MLN_FUNC_VOID(, void, mln_sha1_tobytes, \
              (mln_sha1_t *s, mln_u8ptr_t buf, mln_u32_t len), \
              (s, buf, len), \
{
    if (len == 0 || buf == NULL) return;
    mln_u32_t i = 0;

    buf[i++] = (s->H0 >> 24) & 0xff;
    if (i >= len) return;
    buf[i++] = (s->H0 >> 16) & 0xff;
    if (i >= len) return;
    buf[i++] = (s->H0 >> 8) & 0xff;
    if (i >= len) return;
    buf[i++] = (s->H0) & 0xff;
    if (i >= len) return;

    buf[i++] = (s->H1 >> 24) & 0xff;
    if (i >= len) return;
    buf[i++] = (s->H1 >> 16) & 0xff;
    if (i >= len) return;
    buf[i++] = (s->H1 >> 8) & 0xff;
    if (i >= len) return;
    buf[i++] = (s->H1) & 0xff;
    if (i >= len) return;

    buf[i++] = (s->H2 >> 24) & 0xff;
    if (i >= len) return;
    buf[i++] = (s->H2 >> 16) & 0xff;
    if (i >= len) return;
    buf[i++] = (s->H2 >> 8) & 0xff;
    if (i >= len) return;
    buf[i++] = (s->H2) & 0xff;
    if (i >= len) return;

    buf[i++] = (s->H3 >> 24) & 0xff;
    if (i >= len) return;
    buf[i++] = (s->H3 >> 16) & 0xff;
    if (i >= len) return;
    buf[i++] = (s->H3 >> 8) & 0xff;
    if (i >= len) return;
    buf[i++] = (s->H3) & 0xff;
    if (i >= len) return;

    buf[i++] = (s->H4 >> 24) & 0xff;
    if (i >= len) return;
    buf[i++] = (s->H4 >> 16) & 0xff;
    if (i >= len) return;
    buf[i++] = (s->H4 >> 8) & 0xff;
    if (i >= len) return;
    buf[i++] = (s->H4) & 0xff;
})

MLN_FUNC_VOID(, void, mln_sha1_tostring, \
              (mln_sha1_t *s, mln_s8ptr_t buf, mln_u32_t len), \
              (s, buf, len), \
{
    if (buf == NULL || len == 0) return;
    mln_u32_t i;
    mln_u8_t bytes[20] = {0};

    mln_sha1_tobytes(s, bytes, sizeof(bytes));
    len = len > (sizeof(bytes) << 1)? sizeof(bytes): ((len - 1) >> 1);
    for (i = 0; i < len; ++i) {
        *buf++ = mln_sha_hex_tostring((bytes[i] >> 4) & 0xf);
        *buf++ = mln_sha_hex_tostring(bytes[i] & 0xf);
    }
    *buf = 0;
})

MLN_FUNC_VOID(, void, mln_sha1_dump, (mln_sha1_t *s), (s), {
    printf("%lx %lx %lx %lx %lx\n", \
           (unsigned long)s->H0, \
           (unsigned long)s->H1, \
           (unsigned long)s->H2, \
           (unsigned long)s->H3, \
           (unsigned long)s->H4);
})


/*
 * sha256
 */

static inline void mln_sha256_calc_block(mln_sha256_t *s);

MLN_FUNC_VOID(, void, mln_sha256_init, (mln_sha256_t *s), (s), {
    s->H0 = 0x6a09e667;
    s->H1 = 0xbb67ae85;
    s->H2 = 0x3c6ef372;
    s->H3 = 0xa54ff53a;
    s->H4 = 0x510e527f;
    s->H5 = 0x9b05688c;
    s->H6 = 0x1f83d9ab;
    s->H7 = 0x5be0cd19;
    s->length = 0;
    s->pos = 0;
})

MLN_FUNC(, mln_sha256_t *, mln_sha256_new, (void), (), {
    mln_sha256_t *s = (mln_sha256_t *)malloc(sizeof(mln_sha256_t));
    if (s == NULL) return NULL;
    mln_sha256_init(s);
    return s;
})

MLN_FUNC(, mln_sha256_t *, mln_sha256_pool_new, (mln_alloc_t *pool), (pool), {
    mln_sha256_t *s = (mln_sha256_t *)mln_alloc_m(pool, sizeof(mln_sha256_t));
    if (s == NULL) return NULL;
    mln_sha256_init(s);
    return s;
})

MLN_FUNC_VOID(, void, mln_sha256_free, (mln_sha256_t *s), (s), {
    if (s == NULL) return;
    free(s);
})

MLN_FUNC_VOID(, void, mln_sha256_pool_free, (mln_sha256_t *s), (s), {
    if (s == NULL) return;
    mln_alloc_free(s);
})

MLN_FUNC_VOID(, void, mln_sha256_calc, \
              (mln_sha256_t *s, mln_u8ptr_t input, mln_uauto_t len, mln_u32_t is_last), \
              (s, input, len, is_last), \
{
    mln_uauto_t size;

    s->length += len;
    while (len+s->pos > __M_SHA_BUFLEN) {
        size = __M_SHA_BUFLEN - s->pos;
        memcpy(&(s->buf[s->pos]), input, size);
        len -= size;
        input += size;
        mln_sha256_calc_block(s);
        s->pos = 0;
    }

    if (len > 0) {
        memcpy(&(s->buf[s->pos]), input, len);
        s->pos += len;
    }
    if (is_last) {
        if (s->pos < 56) {
            memset(&(s->buf[s->pos]), 0, 56-s->pos);
            s->buf[s->pos] = 1 << 7;
        } else {
            if (s->pos < __M_SHA_BUFLEN) {
                memset(&(s->buf[s->pos]), 0, __M_SHA_BUFLEN-s->pos);
                s->buf[s->pos] = 1 << 7;
                mln_sha256_calc_block(s);
                s->pos = 0;
                memset(s->buf, 0, 56);
            } else {
                mln_sha256_calc_block(s);
                s->pos = 0;
                memset(s->buf, 0, 56);
                s->buf[s->pos] = 1 << 7;
            }
        }
        s->length <<= 3;
        s->buf[56] = (s->length >> 56) & 0xff;
        s->buf[57] = (s->length >> 48) & 0xff;
        s->buf[58] = (s->length >> 40) & 0xff;
        s->buf[59] = (s->length >> 32) & 0xff;
        s->buf[60] = (s->length >> 24) & 0xff;
        s->buf[61] = (s->length >> 16) & 0xff;
        s->buf[62] = (s->length >> 8) & 0xff;
        s->buf[63] = (s->length) & 0xff;
        mln_sha256_calc_block(s);
        s->pos = 0;
    }
})

#define mln_sha256_S(x,n) (((x) >> (n)) | ((x) << (32 - (n))))
#define mln_sha256_R(x,n) ((x) >> n)
#define mln_sha256_Ch(x,y,z) (((x) & (y)) ^ ((~(x)) & (z)))
#define mln_sha256_Maj(x,y,z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))
#define mln_sha256_Sigma0256(x) (mln_sha256_S((x), 2) ^ mln_sha256_S((x), 13) ^ mln_sha256_S((x), 22))
#define mln_sha256_Sigma1256(x) (mln_sha256_S((x), 6) ^ mln_sha256_S((x), 11) ^ mln_sha256_S((x), 25))
#define mln_sha256_Gamma0256(x) (mln_sha256_S((x), 7) ^ mln_sha256_S((x), 18) ^ mln_sha256_R((x), 3))
#define mln_sha256_Gamma1256(x) (mln_sha256_S((x), 17) ^ mln_sha256_S((x), 19) ^ mln_sha256_R((x), 10))

#define __M_SHA256_ROUND(a,b,c,d,e,f,g,h,w,k) \
{\
    mln_u32_t _t1 = (h) + mln_sha256_Sigma1256(e) + mln_sha256_Ch((e),(f),(g)) + (k) + (w);\
    mln_u32_t _t2 = mln_sha256_Sigma0256(a) + mln_sha256_Maj((a),(b),(c));\
    (h) = (g); (g) = (f); (f) = (e); (e) = (d) + _t1;\
    (d) = (c); (c) = (b); (b) = (a); (a) = _t1 + _t2;\
}

MLN_FUNC_VOID(static inline, void, mln_sha256_calc_block, (mln_sha256_t *s), (s), {
    mln_u32_t h0, h1, h2, h3, h4, h5, h6, h7;
    mln_u32_t j;
    mln_u32_t W[64];
    const mln_u8_t *p = s->buf;

    for (j = 0; j < 16; ++j, p += 4) {
        __M_SHA_LOAD32BE(W[j], p);
    }
    for (j = 16; j < 64; ++j) {
        W[j] = mln_sha256_Gamma1256(W[j-2]) + W[j-7] + mln_sha256_Gamma0256(W[j-15]) + W[j-16];
    }

    h0 = s->H0;
    h1 = s->H1;
    h2 = s->H2;
    h3 = s->H3;
    h4 = s->H4;
    h5 = s->H5;
    h6 = s->H6;
    h7 = s->H7;

    __M_SHA256_ROUND(h0,h1,h2,h3,h4,h5,h6,h7,W[ 0],0x428A2F98);
    __M_SHA256_ROUND(h0,h1,h2,h3,h4,h5,h6,h7,W[ 1],0x71374491);
    __M_SHA256_ROUND(h0,h1,h2,h3,h4,h5,h6,h7,W[ 2],0xB5C0FBCF);
    __M_SHA256_ROUND(h0,h1,h2,h3,h4,h5,h6,h7,W[ 3],0xE9B5DBA5);
    __M_SHA256_ROUND(h0,h1,h2,h3,h4,h5,h6,h7,W[ 4],0x3956C25B);
    __M_SHA256_ROUND(h0,h1,h2,h3,h4,h5,h6,h7,W[ 5],0x59F111F1);
    __M_SHA256_ROUND(h0,h1,h2,h3,h4,h5,h6,h7,W[ 6],0x923F82A4);
    __M_SHA256_ROUND(h0,h1,h2,h3,h4,h5,h6,h7,W[ 7],0xAB1C5ED5);
    __M_SHA256_ROUND(h0,h1,h2,h3,h4,h5,h6,h7,W[ 8],0xD807AA98);
    __M_SHA256_ROUND(h0,h1,h2,h3,h4,h5,h6,h7,W[ 9],0x12835B01);
    __M_SHA256_ROUND(h0,h1,h2,h3,h4,h5,h6,h7,W[10],0x243185BE);
    __M_SHA256_ROUND(h0,h1,h2,h3,h4,h5,h6,h7,W[11],0x550C7DC3);
    __M_SHA256_ROUND(h0,h1,h2,h3,h4,h5,h6,h7,W[12],0x72BE5D74);
    __M_SHA256_ROUND(h0,h1,h2,h3,h4,h5,h6,h7,W[13],0x80DEB1FE);
    __M_SHA256_ROUND(h0,h1,h2,h3,h4,h5,h6,h7,W[14],0x9BDC06A7);
    __M_SHA256_ROUND(h0,h1,h2,h3,h4,h5,h6,h7,W[15],0xC19BF174);
    __M_SHA256_ROUND(h0,h1,h2,h3,h4,h5,h6,h7,W[16],0xE49B69C1);
    __M_SHA256_ROUND(h0,h1,h2,h3,h4,h5,h6,h7,W[17],0xEFBE4786);
    __M_SHA256_ROUND(h0,h1,h2,h3,h4,h5,h6,h7,W[18],0x0FC19DC6);
    __M_SHA256_ROUND(h0,h1,h2,h3,h4,h5,h6,h7,W[19],0x240CA1CC);
    __M_SHA256_ROUND(h0,h1,h2,h3,h4,h5,h6,h7,W[20],0x2DE92C6F);
    __M_SHA256_ROUND(h0,h1,h2,h3,h4,h5,h6,h7,W[21],0x4A7484AA);
    __M_SHA256_ROUND(h0,h1,h2,h3,h4,h5,h6,h7,W[22],0x5CB0A9DC);
    __M_SHA256_ROUND(h0,h1,h2,h3,h4,h5,h6,h7,W[23],0x76F988DA);
    __M_SHA256_ROUND(h0,h1,h2,h3,h4,h5,h6,h7,W[24],0x983E5152);
    __M_SHA256_ROUND(h0,h1,h2,h3,h4,h5,h6,h7,W[25],0xA831C66D);
    __M_SHA256_ROUND(h0,h1,h2,h3,h4,h5,h6,h7,W[26],0xB00327C8);
    __M_SHA256_ROUND(h0,h1,h2,h3,h4,h5,h6,h7,W[27],0xBF597FC7);
    __M_SHA256_ROUND(h0,h1,h2,h3,h4,h5,h6,h7,W[28],0xC6E00BF3);
    __M_SHA256_ROUND(h0,h1,h2,h3,h4,h5,h6,h7,W[29],0xD5A79147);
    __M_SHA256_ROUND(h0,h1,h2,h3,h4,h5,h6,h7,W[30],0x06CA6351);
    __M_SHA256_ROUND(h0,h1,h2,h3,h4,h5,h6,h7,W[31],0x14292967);
    __M_SHA256_ROUND(h0,h1,h2,h3,h4,h5,h6,h7,W[32],0x27B70A85);
    __M_SHA256_ROUND(h0,h1,h2,h3,h4,h5,h6,h7,W[33],0x2E1B2138);
    __M_SHA256_ROUND(h0,h1,h2,h3,h4,h5,h6,h7,W[34],0x4D2C6DFC);
    __M_SHA256_ROUND(h0,h1,h2,h3,h4,h5,h6,h7,W[35],0x53380D13);
    __M_SHA256_ROUND(h0,h1,h2,h3,h4,h5,h6,h7,W[36],0x650A7354);
    __M_SHA256_ROUND(h0,h1,h2,h3,h4,h5,h6,h7,W[37],0x766A0ABB);
    __M_SHA256_ROUND(h0,h1,h2,h3,h4,h5,h6,h7,W[38],0x81C2C92E);
    __M_SHA256_ROUND(h0,h1,h2,h3,h4,h5,h6,h7,W[39],0x92722C85);
    __M_SHA256_ROUND(h0,h1,h2,h3,h4,h5,h6,h7,W[40],0xA2BFE8A1);
    __M_SHA256_ROUND(h0,h1,h2,h3,h4,h5,h6,h7,W[41],0xA81A664B);
    __M_SHA256_ROUND(h0,h1,h2,h3,h4,h5,h6,h7,W[42],0xC24B8B70);
    __M_SHA256_ROUND(h0,h1,h2,h3,h4,h5,h6,h7,W[43],0xC76C51A3);
    __M_SHA256_ROUND(h0,h1,h2,h3,h4,h5,h6,h7,W[44],0xD192E819);
    __M_SHA256_ROUND(h0,h1,h2,h3,h4,h5,h6,h7,W[45],0xD6990624);
    __M_SHA256_ROUND(h0,h1,h2,h3,h4,h5,h6,h7,W[46],0xF40E3585);
    __M_SHA256_ROUND(h0,h1,h2,h3,h4,h5,h6,h7,W[47],0x106AA070);
    __M_SHA256_ROUND(h0,h1,h2,h3,h4,h5,h6,h7,W[48],0x19A4C116);
    __M_SHA256_ROUND(h0,h1,h2,h3,h4,h5,h6,h7,W[49],0x1E376C08);
    __M_SHA256_ROUND(h0,h1,h2,h3,h4,h5,h6,h7,W[50],0x2748774C);
    __M_SHA256_ROUND(h0,h1,h2,h3,h4,h5,h6,h7,W[51],0x34B0BCB5);
    __M_SHA256_ROUND(h0,h1,h2,h3,h4,h5,h6,h7,W[52],0x391C0CB3);
    __M_SHA256_ROUND(h0,h1,h2,h3,h4,h5,h6,h7,W[53],0x4ED8AA4A);
    __M_SHA256_ROUND(h0,h1,h2,h3,h4,h5,h6,h7,W[54],0x5B9CCA4F);
    __M_SHA256_ROUND(h0,h1,h2,h3,h4,h5,h6,h7,W[55],0x682E6FF3);
    __M_SHA256_ROUND(h0,h1,h2,h3,h4,h5,h6,h7,W[56],0x748F82EE);
    __M_SHA256_ROUND(h0,h1,h2,h3,h4,h5,h6,h7,W[57],0x78A5636F);
    __M_SHA256_ROUND(h0,h1,h2,h3,h4,h5,h6,h7,W[58],0x84C87814);
    __M_SHA256_ROUND(h0,h1,h2,h3,h4,h5,h6,h7,W[59],0x8CC70208);
    __M_SHA256_ROUND(h0,h1,h2,h3,h4,h5,h6,h7,W[60],0x90BEFFFA);
    __M_SHA256_ROUND(h0,h1,h2,h3,h4,h5,h6,h7,W[61],0xA4506CEB);
    __M_SHA256_ROUND(h0,h1,h2,h3,h4,h5,h6,h7,W[62],0xBEF9A3F7);
    __M_SHA256_ROUND(h0,h1,h2,h3,h4,h5,h6,h7,W[63],0xC67178F2);

    s->H0 += h0;
    s->H1 += h1;
    s->H2 += h2;
    s->H3 += h3;
    s->H4 += h4;
    s->H5 += h5;
    s->H6 += h6;
    s->H7 += h7;
})

MLN_FUNC_VOID(, void, mln_sha256_tobytes, (mln_sha256_t *s, mln_u8ptr_t buf, mln_u32_t len), (s, buf, len), {
    if (len == 0 || buf == NULL) return;
    mln_u32_t i = 0;

    buf[i++] = (s->H0 >> 24) & 0xff;
    if (i >= len) return;
    buf[i++] = (s->H0 >> 16) & 0xff;
    if (i >= len) return;
    buf[i++] = (s->H0 >> 8) & 0xff;
    if (i >= len) return;
    buf[i++] = (s->H0) & 0xff;
    if (i >= len) return;

    buf[i++] = (s->H1 >> 24) & 0xff;
    if (i >= len) return;
    buf[i++] = (s->H1 >> 16) & 0xff;
    if (i >= len) return;
    buf[i++] = (s->H1 >> 8) & 0xff;
    if (i >= len) return;
    buf[i++] = (s->H1) & 0xff;
    if (i >= len) return;

    buf[i++] = (s->H2 >> 24) & 0xff;
    if (i >= len) return;
    buf[i++] = (s->H2 >> 16) & 0xff;
    if (i >= len) return;
    buf[i++] = (s->H2 >> 8) & 0xff;
    if (i >= len) return;
    buf[i++] = (s->H2) & 0xff;
    if (i >= len) return;

    buf[i++] = (s->H3 >> 24) & 0xff;
    if (i >= len) return;
    buf[i++] = (s->H3 >> 16) & 0xff;
    if (i >= len) return;
    buf[i++] = (s->H3 >> 8) & 0xff;
    if (i >= len) return;
    buf[i++] = (s->H3) & 0xff;
    if (i >= len) return;

    buf[i++] = (s->H4 >> 24) & 0xff;
    if (i >= len) return;
    buf[i++] = (s->H4 >> 16) & 0xff;
    if (i >= len) return;
    buf[i++] = (s->H4 >> 8) & 0xff;
    if (i >= len) return;
    buf[i++] = (s->H4) & 0xff;

    buf[i++] = (s->H5 >> 24) & 0xff;
    if (i >= len) return;
    buf[i++] = (s->H5 >> 16) & 0xff;
    if (i >= len) return;
    buf[i++] = (s->H5 >> 8) & 0xff;
    if (i >= len) return;
    buf[i++] = (s->H5) & 0xff;

    buf[i++] = (s->H6 >> 24) & 0xff;
    if (i >= len) return;
    buf[i++] = (s->H6 >> 16) & 0xff;
    if (i >= len) return;
    buf[i++] = (s->H6 >> 8) & 0xff;
    if (i >= len) return;
    buf[i++] = (s->H6) & 0xff;

    buf[i++] = (s->H7 >> 24) & 0xff;
    if (i >= len) return;
    buf[i++] = (s->H7 >> 16) & 0xff;
    if (i >= len) return;
    buf[i++] = (s->H7 >> 8) & 0xff;
    if (i >= len) return;
    buf[i++] = (s->H7) & 0xff;

})

MLN_FUNC_VOID(, void, mln_sha256_tostring, \
              (mln_sha256_t *s, mln_s8ptr_t buf, mln_u32_t len), \
              (s, buf, len), \
{
    if (buf == NULL || len == 0) return;
    mln_u32_t i;
    mln_u8_t bytes[32] = {0};

    mln_sha256_tobytes(s, bytes, sizeof(bytes));
    len = len > (sizeof(bytes) << 1)? sizeof(bytes): ((len - 1) >> 1);
    for (i = 0; i < len; ++i) {
        *buf++ = mln_sha_hex_tostring((bytes[i] >> 4) & 0xf);
        *buf++ = mln_sha_hex_tostring(bytes[i] & 0xf);
    }
    *buf = 0;
})

MLN_FUNC_VOID(, void, mln_sha256_dump, (mln_sha256_t *s), (s), {
    printf("%lx %lx %lx %lx %lx %lx %lx %lx\n", \
           (unsigned long)s->H0, \
           (unsigned long)s->H1, \
           (unsigned long)s->H2, \
           (unsigned long)s->H3, \
           (unsigned long)s->H4, \
           (unsigned long)s->H5, \
           (unsigned long)s->H6, \
           (unsigned long)s->H7);
})

