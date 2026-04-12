
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
#include "mln_asn1.h"
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

static void test_primitives(mln_alloc_t *pool)
{
    mln_asn1_enresult_t res;
    mln_u8ptr_t buf = NULL;
    mln_u64_t len = 0;
    int err;
    mln_asn1_deresult_t *de;

    assert(mln_asn1_enresult_init(&res, pool) == M_ASN1_RET_OK);

    /* BOOLEAN */
    assert(mln_asn1_encode_boolean(&res, 0xff) == M_ASN1_RET_OK);
    /* INTEGER */
    {
        mln_u8_t v[] = {0x12, 0x34, 0x56};
        assert(mln_asn1_encode_integer(&res, v, sizeof(v)) == M_ASN1_RET_OK);
    }
    /* ENUMERATED */
    {
        mln_u8_t v[] = {0x01};
        assert(mln_asn1_encode_enumerated(&res, v, sizeof(v)) == M_ASN1_RET_OK);
    }
    /* BIT STRING (16 bits) */
    {
        mln_u8_t v[] = {0xde, 0xad};
        assert(mln_asn1_encode_bitstring(&res, v, 16) == M_ASN1_RET_OK);
    }
    /* OCTET STRING */
    {
        mln_u8_t v[] = {0x01, 0x02, 0x03, 0x04, 0x05};
        assert(mln_asn1_encode_octetstring(&res, v, sizeof(v)) == M_ASN1_RET_OK);
    }
    /* NULL */
    assert(mln_asn1_encode_null(&res) == M_ASN1_RET_OK);
    /* OID (rsaEncryption: 1.2.840.113549.1.1.1 without top prefix) */
    {
        mln_u8_t oid[] = {0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x01, 0x01};
        assert(mln_asn1_encode_object_identifier(&res, oid, sizeof(oid)) == M_ASN1_RET_OK);
    }
    /* UTF8String */
    {
        const char *s = "héllo";
        assert(mln_asn1_encode_utf8string(&res, (mln_u8ptr_t)s, strlen(s)) == M_ASN1_RET_OK);
    }
    /* PrintableString */
    {
        const char *s = "Melon";
        assert(mln_asn1_encode_printablestring(&res, (mln_s8ptr_t)s, strlen(s)) == M_ASN1_RET_OK);
    }
    /* IA5String */
    {
        const char *s = "user@example.com";
        assert(mln_asn1_encode_ia5string(&res, (mln_u8ptr_t)s, strlen(s)) == M_ASN1_RET_OK);
    }
    /* T61String */
    {
        const char *s = "abc";
        assert(mln_asn1_encode_t61string(&res, (mln_u8ptr_t)s, strlen(s)) == M_ASN1_RET_OK);
    }
    /* NumericString */
    {
        const char *s = "123 456";
        assert(mln_asn1_encode_numericstring(&res, (mln_s8ptr_t)s, strlen(s)) == M_ASN1_RET_OK);
    }
    /* BMPString (must be even length: "AB" as UCS-2) */
    {
        mln_u8_t bmp[] = {0x00, 'A', 0x00, 'B'};
        assert(mln_asn1_encode_bmpstring(&res, bmp, sizeof(bmp)) == M_ASN1_RET_OK);
    }
    /* UTCTime */
    assert(mln_asn1_encode_utctime(&res, 1700000000) == M_ASN1_RET_OK);
    /* GeneralizedTime */
    assert(mln_asn1_encode_generalized_time(&res, 1700000000) == M_ASN1_RET_OK);

    /* Wrap everything in a SEQUENCE */
    assert(mln_asn1_encode_sequence(&res) == M_ASN1_RET_OK);
    assert(mln_asn1_enresult_get_content(&res, 0, &buf, &len) == M_ASN1_RET_OK);
    assert(len > 0 && buf != NULL);

    /* Decode and verify top-level structure */
    de = mln_asn1_decode_ref(buf, len, &err, pool);
    assert(de != NULL);
    assert(mln_asn1_deresult_ident_get(de) == M_ASN1_ID_SEQUENCE);
    assert(mln_asn1_deresult_content_num(de) == 15);
    {
        mln_asn1_deresult_t *el = mln_asn1_deresult_content_get(de, 2);
        assert(mln_asn1_deresult_ident_get(el) == M_ASN1_ID_ENUMERATED);
    }
    mln_asn1_deresult_free(de);
    mln_asn1_enresult_destroy(&res);
    printf("[asn1] primitives encode/decode OK (15 fields in SEQUENCE)\n");
}

static void test_set_sort(mln_alloc_t *pool)
{
    mln_asn1_enresult_t res;
    mln_u8ptr_t buf = NULL;
    mln_u64_t len = 0;
    int err;
    mln_asn1_deresult_t *de;

    assert(mln_asn1_enresult_init(&res, pool) == M_ASN1_RET_OK);
    {
        mln_u8_t c[] = {0x30};
        mln_u8_t a[] = {0x10};
        mln_u8_t b[] = {0x20};
        /* out-of-order on purpose */
        assert(mln_asn1_encode_integer(&res, c, 1) == M_ASN1_RET_OK);
        assert(mln_asn1_encode_integer(&res, a, 1) == M_ASN1_RET_OK);
        assert(mln_asn1_encode_integer(&res, b, 1) == M_ASN1_RET_OK);
    }
    assert(mln_asn1_encode_setof(&res) == M_ASN1_RET_OK);
    assert(mln_asn1_enresult_get_content(&res, 0, &buf, &len) == M_ASN1_RET_OK);

    de = mln_asn1_decode_ref(buf, len, &err, pool);
    assert(de != NULL);
    assert(mln_asn1_deresult_ident_get(de) == M_ASN1_ID_SET);
    assert(mln_asn1_deresult_content_num(de) == 3);
    /* elements should be sorted now */
    {
        mln_asn1_deresult_t *e;
        mln_u8ptr_t cb;
        e = mln_asn1_deresult_content_get(de, 0);
        e = mln_asn1_deresult_content_get(e, 0);
        cb = mln_asn1_deresult_code_get(e);
        assert(cb[0] == 0x10);
        e = mln_asn1_deresult_content_get(de, 1);
        e = mln_asn1_deresult_content_get(e, 0);
        cb = mln_asn1_deresult_code_get(e);
        assert(cb[0] == 0x20);
        e = mln_asn1_deresult_content_get(de, 2);
        e = mln_asn1_deresult_content_get(e, 0);
        cb = mln_asn1_deresult_code_get(e);
        assert(cb[0] == 0x30);
    }
    mln_asn1_deresult_free(de);
    mln_asn1_enresult_destroy(&res);
    printf("[asn1] SET OF sorted encode/decode OK\n");
}

static void test_long_length(mln_alloc_t *pool)
{
    /* encode a large OCTET STRING to force multi-byte length form */
    mln_asn1_enresult_t res;
    mln_u8ptr_t buf = NULL, payload;
    mln_u64_t len = 0;
    int err, i;
    mln_asn1_deresult_t *de;
    const int N = 1000;

    payload = (mln_u8ptr_t)malloc(N);
    for (i = 0; i < N; ++i) payload[i] = (mln_u8_t)(i & 0xff);

    assert(mln_asn1_enresult_init(&res, pool) == M_ASN1_RET_OK);
    assert(mln_asn1_encode_octetstring(&res, payload, N) == M_ASN1_RET_OK);
    assert(mln_asn1_enresult_get_content(&res, 0, &buf, &len) == M_ASN1_RET_OK);

    de = mln_asn1_decode_ref(buf, len, &err, pool);
    assert(de != NULL);
    assert(mln_asn1_deresult_ident_get(de) == M_ASN1_ID_OCTET_STRING);
    assert(mln_asn1_deresult_content_num(de) == 1);
    {
        mln_asn1_deresult_t *e = mln_asn1_deresult_content_get(de, 0);
        assert(mln_asn1_deresult_code_length_get(e) == (mln_u64_t)N);
        assert(memcmp(mln_asn1_deresult_code_get(e), payload, N) == 0);
    }
    mln_asn1_deresult_free(de);
    mln_asn1_enresult_destroy(&res);
    free(payload);
    printf("[asn1] long-length encode/decode OK (N=%d)\n", N);
}

static void test_implicit_tag(mln_alloc_t *pool)
{
    mln_asn1_enresult_t res;
    mln_u8ptr_t buf = NULL;
    mln_u64_t len = 0;
    int err;
    mln_asn1_deresult_t *de;

    assert(mln_asn1_enresult_init(&res, pool) == M_ASN1_RET_OK);
    {
        mln_u8_t v[] = {0x42};
        assert(mln_asn1_encode_integer(&res, v, sizeof(v)) == M_ASN1_RET_OK);
    }
    /* rewrite tag to context-specific [5] */
    assert(mln_asn1_encode_implicit(&res, 5, 0) == M_ASN1_RET_OK);
    assert(mln_asn1_enresult_get_content(&res, 0, &buf, &len) == M_ASN1_RET_OK);

    de = mln_asn1_decode_ref(buf, len, &err, pool);
    assert(de != NULL);
    assert((mln_asn1_deresult_class_get(de)) == M_ASN1_CLASS_CONTEXT_SPECIFIC);
    assert(mln_asn1_deresult_ident_get(de) == 5);
    mln_asn1_deresult_free(de);
    mln_asn1_enresult_destroy(&res);
    printf("[asn1] IMPLICIT tag rewrite OK\n");
}

static void test_malformed(mln_alloc_t *pool)
{
    int err;
    mln_asn1_deresult_t *de;

    /* truncated length */
    {
        mln_u8_t bad[] = {0x30, 0x82};
        de = mln_asn1_decode(bad, sizeof(bad), &err, pool);
        assert(de == NULL);
        assert(err == M_ASN1_RET_INCOMPLETE);
    }
    /* declared length > available */
    {
        mln_u8_t bad[] = {0x04, 0x10, 0x01, 0x02};
        de = mln_asn1_decode(bad, sizeof(bad), &err, pool);
        assert(de == NULL);
        assert(err == M_ASN1_RET_INCOMPLETE);
    }
    /* indefinite length (unsupported) */
    {
        mln_u8_t bad[] = {0x30, 0x80, 0x00, 0x00};
        de = mln_asn1_decode(bad, sizeof(bad), &err, pool);
        assert(de == NULL);
        assert(err == M_ASN1_RET_ERROR);
    }
    printf("[asn1] malformed-input rejection OK\n");
}

static void bench_asn1(mln_alloc_t *pool)
{
    /* Benchmark: repeated encode+decode of a moderately-sized structure. */
    const int N = 5000;
    int i;
    double t0, t1;
    mln_u8_t octets[64];
    for (i = 0; i < 64; ++i) octets[i] = (mln_u8_t)i;

    t0 = now_us();
    for (i = 0; i < N; ++i) {
        mln_asn1_enresult_t res;
        mln_u8ptr_t buf;
        mln_u64_t len;
        int err;
        mln_asn1_deresult_t *de;

        mln_asn1_enresult_init(&res, pool);
        mln_asn1_encode_integer(&res, octets, 4);
        mln_asn1_encode_octetstring(&res, octets, 64);
        mln_asn1_encode_null(&res);
        mln_asn1_encode_sequence(&res);
        mln_asn1_enresult_get_content(&res, 0, &buf, &len);

        de = mln_asn1_decode_ref(buf, len, &err, pool);
        assert(de != NULL);
        assert(mln_asn1_deresult_content_num(de) == 3);
        mln_asn1_deresult_free(de);
        mln_asn1_enresult_destroy(&res);
    }
    t1 = now_us();
    printf("[asn1] encode+decode bench: %8.3f us/op (%d iters)\n", (t1 - t0) / N, N);
}

int main(int argc, char *argv[])
{
    mln_alloc_t *pool = mln_alloc_init(NULL, 0);
    assert(pool != NULL);

    test_primitives(pool);
    test_set_sort(pool);
    test_long_length(pool);
    test_implicit_tag(pool);
    test_malformed(pool);
    bench_asn1(pool);

    mln_alloc_destroy(pool);
    printf("[asn1] all tests passed\n");
    return 0;
}
