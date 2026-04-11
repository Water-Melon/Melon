#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "mln_string.h"
#include "mln_base64.h"
#include "mln_alloc.h"

static int g_failed = 0;

#define EXPECT(cond, msg) \
    do { \
        if (!(cond)) { \
            fprintf(stderr, "FAIL: %s (line %d)\n", (msg), __LINE__); \
            g_failed = 1; \
        } \
    } while (0)

/*
 * Encode `in_str` and verify the encoding equals `expected`, then
 * decode the encoding and verify the result equals `in_str` again.
 */
static void roundtrip(const char *label, const char *in_str, mln_uauto_t in_len, const char *expected)
{
    mln_u8ptr_t enc = NULL, dec = NULL;
    mln_uauto_t enc_len = 0, dec_len = 0;

    if (mln_base64_encode((mln_u8ptr_t)in_str, in_len, &enc, &enc_len) < 0) {
        fprintf(stderr, "FAIL: encode failed for %s\n", label);
        g_failed = 1;
        return;
    }

    if (enc_len != strlen(expected) || memcmp(enc, expected, enc_len) != 0) {
        fprintf(stderr, "FAIL: %s encode mismatch. got '%.*s' want '%s'\n",
                label, (int)enc_len, enc, expected);
        g_failed = 1;
    }

    if (mln_base64_decode(enc, enc_len, &dec, &dec_len) < 0) {
        fprintf(stderr, "FAIL: decode failed for %s\n", label);
        g_failed = 1;
        mln_base64_free(enc);
        return;
    }

    if (dec_len != in_len || memcmp(dec, in_str, in_len) != 0) {
        fprintf(stderr, "FAIL: %s decode mismatch. got %zu bytes want %zu bytes\n",
                label, (size_t)dec_len, (size_t)in_len);
        g_failed = 1;
    }

    mln_base64_free(enc);
    mln_base64_free(dec);
}

static void test_rfc4648_vectors(void)
{
    /* RFC 4648 §10 test vectors. */
    roundtrip("empty",   "",       0, "");
    roundtrip("f",       "f",      1, "Zg==");
    roundtrip("fo",      "fo",     2, "Zm8=");
    roundtrip("foo",     "foo",    3, "Zm9v");
    roundtrip("foob",    "foob",   4, "Zm9vYg==");
    roundtrip("fooba",   "fooba",  5, "Zm9vYmE=");
    roundtrip("foobar",  "foobar", 6, "Zm9vYmFy");
    roundtrip("Hello",   "Hello",  5, "SGVsbG8=");
    roundtrip("Sphinx",  "Sphinx of black quartz, judge my vow.", 37,
              "U3BoaW54IG9mIGJsYWNrIHF1YXJ0eiwganVkZ2UgbXkgdm93Lg==");
}

static void test_all_bytes(void)
{
    /* Every possible byte value, length 256: exercises full alphabet. */
    mln_u8_t buf[256];
    for (int i = 0; i < 256; ++i) buf[i] = (mln_u8_t)i;

    mln_u8ptr_t enc = NULL, dec = NULL;
    mln_uauto_t enc_len = 0, dec_len = 0;

    EXPECT(mln_base64_encode(buf, sizeof(buf), &enc, &enc_len) == 0, "all-bytes encode");
    /* 256 bytes -> 344 chars, padding-free (256 is not a multiple of 3). */
    EXPECT(enc_len == ((256 + 2) / 3) * 4, "all-bytes encoded length");

    EXPECT(mln_base64_decode(enc, enc_len, &dec, &dec_len) == 0, "all-bytes decode");
    EXPECT(dec_len == sizeof(buf), "all-bytes decoded length");
    EXPECT(memcmp(dec, buf, sizeof(buf)) == 0, "all-bytes content");

    mln_base64_free(enc);
    mln_base64_free(dec);
}

static void test_various_lengths(void)
{
    /* Every length 0..300 gives a full round-trip. */
    mln_u8_t src[320];
    for (int i = 0; i < (int)sizeof(src); ++i) src[i] = (mln_u8_t)((i * 131 + 7) & 0xff);

    for (mln_uauto_t n = 0; n <= 300; ++n) {
        mln_u8ptr_t enc = NULL, dec = NULL;
        mln_uauto_t enc_len = 0, dec_len = 0;

        if (mln_base64_encode(src, n, &enc, &enc_len) < 0) {
            fprintf(stderr, "FAIL: encode(len=%zu)\n", (size_t)n);
            g_failed = 1;
            continue;
        }
        if (mln_base64_decode(enc, enc_len, &dec, &dec_len) < 0) {
            fprintf(stderr, "FAIL: decode(len=%zu)\n", (size_t)n);
            g_failed = 1;
            mln_base64_free(enc);
            continue;
        }
        if (dec_len != n || memcmp(dec, src, n) != 0) {
            fprintf(stderr, "FAIL: roundtrip(len=%zu) mismatch\n", (size_t)n);
            g_failed = 1;
        }
        mln_base64_free(enc);
        mln_base64_free(dec);
    }
}

static void test_pool_api(void)
{
    mln_alloc_t *pool = mln_alloc_init(NULL, 0);
    EXPECT(pool != NULL, "mln_alloc_init");

    const char *msg = "The quick brown fox jumps over the lazy dog";
    mln_u8ptr_t enc = NULL, dec = NULL;
    mln_uauto_t enc_len = 0, dec_len = 0;

    EXPECT(mln_base64_pool_encode(pool, (mln_u8ptr_t)msg, strlen(msg), &enc, &enc_len) == 0,
           "pool encode");
    EXPECT(enc_len == 60, "pool encoded length");
    EXPECT(memcmp(enc, "VGhlIHF1aWNrIGJyb3duIGZveCBqdW1wcyBvdmVyIHRoZSBsYXp5IGRvZw==", 60) == 0,
           "pool encoded content");

    EXPECT(mln_base64_pool_decode(pool, enc, enc_len, &dec, &dec_len) == 0, "pool decode");
    EXPECT(dec_len == strlen(msg), "pool decoded length");
    EXPECT(memcmp(dec, msg, dec_len) == 0, "pool decoded content");

    mln_base64_pool_free(enc);
    mln_base64_pool_free(dec);
    mln_alloc_destroy(pool);
}

static void test_decode_invalid_length(void)
{
    mln_u8ptr_t dec = NULL;
    mln_uauto_t dec_len = 0;
    /* Length 5 is not a multiple of 4 and must be rejected. */
    int r = mln_base64_decode((mln_u8ptr_t)"ABCDE", 5, &dec, &dec_len);
    EXPECT(r < 0, "decode rejects non-multiple-of-4 input");
}

static void test_free_null(void)
{
    /* Freeing NULL is allowed and is a no-op. */
    mln_base64_free(NULL);
    mln_base64_pool_free(NULL);
}

int main(int argc, char *argv[])
{
    /* Keep the original friendly demo output. */
    mln_string_t text = mln_string("Hello");
    mln_string_t tmp;
    mln_u8ptr_t p1, p2;
    mln_uauto_t len1, len2;

    if (mln_base64_encode(text.data, text.len, &p1, &len1) < 0) {
        fprintf(stderr, "encode failed\n");
        return -1;
    }
    mln_string_nset(&tmp, p1, len1);
    write(STDOUT_FILENO, tmp.data, tmp.len);
    write(STDOUT_FILENO, "\n", 1);

    if (mln_base64_decode(p1, len1, &p2, &len2) < 0) {
        fprintf(stderr, "decode failed\n");
        return -1;
    }
    mln_string_nset(&tmp, p2, len2);
    write(STDOUT_FILENO, tmp.data, tmp.len);
    write(STDOUT_FILENO, "\n", 1);

    mln_base64_free(p1);
    mln_base64_free(p2);

    test_rfc4648_vectors();
    test_all_bytes();
    test_various_lengths();
    test_pool_api();
    test_decode_invalid_length();
    test_free_null();

    if (g_failed) {
        fprintf(stderr, "base64 tests FAILED\n");
        return 1;
    }
    fprintf(stdout, "base64 tests PASSED\n");
    return 0;
}
