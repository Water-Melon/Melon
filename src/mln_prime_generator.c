
/*
 * Copyright (C) Niklaus F.Schen.
 */

#include <stdio.h>
#include <stdlib.h>
#if defined(MSVC)
#include "mln_utils.h"
#else
#include <sys/time.h>
#endif
#include "mln_types.h"
#include "mln_func.h"

static inline mln_u32_t rand_scope(mln_u32_t low, mln_u32_t high);
static inline void seperate(mln_u32_t num, mln_u32_t *pwr, mln_u32_t *odd);
static inline mln_u64_t modular_expoinentiation(mln_u32_t base, mln_u32_t pwr, mln_u32_t n);
static inline mln_u32_t witness(mln_u32_t base, mln_u32_t prim);

MLN_FUNC(, mln_u32_t, mln_prime_generate, (mln_u32_t n), (n), {
    if (n <= 2) return 2;
    if (n >= 1073741824) return 1073741827;
    mln_u32_t a, prim = n%2 ? n : n+1;
    int s;
    while (1) {
        s = prim<=4 ? 1 : (prim - 1)>>2;
        for(; s>=0; --s) {
            a = rand_scope(n, prim);
            if (witness(a, prim)) {
                break;
            }
        }
        if (s < 0) break;
        prim += 2;
    }
    return prim;
})

MLN_FUNC(static inline, mln_u32_t, rand_scope, \
         (mln_u32_t low, mln_u32_t high), (low, high), \
{
    struct timeval tv;
    mln_u32_t r = 0;
    while (!r) {
        tv.tv_usec = tv.tv_sec = 0;
        gettimeofday(&tv, NULL);
        srand(tv.tv_sec*1000000+tv.tv_usec);
        r = ((mln_u32_t)rand() + low) % high;
    }
    return r;
})

MLN_FUNC_VOID(static inline, void, seperate, \
              (mln_u32_t num, mln_u32_t *pwr, mln_u32_t *odd), \
              (num, pwr, odd), \
{
    *pwr = 0;
    while (!(num % 2)) {
        num >>= 1;
        ++(*pwr);
    }
    *odd = num;
})

MLN_FUNC(static inline, mln_u64_t, modular_expoinentiation, \
         (mln_u32_t base, mln_u32_t pwr, mln_u32_t n), \
         (base, pwr, n), \
{
    int i;
    mln_u64_t d = 1;
    for (i = sizeof(pwr)*8-1; i>=0; --i) {
        d *= d;
        d %= n;
        if ((1<<i) & pwr) {
            d *= base;
            d %= n;
        }
    }
    return d;
})

MLN_FUNC(static inline, mln_u32_t, witness, \
         (mln_u32_t base, mln_u32_t prim), (base, prim), \
{
    mln_u32_t pwr = 0, odd = 0;
    seperate(prim - 1, &pwr, &odd);
    mln_u64_t new_x = 0, x = modular_expoinentiation(base, odd, prim);
    mln_u32_t i;
    for (i = 0; i<pwr; ++i) {
        new_x = (x * x) % prim;
        if (new_x == 1 && x != 1 && x != prim-1) {
            return 1;
        }
        x = new_x;
    }
    if (new_x != 1) {
        return 1;
    }
    return 0;
})

