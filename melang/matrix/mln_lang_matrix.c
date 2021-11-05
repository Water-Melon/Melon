
/*
 * Copyright (C) Niklaus F.Schen.
 */
#include "matrix/mln_lang_matrix.h"
#include "mln_matrix.h"
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#ifdef __DEBUG__
#include <assert.h>
#define ASSERT(x) assert(x)
#else
#define ASSERT(x);
#endif

static int mln_lang_matrix_mul_handler(mln_lang_ctx_t *ctx);
static int mln_lang_matrix_inv_handler(mln_lang_ctx_t *ctx);
static mln_lang_var_t *mln_lang_matrix_mul_process(mln_lang_ctx_t *ctx);
static mln_lang_var_t *mln_lang_matrix_inv_process(mln_lang_ctx_t *ctx);
static mln_matrix_t *
mln_lang_array2matrix(mln_lang_ctx_t *ctx, mln_lang_array_t *array);
static mln_lang_var_t *mln_lang_matrix2array_exp(mln_lang_ctx_t *ctx, mln_matrix_t *m);

int mln_lang_matrix(mln_lang_ctx_t *ctx)
{
    if (mln_lang_matrix_mul_handler(ctx) < 0) return -1;
    if (mln_lang_matrix_inv_handler(ctx) < 0) return -1;
    return 0;
}

static int mln_lang_matrix_mul_handler(mln_lang_ctx_t *ctx)
{
    mln_lang_val_t *val;
    mln_lang_var_t *var;
    mln_lang_func_detail_t *func;
    mln_string_t funcname = mln_string("mln_matrix_mul");
    mln_string_t v1 = mln_string("a1"), v2 = mln_string("a2");
    if ((func = mln_lang_func_detail_new(ctx, M_FUNC_INTERNAL, mln_lang_matrix_mul_process, NULL, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if ((val = mln_lang_val_new(ctx, M_LANG_VAL_TYPE_NIL, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_func_detail_free(func);
        return -1;
    }
    if ((var = mln_lang_var_new(ctx, &v1, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_val_free(val);
        mln_lang_func_detail_free(func);
        return -1;
    }
    mln_lang_var_chain_add(&(func->args_head), &(func->args_tail), var);
    ++func->nargs;
    if ((val = mln_lang_val_new(ctx, M_LANG_VAL_TYPE_NIL, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_func_detail_free(func);
        return -1;
    }
    if ((var = mln_lang_var_new(ctx, &v2, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_val_free(val);
        mln_lang_func_detail_free(func);
        return -1;
    }
    mln_lang_var_chain_add(&(func->args_head), &(func->args_tail), var);
    ++func->nargs;
    if ((val = mln_lang_val_new(ctx, M_LANG_VAL_TYPE_FUNC, func)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_func_detail_free(func);
        return -1;
    }
    if ((var = mln_lang_var_new(ctx, &funcname, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_val_free(val);
        return -1;
    }
    if (mln_lang_symbol_node_join(ctx, M_LANG_SYMBOL_VAR, var) < 0) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_var_free(var);
        return -1;
    }
    return 0;
}

static mln_lang_var_t *mln_lang_matrix_mul_process(mln_lang_ctx_t *ctx)
{
    int err;
    mln_lang_val_t *val;
    mln_lang_var_t *ret_var = NULL;
    mln_string_t v1 = mln_string("a1"), v2 = mln_string("a2");
    mln_lang_symbol_node_t *sym;
    mln_lang_array_t *a1, *a2;
    mln_matrix_t *m1, *m2, *mres;

    if ((sym = mln_lang_symbol_node_search(ctx, &v1, 1)) == NULL) {
        ASSERT(0);
        mln_lang_errmsg(ctx, "Argument missing.");
        return NULL;
    }
    if (sym->type != M_LANG_SYMBOL_VAR || \
        mln_lang_var_val_type_get(sym->data.var) != M_LANG_VAL_TYPE_ARRAY)
    {
        mln_lang_errmsg(ctx, "Invalid type of argument 1.");
        return NULL;
    }
    val = sym->data.var->val;
    if ((a1 = val->data.array) == NULL) {
        mln_lang_errmsg(ctx, "Invalid argument 1.");
        return NULL;
    }

    if ((sym = mln_lang_symbol_node_search(ctx, &v2, 1)) == NULL) {
        ASSERT(0);
        mln_lang_errmsg(ctx, "Argument missing.");
        return NULL;
    }
    if (sym->type != M_LANG_SYMBOL_VAR || \
        mln_lang_var_val_type_get(sym->data.var) != M_LANG_VAL_TYPE_ARRAY)
    {
        mln_lang_errmsg(ctx, "Invalid type of argument 2.");
        return NULL;
    }
    val = sym->data.var->val;
    if ((a2 = val->data.array) == NULL) {
        mln_lang_errmsg(ctx, "Invalid argument 2.");
        return NULL;
    }

    if ((m1 = mln_lang_array2matrix(ctx, a1)) == NULL) {
        return NULL;
    }
    if ((m2 = mln_lang_array2matrix(ctx, a2)) == NULL) {
        mln_matrix_free(m1);
        return NULL;
    }
    mres = mln_matrix_mul(m1, m2);
    err = errno;
    mln_matrix_free(m1);
    mln_matrix_free(m2);
    if (mres == NULL) {
        if (err == EINVAL) {
            if ((ret_var = mln_lang_var_create_false(ctx, NULL)) == NULL) {
                mln_lang_errmsg(ctx, "No memory.");
                return NULL;
            }
        } else {
            mln_lang_errmsg(ctx, "No memory.");
            return NULL;
        }
    } else {
        ret_var = mln_lang_matrix2array_exp(ctx, mres);
        mln_matrix_free(mres);
    }
    return ret_var;
}

static int mln_lang_matrix_inv_handler(mln_lang_ctx_t *ctx)
{
    mln_lang_val_t *val;
    mln_lang_var_t *var;
    mln_lang_func_detail_t *func;
    mln_string_t funcname = mln_string("mln_matrix_inv");
    mln_string_t v1 = mln_string("array");
    if ((func = mln_lang_func_detail_new(ctx, M_FUNC_INTERNAL, mln_lang_matrix_inv_process, NULL, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if ((val = mln_lang_val_new(ctx, M_LANG_VAL_TYPE_NIL, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_func_detail_free(func);
        return -1;
    }
    if ((var = mln_lang_var_new(ctx, &v1, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_val_free(val);
        mln_lang_func_detail_free(func);
        return -1;
    }
    mln_lang_var_chain_add(&(func->args_head), &(func->args_tail), var);
    ++func->nargs;
    if ((val = mln_lang_val_new(ctx, M_LANG_VAL_TYPE_FUNC, func)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_func_detail_free(func);
        return -1;
    }
    if ((var = mln_lang_var_new(ctx, &funcname, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_val_free(val);
        return -1;
    }
    if (mln_lang_symbol_node_join(ctx, M_LANG_SYMBOL_VAR, var) < 0) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_var_free(var);
        return -1;
    }
    return 0;
}

static mln_lang_var_t *mln_lang_matrix_inv_process(mln_lang_ctx_t *ctx)
{
    int err;
    mln_lang_val_t *val;
    mln_lang_var_t *ret_var;
    mln_string_t v1 = mln_string("array");
    mln_lang_symbol_node_t *sym;
    mln_lang_array_t *a;
    mln_matrix_t *m, *mres;

    if ((sym = mln_lang_symbol_node_search(ctx, &v1, 1)) == NULL) {
        ASSERT(0);
        mln_lang_errmsg(ctx, "Argument missing.");
        return NULL;
    }
    if (sym->type != M_LANG_SYMBOL_VAR || \
        mln_lang_var_val_type_get(sym->data.var) != M_LANG_VAL_TYPE_ARRAY)
    {
        mln_lang_errmsg(ctx, "Invalid type of argument.");
        return NULL;
    }
    val = sym->data.var->val;
    if ((a = val->data.array) == NULL) {
        mln_lang_errmsg(ctx, "Invalid argument.");
        return NULL;
    }

    if ((m = mln_lang_array2matrix(ctx, a)) == NULL) {
        return NULL;
    }
    mres = mln_matrix_inverse(m);
    err = errno;
    mln_matrix_free(m);
    if (mres == NULL) {
        if (err == EINVAL) {
            if ((ret_var = mln_lang_var_create_false(ctx, NULL)) == NULL) {
                mln_lang_errmsg(ctx, "No memory.");
                return NULL;
            }
        } else {
            mln_lang_errmsg(ctx, "No memory.");
            return NULL;
        }
    } else {
        ret_var = mln_lang_matrix2array_exp(ctx, mres);
        mln_matrix_free(mres);
    }
    return ret_var;
}

/*
 * array['row'] = integer; array['col'] = integer; array['data'] = array;
 */
static mln_matrix_t *
mln_lang_array2matrix(mln_lang_ctx_t *ctx, mln_lang_array_t *array)
{
    mln_size_t i, n;
    mln_s32_t type;
    mln_lang_var_t *array_val, kvar;
    mln_lang_val_t kval;
    mln_string_t r = mln_string("row");
    mln_string_t c = mln_string("col");
    mln_string_t d = mln_string("data");
    double *data, *p;
    mln_size_t row, col;
    mln_lang_array_t *darray;
    mln_matrix_t *m;

    kvar.type = M_LANG_VAR_NORMAL;
    kvar.name = NULL;
    kvar.val = &kval;
    kvar.in_set = NULL;
    kvar.prev = kvar.next = NULL;
    kval.data.s = &r;
    kval.type = M_LANG_VAL_TYPE_STRING;
    kval.ref = 1;
    if ((array_val = mln_lang_array_get(ctx, array, &kvar)) == NULL) {
        return NULL;
    }
    if (mln_lang_var_val_type_get(array_val) != M_LANG_VAL_TYPE_INT) {
        mln_lang_errmsg(ctx, "Invalid argument.");
        return NULL;
    }
    row = array_val->val->data.i;

    kvar.type = M_LANG_VAR_NORMAL;
    kvar.name = NULL;
    kvar.val = &kval;
    kvar.in_set = NULL;
    kvar.prev = kvar.next = NULL;
    kval.data.s = &c;
    kval.type = M_LANG_VAL_TYPE_STRING;
    kval.ref = 1;
    if ((array_val = mln_lang_array_get(ctx, array, &kvar)) == NULL) {
        return NULL;
    }
    if (mln_lang_var_val_type_get(array_val) != M_LANG_VAL_TYPE_INT) {
        mln_lang_errmsg(ctx, "Invalid argument.");
        return NULL;
    }
    col = array_val->val->data.i;

    kvar.type = M_LANG_VAR_NORMAL;
    kvar.name = NULL;
    kvar.val = &kval;
    kvar.in_set = NULL;
    kvar.prev = kvar.next = NULL;
    kval.data.s = &d;
    kval.type = M_LANG_VAL_TYPE_STRING;
    kval.ref = 1;
    if ((array_val = mln_lang_array_get(ctx, array, &kvar)) == NULL) {
        return NULL;
    }
    if (mln_lang_var_val_type_get(array_val) != M_LANG_VAL_TYPE_ARRAY) {
        mln_lang_errmsg(ctx, "Invalid argument.");
        return NULL;
    }
    if ((darray = array_val->val->data.array) == NULL || \
        darray->elems_index->nr_node != row*col)
    {
        mln_lang_errmsg(ctx, "Invalid argument.");
        return NULL;
    }

    if ((data = (double *)malloc(row*col*sizeof(double))) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return NULL;
    }

    for (p = data, n = darray->elems_index->nr_node, i = 0; i < n; ++i) {
        kvar.type = M_LANG_VAR_NORMAL;
        kvar.name = NULL;
        kvar.val = &kval;
        kvar.in_set = NULL;
        kvar.prev = kvar.next = NULL;
        kval.data.i = i;
        kval.type = M_LANG_VAL_TYPE_INT;
        kval.ref = 1;
        if ((array_val = mln_lang_array_get(ctx, darray, &kvar)) == NULL) {
            free(data);
            return NULL;
        }
        type = mln_lang_var_val_type_get(array_val);
        if (type == M_LANG_VAL_TYPE_INT) {
            *p++ = array_val->val->data.i;
        } else if (type == M_LANG_VAL_TYPE_REAL) {
            *p++ = array_val->val->data.f;
        } else {
            mln_lang_errmsg(ctx, "Invalid argument.");
            free(data);
            return NULL;
        }
    }
    if ((m = mln_matrix_new(row, col, data, 0)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        free(data);
        return NULL;
    }
    return m;
}

static mln_lang_var_t *mln_lang_matrix2array_exp(mln_lang_ctx_t *ctx, mln_matrix_t *m)
{
    double *p, *pend;
    mln_lang_array_t *array, *darray;
    mln_lang_var_t *ret_var;
    mln_lang_var_t *array_val, var;
    mln_lang_val_t val;
    mln_string_t r = mln_string("row");
    mln_string_t c = mln_string("col");
    mln_string_t d = mln_string("data");

    if ((ret_var = mln_lang_var_create_array(ctx, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return NULL;
    }
    array = ret_var->val->data.array;

    var.type = M_LANG_VAR_NORMAL;
    var.name = NULL;
    var.val = &val;
    var.in_set = NULL;
    var.prev = var.next = NULL;
    val.data.s = &r;
    val.type = M_LANG_VAL_TYPE_STRING;
    val.ref = 1;
    if ((array_val = mln_lang_array_get(ctx, array, &var)) == NULL) {
        mln_lang_var_free(ret_var);
        return NULL;
    }
    var.type = M_LANG_VAR_NORMAL;
    var.name = NULL;
    var.val = &val;
    var.in_set = NULL;
    var.prev = var.next = NULL;
    val.data.i = m->row;
    val.type = M_LANG_VAL_TYPE_INT;
    val.ref = 1;
    if (mln_lang_var_value_set(ctx, array_val, &var) < 0) {
        mln_lang_var_free(ret_var);
        return NULL;
    }

    var.type = M_LANG_VAR_NORMAL;
    var.name = NULL;
    var.val = &val;
    var.in_set = NULL;
    var.prev = var.next = NULL;
    val.data.s = &c;
    val.type = M_LANG_VAL_TYPE_STRING;
    val.ref = 1;
    if ((array_val = mln_lang_array_get(ctx, array, &var)) == NULL) {
        mln_lang_var_free(ret_var);
        return NULL;
    }
    var.type = M_LANG_VAR_NORMAL;
    var.name = NULL;
    var.val = &val;
    var.in_set = NULL;
    var.prev = var.next = NULL;
    val.data.i = m->col;
    val.type = M_LANG_VAL_TYPE_INT;
    val.ref = 1;
    if (mln_lang_var_value_set(ctx, array_val, &var) < 0) {
        mln_lang_var_free(ret_var);
        return NULL;
    }

    if ((darray = mln_lang_array_new(ctx)) == NULL) {
        mln_lang_var_free(ret_var);
        mln_lang_errmsg(ctx, "No memory.");
        return NULL;
    }

    for (p = m->data, pend = m->data+m->row*m->col; p < pend; ++p) {
        if ((array_val = mln_lang_array_get(ctx, darray, NULL)) == NULL) {
            mln_lang_array_free(darray);
            mln_lang_var_free(ret_var);
            return NULL;
        }
        var.type = M_LANG_VAR_NORMAL;
        var.name = NULL;
        var.val = &val;
        var.in_set = NULL;
        var.prev = var.next = NULL;
        val.data.f = *p;
        val.type = M_LANG_VAL_TYPE_REAL;
        val.ref = 1;
        if (mln_lang_var_value_set(ctx, array_val, &var) < 0) {
            mln_lang_array_free(darray);
            mln_lang_var_free(ret_var);
            return NULL;
        }
    }

    var.type = M_LANG_VAR_NORMAL;
    var.name = NULL;
    var.val = &val;
    var.in_set = NULL;
    var.prev = var.next = NULL;
    val.data.s = &d;
    val.type = M_LANG_VAL_TYPE_STRING;
    val.ref = 1;
    if ((array_val = mln_lang_array_get(ctx, array, &var)) == NULL) {
        mln_lang_array_free(darray);
        mln_lang_var_free(ret_var);
        return NULL;
    }
    var.type = M_LANG_VAR_NORMAL;
    var.name = NULL;
    var.val = &val;
    var.in_set = NULL;
    var.prev = var.next = NULL;
    val.data.array = darray;
    val.type = M_LANG_VAL_TYPE_ARRAY;
    val.ref = 1;
    if (mln_lang_var_value_set(ctx, array_val, &var) < 0) {
        mln_lang_array_free(darray);
        mln_lang_var_free(ret_var);
        return NULL;
    }
    return ret_var;
}

