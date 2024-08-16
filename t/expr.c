#include "mln_expr.h"
#include "mln_log.h"
#include <stdio.h>

static mln_expr_val_t *var_expr_handler(mln_string_t *namespace, mln_string_t *name, int is_func, mln_array_t *args, void *data)
{
    mln_string_t *s;
    mln_expr_val_t *ret;
    mln_string_t anon = mln_string("anonymous namespace");

    printf("%p %p %p %p\n", namespace, name, args, data);
    if (is_func)
        mln_log(none, "%S %S %d %U %X\n", namespace? namespace: &anon, name, is_func, args->nelts, data);
    else
        mln_log(none, "%S %S %d %X\n", namespace? namespace: &anon, name, is_func, data);
    if ((s = mln_string_dup(name)) == NULL) return NULL;
    ret = mln_expr_val_new(mln_expr_type_string, s, NULL);
    mln_string_free(s);
    return ret;
}

static mln_expr_val_t *func_expr_handler(mln_string_t *namespace, mln_string_t *name, int is_func, mln_array_t *args, void *data)
{
    mln_expr_val_t *v, *p;
    int i;
    mln_string_t *s1 = NULL, *s2, *s3;
    mln_string_t anon = mln_string("anonymous namespace");

    if (is_func) {
        mln_log(none, "%S %S %d %U %X\n", namespace? namespace: &anon, name, is_func, args->nelts, data);
    } else {
        mln_log(none, "%S %S %d %X\n", namespace? namespace: &anon, name, is_func, data);
        return mln_expr_val_new(mln_expr_type_string, name, NULL);
    }

    for (i = 0, v = p = mln_array_elts(args); i < mln_array_nelts(args); v = p + (++i)) {
        if (s1 == NULL) {
            s1 = mln_string_ref(v->data.s);
            continue;
        }
        s2 = v->data.s;
        s3 = mln_string_strcat(s1, s2);
        mln_string_free(s1);
        s1 = s3;
    }

    v = mln_expr_val_new(mln_expr_type_string, s1, NULL);
    mln_string_free(s1);

    return v;
}

int main(void)
{
    mln_string_t var_exp = mln_string(":aaa (a:b:c:bbb)");
    mln_string_t func_exp = mln_string("abc:def:concat('abc', concat(aaa, 'bbb')) ccc concat('eee', concat(bbb, 'fff'))");

    mln_expr_val_t *v;

    v = mln_expr_run(&var_exp, var_expr_handler, NULL);
    if (v == NULL) {
        mln_log(error, "run failed\n");
        return -1;
    }
    mln_log(debug, "%d %S\n", v->type, v->data.s);
    mln_expr_val_free(v);

    v = mln_expr_run(&func_exp, func_expr_handler, NULL);
    if (v == NULL) {
        mln_log(error, "run failed\n");
        return -1;
    }
    mln_log(debug, "%d %S\n", v->type, v->data.s);
    mln_expr_val_free(v);

    return 0;
}

