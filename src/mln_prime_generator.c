
/*
 * Copyright (C) Niklaus F.Schen.
 */

#include <stdio.h>
#include <stdlib.h>
#include "mln_types.h"
#include "mln_func.h"

/*
 * Small primes for trial division to quickly reject composites.
 */
static const mln_u32_t small_primes[] = {
    2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53
};
static const mln_u32_t small_primes_count = sizeof(small_primes) / sizeof(small_primes[0]);

/*
 * Deterministic witnesses sufficient for all n < 4,759,123,141 (covers all u32).
 */
static const mln_u32_t witnesses[] = {2, 7, 61};
static const mln_u32_t witnesses_count = sizeof(witnesses) / sizeof(witnesses[0]);

static inline mln_u64_t prime_mod_exp(mln_u64_t base, mln_u32_t pwr, mln_u32_t n);
static inline mln_u32_t prime_is_composite_witness(mln_u32_t base, mln_u32_t pwr, mln_u32_t odd, mln_u32_t prim);
static inline mln_u32_t prime_is_prime(mln_u32_t n);

MLN_FUNC(static inline, mln_u64_t, prime_mod_exp, \
         (mln_u64_t base, mln_u32_t pwr, mln_u32_t n), \
         (base, pwr, n), \
{
    mln_u64_t result = 1;
    base %= n;
    while (pwr > 0) {
        if (pwr & 1) {
            result = (result * base) % n;
        }
        pwr >>= 1;
        base = (base * base) % n;
    }
    return result;
})

MLN_FUNC(static inline, mln_u32_t, prime_is_composite_witness, \
         (mln_u32_t base, mln_u32_t pwr, mln_u32_t odd, mln_u32_t prim), \
         (base, pwr, odd, prim), \
{
    mln_u64_t x = prime_mod_exp((mln_u64_t)base, odd, prim);
    mln_u64_t new_x;
    mln_u32_t i;

    if (x == 1 || x == (mln_u64_t)(prim - 1))
        return 0;

    for (i = 1; i < pwr; ++i) {
        new_x = (x * x) % prim;
        if (new_x == 1) return 1;
        if (new_x == (mln_u64_t)(prim - 1)) return 0;
        x = new_x;
    }
    return 1;
})

MLN_FUNC(static inline, mln_u32_t, prime_is_prime, (mln_u32_t n), (n), {
    mln_u32_t i, pwr, odd, d;

    if (n < 2) return 0;

    /* Trial division by small primes */
    for (i = 0; i < small_primes_count; ++i) {
        if (n == small_primes[i]) return 1;
        if (n % small_primes[i] == 0) return 0;
    }

    /* Decompose n-1 = 2^pwr * odd */
    d = n - 1;
    pwr = 0;
    while (!(d & 1)) {
        d >>= 1;
        ++pwr;
    }
    odd = d;

    /* Deterministic Miller-Rabin with fixed witnesses */
    for (i = 0; i < witnesses_count; ++i) {
        if (witnesses[i] >= n) continue;
        if (prime_is_composite_witness(witnesses[i], pwr, odd, n))
            return 0;
    }
    return 1;
})

MLN_FUNC(, mln_u32_t, mln_prime_generate, (mln_u32_t n), (n), {
    if (n <= 2) return 2;
    if (n >= 1073741824) return 1073741827;

    /* Start with an odd candidate */
    mln_u32_t prim = n | 1;

    while (!prime_is_prime(prim)) {
        prim += 2;
    }
    return prim;
})
