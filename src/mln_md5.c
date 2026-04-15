
/*
 * Copyright (C) Niklaus F.Schen.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mln_md5.h"
#include "mln_func.h"

#if defined(__GNUC__) || defined(__clang__)
#define __M_MD5_ALWAYS_INLINE __attribute__((always_inline))
#else
#define __M_MD5_ALWAYS_INLINE
#endif

#if (defined(__x86_64__) || defined(__i386__)) && (defined(__GNUC__) || defined(__clang__))
#if !defined(__BMI__) || !defined(__BMI2__)
#pragma GCC target("bmi,bmi2")
#endif
#endif

static inline __M_MD5_ALWAYS_INLINE void mln_md5_transform(mln_u32_t *pa, mln_u32_t *pb, mln_u32_t *pc, mln_u32_t *pd, const mln_u8_t *blk);

MLN_FUNC_VOID(, void, mln_md5_init, (mln_md5_t *m), (m), {
    m->A = 0x67452301;
    m->B = 0xefcdab89;
    m->C = 0x98badcfe;
    m->D = 0x10325476;
    m->length = 0;
    m->pos = 0;
})

MLN_FUNC(, mln_md5_t *, mln_md5_new, (void), (), {
    mln_md5_t *m = (mln_md5_t *)malloc(sizeof(mln_md5_t));
    if (m == NULL) return m;
    mln_md5_init(m);
    return m;
})

MLN_FUNC(, mln_md5_t *, mln_md5_pool_new, (mln_alloc_t *pool), (pool), {
    mln_md5_t *m = (mln_md5_t *)mln_alloc_m(pool, sizeof(mln_md5_t));
    if (m == NULL) return m;
    mln_md5_init(m);
    return m;
})

MLN_FUNC_VOID(, void, mln_md5_free, (mln_md5_t *m), (m), {
    if (m == NULL) return;
    free(m);
})

MLN_FUNC_VOID(, void, mln_md5_pool_free, (mln_md5_t *m), (m), {
    if (m == NULL) return;
    mln_alloc_free(m);
})

MLN_FUNC_VOID(, void, mln_md5_calc, \
              (mln_md5_t *m, mln_u8ptr_t input, mln_uauto_t len, mln_u32_t is_last), \
              (m, input, len, is_last), \
{
    mln_uauto_t size;
    mln_u32_t sa = m->A, sb = m->B, sc = m->C, sd = m->D;

    m->length += len;

    if (m->pos > 0 && len + m->pos >= __M_MD5_BUFLEN) {
        size = __M_MD5_BUFLEN - m->pos;
        memcpy(&(m->buf[m->pos]), input, size);
        len -= size;
        input += size;
        mln_md5_transform(&sa, &sb, &sc, &sd, m->buf);
        m->pos = 0;
    }

    while (len >= __M_MD5_BUFLEN) {
        mln_md5_transform(&sa, &sb, &sc, &sd, input);
        len -= __M_MD5_BUFLEN;
        input += __M_MD5_BUFLEN;
    }

    if (len > 0) {
        memcpy(&(m->buf[m->pos]), input, len);
        m->pos += len;
    }
    if (is_last) {
        if (m->pos < 56) {
            memset(&(m->buf[m->pos]), 0, 56-m->pos);
            m->buf[m->pos] = 0x80;
        } else {
            if (m->pos < __M_MD5_BUFLEN) {
                memset(&(m->buf[m->pos]), 0, __M_MD5_BUFLEN-m->pos);
                m->buf[m->pos] = 0x80;
                mln_md5_transform(&sa, &sb, &sc, &sd, m->buf);
                m->pos = 0;
                memset(m->buf, 0, 56);
            } else {
                mln_md5_transform(&sa, &sb, &sc, &sd, m->buf);
                m->pos = 0;
                memset(m->buf, 0, 56);
                m->buf[m->pos] = 0x80;
            }
        }
        m->length <<= 3;
        m->buf[56] = m->length & 0xff;
        m->buf[57] = (m->length >> 8) & 0xff;
        m->buf[58] = (m->length >> 16) & 0xff;
        m->buf[59] = (m->length >> 24) & 0xff;
        m->buf[60] = (m->length >> 32) & 0xff;
        m->buf[61] = (m->length >> 40) & 0xff;
        m->buf[62] = (m->length >> 48) & 0xff;
        m->buf[63] = (m->length >> 56) & 0xff;
        mln_md5_transform(&sa, &sb, &sc, &sd, m->buf);
        m->pos = 0;
    }

    m->A = sa;
    m->B = sb;
    m->C = sc;
    m->D = sd;
})

MLN_FUNC_VOID(static inline __M_MD5_ALWAYS_INLINE, void, mln_md5_transform, \
              (mln_u32_t *pa, mln_u32_t *pb, mln_u32_t *pc, mln_u32_t *pd, const mln_u8_t *blk), \
              (pa, pb, pc, pd, blk), \
{
    mln_u32_t a = *pa, b = *pb, c = *pc, d = *pd;
    mln_u32_t w0, w1, w2, w3, w4, w5, w6, w7, w8, w9, w10, w11, w12, w13, w14, w15;

    memcpy(&w0,  blk,      4); memcpy(&w1,  blk +  4, 4);
    memcpy(&w2,  blk +  8, 4); memcpy(&w3,  blk + 12, 4);
    memcpy(&w4,  blk + 16, 4); memcpy(&w5,  blk + 20, 4);
    memcpy(&w6,  blk + 24, 4); memcpy(&w7,  blk + 28, 4);
    memcpy(&w8,  blk + 32, 4); memcpy(&w9,  blk + 36, 4);
    memcpy(&w10, blk + 40, 4); memcpy(&w11, blk + 44, 4);
    memcpy(&w12, blk + 48, 4); memcpy(&w13, blk + 52, 4);
    memcpy(&w14, blk + 56, 4); memcpy(&w15, blk + 60, 4);

#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define __M_MD5_SWAP32(x) (((x) >> 24) | (((x) >> 8) & 0xff00) | (((x) << 8) & 0xff0000) | ((x) << 24))
    w0  = __M_MD5_SWAP32(w0);  w1  = __M_MD5_SWAP32(w1);
    w2  = __M_MD5_SWAP32(w2);  w3  = __M_MD5_SWAP32(w3);
    w4  = __M_MD5_SWAP32(w4);  w5  = __M_MD5_SWAP32(w5);
    w6  = __M_MD5_SWAP32(w6);  w7  = __M_MD5_SWAP32(w7);
    w8  = __M_MD5_SWAP32(w8);  w9  = __M_MD5_SWAP32(w9);
    w10 = __M_MD5_SWAP32(w10); w11 = __M_MD5_SWAP32(w11);
    w12 = __M_MD5_SWAP32(w12); w13 = __M_MD5_SWAP32(w13);
    w14 = __M_MD5_SWAP32(w14); w15 = __M_MD5_SWAP32(w15);
#undef __M_MD5_SWAP32
#endif

    /* Round 1 - FF */
    __M_MD5_FF(a, b, c, d, w0,   7, 0xd76aa478);
    __M_MD5_FF(d, a, b, c, w1,  12, 0xe8c7b756);
    __M_MD5_FF(c, d, a, b, w2,  17, 0x242070db);
    __M_MD5_FF(b, c, d, a, w3,  22, 0xc1bdceee);
    __M_MD5_FF(a, b, c, d, w4,   7, 0xf57c0faf);
    __M_MD5_FF(d, a, b, c, w5,  12, 0x4787c62a);
    __M_MD5_FF(c, d, a, b, w6,  17, 0xa8304613);
    __M_MD5_FF(b, c, d, a, w7,  22, 0xfd469501);
    __M_MD5_FF(a, b, c, d, w8,   7, 0x698098d8);
    __M_MD5_FF(d, a, b, c, w9,  12, 0x8b44f7af);
    __M_MD5_FF(c, d, a, b, w10, 17, 0xffff5bb1);
    __M_MD5_FF(b, c, d, a, w11, 22, 0x895cd7be);
    __M_MD5_FF(a, b, c, d, w12,  7, 0x6b901122);
    __M_MD5_FF(d, a, b, c, w13, 12, 0xfd987193);
    __M_MD5_FF(c, d, a, b, w14, 17, 0xa679438e);
    __M_MD5_FF(b, c, d, a, w15, 22, 0x49b40821);

    /* Round 2 - GG */
    __M_MD5_GG(a, b, c, d, w1,   5, 0xf61e2562);
    __M_MD5_GG(d, a, b, c, w6,   9, 0xc040b340);
    __M_MD5_GG(c, d, a, b, w11, 14, 0x265e5a51);
    __M_MD5_GG(b, c, d, a, w0,  20, 0xe9b6c7aa);
    __M_MD5_GG(a, b, c, d, w5,   5, 0xd62f105d);
    __M_MD5_GG(d, a, b, c, w10,  9, 0x02441453);
    __M_MD5_GG(c, d, a, b, w15, 14, 0xd8a1e681);
    __M_MD5_GG(b, c, d, a, w4,  20, 0xe7d3fbc8);
    __M_MD5_GG(a, b, c, d, w9,   5, 0x21e1cde6);
    __M_MD5_GG(d, a, b, c, w14,  9, 0xc33707d6);
    __M_MD5_GG(c, d, a, b, w3,  14, 0xf4d50d87);
    __M_MD5_GG(b, c, d, a, w8,  20, 0x455a14ed);
    __M_MD5_GG(a, b, c, d, w13,  5, 0xa9e3e905);
    __M_MD5_GG(d, a, b, c, w2,   9, 0xfcefa3f8);
    __M_MD5_GG(c, d, a, b, w7,  14, 0x676f02d9);
    __M_MD5_GG(b, c, d, a, w12, 20, 0x8d2a4c8a);

    /* Round 3 - HH */
    __M_MD5_HH(a, b, c, d, w5,   4, 0xfffa3942);
    __M_MD5_HH(d, a, b, c, w8,  11, 0x8771f681);
    __M_MD5_HH(c, d, a, b, w11, 16, 0x6d9d6122);
    __M_MD5_HH(b, c, d, a, w14, 23, 0xfde5380c);
    __M_MD5_HH(a, b, c, d, w1,   4, 0xa4beea44);
    __M_MD5_HH(d, a, b, c, w4,  11, 0x4bdecfa9);
    __M_MD5_HH(c, d, a, b, w7,  16, 0xf6bb4b60);
    __M_MD5_HH(b, c, d, a, w10, 23, 0xbebfbc70);
    __M_MD5_HH(a, b, c, d, w13,  4, 0x289b7ec6);
    __M_MD5_HH(d, a, b, c, w0,  11, 0xeaa127fa);
    __M_MD5_HH(c, d, a, b, w3,  16, 0xd4ef3085);
    __M_MD5_HH(b, c, d, a, w6,  23, 0x04881d05);
    __M_MD5_HH(a, b, c, d, w9,   4, 0xd9d4d039);
    __M_MD5_HH(d, a, b, c, w12, 11, 0xe6db99e5);
    __M_MD5_HH(c, d, a, b, w15, 16, 0x1fa27cf8);
    __M_MD5_HH(b, c, d, a, w2,  23, 0xc4ac5665);

    /* Round 4 - II */
    __M_MD5_II(a, b, c, d, w0,   6, 0xf4292244);
    __M_MD5_II(d, a, b, c, w7,  10, 0x432aff97);
    __M_MD5_II(c, d, a, b, w14, 15, 0xab9423a7);
    __M_MD5_II(b, c, d, a, w5,  21, 0xfc93a039);
    __M_MD5_II(a, b, c, d, w12,  6, 0x655b59c3);
    __M_MD5_II(d, a, b, c, w3,  10, 0x8f0ccc92);
    __M_MD5_II(c, d, a, b, w10, 15, 0xffeff47d);
    __M_MD5_II(b, c, d, a, w1,  21, 0x85845dd1);
    __M_MD5_II(a, b, c, d, w8,   6, 0x6fa87e4f);
    __M_MD5_II(d, a, b, c, w15, 10, 0xfe2ce6e0);
    __M_MD5_II(c, d, a, b, w6,  15, 0xa3014314);
    __M_MD5_II(b, c, d, a, w13, 21, 0x4e0811a1);
    __M_MD5_II(a, b, c, d, w4,   6, 0xf7537e82);
    __M_MD5_II(d, a, b, c, w11, 10, 0xbd3af235);
    __M_MD5_II(c, d, a, b, w2,  15, 0x2ad7d2bb);
    __M_MD5_II(b, c, d, a, w9,  21, 0xeb86d391);

    *pa += a;
    *pb += b;
    *pc += c;
    *pd += d;
})

MLN_FUNC_VOID(, void, mln_md5_tobytes, \
              (mln_md5_t *m, mln_u8ptr_t buf, mln_u32_t len), (m, buf, len), \
{
    if (len == 0 || buf == NULL) return;
    mln_u32_t i = 0;

    buf[i++] = m->A & 0xff;
    if (i >= len) return;
    buf[i++] = (m->A >> 8) & 0xff;
    if (i >= len) return;
    buf[i++] = (m->A >> 16) & 0xff;
    if (i >= len) return;
    buf[i++] = (m->A >> 24) & 0xff;
    if (i >= len) return;

    buf[i++] = m->B & 0xff;
    if (i >= len) return;
    buf[i++] = (m->B >> 8) & 0xff;
    if (i >= len) return;
    buf[i++] = (m->B >> 16) & 0xff;
    if (i >= len) return;
    buf[i++] = (m->B >> 24) & 0xff;
    if (i >= len) return;

    buf[i++] = m->C & 0xff;
    if (i >= len) return;
    buf[i++] = (m->C >> 8) & 0xff;
    if (i >= len) return;
    buf[i++] = (m->C >> 16) & 0xff;
    if (i >= len) return;
    buf[i++] = (m->C >> 24) & 0xff;
    if (i >= len) return;

    buf[i++] = m->D & 0xff;
    if (i >= len) return;
    buf[i++] = (m->D >> 8) & 0xff;
    if (i >= len) return;
    buf[i++] = (m->D >> 16) & 0xff;
    if (i >= len) return;
    buf[i++] = (m->D >> 24) & 0xff;
})

MLN_FUNC(static inline, mln_s8_t, mln_md5_hex_tostring, (mln_u8_t c), (c), {
    return c < 10? ('0' + c): ('a' + (c - 10));
})

MLN_FUNC_VOID(, void, mln_md5_tostring, \
              (mln_md5_t *m, mln_s8ptr_t buf, mln_u32_t len), \
              (m, buf, len), \
{
    if (buf == NULL || len == 0) return;
    mln_u32_t i;
    mln_u8_t bytes[16] = {0};

    mln_md5_tobytes(m, bytes, sizeof(bytes));
    len = len > (sizeof(bytes) << 1)? sizeof(bytes): ((len - 1) >> 1);
    for (i = 0; i < len; ++i) {
        *buf++ = mln_md5_hex_tostring((bytes[i] >> 4) & 0xf);
        *buf++ = mln_md5_hex_tostring(bytes[i] & 0xf);
    }
    *buf = 0;
})

MLN_FUNC_VOID(, void, mln_md5_dump, (mln_md5_t *m), (m), {
    printf("%lx %lx %lx %lx\n", (unsigned long)m->A, (unsigned long)m->B, (unsigned long)m->C, (unsigned long)m->D);
})


