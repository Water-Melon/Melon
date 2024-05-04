
/*
 * Copyright (C) Niklaus F.Schen.
 */
#if defined(MSVC)
#define _CRT_RAND_S
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#if defined(MSVC)
#include "mln_utils.h"
#else
#include <sys/time.h>
#endif
#include <math.h>
#include "mln_bignum.h"
#include "mln_func.h"

static inline int
mln_bignum_assign_hex(mln_bignum_t *bn, mln_s8ptr_t sval, mln_u32_t tag, mln_u32_t len);
static inline int
mln_bignum_assign_oct(mln_bignum_t *bn, mln_s8ptr_t sval, mln_u32_t tag, mln_u32_t len);
static inline int
mln_bignum_assign_dec(mln_bignum_t *bn, mln_s8ptr_t sval, mln_u32_t tag, mln_u32_t len);
static void
mln_bignum_dec_recursive(mln_u32_t rec_times, mln_u32_t loop_times, mln_bignum_t *tmp);
static inline int
__mln_bignum_bit_test(mln_bignum_t *bn, mln_u32_t index);
static inline int
__mln_bignum_compare(mln_bignum_t *bn1, mln_bignum_t *bn2);
static inline int
__mln_bignum_abs_compare(mln_bignum_t *bn1, mln_bignum_t *bn2);
static inline void
__mln_bignum_add(mln_bignum_t *dest, mln_bignum_t *src);
static inline void
__mln_bignum_sub_core(mln_bignum_t *dest, mln_bignum_t *src);
static inline void
__mln_bignum_sub(mln_bignum_t *dest, mln_bignum_t *src);
static inline int
__mln_bignum_div(mln_bignum_t *dest, mln_bignum_t *src, mln_bignum_t *quotient);
static inline int
__mln_bignum_pwr(mln_bignum_t *dest, mln_bignum_t *exponent, mln_bignum_t *mod);
static inline void
__mln_bignum_left_shift(mln_bignum_t *bn, mln_u32_t n);
static inline void
__mln_bignum_right_shift(mln_bignum_t *bn, mln_u32_t n);
static inline void
__mln_bignum_mul_word(mln_bignum_t *dest, mln_s64_t src);
static inline int
__mln_bignum_div_word(mln_bignum_t *dest, mln_s64_t src, mln_bignum_t *quotient);
/*prime*/
static inline void
mln_bignum_seperate(mln_u32_t *pwr, mln_bignum_t *odd);
static inline mln_u32_t
mln_bignum_witness(mln_bignum_t *base, mln_bignum_t *prim);
static inline void
mln_bignum_random_prime(mln_bignum_t *bn, mln_u32_t bitwidth);
static inline void
mln_bignum_random_scope(mln_bignum_t *bn, mln_u32_t bitwidth, mln_bignum_t *max);


MLN_FUNC(, mln_bignum_t *, mln_bignum_new, (void), (), {
    return (mln_bignum_t *)calloc(1, sizeof(mln_bignum_t));
})

MLN_FUNC(, mln_bignum_t *, mln_bignum_pool_new, (mln_alloc_t *pool), (pool), {
    return (mln_bignum_t *)mln_alloc_c(pool, sizeof(mln_bignum_t));
})

MLN_FUNC_VOID(, void, mln_bignum_free, (mln_bignum_t *bn), (bn), {
    if (bn == NULL) return;
    free(bn);
})

MLN_FUNC_VOID(, void, mln_bignum_pool_free, (mln_bignum_t *bn), (bn), {
    if (bn == NULL) return;
    mln_alloc_free(bn);
})

MLN_FUNC(, mln_bignum_t *, mln_bignum_dup, (mln_bignum_t *bn), (bn), {
    mln_bignum_t *target = (mln_bignum_t *)malloc(sizeof(mln_bignum_t));
    if (target == NULL) return NULL;

    target->tag = bn->tag;
    target->length = bn->length;
    memcpy(target->data, bn->data, bn->length*sizeof(mln_u64_t));
    return target;
})

MLN_FUNC(, mln_bignum_t *, mln_bignum_pool_dup, (mln_alloc_t *pool, mln_bignum_t *bn), (pool, bn), {
    mln_bignum_t *target = (mln_bignum_t *)mln_alloc_m(pool, sizeof(mln_bignum_t));
    if (target == NULL) return NULL;

    target->tag = bn->tag;
    target->length = bn->length;
    memcpy(target->data, bn->data, bn->length*sizeof(mln_u64_t));
    return target;
})

MLN_FUNC(, int, mln_bignum_assign, (mln_bignum_t *bn, mln_s8ptr_t sval, mln_u32_t len), (bn, sval, len), {
    mln_u32_t tag;
    if (sval[0] == '-') {
        tag = M_BIGNUM_NEGATIVE;
        ++sval;
        --len;
    } else {
        tag = M_BIGNUM_POSITIVE;
    }

    if (sval[0] == '0') {
        if (sval[1] == 'x') {
            return mln_bignum_assign_hex(bn, sval+2, tag, len-2);
        } else {
            return mln_bignum_assign_oct(bn, sval+1, tag, len-1);
        }
    }
    return mln_bignum_assign_dec(bn, sval, tag, len);
})

MLN_FUNC(static inline, int, mln_bignum_assign_hex, \
         (mln_bignum_t *bn, mln_s8ptr_t sval, mln_u32_t tag, mln_u32_t len), \
         (bn, sval, tag, len), \
{
    if (len > M_BIGNUM_BITS/4) return -1;
    mln_s8ptr_t p = sval + len - 1;
    mln_s32_t i, j = 0, l = 0;
    mln_u8_t b = 0;
    mln_u64_t *tmp = bn->data;
    memset(bn, 0, sizeof(mln_bignum_t));
    mln_bignum_positive(bn);

    for (i = 0; p >= sval; --p, ++i) {
        if (i % 2) {
            if (mln_isdigit(*p)) {
                b |= (((*p - '0') << 4) & 0xf0);
            } else if (*p >= 'a' && *p <= 'f') {
                b |= (((*p - 'a' + 10) << 4) & 0xf0);
            } else if (*p >= 'A' && *p <= 'F') {
                b |= (((*p - 'A' + 10) << 4) & 0xf0);
            } else {
                return -1;
            }
        } else {
            b = 0;
            if (mln_isdigit(*p)) {
                b |= ((*p - '0') & 0xf);
            } else if (*p >= 'a' && *p <= 'f') {
                b |= ((*p - 'a' + 10) & 0xf);
            } else if (*p >= 'A' && *p <= 'F') {
                b |= ((*p - 'A' + 10) & 0xf);
            } else {
                return -1;
            }
        }
        if (i % 2 || p == sval) {
            tmp[l] |= ((mln_u64_t)b << (j << 3));
            if (j % 4 == 3) {
                ++l;
                j = 0;
            } else {
                ++j;
            }
        }
    }

    bn->tag = tag;
    for (i = M_BIGNUM_SIZE-1; i >= 0; --i) {
        if (tmp[i] != 0) break;
    }
    bn->length = i+1;
    return 0;
})

MLN_FUNC(static inline, int, mln_bignum_assign_oct, \
         (mln_bignum_t *bn, mln_s8ptr_t sval, mln_u32_t tag, mln_u32_t len), \
         (bn, sval, tag, len), \
{
    if (len > M_BIGNUM_BITS/3) return -1;
    mln_s8ptr_t p = sval + len - 1;
    mln_u64_t *tmp = bn->data;
    mln_u32_t j = 0, l = 0;
    mln_s32_t i;
    mln_u8_t b = 0;
    memset(bn, 0, sizeof(mln_bignum_t));
    mln_bignum_positive(bn);

    for (i = 0; p >= sval; --p, ++i) {
        if (i%3 == 0) {
            b = 0;
            if (*p < '0' || *p > '7') return -1;
            b |= ((*p - '0') & 0x7);
        } else if (i%3 == 1) {
            if (*p < '0' || *p > '7') return -1;
            b |= (((*p - '0') << 3) & 0x38);
        } else {
            if (*p < '0' || *p > '3') return -1;
            b |= (((*p - '0') << 6) & 0xc0);
        }

        if (i%3 == 2 || p == sval) {
            tmp[l] |= ((mln_u64_t)b << (j << 3));
            if (j % 4 == 3) {
                ++l;
                j = 0;
            } else {
                ++j;
            }
        }
    }

    bn->tag = tag;
    for (i = M_BIGNUM_SIZE-1; i >= 0; --i) {
        if (tmp[i] != 0) break;
    }
    bn->length = i + 1;
    return 0;
})

MLN_FUNC(static inline, int, mln_bignum_assign_dec, \
         (mln_bignum_t *bn, mln_s8ptr_t sval, mln_u32_t tag, mln_u32_t len), \
         (bn, sval, tag, len), \
{
    if (len > M_BIGNUM_BITS/4) return -1;
    mln_s8ptr_t p = sval + len -1;
    mln_u32_t cnt;
    mln_bignum_t tmp;
    memset(bn, 0, sizeof(mln_bignum_t));
    mln_bignum_positive(bn);

    for (cnt = 0; p >= sval; --p, ++cnt) {
        if (!mln_isdigit(*p)) return -1;
        memset(&tmp, 0, sizeof(tmp));
        mln_bignum_positive(&tmp);
        mln_bignum_dec_recursive(cnt, *p-'0', &tmp);
        __mln_bignum_add(bn, &tmp);
    }

    bn->tag = tag;
    return 0;
})

MLN_FUNC_VOID(static, void, mln_bignum_dec_recursive, \
              (mln_u32_t rec_times, mln_u32_t loop_times, mln_bignum_t *tmp), \
              (rec_times, loop_times, tmp), \
{
    if (!rec_times) {
        mln_bignum_t one = {M_BIGNUM_POSITIVE, 1, {0}};
        one.data[0] = 1;
        memset(tmp, 0, sizeof(mln_bignum_t));
        for (; loop_times > 0; --loop_times) {
            __mln_bignum_add(tmp, &one);
        }
    } else {
        mln_bignum_t val;
        memset(&val, 0, sizeof(val));
        mln_bignum_positive(&val);
        mln_bignum_dec_recursive(rec_times-1, 10, &val);
        for (; loop_times > 0; --loop_times) {
            __mln_bignum_add(tmp, &val);
        }
    }
})

MLN_FUNC_VOID(, void, mln_bignum_add, (mln_bignum_t *dest, mln_bignum_t *src), (dest, src), {
    __mln_bignum_add(dest, src);
})

MLN_FUNC_VOID(static inline, void, __mln_bignum_add, \
              (mln_bignum_t *dest, mln_bignum_t *src), (dest, src), \
{
    if (dest->tag != src->tag) {
        if (dest->tag == M_BIGNUM_NEGATIVE) {
            mln_bignum_t tmp = *src;
            mln_bignum_positive(dest);
            __mln_bignum_sub(&tmp, dest);
            *dest = tmp;
        } else {
            mln_bignum_positive(src);
            __mln_bignum_sub(dest, src);
            mln_bignum_negative(src);
        }
        return;
    }

    mln_u64_t carry = 0;
    mln_u64_t *dest_data = dest->data, *max, *min, *end;
    if (dest->length < src->length) {
        max = src->data;
        min = dest->data;
        end = max + src->length;
    } else {
        max = dest->data;
        min = src->data;
        end = max + dest->length;
    }

    for (; max < end; ++max, ++min, ++dest_data) {
        *dest_data = *max + *min + carry;
        carry = 0;
        if (*dest_data >= M_BIGNUM_UMAX) {
            *dest_data -= M_BIGNUM_UMAX;
            carry = 1;
        }
    }

    if (carry && dest_data < dest->data+M_BIGNUM_SIZE) {
        *(dest_data++) += carry;
        carry = 0;
    }

    dest->length = dest_data - dest->data;
})

MLN_FUNC_VOID(, void, mln_bignum_sub, (mln_bignum_t *dest, mln_bignum_t *src), (dest, src), {
    __mln_bignum_sub(dest, src);
})

MLN_FUNC_VOID(static inline, void, __mln_bignum_sub, \
              (mln_bignum_t *dest, mln_bignum_t *src), (dest, src), \
{
    if (dest->tag != src->tag) {
        if (dest->tag == M_BIGNUM_NEGATIVE) {
            mln_bignum_negative(src);
            __mln_bignum_add(dest, src);
            mln_bignum_positive(src);
        } else {
            mln_bignum_positive(src);
            __mln_bignum_add(dest, src);
            mln_bignum_negative(src);
        }
        return;
    }

    int ret;
    if ((ret = __mln_bignum_compare(dest, src)) == 0) {
        memset(dest, 0, sizeof(mln_bignum_t));
        mln_bignum_positive(dest);
    } else if (ret < 0) {
        if (dest->tag == M_BIGNUM_NEGATIVE) {
            mln_bignum_positive(dest);
            mln_bignum_positive(src);
            __mln_bignum_sub_core(dest, src);
            mln_bignum_negative(dest);
            mln_bignum_negative(src);
        } else {
            mln_bignum_t tmp = *src;
            __mln_bignum_sub_core(&tmp, dest);
            *dest = tmp;
            mln_bignum_negative(dest);
        }
    } else {
        if (dest->tag == M_BIGNUM_NEGATIVE) {
            mln_bignum_t tmp = *src;
            mln_bignum_positive(dest);
            mln_bignum_positive(&tmp);
            __mln_bignum_sub_core(&tmp, dest);
            *dest = tmp;
            mln_bignum_positive(dest);
        } else { /*real calculation*/
            __mln_bignum_sub_core(dest, src);
        }
    }
})

MLN_FUNC_VOID(static inline, void, __mln_bignum_sub_core, \
              (mln_bignum_t *dest, mln_bignum_t *src), (dest, src), \
{
    mln_u32_t borrow = 0;
    mln_u64_t *dest_data = dest->data, *src_data = src->data;
    mln_u64_t *end = dest->data + dest->length;
    for (; dest_data < end; ++dest_data, ++src_data) {
        if (*src_data + borrow > *dest_data) {
            *dest_data = (*dest_data+M_BIGNUM_UMAX)-(*src_data+borrow);
            borrow = 1;
        } else {
            *dest_data -= (*src_data + borrow);
            borrow = 0;
        }
    }

    assert(borrow == 0);
    dest_data = dest->data;
    for (--end; end >= dest_data; --end) {
        if (*end != 0) break;
    }
    dest->length = (end < dest_data)? 0: end - dest_data + 1;
})

MLN_FUNC_VOID(static inline, void, __mln_bignum_mul, \
              (mln_bignum_t *dest, mln_bignum_t *src), \
              (dest, src), \
{
    mln_u32_t tag = dest->tag ^ src->tag;
    if (src->length == 1) {
        __mln_bignum_mul_word(dest, src->data[0]);
        dest->tag = tag;
        return;
    } else if (dest->length == 1) {
        mln_bignum_t tmp = *src;
        __mln_bignum_mul_word(&tmp, dest->data[0]);
        *dest = tmp;
        dest->tag = tag;
        return;
    }
    mln_bignum_t res = {M_BIGNUM_POSITIVE, 0, {0}};
    if (!__mln_bignum_abs_compare(dest, &res) || !__mln_bignum_abs_compare(src, &res)) {
        *dest = res;
        return;
    }

    mln_u64_t *data = res.data, tmp, *dest_data = dest->data, *src_data;
    mln_u64_t *dend, *send, *last = res.data + M_BIGNUM_SIZE;

    for (dend = dest->data+dest->length; dest_data<dend; ++dest_data) {
        src_data = src->data, send = src->data+src->length;
        data = res.data + (dest_data - dest->data);
        if (data >= last) continue;
        for (; src_data<send && data < last; ++src_data, ++data) {
            tmp = (*dest_data) * (*src_data) + *data;
            *data = tmp % M_BIGNUM_UMAX;
            if (data+1 < last) *(data+1) += (tmp >> M_BIGNUM_SHIFT);
        }
    }
    res.length = (data - res.data);
    if (data < last && *data != 0) ++(res.length);
    res.tag = tag;
    *dest = res;
})

MLN_FUNC_VOID(, void, mln_bignum_mul, (mln_bignum_t *dest, mln_bignum_t *src), (dest, src), {
    __mln_bignum_mul(dest, src);
})

MLN_FUNC(, int, mln_bignum_div, \
         (mln_bignum_t *dest, mln_bignum_t *src, mln_bignum_t *quotient), \
         (dest, src, quotient), \
{
    return __mln_bignum_div(dest, src, quotient);
})

MLN_FUNC(static inline, int, __mln_bignum_nmul, \
         (mln_u64_t *a, mln_u64_t b, mln_u64_t *r, int n, int m), \
         (a, b, r, n, m), \
{
    b &= 0xffffffff;
    if (b == 0) {
        memset(r, 0, n * sizeof(mln_u64_t));
        return 0;
    }
    if (b == 1) {
        memcpy(r, a, sizeof(mln_u64_t)*n);
        return 0;
    }

    mln_u64_t c = 0;
    n -= m;
    for (; m > 0; --m, ++a, ++r) {
        *r = *a * b + c;
        c = *r >> M_BIGNUM_SHIFT;
        *r %= M_BIGNUM_UMAX;
    }
    for (; n > 0; --n, ++r) {
        *r = c % M_BIGNUM_UMAX;
        c >>= M_BIGNUM_SHIFT;
    }

    return c > 0;
})

MLN_FUNC(static inline, int, __mln_bignum_nsbb, \
         (mln_u64_t *a, mln_u64_t *b, mln_u64_t *r, int n), \
         (a, b, r, n), \
{
    int c = 0;
    for (; n > 0; --n, ++a, ++b, ++r) {
        if (*a < *b + c) {
            *r = *a + M_BIGNUM_UMAX - *b - c;
            c = 1;
        } else {
            *r = *a - *b - c;
            c = 0;
        }
    }
    return c;
})

MLN_FUNC(static inline, int, __mln_bignum_nadc, \
         (mln_u64_t *a, mln_u64_t *b, mln_u64_t *r, int n), \
         (a, b, r, n), \
{
    int c = 0;
    for (; n > 0; --n, ++a, ++b, ++r) {
        *r = *a + *b + c;
        if (*r >= M_BIGNUM_UMAX) {
            *r -= M_BIGNUM_UMAX;
            c = 1;
        } else {
            c = 0;
        }
    }
    return c;
})

MLN_FUNC(static inline, int, __mln_bignum_ndiv, \
         (mln_u64_t *a, mln_u64_t b, mln_u64_t *r, int n), \
         (a, b, r, n), \
{
    b &= 0xffffffff;
    if (b == 0) return -1;
    if (b == 1) {
        memcpy(r, a, n*sizeof(mln_u64_t));
        mln_u64_t *data = r + n - 1;
        for (; data >= r && *data==0; --data)
            ;
        return data < r? 0: data - r + 1;
    }
    mln_u64_t *dest_data, *data, tmp = 0;
    dest_data = a + n - 1;
    data = r + n - 1;
    mln_u32_t len = 0;

    while (dest_data >= a) {
        tmp += *dest_data;
        *data = tmp / b;
        if (!len && *data) len = data - r + 1;
        --data;
        tmp %= b;
        tmp <<= M_BIGNUM_SHIFT;
        --dest_data;
    }
    return len;
})

MLN_FUNC(static inline, int, __mln_bignum_div, \
         (mln_bignum_t *dest, mln_bignum_t *src, mln_bignum_t *quotient), \
         (dest, src, quotient), \
{
    int ret;
    mln_u32_t tag = dest->tag ^ src->tag;

    if (src->length == 0) return -1;
    if (src->length == 1) {
        int ret = __mln_bignum_div_word(dest, src->data[0], quotient);
        if (ret < 0) return -1;
        dest->tag = tag;
        if (quotient != NULL) quotient->tag = tag;
        return 0;
    }

    if ((ret = __mln_bignum_abs_compare(dest, src)) == 0) {
        memset(dest, 0, sizeof(mln_bignum_t));
        if (quotient != NULL) {
            memset(quotient, 0, sizeof(mln_bignum_t));
            quotient->tag = tag;
            quotient->length = 1;
            quotient->data[0] = 1;
        }
        return 0;
    } else if (ret < 0) {
        dest->tag = tag;
        if (quotient != NULL) {
            memset(quotient, 0, sizeof(mln_bignum_t));
        }
        return 0;
    }


    mln_u64_t ddata[M_BIGNUM_SIZE+1], sdata[M_BIGNUM_SIZE+1], tmp[M_BIGNUM_SIZE+1];
    mln_u64_t *pd = ddata, *ps = sdata, *pr = NULL, q, r, d, t, v_n;
    int dlen = dest->length, slen = src->length, j;
    if (quotient != NULL) {
        memset(quotient, 0, sizeof(mln_bignum_t));
        pr = &quotient->data[dlen-slen];
    }

    /*D1 classical algorithm*/
    ddata[dlen] = sdata[slen] = 0;
    d = M_BIGNUM_UMAX / (src->data[slen-1] + 1);
    __mln_bignum_nmul(dest->data, d, pd, dlen+1, dlen);
    __mln_bignum_nmul(src->data, d, ps, slen, slen);
    v_n = ps[slen-1];

    /*D2 classical algorithm*/
    for (j = dlen - slen; j >= 0; --j) {
        /*D3 classical algorithm*/
        t = (pd[j+slen]<<M_BIGNUM_SHIFT) + pd[j+slen-1];
        q = t / v_n;
        r = t % v_n;

        t = pd[j+slen-2];
        while (q == M_BIGNUM_UMAX || q*ps[slen-2] > (r<<M_BIGNUM_SHIFT)+t) {
            --q;
            r += v_n;
            if (r >= M_BIGNUM_UMAX) break;
        }

        /*D4-7 classical algorithm*/
        __mln_bignum_nmul(ps, q, tmp, slen+1, slen+1);
        if (__mln_bignum_nsbb(pd+j, tmp, pd+j, slen+1)) {
            --q;
            __mln_bignum_nadc(pd+j, ps, pd+j, slen+1);
        }
        if (pr != NULL) *pr-- = q;
    }

    /*D8 classical algorithm*/
    if (quotient != NULL) {
        j = dlen - slen;
        quotient->length = *(quotient->data + j)? j+1: j;
        quotient->tag = tag;
    }

    memset(dest, 0, sizeof(mln_bignum_t));
    dest->length = __mln_bignum_ndiv(ddata, d, dest->data, slen);
    dest->tag = tag;

    return 0;
})

MLN_FUNC(, int, mln_bignum_pwr, \
         (mln_bignum_t *dest, mln_bignum_t *exponent, mln_bignum_t *mod), \
         (dest, exponent, mod), \
{
    return __mln_bignum_pwr(dest, exponent, mod);
})

MLN_FUNC(static inline, int, __mln_bignum_pwr, \
         (mln_bignum_t *dest, mln_bignum_t *exponent, mln_bignum_t *mod), \
         (dest, exponent, mod), \
{
    if (exponent->tag == M_BIGNUM_NEGATIVE) return -1;

    mln_bignum_t x;
    mln_s32_t i;

    for (i = (exponent->length<<5)-1; i >= 0; --i) {
        if (__mln_bignum_bit_test(exponent, i)) break; 
    }
    if (i < 0) {
        memset(dest, 0, sizeof(mln_bignum_t));
        dest->tag = M_BIGNUM_POSITIVE;
        dest->length = 1;
        dest->data[0] = 1;
        return 0;
    }

    x = *dest;

    if (mod != NULL) {
        for (--i; i>= 0; --i) {
            __mln_bignum_mul(dest, dest);
            if (dest->length >= mod->length) {
                if (__mln_bignum_div(dest, mod, NULL) < 0) {
                    return -1;
                }
            }
            if (__mln_bignum_bit_test(exponent, i)) {
                __mln_bignum_mul(dest, &x);
                if (dest->length >= mod->length) {
                    if (__mln_bignum_div(dest, mod, NULL) < 0) {
                        return -1;
                    }
                }
            }
        }
    } else {
        for (--i; i>= 0; --i) {
            __mln_bignum_mul(dest, dest);
            if (__mln_bignum_bit_test(exponent, i)) {
                __mln_bignum_mul(dest, &x);
            }
        }
    }

    return 0;
})

MLN_FUNC(static inline, int, __mln_bignum_abs_compare, \
         (mln_bignum_t *bn1, mln_bignum_t *bn2), (bn1, bn2), \
{
    if (bn1->length != bn2->length) {
        if (bn1->length > bn2->length) return 1;
        return -1;
    }
    if (bn1->length == 0) return 0;

    mln_u64_t *data1 = bn1->data + bn1->length - 1, *data2 = bn2->data + bn2->length - 1, *end = bn1->data;
    for (; data1 >= end; --data1, --data2) {
        if (*data1 > *data2) return 1;
        else if (*data1 < *data2) return -1;
    }

    return 0;
})

MLN_FUNC(, int, mln_bignum_abs_compare, (mln_bignum_t *bn1, mln_bignum_t *bn2), (bn1, bn2), {
    return __mln_bignum_abs_compare(bn1, bn2);
})

MLN_FUNC(static inline, int, __mln_bignum_compare, \
         (mln_bignum_t *bn1, mln_bignum_t *bn2), (bn1, bn2), \
{
    if (bn1->length == bn2->length && bn1->length == 0) return 0;

    if (bn1->tag != bn2->tag) {
        if (bn1->tag == M_BIGNUM_POSITIVE) return 1;
        return -1;
    }

    if (bn1->length != bn2->length) {
        if (bn1->tag == M_BIGNUM_POSITIVE) {
            if (bn1->length > bn2->length) return 1;
            return -1;
        } else {
            if (bn1->length > bn2->length) return -1;
            return 1;
        }
    }

    mln_u64_t *data1 = bn1->data+bn1->length-1, *data2 = bn2->data+bn2->length-1, *end = bn1->data;
    for (; data1 >= end; --data1, --data2) {
        if (*data1 > *data2) return 1;
        else if (*data1 < *data2) return -1;
    }

    return 0;
})

MLN_FUNC(, int, mln_bignum_compare, (mln_bignum_t *bn1, mln_bignum_t *bn2), (bn1, bn2), {
    return  __mln_bignum_compare(bn1, bn2);
})

MLN_FUNC(static inline, int, __mln_bignum_bit_test, \
         (mln_bignum_t *bn, mln_u32_t index), (bn, index), \
{
    return ((bn->data[index/32]) & ((mln_u64_t)1 << (index % 32)))? 1: 0;
})

MLN_FUNC(, int, mln_bignum_bit_test, (mln_bignum_t *bn, mln_u32_t index), (bn, index), {
    if (index >= M_BIGNUM_BITS) return 0;

    return __mln_bignum_bit_test(bn, index);
})

MLN_FUNC_VOID(static inline, void, __mln_bignum_left_shift, \
              (mln_bignum_t *bn, mln_u32_t n), (bn, n), \
{
    if (n == 0) return;
    mln_u32_t step = n / 32, off = n % 32, len;
    if (step >= M_BIGNUM_SIZE) return;
    mln_u64_t *s, *d, *end, cur, *last;

    end = bn->data;
    s = bn->data + bn->length - 1;
    if (bn->length + step > M_BIGNUM_SIZE) {
        s -= (bn->length + step - M_BIGNUM_SIZE);
    }
    d = s + step;
    len = d - bn->data;

    if (off) {
        mln_u64_t tmp = 0;
        last = d < bn->data + M_BIGNUM_SIZE-1? (++len, d+1): &tmp;
        for (; s >= end; --s, --d) {
            cur = *d = *s;
            (*last) |= ((cur >> (32 - off)) & 0xffffffff);
            *d = ((*d) << off) & 0xffffffff;
            last = d;
        }
        for (; d >= end; --d) *d = 0;
    } else {
        for (; s >= end; --s, --d) {
            *d = *s;
        }
        for (; d >= end; --d) *d = 0;
    }

    for (d = bn->data+len, end = bn->data; d >= end; --d) {
        if (*d) break;
    }
    bn->length = d < end? 0: d-end+1;
})

MLN_FUNC_VOID(, void, mln_bignum_left_shift, (mln_bignum_t *bn, mln_u32_t n), (bn, n), {
    __mln_bignum_left_shift(bn, n);
})

MLN_FUNC_VOID(static inline, void, __mln_bignum_right_shift, \
              (mln_bignum_t *bn, mln_u32_t n), (bn, n), \
{
    if (n == 0) return;
    mln_u32_t step = n / 32, off = n % 32;
    if (step >= M_BIGNUM_SIZE) return;
    if (step >= bn->length) {
        memset(bn, 0, sizeof(mln_bignum_t));
        return;
    }
    mln_u64_t *s, *d, *end, cur, *last;

    end = bn->data + bn->length;
    s = bn->data + step;
    d = bn->data;

    if (off) {
        mln_u64_t tmp = 0;
        last = &tmp;
        for (; s < end; ++s, ++d) {
            cur = *d = *s;
            (*last) |= ((cur << (32 - off)) & 0xffffffff);
            *d = ((*d) >> off) & 0xffffffff;
            last = d;
        }
        for (s = d; d < end; ++d) *d = 0;
    } else {
        for (; s < end; ++s, ++d) {
            *d = *s;
        }
        for (s = d; d < end; ++d) *d = 0;
    }

    for (d = bn->data; s>=d; --s) {
        if (*s) break;
    }
    bn->length = s < d? 0: s-d+1;
})

MLN_FUNC_VOID(, void, mln_bignum_right_shift, (mln_bignum_t *bn, mln_u32_t n), (bn, n), {
    __mln_bignum_right_shift(bn, n);
})

MLN_FUNC_VOID(static inline, void, __mln_bignum_mul_word, \
              (mln_bignum_t *dest, mln_s64_t src), (dest, src), \
{
    mln_u32_t tag;
    mln_u64_t *dest_data = dest->data;
    mln_u64_t *dend, carry;

    if (src < 0) {
        tag = dest->tag ^ M_BIGNUM_NEGATIVE;
        src = -src;
    } else {
        tag = dest->tag ^ M_BIGNUM_POSITIVE;
    }
    src &= 0xffffffff;

    carry = 0;
    for (dend = dest->data+dest->length; dest_data<dend; ++dest_data) {
        (*dest_data) = (*dest_data) * src + carry;
        carry = *dest_data >> M_BIGNUM_SHIFT;
        *dest_data %= M_BIGNUM_UMAX;
    }
    while (carry) {
        if (dest_data >= dest->data+M_BIGNUM_SIZE) break;
        *dest_data += carry;
        carry = *dest_data >> M_BIGNUM_SHIFT;
        *dest_data %= M_BIGNUM_UMAX;
        ++(dest->length);
    }
    dest->tag = tag;
})

MLN_FUNC(static inline, int, __mln_bignum_div_word, \
         (mln_bignum_t *dest, mln_s64_t src, mln_bignum_t *quotient), \
         (dest, src, quotient), \
{
    if (src == 0) return -1;
    mln_u32_t tag = src < 0? dest->tag ^ M_BIGNUM_NEGATIVE: dest->tag ^ M_BIGNUM_POSITIVE;
    src &= 0xffffffff;
    if (src == 1 || src == -1) {
        dest->tag = tag;
        if (quotient != NULL) *quotient = *dest;
        memset(dest, 0, sizeof(mln_bignum_t));
        return 0;
    }
    if (dest->length == 0) {
        if (quotient != NULL) memset(quotient, 0, sizeof(mln_bignum_t));
        return 0;
    }

    mln_u64_t *dest_data = dest->data + dest->length - 1;
    mln_u64_t *end = dest->data, *data = NULL;
    mln_u64_t tmp = 0;
    if (quotient != NULL) {
        memset(quotient, 0, sizeof(mln_bignum_t));
        quotient->length = dest->length;
        quotient->tag = tag;
    }

    if (*dest_data < src) {
        tmp = *dest_data << M_BIGNUM_SHIFT;
        *dest_data-- = 0;
        if (quotient != NULL) --(quotient->length);
    }
    if (quotient != NULL) data = quotient->data + quotient->length - 1;

    while (dest_data >= end) {
        tmp += *dest_data;
        if (data != NULL) *data-- = tmp / src;
        tmp %= src;
        tmp <<= M_BIGNUM_SHIFT;
        *dest_data-- = 0;
    }

    dest->tag = tag;
    dest->length = tmp? 1: 0;
    dest->data[0] = tmp >> M_BIGNUM_SHIFT;
    return 0;
})

void mln_bignum_dump(mln_bignum_t *bn)
{
    fprintf(stderr, "Tag: %s\n", bn->tag==M_BIGNUM_POSITIVE?"+":"-");
    fprintf(stderr, "Length: %u\n", bn->length);
    mln_u32_t i;
    fprintf(stderr, "Data:\n");
    for (i = 0; i < M_BIGNUM_SIZE; ++i) {
#if defined(MSVC) || defined(i386) || defined(__arm__) || defined(__wasm__)
        fprintf(stderr, "\t%llx\n", bn->data[i]);
#else
        fprintf(stderr, "\t%lx\n", bn->data[i]);
#endif
    }
    fprintf(stderr, "\n");
    fflush(stderr);
}

/*
 * prime
 */
MLN_FUNC_VOID(static inline, void, mln_bignum_seperate, \
              (mln_u32_t *pwr, mln_bignum_t *odd), (pwr, odd), \
{
    mln_bignum_t tmp, mod;
    mln_bignum_t two = {M_BIGNUM_POSITIVE, 1, {0}};
    two.data[0] = 2;
    *pwr = 0;

    while (1) {
        mod = *odd;
        assert(__mln_bignum_div(&mod, &two, &tmp) >= 0);
        if (mod.length) break;

        *odd = tmp;
        ++(*pwr);
    }
})

MLN_FUNC(static inline, mln_u32_t, mln_bignum_witness, \
         (mln_bignum_t *base, mln_bignum_t *prim), (base, prim), \
{
    mln_u32_t pwr = 0, i;
    mln_bignum_t tmp, new_x = {M_BIGNUM_POSITIVE, 0, {0}}, x, odd;
    mln_bignum_t num = {M_BIGNUM_POSITIVE, 1, {0}};
    num.data[0] = 1;

    odd = *prim;
    mln_bignum_sub(&odd, &num);
    mln_bignum_seperate(&pwr, &odd);

    x = *base;
    __mln_bignum_pwr(&x, &odd, prim);

    for (i = 0; i < pwr; ++i) {
        new_x = x;
        num.data[0] = 2;
        assert(__mln_bignum_pwr(&new_x, &num, prim) >= 0);

        num.data[0] = 1;
        tmp = *prim;
        __mln_bignum_sub(&tmp, &num);
        if (__mln_bignum_abs_compare(&new_x, &num) == 0 && \
            __mln_bignum_abs_compare(&x, &num) && \
            __mln_bignum_abs_compare(&x, &tmp))
        {
            return 1;
        }
        x = new_x;
    }

    if (__mln_bignum_abs_compare(&new_x, &num)) {
        return 1;
    }
    return 0;
})

static inline void mln_bignum_random_prime(mln_bignum_t *bn, mln_u32_t bitwidth)
{
    struct timeval tv;
    memset(bn, 0, sizeof(mln_bignum_t));
    mln_bignum_positive(bn);
    mln_u64_t *data = bn->data;
    gettimeofday(&tv, NULL);
    mln_u32_t val = tv.tv_sec*1000000+tv.tv_usec, times = bitwidth / 32, off;
    mln_s32_t i;

    for (i = 0; i < times; ++i) {
#if defined(MSVC)
        rand_s(&val);
#else
        val = (mln_u32_t)rand_r(&val);
#endif
        data[i] = ((mln_u64_t)val & 0xffffffff);
    }

    if ((off = bitwidth % 32)) {
#if defined(MSVC)
        rand_s(&val);
        data[i] = ((mln_u64_t)val * 0xfdfd) & 0xffffffff;
#else
        data[i] = (((mln_u64_t)rand_r(&val) * 0xfdfd) & 0xffffffff);
#endif
        data[i] |= ((mln_u64_t)1 << (off-1));
        data[i] <<= (64 - off);
        data[i] >>= (64 - off);
        bn->length = i+1;
        data[0] |= 1;
    } else {
        if (times == 0) {
            memset(bn, 0, sizeof(mln_bignum_t));
        } else {
            if (data[i-1] == 0) {
                gettimeofday(&tv, NULL);
                val = tv.tv_sec*1000000+tv.tv_usec;
#if defined(MSVC)
                rand_s(&val);
                data[i-1] = (mln_u32_t)val;
#else
                data[i-1] = (mln_u32_t)rand_r(&val);
#endif
            }
            data[i-1] |= 0x80000000;
            bn->length = i;
            data[0] |= 1;
        }
    }
}

static inline void mln_bignum_random_scope(mln_bignum_t *bn, mln_u32_t bitwidth, mln_bignum_t *max)
{
    mln_u32_t width;
    struct timeval tv;
    mln_u32_t val;

lp:
    gettimeofday(&tv, NULL);
    val = tv.tv_sec*1000000+tv.tv_usec;
#if defined(MSVC)
    rand_s(&val);
    width = val % bitwidth;
#else
    width = (mln_u32_t)rand_r(&val) % bitwidth;
#endif
    if (width < 2) goto lp;
    mln_bignum_random_prime(bn, width);
}

MLN_FUNC(, int, mln_bignum_prime, (mln_bignum_t *res, mln_u32_t bitwidth), (res, bitwidth), {
    if (bitwidth > M_BIGNUM_BITS>>1 || bitwidth < 3) return -1;

    mln_bignum_t prime, tmp, one = {M_BIGNUM_POSITIVE, 1, {0}};
    one.data[0] = 1;
    mln_u32_t times;

    while (1) {
        mln_bignum_random_prime(&prime, bitwidth);
        if (__mln_bignum_abs_compare(&prime, &one) <= 0) continue;
        times = bitwidth<=512? 4: bitwidth >> 9;
        for (; times > 0; --times) {
            mln_bignum_random_scope(&tmp, bitwidth, &prime);
            if (mln_bignum_witness(&tmp, &prime)) break;
        }
        if (times == 0) break;
    }
    *res = prime;

    return 0;
})

#if 0
int mln_bignum_extend_eulid(mln_bignum_t *a, mln_bignum_t *b, mln_bignum_t *x, mln_bignum_t *y, mln_bignum_t *gcd)
{
    mln_bignum_t tx, ty, t, tmp;
    mln_bignum_t zero = {M_BIGNUM_POSITIVE, 0, {0}};
    mln_bignum_t one = {M_BIGNUM_POSITIVE, 1, {0}};
    one.data[0] = 1;

    if (!__mln_bignum_compare(b, &zero)) {
        if (x != NULL) *x = one;
        if (y != NULL) *y = zero;
        if (gcd != NULL) *gcd = *a;
        return 0;
    }

    t = *a;
    if (__mln_bignum_div(&t, b, NULL) < 0) return -1;
    if (mln_bignum_extend_eulid(b, &t, &tx, &ty, gcd) < 0) return -1;
    if (x != NULL) *x = ty;
    t = *a;
    if (__mln_bignum_div(&t, b, &tmp) < 0) return -1;
    __mln_bignum_mul(&tmp, &ty);
    __mln_bignum_sub(&tx, &tmp);
    if (y != NULL) *y = tx;
    return 0;
}
#else
MLN_FUNC(, int, mln_bignum_extend_eulid, \
         (mln_bignum_t *a, mln_bignum_t *b, mln_bignum_t *x, mln_bignum_t *y), \
         (a, b, x, y), \
{
    mln_bignum_t m = *a, n = *b;
    mln_bignum_t r, q, tmp;
    mln_bignum_t tmpx, tmpy, zero = mln_bignum_zero();
    mln_bignum_t x0 = {M_BIGNUM_POSITIVE, 1, {0}};
    x0.data[0] = 1;
    mln_bignum_t y0 = zero;
    mln_bignum_t x1 = zero;
    mln_bignum_t y1 = {M_BIGNUM_POSITIVE, 1, {0}};
    y1.data[0] = 1;
    tmpx = x1;
    tmpy = y1;

    r = m;
    if (__mln_bignum_div(&r, &n, &q) < 0) return -1;

    while (__mln_bignum_compare(&r, &zero) > 0) {
        tmp = q;
        __mln_bignum_mul(&tmp, &x1);
        tmpx = x0;
        __mln_bignum_sub(&tmpx, &tmp);

        tmp = q;
        __mln_bignum_mul(&tmp, &y1);
        tmpy = y0;
        __mln_bignum_sub(&tmpy, &tmp);

        x0 = x1;
        y0 = y1;
        x1 = tmpx;
        y1 = tmpy;
        m = n;
        n = r;
        r = m;
        if (__mln_bignum_div(&r, &n, &q) < 0) return -1;
    }

    if (x != NULL) *x = tmpx;
    if (y != NULL) *y = tmpy;

    return 0;
})
#endif

#if 1
MLN_FUNC(, int, mln_bignum_i2osp, (mln_bignum_t *n, mln_u8ptr_t buf, mln_size_t len), (n, buf, len), {
    if (n->tag == M_BIGNUM_NEGATIVE || (n->length<<2) > len) return -1;

    mln_u64_t *p = n->data + n->length - 1, *end = n->data;
    mln_size_t max = len - (n->length << 2);
    while (max--) {
        *buf++ = 0;
    }
    for (; p >= end; --p) {
        *buf++ = (*p >> 24) & 0xff;
        *buf++ = (*p >> 16) & 0xff;
        *buf++ = (*p >> 8) & 0xff;
        *buf++ = *p & 0xff;
    }
    return 0;
})

MLN_FUNC(, int, mln_bignum_os2ip, (mln_bignum_t *n, mln_u8ptr_t buf, mln_size_t len), (n, buf, len), {
    if (len > (M_BIGNUM_SIZE<<2)) return -1;

    *n = (mln_bignum_t)mln_bignum_zero();
    mln_u64_t *data = n->data;
    mln_u8ptr_t p = buf + len - 1;
    mln_size_t i = 0;
    while (p >= buf) {
        (*data) |= (((mln_u64_t)(*p--)) << i);
        if (i >= 24) {
            *data++ &= 0xffffffff;
            i = 0;
            continue;
        }
        i += 8;
    }

    n->length = i? (data-n->data+1): (data-n->data);
    return 0;
})

#else
int mln_bignum_i2osp(mln_bignum_t *n, mln_u8ptr_t buf, mln_size_t len)
{
    if (n->tag == M_BIGNUM_NEGATIVE || (n->length<<2) > len) return -1;

    mln_u64_t *p = n->data, *end = n->data + n->length;
    mln_size_t max = len - (n->length << 2);
    while (max--) {
        *buf++ = 0;
    }
    for (; p < end; ++p) {
        *buf++ = *p & 0xff;
        *buf++ = (*p >> 8) & 0xff;
        *buf++ = (*p >> 16) & 0xff;
        *buf++ = (*p >> 24) & 0xff;
    }
    return 0;
}

int mln_bignum_os2ip(mln_bignum_t *n, mln_u8ptr_t buf, mln_size_t len)
{
    if (len > (M_BIGNUM_SIZE<<2)) return -1;

    *n = (mln_bignum_t)mln_bignum_zero();
    mln_u64_t *data = n->data;
    mln_u8ptr_t end = buf+len;
    mln_size_t i = 0;
    while (buf < end) {
        (*data) |= (((mln_u64_t)(*buf++)) << i);
        if (i >= 24) {
            *data++ &= 0xffffffff;
            i = 0;
            continue;
        }
        i += 8;
    }

    n->length = i? (data-n->data+1): (data-n->data);
    return 0;
}
#endif

MLN_FUNC(, mln_string_t *, mln_bignum_tostring, (mln_bignum_t *n), (n), {
    mln_string_t *ret;
    mln_u8ptr_t buf, p;
    mln_u8_t tmp;
    mln_u32_t i, size = (n->length << 6);
    mln_bignum_t zero = mln_bignum_zero(), quotient;
    mln_u32_t neg = mln_bignum_is_negative(n);
    mln_bignum_t dup = *n;

    if (!size) ++size;
    if ((buf = (mln_u8ptr_t)malloc(size + 1)) == NULL) {
        return NULL;
    }

    p = buf;
    while (mln_bignum_compare(&dup, &zero)) {
        if (__mln_bignum_div_word(&dup, 10, &quotient) < 0) {
            free(buf);
            return NULL;
        }
        *p++ = (mln_u8_t)(dup.data[0]) + (mln_u8_t)'0';
        dup = quotient;
    }
    if (neg) *p++ = (mln_u8_t)'-';

    size = p - buf;
    for (i = 0; i < (size >> 1); ++i) {
        tmp = buf[i];
        buf[i] = buf[size - i - 1];
        buf[size - i - 1] = tmp;
    }
    if (p == buf) *p++ = '0';
    *p = 0;

    if ((ret = mln_string_buf_new(buf, p - buf)) == NULL) {
        free(buf);
        return NULL;
    }

    return ret;
})

