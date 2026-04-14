
/*
 * Copyright (C) Niklaus F.Schen.
 */

#include <string.h>
#include <stdio.h>
#include "mln_rc.h"
#include "mln_func.h"

static const mln_u8_t rc4_identity[256] = {
      0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15,
     16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
     32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
     48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63,
     64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79,
     80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95,
     96, 97, 98, 99,100,101,102,103,104,105,106,107,108,109,110,111,
    112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,127,
    128,129,130,131,132,133,134,135,136,137,138,139,140,141,142,143,
    144,145,146,147,148,149,150,151,152,153,154,155,156,157,158,159,
    160,161,162,163,164,165,166,167,168,169,170,171,172,173,174,175,
    176,177,178,179,180,181,182,183,184,185,186,187,188,189,190,191,
    192,193,194,195,196,197,198,199,200,201,202,203,204,205,206,207,
    208,209,210,211,212,213,214,215,216,217,218,219,220,221,222,223,
    224,225,226,227,228,229,230,231,232,233,234,235,236,237,238,239,
    240,241,242,243,244,245,246,247,248,249,250,251,252,253,254,255
};

MLN_FUNC_VOID(, void, mln_rc4_init, \
              (mln_u8ptr_t s, mln_u8ptr_t key, mln_uauto_t len), \
              (s, key, len), \
{
    mln_u32_t i;
    mln_u8_t j = 0, tmp;
    mln_u8_t kbuf[256];
    mln_uauto_t pos = 0;

    memcpy(s, rc4_identity, 256);
    if (len == 0) return;
    while (pos + len <= 256) {
        memcpy(kbuf + pos, key, len);
        pos += len;
    }
    if (pos < 256)
        memcpy(kbuf + pos, key, 256 - pos);
    for (i = 0; i < 256; ++i) {
        j += s[i] + kbuf[i];
        tmp = s[i];
        s[i] = s[j];
        s[j] = tmp;
    }
})

MLN_FUNC_VOID(, void, mln_rc4_calc, \
              (mln_u8ptr_t s, mln_u8ptr_t data, mln_uauto_t len), \
              (s, data, len), \
{
    mln_u8_t i = 0, j = 0, si, sj;
    mln_u8_t stmp[256];
    mln_uauto_t k = 0;

    memcpy(stmp, s, 256);

    for (; k + 4 <= len; k += 4) {
        ++i; si = stmp[i]; j += si; sj = stmp[j]; stmp[i] = sj; stmp[j] = si;
        data[k]     ^= stmp[(mln_u8_t)(si + sj)];
        ++i; si = stmp[i]; j += si; sj = stmp[j]; stmp[i] = sj; stmp[j] = si;
        data[k + 1] ^= stmp[(mln_u8_t)(si + sj)];
        ++i; si = stmp[i]; j += si; sj = stmp[j]; stmp[i] = sj; stmp[j] = si;
        data[k + 2] ^= stmp[(mln_u8_t)(si + sj)];
        ++i; si = stmp[i]; j += si; sj = stmp[j]; stmp[i] = sj; stmp[j] = si;
        data[k + 3] ^= stmp[(mln_u8_t)(si + sj)];
    }
    for (; k < len; ++k) {
        ++i; si = stmp[i]; j += si; sj = stmp[j]; stmp[i] = sj; stmp[j] = si;
        data[k] ^= stmp[(mln_u8_t)(si + sj)];
    }
})

