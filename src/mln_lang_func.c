
/*
 * Copyright (C) Niklaus F.Schen.
 */
#include "mln_lang_func.h"
#include <stdio.h>

#ifdef __DEBUG__
#include <assert.h>
#define ASSERT(x) assert(x)
#else
#define ASSERT(x);
#endif

static int
mln_lang_func_assign(mln_lang_ctx_t *ctx, mln_lang_retExp_t **ret, mln_lang_retExp_t *op1, mln_lang_retExp_t *op2);
static int
mln_lang_func_equal(mln_lang_ctx_t *ctx, mln_lang_retExp_t **ret, mln_lang_retExp_t *op1, mln_lang_retExp_t *op2);
static int
mln_lang_func_nonequal(mln_lang_ctx_t *ctx, mln_lang_retExp_t **ret, mln_lang_retExp_t *op1, mln_lang_retExp_t *op2);
static int
mln_lang_func_plus(mln_lang_ctx_t *ctx, mln_lang_retExp_t **ret, mln_lang_retExp_t *op1, mln_lang_retExp_t *op2);
static int
mln_lang_func_not(mln_lang_ctx_t *ctx, mln_lang_retExp_t **ret, mln_lang_retExp_t *op1, mln_lang_retExp_t *op2);

mln_lang_method_t mln_lang_func_oprs = {
    mln_lang_func_assign,
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
    NULL,
    mln_lang_func_equal,
    mln_lang_func_nonequal,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    mln_lang_func_plus,
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
    mln_lang_func_not,
    NULL,
    NULL
};

static int
mln_lang_func_assign(mln_lang_ctx_t *ctx, mln_lang_retExp_t **ret, mln_lang_retExp_t *op1, mln_lang_retExp_t *op2)
{
    ASSERT(op1->type == M_LANG_RETEXP_VAR && op2->type == M_LANG_RETEXP_VAR);
    mln_lang_var_t *var;
    if (mln_lang_var_setValue(ctx, op1->data.var, op2->data.var) < 0) {
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if ((var = mln_lang_var_convert(ctx->pool, op1->data.var)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if ((*ret = mln_lang_retExp_new(ctx->pool, M_LANG_RETEXP_VAR, var)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_var_free(var);
        return -1;
    }
    return 0;
}

static int
mln_lang_func_equal(mln_lang_ctx_t *ctx, mln_lang_retExp_t **ret, mln_lang_retExp_t *op1, mln_lang_retExp_t *op2)
{
    ASSERT(op1->type == M_LANG_RETEXP_VAR && op2->type == M_LANG_RETEXP_VAR);
    mln_u8ptr_t data1, data2;
    mln_lang_func_detail_t *f1, *f2;
    if (mln_lang_var_getValType(op1->data.var) != mln_lang_var_getValType(op2->data.var)) {
f:
        if ((*ret = mln_lang_retExp_createTmpFalse(ctx->pool, NULL)) == NULL) {
            mln_lang_errmsg(ctx, "No memory.");
            return -1;
        }
        return 0;
    }
    f1 = mln_lang_var_getVal(op1->data.var)->data.func;
    f2 = mln_lang_var_getVal(op2->data.var)->data.func;
    if (f1->type != f2->type) goto f;
    data1 = f1->type == M_FUNC_INTERNAL? (mln_u8ptr_t)f1->data.process: (mln_u8ptr_t)f1->data.stm;
    data2 = f2->type == M_FUNC_INTERNAL? (mln_u8ptr_t)f2->data.process: (mln_u8ptr_t)f2->data.stm;
    if (data1 != data2) goto f;
    if ((*ret = mln_lang_retExp_createTmpTrue(ctx->pool, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    return 0;
}

static int
mln_lang_func_nonequal(mln_lang_ctx_t *ctx, mln_lang_retExp_t **ret, mln_lang_retExp_t *op1, mln_lang_retExp_t *op2)
{
    ASSERT(op1->type == M_LANG_RETEXP_VAR && op2->type == M_LANG_RETEXP_VAR);
    mln_u8ptr_t data1, data2;
    mln_lang_func_detail_t *f1, *f2;
    if (mln_lang_var_getValType(op1->data.var) != mln_lang_var_getValType(op2->data.var)) {
t:
        if ((*ret = mln_lang_retExp_createTmpTrue(ctx->pool, NULL)) == NULL) {
            mln_lang_errmsg(ctx, "No memory.");
            return -1;
        }
        return 0;
    }
    f1 = mln_lang_var_getVal(op1->data.var)->data.func;
    f2 = mln_lang_var_getVal(op2->data.var)->data.func;
    if (f1->type != f2->type) goto t;
    data1 = f1->type == M_FUNC_INTERNAL? (mln_u8ptr_t)f1->data.process: (mln_u8ptr_t)f1->data.stm;
    data2 = f2->type == M_FUNC_INTERNAL? (mln_u8ptr_t)f2->data.process: (mln_u8ptr_t)f2->data.stm;
    if (data1 != data2) goto t;
    if ((*ret = mln_lang_retExp_createTmpFalse(ctx->pool, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    return 0;
}

static int
mln_lang_func_plus(mln_lang_ctx_t *ctx, mln_lang_retExp_t **ret, mln_lang_retExp_t *op1, mln_lang_retExp_t *op2)
{
    ASSERT(op1->type == M_LANG_RETEXP_VAR && op2->type == M_LANG_RETEXP_VAR);
    if (mln_lang_var_getValType(op2->data.var) != M_LANG_VAL_TYPE_STRING) {
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

static int
mln_lang_func_not(mln_lang_ctx_t *ctx, mln_lang_retExp_t **ret, mln_lang_retExp_t *op1, mln_lang_retExp_t *op2)
{
    ASSERT(op1->type == M_LANG_RETEXP_VAR);
#ifdef __DEBUG__
    if (op1->data.var->val->data.func->type == M_FUNC_INTERNAL)
        ASSERT(op1->data.var->val->data.func->data.process);
    else
        ASSERT(op1->data.var->val->data.func->data.stm);
#endif
    if ((*ret = mln_lang_retExp_createTmpFalse(ctx->pool, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    return 0;
}

