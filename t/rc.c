#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include "mln_rc.h"

static int test_nr = 0;

#define PASS() do { printf("  #%02d PASS\n", ++test_nr); } while (0)

static void test_basic_encrypt_decrypt(void)
{
    mln_u8_t s[256];
    mln_u8_t text[] = "Hello";
    mln_u8_t orig[sizeof(text)];
    memcpy(orig, text, sizeof(text));

    mln_rc4_init(s, (mln_u8ptr_t)"this is a key", sizeof("this is a key") - 1);

    mln_rc4_calc(s, text, sizeof(text) - 1);
    assert(memcmp(text, orig, sizeof(text) - 1) != 0);

    mln_rc4_calc(s, text, sizeof(text) - 1);
    assert(memcmp(text, orig, sizeof(text) - 1) == 0);
    PASS();
}

static void test_single_byte(void)
{
    mln_u8_t s[256];
    mln_u8_t data = 0x42;
    mln_u8_t orig = data;

    mln_rc4_init(s, (mln_u8ptr_t)"key", 3);
    mln_rc4_calc(s, &data, 1);
    assert(data != orig);
    mln_rc4_calc(s, &data, 1);
    assert(data == orig);
    PASS();
}

static void test_empty_data(void)
{
    mln_u8_t s[256];
    mln_u8_t data[] = "unchanged";
    mln_u8_t orig[sizeof(data)];
    memcpy(orig, data, sizeof(data));

    mln_rc4_init(s, (mln_u8ptr_t)"key", 3);
    mln_rc4_calc(s, data, 0);
    assert(memcmp(data, orig, sizeof(data)) == 0);
    PASS();
}

static void test_short_key(void)
{
    mln_u8_t s[256];
    mln_u8_t text[] = "test data for short key";
    mln_u8_t orig[sizeof(text)];
    memcpy(orig, text, sizeof(text));

    mln_rc4_init(s, (mln_u8ptr_t)"k", 1);
    mln_rc4_calc(s, text, sizeof(text) - 1);
    assert(memcmp(text, orig, sizeof(text) - 1) != 0);
    mln_rc4_calc(s, text, sizeof(text) - 1);
    assert(memcmp(text, orig, sizeof(text) - 1) == 0);
    PASS();
}

static void test_long_key(void)
{
    mln_u8_t s[256];
    mln_u8_t key[256];
    mln_u32_t i;
    mln_u8_t text[] = "test data for long key";
    mln_u8_t orig[sizeof(text)];

    for (i = 0; i < 256; ++i) key[i] = (mln_u8_t)i;
    memcpy(orig, text, sizeof(text));

    mln_rc4_init(s, key, 256);
    mln_rc4_calc(s, text, sizeof(text) - 1);
    assert(memcmp(text, orig, sizeof(text) - 1) != 0);
    mln_rc4_calc(s, text, sizeof(text) - 1);
    assert(memcmp(text, orig, sizeof(text) - 1) == 0);
    PASS();
}

static void test_different_keys_different_output(void)
{
    mln_u8_t s1[256], s2[256];
    mln_u8_t text1[] = "same plaintext";
    mln_u8_t text2[] = "same plaintext";

    mln_rc4_init(s1, (mln_u8ptr_t)"key_one", 7);
    mln_rc4_init(s2, (mln_u8ptr_t)"key_two", 7);

    mln_rc4_calc(s1, text1, sizeof(text1) - 1);
    mln_rc4_calc(s2, text2, sizeof(text2) - 1);

    assert(memcmp(text1, text2, sizeof(text1) - 1) != 0);
    PASS();
}

static void test_same_key_same_keystream(void)
{
    mln_u8_t s1[256], s2[256];
    mln_u8_t text1[] = "same plaintext here";
    mln_u8_t text2[] = "same plaintext here";

    mln_rc4_init(s1, (mln_u8ptr_t)"shared_key", 10);
    mln_rc4_init(s2, (mln_u8ptr_t)"shared_key", 10);

    mln_rc4_calc(s1, text1, sizeof(text1) - 1);
    mln_rc4_calc(s2, text2, sizeof(text2) - 1);

    assert(memcmp(text1, text2, sizeof(text1) - 1) == 0);
    PASS();
}

static void test_state_preserved(void)
{
    mln_u8_t s[256], s_backup[256];

    mln_rc4_init(s, (mln_u8ptr_t)"test_key", 8);
    memcpy(s_backup, s, 256);

    mln_u8_t data[] = "some data";
    mln_rc4_calc(s, data, sizeof(data) - 1);

    assert(memcmp(s, s_backup, 256) == 0);
    PASS();
}

static void test_binary_data(void)
{
    mln_u8_t s[256];
    mln_u8_t data[256], orig[256];
    mln_u32_t i;

    for (i = 0; i < 256; ++i) data[i] = (mln_u8_t)i;
    memcpy(orig, data, 256);

    mln_rc4_init(s, (mln_u8ptr_t)"binary_key", 10);
    mln_rc4_calc(s, data, 256);
    assert(memcmp(data, orig, 256) != 0);
    mln_rc4_calc(s, data, 256);
    assert(memcmp(data, orig, 256) == 0);
    PASS();
}

static void test_large_data(void)
{
    mln_u8_t s[256];
    mln_uauto_t sz = 8192;
    mln_u8ptr_t data = (mln_u8ptr_t)malloc(sz);
    mln_u8ptr_t orig = (mln_u8ptr_t)malloc(sz);
    mln_uauto_t i;

    assert(data != NULL && orig != NULL);
    for (i = 0; i < sz; ++i) data[i] = (mln_u8_t)(i & 0xFF);
    memcpy(orig, data, sz);

    mln_rc4_init(s, (mln_u8ptr_t)"large_data_key", 14);
    mln_rc4_calc(s, data, sz);
    assert(memcmp(data, orig, sz) != 0);
    mln_rc4_calc(s, data, sz);
    assert(memcmp(data, orig, sz) == 0);

    free(data);
    free(orig);
    PASS();
}

static void test_non_aligned_lengths(void)
{
    mln_u8_t s[256];
    mln_uauto_t lens[] = {1, 2, 3, 5, 7, 13, 17, 31, 63, 65, 127, 255, 257};
    mln_uauto_t i, j;

    for (i = 0; i < sizeof(lens) / sizeof(lens[0]); ++i) {
        mln_u8ptr_t data = (mln_u8ptr_t)malloc(lens[i]);
        mln_u8ptr_t orig = (mln_u8ptr_t)malloc(lens[i]);
        assert(data != NULL && orig != NULL);

        for (j = 0; j < lens[i]; ++j) data[j] = (mln_u8_t)(j * 37 + 13);
        memcpy(orig, data, lens[i]);

        mln_rc4_init(s, (mln_u8ptr_t)"align_test", 10);
        mln_rc4_calc(s, data, lens[i]);
        if (lens[i] > 0) assert(memcmp(data, orig, lens[i]) != 0);
        mln_rc4_calc(s, data, lens[i]);
        assert(memcmp(data, orig, lens[i]) == 0);

        free(data);
        free(orig);
    }
    PASS();
}

static void test_init_permutation(void)
{
    mln_u8_t s[256];
    mln_u32_t count[256] = {0};
    mln_u32_t i;

    mln_rc4_init(s, (mln_u8ptr_t)"perm_key", 8);

    for (i = 0; i < 256; ++i) count[s[i]]++;
    for (i = 0; i < 256; ++i) assert(count[i] == 1);
    PASS();
}

static void test_benchmark(void)
{
    mln_u8_t s[256];
    mln_u8_t key[] = "benchmark_key_for_rc4_perf";
    mln_u8_t data[1024];
    struct timespec t0, t1;
    int iters = 500000;
    int i;
    double init_ns, calc_ns;

    printf("  Performance benchmark:\n");

    clock_gettime(CLOCK_MONOTONIC, &t0);
    for (i = 0; i < iters; ++i) {
        memset(s, 0, sizeof(s));
        mln_rc4_init(s, key, sizeof(key) - 1);
    }
    clock_gettime(CLOCK_MONOTONIC, &t1);
    init_ns = (t1.tv_sec - t0.tv_sec) * 1e9 + (t1.tv_nsec - t0.tv_nsec);
    printf("    init:        %.1f ns/op (%d iters)\n", init_ns / iters, iters);

    memset(data, 0xAB, sizeof(data));
    memset(s, 0, sizeof(s));
    mln_rc4_init(s, key, sizeof(key) - 1);
    clock_gettime(CLOCK_MONOTONIC, &t0);
    for (i = 0; i < iters; ++i) {
        mln_rc4_calc(s, data, sizeof(data));
    }
    clock_gettime(CLOCK_MONOTONIC, &t1);
    calc_ns = (t1.tv_sec - t0.tv_sec) * 1e9 + (t1.tv_nsec - t0.tv_nsec);
    printf("    calc(1024B): %.1f ns/op (%d iters)\n", calc_ns / iters, iters);

    PASS();
}

static void test_stability(void)
{
    mln_u8_t s[256];
    mln_u8_t data[512], orig[512];
    int rounds = 10000;
    int i;
    mln_u32_t j;

    for (j = 0; j < 512; ++j) data[j] = (mln_u8_t)(j ^ 0x5A);
    memcpy(orig, data, 512);

    mln_rc4_init(s, (mln_u8ptr_t)"stability_key_test", 18);

    for (i = 0; i < rounds; ++i) {
        mln_rc4_calc(s, data, 512);
        mln_rc4_calc(s, data, 512);
    }
    assert(memcmp(data, orig, 512) == 0);
    PASS();
}

int main(int argc, char *argv[])
{
    (void)argc; (void)argv;

    printf("RC4 test suite:\n");

    test_basic_encrypt_decrypt();
    test_single_byte();
    test_empty_data();
    test_short_key();
    test_long_key();
    test_different_keys_different_output();
    test_same_key_same_keystream();
    test_state_preserved();
    test_binary_data();
    test_large_data();
    test_non_aligned_lengths();
    test_init_permutation();
    test_benchmark();
    test_stability();

    printf("All %d tests passed.\n", test_nr);
    return 0;
}

