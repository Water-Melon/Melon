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
    (void)ns; (void)args; (void)data;
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
    (void)ns; (void)data;
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
    (void)ns; (void)args; (void)data;
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
    (void)ns; (void)args; (void)data;
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
    (void)args; (void)data;
    if (is_func) {
        /* For namespace function calls, return namespace+name concatenated */
        if (ns != NULL) {
            mln_string_t sep = mln_string(":");
            mln_string_t *ns_sep = mln_string_strcat(ns, &sep);
            mln_string_t *result = mln_string_strcat(ns_sep, name);
            mln_string_free(ns_sep);
            mln_expr_val_t *v = mln_expr_val_new(mln_expr_type_string, result, NULL);
            mln_string_free(result);
            return v;
        }
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

/* Handler that uses udata pointer from data argument */
static void udata_free_fn(void *p)
{
    /* no-op for test */
    (void)p;
}

/* Handler that uses mln_string_ref on name to test heap safety.
 * If name is stack-allocated, this would cause use-after-free. */
static mln_expr_val_t *
ref_name_handler(mln_string_t *ns, mln_string_t *name, int is_func, mln_array_t *args, void *data)
{
    (void)ns; (void)args; (void)data;
    mln_string_t *ref;
    mln_expr_val_t *v;

    if (is_func) {
        /* For function calls, return function name as string via ref */
        ref = mln_string_ref(name);
        v = mln_expr_val_new(mln_expr_type_string, ref, NULL);
        mln_string_free(ref);
        return v;
    }
    /* For variables, use mln_string_ref (not dup) on name */
    ref = mln_string_ref(name);
    v = mln_expr_val_new(mln_expr_type_string, ref, NULL);
    mln_string_free(ref);
    return v;
}

/* --- Test functions --- */

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

static void test_integer_hex_upper(void)
{
    TEST("hexadecimal upper case hex digits");
    mln_string_t exp = mln_string("0xAB");
    mln_expr_val_t *v = mln_expr_run(&exp, simple_var_handler, NULL);
    if (v == NULL) { FAIL("run returned NULL"); return; }
    if (v->type != mln_expr_type_int) { FAIL("wrong type"); mln_expr_val_free(v); return; }
    if (v->data.i != 0xAB) { FAIL("wrong value"); mln_expr_val_free(v); return; }
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

static void test_integer_zero(void)
{
    TEST("zero integer");
    mln_string_t exp = mln_string("0");
    mln_expr_val_t *v = mln_expr_run(&exp, simple_var_handler, NULL);
    if (v == NULL) { FAIL("run returned NULL"); return; }
    if (v->type != mln_expr_type_int) { FAIL("wrong type"); mln_expr_val_free(v); return; }
    if (v->data.i != 0) { FAIL("wrong value"); mln_expr_val_free(v); return; }
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

static void test_real_zero(void)
{
    TEST("real zero");
    mln_string_t exp = mln_string("0.0");
    mln_expr_val_t *v = mln_expr_run(&exp, simple_var_handler, NULL);
    if (v == NULL) { FAIL("run returned NULL"); return; }
    if (v->type != mln_expr_type_real) { FAIL("wrong type"); mln_expr_val_free(v); return; }
    if (v->data.r != 0.0) { FAIL("wrong value"); mln_expr_val_free(v); return; }
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
    mln_string_t exp = mln_string("\"hello\\nworld\"");
    mln_expr_val_t *v = mln_expr_run(&exp, simple_var_handler, NULL);
    if (v == NULL) { FAIL("run returned NULL"); return; }
    if (v->type != mln_expr_type_string) { FAIL("wrong type"); mln_expr_val_free(v); return; }
    mln_string_t expected = mln_string("hello\nworld");
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

static void test_sglq_escape(void)
{
    TEST("single-quote escape sequences");
    mln_string_t exp = mln_string("'hello\\tworld'");
    mln_expr_val_t *v = mln_expr_run(&exp, simple_var_handler, NULL);
    if (v == NULL) { FAIL("run returned NULL"); return; }
    if (v->type != mln_expr_type_string) { FAIL("wrong type"); mln_expr_val_free(v); return; }
    mln_string_t expected = mln_string("hello\tworld");
    if (mln_string_strcmp(v->data.s, &expected)) { FAIL("wrong value"); mln_expr_val_free(v); return; }
    mln_expr_val_free(v);
    PASS();
}

static void test_string_empty(void)
{
    TEST("empty string");
    mln_string_t exp = mln_string("\"\"");
    mln_expr_val_t *v = mln_expr_run(&exp, simple_var_handler, NULL);
    if (v == NULL) { FAIL("run returned NULL"); return; }
    if (v->type != mln_expr_type_string) { FAIL("wrong type"); mln_expr_val_free(v); return; }
    if (v->data.s->len != 0) { FAIL("expected empty string"); mln_expr_val_free(v); return; }
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

static void test_underscore_variable(void)
{
    TEST("underscore variable name");
    mln_string_t exp = mln_string("_my_var_1");
    mln_expr_val_t *v = mln_expr_run(&exp, simple_var_handler, NULL);
    if (v == NULL) { FAIL("run returned NULL"); return; }
    if (v->type != mln_expr_type_string) { FAIL("wrong type"); mln_expr_val_free(v); return; }
    mln_string_t expected = mln_string("_my_var_1");
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

static void test_namespace_func(void)
{
    TEST("namespace function call");
    mln_string_t exp = mln_string("ns1:ns2:myfunc()");
    mln_expr_val_t *v = mln_expr_run(&exp, ns_handler, NULL);
    if (v == NULL) { FAIL("run returned NULL"); return; }
    if (v->type != mln_expr_type_string) { FAIL("wrong type"); mln_expr_val_free(v); return; }
    mln_string_t expected = mln_string("ns1:ns2:myfunc");
    if (mln_string_strcmp(v->data.s, &expected)) { FAIL("wrong value"); mln_expr_val_free(v); return; }
    mln_expr_val_free(v);
    PASS();
}

static void test_function_call(void)
{
    TEST("function call with args");
    mln_string_t exp = mln_string("concat('hello', ' world')");
    mln_expr_val_t *v = mln_expr_run(&exp, concat_handler, NULL);
    if (v == NULL) { FAIL("run returned NULL"); return; }
    if (v->type != mln_expr_type_string) { FAIL("wrong type"); mln_expr_val_free(v); return; }
    mln_string_t expected = mln_string("hello world");
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

static void test_multiple_expressions(void)
{
    TEST("multiple expressions");
    mln_string_t exp = mln_string("'first' 'second' 'third'");
    mln_expr_val_t *v = mln_expr_run(&exp, simple_var_handler, NULL);
    if (v == NULL) { FAIL("run returned NULL"); return; }
    if (v->type != mln_expr_type_string) { FAIL("wrong type"); mln_expr_val_free(v); return; }
    mln_string_t expected = mln_string("third");
    if (mln_string_strcmp(v->data.s, &expected)) { FAIL("wrong value"); mln_expr_val_free(v); return; }
    mln_expr_val_free(v);
    PASS();
}

static void test_mixed_types_in_expr(void)
{
    TEST("mixed expression types");
    mln_string_t exp = mln_string("myvar 42");
    mln_expr_val_t *v = mln_expr_run(&exp, simple_var_handler, NULL);
    if (v == NULL) { FAIL("run returned NULL"); return; }
    if (v->type != mln_expr_type_int) { FAIL("wrong type"); mln_expr_val_free(v); return; }
    if (v->data.i != 42) { FAIL("wrong value"); mln_expr_val_free(v); return; }
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

static void test_if_no_else_false(void)
{
    TEST("if-then without else (false)");
    mln_string_t exp = mln_string("'default' if cond_false then result_then fi");
    mln_expr_val_t *v = mln_expr_run(&exp, if_handler, NULL);
    if (v == NULL) { FAIL("run returned NULL"); return; }
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

static void test_nested_if_false_outer(void)
{
    TEST("nested if false outer");
    mln_string_t exp = mln_string("if cond_false then if cond_true then result_then fi else result_else fi");
    mln_expr_val_t *v = mln_expr_run(&exp, if_handler, NULL);
    if (v == NULL) { FAIL("run returned NULL"); return; }
    if (v->type != mln_expr_type_string) { FAIL("wrong type"); mln_expr_val_free(v); return; }
    mln_string_t expected = mln_string("else_branch");
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
    loop_counter = 10;
    mln_string_t exp = mln_string("loop cond do body end");
    mln_expr_val_t *v = mln_expr_run(&exp, loop_handler, NULL);
    if (v == NULL) { FAIL("run returned NULL"); return; }
    if (v->type != mln_expr_type_null) { FAIL("wrong type"); mln_expr_val_free(v); return; }
    mln_expr_val_free(v);
    PASS();
}

static void test_val_new_free(void)
{
    TEST("val_new and val_free");
    mln_expr_val_t *v;

    v = mln_expr_val_new(mln_expr_type_null, NULL, NULL);
    assert(v != NULL && v->type == mln_expr_type_null);
    mln_expr_val_free(v);

    mln_u8_t b = 1;
    v = mln_expr_val_new(mln_expr_type_bool, &b, NULL);
    assert(v != NULL && v->type == mln_expr_type_bool && v->data.b == 1);
    mln_expr_val_free(v);

    mln_s64_t i = 12345;
    v = mln_expr_val_new(mln_expr_type_int, &i, NULL);
    assert(v != NULL && v->type == mln_expr_type_int && v->data.i == 12345);
    mln_expr_val_free(v);

    double r = 1.5;
    v = mln_expr_val_new(mln_expr_type_real, &r, NULL);
    assert(v != NULL && v->type == mln_expr_type_real && v->data.r == 1.5);
    mln_expr_val_free(v);

    mln_string_t s = mln_string("test");
    v = mln_expr_val_new(mln_expr_type_string, &s, NULL);
    assert(v != NULL && v->type == mln_expr_type_string);
    mln_expr_val_free(v);

    mln_expr_val_free(NULL);

    PASS();
}

static void test_val_dup(void)
{
    TEST("val_dup");
    mln_string_t s = mln_string("dup_test");
    mln_expr_val_t *orig = mln_expr_val_new(mln_expr_type_string, &s, NULL);
    assert(orig != NULL);
    mln_expr_val_t *dup = mln_expr_val_dup_own(orig);
    assert(dup != NULL);
    assert(dup->type == mln_expr_type_string);
    assert(!mln_string_strcmp(dup->data.s, &s));
    mln_expr_val_free(orig);
    mln_expr_val_free(dup);
    PASS();
}

static void test_val_dup_all_types(void)
{
    TEST("val_dup all types");
    mln_expr_val_t *v, *d;

    /* null */
    v = mln_expr_val_new(mln_expr_type_null, NULL, NULL);
    d = mln_expr_val_dup_own(v);
    assert(d != NULL && d->type == mln_expr_type_null);
    mln_expr_val_free(v); mln_expr_val_free(d);

    /* bool */
    mln_u8_t b = 0;
    v = mln_expr_val_new(mln_expr_type_bool, &b, NULL);
    d = mln_expr_val_dup_own(v);
    assert(d != NULL && d->type == mln_expr_type_bool && d->data.b == 0);
    mln_expr_val_free(v); mln_expr_val_free(d);

    /* int */
    mln_s64_t i = -42;
    v = mln_expr_val_new(mln_expr_type_int, &i, NULL);
    d = mln_expr_val_dup_own(v);
    assert(d != NULL && d->type == mln_expr_type_int && d->data.i == -42);
    mln_expr_val_free(v); mln_expr_val_free(d);

    /* real */
    double r = 2.718;
    v = mln_expr_val_new(mln_expr_type_real, &r, NULL);
    d = mln_expr_val_dup_own(v);
    assert(d != NULL && d->type == mln_expr_type_real && d->data.r == 2.718);
    mln_expr_val_free(v); mln_expr_val_free(d);

    PASS();
}

static void test_val_copy(void)
{
    TEST("val_copy");
    mln_s64_t i = 999;
    mln_expr_val_t *src = mln_expr_val_new(mln_expr_type_int, &i, NULL);
    mln_expr_val_t dest;
    mln_expr_val_copy_own(&dest, src);
    assert(dest.type == mln_expr_type_int && dest.data.i == 999);
    mln_expr_val_free(src);

    mln_expr_val_copy_own(&dest, NULL);
    PASS();
}

static void test_udata_type(void)
{
    TEST("udata type");
    int mydata = 42;
    mln_expr_val_t *v = mln_expr_val_new(mln_expr_type_udata, &mydata, NULL);
    assert(v != NULL && v->type == mln_expr_type_udata && v->data.u == &mydata);
    mln_expr_val_t *dup = mln_expr_val_dup_own(v);
    assert(dup != NULL && dup->type == mln_expr_type_udata && dup->data.u == &mydata);
    mln_expr_val_free(v);
    mln_expr_val_free(dup);
    PASS();
}

static void test_udata_with_free(void)
{
    TEST("udata with free function");
    int mydata = 99;
    mln_expr_val_t *v = mln_expr_val_new(mln_expr_type_udata, &mydata, udata_free_fn);
    assert(v != NULL && v->type == mln_expr_type_udata);
    assert(v->free == udata_free_fn);
    mln_expr_val_free(v);
    PASS();
}

static void test_udata_dup_non_mutating(void)
{
    TEST("udata dup: ownership transfers to dest");
    int mydata = 77;
    mln_expr_val_t *v = mln_expr_val_new(mln_expr_type_udata, &mydata, udata_free_fn);
    assert(v != NULL && v->free == udata_free_fn);
    mln_expr_val_t *dup = mln_expr_val_dup_own(v);
    assert(dup != NULL && dup->type == mln_expr_type_udata);
    assert(dup->data.u == &mydata);
    /* Dest receives the destructor */
    assert(dup->free == udata_free_fn);
    /* Source's free is cleared (ownership transferred) */
    assert(v->free == NULL);
    mln_expr_val_free(dup);
    mln_expr_val_free(v);
    PASS();
}

static void test_udata_copy_non_mutating(void)
{
    TEST("udata copy: ownership transfers to dest");
    int mydata = 88;
    mln_expr_val_t *v = mln_expr_val_new(mln_expr_type_udata, &mydata, udata_free_fn);
    assert(v != NULL && v->free == udata_free_fn);
    mln_expr_val_t dest;
    mln_expr_val_copy_own(&dest, v);
    assert(dest.type == mln_expr_type_udata);
    assert(dest.data.u == &mydata);
    /* Dest receives the destructor */
    assert(dest.free == udata_free_fn);
    /* Source's free is cleared (ownership transferred) */
    assert(v->free == NULL);
    mln_expr_val_free(v);
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

static void test_whitespace_only(void)
{
    TEST("whitespace only expression");
    mln_string_t exp = mln_string("   \t  \n  ");
    mln_expr_val_t *v = mln_expr_run(&exp, simple_var_handler, NULL);
    if (v == NULL) { FAIL("run returned NULL"); return; }
    if (v->type != mln_expr_type_null) { FAIL("expected null type"); mln_expr_val_free(v); return; }
    mln_expr_val_free(v);
    PASS();
}

static void test_if_with_func(void)
{
    TEST("if with function call in body");
    /* Use a boolean literal so the condition exercises real if evaluation. */
    mln_string_t exp = mln_string("if true then concat('yes', '!') fi");
    mln_expr_val_t *v = mln_expr_run(&exp, concat_handler, NULL);
    if (v == NULL) { FAIL("run returned NULL"); return; }
    if (v->type != mln_expr_type_string) { FAIL("wrong type"); mln_expr_val_free(v); return; }
    mln_string_t expected = mln_string("yes!");
    if (mln_string_strcmp(v->data.s, &expected)) { FAIL("wrong value"); mln_expr_val_free(v); return; }
    mln_expr_val_free(v);
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

static void test_benchmark_if_loop(void)
{
    TEST("benchmark if/loop (performance)");

    int iterations = 10000;
    struct timespec start, end;

    clock_gettime(CLOCK_MONOTONIC, &start);
    for (int i = 0; i < iterations; i++) {
        {
            mln_string_t exp = mln_string("if cond_true then result_then else result_else fi");
            mln_expr_val_t *v = mln_expr_run(&exp, if_handler, NULL);
            if (v == NULL) { FAIL("if returned NULL"); return; }
            mln_expr_val_free(v);
        }
        {
            loop_counter = 0;
            mln_string_t exp = mln_string("loop cond do body end");
            mln_expr_val_t *v = mln_expr_run(&exp, loop_handler, NULL);
            if (v == NULL) { FAIL("loop returned NULL"); return; }
            mln_expr_val_free(v);
        }
    }
    clock_gettime(CLOCK_MONOTONIC, &end);

    double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    double ops_per_sec = (iterations * 2) / elapsed;
    printf("PASS (%.0f ops/sec, %.3f sec for %d iterations)\n", ops_per_sec, elapsed, iterations * 2);
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
        {
            mln_string_t exp = mln_string("if cond_true then result_then else result_else fi");
            mln_expr_val_t *v = mln_expr_run(&exp, if_handler, NULL);
            if (v == NULL) { FAIL("if returned NULL"); return; }
            mln_expr_val_free(v);
        }
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

static void test_stability_all_types(void)
{
    TEST("stability all value types (5K runs)");

    for (int i = 0; i < 5000; i++) {
        mln_expr_val_t *v;
        switch (i % 7) {
            case 0: { mln_string_t e = mln_string("null"); v = mln_expr_run(&e, simple_var_handler, NULL); break; }
            case 1: { mln_string_t e = mln_string("true"); v = mln_expr_run(&e, simple_var_handler, NULL); break; }
            case 2: { mln_string_t e = mln_string("false"); v = mln_expr_run(&e, simple_var_handler, NULL); break; }
            case 3: { mln_string_t e = mln_string("12345"); v = mln_expr_run(&e, simple_var_handler, NULL); break; }
            case 4: { mln_string_t e = mln_string("3.14"); v = mln_expr_run(&e, simple_var_handler, NULL); break; }
            case 5: { mln_string_t e = mln_string("'text'"); v = mln_expr_run(&e, simple_var_handler, NULL); break; }
            default: { mln_string_t e = mln_string("myvar"); v = mln_expr_run(&e, simple_var_handler, NULL); break; }
        }
        if (v == NULL) { FAIL("NULL result"); return; }
        mln_expr_val_free(v);
    }

    PASS();
}

/*
 * Helper: extract real args from expr args array.
 * The expr parser inserts null entries as comma separators, so f(a, b) produces
 * [a_val, null_sep, b_val]. This helper collects pointers to non-null entries.
 * Returns the number of real arguments.
 */
static int extract_args(mln_array_t *args, mln_expr_val_t **out, int max_out)
{
    mln_expr_val_t *elts = (mln_expr_val_t *)mln_array_elts(args);
    int total = (int)mln_array_nelts(args);
    int count = 0, i;
    for (i = 0; i < total && count < max_out; i++) {
        if (elts[i].type != mln_expr_type_null)
            out[count++] = &elts[i];
    }
    return count;
}

/* ======================================================================
 * Universal handler for complex combination tests.
 * Variables:  x -> int 10, y -> int 20, z -> int 0, flag -> bool true,
 *             off -> bool false, msg -> string "hello", tag -> string "world",
 *             counter -> tracks call count (int), anything else -> string(name)
 * Functions:  add(a,b) -> int a+b, mul(a,b) -> int a*b,
 *             cat(args...) -> string concatenation,
 *             iif(cond,a,b) -> a if cond true else b,
 *             len(s) -> int string length,
 *             upper(s) -> string (returns "UPPER:" + s),
 *             identity(v) -> v unchanged
 * Namespace:  returns "NS(ns):name" string
 * ====================================================================== */
static int complex_counter = 0;

static mln_expr_val_t *
complex_handler(mln_string_t *ns, mln_string_t *name, int is_func, mln_array_t *args, void *data)
{
    (void)data;
    mln_string_t s_x = mln_string("x");
    mln_string_t s_y = mln_string("y");
    mln_string_t s_z = mln_string("z");
    mln_string_t s_flag = mln_string("flag");
    mln_string_t s_off = mln_string("off");
    mln_string_t s_msg = mln_string("msg");
    mln_string_t s_tag = mln_string("tag");
    mln_string_t s_counter = mln_string("counter");

    /* --- Namespace handling --- */
    if (ns != NULL) {
        /* Build "NS(ns):name" or for functions "NS(ns):name()" */
        mln_string_t prefix = mln_string("NS(");
        mln_string_t mid = mln_string("):");
        mln_string_t *s1 = mln_string_strcat(&prefix, ns);
        mln_string_t *s2 = mln_string_strcat(s1, &mid);
        mln_string_free(s1);
        mln_string_t *s3 = mln_string_strcat(s2, name);
        mln_string_free(s2);
        mln_expr_val_t *r = mln_expr_val_new(mln_expr_type_string, s3, NULL);
        mln_string_free(s3);
        return r;
    }

    if (!is_func) {
        /* Variable resolution */
        if (!mln_string_strcmp(name, &s_x)) { mln_s64_t v = 10; return mln_expr_val_new(mln_expr_type_int, &v, NULL); }
        if (!mln_string_strcmp(name, &s_y)) { mln_s64_t v = 20; return mln_expr_val_new(mln_expr_type_int, &v, NULL); }
        if (!mln_string_strcmp(name, &s_z)) { mln_s64_t v = 0; return mln_expr_val_new(mln_expr_type_int, &v, NULL); }
        if (!mln_string_strcmp(name, &s_flag)) { mln_u8_t v = 1; return mln_expr_val_new(mln_expr_type_bool, &v, NULL); }
        if (!mln_string_strcmp(name, &s_off)) { mln_u8_t v = 0; return mln_expr_val_new(mln_expr_type_bool, &v, NULL); }
        if (!mln_string_strcmp(name, &s_counter)) {
            complex_counter++;
            mln_s64_t v = complex_counter;
            return mln_expr_val_new(mln_expr_type_int, &v, NULL);
        }
        if (!mln_string_strcmp(name, &s_msg)) {
            mln_string_t tmp = mln_string("hello");
            mln_string_t *s = mln_string_dup(&tmp);
            mln_expr_val_t *r = mln_expr_val_new(mln_expr_type_string, s, NULL);
            mln_string_free(s);
            return r;
        }
        if (!mln_string_strcmp(name, &s_tag)) {
            mln_string_t tmp = mln_string("world");
            mln_string_t *s = mln_string_dup(&tmp);
            mln_expr_val_t *r = mln_expr_val_new(mln_expr_type_string, s, NULL);
            mln_string_free(s);
            return r;
        }
        /* default: return string of the variable name */
        {
            mln_string_t *s = mln_string_dup(name);
            if (s == NULL) return NULL;
            mln_expr_val_t *r = mln_expr_val_new(mln_expr_type_string, s, NULL);
            mln_string_free(s);
            return r;
        }
    }

    /* --- Function handling --- */
    mln_string_t f_add = mln_string("add");
    mln_string_t f_mul = mln_string("mul");
    mln_string_t f_cat = mln_string("cat");
    mln_string_t f_iif = mln_string("iif");
    mln_string_t f_len = mln_string("len");
    mln_string_t f_upper = mln_string("upper");
    mln_string_t f_identity = mln_string("identity");

    mln_expr_val_t *real_args[32];
    int n = extract_args(args, real_args, 32);

    if (!mln_string_strcmp(name, &f_add)) {
        if (n >= 2 && real_args[0]->type == mln_expr_type_int && real_args[1]->type == mln_expr_type_int) {
            mln_s64_t v = real_args[0]->data.i + real_args[1]->data.i;
            return mln_expr_val_new(mln_expr_type_int, &v, NULL);
        }
        return mln_expr_val_new(mln_expr_type_null, NULL, NULL);
    }
    if (!mln_string_strcmp(name, &f_mul)) {
        if (n >= 2 && real_args[0]->type == mln_expr_type_int && real_args[1]->type == mln_expr_type_int) {
            mln_s64_t v = real_args[0]->data.i * real_args[1]->data.i;
            return mln_expr_val_new(mln_expr_type_int, &v, NULL);
        }
        return mln_expr_val_new(mln_expr_type_null, NULL, NULL);
    }
    if (!mln_string_strcmp(name, &f_cat)) {
        mln_string_t *acc = NULL;
        int i;
        for (i = 0; i < n; i++) {
            if (real_args[i]->type != mln_expr_type_string) continue;
            if (acc == NULL) { acc = mln_string_ref(real_args[i]->data.s); continue; }
            mln_string_t *tmp = mln_string_strcat(acc, real_args[i]->data.s);
            mln_string_free(acc);
            acc = tmp;
        }
        if (acc == NULL) return mln_expr_val_new(mln_expr_type_null, NULL, NULL);
        mln_expr_val_t *r = mln_expr_val_new(mln_expr_type_string, acc, NULL);
        mln_string_free(acc);
        return r;
    }
    if (!mln_string_strcmp(name, &f_iif)) {
        if (n >= 3) {
            int cond = 0;
            if (real_args[0]->type == mln_expr_type_bool) cond = real_args[0]->data.b;
            else if (real_args[0]->type == mln_expr_type_int) cond = real_args[0]->data.i != 0;
            return mln_expr_val_dup_own(cond ? real_args[1] : real_args[2]);
        }
        return mln_expr_val_new(mln_expr_type_null, NULL, NULL);
    }
    if (!mln_string_strcmp(name, &f_len)) {
        if (n >= 1 && real_args[0]->type == mln_expr_type_string) {
            mln_s64_t v = (mln_s64_t)real_args[0]->data.s->len;
            return mln_expr_val_new(mln_expr_type_int, &v, NULL);
        }
        mln_s64_t v = 0;
        return mln_expr_val_new(mln_expr_type_int, &v, NULL);
    }
    if (!mln_string_strcmp(name, &f_upper)) {
        if (n >= 1 && real_args[0]->type == mln_expr_type_string) {
            mln_string_t prefix = mln_string("UPPER:");
            mln_string_t *s = mln_string_strcat(&prefix, real_args[0]->data.s);
            mln_expr_val_t *r = mln_expr_val_new(mln_expr_type_string, s, NULL);
            mln_string_free(s);
            return r;
        }
        return mln_expr_val_new(mln_expr_type_null, NULL, NULL);
    }
    if (!mln_string_strcmp(name, &f_identity)) {
        if (n >= 1) return mln_expr_val_dup_own(real_args[0]);
        return mln_expr_val_new(mln_expr_type_null, NULL, NULL);
    }
    /* unknown function: return null */
    return mln_expr_val_new(mln_expr_type_null, NULL, NULL);
}

/* --- Loop+counter handler for complex loop tests --- */
static int complex_loop_max = 0;

static mln_expr_val_t *
complex_loop_handler(mln_string_t *ns, mln_string_t *name, int is_func, mln_array_t *args, void *data)
{
    (void)data;
    mln_string_t s_cond = mln_string("cond");
    mln_string_t s_body = mln_string("body");
    mln_string_t s_flag = mln_string("flag");
    mln_string_t s_off = mln_string("off");
    mln_string_t s_x = mln_string("x");
    mln_string_t s_msg = mln_string("msg");

    /* namespace */
    if (ns != NULL) {
        mln_string_t *s = mln_string_dup(name);
        if (s == NULL) return NULL;
        mln_expr_val_t *r = mln_expr_val_new(mln_expr_type_string, s, NULL);
        mln_string_free(s);
        return r;
    }

    if (!is_func) {
        if (!mln_string_strcmp(name, &s_cond)) {
            mln_u8_t b = (complex_counter < complex_loop_max) ? 1 : 0;
            return mln_expr_val_new(mln_expr_type_bool, &b, NULL);
        }
        if (!mln_string_strcmp(name, &s_body)) {
            complex_counter++;
            mln_s64_t v = complex_counter;
            return mln_expr_val_new(mln_expr_type_int, &v, NULL);
        }
        if (!mln_string_strcmp(name, &s_flag)) { mln_u8_t b = 1; return mln_expr_val_new(mln_expr_type_bool, &b, NULL); }
        if (!mln_string_strcmp(name, &s_off)) { mln_u8_t b = 0; return mln_expr_val_new(mln_expr_type_bool, &b, NULL); }
        if (!mln_string_strcmp(name, &s_x)) { mln_s64_t v = 10; return mln_expr_val_new(mln_expr_type_int, &v, NULL); }
        if (!mln_string_strcmp(name, &s_msg)) {
            mln_string_t tmp = mln_string("hello");
            mln_string_t *s = mln_string_dup(&tmp);
            mln_expr_val_t *r = mln_expr_val_new(mln_expr_type_string, s, NULL);
            mln_string_free(s);
            return r;
        }
        {
            mln_string_t *s = mln_string_dup(name);
            if (s == NULL) return NULL;
            mln_expr_val_t *r = mln_expr_val_new(mln_expr_type_string, s, NULL);
            mln_string_free(s);
            return r;
        }
    }

    /* functions: add, cat, identity */
    mln_string_t f_add = mln_string("add");
    mln_string_t f_cat = mln_string("cat");
    mln_string_t f_identity = mln_string("identity");
    mln_expr_val_t *real_args[32];
    int n = extract_args(args, real_args, 32);

    if (!mln_string_strcmp(name, &f_add)) {
        if (n >= 2 && real_args[0]->type == mln_expr_type_int && real_args[1]->type == mln_expr_type_int) {
            mln_s64_t v = real_args[0]->data.i + real_args[1]->data.i;
            return mln_expr_val_new(mln_expr_type_int, &v, NULL);
        }
        return mln_expr_val_new(mln_expr_type_null, NULL, NULL);
    }
    if (!mln_string_strcmp(name, &f_cat)) {
        mln_string_t *acc = NULL;
        int i;
        for (i = 0; i < n; i++) {
            if (real_args[i]->type != mln_expr_type_string) continue;
            if (acc == NULL) { acc = mln_string_ref(real_args[i]->data.s); continue; }
            mln_string_t *tmp = mln_string_strcat(acc, real_args[i]->data.s);
            mln_string_free(acc);
            acc = tmp;
        }
        if (acc == NULL) return mln_expr_val_new(mln_expr_type_null, NULL, NULL);
        mln_expr_val_t *r = mln_expr_val_new(mln_expr_type_string, acc, NULL);
        mln_string_free(acc);
        return r;
    }
    if (!mln_string_strcmp(name, &f_identity)) {
        if (n >= 1) return mln_expr_val_dup_own(real_args[0]);
        return mln_expr_val_new(mln_expr_type_null, NULL, NULL);
    }
    return mln_expr_val_new(mln_expr_type_null, NULL, NULL);
}

/* =====================================================================
 * Complex combination tests
 * ===================================================================== */

/* Test 1: if-then with nested function calls inside both branches */
static void test_complex_if_with_nested_funcs(void)
{
    TEST("complex: if-then-else with nested function calls");
    /* if flag then cat(upper(msg), ' ', tag) else cat('no', 'pe') fi
     * flag=true -> cat(upper("hello"), " ", "world") = cat("UPPER:hello", " ", "world") = "UPPER:hello world"
     */
    mln_string_t exp = mln_string("if flag then cat(upper(msg), ' ', tag) else cat('no', 'pe') fi");
    mln_expr_val_t *v = mln_expr_run(&exp, complex_handler, NULL);
    if (v == NULL) { FAIL("run returned NULL"); return; }
    if (v->type != mln_expr_type_string) { FAIL("wrong type"); mln_expr_val_free(v); return; }
    mln_string_t expected = mln_string("UPPER:hello world");
    if (mln_string_strcmp(v->data.s, &expected)) { FAIL("wrong value"); mln_expr_val_free(v); return; }
    mln_expr_val_free(v);
    PASS();
}

/* Test 2: if false branch with deeply nested functions */
static void test_complex_if_false_nested_funcs(void)
{
    TEST("complex: if-else false branch with deeply nested funcs");
    /* if off then identity(x) else add(mul(x, y), add(x, y)) fi
     * off=false -> add(mul(10,20), add(10,20)) = add(200, 30) = 230
     */
    mln_string_t exp = mln_string("if off then identity(x) else add(mul(x, y), add(x, y)) fi");
    mln_expr_val_t *v = mln_expr_run(&exp, complex_handler, NULL);
    if (v == NULL) { FAIL("run returned NULL"); return; }
    if (v->type != mln_expr_type_int) { FAIL("wrong type"); mln_expr_val_free(v); return; }
    if (v->data.i != 230) { FAIL("wrong value"); mln_expr_val_free(v); return; }
    mln_expr_val_free(v);
    PASS();
}

/* Test 3: Nested if-else-if (3 levels deep) */
static void test_complex_triple_nested_if(void)
{
    TEST("complex: triple nested if-else");
    /* if flag then
     *   if flag then
     *     if off then 'deep_wrong' else cat('triple', '_', 'nested') fi
     *   else 'mid_wrong' fi
     * else 'outer_wrong' fi
     * => cat("triple","_","nested") = "triple_nested"
     */
    mln_string_t exp = mln_string(
        "if flag then "
            "if flag then "
                "if off then 'deep_wrong' else cat('triple', '_', 'nested') fi "
            "else 'mid_wrong' fi "
        "else 'outer_wrong' fi"
    );
    mln_expr_val_t *v = mln_expr_run(&exp, complex_handler, NULL);
    if (v == NULL) { FAIL("run returned NULL"); return; }
    if (v->type != mln_expr_type_string) { FAIL("wrong type"); mln_expr_val_free(v); return; }
    mln_string_t expected = mln_string("triple_nested");
    if (mln_string_strcmp(v->data.s, &expected)) { FAIL("wrong value"); mln_expr_val_free(v); return; }
    mln_expr_val_free(v);
    PASS();
}

/* Test 4: Multiple expressions with if in the middle */
static void test_complex_multi_expr_with_if(void)
{
    TEST("complex: multiple expressions with if in middle");
    /* 42 if flag then add(x, y) fi 'end_marker'
     * 42 -> ret=42, if flag then add(10,20) fi -> ret=30, 'end_marker' -> ret='end_marker'
     */
    mln_string_t exp = mln_string("42 if flag then add(x, y) fi 'end_marker'");
    mln_expr_val_t *v = mln_expr_run(&exp, complex_handler, NULL);
    if (v == NULL) { FAIL("run returned NULL"); return; }
    if (v->type != mln_expr_type_string) { FAIL("wrong type"); mln_expr_val_free(v); return; }
    mln_string_t expected = mln_string("end_marker");
    if (mln_string_strcmp(v->data.s, &expected)) { FAIL("wrong value"); mln_expr_val_free(v); return; }
    mln_expr_val_free(v);
    PASS();
}

/* Test 5: Loop containing if-else in body */
static void test_complex_loop_with_if_body(void)
{
    TEST("complex: loop containing if-else in body");
    /* loop cond do if flag then body else 0 fi end
     * loops 3 times, each time flag=true so body increments counter
     */
    complex_counter = 0;
    complex_loop_max = 3;
    mln_string_t exp = mln_string("loop cond do if flag then body else 'skip' fi end");
    mln_expr_val_t *v = mln_expr_run(&exp, complex_loop_handler, NULL);
    if (v == NULL) { FAIL("run returned NULL"); return; }
    if (v->type != mln_expr_type_int) { FAIL("wrong type"); mln_expr_val_free(v); return; }
    if (v->data.i != 3) { FAIL("wrong value"); mln_expr_val_free(v); return; }
    if (complex_counter != 3) { FAIL("counter wrong"); mln_expr_val_free(v); return; }
    mln_expr_val_free(v);
    PASS();
}

/* Test 6: Loop containing function call in body */
static void test_complex_loop_with_func_body(void)
{
    TEST("complex: loop with function calls in body");
    /* loop cond do add(body, x) end -> loops 4 times, last = add(4, 10) = 14 */
    complex_counter = 0;
    complex_loop_max = 4;
    mln_string_t exp = mln_string("loop cond do add(body, x) end");
    mln_expr_val_t *v = mln_expr_run(&exp, complex_loop_handler, NULL);
    if (v == NULL) { FAIL("run returned NULL"); return; }
    if (v->type != mln_expr_type_int) { FAIL("wrong type"); mln_expr_val_free(v); return; }
    if (v->data.i != 14) { FAIL("wrong value"); mln_expr_val_free(v); return; }
    mln_expr_val_free(v);
    PASS();
}

/* Test 7: Function calling function calling function (4 levels) with mixed types */
static void test_complex_deep_func_chain(void)
{
    TEST("complex: 4-level deep function chain with mixed types");
    /* add(add(add(x, y), mul(x, x)), add(y, y))
     * = add(add(30, 100), 40) = add(130, 40) = 170
     */
    mln_string_t exp = mln_string("add(add(add(x, y), mul(x, x)), add(y, y))");
    mln_expr_val_t *v = mln_expr_run(&exp, complex_handler, NULL);
    if (v == NULL) { FAIL("run returned NULL"); return; }
    if (v->type != mln_expr_type_int) { FAIL("wrong type"); mln_expr_val_free(v); return; }
    if (v->data.i != 170) { FAIL("wrong value"); mln_expr_val_free(v); return; }
    mln_expr_val_free(v);
    PASS();
}

/* Test 8: String concatenation with escapes inside if */
static void test_complex_string_escape_in_if(void)
{
    TEST("complex: string escapes inside if branches");
    /* if flag then cat("hello\\nworld", '\\t', "end") fi
     * -> cat("hello\nworld", "\t", "end") = "hello\nworld\tend"
     */
    mln_string_t exp = mln_string("if flag then cat(\"hello\\nworld\", '\\t', \"end\") fi");
    mln_expr_val_t *v = mln_expr_run(&exp, complex_handler, NULL);
    if (v == NULL) { FAIL("run returned NULL"); return; }
    if (v->type != mln_expr_type_string) { FAIL("wrong type"); mln_expr_val_free(v); return; }
    mln_string_t expected = mln_string("hello\nworld\tend");
    if (mln_string_strcmp(v->data.s, &expected)) { FAIL("wrong value"); mln_expr_val_free(v); return; }
    mln_expr_val_free(v);
    PASS();
}

/* Test 9: Namespace + function + if combination */
static void test_complex_ns_func_if(void)
{
    TEST("complex: namespace + function + if combination");
    /* if flag then mod:sub:identity(cat('ns', '_', 'test')) fi
     * flag=true, mod:sub:identity -> handler sees ns="mod:sub", name="identity"
     * -> returns "NS(mod:sub):identity" (namespace handler doesn't call args)
     */
    mln_string_t exp = mln_string("if flag then mod:sub:identity(cat('ns', '_', 'test')) fi");
    mln_expr_val_t *v = mln_expr_run(&exp, complex_handler, NULL);
    if (v == NULL) { FAIL("run returned NULL"); return; }
    if (v->type != mln_expr_type_string) { FAIL("wrong type"); mln_expr_val_free(v); return; }
    mln_string_t expected = mln_string("NS(mod:sub):identity");
    if (mln_string_strcmp(v->data.s, &expected)) { FAIL("wrong value"); mln_expr_val_free(v); return; }
    mln_expr_val_free(v);
    PASS();
}

/* Test 10: Multiple if-else blocks sequentially */
static void test_complex_sequential_ifs(void)
{
    TEST("complex: multiple sequential if blocks");
    /* if off then 'skip1' else 'first' fi  if flag then add(x, y) fi  if off then 'skip2' fi
     * -> 'first', then 30, then null (no else). Last non-trivial = depends on impl.
     * Actually last expr result is kept. 'if off then skip2 fi' with no else => null stays from loop.
     * Last result should be what the loop last produced.
     */
    mln_string_t exp = mln_string("if off then 'skip1' else 'first' fi if flag then add(x, y) fi");
    mln_expr_val_t *v = mln_expr_run(&exp, complex_handler, NULL);
    if (v == NULL) { FAIL("run returned NULL"); return; }
    if (v->type != mln_expr_type_int) { FAIL("wrong type"); mln_expr_val_free(v); return; }
    if (v->data.i != 30) { FAIL("wrong value"); mln_expr_val_free(v); return; }
    mln_expr_val_free(v);
    PASS();
}

/* Test 11: if with constants (hex, oct, real) in branches */
static void test_complex_if_with_all_constants(void)
{
    TEST("complex: if branches with hex, oct, real constants");
    /* if flag then 0xff else 0777 fi -> flag=true -> 255 */
    mln_string_t exp = mln_string("if flag then 0xff else 0777 fi");
    mln_expr_val_t *v = mln_expr_run(&exp, complex_handler, NULL);
    if (v == NULL) { FAIL("run returned NULL"); return; }
    if (v->type != mln_expr_type_int) { FAIL("wrong type"); mln_expr_val_free(v); return; }
    if (v->data.i != 255) { FAIL("wrong value"); mln_expr_val_free(v); return; }
    mln_expr_val_free(v);

    /* if off then 0xff else 3.14 fi -> off=false -> 3.14 */
    mln_string_t exp2 = mln_string("if off then 0xff else 3.14 fi");
    v = mln_expr_run(&exp2, complex_handler, NULL);
    if (v == NULL) { FAIL("run returned NULL (2)"); return; }
    if (v->type != mln_expr_type_real) { FAIL("wrong type (2)"); mln_expr_val_free(v); return; }
    if (v->data.r < 3.13 || v->data.r > 3.15) { FAIL("wrong value (2)"); mln_expr_val_free(v); return; }
    mln_expr_val_free(v);
    PASS();
}

/* Test 12: Long expression - function chain with many args and nested calls */
static void test_complex_long_func_chain(void)
{
    TEST("complex: long function chain with many args");
    /* cat(cat('a','b','c'), cat('d','e'), cat(upper('f'), cat('g','h')), 'i', 'j')
     * = cat("abc", "de", "UPPER:fgh", "i", "j") = "abcdeUPPER:fghij"
     */
    mln_string_t exp = mln_string(
        "cat(cat('a', 'b', 'c'), cat('d', 'e'), cat(upper('f'), cat('g', 'h')), 'i', 'j')"
    );
    mln_expr_val_t *v = mln_expr_run(&exp, complex_handler, NULL);
    if (v == NULL) { FAIL("run returned NULL"); return; }
    if (v->type != mln_expr_type_string) { FAIL("wrong type"); mln_expr_val_free(v); return; }
    mln_string_t expected = mln_string("abcdeUPPER:fghij");
    if (mln_string_strcmp(v->data.s, &expected)) { FAIL("wrong value"); mln_expr_val_free(v); return; }
    mln_expr_val_free(v);
    PASS();
}

/* Test 13: Loop followed by if */
static void test_complex_loop_then_if(void)
{
    TEST("complex: loop followed by if expression");
    /* loop cond do body end if flag then 'after_loop' fi
     * loop runs 2 times, then 'after_loop'
     */
    complex_counter = 0;
    complex_loop_max = 2;
    mln_string_t exp = mln_string("loop cond do body end if flag then 'after_loop' fi");
    mln_expr_val_t *v = mln_expr_run(&exp, complex_loop_handler, NULL);
    if (v == NULL) { FAIL("run returned NULL"); return; }
    if (v->type != mln_expr_type_string) { FAIL("wrong type"); mln_expr_val_free(v); return; }
    mln_string_t expected = mln_string("after_loop");
    if (mln_string_strcmp(v->data.s, &expected)) { FAIL("wrong value"); mln_expr_val_free(v); return; }
    mln_expr_val_free(v);
    PASS();
}

/* Test 14: if before loop */
static void test_complex_if_then_loop(void)
{
    TEST("complex: if expression followed by loop");
    /* if flag then 'before' fi loop cond do body end
     * flag=true -> 'before', then loop 3 times -> last body=3
     */
    complex_counter = 0;
    complex_loop_max = 3;
    mln_string_t exp = mln_string("if flag then 'before' fi loop cond do body end");
    mln_expr_val_t *v = mln_expr_run(&exp, complex_loop_handler, NULL);
    if (v == NULL) { FAIL("run returned NULL"); return; }
    if (v->type != mln_expr_type_int) { FAIL("wrong type"); mln_expr_val_free(v); return; }
    if (v->data.i != 3) { FAIL("wrong value"); mln_expr_val_free(v); return; }
    mln_expr_val_free(v);
    PASS();
}

/* Test 15: Very long expression with 10+ mixed elements */
static void test_complex_very_long_mixed(void)
{
    TEST("complex: very long mixed expression (10+ elements)");
    /* 42 true 'hello' 0xff null 3.14 false 0777 "world\\n" add(x, y) cat('a', 'b') if flag then 'yes' fi
     * Last result is from 'if flag then yes fi' = "yes"
     */
    mln_string_t exp = mln_string(
        "42 true 'hello' 0xff null 3.14 false 0777 \"world\\n\" add(x, y) cat('a', 'b') if flag then 'yes' fi"
    );
    mln_expr_val_t *v = mln_expr_run(&exp, complex_handler, NULL);
    if (v == NULL) { FAIL("run returned NULL"); return; }
    if (v->type != mln_expr_type_string) { FAIL("wrong type"); mln_expr_val_free(v); return; }
    mln_string_t expected = mln_string("yes");
    if (mln_string_strcmp(v->data.s, &expected)) { FAIL("wrong value"); mln_expr_val_free(v); return; }
    mln_expr_val_free(v);
    PASS();
}

/* Test 16: if with function results as condition (true int) */
static void test_complex_if_func_condition(void)
{
    TEST("complex: if with function result as condition");
    /* if x then 'nonzero' else 'zero' fi
     * x=10 (int, truthy) -> 'nonzero'
     */
    mln_string_t exp = mln_string("if x then 'nonzero' else 'zero' fi");
    mln_expr_val_t *v = mln_expr_run(&exp, complex_handler, NULL);
    if (v == NULL) { FAIL("run returned NULL"); return; }
    if (v->type != mln_expr_type_string) { FAIL("wrong type"); mln_expr_val_free(v); return; }
    mln_string_t expected = mln_string("nonzero");
    if (mln_string_strcmp(v->data.s, &expected)) { FAIL("wrong value"); mln_expr_val_free(v); return; }
    mln_expr_val_free(v);

    /* if z then 'nonzero' else 'zero' fi   z=0 (falsy) */
    mln_string_t exp2 = mln_string("if z then 'nonzero' else 'zero' fi");
    v = mln_expr_run(&exp2, complex_handler, NULL);
    if (v == NULL) { FAIL("run returned NULL (2)"); return; }
    if (v->type != mln_expr_type_string) { FAIL("wrong type (2)"); mln_expr_val_free(v); return; }
    mln_string_t expected2 = mln_string("zero");
    if (mln_string_strcmp(v->data.s, &expected2)) { FAIL("wrong value (2)"); mln_expr_val_free(v); return; }
    mln_expr_val_free(v);
    PASS();
}

/* Test 17: Deep nested if with else chains */
static void test_complex_if_else_cascade(void)
{
    TEST("complex: if-else cascade (4 levels, alternating true/false)");
    /* if flag then
     *   if off then 'L2_wrong' else
     *     if flag then
     *       if off then 'L4_wrong' else add(mul(x, y), x) fi
     *     else 'L3_wrong' fi
     *   fi
     * else 'L1_wrong' fi
     * => add(mul(10,20), 10) = add(200, 10) = 210
     */
    mln_string_t exp = mln_string(
        "if flag then "
            "if off then 'L2_wrong' else "
                "if flag then "
                    "if off then 'L4_wrong' else add(mul(x, y), x) fi "
                "else 'L3_wrong' fi "
            "fi "
        "else 'L1_wrong' fi"
    );
    mln_expr_val_t *v = mln_expr_run(&exp, complex_handler, NULL);
    if (v == NULL) { FAIL("run returned NULL"); return; }
    if (v->type != mln_expr_type_int) { FAIL("wrong type"); mln_expr_val_free(v); return; }
    if (v->data.i != 210) { FAIL("wrong value"); mln_expr_val_free(v); return; }
    mln_expr_val_free(v);
    PASS();
}

/* Test 18: Loop with nested if + func in body */
static void test_complex_loop_if_func_ns(void)
{
    TEST("complex: loop with if + func in body");
    /* loop cond do
     *   if flag then body fi
     * end
     * loops 2 times, each time flag=true: body increments counter
     */
    complex_counter = 0;
    complex_loop_max = 2;
    mln_string_t exp = mln_string("loop cond do if flag then body fi end");
    mln_expr_val_t *v = mln_expr_run(&exp, complex_loop_handler, NULL);
    if (v == NULL) { FAIL("run returned NULL"); return; }
    if (v->type != mln_expr_type_int) { FAIL("wrong type"); mln_expr_val_free(v); return; }
    if (v->data.i != 2) { FAIL("wrong value"); mln_expr_val_free(v); return; }
    mln_expr_val_free(v);
    PASS();
}

/* Test 19: Multiple function + namespace + constants in one long expression */
static void test_complex_mixed_all_features(void)
{
    TEST("complex: all features in one expression");
    /* ns1:ns2:myvar  add(x, y) cat(upper(msg), ' ', tag, '!') 42 if flag then mul(x, x) else 0 fi
     * Sequence:
     *   ns1:ns2:myvar -> handler returns NS(ns1:ns2):myvar -> ret
     *   add(10, 20) -> 30 -> ret
     *   cat(upper("hello"), " ", "world", "!") -> cat("UPPER:hello", " ", "world", "!") -> "UPPER:hello world!" -> ret
     *   42 -> ret
     *   if flag then mul(10,10) else 0 fi -> 100 -> ret
     * Final result: 100
     */
    mln_string_t exp = mln_string(
        "ns1:ns2:myvar add(x, y) cat(upper(msg), ' ', tag, '!') 42 if flag then mul(x, x) else 0 fi"
    );
    mln_expr_val_t *v = mln_expr_run(&exp, complex_handler, NULL);
    if (v == NULL) { FAIL("run returned NULL"); return; }
    if (v->type != mln_expr_type_int) { FAIL("wrong type"); mln_expr_val_free(v); return; }
    if (v->data.i != 100) { FAIL("wrong value"); mln_expr_val_free(v); return; }
    mln_expr_val_free(v);
    PASS();
}

/* Test 20: Complex expression with all constant types and escapes */
static void test_complex_all_constants_expr(void)
{
    TEST("complex: expression with every constant type");
    /* 0 0xff 0777 3.14 true false null 'single' "double" "esc\\n\\t" 42
     * Last result: 42
     */
    mln_string_t exp = mln_string("0 0xff 0777 3.14 true false null 'single' \"double\" \"esc\\n\\t\" 42");
    mln_expr_val_t *v = mln_expr_run(&exp, complex_handler, NULL);
    if (v == NULL) { FAIL("run returned NULL"); return; }
    if (v->type != mln_expr_type_int) { FAIL("wrong type"); mln_expr_val_free(v); return; }
    if (v->data.i != 42) { FAIL("wrong value"); mln_expr_val_free(v); return; }
    mln_expr_val_free(v);
    PASS();
}

/* Test 21: Function with many args (>8 = beyond MLN_EXPR_DEFAULT_ARGS) */
static void test_complex_many_func_args(void)
{
    TEST("complex: function with >8 arguments (array growth)");
    /* cat('a','b','c','d','e','f','g','h','i','j') = "abcdefghij" */
    mln_string_t exp = mln_string("cat('a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j')");
    mln_expr_val_t *v = mln_expr_run(&exp, complex_handler, NULL);
    if (v == NULL) { FAIL("run returned NULL"); return; }
    if (v->type != mln_expr_type_string) { FAIL("wrong type"); mln_expr_val_free(v); return; }
    mln_string_t expected = mln_string("abcdefghij");
    if (mln_string_strcmp(v->data.s, &expected)) { FAIL("wrong value"); mln_expr_val_free(v); return; }
    mln_expr_val_free(v);
    PASS();
}

/* Test 22: if with string condition (truthy = non-empty) */
static void test_complex_if_string_condition(void)
{
    TEST("complex: if with string condition (truthy check)");
    /* if msg then 'truthy' else 'falsy' fi    msg="hello" (non-empty=true) -> 'truthy' */
    mln_string_t exp = mln_string("if msg then 'truthy' else 'falsy' fi");
    mln_expr_val_t *v = mln_expr_run(&exp, complex_handler, NULL);
    if (v == NULL) { FAIL("run returned NULL"); return; }
    if (v->type != mln_expr_type_string) { FAIL("wrong type"); mln_expr_val_free(v); return; }
    mln_string_t expected = mln_string("truthy");
    if (mln_string_strcmp(v->data.s, &expected)) { FAIL("wrong value"); mln_expr_val_free(v); return; }
    mln_expr_val_free(v);
    PASS();
}

/* Test 23: if with null condition (falsy) */
static void test_complex_if_null_condition(void)
{
    TEST("complex: if with null condition (falsy check)");
    /* if null then 'wrong' else 'correct' fi -> null is falsy -> 'correct' */
    mln_string_t exp = mln_string("if null then 'wrong' else 'correct' fi");
    mln_expr_val_t *v = mln_expr_run(&exp, complex_handler, NULL);
    if (v == NULL) { FAIL("run returned NULL"); return; }
    if (v->type != mln_expr_type_string) { FAIL("wrong type"); mln_expr_val_free(v); return; }
    mln_string_t expected = mln_string("correct");
    if (mln_string_strcmp(v->data.s, &expected)) { FAIL("wrong value"); mln_expr_val_free(v); return; }
    mln_expr_val_free(v);
    PASS();
}

/* Test 24: Very long expression with 20+ tokens */
static void test_complex_20_token_expr(void)
{
    TEST("complex: 20+ token expression with mixed features");
    /* cat('start', '_') add(x, mul(y, x)) if flag then cat(upper('test'), ':', tag) else 'fail' fi
     *   cat('start','_') -> "start_" -> ret
     *   add(10, mul(20,10)) = add(10, 200) = 210 -> ret
     *   if true then cat("UPPER:test", ":", "world") = "UPPER:test:world" -> ret
     * Final: "UPPER:test:world"
     */
    mln_string_t exp = mln_string(
        "cat('start', '_') add(x, mul(y, x)) if flag then cat(upper('test'), ':', tag) else 'fail' fi"
    );
    mln_expr_val_t *v = mln_expr_run(&exp, complex_handler, NULL);
    if (v == NULL) { FAIL("run returned NULL"); return; }
    if (v->type != mln_expr_type_string) { FAIL("wrong type"); mln_expr_val_free(v); return; }
    mln_string_t expected = mln_string("UPPER:test:world");
    if (mln_string_strcmp(v->data.s, &expected)) { FAIL("wrong value"); mln_expr_val_free(v); return; }
    mln_expr_val_free(v);
    PASS();
}

/* Test 25: Loop that runs 0 times followed by complex expression */
static void test_complex_zero_loop_then_expr(void)
{
    TEST("complex: zero-iteration loop then complex expression");
    complex_counter = 100;
    complex_loop_max = 3;
    mln_string_t exp = mln_string("loop cond do body end if flag then cat('after', '_', 'zero') fi");
    mln_expr_val_t *v = mln_expr_run(&exp, complex_loop_handler, NULL);
    if (v == NULL) { FAIL("run returned NULL"); return; }
    if (v->type != mln_expr_type_string) { FAIL("wrong type"); mln_expr_val_free(v); return; }
    mln_string_t expected = mln_string("after_zero");
    if (mln_string_strcmp(v->data.s, &expected)) { FAIL("wrong value"); mln_expr_val_free(v); return; }
    mln_expr_val_free(v);
    PASS();
}

/* =====================================================================
 * Complex expression benchmarks
 * ===================================================================== */

static void test_benchmark_complex_nested_if(void)
{
    TEST("benchmark: complex nested if-else with functions");
    mln_string_t exp = mln_string(
        "if flag then "
            "if off then 'wrong' else "
                "if flag then add(mul(x, y), add(x, y)) else 'wrong' fi "
            "fi "
        "else 'wrong' fi"
    );
    int iterations = 50000;
    struct timespec start, end;

    clock_gettime(CLOCK_MONOTONIC, &start);
    for (int i = 0; i < iterations; i++) {
        mln_expr_val_t *v = mln_expr_run(&exp, complex_handler, NULL);
        if (v == NULL) { FAIL("NULL during benchmark"); return; }
        mln_expr_val_free(v);
    }
    clock_gettime(CLOCK_MONOTONIC, &end);

    double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    double avg_us = (elapsed / iterations) * 1e6;
    printf("PASS (%d iters, avg %.3f us/op, %.0f ops/sec)\n", iterations, avg_us, iterations / elapsed);
    pass_count++;
}

static void test_benchmark_complex_long_expr(void)
{
    TEST("benchmark: long mixed expression (all features)");
    mln_string_t exp = mln_string(
        "ns1:ns2:myvar add(x, y) cat(upper(msg), ' ', tag, '!') 42 if flag then mul(x, x) else 0 fi"
    );
    int iterations = 50000;
    struct timespec start, end;

    clock_gettime(CLOCK_MONOTONIC, &start);
    for (int i = 0; i < iterations; i++) {
        mln_expr_val_t *v = mln_expr_run(&exp, complex_handler, NULL);
        if (v == NULL) { FAIL("NULL during benchmark"); return; }
        mln_expr_val_free(v);
    }
    clock_gettime(CLOCK_MONOTONIC, &end);

    double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    double avg_us = (elapsed / iterations) * 1e6;
    printf("PASS (%d iters, avg %.3f us/op, %.0f ops/sec)\n", iterations, avg_us, iterations / elapsed);
    pass_count++;
}

static void test_benchmark_complex_loop_if(void)
{
    TEST("benchmark: loop with if + body in body");
    int iterations = 10000;
    struct timespec start, end;

    clock_gettime(CLOCK_MONOTONIC, &start);
    for (int i = 0; i < iterations; i++) {
        complex_counter = 0;
        complex_loop_max = 3;
        mln_string_t exp = mln_string("loop cond do if flag then body fi end");
        mln_expr_val_t *v = mln_expr_run(&exp, complex_loop_handler, NULL);
        if (v == NULL) { FAIL("NULL during benchmark"); return; }
        mln_expr_val_free(v);
    }
    clock_gettime(CLOCK_MONOTONIC, &end);

    double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    double avg_us = (elapsed / iterations) * 1e6;
    printf("PASS (%d iters, avg %.3f us/op, %.0f ops/sec)\n", iterations, avg_us, iterations / elapsed);
    pass_count++;
}

static void test_benchmark_complex_deep_funcs(void)
{
    TEST("benchmark: deep func chain (4 levels, 8 args)");
    mln_string_t exp = mln_string(
        "cat(cat('a', 'b', 'c'), cat('d', 'e'), cat(upper('f'), cat('g', 'h')), 'i', 'j')"
    );
    int iterations = 50000;
    struct timespec start, end;

    clock_gettime(CLOCK_MONOTONIC, &start);
    for (int i = 0; i < iterations; i++) {
        mln_expr_val_t *v = mln_expr_run(&exp, complex_handler, NULL);
        if (v == NULL) { FAIL("NULL during benchmark"); return; }
        mln_expr_val_free(v);
    }
    clock_gettime(CLOCK_MONOTONIC, &end);

    double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    double avg_us = (elapsed / iterations) * 1e6;
    printf("PASS (%d iters, avg %.3f us/op, %.0f ops/sec)\n", iterations, avg_us, iterations / elapsed);
    pass_count++;
}

/* =====================================================================
 * Complex expression stability tests
 * ===================================================================== */

static void test_stability_complex(void)
{
    TEST("stability: 5K complex diverse expressions");

    mln_string_t exprs[] = {
        mln_string("if flag then cat(upper(msg), ' ', tag) else cat('no', 'pe') fi"),
        mln_string("add(add(add(x, y), mul(x, x)), add(y, y))"),
        mln_string("cat(cat('a', 'b', 'c'), cat('d', 'e'), 'f')"),
        mln_string("if off then 'skip' else add(mul(x, y), add(x, y)) fi"),
        mln_string("ns1:ns2:myvar add(x, y) if flag then mul(x, x) fi"),
        mln_string("if flag then if off then 'wrong' else add(x, y) fi else 'fail' fi"),
        mln_string("42 true 'hello' 0xff null 3.14 false 0777 if flag then 'yes' fi"),
        mln_string("cat('a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j')"),
        mln_string("if null then 'wrong' else if msg then 'correct' else 'fail' fi fi"),
        mln_string("cat(upper('test'), ':', tag) add(x, mul(y, x))"),
    };
    int n = sizeof(exprs) / sizeof(exprs[0]);

    for (int i = 0; i < 5000; i++) {
        mln_expr_val_t *v = mln_expr_run(&exprs[i % n], complex_handler, NULL);
        if (v == NULL) { FAIL("NULL during stability"); return; }
        mln_expr_val_free(v);
    }
    PASS();
}

static void test_stability_complex_loop(void)
{
    TEST("stability: 2K loop + if + func expressions");
    for (int i = 0; i < 2000; i++) {
        complex_counter = 0;
        complex_loop_max = 2;
        mln_string_t exp = mln_string("loop cond do if flag then body fi end");
        mln_expr_val_t *v = mln_expr_run(&exp, complex_loop_handler, NULL);
        if (v == NULL) { FAIL("NULL during stability"); return; }
        mln_expr_val_free(v);
    }
    PASS();
}

/* =====================================================================
 * Error handling tests (review feedback fixes)
 *
 * These verify that malformed expressions properly return NULL (error)
 * instead of silently succeeding with a partial/wrong parse.
 * ===================================================================== */

/* Test: unterminated string should return NULL (FT_ERR from scanner) */
static void test_err_unterminated_string(void)
{
    TEST("error: unterminated double-quoted string");
    mln_string_t exp = mln_string("\"hello");
    mln_expr_val_t *v = mln_expr_run(&exp, simple_var_handler, NULL);
    if (v != NULL) { FAIL("expected NULL for unterminated string"); mln_expr_val_free(v); return; }
    PASS();
}

/* Test: invalid escape sequence in string should return NULL */
static void test_err_invalid_escape(void)
{
    TEST("error: invalid escape sequence in string");
    mln_string_t exp = mln_string("\"hello\\z\"");
    mln_expr_val_t *v = mln_expr_run(&exp, simple_var_handler, NULL);
    if (v != NULL) { FAIL("expected NULL for invalid escape"); mln_expr_val_free(v); return; }
    PASS();
}

/* Test: 0x with no hex digits should return NULL (FT_ERR) */
static void test_err_hex_no_digits(void)
{
    TEST("error: 0x with no hex digits");
    mln_string_t exp = mln_string("0x");
    mln_expr_val_t *v = mln_expr_run(&exp, simple_var_handler, NULL);
    if (v != NULL) { FAIL("expected NULL for 0x without digits"); mln_expr_val_free(v); return; }
    PASS();
}

/* Test: 0x followed by non-hex char should return NULL */
static void test_err_hex_no_digits_eof(void)
{
    TEST("error: 0x followed by non-hex");
    mln_string_t exp = mln_string("0xzz");
    mln_expr_val_t *v = mln_expr_run(&exp, simple_var_handler, NULL);
    if (v != NULL) { FAIL("expected NULL for 0x with no hex digits"); mln_expr_val_free(v); return; }
    PASS();
}

/* Test: unknown/unhandled character (e.g. '@') should return NULL */
static void test_err_unknown_char(void)
{
    TEST("error: unknown character '@'");
    mln_string_t exp = mln_string("@");
    mln_expr_val_t *v = mln_expr_run(&exp, simple_var_handler, NULL);
    if (v != NULL) { FAIL("expected NULL for unknown char"); mln_expr_val_free(v); return; }
    PASS();
}

/* Test: stray 'then' keyword (not preceded by 'if') should return NULL */
static void test_err_stray_then(void)
{
    TEST("error: stray 'then' keyword");
    mln_string_t exp = mln_string("then 42");
    mln_expr_val_t *v = mln_expr_run(&exp, simple_var_handler, NULL);
    if (v != NULL) { FAIL("expected NULL for stray then"); mln_expr_val_free(v); return; }
    PASS();
}

/* Test: stray 'fi' keyword should return NULL */
static void test_err_stray_fi(void)
{
    TEST("error: stray 'fi' keyword");
    mln_string_t exp = mln_string("fi");
    mln_expr_val_t *v = mln_expr_run(&exp, simple_var_handler, NULL);
    if (v != NULL) { FAIL("expected NULL for stray fi"); mln_expr_val_free(v); return; }
    PASS();
}

/* Test: stray 'else' keyword should return NULL */
static void test_err_stray_else(void)
{
    TEST("error: stray 'else' keyword");
    mln_string_t exp = mln_string("else 'hello'");
    mln_expr_val_t *v = mln_expr_run(&exp, simple_var_handler, NULL);
    if (v != NULL) { FAIL("expected NULL for stray else"); mln_expr_val_free(v); return; }
    PASS();
}

/* Test: stray 'do' keyword should return NULL */
static void test_err_stray_do(void)
{
    TEST("error: stray 'do' keyword");
    mln_string_t exp = mln_string("do 42 end");
    mln_expr_val_t *v = mln_expr_run(&exp, simple_var_handler, NULL);
    if (v != NULL) { FAIL("expected NULL for stray do"); mln_expr_val_free(v); return; }
    PASS();
}

/* Test: stray 'end' keyword should return NULL */
static void test_err_stray_end(void)
{
    TEST("error: stray 'end' keyword");
    mln_string_t exp = mln_string("end");
    mln_expr_val_t *v = mln_expr_run(&exp, simple_var_handler, NULL);
    if (v != NULL) { FAIL("expected NULL for stray end"); mln_expr_val_free(v); return; }
    PASS();
}

/* =====================================================================
 * API compatibility tests (review feedback round 2)
 * ===================================================================== */

/* Test: leading colon should be skipped like lex-based parser */
static void test_leading_colon_variable(void)
{
    TEST("leading colon: ':x' parses like 'x'");
    mln_string_t exp = mln_string(":x");
    mln_expr_val_t *v = mln_expr_run(&exp, simple_var_handler, NULL);
    if (v == NULL) { FAIL("run returned NULL"); return; }
    if (v->type != mln_expr_type_string) { FAIL("wrong type"); mln_expr_val_free(v); return; }
    mln_string_t expected = mln_string("x");
    if (mln_string_strcmp(v->data.s, &expected)) { FAIL("wrong value"); mln_expr_val_free(v); return; }
    mln_expr_val_free(v);
    PASS();
}

/* Test: multiple leading colons should be skipped */
static void test_multiple_leading_colons(void)
{
    TEST("multiple leading colons: '::x' parses like 'x'");
    mln_string_t exp = mln_string("::x");
    mln_expr_val_t *v = mln_expr_run(&exp, simple_var_handler, NULL);
    if (v == NULL) { FAIL("run returned NULL"); return; }
    if (v->type != mln_expr_type_string) { FAIL("wrong type"); mln_expr_val_free(v); return; }
    mln_string_t expected = mln_string("x");
    if (mln_string_strcmp(v->data.s, &expected)) { FAIL("wrong value"); mln_expr_val_free(v); return; }
    mln_expr_val_free(v);
    PASS();
}

/* Test: leading colon before function call */
static void test_leading_colon_func(void)
{
    TEST("leading colon: ':concat(\"a\",\"b\")' works");
    mln_string_t exp = mln_string(":concat(\"a\",\"b\")");
    mln_expr_val_t *v = mln_expr_run(&exp, concat_handler, NULL);
    if (v == NULL) { FAIL("run returned NULL"); return; }
    if (v->type != mln_expr_type_string) { FAIL("wrong type"); mln_expr_val_free(v); return; }
    mln_string_t expected = mln_string("ab");
    if (mln_string_strcmp(v->data.s, &expected)) { FAIL("wrong value"); mln_expr_val_free(v); return; }
    mln_expr_val_free(v);
    PASS();
}

/* Test: colon before numeric constant */
static void test_leading_colon_number(void)
{
    TEST("leading colon: ':42' parses like '42'");
    mln_string_t exp = mln_string(":42");
    mln_expr_val_t *v = mln_expr_run(&exp, simple_var_handler, NULL);
    if (v == NULL) { FAIL("run returned NULL"); return; }
    if (v->type != mln_expr_type_int) { FAIL("wrong type"); mln_expr_val_free(v); return; }
    if (v->data.i != 42) { FAIL("wrong value"); mln_expr_val_free(v); return; }
    mln_expr_val_free(v);
    PASS();
}

/* Test: callback name parameter is safe for mln_string_ref (variable) */
static void test_cb_name_ref_variable(void)
{
    TEST("callback name ref safety: variable");
    mln_string_t exp = mln_string("myvar");
    mln_expr_val_t *v = mln_expr_run(&exp, ref_name_handler, NULL);
    if (v == NULL) { FAIL("run returned NULL"); return; }
    if (v->type != mln_expr_type_string) { FAIL("wrong type"); mln_expr_val_free(v); return; }
    mln_string_t expected = mln_string("myvar");
    if (mln_string_strcmp(v->data.s, &expected)) { FAIL("wrong value"); mln_expr_val_free(v); return; }
    mln_expr_val_free(v);
    PASS();
}

/* Test: callback name parameter is safe for mln_string_ref (function call) */
static void test_cb_name_ref_function(void)
{
    TEST("callback name ref safety: function call");
    mln_string_t exp = mln_string("myfunc(42)");
    mln_expr_val_t *v = mln_expr_run(&exp, ref_name_handler, NULL);
    if (v == NULL) { FAIL("run returned NULL"); return; }
    if (v->type != mln_expr_type_string) { FAIL("wrong type"); mln_expr_val_free(v); return; }
    mln_string_t expected = mln_string("myfunc");
    if (mln_string_strcmp(v->data.s, &expected)) { FAIL("wrong value"); mln_expr_val_free(v); return; }
    mln_expr_val_free(v);
    PASS();
}

/* --- Oversized literal rejection tests --- */
static void test_err_oversized_int(void)
{
    TEST("error: oversized integer literal (>=32 chars)");
    /* 32 digit number exceeds the 32-byte buffer */
    mln_string_t exp = mln_string("12345678901234567890123456789012");
    mln_expr_val_t *v = mln_expr_run(&exp, simple_var_handler, NULL);
    if (v != NULL) { FAIL("expected NULL for oversized int"); mln_expr_val_free(v); return; }
    PASS();
}

static void test_err_oversized_real(void)
{
    TEST("error: oversized real literal (>=64 chars)");
    /* 64+ char real exceeds the 64-byte buffer */
    mln_string_t exp = mln_string("1.234567890123456789012345678901234567890123456789012345678901234");
    mln_expr_val_t *v = mln_expr_run(&exp, simple_var_handler, NULL);
    if (v != NULL) { FAIL("expected NULL for oversized real"); mln_expr_val_free(v); return; }
    PASS();
}

static void test_err_oversized_hex(void)
{
    TEST("error: oversized hex literal (>=32 chars)");
    /* 0x + 31 hex digits = 33 chars, exceeds 32-byte buffer */
    mln_string_t exp = mln_string("0x1234567890abcdef1234567890abcde");
    mln_expr_val_t *v = mln_expr_run(&exp, simple_var_handler, NULL);
    if (v != NULL) { FAIL("expected NULL for oversized hex"); mln_expr_val_free(v); return; }
    PASS();
}

/* =====================================================================
 * Octal/hex consistency fixes (review round 6)
 * ===================================================================== */

/* Test: "09" — non-octal digit after leading 0, should be invalid octal */
static void test_err_invalid_octal_09(void)
{
    TEST("error: invalid octal '09'");
    mln_string_t exp = mln_string("09");
    mln_expr_val_t *v = mln_expr_run(&exp, simple_var_handler, NULL);
    if (v != NULL) { FAIL("expected NULL for invalid octal 09"); mln_expr_val_free(v); return; }
    PASS();
}

/* Test: "08" — non-octal digit after leading 0, should be invalid octal */
static void test_err_invalid_octal_08(void)
{
    TEST("error: invalid octal '08'");
    mln_string_t exp = mln_string("08");
    mln_expr_val_t *v = mln_expr_run(&exp, simple_var_handler, NULL);
    if (v != NULL) { FAIL("expected NULL for invalid octal 08"); mln_expr_val_free(v); return; }
    PASS();
}

/* Test: "09.5" — non-octal digit after leading 0, but real due to '.' */
static void test_invalid_octal_real_fallback(void)
{
    TEST("real fallback: '09.5' is valid real despite invalid octal prefix");
    mln_string_t exp = mln_string("09.5");
    mln_expr_val_t *v = mln_expr_run(&exp, simple_var_handler, NULL);
    if (v == NULL) { FAIL("run returned NULL"); return; }
    if (v->type != mln_expr_type_real) { FAIL("wrong type"); mln_expr_val_free(v); return; }
    if (v->data.r < 9.4 || v->data.r > 9.6) { FAIL("wrong value"); mln_expr_val_free(v); return; }
    mln_expr_val_free(v);
    PASS();
}

/* Test: "0XAB" — uppercase X is not a hex prefix, so not parsed as hex */
static void test_err_uppercase_hex_prefix(void)
{
    TEST("error: uppercase '0X' not recognized as hex prefix");
    mln_string_t exp = mln_string("0XAB");
    mln_expr_val_t *v = mln_expr_run(&exp, simple_var_handler, NULL);
    /* 0XAB is parsed as 0 (dec) then XAB (identifier); last value is string "XAB" */
    if (v == NULL) { FAIL("run returned NULL"); return; }
    if (v->type != mln_expr_type_string) { FAIL("expected string type for XAB identifier"); mln_expr_val_free(v); return; }
    mln_expr_val_free(v);
    PASS();
}

/* Test: 'if true x fi' — missing 'then', should error (not leak tok) */
static void test_if_missing_then(void)
{
    TEST("error: if missing 'then' keyword");
    mln_string_t exp = mln_string("if true x fi");
    mln_expr_val_t *v = mln_expr_run(&exp, simple_var_handler, NULL);
    if (v != NULL) { FAIL("expected NULL for if missing then"); mln_expr_val_free(v); return; }
    PASS();
}

/* Test: if with heap-backed string token where 'then' expected — no leak */
static void test_if_missing_then_string_tok(void)
{
    TEST("error: if missing 'then' with string token (heap free)");
    mln_string_t exp = mln_string("if true 'hello' fi");
    mln_expr_val_t *v = mln_expr_run(&exp, simple_var_handler, NULL);
    if (v != NULL) { FAIL("expected NULL for if missing then (string tok)"); mln_expr_val_free(v); return; }
    PASS();
}

/* Test: 'loop true x end' — missing 'do', should error (not leak tok) */
static void test_loop_missing_do(void)
{
    TEST("error: loop missing 'do' keyword");
    mln_string_t exp = mln_string("loop true x end");
    mln_expr_val_t *v = mln_expr_run(&exp, simple_var_handler, NULL);
    if (v != NULL) { FAIL("expected NULL for loop missing do"); mln_expr_val_free(v); return; }
    PASS();
}

/* Test: loop with heap-backed string token where 'do' expected — no leak */
static void test_loop_missing_do_string_tok(void)
{
    TEST("error: loop missing 'do' with string token (heap free)");
    mln_string_t exp = mln_string("loop true 'world' end");
    mln_expr_val_t *v = mln_expr_run(&exp, simple_var_handler, NULL);
    if (v != NULL) { FAIL("expected NULL for loop missing do (string tok)"); mln_expr_val_free(v); return; }
    PASS();
}

int main(void)
{
    printf("=== mln_expr tests ===\n");

    /* Constants */
    test_integer_dec();
    test_integer_hex();
    test_integer_hex_upper();
    test_integer_oct();
    test_integer_zero();
    test_real_constant();
    test_real_zero();
    test_string_dblq();
    test_string_sglq();
    test_string_escape();
    test_all_escape_chars();
    test_sglq_escape();
    test_string_empty();
    test_bool_true();
    test_bool_false();
    test_null_keyword();

    /* Variables */
    test_simple_variable();
    test_underscore_variable();
    test_namespace();
    test_namespace_func();

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
    test_if_no_else_false();
    test_nested_if();
    test_nested_if_false_outer();
    test_if_with_func();

    /* Loop */
    test_loop();
    test_loop_false_condition();

    /* Value API */
    test_val_new_free();
    test_val_dup();
    test_val_dup_all_types();
    test_val_copy();
    test_udata_type();
    test_udata_with_free();
    test_empty_expression();
    test_whitespace_only();

    /* Performance */
    test_benchmark();
    test_benchmark_numbers();
    test_benchmark_if_loop();

    /* Stability */
    test_stability();
    test_stability_if_loop();
    test_stability_all_types();

    /* Complex combination tests */
    test_complex_if_with_nested_funcs();
    test_complex_if_false_nested_funcs();
    test_complex_triple_nested_if();
    test_complex_multi_expr_with_if();
    test_complex_loop_with_if_body();
    test_complex_loop_with_func_body();
    test_complex_deep_func_chain();
    test_complex_string_escape_in_if();
    test_complex_ns_func_if();
    test_complex_sequential_ifs();
    test_complex_if_with_all_constants();
    test_complex_long_func_chain();
    test_complex_loop_then_if();
    test_complex_if_then_loop();
    test_complex_very_long_mixed();
    test_complex_if_func_condition();
    test_complex_if_else_cascade();
    test_complex_loop_if_func_ns();
    test_complex_mixed_all_features();
    test_complex_all_constants_expr();
    test_complex_many_func_args();
    test_complex_if_string_condition();
    test_complex_if_null_condition();
    test_complex_20_token_expr();
    test_complex_zero_loop_then_expr();

    /* Complex benchmarks (with avg processing time) */
    test_benchmark_complex_nested_if();
    test_benchmark_complex_long_expr();
    test_benchmark_complex_loop_if();
    test_benchmark_complex_deep_funcs();

    /* Complex stability */
    test_stability_complex();
    test_stability_complex_loop();

    /* Error handling tests (review feedback fixes) */
    test_err_unterminated_string();
    test_err_invalid_escape();
    test_err_hex_no_digits();
    test_err_hex_no_digits_eof();
    test_err_unknown_char();
    test_err_stray_then();
    test_err_stray_fi();
    test_err_stray_else();
    test_err_stray_do();
    test_err_stray_end();

    /* API compatibility tests (review feedback round 2) */
    test_leading_colon_variable();
    test_multiple_leading_colons();
    test_leading_colon_func();
    test_leading_colon_number();
    test_cb_name_ref_variable();
    test_cb_name_ref_function();

    /* Robustness tests (review feedback round 3) */
    test_err_oversized_int();
    test_err_oversized_real();
    test_err_oversized_hex();

    /* Token leak fixes in if/loop early returns (review feedback round 5) */
    test_if_missing_then();
    test_if_missing_then_string_tok();
    test_loop_missing_do();
    test_loop_missing_do_string_tok();

    /* udata ownership semantics tests (review feedback round 4) */
    test_udata_dup_non_mutating();
    test_udata_copy_non_mutating();

    /* Octal/hex consistency tests (review feedback round 6) */
    test_err_invalid_octal_09();
    test_err_invalid_octal_08();
    test_invalid_octal_real_fallback();
    test_err_uppercase_hex_prefix();

    printf("\n=== Results: %d/%d passed ===\n", pass_count, test_count);
    return (pass_count == test_count) ? 0 : 1;
}
