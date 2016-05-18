
/*
 * Copyright (C) Niklaus F.Schen.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mln_sha.h"

static mln_u32_t k[4] = {
    0x5A827999,
    0x6ED9EBA1,
    0x8F1BBCDC,
    0xCA62C1D6
};

static inline void mln_sha1_calc_block(mln_sha1_t *s);

void mln_sha1_init(mln_sha1_t *s)
{
    s->H0 = 0x67452301;
    s->H1 = 0xefcdab89;
    s->H2 = 0x98badcfe;
    s->H3 = 0x10325476;
    s->H4 = 0xc3d2e1f0;
    s->length = 0;
    s->pos = 0;
}

mln_sha1_t *mln_sha1_new(void)
{
    mln_sha1_t *s = (mln_sha1_t *)malloc(sizeof(mln_sha1_t));
    if (s == NULL) return NULL;
    mln_sha1_init(s);
    return s;
}

mln_sha1_t *mln_sha1_pool_new(mln_alloc_t *pool)
{
    mln_sha1_t *s = (mln_sha1_t *)mln_alloc_m(pool, sizeof(mln_sha1_t));
    if (s == NULL) return NULL;
    mln_sha1_init(s);
    return s;
}

void mln_sha1_free(mln_sha1_t *s)
{
    if (s == NULL) return;
    free(s);
}

void mln_sha1_pool_free(mln_sha1_t *s)
{
    if (s == NULL) return;
    mln_alloc_free(s);
}

void mln_sha1_calc(mln_sha1_t *s, mln_u8ptr_t input, mln_uauto_t len, mln_u32_t is_last)
{
    mln_uauto_t size;

    s->length += len;
    while (len+s->pos > __M_SHA_BUFLEN) {
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
        s->buf[57] = (s->length >> 48) & 0xff;;
        s->buf[58] = (s->length >> 40) & 0xff;;
        s->buf[59] = (s->length >> 32) & 0xff;;
        s->buf[60] = (s->length >> 24) & 0xff;;
        s->buf[61] = (s->length >> 16) & 0xff;;
        s->buf[62] = (s->length >> 8) & 0xff;;
        s->buf[63] = (s->length) & 0xff;
        mln_sha1_calc_block(s);
        s->pos = 0;
    }
}

static inline void mln_sha1_calc_block(mln_sha1_t *s)
{
    mln_u32_t i = 0, j = 0, group[16];
    mln_u32_t a = s->H0, b = s->H1, c = s->H2, d = s->H3, e = s->H4;
    while (i < __M_SHA_BUFLEN) {
        group[j] = 0;
        group[j] |= ((s->buf[i++] & 0xff) << 24);
        group[j] |= ((s->buf[i++] & 0xff) << 16);
        group[j] |= ((s->buf[i++] & 0xff) << 8);
        group[j] |= ((s->buf[i++] & 0xff));
        j++;
    }

    __M_SHA1_FF1(a, b, c, d, e, group[0], k[0]);
    __M_SHA1_FF1(e, a, b, c, d, group[1], k[0]);
    __M_SHA1_FF1(d, e, a, b, c, group[2], k[0]);
    __M_SHA1_FF1(c, d, e, a, b, group[3], k[0]);
    __M_SHA1_FF1(b, c, d, e, a, group[4], k[0]);
    __M_SHA1_FF1(a, b, c, d, e, group[5], k[0]);
    __M_SHA1_FF1(e, a, b, c, d, group[6], k[0]);
    __M_SHA1_FF1(d, e, a, b, c, group[7], k[0]);
    __M_SHA1_FF1(c, d, e, a, b, group[8], k[0]);
    __M_SHA1_FF1(b, c, d, e, a, group[9], k[0]);
    __M_SHA1_FF1(a, b, c, d, e, group[10], k[0]);
    __M_SHA1_FF1(e, a, b, c, d, group[11], k[0]);
    __M_SHA1_FF1(d, e, a, b, c, group[12], k[0]);
    __M_SHA1_FF1(c, d, e, a, b, group[13], k[0]);
    __M_SHA1_FF1(b, c, d, e, a, group[14], k[0]);
    __M_SHA1_FF1(a, b, c, d, e, group[15], k[0]);
    __M_SHA1_FF1(e, a, b, c, d, __M_SHA1_W(16, group), k[0]);
    __M_SHA1_FF1(d, e, a, b, c, __M_SHA1_W(17, group), k[0]);
    __M_SHA1_FF1(c, d, e, a, b, __M_SHA1_W(18, group), k[0]);
    __M_SHA1_FF1(b, c, d, e, a, __M_SHA1_W(19, group), k[0]);

    __M_SHA1_FF2(a, b, c, d, e, __M_SHA1_W(20, group), k[1]);
    __M_SHA1_FF2(e, a, b, c, d, __M_SHA1_W(21, group), k[1]);
    __M_SHA1_FF2(d, e, a, b, c, __M_SHA1_W(22, group), k[1]);
    __M_SHA1_FF2(c, d, e, a, b, __M_SHA1_W(23, group), k[1]);
    __M_SHA1_FF2(b, c, d, e, a, __M_SHA1_W(24, group), k[1]);
    __M_SHA1_FF2(a, b, c, d, e, __M_SHA1_W(25, group), k[1]);
    __M_SHA1_FF2(e, a, b, c, d, __M_SHA1_W(26, group), k[1]);
    __M_SHA1_FF2(d, e, a, b, c, __M_SHA1_W(27, group), k[1]);
    __M_SHA1_FF2(c, d, e, a, b, __M_SHA1_W(28, group), k[1]);
    __M_SHA1_FF2(b, c, d, e, a, __M_SHA1_W(29, group), k[1]);
    __M_SHA1_FF2(a, b, c, d, e, __M_SHA1_W(30, group), k[1]);
    __M_SHA1_FF2(e, a, b, c, d, __M_SHA1_W(31, group), k[1]);
    __M_SHA1_FF2(d, e, a, b, c, __M_SHA1_W(32, group), k[1]);
    __M_SHA1_FF2(c, d, e, a, b, __M_SHA1_W(33, group), k[1]);
    __M_SHA1_FF2(b, c, d, e, a, __M_SHA1_W(34, group), k[1]);
    __M_SHA1_FF2(a, b, c, d, e, __M_SHA1_W(35, group), k[1]);
    __M_SHA1_FF2(e, a, b, c, d, __M_SHA1_W(36, group), k[1]);
    __M_SHA1_FF2(d, e, a, b, c, __M_SHA1_W(37, group), k[1]);
    __M_SHA1_FF2(c, d, e, a, b, __M_SHA1_W(38, group), k[1]);
    __M_SHA1_FF2(b, c, d, e, a, __M_SHA1_W(39, group), k[1]);

    __M_SHA1_FF3(a, b, c, d, e, __M_SHA1_W(40, group), k[2]);
    __M_SHA1_FF3(e, a, b, c, d, __M_SHA1_W(41, group), k[2]);
    __M_SHA1_FF3(d, e, a, b, c, __M_SHA1_W(42, group), k[2]);
    __M_SHA1_FF3(c, d, e, a, b, __M_SHA1_W(43, group), k[2]);
    __M_SHA1_FF3(b, c, d, e, a, __M_SHA1_W(44, group), k[2]);
    __M_SHA1_FF3(a, b, c, d, e, __M_SHA1_W(45, group), k[2]);
    __M_SHA1_FF3(e, a, b, c, d, __M_SHA1_W(46, group), k[2]);
    __M_SHA1_FF3(d, e, a, b, c, __M_SHA1_W(47, group), k[2]);
    __M_SHA1_FF3(c, d, e, a, b, __M_SHA1_W(48, group), k[2]);
    __M_SHA1_FF3(b, c, d, e, a, __M_SHA1_W(49, group), k[2]);
    __M_SHA1_FF3(a, b, c, d, e, __M_SHA1_W(50, group), k[2]);
    __M_SHA1_FF3(e, a, b, c, d, __M_SHA1_W(51, group), k[2]);
    __M_SHA1_FF3(d, e, a, b, c, __M_SHA1_W(52, group), k[2]);
    __M_SHA1_FF3(c, d, e, a, b, __M_SHA1_W(53, group), k[2]);
    __M_SHA1_FF3(b, c, d, e, a, __M_SHA1_W(54, group), k[2]);
    __M_SHA1_FF3(a, b, c, d, e, __M_SHA1_W(55, group), k[2]);
    __M_SHA1_FF3(e, a, b, c, d, __M_SHA1_W(56, group), k[2]);
    __M_SHA1_FF3(d, e, a, b, c, __M_SHA1_W(57, group), k[2]);
    __M_SHA1_FF3(c, d, e, a, b, __M_SHA1_W(58, group), k[2]);
    __M_SHA1_FF3(b, c, d, e, a, __M_SHA1_W(59, group), k[2]);

    __M_SHA1_FF4(a, b, c, d, e, __M_SHA1_W(60, group), k[3]);
    __M_SHA1_FF4(e, a, b, c, d, __M_SHA1_W(61, group), k[3]);
    __M_SHA1_FF4(d, e, a, b, c, __M_SHA1_W(62, group), k[3]);
    __M_SHA1_FF4(c, d, e, a, b, __M_SHA1_W(63, group), k[3]);
    __M_SHA1_FF4(b, c, d, e, a, __M_SHA1_W(64, group), k[3]);
    __M_SHA1_FF4(a, b, c, d, e, __M_SHA1_W(65, group), k[3]);
    __M_SHA1_FF4(e, a, b, c, d, __M_SHA1_W(66, group), k[3]);
    __M_SHA1_FF4(d, e, a, b, c, __M_SHA1_W(67, group), k[3]);
    __M_SHA1_FF4(c, d, e, a, b, __M_SHA1_W(68, group), k[3]);
    __M_SHA1_FF4(b, c, d, e, a, __M_SHA1_W(69, group), k[3]);
    __M_SHA1_FF4(a, b, c, d, e, __M_SHA1_W(70, group), k[3]);
    __M_SHA1_FF4(e, a, b, c, d, __M_SHA1_W(71, group), k[3]);
    __M_SHA1_FF4(d, e, a, b, c, __M_SHA1_W(72, group), k[3]);
    __M_SHA1_FF4(c, d, e, a, b, __M_SHA1_W(73, group), k[3]);
    __M_SHA1_FF4(b, c, d, e, a, __M_SHA1_W(74, group), k[3]);
    __M_SHA1_FF4(a, b, c, d, e, __M_SHA1_W(75, group), k[3]);
    __M_SHA1_FF4(e, a, b, c, d, __M_SHA1_W(76, group), k[3]);
    __M_SHA1_FF4(d, e, a, b, c, __M_SHA1_W(77, group), k[3]);
    __M_SHA1_FF4(c, d, e, a, b, __M_SHA1_W(78, group), k[3]);
    __M_SHA1_FF4(b, c, d, e, a, __M_SHA1_W(79, group), k[3]);

    s->H0 += a;
    s->H1 += b;
    s->H2 += c;
    s->H3 += d;
    s->H4 += e;
}

void mln_sha1_toBytes(mln_sha1_t *s, mln_u8ptr_t buf, mln_u32_t len)
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
}

void mln_sha1_toString(mln_sha1_t *s, mln_s8ptr_t buf, mln_u32_t len)
{
    if (buf == NULL || len == 0) return;
    mln_u32_t i, n = 0;
    mln_u8_t bytes[20] = {0};

    mln_sha1_toBytes(s, bytes, sizeof(bytes));
    for (i = 0; i < sizeof(bytes); i++) {
        if (n >= len) break;
        n += snprintf(buf + n, len - n, "%02x", bytes[i]);
    }
    if (n < len) buf[n] = 0;
}

void mln_sha1_dump(mln_sha1_t *s)
{
    printf("%lx %lx %lx %lx %lx\n", \
           (unsigned long)s->H0, \
           (unsigned long)s->H1, \
           (unsigned long)s->H2, \
           (unsigned long)s->H3, \
           (unsigned long)s->H4);
}

