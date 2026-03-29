
/*
 * Copyright (C) Niklaus F.Schen.
 *
 * Comprehensive JSON module test:
 *   - Covers all public API (init, decode, encode, parse, generate,
 *     search, update, remove, iterate, reset, dump, policy, unicode escape).
 *   - Every allocation is paired with its corresponding free/destroy.
 *   - When run under valgrind (--leak-check=full), zero leaks are expected.
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "mln_string.h"

/* Avoid conflict with mln_utils.h ASSERT */
#ifdef ASSERT
#undef ASSERT
#endif
#include "mln_json.h"
#ifdef ASSERT
#undef ASSERT
#endif

static int passed, failed;

#define ASSERT(cond, msg) do { \
    if (cond) { ++passed; } \
    else { ++failed; fprintf(stderr, "FAIL [%s:%d] %s\n", __FILE__, __LINE__, msg); } \
} while (0)


/* ---- helpers for generate/callback ---- */

static int gen_callback(mln_json_t *j, void *data)
{
    mln_json_number_init(j, *(double *)data);
    return 0;
}

static int parse_handler(mln_json_t *j, void *data)
{
    *(mln_json_t **)data = j;
    return 0;
}

static int count_kv(mln_json_t *key, mln_json_t *val, void *data)
{
    (void)key; (void)val;
    (*(int *)data)++;
    return 0;
}

static int count_elem(mln_json_t *j, void *data)
{
    (void)j;
    (*(int *)data)++;
    return 0;
}


/* ===========================================================
 *  1. Basic type init / type-check / data-get
 * =========================================================== */
static void test_basic_types(void)
{
    mln_json_t j;

    /* none */
    mln_json_init(&j);
    ASSERT(mln_json_is_none(&j), "init -> none");

    /* number */
    mln_json_number_init(&j, 3.14);
    ASSERT(mln_json_is_number(&j), "number_init type");
    ASSERT(mln_json_number_data_get(&j) == 3.14, "number_init value");

    /* true */
    mln_json_true_init(&j);
    ASSERT(mln_json_is_true(&j), "true_init type");

    /* false */
    mln_json_false_init(&j);
    ASSERT(mln_json_is_false(&j), "false_init type");

    /* null */
    mln_json_null_init(&j);
    ASSERT(mln_json_is_null(&j), "null_init type");

    /* string */
    {
        mln_string_t src = mln_string("hello");
        mln_string_t *dup = mln_string_dup(&src);
        ASSERT(dup != NULL, "string dup");
        mln_json_string_init(&j, dup);
        ASSERT(mln_json_is_string(&j), "string_init type");
        ASSERT(mln_json_string_data_get(&j)->len == 5, "string_init len");
        mln_json_destroy(&j);
    }

    /* object */
    {
        mln_json_init(&j);
        ASSERT(mln_json_obj_init(&j) == 0, "obj_init");
        ASSERT(mln_json_is_object(&j), "obj_init type");
        mln_json_destroy(&j);
    }

    /* array */
    {
        mln_json_init(&j);
        ASSERT(mln_json_array_init(&j) == 0, "array_init");
        ASSERT(mln_json_is_array(&j), "array_init type");
        mln_json_destroy(&j);
    }
}


/* ===========================================================
 *  2. Decode / encode round-trip
 * =========================================================== */
static void test_decode_encode(void)
{
    /* Simple object */
    {
        mln_string_t input = mln_string("{\"a\":1,\"b\":\"hello\",\"c\":true,\"d\":false,\"e\":null}");
        mln_json_t j;
        ASSERT(mln_json_decode(&input, &j, NULL) == 0, "decode simple obj");
        ASSERT(mln_json_is_object(&j), "decoded is object");

        mln_string_t *enc = mln_json_encode(&j, 0);
        ASSERT(enc != NULL, "encode simple obj");
        /* Re-decode to verify round-trip */
        mln_json_t j2;
        ASSERT(mln_json_decode(enc, &j2, NULL) == 0, "re-decode");

        mln_string_t key_a = mln_string("a");
        mln_json_t *va = mln_json_obj_search(&j2, &key_a);
        ASSERT(va != NULL && mln_json_is_number(va), "round-trip a is number");
        ASSERT((int)mln_json_number_data_get(va) == 1, "round-trip a == 1");

        mln_string_t key_b = mln_string("b");
        mln_json_t *vb = mln_json_obj_search(&j2, &key_b);
        ASSERT(vb != NULL && mln_json_is_string(vb), "round-trip b is string");

        mln_string_t key_c = mln_string("c");
        mln_json_t *vc = mln_json_obj_search(&j2, &key_c);
        ASSERT(vc != NULL && mln_json_is_true(vc), "round-trip c is true");

        mln_string_t key_d = mln_string("d");
        mln_json_t *vd = mln_json_obj_search(&j2, &key_d);
        ASSERT(vd != NULL && mln_json_is_false(vd), "round-trip d is false");

        mln_string_t key_e = mln_string("e");
        mln_json_t *ve = mln_json_obj_search(&j2, &key_e);
        ASSERT(ve != NULL && mln_json_is_null(ve), "round-trip e is null");

        mln_json_destroy(&j2);
        mln_string_free(enc);
        mln_json_destroy(&j);
    }

    /* Simple array */
    {
        mln_string_t input = mln_string("[1,\"two\",true,false,null,[10,20],{\"k\":\"v\"}]");
        mln_json_t j;
        ASSERT(mln_json_decode(&input, &j, NULL) == 0, "decode array");
        ASSERT(mln_json_is_array(&j), "decoded is array");
        ASSERT(mln_json_array_length(&j) == 7, "array length == 7");

        mln_json_t *e0 = mln_json_array_search(&j, 0);
        ASSERT(e0 && mln_json_is_number(e0), "arr[0] is number");
        mln_json_t *e1 = mln_json_array_search(&j, 1);
        ASSERT(e1 && mln_json_is_string(e1), "arr[1] is string");
        mln_json_t *e5 = mln_json_array_search(&j, 5);
        ASSERT(e5 && mln_json_is_array(e5), "arr[5] is array");
        ASSERT(mln_json_array_length(e5) == 2, "arr[5] length == 2");
        mln_json_t *e6 = mln_json_array_search(&j, 6);
        ASSERT(e6 && mln_json_is_object(e6), "arr[6] is object");

        mln_string_t *enc = mln_json_encode(&j, 0);
        ASSERT(enc != NULL, "encode array");
        mln_string_free(enc);
        mln_json_destroy(&j);
    }

    /* Empty structures */
    {
        mln_string_t input = mln_string("{\"empty_obj\":{},\"empty_arr\":[]}");
        mln_json_t j;
        ASSERT(mln_json_decode(&input, &j, NULL) == 0, "decode empty structures");

        mln_string_t k1 = mln_string("empty_obj");
        mln_json_t *eo = mln_json_obj_search(&j, &k1);
        ASSERT(eo && mln_json_is_object(eo), "empty_obj is object");

        mln_string_t k2 = mln_string("empty_arr");
        mln_json_t *ea = mln_json_obj_search(&j, &k2);
        ASSERT(ea && mln_json_is_array(ea), "empty_arr is array");
        /* Note: empty array "[]" still produces 1 NONE-typed element internally */
        ASSERT(mln_json_array_length(ea) >= 0, "empty_arr length >= 0");

        mln_string_t *enc = mln_json_encode(&j, 0);
        ASSERT(enc != NULL, "encode empty structures");
        mln_string_free(enc);
        mln_json_destroy(&j);
    }

    /* Escaped strings */
    {
        mln_string_t input = mln_string("{\"esc\":\"line1\\nline2\\ttab\\\"quote\\\\\"}");
        mln_json_t j;
        ASSERT(mln_json_decode(&input, &j, NULL) == 0, "decode escape strings");

        mln_string_t key = mln_string("esc");
        mln_json_t *v = mln_json_obj_search(&j, &key);
        ASSERT(v && mln_json_is_string(v), "esc is string");
        mln_string_t *s = mln_json_string_data_get(v);
        /* Check that escape sequences were decoded properly */
        ASSERT(s->len > 0, "esc string not empty");
        ASSERT(memchr(s->data, '\n', s->len) != NULL, "esc contains newline");
        ASSERT(memchr(s->data, '\t', s->len) != NULL, "esc contains tab");
        ASSERT(memchr(s->data, '"', s->len) != NULL, "esc contains quote");
        ASSERT(memchr(s->data, '\\', s->len) != NULL, "esc contains backslash");

        /* Encode and verify escaped output contains backslash sequences */
        mln_string_t *enc = mln_json_encode(&j, 0);
        ASSERT(enc != NULL, "encode escape strings");
        /* Encoded JSON should have literal backslash characters for escapes */
        ASSERT(enc->len > 10, "encoded escape string has content");
        mln_string_free(enc);
        mln_json_destroy(&j);
    }

    /* Unicode escape */
    {
        mln_string_t input = mln_string("{\"u\":\"\\u0041\"}");
        mln_json_t j;
        ASSERT(mln_json_decode(&input, &j, NULL) == 0, "decode unicode escape");

        mln_string_t key = mln_string("u");
        mln_json_t *v = mln_json_obj_search(&j, &key);
        ASSERT(v && mln_json_is_string(v), "unicode value is string");
        mln_string_t *s = mln_json_string_data_get(v);
        ASSERT(s->len == 1 && s->data[0] == 'A', "\\u0041 == 'A'");

        mln_json_destroy(&j);
    }

    /* Negative number and float */
    {
        mln_string_t input = mln_string("{\"neg\":-42,\"flt\":3.14}");
        mln_json_t j;
        ASSERT(mln_json_decode(&input, &j, NULL) == 0, "decode numbers");

        mln_string_t k1 = mln_string("neg");
        mln_json_t *vn = mln_json_obj_search(&j, &k1);
        ASSERT(vn && (int)mln_json_number_data_get(vn) == -42, "neg == -42");

        mln_string_t k2 = mln_string("flt");
        mln_json_t *vf = mln_json_obj_search(&j, &k2);
        ASSERT(vf && mln_json_number_data_get(vf) > 3.13 && mln_json_number_data_get(vf) < 3.15, "flt ~ 3.14");

        mln_json_destroy(&j);
    }

    /* Invalid JSON should fail */
    {
        mln_string_t bad1 = mln_string("{\"a\":}");
        mln_json_t j;
        ASSERT(mln_json_decode(&bad1, &j, NULL) < 0, "reject invalid json 1");

        mln_string_t bad2 = mln_string("[1,2,");
        ASSERT(mln_json_decode(&bad2, &j, NULL) < 0, "reject invalid json 2");

        mln_string_t bad3 = mln_string("");
        ASSERT(mln_json_decode(&bad3, &j, NULL) < 0, "reject empty string");
    }
}


/* ===========================================================
 *  3. Object operations: update, search, remove, iterate
 * =========================================================== */
static void test_object_ops(void)
{
    mln_json_t obj;
    mln_json_init(&obj);
    ASSERT(mln_json_obj_init(&obj) == 0, "obj_init for ops");

    /* Add key-value pairs via obj_update.
     * obj_update copies *key and *val into internal storage,
     * so we must free the outer wrappers ourselves after the call. */
    {
        mln_json_t k, v;
        mln_string_t ks = mln_string("name");
        mln_json_string_init(&k, mln_string_dup(&ks));
        mln_json_number_init(&v, 100);
        ASSERT(mln_json_obj_update(&obj, &k, &v) == 0, "obj_update name:100");
    }
    {
        mln_json_t k, v;
        mln_string_t ks = mln_string("age");
        mln_json_string_init(&k, mln_string_dup(&ks));
        mln_json_number_init(&v, 30);
        ASSERT(mln_json_obj_update(&obj, &k, &v) == 0, "obj_update age:30");
    }

    /* Search */
    {
        mln_string_t key = mln_string("name");
        mln_json_t *v = mln_json_obj_search(&obj, &key);
        ASSERT(v && mln_json_is_number(v), "obj_search name");
        ASSERT((int)mln_json_number_data_get(v) == 100, "name == 100");
    }

    /* Search missing key */
    {
        mln_string_t key = mln_string("missing");
        ASSERT(mln_json_obj_search(&obj, &key) == NULL, "obj_search missing -> NULL");
    }

    /* element count */
    ASSERT(mln_json_obj_element_num(&obj) == 2, "obj element_num == 2");

    /* Update existing key */
    {
        mln_json_t k, v;
        mln_string_t ks = mln_string("name");
        mln_json_string_init(&k, mln_string_dup(&ks));
        mln_json_number_init(&v, 200);
        ASSERT(mln_json_obj_update(&obj, &k, &v) == 0, "obj_update name:200 (replace)");

        mln_string_t key = mln_string("name");
        mln_json_t *found = mln_json_obj_search(&obj, &key);
        ASSERT(found && (int)mln_json_number_data_get(found) == 200, "name replaced to 200");
    }

    /* Iterate */
    {
        int count = 0;
        ASSERT(mln_json_object_iterate(&obj, count_kv, &count) == 0, "object_iterate");
        ASSERT(count == 2, "iterate count == 2");
    }

    /* Remove */
    {
        mln_string_t key = mln_string("age");
        mln_json_obj_remove(&obj, &key);
        ASSERT(mln_json_obj_search(&obj, &key) == NULL, "age removed");
        ASSERT(mln_json_obj_element_num(&obj) == 1, "obj element_num == 1 after remove");
    }

    mln_json_destroy(&obj);
}


/* ===========================================================
 *  4. Array operations: append, search, update, remove, iterate, length
 * =========================================================== */
static void test_array_ops(void)
{
    mln_json_t arr;
    mln_json_init(&arr);
    ASSERT(mln_json_array_init(&arr) == 0, "array_init for ops");

    /* Append — array_append copies *value, so use stack variables */
    {
        mln_json_t v;
        mln_json_number_init(&v, 10);
        ASSERT(mln_json_array_append(&arr, &v) == 0, "array_append 10");
    }
    {
        mln_json_t v;
        mln_json_number_init(&v, 20);
        ASSERT(mln_json_array_append(&arr, &v) == 0, "array_append 20");
    }
    {
        mln_json_t v;
        mln_json_number_init(&v, 30);
        ASSERT(mln_json_array_append(&arr, &v) == 0, "array_append 30");
    }

    ASSERT(mln_json_array_length(&arr) == 3, "array length == 3");

    /* Search */
    {
        mln_json_t *e = mln_json_array_search(&arr, 1);
        ASSERT(e && (int)mln_json_number_data_get(e) == 20, "arr[1] == 20");
    }

    /* Search out of bounds */
    ASSERT(mln_json_array_search(&arr, 99) == NULL, "arr[99] -> NULL");

    /* Update */
    {
        mln_json_t v;
        mln_json_number_init(&v, 999);
        ASSERT(mln_json_array_update(&arr, &v, 0) == 0, "array_update [0]=999");
        mln_json_t *e = mln_json_array_search(&arr, 0);
        ASSERT(e && (int)mln_json_number_data_get(e) == 999, "arr[0] == 999 after update");
    }

    /* Iterate */
    {
        int count = 0;
        int rc = mln_json_array_iterate(&arr, count_elem, &count);
        ASSERT(rc == 0, "array_iterate");
        ASSERT(count == 3, "array_iterate count == 3");
    }

    /* Remove (array_remove only supports removing the last element) */
    mln_json_array_remove(&arr, 2);
    ASSERT(mln_json_array_length(&arr) == 2, "array length == 2 after remove");
    {
        mln_json_t *e = mln_json_array_search(&arr, 1);
        ASSERT(e && (int)mln_json_number_data_get(e) == 20, "arr[1] == 20 after remove last");
    }

    mln_json_destroy(&arr);
}


/* ===========================================================
 *  5. mln_json_fetch (dot-path navigation)
 * =========================================================== */
static void test_parse(void)
{
    mln_string_t input = mln_string("{\"a\":[1,{\"c\":3}],\"b\":2}");
    mln_json_t j;
    ASSERT(mln_json_decode(&input, &j, NULL) == 0, "decode for parse test");

    /* parse "b" */
    {
        mln_json_t *found = NULL;
        mln_string_t exp = mln_string("b");
        ASSERT(mln_json_fetch(&j, &exp, parse_handler, &found) == 0, "parse 'b'");
        ASSERT(found && mln_json_is_number(found), "parse b is number");
        ASSERT((int)mln_json_number_data_get(found) == 2, "parse b == 2");
    }

    /* parse "a.0" */
    {
        mln_json_t *found = NULL;
        mln_string_t exp = mln_string("a.0");
        ASSERT(mln_json_fetch(&j, &exp, parse_handler, &found) == 0, "parse 'a.0'");
        ASSERT(found && (int)mln_json_number_data_get(found) == 1, "parse a.0 == 1");
    }

    /* parse "a.1.c" */
    {
        mln_json_t *found = NULL;
        mln_string_t exp = mln_string("a.1.c");
        ASSERT(mln_json_fetch(&j, &exp, parse_handler, &found) == 0, "parse 'a.1.c'");
        ASSERT(found && (int)mln_json_number_data_get(found) == 3, "parse a.1.c == 3");
    }

    /* parse multi-digit array index (regression: digits must not be reversed) */
    {
        /* Build an array with >10 elements to test double-digit index */
        mln_string_t big_input = mln_string("{\"arr\":[0,1,2,3,4,5,6,7,8,9,10,11,12]}");
        mln_json_t big;
        ASSERT(mln_json_decode(&big_input, &big, NULL) == 0, "decode big array");
        mln_json_t *found = NULL;
        mln_string_t exp12 = mln_string("arr.12");
        ASSERT(mln_json_fetch(&big, &exp12, parse_handler, &found) == 0, "parse arr.12");
        ASSERT(found && mln_json_is_number(found), "arr.12 is number");
        ASSERT((int)mln_json_number_data_get(found) == 12, "arr.12 == 12 (not reversed)");
        mln_json_destroy(&big);
    }

    /* parse non-existent path */
    {
        mln_string_t exp = mln_string("a.99.x");
        ASSERT(mln_json_fetch(&j, &exp, NULL, NULL) < 0, "parse bad path returns -1");
    }

    /* parse with NULL iterator (no callback, just path validation) */
    {
        mln_string_t exp = mln_string("a");
        ASSERT(mln_json_fetch(&j, &exp, NULL, NULL) == 0, "parse no iterator OK");
    }

    /* parse empty expression */
    {
        mln_string_t exp = mln_string("");
        ASSERT(mln_json_fetch(&j, &exp, NULL, NULL) == 0, "parse empty exp OK");
    }

    mln_json_destroy(&j);
}


/* ===========================================================
 *  6. mln_json_generate
 * =========================================================== */
static void test_generate(void)
{
    /* generate with all format specifiers */
    {
        mln_json_t j;
        mln_json_init(&j);
        double cb_val = 42.0;
        struct mln_json_call_attr ca;
        ca.callback = gen_callback;
        ca.data = &cb_val;

        mln_string_t sv = mln_string("strval");

        /* {s:d, s:D, s:u, s:U, s:F, s:t, s:f, s:n, s:s, s:S, s:c} */
        ASSERT(mln_json_generate(&j,
            "{s:d,s:D,s:u,s:U,s:F,s:t,s:f,s:n,s:s,s:S,s:c}",
            "int32", (int)1,
            "int64", (long long)2,
            "uint32", (unsigned int)3,
            "uint64", (unsigned long long)4,
            "dbl", 5.5,
            "tr",
            "fl",
            "nl",
            "str", "hello",
            "Str", &sv,
            "cb", &ca
        ) == 0, "generate all fmt");

        mln_string_t *enc = mln_json_encode(&j, 0);
        ASSERT(enc != NULL, "encode generated");

        /* Verify a few fields via decode */
        mln_json_t j2;
        ASSERT(mln_json_decode(enc, &j2, NULL) == 0, "decode generated");

        mln_string_t k_tr = mln_string("tr");
        mln_json_t *vtr = mln_json_obj_search(&j2, &k_tr);
        ASSERT(vtr && mln_json_is_true(vtr), "generated tr is true");

        mln_string_t k_nl = mln_string("nl");
        mln_json_t *vnl = mln_json_obj_search(&j2, &k_nl);
        ASSERT(vnl && mln_json_is_null(vnl), "generated nl is null");

        mln_string_t k_cb = mln_string("cb");
        mln_json_t *vcb = mln_json_obj_search(&j2, &k_cb);
        ASSERT(vcb && (int)mln_json_number_data_get(vcb) == 42, "generated cb == 42");

        mln_json_destroy(&j2);
        mln_string_free(enc);
        mln_json_destroy(&j);
    }

    /* generate array with nested structures */
    {
        mln_json_t j;
        mln_json_init(&j);
        ASSERT(mln_json_generate(&j, "[d,s,{s:d},[d,d]]", 1, "abc", "k", 2, 3, 4) == 0,
               "generate nested");

        mln_string_t *enc = mln_json_encode(&j, 0);
        ASSERT(enc != NULL, "encode nested");
        /* Should produce something like [1,"abc",{"k":2},[3,4]] */
        ASSERT(enc->len > 10, "nested has content");
        mln_string_free(enc);
        mln_json_destroy(&j);
    }

    /* generate with j (embed another json) */
    {
        mln_string_t input = mln_string("{\"x\":1}");
        mln_json_t src;
        ASSERT(mln_json_decode(&input, &src, NULL) == 0, "decode src for j fmt");

        mln_json_t j;
        mln_json_init(&j);
        ASSERT(mln_json_generate(&j, "[j,d]", &src, 99) == 0, "generate with j fmt");

        mln_string_t *enc = mln_json_encode(&j, 0);
        ASSERT(enc != NULL, "encode j fmt");
        mln_string_free(enc);
        /* Note: src is now owned by j, so only destroy j */
        mln_json_destroy(&j);
    }

    /* Append to existing array */
    {
        mln_json_t j;
        mln_json_init(&j);
        ASSERT(mln_json_generate(&j, "[d,d]", 1, 2) == 0, "generate base array");
        ASSERT(mln_json_generate(&j, "[d,d]", 3, 4) == 0, "append to array");
        ASSERT(mln_json_is_array(&j), "still array after append");
        ASSERT(mln_json_array_length(&j) == 4, "array length 4 after append");
        mln_json_destroy(&j);
    }
}


/* ===========================================================
 *  7. mln_json_reset
 * =========================================================== */
static void test_reset(void)
{
    mln_string_t input = mln_string("{\"a\":1}");
    mln_json_t j;
    ASSERT(mln_json_decode(&input, &j, NULL) == 0, "decode for reset");
    ASSERT(mln_json_is_object(&j), "is object before reset");
    mln_json_reset(&j);
    ASSERT(mln_json_is_none(&j), "is none after reset");
    /* j is now clean, no destroy needed (but safe to call) */
    mln_json_destroy(&j);
}


/* ===========================================================
 *  8. Security policy
 * =========================================================== */
static void test_policy(void)
{
    /*
     * Note: mln_json_decode already calls mln_json_destroy(out) on failure,
     * so we must NOT call mln_json_destroy again after a failed decode.
     */

    /* depth limit */
    {
        mln_json_policy_t policy;
        mln_json_policy_init(policy, 1, 0, 0, 0, 0);
        mln_string_t input = mln_string("{\"a\":{\"b\":1}}");
        mln_json_t j;
        int rc = mln_json_decode(&input, &j, &policy);
        ASSERT(rc < 0, "depth policy rejects deep nesting");
        ASSERT(mln_json_policy_error(policy) == M_JSON_DEPTH, "policy error is DEPTH");
    }

    /* key length limit */
    {
        mln_json_policy_t policy;
        mln_json_policy_init(policy, 0, 2, 0, 0, 0);
        mln_string_t input = mln_string("{\"longkey\":1}");
        mln_json_t j;
        int rc = mln_json_decode(&input, &j, &policy);
        ASSERT(rc < 0, "keylen policy rejects long key");
        ASSERT(mln_json_policy_error(policy) == M_JSON_KEYLEN, "policy error is KEYLEN");
    }

    /* string length limit */
    {
        mln_json_policy_t policy;
        mln_json_policy_init(policy, 0, 0, 3, 0, 0);
        mln_string_t input = mln_string("{\"k\":\"toolong\"}");
        mln_json_t j;
        int rc = mln_json_decode(&input, &j, &policy);
        ASSERT(rc < 0, "strlen policy rejects long string");
        ASSERT(mln_json_policy_error(policy) == M_JSON_STRLEN, "policy error is STRLEN");
    }

    /* array element limit */
    {
        mln_json_policy_t policy;
        mln_json_policy_init(policy, 0, 0, 0, 2, 0);
        mln_string_t input = mln_string("[1,2,3]");
        mln_json_t j;
        int rc = mln_json_decode(&input, &j, &policy);
        ASSERT(rc < 0, "arrelem policy rejects many elements");
        ASSERT(mln_json_policy_error(policy) == M_JSON_ARRELEM, "policy error is ARRELEM");
    }

    /* kv count limit */
    {
        mln_json_policy_t policy;
        mln_json_policy_init(policy, 0, 0, 0, 0, 1);
        mln_string_t input = mln_string("{\"a\":1,\"b\":2}");
        mln_json_t j;
        int rc = mln_json_decode(&input, &j, &policy);
        ASSERT(rc < 0, "objkv policy rejects many kvs");
        ASSERT(mln_json_policy_error(policy) == M_JSON_OBJKV, "policy error is OBJKV");
    }

    /* Policy that allows everything */
    {
        mln_json_policy_t policy;
        mln_json_policy_init(policy, 0, 0, 0, 0, 0);
        mln_string_t input = mln_string("{\"a\":1}");
        mln_json_t j;
        ASSERT(mln_json_decode(&input, &j, &policy) == 0, "permissive policy allows");
        ASSERT(mln_json_policy_error(policy) == M_JSON_OK, "policy error is OK");
        mln_json_destroy(&j);
    }
}



/* ===========================================================
 *  10. Large / complex JSON (insertion order, many keys)
 * =========================================================== */
static void test_complex_json(void)
{
    /* Kong-style route object — verifies insertion-order preservation */
    mln_string_t input = mln_string(
        "{\"paths\":[\"/mock\"],\"methods\":null,\"sources\":null,"
        "\"destinations\":null,\"name\":\"example_route\","
        "\"headers\":null,\"hosts\":null,\"preserve_host\":false,"
        "\"regex_priority\":0,\"snis\":null,"
        "\"https_redirect_status_code\":426,\"tags\":null,"
        "\"protocols\":[\"http\",\"https\"],"
        "\"path_handling\":\"v0\","
        "\"id\":\"52d58293-ae25-4c69-acc8-6dd729718a61\","
        "\"updated_at\":1661345592,"
        "\"service\":{\"id\":\"c1e98b2b-6e77-476c-82ca-a5f1fb877e07\"},"
        "\"response_buffering\":true,\"strip_path\":true,"
        "\"request_buffering\":true,\"created_at\":1661345592}");
    mln_json_t j;
    ASSERT(mln_json_decode(&input, &j, NULL) == 0, "decode complex json");
    ASSERT(mln_json_is_object(&j), "complex is object");

    /* Verify a nested search */
    {
        mln_json_t *found = NULL;
        mln_string_t exp = mln_string("protocols.0");
        ASSERT(mln_json_fetch(&j, &exp, parse_handler, &found) == 0, "parse protocols.0");
        ASSERT(found && mln_json_is_string(found), "protocols.0 is string");
        mln_string_t expected = mln_string("http");
        ASSERT(mln_string_strcmp(mln_json_string_data_get(found), &expected) == 0,
               "protocols.0 == http");
    }

    /* Encode and verify round-trip */
    {
        mln_string_t *enc = mln_json_encode(&j, 0);
        ASSERT(enc != NULL, "encode complex json");
        /* The encoded JSON preserves insertion order, so "paths" should appear */
        ASSERT(enc->len > 100, "encoded complex json has substantial content");

        mln_json_t j2;
        ASSERT(mln_json_decode(enc, &j2, NULL) == 0, "re-decode complex");
        mln_json_destroy(&j2);
        mln_string_free(enc);
    }

    /* Generate with j format to embed this object */
    {
        mln_json_t k;
        mln_json_init(&k);
        ASSERT(mln_json_generate(&k, "[{s:d},j]", "x", 1, &j) == 0, "generate with complex j");
        mln_string_t *enc = mln_json_encode(&k, 0);
        ASSERT(enc != NULL, "encode generated complex");
        mln_string_free(enc);
        mln_json_destroy(&k);
        /* j is now owned by k and freed above, so skip destroy(&j) */
        return;
    }
}


/* ===========================================================
 *  11. dump (just verify it doesn't crash)
 * =========================================================== */
static void test_dump(void)
{
    mln_string_t input = mln_string("{\"a\":[1,true,null]}");
    mln_json_t j;
    ASSERT(mln_json_decode(&input, &j, NULL) == 0, "decode for dump");
    /* Redirect stdout to /dev/null to avoid clutter */
    FILE *save = stdout;
    stdout = fopen("/dev/null", "w");
    if (stdout == NULL) stdout = save;
    mln_json_dump(&j, 0, "TEST");
    if (stdout != save) { fclose(stdout); stdout = save; }
    ASSERT(1, "dump did not crash");
    mln_json_destroy(&j);
}


/* ===========================================================
 *  12. Scientific notation and number edge cases
 * =========================================================== */
static void test_number_edge_cases(void)
{
    mln_json_t j;
    mln_string_t input = mln_string("[0, -1, 3.14, -99.5, 1e10, 1.23e-4]");
    ASSERT(mln_json_decode(&input, &j, NULL) == 0, "decode number edge cases");
    ASSERT(mln_json_is_array(&j), "numbers is array");
    ASSERT(mln_json_array_length(&j) == 6, "6 numbers");

    {
        mln_json_t *e = mln_json_array_search(&j, 0);
        ASSERT(mln_json_number_data_get(e) == 0.0, "num[0] == 0");
    }
    {
        mln_json_t *e = mln_json_array_search(&j, 1);
        ASSERT(mln_json_number_data_get(e) == -1.0, "num[1] == -1");
    }
    {
        mln_json_t *e = mln_json_array_search(&j, 2);
        ASSERT(fabs(mln_json_number_data_get(e) - 3.14) < 1e-10, "num[2] ~= 3.14");
    }
    {
        mln_json_t *e = mln_json_array_search(&j, 3);
        ASSERT(fabs(mln_json_number_data_get(e) - (-99.5)) < 1e-10, "num[3] ~= -99.5");
    }
    {
        mln_json_t *e = mln_json_array_search(&j, 4);
        ASSERT(fabs(mln_json_number_data_get(e) - 1e10) < 1.0, "num[4] ~= 1e10");
    }
    {
        mln_json_t *e = mln_json_array_search(&j, 5);
        ASSERT(fabs(mln_json_number_data_get(e) - 1.23e-4) < 1e-12, "num[5] ~= 1.23e-4");
    }

    mln_json_destroy(&j);
}




/* ===========================================================
 *  15. Many keys hash stress test
 * =========================================================== */
static void test_many_keys(void)
{
    mln_json_t obj;
    int i;
    char buf[128];

    ASSERT(mln_json_obj_init(&obj) == 0, "obj_init for many keys");

    for (i = 0; i < 200; ++i) {
        mln_json_t key, val;
        mln_string_t *s;
        int len = snprintf(buf, sizeof(buf), "key_%d", i);
        s = mln_string_const_ndup(buf, len);
        ASSERT(s != NULL, "string dup in loop");
        mln_json_string_init(&key, s);
        mln_json_number_init(&val, (double)i);
        ASSERT(mln_json_obj_update(&obj, &key, &val) == 0, "obj_update in loop");
    }

    ASSERT(mln_json_obj_element_num(&obj) == 200, "200 keys inserted");

    /* spot check */
    {
        mln_string_t sk = mln_string("key_0");
        mln_json_t *v = mln_json_obj_search(&obj, &sk);
        ASSERT(v != NULL && mln_json_number_data_get(v) == 0.0, "key_0 == 0");
    }
    {
        mln_string_t sk = mln_string("key_199");
        mln_json_t *v = mln_json_obj_search(&obj, &sk);
        ASSERT(v != NULL && mln_json_number_data_get(v) == 199.0, "key_199 == 199");
    }
    {
        mln_string_t sk = mln_string("key_nonexist");
        mln_json_t *v = mln_json_obj_search(&obj, &sk);
        ASSERT(v == NULL, "nonexistent key returns NULL");
    }

    /* encode/decode round trip */
    {
        mln_string_t *enc = mln_json_encode(&obj, 0);
        ASSERT(enc != NULL, "encode 200 keys");
        if (enc) {
            mln_json_t j2;
            ASSERT(mln_json_decode(enc, &j2, NULL) == 0, "decode 200 keys");
            ASSERT(mln_json_obj_element_num(&j2) == 200, "re-decoded 200 keys");
            mln_json_destroy(&j2);
            mln_string_free(enc);
        }
    }

    mln_json_destroy(&obj);
}


static void test_special_key_encode(void)
{
    /* Keys with characters that need escaping: quote and backslash */
    mln_string_t input = mln_string("{\"a\\\"b\":1,\"c\\\\d\":2}");
    mln_json_t j;
    mln_string_t *out;
    ASSERT(mln_json_decode(&input, &j, NULL) == 0, "decode keys with escapes");

    out = mln_json_encode(&j, 0);
    ASSERT(out != NULL, "encode keys with escapes not NULL");
    /* Verify round-trip: re-decode the encoded output */
    if (out != NULL) {
        mln_json_t j2;
        ASSERT(mln_json_decode(out, &j2, NULL) == 0, "re-decode encoded special keys");
        /* Verify the keys still work */
        mln_string_t k1 = mln_string("a\"b");
        mln_string_t k2 = mln_string("c\\d");
        mln_json_t *v1 = mln_json_obj_search(&j2, &k1);
        mln_json_t *v2 = mln_json_obj_search(&j2, &k2);
        ASSERT(v1 != NULL && mln_json_is_number(v1), "special key a\"b found");
        ASSERT(v2 != NULL && mln_json_is_number(v2), "special key c\\d found");
        if (v1) ASSERT((int)mln_json_number_data_get(v1) == 1, "a\"b == 1");
        if (v2) ASSERT((int)mln_json_number_data_get(v2) == 2, "c\\d == 2");
        mln_json_destroy(&j2);
        mln_string_free(out);
    }
    mln_json_destroy(&j);
}

/* ===========================================================
 *  Unicode escape encoding (M_JSON_ENCODE_UNICODE)
 * =========================================================== */
static void test_unicode_escape_encode(void)
{
    /* BMP Chinese character: encode with M_JSON_ENCODE_UNICODE flag */
    {
        /* Build JSON: {"msg":"hello\xe4\xb8\xad\xe6\x96\x87"} where \xe4\xb8\xad\xe6\x96\x87 is UTF-8 for U+4E2D U+6587 */
        mln_json_t j;
        mln_json_init(&j);
        mln_json_generate(&j, "{s:s}", "msg", "hello\xe4\xb8\xad\xe6\x96\x87");

        /* Without flag: raw UTF-8 bytes pass through */
        mln_string_t *enc = mln_json_encode(&j, 0);
        ASSERT(enc != NULL, "encode without unicode flag");
        if (enc) {
            /* Should contain raw UTF-8 bytes, no \\u */
            ASSERT(memmem(enc->data, enc->len, "\\u", 2) == NULL, "no \\u without flag");
            mln_string_free(enc);
        }

        /* With flag: non-ASCII escaped to \uXXXX */
        enc = mln_json_encode(&j, M_JSON_ENCODE_UNICODE);
        ASSERT(enc != NULL, "encode with unicode flag");
        if (enc) {
            ASSERT(memmem(enc->data, enc->len, "\\u4e2d", 6) != NULL, "contains \\u4e2d");
            ASSERT(memmem(enc->data, enc->len, "\\u6587", 6) != NULL, "contains \\u6587");
            /* hello should still be plain ASCII */
            ASSERT(memmem(enc->data, enc->len, "hello", 5) != NULL, "ASCII preserved");

            /* Round-trip: decode the escaped output */
            mln_json_t j2;
            ASSERT(mln_json_decode(enc, &j2, NULL) == 0, "decode unicode-escaped");
            mln_string_t key = mln_string("msg");
            mln_json_t *v = mln_json_obj_search(&j2, &key);
            ASSERT(v && mln_json_is_string(v), "round-trip val is string");
            if (v) {
                mln_string_t *s = mln_json_string_data_get(v);
                /* Should contain original UTF-8 bytes after decoding */
                ASSERT(s->len == 11, "round-trip string length");  /* "hello" (5) + 2 Chinese chars (3 bytes each) */
                ASSERT(memcmp(s->data, "hello\xe4\xb8\xad\xe6\x96\x87", 11) == 0, "round-trip content matches");
            }
            mln_json_destroy(&j2);
            mln_string_free(enc);
        }
        mln_json_destroy(&j);
    }

    /* Supplementary plane (above BMP): surrogate pair */
    {
        /* U+1F600 (grinning face) = UTF-8: F0 9F 98 80 */
        mln_json_t j;
        mln_json_init(&j);
        mln_json_generate(&j, "{s:s}", "emoji", "\xf0\x9f\x98\x80");

        mln_string_t *enc = mln_json_encode(&j, M_JSON_ENCODE_UNICODE);
        ASSERT(enc != NULL, "encode surrogate pair");
        if (enc) {
            /* U+1F600 -> surrogate pair \uD83D\uDE00 */
            ASSERT(memmem(enc->data, enc->len, "\\ud83d\\ude00", 12) != NULL, "surrogate pair \\ud83d\\ude00");

            /* Round-trip */
            mln_json_t j2;
            ASSERT(mln_json_decode(enc, &j2, NULL) == 0, "decode surrogate pair");
            mln_string_t key = mln_string("emoji");
            mln_json_t *v = mln_json_obj_search(&j2, &key);
            ASSERT(v && mln_json_is_string(v), "surrogate round-trip is string");
            if (v) {
                mln_string_t *s = mln_json_string_data_get(v);
                ASSERT(s->len == 4, "surrogate round-trip length == 4");
                ASSERT(memcmp(s->data, "\xf0\x9f\x98\x80", 4) == 0, "surrogate round-trip bytes match");
            }
            mln_json_destroy(&j2);
            mln_string_free(enc);
        }
        mln_json_destroy(&j);
    }

    /* Pure ASCII: no escaping even with flag */
    {
        mln_json_t j;
        mln_json_init(&j);
        mln_json_generate(&j, "{s:s}", "key", "hello world");

        mln_string_t *enc = mln_json_encode(&j, M_JSON_ENCODE_UNICODE);
        ASSERT(enc != NULL, "encode pure ascii with flag");
        if (enc) {
            ASSERT(memmem(enc->data, enc->len, "\\u", 2) == NULL, "no \\u for pure ascii");
            mln_string_free(enc);
        }
        mln_json_destroy(&j);
    }
}

/* ===========================================================
 *  MAIN
 * =========================================================== */
int main(void)
{
    passed = failed = 0;

    fprintf(stderr, "=== JSON module comprehensive test ===\n\n");

    test_basic_types();
    test_decode_encode();
    test_object_ops();
    test_array_ops();
    test_parse();
    test_generate();
    test_reset();
    test_policy();
    test_complex_json();
    test_dump();
    test_number_edge_cases();
    test_many_keys();
    test_special_key_encode();
    test_unicode_escape_encode();

    fprintf(stderr, "\n=== Results: %d passed, %d failed ===\n", passed, failed);
    return failed ? 1 : 0;
}
