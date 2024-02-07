
/*
 * Copyright (C) Niklaus F.Schen.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mln_md5.h"
#include "mln_func.h"

static inline void mln_md5_calc_block(mln_md5_t *m);

static mln_u32_t s[4][4] = {
    {7, 12, 17, 22},
    {5, 9, 14, 20},
    {4, 11, 16, 23},
    {6, 10, 15, 21}
};
static mln_u32_t ti[4][16] = {
    {0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee,
     0xf57c0faf, 0x4787c62a, 0xa8304613, 0xfd469501,
     0x698098d8, 0x8b44f7af, 0xffff5bb1, 0x895cd7be,
     0x6b901122, 0xfd987193, 0xa679438e, 0x49b40821},
    {0xf61e2562, 0xc040b340, 0x265e5a51, 0xe9b6c7aa,
     0xd62f105d, 0x2441453,  0xd8a1e681, 0xe7d3fbc8,
     0x21e1cde6, 0xc33707d6, 0xf4d50d87, 0x455a14ed,
     0xa9e3e905, 0xfcefa3f8, 0x676f02d9, 0x8d2a4c8a},
    {0xfffa3942, 0x8771f681, 0x6d9d6122, 0xfde5380c,
     0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70,
     0x289b7ec6, 0xeaa127fa, 0xd4ef3085, 0x4881d05,
     0xd9d4d039, 0xe6db99e5, 0x1fa27cf8, 0xc4ac5665},
    {0xf4292244, 0x432aff97, 0xab9423a7, 0xfc93a039,
     0x655b59c3, 0x8f0ccc92, 0xffeff47d, 0x85845dd1,
     0x6fa87e4f, 0xfe2ce6e0, 0xa3014314, 0x4e0811a1,
     0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391}
};

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

    m->length += len;
    while (len+m->pos > __M_MD5_BUFLEN) {
        size = __M_MD5_BUFLEN - m->pos;
        memcpy(&(m->buf[m->pos]), input, size);
        len -= size;
        input += size;
        mln_md5_calc_block(m);
        m->pos = 0;
    }

    if (len > 0) {
        memcpy(&(m->buf[m->pos]), input, len);
        m->pos += len;
    }
    if (is_last) {
        if (m->pos < 56) {
            memset(&(m->buf[m->pos]), 0, 56-m->pos);
            m->buf[m->pos] = 1 << 7;
        } else {
            if (m->pos < __M_MD5_BUFLEN) {
                memset(&(m->buf[m->pos]), 0, __M_MD5_BUFLEN-m->pos);
                m->buf[m->pos] = 1 << 7;
                mln_md5_calc_block(m);
                m->pos = 0;
                memset(m->buf, 0, 56);
            } else {
                mln_md5_calc_block(m);
                m->pos = 0;
                memset(m->buf, 0, 56);
                m->buf[m->pos] = 1 << 7;
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
        mln_md5_calc_block(m);
        m->pos = 0;
    }
})

MLN_FUNC_VOID(static inline, void, mln_md5_calc_block, (mln_md5_t *m), (m), {
    mln_u32_t i = 0, j = 0, group[16];
    mln_u32_t a = m->A, b = m->B, c = m->C, d = m->D;
    while (i < __M_MD5_BUFLEN) {
        group[j] = 0;
        group[j] |= (m->buf[i++] & 0xff);
        group[j] |= ((m->buf[i++] & 0xff) << 8);
        group[j] |= ((m->buf[i++] & 0xff) << 16);
        group[j++] |= ((m->buf[i++] & 0xff) << 24);
    }

    __M_MD5_FF(a, b, c, d, group[0], s[0][0], ti[0][0]);
    __M_MD5_FF(d, a, b, c, group[1], s[0][1], ti[0][1]);
    __M_MD5_FF(c, d, a, b, group[2], s[0][2], ti[0][2]);
    __M_MD5_FF(b, c, d, a, group[3], s[0][3], ti[0][3]);
    __M_MD5_FF(a, b, c, d, group[4], s[0][0], ti[0][4]);
    __M_MD5_FF(d, a, b, c, group[5], s[0][1], ti[0][5]);
    __M_MD5_FF(c, d, a, b, group[6], s[0][2], ti[0][6]);
    __M_MD5_FF(b, c, d, a, group[7], s[0][3], ti[0][7]);
    __M_MD5_FF(a, b, c, d, group[8], s[0][0], ti[0][8]);
    __M_MD5_FF(d, a, b, c, group[9], s[0][1], ti[0][9]);
    __M_MD5_FF(c, d, a, b, group[10], s[0][2], ti[0][10]);
    __M_MD5_FF(b, c, d, a, group[11], s[0][3], ti[0][11]);
    __M_MD5_FF(a, b, c, d, group[12], s[0][0], ti[0][12]);
    __M_MD5_FF(d, a, b, c, group[13], s[0][1], ti[0][13]);
    __M_MD5_FF(c, d, a, b, group[14], s[0][2], ti[0][14]);
    __M_MD5_FF(b, c, d, a, group[15], s[0][3], ti[0][15]);

    __M_MD5_GG(a, b, c, d, group[1], s[1][0], ti[1][0]);
    __M_MD5_GG(d, a, b, c, group[6], s[1][1], ti[1][1]);
    __M_MD5_GG(c, d, a, b, group[11], s[1][2], ti[1][2]);
    __M_MD5_GG(b, c, d, a, group[0], s[1][3], ti[1][3]);
    __M_MD5_GG(a, b, c, d, group[5], s[1][0], ti[1][4]);
    __M_MD5_GG(d, a, b, c, group[10], s[1][1], ti[1][5]);
    __M_MD5_GG(c, d, a, b, group[15], s[1][2], ti[1][6]);
    __M_MD5_GG(b, c, d, a, group[4], s[1][3], ti[1][7]);
    __M_MD5_GG(a, b, c, d, group[9], s[1][0], ti[1][8]);
    __M_MD5_GG(d, a, b, c, group[14], s[1][1], ti[1][9]);
    __M_MD5_GG(c, d, a, b, group[3], s[1][2], ti[1][10]);
    __M_MD5_GG(b, c, d, a, group[8], s[1][3], ti[1][11]);
    __M_MD5_GG(a, b, c, d, group[13], s[1][0], ti[1][12]);
    __M_MD5_GG(d, a, b, c, group[2], s[1][1], ti[1][13]);
    __M_MD5_GG(c, d, a, b, group[7], s[1][2], ti[1][14]);
    __M_MD5_GG(b, c, d, a, group[12], s[1][3], ti[1][15]);

    __M_MD5_HH(a, b, c, d, group[5], s[2][0], ti[2][0]);
    __M_MD5_HH(d, a, b, c, group[8], s[2][1], ti[2][1]);
    __M_MD5_HH(c, d, a, b, group[11], s[2][2], ti[2][2]);
    __M_MD5_HH(b, c, d, a, group[14], s[2][3], ti[2][3]);
    __M_MD5_HH(a, b, c, d, group[1], s[2][0], ti[2][4]);
    __M_MD5_HH(d, a, b, c, group[4], s[2][1], ti[2][5]);
    __M_MD5_HH(c, d, a, b, group[7], s[2][2], ti[2][6]);
    __M_MD5_HH(b, c, d, a, group[10], s[2][3], ti[2][7]);
    __M_MD5_HH(a, b, c, d, group[13], s[2][0], ti[2][8]);
    __M_MD5_HH(d, a, b, c, group[0], s[2][1], ti[2][9]);
    __M_MD5_HH(c, d, a, b, group[3], s[2][2], ti[2][10]);
    __M_MD5_HH(b, c, d, a, group[6], s[2][3], ti[2][11]);
    __M_MD5_HH(a, b, c, d, group[9], s[2][0], ti[2][12]);
    __M_MD5_HH(d, a, b, c, group[12], s[2][1], ti[2][13]);
    __M_MD5_HH(c, d, a, b, group[15], s[2][2], ti[2][14]);
    __M_MD5_HH(b, c, d, a, group[2], s[2][3], ti[2][15]);

    __M_MD5_II(a, b, c, d, group[0], s[3][0], ti[3][0]);
    __M_MD5_II(d, a, b, c, group[7], s[3][1], ti[3][1]);
    __M_MD5_II(c, d, a, b, group[14], s[3][2], ti[3][2]);
    __M_MD5_II(b, c, d, a, group[5], s[3][3], ti[3][3]);
    __M_MD5_II(a, b, c, d, group[12], s[3][0], ti[3][4]);
    __M_MD5_II(d, a, b, c, group[3], s[3][1], ti[3][5]);
    __M_MD5_II(c, d, a, b, group[10], s[3][2], ti[3][6]);
    __M_MD5_II(b, c, d, a, group[1], s[3][3], ti[3][7]);
    __M_MD5_II(a, b, c, d, group[8], s[3][0], ti[3][8]);
    __M_MD5_II(d, a, b, c, group[15], s[3][1], ti[3][9]);
    __M_MD5_II(c, d, a, b, group[6], s[3][2], ti[3][10]);
    __M_MD5_II(b, c, d, a, group[13], s[3][3], ti[3][11]);
    __M_MD5_II(a, b, c, d, group[4], s[3][0], ti[3][12]);
    __M_MD5_II(d, a, b, c, group[11], s[3][1], ti[3][13]);
    __M_MD5_II(c, d, a, b, group[2], s[3][2], ti[3][14]);
    __M_MD5_II(b, c, d, a, group[9], s[3][3], ti[3][15]);

    m->A += a;
    m->B += b;
    m->C += c;
    m->D += d;
    m->A &= 0xffffffff;
    m->B &= 0xffffffff;
    m->C &= 0xffffffff;
    m->D &= 0xffffffff;
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

