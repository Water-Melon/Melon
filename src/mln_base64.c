
/*
 * Copyright (C) Niklaus F.Schen.
 */
#include <stdlib.h>
#include <stdio.h>
#include "mln_base64.h"
#include "mln_func.h"

static mln_s8_t base_map[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

MLN_FUNC(, int, mln_base64_encode, \
         (mln_u8ptr_t in, mln_uauto_t inlen, mln_u8ptr_t *out, mln_uauto_t *outlen), \
         (in, inlen, out, outlen), \
{
    mln_uauto_t i, state, j;

    *outlen = inlen / 3 * 4;
    if (inlen % 3) (*outlen) += 4;

    *out = (mln_u8ptr_t)calloc(1, *outlen + 1);
    if (*out == NULL) return -1;

    mln_u8ptr_t o = *out;

    for (i = 0, state = 0, j = 0; i < inlen; ++j) {
        if (state == 0) {
            o[j] = (mln_u8_t)base_map[(in[i] >> 2) & 0x3f];
            state = 1;
        } else if (state == 1) {
            if (i+1 >= inlen) {
                o[j++] = (mln_u8_t)base_map[(in[i] & 0x3) << 4];
                o[j++] = (mln_u8_t)'=';
                o[j] = (mln_u8_t)'=';
                break;
            } else {
                o[j] = (mln_u8_t)base_map[((in[i] & 0x3) << 4)|((in[i+1] >> 4) & 0xf)];
                ++i;
                state = 2;
            }
        } else {
            if (i+1 >= inlen) {
                o[j++] = (mln_u8_t)base_map[((in[i] & 0xf) << 2)];
                o[j] = (mln_u8_t)'=';
                break;
            } else {
                o[j++] = (mln_u8_t)base_map[((in[i] & 0xf) << 2)|((in[i+1] >> 6) & 0x3)];
                o[j] = (mln_u8_t)base_map[in[++i] & 0x3f];
                ++i;
                state = 0;
            }
        }
    }

    return 0;
})

MLN_FUNC(, int, mln_base64_pool_encode, \
         (mln_alloc_t *pool, mln_u8ptr_t in, mln_uauto_t inlen, mln_u8ptr_t *out, mln_uauto_t *outlen), \
         (pool, in, inlen, out, outlen), \
{
    mln_uauto_t i, state, j;

    *outlen = inlen / 3 * 4;
    if (inlen % 3) (*outlen) += 4;

    *out = (mln_u8ptr_t)mln_alloc_c(pool, *outlen + 1);
    if (*out == NULL) return -1;

    mln_u8ptr_t o = *out;

    for (i = 0, state = 0, j = 0; i < inlen; ++j) {
        if (state == 0) {
            o[j] = (mln_u8_t)base_map[(in[i] >> 2) & 0x3f];
            state = 1;
        } else if (state == 1) {
            if (i+1 >= inlen) {
                o[j++] = (mln_u8_t)base_map[(in[i] & 0x3) << 4];
                o[j++] = (mln_u8_t)'=';
                o[j] = (mln_u8_t)'=';
                break;
            } else {
                o[j] = (mln_u8_t)base_map[((in[i] & 0x3) << 4)|((in[i+1] >> 4) & 0xf)];
                ++i;
                state = 2;
            }
        } else {
            if (i+1 >= inlen) {
                o[j++] = (mln_u8_t)base_map[((in[i] & 0xf) << 2)];
                o[j] = (mln_u8_t)'=';
                break;
            } else {
                o[j++] = (mln_u8_t)base_map[((in[i] & 0xf) << 2)|((in[i+1] >> 6) & 0x3)];
                o[j] = (mln_u8_t)base_map[in[++i] & 0x3f];
                ++i;
                state = 0;
            }
        }
    }

    return 0;
})

MLN_FUNC(, int, mln_base64_decode, \
         (mln_u8ptr_t in, mln_uauto_t inlen, mln_u8ptr_t *out, mln_uauto_t *outlen), \
         (in, inlen, out, outlen), \
{
    if (inlen % 4) return -1;

    mln_u8_t ascii[256] = {0};
    mln_uauto_t i, state, j;

    for (i = 0; i < 64; ++i) {
        ascii[(mln_u8_t)base_map[i]] = i;
    }

    *outlen = inlen / 4 * 3;
    if (in[inlen-1] == '=') --(*outlen);
    if (in[inlen-2] == '=') --(*outlen);

    *out = (mln_u8ptr_t)calloc(1, *outlen + 1);
    if (*out == NULL) return -1;
    mln_u8ptr_t o = *out;

    for (i = 0, state = 0, j = 0; i < inlen; ) {
        if (state == 0) {
            o[j] = (ascii[in[i]] << 2);
            state = 1;
            ++i;
        } else if (state == 1) {
            o[j++] |= ((ascii[in[i]] >> 4) & 0x3);
            state = 2;
        } else if (state == 2) {
            o[j] = (ascii[in[i]] & 0xf) << 4;
            state = 3;
            ++i;
        } else if (state == 3) {
            if (in[i] == '=') break;
            o[j++] |= ((ascii[in[i]] >> 2) & 0xf);
            state = 4;
        } else if (state == 4) {
            o[j] = (ascii[in[i]] & 0x3) << 6;
            state = 5;
            ++i;
        } else {
            if (in[i] == '=') break;
            o[j++] |= (ascii[in[i]] & 0x3f);
            state = 0;
            ++i;
        }
    }

    return 0;
})

MLN_FUNC(, int, mln_base64_pool_decode, \
         (mln_alloc_t *pool, mln_u8ptr_t in, mln_uauto_t inlen, mln_u8ptr_t *out, mln_uauto_t *outlen), \
         (pool, in, inlen, out, outlen), \
{
    if (inlen % 4) return -1;

    mln_u8_t ascii[256] = {0};
    mln_uauto_t i, state, j;

    for (i = 0; i < 64; ++i) {
        ascii[(mln_u8_t)base_map[i]] = i;
    }

    *outlen = inlen / 4 * 3;
    if (in[inlen-1] == '=') --(*outlen);
    if (in[inlen-2] == '=') --(*outlen);

    *out = (mln_u8ptr_t)mln_alloc_c(pool, *outlen + 1);
    if (*out == NULL) return -1;
    mln_u8ptr_t o = *out;

    for (i = 0, state = 0, j = 0; i < inlen; ) {
        if (state == 0) {
            o[j] = (ascii[in[i]] << 2);
            state = 1;
            ++i;
        } else if (state == 1) {
            o[j++] |= ((ascii[in[i]] >> 4) & 0x3);
            state = 2;
        } else if (state == 2) {
            o[j] = (ascii[in[i]] & 0xf) << 4;
            state = 3;
            ++i;
        } else if (state == 3) {
            if (in[i] == '=') break;
            o[j++] |= ((ascii[in[i]] >> 2) & 0xf);
            state = 4;
        } else if (state == 4) {
            o[j] = (ascii[in[i]] & 0x3) << 6;
            state = 5;
            ++i;
        } else {
            if (in[i] == '=') break;
            o[j++] |= (ascii[in[i]] & 0x3f);
            state = 0;
            ++i;
        }
    }

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

