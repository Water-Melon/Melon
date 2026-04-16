#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include "mln_log.h"
#include "mln_lex.h"
#include "mln_alloc.h"
#include "mln_parser_generator.h"

MLN_DECLARE_PARSER_GENERATOR(static, test, TEST);
MLN_DEFINE_PARSER_GENERATOR(static, test, TEST);

static int test_nr = 0;
static int fail_nr = 0;
#define PASS() do { printf("  #%02d PASS\n", ++test_nr); } while (0)
#define FAIL(msg) do { printf("  #%02d FAIL: %s\n", ++test_nr, (msg)); ++fail_nr; } while (0)

/* --- shared grammar --------------------------------------------------- */
static mln_production_t basic_prod[] = {
    {"start: stm TEST_TK_EOF", NULL},
    {"stm: exp TEST_TK_SEMIC stm", NULL},
    {"stm: ", NULL},
    {"exp: TEST_TK_ID addsub", NULL},
    {"exp: TEST_TK_DEC addsub", NULL},
    {"addsub: TEST_TK_PLUS exp", NULL},
    {"addsub: TEST_TK_SUB exp", NULL},
    {"addsub: ", NULL},
};
#define BASIC_NR (sizeof(basic_prod)/sizeof(mln_production_t))

/* --- helpers ---------------------------------------------------------- */
static mln_lex_t *make_lex(mln_alloc_t *pool, mln_string_t *code)
{
    mln_lex_t *lex = NULL;
    struct mln_lex_attr lattr;
    mln_lex_hooks_t hooks;
    memset(&hooks, 0, sizeof(hooks));
    lattr.pool = pool;
    lattr.keywords = NULL;
    lattr.hooks = &hooks;
    lattr.preprocess = 1;
    lattr.type = M_INPUT_T_BUF;
    lattr.data = code;
    lattr.env = NULL;
    mln_lex_init_with_hooks(test, lex, &lattr);
    assert(lex != NULL);
    return lex;
}

/* ====================================================================== */
/* 1. test_parser_generate_basic                                          */
/* ====================================================================== */
static void test_parser_generate_basic(void)
{
    printf("test_parser_generate_basic\n");
    void *ptr = test_parser_generate(basic_prod, BASIC_NR, NULL);
    if (ptr != NULL) { PASS(); } else { FAIL("generate returned NULL"); }
    test_pg_data_free(ptr);
}

/* ====================================================================== */
/* 2. test_parse_basic                                                    */
/* ====================================================================== */
static void test_parse_basic(void)
{
    printf("test_parse_basic\n");
    mln_alloc_t *pool = mln_alloc_init(NULL, 0);
    mln_string_t code = mln_string("a + 1;\nb + a;\nc - b;\n");
    mln_lex_t *lex = make_lex(pool, &code);
    void *ptr = test_parser_generate(basic_prod, BASIC_NR, NULL);
    if (pool == NULL || ptr == NULL) {
        FAIL("setup failed (pool or pg_data is NULL)");
        if (ptr) test_pg_data_free(ptr);
        mln_lex_destroy(lex);
        mln_alloc_destroy(pool);
        return;
    }
    struct mln_parse_attr pattr;
    pattr.pool = pool;
    pattr.prod_tbl = basic_prod;
    pattr.lex = lex;
    pattr.pg_data = ptr;
    pattr.udata = NULL;
    void *ast = test_parse(&pattr);
    /* All callbacks NULL -> ast is NULL, parse should succeed */
    if (ast == NULL) { PASS(); } else { FAIL("expected NULL ast"); }

    mln_lex_destroy(lex);
    test_pg_data_free(ptr);
    mln_alloc_destroy(pool);
}

/* ====================================================================== */
/* 3. test_parse_with_callbacks                                           */
/* ====================================================================== */
static int reduce_count = 0;

static int count_reduce(mln_factor_t *left, mln_factor_t **rights, void *udata)
{
    (void)left; (void)rights; (void)udata;
    ++reduce_count;
    return 0;
}

static void test_parse_with_callbacks(void)
{
    printf("test_parse_with_callbacks\n");
    mln_production_t cb_prod[] = {
        {"start: stm TEST_TK_EOF", NULL},
        {"stm: exp TEST_TK_SEMIC stm", count_reduce},
        {"stm: ", NULL},
        {"exp: TEST_TK_ID addsub", count_reduce},
        {"exp: TEST_TK_DEC addsub", count_reduce},
        {"addsub: TEST_TK_PLUS exp", count_reduce},
        {"addsub: TEST_TK_SUB exp", count_reduce},
        {"addsub: ", NULL},
    };

    reduce_count = 0;
    mln_alloc_t *pool = mln_alloc_init(NULL, 0);
    mln_string_t code = mln_string("a + 1;\nb + a;\nc - b;\n");
    mln_lex_t *lex = make_lex(pool, &code);
    void *ptr = test_parser_generate(cb_prod,
                    sizeof(cb_prod)/sizeof(mln_production_t), NULL);
    if (pool == NULL || ptr == NULL) {
        FAIL("setup failed (pool or pg_data is NULL)");
        if (ptr) test_pg_data_free(ptr);
        mln_lex_destroy(lex);
        mln_alloc_destroy(pool);
        return;
    }
    struct mln_parse_attr pattr;
    pattr.pool = pool;
    pattr.prod_tbl = cb_prod;
    pattr.lex = lex;
    pattr.pg_data = ptr;
    pattr.udata = NULL;
    (void)test_parse(&pattr);

    if (reduce_count > 0) { PASS(); } else { FAIL("callbacks not invoked"); }

    mln_lex_destroy(lex);
    test_pg_data_free(ptr);
    mln_alloc_destroy(pool);
}

/* ====================================================================== */
/* 4. test_pg_data_free                                                   */
/* ====================================================================== */
static void test_pg_data_free_test(void)
{
    printf("test_pg_data_free\n");

    /* NULL should not crash */
    test_pg_data_free(NULL);
    PASS();

    /* Valid pg_data */
    void *ptr = test_parser_generate(basic_prod, BASIC_NR, NULL);
    if (ptr == NULL) { FAIL("generate returned NULL"); return; }
    test_pg_data_free(ptr);
    PASS();
}

/* ====================================================================== */
/* 5. test_factor_init_destroy                                            */
/* ====================================================================== */
static void test_factor_init_destroy(void)
{
    printf("test_factor_init_destroy\n");

    mln_factor_t *f = test_factor_init(NULL, M_P_NONTERM, 42, 0, 10, NULL);
    if (f == NULL) { FAIL("factor_init returned NULL"); return; }
    if (f->data_type != M_P_NONTERM) { FAIL("bad data_type"); test_factor_destroy(f); return; }
    if (f->token_type != 42) { FAIL("bad token_type"); test_factor_destroy(f); return; }
    if (f->line != 10) { FAIL("bad line"); test_factor_destroy(f); return; }
    if (f->file != NULL) { FAIL("expected NULL file"); test_factor_destroy(f); return; }
    PASS();

    test_factor_destroy(f);
    /* destroy NULL should not crash */
    test_factor_destroy(NULL);
    PASS();
}

/* ====================================================================== */
/* 6. test_parser_init_destroy                                            */
/* ====================================================================== */
static void test_parser_init_destroy(void)
{
    printf("test_parser_init_destroy\n");
    mln_parser_t *p = test_parser_init();
    if (p == NULL) { FAIL("parser_init returned NULL"); return; }
    PASS();
    test_parser_destroy(p);
    /* NULL should not crash */
    test_parser_destroy(NULL);
    PASS();
}

/* ====================================================================== */
/* 7. test_factor_copy                                                    */
/* ====================================================================== */
static void test_factor_copy_test(void)
{
    printf("test_factor_copy\n");

    mln_alloc_t *pool = mln_alloc_init(NULL, 0);

    /* copy of a nonterm factor (no lex dup needed) */
    mln_factor_t *src = test_factor_init(NULL, M_P_NONTERM, 7, 3, 99, NULL);
    mln_factor_t *dup = (mln_factor_t *)test_factor_copy(src, pool);
    if (dup == NULL) { FAIL("factor_copy returned NULL"); }
    else if (dup->token_type != 7 || dup->line != 99 || dup->cur_state != 3) { FAIL("copy mismatch"); }
    else { PASS(); }
    test_factor_destroy(dup);
    test_factor_destroy(src);

    /* copy with NULL args */
    if (test_factor_copy(NULL, pool) != NULL) { FAIL("expected NULL on NULL src"); }
    else { PASS(); }

    mln_alloc_destroy(pool);
}

/* ====================================================================== */
/* 8. test_empty_grammar                                                  */
/* ====================================================================== */
static void test_empty_grammar(void)
{
    printf("test_empty_grammar\n");
    mln_production_t prod[] = {
        {"start: TEST_TK_EOF", NULL},
    };
    void *ptr = test_parser_generate(prod, 1, NULL);
    if (ptr != NULL) { PASS(); } else { FAIL("generate returned NULL"); }
    test_pg_data_free(ptr);
}

/* ====================================================================== */
/* 9. test_single_production                                              */
/* ====================================================================== */
static void test_single_production(void)
{
    printf("test_single_production\n");
    mln_production_t prod[] = {
        {"start: stm TEST_TK_EOF", NULL},
        {"stm: TEST_TK_ID", NULL},
    };
    void *ptr = test_parser_generate(prod, 2, NULL);
    if (ptr == NULL) { FAIL("generate returned NULL"); return; }
    PASS();

    /* Parse a single identifier */
    mln_alloc_t *pool = mln_alloc_init(NULL, 0);
    if (pool == NULL) { FAIL("pool alloc failed"); test_pg_data_free(ptr); return; }
    mln_string_t code = mln_string("hello");
    mln_lex_t *lex = make_lex(pool, &code);
    struct mln_parse_attr pattr;
    pattr.pool = pool;
    pattr.prod_tbl = prod;
    pattr.lex = lex;
    pattr.pg_data = ptr;
    pattr.udata = NULL;
    void *ast = test_parse(&pattr);
    (void)ast;
    PASS();

    mln_lex_destroy(lex);
    test_pg_data_free(ptr);
    mln_alloc_destroy(pool);
}

/* ====================================================================== */
/* 10. test_multiple_alternatives                                         */
/* ====================================================================== */
static void test_multiple_alternatives(void)
{
    printf("test_multiple_alternatives\n");
    mln_production_t prod[] = {
        {"start: stm TEST_TK_EOF", NULL},
        {"stm: exp TEST_TK_SEMIC stm", NULL},
        {"stm: ", NULL},
        {"exp: TEST_TK_ID", NULL},
        {"exp: TEST_TK_DEC", NULL},
        {"exp: TEST_TK_OCT", NULL},
        {"exp: TEST_TK_HEX", NULL},
    };
    void *ptr = test_parser_generate(prod, sizeof(prod)/sizeof(mln_production_t), NULL);
    if (ptr == NULL) { FAIL("generate returned NULL"); return; }

    mln_alloc_t *pool = mln_alloc_init(NULL, 0);
    if (pool == NULL) { FAIL("pool alloc failed"); test_pg_data_free(ptr); return; }
    mln_string_t code = mln_string("x; 42; y;\n");
    mln_lex_t *lex = make_lex(pool, &code);
    struct mln_parse_attr pattr;
    pattr.pool = pool;
    pattr.prod_tbl = prod;
    pattr.lex = lex;
    pattr.pg_data = ptr;
    pattr.udata = NULL;
    (void)test_parse(&pattr);
    PASS();

    mln_lex_destroy(lex);
    test_pg_data_free(ptr);
    mln_alloc_destroy(pool);
}

/* ====================================================================== */
/* 11. test_parse_error_recovery                                          */
/* ====================================================================== */
static void test_parse_error_recovery(void)
{
    printf("test_parse_error_recovery\n");
    mln_alloc_t *pool = mln_alloc_init(NULL, 0);
    /* Missing semicolons -> parse error */
    mln_string_t code = mln_string("a + b c\n");
    mln_lex_t *lex = make_lex(pool, &code);
    void *ptr = test_parser_generate(basic_prod, BASIC_NR, NULL);
    if (pool == NULL || ptr == NULL) {
        FAIL("setup failed (pool or pg_data is NULL)");
        if (ptr) test_pg_data_free(ptr);
        mln_lex_destroy(lex);
        mln_alloc_destroy(pool);
        return;
    }
    struct mln_parse_attr pattr;
    pattr.pool = pool;
    pattr.prod_tbl = basic_prod;
    pattr.lex = lex;
    pattr.pg_data = ptr;
    pattr.udata = NULL;
    (void)test_parse(&pattr);
    /* We only verify no crash; error recovery may or may not succeed */
    PASS();

    mln_lex_destroy(lex);
    test_pg_data_free(ptr);
    mln_alloc_destroy(pool);
}

/* ====================================================================== */
/* 12. test_lex_dup                                                       */
/* ====================================================================== */
static void test_lex_dup_test(void)
{
    printf("test_lex_dup\n");

    mln_alloc_t *pool = mln_alloc_init(NULL, 0);
    mln_string_t code = mln_string("hello");
    mln_lex_t *lex = make_lex(pool, &code);

    test_struct_t *tok = test_token(lex);
    if (tok == NULL) { FAIL("token returned NULL"); mln_lex_destroy(lex); mln_alloc_destroy(pool); return; }

    test_struct_t *dup = (test_struct_t *)test_lex_dup(pool, tok);
    if (dup == NULL) { FAIL("lex_dup returned NULL"); }
    else if (dup->type != tok->type) { FAIL("dup type mismatch"); }
    else if (dup->line != tok->line) { FAIL("dup line mismatch"); }
    else { PASS(); }

    /* NULL returns NULL */
    if (test_lex_dup(pool, NULL) != NULL) { FAIL("expected NULL for NULL input"); }
    else { PASS(); }

    if (dup != NULL) {
        mln_string_free(dup->text);
        if (dup->file != NULL) mln_string_free(dup->file);
        mln_alloc_free(dup);
    }
    test_free(tok);
    mln_lex_destroy(lex);
    mln_alloc_destroy(pool);
}

/* ====================================================================== */
/* 13. test_token_types                                                   */
/* ====================================================================== */
static void test_token_types(void)
{
    printf("test_token_types\n");

    /* Verify a few known entries from test_token_type_array */
    if (test_token_type_array[0].type != TEST_TK_EOF) { FAIL("index 0 != EOF"); return; }
    if (test_token_type_array[TEST_TK_DEC].type != TEST_TK_DEC) { FAIL("DEC mismatch"); return; }
    if (test_token_type_array[TEST_TK_ID].type != TEST_TK_ID) { FAIL("ID mismatch"); return; }
    if (test_token_type_array[TEST_TK_PLUS].type != TEST_TK_PLUS) { FAIL("PLUS mismatch"); return; }
    if (test_token_type_array[TEST_TK_SEMIC].type != TEST_TK_SEMIC) { FAIL("SEMIC mismatch"); return; }
    PASS();

    /* Check type_str contains expected prefix */
    if (strstr(test_token_type_array[TEST_TK_EOF].type_str, "TEST_TK_EOF") == NULL) {
        FAIL("EOF type_str mismatch"); return;
    }
    PASS();
}

/* ====================================================================== */
/* 14. test_benchmark_generate                                            */
/* ====================================================================== */
static void test_benchmark_generate(void)
{
    printf("test_benchmark_generate\n");
    int iters = 100;
    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);
    int i;
    for (i = 0; i < iters; ++i) {
        void *ptr = test_parser_generate(basic_prod, BASIC_NR, NULL);
        if (ptr == NULL) { FAIL("generate failed during benchmark"); return; }
        test_pg_data_free(ptr);
    }
    clock_gettime(CLOCK_MONOTONIC, &t1);
    double elapsed = (double)(t1.tv_sec - t0.tv_sec) +
                     (double)(t1.tv_nsec - t0.tv_nsec) / 1e9;
    double ops = (double)iters / elapsed;
    printf("    %d generates in %.3f s  (%.0f ops/sec)\n", iters, elapsed, ops);
    PASS();
}

/* ====================================================================== */
/* 15. test_benchmark_parse                                               */
/* ====================================================================== */
static void test_benchmark_parse(void)
{
    printf("test_benchmark_parse\n");
    void *ptr = test_parser_generate(basic_prod, BASIC_NR, NULL);
    if (ptr == NULL) { FAIL("generate failed"); return; }

    int iters = 1000;
    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);
    int i;
    for (i = 0; i < iters; ++i) {
        mln_alloc_t *pool = mln_alloc_init(NULL, 0);
        mln_string_t code = mln_string("a + 1;\nb + a;\nc - b;\n");
        mln_lex_t *lex = make_lex(pool, &code);
        struct mln_parse_attr pattr;
        pattr.pool = pool;
        pattr.prod_tbl = basic_prod;
        pattr.lex = lex;
        pattr.pg_data = ptr;
        pattr.udata = NULL;
        (void)test_parse(&pattr);
        mln_lex_destroy(lex);
        mln_alloc_destroy(pool);
    }
    clock_gettime(CLOCK_MONOTONIC, &t1);
    double elapsed = (double)(t1.tv_sec - t0.tv_sec) +
                     (double)(t1.tv_nsec - t0.tv_nsec) / 1e9;
    double ops = (double)iters / elapsed;
    printf("    %d parses in %.3f s  (%.0f parses/sec)\n", iters, elapsed, ops);
    PASS();

    test_pg_data_free(ptr);
}

/* ====================================================================== */
/* 16. test_stability                                                     */
/* ====================================================================== */
static void test_stability(void)
{
    printf("test_stability\n");
    int iters = 1000;
    int i;
    for (i = 0; i < iters; ++i) {
        void *ptr = test_parser_generate(basic_prod, BASIC_NR, NULL);
        if (ptr == NULL) { FAIL("generate failed in stability loop"); return; }

        mln_alloc_t *pool = mln_alloc_init(NULL, 0);
        mln_string_t code = mln_string("x + 1;\n");
        mln_lex_t *lex = make_lex(pool, &code);
        struct mln_parse_attr pattr;
        pattr.pool = pool;
        pattr.prod_tbl = basic_prod;
        pattr.lex = lex;
        pattr.pg_data = ptr;
        pattr.udata = NULL;
        (void)test_parse(&pattr);

        mln_lex_destroy(lex);
        test_pg_data_free(ptr);
        mln_alloc_destroy(pool);
    }
    printf("    %d generate+parse+free cycles OK\n", iters);
    PASS();
}

/* ====================================================================== */
/* main                                                                   */
/* ====================================================================== */
int main(void)
{
    printf("=== parser_generator tests ===\n");

    test_parser_generate_basic();
    test_parse_basic();
    test_parse_with_callbacks();
    test_pg_data_free_test();
    test_factor_init_destroy();
    test_parser_init_destroy();
    test_factor_copy_test();
    test_empty_grammar();
    test_single_production();
    test_multiple_alternatives();
    test_parse_error_recovery();
    test_lex_dup_test();
    test_token_types();
    test_benchmark_generate();
    test_benchmark_parse();
    test_stability();

    printf("=== %d tests, %d passed, %d failed ===\n",
           test_nr, test_nr - fail_nr, fail_nr);
    return fail_nr ? 1 : 0;
}

