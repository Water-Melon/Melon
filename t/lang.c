/*
 * Comprehensive Melang (mln_lang) test suite.
 *
 * Covers every language feature supported by the interpreter:
 *   - Integer / real / bool / nil / string literals
 *   - Arithmetic operators: + - * / %
 *   - Bitwise operators: | & ^ ~ << >>
 *   - Logical operators (short-circuit): || &&
 *   - Comparison operators: == != < <= > >=
 *   - Unary: ! -
 *   - All assignment forms: = += -= *= /= %= |= &= ^= <<= >>= on locals, properties, and indices
 *   - Prefix / suffix ++ -- on locals, globals, properties, and indices
 *   - if / else / fi
 *   - while loop (break / continue)
 *   - for loop (break / continue)
 *   - switch / case / default
 *   - goto / label
 *   - Comma expression
 *   - User-defined functions (@F(...){...})
 *   - Self-recursion (Fibonacci)
 *   - Multiple-argument functions
 *   - Closures (function capturing outer variable)
 *   - Sets (objects / classes) with member access
 *   - Array literals and indexing
 *   - Nested function calls (cross-function, non-self)
 *   - Global variables visible across functions
 *   - String concatenation via +
 *   - Dump() built-in (smoke test)
 *
 * Each test case runs a script via mln_lang_job_new, waits for completion,
 * then validates ctx->ret_var against the expected result.
 *
 * Compile:
 *   cc -O2 -Wall -Iinclude -o /tmp/test_lang t/lang.c \
 *       -Llib/ -lmelon_static -lpthread -ldl
 */
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include "mln_lang.h"
#include "mln_event.h"

/* =========================================================
 * Globals shared by the test harness
 * ========================================================= */

static int fds[2];
static int g_n_pass = 0;
static int g_n_fail = 0;

static int lang_signal(mln_lang_t *lang)
{
    return mln_event_fd_set(mln_lang_event_get(lang), fds[0],
                            M_EV_SEND | M_EV_ONESHOT, M_EV_UNLIMITED,
                            lang, mln_lang_launcher_get(lang));
}

static int lang_clear(mln_lang_t *lang)
{
    return mln_event_fd_set(mln_lang_event_get(lang), fds[0],
                            M_EV_CLR, M_EV_UNLIMITED, NULL, NULL);
}

/* Per-test expected-result descriptor */
typedef enum {
    EXPECT_INT,
    EXPECT_REAL,
    EXPECT_BOOL_TRUE,
    EXPECT_BOOL_FALSE,
    EXPECT_NIL,
    EXPECT_STRING,
    EXPECT_NONE   /* script has no return value; just checks it completes */
} expect_type_t;

typedef struct {
    mln_event_t  *ev;
    const char   *name;
    expect_type_t etype;
    mln_s64_t     eint;
    double        ereal;
    const char   *estr;
} test_ctx_t;

static void test_return_handler(mln_lang_ctx_t *ctx)
{
    test_ctx_t  *tc  = (test_ctx_t *)mln_lang_ctx_data_get(ctx);
    mln_lang_var_t *rv = ctx->ret_var;
    int ok = 0;

    switch (tc->etype) {
        case EXPECT_NONE:
            ok = 1;
            break;
        case EXPECT_INT:
            ok = (rv != NULL && rv->val != NULL &&
                  rv->val->type == M_LANG_VAL_TYPE_INT &&
                  rv->val->data.i == tc->eint);
            if (!ok) {
                if (rv == NULL || rv->val == NULL)
                    fprintf(stderr, "  FAIL [%s]: ret_var is NULL\n", tc->name);
                else if (rv->val->type != M_LANG_VAL_TYPE_INT)
                    fprintf(stderr, "  FAIL [%s]: type=%d expected INT\n",
                            tc->name, rv->val->type);
                else
                    fprintf(stderr, "  FAIL [%s]: got %lld expected %lld\n",
                            tc->name, (long long)rv->val->data.i,
                            (long long)tc->eint);
            }
            break;
        case EXPECT_REAL: {
            double got = 0.0;
            if (rv && rv->val && rv->val->type == M_LANG_VAL_TYPE_REAL)
                got = rv->val->data.f;
            ok = (rv != NULL && rv->val != NULL &&
                  rv->val->type == M_LANG_VAL_TYPE_REAL &&
                  fabs(got - tc->ereal) < 1e-9);
            if (!ok)
                fprintf(stderr, "  FAIL [%s]: real mismatch\n", tc->name);
            break;
        }
        case EXPECT_BOOL_TRUE:
            ok = (rv != NULL && rv->val != NULL &&
                  rv->val->type == M_LANG_VAL_TYPE_BOOL &&
                  rv->val->data.b != 0);
            if (!ok)
                fprintf(stderr, "  FAIL [%s]: expected true\n", tc->name);
            break;
        case EXPECT_BOOL_FALSE:
            ok = (rv != NULL && rv->val != NULL &&
                  rv->val->type == M_LANG_VAL_TYPE_BOOL &&
                  rv->val->data.b == 0);
            if (!ok)
                fprintf(stderr, "  FAIL [%s]: expected false\n", tc->name);
            break;
        case EXPECT_NIL:
            ok = (rv == NULL ||
                  (rv->val != NULL && rv->val->type == M_LANG_VAL_TYPE_NIL));
            if (!ok)
                fprintf(stderr, "  FAIL [%s]: expected nil\n", tc->name);
            break;
        case EXPECT_STRING:
            ok = (rv != NULL && rv->val != NULL &&
                  rv->val->type == M_LANG_VAL_TYPE_STRING &&
                  rv->val->data.s != NULL &&
                  tc->estr != NULL &&
                  mln_string_const_strcmp(rv->val->data.s, (char *)tc->estr) == 0);
            if (!ok) {
                if (rv && rv->val && rv->val->type == M_LANG_VAL_TYPE_STRING
                    && rv->val->data.s)
                    fprintf(stderr, "  FAIL [%s]: got \"%.*s\" expected \"%s\"\n",
                            tc->name,
                            (int)rv->val->data.s->len,
                            (char *)rv->val->data.s->data,
                            tc->estr);
                else
                    fprintf(stderr, "  FAIL [%s]: string mismatch\n", tc->name);
            }
            break;
    }

    if (ok) {
        ++g_n_pass;
        printf("  PASS [%s]\n", tc->name);
    } else {
        ++g_n_fail;
    }

    mln_event_break_set(tc->ev);
}

/* =========================================================
 * Helper: run one test
 * ========================================================= */

static void run_test(mln_lang_t *lang, mln_event_t *ev,
                     const char *name, const char *code,
                     expect_type_t etype, mln_s64_t eint,
                     double ereal, const char *estr)
{
    test_ctx_t tc;
    tc.ev    = ev;
    tc.name  = name;
    tc.etype = etype;
    tc.eint  = eint;
    tc.ereal = ereal;
    tc.estr  = estr;

    mln_string_t src;
    mln_string_nset(&src, (mln_u8ptr_t)code, strlen(code));

    mln_event_break_reset(ev);

    mln_lang_ctx_t *ctx = mln_lang_job_new(lang, NULL, M_INPUT_T_BUF,
                                           &src, &tc, test_return_handler);
    if (ctx == NULL) {
        fprintf(stderr, "  FAIL [%s]: mln_lang_job_new returned NULL\n", name);
        ++g_n_fail;
        return;
    }

    mln_event_dispatch(ev);
}

/* Convenience wrappers */
#define T_INT(lang, ev, name, code, expected) \
    run_test(lang, ev, name, code, EXPECT_INT, (mln_s64_t)(expected), 0.0, NULL)
#define T_REAL(lang, ev, name, code, expected) \
    run_test(lang, ev, name, code, EXPECT_REAL, 0, (double)(expected), NULL)
#define T_TRUE(lang, ev, name, code) \
    run_test(lang, ev, name, code, EXPECT_BOOL_TRUE, 0, 0.0, NULL)
#define T_FALSE(lang, ev, name, code) \
    run_test(lang, ev, name, code, EXPECT_BOOL_FALSE, 0, 0.0, NULL)
#define T_NIL(lang, ev, name, code) \
    run_test(lang, ev, name, code, EXPECT_NIL, 0, 0.0, NULL)
#define T_STR(lang, ev, name, code, expected) \
    run_test(lang, ev, name, code, EXPECT_STRING, 0, 0.0, expected)
#define T_NONE(lang, ev, name, code) \
    run_test(lang, ev, name, code, EXPECT_NONE, 0, 0.0, NULL)

/* =========================================================
 * Multi-job test helpers (sections 31-32)
 * ========================================================= */

static volatile int multi_done    = 0;
static volatile int multi_result1 = -1;
static volatile int multi_result2 = -1;

typedef struct {
    mln_event_t *ev;
    int          job_id;   /* 1 or 2 */
} multi_tc_t;

static void multi_return_handler(mln_lang_ctx_t *ctx) {
    multi_tc_t *mtc = (multi_tc_t *)mln_lang_ctx_data_get(ctx);
    mln_lang_var_t *rv = ctx->ret_var;
    int val = -1;
    if (rv && rv->val && rv->val->type == M_LANG_VAL_TYPE_INT)
        val = (int)rv->val->data.i;
    if (mtc->job_id == 1) multi_result1 = val;
    else                  multi_result2 = val;
    if (++multi_done >= 2)
        mln_event_break_set(mtc->ev);
}

/* Error isolation: one script terminates on runtime error; the other
 * should still complete normally.  Job 1 intentionally triggers a
 * runtime error by calling a non-function value; job 2 returns 42. */
static volatile int iso_done = 0;
static volatile int iso_result_good = -1;  /* job 2 (valid) */
static volatile int iso_job1_fired  = 0;   /* job 1 return_handler was called */

typedef struct {
    mln_event_t *ev;
    int          job_id;
} iso_tc_t;

static void iso_return_handler(mln_lang_ctx_t *ctx) {
    iso_tc_t *itc = (iso_tc_t *)mln_lang_ctx_data_get(ctx);
    if (itc->job_id == 1) {
        /* Expect this to fire even on error (return_handler always called). */
        iso_job1_fired = 1;
    } else {
        mln_lang_var_t *rv = ctx->ret_var;
        if (rv && rv->val && rv->val->type == M_LANG_VAL_TYPE_INT)
            iso_result_good = (int)rv->val->data.i;
    }
    if (++iso_done >= 2)
        mln_event_break_set(itc->ev);
}

/* =========================================================
 * main
 * ========================================================= */

int main(void)
{
    assert(socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0);

    mln_event_t *ev = mln_event_new();
    assert(ev != NULL);
    mln_lang_t *lang = mln_lang_new(ev, lang_signal, lang_clear);
    assert(lang != NULL);

    printf("=== Melang language feature tests ===\n");

    /* -------------------------------------------------
     * 1. Integer literals and basic arithmetic
     * ------------------------------------------------- */
    T_INT(lang, ev, "int_literal",           "return 42;",                     42);
    T_INT(lang, ev, "int_add",               "return 3 + 4;",                  7);
    T_INT(lang, ev, "int_sub",               "return 10 - 3;",                 7);
    T_INT(lang, ev, "int_mul",               "return 6 * 7;",                  42);
    T_INT(lang, ev, "int_div",               "return 20 / 4;",                 5);
    T_INT(lang, ev, "int_mod",               "return 17 % 5;",                 2);
    T_INT(lang, ev, "int_neg_unary",         "return -9;",                     -9);
    T_INT(lang, ev, "int_precedence",        "return 2 + 3 * 4;",              14);
    T_INT(lang, ev, "int_parens",            "return (2 + 3) * 4;",            20);
    T_INT(lang, ev, "hex_literal",           "return 0xff;",                   255);
    T_INT(lang, ev, "octal_literal",         "return 010;",                    8);

    /* -------------------------------------------------
     * 2. Real literals
     * ------------------------------------------------- */
    T_REAL(lang, ev, "real_literal",         "return 3.14;",                   3.14);
    T_REAL(lang, ev, "real_add",             "return 1.5 + 2.5;",              4.0);
    T_REAL(lang, ev, "real_mul",             "return 2.0 * 3.0;",              6.0);

    /* -------------------------------------------------
     * 3. Bool and nil literals
     * ------------------------------------------------- */
    T_TRUE(lang, ev,  "bool_true",           "return true;");
    T_FALSE(lang, ev, "bool_false",          "return false;");
    T_NIL(lang, ev,   "nil_literal",         "return nil;");
    T_FALSE(lang, ev, "bool_not_true",       "return !true;");
    T_TRUE(lang, ev,  "bool_not_false",      "return !false;");

    /* -------------------------------------------------
     * 4. String literals
     * ------------------------------------------------- */
    T_STR(lang, ev, "string_literal",        "return 'hello';",                "hello");
    T_STR(lang, ev, "string_concat",         "return 'foo' + 'bar';",          "foobar");
    T_STR(lang, ev, "string_dquote",         "return \"world\";",              "world");

    /* -------------------------------------------------
     * 5. Comparison operators
     * ------------------------------------------------- */
    T_TRUE(lang,  ev, "cmp_lt_true",         "return 1 < 2;");
    T_FALSE(lang, ev, "cmp_lt_false",        "return 2 < 1;");
    T_TRUE(lang,  ev, "cmp_le_equal",        "return 2 <= 2;");
    T_TRUE(lang,  ev, "cmp_gt_true",         "return 3 > 2;");
    T_TRUE(lang,  ev, "cmp_ge_equal",        "return 3 >= 3;");
    T_TRUE(lang,  ev, "cmp_eq_true",         "return 5 == 5;");
    T_FALSE(lang, ev, "cmp_eq_false",        "return 5 == 6;");
    T_TRUE(lang,  ev, "cmp_ne_true",         "return 5 != 6;");
    T_FALSE(lang, ev, "cmp_ne_false",        "return 5 != 5;");

    /* -------------------------------------------------
     * 6. Logical operators (short-circuit)
     * ------------------------------------------------- */
    T_TRUE(lang,  ev, "logic_and_tt",        "return true && true;");
    T_FALSE(lang, ev, "logic_and_tf",        "return true && false;");
    T_FALSE(lang, ev, "logic_and_ff",        "return false && false;");
    T_TRUE(lang,  ev, "logic_or_ft",         "return false || true;");
    T_TRUE(lang,  ev, "logic_or_tt",         "return true || false;");
    T_FALSE(lang, ev, "logic_or_ff",         "return false || false;");
    /* Short-circuit: right side of && must NOT be evaluated when left is false */
    T_INT(lang, ev, "logic_and_sc",
          "@F() { a=0; false && (a=1); return a; } return F();", 0);
    /* Short-circuit: right side of || must NOT be evaluated when left is true */
    T_INT(lang, ev, "logic_or_sc",
          "@F() { a=0; true || (a=1); return a; } return F();", 0);

    /* -------------------------------------------------
     * 7. Bitwise operators
     * ------------------------------------------------- */
    T_INT(lang, ev, "bor",                   "return 3 | 5;",                  7);
    T_INT(lang, ev, "band",                  "return 6 & 5;",                  4);
    T_INT(lang, ev, "bxor",                  "return 6 ^ 5;",                  3);
    T_INT(lang, ev, "lshift",                "return 1 << 4;",                 16);
    T_INT(lang, ev, "rshift",                "return 16 >> 2;",                4);
    T_INT(lang, ev, "bitwise_chain",         "return (0xf0 & 0xff) | 0x0f;",   255);
    /* Unary bitwise NOT ~ */
    T_INT(lang, ev, "bitnot_zero",           "return ~0;",                     -1);
    T_INT(lang, ev, "bitnot_ff",             "return ~0xff;",                  -256);
    T_INT(lang, ev, "bitnot_expr",           "@F() { a=5; return ~a; } return F();", -6);

    /* -------------------------------------------------
     * 8. Assignment and compound assignment
     * ------------------------------------------------- */
    T_INT(lang, ev, "assign_basic",
          "@F() { a=7; return a; } return F();",                               7);
    T_INT(lang, ev, "assign_pluseq",
          "@F() { a=3; a+=4; return a; } return F();",                        7);
    T_INT(lang, ev, "assign_subeq",
          "@F() { a=10; a-=3; return a; } return F();",                       7);
    T_INT(lang, ev, "assign_muleq",
          "@F() { a=3; a*=4; return a; } return F();",                        12);
    T_INT(lang, ev, "assign_diveq",
          "@F() { a=20; a/=4; return a; } return F();",                       5);
    T_INT(lang, ev, "assign_modeq",
          "@F() { a=17; a%=5; return a; } return F();",                       2);
    T_INT(lang, ev, "assign_oreq",
          "@F() { a=3; a|=5; return a; } return F();",                        7);
    T_INT(lang, ev, "assign_andeq",
          "@F() { a=6; a&=5; return a; } return F();",                        4);
    T_INT(lang, ev, "assign_xoreq",
          "@F() { a=6; a^=5; return a; } return F();",                        3);
    T_INT(lang, ev, "assign_lshifteq",
          "@F() { a=1; a<<=4; return a; } return F();",                       16);
    T_INT(lang, ev, "assign_rshifteq",
          "@F() { a=16; a>>=2; return a; } return F();",                      4);

    /* -------------------------------------------------
     * 9. Prefix and suffix ++ / --
     * ------------------------------------------------- */
    T_INT(lang, ev, "suffix_inc",
          "@F() { a=5; b=a++; return b; } return F();",                       5);
    T_INT(lang, ev, "suffix_inc_after",
          "@F() { a=5; a++; return a; } return F();",                         6);
    T_INT(lang, ev, "suffix_dec",
          "@F() { a=5; b=a--; return b; } return F();",                       5);
    T_INT(lang, ev, "suffix_dec_after",
          "@F() { a=5; a--; return a; } return F();",                         4);
    T_INT(lang, ev, "prefix_inc",
          "@F() { a=5; b=++a; return b; } return F();",                       6);
    T_INT(lang, ev, "prefix_dec",
          "@F() { a=5; b=--a; return b; } return F();",                       4);

    /* -------------------------------------------------
     * 10. Comma expression (value = last sub-expression)
     * ------------------------------------------------- */
    T_INT(lang, ev, "comma_expr",
          "@F() { a=1; return (a=10, 99); } return F();",                     99);
    T_INT(lang, ev, "comma_side_effect",
          "@F() { a=0; (a=5, a=a+1); return a; } return F();",               6);

    /* -------------------------------------------------
     * 11. if / else / fi
     * ------------------------------------------------- */
    T_INT(lang, ev, "if_true_branch",
          "@F() { if (1) { return 10; } fi return 20; } return F();",         10);
    T_INT(lang, ev, "if_false_branch",
          "@F() { if (0) { return 10; } fi return 20; } return F();",         20);
    T_INT(lang, ev, "if_else_true",
          "@F() { if (1) { return 10; } else { return 20; } } return F();",   10);
    T_INT(lang, ev, "if_else_false",
          "@F() { if (0) { return 10; } else { return 20; } } return F();",   20);
    T_INT(lang, ev, "if_nested",
          "@F() { if (1) { if (1) { return 7; } fi } fi return 0; } return F();", 7);
    T_INT(lang, ev, "if_chain",
          "@F(x) { if (x==1) { return 10; } else { if (x==2) { return 20; } else { return 30; } } } return F(2);", 20);

    /* -------------------------------------------------
     * 12. while loop + break + continue
     * ------------------------------------------------- */
    T_INT(lang, ev, "while_basic",
          "@F() { i=0; s=0; while (i<5) { s=s+i; i=i+1; } return s; } return F();", 10);
    T_INT(lang, ev, "while_break",
          "@F() { i=0; while (true) { if (i>=3) { break; } fi i=i+1; } return i; } return F();", 3);
    T_INT(lang, ev, "while_continue",
          "@F() { i=0; s=0; while (i<10) { i=i+1; if (i%2==0) { continue; } fi s=s+i; } return s; } return F();", 25);

    /* -------------------------------------------------
     * 13. for loop + break + continue
     * ------------------------------------------------- */
    T_INT(lang, ev, "for_basic",
          "@F() { s=0; for (i=0; i<5; i++) { s=s+i; } return s; } return F();", 10);
    T_INT(lang, ev, "for_break",
          "@F() { s=0; for (i=0; i<10; i++) { if (i==5) { break; } fi s=s+i; } return s; } return F();", 10);
    T_INT(lang, ev, "for_nested",
          "@F() { s=0; for (i=0; i<3; i++) { for (j=0; j<3; j++) { s=s+1; } } return s; } return F();", 9);

    /* -------------------------------------------------
     * 14. switch / case / default
     * ------------------------------------------------- */
    T_INT(lang, ev, "switch_match_first",
          "@F(x) { switch (x) { case 1: { return 10; } case 2: { return 20; } default: { return 99; } } } return F(1);", 10);
    T_INT(lang, ev, "switch_match_second",
          "@F(x) { switch (x) { case 1: { return 10; } case 2: { return 20; } default: { return 99; } } } return F(2);", 20);
    T_INT(lang, ev, "switch_default",
          "@F(x) { switch (x) { case 1: { return 10; } default: { return 99; } } } return F(5);", 99);

    /* -------------------------------------------------
     * 15. goto / label
     * ------------------------------------------------- */
    T_INT(lang, ev, "goto_forward",
          "@F() { a=0; goto done; a=42; done: return a; } return F();",       0);
    T_INT(lang, ev, "goto_loop",
          "@F() { i=0; s=0; loop: if (i>=5) { goto end; } fi s=s+i; i=i+1; goto loop; end: return s; } return F();", 10);

    /* -------------------------------------------------
     * 16. User-defined functions — no args, with args
     * ------------------------------------------------- */
    T_INT(lang, ev, "func_noarg",
          "@Answer() { return 42; } return Answer();",                        42);
    T_INT(lang, ev, "func_onearg",
          "@Double(x) { return x * 2; } return Double(21);",                 42);
    T_INT(lang, ev, "func_twoarg",
          "@Add(a, b) { return a + b; } return Add(17, 25);",                42);
    T_INT(lang, ev, "func_return_nil",
          "@F() { } return 0;",                                               0);

    /* -------------------------------------------------
     * 17. Recursion — Fibonacci (self-recursion / CALL_SELF)
     * ------------------------------------------------- */
    T_INT(lang, ev, "fib_10",
          "@F(i) { if (i<=2) { return 1; } fi return F(i-1)+F(i-2); } return F(10);", 55);
    T_INT(lang, ev, "fib_1",
          "@F(i) { if (i<=2) { return 1; } fi return F(i-1)+F(i-2); } return F(1);",  1);
    T_INT(lang, ev, "fib_2",
          "@F(i) { if (i<=2) { return 1; } fi return F(i-1)+F(i-2); } return F(2);",  1);

    /* -------------------------------------------------
     * 18. Cross-function calls (non-self)
     * ------------------------------------------------- */
    T_INT(lang, ev, "cross_call",
          "@Triple(x) { return x*3; } @Double(x) { return x*2; } return Double(Triple(7));", 42);
    T_TRUE(lang, ev, "cross_mutual",
          "@IsEven(n) { if (n==0) { return true; } fi return IsOdd(n-1); } "
          "@IsOdd(n)  { if (n==0) { return false; } fi return IsEven(n-1); } "
          "return IsEven(8);");

    /* -------------------------------------------------
     * 19. Closures — function capturing outer variable
     *     use clause syntax: $(var1, var2, ...)
     * ------------------------------------------------- */
    T_INT(lang, ev, "closure_basic",
          "@Outer() { x=10; @Inner() $(x) { return x; } return Inner(); } return Outer();", 10);
    T_INT(lang, ev, "closure_arg_plus_capture",
          "@Outer() { base=100; @Adder(n) $(base) { return base+n; } return Adder(42); } return Outer();", 142);

    /* -------------------------------------------------
     * 20. Sets (classes / objects)
     *     Syntax: Name { member1; member2; @Method() { ... } }
     *     Instantiation: $Name
     * ------------------------------------------------- */
    T_INT(lang, ev, "set_member_access",
          "Point { x; y; } p = $Point; p.x = 10; p.y = 20; return p.x + p.y;", 30);
    T_INT(lang, ev, "set_method",
          "Counter { n; @Inc() { this.n = this.n + 1; } } "
          "c = $Counter; c.n = 0; c.Inc(); c.Inc(); c.Inc(); return c.n;",    3);

    /* -------------------------------------------------
     * 21. Arrays
     * ------------------------------------------------- */
    T_INT(lang, ev, "array_index",
          "a = [10, 20, 30]; return a[1];",                                   20);
    T_INT(lang, ev, "array_assign",
          "a = [1, 2, 3]; a[0] = 99; return a[0];",                          99);
    T_INT(lang, ev, "array_len_via_loop",
          "a = [1, 2, 3, 4, 5]; i=0; s=0; while (i<5) { s=s+a[i]; i=i+1; } return s;", 15);
    T_INT(lang, ev, "array_string_key",
          "a = ['x': 1, 'y': 2]; return a['x'] + a['y'];",                   3);

    /* -------------------------------------------------
     * 22. Multiple return paths
     * ------------------------------------------------- */
    T_INT(lang, ev, "multi_return_early",
          "@F(x) { if (x < 0) { return -1; } fi if (x == 0) { return 0; } fi return 1; } return F(-5);", -1);
    T_INT(lang, ev, "multi_return_mid",
          "@F(x) { if (x < 0) { return -1; } fi if (x == 0) { return 0; } fi return 1; } return F(0);", 0);
    T_INT(lang, ev, "multi_return_last",
          "@F(x) { if (x < 0) { return -1; } fi if (x == 0) { return 0; } fi return 1; } return F(7);", 1);

    /* -------------------------------------------------
     * 23. Top-level closure: captured outer variable (read-only)
     * ------------------------------------------------- */
    T_INT(lang, ev, "closure_toplevel",
          "base=7; @Adder(n) $(base) { return base+n; } return Adder(35);",  42);

    /* -------------------------------------------------
     * 24. Nested arithmetic / operator precedence
     * ------------------------------------------------- */
    T_INT(lang, ev, "prec_add_mul",     "return 2 + 3 * 4;",                 14);
    T_INT(lang, ev, "prec_paren",       "return (2 + 3) * 4;",               20);
    T_INT(lang, ev, "prec_div_mod",     "return 10 / 3 + 10 % 3;",           4);
    T_INT(lang, ev, "prec_neg",         "return -2 * 3;",                    -6);
    T_INT(lang, ev, "prec_shift_arith", "return 1 + 2 << 1;",                6); /* Melang: addsub > move, so (1+2)<<1 = 6 */

    /* -------------------------------------------------
     * 25. Dump() built-in smoke test (just checks it completes)
     * ------------------------------------------------- */
    T_NONE(lang, ev, "dump_string",     "Dump('hello');");
    T_NONE(lang, ev, "dump_int",        "Dump(42);");

    /* -------------------------------------------------
     * 26. Larger program: iterative sum 1..100
     * ------------------------------------------------- */
    T_INT(lang, ev, "iter_sum_100",
          "@F() { s=0; for (i=1; i<=100; i++) { s=s+i; } return s; } return F();", 5050);

    /* -------------------------------------------------
     * 27. Recursive factorial
     * ------------------------------------------------- */
    T_INT(lang, ev, "factorial_10",
          "@Fact(n) { if (n<=1) { return 1; } fi return n * Fact(n-1); } return Fact(10);", 3628800);

    /* -------------------------------------------------
     * 28. Tier 3 fix: continue inside for loop
     *     Sum only odd numbers 1..9 = 1+3+5+7+9 = 25
     * ------------------------------------------------- */
    T_INT(lang, ev, "for_continue",
          "@F() { s=0; for (i=0; i<10; i++) { if (i%2==0) { continue; } fi s=s+i; } return s; } return F();", 25);

    /* -------------------------------------------------
     * 29. Reference parameters (&x)
     *
     *  &x in a function call passes by reference, so the callee can
     *  modify the caller's variable.
     * ------------------------------------------------- */
    T_INT(lang, ev, "ref_local",
          "@inc(&v) { v = v + 1; } "
          "@F() { a = 10; inc(&a); return a; } "
          "return F();",
          11);
    T_INT(lang, ev, "ref_global_modify",
          "@setG(&v, n) { v = n; } "
          "g = 0; setG(&g, 99); return g;",
          99);
    T_INT(lang, ev, "ref_swap",
          "@swap(&a, &b) { tmp = a; a = b; b = tmp; } "
          "@F() { x = 3; y = 7; swap(&x, &y); return x * 10 + y; } "
          "return F();",
          73); /* x=7, y=3 → 73 */

    /* -------------------------------------------------
     * 29a. Compound assignment on property lvalues: obj.x += 1
     * -------------------------------------------------
     * Verified: VM now lowers `obj.x += val` to a DUP+GET+binop+SET
     * sequence rather than bailing out at compile time.
     * ------------------------------------------------- */
    T_INT(lang, ev, "prop_pluseq",
          "Point { x; y; } "
          "p = $Point; p.x = 3; p.x += 4; return p.x;",                   7);
    T_INT(lang, ev, "prop_minuseq",
          "Point { x; } p = $Point; p.x = 10; p.x -= 3; return p.x;",     7);
    T_INT(lang, ev, "prop_muleq",
          "Point { x; } p = $Point; p.x = 3;  p.x *= 4; return p.x;",    12);
    T_INT(lang, ev, "prop_diveq",
          "Point { x; } p = $Point; p.x = 20; p.x /= 4; return p.x;",     5);

    /* -------------------------------------------------
     * 29b. Compound assignment on index lvalues: arr[i] += 1
     * ------------------------------------------------- */
    T_INT(lang, ev, "index_pluseq",
          "a = [10, 20, 30]; a[1] += 5; return a[1];",                    25);
    T_INT(lang, ev, "index_minuseq",
          "a = [10, 20, 30]; a[2] -= 5; return a[2];",                    25);
    T_INT(lang, ev, "index_muleq",
          "a = [2, 3, 4]; a[0] *= 3; return a[0];",                        6);
    T_INT(lang, ev, "index_lshifteq",
          "a = [1, 2, 3]; a[0] <<= 3; return a[0];",                       8);

    /* -------------------------------------------------
     * 29c. Postfix ++/-- on global and property/index lvalues
     * ------------------------------------------------- */
    /* global g++ / g-- */
    T_INT(lang, ev, "global_suffix_inc_result",
          "g = 5; r = g++; return r;",                                      5);
    T_INT(lang, ev, "global_suffix_inc_after",
          "g = 5; g++; return g;",                                          6);
    T_INT(lang, ev, "global_suffix_dec_result",
          "g = 5; r = g--; return r;",                                      5);
    T_INT(lang, ev, "global_suffix_dec_after",
          "g = 5; g--; return g;",                                          4);
    /* obj.x++ / obj.x-- */
    T_INT(lang, ev, "prop_suffix_inc_result",
          "C { v; } obj = $C; obj.v = 7; r = obj.v++; return r;",          7);
    T_INT(lang, ev, "prop_suffix_inc_after",
          "C { v; } obj = $C; obj.v = 7; obj.v++; return obj.v;",          8);
    T_INT(lang, ev, "prop_suffix_dec_result",
          "C { v; } obj = $C; obj.v = 7; r = obj.v--; return r;",          7);
    T_INT(lang, ev, "prop_suffix_dec_after",
          "C { v; } obj = $C; obj.v = 7; obj.v--; return obj.v;",          6);
    /* arr[i]++ / arr[i]-- */
    T_INT(lang, ev, "index_suffix_inc_result",
          "a = [10, 20, 30]; r = a[1]++; return r;",                      20);
    T_INT(lang, ev, "index_suffix_inc_after",
          "a = [10, 20, 30]; a[1]++; return a[1];",                       21);
    T_INT(lang, ev, "index_suffix_dec_result",
          "a = [10, 20, 30]; r = a[0]--; return r;",                      10);
    T_INT(lang, ev, "index_suffix_dec_after",
          "a = [10, 20, 30]; a[0]--; return a[0];",                        9);

    /* -------------------------------------------------
     * 29d. Prefix ++/-- on global lvalues
     * ------------------------------------------------- */
    T_INT(lang, ev, "global_prefix_inc",
          "g = 5; r = ++g; return r;",                                      6);
    T_INT(lang, ev, "global_prefix_inc_after",
          "g = 5; ++g; return g;",                                          6);
    T_INT(lang, ev, "global_prefix_dec",
          "g = 5; r = --g; return r;",                                      4);
    T_INT(lang, ev, "global_prefix_dec_after",
          "g = 5; --g; return g;",                                          4);

    /* -------------------------------------------------
     * 30. Reactive programming: Watch / Unwatch
     *
     *  Watch(var, func, userData): func is called as func(newval, userData)
     *  when var is assigned a new value.  Both arguments in func can be
     *  reference params (&), which allows the callback to modify the
     *  caller's variables.
     * ------------------------------------------------- */
    /* Basic: callback sets ud = new value of watched var */
    T_INT(lang, ev, "watch_basic",
          "@onChange(&nv, &ud) { ud = nv; } "
          "x = 0; result = 99; "
          "Watch(x, onChange, result); "
          "x = 42; "
          "return result;",
          42);

    /* Watch fires on each subsequent assignment */
    T_INT(lang, ev, "watch_repeated",
          "@cb(&nv, &ud) { ud = ud + nv; } "
          "x = 0; acc = 0; "
          "Watch(x, cb, acc); "
          "x = 1; x = 2; x = 3; "
          "return acc;",  /* 0+1+2+3 = 6 */
          6);

    /* Unwatch stops the callback from firing */
    T_INT(lang, ev, "watch_unwatch",
          "@cb(&nv, &ud) { ud = nv; } "
          "x = 0; result = 0; "
          "Watch(x, cb, result); "
          "x = 10; "          /* triggers: result = 10 */
          "Unwatch(x); "
          "x = 99; "          /* no trigger: result stays 10 */
          "return result;",
          10);

    /* -------------------------------------------------
     * 31. Multiple scripts on the same event loop
     *
     *  Launch two jobs simultaneously.  The event loop runs them
     *  cooperatively (time-sliced).  We break when BOTH jobs finish.
     * ------------------------------------------------- */
    {
        /* Shared state for the two concurrent jobs */
        multi_done = 0; multi_result1 = -1; multi_result2 = -1;
        mln_event_break_reset(ev);

        multi_tc_t mtc1, mtc2;
        mtc1.ev = ev; mtc1.job_id = 1;
        mtc2.ev = ev; mtc2.job_id = 2;

        /* Job 1: sum 1..200 = 20100 */
        const char *code1 =
            "@F() { s=0; for (i=1; i<=200; i++) { s=s+i; } return s; } return F();";
        /* Job 2: fib(15) = 610 */
        const char *code2 =
            "@Fib(n) { if (n<=1) { return n; } fi return Fib(n-1)+Fib(n-2); } return Fib(15);";

        mln_string_t src1, src2;
        mln_string_nset(&src1, (mln_u8ptr_t)code1, strlen(code1));
        mln_string_nset(&src2, (mln_u8ptr_t)code2, strlen(code2));

        mln_lang_ctx_t *c1 = mln_lang_job_new(lang, NULL, M_INPUT_T_BUF,
                                               &src1, &mtc1, multi_return_handler);
        mln_lang_ctx_t *c2 = mln_lang_job_new(lang, NULL, M_INPUT_T_BUF,
                                               &src2, &mtc2, multi_return_handler);

        if (c1 == NULL || c2 == NULL) {
            fprintf(stderr, "  FAIL [multi_concurrent]: job creation failed\n");
            ++g_n_fail;
        } else {
            mln_event_dispatch(ev);  /* runs until both handlers call break_set */

            int ok1 = (multi_result1 == 20100);
            int ok2 = (multi_result2 == 610);
            if (ok1) { ++g_n_pass; printf("  PASS [multi_job1_sum200]\n"); }
            else     { ++g_n_fail; fprintf(stderr, "  FAIL [multi_job1_sum200]: got %d expected 20100\n", multi_result1); }
            if (ok2) { ++g_n_pass; printf("  PASS [multi_job2_fib15]\n"); }
            else     { ++g_n_fail; fprintf(stderr, "  FAIL [multi_job2_fib15]: got %d expected 610\n", multi_result2); }
        }
    }

    /* -------------------------------------------------
     * 32. Error isolation: a failing script must not affect other tasks
     *
     *  Launch two jobs simultaneously.  Job 1 intentionally causes a
     *  runtime error (integer division by zero).  Job 2 is a simple
     *  valid script that returns 42.  Both return_handlers must fire,
     *  and job 2 must return the correct result.
     * ------------------------------------------------- */
    {
        iso_done = 0; iso_result_good = -1; iso_job1_fired = 0;
        mln_event_break_reset(ev);

        iso_tc_t itc1, itc2;
        itc1.ev = ev; itc1.job_id = 1;
        itc2.ev = ev; itc2.job_id = 2;

        /* Job 1: division by zero → runtime error */
        const char *bad_code  = "x = 1 / 0; return x;";
        /* Job 2: valid script */
        const char *good_code = "return 42;";

        mln_string_t s1, s2;
        mln_string_nset(&s1, (mln_u8ptr_t)bad_code,  strlen(bad_code));
        mln_string_nset(&s2, (mln_u8ptr_t)good_code, strlen(good_code));

        mln_lang_ctx_t *ic1 = mln_lang_job_new(lang, NULL, M_INPUT_T_BUF,
                                                &s1, &itc1, iso_return_handler);
        mln_lang_ctx_t *ic2 = mln_lang_job_new(lang, NULL, M_INPUT_T_BUF,
                                                &s2, &itc2, iso_return_handler);

        if (ic1 == NULL || ic2 == NULL) {
            fprintf(stderr, "  FAIL [error_isolation]: job creation failed\n");
            ++g_n_fail;
        } else {
            mln_event_dispatch(ev);  /* runs until both handlers call break_set */

            int ok_iso = (iso_job1_fired == 1) && (iso_result_good == 42);
            if (ok_iso) { ++g_n_pass; printf("  PASS [error_isolation]\n"); }
            else {
                ++g_n_fail;
                fprintf(stderr,
                    "  FAIL [error_isolation]: job1_fired=%d, good_result=%d (expected 42)\n",
                    iso_job1_fired, iso_result_good);
            }
        }
    }

    /* -------------------------------------------------
     * 34. Assignment-as-expression for property and index lvalues.
     *     `return (obj.x = v)` and `return (a[i] = v)` must return v.
     * ------------------------------------------------- */
    T_INT(lang, ev, "prop_assign_expr",
          "C { v; } obj = $C; return (obj.v = 42);",                        42);
    T_INT(lang, ev, "index_assign_expr",
          "a = [0, 0, 0]; return (a[1] = 99);",                             99);
    /* Chained: the result of an assignment can be used in a larger expr */
    T_INT(lang, ev, "prop_assign_chain",
          "C { v; } obj = $C; x = (obj.v = 7); return x + obj.v;",         14);

    /* -------------------------------------------------
     * 35. Reading an unbound identifier creates a nil variable (matches
     *     AST interpreter semantics; must not abort the script).
     * ------------------------------------------------- */
    T_INT(lang, ev, "unbound_read_nil",
          "x = unbound_var; if (x == nil) { return 1; } fi return 0;",      1);

    /* -------------------------------------------------
     * 36. Assignment inside a function must update an existing outer
     *     (global) variable, not shadow it with a new local.
     * ------------------------------------------------- */
    T_INT(lang, ev, "func_updates_global",
          "g = 1; @F() { g = 2; } F(); return g;",                          2);
    T_INT(lang, ev, "func_reads_global",
          "g = 10; @F() { return g; } return F();",                         10);
    T_INT(lang, ev, "func_modifies_global_compound",
          "g = 5; @F() { g += 3; } F(); return g;",                         8);

    /* -------------------------------------------------
     * Report
     * ------------------------------------------------- */
    printf("=== Results: %d passed, %d failed ===\n", g_n_pass, g_n_fail);

    mln_lang_free(lang);
    mln_event_free(ev);
    close(fds[0]);
    close(fds[1]);

    return g_n_fail == 0 ? 0 : 1;
}
