#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include "mln_sha.h"

static int test_nr = 0;

#define PASS() do { printf("  #%02d PASS\n", ++test_nr); } while (0)

static void test_sha1_basic(void)
{
    mln_sha1_t s;
    char text[128] = {0};

    mln_sha1_init(&s);
    mln_sha1_calc(&s, (mln_u8ptr_t)"Hello", 5, 1);
    mln_sha1_tostring(&s, text, sizeof(text) - 1);
    assert(strcmp(text, "f7ff9e8b7bb2e09b70935a5d785e0cc5d9d0abf0") == 0);
    PASS();
}

static void test_sha1_empty(void)
{
    mln_sha1_t s;
    char text[128] = {0};

    mln_sha1_init(&s);
    mln_sha1_calc(&s, (mln_u8ptr_t)"", 0, 1);
    mln_sha1_tostring(&s, text, sizeof(text) - 1);
    assert(strcmp(text, "da39a3ee5e6b4b0d3255bfef95601890afd80709") == 0);
    PASS();
}

static void test_sha1_tobytes(void)
{
    mln_sha1_t s;
    mln_u8_t bytes[20];

    mln_sha1_init(&s);
    mln_sha1_calc(&s, (mln_u8ptr_t)"Hello", 5, 1);
    mln_sha1_tobytes(&s, bytes, sizeof(bytes));

    assert(bytes[0] == 0xf7);
    assert(bytes[1] == 0xff);
    assert(bytes[19] == 0xf0);
    PASS();
}

static void test_sha1_batch(void)
{
    mln_sha1_t s1, s2;
    char text1[128] = {0}, text2[128] = {0};

    mln_sha1_init(&s1);
    mln_sha1_calc(&s1, (mln_u8ptr_t)"Hello, World!", 13, 1);
    mln_sha1_tostring(&s1, text1, sizeof(text1) - 1);

    mln_sha1_init(&s2);
    mln_sha1_calc(&s2, (mln_u8ptr_t)"Hello, ", 7, 0);
    mln_sha1_calc(&s2, (mln_u8ptr_t)"World!", 6, 1);
    mln_sha1_tostring(&s2, text2, sizeof(text2) - 1);

    assert(strcmp(text1, text2) == 0);
    PASS();
}

static void test_sha1_long_message(void)
{
    mln_sha1_t s;
    char text[128] = {0};
    unsigned char data[1024];
    memset(data, 'a', sizeof(data));

    mln_sha1_init(&s);
    mln_sha1_calc(&s, data, sizeof(data), 1);
    mln_sha1_tostring(&s, text, sizeof(text) - 1);
    assert(strlen(text) == 40);
    PASS();
}

static void test_sha1_exact_block(void)
{
    mln_sha1_t s;
    char text[128] = {0};
    unsigned char data[64];
    memset(data, 'B', sizeof(data));

    mln_sha1_init(&s);
    mln_sha1_calc(&s, data, sizeof(data), 1);
    mln_sha1_tostring(&s, text, sizeof(text) - 1);
    assert(strlen(text) == 40);
    PASS();
}

static void test_sha1_new_free(void)
{
    mln_sha1_t *s = mln_sha1_new();
    assert(s != NULL);
    mln_sha1_calc(s, (mln_u8ptr_t)"test", 4, 1);
    char text[128] = {0};
    mln_sha1_tostring(s, text, sizeof(text) - 1);
    assert(strlen(text) == 40);
    mln_sha1_free(s);
    mln_sha1_free(NULL);
    PASS();
}

static void test_sha1_pool_new_free(void)
{
    mln_alloc_t *pool = mln_alloc_init(NULL, 0);
    assert(pool != NULL);
    mln_sha1_t *s = mln_sha1_pool_new(pool);
    assert(s != NULL);
    mln_sha1_calc(s, (mln_u8ptr_t)"test", 4, 1);
    mln_sha1_pool_free(s);
    mln_sha1_pool_free(NULL);
    mln_alloc_destroy(pool);
    PASS();
}

static void test_sha1_multi_batch(void)
{
    mln_sha1_t s1, s2;
    char text1[128] = {0}, text2[128] = {0};
    unsigned char data[256];
    memset(data, 0xCD, sizeof(data));

    mln_sha1_init(&s1);
    mln_sha1_calc(&s1, data, sizeof(data), 1);
    mln_sha1_tostring(&s1, text1, sizeof(text1) - 1);

    mln_sha1_init(&s2);
    mln_sha1_calc(&s2, data, 50, 0);
    mln_sha1_calc(&s2, data + 50, 50, 0);
    mln_sha1_calc(&s2, data + 100, 156, 1);
    mln_sha1_tostring(&s2, text2, sizeof(text2) - 1);

    assert(strcmp(text1, text2) == 0);
    PASS();
}

static void test_sha1_tostring_short_buf(void)
{
    mln_sha1_t s;
    char text[11] = {0};

    mln_sha1_init(&s);
    mln_sha1_calc(&s, (mln_u8ptr_t)"Hello", 5, 1);
    mln_sha1_tostring(&s, text, sizeof(text) - 1);
    assert(strlen(text) == 8);
    PASS();
}

static void test_sha256_basic(void)
{
    mln_sha256_t s;
    char text[128] = {0};

    mln_sha256_init(&s);
    mln_sha256_calc(&s, (mln_u8ptr_t)"Hello", 5, 1);
    mln_sha256_tostring(&s, text, sizeof(text) - 1);
    assert(strcmp(text, "185f8db32271fe25f561a6fc938b2e264306ec304eda518007d1764826381969") == 0);
    PASS();
}

static void test_sha256_empty(void)
{
    mln_sha256_t s;
    char text[128] = {0};

    mln_sha256_init(&s);
    mln_sha256_calc(&s, (mln_u8ptr_t)"", 0, 1);
    mln_sha256_tostring(&s, text, sizeof(text) - 1);
    assert(strcmp(text, "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855") == 0);
    PASS();
}

static void test_sha256_tobytes(void)
{
    mln_sha256_t s;
    mln_u8_t bytes[32];

    mln_sha256_init(&s);
    mln_sha256_calc(&s, (mln_u8ptr_t)"Hello", 5, 1);
    mln_sha256_tobytes(&s, bytes, sizeof(bytes));

    assert(bytes[0] == 0x18);
    assert(bytes[1] == 0x5f);
    assert(bytes[31] == 0x69);
    PASS();
}

static void test_sha256_batch(void)
{
    mln_sha256_t s1, s2;
    char text1[128] = {0}, text2[128] = {0};

    mln_sha256_init(&s1);
    mln_sha256_calc(&s1, (mln_u8ptr_t)"Hello, World!", 13, 1);
    mln_sha256_tostring(&s1, text1, sizeof(text1) - 1);

    mln_sha256_init(&s2);
    mln_sha256_calc(&s2, (mln_u8ptr_t)"Hello, ", 7, 0);
    mln_sha256_calc(&s2, (mln_u8ptr_t)"World!", 6, 1);
    mln_sha256_tostring(&s2, text2, sizeof(text2) - 1);

    assert(strcmp(text1, text2) == 0);
    PASS();
}

static void test_sha256_long_message(void)
{
    mln_sha256_t s;
    char text[128] = {0};
    unsigned char data[1024];
    memset(data, 'a', sizeof(data));

    mln_sha256_init(&s);
    mln_sha256_calc(&s, data, sizeof(data), 1);
    mln_sha256_tostring(&s, text, sizeof(text) - 1);
    assert(strlen(text) == 64);
    PASS();
}

static void test_sha256_exact_block(void)
{
    mln_sha256_t s;
    char text[128] = {0};
    unsigned char data[64];
    memset(data, 'B', sizeof(data));

    mln_sha256_init(&s);
    mln_sha256_calc(&s, data, sizeof(data), 1);
    mln_sha256_tostring(&s, text, sizeof(text) - 1);
    assert(strlen(text) == 64);
    PASS();
}

static void test_sha256_new_free(void)
{
    mln_sha256_t *s = mln_sha256_new();
    assert(s != NULL);
    mln_sha256_calc(s, (mln_u8ptr_t)"test", 4, 1);
    char text[128] = {0};
    mln_sha256_tostring(s, text, sizeof(text) - 1);
    assert(strlen(text) == 64);
    mln_sha256_free(s);
    mln_sha256_free(NULL);
    PASS();
}

static void test_sha256_pool_new_free(void)
{
    mln_alloc_t *pool = mln_alloc_init(NULL, 0);
    assert(pool != NULL);
    mln_sha256_t *s = mln_sha256_pool_new(pool);
    assert(s != NULL);
    mln_sha256_calc(s, (mln_u8ptr_t)"test", 4, 1);
    mln_sha256_pool_free(s);
    mln_sha256_pool_free(NULL);
    mln_alloc_destroy(pool);
    PASS();
}

static void test_sha256_multi_batch(void)
{
    mln_sha256_t s1, s2;
    char text1[128] = {0}, text2[128] = {0};
    unsigned char data[256];
    memset(data, 0xCD, sizeof(data));

    mln_sha256_init(&s1);
    mln_sha256_calc(&s1, data, sizeof(data), 1);
    mln_sha256_tostring(&s1, text1, sizeof(text1) - 1);

    mln_sha256_init(&s2);
    mln_sha256_calc(&s2, data, 50, 0);
    mln_sha256_calc(&s2, data + 50, 50, 0);
    mln_sha256_calc(&s2, data + 100, 156, 1);
    mln_sha256_tostring(&s2, text2, sizeof(text2) - 1);

    assert(strcmp(text1, text2) == 0);
    PASS();
}

static void test_sha256_tostring_short_buf(void)
{
    mln_sha256_t s;
    char text[11] = {0};

    mln_sha256_init(&s);
    mln_sha256_calc(&s, (mln_u8ptr_t)"Hello", 5, 1);
    mln_sha256_tostring(&s, text, sizeof(text) - 1);
    assert(strlen(text) == 8);
    PASS();
}

static void test_sha256_tobytes_short_buf(void)
{
    mln_sha256_t s;
    mln_u8_t bytes[4] = {0};

    mln_sha256_init(&s);
    mln_sha256_calc(&s, (mln_u8ptr_t)"Hello", 5, 1);
    mln_sha256_tobytes(&s, bytes, sizeof(bytes));

    assert(bytes[0] == 0x18);
    assert(bytes[1] == 0x5f);
    assert(bytes[2] == 0x8d);
    assert(bytes[3] == 0xb3);
    PASS();
}

static void test_sha1_tobytes_null_buf(void)
{
    mln_sha1_t s;
    mln_sha1_init(&s);
    mln_sha1_calc(&s, (mln_u8ptr_t)"Hello", 5, 1);
    mln_sha1_tobytes(&s, NULL, 20);
    mln_sha1_tobytes(&s, (mln_u8ptr_t)"x", 0);
    PASS();
}

static void test_sha256_tobytes_null_buf(void)
{
    mln_sha256_t s;
    mln_sha256_init(&s);
    mln_sha256_calc(&s, (mln_u8ptr_t)"Hello", 5, 1);
    mln_sha256_tobytes(&s, NULL, 32);
    mln_sha256_tobytes(&s, (mln_u8ptr_t)"x", 0);
    PASS();
}

static void test_sha1_tostring_null_buf(void)
{
    mln_sha1_t s;
    mln_sha1_init(&s);
    mln_sha1_calc(&s, (mln_u8ptr_t)"Hello", 5, 1);
    mln_sha1_tostring(&s, NULL, 40);
    char buf[2] = {0};
    mln_sha1_tostring(&s, buf, 0);
    PASS();
}

static void test_sha256_tostring_null_buf(void)
{
    mln_sha256_t s;
    mln_sha256_init(&s);
    mln_sha256_calc(&s, (mln_u8ptr_t)"Hello", 5, 1);
    mln_sha256_tostring(&s, NULL, 64);
    char buf[2] = {0};
    mln_sha256_tostring(&s, buf, 0);
    PASS();
}

static void test_sha1_55bytes(void)
{
    mln_sha1_t s;
    char text[128] = {0};
    unsigned char data[55];
    memset(data, 'X', sizeof(data));

    mln_sha1_init(&s);
    mln_sha1_calc(&s, data, sizeof(data), 1);
    mln_sha1_tostring(&s, text, sizeof(text) - 1);
    assert(strlen(text) == 40);
    PASS();
}

static void test_sha256_55bytes(void)
{
    mln_sha256_t s;
    char text[128] = {0};
    unsigned char data[55];
    memset(data, 'X', sizeof(data));

    mln_sha256_init(&s);
    mln_sha256_calc(&s, data, sizeof(data), 1);
    mln_sha256_tostring(&s, text, sizeof(text) - 1);
    assert(strlen(text) == 64);
    PASS();
}

static void test_sha1_56bytes(void)
{
    mln_sha1_t s;
    char text[128] = {0};
    unsigned char data[56];
    memset(data, 'Y', sizeof(data));

    mln_sha1_init(&s);
    mln_sha1_calc(&s, data, sizeof(data), 1);
    mln_sha1_tostring(&s, text, sizeof(text) - 1);
    assert(strlen(text) == 40);
    PASS();
}

static void test_sha256_56bytes(void)
{
    mln_sha256_t s;
    char text[128] = {0};
    unsigned char data[56];
    memset(data, 'Y', sizeof(data));

    mln_sha256_init(&s);
    mln_sha256_calc(&s, data, sizeof(data), 1);
    mln_sha256_tostring(&s, text, sizeof(text) - 1);
    assert(strlen(text) == 64);
    PASS();
}

static void test_performance(void)
{
    int i;
    struct timespec t0, t1;
    double sha1_sec, sha256_sec;
    mln_sha1_t s1;
    mln_sha256_t s256;
    unsigned char data[1024];
    memset(data, 0xAB, sizeof(data));
    int iters = 10000;

    clock_gettime(CLOCK_MONOTONIC, &t0);
    for (i = 0; i < iters; ++i) {
        mln_sha1_init(&s1);
        mln_sha1_calc(&s1, data, sizeof(data), 1);
    }
    clock_gettime(CLOCK_MONOTONIC, &t1);
    sha1_sec = (t1.tv_sec - t0.tv_sec) + (t1.tv_nsec - t0.tv_nsec) / 1e9;

    clock_gettime(CLOCK_MONOTONIC, &t0);
    for (i = 0; i < iters; ++i) {
        mln_sha256_init(&s256);
        mln_sha256_calc(&s256, data, sizeof(data), 1);
    }
    clock_gettime(CLOCK_MONOTONIC, &t1);
    sha256_sec = (t1.tv_sec - t0.tv_sec) + (t1.tv_nsec - t0.tv_nsec) / 1e9;

    printf("  SHA1   %d iters: %.3f sec (%.1f MB/s)\n", iters, sha1_sec, (double)iters * 1024.0 / sha1_sec / 1e6);
    printf("  SHA256 %d iters: %.3f sec (%.1f MB/s)\n", iters, sha256_sec, (double)iters * 1024.0 / sha256_sec / 1e6);

    assert(sha1_sec < 30.0);
    assert(sha256_sec < 30.0);
    PASS();
}

static void test_stability(void)
{
    int i;
    mln_sha1_t s1;
    mln_sha256_t s256;
    char text1[128], text2[128];
    unsigned char data[512];
    memset(data, 0xEF, sizeof(data));

    mln_sha1_init(&s1);
    mln_sha1_calc(&s1, data, sizeof(data), 1);
    mln_sha1_tostring(&s1, text1, sizeof(text1) - 1);

    for (i = 0; i < 10000; ++i) {
        mln_sha1_init(&s1);
        mln_sha1_calc(&s1, data, sizeof(data), 1);
        mln_sha1_tostring(&s1, text2, sizeof(text2) - 1);
        assert(strcmp(text1, text2) == 0);
    }

    mln_sha256_init(&s256);
    mln_sha256_calc(&s256, data, sizeof(data), 1);
    mln_sha256_tostring(&s256, text1, sizeof(text1) - 1);

    for (i = 0; i < 10000; ++i) {
        mln_sha256_init(&s256);
        mln_sha256_calc(&s256, data, sizeof(data), 1);
        mln_sha256_tostring(&s256, text2, sizeof(text2) - 1);
        assert(strcmp(text1, text2) == 0);
    }
    PASS();
}

static void test_sha1_single_byte(void)
{
    mln_sha1_t s;
    char text[128] = {0};

    mln_sha1_init(&s);
    mln_sha1_calc(&s, (mln_u8ptr_t)"a", 1, 1);
    mln_sha1_tostring(&s, text, sizeof(text) - 1);
    assert(strcmp(text, "86f7e437faa5a7fce15d1ddcb9eaeaea377667b8") == 0);
    PASS();
}

static void test_sha256_single_byte(void)
{
    mln_sha256_t s;
    char text[128] = {0};

    mln_sha256_init(&s);
    mln_sha256_calc(&s, (mln_u8ptr_t)"a", 1, 1);
    mln_sha256_tostring(&s, text, sizeof(text) - 1);
    assert(strcmp(text, "ca978112ca1bbdcafac231b39a23dc4da786eff8147c4e72b9807785afee48bb") == 0);
    PASS();
}

static void test_sha1_binary_data(void)
{
    mln_sha1_t s1, s2;
    char text1[128] = {0}, text2[128] = {0};
    unsigned char data[256];
    int i;
    for (i = 0; i < 256; ++i) data[i] = (unsigned char)i;

    mln_sha1_init(&s1);
    mln_sha1_calc(&s1, data, sizeof(data), 1);
    mln_sha1_tostring(&s1, text1, sizeof(text1) - 1);

    mln_sha1_init(&s2);
    mln_sha1_calc(&s2, data, sizeof(data), 1);
    mln_sha1_tostring(&s2, text2, sizeof(text2) - 1);

    assert(strcmp(text1, text2) == 0);
    PASS();
}

static void test_sha256_binary_data(void)
{
    mln_sha256_t s1, s2;
    char text1[128] = {0}, text2[128] = {0};
    unsigned char data[256];
    int i;
    for (i = 0; i < 256; ++i) data[i] = (unsigned char)i;

    mln_sha256_init(&s1);
    mln_sha256_calc(&s1, data, sizeof(data), 1);
    mln_sha256_tostring(&s1, text1, sizeof(text1) - 1);

    mln_sha256_init(&s2);
    mln_sha256_calc(&s2, data, sizeof(data), 1);
    mln_sha256_tostring(&s2, text2, sizeof(text2) - 1);

    assert(strcmp(text1, text2) == 0);
    PASS();
}

int main(int argc, char *argv[])
{
    (void)argc; (void)argv;
    printf("SHA test suite\n");

    test_sha1_basic();
    test_sha1_empty();
    test_sha1_tobytes();
    test_sha1_batch();
    test_sha1_long_message();
    test_sha1_exact_block();
    test_sha1_new_free();
    test_sha1_pool_new_free();
    test_sha1_multi_batch();
    test_sha1_tostring_short_buf();
    test_sha1_tobytes_null_buf();
    test_sha1_tostring_null_buf();
    test_sha1_55bytes();
    test_sha1_56bytes();
    test_sha1_single_byte();
    test_sha1_binary_data();

    test_sha256_basic();
    test_sha256_empty();
    test_sha256_tobytes();
    test_sha256_batch();
    test_sha256_long_message();
    test_sha256_exact_block();
    test_sha256_new_free();
    test_sha256_pool_new_free();
    test_sha256_multi_batch();
    test_sha256_tostring_short_buf();
    test_sha256_tobytes_short_buf();
    test_sha256_tobytes_null_buf();
    test_sha256_tostring_null_buf();
    test_sha256_55bytes();
    test_sha256_56bytes();
    test_sha256_single_byte();
    test_sha256_binary_data();

    test_performance();
    test_stability();

    printf("All %d tests passed.\n", test_nr);
    return 0;
}
