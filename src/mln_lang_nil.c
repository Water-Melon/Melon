
/*
 * Copyright (C) Niklaus F.Schen.
 */
#include "mln_lang_nil.h"
#include <stdio.h>

#ifdef __DEBUG__
#include <assert.h>
#define ASSERT(x) assert(x)
#else
#define ASSERT(x);
#endif

static int
mln_lang_nil_assign(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_nil_pluseq(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_nil_equal(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_nil_nonequal(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_nil_plus(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_nil_not(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);

mln_lang_method_t mln_lang_nil_oprs = {
    mln_lang_nil_assign,
    mln_lang_nil_pluseq,
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
    mln_lang_nil_equal,
    mln_lang_nil_nonequal,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    mln_lang_nil_plus,
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
    mln_lang_nil_not,
    NULL,
    NULL
};

static int
mln_lang_nil_assign(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2)
{
    if (mln_lang_var_setValue(ctx, op1, op2) < 0) {
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if ((*ret = mln_lang_var_convert(ctx, op1)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    return 0;
}

static int
mln_lang_nil_pluseq(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2)
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
mln_lang_nil_equal(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2)
{
    if (mln_lang_var_getValType(op1) != mln_lang_var_getValType(op2)) {
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

static int
mln_lang_nil_nonequal(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2)
{
    if (mln_lang_var_getValType(op1) != mln_lang_var_getValType(op2)) {
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
mln_lang_nil_plus(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2)
{
    mln_s32_t type = mln_lang_var_getValType(op2);
    if (type != M_LANG_VAL_TYPE_STRING) {
        mln_lang_errmsg(ctx, "Operation Not support.");
        return -1;
    }
    mln_lang_method_t *method = mln_lang_methods[type];
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

static int
mln_lang_nil_not(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2)
{
    if ((*ret = mln_lang_var_createTmpTrue(ctx, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    return 0;
}

