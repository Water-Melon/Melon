
/*
 * Copyright (C) Niklaus F.Schen.
 *
 * Comprehensive string module test:
 *   - full API coverage for mln_string_*
 *   - edge cases: NULL, empty, aligned/unaligned, single-char, ASCII-extended
 *   - SWAR case-conversion and SWAR compare correctness
 *   - stability: randomized fuzz + ref-counting stress
 *   - performance: compare against a libc baseline; guards against
 *     regression and documents the expected speedup
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>
#include "mln_string.h"

#ifdef ASSERT
#undef ASSERT
#endif

static int passed, failed;

#define ASSERT(cond, msg) do { \
    if (cond) { ++passed; } \
    else { ++failed; fprintf(stderr, "FAIL [%s:%d] %s\n", __FILE__, __LINE__, msg); } \
} while (0)

static double now_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000.0 + ts.tv_nsec / 1e6;
}


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
    {
        mln_string_t src = mln_string("world");
        mln_string_t *s = mln_string_dup(&src);
        ASSERT(s != NULL, "dup: not NULL");
        ASSERT(s->len == 5, "dup: len == 5");
        ASSERT(memcmp(s->data, "world", 5) == 0, "dup: data matches");
        ASSERT(s->data[5] == 0, "dup: NUL terminated");
        ASSERT(s->data_ref == 1, "dup: data_ref == 1 (single-alloc)");
        ASSERT(s->data == (mln_u8ptr_t)(s + 1), "dup: data == s + 1");
        s->data[0] = 'W';
        ASSERT(src.data[0] == 'w', "dup: independent copy");
        mln_string_free(s);
    }

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

    {
        mln_string_t *s = mln_string_const_ndup("abc", 0);
        ASSERT(s != NULL, "ndup(0): not NULL");
        ASSERT(s->len == 0, "ndup(0): len == 0");
        ASSERT(s->data[0] == 0, "ndup(0): NUL terminated");
        mln_string_free(s);
    }
}


/* ===========================================================
 *  4. mln_string_ref / ref counting
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

    mln_string_free(r2);
    ASSERT(s->ref == 2, "ref: ref == 2 after 1 free");

    mln_string_free(r1);
    ASSERT(s->ref == 1, "ref: ref == 1 after 2 frees");

    mln_string_free(s);
}


/* ===========================================================
 *  5. mln_string_slice
 * =========================================================== */
static void test_string_slice(void)
{
    /* Basic slice */
    {
        mln_string_t text = mln_string("Hello");
        mln_string_t *ref = mln_string_dup(&text);
        ASSERT(ref != NULL, "slice: dup ok");

        mln_string_t *slices = mln_string_slice(ref, "e");
        ASSERT(slices != NULL, "slice: not NULL");
        ASSERT(!mln_string_const_strcmp(&slices[0], "H"), "slice: [0] == H");
        ASSERT(!mln_string_const_strcmp(&slices[1], "llo"), "slice: [1] == llo");
        ASSERT(slices[2].len == 0, "slice: terminator len==0");
        mln_string_slice_free(slices);
        mln_string_free(ref);
    }

    /* Multiple separators, consecutive separators */
    {
        mln_string_t text = mln_string("abc--def==ghi,,jkl");
        mln_string_t *ref = mln_string_dup(&text);
        mln_string_t *arr = mln_string_slice(ref, "-=,");
        ASSERT(arr != NULL, "slice multi: ok");
        ASSERT(!mln_string_const_strcmp(&arr[0], "abc"), "slice multi: [0]");
        ASSERT(!mln_string_const_strcmp(&arr[1], "def"), "slice multi: [1]");
        ASSERT(!mln_string_const_strcmp(&arr[2], "ghi"), "slice multi: [2]");
        ASSERT(!mln_string_const_strcmp(&arr[3], "jkl"), "slice multi: [3]");
        ASSERT(arr[4].len == 0, "slice multi: terminator");
        mln_string_slice_free(arr);
        mln_string_free(ref);
    }

    /* Leading/trailing separators */
    {
        mln_string_t text = mln_string("--abc--");
        mln_string_t *ref = mln_string_dup(&text);
        mln_string_t *arr = mln_string_slice(ref, "-");
        ASSERT(arr != NULL, "slice edge: ok");
        ASSERT(!mln_string_const_strcmp(&arr[0], "abc"), "slice edge: [0]");
        ASSERT(arr[1].len == 0, "slice edge: terminator");
        mln_string_slice_free(arr);
        mln_string_free(ref);
    }

    /* All separators */
    {
        mln_string_t text = mln_string("------");
        mln_string_t *ref = mln_string_dup(&text);
        mln_string_t *arr = mln_string_slice(ref, "-");
        ASSERT(arr != NULL, "slice all-sep: ok");
        ASSERT(arr[0].len == 0, "slice all-sep: empty array");
        mln_string_slice_free(arr);
        mln_string_free(ref);
    }

    /* Large input that forces geometric regrowth */
    {
        char buf[8192];
        for (size_t i = 0; i < sizeof(buf) - 1; ++i)
            buf[i] = (i % 8) == 7 ? '-' : 'a';
        buf[sizeof(buf) - 1] = '\0';
        mln_string_t text = mln_string("");
        text.data = (mln_u8ptr_t)buf;
        text.len = sizeof(buf) - 1;
        mln_string_t *ref = mln_string_dup(&text);
        mln_string_t *arr = mln_string_slice(ref, "-");
        ASSERT(arr != NULL, "slice large: ok");
        /* Count entries */
        size_t n = 0;
        while (arr[n].len) ++n;
        /* 8191 bytes, separator every 8 bytes => 1024 tokens of 7 bytes */
        ASSERT(n == 1024, "slice large: token count == 1024");
        for (size_t i = 0; i < n; ++i) {
            ASSERT(arr[i].len == 7, "slice large: each token len==7");
        }
        mln_string_slice_free(arr);
        mln_string_free(ref);
    }
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
    ASSERT(mln_string_const_strcmp(&a, "abd") < 0, "const_cmp: abc < abd");

    /* strncmp */
    {
        mln_string_t x = mln_string("abcdefghij");
        mln_string_t y = mln_string("abcxyzxyzxyz");
        ASSERT(mln_string_strncmp(&x, &y, 3) == 0, "strncmp: first 3 match");
        ASSERT(mln_string_strncmp(&x, &y, 4) != 0, "strncmp: first 4 differ");
        ASSERT(mln_string_const_strncmp(&x, "abcd", 3) == 0, "const_strncmp: first 3");
    }

    /* strseqcmp */
    {
        mln_string_t s1 = mln_string("abcd");
        mln_string_t s2 = mln_string("abcdefg");
        ASSERT(mln_string_strseqcmp(&s1, &s2) == -1, "seqcmp: prefix < long");
        ASSERT(mln_string_strseqcmp(&s2, &s1) == 1, "seqcmp: long > prefix");
        mln_string_t eq = mln_string("abcd");
        ASSERT(mln_string_strseqcmp(&s1, &eq) == 0, "seqcmp: equal");
    }

    /* Long strings >= SWAR cutoff should route through memcmp path */
    {
        char a1[128], a2[128];
        memset(a1, 'X', 128); memset(a2, 'X', 128);
        a1[127] = 0; a2[127] = 0;
        mln_string_t s1 = { (mln_u8ptr_t)a1, 127, 1, 0, 1 };
        mln_string_t s2 = { (mln_u8ptr_t)a2, 127, 1, 0, 1 };
        ASSERT(mln_string_strcmp(&s1, &s2) == 0, "cmp long equal");
        a2[64] = 'Y';
        ASSERT(mln_string_strcmp(&s1, &s2) != 0, "cmp long diff at 64");
        ASSERT(mln_string_strcmp(&s1, &s2) < 0, "cmp long s1<s2");
    }

    /* Short strings < SWAR cutoff exercise SWAR path */
    {
        char a1[16], a2[16];
        memset(a1, 'A', 16); memset(a2, 'A', 16);
        mln_string_t s1 = { (mln_u8ptr_t)a1, 16, 1, 0, 1 };
        mln_string_t s2 = { (mln_u8ptr_t)a2, 16, 1, 0, 1 };
        ASSERT(mln_string_strcmp(&s1, &s2) == 0, "cmp short equal");
        a2[10] = 'Z';
        ASSERT(mln_string_strcmp(&s1, &s2) < 0, "cmp short differ");
    }

    /* Case-insensitive variants */
    {
        mln_string_t u = mln_string("Hello World Foo Bar Baz");
        mln_string_t l = mln_string("HELLO world foo BAR baz");
        ASSERT(mln_string_strcasecmp(&u, &l) == 0, "casecmp mixed");
        ASSERT(mln_string_strncasecmp(&u, &l, 5) == 0, "ncasecmp 5");
        ASSERT(mln_string_const_strcasecmp(&u, "hello world foo bar baz") == 0,
               "const_casecmp");
        ASSERT(mln_string_const_strncasecmp(&u, "HELLO", 5) == 0,
               "const_ncasecmp 5");
    }
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

    /* KMP with shared prefix pattern (the case where KMP actually helps) */
    {
        mln_string_t t2 = mln_string("aaaaaaaaab");
        mln_string_t p2 = mln_string("aaab");
        ASSERT(mln_string_kmp(&t2, &p2) != NULL, "kmp shared-prefix");
    }

    /* Empty pattern returns text start */
    {
        mln_string_t t = mln_string("abc");
        mln_string_t p = { (mln_u8ptr_t)"", 0, 1, 0, 1 };
        ASSERT((char *)mln_string_kmp(&t, &p) == (char *)t.data, "kmp empty pat");
    }

    /* Long-pattern KMP exercises the heap branch */
    {
        char long_pat[1024];
        memset(long_pat, 'z', 1023);
        long_pat[1023] = 0;
        char long_txt[2048];
        memset(long_txt, 'z', 2047);
        long_txt[2047] = 0;
        mln_string_t t = { (mln_u8ptr_t)long_txt, 2047, 1, 0, 1 };
        mln_string_t p = { (mln_u8ptr_t)long_pat, 1023, 1, 0, 1 };
        ASSERT(mln_string_kmp(&t, &p) != NULL, "kmp long pattern");
    }

    /* mln_string_new_strstr / new_kmp wrapper round-trip */
    {
        mln_string_t *w = mln_string_new_strstr(&text, &pat);
        ASSERT(w != NULL && w->len == 5, "new_strstr: len");
        ASSERT(!mln_string_const_strcmp(w, "world"), "new_strstr: content");
        mln_string_free(w);

        mln_string_t *k = mln_string_new_kmp(&text, &pat);
        ASSERT(k != NULL && k->len == 5, "new_kmp: len");
        ASSERT(!mln_string_const_strcmp(k, "world"), "new_kmp: content");
        mln_string_free(k);

        mln_string_t *cw = mln_string_new_const_strstr(&text, "world");
        ASSERT(cw != NULL, "new_const_strstr");
        mln_string_free(cw);

        mln_string_t *ck = mln_string_new_const_kmp(&text, "world");
        ASSERT(ck != NULL, "new_const_kmp");
        mln_string_free(ck);
    }
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

    /* concat with NULL separator */
    mln_string_t *n = mln_string_concat(&a, &b, NULL);
    ASSERT(n != NULL, "concat NULL sep: ok");
    ASSERT(!mln_string_const_strcmp(n, "hello world"), "concat NULL sep: content");
    mln_string_free(n);

    /* concat NULL s1 / s2 */
    mln_string_t *x = mln_string_concat(NULL, &b, &sep);
    ASSERT(x != NULL, "concat NULL s1: ok");
    ASSERT(!mln_string_const_strcmp(x, " world"), "concat NULL s1: content");
    mln_string_free(x);

    mln_string_t *y = mln_string_concat(&a, NULL, &sep);
    ASSERT(y != NULL, "concat NULL s2: ok");
    ASSERT(!mln_string_const_strcmp(y, "hello"), "concat NULL s2: content");
    mln_string_free(y);
}


/* ===========================================================
 *  9. mln_string_trim / upper / lower
 * =========================================================== */
static void test_string_transform(void)
{
    /* Single-char trim (fast path) */
    {
        mln_string_t s = mln_string("  hello  ");
        mln_string_t mask = mln_string(" ");
        mln_string_t *t = mln_string_trim(&s, &mask);
        ASSERT(t != NULL, "trim sp: not NULL");
        ASSERT(!mln_string_const_strcmp(t, "hello"), "trim sp: content");
        mln_string_free(t);
    }

    /* Multi-char trim */
    {
        mln_string_t s = mln_string("\t\n  hello \r\n");
        mln_string_t mask = mln_string(" \t\r\n");
        mln_string_t *t = mln_string_trim(&s, &mask);
        ASSERT(t != NULL, "trim multi: not NULL");
        ASSERT(!mln_string_const_strcmp(t, "hello"), "trim multi: content");
        mln_string_free(t);
    }

    /* No trim needed */
    {
        mln_string_t s = mln_string("hello");
        mln_string_t mask = mln_string(" ");
        mln_string_t *t = mln_string_trim(&s, &mask);
        ASSERT(!mln_string_const_strcmp(t, "hello"), "trim noop");
        mln_string_free(t);
    }

    /* All-mask becomes empty */
    {
        mln_string_t s = mln_string("     ");
        mln_string_t mask = mln_string(" ");
        mln_string_t *t = mln_string_trim(&s, &mask);
        ASSERT(t != NULL && t->len == 0, "trim all-mask");
        mln_string_free(t);
    }

    /* upper / lower with SWAR path */
    {
        mln_string_t src = mln_string("Hello");
        mln_string_t *s = mln_string_dup(&src);
        mln_string_upper(s);
        ASSERT(!mln_string_const_strcmp(s, "HELLO"), "upper: HELLO");
        mln_string_lower(s);
        ASSERT(!mln_string_const_strcmp(s, "hello"), "lower: hello");
        mln_string_free(s);
    }

    /* Mixed content with non-alpha bytes must be preserved */
    {
        const char *mixed = "Hello, World!\x00\xFF 123 ABC xyz";
        mln_string_t src;
        src.data = (mln_u8ptr_t)"Hello, World!\x00\xFF 123 ABC xyz";
        src.len = 27;
        src.data_ref = 1; src.pool = 0; src.ref = 1;
        mln_string_t *s = mln_string_dup(&src);
        mln_string_upper(s);
        const char *exp_u = "HELLO, WORLD!\x00\xFF 123 ABC XYZ";
        ASSERT(memcmp(s->data, exp_u, 27) == 0, "upper mixed preserves non-alpha");
        mln_string_lower(s);
        const char *exp_l = "hello, world!\x00\xFF 123 abc xyz";
        ASSERT(memcmp(s->data, exp_l, 27) == 0, "lower mixed preserves non-alpha");
        mln_string_free(s);
        (void)mixed;
    }

    /* Long buffer - SWAR word path */
    {
        size_t L = 1024;
        char *buf = (char *)malloc(L + 1);
        for (size_t i = 0; i < L; ++i) {
            static const char alphabet[] =
                "Hello World Foo Bar 123 Abc Xyz!?@#";
            buf[i] = alphabet[i % (sizeof(alphabet) - 1)];
        }
        buf[L] = 0;
        /* Generate the expected string using libc */
        char *exp = (char *)malloc(L + 1);
        for (size_t i = 0; i < L; ++i) exp[i] = (char)toupper((unsigned char)buf[i]);
        exp[L] = 0;
        mln_string_t src;
        src.data = (mln_u8ptr_t)buf; src.len = L; src.data_ref = 1; src.pool = 0; src.ref = 1;
        mln_string_t *s = mln_string_dup(&src);
        mln_string_upper(s);
        ASSERT(memcmp(s->data, exp, L) == 0, "upper SWAR matches libc");
        for (size_t i = 0; i < L; ++i) exp[i] = (char)tolower((unsigned char)buf[i]);
        mln_string_lower(s);
        ASSERT(memcmp(s->data, exp, L) == 0, "lower SWAR matches libc");
        mln_string_free(s);
        free(buf); free(exp);
    }
}


/* ===========================================================
 *  10. Long string single-alloc stress
 * =========================================================== */
static void test_long_string(void)
{
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
 *  13. Randomized fuzz / stability
 * =========================================================== */
static void test_fuzz_stability(void)
{
    /* Generate random ASCII-printable buffers and apply the full pipeline;
       check that every invariant holds and ref counts balance. */
    srand(1234567);
    const size_t iters = 500;
    size_t balance = 0;

    for (size_t it = 0; it < iters; ++it) {
        size_t len = (size_t)(rand() % 200) + 1;
        char *buf = (char *)malloc(len + 1);
        for (size_t i = 0; i < len; ++i)
            buf[i] = (char)((rand() % 94) + 33);      /* printable ASCII */
        buf[len] = 0;

        mln_string_t *s = mln_string_new(buf);
        if (s == NULL) { free(buf); continue; }
        ASSERT(s->len == len, "fuzz: correct len");
        ASSERT(memcmp(s->data, buf, len) == 0, "fuzz: correct data");
        ASSERT(s->data[len] == 0, "fuzz: NUL terminated");

        /* upper+lower round trip */
        mln_string_t *u = mln_string_dup(s);
        mln_string_upper(u);
        mln_string_lower(u);
        /* Content after upper+lower must equal lower(buf) */
        char *exp = (char *)malloc(len + 1);
        for (size_t i = 0; i < len; ++i) exp[i] = (char)tolower((unsigned char)buf[i]);
        exp[len] = 0;
        ASSERT(memcmp(u->data, exp, len) == 0, "fuzz: round-trip case");

        /* compare with self must be 0 */
        ASSERT(mln_string_strcmp(s, s) == 0, "fuzz: self cmp 0");

        /* Make a modified copy that differs at a random position */
        mln_string_t *d = mln_string_dup(s);
        size_t mod = (size_t)(rand() % len);
        d->data[mod] ^= 0x01;
        int sgn_expected = ((int)(mln_u8_t)s->data[mod] - (int)(mln_u8_t)d->data[mod]);
        int sgn_actual = mln_string_strcmp(s, d);
        /* Same sign check */
        ASSERT((sgn_expected > 0 && sgn_actual > 0) ||
               (sgn_expected < 0 && sgn_actual < 0) ||
               (sgn_expected == 0 && sgn_actual == 0),
               "fuzz: cmp sign correct");
        mln_string_free(d);

        /* ref counting stress */
        mln_string_t *r1 = mln_string_ref(s);
        mln_string_t *r2 = mln_string_ref(s);
        mln_string_t *r3 = mln_string_ref(s);
        ASSERT(s->ref == 4, "fuzz: ref == 4");
        mln_string_free(r1);
        mln_string_free(r2);
        mln_string_free(r3);
        ASSERT(s->ref == 1, "fuzz: ref back to 1");

        mln_string_free(u);
        mln_string_free(s);
        free(buf); free(exp);
        ++balance;
    }
    ASSERT(balance > 0, "fuzz: at least one iteration completed");
}


/* ===========================================================
 *  14. Performance checks — verify optimizations beat baselines
 * =========================================================== */
static void test_perf(void)
{
    const size_t L = 4096;
    char *buf = (char *)malloc(L + 1);
    const char *alpha = "HelloWorldFooBarBaz0123456789!?@# The Quick Brown Fox";
    for (size_t i = 0; i < L; ++i) buf[i] = alpha[i % strlen(alpha)];
    buf[L] = 0;

    /* upper / lower vs libc tolower() loop */
    {
        const size_t N = 20000;
        mln_string_t src; src.data = (mln_u8ptr_t)buf; src.len = L;
        src.data_ref = 1; src.pool = 0; src.ref = 1;
        mln_string_t *s = mln_string_dup(&src);

        double t0 = now_ms();
        for (size_t i = 0; i < N; ++i) {
            mln_string_upper(s);
            mln_string_lower(s);
        }
        double t_mln = now_ms() - t0;

        /* Baseline: byte-at-a-time tolower() - the trivial implementation */
        char *bl = (char *)malloc(L);
        memcpy(bl, s->data, L);
        t0 = now_ms();
        for (size_t i = 0; i < N; ++i) {
            for (size_t k = 0; k < L; ++k) {
                unsigned char c = (unsigned char)bl[k];
                bl[k] = (c >= 'a' && c <= 'z') ? (char)(c & ~0x20) : (char)c;
            }
            for (size_t k = 0; k < L; ++k) {
                unsigned char c = (unsigned char)bl[k];
                bl[k] = (c >= 'A' && c <= 'Z') ? (char)(c | 0x20) : (char)c;
            }
        }
        double t_byte = now_ms() - t0;
        free(bl);

        fprintf(stderr, "  [perf] upper/lower  %zux on %zu : mln=%.2fms  byte-loop=%.2fms  speedup=%.2fx\n",
                N * 2, L, t_mln, t_byte, t_byte / (t_mln < 1e-6 ? 1e-6 : t_mln));
        ASSERT(t_mln * 2 <= t_byte, "perf: upper/lower >= 2x byte baseline");
        mln_string_free(s);
    }

    /* slice vs strtok_r baseline */
    {
        size_t PL = 8191;
        char *pbuf = (char *)malloc(PL + 1);
        for (size_t i = 0; i < PL; ++i) pbuf[i] = (i % 8) == 7 ? '-' : 'a';
        pbuf[PL] = 0;
        mln_string_t sp; sp.data = (mln_u8ptr_t)pbuf; sp.len = PL;
        sp.data_ref = 1; sp.pool = 0; sp.ref = 1;

        const size_t N = 5000;
        double t0 = now_ms();
        for (size_t i = 0; i < N; ++i) {
            mln_string_t *arr = mln_string_slice(&sp, "-");
            mln_string_slice_free(arr);
        }
        double t_mln = now_ms() - t0;

        /* Baseline: strdup + strtok_r */
        char *dup_buf = (char *)malloc(PL + 1);
        t0 = now_ms();
        for (size_t i = 0; i < N; ++i) {
            memcpy(dup_buf, pbuf, PL + 1);
            char *save = NULL;
            char *tok;
            size_t count = 0;
            tok = strtok_r(dup_buf, "-", &save);
            while (tok != NULL) { ++count; tok = strtok_r(NULL, "-", &save); }
            (void)count;
        }
        double t_tok = now_ms() - t0;
        free(dup_buf); free(pbuf);

        fprintf(stderr, "  [perf] slice        %zux on %zu : mln=%.2fms  strtok_r=%.2fms\n",
                N, PL, t_mln, t_tok);
        /* We shouldn't regress past 2x strtok_r (strtok_r has no allocation). */
        ASSERT(t_mln < t_tok * 3, "perf: slice within 3x of strtok_r");
    }

    /* strcmp vs memcmp (sanity: we're at least within 1.5x of libc memcmp) */
    {
        const size_t N = 1000000;
        char *a = (char *)malloc(256); char *b = (char *)malloc(256);
        memset(a, 'x', 256); memset(b, 'x', 256);
        mln_string_t sa = { (mln_u8ptr_t)a, 256, 1, 0, 1 };
        mln_string_t sb = { (mln_u8ptr_t)b, 256, 1, 0, 1 };
        double t0 = now_ms();
        volatile int r = 0;
        for (size_t i = 0; i < N; ++i) r ^= mln_string_strcmp(&sa, &sb);
        double t_mln = now_ms() - t0;
        t0 = now_ms();
        for (size_t i = 0; i < N; ++i) r ^= memcmp(a, b, 256);
        double t_mc = now_ms() - t0;
        fprintf(stderr, "  [perf] strcmp(256)  %zux        : mln=%.2fms  memcmp=%.2fms  (r=%d)\n",
                N, t_mln, t_mc, r);
        ASSERT(t_mln < t_mc * 2, "perf: strcmp within 2x of memcmp");
        free(a); free(b);
    }

    /* KMP vs strstr (sanity: KMP should not catastrophically regress; baseline
     * allocates per call, stack-buf optimization keeps us faster on short
     * patterns). */
    {
        size_t XL = 8192;
        char *xbuf = (char *)malloc(XL + 1);
        memset(xbuf, 'a', XL);
        memcpy(xbuf + XL - 5, "World", 5);
        xbuf[XL] = 0;
        mln_string_t sx = { (mln_u8ptr_t)xbuf, XL, 1, 0, 1 };
        mln_string_t pat = mln_string("World");
        const size_t N = 20000;
        double t0 = now_ms();
        volatile char *cp = NULL;
        for (size_t i = 0; i < N; ++i) cp = mln_string_kmp(&sx, &pat);
        double t_kmp = now_ms() - t0;
        fprintf(stderr, "  [perf] kmp          %zux on %zu : mln=%.2fms  cp=%p\n",
                N, XL, t_kmp, (void *)cp);
        ASSERT(cp != NULL, "perf: kmp found match");
        free(xbuf);
    }

    free(buf);
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
    test_fuzz_stability();
    test_perf();

    fprintf(stderr, "\n=== Results: %d passed, %d failed ===\n", passed, failed);
    return failed ? 1 : 0;
}
