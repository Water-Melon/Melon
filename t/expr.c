#include "mln_expr.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>

static int test_count = 0;
static int pass_count = 0;

#define TEST(name) do { \
    test_count++; \
    printf("  TEST %d: %s ... ", test_count, name); \
} while (0)

#define PASS() do { \
    pass_count++; \
    printf("PASS\n"); \
} while (0)

#define FAIL(msg) do { \
    printf("FAIL: %s\n", msg); \
} while (0)

/* --- Callback handlers --- */

static mln_expr_val_t *
simple_var_handler(mln_string_t *ns, mln_string_t *name, int is_func, mln_array_t *args, void *data)
{
    mln_string_t *s;
    if (is_func) return NULL;
    s = mln_string_dup(name);
    if (s == NULL) return NULL;
    mln_expr_val_t *ret = mln_expr_val_new(mln_expr_type_string, s, NULL);
    mln_string_free(s);
    return ret;
}

static mln_expr_val_t *
concat_handler(mln_string_t *ns, mln_string_t *name, int is_func, mln_array_t *args, void *data)
{
    mln_expr_val_t *v, *p;
    int i;
    mln_string_t *s1 = NULL, *s2, *s3;

    if (!is_func) {
        mln_string_t *s = mln_string_dup(name);
        if (s == NULL) return NULL;
        v = mln_expr_val_new(mln_expr_type_string, s, NULL);
        mln_string_free(s);
        return v;
    }

    for (i = 0, v = p = (mln_expr_val_t *)mln_array_elts(args); i < (int)mln_array_nelts(args); v = p + (++i)) {
        if (v->type != mln_expr_type_string) continue;
        if (s1 == NULL) {
            s1 = mln_string_ref(v->data.s);
            continue;
        }
        s2 = v->data.s;
        s3 = mln_string_strcat(s1, s2);
        mln_string_free(s1);
        s1 = s3;
    }

    if (s1 == NULL) return mln_expr_val_new(mln_expr_type_null, NULL, NULL);

    v = mln_expr_val_new(mln_expr_type_string, s1, NULL);
    mln_string_free(s1);
    return v;
}

static mln_expr_val_t *
if_handler(mln_string_t *ns, mln_string_t *name, int is_func, mln_array_t *args, void *data)
{
    mln_string_t true_str = mln_string("cond_true");
    mln_string_t then_var = mln_string("result_then");
    mln_string_t else_var = mln_string("result_else");
    mln_u8_t b;

    if (!is_func) {
        if (!mln_string_strcmp(name, &true_str)) {
            b = 1;
            return mln_expr_val_new(mln_expr_type_bool, &b, NULL);
        }
        if (!mln_string_strcmp(name, &then_var)) {
            mln_string_t tmp = mln_string("then_branch");
            mln_string_t *s = mln_string_dup(&tmp);
            mln_expr_val_t *r = mln_expr_val_new(mln_expr_type_string, s, NULL);
            mln_string_free(s);
            return r;
        }
        if (!mln_string_strcmp(name, &else_var)) {
            mln_string_t tmp = mln_string("else_branch");
            mln_string_t *s = mln_string_dup(&tmp);
            mln_expr_val_t *r = mln_expr_val_new(mln_expr_type_string, s, NULL);
            mln_string_free(s);
            return r;
        }
        b = 0;
        return mln_expr_val_new(mln_expr_type_bool, &b, NULL);
    }
    return mln_expr_val_new(mln_expr_type_null, NULL, NULL);
}

static int loop_counter = 0;

static mln_expr_val_t *
loop_handler(mln_string_t *ns, mln_string_t *name, int is_func, mln_array_t *args, void *data)
{
    mln_string_t cond_name = mln_string("cond");
    mln_string_t body_name = mln_string("body");
    mln_u8_t b;

    if (!is_func) {
        if (!mln_string_strcmp(name, &cond_name)) {
            b = (loop_counter < 5) ? 1 : 0;
            return mln_expr_val_new(mln_expr_type_bool, &b, NULL);
        }
        if (!mln_string_strcmp(name, &body_name)) {
            loop_counter++;
            mln_s64_t val = loop_counter;
            return mln_expr_val_new(mln_expr_type_int, &val, NULL);
        }
        {
            mln_string_t *s = mln_string_dup(name);
            if (s == NULL) return NULL;
            mln_expr_val_t *r = mln_expr_val_new(mln_expr_type_string, s, NULL);
            mln_string_free(s);
            return r;
        }
    }
    return mln_expr_val_new(mln_expr_type_null, NULL, NULL);
}

static mln_expr_val_t *
ns_handler(mln_string_t *ns, mln_string_t *name, int is_func, mln_array_t *args, void *data)
{
    if (is_func) {
        return mln_expr_val_new(mln_expr_type_null, NULL, NULL);
    }
    if (ns != NULL) {
        mln_string_t sep = mln_string(":");
        mln_string_t *ns_sep = mln_string_strcat(ns, &sep);
        mln_string_t *result = mln_string_strcat(ns_sep, name);
        mln_string_free(ns_sep);
        mln_expr_val_t *v = mln_expr_val_new(mln_expr_type_string, result, NULL);
        mln_string_free(result);
        return v;
    }
    {
        mln_string_t *s = mln_string_dup(name);
        if (s == NULL) return NULL;
        mln_expr_val_t *v = mln_expr_val_new(mln_expr_type_string, s, NULL);
        mln_string_free(s);
        return v;
    }
}

/* --- Test functions --- */

static void test_simple_variable(void)
{
    TEST("simple variable");
    mln_string_t exp = mln_string("myvar");
    mln_expr_val_t *v = mln_expr_run(&exp, simple_var_handler, NULL);
    if (v == NULL) { FAIL("run returned NULL"); return; }
    if (v->type != mln_expr_type_string) { FAIL("wrong type"); mln_expr_val_free(v); return; }
    mln_string_t expected = mln_string("myvar");
    if (mln_string_strcmp(v->data.s, &expected)) { FAIL("wrong value"); mln_expr_val_free(v); return; }
    mln_expr_val_free(v);
    PASS();
}

static void test_integer_dec(void)
{
    TEST("decimal integer constant");
    mln_string_t exp = mln_string("42");
    mln_expr_val_t *v = mln_expr_run(&exp, simple_var_handler, NULL);
    if (v == NULL) { FAIL("run returned NULL"); return; }
    if (v->type != mln_expr_type_int) { FAIL("wrong type"); mln_expr_val_free(v); return; }
    if (v->data.i != 42) { FAIL("wrong value"); mln_expr_val_free(v); return; }
    mln_expr_val_free(v);
    PASS();
}

static void test_integer_hex(void)
{
    TEST("hexadecimal integer constant");
    mln_string_t exp = mln_string("0xff");
    mln_expr_val_t *v = mln_expr_run(&exp, simple_var_handler, NULL);
    if (v == NULL) { FAIL("run returned NULL"); return; }
    if (v->type != mln_expr_type_int) { FAIL("wrong type"); mln_expr_val_free(v); return; }
    if (v->data.i != 255) { FAIL("wrong value"); mln_expr_val_free(v); return; }
    mln_expr_val_free(v);
    PASS();
}

static void test_integer_oct(void)
{
    TEST("octal integer constant");
    mln_string_t exp = mln_string("0777");
    mln_expr_val_t *v = mln_expr_run(&exp, simple_var_handler, NULL);
    if (v == NULL) { FAIL("run returned NULL"); return; }
    if (v->type != mln_expr_type_int) { FAIL("wrong type"); mln_expr_val_free(v); return; }
    if (v->data.i != 0777) { FAIL("wrong value"); mln_expr_val_free(v); return; }
    mln_expr_val_free(v);
    PASS();
}

static void test_real_constant(void)
{
    TEST("real constant");
    mln_string_t exp = mln_string("3.14");
    mln_expr_val_t *v = mln_expr_run(&exp, simple_var_handler, NULL);
    if (v == NULL) { FAIL("run returned NULL"); return; }
    if (v->type != mln_expr_type_real) { FAIL("wrong type"); mln_expr_val_free(v); return; }
    if (v->data.r < 3.13 || v->data.r > 3.15) { FAIL("wrong value"); mln_expr_val_free(v); return; }
    mln_expr_val_free(v);
    PASS();
}

static void test_string_dblq(void)
{
    TEST("double-quoted string constant");
    mln_string_t exp = mln_string("\"hello world\"");
    mln_expr_val_t *v = mln_expr_run(&exp, simple_var_handler, NULL);
    if (v == NULL) { FAIL("run returned NULL"); return; }
    if (v->type != mln_expr_type_string) { FAIL("wrong type"); mln_expr_val_free(v); return; }
    mln_string_t expected = mln_string("hello world");
    if (mln_string_strcmp(v->data.s, &expected)) { FAIL("wrong value"); mln_expr_val_free(v); return; }
    mln_expr_val_free(v);
    PASS();
}

static void test_string_sglq(void)
{
    TEST("single-quoted string constant");
    mln_string_t exp = mln_string("'hello world'");
    mln_expr_val_t *v = mln_expr_run(&exp, simple_var_handler, NULL);
    if (v == NULL) { FAIL("run returned NULL"); return; }
    if (v->type != mln_expr_type_string) { FAIL("wrong type"); mln_expr_val_free(v); return; }
    mln_string_t expected = mln_string("hello world");
    if (mln_string_strcmp(v->data.s, &expected)) { FAIL("wrong value"); mln_expr_val_free(v); return; }
    mln_expr_val_free(v);
    PASS();
}

static void test_string_escape(void)
{
    TEST("string escape sequences");
    mln_string_t exp = mln_string("\"hello\\nworld\\t!\"");
    mln_expr_val_t *v = mln_expr_run(&exp, simple_var_handler, NULL);
    if (v == NULL) { FAIL("run returned NULL"); return; }
    if (v->type != mln_expr_type_string) { FAIL("wrong type"); mln_expr_val_free(v); return; }
    mln_string_t expected = mln_string("hello\nworld\t!");
    if (mln_string_strcmp(v->data.s, &expected)) { FAIL("wrong value"); mln_expr_val_free(v); return; }
    mln_expr_val_free(v);
    PASS();
}

static void test_bool_true(void)
{
    TEST("bool true keyword");
    mln_string_t exp = mln_string("true");
    mln_expr_val_t *v = mln_expr_run(&exp, simple_var_handler, NULL);
    if (v == NULL) { FAIL("run returned NULL"); return; }
    if (v->type != mln_expr_type_bool) { FAIL("wrong type"); mln_expr_val_free(v); return; }
    if (v->data.b != 1) { FAIL("wrong value"); mln_expr_val_free(v); return; }
    mln_expr_val_free(v);
    PASS();
}

static void test_bool_false(void)
{
    TEST("bool false keyword");
    mln_string_t exp = mln_string("false");
    mln_expr_val_t *v = mln_expr_run(&exp, simple_var_handler, NULL);
    if (v == NULL) { FAIL("run returned NULL"); return; }
    if (v->type != mln_expr_type_bool) { FAIL("wrong type"); mln_expr_val_free(v); return; }
    if (v->data.b != 0) { FAIL("wrong value"); mln_expr_val_free(v); return; }
    mln_expr_val_free(v);
    PASS();
}

static void test_null_keyword(void)
{
    TEST("null keyword");
    mln_string_t exp = mln_string("null");
    mln_expr_val_t *v = mln_expr_run(&exp, simple_var_handler, NULL);
    if (v == NULL) { FAIL("run returned NULL"); return; }
    if (v->type != mln_expr_type_null) { FAIL("wrong type"); mln_expr_val_free(v); return; }
    mln_expr_val_free(v);
    PASS();
}

static void test_function_call(void)
{
    TEST("function call with args");
    mln_string_t exp = mln_string("concat('abc', 'def')");
    mln_expr_val_t *v = mln_expr_run(&exp, concat_handler, NULL);
    if (v == NULL) { FAIL("run returned NULL"); return; }
    if (v->type != mln_expr_type_string) { FAIL("wrong type"); mln_expr_val_free(v); return; }
    mln_string_t expected = mln_string("abcdef");
    if (mln_string_strcmp(v->data.s, &expected)) { FAIL("wrong value"); mln_expr_val_free(v); return; }
    mln_expr_val_free(v);
    PASS();
}

static void test_nested_function(void)
{
    TEST("nested function calls");
    mln_string_t exp = mln_string("concat('a', concat('b', 'c'))");
    mln_expr_val_t *v = mln_expr_run(&exp, concat_handler, NULL);
    if (v == NULL) { FAIL("run returned NULL"); return; }
    if (v->type != mln_expr_type_string) { FAIL("wrong type"); mln_expr_val_free(v); return; }
    mln_string_t expected = mln_string("abc");
    if (mln_string_strcmp(v->data.s, &expected)) { FAIL("wrong value"); mln_expr_val_free(v); return; }
    mln_expr_val_free(v);
    PASS();
}

static void test_multiple_expressions(void)
{
    TEST("multiple expressions");
    mln_string_t exp = mln_string("concat('a', 'b') concat('c', 'd')");
    mln_expr_val_t *v = mln_expr_run(&exp, concat_handler, NULL);
    if (v == NULL) { FAIL("run returned NULL"); return; }
    if (v->type != mln_expr_type_string) { FAIL("wrong type"); mln_expr_val_free(v); return; }
    mln_string_t expected = mln_string("cd");
    if (mln_string_strcmp(v->data.s, &expected)) { FAIL("wrong value"); mln_expr_val_free(v); return; }
    mln_expr_val_free(v);
    PASS();
}

static void test_namespace(void)
{
    TEST("namespace prefix");
    mln_string_t exp = mln_string("a:b:c:myvar");
    mln_expr_val_t *v = mln_expr_run(&exp, ns_handler, NULL);
    if (v == NULL) { FAIL("run returned NULL"); return; }
    if (v->type != mln_expr_type_string) { FAIL("wrong type"); mln_expr_val_free(v); return; }
    mln_string_t expected = mln_string("a:b:c:myvar");
    if (mln_string_strcmp(v->data.s, &expected)) { FAIL("wrong value"); mln_expr_val_free(v); return; }
    mln_expr_val_free(v);
    PASS();
}

static void test_if_true_branch(void)
{
    TEST("if-then-else true branch");
    mln_string_t exp = mln_string("if cond_true then result_then else result_else fi");
    mln_expr_val_t *v = mln_expr_run(&exp, if_handler, NULL);
    if (v == NULL) { FAIL("run returned NULL"); return; }
    if (v->type != mln_expr_type_string) { FAIL("wrong type"); mln_expr_val_free(v); return; }
    mln_string_t expected = mln_string("then_branch");
    if (mln_string_strcmp(v->data.s, &expected)) { FAIL("wrong value"); mln_expr_val_free(v); return; }
    mln_expr_val_free(v);
    PASS();
}

static void test_if_false_branch(void)
{
    TEST("if-then-else false branch");
    mln_string_t exp = mln_string("if cond_false then result_then else result_else fi");
    mln_expr_val_t *v = mln_expr_run(&exp, if_handler, NULL);
    if (v == NULL) { FAIL("run returned NULL"); return; }
    if (v->type != mln_expr_type_string) { FAIL("wrong type"); mln_expr_val_free(v); return; }
    mln_string_t expected = mln_string("else_branch");
    if (mln_string_strcmp(v->data.s, &expected)) { FAIL("wrong value"); mln_expr_val_free(v); return; }
    mln_expr_val_free(v);
    PASS();
}

static void test_if_no_else(void)
{
    TEST("if-then without else");
    mln_string_t exp = mln_string("if cond_true then result_then fi");
    mln_expr_val_t *v = mln_expr_run(&exp, if_handler, NULL);
    if (v == NULL) { FAIL("run returned NULL"); return; }
    if (v->type != mln_expr_type_string) { FAIL("wrong type"); mln_expr_val_free(v); return; }
    mln_string_t expected = mln_string("then_branch");
    if (mln_string_strcmp(v->data.s, &expected)) { FAIL("wrong value"); mln_expr_val_free(v); return; }
    mln_expr_val_free(v);
    PASS();
}

static void test_nested_if(void)
{
    TEST("nested if-then-else");
    mln_string_t exp = mln_string("if cond_true then if cond_true then result_then fi else result_else fi");
    mln_expr_val_t *v = mln_expr_run(&exp, if_handler, NULL);
    if (v == NULL) { FAIL("run returned NULL"); return; }
    if (v->type != mln_expr_type_string) { FAIL("wrong type"); mln_expr_val_free(v); return; }
    mln_string_t expected = mln_string("then_branch");
    if (mln_string_strcmp(v->data.s, &expected)) { FAIL("wrong value"); mln_expr_val_free(v); return; }
    mln_expr_val_free(v);
    PASS();
}

static void test_loop(void)
{
    TEST("loop with counter");
    loop_counter = 0;
    mln_string_t exp = mln_string("loop cond do body end");
    mln_expr_val_t *v = mln_expr_run(&exp, loop_handler, NULL);
    if (v == NULL) { FAIL("run returned NULL"); return; }
    if (v->type != mln_expr_type_int) { FAIL("wrong type"); mln_expr_val_free(v); return; }
    if (v->data.i != 5) { FAIL("wrong value"); mln_expr_val_free(v); return; }
    if (loop_counter != 5) { FAIL("loop did not iterate 5 times"); mln_expr_val_free(v); return; }
    mln_expr_val_free(v);
    PASS();
}

static void test_loop_false_condition(void)
{
    TEST("loop with false condition");
    loop_counter = 10; /* already past threshold */
    mln_string_t exp = mln_string("loop cond do body end");
    mln_expr_val_t *v = mln_expr_run(&exp, loop_handler, NULL);
    if (v == NULL) { FAIL("run returned NULL"); return; }
    /* Should be null since body never executed */
    if (v->type != mln_expr_type_null) { FAIL("wrong type"); mln_expr_val_free(v); return; }
    mln_expr_val_free(v);
    PASS();
}

static void test_val_new_free(void)
{
    TEST("val_new and val_free");
    mln_expr_val_t *v;

    /* null */
    v = mln_expr_val_new(mln_expr_type_null, NULL, NULL);
    assert(v != NULL && v->type == mln_expr_type_null);
    mln_expr_val_free(v);

    /* bool */
    mln_u8_t b = 1;
    v = mln_expr_val_new(mln_expr_type_bool, &b, NULL);
    assert(v != NULL && v->type == mln_expr_type_bool && v->data.b == 1);
    mln_expr_val_free(v);

    /* int */
    mln_s64_t i = 12345;
    v = mln_expr_val_new(mln_expr_type_int, &i, NULL);
    assert(v != NULL && v->type == mln_expr_type_int && v->data.i == 12345);
    mln_expr_val_free(v);

    /* real */
    double r = 1.5;
    v = mln_expr_val_new(mln_expr_type_real, &r, NULL);
    assert(v != NULL && v->type == mln_expr_type_real && v->data.r == 1.5);
    mln_expr_val_free(v);

    /* string */
    mln_string_t s = mln_string("test");
    v = mln_expr_val_new(mln_expr_type_string, &s, NULL);
    assert(v != NULL && v->type == mln_expr_type_string);
    mln_expr_val_free(v);

    /* free NULL */
    mln_expr_val_free(NULL);

    PASS();
}

static void test_val_dup(void)
{
    TEST("val_dup");
    mln_string_t s = mln_string("dup_test");
    mln_expr_val_t *orig = mln_expr_val_new(mln_expr_type_string, &s, NULL);
    assert(orig != NULL);
    mln_expr_val_t *dup = mln_expr_val_dup(orig);
    assert(dup != NULL);
    assert(dup->type == mln_expr_type_string);
    assert(!mln_string_strcmp(dup->data.s, &s));
    mln_expr_val_free(orig);
    mln_expr_val_free(dup);
    PASS();
}

static void test_val_copy(void)
{
    TEST("val_copy");
    mln_s64_t i = 999;
    mln_expr_val_t *src = mln_expr_val_new(mln_expr_type_int, &i, NULL);
    mln_expr_val_t dest;
    mln_expr_val_copy(&dest, src);
    assert(dest.type == mln_expr_type_int && dest.data.i == 999);
    mln_expr_val_free(src);

    /* copy NULL src */
    mln_expr_val_copy(&dest, NULL);
    PASS();
}

static void test_empty_expression(void)
{
    TEST("empty expression");
    mln_string_t exp = mln_string("");
    mln_expr_val_t *v = mln_expr_run(&exp, simple_var_handler, NULL);
    if (v == NULL) { FAIL("run returned NULL for empty"); return; }
    if (v->type != mln_expr_type_null) { FAIL("expected null type"); mln_expr_val_free(v); return; }
    mln_expr_val_free(v);
    PASS();
}

static void test_func_with_variable_args(void)
{
    TEST("function with variable arguments");
    mln_string_t exp = mln_string("concat(myvar, 'suffix')");
    mln_expr_val_t *v = mln_expr_run(&exp, concat_handler, NULL);
    if (v == NULL) { FAIL("run returned NULL"); return; }
    if (v->type != mln_expr_type_string) { FAIL("wrong type"); mln_expr_val_free(v); return; }
    mln_string_t expected = mln_string("myvarsuffix");
    if (mln_string_strcmp(v->data.s, &expected)) { FAIL("wrong value"); mln_expr_val_free(v); return; }
    mln_expr_val_free(v);
    PASS();
}

static void test_all_escape_chars(void)
{
    TEST("all escape characters");
    mln_string_t exp = mln_string("\"\\\"\\'\\\\ \\n\\t\\b\\a\\f\\r\\v\"");
    mln_expr_val_t *v = mln_expr_run(&exp, simple_var_handler, NULL);
    if (v == NULL) { FAIL("run returned NULL"); return; }
    if (v->type != mln_expr_type_string) { FAIL("wrong type"); mln_expr_val_free(v); return; }
    mln_string_t expected = mln_string("\"'\\ \n\t\b\a\f\r\v");
    if (mln_string_strcmp(v->data.s, &expected)) { FAIL("wrong value"); mln_expr_val_free(v); return; }
    mln_expr_val_free(v);
    PASS();
}

static void test_deeply_nested_calls(void)
{
    TEST("deeply nested function calls");
    mln_string_t exp = mln_string("concat(concat(concat('a', 'b'), 'c'), 'd')");
    mln_expr_val_t *v = mln_expr_run(&exp, concat_handler, NULL);
    if (v == NULL) { FAIL("run returned NULL"); return; }
    if (v->type != mln_expr_type_string) { FAIL("wrong type"); mln_expr_val_free(v); return; }
    mln_string_t expected = mln_string("abcd");
    if (mln_string_strcmp(v->data.s, &expected)) { FAIL("wrong value"); mln_expr_val_free(v); return; }
    mln_expr_val_free(v);
    PASS();
}

static void test_func_no_args(void)
{
    TEST("function with no arguments");
    mln_string_t exp = mln_string("concat()");
    mln_expr_val_t *v = mln_expr_run(&exp, concat_handler, NULL);
    if (v == NULL) { FAIL("run returned NULL"); return; }
    mln_expr_val_free(v);
    PASS();
}

static void test_mixed_types_in_expr(void)
{
    TEST("mixed expression types");
    /* variable then integer - last expression wins */
    mln_string_t exp = mln_string("myvar 42");
    mln_expr_val_t *v = mln_expr_run(&exp, simple_var_handler, NULL);
    if (v == NULL) { FAIL("run returned NULL"); return; }
    if (v->type != mln_expr_type_int) { FAIL("wrong type"); mln_expr_val_free(v); return; }
    if (v->data.i != 42) { FAIL("wrong value"); mln_expr_val_free(v); return; }
    mln_expr_val_free(v);
    PASS();
}

static void test_udata_type(void)
{
    TEST("udata type");
    int mydata = 42;
    mln_expr_val_t *v = mln_expr_val_new(mln_expr_type_udata, &mydata, NULL);
    assert(v != NULL && v->type == mln_expr_type_udata && v->data.u == &mydata);
    mln_expr_val_t *dup = mln_expr_val_dup(v);
    assert(dup != NULL && dup->type == mln_expr_type_udata && dup->data.u == &mydata);
    mln_expr_val_free(v);
    mln_expr_val_free(dup);
    PASS();
}

static void test_benchmark(void)
{
    TEST("benchmark (performance)");

    mln_string_t exp = mln_string("concat('hello', concat(world, '!'))");
    int iterations = 100000;
    struct timespec start, end;

    clock_gettime(CLOCK_MONOTONIC, &start);
    for (int i = 0; i < iterations; i++) {
        mln_expr_val_t *v = mln_expr_run(&exp, concat_handler, NULL);
        if (v == NULL) { FAIL("run returned NULL during benchmark"); return; }
        mln_expr_val_free(v);
    }
    clock_gettime(CLOCK_MONOTONIC, &end);

    double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    double ops_per_sec = iterations / elapsed;
    printf("PASS (%.0f ops/sec, %.3f sec for %d iterations)\n", ops_per_sec, elapsed, iterations);
    pass_count++;
}

static void test_benchmark_numbers(void)
{
    TEST("benchmark numbers (performance)");

    mln_string_t exp = mln_string("42 0xff 3.14 0777");
    int iterations = 100000;
    struct timespec start, end;

    clock_gettime(CLOCK_MONOTONIC, &start);
    for (int i = 0; i < iterations; i++) {
        mln_expr_val_t *v = mln_expr_run(&exp, simple_var_handler, NULL);
        if (v == NULL) { FAIL("run returned NULL during benchmark"); return; }
        mln_expr_val_free(v);
    }
    clock_gettime(CLOCK_MONOTONIC, &end);

    double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    double ops_per_sec = iterations / elapsed;
    printf("PASS (%.0f ops/sec, %.3f sec for %d iterations)\n", ops_per_sec, elapsed, iterations);
    pass_count++;
}

static void test_stability(void)
{
    TEST("stability (10K diverse runs)");

    mln_string_t exprs[] = {
        mln_string("myvar"),
        mln_string("42"),
        mln_string("0xff"),
        mln_string("3.14"),
        mln_string("'hello'"),
        mln_string("\"world\""),
        mln_string("true"),
        mln_string("false"),
        mln_string("null"),
        mln_string("concat('a', 'b')"),
        mln_string("concat(concat('x', 'y'), 'z')"),
        mln_string("a:b:var1"),
    };
    int n = sizeof(exprs) / sizeof(exprs[0]);

    for (int i = 0; i < 10000; i++) {
        mln_string_t *exp = &exprs[i % n];
        mln_expr_val_t *v;
        if (i % n < 9) {
            v = mln_expr_run(exp, simple_var_handler, NULL);
        } else if (i % n == 11) {
            v = mln_expr_run(exp, ns_handler, NULL);
        } else {
            v = mln_expr_run(exp, concat_handler, NULL);
        }
        if (v == NULL) {
            FAIL("NULL result during stability test");
            return;
        }
        mln_expr_val_free(v);
    }

    PASS();
}

static void test_stability_if_loop(void)
{
    TEST("stability if/loop (1K runs)");

    for (int i = 0; i < 1000; i++) {
        /* if */
        {
            mln_string_t exp = mln_string("if cond_true then result_then else result_else fi");
            mln_expr_val_t *v = mln_expr_run(&exp, if_handler, NULL);
            if (v == NULL) { FAIL("if returned NULL"); return; }
            mln_expr_val_free(v);
        }
        /* loop */
        {
            loop_counter = 0;
            mln_string_t exp = mln_string("loop cond do body end");
            mln_expr_val_t *v = mln_expr_run(&exp, loop_handler, NULL);
            if (v == NULL) { FAIL("loop returned NULL"); return; }
            mln_expr_val_free(v);
        }
    }

    PASS();
}

int main(void)
{
    printf("=== mln_expr tests ===\n");

    /* Constants */
    test_integer_dec();
    test_integer_hex();
    test_integer_oct();
    test_real_constant();
    test_string_dblq();
    test_string_sglq();
    test_string_escape();
    test_all_escape_chars();
    test_bool_true();
    test_bool_false();
    test_null_keyword();

    /* Variables */
    test_simple_variable();
    test_namespace();

    /* Functions */
    test_function_call();
    test_nested_function();
    test_func_with_variable_args();
    test_deeply_nested_calls();
    test_func_no_args();
    test_multiple_expressions();
    test_mixed_types_in_expr();

    /* If/else */
    test_if_true_branch();
    test_if_false_branch();
    test_if_no_else();
    test_nested_if();

    /* Loop */
    test_loop();
    test_loop_false_condition();

    /* Value API */
    test_val_new_free();
    test_val_dup();
    test_val_copy();
    test_udata_type();
    test_empty_expression();

    /* Performance */
    test_benchmark();
    test_benchmark_numbers();

    /* Stability */
    test_stability();
    test_stability_if_loop();

    printf("\n=== Results: %d/%d passed ===\n", pass_count, test_count);
    return (pass_count == test_count) ? 0 : 1;
}
