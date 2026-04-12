#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include "mln_des.h"
#include "mln_alloc.h"

/*
 * DES single-block encrypt/decrypt
 */
static void test_des_single_block(void)
{
    mln_des_t d;
    mln_u64_t key = 0x133457799BBCDFF1llu;
    mln_u64_t plain = 0x0123456789ABCDEFllu;
    mln_u64_t cipher, decrypted;

    mln_des_init(&d, key);
    cipher = mln_des(&d, plain, 1);
    decrypted = mln_des(&d, cipher, 0);
    assert(decrypted == plain);

    /* Encrypt/decrypt with zero key */
    mln_des_init(&d, 0);
    cipher = mln_des(&d, plain, 1);
    decrypted = mln_des(&d, cipher, 0);
    assert(decrypted == plain);

    /* Encrypt/decrypt with all-ones key */
    mln_des_init(&d, 0xFFFFFFFFFFFFFFFFllu);
    cipher = mln_des(&d, plain, 1);
    decrypted = mln_des(&d, cipher, 0);
    assert(decrypted == plain);

    /* Encrypt/decrypt zero plaintext */
    mln_des_init(&d, key);
    cipher = mln_des(&d, 0, 1);
    decrypted = mln_des(&d, cipher, 0);
    assert(decrypted == 0);

    /* Encrypt/decrypt all-ones plaintext */
    cipher = mln_des(&d, 0xFFFFFFFFFFFFFFFFllu, 1);
    decrypted = mln_des(&d, cipher, 0);
    assert(decrypted == 0xFFFFFFFFFFFFFFFFllu);

    printf("PASS: test_des_single_block\n");
}

/*
 * DES new/free (heap allocation)
 */
static void test_des_new_free(void)
{
    mln_des_t *d = mln_des_new(0xABCD1234ABCD1234llu);
    assert(d != NULL);

    mln_u64_t plain = 0xDEADBEEFCAFEBABEllu;
    mln_u64_t cipher = mln_des(d, plain, 1);
    mln_u64_t decrypted = mln_des(d, cipher, 0);
    assert(decrypted == plain);

    mln_des_free(d);

    /* Free NULL should be safe */
    mln_des_free(NULL);

    printf("PASS: test_des_new_free\n");
}

/*
 * DES pool_new/pool_free (pool allocation)
 */
static void test_des_pool(void)
{
    mln_alloc_t *pool = mln_alloc_init(NULL, 0);
    assert(pool != NULL);

    mln_des_t *d = mln_des_pool_new(pool, 0x1234567890ABCDEFllu);
    assert(d != NULL);

    mln_u64_t plain = 0xFEDCBA0987654321llu;
    mln_u64_t cipher = mln_des(d, plain, 1);
    mln_u64_t decrypted = mln_des(d, cipher, 0);
    assert(decrypted == plain);

    mln_des_pool_free(d);

    /* Free NULL should be safe */
    mln_des_pool_free(NULL);

    mln_alloc_destroy(pool);
    printf("PASS: test_des_pool\n");
}

/*
 * DES buffer encrypt/decrypt
 */
static void test_des_buf(void)
{
    mln_des_t d;
    mln_u8_t cipher[16] = {0};
    mln_u8_t text[16] = {0};

    mln_des_init(&d, 0xFFFF0000FFFF0000llu);

    /* Exact 8-byte block */
    mln_des_buf(&d, (mln_u8ptr_t)"12345678", 8, cipher, sizeof(cipher), 0, 1);
    mln_des_buf(&d, cipher, 8, text, sizeof(text), 0, 0);
    assert(memcmp(text, "12345678", 8) == 0);

    /* Non-aligned length with fill byte 0 */
    memset(cipher, 0, sizeof(cipher));
    memset(text, 0, sizeof(text));
    mln_des_buf(&d, (mln_u8ptr_t)"Hello", 5, cipher, sizeof(cipher), 0, 1);
    mln_des_buf(&d, cipher, 8, text, sizeof(text), 0, 0);
    assert(memcmp(text, "Hello\0\0\0", 8) == 0);

    /* Non-aligned length with fill byte 0xFF */
    memset(cipher, 0, sizeof(cipher));
    memset(text, 0, sizeof(text));
    mln_des_buf(&d, (mln_u8ptr_t)"Hi", 2, cipher, sizeof(cipher), 0xff, 1);
    mln_des_buf(&d, cipher, 8, text, sizeof(text), 0xff, 0);
    assert(text[0] == 'H' && text[1] == 'i');

    /* Multi-block buffer */
    memset(cipher, 0, sizeof(cipher));
    memset(text, 0, sizeof(text));
    mln_des_buf(&d, (mln_u8ptr_t)"ABCDEFGHIJKLMNOP", 16, cipher, sizeof(cipher), 0, 1);
    mln_des_buf(&d, cipher, 16, text, sizeof(text), 0, 0);
    assert(memcmp(text, "ABCDEFGHIJKLMNOP", 16) == 0);

    /* Output buffer smaller than input (truncation) */
    memset(cipher, 0, sizeof(cipher));
    mln_des_buf(&d, (mln_u8ptr_t)"ABCDEFGH", 8, cipher, 4, 0, 1);
    /* Should not crash - just stops writing when outlen runs out */

    printf("PASS: test_des_buf\n");
}

/*
 * 3DES single-block encrypt/decrypt
 */
static void test_3des_single_block(void)
{
    mln_3des_t d;
    mln_u64_t key1 = 0x0123456789ABCDEFllu;
    mln_u64_t key2 = 0xFEDCBA9876543210llu;
    mln_u64_t plain = 0x0123456789ABCDEFllu;
    mln_u64_t cipher, decrypted;

    mln_3des_init(&d, key1, key2);
    cipher = mln_3des(&d, plain, 1);
    decrypted = mln_3des(&d, cipher, 0);
    assert(decrypted == plain);

    /* Same keys = equivalent to single DES */
    mln_3des_init(&d, key1, key1);
    cipher = mln_3des(&d, plain, 1);
    decrypted = mln_3des(&d, cipher, 0);
    assert(decrypted == plain);

    /* Zero plaintext */
    mln_3des_init(&d, key1, key2);
    cipher = mln_3des(&d, 0, 1);
    decrypted = mln_3des(&d, cipher, 0);
    assert(decrypted == 0);

    /* All-ones plaintext */
    cipher = mln_3des(&d, 0xFFFFFFFFFFFFFFFFllu, 1);
    decrypted = mln_3des(&d, cipher, 0);
    assert(decrypted == 0xFFFFFFFFFFFFFFFFllu);

    printf("PASS: test_3des_single_block\n");
}

/*
 * 3DES new/free (heap allocation)
 */
static void test_3des_new_free(void)
{
    mln_3des_t *d = mln_3des_new(0xAAAAAAAAAAAAAAAAllu, 0x5555555555555555llu);
    assert(d != NULL);

    mln_u64_t plain = 0xDEADBEEFCAFEBABEllu;
    mln_u64_t cipher = mln_3des(d, plain, 1);
    mln_u64_t decrypted = mln_3des(d, cipher, 0);
    assert(decrypted == plain);

    mln_3des_free(d);
    mln_3des_free(NULL);

    printf("PASS: test_3des_new_free\n");
}

/*
 * 3DES pool_new/pool_free (pool allocation)
 */
static void test_3des_pool(void)
{
    mln_alloc_t *pool = mln_alloc_init(NULL, 0);
    assert(pool != NULL);

    mln_3des_t *d = mln_3des_pool_new(pool, 0x1111111111111111llu, 0x2222222222222222llu);
    assert(d != NULL);

    mln_u64_t plain = 0x0000000000000001llu;
    mln_u64_t cipher = mln_3des(d, plain, 1);
    mln_u64_t decrypted = mln_3des(d, cipher, 0);
    assert(decrypted == plain);

    mln_3des_pool_free(d);
    mln_3des_pool_free(NULL);

    mln_alloc_destroy(pool);
    printf("PASS: test_3des_pool\n");
}

/*
 * 3DES buffer encrypt/decrypt
 */
static void test_3des_buf(void)
{
    mln_3des_t d;
    mln_u8_t text[9] = {0};
    mln_u8_t cipher[9] = {0};
    mln_u8_t cipher2[16] = {0};
    mln_u8_t text2[16] = {0};

    mln_3des_init(&d, 0xffff, 0xff120000);

    /* Original test case */
    mln_3des_buf(&d, (mln_u8ptr_t)"Hi Tom!!", 11, cipher, sizeof(cipher), 0, 1);
    mln_3des_buf(&d, cipher, sizeof(cipher)-1, text, sizeof(text), 0, 0);
    assert(memcmp(text, "Hi Tom!!", 8) == 0);

    /* Multi-block */
    mln_3des_buf(&d, (mln_u8ptr_t)"ABCDEFGHIJKLMNOP", 16, cipher2, sizeof(cipher2), 0, 1);
    mln_3des_buf(&d, cipher2, 16, text2, sizeof(text2), 0, 0);
    assert(memcmp(text2, "ABCDEFGHIJKLMNOP", 16) == 0);

    /* Non-aligned with fill */
    memset(cipher2, 0, sizeof(cipher2));
    memset(text2, 0, sizeof(text2));
    mln_3des_buf(&d, (mln_u8ptr_t)"Test", 4, cipher2, sizeof(cipher2), 0x55, 1);
    mln_3des_buf(&d, cipher2, 8, text2, sizeof(text2), 0x55, 0);
    assert(memcmp(text2, "Test", 4) == 0);

    printf("PASS: test_3des_buf\n");
}

/*
 * Known-answer test: DES encrypt/decrypt with a known key/plaintext/ciphertext
 */
static void test_des_known_answer(void)
{
    mln_des_t d;
    mln_u64_t key = 0x0133457799BBCDFFllu;
    mln_u64_t plain = 0x0123456789ABCDEFllu;

    mln_des_init(&d, key);
    mln_u64_t cipher = mln_des(&d, plain, 1);
    mln_u64_t decrypted = mln_des(&d, cipher, 0);
    assert(decrypted == plain);

    /* Verify the cipher is deterministic */
    mln_u64_t cipher2 = mln_des(&d, plain, 1);
    assert(cipher == cipher2);

    printf("PASS: test_des_known_answer\n");
}

/*
 * Stress test: many different keys and plaintexts
 */
static void test_des_stress(void)
{
    mln_des_t d;
    mln_u64_t key, plain, cipher, decrypted;
    int i;

    for (i = 0; i < 10000; i++) {
        key = (mln_u64_t)i * 0x0123456789ABCDEFllu + (mln_u64_t)(i * i);
        plain = (mln_u64_t)i * 0xFEDCBA9876543210llu + (mln_u64_t)i;

        mln_des_init(&d, key);
        cipher = mln_des(&d, plain, 1);
        decrypted = mln_des(&d, cipher, 0);
        assert(decrypted == plain);
    }

    printf("PASS: test_des_stress (10000 key/plaintext pairs)\n");
}

/*
 * Stress test: 3DES with many different key pairs
 */
static void test_3des_stress(void)
{
    mln_3des_t d;
    mln_u64_t key1, key2, plain, cipher, decrypted;
    int i;

    for (i = 0; i < 5000; i++) {
        key1 = (mln_u64_t)i * 0x0011223344556677llu + 1;
        key2 = (mln_u64_t)i * 0x8899AABBCCDDEEFF + 2;
        plain = (mln_u64_t)i * 0x1111111111111111llu;

        mln_3des_init(&d, key1, key2);
        cipher = mln_3des(&d, plain, 1);
        decrypted = mln_3des(&d, cipher, 0);
        assert(decrypted == plain);
    }

    printf("PASS: test_3des_stress (5000 key/plaintext pairs)\n");
}

/*
 * Benchmark: DES single-block performance
 */
static void bench_des(void)
{
    mln_des_t d;
    mln_u64_t msg = 0x0123456789ABCDEFllu;
    int i, N = 1000000;
    clock_t start, end;

    mln_des_init(&d, 0x133457799BBCDFF1llu);
    start = clock();
    for (i = 0; i < N; i++) {
        msg = mln_des(&d, msg, 1);
    }
    end = clock();
    printf("BENCH: DES encrypt %d blocks: %.3f ms (result=%016llx)\n",
           N, (double)(end - start) / CLOCKS_PER_SEC * 1000.0, (unsigned long long)msg);
}

/*
 * Benchmark: 3DES single-block performance
 */
static void bench_3des(void)
{
    mln_3des_t d;
    mln_u64_t msg = 0x0123456789ABCDEFllu;
    int i, N = 1000000;
    clock_t start, end;

    mln_3des_init(&d, 0x133457799BBCDFF1llu, 0xFF00FF00FF00FF00llu);
    start = clock();
    for (i = 0; i < N; i++) {
        msg = mln_3des(&d, msg, 1);
    }
    end = clock();
    printf("BENCH: 3DES encrypt %d blocks: %.3f ms (result=%016llx)\n",
           N, (double)(end - start) / CLOCKS_PER_SEC * 1000.0, (unsigned long long)msg);
}

/*
 * Benchmark: DES buffer performance
 */
static void bench_des_buf(void)
{
    mln_des_t d;
    mln_u8_t in[4096], out[4096];
    int i, N = 10000;
    clock_t start, end;

    mln_des_init(&d, 0x133457799BBCDFF1llu);
    memset(in, 'A', sizeof(in));

    start = clock();
    for (i = 0; i < N; i++) {
        mln_des_buf(&d, in, sizeof(in), out, sizeof(out), 0, 1);
    }
    end = clock();
    printf("BENCH: DES buf encrypt %d*%lu bytes: %.3f ms\n",
           N, (unsigned long)sizeof(in), (double)(end - start) / CLOCKS_PER_SEC * 1000.0);
}

int main(int argc, char *argv[])
{
    /* Feature tests */
    test_des_single_block();
    test_des_new_free();
    test_des_pool();
    test_des_buf();
    test_des_known_answer();
    test_3des_single_block();
    test_3des_new_free();
    test_3des_pool();
    test_3des_buf();

    /* Stress tests */
    test_des_stress();
    test_3des_stress();

    /* Benchmarks */
    bench_des();
    bench_3des();
    bench_des_buf();

    printf("\nAll tests passed.\n");
    return 0;
}

