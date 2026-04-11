
/*
 * Copyright (C) Niklaus F.Schen.
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "mln_base64.h"
#include "mln_func.h"

static const mln_u8_t base64_enc_table[64] = {
    'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P',
    'Q','R','S','T','U','V','W','X','Y','Z','a','b','c','d','e','f',
    'g','h','i','j','k','l','m','n','o','p','q','r','s','t','u','v',
    'w','x','y','z','0','1','2','3','4','5','6','7','8','9','+','/'
};

/*
 * 12-bit -> 2-character lookup table. Each entry encodes a 12-bit input
 * as two base64 characters packed into a 16-bit integer (low byte first
 * so a plain memcpy produces the correct output order on little-endian
 * CPUs; on big-endian CPUs the table is built with the opposite layout).
 *
 * The table is ~8 KiB and is lazily initialized on first use. Because
 * every write produces the same value, concurrent initialization from
 * multiple threads is safe without synchronization.
 */
static mln_u16_t base64_enc12_table[4096];
static int base64_enc12_ready = 0;

static inline void base64_enc12_init(void)
{
    /* Detect endianness at runtime so the table is correct either way. */
    mln_u16_t probe = 0x0100;
    int le = (*(mln_u8_t *)&probe) == 0x00;
    int i;
    for (i = 0; i < 4096; ++i) {
        mln_u8_t a = base64_enc_table[(i >> 6) & 0x3f];
        mln_u8_t b = base64_enc_table[i & 0x3f];
        base64_enc12_table[i] = le
            ? (mln_u16_t)((mln_u16_t)a | ((mln_u16_t)b << 8))
            : (mln_u16_t)(((mln_u16_t)a << 8) | (mln_u16_t)b);
    }
    base64_enc12_ready = 1;
}

/*
 * Decode table: valid characters map to their 6-bit value; all other
 * entries default to 0 via C99 default zero initialization, which matches
 * the historical behavior of this module.
 */
static const mln_u8_t base64_dec_table[256] = {
    ['A'] =  0, ['B'] =  1, ['C'] =  2, ['D'] =  3,
    ['E'] =  4, ['F'] =  5, ['G'] =  6, ['H'] =  7,
    ['I'] =  8, ['J'] =  9, ['K'] = 10, ['L'] = 11,
    ['M'] = 12, ['N'] = 13, ['O'] = 14, ['P'] = 15,
    ['Q'] = 16, ['R'] = 17, ['S'] = 18, ['T'] = 19,
    ['U'] = 20, ['V'] = 21, ['W'] = 22, ['X'] = 23,
    ['Y'] = 24, ['Z'] = 25,
    ['a'] = 26, ['b'] = 27, ['c'] = 28, ['d'] = 29,
    ['e'] = 30, ['f'] = 31, ['g'] = 32, ['h'] = 33,
    ['i'] = 34, ['j'] = 35, ['k'] = 36, ['l'] = 37,
    ['m'] = 38, ['n'] = 39, ['o'] = 40, ['p'] = 41,
    ['q'] = 42, ['r'] = 43, ['s'] = 44, ['t'] = 45,
    ['u'] = 46, ['v'] = 47, ['w'] = 48, ['x'] = 49,
    ['y'] = 50, ['z'] = 51,
    ['0'] = 52, ['1'] = 53, ['2'] = 54, ['3'] = 55,
    ['4'] = 56, ['5'] = 57, ['6'] = 58, ['7'] = 59,
    ['8'] = 60, ['9'] = 61,
    ['+'] = 62, ['/'] = 63
};

/*
 * Fast encode core: reads inlen bytes from in and writes the
 * base64 encoding (without trailing NUL) into out. The caller
 * must ensure out is at least ((inlen + 2) / 3) * 4 bytes.
 * Returns the number of bytes written.
 */
MLN_FUNC(static, mln_uauto_t, mln_base64_encode_core, \
         (mln_u8ptr_t in, mln_uauto_t inlen, mln_u8ptr_t out), \
         (in, inlen, out), \
{
    if (!base64_enc12_ready) base64_enc12_init();
    const mln_u16_t *t12 = base64_enc12_table;
    const mln_u8_t *enc = base64_enc_table;
    mln_u8ptr_t o = out;
    mln_uauto_t i = 0;
    mln_uauto_t bulk = inlen - (inlen % 3);
    mln_uauto_t bulk4 = (inlen / 12) * 12;

    /* Unrolled hot loop: 12 input bytes -> 16 output chars per iteration. */
    while (i < bulk4) {
        mln_u32_t t0 = ((mln_u32_t)in[i     ] << 16)
                     | ((mln_u32_t)in[i +  1] << 8)
                     |  (mln_u32_t)in[i +  2];
        mln_u32_t t1 = ((mln_u32_t)in[i +  3] << 16)
                     | ((mln_u32_t)in[i +  4] << 8)
                     |  (mln_u32_t)in[i +  5];
        mln_u32_t t2 = ((mln_u32_t)in[i +  6] << 16)
                     | ((mln_u32_t)in[i +  7] << 8)
                     |  (mln_u32_t)in[i +  8];
        mln_u32_t t3 = ((mln_u32_t)in[i +  9] << 16)
                     | ((mln_u32_t)in[i + 10] << 8)
                     |  (mln_u32_t)in[i + 11];
        mln_u16_t a = t12[(t0 >> 12) & 0xfff];
        mln_u16_t b = t12[ t0        & 0xfff];
        mln_u16_t c = t12[(t1 >> 12) & 0xfff];
        mln_u16_t d = t12[ t1        & 0xfff];
        mln_u16_t e = t12[(t2 >> 12) & 0xfff];
        mln_u16_t f = t12[ t2        & 0xfff];
        mln_u16_t g = t12[(t3 >> 12) & 0xfff];
        mln_u16_t h = t12[ t3        & 0xfff];
        memcpy(o,      &a, 2);
        memcpy(o +  2, &b, 2);
        memcpy(o +  4, &c, 2);
        memcpy(o +  6, &d, 2);
        memcpy(o +  8, &e, 2);
        memcpy(o + 10, &f, 2);
        memcpy(o + 12, &g, 2);
        memcpy(o + 14, &h, 2);
        o += 16;
        i += 12;
    }

    /* Tail of the 3-byte bulk (0 or 3 bytes left before remainder). */
    while (i < bulk) {
        mln_u32_t t = ((mln_u32_t)in[i] << 16)
                    | ((mln_u32_t)in[i + 1] << 8)
                    |  (mln_u32_t)in[i + 2];
        mln_u16_t hi = t12[(t >> 12) & 0xfff];
        mln_u16_t lo = t12[ t        & 0xfff];
        memcpy(o,     &hi, 2);
        memcpy(o + 2, &lo, 2);
        o += 4;
        i += 3;
    }

    mln_uauto_t rem = inlen - i;
    if (rem == 1) {
        mln_u32_t t = (mln_u32_t)in[i] << 16;
        o[0] = enc[(t >> 18) & 0x3f];
        o[1] = enc[(t >> 12) & 0x3f];
        o[2] = (mln_u8_t)'=';
        o[3] = (mln_u8_t)'=';
        o += 4;
    } else if (rem == 2) {
        mln_u32_t t = ((mln_u32_t)in[i] << 16)
                    | ((mln_u32_t)in[i + 1] << 8);
        o[0] = enc[(t >> 18) & 0x3f];
        o[1] = enc[(t >> 12) & 0x3f];
        o[2] = enc[(t >>  6) & 0x3f];
        o[3] = (mln_u8_t)'=';
        o += 4;
    }

    return (mln_uauto_t)(o - out);
})

/*
 * Fast decode core: decodes inlen base64 characters from in into out.
 * Caller must ensure inlen is a multiple of 4 and that out has room
 * for inlen / 4 * 3 bytes. Returns the number of decoded bytes.
 */
MLN_FUNC(static, mln_uauto_t, mln_base64_decode_core, \
         (mln_u8ptr_t in, mln_uauto_t inlen, mln_u8ptr_t out), \
         (in, inlen, out), \
{
    const mln_u8_t *dec = base64_dec_table;
    mln_u8ptr_t o = out;
    mln_uauto_t i = 0;
    mln_uauto_t bulk = inlen;

    if (bulk >= 4 && in[bulk - 1] == '=') bulk -= 4;

    /*
     * Unrolled hot loop: 8 input chars -> 6 output bytes per iteration.
     *
     * Each 4-char block is assembled into a 24-bit value placed in the
     * high bytes of a 32-bit word, byte-swapped, and stored with a
     * single 4-byte memcpy. The trailing zero byte of the first store
     * is harmlessly overwritten by the second store. We reserve a
     * safety margin so the last 4-byte store always has room.
     */
    while (i + 12 <= bulk) {
        mln_u32_t v0 = ((mln_u32_t)dec[in[i    ]] << 26)
                     | ((mln_u32_t)dec[in[i + 1]] << 20)
                     | ((mln_u32_t)dec[in[i + 2]] << 14)
                     | ((mln_u32_t)dec[in[i + 3]] <<  8);
        mln_u32_t v1 = ((mln_u32_t)dec[in[i + 4]] << 26)
                     | ((mln_u32_t)dec[in[i + 5]] << 20)
                     | ((mln_u32_t)dec[in[i + 6]] << 14)
                     | ((mln_u32_t)dec[in[i + 7]] <<  8);
        v0 = __builtin_bswap32(v0);
        v1 = __builtin_bswap32(v1);
        memcpy(o,     &v0, 4);
        memcpy(o + 3, &v1, 4);
        o += 6;
        i += 8;
    }

    while (i < bulk) {
        mln_u32_t v = ((mln_u32_t)dec[in[i    ]] << 18)
                    | ((mln_u32_t)dec[in[i + 1]] << 12)
                    | ((mln_u32_t)dec[in[i + 2]] <<  6)
                    |  (mln_u32_t)dec[in[i + 3]];
        o[0] = (mln_u8_t)((v >> 16) & 0xff);
        o[1] = (mln_u8_t)((v >>  8) & 0xff);
        o[2] = (mln_u8_t)( v        & 0xff);
        o += 3;
        i += 4;
    }

    if (i < inlen) {
        mln_u32_t v = ((mln_u32_t)dec[in[i    ]] << 18)
                    | ((mln_u32_t)dec[in[i + 1]] << 12);
        if (in[i + 2] == '=') {
            o[0] = (mln_u8_t)((v >> 16) & 0xff);
            o += 1;
        } else {
            v |= ((mln_u32_t)dec[in[i + 2]] << 6);
            if (in[i + 3] == '=') {
                o[0] = (mln_u8_t)((v >> 16) & 0xff);
                o[1] = (mln_u8_t)((v >>  8) & 0xff);
                o += 2;
            } else {
                v |= (mln_u32_t)dec[in[i + 3]];
                o[0] = (mln_u8_t)((v >> 16) & 0xff);
                o[1] = (mln_u8_t)((v >>  8) & 0xff);
                o[2] = (mln_u8_t)( v        & 0xff);
                o += 3;
            }
        }
    }

    return (mln_uauto_t)(o - out);
})

MLN_FUNC(, int, mln_base64_encode, \
         (mln_u8ptr_t in, mln_uauto_t inlen, mln_u8ptr_t *out, mln_uauto_t *outlen), \
         (in, inlen, out, outlen), \
{
    mln_uauto_t need = (inlen / 3) * 4;
    if (inlen % 3) need += 4;

    *out = (mln_u8ptr_t)malloc(need + 1);
    if (*out == NULL) return -1;

    *outlen = mln_base64_encode_core(in, inlen, *out);
    (*out)[*outlen] = 0;
    return 0;
})

MLN_FUNC(, int, mln_base64_pool_encode, \
         (mln_alloc_t *pool, mln_u8ptr_t in, mln_uauto_t inlen, mln_u8ptr_t *out, mln_uauto_t *outlen), \
         (pool, in, inlen, out, outlen), \
{
    mln_uauto_t need = (inlen / 3) * 4;
    if (inlen % 3) need += 4;

    *out = (mln_u8ptr_t)mln_alloc_m(pool, need + 1);
    if (*out == NULL) return -1;

    *outlen = mln_base64_encode_core(in, inlen, *out);
    (*out)[*outlen] = 0;
    return 0;
})

MLN_FUNC(, int, mln_base64_decode, \
         (mln_u8ptr_t in, mln_uauto_t inlen, mln_u8ptr_t *out, mln_uauto_t *outlen), \
         (in, inlen, out, outlen), \
{
    if (inlen % 4) return -1;

    mln_uauto_t need = (inlen / 4) * 3;
    if (inlen >= 2) {
        if (in[inlen - 1] == '=') --need;
        if (in[inlen - 2] == '=') --need;
    }

    *out = (mln_u8ptr_t)malloc(need + 1);
    if (*out == NULL) return -1;

    if (inlen == 0) {
        *outlen = 0;
        (*out)[0] = 0;
        return 0;
    }

    *outlen = mln_base64_decode_core(in, inlen, *out);
    (*out)[*outlen] = 0;
    return 0;
})

MLN_FUNC(, int, mln_base64_pool_decode, \
         (mln_alloc_t *pool, mln_u8ptr_t in, mln_uauto_t inlen, mln_u8ptr_t *out, mln_uauto_t *outlen), \
         (pool, in, inlen, out, outlen), \
{
    if (inlen % 4) return -1;

    mln_uauto_t need = (inlen / 4) * 3;
    if (inlen >= 2) {
        if (in[inlen - 1] == '=') --need;
        if (in[inlen - 2] == '=') --need;
    }

    *out = (mln_u8ptr_t)mln_alloc_m(pool, need + 1);
    if (*out == NULL) return -1;

    if (inlen == 0) {
        *outlen = 0;
        (*out)[0] = 0;
        return 0;
    }

    *outlen = mln_base64_decode_core(in, inlen, *out);
    (*out)[*outlen] = 0;
    return 0;
})

MLN_FUNC_VOID(, void, mln_base64_free, (mln_u8ptr_t data), (data), {
    if (data == NULL) return;
    free(data);
})

MLN_FUNC_VOID(, void, mln_base64_pool_free, (mln_u8ptr_t data), (data), {
    if (data == NULL) return;
    mln_alloc_free(data);
})

