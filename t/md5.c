#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include "mln_md5.h"

static int test_nr = 0;

#define PASS() do { printf("  #%02d PASS\n", ++test_nr); } while (0)

static void test_basic_hash(void)
{
    mln_md5_t m;
    char output[33] = {0};

    mln_md5_init(&m);
    mln_md5_calc(&m, (mln_u8ptr_t)"Hello", 5, 1);
    mln_md5_tostring(&m, output, sizeof(output));
    assert(strcmp(output, "8b1a9953c4611296a827abf8c47804d7") == 0);
    PASS();
}

static void test_empty_string(void)
{
    mln_md5_t m;
    char output[33] = {0};

    mln_md5_init(&m);
    mln_md5_calc(&m, (mln_u8ptr_t)"", 0, 1);
    mln_md5_tostring(&m, output, sizeof(output));
    assert(strcmp(output, "d41d8cd98f00b204e9800998ecf8427e") == 0);
    PASS();
}

static void test_single_char(void)
{
    mln_md5_t m;
    char output[33] = {0};

    mln_md5_init(&m);
    mln_md5_calc(&m, (mln_u8ptr_t)"a", 1, 1);
    mln_md5_tostring(&m, output, sizeof(output));
    assert(strcmp(output, "0cc175b9c0f1b6a831c399e269772661") == 0);
    PASS();
}

static void test_rfc1321_vectors(void)
{
    /* RFC 1321 test vectors */
    static const struct {
        const char *input;
        const char *expected;
    } vectors[] = {
        {"",        "d41d8cd98f00b204e9800998ecf8427e"},
        {"a",       "0cc175b9c0f1b6a831c399e269772661"},
        {"abc",     "900150983cd24fb0d6963f7d28e17f72"},
        {"message digest", "f96b697d7cb7938d525a2f31aaf161d0"},
        {"abcdefghijklmnopqrstuvwxyz", "c3fcd3d76192e4007dfb496cca67e13b"},
        {"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789",
         "d174ab98d277d9f5a5611c2c9f419d9f"},
        {"12345678901234567890123456789012345678901234567890123456789012345678901234567890",
         "57edf4a22be3c955ac49da2e2107b67a"},
    };
    int i;
    mln_md5_t m;
    char output[33] = {0};

    for (i = 0; i < (int)(sizeof(vectors)/sizeof(vectors[0])); ++i) {
        mln_md5_init(&m);
        mln_md5_calc(&m, (mln_u8ptr_t)vectors[i].input, strlen(vectors[i].input), 1);
        mln_md5_tostring(&m, output, sizeof(output));
        assert(strcmp(output, vectors[i].expected) == 0);
    }
    PASS();
}

static void test_streaming_calc(void)
{
    mln_md5_t m1, m2;
    char out1[33] = {0}, out2[33] = {0};
    const char *msg = "abcdefghijklmnopqrstuvwxyz";

    /* One-shot */
    mln_md5_init(&m1);
    mln_md5_calc(&m1, (mln_u8ptr_t)msg, strlen(msg), 1);
    mln_md5_tostring(&m1, out1, sizeof(out1));

    /* Streaming: split into 3 parts */
    mln_md5_init(&m2);
    mln_md5_calc(&m2, (mln_u8ptr_t)msg, 10, 0);
    mln_md5_calc(&m2, (mln_u8ptr_t)(msg + 10), 10, 0);
    mln_md5_calc(&m2, (mln_u8ptr_t)(msg + 20), strlen(msg) - 20, 1);
    mln_md5_tostring(&m2, out2, sizeof(out2));

    assert(strcmp(out1, out2) == 0);
    PASS();
}

static void test_streaming_byte_at_a_time(void)
{
    mln_md5_t m1, m2;
    char out1[33] = {0}, out2[33] = {0};
    const char *msg = "Hello, World!";
    mln_uauto_t i, len = strlen(msg);

    mln_md5_init(&m1);
    mln_md5_calc(&m1, (mln_u8ptr_t)msg, len, 1);
    mln_md5_tostring(&m1, out1, sizeof(out1));

    mln_md5_init(&m2);
    for (i = 0; i < len - 1; ++i)
        mln_md5_calc(&m2, (mln_u8ptr_t)(msg + i), 1, 0);
    mln_md5_calc(&m2, (mln_u8ptr_t)(msg + len - 1), 1, 1);
    mln_md5_tostring(&m2, out2, sizeof(out2));

    assert(strcmp(out1, out2) == 0);
    PASS();
}

static void test_exactly_64_bytes(void)
{
    mln_md5_t m;
    char output[33] = {0};
    mln_u8_t data[64];
    memset(data, 'A', sizeof(data));

    mln_md5_init(&m);
    mln_md5_calc(&m, data, sizeof(data), 1);
    mln_md5_tostring(&m, output, sizeof(output));
    /* 64 'A's */
    assert(strcmp(output, "d289a97565bc2d27ac8b8545a5ddba45") == 0);
    PASS();
}

static void test_55_bytes(void)
{
    /* 55 bytes is the boundary: message + 0x80 + 8-byte length = 64 (single block padding) */
    mln_md5_t m;
    char output[33] = {0};
    mln_u8_t data[55];
    memset(data, 'B', sizeof(data));

    mln_md5_init(&m);
    mln_md5_calc(&m, data, sizeof(data), 1);
    mln_md5_tostring(&m, output, sizeof(output));
    assert(strlen(output) == 32);
    assert(strcmp(output, "10e495e8105224cf5aa00fd86b61ed44") == 0);
    PASS();
}

static void test_56_bytes(void)
{
    /* 56 bytes: needs two blocks for padding (message + 0x80 doesn't fit with 8-byte length) */
    mln_md5_t m;
    char output[33] = {0};
    mln_u8_t data[56];
    memset(data, 'C', sizeof(data));

    mln_md5_init(&m);
    mln_md5_calc(&m, data, sizeof(data), 1);
    mln_md5_tostring(&m, output, sizeof(output));
    assert(strlen(output) == 32);
    assert(strcmp(output, "ddeabe78031243dc616e86065dfa8161") == 0);
    PASS();
}

static void test_tobytes(void)
{
    mln_md5_t m;
    mln_u8_t bytes[16] = {0};

    mln_md5_init(&m);
    mln_md5_calc(&m, (mln_u8ptr_t)"Hello", 5, 1);
    mln_md5_tobytes(&m, bytes, sizeof(bytes));

    /* MD5("Hello") = 8b1a9953c4611296a827abf8c47804d7 */
    assert(bytes[0] == 0x8b && bytes[1] == 0x1a && bytes[2] == 0x99 && bytes[3] == 0x53);
    assert(bytes[12] == 0xc4 && bytes[13] == 0x78 && bytes[14] == 0x04 && bytes[15] == 0xd7);
    PASS();
}

static void test_tobytes_partial(void)
{
    mln_md5_t m;
    mln_u8_t bytes[4] = {0};

    mln_md5_init(&m);
    mln_md5_calc(&m, (mln_u8ptr_t)"Hello", 5, 1);
    mln_md5_tobytes(&m, bytes, sizeof(bytes));

    assert(bytes[0] == 0x8b && bytes[1] == 0x1a && bytes[2] == 0x99 && bytes[3] == 0x53);
    PASS();
}

static void test_tobytes_null(void)
{
    mln_md5_t m;
    mln_md5_init(&m);
    mln_md5_calc(&m, (mln_u8ptr_t)"Hello", 5, 1);
    /* Should not crash */
    mln_md5_tobytes(&m, NULL, 0);
    mln_md5_tobytes(&m, NULL, 16);
    PASS();
}

static void test_tostring_short_buf(void)
{
    mln_md5_t m;
    char output[9] = {0};

    mln_md5_init(&m);
    mln_md5_calc(&m, (mln_u8ptr_t)"Hello", 5, 1);
    mln_md5_tostring(&m, output, sizeof(output));
    /* With 9-byte buffer: (9-1)/2 = 4 bytes = 8 hex chars + null */
    assert(strlen(output) == 8);
    assert(strncmp(output, "8b1a9953", 8) == 0);
    PASS();
}

static void test_tostring_null(void)
{
    mln_md5_t m;
    mln_md5_init(&m);
    mln_md5_calc(&m, (mln_u8ptr_t)"Hello", 5, 1);
    /* Should not crash */
    mln_md5_tostring(&m, NULL, 0);
    mln_md5_tostring(&m, NULL, 33);
    PASS();
}

static void test_new_free(void)
{
    mln_md5_t *m = mln_md5_new();
    assert(m != NULL);
    mln_md5_calc(m, (mln_u8ptr_t)"test", 4, 1);
    {
        char output[33] = {0};
        mln_md5_tostring(m, output, sizeof(output));
        assert(strcmp(output, "098f6bcd4621d373cade4e832627b4f6") == 0);
    }
    mln_md5_free(m);
    /* Free NULL should not crash */
    mln_md5_free(NULL);
    PASS();
}

static void test_large_data(void)
{
    mln_md5_t m1, m2;
    char out1[33] = {0}, out2[33] = {0};
    mln_u8_t *data;
    mln_uauto_t i, total = 4096;

    data = (mln_u8_t *)malloc(total);
    assert(data != NULL);
    for (i = 0; i < total; ++i)
        data[i] = (mln_u8_t)(i & 0xff);

    /* One-shot */
    mln_md5_init(&m1);
    mln_md5_calc(&m1, data, total, 1);
    mln_md5_tostring(&m1, out1, sizeof(out1));

    /* Streaming with 100-byte chunks */
    mln_md5_init(&m2);
    for (i = 0; i + 100 < total; i += 100)
        mln_md5_calc(&m2, data + i, 100, 0);
    mln_md5_calc(&m2, data + i, total - i, 1);
    mln_md5_tostring(&m2, out2, sizeof(out2));

    assert(strcmp(out1, out2) == 0);
    free(data);
    PASS();
}

static void test_binary_data(void)
{
    mln_md5_t m;
    char output[33] = {0};
    mln_u8_t data[256];
    int i;

    for (i = 0; i < 256; ++i)
        data[i] = (mln_u8_t)i;

    mln_md5_init(&m);
    mln_md5_calc(&m, data, sizeof(data), 1);
    mln_md5_tostring(&m, output, sizeof(output));
    /* MD5(0x00..0xff) = "e2c865db4162bed963bfaa9ef6ac18f0" */
    assert(strcmp(output, "e2c865db4162bed963bfaa9ef6ac18f0") == 0);
    PASS();
}

static void test_streaming_across_block_boundary(void)
{
    /* Feed data that straddles the 64-byte block boundary */
    mln_md5_t m1, m2;
    char out1[33] = {0}, out2[33] = {0};
    mln_u8_t data[128];
    memset(data, 0xAA, sizeof(data));

    mln_md5_init(&m1);
    mln_md5_calc(&m1, data, sizeof(data), 1);
    mln_md5_tostring(&m1, out1, sizeof(out1));

    /* Split at 63 bytes (inside first block) */
    mln_md5_init(&m2);
    mln_md5_calc(&m2, data, 63, 0);
    mln_md5_calc(&m2, data + 63, sizeof(data) - 63, 1);
    mln_md5_tostring(&m2, out2, sizeof(out2));

    assert(strcmp(out1, out2) == 0);
    PASS();
}

static void test_different_inputs_different_hashes(void)
{
    mln_md5_t m1, m2;
    char out1[33] = {0}, out2[33] = {0};

    mln_md5_init(&m1);
    mln_md5_calc(&m1, (mln_u8ptr_t)"hello", 5, 1);
    mln_md5_tostring(&m1, out1, sizeof(out1));

    mln_md5_init(&m2);
    mln_md5_calc(&m2, (mln_u8ptr_t)"world", 5, 1);
    mln_md5_tostring(&m2, out2, sizeof(out2));

    assert(strcmp(out1, out2) != 0);
    PASS();
}

static void test_benchmark(void)
{
    mln_md5_t m;
    mln_u8_t data[1024];
    int iters = 500000, i;
    struct timespec t0, t1;
    double ns;

    memset(data, 0xAB, sizeof(data));

    clock_gettime(CLOCK_MONOTONIC, &t0);
    for (i = 0; i < iters; ++i) {
        mln_md5_init(&m);
        mln_md5_calc(&m, data, sizeof(data), 1);
    }
    clock_gettime(CLOCK_MONOTONIC, &t1);
    ns = (t1.tv_sec - t0.tv_sec) * 1e9 + (t1.tv_nsec - t0.tv_nsec);
    printf("  Performance: %.1f ns/op for 1024B (%d iters)\n", ns / iters, iters);
    PASS();
}

static void test_stability(void)
{
    mln_md5_t m;
    char first[33] = {0}, current[33] = {0};
    mln_u8_t data[512];
    int i;

    memset(data, 0x55, sizeof(data));

    mln_md5_init(&m);
    mln_md5_calc(&m, data, sizeof(data), 1);
    mln_md5_tostring(&m, first, sizeof(first));

    for (i = 0; i < 10000; ++i) {
        mln_md5_init(&m);
        mln_md5_calc(&m, data, sizeof(data), 1);
        mln_md5_tostring(&m, current, sizeof(current));
        assert(strcmp(first, current) == 0);
    }
    PASS();
}

int main(int argc, char *argv[])
{
    (void)argc; (void)argv;

    printf("MD5 test suite:\n");

    test_basic_hash();
    test_empty_string();
    test_single_char();
    test_rfc1321_vectors();
    test_streaming_calc();
    test_streaming_byte_at_a_time();
    test_exactly_64_bytes();
    test_55_bytes();
    test_56_bytes();
    test_tobytes();
    test_tobytes_partial();
    test_tobytes_null();
    test_tostring_short_buf();
    test_tostring_null();
    test_new_free();
    test_large_data();
    test_binary_data();
    test_streaming_across_block_boundary();
    test_different_inputs_different_hashes();
    test_benchmark();
    test_stability();

    printf("All %d tests passed.\n", test_nr);
    return 0;
}

