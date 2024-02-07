
/*
 * Copyright (C) Niklaus F.Schen.
 */

#include <string.h>
#include <stdio.h>
#include "mln_rc.h"
#include "mln_func.h"

MLN_FUNC_VOID(, void, mln_rc4_init, \
              (mln_u8ptr_t s, mln_u8ptr_t key, mln_uauto_t len), \
              (s, key, len), \
{
    mln_u32_t i = 0, j = 0;
    mln_u8_t tmp;
    mln_uauto_t k[256] = {0};

    for (i = 0; i < 256; ++i) {
        s[i] = i;
        k[i] = key[i % len];
    }
    for (i = 0; i < 256; ++i) {
        j = (j + s[i] + k[i]) % 256;
        tmp = s[i];
        s[i] = s[j];
        s[j] = tmp;
    }
})

MLN_FUNC_VOID(, void, mln_rc4_calc, \
              (mln_u8ptr_t s, mln_u8ptr_t data, mln_uauto_t len), \
              (s, data, len), \
{
    mln_u32_t i = 0, j = 0, t;
    mln_u8_t tmp, stmp[256];
    mln_uauto_t k;

    memcpy(stmp, s, sizeof(stmp));

    for (k = 0; k < len; ++k) {
        i = (i + 1) % 256;
        j = (j + stmp[i]) % 256;
        tmp = stmp[i];
        stmp[i] = stmp[j];
        stmp[j] = tmp;
        t = (stmp[i] + stmp[j]) % 256;
        data[k] ^= stmp[t];
    }
})

