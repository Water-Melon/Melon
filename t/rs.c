#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include "mln_rs.h"

static void verify_block(mln_rs_result_t *result, int index, const uint8_t *expected, int len)
{
    uint8_t *p = mln_rs_result_get_data_by_index(result, index);
    assert(p != NULL);
    assert(memcmp(p, expected, len) == 0);
}

/*
 * Test basic encode/decode round-trip
 */
static void test_basic_encode_decode(void)
{
    int i, j;
    mln_rs_result_t *res, *dres;
    uint8_t origin[40] = "AAAABBBBCCCCDDDDEEEEFFFFGGGGHHHH11112222";
    uint8_t *err[6] = {0};

    res = mln_rs_encode(origin, 10, 4, 2);
    assert(res != NULL);
    assert(mln_rs_result_get_num(res) == 6);

    for (i = 0; i < 6; ++i)
        err[i] = mln_rs_result_get_data_by_index(res, i);

    /* Verify encoded data blocks match original */
    for (i = 0; i < 4; ++i) {
        assert(memcmp(err[i], origin + i * 10, 10) == 0);
    }

    /* Erase first 2 data blocks */
    err[0] = NULL;
    err[1] = NULL;

    dres = mln_rs_decode(err, 10, 4, 2);
    assert(dres != NULL);
    assert(mln_rs_result_get_num(dres) == 4);

    /* Verify all decoded blocks match original */
    for (i = 0; i < 4; ++i) {
        uint8_t *p = mln_rs_result_get_data_by_index(dres, i);
        for (j = 0; j < 10; ++j) {
            assert(p[j] == origin[i * 10 + j]);
        }
    }

    mln_rs_result_free(res);
    mln_rs_result_free(dres);
    printf("PASS: test_basic_encode_decode\n");
}

/*
 * Test various n, k, len configurations
 */
static void test_different_parameters(void)
{
    int i;
    struct { int n; int k; int len; } cases[] = {
        {2, 1, 4},
        {3, 1, 8},
        {4, 2, 16},
        {8, 3, 32},
        {10, 4, 100},
        {1, 1, 5},
    };
    int ncases = (int)(sizeof(cases) / sizeof(cases[0]));
    int c;

    for (c = 0; c < ncases; ++c) {
        int n = cases[c].n, k = cases[c].k, len = cases[c].len;
        uint8_t *data = (uint8_t *)malloc(n * len);
        assert(data != NULL);

        /* Fill with known pattern */
        for (i = 0; i < n * len; ++i) data[i] = (uint8_t)((i * 37 + 13) & 0xff);

        mln_rs_result_t *res = mln_rs_encode(data, len, n, k);
        assert(res != NULL);
        assert((int)mln_rs_result_get_num(res) == n + k);

        /* Erase first k data blocks, use parity to recover */
        uint8_t **err = (uint8_t **)calloc(n + k, sizeof(uint8_t *));
        assert(err != NULL);
        for (i = 0; i < n + k; ++i)
            err[i] = mln_rs_result_get_data_by_index(res, i);
        for (i = 0; i < k; ++i)
            err[i] = NULL;

        mln_rs_result_t *dres = mln_rs_decode(err, len, n, k);
        assert(dres != NULL);

        /* Verify decoded matches original */
        for (i = 0; i < n; ++i) {
            uint8_t *p = mln_rs_result_get_data_by_index(dres, i);
            assert(memcmp(p, data + i * len, len) == 0);
        }

        mln_rs_result_free(res);
        mln_rs_result_free(dres);
        free(err);
        free(data);
    }
    printf("PASS: test_different_parameters\n");
}

/*
 * Test various erasure patterns
 */
static void test_erasure_patterns(void)
{
    int i;
    int n = 4, k = 2, len = 8;
    uint8_t data[32];
    for (i = 0; i < 32; ++i) data[i] = (uint8_t)(i + 1);

    mln_rs_result_t *res = mln_rs_encode(data, len, n, k);
    assert(res != NULL);

    /* Pattern 1: erase first 2 data blocks */
    {
        uint8_t *err[6];
        for (i = 0; i < 6; ++i) err[i] = mln_rs_result_get_data_by_index(res, i);
        err[0] = NULL;
        err[1] = NULL;
        mln_rs_result_t *d = mln_rs_decode(err, len, n, k);
        assert(d != NULL);
        for (i = 0; i < n; ++i)
            verify_block(d, i, data + i * len, len);
        mln_rs_result_free(d);
    }

    /* Pattern 2: erase last 2 data blocks */
    {
        uint8_t *err[6];
        for (i = 0; i < 6; ++i) err[i] = mln_rs_result_get_data_by_index(res, i);
        err[2] = NULL;
        err[3] = NULL;
        mln_rs_result_t *d = mln_rs_decode(err, len, n, k);
        assert(d != NULL);
        for (i = 0; i < n; ++i)
            verify_block(d, i, data + i * len, len);
        mln_rs_result_free(d);
    }

    /* Pattern 3: erase 1 data + 1 parity */
    {
        uint8_t *err[6];
        for (i = 0; i < 6; ++i) err[i] = mln_rs_result_get_data_by_index(res, i);
        err[1] = NULL;
        err[4] = NULL;
        mln_rs_result_t *d = mln_rs_decode(err, len, n, k);
        assert(d != NULL);
        for (i = 0; i < n; ++i)
            verify_block(d, i, data + i * len, len);
        mln_rs_result_free(d);
    }

    /* Pattern 4: erase alternating blocks */
    {
        uint8_t *err[6];
        for (i = 0; i < 6; ++i) err[i] = mln_rs_result_get_data_by_index(res, i);
        err[0] = NULL;
        err[2] = NULL;
        mln_rs_result_t *d = mln_rs_decode(err, len, n, k);
        assert(d != NULL);
        for (i = 0; i < n; ++i)
            verify_block(d, i, data + i * len, len);
        mln_rs_result_free(d);
    }

    mln_rs_result_free(res);
    printf("PASS: test_erasure_patterns\n");
}

/*
 * Test maximum erasure recovery (erase exactly k blocks)
 */
static void test_max_erasures(void)
{
    int i;
    int n = 6, k = 3, len = 12;
    uint8_t data[72];
    for (i = 0; i < 72; ++i) data[i] = (uint8_t)(i * 7 + 3);

    mln_rs_result_t *res = mln_rs_encode(data, len, n, k);
    assert(res != NULL);

    /* Erase exactly k=3 blocks: blocks 0, 3, 5 */
    uint8_t *err[9];
    for (i = 0; i < 9; ++i) err[i] = mln_rs_result_get_data_by_index(res, i);
    err[0] = NULL;
    err[3] = NULL;
    err[5] = NULL;

    mln_rs_result_t *d = mln_rs_decode(err, len, n, k);
    assert(d != NULL);
    for (i = 0; i < n; ++i)
        verify_block(d, i, data + i * len, len);

    mln_rs_result_free(d);
    mln_rs_result_free(res);
    printf("PASS: test_max_erasures\n");
}

/*
 * Test single parity (k=1)
 */
static void test_single_parity(void)
{
    int i;
    int n = 5, k = 1, len = 10;
    uint8_t data[50];
    for (i = 0; i < 50; ++i) data[i] = (uint8_t)(i);

    mln_rs_result_t *res = mln_rs_encode(data, len, n, k);
    assert(res != NULL);
    assert(mln_rs_result_get_num(res) == (mln_size_t)(n + k));

    /* Erase 1 block */
    uint8_t *err[6];
    for (i = 0; i < 6; ++i) err[i] = mln_rs_result_get_data_by_index(res, i);
    err[2] = NULL;

    mln_rs_result_t *d = mln_rs_decode(err, len, n, k);
    assert(d != NULL);
    for (i = 0; i < n; ++i)
        verify_block(d, i, data + i * len, len);

    mln_rs_result_free(d);
    mln_rs_result_free(res);
    printf("PASS: test_single_parity\n");
}

/*
 * Test no erasure (all data blocks present)
 */
static void test_no_erasure(void)
{
    int i;
    int n = 4, k = 2, len = 8;
    uint8_t data[32];
    for (i = 0; i < 32; ++i) data[i] = (uint8_t)(i + 100);

    mln_rs_result_t *res = mln_rs_encode(data, len, n, k);
    assert(res != NULL);

    /* All data blocks present, no erasure */
    uint8_t *err[6];
    for (i = 0; i < 6; ++i) err[i] = mln_rs_result_get_data_by_index(res, i);

    mln_rs_result_t *d = mln_rs_decode(err, len, n, k);
    assert(d != NULL);
    for (i = 0; i < n; ++i)
        verify_block(d, i, data + i * len, len);

    mln_rs_result_free(d);
    mln_rs_result_free(res);
    printf("PASS: test_no_erasure\n");
}

/*
 * Test k=0 decode path (no parity, just copy)
 */
static void test_k_zero(void)
{
    int i;
    int n = 3, len = 6;
    uint8_t data[18];
    for (i = 0; i < 18; ++i) data[i] = (uint8_t)(i);

    uint8_t *ptrs[3];
    ptrs[0] = data;
    ptrs[1] = data + 6;
    ptrs[2] = data + 12;

    mln_rs_result_t *d = mln_rs_decode(ptrs, len, n, 0);
    assert(d != NULL);
    assert(mln_rs_result_get_num(d) == 3);
    for (i = 0; i < n; ++i)
        verify_block(d, i, data + i * len, len);

    mln_rs_result_free(d);
    printf("PASS: test_k_zero\n");
}

/*
 * Test invalid inputs
 */
static void test_invalid_input(void)
{
    mln_rs_result_t *res;

    /* NULL data */
    res = mln_rs_encode(NULL, 10, 4, 2);
    assert(res == NULL);

    /* Zero length */
    uint8_t dummy[4] = {0};
    res = mln_rs_encode(dummy, 0, 1, 1);
    assert(res == NULL);

    /* Zero n */
    res = mln_rs_encode(dummy, 4, 0, 1);
    assert(res == NULL);

    /* Zero k */
    res = mln_rs_encode(dummy, 4, 1, 0);
    assert(res == NULL);

    /* Decode with n=0 and k=0 */
    uint8_t *ptrs[1] = {dummy};
    res = mln_rs_decode(ptrs, 4, 0, 0);
    assert(res == NULL);

    /* result macros with NULL */
    mln_rs_result_t *null_result = NULL;
    assert(mln_rs_result_get_num(null_result) == 0);
    assert(mln_rs_result_get_data_by_index(null_result, 0) == NULL);

    printf("PASS: test_invalid_input\n");
}

/*
 * Test with larger data sizes
 */
static void test_large_data(void)
{
    int i;
    int n = 10, k = 4, len = 256;
    uint8_t *data = (uint8_t *)malloc(n * len);
    assert(data != NULL);

    for (i = 0; i < n * len; ++i) data[i] = (uint8_t)(i & 0xff);

    mln_rs_result_t *res = mln_rs_encode(data, len, n, k);
    assert(res != NULL);

    uint8_t **err = (uint8_t **)calloc(n + k, sizeof(uint8_t *));
    assert(err != NULL);
    for (i = 0; i < n + k; ++i) err[i] = mln_rs_result_get_data_by_index(res, i);

    /* Erase k blocks spread across data and parity */
    err[1] = NULL;
    err[4] = NULL;
    err[7] = NULL;
    err[10] = NULL;

    mln_rs_result_t *d = mln_rs_decode(err, len, n, k);
    assert(d != NULL);
    for (i = 0; i < n; ++i)
        verify_block(d, i, data + i * len, len);

    mln_rs_result_free(d);
    mln_rs_result_free(res);
    free(err);
    free(data);
    printf("PASS: test_large_data\n");
}

/*
 * Test with original test data (n=10, k=2, len=10)
 */
static void test_original_scenario(void)
{
    int i, j;
    mln_rs_result_t *res, *dres;
    uint8_t origin[100] = "0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789";
    uint8_t *err[12] = {0};

    res = mln_rs_encode(origin, 10, 10, 2);
    assert(res != NULL);
    assert(mln_rs_result_get_num(res) == 12);

    for (i = 0; i < 12; ++i) err[i] = mln_rs_result_get_data_by_index(res, i);
    err[0] = NULL;
    err[1] = NULL;

    dres = mln_rs_decode(err, 10, 10, 2);
    assert(dres != NULL);
    assert(mln_rs_result_get_num(dres) == 10);

    for (i = 0; i < 10; ++i) {
        uint8_t *p = mln_rs_result_get_data_by_index(dres, i);
        for (j = 0; j < 10; ++j)
            assert(p[j] == origin[i * 10 + j]);
    }

    mln_rs_result_free(res);
    mln_rs_result_free(dres);
    printf("PASS: test_original_scenario\n");
}

/*
 * Test mln_rs_result macros
 */
static void test_result_macros(void)
{
    uint8_t data[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    mln_rs_result_t *res = mln_rs_encode(data, 4, 2, 1);
    assert(res != NULL);

    mln_size_t num = mln_rs_result_get_num(res);
    assert(num == 3);

    /* Valid index access */
    assert(mln_rs_result_get_data_by_index(res, 0) != NULL);
    assert(mln_rs_result_get_data_by_index(res, 1) != NULL);
    assert(mln_rs_result_get_data_by_index(res, 2) != NULL);

    /* Out-of-bounds index */
    assert(mln_rs_result_get_data_by_index(res, 3) == NULL);
    assert(mln_rs_result_get_data_by_index(res, 100) == NULL);

    mln_rs_result_free(res);
    printf("PASS: test_result_macros\n");
}

/*
 * Performance benchmark
 */
static void test_performance(void)
{
    int i;
    struct timespec start, end;
    double elapsed;
    int iters = 50000;
    int n = 10, k = 4, len = 100;

    uint8_t *data = (uint8_t *)malloc(n * len);
    assert(data != NULL);
    for (i = 0; i < n * len; ++i) data[i] = (uint8_t)(i & 0xff);

    /* Benchmark encode */
    clock_gettime(CLOCK_MONOTONIC, &start);
    for (i = 0; i < iters; ++i) {
        mln_rs_result_t *res = mln_rs_encode(data, len, n, k);
        assert(res != NULL);
        mln_rs_result_free(res);
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    printf("  Encode: %d iters in %.3f s (%.0f iters/s, %.2f MB/s)\n",
           iters, elapsed, (double)iters / elapsed,
           (double)iters * n * len / elapsed / 1e6);

    /* Benchmark decode */
    mln_rs_result_t *res = mln_rs_encode(data, len, n, k);
    assert(res != NULL);
    uint8_t **err = (uint8_t **)calloc(n + k, sizeof(uint8_t *));
    assert(err != NULL);
    for (i = 0; i < n + k; ++i) err[i] = mln_rs_result_get_data_by_index(res, i);
    for (i = 0; i < k; ++i) err[i] = NULL;

    clock_gettime(CLOCK_MONOTONIC, &start);
    for (i = 0; i < iters; ++i) {
        mln_rs_result_t *dres = mln_rs_decode(err, len, n, k);
        assert(dres != NULL);
        mln_rs_result_free(dres);
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    printf("  Decode: %d iters in %.3f s (%.0f iters/s, %.2f MB/s)\n",
           iters, elapsed, (double)iters / elapsed,
           (double)iters * n * len / elapsed / 1e6);

    mln_rs_result_free(res);
    free(err);
    free(data);
    printf("PASS: test_performance\n");
}

/*
 * Stability test: repeated encode/decode with various patterns
 */
static void test_stability(void)
{
    int round, i;
    int n = 8, k = 3, len = 64;
    int total = n + k;

    uint8_t *data = (uint8_t *)malloc(n * len);
    assert(data != NULL);

    for (round = 0; round < 1000; ++round) {
        /* Fill with round-dependent pattern */
        for (i = 0; i < n * len; ++i)
            data[i] = (uint8_t)((i + round * 31) & 0xff);

        mln_rs_result_t *res = mln_rs_encode(data, len, n, k);
        assert(res != NULL);

        uint8_t **err = (uint8_t **)calloc(total, sizeof(uint8_t *));
        assert(err != NULL);
        for (i = 0; i < total; ++i)
            err[i] = mln_rs_result_get_data_by_index(res, i);

        /* Erase k blocks with round-dependent pattern */
        for (i = 0; i < k; ++i)
            err[(round * 3 + i * 2) % total] = NULL;

        mln_rs_result_t *dres = mln_rs_decode(err, len, n, k);
        assert(dres != NULL);

        for (i = 0; i < n; ++i)
            verify_block(dres, i, data + i * len, len);

        mln_rs_result_free(dres);
        mln_rs_result_free(res);
        free(err);
    }

    free(data);
    printf("PASS: test_stability (1000 rounds)\n");
}

int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    test_basic_encode_decode();
    test_different_parameters();
    test_erasure_patterns();
    test_max_erasures();
    test_single_parity();
    test_no_erasure();
    test_k_zero();
    test_invalid_input();
    test_large_data();
    test_original_scenario();
    test_result_macros();
    test_performance();
    test_stability();
    printf("\nAll RS tests passed!\n");
    return 0;
}
