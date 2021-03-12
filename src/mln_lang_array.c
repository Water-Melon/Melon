
/*
 * Copyright (C) Niklaus F.Schen.
 */
#include "mln_lang_array.h"
#include <stdio.h>

#ifdef __DEBUG__
#include <assert.h>
#define ASSERT(x) assert(x)
#else
#define ASSERT(x);
#endif

static int
mln_lang_array_assign(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_array_pluseq(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_array_equal(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_array_nonequal(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_array_plus(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_array_index(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_array_not(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);

mln_lang_method_t mln_lang_array_oprs = {
    mln_lang_array_assign,
    mln_lang_array_pluseq,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    mln_lang_array_equal,
    mln_lang_array_nonequal,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    mln_lang_array_plus,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    mln_lang_array_index,
    NULL,
    NULL,
    NULL,
    mln_lang_array_not,
    NULL,
    NULL
};

static int
mln_lang_array_assign(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2)
{
    if (mln_lang_var_setValue(ctx, op1, op2) < 0) {
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    *ret = mln_lang_var_ref(op1);
    return 0;
}

static int
mln_lang_array_equal(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2)
{
    mln_s32_t type = mln_lang_var_getValType(op2);
    if (type == mln_lang_var_getValType(op1) && \
        mln_lang_var_getVal(op1)->data.array == mln_lang_var_getVal(op2)->data.array)
    {
        if ((*ret = mln_lang_var_createTmpTrue(ctx, NULL)) == NULL) {
            mln_lang_errmsg(ctx, "No memory.");
            return -1;
        }
    } else {
        if ((*ret = mln_lang_var_createTmpFalse(ctx, NULL)) == NULL) {
            mln_lang_errmsg(ctx, "No memory.");
            return -1;
        }
    }
    return 0;
}

static int
mln_lang_array_pluseq(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2)
{
    mln_s32_t type = mln_lang_var_getValType(op1);
    if (type != M_LANG_VAL_TYPE_STRING) {
        mln_lang_errmsg(ctx, "Operation Not support.");
        return -1;
    }
    mln_lang_method_t *method = mln_lang_methods[type];
    if (method == NULL) {
        mln_lang_errmsg(ctx, "Operation Not support.");
        return -1;
    }
    mln_lang_op handler = method->pluseq_handler;
    if (handler == NULL) {
        mln_lang_errmsg(ctx, "Operation Not support.");
        return -1;
    }
    return handler(ctx, ret, op1, op2);
}

static int
mln_lang_array_nonequal(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2)
{
    mln_s32_t type = mln_lang_var_getValType(op2);
    if (type != mln_lang_var_getValType(op1) || \
        mln_lang_var_getVal(op1)->data.array != mln_lang_var_getVal(op2)->data.array)
    {
        if ((*ret = mln_lang_var_createTmpTrue(ctx, NULL)) == NULL) {
            mln_lang_errmsg(ctx, "No memory.");
            return -1;
        }
    } else {
        if ((*ret = mln_lang_var_createTmpFalse(ctx, NULL)) == NULL) {
            mln_lang_errmsg(ctx, "No memory.");
            return -1;
        }
    }
    return 0;
}

static int
mln_lang_array_plus(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2)
{
    if (mln_lang_var_getValType(op2) != M_LANG_VAL_TYPE_STRING) {
        mln_lang_errmsg(ctx, "Operation Not support.");
        return -1;
    }
    mln_lang_method_t *method = mln_lang_methods[M_LANG_VAL_TYPE_STRING];
    if (method == NULL) {
        mln_lang_errmsg(ctx, "Operation Not support.");
        return -1;
    }
    mln_lang_op handler = method->plus_handler;
    if (handler == NULL) {
        mln_lang_errmsg(ctx, "Operation Not support.");
        return -1;
    }
    return handler(ctx, ret, op1, op2);
}
#include <stdlib.h>
static int
mln_lang_array_index(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2)
{
    mln_lang_array_t *array = mln_lang_var_getVal(op1)->data.array;
    mln_lang_var_t *rv;
    mln_s64_t save = ~((mln_s64_t)0);
    if (mln_lang_var_getValType(op2) == M_LANG_VAL_TYPE_INT && mln_lang_var_getVal(op2)->data.i < 0) {
        save = mln_lang_var_getVal(op2)->data.i;
        mln_lang_var_getVal(op2)->data.i += array->index;
        if (mln_lang_var_getVal(op2)->data.i < 0) {
            mln_lang_errmsg(ctx, "Invalid offset");
            return -1;
        }
    }
    if ((rv = mln_lang_array_getAndNew(ctx, array, op2)) == NULL) {
        return -1;
    }
    if (save != ~((mln_s64_t)0)) {
        mln_lang_var_getVal(op2)->data.i = save;
    }
    *ret = mln_lang_var_ref(rv);
    return 0;
}

static int
mln_lang_array_not(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2)
{
    if (mln_lang_condition_isTrue(op1)) {
        if ((*ret = mln_lang_var_createTmpFalse(ctx, NULL)) == NULL) {
            mln_lang_errmsg(ctx, "No memory.");
            return -1;
        }
    } else {
        if ((*ret = mln_lang_var_createTmpTrue(ctx, NULL)) == NULL) {
            mln_lang_errmsg(ctx, "No memory.");
            return -1;
        }
    }
    return 0;
}

