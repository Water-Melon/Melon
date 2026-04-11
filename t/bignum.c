
/*
 * Copyright (C) Niklaus F.Schen.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#if defined(MSVC)
#include <windows.h>
#else
#include <sys/time.h>
#endif
#include "mln_bignum.h"
#include "mln_func.h"

static double now_us(void) {
#if defined(MSVC)
    LARGE_INTEGER freq, cnt;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&cnt);
    return (double)cnt.QuadPart / (double)freq.QuadPart * 1e6;
#else
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1e6 + tv.tv_usec;
#endif
}

static void check_tostring(mln_bignum_t *bn, const char *expected)
{
    mln_string_t *s = mln_bignum_tostring(bn);
    assert(s != NULL);
    assert(s->len == strlen(expected));
    assert(memcmp(s->data, expected, s->len) == 0);
    mln_string_free(s);
}

/* ---- feature tests ---- */

MLN_FUNC_VOID(static, void, test_init_and_zero, (void), (), {
    mln_bignum_t n1, n2 = mln_bignum_zero();
    mln_bignum_init(n1);
    assert(mln_bignum_is_positive(&n1));
    assert(mln_bignum_get_length(&n1) == 0);
    assert(mln_bignum_is_positive(&n2));
    assert(mln_bignum_get_length(&n2) == 0);
    assert(mln_bignum_compare(&n1, &n2) == 0);
    printf("  test_init_and_zero passed\n");
})

MLN_FUNC_VOID(static, void, test_new_free, (void), (), {
    mln_bignum_t *bn = mln_bignum_new();
    assert(bn != NULL);
    assert(mln_bignum_is_positive(bn));
    assert(mln_bignum_get_length(bn) == 0);
    mln_bignum_free(bn);
    mln_bignum_free(NULL); /* should not crash */
    printf("  test_new_free passed\n");
})

MLN_FUNC_VOID(static, void, test_dup, (void), (), {
    mln_bignum_t a;
    mln_bignum_assign(&a, "123456789", 9);
    mln_bignum_t *dup = mln_bignum_dup(&a);
    assert(dup != NULL);
    assert(mln_bignum_compare(&a, dup) == 0);
    check_tostring(dup, "123456789");
    mln_bignum_free(dup);
    printf("  test_dup passed\n");
})

MLN_FUNC_VOID(static, void, test_assign_dec, (void), (), {
    mln_bignum_t bn;
    assert(mln_bignum_assign(&bn, "0", 1) == 0);
    check_tostring(&bn, "0");

    assert(mln_bignum_assign(&bn, "1", 1) == 0);
    check_tostring(&bn, "1");

    assert(mln_bignum_assign(&bn, "999999999", 9) == 0);
    check_tostring(&bn, "999999999");

    assert(mln_bignum_assign(&bn, "1000000000", 10) == 0);
    check_tostring(&bn, "1000000000");

    assert(mln_bignum_assign(&bn, "123456789012345678901234567890", 30) == 0);
    check_tostring(&bn, "123456789012345678901234567890");

    assert(mln_bignum_assign(&bn, "-42", 3) == 0);
    check_tostring(&bn, "-42");
    assert(mln_bignum_is_negative(&bn));

    assert(mln_bignum_assign(&bn, "-99999999999999999999", 21) == 0);
    check_tostring(&bn, "-99999999999999999999");
    printf("  test_assign_dec passed\n");
})

MLN_FUNC_VOID(static, void, test_assign_hex, (void), (), {
    mln_bignum_t bn;
    assert(mln_bignum_assign(&bn, "0xff", 4) == 0);
    check_tostring(&bn, "255");

    assert(mln_bignum_assign(&bn, "0x0", 3) == 0);
    check_tostring(&bn, "0");

    assert(mln_bignum_assign(&bn, "0x100", 5) == 0);
    check_tostring(&bn, "256");

    assert(mln_bignum_assign(&bn, "0xFFFFFFFF", 10) == 0);
    check_tostring(&bn, "4294967295");

    assert(mln_bignum_assign(&bn, "-0xA", 4) == 0);
    check_tostring(&bn, "-10");
    assert(mln_bignum_is_negative(&bn));
    printf("  test_assign_hex passed\n");
})

MLN_FUNC_VOID(static, void, test_assign_oct, (void), (), {
    mln_bignum_t bn;
    assert(mln_bignum_assign(&bn, "0377", 4) == 0);
    check_tostring(&bn, "255");

    assert(mln_bignum_assign(&bn, "00", 2) == 0);
    check_tostring(&bn, "0");

    assert(mln_bignum_assign(&bn, "010", 3) == 0);
    check_tostring(&bn, "8");
    printf("  test_assign_oct passed\n");
})

MLN_FUNC_VOID(static, void, test_positive_negative, (void), (), {
    mln_bignum_t bn;
    mln_bignum_assign(&bn, "42", 2);
    assert(mln_bignum_is_positive(&bn));
    assert(!mln_bignum_is_negative(&bn));

    mln_bignum_negative(&bn);
    assert(mln_bignum_is_negative(&bn));
    assert(!mln_bignum_is_positive(&bn));

    mln_bignum_positive(&bn);
    assert(mln_bignum_is_positive(&bn));
    printf("  test_positive_negative passed\n");
})

MLN_FUNC_VOID(static, void, test_add, (void), (), {
    mln_bignum_t a, b;

    /* basic add */
    mln_bignum_assign(&a, "100", 3);
    mln_bignum_assign(&b, "200", 3);
    mln_bignum_add(&a, &b);
    check_tostring(&a, "300");

    /* add with carry across 32-bit word */
    mln_bignum_assign(&a, "4294967295", 10);
    mln_bignum_assign(&b, "1", 1);
    mln_bignum_add(&a, &b);
    check_tostring(&a, "4294967296");

    /* add two large numbers */
    mln_bignum_assign(&a, "99999999999999999999999999999999999999", 38);
    mln_bignum_assign(&b, "1", 1);
    mln_bignum_add(&a, &b);
    check_tostring(&a, "100000000000000000000000000000000000000");

    /* add negative + positive (positive result) */
    mln_bignum_assign(&a, "-100", 4);
    mln_bignum_assign(&b, "200", 3);
    mln_bignum_add(&a, &b);
    check_tostring(&a, "100");

    /* add positive + negative (negative result) */
    mln_bignum_assign(&a, "100", 3);
    mln_bignum_assign(&b, "-200", 4);
    mln_bignum_add(&a, &b);
    check_tostring(&a, "-100");

    /* add zero */
    mln_bignum_assign(&a, "42", 2);
    mln_bignum_assign(&b, "0", 1);
    mln_bignum_add(&a, &b);
    check_tostring(&a, "42");
    printf("  test_add passed\n");
})

MLN_FUNC_VOID(static, void, test_sub, (void), (), {
    mln_bignum_t a, b;

    /* basic sub */
    mln_bignum_assign(&a, "300", 3);
    mln_bignum_assign(&b, "200", 3);
    mln_bignum_sub(&a, &b);
    check_tostring(&a, "100");

    /* sub to negative */
    mln_bignum_assign(&a, "100", 3);
    mln_bignum_assign(&b, "200", 3);
    mln_bignum_sub(&a, &b);
    check_tostring(&a, "-100");

    /* sub equal numbers */
    mln_bignum_assign(&a, "42", 2);
    mln_bignum_assign(&b, "42", 2);
    mln_bignum_sub(&a, &b);
    check_tostring(&a, "0");

    /* sub with borrow across words */
    mln_bignum_assign(&a, "4294967296", 10);
    mln_bignum_assign(&b, "1", 1);
    mln_bignum_sub(&a, &b);
    check_tostring(&a, "4294967295");

    /* sub zero */
    mln_bignum_assign(&a, "42", 2);
    mln_bignum_assign(&b, "0", 1);
    mln_bignum_sub(&a, &b);
    check_tostring(&a, "42");
    printf("  test_sub passed\n");
})

MLN_FUNC_VOID(static, void, test_mul, (void), (), {
    mln_bignum_t a, b;

    /* basic mul */
    mln_bignum_assign(&a, "12345", 5);
    mln_bignum_assign(&b, "67890", 5);
    mln_bignum_mul(&a, &b);
    check_tostring(&a, "838102050");

    /* mul by zero */
    mln_bignum_assign(&a, "12345", 5);
    mln_bignum_assign(&b, "0", 1);
    mln_bignum_mul(&a, &b);
    check_tostring(&a, "0");

    /* mul by one */
    mln_bignum_assign(&a, "12345", 5);
    mln_bignum_assign(&b, "1", 1);
    mln_bignum_mul(&a, &b);
    check_tostring(&a, "12345");

    /* mul large numbers */
    mln_bignum_assign(&a, "999999999999999999", 18);
    mln_bignum_assign(&b, "999999999999999999", 18);
    mln_bignum_mul(&a, &b);
    check_tostring(&a, "999999999999999998000000000000000001");

    /* mul with mixed signs */
    mln_bignum_assign(&a, "-5", 2);
    mln_bignum_assign(&b, "3", 1);
    mln_bignum_mul(&a, &b);
    check_tostring(&a, "-15");

    /* mul negative * negative = positive */
    mln_bignum_assign(&a, "-5", 2);
    mln_bignum_assign(&b, "-3", 2);
    mln_bignum_mul(&a, &b);
    check_tostring(&a, "15");
    printf("  test_mul passed\n");
})

MLN_FUNC_VOID(static, void, test_div, (void), (), {
    mln_bignum_t a, b, q;

    /* basic div with remainder */
    mln_bignum_assign(&a, "1000", 4);
    mln_bignum_assign(&b, "3", 1);
    assert(mln_bignum_div(&a, &b, &q) == 0);
    check_tostring(&q, "333");
    check_tostring(&a, "1");

    /* exact division */
    mln_bignum_assign(&a, "100", 3);
    mln_bignum_assign(&b, "10", 2);
    assert(mln_bignum_div(&a, &b, &q) == 0);
    check_tostring(&q, "10");
    check_tostring(&a, "0");

    /* divide by itself */
    mln_bignum_assign(&a, "42", 2);
    mln_bignum_assign(&b, "42", 2);
    assert(mln_bignum_div(&a, &b, &q) == 0);
    check_tostring(&q, "1");

    /* divide smaller by larger */
    mln_bignum_assign(&a, "3", 1);
    mln_bignum_assign(&b, "1000", 4);
    assert(mln_bignum_div(&a, &b, &q) == 0);
    check_tostring(&q, "0");
    check_tostring(&a, "3");

    /* divide by zero */
    mln_bignum_assign(&a, "42", 2);
    mln_bignum_assign(&b, "0", 1);
    assert(mln_bignum_div(&a, &b, &q) == -1);

    /* div without quotient */
    mln_bignum_assign(&a, "100", 3);
    mln_bignum_assign(&b, "7", 1);
    assert(mln_bignum_div(&a, &b, NULL) == 0);
    check_tostring(&a, "2");

    /* large division */
    mln_bignum_assign(&a, "123456789012345678901234567890", 30);
    mln_bignum_assign(&b, "1000000000", 10);
    assert(mln_bignum_div(&a, &b, &q) == 0);
    check_tostring(&q, "123456789012345678901");
    check_tostring(&a, "234567890");
    printf("  test_div passed\n");
})

MLN_FUNC_VOID(static, void, test_pwr, (void), (), {
    mln_bignum_t a, b, c;

    /* 10^30 */
    mln_bignum_assign(&a, "10", 2);
    mln_bignum_assign(&b, "30", 2);
    assert(mln_bignum_pwr(&a, &b, NULL) == 0);
    check_tostring(&a, "1000000000000000000000000000000");

    /* 2^10 */
    mln_bignum_assign(&a, "2", 1);
    mln_bignum_assign(&b, "10", 2);
    assert(mln_bignum_pwr(&a, &b, NULL) == 0);
    check_tostring(&a, "1024");

    /* x^0 = 1 */
    mln_bignum_assign(&a, "42", 2);
    mln_bignum_assign(&b, "0", 1);
    assert(mln_bignum_pwr(&a, &b, NULL) == 0);
    check_tostring(&a, "1");

    /* x^1 = x */
    mln_bignum_assign(&a, "42", 2);
    mln_bignum_assign(&b, "1", 1);
    assert(mln_bignum_pwr(&a, &b, NULL) == 0);
    check_tostring(&a, "42");

    /* pwr with mod: 2^10 mod 1000 = 24 */
    mln_bignum_assign(&a, "2", 1);
    mln_bignum_assign(&b, "10", 2);
    mln_bignum_assign(&c, "1000", 4);
    assert(mln_bignum_pwr(&a, &b, &c) == 0);
    check_tostring(&a, "24");

    /* negative exponent should fail */
    mln_bignum_assign(&a, "2", 1);
    mln_bignum_assign(&b, "-1", 2);
    assert(mln_bignum_pwr(&a, &b, NULL) == -1);
    printf("  test_pwr passed\n");
})

MLN_FUNC_VOID(static, void, test_compare, (void), (), {
    mln_bignum_t a, b;

    mln_bignum_assign(&a, "100", 3);
    mln_bignum_assign(&b, "200", 3);
    assert(mln_bignum_compare(&a, &b) < 0);
    assert(mln_bignum_compare(&b, &a) > 0);
    assert(mln_bignum_compare(&a, &a) == 0);

    /* compare with negative */
    mln_bignum_assign(&a, "-100", 4);
    mln_bignum_assign(&b, "100", 3);
    assert(mln_bignum_compare(&a, &b) < 0);
    assert(mln_bignum_compare(&b, &a) > 0);

    /* compare two negatives (different length) */
    mln_bignum_assign(&a, "-4294967296", 11);
    mln_bignum_assign(&b, "-100", 4);
    assert(mln_bignum_compare(&a, &b) < 0);
    assert(mln_bignum_compare(&b, &a) > 0);

    /* compare zeros */
    mln_bignum_assign(&a, "0", 1);
    mln_bignum_assign(&b, "0", 1);
    assert(mln_bignum_compare(&a, &b) == 0);

    /* abs_compare */
    mln_bignum_assign(&a, "-200", 4);
    mln_bignum_assign(&b, "100", 3);
    assert(mln_bignum_abs_compare(&a, &b) > 0);
    assert(mln_bignum_abs_compare(&b, &a) < 0);
    printf("  test_compare passed\n");
})

MLN_FUNC_VOID(static, void, test_shifts, (void), (), {
    mln_bignum_t a;

    /* left shift by 32 */
    mln_bignum_assign(&a, "1", 1);
    mln_bignum_left_shift(&a, 32);
    check_tostring(&a, "4294967296");

    /* right shift by 32 */
    mln_bignum_assign(&a, "4294967296", 10);
    mln_bignum_right_shift(&a, 32);
    check_tostring(&a, "1");

    /* left shift by 1 */
    mln_bignum_assign(&a, "1", 1);
    mln_bignum_left_shift(&a, 1);
    check_tostring(&a, "2");

    /* right shift by 1 */
    mln_bignum_assign(&a, "4", 1);
    mln_bignum_right_shift(&a, 1);
    check_tostring(&a, "2");

    /* shift by 0 */
    mln_bignum_assign(&a, "42", 2);
    mln_bignum_left_shift(&a, 0);
    check_tostring(&a, "42");
    mln_bignum_right_shift(&a, 0);
    check_tostring(&a, "42");

    /* large shift */
    mln_bignum_assign(&a, "1", 1);
    mln_bignum_left_shift(&a, 64);
    check_tostring(&a, "18446744073709551616");
    printf("  test_shifts passed\n");
})

MLN_FUNC_VOID(static, void, test_bit_test, (void), (), {
    mln_bignum_t a;
    mln_bignum_assign(&a, "0xff", 4);

    assert(mln_bignum_bit_test(&a, 0) == 1);
    assert(mln_bignum_bit_test(&a, 7) == 1);
    assert(mln_bignum_bit_test(&a, 8) == 0);

    /* out of range */
    assert(mln_bignum_bit_test(&a, M_BIGNUM_BITS) == 0);
    assert(mln_bignum_bit_test(&a, M_BIGNUM_BITS + 1) == 0);

    mln_bignum_assign(&a, "1", 1);
    assert(mln_bignum_bit_test(&a, 0) == 1);
    assert(mln_bignum_bit_test(&a, 1) == 0);
    printf("  test_bit_test passed\n");
})

MLN_FUNC_VOID(static, void, test_i2osp_os2ip, (void), (), {
    mln_bignum_t a, b;
    mln_u8_t buf[32];

    mln_bignum_assign(&a, "256", 3);
    memset(buf, 0, sizeof(buf));
    assert(mln_bignum_i2osp(&a, buf, 32) == 0);
    memset(&b, 0, sizeof(b));
    assert(mln_bignum_os2ip(&b, buf, 32) == 0);
    check_tostring(&a, "256");
    check_tostring(&b, "256");

    /* negative number should fail for i2osp */
    mln_bignum_assign(&a, "-1", 2);
    assert(mln_bignum_i2osp(&a, buf, 32) == -1);
    printf("  test_i2osp_os2ip passed\n");
})

MLN_FUNC_VOID(static, void, test_tostring, (void), (), {
    mln_bignum_t a;

    mln_bignum_assign(&a, "0", 1);
    check_tostring(&a, "0");

    mln_bignum_assign(&a, "1", 1);
    check_tostring(&a, "1");

    mln_bignum_assign(&a, "-1", 2);
    check_tostring(&a, "-1");

    mln_bignum_assign(&a, "123456789012345678901234567890", 30);
    check_tostring(&a, "123456789012345678901234567890");

    /* number with trailing zeros */
    mln_bignum_assign(&a, "10000000000000000000", 20);
    check_tostring(&a, "10000000000000000000");

    /* negative zero should print as "0", not "-" */
    mln_bignum_assign(&a, "0", 1);
    mln_bignum_negative(&a);
    check_tostring(&a, "0");
    printf("  test_tostring passed\n");
})

MLN_FUNC_VOID(static, void, test_extend_eulid, (void), (), {
    mln_bignum_t a, b, x, y;
    mln_bignum_assign(&a, "35", 2);
    mln_bignum_assign(&b, "15", 2);
    assert(mln_bignum_extend_eulid(&a, &b, &x, &y) == 0);
    /* Verify: a*x + b*y = gcd(35,15) = 5 */
    mln_bignum_t ax = a, by = b;
    mln_bignum_mul(&ax, &x);
    mln_bignum_mul(&by, &y);
    mln_bignum_add(&ax, &by);
    check_tostring(&ax, "5");
    printf("  test_extend_eulid passed\n");
})

/* ---- performance benchmarks ---- */

MLN_FUNC_VOID(static, void, bench_assign_dec, (void), (), {
    int i, N = 10000;
    mln_bignum_t a;
    double t0 = now_us();
    for (i = 0; i < N; i++) {
        mln_bignum_assign(&a, "123456789012345678901234567890", 30);
    }
    double t1 = now_us();
    printf("  assign_dec x%d: %.1f us\n", N, t1 - t0);
})

MLN_FUNC_VOID(static, void, bench_assign_hex, (void), (), {
    int i, N = 10000;
    mln_bignum_t a;
    double t0 = now_us();
    for (i = 0; i < N; i++) {
        mln_bignum_assign(&a, "0x5C2A04837BF02B8F7D7C4E43", 26);
    }
    double t1 = now_us();
    printf("  assign_hex x%d: %.1f us\n", N, t1 - t0);
})

MLN_FUNC_VOID(static, void, bench_add, (void), (), {
    int i, N = 10000;
    mln_bignum_t a, b, c;
    mln_bignum_assign(&a, "0x5C2A04837BF02B8F7D7C4E43", 26);
    mln_bignum_assign(&b, "0x3A1B2C3D4E5F60718293A4B5", 26);
    double t0 = now_us();
    for (i = 0; i < N; i++) {
        c = a;
        mln_bignum_add(&c, &b);
    }
    double t1 = now_us();
    printf("  add x%d: %.1f us\n", N, t1 - t0);
})

MLN_FUNC_VOID(static, void, bench_sub, (void), (), {
    int i, N = 10000;
    mln_bignum_t a, b, c;
    mln_bignum_assign(&a, "0x5C2A04837BF02B8F7D7C4E43", 26);
    mln_bignum_assign(&b, "0x3A1B2C3D4E5F60718293A4B5", 26);
    double t0 = now_us();
    for (i = 0; i < N; i++) {
        c = a;
        mln_bignum_sub(&c, &b);
    }
    double t1 = now_us();
    printf("  sub x%d: %.1f us\n", N, t1 - t0);
})

MLN_FUNC_VOID(static, void, bench_mul, (void), (), {
    int i, N = 10000;
    mln_bignum_t a, b, c;
    mln_bignum_assign(&a, "0x5C2A04837BF02B8F7D7C4E43", 26);
    mln_bignum_assign(&b, "0x3A1B2C3D4E5F60718293A4B5", 26);
    double t0 = now_us();
    for (i = 0; i < N; i++) {
        c = a;
        mln_bignum_mul(&c, &b);
    }
    double t1 = now_us();
    printf("  mul x%d: %.1f us\n", N, t1 - t0);
})

MLN_FUNC_VOID(static, void, bench_div, (void), (), {
    int i, N = 10000;
    mln_bignum_t a, b, c, q;
    mln_bignum_assign(&a, "0x5C2A04837BF02B8F7D7C4E43", 26);
    mln_bignum_assign(&b, "0x3A1B2C3D4E5F60718293A4B5", 26);
    double t0 = now_us();
    for (i = 0; i < N; i++) {
        c = a;
        mln_bignum_div(&c, &b, &q);
    }
    double t1 = now_us();
    printf("  div x%d: %.1f us\n", N, t1 - t0);
})

MLN_FUNC_VOID(static, void, bench_shifts, (void), (), {
    int i, N = 10000;
    mln_bignum_t a, c;
    mln_bignum_assign(&a, "0x5C2A04837BF02B8F7D7C4E43", 26);
    double t0 = now_us();
    for (i = 0; i < N; i++) {
        c = a;
        mln_bignum_left_shift(&c, 64);
    }
    double t1 = now_us();
    printf("  left_shift x%d: %.1f us\n", N, t1 - t0);

    t0 = now_us();
    for (i = 0; i < N; i++) {
        c = a;
        mln_bignum_right_shift(&c, 32);
    }
    t1 = now_us();
    printf("  right_shift x%d: %.1f us\n", N, t1 - t0);
})

MLN_FUNC_VOID(static, void, bench_compare, (void), (), {
    int i, N = 10000;
    mln_bignum_t a, b;
    mln_bignum_assign(&a, "0x5C2A04837BF02B8F7D7C4E43", 26);
    mln_bignum_assign(&b, "0x3A1B2C3D4E5F60718293A4B5", 26);
    double t0 = now_us();
    for (i = 0; i < N; i++) {
        mln_bignum_compare(&a, &b);
    }
    double t1 = now_us();
    printf("  compare x%d: %.1f us\n", N, t1 - t0);
})

MLN_FUNC_VOID(static, void, bench_tostring, (void), (), {
    int i, N = 1000;
    mln_bignum_t a;
    mln_bignum_assign(&a, "0x5C2A04837BF02B8F7D7C4E43", 26);
    double t0 = now_us();
    for (i = 0; i < N; i++) {
        mln_string_t *s = mln_bignum_tostring(&a);
        mln_string_free(s);
    }
    double t1 = now_us();
    printf("  tostring x%d: %.1f us\n", N, t1 - t0);
})

MLN_FUNC_VOID(static, void, bench_pwr, (void), (), {
    int i, N = 1000;
    mln_bignum_t a, b, c;
    mln_bignum_assign(&a, "10", 2);
    mln_bignum_assign(&b, "30", 2);
    double t0 = now_us();
    for (i = 0; i < N; i++) {
        c = a;
        mln_bignum_pwr(&c, &b, NULL);
    }
    double t1 = now_us();
    printf("  pwr(10,30) x%d: %.1f us\n", N, t1 - t0);
})

int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    printf("=== Feature Tests ===\n");
    test_init_and_zero();
    test_new_free();
    test_dup();
    test_assign_dec();
    test_assign_hex();
    test_assign_oct();
    test_positive_negative();
    test_add();
    test_sub();
    test_mul();
    test_div();
    test_pwr();
    test_compare();
    test_shifts();
    test_bit_test();
    test_i2osp_os2ip();
    test_tostring();
    test_extend_eulid();
    printf("All feature tests passed!\n\n");

    printf("=== Performance Benchmarks ===\n");
    bench_assign_dec();
    bench_assign_hex();
    bench_add();
    bench_sub();
    bench_mul();
    bench_div();
    bench_shifts();
    bench_compare();
    bench_tostring();
    bench_pwr();
    printf("All benchmarks complete.\n");

    return 0;
}
