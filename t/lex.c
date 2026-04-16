#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include "mln_lex.h"

static int test_nr = 0;

#define PASS() do { printf("  #%02d PASS\n", ++test_nr); } while (0)

/* ------------------------------------------------------------------ */
/* Token type definition                                               */
/* ------------------------------------------------------------------ */

mln_string_t keywords[] = {
    mln_string("on"),
    mln_string("off"),
    mln_string(NULL)
};

MLN_DEFINE_TOKEN_TYPE_AND_STRUCT(static, mln_test, TEST, TEST_TK_ON, TEST_TK_OFF, TEST_TK_STRING);
MLN_DEFINE_TOKEN(static, mln_test, TEST, {TEST_TK_ON, "TEST_TK_ON"}, {TEST_TK_OFF, "TEST_TK_OFF"}, {TEST_TK_STRING, "TEST_TK_STRING"});

/* ------------------------------------------------------------------ */
/* Custom double-quote handler (for string literals)                   */
/* ------------------------------------------------------------------ */

static inline int
mln_get_char(mln_lex_t *lex, char c)
{
    if (c == '\\') {
        char n;
        if ((n = mln_lex_getchar(lex)) == MLN_ERR) return -1;
        switch (n) {
            case '\"': if (mln_lex_putchar(lex, n) == MLN_ERR) return -1; break;
            case '\'': if (mln_lex_putchar(lex, n) == MLN_ERR) return -1; break;
            case 'n':  if (mln_lex_putchar(lex, '\n') == MLN_ERR) return -1; break;
            case 't':  if (mln_lex_putchar(lex, '\t') == MLN_ERR) return -1; break;
            case '\\': if (mln_lex_putchar(lex, '\\') == MLN_ERR) return -1; break;
            default:   mln_lex_error_set(lex, MLN_LEX_EINVCHAR); return -1;
        }
    } else {
        if (mln_lex_putchar(lex, c) == MLN_ERR) return -1;
    }
    return 0;
}

static mln_test_struct_t *
mln_test_dblq_handler(mln_lex_t *lex, void *data)
{
    mln_lex_result_clean(lex);
    char c;
    while (1) {
        c = mln_lex_getchar(lex);
        if (c == MLN_ERR) return NULL;
        if (c == MLN_EOF) { mln_lex_error_set(lex, MLN_LEX_EINVEOF); return NULL; }
        if (c == '\"') break;
        if (mln_get_char(lex, c) < 0) return NULL;
    }
    return mln_test_new(lex, TEST_TK_STRING);
}

/* ------------------------------------------------------------------ */
/* Helper: create a lexer from a buffer string                         */
/* ------------------------------------------------------------------ */

static mln_lex_t *create_lex_from_buf(mln_alloc_t *pool, const char *input, int preprocess)
{
    mln_string_t buf;
    mln_lex_t *lex = NULL;
    struct mln_lex_attr lattr;
    mln_lex_hooks_t hooks;
    memset(&hooks, 0, sizeof(hooks));
    mln_string_nset(&buf, input, strlen(input));
    lattr.pool = pool;
    lattr.keywords = keywords;
    lattr.hooks = &hooks;
    lattr.preprocess = preprocess;
    lattr.padding = 0;
    lattr.type = M_INPUT_T_BUF;
    lattr.data = &buf;
    lattr.env = NULL;
    mln_lex_init_with_hooks(mln_test, lex, &lattr);
    return lex;
}

static mln_lex_t *create_lex_with_dblq(mln_alloc_t *pool, const char *input, int preprocess)
{
    mln_string_t buf;
    mln_lex_t *lex = NULL;
    struct mln_lex_attr lattr;
    mln_lex_hooks_t hooks;
    memset(&hooks, 0, sizeof(hooks));
    hooks.dblq_handler = (lex_hook)mln_test_dblq_handler;
    mln_string_nset(&buf, input, strlen(input));
    lattr.pool = pool;
    lattr.keywords = keywords;
    lattr.hooks = &hooks;
    lattr.preprocess = preprocess;
    lattr.padding = 0;
    lattr.type = M_INPUT_T_BUF;
    lattr.data = &buf;
    lattr.env = NULL;
    mln_lex_init_with_hooks(mln_test, lex, &lattr);
    return lex;
}

/* ================================================================== */
/* Test cases                                                          */
/* ================================================================== */

/* 1. Basic tokenize: "a = b; c = d" */
static void test_basic_tokenize(void)
{
    mln_alloc_t *pool = mln_alloc_init(NULL, 0);
    assert(pool != NULL);
    mln_lex_t *lex = create_lex_from_buf(pool, "a = b; c = d", 0);
    assert(lex != NULL);

    /* Expected: ID(a) EQUAL ID(b) SEMIC ID(c) EQUAL ID(d) EOF */
    enum mln_test_enum expect[] = {
        TEST_TK_ID, TEST_TK_EQUAL, TEST_TK_ID, TEST_TK_SEMIC,
        TEST_TK_ID, TEST_TK_EQUAL, TEST_TK_ID, TEST_TK_EOF
    };
    int i;
    for (i = 0; i < (int)(sizeof(expect)/sizeof(expect[0])); i++) {
        mln_test_struct_t *ts = mln_test_token(lex);
        assert(ts != NULL);
        assert(ts->type == expect[i]);
        mln_test_free(ts);
    }

    mln_lex_destroy(lex);
    mln_alloc_destroy(pool);
    PASS();
}

/* 2. Keywords: "on" and "off" map to custom keyword tokens */
static void test_keywords(void)
{
    mln_alloc_t *pool = mln_alloc_init(NULL, 0);
    assert(pool != NULL);
    mln_lex_t *lex = create_lex_from_buf(pool, "on off hello", 0);
    assert(lex != NULL);

    mln_test_struct_t *ts;

    ts = mln_test_token(lex);
    assert(ts != NULL && ts->type == TEST_TK_ON);
    mln_test_free(ts);

    ts = mln_test_token(lex);
    assert(ts != NULL && ts->type == TEST_TK_OFF);
    mln_test_free(ts);

    ts = mln_test_token(lex);
    assert(ts != NULL && ts->type == TEST_TK_ID);
    assert(ts->text != NULL && ts->text->len == 5);
    assert(memcmp(ts->text->data, "hello", 5) == 0);
    mln_test_free(ts);

    ts = mln_test_token(lex);
    assert(ts != NULL && ts->type == TEST_TK_EOF);
    mln_test_free(ts);

    mln_lex_destroy(lex);
    mln_alloc_destroy(pool);
    PASS();
}

/* 3. Decimal numbers */
static void test_numbers_dec(void)
{
    mln_alloc_t *pool = mln_alloc_init(NULL, 0);
    assert(pool != NULL);
    mln_lex_t *lex = create_lex_from_buf(pool, "42 0 123456", 0);
    assert(lex != NULL);

    mln_test_struct_t *ts;

    ts = mln_test_token(lex);
    assert(ts != NULL && ts->type == TEST_TK_DEC);
    assert(memcmp(ts->text->data, "42", 2) == 0);
    mln_test_free(ts);

    /* Single "0" is octal in this lexer (starts with 0, length 1 -> goes to dec path) */
    ts = mln_test_token(lex);
    assert(ts != NULL && ts->type == TEST_TK_DEC);
    mln_test_free(ts);

    ts = mln_test_token(lex);
    assert(ts != NULL && ts->type == TEST_TK_DEC);
    assert(memcmp(ts->text->data, "123456", 6) == 0);
    mln_test_free(ts);

    mln_lex_destroy(lex);
    mln_alloc_destroy(pool);
    PASS();
}

/* 4. Hex numbers */
static void test_numbers_hex(void)
{
    mln_alloc_t *pool = mln_alloc_init(NULL, 0);
    assert(pool != NULL);
    mln_lex_t *lex = create_lex_from_buf(pool, "0x1F 0xFF 0xABCD", 0);
    assert(lex != NULL);

    mln_test_struct_t *ts;

    ts = mln_test_token(lex);
    assert(ts != NULL && ts->type == TEST_TK_HEX);
    mln_test_free(ts);

    ts = mln_test_token(lex);
    assert(ts != NULL && ts->type == TEST_TK_HEX);
    mln_test_free(ts);

    ts = mln_test_token(lex);
    assert(ts != NULL && ts->type == TEST_TK_HEX);
    assert(memcmp(ts->text->data, "0xABCD", 6) == 0);
    mln_test_free(ts);

    mln_lex_destroy(lex);
    mln_alloc_destroy(pool);
    PASS();
}

/* 5. Octal numbers */
static void test_numbers_oct(void)
{
    mln_alloc_t *pool = mln_alloc_init(NULL, 0);
    assert(pool != NULL);
    mln_lex_t *lex = create_lex_from_buf(pool, "07 0377", 0);
    assert(lex != NULL);

    mln_test_struct_t *ts;

    ts = mln_test_token(lex);
    assert(ts != NULL && ts->type == TEST_TK_OCT);
    assert(memcmp(ts->text->data, "07", 2) == 0);
    mln_test_free(ts);

    ts = mln_test_token(lex);
    assert(ts != NULL && ts->type == TEST_TK_OCT);
    assert(memcmp(ts->text->data, "0377", 4) == 0);
    mln_test_free(ts);

    mln_lex_destroy(lex);
    mln_alloc_destroy(pool);
    PASS();
}

/* 6. Real numbers */
static void test_numbers_real(void)
{
    mln_alloc_t *pool = mln_alloc_init(NULL, 0);
    assert(pool != NULL);
    mln_lex_t *lex = create_lex_from_buf(pool, "3.14 0.5 100.0", 0);
    assert(lex != NULL);

    mln_test_struct_t *ts;

    ts = mln_test_token(lex);
    assert(ts != NULL && ts->type == TEST_TK_REAL);
    assert(ts->text->len == 4 && memcmp(ts->text->data, "3.14", 4) == 0);
    mln_test_free(ts);

    ts = mln_test_token(lex);
    assert(ts != NULL && ts->type == TEST_TK_REAL);
    mln_test_free(ts);

    ts = mln_test_token(lex);
    assert(ts != NULL && ts->type == TEST_TK_REAL);
    mln_test_free(ts);

    mln_lex_destroy(lex);
    mln_alloc_destroy(pool);
    PASS();
}

/* 7. Special characters */
static void test_special_chars(void)
{
    mln_alloc_t *pool = mln_alloc_init(NULL, 0);
    assert(pool != NULL);

    /* Test a representative set of special characters and their token types */
    struct { const char *input; enum mln_test_enum type; } cases[] = {
        {"!",  TEST_TK_EXCL},
        {"\"", TEST_TK_DBLQ},
        {"$",  TEST_TK_DOLL},
        {"%",  TEST_TK_PERC},
        {"&",  TEST_TK_AMP},
        {"'",  TEST_TK_SGLQ},
        {"(",  TEST_TK_LPAR},
        {")",  TEST_TK_RPAR},
        {"*",  TEST_TK_AST},
        {"+",  TEST_TK_PLUS},
        {",",  TEST_TK_COMMA},
        {"-",  TEST_TK_SUB},
        {".",  TEST_TK_PERIOD},
        {"/",  TEST_TK_SLASH},
        {":",  TEST_TK_COLON},
        {";",  TEST_TK_SEMIC},
        {"<",  TEST_TK_LAGL},
        {"=",  TEST_TK_EQUAL},
        {">",  TEST_TK_RAGL},
        {"?",  TEST_TK_QUES},
        {"@",  TEST_TK_AT},
        {"[",  TEST_TK_LSQUAR},
        {"\\", TEST_TK_BSLASH},
        {"]",  TEST_TK_RSQUAR},
        {"^",  TEST_TK_XOR},
        {"`",  TEST_TK_FULSTP},
        {"{",  TEST_TK_LBRACE},
        {"|",  TEST_TK_VERTL},
        {"}",  TEST_TK_RBRACE},
        {"~",  TEST_TK_DASH},
    };
    int i;
    for (i = 0; i < (int)(sizeof(cases)/sizeof(cases[0])); i++) {
        mln_lex_t *lex = create_lex_from_buf(pool, cases[i].input, 0);
        assert(lex != NULL);
        mln_test_struct_t *ts = mln_test_token(lex);
        assert(ts != NULL);
        assert(ts->type == cases[i].type);
        mln_test_free(ts);
        mln_lex_destroy(lex);
    }

    mln_alloc_destroy(pool);
    PASS();
}

/* 8. Identifiers */
static void test_identifiers(void)
{
    mln_alloc_t *pool = mln_alloc_init(NULL, 0);
    assert(pool != NULL);
    mln_lex_t *lex = create_lex_from_buf(pool, "abc Xyz _foo a1b2c3", 0);
    assert(lex != NULL);

    mln_test_struct_t *ts;
    const char *expected[] = {"abc", "Xyz", "_foo", "a1b2c3"};
    int i;
    for (i = 0; i < 4; i++) {
        ts = mln_test_token(lex);
        assert(ts != NULL && ts->type == TEST_TK_ID);
        assert(ts->text->len == strlen(expected[i]));
        assert(memcmp(ts->text->data, expected[i], ts->text->len) == 0);
        mln_test_free(ts);
    }

    mln_lex_destroy(lex);
    mln_alloc_destroy(pool);
    PASS();
}

/* 9. Whitespace and newline handling, line tracking */
static void test_whitespace_newline(void)
{
    mln_alloc_t *pool = mln_alloc_init(NULL, 0);
    assert(pool != NULL);
    /* Use trailing newline to avoid line-reset on buffer exhaustion */
    mln_lex_t *lex = create_lex_from_buf(pool, "a\n\nb\n  c\n", 0);
    assert(lex != NULL);

    mln_test_struct_t *ts;

    ts = mln_test_token(lex);
    assert(ts != NULL && ts->type == TEST_TK_ID);
    assert(ts->line == 1);
    mln_test_free(ts);

    ts = mln_test_token(lex);
    assert(ts != NULL && ts->type == TEST_TK_ID);
    assert(ts->line == 3);
    mln_test_free(ts);

    ts = mln_test_token(lex);
    assert(ts != NULL && ts->type == TEST_TK_ID);
    assert(ts->line == 4);
    mln_test_free(ts);

    mln_lex_destroy(lex);
    mln_alloc_destroy(pool);
    PASS();
}

/* 10. EOF token */
static void test_eof(void)
{
    mln_alloc_t *pool = mln_alloc_init(NULL, 0);
    assert(pool != NULL);
    mln_lex_t *lex = create_lex_from_buf(pool, "x", 0);
    assert(lex != NULL);

    mln_test_struct_t *ts;

    ts = mln_test_token(lex);
    assert(ts != NULL && ts->type == TEST_TK_ID);
    mln_test_free(ts);

    ts = mln_test_token(lex);
    assert(ts != NULL && ts->type == TEST_TK_EOF);
    assert(ts->text != NULL);
    assert(memcmp(ts->text->data, "##EOF##", 7) == 0);
    mln_test_free(ts);

    mln_lex_destroy(lex);
    mln_alloc_destroy(pool);
    PASS();
}

/* 11. Custom hook: double-quote string handler */
static void test_custom_hooks(void)
{
    mln_alloc_t *pool = mln_alloc_init(NULL, 0);
    assert(pool != NULL);
    mln_lex_t *lex = create_lex_with_dblq(pool, "\"hello world\"", 0);
    assert(lex != NULL);

    mln_test_struct_t *ts = mln_test_token(lex);
    assert(ts != NULL);
    assert(ts->type == TEST_TK_STRING);
    assert(ts->text->len == 11);
    assert(memcmp(ts->text->data, "hello world", 11) == 0);
    mln_test_free(ts);

    mln_lex_destroy(lex);
    mln_alloc_destroy(pool);
    PASS();
}

/* 12. stepback: read a token, verify next token is correct */
static void test_stepback(void)
{
    mln_alloc_t *pool = mln_alloc_init(NULL, 0);
    assert(pool != NULL);
    mln_lex_t *lex = create_lex_from_buf(pool, "ab cd", 0);
    assert(lex != NULL);

    /* Token "ab" is read; then internally stepback is used for the space.
       Verify both tokens come through correctly. */
    mln_test_struct_t *ts;
    ts = mln_test_token(lex);
    assert(ts != NULL && ts->type == TEST_TK_ID);
    assert(ts->text->len == 2 && memcmp(ts->text->data, "ab", 2) == 0);
    mln_test_free(ts);

    ts = mln_test_token(lex);
    assert(ts != NULL && ts->type == TEST_TK_ID);
    assert(ts->text->len == 2 && memcmp(ts->text->data, "cd", 2) == 0);
    mln_test_free(ts);

    mln_lex_destroy(lex);
    mln_alloc_destroy(pool);
    PASS();
}

/* 13. putchar / getchar operations */
static void test_putchar_getchar(void)
{
    mln_alloc_t *pool = mln_alloc_init(NULL, 0);
    assert(pool != NULL);
    mln_lex_t *lex = create_lex_from_buf(pool, "X", 0);
    assert(lex != NULL);

    /* Use getchar to read 'X', putchar to store it, then verify via token */
    char c = mln_lex_getchar(lex);
    assert(c == 'X');
    assert(mln_lex_putchar(lex, c) == 0);

    /* Now stepback + read token should yield EOF (we consumed 'X' manually) */
    mln_test_struct_t *ts = mln_test_token(lex);
    assert(ts != NULL && ts->type == TEST_TK_EOF);
    mln_test_free(ts);

    mln_lex_destroy(lex);
    mln_alloc_destroy(pool);
    PASS();
}

/* 14. strerror */
static void test_strerror(void)
{
    mln_alloc_t *pool = mln_alloc_init(NULL, 0);
    assert(pool != NULL);
    mln_lex_t *lex = create_lex_from_buf(pool, "ok", 0);
    assert(lex != NULL);

    mln_lex_error_set(lex, MLN_LEX_EINVCHAR);
    char *msg = mln_lex_strerror(lex);
    assert(msg != NULL);
    assert(strlen(msg) > 0);

    mln_lex_destroy(lex);
    mln_alloc_destroy(pool);
    PASS();
}

/* 15. Push buffer stream */
static void test_push_buf_stream(void)
{
    mln_alloc_t *pool = mln_alloc_init(NULL, 0);
    assert(pool != NULL);
    mln_lex_t *lex = create_lex_from_buf(pool, "x", 0);
    assert(lex != NULL);

    /* Push an additional buffer stream; streams are consumed as continuous input */
    mln_string_t extra = mln_string("y ");
    int ret = mln_lex_push_input_buf_stream(lex, &extra);
    assert(ret == 0);

    mln_test_struct_t *ts;

    /* "y " is consumed first (from pushed stream), producing token "y" */
    ts = mln_test_token(lex);
    assert(ts != NULL && ts->type == TEST_TK_ID);
    assert(ts->text->len == 1 && ts->text->data[0] == 'y');
    mln_test_free(ts);

    /* Then "x" from original stream */
    ts = mln_test_token(lex);
    assert(ts != NULL && ts->type == TEST_TK_ID);
    assert(ts->text->len == 1 && ts->text->data[0] == 'x');
    mln_test_free(ts);

    ts = mln_test_token(lex);
    assert(ts != NULL && ts->type == TEST_TK_EOF);
    mln_test_free(ts);

    mln_lex_destroy(lex);
    mln_alloc_destroy(pool);
    PASS();
}

/* 16. lex_result_clean */
static void test_lex_result_clean(void)
{
    mln_alloc_t *pool = mln_alloc_init(NULL, 0);
    assert(pool != NULL);
    mln_lex_t *lex = create_lex_from_buf(pool, "abc", 0);
    assert(lex != NULL);

    /* Manually put some chars then clean */
    assert(mln_lex_putchar(lex, 'z') == 0);
    assert(mln_lex_putchar(lex, 'z') == 0);
    mln_lex_result_clean(lex);

    /* After clean, a normal token should still work */
    mln_test_struct_t *ts = mln_test_token(lex);
    assert(ts != NULL && ts->type == TEST_TK_ID);
    assert(ts->text->len == 3 && memcmp(ts->text->data, "abc", 3) == 0);
    mln_test_free(ts);

    mln_lex_destroy(lex);
    mln_alloc_destroy(pool);
    PASS();
}

/* 17. Preprocess: #define macro expansion */
static void test_preprocess_define(void)
{
    mln_alloc_t *pool = mln_alloc_init(NULL, 0);
    assert(pool != NULL);
    mln_lex_t *lex = create_lex_from_buf(pool, "#define FOO 42\n#FOO", 1);
    assert(lex != NULL);

    /* After #define FOO 42, #FOO should expand to 42 (a DEC token) */
    mln_test_struct_t *ts;
    ts = mln_test_token(lex);
    assert(ts != NULL && ts->type == TEST_TK_DEC);
    assert(ts->text->len == 2 && memcmp(ts->text->data, "42", 2) == 0);
    mln_test_free(ts);

    ts = mln_test_token(lex);
    assert(ts != NULL && ts->type == TEST_TK_EOF);
    mln_test_free(ts);

    mln_lex_destroy(lex);
    mln_alloc_destroy(pool);
    PASS();
}

/* 18. Preprocess: #if / #else / #endif */
static void test_preprocess_if(void)
{
    mln_alloc_t *pool = mln_alloc_init(NULL, 0);
    assert(pool != NULL);
    /* #define X\n#if X\naaa\n#else\nbbb\n#endif */
    mln_lex_t *lex = create_lex_from_buf(pool,
        "#define X\n#if X\naaa\n#else\nbbb\n#endif\n", 1);
    assert(lex != NULL);

    mln_test_struct_t *ts;
    ts = mln_test_token(lex);
    assert(ts != NULL && ts->type == TEST_TK_ID);
    assert(ts->text->len == 3 && memcmp(ts->text->data, "aaa", 3) == 0);
    mln_test_free(ts);

    ts = mln_test_token(lex);
    assert(ts != NULL && ts->type == TEST_TK_EOF);
    mln_test_free(ts);

    mln_lex_destroy(lex);
    mln_alloc_destroy(pool);
    PASS();
}

/* 19. Preprocess: #undef */
static void test_preprocess_undef(void)
{
    mln_alloc_t *pool = mln_alloc_init(NULL, 0);
    assert(pool != NULL);
    /* Define Y, then undef Y, then #if !Y should be true */
    mln_lex_t *lex = create_lex_from_buf(pool,
        "#define Y\n#undef Y\n#if !Y\nvisible\n#endif\n", 1);
    assert(lex != NULL);

    mln_test_struct_t *ts;
    ts = mln_test_token(lex);
    assert(ts != NULL && ts->type == TEST_TK_ID);
    assert(ts->text->len == 7 && memcmp(ts->text->data, "visible", 7) == 0);
    mln_test_free(ts);

    ts = mln_test_token(lex);
    assert(ts != NULL && ts->type == TEST_TK_EOF);
    mln_test_free(ts);

    mln_lex_destroy(lex);
    mln_alloc_destroy(pool);
    PASS();
}

/* 20. Snapshot record/apply */
static void test_snapshot(void)
{
    mln_alloc_t *pool = mln_alloc_init(NULL, 0);
    assert(pool != NULL);
    mln_lex_t *lex = create_lex_from_buf(pool, "abc def", 0);
    assert(lex != NULL);

    /* Record snapshot at start */
    mln_lex_off_t snap = mln_lex_snapshot_record(lex);

    mln_test_struct_t *ts;
    ts = mln_test_token(lex);
    assert(ts != NULL && ts->type == TEST_TK_ID);
    assert(memcmp(ts->text->data, "abc", 3) == 0);
    mln_test_free(ts);

    /* Apply snapshot to go back */
    mln_lex_snapshot_apply(lex, snap);

    /* Should re-read "abc" again */
    ts = mln_test_token(lex);
    assert(ts != NULL && ts->type == TEST_TK_ID);
    assert(memcmp(ts->text->data, "abc", 3) == 0);
    mln_test_free(ts);

    mln_lex_destroy(lex);
    mln_alloc_destroy(pool);
    PASS();
}

/* 21. Empty input */
static void test_empty_input(void)
{
    mln_alloc_t *pool = mln_alloc_init(NULL, 0);
    assert(pool != NULL);
    mln_lex_t *lex = create_lex_from_buf(pool, "", 0);
    assert(lex != NULL);

    mln_test_struct_t *ts = mln_test_token(lex);
    assert(ts != NULL && ts->type == TEST_TK_EOF);
    mln_test_free(ts);

    mln_lex_destroy(lex);
    mln_alloc_destroy(pool);
    PASS();
}

/* 22. Long identifier (tests buffer growth) */
static void test_long_identifier(void)
{
    mln_alloc_t *pool = mln_alloc_init(NULL, 0);
    assert(pool != NULL);

    /* Build a 5000-char identifier */
    char buf[5001];
    memset(buf, 'a', 5000);
    buf[5000] = '\0';

    mln_lex_t *lex = create_lex_from_buf(pool, buf, 0);
    assert(lex != NULL);

    mln_test_struct_t *ts = mln_test_token(lex);
    assert(ts != NULL && ts->type == TEST_TK_ID);
    assert(ts->text->len == 5000);
    mln_test_free(ts);

    mln_lex_destroy(lex);
    mln_alloc_destroy(pool);
    PASS();
}

/* 23. Mixed tokens: complex expression */
static void test_mixed_tokens(void)
{
    mln_alloc_t *pool = mln_alloc_init(NULL, 0);
    assert(pool != NULL);
    mln_lex_t *lex = create_lex_from_buf(pool, "foo = 0xFF + 3.14 ; on ( off )", 0);
    assert(lex != NULL);

    enum mln_test_enum expect[] = {
        TEST_TK_ID, TEST_TK_EQUAL, TEST_TK_HEX, TEST_TK_PLUS,
        TEST_TK_REAL, TEST_TK_SEMIC, TEST_TK_ON, TEST_TK_LPAR,
        TEST_TK_OFF, TEST_TK_RPAR, TEST_TK_EOF
    };
    int i;
    for (i = 0; i < (int)(sizeof(expect)/sizeof(expect[0])); i++) {
        mln_test_struct_t *ts = mln_test_token(lex);
        assert(ts != NULL);
        assert(ts->type == expect[i]);
        mln_test_free(ts);
    }

    mln_lex_destroy(lex);
    mln_alloc_destroy(pool);
    PASS();
}

/* 24. Error: invalid hex "0x" with no digits after */
static void test_error_invalid_hex(void)
{
    mln_alloc_t *pool = mln_alloc_init(NULL, 0);
    assert(pool != NULL);
    mln_lex_t *lex = create_lex_from_buf(pool, "0x", 0);
    assert(lex != NULL);

    mln_test_struct_t *ts = mln_test_token(lex);
    /* Should return NULL (error) for invalid hex */
    assert(ts == NULL);
    assert(lex->error == MLN_LEX_EINVHEX);

    mln_lex_destroy(lex);
    mln_alloc_destroy(pool);
    PASS();
}

/* 25. Error: invalid octal "09" */
static void test_error_invalid_oct(void)
{
    mln_alloc_t *pool = mln_alloc_init(NULL, 0);
    assert(pool != NULL);
    mln_lex_t *lex = create_lex_from_buf(pool, "09", 0);
    assert(lex != NULL);

    mln_test_struct_t *ts = mln_test_token(lex);
    assert(ts == NULL);
    assert(lex->error == MLN_LEX_EINVOCT);

    mln_lex_destroy(lex);
    mln_alloc_destroy(pool);
    PASS();
}

/* 26. Underscore only "_" is special char (UNDER) */
static void test_underscore_only(void)
{
    mln_alloc_t *pool = mln_alloc_init(NULL, 0);
    assert(pool != NULL);
    mln_lex_t *lex = create_lex_from_buf(pool, "_ _x", 0);
    assert(lex != NULL);

    mln_test_struct_t *ts;

    /* Single "_" is UNDER special char */
    ts = mln_test_token(lex);
    assert(ts != NULL && ts->type == TEST_TK_UNDER);
    mln_test_free(ts);

    /* "_x" is an identifier */
    ts = mln_test_token(lex);
    assert(ts != NULL && ts->type == TEST_TK_ID);
    assert(ts->text->len == 2 && memcmp(ts->text->data, "_x", 2) == 0);
    mln_test_free(ts);

    mln_lex_destroy(lex);
    mln_alloc_destroy(pool);
    PASS();
}

/* 27. Benchmark */
static void test_benchmark(void)
{
    mln_alloc_t *pool = mln_alloc_init(NULL, 0);
    assert(pool != NULL);

    /* Build a large input buffer */
    const char *line = "abc_123 = 0xFF + 3.14;\n";
    size_t line_len = strlen(line);
    int repeat = 2000;
    size_t total_len = line_len * (size_t)repeat;
    char *big = (char *)malloc(total_len + 1);
    assert(big != NULL);
    {
        size_t off = 0;
        int i;
        for (i = 0; i < repeat; i++) {
            memcpy(big + off, line, line_len);
            off += line_len;
        }
        big[total_len] = '\0';
    }

    int iters = 50;
    long total_tokens = 0;
    struct timespec t0, t1;

    clock_gettime(CLOCK_MONOTONIC, &t0);
    {
        int it;
        for (it = 0; it < iters; it++) {
            mln_lex_t *lex = create_lex_from_buf(pool, big, 0);
            assert(lex != NULL);
            while (1) {
                mln_test_struct_t *ts = mln_test_token(lex);
                if (ts == NULL) break;
                if (ts->type == TEST_TK_EOF) { mln_test_free(ts); break; }
                total_tokens++;
                mln_test_free(ts);
            }
            mln_lex_destroy(lex);
        }
    }
    clock_gettime(CLOCK_MONOTONIC, &t1);

    double secs = (double)(t1.tv_sec - t0.tv_sec) + (double)(t1.tv_nsec - t0.tv_nsec) / 1e9;
    if (secs > 0)
        printf("  Lex benchmark: %.0f tokens/sec\n", (double)total_tokens / secs);

    free(big);
    mln_alloc_destroy(pool);
    PASS();
}

/* 28. Stability: 10000 iterations of tokenize/free */
static void test_stability(void)
{
    int i;
    for (i = 0; i < 10000; i++) {
        mln_alloc_t *pool = mln_alloc_init(NULL, 0);
        assert(pool != NULL);
        mln_lex_t *lex = create_lex_from_buf(pool, "on = 42 + 0xFF; off", 0);
        assert(lex != NULL);
        while (1) {
            mln_test_struct_t *ts = mln_test_token(lex);
            if (ts == NULL) break;
            if (ts->type == TEST_TK_EOF) { mln_test_free(ts); break; }
            mln_test_free(ts);
        }
        mln_lex_destroy(lex);
        mln_alloc_destroy(pool);
    }
    PASS();
}

/* ================================================================== */
/* main                                                                */
/* ================================================================== */

int main(int argc, char *argv[])
{
    (void)argc; (void)argv;

    printf("Lex test suite:\n");

    test_basic_tokenize();
    test_keywords();
    test_numbers_dec();
    test_numbers_hex();
    test_numbers_oct();
    test_numbers_real();
    test_special_chars();
    test_identifiers();
    test_whitespace_newline();
    test_eof();
    test_custom_hooks();
    test_stepback();
    test_putchar_getchar();
    test_strerror();
    test_push_buf_stream();
    test_lex_result_clean();
    test_preprocess_define();
    test_preprocess_if();
    test_preprocess_undef();
    test_snapshot();
    test_empty_input();
    test_long_identifier();
    test_mixed_tokens();
    test_error_invalid_hex();
    test_error_invalid_oct();
    test_underscore_only();
    test_benchmark();
    test_stability();

    printf("All %d tests passed.\n", test_nr);
    return 0;
}
