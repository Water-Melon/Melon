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
 *       -Llib/ -lmelon_static -lpthread -ldl -lm
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
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
    EXPECT_NONE,    /* script has no return value; just checks it completes */
    EXPECT_EXITED   /* Script terminated via Exit() or a self-targeted
                     * Kill(<own alias>).  By that point an intermediate AST
                     * or VM step has typically already migrated ctx->ret_var
                     * into its own node, so the return_handler sees a NULL
                     * ret_var.  The critical property the test verifies is
                     * that ret_var is NOT some user-supplied value produced
                     * by code following the Exit/Kill — if quit-marking were
                     * to leak (e.g. `Exit(); return 999;` actually executing
                     * the return), ret_var would be a non-nil INT here and
                     * the test would fail. */
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
            /* Script should complete without error: ctx->ret_var is set
             * to nil (at minimum) for any clean exit, and NULL only when
             * the context was terminated by an error before it could
             * return. */
            ok = (rv != NULL);
            if (!ok)
                fprintf(stderr, "  FAIL [%s]: ret_var is NULL (script aborted?)\n",
                        tc->name);
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
            ok = (rv != NULL && rv->val != NULL &&
                  rv->val->type == M_LANG_VAL_TYPE_NIL);
            if (!ok) {
                if (rv == NULL)
                    fprintf(stderr, "  FAIL [%s]: ret_var is NULL (script aborted?)\n", tc->name);
                else
                    fprintf(stderr, "  FAIL [%s]: expected nil\n", tc->name);
            }
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
        case EXPECT_EXITED:
            /*
             * "Exit terminated the coroutine and no unreachable user code
             *  ran."  See comment on EXPECT_EXITED in expect_type_t.
             *
             * Pass:
             *   - ret_var is NULL (the usual case — AST/VM step consumed it
             *     before quit fired).
             *   - ret_var holds a NIL value (the nil that Exit returned and
             *     that no intermediate step happened to claim).
             *
             * Fail:
             *   - Any other return type.  In particular `Exit(); return 999;`
             *     would surface ret_var as INT(999) if quit-marking failed
             *     to short-circuit the next statement.
             */
            ok = (rv == NULL ||
                  (rv->val != NULL && rv->val->type == M_LANG_VAL_TYPE_NIL));
            if (!ok) {
                if (rv && rv->val && rv->val->type == M_LANG_VAL_TYPE_INT)
                    fprintf(stderr,
                            "  FAIL [%s]: script ran post-Exit code; "
                            "ret_var=INT(%lld) (expected terminated)\n",
                            tc->name, (long long)rv->val->data.i);
                else if (rv && rv->val)
                    fprintf(stderr,
                            "  FAIL [%s]: script ran post-Exit code; "
                            "ret_var type=%d (expected terminated)\n",
                            tc->name, rv->val->type);
                else
                    fprintf(stderr,
                            "  FAIL [%s]: unexpected ret_var state\n",
                            tc->name);
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

static void run_test_aliased(mln_lang_t *lang, mln_event_t *ev,
                             const char *name, const char *code,
                             const char *alias_str,
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

    mln_string_t alias_storage;
    mln_string_t *alias_p = NULL;
    if (alias_str != NULL) {
        mln_string_nset(&alias_storage, (mln_u8ptr_t)alias_str, strlen(alias_str));
        alias_p = &alias_storage;
    }

    mln_event_break_reset(ev);

    mln_lang_ctx_t *ctx = mln_lang_job_new(lang, alias_p, M_INPUT_T_BUF,
                                           &src, &tc, test_return_handler);
    if (ctx == NULL) {
        fprintf(stderr, "  FAIL [%s]: mln_lang_job_new returned NULL\n", name);
        ++g_n_fail;
        return;
    }

    mln_event_dispatch(ev);
}

static void run_test(mln_lang_t *lang, mln_event_t *ev,
                     const char *name, const char *code,
                     expect_type_t etype, mln_s64_t eint,
                     double ereal, const char *estr)
{
    run_test_aliased(lang, ev, name, code, NULL,
                     etype, eint, ereal, estr);
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
#define T_EXIT(lang, ev, name, code) \
    run_test(lang, ev, name, code, EXPECT_EXITED, 0, 0.0, NULL)
#define T_EXIT_ALIAS(lang, ev, name, code, alias) \
    run_test_aliased(lang, ev, name, code, alias, EXPECT_EXITED, 0, 0.0, NULL)

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
 * runtime error via integer division by zero; job 2 returns 42. */
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

/* -------------------------------------------------
 * Cross-coroutine Kill() test helper
 *
 * A custom return handler that checks:
 *   1. The parent's ret_var == INT(42)
 *   2. After the parent completes, the lang scheduler has no remaining
 *      queued contexts (i.e., the killed child was properly freed).
 * ------------------------------------------------- */
typedef struct {
    mln_event_t *ev;
    const char  *name;
    int          parent_ok;
    int          cleanup_ok;
} kill_cross_tc_t;

static void kill_cross_return_handler(mln_lang_ctx_t *ctx) {
    kill_cross_tc_t *kc = (kill_cross_tc_t *)mln_lang_ctx_data_get(ctx);
    mln_lang_var_t *rv = ctx->ret_var;

    /* Check 1: parent returns 42. */
    kc->parent_ok = (rv != NULL && rv->val != NULL &&
                     rv->val->type == M_LANG_VAL_TYPE_INT &&
                     rv->val->data.i == 42);

    /* Check 2: after the parent's return_handler fires, no other ctx
     * should be queued — the killed child must have been freed.
     * Note: ctx itself is about to be freed by the dispatcher AFTER this
     * handler returns, so it is still on run_head right now.  We check
     * that no OTHER ctx is present: run_head == ctx && run_head->next ==
     * NULL and wait_head == NULL. */
    mln_lang_t *lang = ctx->lang;
    kc->cleanup_ok = (lang->run_head == ctx &&
                      ctx->next == NULL &&
                      lang->wait_head == NULL);

    mln_event_break_set(kc->ev);
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
          "g = 5; @F() { g += 3; } F(); return g;                          ", 8);

    /* =========================================================================
     * The blocks below specifically target VM code paths that were added by
     * the perf/lang commits ahead of origin/master.  Each test verifies the
     * VM result matches the AST-walker semantics described in mln_lang.c.
     * ========================================================================= */

    /* -------------------------------------------------
     * 37. Comma chains with 3+ elements (commit 7bb0fef enabled this in the
     *     VM; previously the VM bailed for any chain longer than one node).
     *     Value of (e1, e2, ..., en) is en; intermediate side effects must
     *     still happen in order.
     * ------------------------------------------------- */
    T_INT(lang, ev, "comma_chain_5",
          "@F() { return (10, 20, 30, 40, 50); } return F();",                50);
    T_INT(lang, ev, "comma_chain_assigns",
          "@F() { a=0; b=0; c=0; return (a=1, b=a+1, c=a+b, c+10); } return F();", 13);
    T_INT(lang, ev, "comma_chain_in_for_init",
          /* for-init isn't a comma in Melang grammar; use comma inside
           * the post step expression instead */
          "@F() { s=0; for (i=0; i<5; (s=s+i, i=i+1)) { } return s; } return F();", 10);

    /* -------------------------------------------------
     * 38. Bitwise/shift compound assignment on locals (commit 7bb0fef
     *     wired |= &= ^= <<= >>= through new BOR/BAND/BXOR/LSHIFT/RSHIFT
     *     opcodes; previously the VM bailed and fell back to AST).
     * ------------------------------------------------- */
    T_INT(lang, ev, "compound_bor_local",
          "@F() { a=3; a |= 5; return a; } return F();",                    7);
    T_INT(lang, ev, "compound_band_local",
          "@F() { a=7; a &= 4; return a; } return F();",                    4);
    T_INT(lang, ev, "compound_bxor_local",
          "@F() { a=6; a ^= 3; return a; } return F();",                    5);
    T_INT(lang, ev, "compound_lshift_local",
          "@F() { a=1; a <<= 5; return a; } return F();",                  32);
    T_INT(lang, ev, "compound_rshift_local",
          "@F() { a=64; a >>= 3; return a; } return F();",                  8);

    /* -------------------------------------------------
     * 39. Mixed int/real arithmetic and comparison.  The VM emits ADD/SUB/...
     *     opcodes; when the runtime types are not both INT, dispatch falls
     *     back to the methods table (mln_lang_int_plus, etc.) which promotes
     *     int→real.  This exercises the non-fast-path branch in dispatch_one.
     * ------------------------------------------------- */
    T_REAL(lang, ev, "mixed_add",            "return 1 + 2.5;",                3.5);
    T_REAL(lang, ev, "mixed_sub",            "return 5.5 - 2;",                3.5);
    T_REAL(lang, ev, "mixed_mul",            "return 3 * 2.5;",                7.5);
    T_REAL(lang, ev, "mixed_div",            "return 5 / 2.0;",                2.5);
    T_TRUE(lang,  ev, "mixed_eq_true",       "return 2.0 == 2;");
    T_FALSE(lang, ev, "mixed_eq_false",      "return 2.0 == 3;");
    T_TRUE(lang,  ev, "mixed_lt",            "return 1 < 1.5;");
    T_TRUE(lang,  ev, "mixed_ge",            "return 2.5 >= 2;");

    /* -------------------------------------------------
     * 40. Bitwise / shift on variables (not literals).  The VM has separate
     *     fast-paths for INT-INT operands; these tests force both operands
     *     through LOAD_LOCAL before the opcode, matching what real code does.
     * ------------------------------------------------- */
    T_INT(lang, ev, "bor_vars",
          "@F() { a=3; b=5; return a | b; } return F();",                   7);
    T_INT(lang, ev, "band_vars",
          "@F() { a=6; b=5; return a & b; } return F();",                   4);
    T_INT(lang, ev, "bxor_vars",
          "@F() { a=6; b=5; return a ^ b; } return F();",                   3);
    T_INT(lang, ev, "lshift_vars",
          "@F() { a=1; b=4; return a << b; } return F();",                 16);
    T_INT(lang, ev, "rshift_vars",
          "@F() { a=64; b=2; return a >> b; } return F();",                16);
    T_INT(lang, ev, "bitnot_var_chain",
          "@F() { a=0xff; return ~a + 256; } return F();",                  0);

    /* -------------------------------------------------
     * 41. String equality / inequality.  Emits EQ / NE between non-INT
     *     operands; dispatch falls back to mln_lang_str_equal.
     * ------------------------------------------------- */
    T_TRUE(lang,  ev, "str_eq_true",         "return 'abc' == 'abc';");
    T_FALSE(lang, ev, "str_eq_false",        "return 'abc' == 'xyz';");
    T_TRUE(lang,  ev, "str_ne_true",         "return 'foo' != 'bar';");

    /* -------------------------------------------------
     * 42. Closure with reference-capture: $(&n).  Sequential calls share
     *     the captured variable through the same pointer; each call sees
     *     the side effects of the previous one.  (Inline expressions like
     *     c() + c() + c() have evaluation-order surprises in the AST
     *     walker too — using sequential statements isolates the VM path.)
     * ------------------------------------------------- */
    T_INT(lang, ev, "closure_ref_capture",
          "@mc() { n=0; @b() $(&n) { n = n + 1; return n; } return b; } "
          "c = mc(); a = c(); b = c(); d = c(); return a + b + d;",         6);
    T_INT(lang, ev, "closure_ref_isolated",
          /* Two independent counters from the same factory — the captured
           * `n` must be per-call, not shared globally. */
          "@mc() { n=0; @b() $(&n) { n=n+1; return n; } return b; } "
          "c1 = mc(); c2 = mc(); c1(); c1(); c1(); return c2();",           1);

    /* -------------------------------------------------
     * 43. Function-as-value.  A bare identifier referring to a user-defined
     *     function is loaded as a VAL_TYPE_FUNC variable and can be assigned
     *     to a local, passed as an arg, or invoked through the local
     *     (CALL_VALUE rather than CALL_SELF).
     * ------------------------------------------------- */
    T_INT(lang, ev, "func_as_value_assign",
          "@F() { return 42; } g = F; return g();",                        42);
    T_INT(lang, ev, "func_as_value_arg",
          "@Twice(x) { return x * 2; } "
          "@Apply(f, n) { return f(n); } "
          "return Apply(Twice, 21);",                                      42);
    T_INT(lang, ev, "func_as_value_returned",
          "@make() { @inner() { return 7; } return inner; } "
          "f = make(); return f() + f();",                                 14);

    /* -------------------------------------------------
     * 44. Many-argument function (>2 args).  Exercises arg-binding and
     *     populate_locals beyond the trivial case.
     * ------------------------------------------------- */
    T_INT(lang, ev, "func_seven_args",
          "@F(a,b,c,d,e,f,g) { return a*1+b*2+c*3+d*4+e*5+f*6+g*7; } "
          "return F(1,1,1,1,1,1,1);",                                       28);
    T_INT(lang, ev, "func_seven_args_nontrivial",
          "@F(a,b,c,d,e,f,g) { return a-b+c-d+e-f+g; } "
          "return F(10,1,10,1,10,1,10);",                                   37);

    /* -------------------------------------------------
     * 45. Method calling another method on the same set via this.
     *     CALL_METHOD opcode dispatches through the obj on the stack.
     * ------------------------------------------------- */
    T_INT(lang, ev, "method_calls_method",
          "C { v; "
          "  @add1() { this.v = this.v + 1; } "
          "  @add3() { this.add1(); this.add1(); this.add1(); } "
          "} "
          "c = $C; c.v = 10; c.add3(); return c.v;",                       13);
    T_INT(lang, ev, "method_simple_return",
          "C { @id() { return 7; } @doit() { return this.id(); } } "
          "c = $C; return c.doit();",                                       7);

    /* -------------------------------------------------
     * 46. Multi-level property access: o.i.v.  Each `.` lowers to a separate
     *     GET_PROPERTY; the prev_was_property flag in the compiler is what
     *     keeps the chain together for the final SET_PROPERTY.
     * ------------------------------------------------- */
    T_INT(lang, ev, "chained_property_read",
          "Inner { v; } Outer { i; } "
          "o = $Outer; o.i = $Inner; o.i.v = 99; return o.i.v;",           99);

    /* -------------------------------------------------
     * 47. Nested array literals (array of arrays) and 2-D indexing.
     *     Each inner [..] is an ARRAY_PUT chain at compile time; the outer
     *     consumes those vars as elements.
     * ------------------------------------------------- */
    T_INT(lang, ev, "array_of_arrays",
          "m = [[1,2,3],[4,5,6],[7,8,9]]; return m[1][2];",                 6);
    T_INT(lang, ev, "array_of_arrays_assign",
          "m = [[0,0],[0,0]]; m[1][1] = 42; return m[1][1];",              42);

    /* -------------------------------------------------
     * 48. Array passed to a function (by value/sharing — Melang arrays are
     *     reference-typed).  The callee should see the caller's contents.
     * ------------------------------------------------- */
    T_INT(lang, ev, "array_as_arg",
          "@sum3(a) { return a[0] + a[1] + a[2]; } "
          "return sum3([10, 20, 30]);",                                     60);

    /* -------------------------------------------------
     * 49. Empty array literal: `a = []` followed by index assignment.
     *     Exercises the NEW_ARRAY opcode with no ARRAY_PUT entries, and
     *     SET_INDEX growing the array on the fly.
     * ------------------------------------------------- */
    T_INT(lang, ev, "empty_array_grow",
          "a = []; a[0] = 11; a[1] = 22; a[2] = 33; "
          "return a[0] + a[1] + a[2];",                                     66);
    T_NIL(lang, ev,  "empty_array_unset",
          "a = []; return a[3];");

    /* -------------------------------------------------
     * 50. Deep recursion exercises the per-ctx vm_frame_t freelist.  The
     *     freelist cap is M_LANG_FRAME_FREELIST_MAX = 64 (mln_lang.h), so
     *     a depth-100 chain forces at least 36 fresh slab allocations on
     *     top of the recycled frames. Returns 100 (one increment per frame).
     * ------------------------------------------------- */
    T_INT(lang, ev, "deep_recursion_100",
          "@F(n) { if (n<=0) { return 0; } fi return F(n-1) + 1; } "
          "return F(100);",                                                100);

    /* -------------------------------------------------
     * 51. Dynamic-array growth in compile-time book-keeping (commit 345b09e).
     *     The compiler used to keep fixed-size buffers for breaks (16),
     *     continues (16), labels (32) and switch cases (64); the new
     *     inline+pool buffers grow past those caps.  These three tests
     *     deliberately overflow each one.
     * ------------------------------------------------- */
    {
        /* 51a. >16 break statements in one loop. The script enters the
         * while, hits the first break that triggers (k==5), but the
         * compiler still has to record patches for ALL 20 breaks. */
        char code[2048];
        char *p = code;
        p += snprintf(p, sizeof(code) - (p - code),
                      "@F(k) { while (true) {");
        for (int i = 0; i < 20; i++) {
            p += snprintf(p, sizeof(code) - (p - code),
                          " if (k==%d) { break; } fi", i);
        }
        p += snprintf(p, sizeof(code) - (p - code),
                      " break; } return k * 100; } return F(5);");
        T_INT(lang, ev, "many_breaks_20", code, 500);
    }
    {
        /* 51b. >16 continue statements in one for-loop. */
        char code[2048];
        char *p = code;
        p += snprintf(p, sizeof(code) - (p - code),
                      "@F() { s=0; for (i=0; i<20; i++) {");
        for (int i = 0; i < 20; i++) {
            p += snprintf(p, sizeof(code) - (p - code),
                          " if (i==%d) { continue; } fi", i);
        }
        p += snprintf(p, sizeof(code) - (p - code),
                      " s = s + i; } return s; } return F();");
        /* All 20 iterations skip; s stays 0. */
        T_INT(lang, ev, "many_continues_20", code, 0);
    }
    {
        /* 51c. >32 labels + gotos in one function. Goto L0; each label
         * falls through to the next, so all 35 increment statements run. */
        char code[4096];
        char *p = code;
        p += snprintf(p, sizeof(code) - (p - code),
                      "@F() { s=0; goto L0;");
        for (int i = 0; i < 35; i++) {
            p += snprintf(p, sizeof(code) - (p - code),
                          " L%d: s = s + 1;", i);
        }
        p += snprintf(p, sizeof(code) - (p - code),
                      " return s; } return F();");
        T_INT(lang, ev, "many_labels_35", code, 35);
    }
    {
        /* 51d. >64 switch cases.  Pick a case in the middle so we know
         * dispatch picks the right body. */
        char code[8192];
        char *p = code;
        p += snprintf(p, sizeof(code) - (p - code),
                      "@F(x) { switch (x) {");
        for (int i = 0; i < 80; i++) {
            p += snprintf(p, sizeof(code) - (p - code),
                          " case %d: { return %d; }", i, i * 100);
        }
        p += snprintf(p, sizeof(code) - (p - code),
                      " default: { return -1; } } } return F(70);");
        T_INT(lang, ev, "many_cases_80", code, 7000);
    }

    /* -------------------------------------------------
     * 52. Postfix ++/-- result on locals, when consumed inside an
     *     arithmetic expression (rather than discarded).  Exercises
     *     LOAD_LOCAL_INC/DEC's "push old, store new" path.
     * ------------------------------------------------- */
    T_INT(lang, ev, "postfix_in_expr",
          "@F() { i=10; r = i++ + i; return r; } return F();",             21);
    T_INT(lang, ev, "prefix_in_expr",
          "@F() { i=10; r = ++i + i; return r; } return F();",             22);

    /* -------------------------------------------------
     * 53. Set field default + method that returns this.field.  Verifies
     *     GET_PROPERTY on `this` inside a method.
     *
     *     NOTE: Melang has a known quirk (present in both AST walker and
     *     VM) where a method whose body is `this.field = <param>` causes
     *     the call to leave the object on the operand stack rather than
     *     the field value, which then leaks into ret_var.  The compound
     *     read-modify-write form `this.field = this.field + <expr>` is
     *     the safe pattern used throughout the standard tests; we use it
     *     here so the test is robust against that quirk.
     * ------------------------------------------------- */
    T_INT(lang, ev, "method_returns_this_field",
          "Box { v; @get() { return this.v; } @bump() { this.v = this.v + 1; } } "
          "b = $Box; b.v = 122; b.bump(); return b.get();",               123);

    /* -------------------------------------------------
     * 54. Compound assign with a non-trivial right-hand expression.
     *     Confirms the RHS is fully reduced before the op-store back.
     * ------------------------------------------------- */
    T_INT(lang, ev, "compound_pluseq_expr_rhs",
          "@F() { a=10; a += 2 * 3 + 4; return a; } return F();",          20);
    T_INT(lang, ev, "compound_muleq_expr_rhs",
          "@F() { a=2; a *= (1 + 2 + 3); return a; } return F();",         12);
    T_INT(lang, ev, "compound_oreq_expr_rhs",
          "@F() { a=1; a |= (2 | 4); return a; } return F();",              7);

    /* -------------------------------------------------
     * 55. Big iterative + recursive workload, larger than fib_10.
     *     fib(20)=6765 is well past the int fast-path warm-up; if the VM
     *     mishandles any call/return path we'll see it in the totals.
     * ------------------------------------------------- */
    T_INT(lang, ev, "fib_20",
          "@Fib(n) { if (n<=2) { return 1; } fi return Fib(n-1) + Fib(n-2); } "
          "return Fib(20);",                                             6765);

    /* -------------------------------------------------
     * 56. Bitwise NOT operator (~).
     *
     * PR comment: "The VM compiler still bails out on several existing
     * unary spec operators (for example ~/M_SPEC_REVERSE)."
     * Resolution: MLN_VOP_BITNOT added and dispatched.
     *
     * Simple direct tests of ~x on various int values.
     * ------------------------------------------------- */
    T_INT(lang, ev, "bitnot_zero2",
          "return ~0;",                                                  -1);
    T_INT(lang, ev, "bitnot_one",
          "return ~1;",                                                  -2);
    T_INT(lang, ev, "bitnot_minus1",
          "return ~(-1);",                                                0);
    T_INT(lang, ev, "bitnot_local",
          "@F() { a = 5; return ~a; } return F();",                      -6);
    T_INT(lang, ev, "bitnot_double",
          "return ~~42;",                                                42);

    /* -------------------------------------------------
     * 57. Operator overload: __int_plus_operator__.
     *
     * PR comment: "This compile-time guard only looks at the
     * operator-overload flags that are already set on the current
     * context. Because the whole top-level chunk is compiled before
     * later __* overload functions in the same script are bound,
     * scripts that define an overload and then use the overloaded
     * operator afterward will still be compiled down the non-overload
     * VM path."
     *
     * Resolution: compile-time guard removed; apply_binop checks
     * !ctx->op_int_flag at runtime on every dispatch.
     *
     * The overload function may safely use + itself (the re-entrancy
     * guard in mln_lang_funccall_val_operator_overload_test detects
     * the call inside the overload scope and falls back to default +).
     * ------------------------------------------------- */
    /* Basic: overload returns a+b+1 instead of a+b */
    T_INT(lang, ev, "op_overload_plus_basic",
          "@__int_plus_operator__(a, b) { return a + b + 1; } "
          "return 3 + 4;",
          8);  /* 3+4+1 = 8 */

    /* Overload receives the correct operands */
    T_INT(lang, ev, "op_overload_plus_operands",
          "@__int_plus_operator__(a, b) { return a * 100 + b; } "
          "return 5 + 7;",
          507);  /* 5*100+7 = 507 */

    /* -------------------------------------------------
     * 58. vm_state caching with operator overloads.
     *
     * PR comment: "vm_state is cached permanently after the first call,
     * but whether the VM path is valid depends on mutable per-context
     * overload flags. If a function is compiled before a later __*
     * overload definition runs, subsequent calls will keep executing
     * the stale bytecode instead of switching to the overload-aware
     * path."
     *
     * Resolution: apply_binop now checks ctx->op_int_flag at runtime
     * on every call. The compiled bytecode is always valid; what changes
     * is only whether the int fast-path is taken or the methods table.
     *
     * Test: compile @F() before defining __int_plus_operator__; call F()
     * before and after the overload definition; verify the results differ.
     * ------------------------------------------------- */
    T_INT(lang, ev, "op_overload_after_compile",
          /* F() is compiled with ADD opcode; no overload flag yet.
           * First call: 10+5 = 15 (normal).
           * Then __int_plus_operator__ is defined → op_int_flag = 1.
           * Second call: apply_binop detects op_int_flag, routes through
           * methods table, calls overload → 10+5+1 = 16.
           * return r2 - r1 = 16 - 15 = 1 (SUB is not overloaded). */
          "@F() { return 10 + 5; } "
          "r1 = F(); "
          "@__int_plus_operator__(a, b) { return a + b + 1; } "
          "r2 = F(); "
          "return r2 - r1;",
          1);  /* 16 - 15 = 1 */

    /* -------------------------------------------------
     * 59. Multiple operator overloads: __int_plus_operator__ and
     *     __int_mul_operator__ active at the same time.  Verifies that
     *     each opcode independently routes to its own overload.
     *
     * + overload: a+b+10 (the suppression guard prevents recursion
     *   inside __int_plus_operator__ itself, but + IS overloaded when
     *   executing inside the * overload body).
     * * overload: a*b+100
     *
     * (3+4)*2:
     *   1. (3+4) → __int_plus_operator__(3,4) → 3+4+10 = 17
     *      (inner + ops are default because we're in the + overload scope)
     *   2. 17*2 → __int_mul_operator__(17,2) → 17*2 + 100
     *      * is default inside the * overload; but + is NOT suppressed here
     *      → 34 + 100 calls __int_plus_operator__(34,100) → 34+100+10 = 144
     * ------------------------------------------------- */
    T_INT(lang, ev, "op_overload_plus_and_mul",
          "@__int_plus_operator__(a, b) { return a + b + 10; } "
          "@__int_mul_operator__(a, b)  { return a * b + 100; } "
          "return (3 + 4) * 2;",
          144);  /* see comment above */

    /* -------------------------------------------------
     * 60. Tagged-value operand stack (high-cost-tier perf work).
     *
     * The opstack now stores tagged 16-byte mln_lang_value_t slots
     * instead of mln_lang_var_t* pointers. Scalars (int/bool/nil/real)
     * stay unboxed on the stack; vars carry a borrow bit so DUP and
     * LOAD_LOCAL skip ref-count traffic. These tests pin down the
     * subtle interactions that the rework must preserve.
     * ------------------------------------------------- */

    /* a) Hot integer arithmetic round-trip — exercises the unboxed
     *    int fast path in MLN_VOP_ADD/SUB/MUL/CMP. */
    T_INT(lang, ev, "tv_int_add",          "return 1 + 2 + 3 + 4 + 5;",                       15);
    T_INT(lang, ev, "tv_int_chain",        "return ((10-3)*2 + 1) * 2 + 7;",                  37);
    /* Negative-int and large-int round trip — exercises the unboxed
     * 64-bit slot's full int range. */
    T_INT(lang, ev, "tv_int_neg_arith",    "return -1000000 + 999999;",                       -1);
    T_INT(lang, ev, "tv_int_large_mul",    "return 1000000 * 1000000;",       1000000000000LL);
    T_TRUE (lang, ev, "tv_cmp_lt",         "return 1 < 2;");
    T_FALSE(lang, ev, "tv_cmp_lt_neg",     "return 2 < 1;");
    T_TRUE (lang, ev, "tv_cmp_eq",         "return 1+2 == 3;");
    T_FALSE(lang, ev, "tv_cmp_ne",         "return 1+2 != 3;");
    T_INT(lang, ev, "tv_int_bitops",       "return (0xff | 0x300) ^ 0xf0;",                   0x30f);
    T_INT(lang, ev, "tv_int_shift",        "return (1 << 8) | (1 << 4) | 1;",                 0x111);

    /* b) NEG / BITNOT on unboxed ints — these now bypass var alloc. */
    T_INT(lang, ev, "tv_neg_int",          "return -42;",                                    -42);
    T_INT(lang, ev, "tv_neg_double",       "return -(-7);",                                    7);
    T_INT(lang, ev, "tv_bitnot_int",       "return ~0;",                                      -1);
    T_INT(lang, ev, "tv_bitnot_chain",     "return ~~~0xa;",                                 ~10);

    /* c) NOT operator on each unboxed kind — exercises value_is_truthy. */
    T_TRUE (lang, ev, "tv_not_zero",       "return !0;");
    T_FALSE(lang, ev, "tv_not_int",        "return !42;");
    T_TRUE (lang, ev, "tv_not_nil",        "return !nil;");
    T_FALSE(lang, ev, "tv_not_true",       "return !true;");
    T_TRUE (lang, ev, "tv_not_false",      "return !false;");
    T_FALSE(lang, ev, "tv_not_real",       "return !1.5;");

    /* d) Conditional flow with unboxed bool/int — JUMP_IF_FALSE/TRUE
     *    walks value_is_truthy without materializing. */
    T_INT(lang, ev, "tv_if_int",
          "@F(x) { if (x) {return 1;} fi return 0; } "
          "return F(0) + F(1) + F(42) + F(-1);",
          3);
    T_INT(lang, ev, "tv_while_int",
          "@F(n) { s=0; i=0; while (i < n) { s = s+i; i = i+1; } return s; } "
          "return F(10);",  /* 0+1+...+9 = 45 */
          45);

    /* e) DUP borrow bit — the duplicated slot does NOT own a ref, so
     *    consuming it must not over-decrement. The postfix++ → store
     *    pattern is the canonical multi-DUP path. */
    T_INT(lang, ev, "tv_dup_in_chain",
          "@F() { a=5; b=a; c=b; return a+b+c; } return F();",
          15);
    T_INT(lang, ev, "tv_dup_borrow_compound",
          /* postfix ++ in expression: pushes old, increments slot.
           * The "return r * 100 + i" reads i again — borrow must
           * survive the intervening store. */
          "@F() { i=10; r = i++ + i; return r; } return F();",
          21);  /* r = 10 + 11 */

    /* f) Return-value materialization — RETURN materializes the top
     *    tagged value into an owned var the caller's stack receives. */
    T_INT(lang, ev, "tv_ret_int_lit",      "@F() { return 17; } return F();",                17);
    T_INT(lang, ev, "tv_ret_int_borrow",   "@F() { x=99; return x; } return F();",           99);
    T_INT(lang, ev, "tv_ret_chain",
          "@F() { return G() + H(); } "
          "@G() { return 7; } "
          "@H() { return 8; } "
          "return F();",
          15);
    T_INT(lang, ev, "tv_ret_local_fused",  /* RETURN_LOCAL super-instruction */
          "@F() { x=42; return x; } return F();",                                             42);

    /* g) Self-recursion (CALL_SELF) — args go through value_take_var
     *    at the call boundary, then come back into slots as owned vars. */
    T_INT(lang, ev, "tv_recur_fib",
          "@F(i) { if (i <= 2) {return 1;} fi return F(i-1) + F(i-2); } return F(10);",
          55);

    /* h) Closures — captured vars must remain valid; the borrow-bit
     *    optimization on LOAD_LOCAL must not break captured-var lifetime.
     *    Use the same $(&n) capture syntax as section 42. */
    T_INT(lang, ev, "tv_closure_capture",
          "@make() { n=100; @inner() $(&n) { return n; } return inner; } "
          "f = make(); return f();",
          100);

    /* i) Reference parameters (&x) — LOAD_LOCAL_REF must still construct
     *    a VAR_REFER, even though plain LOAD_LOCAL pushes a borrow. */
    T_INT(lang, ev, "tv_ref_param",
          "@bump(&v) { v = v + 1; } @F() { x=5; bump(&x); return x; } return F();",
          6);

    /* j) Mixed-type arithmetic (slow path) — int+real should fall
     *    through to apply_binop via value_take_var. */
    T_REAL(lang, ev, "tv_int_plus_real",   "return 1 + 2.5;",                                3.5);
    T_REAL(lang, ev, "tv_real_minus_int",  "return 5.5 - 2;",                                3.5);

    /* k) Property access fast path on tagged stack — IC + borrow on
     *    member reads must produce identical observable results.
     *    Note: Melang spells the set keyword without "Set", and
     *    instantiation is `$Name` (no parens). Methods reference
     *    fields via `this.x`.
     *
     *    NB: there is a pre-existing language quirk (visible on
     *    origin/master too) where calling a method that takes one
     *    or more arguments and whose RETURN expression depends on
     *    those arguments produces a NULL return-value at the call
     *    site. The same quirk is acknowledged in test #53
     *    (`method_returns_this_field`). To stay focused on the
     *    tagged-value rework, this test exercises the property
     *    access fast path through a no-arg method that mutates and
     *    re-reads `this.x`, which is not affected by the quirk. */
    T_INT(lang, ev, "tv_set_prop_chain",
          "Box2 { x; @bump5() { this.x = this.x + 5; } } "
          "@F() { o = $Box2; o.x = 10; o.bump5(); r = o.x; o.x = r + 1; return o.x; } "
          "return F();",
          16);

    /* l) Array fill + sum — exercises NEW_ARRAY, ARRAY_PUT (peek as
     *    VAR), GET_INDEX, all under the new tagged stack. */
    T_INT(lang, ev, "tv_array_sum",
          "@F() { a = [1, 2, 3, 4, 5]; s=0; for (i=0; i<5; i++) { s = s + a[i]; } return s; } "
          "return F();",
          15);

    /* m) Watch elision interaction — STORE_LOCAL on a watched slot must
     *    fire vm_fire_watcher with the materialized value. */
    T_INT(lang, ev, "tv_watch_local",
          "@cb(&v, &n) { n = n + 1; } "
          "@F() { x = 0; n = 0; Watch(x, cb, &n); x = 1; x = 2; x = 3; Unwatch(x); return n; } "
          "return F();",
          3);

    /* n) Compound assignment on global (ASSIGN_GLOBAL borrow re-push). */
    T_INT(lang, ev, "tv_compound_global",
          "g = 1; g = g + 10; g = g * 2; return g;",
          22);

    /* o) Postfix ++ on local — emits LOAD_LOCAL_INC which pushes the
     *    OLD value as an unboxed int. */
    T_INT(lang, ev, "tv_postfix_local",
          "@F() { i=5; r = i++; return r * 100 + i; } return F();",
          506);  /* r=5, i=6 → 5*100+6 */

    /* p) Prefix ++ on local — emits INC_LOCAL_LOAD pushing NEW value. */
    T_INT(lang, ev, "tv_prefix_local",
          "@F() { i=5; r = ++i; return r * 100 + i; } return F();",
          606);  /* r=6, i=6 → 6*100+6 */

    /* q) Super-instruction ADD_LL on int slots — should hit the inlined
     *    fast path that pushes an unboxed int. */
    T_INT(lang, ev, "tv_super_add_ll",
          "@F(a, b) { return a + b; } return F(7, 11);",
          18);

    /* r) Super-instruction ADD_LI (local + iconst) — same fast path. */
    T_INT(lang, ev, "tv_super_add_li",
          "@F(a) { return a + 100; } return F(23);",
          123);

    /* s) Super-instruction LT_LL fast path. */
    T_TRUE(lang, ev, "tv_super_lt_ll",
          "@F(a, b) { return a < b; } return F(3, 7);");

    /* t) Top-of-stack in-place via DUP+POP — should round-trip cleanly
     *    even when the value is unboxed. Uses if/fi (no `else`/`fi` is
     *    spelt with two if's in this dialect — we use the if/fi form). */
    T_INT(lang, ev, "tv_dup_pop_clean",
          "@F() { a=1; b=2; r=99; if (a < b) {r = a;} fi return r; } return F();",
          1);

    /* u) String values on the tagged stack — strings stay boxed, so
     *    every push/pop must go through value_take_var with proper
     *    refcount handling. */
    T_STR(lang, ev, "tv_string_concat_chain",
          "@F() { a = 'hello'; b = 'world'; return a + ' ' + b + '!'; } return F();",
          "hello world!");
    T_STR(lang, ev, "tv_string_pass_through_var",
          "@F() { s = 'abc'; t = s; u = t; return u; } return F();",
          "abc");
    T_INT(lang, ev, "tv_string_compare_eq",
          "@F() { if ('foo' == 'foo') {return 1;} fi return 0; } return F();",
          1);
    T_INT(lang, ev, "tv_string_compare_ne",
          "@F() { if ('foo' == 'bar') {return 1;} fi return 0; } return F();",
          0);

    /* v) Real-only arithmetic on the slow path — apply_binop must
     *    materialize both operands cleanly. */
    T_REAL(lang, ev, "tv_real_arith_chain",
          "@F() { return 1.5 * 2.0 + 0.5; } return F();",
          3.5);
    T_REAL(lang, ev, "tv_real_negate_chain",
          "@F() { x = -2.5; return -x + 1.0; } return F();",
          3.5);

    /* w) Logical short-circuit on unboxed bools — only the LHS is
     *    consumed when the result is determined; the RHS opstack
     *    push must not leak when the jump skips it. */
    T_INT(lang, ev, "tv_logical_or_short",
          "@F() { a = 1; if (a || (1/0)) {return 7;} fi return 0; } return F();",
          7);
    T_INT(lang, ev, "tv_logical_and_short",
          "@F() { a = 0; if (a && (1/0)) {return 1;} fi return 9; } return F();",
          9);

    /* x) Switch over unboxed int — every case-compare must handle the
     *    unboxed scrutinee without box/unbox round-trips. */
    T_INT(lang, ev, "tv_switch_int",
          "@F(n) { switch (n) { case 1: return 100; case 2: return 200; default: return -1; } } "
          "return F(2);",
          200);

    /* y) Index write-then-read — SET_INDEX on an unboxed int RHS,
     *    GET_INDEX returning an unboxed int. Borrow chain on a[i]
     *    must not double-decref when the slot is reassigned. */
    T_INT(lang, ev, "tv_index_assign_read",
          "@F() { a = [0, 0, 0]; a[0] = 11; a[1] = 22; a[2] = 33; "
          "       return a[0] + a[1] + a[2]; } return F();",
          66);

    /* z) Long expression chain — many DUP / LOAD_LOCAL / arithmetic
     *    ops in one frame. Stresses the borrow-bit + frame stack-depth
     *    accounting under realistic expression density.
     *
     *    Compute: (a+b)*c=9, (d-a)*e=15, 9+15-b=22, 22*(c-a)=44,
     *             +(a+b+c+d+e)=44+15=59. */
    T_INT(lang, ev, "tv_long_expr_chain",
          "@F() { a=1; b=2; c=3; d=4; e=5; "
          "       return ((a+b)*c + (d-a)*e - b) * (c-a) + (a+b+c+d+e); } "
          "return F();",
          59);

    /* aa) Operator overload routed through the AST-call path — when an
     *     overload fires, the binop handler materializes both operands
     *     into vars and re-enters as a function call. Confirms the
     *     borrow + materialize boundary stays correct when the binop
     *     fast path is bypassed. Uses the same overload mechanism as
     *     op_overload_plus_basic but in a deeper call frame. */
    T_INT(lang, ev, "tv_op_overload_in_frame",
          "@__int_plus_operator__(a, b) { return a + b + 100; } "
          "@F() { x = 1; y = 2; return x + y; } "
          "return F();",
          103);  /* 1+2+100 */

    /* bb) Builtin call boundary — Dump/sys-style builtins receive
     *     materialized mln_lang_var_t* via the methods table. The
     *     return-int from a builtin lands as an owned VAR on the
     *     tagged stack. Use a builtin we know is always available. */
    T_NONE(lang, ev, "tv_builtin_call_boundary",
          "@F() { Dump(); return; } F();");

    /* cc) Deep nested function returns — each RETURN materializes,
     *     each caller's resume-path must correctly rebox into its slot. */
    T_INT(lang, ev, "tv_nested_returns",
          "@A(n) { return n + 1; } "
          "@B(n) { return A(n) + 10; } "
          "@C(n) { return B(n) * 2; } "
          "@F() { return C(3); } return F();",
          28);  /* C(3) = B(3)*2 = (A(3)+10)*2 = (4+10)*2 = 28 */

    /* dd) AST fallback — a function body that exceeds the VM compiler's
     *     loop-nesting limit (MLN_VM_MAX_LOOPS=16) must fall back to the
     *     AST stack-walker transparently and still return the correct value.
     *     17 nested while(1){...break} loops trigger vm_state==-1. */
    T_INT(lang, ev, "tv_ast_fallback_deep_loops",
          "@F() {"
          "  while(1){while(1){while(1){while(1){while(1){"
          "  while(1){while(1){while(1){while(1){while(1){"
          "  while(1){while(1){while(1){while(1){while(1){"
          "  while(1){while(1){return 99;}"
          "  break;}break;}break;}break;}break;}"
          "  break;}break;}break;}break;}break;}"
          "  break;}break;}break;}break;}break;}"
          "  break;}"
          "  return 0;"
          "}"
          "return F();",
          99);

    /* =========================================================
     * Section 33: Complex bug-hunting tests
     *
     * These are designed to stress specific code paths in mln_lang_vm.c
     * with the goal of finding latent bugs:
     *  - frame freelist + opstack growth under deep recursion
     *  - DUP borrow-bit correctness when an opstack value's underlying
     *    var is re-stored into a slot mid-expression
     *  - operator-overload re-entrancy during a VM-compiled function
     *  - reference parameters that point at slot vars (slot val mutation
     *    must propagate back to the caller, but the wrapper var must not
     *    be kept after the call returns)
     *  - mixed-ref/by-value parameter declarations (regression test for
     *    the funcdef_args_get bug where a leading &-prefixed parameter
     *    caused all subsequent parameters to be silently created as
     *    REFER and aliased to the caller's variable)
     *  - postfix/prefix ++ on slots and globals
     *  - mixed int/real arithmetic chain that bounces between fast paths
     *    and the methods-table dispatch
     *  - deep cross-function call chain that forces opstack growth
     *  - val/var freelist correctness when the same val is recycled
     *    multiple times in a single ctx
     *  - early return inside a switch/for/goto interaction
     *  - global lookup via LOAD_GLOBAL that creates a new slot
     *    (LOAD then STORE then re-LOAD)
     * ========================================================= */

    /* 33.a Deep recursion (>= 64 frames). Forces vm_frame_freelist to fill
     * to its M_LANG_FRAME_FREELIST_MAX cap and exercise the
     * "freelist full -> mln_alloc_free" path during pop. */
    T_INT(lang, ev, "complex_deep_recursion",
          "@C(n) { if (n <= 0) { return 0; } fi return 1 + C(n - 1); } "
          "return C(200);", 200);

    /* 33.b Heavy non-tail recursive chain that grows the operand stack:
     * each level holds (n+1) live values before tail-summing. Forces
     * vm_frame_grow_opstack to exercise the realloc path. */
    T_INT(lang, ev, "complex_opstack_growth",
          "@G(n) { "
          "  if (n <= 0) { return 0; } fi "
          "  return n + (n-1) + (n-2) + (n-3) + (n-4) + (n-5) + (n-6) + (n-7) + G(n-8); "
          "} "
          "return G(40);",
          /* G(n) = n+(n-1)+...+(n-7) + G(n-8). Closed form: total = sum
           * 0..40 = 820. */
          820);

    /* 33.c DUP-borrow stress: the same slot var is read 5 times in one
     * expression. With the borrow-bit optimization, only one DUP should
     * own; the others are non-owning views. If a release path mistakenly
     * frees the underlying var, we'd crash. Verifies repeated borrow. */
    T_INT(lang, ev, "complex_dup_borrow_5x",
          "@F() { x = 7; return x + x + x + x + x; } return F();",
          35);

    /* 33.d Aliased val mutation: in Melang, `y = x` copies the value
     * (not the val pointer), so y++ must NOT alter x. */
    T_INT(lang, ev, "complex_assign_no_alias",
          "@F() { x = 5; y = x; y++; return x; } return F();",
          5);

    /* 33.e Reference parameter (&) at call site mutates caller; non-&
     * argument must remain untouched.  Direct VM-only check (the AST
     * walker rejects `&` in calls). */
    T_INT(lang, ev, "complex_ref_param_mutates",
          "@inc(x, y) { x = x + 100; y = y + 100; return; } "
          "@F() { a = 1; b = 2; inc(&a, b); return a * 1000 + b; } "
          "return F();",
          /* a became 101 (by ref); b stayed 2. */
          101 * 1000 + 2);

    /* 33.e2 REGRESSION TEST for the funcdef_args_get bug.  When the
     * function declaration has `&x` for the first parameter and `y`
     * (no &) for the second, calling inc(a, b) used to alias b into
     * the caller because `type` was not reset between iterations of
     * funcdef_args_get -- so y's protocol var was stamped REFER and
     * mln_lang_var_transform's REFER branch aliased the caller's val
     * into the callee slot.  After the fix, b stays 2. */
    T_INT(lang, ev, "complex_decl_mixed_ref",
          "@inc(&x, y) { x = x + 100; y = y + 100; return; } "
          "@F() { a = 1; b = 2; inc(&a, b); return a * 1000 + b; } "
          "return F();",
          /* a=101 (decl &x AND call &a), b=2 (decl y, no & at call). */
          101 * 1000 + 2);

    /* 33.e3 REGRESSION TEST: 3-arg variant where only the first param
     * is `&`-declared.  Confirms the bug fix flushes `type` per-arg
     * even when more than one trailing arg is by-value. */
    T_INT(lang, ev, "complex_decl_mixed_ref_3args",
          "@modify(&x, y, z) { x = x + 100; y = y + 100; z = z + 100; return; } "
          "@F() { a = 1; b = 2; c = 3; modify(&a, b, c); "
          "       return a*1000000 + b*1000 + c; } "
          "return F();",
          /* a=101 (ref), b=2 (by-value), c=3 (by-value). */
          101 * 1000000 + 2 * 1000 + 3);

    /* 33.f Operator overload defined AFTER a function compiled.  Once
     * `__int_plus_operator__` is in scope, the second F() call dispatches
     * the overload.  Inside the overload body, `+ 1` triggers the overload
     * recursively (Melang scopes op_int_flag per binop, not per call), so
     * the body computes ((a+b)+1)+1 = a+b+2.  Both AST and VM must agree
     * on this 5/7 sequence. */
    T_INT(lang, ev, "complex_overload_post_compile",
          "@F() { return 2 + 3; } "
          "first = F(); "
          "@__int_plus_operator__(a, b) { return a + b + 1; } "
          "second = F(); "
          "return first * 1000 + second;",
          5 * 1000 + 7);

    /* 33.g Closure capturing a mutable counter via $(&). */
    T_INT(lang, ev, "complex_closure_counter",
          "@make_counter() { "
          "  count = 0; "
          "  @inc()$(&count) { count = count + 1; return count; } "
          "  return inc; "
          "} "
          "f = make_counter(); "
          "a = f(); b = f(); c = f(); d = f(); e = f(); "
          "return a*10000 + b*1000 + c*100 + d*10 + e;",
          12345);

    /* 33.h Two independent counters made by the same factory must
     * not share state.  After f() called 3 times and g() called 1,
     * next f() returns 4 and next g() returns 2. */
    T_INT(lang, ev, "complex_closure_isolation",
          "@make_counter() { "
          "  count = 0; "
          "  @inc()$(&count) { count = count + 1; return count; } "
          "  return inc; "
          "} "
          "f = make_counter(); g = make_counter(); "
          "f(); f(); f(); /* f counter is 3 */ "
          "g(); /* g counter is 1 */ "
          "return f() * 100 + g();",
          /* next f() = 4; next g() = 2 */
          4 * 100 + 2);

    /* 33.i Switch with default fall-through.  The match misses every
     * explicit case so default's body must run. */
    T_INT(lang, ev, "complex_switch_default",
          "@F(n) { "
          "  r = 0; "
          "  switch (n) { "
          "    case 1: { r = 11; } "
          "    case 2: { r = 22; } "
          "    default: { r = 99; } "
          "  } "
          "  return r; "
          "} "
          "return F(7);",
          99);

    /* 33.j Mixed int/real arithmetic chain. */
    T_REAL(lang, ev, "complex_mixed_int_real",
          "@F() { x = 1; y = 0.5; return x + y + x + y + x; } "
          "return F();",
          4.0);

    /* 33.k Return-from-loop. */
    T_INT(lang, ev, "complex_return_in_loop",
          "@F() { "
          "  i = 0; "
          "  for (i = 0; i < 100; i++) { "
          "    if (i == 7) { return i * i; } fi "
          "  } "
          "  return -1; "
          "} "
          "return F();",
          49);

    /* 33.l Postfix ++ used as expression value. */
    T_INT(lang, ev, "complex_postfix_in_expr",
          "@F() { i = 5; r = (i++) * 10 + i; return r; } return F();",
          /* (i++) yields 5, i becomes 6, then i is 6: 5*10 + 6 */
          56);

    /* 33.m Prefix ++ used as expression value. */
    T_INT(lang, ev, "complex_prefix_in_expr",
          "@F() { i = 5; r = (++i) * 10 + i; return r; } return F();",
          /* (++i) yields 6, then i is 6: 6*10 + 6 */
          66);

    /* 33.n Bitwise NOT and shift chain.  ~7 = -8; (-8) << 2 = -32; (-32) >> 1 = -16. */
    T_INT(lang, ev, "complex_bit_chain",
          "@F() { x = ~7; y = x << 2; return y >> 1; } return F();",
          -16);

    /* 33.o Long expression chain that the VM compiler may constant-fold. */
    T_INT(lang, ev, "complex_long_const_fold",
          "@F() { return 1+2+3+4+5+6+7+8+9+10+11+12+13+14+15+16+17+18+19+20; } "
          "return F();",
          210);

    /* 33.p Array growth + index assignment via VM. */
    T_INT(lang, ev, "complex_array_grow",
          "@F() { arr = []; "
          "  i = 0; while (i < 10) { arr[i] = i * i; i++; } "
          "  return arr[3] + arr[7]; "
          "} "
          "return F();",
          9 + 49);

    /* 33.q Property write-then-read on a freshly-instantiated set. */
    T_INT(lang, ev, "complex_set_property",
          "Box { x; y; } "
          "@F() { b = $Box; b.x = 7; b.y = 11; return b.x * b.y; } "
          "return F();",
          77);

    /* 33.r Re-entry via Eval. */
    T_INT(lang, ev, "complex_eval_reentry",
          "@F() { "
          "  x = 100; "
          "  Eval('y=42; return y;'); "
          "  return x; "
          "} return F();",
          100);

    /* 33.s Loop with break + continue interleaved. */
    T_INT(lang, ev, "complex_loop_break_continue",
          "@F() { "
          "  s = 0; "
          "  for (i = 0; i < 20; i++) { "
          "    if (i % 3 == 0) { continue; } fi "
          "    if (i > 10) { break; } fi "
          "    s = s + i; "
          "  } "
          "  return s; "
          "} return F();",
          /* i=1,2,4,5,7,8,10 -> 1+2+4+5+7+8+10 = 37 */
          37);

    /* 33.t goto across a label that comes BEFORE the goto in source order. */
    T_INT(lang, ev, "complex_goto_backward",
          "@F() { "
          "  i = 0; s = 0; "
          "  loop: "
          "  if (i >= 5) { return s; } fi "
          "  s = s + i; "
          "  i = i + 1; "
          "  goto loop; "
          "} return F();",
          0+1+2+3+4); /* 10 */

    /* 33.u Comma expression as condition. */
    T_INT(lang, ev, "complex_comma_in_cond",
          "@F() { "
          "  init = 0; "
          "  if ((init = 1, init > 0)) { return init * 10; } fi "
          "  return -1; "
          "} return F();",
          10);

    /* 33.v Empty function body returns nil. */
    T_NIL(lang, ev, "complex_empty_function",
          "@F() {} return F();");

    /* 33.w Recursive function called via a global variable lookup. */
    T_INT(lang, ev, "complex_indirect_call",
          "@H(n) { if (n <= 0) { return 0; } fi return 1 + H(n - 1); } "
          "f = H; "
          "return f(50);",
          50);

    /* 33.x Set with a method that returns a constant. */
    T_INT(lang, ev, "complex_set_method",
          "P { @get() { return 42; } } "
          "@F() { p = $P; return p.get(); } "
          "return F();",
          42);

    /* 33.y Long string built via concatenation + comparison. */
    T_TRUE(lang, ev, "complex_string_build",
          "@F() { "
          "  s = 'a'; "
          "  i = 0; "
          "  while (i < 5) { s = s + 'b'; i = i + 1; } "
          "  return s == 'abbbbb'; "
          "} return F();");

    /* 33.z High-arity function (5 args) -- exercises the n_args binding
     * loop in vm_push_frame and confirms the funcdef_args_get bug fix
     * keeps every parameter in the by-value path. */
    T_INT(lang, ev, "complex_arity_5",
          "@A(a, b, c, d, e) { return a + b*2 + c*3 + d*4 + e*5; } "
          "return A(1, 2, 3, 4, 5);",
          1 + 4 + 9 + 16 + 25);

    /* 33.aa Reference parameter with closure: a closure captures a
     * slot whose val is shared via &.  After the closure runs, the
     * outer caller's slot reflects the change. */
    T_INT(lang, ev, "complex_ref_via_closure",
          "@bump(&v) { @inner()$(&v) { v = v + 7; return; } inner(); return; } "
          "@F() { x = 100; bump(&x); return x; } return F();",
          107);

    /* 33.bb Switch: exact match path with explicit return per case. */
    T_INT(lang, ev, "complex_switch_match",
          "@F(n) { "
          "  switch (n) { "
          "    case 0: { return 0; } "
          "    case 1: { return 11; } "
          "    case 2: { return 22; } "
          "    case 3: { return 33; } "
          "    default: { return -1; } "
          "  } "
          "  return -2; "
          "} "
          "return F(2);",
          22);

    /* -------------------------------------------------
     * 34. Eval co-existence with operator overloads.
     *
     * Melang's Eval() always creates a NEW, isolated context.  It cannot
     * inject definitions into the calling context's scope.  However, if
     * the top-level AST contains ANY Eval() call the VM compiler
     * conservatively pre-sets ctx->op_int_flag = 1 before compiling the
     * top-level chunk.  This prevents compile-time constant folding of
     * integer literals so that any __int_*_operator__ defined DIRECTLY in
     * the same script is still honoured by the emitted opcodes at runtime.
     *
     * These tests verify:
     *   (a) Pre-scan + direct overload: a function compiled before the
     *       overload definition still picks up the overload on its next
     *       call when Eval() is also present in the script.
     *   (b) Pre-scan without overload: arithmetic in a script that
     *       contains Eval() but no overload returns the correct default
     *       result (no constant fold, no spurious dispatch).
     *   (c) Eval nested inside a helper body: the pre-scan's recursive
     *       descent into function bodies detects the Eval(), so the same
     *       conservative no-fold guarantee applies at the top level.
     * ------------------------------------------------- */

    /* 34.a  Eval present AND a direct overload defined later in the script.
     * The pre-scan finds Eval → sets ctx->op_int_flag = 1 → disables
     * constant folding.  F() is first compiled with safe_to_fold = 0.
     * After @__int_plus_operator__ is defined, the SECOND call to F()
     * routes through the overload: 3+4+1 = 8.  Difference = 1. */
    T_INT(lang, ev, "eval_present_overload_func_cache",
          "@F() { return 3 + 4; } "
          "r1 = F(); "                  /* no overload yet => 7 */
          "Eval('return 0;'); "         /* Eval in AST: conservative pre-scan */
          "@__int_plus_operator__(a, b) { return a + b + 1; } "
          "r2 = F(); "                  /* overload active => 3+4+1 = 8 */
          "return r2 - r1;",            /* 8 - 7 = 1 */
          1);

    /* 34.b  Eval present, no overload defined in this script.
     * Pre-scan sets op_int_flag = 1, so 5+6 emits LOAD_INT + LOAD_INT +
     * ADD (not a folded LOAD_INT 11).  At runtime Eval runs a separate
     * context and defines nothing in the current scope.  apply_binop
     * finds no __int_plus_operator__ → falls through to default
     * arithmetic → 5+6 = 11 (the correct unoverridden result). */
    T_INT(lang, ev, "eval_present_no_overload",
          "Eval('return 0;'); "         /* Eval in AST: op_int_flag pre-set */
          "return 5 + 6;",              /* no overload in scope => 11 */
          11);

    /* 34.c  Eval nested inside a helper function body.
     * scan_stm_for_eval_call recurses into function bodies, so the Eval
     * inside helper() is still detected and op_int_flag is pre-set.
     * Arithmetic follows the same no-fold, no-overload path as 34.b. */
    T_INT(lang, ev, "eval_in_nested_func_no_overload",
          "@helper() { Eval('return 0;'); } "
          "helper(); "
          "return 5 + 6;",              /* no overload in scope => 11 */
          11);

    /* -------------------------------------------------
     * 35. MELANG_VM_OFF=1 — AST-walker diagnostic mode
     *
     * When MELANG_VM_OFF is set to a recognised truthy value ("1", "yes",
     * "true", "on" — case-insensitive), mln_lang_run_handler bypasses the
     * bytecode VM and falls back to the original AST stack-walking
     * interpreter.  Scripts must produce identical results in both modes.
     * ------------------------------------------------- */
    setenv("MELANG_VM_OFF", "1", 1);

    T_INT(lang, ev, "vm_off_basic_arith",
          "return 3 + 4;",
          7);

    T_INT(lang, ev, "vm_off_func_call",
          "@F(n) { return n * 2; } return F(21);",
          42);

    T_INT(lang, ev, "vm_off_while_loop",
          "s = 0; i = 0; "
          "while (i < 5) { s = s + i; i = i + 1; } "
          "return s;",
          10);

    /* Verify case-insensitive truthy variants also activate AST mode. */
    setenv("MELANG_VM_OFF", "Yes", 1);
    T_INT(lang, ev, "vm_off_truthy_Yes",
          "return 100 - 58;",
          42);

    /* Verify "0" leaves VM enabled (so the same script still returns 42). */
    setenv("MELANG_VM_OFF", "0", 1);
    T_INT(lang, ev, "vm_off_falsy_zero",
          "return 40 + 2;",
          42);

    unsetenv("MELANG_VM_OFF");

    /* -------------------------------------------------
     * 36. MELANG_VM_TRACE=1 — compile-trace toggle
     *
     * MELANG_VM_TRACE=1 causes mln_lang_vm_try_compile to print a
     * one-line summary to stderr for each successfully compiled chunk.
     *
     * Test 36.a: correctness is unaffected with tracing on.
     * Test 36.b: trace output is actually written to stderr.
     * ------------------------------------------------- */

    /* 36.a  Verify the script still produces correct results with tracing. */
    setenv("MELANG_VM_TRACE", "1", 1);

    T_INT(lang, ev, "vm_trace_correct_result",
          "@TracedF(n) { return n + 1; } return TracedF(41);",
          42);

    /* 36.b  Verify trace output is actually emitted.
     *   Redirect stderr to a temp file, compile a fresh function body,
     *   restore stderr, then confirm the file contains the "[vm]" prefix
     *   that mln_lang_vm_try_compile emits. */
    {
        const char *tmpdir = getenv("TMPDIR");
        char trace_tmp[512];
        int  trace_fd;
        int  saved_stderr;
        ssize_t n_read;
        char buf[512];
        int  has_trace;

        if (tmpdir == NULL || tmpdir[0] == '\0') tmpdir = "/tmp";
        snprintf(trace_tmp, sizeof(trace_tmp), "%s/melang_trace_XXXXXX", tmpdir);

        trace_fd = mkstemp(trace_tmp);
        if (trace_fd < 0) {
            ++g_n_fail;
            fprintf(stderr,
                    "  FAIL [vm_trace_emits_output_check]: "
                    "mkstemp failed: %s\n", strerror(errno));
            unsetenv("MELANG_VM_TRACE");
            goto vm_trace_done;
        }

        saved_stderr = dup(fileno(stderr));
        if (saved_stderr < 0) {
            ++g_n_fail;
            fprintf(stderr,
                    "  FAIL [vm_trace_emits_output_check]: "
                    "dup(stderr) failed\n");
            close(trace_fd);
            unlink(trace_tmp);
            unsetenv("MELANG_VM_TRACE");
            goto vm_trace_done;
        }
        dup2(trace_fd, fileno(stderr));

        /* Run a script with a function body that hasn't been compiled yet
         * (unique name TracedG) so the compiler is guaranteed to fire. */
        run_test(lang, ev, "vm_trace_emits_output_run",
                 "@TracedG(x) { return x * 10; } return TracedG(5);",
                 EXPECT_INT, (mln_s64_t)50, 0.0, NULL);

        fflush(stderr);
        dup2(saved_stderr, fileno(stderr));
        close(saved_stderr);

        unsetenv("MELANG_VM_TRACE");

        /* Read trace file and search for the "[vm]" marker. */
        lseek(trace_fd, (off_t)0, SEEK_SET);
        n_read = read(trace_fd, buf, sizeof(buf) - 1);
        if (n_read > 0) {
            buf[n_read] = '\0';
        } else if (n_read == 0) {
            buf[0] = '\0';
        } else {
            /* read() error — treat as empty */
            buf[0] = '\0';
        }
        has_trace = (strstr(buf, "[vm]") != NULL);

        close(trace_fd);
        unlink(trace_tmp);

        if (has_trace) {
            ++g_n_pass;
            printf("  PASS [vm_trace_emits_output_check]\n");
        } else {
            ++g_n_fail;
            fprintf(stderr,
                    "  FAIL [vm_trace_emits_output_check]: "
                    "no \"[vm]\" trace line found in captured stderr\n");
        }
    }
vm_trace_done:;

    /* -------------------------------------------------
     * 37. Top-level VM-compile fallback to AST walker.
     *
     * When the top-level statement chain exceeds a VM-compiler limit
     * (e.g. MLN_VM_MAX_LOOPS=16) or contains a construct the VM
     * compiler does not yet support, mln_lang_vm_run_toplevel returns 0.
     * Before this fix, mln_lang_run_handler aborted the script with
     * "VM: top-level cannot be compiled (internal error)".  After the
     * fix, it flips ctx->vm_use_ast=1 and lets the AST stack-walker —
     * which already has ctx->stm pushed onto its run-stack from
     * mln_lang_ctx_new — drive the script transparently.  This mirrors
     * the per-call AST fallback already exercised in section 32 (dd).
     *
     * The tests below exercise top-level constructs that the VM
     * compiler bails on but the AST walker still handles:
     *
     *   37.a  17 nested while(1){break;} loops (>= MLN_VM_MAX_LOOPS+1).
     *         Triggers `bail(c)` in compile_whilestm.  The trailing
     *         `return 42;` runs only when the AST walker takes over.
     *   37.b  Same construct followed by a function call from the
     *         top-level fallback context.  Confirms that nested calls
     *         from an AST-fallback top-level still resolve correctly
     *         (no run-stack ordering corruption).
     *   37.c  Assertion that the script's value is computed by the
     *         AST walker — checked by combining a VM-incompatible
     *         top-level shape with arithmetic that would otherwise
     *         constant-fold under the VM.
     *
     * Cross-platform: these tests do not depend on environment
     * variables, /tmp paths, or POSIX-only APIs.  They use only the
     * existing run_test/T_INT helper and the static C string buffer
     * built up via snprintf, so they compile cleanly on Linux, macOS,
     * MSYS2, and MSVC builds.
     * ------------------------------------------------- */
    {
        /* Build "while(1){" * 17 + "break;}" * 17 + "return 42;". */
        char buf[2048];
        char *p = buf;
        int i;
        size_t cap;
        for (i = 0; i < 17; ++i) {
            cap = sizeof(buf) - (size_t)(p - buf);
            p += snprintf(p, cap, "while(1){");
        }
        for (i = 0; i < 17; ++i) {
            cap = sizeof(buf) - (size_t)(p - buf);
            p += snprintf(p, cap, "break;}");
        }
        cap = sizeof(buf) - (size_t)(p - buf);
        snprintf(p, cap, "return 42;");
        T_INT(lang, ev, "tv_toplevel_ast_fallback_deep_loops", buf, 42);

        /* 37.b — fallback at top level invokes a function (which will
         * itself be VM-compiled or AST-walked depending on its body). */
        p = buf;
        for (i = 0; i < 17; ++i) {
            cap = sizeof(buf) - (size_t)(p - buf);
            p += snprintf(p, cap, "while(1){");
        }
        for (i = 0; i < 17; ++i) {
            cap = sizeof(buf) - (size_t)(p - buf);
            p += snprintf(p, cap, "break;}");
        }
        cap = sizeof(buf) - (size_t)(p - buf);
        snprintf(p, cap, "@G(n){return n*3;} return G(14);");
        T_INT(lang, ev, "tv_toplevel_ast_fallback_then_call", buf, 42);

        /* 37.c — fallback at top level still computes the arithmetic
         * value correctly via the AST walker. */
        p = buf;
        for (i = 0; i < 17; ++i) {
            cap = sizeof(buf) - (size_t)(p - buf);
            p += snprintf(p, cap, "while(1){");
        }
        for (i = 0; i < 17; ++i) {
            cap = sizeof(buf) - (size_t)(p - buf);
            p += snprintf(p, cap, "break;}");
        }
        cap = sizeof(buf) - (size_t)(p - buf);
        snprintf(p, cap, "a=10; b=4; c=a*b+2; return c;");
        T_INT(lang, ev, "tv_toplevel_ast_fallback_arith", buf, 42);
    }

    /* -------------------------------------------------
     * 38. Exit() built-in — self-terminating coroutine
     *
     * Exit() must set ctx->quit on the calling coroutine and cause the
     * dispatcher to free it at the next iteration boundary, with NO
     * subsequent script-level statements or expressions executing.
     *
     * Every script below has a `return 999;` (or analogous tail) after
     * the Exit() call.  If quit-marking ever fails to short-circuit the
     * next opcode/AST step, the tail would run and ret_var would surface
     * as INT(999) — T_EXIT catches that.
     * ------------------------------------------------- */

    /* Bare Exit() at the top of the script. */
    T_EXIT(lang, ev, "exit_basic",
           "Exit();");

    /* Statement after Exit() at top level — must not execute. */
    T_EXIT(lang, ev, "exit_then_unreachable",
           "Exit(); return 999;");

    /* Exit() deep inside a user function body — unreachable return must
     * not surface as ret_var=999. */
    T_EXIT(lang, ev, "exit_in_func",
           "@F() { Exit(); return 999; } return F();");

    /* Recursive call chain that Exit()s from the deepest frame.  None of
     * the intermediate `return` statements on the unwind path may run. */
    T_EXIT(lang, ev, "exit_deep_recursion",
           "@inner(d) {"
           "  if (d <= 0) { Exit(); return 999; } fi"
           "  return inner(d - 1);"
           "}"
           "return inner(5);");

    /* Exit() in the left operand of an expression — the right operand
     * (`+ 1`) must not be evaluated, and the assignment that wraps the
     * expression must not run. */
    T_EXIT(lang, ev, "exit_in_expr",
           "@side() { Exit(); return 0; } "
           "x = side() + 1; return x;");

    /* Exit() in the middle of an array literal.  Subsequent element
     * expressions must not be evaluated. */
    T_EXIT(lang, ev, "exit_in_array_literal",
           "@side() { Exit(); return 0; } "
           "a = [1, side(), 3]; return 999;");

    /* Exit() inside a loop body — the loop must not produce another
     * iteration, and the post-loop return must not run. */
    T_EXIT(lang, ev, "exit_in_for_loop",
           "for (i = 0; i < 5; i++) {"
           "  if (i == 2) { Exit(); } fi"
           "} return 999;");

    /* Exit() inside a while loop. */
    T_EXIT(lang, ev, "exit_in_while_loop",
           "i = 0; while (i < 100) {"
           "  if (i == 3) { Exit(); } fi"
           "  i = i + 1;"
           "} return 999;");

    /* Exit() inside a Set method body. */
    T_EXIT(lang, ev, "exit_in_set_method",
           "S { @m() { Exit(); return 0; } } "
           "o = $S; o.m(); return 999;");

    /* Multiple Exit() calls in sequence — only the first ever runs;
     * remaining statements (including additional Exit()s) are
     * unreachable.  Idempotent at the language level. */
    T_EXIT(lang, ev, "exit_multi_call",
           "Exit(); Exit(); Exit(); return 999;");

    /* Exit() through an indirect function-value call.  The caller
     * resolves a function via a variable and then invokes it; Exit
     * inside the callee must still propagate cleanly. */
    T_EXIT(lang, ev, "exit_via_func_value",
           "@helper() { Exit(); return 0; } "
           "f = helper; f(); return 999;");

    /* -------------------------------------------------
     * 39. Kill() built-in — self-kill via alias
     *
     * When the alias passed to Kill matches the calling ctx's own alias,
     * the call must NOT free ctx synchronously (that would UAF the rest
     * of funccall_run + the dispatcher).  It must instead route through
     * ctx->quit, exactly like Exit().  These tests use the
     * T_EXIT_ALIAS helper that creates the ctx with a named alias so
     * Kill(<that name>) targets the running coroutine.
     * ------------------------------------------------- */

    /* Bare Kill(self) at top level. */
    T_EXIT_ALIAS(lang, ev, "kill_self_basic",
                 "Kill('self_basic');", "self_basic");

    /* Kill(self) followed by an unreachable return. */
    T_EXIT_ALIAS(lang, ev, "kill_self_then_unreachable",
                 "Kill('self_unreach'); return 999;",
                 "self_unreach");

    /* Kill(self) deep inside a user function — same quit semantics. */
    T_EXIT_ALIAS(lang, ev, "kill_self_in_func",
                 "@F() { Kill('self_in_func'); return 999; } return F();",
                 "self_in_func");

    /* Kill(self) inside an expression. */
    T_EXIT_ALIAS(lang, ev, "kill_self_in_expr",
                 "@side() { Kill('self_in_expr'); return 0; } "
                 "x = side() + 1; return x;",
                 "self_in_expr");

    /* Kill(self) inside a loop. */
    T_EXIT_ALIAS(lang, ev, "kill_self_in_loop",
                 "for (i = 0; i < 5; i++) {"
                 "  if (i == 2) { Kill('self_in_loop'); } fi"
                 "} return 999;",
                 "self_in_loop");

    /* Kill of a non-existent alias is a no-op: the script continues
     * normally and returns 42.  This is the only Kill() case that does
     * NOT terminate the coroutine — verify it explicitly so we know the
     * self-kill path branches on alias resolution correctly. */
    T_INT(lang, ev, "kill_nonexistent_is_noop",
          "Kill('no_such_alias_anywhere'); return 42;",
          42);

    /* -------------------------------------------------
     * 40. Kill() across coroutines — peer is freed, killer continues
     *
     * Parent script spawns a busy-loop child via Eval (named 'victim'),
     * then Kill()s it and returns 42.  We verify:
     *   - The parent's return_handler fires with ret_var = 42 (the
     *     scheduler did not crash, the parent completed normally).
     *   - After the parent's return_handler fires, the killed child ctx
     *     has been freed: lang->run_head has only the parent, and
     *     lang->wait_head is NULL.
     *
     * The child's body is intentionally CPU-bound rather than calling
     * sys.msleep — t/lang.c doesn't link against the sys dynamic
     * library, so the child runs purely in the interpreter core.
     * ------------------------------------------------- */
    {
        kill_cross_tc_t kc;
        kc.ev         = ev;
        kc.name       = "kill_other_keeps_parent_alive";
        kc.parent_ok  = 0;
        kc.cleanup_ok = 0;

        const char *code =
            "Eval('"
            "  s = 0;"
            "  for (i = 0; i < 1000000; i = i + 1) { s = s + i; }"
            "  return s;"
            "', nil, true, 'victim');"
            "Kill('victim');"
            "return 42;";

        mln_string_t src;
        mln_string_nset(&src, (mln_u8ptr_t)code, strlen(code));
        mln_event_break_reset(ev);

        mln_lang_ctx_t *ctx = mln_lang_job_new(lang, NULL, M_INPUT_T_BUF,
                                               &src, &kc, kill_cross_return_handler);
        if (ctx == NULL) {
            fprintf(stderr, "  FAIL [%s]: mln_lang_job_new returned NULL\n", kc.name);
            ++g_n_fail;
        } else {
            mln_event_dispatch(ev);
            if (kc.parent_ok && kc.cleanup_ok) {
                ++g_n_pass;
                printf("  PASS [%s]\n", kc.name);
            } else {
                ++g_n_fail;
                if (!kc.parent_ok)
                    fprintf(stderr, "  FAIL [%s]: parent did not return 42\n", kc.name);
                if (!kc.cleanup_ok)
                    fprintf(stderr, "  FAIL [%s]: killed child ctx still queued after parent completed\n", kc.name);
            }
        }
    }

    /* -------------------------------------------------
     * 41. Exit() terminates coroutine — ctx is not rescheduled
     *
     * Exit() inside a conditional: iteration before the exit should
     * produce observable side effects (variable mutation), but the
     * return statement after the loop must never execute.
     * ------------------------------------------------- */
    T_EXIT(lang, ev, "exit_in_conditional_loop",
           "x = 0;"
           "for (i = 0; i < 10; i = i + 1) {"
           "  if (i == 5) { Exit(); } fi"
           "  x = x + 1;"
           "}"
           "return x;");

    /* Exit() inside a nested function called from a loop. */
    T_EXIT(lang, ev, "exit_nested_func_in_loop",
           "@bail(n) { if (n > 2) { Exit(); } fi return n; }"
           "s = 0;"
           "for (i = 0; i < 100; i = i + 1) { s = s + bail(i); }"
           "return s;");

    /* Kill(self) inside a while loop — same semantics as Exit(). */
    T_EXIT_ALIAS(lang, ev, "kill_self_in_while_loop",
                 "i = 0;"
                 "while (true) {"
                 "  if (i == 3) { Kill('ks_while'); } fi"
                 "  i = i + 1;"
                 "}"
                 "return 999;",
                 "ks_while");

    /* -------------------------------------------------
     * 42. VULN: Import() with an over-long module name must be rejected,
     *     never crash the host.
     *
     * The Import builtin copies the script-supplied module name into two
     * fixed 1024-byte stack buffers (path[]/tmp_path[]).  Before the fix it
     * wrote the buffer terminator and combined paths using the *untruncated*
     * snprintf() return value — `path[n] = 0;` and `memcpy(path, tmp_path,
     * n);` — so any name long enough for snprintf to truncate made n exceed
     * the buffer and smashed the stack.  A single untrusted script calling
     * Import() with a long string could therefore abort or corrupt the host.
     *
     * Detection (same idea as t/lang_vm.c): the Import is followed by
     * `return <SENTINEL>;`.  On a fixed build Import raises a runtime error
     * ("Module name too long." for the very long name, or a clean
     * "Load dynamic library [...] failed." for the medium one) which aborts
     * the job before the return runs, so ret_var is never the sentinel and
     * EXPECT_EXITED passes.  On a vulnerable build the overflow trips the
     * stack protector and SIGABRTs the whole process, which also fails CI.
     *
     * Two lengths are exercised:
     *   - 4000 bytes: far past sizeof(path); hits the first copy site
     *     (path[n] = 0 after the "./%s.so" snprintf).
     *   - 1000 bytes: passes the early length guard but, once combined with
     *     the dynamic-library install prefix ("%s/%s"), overflows tmp_path
     *     and drives the memcpy(path, tmp_path, n) site.
     * ------------------------------------------------- */
    {
        size_t lens[2] = { 4000, 1000 };
        const char *names[2] = {
            "import_overlong_name_rejected",
            "import_long_name_combine_path"
        };
        size_t k;
        for (k = 0; k < 2; ++k) {
            size_t namelen = lens[k];
            char *code = (char *)malloc(namelen + 64);
            assert(code != NULL);
            char *p = code;
            memcpy(p, "Import('", 8); p += 8;
            memset(p, 'a', namelen);  p += namelen;
            /* The trailing return uses the same sentinel as t/lang_vm.c. */
            memcpy(p, "'); return 123456789;", 21); p += 21;
            *p = 0;

            test_ctx_t tc;
            tc.ev = ev;
            tc.name = names[k];
            tc.etype = EXPECT_EXITED;
            tc.eint = 0; tc.ereal = 0; tc.estr = NULL;

            mln_string_t src;
            mln_string_nset(&src, (mln_u8ptr_t)code, strlen(code));
            mln_event_break_reset(ev);

            mln_lang_ctx_t *ctx = mln_lang_job_new(lang, NULL, M_INPUT_T_BUF,
                                                   &src, &tc, test_return_handler);
            if (ctx == NULL) {
                fprintf(stderr, "  FAIL [%s]: mln_lang_job_new returned NULL\n",
                        names[k]);
                ++g_n_fail;
            } else {
                mln_event_dispatch(ev);
            }
            free(code);
        }
    }

    /* -------------------------------------------------
     * 43. VULN: Eval('@<relative path>', ...) must not overflow the host.
     *
     * When the first argument starts with '@' and the current job has a
     * filename, the Eval builtin resolves the path relative to the current
     * script's directory by copying into a fixed 1024-byte stack buffer:
     *
     *     n  = <dir length, clamped to sizeof(buf)-1>;
     *     memcpy(buf, ctx->filename->data, n);
     *     n += snprintf(buf + n, sizeof(buf) - n - 1, "/%s", &path->data[1]);
     *     buf[n] = 0;                       <- OOB write before the fix
     *     mln_string_nset(&tmp, buf, n);
     *
     * snprintf() returns the length it *would* have written, so a long
     * script-supplied '@' path drove n past the buffer and smashed the
     * stack — a single untrusted script could crash or corrupt the host.
     *
     * The branch only fires when ctx->filename is set and contains '/', so
     * the reproducer writes a temporary script file (path under /tmp) and
     * runs it with M_INPUT_T_FILE.  On a fixed build the over-long relative
     * path is safely truncated, the resolved file does not exist, Eval()
     * returns false, and the script proceeds to `return 123456789;` — so
     * EXPECT_INT(123456789) confirms it ran to completion without crashing.
     * On a vulnerable build the stray "buf[n] = 0" lands far past the
     * 1024-byte buffer (n grows with the path length); a large name pushes
     * the write beyond the top of the stack, so the process dies with
     * SIGSEGV/SIGABRT and fails CI.
     * ------------------------------------------------- */
    {
        const char *scrpath = "/tmp/mln_lang_eval_vuln4.m";
        const char *pre  = "Eval('@";
        const char *post = "', nil, false, nil); return 123456789;";
        /* Large enough that the unclamped offset overshoots the stack on a
         * vulnerable build (empirically anything past a few tens of KB
         * faults); on a fixed build the path is simply truncated. */
        size_t namelen = 1024 * 1024;
        size_t prelen = strlen(pre), postlen = strlen(post);
        char *script = (char *)malloc(prelen + namelen + postlen + 1);
        assert(script != NULL);
        char *q = script;
        memcpy(q, pre, prelen);   q += prelen;
        memset(q, 'a', namelen);  q += namelen;
        memcpy(q, post, postlen); q += postlen;
        *q = 0;

        FILE *fp = fopen(scrpath, "wb");
        if (fp == NULL) {
            fprintf(stderr, "  FAIL [eval_at_overlong_path]: cannot create temp script\n");
            ++g_n_fail;
        } else {
            fwrite(script, 1, strlen(script), fp);
            fclose(fp);

            test_ctx_t tc;
            tc.ev = ev;
            tc.name = "eval_at_overlong_path";
            tc.etype = EXPECT_INT;
            tc.eint = 123456789;
            tc.ereal = 0; tc.estr = NULL;

            mln_string_t paths;
            mln_string_nset(&paths, (mln_u8ptr_t)scrpath, strlen(scrpath));
            mln_event_break_reset(ev);

            mln_lang_ctx_t *ctx = mln_lang_job_new(lang, NULL, M_INPUT_T_FILE,
                                                   &paths, &tc, test_return_handler);
            if (ctx == NULL) {
                fprintf(stderr, "  FAIL [eval_at_overlong_path]: job creation failed\n");
                ++g_n_fail;
            } else {
                mln_event_dispatch(ev);
            }
            remove(scrpath);
        }
        free(script);
    }

    /* -------------------------------------------------
     * 44. VULN: loading a relative script path must not overflow the host
     *     when MELANG_PATH is set.
     *
     * mln_lang_ast_file_open() resolves a relative script path against the
     * search directories in $MELANG_PATH (and the install prefix) by combining
     * them into a fixed 1024-byte stack buffer:
     *
     *     n = snprintf(tmp_path, sizeof(tmp_path)-1, "%s/%s", melang_path, path);
     *     tmp_path[n] = 0;                  <- OOB write before the fix
     *
     * snprintf() returns the length it *would* have written, so a long
     * $MELANG_PATH entry (or a long relative path) drove n past the buffer
     * and the terminator write smashed the stack.  This is reached from a
     * single untrusted script: Eval('<relative file>', nil, false, nil)
     * spawns a M_INPUT_T_FILE job whose path does not start with '/' or '@',
     * so the loader calls mln_lang_ast_file_open() with the script's string.
     *
     * mln_lang_ast_file_open() is reached only on the cached-AST code path
     * (mln_lang_ast_cache_search); the non-cached path goes through the lexer
     * instead.  The reproducer therefore enables the AST cache, sets a 1 MiB
     * $MELANG_PATH, and evals a non-existent relative file.  On a fixed build
     * the combined path is truncated to the buffer, the file is not found,
     * Eval() returns false, and the outer script runs on to
     * `return 123456789;` (EXPECT_INT).  On a vulnerable build the "%s/%s"
     * expansion makes n ~1 MiB and "tmp_path[n] = 0" overshoots the top of
     * the stack, killing the process with SIGSEGV.
     * ------------------------------------------------- */
    {
        size_t pathlen = 1024 * 1024;
        char *envval = (char *)malloc(pathlen + 1);
        assert(envval != NULL);
        memset(envval, 'a', pathlen);
        envval[pathlen] = 0;
        setenv("MELANG_PATH", envval, 1);
        mln_lang_cache_set(lang);

        const char *code =
            "Eval('zz_mln_lang_vuln5_nonexistent', nil, false, nil);"
            " return 123456789;";

        test_ctx_t tc;
        tc.ev = ev;
        tc.name = "load_relpath_overlong_melang_path";
        tc.etype = EXPECT_INT;
        tc.eint = 123456789;
        tc.ereal = 0; tc.estr = NULL;

        mln_string_t src;
        mln_string_nset(&src, (mln_u8ptr_t)code, strlen(code));
        mln_event_break_reset(ev);

        mln_lang_ctx_t *ctx = mln_lang_job_new(lang, NULL, M_INPUT_T_BUF,
                                               &src, &tc, test_return_handler);
        if (ctx == NULL) {
            fprintf(stderr, "  FAIL [load_relpath_overlong_melang_path]: job creation failed\n");
            ++g_n_fail;
        } else {
            mln_event_dispatch(ev);
        }
        lang->cache = 0;
        unsetenv("MELANG_PATH");
        free(envval);
    }

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
