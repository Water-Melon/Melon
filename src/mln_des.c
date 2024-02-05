
/*
 * Copyright (C) Niklaus F.Schen.
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "mln_des.h"
#include "mln_func.h"

static inline void mln_des_begin_permute(mln_u64_t *msg);
static inline void mln_des_extension_permute(mln_u64_t *right);
static inline void mln_des_s_permute(mln_u64_t *right);
static inline void mln_des_p_permute(mln_u64_t *right);
static inline void mln_des_final_permute(mln_u64_t *msg);

static mln_u8_t begin_permutation[] = {
    57, 49, 41, 33, 25, 17, 9, 1, 59, 51, 43, 35, 27, 19, 11, 3,
    61, 53, 45, 37, 29, 21, 13, 5, 63, 55, 47, 39, 31, 23, 15, 7,
    56, 48, 40, 32, 24, 16, 8, 0, 58, 50, 42, 34, 26, 18, 10, 2,
    60, 52, 44, 36, 28, 20, 12, 4, 62, 54, 46, 38, 30, 22, 14, 6
};

static mln_u8_t key_permutation[] = {
    56, 48, 40, 32, 24, 16, 8, 0, 57, 49, 41, 33, 25, 17,
    9, 1, 58, 50, 42, 34, 26, 18, 10, 2, 59, 51, 43, 35,
    62, 54, 46, 38, 30, 22, 14, 6, 61, 53, 45, 37, 29, 21,
    13, 5, 60, 52, 44, 36, 28, 20, 12, 4, 27, 19, 11, 3
};

static mln_u8_t move_times[] = {
    1, 1, 2, 2, 2, 2, 2, 2, 1, 2, 2, 2, 2, 2, 2, 1
};

static mln_u8_t compression_permutation[] = {
    13, 16, 10, 23, 0, 4, 2, 27, 14, 5, 20, 9,
    22, 18, 11, 3, 25, 7, 15, 6, 26, 19, 12, 1,
    40, 51, 30, 36, 46, 54, 29, 39, 50, 44, 32, 47,
    43, 48, 38, 55, 33, 52, 45, 41, 49, 35, 28, 31
};

static mln_u8_t extension_permutation[] = {
    31, 0, 1, 2, 3, 4, 3, 4, 5, 6, 7, 8,
    7, 8, 9, 10, 11, 12, 11, 12, 13, 14, 15, 16,
    15, 16, 17, 18, 19, 20, 19, 20, 21, 22, 23, 24,
    23, 24, 25, 26, 27, 28, 27, 28, 29, 30, 31, 0
};

static mln_u8_t s_box[8][64] = {
    {
    14, 4, 13, 1, 2, 15, 11, 8, 3, 10, 6, 12, 5, 9, 0, 7,
    0, 15, 7, 4, 14, 2, 13, 1, 10, 6, 12, 11, 9, 5, 3, 8,
    4, 1, 14, 8, 13, 6, 2, 11, 15, 12, 9, 7, 3, 10, 5, 0,
    15, 12, 8, 2, 4, 9, 1, 7, 5, 11, 3, 14, 10, 0, 6, 13
    },
    {
    15, 1, 8, 14, 6, 11, 3, 4, 9, 7, 2, 13, 12, 0, 5, 10,
    3, 13, 4, 7, 15, 2, 8, 14, 12, 0, 1, 10, 6, 9, 11, 5,
    0, 14, 7, 11, 10, 4, 13, 1, 5, 8, 12, 6, 9, 3, 2, 15,
    13, 8, 10, 1, 3, 15, 4, 2, 11, 6, 7, 12, 0, 5, 14, 9
    },
    {
    10, 0, 9, 14, 6, 3, 15, 5, 1, 13, 12, 7, 11, 4, 2, 8,
    13, 7, 0, 9, 3, 4, 6, 10, 2, 8, 5, 14, 12, 11, 15, 1,
    13, 6, 4, 9, 8, 15, 3, 0, 11, 1, 2, 12, 5, 10, 14, 7,
    1, 10, 13, 0, 6, 9, 8, 7, 4, 15, 14, 3, 11, 5, 2, 12
    },
    {
    7, 13, 14, 3, 0, 6, 9, 10, 1, 2, 8, 5, 11, 12, 4, 15,
    13, 8, 11, 5, 6, 15, 0, 3, 4, 7, 2, 12, 1, 10, 14, 9,
    10, 6, 9, 0, 12, 11, 7, 13, 15, 1, 3, 14, 5, 2, 8, 4,
    3, 15, 0, 6, 10, 1, 13, 8, 9, 4, 5, 11, 12, 7, 2, 14
    },
    {
    2, 12, 4, 1, 7, 10, 11, 6, 8, 5, 3, 15, 13, 0, 14, 9,
    14, 11, 2, 12, 4, 7, 13, 1, 5, 0, 15, 10, 3, 9, 8, 6,
    4, 2, 1, 11, 10, 13, 7, 8, 15, 9, 12, 5, 6, 3, 0, 14,
    11, 8, 12, 7, 1, 14, 2, 13, 6, 15, 0, 9, 10, 4, 5, 3
    },
    {
    12, 1, 10, 15, 9, 2, 6, 8, 0, 13, 3, 4, 14, 7, 5, 11,
    10, 15, 4, 2, 7, 12, 9, 5, 6, 1, 13, 14, 0, 11, 3, 8,
    9, 14, 15, 5, 2, 8, 12, 3, 7, 0, 4, 10, 1, 13, 11, 6,
    4, 3, 2, 12, 9, 5, 15, 10, 11, 14, 1, 7, 6, 0, 8, 13
    },
    {
    4, 11, 2, 14, 15, 0, 8, 13, 3, 12, 9, 7, 5, 10, 6, 1,
    13, 0, 11, 7, 4, 9, 1, 10, 14, 3, 5, 12, 2, 15, 8, 6,
    1, 4, 11, 13, 12, 3, 7, 14, 10, 15, 6, 8, 0, 5, 9, 2,
    6, 11, 13, 8, 1, 4, 10, 7, 9, 5, 0, 15, 14, 2, 3, 12
    },
    {
    13, 2, 8, 4, 6, 15, 11, 1, 10, 9, 3, 14, 5, 0, 12, 7,
    1, 15, 13, 8, 10, 3, 7, 4, 12, 5, 6, 11, 0, 14, 9, 2,
    7, 11, 4, 1, 9, 12, 14, 2, 0, 6, 10, 13, 15, 3, 5, 8,
    2, 1, 14, 7, 4, 10, 8, 13, 15, 12, 9, 0, 3, 5, 6, 11
    }
};

static mln_u8_t p_permutation[] = {
    15, 6, 19, 20, 28, 11, 27, 16, 0, 14, 22, 25, 4, 17, 30, 9,
    1, 7, 23, 13, 31, 26, 2, 8, 18, 12, 29, 5, 21, 10, 3, 24
};

static mln_u8_t final_permutation[] = {
    39, 7, 47, 15, 55, 23, 63, 31, 38, 6, 46, 14, 54, 22, 62, 30,
    37, 5, 45, 13, 53, 21, 61, 29, 36, 4, 44, 12, 52, 20, 60, 28,
    35, 3, 43, 11, 51, 19, 59, 27, 34, 2, 42, 10, 50, 18, 58, 26,
    33, 1, 41, 9, 49, 17, 57, 25, 32, 0, 40, 8, 48, 16, 56, 24
};


MLN_FUNC_VOID(, void, mln_des_init, (mln_des_t *d, mln_u64_t key), (d, key), {
    mln_u64_t _56key = 0, _h28key, _l28key;
    mln_s32_t i, j;
    mln_u8ptr_t scan, end = key_permutation + sizeof(key_permutation);

    d->key = key;

    for (i = 55, scan = key_permutation; scan < end; ++scan, --i) {
        _56key |= (((key >> (63 - (*scan))) & 0x1) << i);
    }

    for (j = 0; j < 16; ++j) {
        _h28key = (_56key >> 28) & 0xfffffff;
        _l28key = _56key & 0xfffffff;
        _h28key = __M_DES_ROL28(_h28key, move_times[j]);
        _l28key = __M_DES_ROL28(_l28key, move_times[j]);
        _56key = ((_h28key << 28) | _l28key) & 0xffffffffffffffllu;
        end = compression_permutation + sizeof(compression_permutation);
        d->sub_keys[j] = 0;
        for (i = 47, scan = compression_permutation; scan < end; ++scan, --i) {
            d->sub_keys[j] |= (((_56key >> (55 - (*scan))) & 0x1) << i);
        }
        d->sub_keys[j] &= 0xffffffffffffllu;
    }
})

MLN_FUNC(, mln_des_t *, mln_des_new, (mln_u64_t key), (key), {
    mln_des_t *d = (mln_des_t *)malloc(sizeof(mln_des_t));
    if (d == NULL) return NULL;
    mln_des_init(d, key);
    return d;
})

MLN_FUNC(, mln_des_t *, mln_des_pool_new, (mln_alloc_t *pool, mln_u64_t key), (pool, key), {
    mln_des_t *d = (mln_des_t *)mln_alloc_m(pool, sizeof(mln_des_t));
    if (d == NULL) return NULL;
    mln_des_init(d, key);
    return d;
})

MLN_FUNC_VOID(, void, mln_des_free, (mln_des_t *d), (d), {
    if (d == NULL) return;
    free(d);
})

MLN_FUNC_VOID(, void, mln_des_pool_free, (mln_des_t *d), (d), {
    if (d == NULL) return;
    mln_alloc_free(d);
})

MLN_FUNC(, mln_u64_t, mln_des, (mln_des_t *d, mln_u64_t msg, mln_u32_t is_encrypt), (d, msg, is_encrypt), {
    mln_s32_t i;
    mln_u64_t left = 0, right = 0, tmp;

    mln_des_begin_permute(&msg);

    left = (msg >> 32) & 0xffffffff;
    right = msg & 0xffffffff;
    if (is_encrypt) {
        for (i = 0; i < 16; ++i) {
            tmp = right;
            mln_des_extension_permute(&right);
            right ^= d->sub_keys[i];
            mln_des_s_permute(&right);
            mln_des_p_permute(&right);
            right ^= left;
            left = tmp;
        }
    } else {
        for (i = 15; i >= 0; --i) {
            tmp = right;
            mln_des_extension_permute(&right);
            right ^= d->sub_keys[i];
            mln_des_s_permute(&right);
            mln_des_p_permute(&right);
            right ^= left;
            left = tmp;
        }
    }
    msg = ((right & 0xffffffff) << 32) | (left & 0xffffffff);

    mln_des_final_permute(&msg);
    return msg;
})

MLN_FUNC_VOID(static inline, void, mln_des_begin_permute, (mln_u64_t *msg), (msg), {
    mln_u64_t dup = *msg, ret = 0;
    mln_s32_t i;
    mln_u8_t *scan, *end = begin_permutation + sizeof(begin_permutation);

    for (i = 63, scan = begin_permutation; scan < end; ++scan, --i) {
        ret |= (((dup >> (63 - (*scan))) & 0x1) << i);
    }

    *msg = ret;
})

MLN_FUNC_VOID(static inline, void, mln_des_extension_permute, (mln_u64_t *right), (right), {
    mln_u64_t dup = *right, ret = 0;
    mln_s32_t i;
    mln_u8_t *scan, *end = extension_permutation + sizeof(extension_permutation);

    for (i = 47, scan = extension_permutation; scan < end; ++scan, --i) {
        ret |= (((dup >> (31 - (*scan))) & 0x1) << i);
    }

    *right = ret & 0xffffffffffffllu;
})

MLN_FUNC_VOID(static inline, void, mln_des_s_permute, (mln_u64_t *right), (right), {
    mln_u64_t dup = *right, ret = 0, _64tmp;
    mln_u32_t i = 0, index, _32tmp;

    for (i = 0; i < 8; ++i) {
        _64tmp = (dup >> (i*6)) & 0x3f;
        _32tmp = (_64tmp & 0x1) | (((_64tmp >> 5) & 0x1) << 1);
        index = ((_64tmp >> 1) & 0xf) + (_32tmp << 4);
        ret |= ((mln_u64_t)(s_box[7 - i][index] & 0xf) << (i << 2));
    }

    *right = ret & 0xffffffff;
})

MLN_FUNC_VOID(static inline, void, mln_des_p_permute, (mln_u64_t *right), (right), {
    mln_u64_t dup = *right, ret = 0;
    mln_s32_t i;
    mln_u8_t *scan, *end = p_permutation + sizeof(p_permutation);

    for (i = 31, scan = p_permutation; scan < end; ++scan, --i) {
        ret |= (((dup >> (31 - (*scan))) & 0x1) << i);
    }
    *right = ret & 0xffffffff;
})

MLN_FUNC_VOID(static inline, void, mln_des_final_permute, (mln_u64_t *msg), (msg), {
    mln_u64_t dup = *msg, ret = 0;
    mln_u32_t i = 0;
    mln_u8_t *scan, *end = final_permutation + sizeof(final_permutation);

    for (scan = final_permutation; scan < end; ++scan, ++i) {
        ret |= (((dup >> (*scan)) & 0x1) << i);
    }

    *msg = ret;
})

MLN_FUNC_VOID(, void, mln_des_buf, \
              (mln_des_t *d, \
               mln_u8ptr_t in, mln_uauto_t inlen, mln_u8ptr_t out, mln_uauto_t outlen, \
               mln_u8_t fill, mln_u32_t is_encrypt), \
              (d, in, inlen, out, outlen, fill, is_encrypt), \
{
    mln_uauto_t i = 0;
    mln_u64_t input, output;

    while (inlen) {
        input = 0;
        for (i = 0; i < sizeof(mln_u64_t) && inlen > 0; ++i, --inlen, ++in) {
            input |= ((((mln_u64_t)(*in)) & 0xff) << ((sizeof(mln_u64_t)-1-i) << 3));
        }
        if (i < sizeof(mln_u64_t)) {
            for (; i < sizeof(mln_u64_t); ++i) {
                input |= ((((mln_u64_t)fill) & 0xff) << ((sizeof(mln_u64_t)-1-i) << 3));
            }
        }
        output = mln_des(d, input, is_encrypt);

        for (i = 0; i < sizeof(output); ++i, --outlen, ++out) {
            if (outlen == 0) return;

            *out = (output >> ((sizeof(mln_u64_t)-1-i) << 3)) & 0xff;
        }
    }
})

/*
 * 3DES
 */
MLN_FUNC_VOID(, void, mln_3des_init, (mln_3des_t *tdes, mln_u64_t key1, mln_u64_t key2), (tdes, key1, key2), {
    mln_des_init(&(tdes->_1key), key1);
    mln_des_init(&(tdes->_2key), key2);
})

MLN_FUNC(, mln_3des_t *, mln_3des_new, (mln_u64_t key1, mln_u64_t key2), (key1, key2), {
    mln_3des_t *_3d = (mln_3des_t *)malloc(sizeof(mln_3des_t));
    if (_3d == NULL) return NULL;
    mln_3des_init(_3d, key1, key2);
    return _3d;
})

MLN_FUNC(, mln_3des_t *, mln_3des_pool_new, \
         (mln_alloc_t *pool, mln_u64_t key1, mln_u64_t key2), \
         (pool, key1, key2), \
{
    mln_3des_t *_3d = (mln_3des_t *)mln_alloc_m(pool, sizeof(mln_3des_t));
    if (_3d == NULL) return NULL;
    mln_3des_init(_3d, key1, key2);
    return _3d;
})

MLN_FUNC_VOID(, void, mln_3des_free, (mln_3des_t *tdes), (tdes), {
    if (tdes == NULL) return;
    free(tdes);
})

MLN_FUNC_VOID(, void, mln_3des_pool_free, (mln_3des_t *tdes), (tdes), {
    if (tdes == NULL) return;
    mln_alloc_free(tdes);
})

MLN_FUNC(, mln_u64_t, mln_3des, \
         (mln_3des_t *tdes, mln_u64_t msg, mln_u32_t is_encrypt), \
         (tdes, msg, is_encrypt), \
{
    if (is_encrypt)
        return mln_des(&(tdes->_1key), mln_des(&(tdes->_2key), mln_des(&(tdes->_1key), msg, 1), 0), 1);
    return mln_des(&(tdes->_1key), mln_des(&(tdes->_2key), mln_des(&(tdes->_1key), msg, 0), 1), 0);
})

MLN_FUNC_VOID(, void, mln_3des_buf, \
              (mln_3des_t *tdes, mln_u8ptr_t in, mln_uauto_t inlen, \
               mln_u8ptr_t out, mln_uauto_t outlen, mln_u8_t fill, mln_u32_t is_encrypt), \
              (tdes, in, inlen, out, outlen, fill, is_encrypt), \
{
    mln_uauto_t i = 0;
    mln_u64_t input, output;

    while (inlen) {
        input = 0;
        for (i = 0; i < sizeof(mln_u64_t) && inlen > 0; ++i, --inlen, ++in) {
            input |= ((((mln_u64_t)(*in)) & 0xff) << ((sizeof(mln_u64_t)-1-i) << 3));
        }
        if (i < sizeof(mln_u64_t)) {
            for (; i < sizeof(mln_u64_t); ++i) {
                input |= ((((mln_u64_t)fill) & 0xff) << ((sizeof(mln_u64_t)-1-i) << 3));
            }
        }
        output = mln_3des(tdes, input, is_encrypt);

        for (i = 0; i < sizeof(output); ++i, --outlen, ++out) {
            if (outlen == 0) return;

            *out = (output >> ((sizeof(mln_u64_t)-1-i) << 3)) & 0xff;
        }
    }
})

