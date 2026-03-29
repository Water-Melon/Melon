
/*
 * Copyright (C) Niklaus F.Schen.
 *
 * Comprehensive string module test:
 *   - mln_string_new, mln_string_dup, mln_string_const_ndup (single-alloc)
 *   - mln_string_ref, mln_string_free (ref counting)
 *   - mln_string_slice, mln_string_strcat, mln_string_concat
 *   - mln_string_strcmp, mln_string_strstr, mln_string_kmp
 *   - mln_string_trim, mln_string_upper, mln_string_lower
 *   - edge cases: NULL, empty, long strings
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "mln_string.h"

#ifdef ASSERT
#undef ASSERT
#endif

static int passed, failed;

#define ASSERT(cond, msg) do { \
    if (cond) { ++passed; } \
    else { ++failed; fprintf(stderr, "FAIL [%s:%d] %s\n", __FILE__, __LINE__, msg); } \
} while (0)


/* ===========================================================
 *  1. mln_string_new (single-alloc)
 * =========================================================== */
static void test_string_new(void)
{
    /* Normal string */
    {
        mln_string_t *s = mln_string_new("hello");
        ASSERT(s != NULL, "new: not NULL");
        ASSERT(s->len == 5, "new: len == 5");
        ASSERT(memcmp(s->data, "hello", 5) == 0, "new: data matches");
        ASSERT(s->data[5] == 0, "new: NUL terminated");
        ASSERT(s->data_ref == 1, "new: data_ref == 1 (single-alloc)");
        ASSERT(s->pool == 0, "new: pool == 0");
        ASSERT(s->ref == 1, "new: ref == 1");
        /* Verify single-alloc: data immediately follows struct */
        ASSERT(s->data == (mln_u8ptr_t)(s + 1), "new: data == s + 1");
        mln_string_free(s);
    }

    /* NULL input */
    {
        mln_string_t *s = mln_string_new(NULL);
        ASSERT(s != NULL, "new(NULL): not NULL");
        ASSERT(s->data != NULL, "new(NULL): data is not NULL");
        ASSERT(s->data[0] == 0, "new(NULL): data[0] == 0");
        ASSERT(s->len == 0, "new(NULL): len == 0");
        mln_string_free(s);
    }

    /* Empty string */
    {
        mln_string_t *s = mln_string_new("");
        ASSERT(s != NULL, "new(''): not NULL");
        ASSERT(s->len == 0, "new(''): len == 0");
        ASSERT(s->data[0] == 0, "new(''): NUL terminated");
        ASSERT(s->data == (mln_u8ptr_t)(s + 1), "new(''): single-alloc");
        mln_string_free(s);
    }
}


/* ===========================================================
 *  2. mln_string_dup (single-alloc)
 * =========================================================== */
static void test_string_dup(void)
{
    /* Normal dup */
    {
        mln_string_t src = mln_string("world");
        mln_string_t *s = mln_string_dup(&src);
        ASSERT(s != NULL, "dup: not NULL");
        ASSERT(s->len == 5, "dup: len == 5");
        ASSERT(memcmp(s->data, "world", 5) == 0, "dup: data matches");
        ASSERT(s->data[5] == 0, "dup: NUL terminated");
        ASSERT(s->data_ref == 1, "dup: data_ref == 1 (single-alloc)");
        ASSERT(s->data == (mln_u8ptr_t)(s + 1), "dup: data == s + 1");
        /* Verify independence: modifying dup doesn't affect src */
        s->data[0] = 'W';
        ASSERT(src.data[0] == 'w', "dup: independent copy");
        mln_string_free(s);
    }

    /* Dup of stack string */
    {
        mln_string_t src = mln_string("test");
        mln_string_t *s = mln_string_dup(&src);
        ASSERT(s != NULL && s->len == 4, "dup stack: ok");
        ASSERT(!mln_string_strcmp(s, &src), "dup stack: content matches");
        mln_string_free(s);
    }
}


/* ===========================================================
 *  3. mln_string_const_ndup (single-alloc)
 * =========================================================== */
static void test_string_const_ndup(void)
{
    /* Normal ndup */
    {
        mln_string_t *s = mln_string_const_ndup("abcdef", 4);
        ASSERT(s != NULL, "ndup: not NULL");
        ASSERT(s->len == 4, "ndup: len == 4");
        ASSERT(memcmp(s->data, "abcd", 4) == 0, "ndup: data matches");
        ASSERT(s->data[4] == 0, "ndup: NUL terminated");
        ASSERT(s->data_ref == 1, "ndup: data_ref == 1 (single-alloc)");
        ASSERT(s->data == (mln_u8ptr_t)(s + 1), "ndup: data == s + 1");
        mln_string_free(s);
    }

    /* Zero length */
    {
        mln_string_t *s = mln_string_const_ndup("abc", 0);
        ASSERT(s != NULL, "ndup(0): not NULL");
        ASSERT(s->len == 0, "ndup(0): len == 0");
        ASSERT(s->data[0] == 0, "ndup(0): NUL terminated");
        mln_string_free(s);
    }

    /* Negative size */
    {
        mln_string_t *s = mln_string_const_ndup("abc", -1);
        ASSERT(s == NULL, "ndup(-1): returns NULL");
    }
}


/* ===========================================================
 *  4. mln_string_ref and ref counting
 * =========================================================== */
static void test_string_ref(void)
{
    mln_string_t *s = mln_string_new("ref_test");
    ASSERT(s != NULL && s->ref == 1, "ref: initial ref == 1");

    mln_string_t *r1 = mln_string_ref(s);
    ASSERT(r1 == s, "ref: returns same pointer");
    ASSERT(s->ref == 2, "ref: ref == 2");

    mln_string_t *r2 = mln_string_ref(s);
    ASSERT(s->ref == 3, "ref: ref == 3");

    /* Free twice — should just decrement ref */
    mln_string_free(r2);
    ASSERT(s->ref == 2, "ref: ref == 2 after 1 free");

    mln_string_free(r1);
    ASSERT(s->ref == 1, "ref: ref == 1 after 2 frees");

    /* Final free actually deallocates */
    mln_string_free(s);
}


/* ===========================================================
 *  5. mln_string_slice
 * =========================================================== */
static void test_string_slice(void)
{
    mln_string_t text = mln_string("Hello");
    mln_string_t *ref = mln_string_dup(&text);
    ASSERT(ref != NULL, "slice: dup ok");

    mln_string_t *slices = mln_string_slice(ref, "e");
    ASSERT(slices != NULL, "slice: not NULL");
    ASSERT(slices[0].len > 0, "slice: [0] has content");
    ASSERT(!mln_string_const_strcmp(&slices[0], "H"), "slice: [0] == H");
    ASSERT(slices[1].len > 0, "slice: [1] has content");
    ASSERT(!mln_string_const_strcmp(&slices[1], "llo"), "slice: [1] == llo");

    mln_string_slice_free(slices);
    mln_string_free(ref);
}


/* ===========================================================
 *  6. mln_string_strcmp / strncmp / strcasecmp
 * =========================================================== */
static void test_string_cmp(void)
{
    mln_string_t a = mln_string("abc");
    mln_string_t b = mln_string("abc");
    mln_string_t c = mln_string("abd");
    mln_string_t d = mln_string("ABC");

    ASSERT(mln_string_strcmp(&a, &b) == 0, "cmp: abc == abc");
    ASSERT(mln_string_strcmp(&a, &c) < 0, "cmp: abc < abd");
    ASSERT(mln_string_strcmp(&c, &a) > 0, "cmp: abd > abc");
    ASSERT(mln_string_strcasecmp(&a, &d) == 0, "casecmp: abc == ABC");
    ASSERT(mln_string_const_strcmp(&a, "abc") == 0, "const_cmp: abc == abc");
}


/* ===========================================================
 *  7. mln_string_strstr / kmp
 * =========================================================== */
static void test_string_search(void)
{
    mln_string_t text = mln_string("hello world");
    mln_string_t pat = mln_string("world");
    mln_string_t nopat = mln_string("xyz");

    ASSERT(mln_string_strstr(&text, &pat) != NULL, "strstr: found");
    ASSERT(mln_string_strstr(&text, &nopat) == NULL, "strstr: not found");
    ASSERT(mln_string_kmp(&text, &pat) != NULL, "kmp: found");
    ASSERT(mln_string_kmp(&text, &nopat) == NULL, "kmp: not found");
}


/* ===========================================================
 *  8. mln_string_strcat / concat
 * =========================================================== */
static void test_string_cat(void)
{
    mln_string_t a = mln_string("hello");
    mln_string_t b = mln_string(" world");
    mln_string_t *c = mln_string_strcat(&a, &b);
    ASSERT(c != NULL, "strcat: not NULL");
    ASSERT(c->len == 11, "strcat: len == 11");
    ASSERT(!mln_string_const_strcmp(c, "hello world"), "strcat: content");
    mln_string_free(c);

    mln_string_t sep = mln_string("-");
    mln_string_t *d = mln_string_concat(&a, &b, &sep);
    ASSERT(d != NULL, "concat: not NULL");
    ASSERT(!mln_string_const_strcmp(d, "hello- world"), "concat: content");
    mln_string_free(d);
}


/* ===========================================================
 *  9. mln_string_trim / upper / lower
 * =========================================================== */
static void test_string_transform(void)
{
    {
        mln_string_t s = mln_string("  hello  ");
        mln_string_t mask = mln_string(" ");
        mln_string_t *t = mln_string_trim(&s, &mask);
        ASSERT(t != NULL, "trim: not NULL");
        ASSERT(!mln_string_const_strcmp(t, "hello"), "trim: content");
        mln_string_free(t);
    }

    {
        mln_string_t src = mln_string("Hello");
        mln_string_t *s = mln_string_dup(&src);
        mln_string_upper(s);
        ASSERT(!mln_string_const_strcmp(s, "HELLO"), "upper: HELLO");
        mln_string_lower(s);
        ASSERT(!mln_string_const_strcmp(s, "hello"), "lower: hello");
        mln_string_free(s);
    }
}


/* ===========================================================
 *  10. Long string single-alloc stress
 * =========================================================== */
static void test_long_string(void)
{
    /* 10KB string */
    char buf[10240];
    memset(buf, 'A', sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';

    mln_string_t *s = mln_string_new(buf);
    ASSERT(s != NULL, "long: new not NULL");
    ASSERT(s->len == sizeof(buf) - 1, "long: correct len");
    ASSERT(s->data == (mln_u8ptr_t)(s + 1), "long: single-alloc");
    ASSERT(s->data[0] == 'A', "long: first byte");
    ASSERT(s->data[s->len - 1] == 'A', "long: last byte");
    mln_string_free(s);

    mln_string_t src;
    mln_string_nset(&src, buf, sizeof(buf) - 1);
    mln_string_t *d = mln_string_dup(&src);
    ASSERT(d != NULL, "long dup: not NULL");
    ASSERT(d->len == sizeof(buf) - 1, "long dup: correct len");
    ASSERT(d->data == (mln_u8ptr_t)(d + 1), "long dup: single-alloc");
    mln_string_free(d);
}


/* ===========================================================
 *  11. mln_string_ref_dup / const_ref_dup
 * =========================================================== */
static void test_ref_dup(void)
{
    mln_string_t src = mln_string("ref_dup");
    mln_string_t *s = mln_string_ref_dup(&src);
    ASSERT(s != NULL, "ref_dup: not NULL");
    ASSERT(s->data == src.data, "ref_dup: shares data");
    ASSERT(s->data_ref == 1, "ref_dup: data_ref == 1");
    mln_string_free(s);

    mln_string_t *s2 = mln_string_const_ref_dup("const_ref");
    ASSERT(s2 != NULL, "const_ref_dup: not NULL");
    ASSERT(s2->len == 9, "const_ref_dup: len == 9");
    ASSERT(s2->data_ref == 1, "const_ref_dup: data_ref == 1");
    mln_string_free(s2);
}


/* ===========================================================
 *  12. mln_string_buf_new
 * =========================================================== */
static void test_buf_new(void)
{
    mln_u8ptr_t buf = (mln_u8ptr_t)malloc(6);
    memcpy(buf, "bufnw", 5);
    buf[5] = 0;
    mln_string_t *s = mln_string_buf_new(buf, 5);
    ASSERT(s != NULL, "buf_new: not NULL");
    ASSERT(s->data == buf, "buf_new: data points to buf");
    ASSERT(s->len == 5, "buf_new: len == 5");
    ASSERT(s->data_ref == 0, "buf_new: data_ref == 0");
    mln_string_free(s); /* frees both s and buf */
}



/* ===========================================================
 *  MAIN
 * =========================================================== */
int main(void)
{
    passed = failed = 0;

    fprintf(stderr, "=== String module comprehensive test ===\n\n");

    test_string_new();
    test_string_dup();
    test_string_const_ndup();
    test_string_ref();
    test_string_slice();
    test_string_cmp();
    test_string_search();
    test_string_cat();
    test_string_transform();
    test_long_string();
    test_ref_dup();
    test_buf_new();

    fprintf(stderr, "\n=== Results: %d passed, %d failed ===\n", passed, failed);
    return failed ? 1 : 0;
}
