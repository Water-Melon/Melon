/*
 * t/lang_vm.c — tests for the bytecode VM (src/mln_lang_vm.c), the AST
 * type-operator handlers (src/mln_lang_int.c) and the AST muldiv fast path
 * (src/mln_lang.c).
 *
 * Regression coverage for the INT64_MIN / -1 (and INT64_MIN % -1) signed
 * overflow: the quotient 2^63 is out of range, which is undefined behavior
 * in C and traps with SIGFPE on x86-64 (idiv #DE); on aarch64 sdiv wraps
 * silently and returns INT64_MIN (a wrong result, no crash). Every integer
 * division/modulo code path must reject it with a runtime error instead of
 * evaluating it.
 *
 * Each case runs twice: once on the default bytecode VM and once with
 * MELANG_VM_OFF=1 forcing the AST walker, so both code paths are checked.
 *
 * Detection strategy (works uniformly on VM and AST, and on both arches):
 * the dangerous operation is followed by `return <SENTINEL>;`. On a buggy
 * build the operation silently succeeds and the sentinel is returned, so the
 * test sees the sentinel and fails. On a fixed build the operation raises a
 * runtime error and aborts the job before the return runs, so ret_var is
 * never the sentinel and the test passes. On an unfixed x86-64 build the
 * idiv traps with SIGFPE and the process dies, which also fails CI.
 *
 * Exit code 0 => all good. Non-zero => a case misbehaved (CI 'make run'
 * chains tests with && so a non-zero exit fails the build).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/socket.h>
#include <unistd.h>
#include "mln_lang.h"
#include "mln_event.h"

static int          fds[2];
static mln_event_t *g_ev;
static int          g_failed = 0;

/* Distinctive value the dangerous operation's job returns if it was NOT
 * rejected. No legitimate test computation produces it. */
#define SENTINEL 123456789LL

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

typedef enum {
    EXP_REJECT,  /* op must be rejected: ret_var must NOT be INT(SENTINEL) */
    EXP_INT      /* op must return an INT equal to .eint                   */
} exp_t;

typedef struct {
    const char *name;
    exp_t       etype;
    mln_s64_t   eint;
} case_t;

static void ret_handler(mln_lang_ctx_t *ctx)
{
    case_t         *c  = (case_t *)mln_lang_ctx_data_get(ctx);
    mln_lang_var_t *rv = ctx->ret_var;
    int ok = 0;

    switch (c->etype) {
        case EXP_REJECT:
            /* PASS unless the sentinel made it back, which means the
             * dangerous operation was evaluated rather than rejected. */
            ok = !(rv && rv->val &&
                   rv->val->type == M_LANG_VAL_TYPE_INT &&
                   rv->val->data.i == SENTINEL);
            if (!ok)
                fprintf(stderr,
                    "  FAIL [%s]: operation was evaluated (sentinel returned) "
                    "instead of being rejected\n", c->name);
            break;
        case EXP_INT:
            ok = (rv && rv->val &&
                  rv->val->type == M_LANG_VAL_TYPE_INT &&
                  rv->val->data.i == c->eint);
            if (!ok)
                fprintf(stderr, "  FAIL [%s]: bad int result (expected %lld)\n",
                        c->name, (long long)c->eint);
            break;
    }

    if (ok) fprintf(stderr, "  ok   [%s]\n", c->name);
    else    g_failed = 1;

    mln_event_break_set(g_ev);
}

static void run(mln_lang_t *lang, case_t *c, const char *code)
{
    mln_string_t src;
    mln_string_nset(&src, (mln_u8ptr_t)code, strlen(code));
    mln_event_break_reset(g_ev);
    mln_lang_ctx_t *ctx =
        mln_lang_job_new(lang, NULL, M_INPUT_T_BUF, &src, c, ret_handler);
    if (ctx == NULL) {
        fprintf(stderr, "  FAIL [%s]: mln_lang_job_new returned NULL\n", c->name);
        g_failed = 1;
        return;
    }
    mln_lang_ctx_data_set(ctx, c);
    mln_event_dispatch(g_ev);
}

/* INT64_MIN expressed without a literal the lexer would reject. */
#define INT_MIN_EXPR "0 - 9223372036854775807 - 1"

static void run_suite(mln_lang_t *lang)
{
    /* VULN-1: INT64_MIN / -1 must be rejected (would SIGFPE on x86-64).
     * Operands held in variables so the constant folder does not apply. */
    { case_t c = { "intmin_div_neg1", EXP_REJECT, 0 };
      run(lang, &c, "a = " INT_MIN_EXPR "; b = 0 - 1; c = a / b; return 123456789;"); }

    /* INT64_MIN % -1 traps identically and must also be rejected. */
    { case_t c = { "intmin_mod_neg1", EXP_REJECT, 0 };
      run(lang, &c, "a = " INT_MIN_EXPR "; b = 0 - 1; c = a % b; return 123456789;"); }

    /* Compound-assign forms hit the same arithmetic. */
    { case_t c = { "intmin_diveq_neg1", EXP_REJECT, 0 };
      run(lang, &c, "a = " INT_MIN_EXPR "; b = 0 - 1; a /= b; return 123456789;"); }

    { case_t c = { "intmin_modeq_neg1", EXP_REJECT, 0 };
      run(lang, &c, "a = " INT_MIN_EXPR "; b = 0 - 1; a %= b; return 123456789;"); }

    /* Regression: ordinary division/modulo must keep working. */
    { case_t c = { "normal_div", EXP_INT, 5 };
      run(lang, &c, "a = 20; b = 4; return a / b;"); }
    { case_t c = { "normal_mod", EXP_INT, 2 };
      run(lang, &c, "a = 17; b = 5; return a % b;"); }
    { case_t c = { "intmin_div_neg2", EXP_INT, 4611686018427387904LL };
      run(lang, &c, "a = " INT_MIN_EXPR "; b = 0 - 2; return a / b;"); }

    /* Regression: division by zero stays a (different) runtime error and
     * must likewise be rejected (sentinel must not come back). */
    { case_t c = { "div_by_zero", EXP_REJECT, 0 };
      run(lang, &c, "a = 1; b = 0; c = a / b; return 123456789;"); }
}

int main(void)
{
    assert(socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0);
    g_ev = mln_event_new();
    assert(g_ev != NULL);

    fprintf(stderr, "== bytecode VM path ==\n");
    unsetenv("MELANG_VM_OFF");
    {
        mln_lang_t *lang = mln_lang_new(g_ev, lang_signal, lang_clear);
        assert(lang != NULL);
        run_suite(lang);
        mln_lang_free(lang);
    }

    fprintf(stderr, "== AST fallback path (MELANG_VM_OFF=1) ==\n");
    setenv("MELANG_VM_OFF", "1", 1);
    {
        mln_lang_t *lang = mln_lang_new(g_ev, lang_signal, lang_clear);
        assert(lang != NULL);
        run_suite(lang);
        mln_lang_free(lang);
    }

    mln_event_free(g_ev);
    close(fds[0]);
    close(fds[1]);

    if (g_failed) {
        fprintf(stderr, "\nlang_vm: FAILED\n");
        return 1;
    }
    fprintf(stderr, "\nlang_vm: all tests passed\n");
    return 0;
}
