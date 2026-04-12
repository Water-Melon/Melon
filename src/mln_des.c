
/*
 * Copyright (C) Niklaus F.Schen.
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "mln_des.h"
#include "mln_func.h"

static inline void mln_des_begin_permute(mln_u64_t *msg);
static inline mln_u32_t mln_des_round_f(mln_u32_t right, mln_u64_t subkey);
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

/*
 * Combined S-box + P-permutation lookup tables.
 * sp_box[g][v] for group g (0-7) and 6-bit input v (0-63) gives
 * the 32-bit P-permuted output of S-box substitution for that group.
 * This replaces the separate expansion, S-box, and P permutation steps.
 */
static const mln_u32_t sp_box[8][64] = {
    {
    0x08000820u, 0x00000800u, 0x00020000u, 0x08020820u, 0x08000000u, 0x08000820u, 0x00000020u, 0x08000000u,
    0x00020020u, 0x08020000u, 0x08020820u, 0x00020800u, 0x08020800u, 0x00020820u, 0x00000800u, 0x00000020u,
    0x08020000u, 0x08000020u, 0x08000800u, 0x00000820u, 0x00020800u, 0x00020020u, 0x08020020u, 0x08020800u,
    0x00000820u, 0x00000000u, 0x00000000u, 0x08020020u, 0x08000020u, 0x08000800u, 0x00020820u, 0x00020000u,
    0x00020820u, 0x00020000u, 0x08020800u, 0x00000800u, 0x00000020u, 0x08020020u, 0x00000800u, 0x00020820u,
    0x08000800u, 0x00000020u, 0x08000020u, 0x08020000u, 0x08020020u, 0x08000000u, 0x00020000u, 0x08000820u,
    0x00000000u, 0x08020820u, 0x00020020u, 0x08000020u, 0x08020000u, 0x08000800u, 0x08000820u, 0x00000000u,
    0x08020820u, 0x00020800u, 0x00020800u, 0x00000820u, 0x00000820u, 0x00020020u, 0x08000000u, 0x08020800u
    },
    {
    0x00100000u, 0x02100001u, 0x02000401u, 0x00000000u, 0x00000400u, 0x02000401u, 0x00100401u, 0x02100400u,
    0x02100401u, 0x00100000u, 0x00000000u, 0x02000001u, 0x00000001u, 0x02000000u, 0x02100001u, 0x00000401u,
    0x02000400u, 0x00100401u, 0x00100001u, 0x02000400u, 0x02000001u, 0x02100000u, 0x02100400u, 0x00100001u,
    0x02100000u, 0x00000400u, 0x00000401u, 0x02100401u, 0x00100400u, 0x00000001u, 0x02000000u, 0x00100400u,
    0x02000000u, 0x00100400u, 0x00100000u, 0x02000401u, 0x02000401u, 0x02100001u, 0x02100001u, 0x00000001u,
    0x00100001u, 0x02000000u, 0x02000400u, 0x00100000u, 0x02100400u, 0x00000401u, 0x00100401u, 0x02100400u,
    0x00000401u, 0x02000001u, 0x02100401u, 0x02100000u, 0x00100400u, 0x00000000u, 0x00000001u, 0x02100401u,
    0x00000000u, 0x00100401u, 0x02100000u, 0x00000400u, 0x02000001u, 0x02000400u, 0x00000400u, 0x00100001u
    },
    {
    0x10000008u, 0x10200000u, 0x00002000u, 0x10202008u, 0x10200000u, 0x00000008u, 0x10202008u, 0x00200000u,
    0x10002000u, 0x00202008u, 0x00200000u, 0x10000008u, 0x00200008u, 0x10002000u, 0x10000000u, 0x00002008u,
    0x00000000u, 0x00200008u, 0x10002008u, 0x00002000u, 0x00202000u, 0x10002008u, 0x00000008u, 0x10200008u,
    0x10200008u, 0x00000000u, 0x00202008u, 0x10202000u, 0x00002008u, 0x00202000u, 0x10202000u, 0x10000000u,
    0x10002000u, 0x00000008u, 0x10200008u, 0x00202000u, 0x10202008u, 0x00200000u, 0x00002008u, 0x10000008u,
    0x00200000u, 0x10002000u, 0x10000000u, 0x00002008u, 0x10000008u, 0x10202008u, 0x00202000u, 0x10200000u,
    0x00202008u, 0x10202000u, 0x00000000u, 0x10200008u, 0x00000008u, 0x00002000u, 0x10200000u, 0x00202008u,
    0x00002000u, 0x00200008u, 0x10002008u, 0x00000000u, 0x10202000u, 0x10000000u, 0x00200008u, 0x10002008u
    },
    {
    0x00000080u, 0x01040080u, 0x01040000u, 0x21000080u, 0x00040000u, 0x00000080u, 0x20000000u, 0x01040000u,
    0x20040080u, 0x00040000u, 0x01000080u, 0x20040080u, 0x21000080u, 0x21040000u, 0x00040080u, 0x20000000u,
    0x01000000u, 0x20040000u, 0x20040000u, 0x00000000u, 0x20000080u, 0x21040080u, 0x21040080u, 0x01000080u,
    0x21040000u, 0x20000080u, 0x00000000u, 0x21000000u, 0x01040080u, 0x01000000u, 0x21000000u, 0x00040080u,
    0x00040000u, 0x21000080u, 0x00000080u, 0x01000000u, 0x20000000u, 0x01040000u, 0x21000080u, 0x20040080u,
    0x01000080u, 0x20000000u, 0x21040000u, 0x01040080u, 0x20040080u, 0x00000080u, 0x01000000u, 0x21040000u,
    0x21040080u, 0x00040080u, 0x21000000u, 0x21040080u, 0x01040000u, 0x00000000u, 0x20040000u, 0x21000000u,
    0x00040080u, 0x01000080u, 0x20000080u, 0x00040000u, 0x00000000u, 0x20040000u, 0x01040080u, 0x20000080u
    },
    {
    0x80401000u, 0x80001040u, 0x80001040u, 0x00000040u, 0x00401040u, 0x80400040u, 0x80400000u, 0x80001000u,
    0x00000000u, 0x00401000u, 0x00401000u, 0x80401040u, 0x80000040u, 0x00000000u, 0x00400040u, 0x80400000u,
    0x80000000u, 0x00001000u, 0x00400000u, 0x80401000u, 0x00000040u, 0x00400000u, 0x80001000u, 0x00001040u,
    0x80400040u, 0x80000000u, 0x00001040u, 0x00400040u, 0x00001000u, 0x00401040u, 0x80401040u, 0x80000040u,
    0x00400040u, 0x80400000u, 0x00401000u, 0x80401040u, 0x80000040u, 0x00000000u, 0x00000000u, 0x00401000u,
    0x00001040u, 0x00400040u, 0x80400040u, 0x80000000u, 0x80401000u, 0x80001040u, 0x80001040u, 0x00000040u,
    0x80401040u, 0x80000040u, 0x80000000u, 0x00001000u, 0x80400000u, 0x80001000u, 0x00401040u, 0x80400040u,
    0x80001000u, 0x00001040u, 0x00400000u, 0x80401000u, 0x00000040u, 0x00400000u, 0x00001000u, 0x00401040u
    },
    {
    0x00000104u, 0x04010100u, 0x00000000u, 0x04010004u, 0x04000100u, 0x00000000u, 0x00010104u, 0x04000100u,
    0x00010004u, 0x04000004u, 0x04000004u, 0x00010000u, 0x04010104u, 0x00010004u, 0x04010000u, 0x00000104u,
    0x04000000u, 0x00000004u, 0x04010100u, 0x00000100u, 0x00010100u, 0x04010000u, 0x04010004u, 0x00010104u,
    0x04000104u, 0x00010100u, 0x00010000u, 0x04000104u, 0x00000004u, 0x04010104u, 0x00000100u, 0x04000000u,
    0x04010100u, 0x04000000u, 0x00010004u, 0x00000104u, 0x00010000u, 0x04010100u, 0x04000100u, 0x00000000u,
    0x00000100u, 0x00010004u, 0x04010104u, 0x04000100u, 0x04000004u, 0x00000100u, 0x00000000u, 0x04010004u,
    0x04000104u, 0x00010000u, 0x04000000u, 0x04010104u, 0x00000004u, 0x00010104u, 0x00010100u, 0x04000004u,
    0x04010000u, 0x04000104u, 0x00000104u, 0x04010000u, 0x00010104u, 0x00000004u, 0x04010004u, 0x00010100u
    },
    {
    0x40084010u, 0x40004000u, 0x00004000u, 0x00084010u, 0x00080000u, 0x00000010u, 0x40080010u, 0x40004010u,
    0x40000010u, 0x40084010u, 0x40084000u, 0x40000000u, 0x40004000u, 0x00080000u, 0x00000010u, 0x40080010u,
    0x00084000u, 0x00080010u, 0x40004010u, 0x00000000u, 0x40000000u, 0x00004000u, 0x00084010u, 0x40080000u,
    0x00080010u, 0x40000010u, 0x00000000u, 0x00084000u, 0x00004010u, 0x40084000u, 0x40080000u, 0x00004010u,
    0x00000000u, 0x00084010u, 0x40080010u, 0x00080000u, 0x40004010u, 0x40080000u, 0x40084000u, 0x00004000u,
    0x40080000u, 0x40004000u, 0x00000010u, 0x40084010u, 0x00084010u, 0x00000010u, 0x00004000u, 0x40000000u,
    0x00004010u, 0x40084000u, 0x00080000u, 0x40000010u, 0x00080010u, 0x40004010u, 0x40000010u, 0x00080010u,
    0x00084000u, 0x00000000u, 0x40004000u, 0x00004010u, 0x40000000u, 0x40080010u, 0x40084010u, 0x00084000u
    },
    {
    0x00808200u, 0x00000000u, 0x00008000u, 0x00808202u, 0x00808002u, 0x00008202u, 0x00000002u, 0x00008000u,
    0x00000200u, 0x00808200u, 0x00808202u, 0x00000200u, 0x00800202u, 0x00808002u, 0x00800000u, 0x00000002u,
    0x00000202u, 0x00800200u, 0x00800200u, 0x00008200u, 0x00008200u, 0x00808000u, 0x00808000u, 0x00800202u,
    0x00008002u, 0x00800002u, 0x00800002u, 0x00008002u, 0x00000000u, 0x00000202u, 0x00008202u, 0x00800000u,
    0x00008000u, 0x00808202u, 0x00000002u, 0x00808000u, 0x00808200u, 0x00800000u, 0x00800000u, 0x00000200u,
    0x00808002u, 0x00008000u, 0x00008200u, 0x00800002u, 0x00000200u, 0x00000002u, 0x00800202u, 0x00008202u,
    0x00808202u, 0x00008002u, 0x00808000u, 0x00800202u, 0x00800002u, 0x00000202u, 0x00008202u, 0x00808200u,
    0x00000202u, 0x00800200u, 0x00800200u, 0x00000000u, 0x00008002u, 0x00008200u, 0x00000000u, 0x00808002u
    }
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
            right = left ^ mln_des_round_f((mln_u32_t)right, d->sub_keys[i]);
            left = tmp;
        }
    } else {
        for (i = 15; i >= 0; --i) {
            tmp = right;
            right = left ^ mln_des_round_f((mln_u32_t)right, d->sub_keys[i]);
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

/*
 * Combined E + S + P round function using precomputed SP-box tables.
 * Extracts 6-bit groups from the 32-bit right half (expansion permutation),
 * XORs with corresponding subkey bits, and looks up combined S+P tables.
 */
MLN_FUNC(static inline, mln_u32_t, mln_des_round_f, \
         (mln_u32_t right, mln_u64_t subkey), (right, subkey), {
    return sp_box[0][(((right >> 31) & 1) | ((right & 0x1f) << 1)) ^ ((mln_u32_t)(subkey      ) & 0x3f)]
         ^ sp_box[1][((right >>  3) & 0x3f) ^ ((mln_u32_t)(subkey >>  6) & 0x3f)]
         ^ sp_box[2][((right >>  7) & 0x3f) ^ ((mln_u32_t)(subkey >> 12) & 0x3f)]
         ^ sp_box[3][((right >> 11) & 0x3f) ^ ((mln_u32_t)(subkey >> 18) & 0x3f)]
         ^ sp_box[4][((right >> 15) & 0x3f) ^ ((mln_u32_t)(subkey >> 24) & 0x3f)]
         ^ sp_box[5][((right >> 19) & 0x3f) ^ ((mln_u32_t)(subkey >> 30) & 0x3f)]
         ^ sp_box[6][((right >> 23) & 0x3f) ^ ((mln_u32_t)(subkey >> 36) & 0x3f)]
         ^ sp_box[7][(((right >> 27) & 0x1f) | ((right & 1) << 5)) ^ ((mln_u32_t)(subkey >> 42) & 0x3f)];
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

