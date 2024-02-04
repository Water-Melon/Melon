
/*
 * Copyright (C) Niklaus F.Schen.
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "mln_aes.h"
#include "mln_func.h"

static inline void mln_aes_addroundkey(mln_u32_t *state, mln_u32_t *roundkey, int round);
static inline void mln_aes_mixcolume(mln_u32_t *state);
static inline void mln_aes_bytesub(mln_u32_t *state);
static inline void mln_aes_shiftrow(mln_u32_t *state);
static inline void mln_aes_invshiftrow(mln_u32_t *state);
static inline void mln_aes_invbytesub(mln_u32_t *state);
static inline void mln_aes_invmixcolume(mln_u32_t *state);

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

#define mln_aes_rotbyte(val) (((val >> 24) & 0xff) | ((val & 0xffffff) << 8))

#define mln_aes_subbyte(val) ((((mln_u32_t)sbox[val & 0xff]) & 0xff) | \
                                 ((((mln_u32_t)sbox[(val >> 8) & 0xff]) & 0xff) << 8) | \
                                 ((((mln_u32_t)sbox[(val >> 16) & 0xff]) & 0xff) << 16) | \
                                 ((((mln_u32_t)sbox[(val >> 24) & 0xff]) & 0xff) << 24))

MLN_FUNC(, int, mln_aes_init, (mln_aes_t *a, mln_u8ptr_t key, mln_u32_t bits), (a, key, bits), {
    int i;
    mln_u32_t nk, times;
    mln_u32_t temp, *roundkey = a->w;

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

MLN_FUNC_VOID(static inline, void, mln_aes_addroundkey, (mln_u32_t *state, mln_u32_t *roundkey, int round), (state, roundkey, round), {
    int i, j;
    mln_u8_t b;
    for (i = 0; i < sizeof(mln_u32_t); ++i) {
        for (j = 0; j < sizeof(mln_u32_t); ++j) {
            b = (state[j] >> ((sizeof(mln_u32_t)-1-i) << 3)) & 0xff;
            state[j] ^= ((mln_u32_t)b) << ((sizeof(mln_u32_t)-1-i) << 3);
            b ^= ((roundkey[round*__MLN_AES_Nb+i] >> ((sizeof(mln_u32_t)-1-j) << 3)) & 0xff);
            state[j] |= ((((mln_u32_t)b) & 0xff) << ((sizeof(mln_u32_t)-1-i) << 3));
        }
    }
})

MLN_FUNC_VOID(static inline, void, mln_aes_mixcolume, (mln_u32_t *state), (state), {
    int i;
    mln_u8_t _0, _1, _2, _3, b;

    for (i = 0; i < 4; ++i) {
        _0 = (state[0] >> ((sizeof(mln_u32_t)-1-i) << 3)) & 0xff;
        _1 = (state[1] >> ((sizeof(mln_u32_t)-1-i) << 3)) & 0xff;
        _2 = (state[2] >> ((sizeof(mln_u32_t)-1-i) << 3)) & 0xff;
        _3 = (state[3] >> ((sizeof(mln_u32_t)-1-i) << 3)) & 0xff;

        state[0] ^= (((mln_u32_t)_0) << ((sizeof(mln_u32_t)-1-i) << 3));
        b = __MLN_AES_MULTIB_02(_0) ^ __MLN_AES_MULTIB_03(_1) ^ __MLN_AES_MULTIB_01(_2) ^ __MLN_AES_MULTIB_01(_3);
        state[0] |= (((mln_u32_t)b) << ((sizeof(mln_u32_t)-1-i) << 3));

        state[1] ^= (((mln_u32_t)_1) << ((sizeof(mln_u32_t)-1-i) << 3));
        b = __MLN_AES_MULTIB_01(_0) ^ __MLN_AES_MULTIB_02(_1) ^ __MLN_AES_MULTIB_03(_2) ^ __MLN_AES_MULTIB_01(_3);
        state[1] |= (((mln_u32_t)b) << ((sizeof(mln_u32_t)-1-i) << 3));

        state[2] ^= (((mln_u32_t)_2) << ((sizeof(mln_u32_t)-1-i) << 3));
        b = __MLN_AES_MULTIB_01(_0) ^ __MLN_AES_MULTIB_01(_1) ^ __MLN_AES_MULTIB_02(_2) ^ __MLN_AES_MULTIB_03(_3);
        state[2] |= (((mln_u32_t)b) << ((sizeof(mln_u32_t)-1-i) << 3));

        state[3] ^= (((mln_u32_t)_3) << ((sizeof(mln_u32_t)-1-i) << 3));
        b = __MLN_AES_MULTIB_03(_0) ^ __MLN_AES_MULTIB_01(_1) ^ __MLN_AES_MULTIB_01(_2) ^ __MLN_AES_MULTIB_02(_3);
        state[3] |= (((mln_u32_t)b) << ((sizeof(mln_u32_t)-1-i) << 3));
    }
})

MLN_FUNC_VOID(static inline, void, mln_aes_bytesub, (mln_u32_t *state), (state), {
    mln_u32_t i, j;
    mln_u8_t b;

    for (i = 0; i < sizeof(mln_u32_t); ++i) {
        for (j = 0; j < sizeof(mln_u32_t); ++j) {
            b = (state[j] >> ((sizeof(mln_u32_t)-1-i) << 3)) & 0xff;
            state[j] ^= ((mln_u32_t)b) << ((sizeof(mln_u32_t)-1-i) << 3);
            state[j] |= ((((mln_u32_t)sbox[b]) & 0xff) << ((sizeof(mln_u32_t)-1-i) << 3));
        }
    }
})

MLN_FUNC_VOID(static inline, void, mln_aes_shiftrow, (mln_u32_t *state), (state), {
    state[1] = (((state[1] << 8) | (state[1] >> 24)) & 0xffffffff);
    state[2] = (((state[2] << 16) | (state[2] >> 16)) & 0xffffffff);
    state[3] = (((state[3] << 24) | (state[3] >> 8)) & 0xffffffff);
})

MLN_FUNC(, int, mln_aes_encrypt, (mln_aes_t *a, mln_u8ptr_t text), (a, text), {
    mln_u32_t state[4] = {0, 0, 0, 0}, i, j, round, nr;

    switch (a->bits) {
        case M_AES_128:
            nr = __MLN_AES128_Nr;
            break;
        case M_AES_192:
            nr = __MLN_AES192_Nr;
            break;
        case M_AES_256:
            nr = __MLN_AES256_Nr;
            break;
        default: return -1;
    }

    for (i = 0; i < sizeof(mln_u32_t); ++i) {
        for (j = 0; j < sizeof(mln_u32_t); ++j) {
            state[j] |= (((mln_u32_t)(text[(i<<2) + j] & 0xff)) << ((sizeof(mln_u32_t)-1-i) << 3));
        }
    }

    mln_aes_addroundkey(state, a->w, 0);

    for (round = 1; round < nr; ++round) {
        mln_aes_bytesub(state);
        mln_aes_shiftrow(state);
        mln_aes_mixcolume(state);
        mln_aes_addroundkey(state, a->w, round);
    }

    mln_aes_bytesub(state);
    mln_aes_shiftrow(state);
    mln_aes_addroundkey(state, a->w, nr);

    for (i = 0; i < sizeof(mln_u32_t); ++i) {
        for (j = 0; j < sizeof(mln_u32_t); ++j) {
            text[(i << 2)+j] = (state[j] >> ((sizeof(mln_u32_t)-1-i) << 3)) & 0xff;
        }
    }

    return 0;
})

MLN_FUNC(, int, mln_aes_decrypt, (mln_aes_t *a, mln_u8ptr_t cipher), (a, cipher), {
    mln_u32_t state[4] = {0, 0, 0, 0}, i, j, round, nr;

    switch (a->bits) {
        case M_AES_128:
            nr = __MLN_AES128_Nr;
            break;
        case M_AES_192:
            nr = __MLN_AES192_Nr;
            break;
        case M_AES_256:
            nr = __MLN_AES256_Nr;
            break;
        default: return -1;
    }

    for (i = 0; i < sizeof(mln_u32_t); ++i) {
        for (j = 0; j < sizeof(mln_u32_t); ++j) {
            state[j] |= (((mln_u32_t)(cipher[(i<<2) + j] & 0xff)) << ((sizeof(mln_u32_t)-1-i) << 3));
        }
    }

    mln_aes_addroundkey(state, a->w, nr);

    for (round = nr-1; round > 0; --round) {
        mln_aes_invshiftrow(state);
        mln_aes_invbytesub(state);
        mln_aes_addroundkey(state, a->w, round);
        mln_aes_invmixcolume(state);
    }

    mln_aes_invshiftrow(state);
    mln_aes_invbytesub(state);
    mln_aes_addroundkey(state, a->w, 0);

    for (i = 0; i < sizeof(mln_u32_t); ++i) {
        for (j = 0; j < sizeof(mln_u32_t); ++j) {
            cipher[(i << 2)+j] = (state[j] >> ((sizeof(mln_u32_t)-1-i) << 3)) & 0xff;
        }
    }

    return 0;
})

MLN_FUNC_VOID(static inline, void, mln_aes_invshiftrow, (mln_u32_t *state), (state), {
    state[1] = (((state[1] >> 8) | (state[1] << 24)) & 0xffffffff);
    state[2] = (((state[2] >> 16) | (state[2] << 16)) & 0xffffffff);
    state[3] = (((state[3] >> 24) | (state[3] << 8)) & 0xffffffff);
})

MLN_FUNC_VOID(static inline, void, mln_aes_invbytesub, (mln_u32_t *state), (state), {
    mln_u32_t i, j;
    mln_u8_t b;

    for (i = 0; i < sizeof(mln_u32_t); ++i) {
        for (j = 0; j < sizeof(mln_u32_t); ++j) {
            b = (state[j] >> ((sizeof(mln_u32_t)-1-i) << 3)) & 0xff;
            state[j] ^= ((mln_u32_t)b) << ((sizeof(mln_u32_t)-1-i) << 3);
            state[j] |= ((((mln_u32_t)rsbox[b]) & 0xff) << ((sizeof(mln_u32_t)-1-i) << 3));
        }
    }
})

MLN_FUNC_VOID(static inline, void, mln_aes_invmixcolume, (mln_u32_t *state), (state), {
    int i;
    mln_u8_t _0, _1, _2, _3;

    for (i = 0; i < 4; ++i) {
        _0 = (state[0] >> ((sizeof(mln_u32_t)-1-i) << 3)) & 0xff;
        _1 = (state[1] >> ((sizeof(mln_u32_t)-1-i) << 3)) & 0xff;
        _2 = (state[2] >> ((sizeof(mln_u32_t)-1-i) << 3)) & 0xff;
        _3 = (state[3] >> ((sizeof(mln_u32_t)-1-i) << 3)) & 0xff;

        state[0] ^= (((mln_u32_t)_0) << ((sizeof(mln_u32_t)-1-i) << 3));
        state[0] |= (((mln_u32_t)(__MLN_AES_MULTIB_0e(_0) ^ __MLN_AES_MULTIB_0b(_1) ^ __MLN_AES_MULTIB_0d(_2) ^ __MLN_AES_MULTIB_09(_3))) << ((sizeof(mln_u32_t)-1-i) << 3));

        state[1] ^= (((mln_u32_t)_1) << ((sizeof(mln_u32_t)-1-i) << 3));
        state[1] |= (((mln_u32_t)(__MLN_AES_MULTIB_09(_0) ^ __MLN_AES_MULTIB_0e(_1) ^ __MLN_AES_MULTIB_0b(_2) ^ __MLN_AES_MULTIB_0d(_3))) << ((sizeof(mln_u32_t)-1-i) << 3));

        state[2] ^= (((mln_u32_t)_2) << ((sizeof(mln_u32_t)-1-i) << 3));
        state[2] |= (((mln_u32_t)(__MLN_AES_MULTIB_0d(_0) ^ __MLN_AES_MULTIB_09(_1) ^ __MLN_AES_MULTIB_0e(_2) ^ __MLN_AES_MULTIB_0b(_3))) << ((sizeof(mln_u32_t)-1-i) << 3));

        state[3] ^= (((mln_u32_t)_3) << ((sizeof(mln_u32_t)-1-i) << 3));
        state[3] |= (((mln_u32_t)(__MLN_AES_MULTIB_0b(_0) ^ __MLN_AES_MULTIB_0d(_1) ^ __MLN_AES_MULTIB_09(_2) ^ __MLN_AES_MULTIB_0e(_3))) << ((sizeof(mln_u32_t)-1-i) << 3));
    }
})

