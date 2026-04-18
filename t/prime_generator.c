#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#if defined(MSVC)
#include "mln_utils.h"
#else
#include <sys/time.h>
#endif
#include "mln_prime_generator.h"

/*
 * Naive primality test for verification.
 */
static int naive_is_prime(mln_u32_t n)
{
    mln_u32_t i;
    if (n < 2) return 0;
    if (n < 4) return 1;
    if (n % 2 == 0 || n % 3 == 0) return 0;
    for (i = 5; (mln_u64_t)i * i <= n; i += 6) {
        if (n % i == 0 || n % (i + 2) == 0) return 0;
    }
    return 1;
}

static int test_basic(void)
{
    mln_u32_t r;

    /* n=0,1,2 should all return 2 */
    r = mln_prime_generate(0);
    assert(r == 2);
    r = mln_prime_generate(1);
    assert(r == 2);
    r = mln_prime_generate(2);
    assert(r == 2);

    /* n=3 should return 3 */
    r = mln_prime_generate(3);
    assert(r == 3);

    /* n=4 should return 5 */
    r = mln_prime_generate(4);
    assert(r == 5);

    /* n=5 should return 5 */
    r = mln_prime_generate(5);
    assert(r == 5);

    printf("  basic tests passed\n");
    return 0;
}

static int test_known_primes(void)
{
    /* Test that known primes return themselves */
    mln_u32_t known[] = {7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47,
                         97, 101, 127, 251, 509, 1021, 2039, 4093, 8191,
                         65537, 131071, 524287, 1048583};
    mln_u32_t i;

    for (i = 0; i < sizeof(known)/sizeof(known[0]); ++i) {
        mln_u32_t r = mln_prime_generate(known[i]);
        assert(r == known[i]);
    }

    printf("  known primes tests passed\n");
    return 0;
}

static int test_composite_inputs(void)
{
    /* Test that composite inputs produce the next prime */
    mln_u32_t composites[][2] = {
        {4, 5}, {6, 7}, {8, 11}, {9, 11}, {10, 11},
        {14, 17}, {15, 17}, {16, 17}, {20, 23}, {24, 29},
        {32765, 32771}, {100, 101}, {1000, 1009},
        {10000, 10007}, {100000, 100003}, {1000000, 1000003}
    };
    mln_u32_t i;

    for (i = 0; i < sizeof(composites)/sizeof(composites[0]); ++i) {
        mln_u32_t r = mln_prime_generate(composites[i][0]);
        assert(r == composites[i][1]);
    }

    printf("  composite input tests passed\n");
    return 0;
}

static int test_upper_bound(void)
{
    mln_u32_t r;

    /* Upper boundary */
    r = mln_prime_generate(1073741824);
    assert(r == 1073741827);
    r = mln_prime_generate(1073741825);
    assert(r == 1073741827);
    r = mln_prime_generate(0xFFFFFFFF);
    assert(r == 1073741827);

    printf("  upper bound tests passed\n");
    return 0;
}

static int test_correctness_sweep(void)
{
    /*
     * Sweep a range and verify every result is prime
     * and >= input using naive check.
     */
    mln_u32_t i, r;

    /* Dense check for small values 0..2000 */
    for (i = 0; i <= 2000; ++i) {
        r = mln_prime_generate(i);
        assert(r >= i);
        assert(naive_is_prime(r));
        /* Verify no prime was skipped */
        if (r > i) {
            mln_u32_t j;
            for (j = i; j < r; ++j) {
                assert(!naive_is_prime(j));
            }
        }
    }

    /* Spot check larger values */
    for (i = 10000; i <= 12000; ++i) {
        r = mln_prime_generate(i);
        assert(r >= i);
        assert(naive_is_prime(r));
    }

    printf("  correctness sweep passed\n");
    return 0;
}

static int test_stability(void)
{
    /*
     * Call the same input many times to ensure deterministic results.
     */
    mln_u32_t inputs[] = {1, 100, 10000, 1000000, 1073741824};
    mln_u32_t i, j, expected, r;

    for (i = 0; i < sizeof(inputs)/sizeof(inputs[0]); ++i) {
        expected = mln_prime_generate(inputs[i]);
        for (j = 0; j < 1000; ++j) {
            r = mln_prime_generate(inputs[i]);
            assert(r == expected);
        }
    }

    printf("  stability tests passed\n");
    return 0;
}

static int test_performance(void)
{
    struct timeval start, end;
    mln_u32_t i, r;
    double elapsed;
    int count = 100000;

    gettimeofday(&start, NULL);
    for (i = 0; i < (mln_u32_t)count; ++i) {
        r = mln_prime_generate(i * 10 + 1);
        (void)r;
    }
    gettimeofday(&end, NULL);

    elapsed = (end.tv_sec - start.tv_sec) * 1000.0
            + (end.tv_usec - start.tv_usec) / 1000.0;

    printf("  performance: %d calls in %.2f ms (%.2f us/call)\n",
           count, elapsed, elapsed * 1000.0 / count);

    /* Large value performance */
    count = 10000;
    gettimeofday(&start, NULL);
    for (i = 0; i < (mln_u32_t)count; ++i) {
        r = mln_prime_generate(1000000 + i * 100);
        (void)r;
    }
    gettimeofday(&end, NULL);

    elapsed = (end.tv_sec - start.tv_sec) * 1000.0
            + (end.tv_usec - start.tv_usec) / 1000.0;

    printf("  performance (large): %d calls in %.2f ms (%.2f us/call)\n",
           count, elapsed, elapsed * 1000.0 / count);

    return 0;
}

int main(void)
{
    printf("prime_generator tests:\n");
    test_basic();
    test_known_primes();
    test_composite_inputs();
    test_upper_bound();
    test_correctness_sweep();
    test_stability();
    test_performance();
    printf("ALL TESTS PASSED\n");
    return 0;
}
