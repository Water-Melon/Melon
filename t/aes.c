#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mln_string.h"
#include "mln_aes.h"

static int test_count = 0;
static int pass_count = 0;
static int fail_count = 0;

static void check(const char *name, int cond)
{
    test_count++;
    if (cond) {
        pass_count++;
    } else {
        fail_count++;
        fprintf(stderr, "FAIL: %s\n", name);
    }
}

static void hex2bin(const char *hex, mln_u8_t *bin, int len)
{
    int i;
    for (i = 0; i < len; i++) {
        unsigned int v;
        sscanf(hex + i * 2, "%2x", &v);
        bin[i] = (mln_u8_t)v;
    }
}

/* Test AES-128 basic encrypt/decrypt roundtrip */
static void test_aes128_roundtrip(void)
{
    mln_aes_t a;
    char p[] = "1234567890123456";
    char saved[17];
    mln_string_t s;

    memcpy(saved, p, 16);
    saved[16] = '\0';

    check("aes128_init", mln_aes_init(&a, (mln_u8ptr_t)"abcdefghijklmnop", M_AES_128) == 0);

    mln_string_set(&s, p);
    check("aes128_encrypt", mln_aes_encrypt(&a, s.data) == 0);
    check("aes128_ciphertext_differs", memcmp(s.data, saved, 16) != 0);
    check("aes128_decrypt", mln_aes_decrypt(&a, s.data) == 0);
    check("aes128_roundtrip", memcmp(s.data, saved, 16) == 0);
}

/* Test AES-192 basic encrypt/decrypt roundtrip */
static void test_aes192_roundtrip(void)
{
    mln_aes_t a;
    mln_u8_t text[16] = "TestAES192Block!";
    mln_u8_t saved[16];

    memcpy(saved, text, 16);

    check("aes192_init", mln_aes_init(&a, (mln_u8ptr_t)"abcdefghijklmnopqrstuvwx", M_AES_192) == 0);
    check("aes192_encrypt", mln_aes_encrypt(&a, text) == 0);
    check("aes192_ciphertext_differs", memcmp(text, saved, 16) != 0);
    check("aes192_decrypt", mln_aes_decrypt(&a, text) == 0);
    check("aes192_roundtrip", memcmp(text, saved, 16) == 0);
}

/* Test AES-256 basic encrypt/decrypt roundtrip */
static void test_aes256_roundtrip(void)
{
    mln_aes_t a;
    mln_u8_t text[16] = "TestAES256Block!";
    mln_u8_t saved[16];

    memcpy(saved, text, 16);

    check("aes256_init", mln_aes_init(&a, (mln_u8ptr_t)"abcdefghijklmnopqrstuvwxyz012345", M_AES_256) == 0);
    check("aes256_encrypt", mln_aes_encrypt(&a, text) == 0);
    check("aes256_ciphertext_differs", memcmp(text, saved, 16) != 0);
    check("aes256_decrypt", mln_aes_decrypt(&a, text) == 0);
    check("aes256_roundtrip", memcmp(text, saved, 16) == 0);
}

/* Test invalid bits parameter */
static void test_invalid_bits(void)
{
    mln_aes_t a;
    check("invalid_bits_3", mln_aes_init(&a, (mln_u8ptr_t)"abcdefghijklmnop", 3) < 0);
    check("invalid_bits_99", mln_aes_init(&a, (mln_u8ptr_t)"abcdefghijklmnop", 99) < 0);
}

/* Test that same key+plaintext always produces same ciphertext (determinism) */
static void test_determinism(void)
{
    mln_aes_t a;
    mln_u8_t text1[16] = "DeterministicTst";
    mln_u8_t text2[16] = "DeterministicTst";

    mln_aes_init(&a, (mln_u8ptr_t)"abcdefghijklmnop", M_AES_128);
    mln_aes_encrypt(&a, text1);
    mln_aes_encrypt(&a, text2);
    check("determinism", memcmp(text1, text2, 16) == 0);
}

/* Test that different keys produce different ciphertext */
static void test_different_keys(void)
{
    mln_aes_t a1, a2;
    mln_u8_t text1[16] = "SameTextDiffKey!";
    mln_u8_t text2[16] = "SameTextDiffKey!";

    mln_aes_init(&a1, (mln_u8ptr_t)"aaaaaaaaaaaaaaaa", M_AES_128);
    mln_aes_init(&a2, (mln_u8ptr_t)"bbbbbbbbbbbbbbbb", M_AES_128);
    mln_aes_encrypt(&a1, text1);
    mln_aes_encrypt(&a2, text2);
    check("different_keys_different_cipher", memcmp(text1, text2, 16) != 0);
}

/* Test that different plaintexts produce different ciphertexts */
static void test_different_plaintexts(void)
{
    mln_aes_t a;
    mln_u8_t text1[16] = "PlaintextAAAAA01";
    mln_u8_t text2[16] = "PlaintextBBBBB02";

    mln_aes_init(&a, (mln_u8ptr_t)"abcdefghijklmnop", M_AES_128);
    mln_aes_encrypt(&a, text1);
    mln_aes_encrypt(&a, text2);
    check("different_plaintexts_different_cipher", memcmp(text1, text2, 16) != 0);
}

/* Test all-zero plaintext and all-zero key */
static void test_zero_data(void)
{
    mln_aes_t a;
    mln_u8_t key[16];
    mln_u8_t text[16];
    mln_u8_t saved[16];

    memset(key, 0, 16);
    memset(text, 0, 16);
    memset(saved, 0, 16);

    mln_aes_init(&a, key, M_AES_128);
    mln_aes_encrypt(&a, text);
    check("zero_encrypt_changes", memcmp(text, saved, 16) != 0);
    mln_aes_decrypt(&a, text);
    check("zero_roundtrip", memcmp(text, saved, 16) == 0);
}

/* Test all-0xff plaintext and key */
static void test_allff_data(void)
{
    mln_aes_t a;
    mln_u8_t key[16];
    mln_u8_t text[16];
    mln_u8_t saved[16];

    memset(key, 0xff, 16);
    memset(text, 0xff, 16);
    memset(saved, 0xff, 16);

    mln_aes_init(&a, key, M_AES_128);
    mln_aes_encrypt(&a, text);
    check("allff_encrypt_changes", memcmp(text, saved, 16) != 0);
    mln_aes_decrypt(&a, text);
    check("allff_roundtrip", memcmp(text, saved, 16) == 0);
}

/* Test mln_aes_new / mln_aes_free (heap allocation) */
static void test_new_free(void)
{
    mln_aes_t *a;
    mln_u8_t text[16] = "HeapAllocTest!XY";
    mln_u8_t saved[16];

    memcpy(saved, text, 16);

    a = mln_aes_new((mln_u8ptr_t)"abcdefghijklmnop", M_AES_128);
    check("aes_new_not_null", a != NULL);
    if (a != NULL) {
        mln_aes_encrypt(a, text);
        check("aes_new_encrypt_changes", memcmp(text, saved, 16) != 0);
        mln_aes_decrypt(a, text);
        check("aes_new_roundtrip", memcmp(text, saved, 16) == 0);
        mln_aes_free(a);
    }

    /* mln_aes_free(NULL) should not crash */
    mln_aes_free(NULL);
    check("aes_free_null_safe", 1);
}

/* Test multiple blocks encryption (simulating ECB mode) */
static void test_multiple_blocks(void)
{
    mln_aes_t a;
    mln_u8_t data[48] = "Block1__16bytes!Block2__16bytes!Block3__16bytes!";
    mln_u8_t saved[48];
    int i;

    memcpy(saved, data, 48);
    mln_aes_init(&a, (mln_u8ptr_t)"abcdefghijklmnop", M_AES_128);

    for (i = 0; i < 48; i += 16)
        mln_aes_encrypt(&a, data + i);

    check("multiblock_encrypted", memcmp(data, saved, 48) != 0);

    for (i = 0; i < 48; i += 16)
        mln_aes_decrypt(&a, data + i);

    check("multiblock_roundtrip", memcmp(data, saved, 48) == 0);
}

/* Test cross-key-size: encrypt with one size, verify cannot decrypt with different size */
static void test_cross_key_mismatch(void)
{
    mln_aes_t a128, a256;
    mln_u8_t text[16] = "CrossKeyMismatch";
    mln_u8_t saved[16];

    memcpy(saved, text, 16);
    mln_aes_init(&a128, (mln_u8ptr_t)"abcdefghijklmnop", M_AES_128);
    mln_aes_init(&a256, (mln_u8ptr_t)"abcdefghijklmnopqrstuvwxyz012345", M_AES_256);

    mln_aes_encrypt(&a128, text);
    mln_aes_decrypt(&a256, text);
    check("cross_key_mismatch_no_roundtrip", memcmp(text, saved, 16) != 0);
}

/* Test known AES-128 test vector (NIST FIPS 197 Appendix B) */
static void test_nist_vector(void)
{
    mln_aes_t a;
    mln_u8_t key[16], text[16], expected[16];

    /* NIST FIPS 197 Appendix B */
    hex2bin("2b7e151628aed2a6abf7158809cf4f3c", key, 16);
    hex2bin("3243f6a8885a308d313198a2e0370734", text, 16);
    hex2bin("3925841d02dc09fbdc118597196a0b32", expected, 16);

    mln_aes_init(&a, key, M_AES_128);
    mln_aes_encrypt(&a, text);
    check("nist_aes128_encrypt", memcmp(text, expected, 16) == 0);

    mln_aes_decrypt(&a, text);
    hex2bin("3243f6a8885a308d313198a2e0370734", expected, 16);
    check("nist_aes128_decrypt", memcmp(text, expected, 16) == 0);
}

/* Test re-initialization with different key */
static void test_reinit(void)
{
    mln_aes_t a;
    mln_u8_t text1[16] = "ReinitTestData01";
    mln_u8_t text2[16] = "ReinitTestData01";
    mln_u8_t saved[16];

    memcpy(saved, text1, 16);

    mln_aes_init(&a, (mln_u8ptr_t)"key1key1key1key1", M_AES_128);
    mln_aes_encrypt(&a, text1);

    mln_aes_init(&a, (mln_u8ptr_t)"key2key2key2key2", M_AES_128);
    mln_aes_encrypt(&a, text2);

    check("reinit_different_output", memcmp(text1, text2, 16) != 0);

    mln_aes_decrypt(&a, text2);
    check("reinit_roundtrip_key2", memcmp(text2, saved, 16) == 0);
}

/* Test double encrypt and double decrypt */
static void test_double_encrypt(void)
{
    mln_aes_t a;
    mln_u8_t text[16] = "DoubleEncDecTest";
    mln_u8_t saved[16];

    memcpy(saved, text, 16);
    mln_aes_init(&a, (mln_u8ptr_t)"abcdefghijklmnop", M_AES_128);

    mln_aes_encrypt(&a, text);
    mln_aes_encrypt(&a, text);
    mln_aes_decrypt(&a, text);
    mln_aes_decrypt(&a, text);

    check("double_encrypt_decrypt_roundtrip", memcmp(text, saved, 16) == 0);
}

int main(int argc, char *argv[])
{
    test_aes128_roundtrip();
    test_aes192_roundtrip();
    test_aes256_roundtrip();
    test_invalid_bits();
    test_determinism();
    test_different_keys();
    test_different_plaintexts();
    test_zero_data();
    test_allff_data();
    test_new_free();
    test_multiple_blocks();
    test_cross_key_mismatch();
    test_nist_vector();
    test_reinit();
    test_double_encrypt();

    printf("\nAES Tests: %d passed, %d failed, %d total\n",
           pass_count, fail_count, test_count);

    return fail_count > 0 ? 1 : 0;
}
