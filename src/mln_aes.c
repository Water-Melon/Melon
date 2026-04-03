
/*
 * Copyright (C) Niklaus F.Schen.
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "mln_aes.h"
#include "mln_func.h"

static mln_u8_t sbox[] = {
    0x63, 0x7c, 0x77, 0x7b, 0xf2, 0x6b, 0x6f, 0xc5, 0x30, 0x01, 0x67, 0x2b, 0xfe, 0xd7, 0xab, 0x76,
    0xca, 0x82, 0xc9, 0x7d, 0xfa, 0x59, 0x47, 0xf0, 0xad, 0xd4, 0xa2, 0xaf, 0x9c, 0xa4, 0x72, 0xc0,
    0xb7, 0xfd, 0x93, 0x26, 0x36, 0x3f, 0xf7, 0xcc, 0x34, 0xa5, 0xe5, 0xf1, 0x71, 0xd8, 0x31, 0x15,
    0x04, 0xc7, 0x23, 0xc3, 0x18, 0x96, 0x05, 0x9a, 0x07, 0x12, 0x80, 0xe2, 0xeb, 0x27, 0xb2, 0x75,
    0x09, 0x83, 0x2c, 0x1a, 0x1b, 0x6e, 0x5a, 0xa0, 0x52, 0x3b, 0xd6, 0xb3, 0x29, 0xe3, 0x2f, 0x84,
    0x53, 0xd1, 0x00, 0xed, 0x20, 0xfc, 0xb1, 0x5b, 0x6a, 0xcb, 0xbe, 0x39, 0x4a, 0x4c, 0x58, 0xcf,
    0xd0, 0xef, 0xaa, 0xfb, 0x43, 0x4d, 0x33, 0x85, 0x45, 0xf9, 0x02, 0x7f, 0x50, 0x3c, 0x9f, 0xa8,
    0x51, 0xa3, 0x40, 0x8f, 0x92, 0x9d, 0x38, 0xf5, 0xbc, 0xb6, 0xda, 0x21, 0x10, 0xff, 0xf3, 0xd2,
    0xcd, 0x0c, 0x13, 0xec, 0x5f, 0x97, 0x44, 0x17, 0xc4, 0xa7, 0x7e, 0x3d, 0x64, 0x5d, 0x19, 0x73,
    0x60, 0x81, 0x4f, 0xdc, 0x22, 0x2a, 0x90, 0x88, 0x46, 0xee, 0xb8, 0x14, 0xde, 0x5e, 0x0b, 0xdb,
    0xe0, 0x32, 0x3a, 0x0a, 0x49, 0x06, 0x24, 0x5c, 0xc2, 0xd3, 0xac, 0x62, 0x91, 0x95, 0xe4, 0x79,
    0xe7, 0xc8, 0x37, 0x6d, 0x8d, 0xd5, 0x4e, 0xa9, 0x6c, 0x56, 0xf4, 0xea, 0x65, 0x7a, 0xae, 0x08,
    0xba, 0x78, 0x25, 0x2e, 0x1c, 0xa6, 0xb4, 0xc6, 0xe8, 0xdd, 0x74, 0x1f, 0x4b, 0xbd, 0x8b, 0x8a,
    0x70, 0x3e, 0xb5, 0x66, 0x48, 0x03, 0xf6, 0x0e, 0x61, 0x35, 0x57, 0xb9, 0x86, 0xc1, 0x1d, 0x9e,
    0xe1, 0xf8, 0x98, 0x11, 0x69, 0xd9, 0x8e, 0x94, 0x9b, 0x1e, 0x87, 0xe9, 0xce, 0x55, 0x28, 0xdf,
    0x8c, 0xa1, 0x89, 0x0d, 0xbf, 0xe6, 0x42, 0x68, 0x41, 0x99, 0x2d, 0x0f, 0xb0, 0x54, 0xbb, 0x16
};

static mln_u8_t rsbox[] = {
    0x52, 0x09, 0x6a, 0xd5, 0x30, 0x36, 0xa5, 0x38, 0xbf, 0x40, 0xa3, 0x9e, 0x81, 0xf3, 0xd7, 0xfb,
    0x7c, 0xe3, 0x39, 0x82, 0x9b, 0x2f, 0xff, 0x87, 0x34, 0x8e, 0x43, 0x44, 0xc4, 0xde, 0xe9, 0xcb,
    0x54, 0x7b, 0x94, 0x32, 0xa6, 0xc2, 0x23, 0x3d, 0xee, 0x4c, 0x95, 0x0b, 0x42, 0xfa, 0xc3, 0x4e,
    0x08, 0x2e, 0xa1, 0x66, 0x28, 0xd9, 0x24, 0xb2, 0x76, 0x5b, 0xa2, 0x49, 0x6d, 0x8b, 0xd1, 0x25,
    0x72, 0xf8, 0xf6, 0x64, 0x86, 0x68, 0x98, 0x16, 0xd4, 0xa4, 0x5c, 0xcc, 0x5d, 0x65, 0xb6, 0x92,
    0x6c, 0x70, 0x48, 0x50, 0xfd, 0xed, 0xb9, 0xda, 0x5e, 0x15, 0x46, 0x57, 0xa7, 0x8d, 0x9d, 0x84,
    0x90, 0xd8, 0xab, 0x00, 0x8c, 0xbc, 0xd3, 0x0a, 0xf7, 0xe4, 0x58, 0x05, 0xb8, 0xb3, 0x45, 0x06,
    0xd0, 0x2c, 0x1e, 0x8f, 0xca, 0x3f, 0x0f, 0x02, 0xc1, 0xaf, 0xbd, 0x03, 0x01, 0x13, 0x8a, 0x6b,
    0x3a, 0x91, 0x11, 0x41, 0x4f, 0x67, 0xdc, 0xea, 0x97, 0xf2, 0xcf, 0xce, 0xf0, 0xb4, 0xe6, 0x73,
    0x96, 0xac, 0x74, 0x22, 0xe7, 0xad, 0x35, 0x85, 0xe2, 0xf9, 0x37, 0xe8, 0x1c, 0x75, 0xdf, 0x6e,
    0x47, 0xf1, 0x1a, 0x71, 0x1d, 0x29, 0xc5, 0x89, 0x6f, 0xb7, 0x62, 0x0e, 0xaa, 0x18, 0xbe, 0x1b,
    0xfc, 0x56, 0x3e, 0x4b, 0xc6, 0xd2, 0x79, 0x20, 0x9a, 0xdb, 0xc0, 0xfe, 0x78, 0xcd, 0x5a, 0xf4,
    0x1f, 0xdd, 0xa8, 0x33, 0x88, 0x07, 0xc7, 0x31, 0xb1, 0x12, 0x10, 0x59, 0x27, 0x80, 0xec, 0x5f,
    0x60, 0x51, 0x7f, 0xa9, 0x19, 0xb5, 0x4a, 0x0d, 0x2d, 0xe5, 0x7a, 0x9f, 0x93, 0xc9, 0x9c, 0xef,
    0xa0, 0xe0, 0x3b, 0x4d, 0xae, 0x2a, 0xf5, 0xb0, 0xc8, 0xeb, 0xbb, 0x3c, 0x83, 0x53, 0x99, 0x61,
    0x17, 0x2b, 0x04, 0x7e, 0xba, 0x77, 0xd6, 0x26, 0xe1, 0x69, 0x14, 0x63, 0x55, 0x21, 0x0c, 0x7d
};

static mln_u32_t rcon[] = {
    0x0,
    0x01000000,
    0x02000000,
    0x04000000,
    0x08000000,
    0x10000000,
    0x20000000,
    0x40000000,
    0x80000000,
    0x1b000000,
    0x36000000
};

/*
 * T-tables: precomputed SubBytes + MixColumns combined lookup.
 * Te0[x] = { S[x]·2, S[x]·1, S[x]·1, S[x]·3 } as big-endian u32
 * Te1 = RotWord(Te0, 8), Te2 = RotWord(Te0, 16), Te3 = RotWord(Te0, 24)
 * Td0-Td3: same idea with inverse S-box and inverse MixColumns.
 */
static mln_u32_t Te0[256];
static mln_u32_t Te1[256];
static mln_u32_t Te2[256];
static mln_u32_t Te3[256];
static mln_u32_t Td0[256];
static mln_u32_t Td1[256];
static mln_u32_t Td2[256];
static mln_u32_t Td3[256];
static int ttables_inited = 0;

MLN_FUNC(static inline, mln_u8_t, xtime, (mln_u8_t x), (x), \
{
    return (x << 1) ^ (((x >> 7) & 1) * 0x1b);
})

MLN_FUNC(static inline, mln_u8_t, gmul, (mln_u8_t a, mln_u8_t b), (a, b), \
{
    mln_u8_t p = 0;
    int i;
    for (i = 0; i < 8; ++i) {
        if (b & 1) p ^= a;
        a = xtime(a);
        b >>= 1;
    }
    return p;
})

MLN_FUNC_VOID(static, void, mln_aes_init_ttables, (void), (), \
{
    int i;
    for (i = 0; i < 256; ++i) {
        mln_u8_t s = sbox[i];
        mln_u8_t s2 = xtime(s);
        mln_u8_t s3 = s2 ^ s;
        /* Te0[i] = { s·2, s·1, s·1, s·3 } big-endian */
        mln_u32_t t = ((mln_u32_t)s2 << 24) | ((mln_u32_t)s << 16) | ((mln_u32_t)s << 8) | (mln_u32_t)s3;
        Te0[i] = t;
        Te1[i] = (t << 24) | (t >> 8);
        Te2[i] = (t << 16) | (t >> 16);
        Te3[i] = (t << 8)  | (t >> 24);
    }
    for (i = 0; i < 256; ++i) {
        mln_u8_t s = rsbox[i];
        mln_u8_t se = gmul(s, 0x0e);
        mln_u8_t s9 = gmul(s, 0x09);
        mln_u8_t sd = gmul(s, 0x0d);
        mln_u8_t sb = gmul(s, 0x0b);
        /* Td0[i] = { RS[i]·0e, RS[i]·09, RS[i]·0d, RS[i]·0b } */
        mln_u32_t t = ((mln_u32_t)se << 24) | ((mln_u32_t)s9 << 16) | ((mln_u32_t)sd << 8) | (mln_u32_t)sb;
        Td0[i] = t;
        Td1[i] = (t << 24) | (t >> 8);
        Td2[i] = (t << 16) | (t >> 16);
        Td3[i] = (t << 8)  | (t >> 24);
    }
    ttables_inited = 1;
})

#define mln_aes_rotbyte(val) (((val >> 24) & 0xff) | ((val & 0xffffff) << 8))

#define mln_aes_subbyte(val) ((((mln_u32_t)sbox[val & 0xff]) & 0xff) | \
                                 ((((mln_u32_t)sbox[(val >> 8) & 0xff]) & 0xff) << 8) | \
                                 ((((mln_u32_t)sbox[(val >> 16) & 0xff]) & 0xff) << 16) | \
                                 ((((mln_u32_t)sbox[(val >> 24) & 0xff]) & 0xff) << 24))

MLN_FUNC(, int, mln_aes_init, (mln_aes_t *a, mln_u8ptr_t key, mln_u32_t bits), (a, key, bits), {
    int i;
    mln_u32_t nk, times;
    mln_u32_t temp, *roundkey = a->w;

    if (!ttables_inited) mln_aes_init_ttables();

    switch (bits) {
        case M_AES_128:
            nk = __MLN_AES128_Nk;
            times = (__MLN_AES128_Nr + 1) * 4;
            break;
        case M_AES_192:
            nk = __MLN_AES192_Nk;
            times = (__MLN_AES192_Nr + 1) * 4;
            break;
        case M_AES_256:
            nk = __MLN_AES256_Nk;
            times = (__MLN_AES256_Nr + 1) * 4;
            break;
        default: return -1;
    }

    for (i = 0; i < nk; ++i) {
        roundkey[i] = 0;
        roundkey[i] |= ((((mln_u32_t)key[i<<2] & 0xff) << 24) | (((mln_u32_t)key[(i<<2)+1] & 0xff) << 16) | \
                        (((mln_u32_t)key[(i<<2)+2] & 0xff) << 8) | ((mln_u32_t)key[(i<<2)+3] & 0xff));
    }
    for (; i < times; ++i) {
        temp = roundkey[i-1];
        if (i % nk == 0) {
            temp = mln_aes_subbyte(mln_aes_rotbyte(temp)) ^ rcon[i / nk];
        } else if (bits == M_AES_256 && (i % nk == 4)) {
            temp = mln_aes_subbyte(temp);
        }
        roundkey[i] = roundkey[i - nk] ^ temp;
    }

    for (; i < 60; ++i) roundkey[i] = 0;

    a->bits = bits;

    /* Precompute decryption round keys: apply InvMixColumns to middle round keys */
    {
        mln_u32_t nr_val;
        int r;
        switch (bits) {
            case M_AES_128: nr_val = __MLN_AES128_Nr; break;
            case M_AES_192: nr_val = __MLN_AES192_Nr; break;
            default:        nr_val = __MLN_AES256_Nr; break;
        }
        /* First and last round keys are used as-is */
        for (i = 0; i < 4; ++i) {
            a->dw[i] = roundkey[i];
            a->dw[nr_val * 4 + i] = roundkey[nr_val * 4 + i];
        }
        /* Middle round keys get InvMixColumns applied */
        for (r = 1; r < (int)nr_val; ++r) {
            for (i = 0; i < 4; ++i) {
                mln_u32_t w = roundkey[r * 4 + i];
                mln_u8_t b0 = (w >> 24) & 0xff;
                mln_u8_t b1 = (w >> 16) & 0xff;
                mln_u8_t b2 = (w >> 8) & 0xff;
                mln_u8_t b3 = w & 0xff;
                a->dw[r * 4 + i] =
                    ((mln_u32_t)(gmul(b0, 0x0e) ^ gmul(b1, 0x0b) ^ gmul(b2, 0x0d) ^ gmul(b3, 0x09)) << 24) |
                    ((mln_u32_t)(gmul(b0, 0x09) ^ gmul(b1, 0x0e) ^ gmul(b2, 0x0b) ^ gmul(b3, 0x0d)) << 16) |
                    ((mln_u32_t)(gmul(b0, 0x0d) ^ gmul(b1, 0x09) ^ gmul(b2, 0x0e) ^ gmul(b3, 0x0b)) << 8)  |
                    ((mln_u32_t)(gmul(b0, 0x0b) ^ gmul(b1, 0x0d) ^ gmul(b2, 0x09) ^ gmul(b3, 0x0e)));
            }
        }
        /* Zero out remaining dw entries */
        for (i = (nr_val + 1) * 4; i < 60; ++i) a->dw[i] = 0;
    }

    return 0;
})

MLN_FUNC(, mln_aes_t *, mln_aes_new, (mln_u8ptr_t key, mln_u32_t bits), (key, bits), {
    mln_aes_t *a = (mln_aes_t *)malloc(sizeof(mln_aes_t));
    if (a == NULL) return NULL;
    mln_aes_init(a, key, bits);
    return a;
})

MLN_FUNC(, mln_aes_t *, mln_aes_pool_new, (mln_alloc_t *pool, mln_u8ptr_t key, mln_u32_t bits), (pool, key, bits), {
    mln_aes_t *a = (mln_aes_t *)mln_alloc_m(pool, sizeof(mln_aes_t));
    if (a == NULL) return NULL;
    mln_aes_init(a, key, bits);
    return a;
})

MLN_FUNC_VOID(, void, mln_aes_free, (mln_aes_t *a), (a), {
    if (a == NULL) return;
    free(a);
})

MLN_FUNC_VOID(, void, mln_aes_pool_free, (mln_aes_t *a), (a), {
    if (a == NULL) return;
    mln_alloc_free(a);
})

/*
 * T-table based AES encrypt.
 * State is column-major: state[c] = col c of AES state, big-endian.
 * This matches the round key layout w[c] directly, so AddRoundKey is a simple XOR.
 * Each inner round combines SubBytes + ShiftRows + MixColumns via T-table lookups.
 */
MLN_FUNC(, int, mln_aes_encrypt, (mln_aes_t *a, mln_u8ptr_t text), (a, text), {
    mln_u32_t s0, s1, s2, s3;
    mln_u32_t t0, t1, t2, t3;
    mln_u32_t nr, round;
    mln_u32_t *rk = a->w;

    switch (a->bits) {
        case M_AES_128: nr = __MLN_AES128_Nr; break;
        case M_AES_192: nr = __MLN_AES192_Nr; break;
        case M_AES_256: nr = __MLN_AES256_Nr; break;
        default: return -1;
    }

    /* Load state as columns (big-endian), XOR with round key 0 */
    s0 = (((mln_u32_t)text[ 0] << 24) | ((mln_u32_t)text[ 1] << 16) | ((mln_u32_t)text[ 2] << 8) | (mln_u32_t)text[ 3]) ^ rk[0];
    s1 = (((mln_u32_t)text[ 4] << 24) | ((mln_u32_t)text[ 5] << 16) | ((mln_u32_t)text[ 6] << 8) | (mln_u32_t)text[ 7]) ^ rk[1];
    s2 = (((mln_u32_t)text[ 8] << 24) | ((mln_u32_t)text[ 9] << 16) | ((mln_u32_t)text[10] << 8) | (mln_u32_t)text[11]) ^ rk[2];
    s3 = (((mln_u32_t)text[12] << 24) | ((mln_u32_t)text[13] << 16) | ((mln_u32_t)text[14] << 8) | (mln_u32_t)text[15]) ^ rk[3];

    /* Inner rounds: SubBytes + ShiftRows + MixColumns via T-tables */
    for (round = 1; round < nr; ++round) {
        rk += 4;
        t0 = Te0[(s0 >> 24) & 0xff] ^ Te1[(s1 >> 16) & 0xff] ^ Te2[(s2 >> 8) & 0xff] ^ Te3[s3 & 0xff] ^ rk[0];
        t1 = Te0[(s1 >> 24) & 0xff] ^ Te1[(s2 >> 16) & 0xff] ^ Te2[(s3 >> 8) & 0xff] ^ Te3[s0 & 0xff] ^ rk[1];
        t2 = Te0[(s2 >> 24) & 0xff] ^ Te1[(s3 >> 16) & 0xff] ^ Te2[(s0 >> 8) & 0xff] ^ Te3[s1 & 0xff] ^ rk[2];
        t3 = Te0[(s3 >> 24) & 0xff] ^ Te1[(s0 >> 16) & 0xff] ^ Te2[(s1 >> 8) & 0xff] ^ Te3[s2 & 0xff] ^ rk[3];
        s0 = t0; s1 = t1; s2 = t2; s3 = t3;
    }

    /* Last round: SubBytes + ShiftRows (no MixColumns) */
    rk += 4;
    t0 = ((mln_u32_t)sbox[(s0 >> 24) & 0xff] << 24) | ((mln_u32_t)sbox[(s1 >> 16) & 0xff] << 16) |
         ((mln_u32_t)sbox[(s2 >> 8) & 0xff] << 8)   |  (mln_u32_t)sbox[s3 & 0xff];
    t1 = ((mln_u32_t)sbox[(s1 >> 24) & 0xff] << 24) | ((mln_u32_t)sbox[(s2 >> 16) & 0xff] << 16) |
         ((mln_u32_t)sbox[(s3 >> 8) & 0xff] << 8)   |  (mln_u32_t)sbox[s0 & 0xff];
    t2 = ((mln_u32_t)sbox[(s2 >> 24) & 0xff] << 24) | ((mln_u32_t)sbox[(s3 >> 16) & 0xff] << 16) |
         ((mln_u32_t)sbox[(s0 >> 8) & 0xff] << 8)   |  (mln_u32_t)sbox[s1 & 0xff];
    t3 = ((mln_u32_t)sbox[(s3 >> 24) & 0xff] << 24) | ((mln_u32_t)sbox[(s0 >> 16) & 0xff] << 16) |
         ((mln_u32_t)sbox[(s1 >> 8) & 0xff] << 8)   |  (mln_u32_t)sbox[s2 & 0xff];
    s0 = t0 ^ rk[0]; s1 = t1 ^ rk[1]; s2 = t2 ^ rk[2]; s3 = t3 ^ rk[3];

    /* Store state back as bytes (big-endian columns) */
    text[ 0] = (s0 >> 24) & 0xff; text[ 1] = (s0 >> 16) & 0xff; text[ 2] = (s0 >> 8) & 0xff; text[ 3] = s0 & 0xff;
    text[ 4] = (s1 >> 24) & 0xff; text[ 5] = (s1 >> 16) & 0xff; text[ 6] = (s1 >> 8) & 0xff; text[ 7] = s1 & 0xff;
    text[ 8] = (s2 >> 24) & 0xff; text[ 9] = (s2 >> 16) & 0xff; text[10] = (s2 >> 8) & 0xff; text[11] = s2 & 0xff;
    text[12] = (s3 >> 24) & 0xff; text[13] = (s3 >> 16) & 0xff; text[14] = (s3 >> 8) & 0xff; text[15] = s3 & 0xff;

    return 0;
})

/*
 * T-table based AES decrypt.
 * InvShiftRows: row 1 shifts right 1, row 2 shifts right 2, row 3 shifts right 3.
 * For column c: byte from row 0 comes from col c, row 1 from col (c+3)%4,
 *               row 2 from col (c+2)%4, row 3 from col (c+1)%4.
 */
MLN_FUNC(, int, mln_aes_decrypt, (mln_aes_t *a, mln_u8ptr_t cipher), (a, cipher), {
    mln_u32_t s0, s1, s2, s3;
    mln_u32_t t0, t1, t2, t3;
    mln_u32_t nr, round;
    mln_u32_t *rk;

    switch (a->bits) {
        case M_AES_128: nr = __MLN_AES128_Nr; break;
        case M_AES_192: nr = __MLN_AES192_Nr; break;
        case M_AES_256: nr = __MLN_AES256_Nr; break;
        default: return -1;
    }

    rk = a->dw + nr * 4;

    /* Load state as columns (big-endian), XOR with last round key */
    s0 = (((mln_u32_t)cipher[ 0] << 24) | ((mln_u32_t)cipher[ 1] << 16) | ((mln_u32_t)cipher[ 2] << 8) | (mln_u32_t)cipher[ 3]) ^ rk[0];
    s1 = (((mln_u32_t)cipher[ 4] << 24) | ((mln_u32_t)cipher[ 5] << 16) | ((mln_u32_t)cipher[ 6] << 8) | (mln_u32_t)cipher[ 7]) ^ rk[1];
    s2 = (((mln_u32_t)cipher[ 8] << 24) | ((mln_u32_t)cipher[ 9] << 16) | ((mln_u32_t)cipher[10] << 8) | (mln_u32_t)cipher[11]) ^ rk[2];
    s3 = (((mln_u32_t)cipher[12] << 24) | ((mln_u32_t)cipher[13] << 16) | ((mln_u32_t)cipher[14] << 8) | (mln_u32_t)cipher[15]) ^ rk[3];

    /* Inner rounds: InvSubBytes + InvShiftRows + InvMixColumns via Td-tables */
    for (round = nr - 1; round > 0; --round) {
        rk -= 4;
        t0 = Td0[(s0 >> 24) & 0xff] ^ Td1[(s3 >> 16) & 0xff] ^ Td2[(s2 >> 8) & 0xff] ^ Td3[s1 & 0xff] ^ rk[0];
        t1 = Td0[(s1 >> 24) & 0xff] ^ Td1[(s0 >> 16) & 0xff] ^ Td2[(s3 >> 8) & 0xff] ^ Td3[s2 & 0xff] ^ rk[1];
        t2 = Td0[(s2 >> 24) & 0xff] ^ Td1[(s1 >> 16) & 0xff] ^ Td2[(s0 >> 8) & 0xff] ^ Td3[s3 & 0xff] ^ rk[2];
        t3 = Td0[(s3 >> 24) & 0xff] ^ Td1[(s2 >> 16) & 0xff] ^ Td2[(s1 >> 8) & 0xff] ^ Td3[s0 & 0xff] ^ rk[3];
        s0 = t0; s1 = t1; s2 = t2; s3 = t3;
    }

    /* Last round: InvSubBytes + InvShiftRows (no InvMixColumns) */
    rk -= 4;
    t0 = ((mln_u32_t)rsbox[(s0 >> 24) & 0xff] << 24) | ((mln_u32_t)rsbox[(s3 >> 16) & 0xff] << 16) |
         ((mln_u32_t)rsbox[(s2 >> 8) & 0xff] << 8)   |  (mln_u32_t)rsbox[s1 & 0xff];
    t1 = ((mln_u32_t)rsbox[(s1 >> 24) & 0xff] << 24) | ((mln_u32_t)rsbox[(s0 >> 16) & 0xff] << 16) |
         ((mln_u32_t)rsbox[(s3 >> 8) & 0xff] << 8)   |  (mln_u32_t)rsbox[s2 & 0xff];
    t2 = ((mln_u32_t)rsbox[(s2 >> 24) & 0xff] << 24) | ((mln_u32_t)rsbox[(s1 >> 16) & 0xff] << 16) |
         ((mln_u32_t)rsbox[(s0 >> 8) & 0xff] << 8)   |  (mln_u32_t)rsbox[s3 & 0xff];
    t3 = ((mln_u32_t)rsbox[(s3 >> 24) & 0xff] << 24) | ((mln_u32_t)rsbox[(s2 >> 16) & 0xff] << 16) |
         ((mln_u32_t)rsbox[(s1 >> 8) & 0xff] << 8)   |  (mln_u32_t)rsbox[s0 & 0xff];
    s0 = t0 ^ rk[0]; s1 = t1 ^ rk[1]; s2 = t2 ^ rk[2]; s3 = t3 ^ rk[3];

    /* Store state back as bytes */
    cipher[ 0] = (s0 >> 24) & 0xff; cipher[ 1] = (s0 >> 16) & 0xff; cipher[ 2] = (s0 >> 8) & 0xff; cipher[ 3] = s0 & 0xff;
    cipher[ 4] = (s1 >> 24) & 0xff; cipher[ 5] = (s1 >> 16) & 0xff; cipher[ 6] = (s1 >> 8) & 0xff; cipher[ 7] = s1 & 0xff;
    cipher[ 8] = (s2 >> 24) & 0xff; cipher[ 9] = (s2 >> 16) & 0xff; cipher[10] = (s2 >> 8) & 0xff; cipher[11] = s2 & 0xff;
    cipher[12] = (s3 >> 24) & 0xff; cipher[13] = (s3 >> 16) & 0xff; cipher[14] = (s3 >> 8) & 0xff; cipher[15] = s3 & 0xff;

    return 0;
})
