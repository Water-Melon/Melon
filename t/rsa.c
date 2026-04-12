
/*
 * Copyright (C) Niklaus F.Schen.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#if defined(MSVC)
#include <windows.h>
#include <io.h>
#else
#include <sys/time.h>
#include <unistd.h>
#endif
#include "mln_rsa.h"
#include "mln_alloc.h"
#include "mln_func.h"

static double now_us(void)
{
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

static void roundtrip_encrypt_decrypt(mln_rsa_key_t *pub, mln_rsa_key_t *pri, const char *msg)
{
    mln_string_t tmp, *cipher, *plain;
    mln_string_nset(&tmp, (char *)msg, strlen(msg));

    /* public encrypt -> private decrypt */
    cipher = mln_RSAESPKCS1V15_public_encrypt(pub, &tmp);
    assert(cipher != NULL);

    plain = mln_RSAESPKCS1V15_private_decrypt(pri, cipher);
    assert(plain != NULL);
    assert(plain->len == strlen(msg));
    assert(memcmp(plain->data, msg, plain->len) == 0);
    mln_RSAESPKCS1V15_free(cipher);
    mln_RSAESPKCS1V15_free(plain);

    /* reverse: private encrypt -> public decrypt */
    cipher = mln_RSAESPKCS1V15_private_encrypt(pri, &tmp);
    assert(cipher != NULL);

    plain = mln_RSAESPKCS1V15_public_decrypt(pub, cipher);
    assert(plain != NULL);
    assert(plain->len == strlen(msg));
    assert(memcmp(plain->data, msg, plain->len) == 0);
    mln_RSAESPKCS1V15_free(cipher);
    mln_RSAESPKCS1V15_free(plain);
}

static void sign_verify_case(mln_alloc_t *pool, mln_rsa_key_t *pub, mln_rsa_key_t *pri,
                             const char *msg, mln_u32_t ht)
{
    mln_string_t m, *sig;
    mln_string_nset(&m, (char *)msg, strlen(msg));

    sig = mln_RSASSAPKCS1V15_sign(pool, pri, &m, ht);
    assert(sig != NULL);
    assert(mln_RSASSAPKCS1V15_verify(pool, pub, &m, sig) == 0);

    /* tamper detection */
    sig->data[0] ^= 0x01;
    assert(mln_RSASSAPKCS1V15_verify(pool, pub, &m, sig) != 0);
    sig->data[0] ^= 0x01;

    mln_RSAESPKCS1V15_free(sig);
}

static void bench_rsa(mln_alloc_t *pool, mln_rsa_key_t *pub, mln_rsa_key_t *pri)
{
    const int N = 20;
    int i;
    double t0, t1;
    mln_string_t tmp, *cipher, *plain, *sig;
    char *msg = "Bench";
    mln_string_nset(&tmp, msg, strlen(msg));

    /* public encrypt bench */
    t0 = now_us();
    for (i = 0; i < N; ++i) {
        cipher = mln_RSAESPKCS1V15_public_encrypt(pub, &tmp);
        assert(cipher != NULL);
        mln_RSAESPKCS1V15_free(cipher);
    }
    t1 = now_us();
    printf("  public_encrypt : %8.2f us/op (%d iters)\n", (t1 - t0) / N, N);

    /* private decrypt bench (re-encrypt each time since decrypt modifies cipher) */
    t0 = now_us();
    for (i = 0; i < N; ++i) {
        cipher = mln_RSAESPKCS1V15_public_encrypt(pub, &tmp);
        assert(cipher != NULL);
        plain = mln_RSAESPKCS1V15_private_decrypt(pri, cipher);
        assert(plain != NULL);
        mln_RSAESPKCS1V15_free(plain);
        mln_RSAESPKCS1V15_free(cipher);
    }
    t1 = now_us();
    printf("  pub_enc+pri_dec: %8.2f us/op (%d iters)\n", (t1 - t0) / N, N);

    /* sign bench */
    t0 = now_us();
    for (i = 0; i < N; ++i) {
        sig = mln_RSASSAPKCS1V15_sign(pool, pri, &tmp, M_EMSAPKCS1V15_HASH_SHA256);
        assert(sig != NULL);
        mln_RSAESPKCS1V15_free(sig);
    }
    t1 = now_us();
    printf("  sign (SHA-256) : %8.2f us/op (%d iters)\n", (t1 - t0) / N, N);

    /* verify bench (re-sign each time since verify modifies sig in-place) */
    t0 = now_us();
    for (i = 0; i < N; ++i) {
        sig = mln_RSASSAPKCS1V15_sign(pool, pri, &tmp, M_EMSAPKCS1V15_HASH_SHA256);
        assert(sig != NULL);
        assert(mln_RSASSAPKCS1V15_verify(pool, pub, &tmp, sig) == 0);
        mln_RSAESPKCS1V15_free(sig);
    }
    t1 = now_us();
    printf("  sign+verify    : %8.2f us/op (%d iters)\n", (t1 - t0) / N, N);
}

static void stability_test(mln_rsa_key_t *pub, mln_rsa_key_t *pri)
{
    int i;
    mln_string_t tmp, *cipher, *plain;
    char msg[64];

    for (i = 0; i < 32; ++i) {
        int len = 1 + (i * 3) % 4;  /* keep short for 128-bit key (max payload ~5 bytes) */
        int k;
        for (k = 0; k < len; ++k) msg[k] = 'A' + ((i + k) % 26);
        msg[len] = 0;
        mln_string_nset(&tmp, msg, len);

        cipher = mln_RSAESPKCS1V15_public_encrypt(pub, &tmp);
        assert(cipher != NULL);
        plain = mln_RSAESPKCS1V15_private_decrypt(pri, cipher);
        assert(plain != NULL);
        assert(plain->len == (mln_u64_t)len);
        assert(memcmp(plain->data, msg, len) == 0);
        mln_RSAESPKCS1V15_free(cipher);
        mln_RSAESPKCS1V15_free(plain);
    }
    printf("  32 randomised roundtrips: OK\n");
}

int main(int argc, char *argv[])
{
    mln_rsa_key_t *pub = NULL, *pri = NULL;
    mln_alloc_t *pool = NULL;
    double t0, t1;
    int rc = 1;

    pub = mln_rsa_key_new();
    pri = mln_rsa_key_new();
    if (pri == NULL || pub == NULL) {
        fprintf(stderr, "new pub/pri key failed\n");
        goto out;
    }

    pool = mln_alloc_init(NULL, 0);
    if (pool == NULL) {
        fprintf(stderr, "alloc pool init failed\n");
        goto out;
    }

    printf("[rsa] generating 128-bit keypair...\n");
    t0 = now_us();
    if (mln_rsa_key_generate(pub, pri, 128) < 0) {
        fprintf(stderr, "key generate failed\n");
        goto out;
    }
    t1 = now_us();
    printf("[rsa] keygen: %.2f ms\n", (t1 - t0) / 1000.0);

    printf("[rsa] encrypt/decrypt roundtrips...\n");
    roundtrip_encrypt_decrypt(pub, pri, "Hello");
    roundtrip_encrypt_decrypt(pub, pri, "Hi");
    printf("[rsa] roundtrips OK\n");

    printf("[rsa] sign/verify with all hash types...\n");
    sign_verify_case(pool, pub, pri, "md5",    M_EMSAPKCS1V15_HASH_MD5);
    sign_verify_case(pool, pub, pri, "sha1",   M_EMSAPKCS1V15_HASH_SHA1);
    sign_verify_case(pool, pub, pri, "sha256", M_EMSAPKCS1V15_HASH_SHA256);
    printf("[rsa] sign/verify OK\n");

    printf("[rsa] stability test...\n");
    stability_test(pub, pri);

    printf("[rsa] microbench:\n");
    bench_rsa(pool, pub, pri);

    printf("[rsa] all tests passed\n");
    rc = 0;
out:
    if (pool != NULL) mln_alloc_destroy(pool);
    if (pub != NULL) mln_rsa_key_free(pub);
    if (pri != NULL) mln_rsa_key_free(pri);
    return rc;
}
