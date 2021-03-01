
/*
 * Copyright (C) Niklaus F.Schen.
 */
#include "mln_lang_real.h"
#include <stdlib.h>
#include <stdio.h>

#ifdef __DEBUG__
#include <assert.h>
#define ASSERT(x) assert(x)
#else
#define ASSERT(x);
#endif

static int
mln_lang_real_assign(mln_lang_ctx_t *ctx, mln_lang_retExp_t **ret, mln_lang_retExp_t *op1, mln_lang_retExp_t *op2);
static int
mln_lang_real_pluseq(mln_lang_ctx_t *ctx, mln_lang_retExp_t **ret, mln_lang_retExp_t *op1, mln_lang_retExp_t *op2);
static int
mln_lang_real_subeq(mln_lang_ctx_t *ctx, mln_lang_retExp_t **ret, mln_lang_retExp_t *op1, mln_lang_retExp_t *op2);
static int
mln_lang_real_muleq(mln_lang_ctx_t *ctx, mln_lang_retExp_t **ret, mln_lang_retExp_t *op1, mln_lang_retExp_t *op2);
static int
mln_lang_real_diveq(mln_lang_ctx_t *ctx, mln_lang_retExp_t **ret, mln_lang_retExp_t *op1, mln_lang_retExp_t *op2);
static int
mln_lang_real_equal(mln_lang_ctx_t *ctx, mln_lang_retExp_t **ret, mln_lang_retExp_t *op1, mln_lang_retExp_t *op2);
static int
mln_lang_real_nonequal(mln_lang_ctx_t *ctx, mln_lang_retExp_t **ret, mln_lang_retExp_t *op1, mln_lang_retExp_t *op2);
static int
mln_lang_real_less(mln_lang_ctx_t *ctx, mln_lang_retExp_t **ret, mln_lang_retExp_t *op1, mln_lang_retExp_t *op2);
static int
mln_lang_real_lesseq(mln_lang_ctx_t *ctx, mln_lang_retExp_t **ret, mln_lang_retExp_t *op1, mln_lang_retExp_t *op2);
static int
mln_lang_real_grea(mln_lang_ctx_t *ctx, mln_lang_retExp_t **ret, mln_lang_retExp_t *op1, mln_lang_retExp_t *op2);
static int
mln_lang_real_greale(mln_lang_ctx_t *ctx, mln_lang_retExp_t **ret, mln_lang_retExp_t *op1, mln_lang_retExp_t *op2);
static int
mln_lang_real_plus(mln_lang_ctx_t *ctx, mln_lang_retExp_t **ret, mln_lang_retExp_t *op1, mln_lang_retExp_t *op2);
static int
mln_lang_real_sub(mln_lang_ctx_t *ctx, mln_lang_retExp_t **ret, mln_lang_retExp_t *op1, mln_lang_retExp_t *op2);
static int
mln_lang_real_mul(mln_lang_ctx_t *ctx, mln_lang_retExp_t **ret, mln_lang_retExp_t *op1, mln_lang_retExp_t *op2);
static int
mln_lang_real_div(mln_lang_ctx_t *ctx, mln_lang_retExp_t **ret, mln_lang_retExp_t *op1, mln_lang_retExp_t *op2);
static int
mln_lang_real_sdec(mln_lang_ctx_t *ctx, mln_lang_retExp_t **ret, mln_lang_retExp_t *op1, mln_lang_retExp_t *op2);
static int
mln_lang_real_sinc(mln_lang_ctx_t *ctx, mln_lang_retExp_t **ret, mln_lang_retExp_t *op1, mln_lang_retExp_t *op2);
static int
mln_lang_real_negative(mln_lang_ctx_t *ctx, mln_lang_retExp_t **ret, mln_lang_retExp_t *op1, mln_lang_retExp_t *op2);
static int
mln_lang_real_not(mln_lang_ctx_t *ctx, mln_lang_retExp_t **ret, mln_lang_retExp_t *op1, mln_lang_retExp_t *op2);
static int
mln_lang_real_pinc(mln_lang_ctx_t *ctx, mln_lang_retExp_t **ret, mln_lang_retExp_t *op1, mln_lang_retExp_t *op2);
static int
mln_lang_real_pdec(mln_lang_ctx_t *ctx, mln_lang_retExp_t **ret, mln_lang_retExp_t *op1, mln_lang_retExp_t *op2);

mln_lang_method_t mln_lang_real_oprs = {
    mln_lang_real_assign,
    mln_lang_real_pluseq,
    mln_lang_real_subeq,
    NULL,
    NULL,
    mln_lang_real_muleq,
    mln_lang_real_diveq,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    mln_lang_real_equal,
    mln_lang_real_nonequal,
    mln_lang_real_less,
    mln_lang_real_lesseq,
    mln_lang_real_grea,
    mln_lang_real_greale,
    NULL,
    NULL,
    mln_lang_real_plus,
    mln_lang_real_sub,
    mln_lang_real_mul,
    mln_lang_real_div,
    NULL,
    mln_lang_real_sdec,
    mln_lang_real_sinc,
    NULL,
    NULL,
    mln_lang_real_negative,
    NULL,
    mln_lang_real_not,
    mln_lang_real_pinc,
    mln_lang_real_pdec
};

static int
mln_lang_real_assign(mln_lang_ctx_t *ctx, mln_lang_retExp_t **ret, mln_lang_retExp_t *op1, mln_lang_retExp_t *op2)
{
    ASSERT(op1->type == M_LANG_RETEXP_VAR && op2->type == M_LANG_RETEXP_VAR);
    mln_lang_var_t *var;
    if (mln_lang_var_setValue(ctx, op1->data.var, op2->data.var) < 0) {
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if ((var = mln_lang_var_convert(ctx, op1->data.var)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if ((*ret = mln_lang_retExp_new(ctx, M_LANG_RETEXP_VAR, var)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_var_free(var);
        return -1;
    }
    return 0;
}

static int
mln_lang_real_pluseq(mln_lang_ctx_t *ctx, mln_lang_retExp_t **ret, mln_lang_retExp_t *op1, mln_lang_retExp_t *op2)
{
    ASSERT(op1->type == M_LANG_RETEXP_VAR && op2->type == M_LANG_RETEXP_VAR);
    mln_s32_t type = mln_lang_var_getValType(op1->data.var);
    if (type == M_LANG_VAL_TYPE_OBJECT || \
        type == M_LANG_VAL_TYPE_FUNC || \
        type == M_LANG_VAL_TYPE_ARRAY || \
        type == M_LANG_VAL_TYPE_BOOL || \
        type == M_LANG_VAL_TYPE_NIL)
    {
        mln_lang_errmsg(ctx, "Operation Not support.");
        return -1;
    }
    if (type == M_LANG_VAL_TYPE_STRING) {
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
    mln_lang_var_t *var;
    mln_lang_var_setReal(op1->data.var, \
                         mln_lang_var_toReal(op1->data.var) + \
                             mln_lang_var_toReal(op2->data.var));
    if ((var = mln_lang_var_convert(ctx, op1->data.var)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if ((*ret = mln_lang_retExp_new(ctx, M_LANG_RETEXP_VAR, var)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_var_free(var);
        return -1;
    }
    return 0;
}

static int
mln_lang_real_subeq(mln_lang_ctx_t *ctx, mln_lang_retExp_t **ret, mln_lang_retExp_t *op1, mln_lang_retExp_t *op2)
{
    ASSERT(op1->type == M_LANG_RETEXP_VAR && op2->type == M_LANG_RETEXP_VAR);
    mln_s32_t type = mln_lang_var_getValType(op1->data.var);
    if (type == M_LANG_VAL_TYPE_OBJECT || \
        type == M_LANG_VAL_TYPE_FUNC || \
        type == M_LANG_VAL_TYPE_ARRAY || \
        type == M_LANG_VAL_TYPE_BOOL || \
        type == M_LANG_VAL_TYPE_NIL || \
        type == M_LANG_VAL_TYPE_STRING)
    {
        mln_lang_errmsg(ctx, "Operation Not support.");
        return -1;
    }
    mln_lang_var_t *var;
    mln_lang_var_setReal(op1->data.var, \
                         mln_lang_var_toReal(op1->data.var) - \
                             mln_lang_var_toReal(op2->data.var));
    if ((var = mln_lang_var_convert(ctx, op1->data.var)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if ((*ret = mln_lang_retExp_new(ctx, M_LANG_RETEXP_VAR, var)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_var_free(var);
        return -1;
    }
    return 0;
}

static int
mln_lang_real_muleq(mln_lang_ctx_t *ctx, mln_lang_retExp_t **ret, mln_lang_retExp_t *op1, mln_lang_retExp_t *op2)
{
    ASSERT(op1->type == M_LANG_RETEXP_VAR && op2->type == M_LANG_RETEXP_VAR);
    mln_s32_t type = mln_lang_var_getValType(op1->data.var);
    if (type == M_LANG_VAL_TYPE_OBJECT || \
        type == M_LANG_VAL_TYPE_FUNC || \
        type == M_LANG_VAL_TYPE_ARRAY || \
        type == M_LANG_VAL_TYPE_BOOL || \
        type == M_LANG_VAL_TYPE_NIL || \
        type == M_LANG_VAL_TYPE_STRING)
    {
        mln_lang_errmsg(ctx, "Operation Not support.");
        return -1;
    }
    mln_lang_var_t *var;
    mln_lang_var_setReal(op1->data.var, \
                         mln_lang_var_toReal(op1->data.var) * \
                             mln_lang_var_toReal(op2->data.var));
    if ((var = mln_lang_var_convert(ctx, op1->data.var)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if ((*ret = mln_lang_retExp_new(ctx, M_LANG_RETEXP_VAR, var)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_var_free(var);
        return -1;
    }
    return 0;
}

static int
mln_lang_real_diveq(mln_lang_ctx_t *ctx, mln_lang_retExp_t **ret, mln_lang_retExp_t *op1, mln_lang_retExp_t *op2)
{
    ASSERT(op1->type == M_LANG_RETEXP_VAR && op2->type == M_LANG_RETEXP_VAR);
    mln_s32_t type = mln_lang_var_getValType(op1->data.var);
    if (type == M_LANG_VAL_TYPE_OBJECT || \
        type == M_LANG_VAL_TYPE_FUNC || \
        type == M_LANG_VAL_TYPE_ARRAY || \
        type == M_LANG_VAL_TYPE_BOOL || \
        type == M_LANG_VAL_TYPE_NIL || \
        type == M_LANG_VAL_TYPE_STRING)
    {
        mln_lang_errmsg(ctx, "Operation Not support.");
        return -1;
    }
    mln_lang_var_t *var;
    double r = mln_lang_var_toReal(op2->data.var);
    double tmp = r < 0? -r: r;
    if (tmp <= 1e-15) {
        mln_lang_errmsg(ctx, "Division by zero.");
        return -1;
    }
    mln_lang_var_setReal(op1->data.var, mln_lang_var_toReal(op1->data.var) / r);
    if ((var = mln_lang_var_convert(ctx, op1->data.var)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if ((*ret = mln_lang_retExp_new(ctx, M_LANG_RETEXP_VAR, var)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_var_free(var);
        return -1;
    }
    return 0;
}

static int
mln_lang_real_equal(mln_lang_ctx_t *ctx, mln_lang_retExp_t **ret, mln_lang_retExp_t *op1, mln_lang_retExp_t *op2)
{
    ASSERT(op1->type == M_LANG_RETEXP_VAR && op2->type == M_LANG_RETEXP_VAR);
    mln_s32_t type = mln_lang_var_getValType(op2->data.var);
    if (type == M_LANG_VAL_TYPE_OBJECT || \
        type == M_LANG_VAL_TYPE_FUNC || \
        type == M_LANG_VAL_TYPE_ARRAY || \
        type == M_LANG_VAL_TYPE_BOOL || \
        type == M_LANG_VAL_TYPE_NIL)
    {
        if ((*ret = mln_lang_retExp_createTmpFalse(ctx, NULL)) == NULL) {
            mln_lang_errmsg(ctx, "No memory.");
            return -1;
        }
        return 0;
    }
    mln_lang_var_t *var;
    mln_lang_val_t *val;
    mln_u8_t b = mln_lang_var_toReal(op1->data.var) == mln_lang_var_toReal(op2->data.var);
    if ((val = mln_lang_val_new(ctx->pool, M_LANG_VAL_TYPE_BOOL, &b)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if ((var = mln_lang_var_new(ctx, NULL, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_val_free(val);
        return -1;
    }
    if ((*ret = mln_lang_retExp_new(ctx, M_LANG_RETEXP_VAR, var)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_var_free(var);
        return -1;
    }
    return 0;
}

static int
mln_lang_real_nonequal(mln_lang_ctx_t *ctx, mln_lang_retExp_t **ret, mln_lang_retExp_t *op1, mln_lang_retExp_t *op2)
{
    ASSERT(op1->type == M_LANG_RETEXP_VAR && op2->type == M_LANG_RETEXP_VAR);
    mln_s32_t type = mln_lang_var_getValType(op2->data.var);
    if (type == M_LANG_VAL_TYPE_OBJECT || \
        type == M_LANG_VAL_TYPE_FUNC || \
        type == M_LANG_VAL_TYPE_ARRAY || \
        type == M_LANG_VAL_TYPE_BOOL || \
        type == M_LANG_VAL_TYPE_NIL)
    {
        if ((*ret = mln_lang_retExp_createTmpTrue(ctx, NULL)) == NULL) {
            mln_lang_errmsg(ctx, "No memory.");
            return -1;
        }
        return 0;
    }
    mln_lang_var_t *var;
    mln_lang_val_t *val;
    mln_u8_t b = mln_lang_var_toReal(op1->data.var) != mln_lang_var_toReal(op2->data.var);
    if ((val = mln_lang_val_new(ctx->pool, M_LANG_VAL_TYPE_BOOL, &b)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if ((var = mln_lang_var_new(ctx, NULL, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_val_free(val);
        return -1;
    }
    if ((*ret = mln_lang_retExp_new(ctx, M_LANG_RETEXP_VAR, var)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_var_free(var);
        return -1;
    }
    return 0;
}

static int
mln_lang_real_less(mln_lang_ctx_t *ctx, mln_lang_retExp_t **ret, mln_lang_retExp_t *op1, mln_lang_retExp_t *op2)
{
    ASSERT(op1->type == M_LANG_RETEXP_VAR && op2->type == M_LANG_RETEXP_VAR);
    mln_lang_var_t *var;
    mln_lang_val_t *val;
    mln_u8_t b;
    mln_s32_t type = mln_lang_var_getValType(op2->data.var);
    if (type == M_LANG_VAL_TYPE_OBJECT || \
        type == M_LANG_VAL_TYPE_FUNC || \
        type == M_LANG_VAL_TYPE_ARRAY || \
        type == M_LANG_VAL_TYPE_BOOL || \
        type == M_LANG_VAL_TYPE_NIL)
    {
        mln_lang_errmsg(ctx, "Operation Not support.");
        return -1;
    }
    if (type == M_LANG_VAL_TYPE_STRING) {
        mln_lang_method_t *method = mln_lang_methods[type];
        if (method == NULL) {
            mln_lang_errmsg(ctx, "Operation Not support.");
            return -1;
        }
        mln_lang_op handler = method->less_handler;
        if (handler == NULL) {
            mln_lang_errmsg(ctx, "Operation Not support.");
            return -1;
        }
        return handler(ctx, ret, op1, op2);
    }
    b = mln_lang_var_toReal(op1->data.var) < mln_lang_var_toReal(op2->data.var);
    if ((val = mln_lang_val_new(ctx->pool, M_LANG_VAL_TYPE_BOOL, &b)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if ((var = mln_lang_var_new(ctx, NULL, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_val_free(val);
        return -1;
    }
    if ((*ret = mln_lang_retExp_new(ctx, M_LANG_RETEXP_VAR, var)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_var_free(var);
        return -1;
    }
    return 0;
}

static int
mln_lang_real_lesseq(mln_lang_ctx_t *ctx, mln_lang_retExp_t **ret, mln_lang_retExp_t *op1, mln_lang_retExp_t *op2)
{
    ASSERT(op1->type == M_LANG_RETEXP_VAR && op2->type == M_LANG_RETEXP_VAR);
    mln_lang_var_t *var;
    mln_lang_val_t *val;
    mln_u8_t b;
    mln_s32_t type = mln_lang_var_getValType(op2->data.var);
    if (type == M_LANG_VAL_TYPE_OBJECT || \
        type == M_LANG_VAL_TYPE_FUNC || \
        type == M_LANG_VAL_TYPE_ARRAY || \
        type == M_LANG_VAL_TYPE_BOOL || \
        type == M_LANG_VAL_TYPE_NIL)
    {
        mln_lang_errmsg(ctx, "Operation Not support.");
        return -1;
    }
    if (type == M_LANG_VAL_TYPE_STRING) {
        mln_lang_method_t *method = mln_lang_methods[type];
        if (method == NULL) {
            mln_lang_errmsg(ctx, "Operation Not support.");
            return -1;
        }
        mln_lang_op handler = method->lesseq_handler;
        if (handler == NULL) {
            mln_lang_errmsg(ctx, "Operation Not support.");
            return -1;
        }
        return handler(ctx, ret, op1, op2);
    }
    b = mln_lang_var_toReal(op1->data.var) <= mln_lang_var_toReal(op2->data.var);
    if ((val = mln_lang_val_new(ctx->pool, M_LANG_VAL_TYPE_BOOL, &b)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if ((var = mln_lang_var_new(ctx, NULL, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_val_free(val);
        return -1;
    }
    if ((*ret = mln_lang_retExp_new(ctx, M_LANG_RETEXP_VAR, var)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_var_free(var);
        return -1;
    }
    return 0;
}

static int
mln_lang_real_grea(mln_lang_ctx_t *ctx, mln_lang_retExp_t **ret, mln_lang_retExp_t *op1, mln_lang_retExp_t *op2)
{
    ASSERT(op1->type == M_LANG_RETEXP_VAR && op2->type == M_LANG_RETEXP_VAR);
    mln_lang_var_t *var;
    mln_lang_val_t *val;
    mln_u8_t b;
    mln_s32_t type = mln_lang_var_getValType(op2->data.var);
    if (type == M_LANG_VAL_TYPE_OBJECT || \
        type == M_LANG_VAL_TYPE_FUNC || \
        type == M_LANG_VAL_TYPE_ARRAY || \
        type == M_LANG_VAL_TYPE_BOOL || \
        type == M_LANG_VAL_TYPE_NIL)
    {
        mln_lang_errmsg(ctx, "Operation Not support.");
        return -1;
    }
    if (type == M_LANG_VAL_TYPE_STRING) {
        mln_lang_method_t *method = mln_lang_methods[type];
        if (method == NULL) {
            mln_lang_errmsg(ctx, "Operation Not support.");
            return -1;
        }
        mln_lang_op handler = method->grea_handler;
        if (handler == NULL) {
            mln_lang_errmsg(ctx, "Operation Not support.");
            return -1;
        }
        return handler(ctx, ret, op1, op2);
    }
    b = mln_lang_var_toReal(op1->data.var) > mln_lang_var_toReal(op2->data.var);
    if ((val = mln_lang_val_new(ctx->pool, M_LANG_VAL_TYPE_BOOL, &b)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if ((var = mln_lang_var_new(ctx, NULL, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_val_free(val);
        return -1;
    }
    if ((*ret = mln_lang_retExp_new(ctx, M_LANG_RETEXP_VAR, var)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_var_free(var);
        return -1;
    }
    return 0;
}

static int
mln_lang_real_greale(mln_lang_ctx_t *ctx, mln_lang_retExp_t **ret, mln_lang_retExp_t *op1, mln_lang_retExp_t *op2)
{
    ASSERT(op1->type == M_LANG_RETEXP_VAR && op2->type == M_LANG_RETEXP_VAR);
    mln_lang_var_t *var;
    mln_lang_val_t *val;
    mln_u8_t b;
    mln_s32_t type = mln_lang_var_getValType(op2->data.var);
    if (type == M_LANG_VAL_TYPE_OBJECT || \
        type == M_LANG_VAL_TYPE_FUNC || \
        type == M_LANG_VAL_TYPE_ARRAY || \
        type == M_LANG_VAL_TYPE_BOOL || \
        type == M_LANG_VAL_TYPE_NIL)
    {
        mln_lang_errmsg(ctx, "Operation Not support.");
        return -1;
    }
    if (type == M_LANG_VAL_TYPE_STRING) {
        mln_lang_method_t *method = mln_lang_methods[type];
        if (method == NULL) {
            mln_lang_errmsg(ctx, "Operation Not support.");
            return -1;
        }
        mln_lang_op handler = method->greale_handler;
        if (handler == NULL) {
            mln_lang_errmsg(ctx, "Operation Not support.");
            return -1;
        }
        return handler(ctx, ret, op1, op2);
    }
    b = mln_lang_var_toReal(op1->data.var) >= mln_lang_var_toReal(op2->data.var);
    if ((val = mln_lang_val_new(ctx->pool, M_LANG_VAL_TYPE_BOOL, &b)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if ((var = mln_lang_var_new(ctx, NULL, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_val_free(val);
        return -1;
    }
    if ((*ret = mln_lang_retExp_new(ctx, M_LANG_RETEXP_VAR, var)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_var_free(var);
        return -1;
    }
    return 0;
}

static int
mln_lang_real_plus(mln_lang_ctx_t *ctx, mln_lang_retExp_t **ret, mln_lang_retExp_t *op1, mln_lang_retExp_t *op2)
{
    ASSERT(op1->type == M_LANG_RETEXP_VAR && op2->type == M_LANG_RETEXP_VAR);
    mln_s32_t type = mln_lang_var_getValType(op2->data.var);
    if (type == M_LANG_VAL_TYPE_OBJECT || \
        type == M_LANG_VAL_TYPE_FUNC || \
        type == M_LANG_VAL_TYPE_ARRAY || \
        type == M_LANG_VAL_TYPE_BOOL || \
        type == M_LANG_VAL_TYPE_NIL)
    {
        mln_lang_errmsg(ctx, "Operation Not support.");
        return -1;
    }
    if (type == M_LANG_VAL_TYPE_STRING) {
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
    mln_lang_var_t *var;
    mln_lang_val_t *val;
    double r = mln_lang_var_toReal(op1->data.var) + mln_lang_var_toReal(op2->data.var);
    if ((val = mln_lang_val_new(ctx->pool, M_LANG_VAL_TYPE_REAL, &r)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if ((var = mln_lang_var_new(ctx, NULL, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_val_free(val);
        return -1;
    }
    if ((*ret = mln_lang_retExp_new(ctx, M_LANG_RETEXP_VAR, var)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_var_free(var);
        return -1;
    }
    return 0;
}

static int
mln_lang_real_sub(mln_lang_ctx_t *ctx, mln_lang_retExp_t **ret, mln_lang_retExp_t *op1, mln_lang_retExp_t *op2)
{
    ASSERT(op1->type == M_LANG_RETEXP_VAR && op2->type == M_LANG_RETEXP_VAR);
    mln_s32_t type = mln_lang_var_getValType(op2->data.var);
    if (type == M_LANG_VAL_TYPE_OBJECT || \
        type == M_LANG_VAL_TYPE_FUNC || \
        type == M_LANG_VAL_TYPE_ARRAY || \
        type == M_LANG_VAL_TYPE_BOOL || \
        type == M_LANG_VAL_TYPE_NIL)
    {
        mln_lang_errmsg(ctx, "Operation Not support.");
        return -1;
    }
    mln_lang_var_t *var;
    mln_lang_val_t *val;
    double r = mln_lang_var_toReal(op1->data.var) - mln_lang_var_toReal(op2->data.var);
    if ((val = mln_lang_val_new(ctx->pool, M_LANG_VAL_TYPE_REAL, &r)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if ((var = mln_lang_var_new(ctx, NULL, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_val_free(val);
        return -1;
    }
    if ((*ret = mln_lang_retExp_new(ctx, M_LANG_RETEXP_VAR, var)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_var_free(var);
        return -1;
    }
    return 0;
}

static int
mln_lang_real_mul(mln_lang_ctx_t *ctx, mln_lang_retExp_t **ret, mln_lang_retExp_t *op1, mln_lang_retExp_t *op2)
{
    ASSERT(op1->type == M_LANG_RETEXP_VAR && op2->type == M_LANG_RETEXP_VAR);
    mln_s32_t type = mln_lang_var_getValType(op2->data.var);
    if (type == M_LANG_VAL_TYPE_OBJECT || \
        type == M_LANG_VAL_TYPE_FUNC || \
        type == M_LANG_VAL_TYPE_ARRAY || \
        type == M_LANG_VAL_TYPE_BOOL || \
        type == M_LANG_VAL_TYPE_NIL)
    {
        mln_lang_errmsg(ctx, "Operation Not support.");
        return -1;
    }
    mln_lang_var_t *var;
    mln_lang_val_t *val;
    double r = mln_lang_var_toReal(op1->data.var) * mln_lang_var_toReal(op2->data.var);
    if ((val = mln_lang_val_new(ctx->pool, M_LANG_VAL_TYPE_REAL, &r)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if ((var = mln_lang_var_new(ctx, NULL, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_val_free(val);
        return -1;
    }
    if ((*ret = mln_lang_retExp_new(ctx, M_LANG_RETEXP_VAR, var)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_var_free(var);
        return -1;
    }
    return 0;
}

static int
mln_lang_real_div(mln_lang_ctx_t *ctx, mln_lang_retExp_t **ret, mln_lang_retExp_t *op1, mln_lang_retExp_t *op2)
{
    ASSERT(op1->type == M_LANG_RETEXP_VAR && op2->type == M_LANG_RETEXP_VAR);
    mln_s32_t type = mln_lang_var_getValType(op2->data.var);
    if (type == M_LANG_VAL_TYPE_OBJECT || \
        type == M_LANG_VAL_TYPE_FUNC || \
        type == M_LANG_VAL_TYPE_ARRAY || \
        type == M_LANG_VAL_TYPE_BOOL || \
        type == M_LANG_VAL_TYPE_NIL)
    {
        mln_lang_errmsg(ctx, "Operation Not support.");
        return -1;
    }
    mln_lang_var_t *var;
    mln_lang_val_t *val;
    double tmp = mln_lang_var_toReal(op2->data.var);
    double tmpr = tmp < 0? -tmp: tmp;
    if (tmpr <= 1e-15) {
        mln_lang_errmsg(ctx, "Division by zero.");
        return -1;
    }
    double r = mln_lang_var_toReal(op1->data.var) / tmp;
    if ((val = mln_lang_val_new(ctx->pool, M_LANG_VAL_TYPE_REAL, &r)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if ((var = mln_lang_var_new(ctx, NULL, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_val_free(val);
        return -1;
    }
    if ((*ret = mln_lang_retExp_new(ctx, M_LANG_RETEXP_VAR, var)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_var_free(var);
        return -1;
    }
    return 0;
}

static int
mln_lang_real_sdec(mln_lang_ctx_t *ctx, mln_lang_retExp_t **ret, mln_lang_retExp_t *op1, mln_lang_retExp_t *op2)
{
    ASSERT(op1->type == M_LANG_RETEXP_VAR);
    mln_lang_var_t *var;
    mln_lang_val_t *val;
    double r = mln_lang_var_toReal(op1->data.var);
    mln_lang_var_setReal(op1->data.var, r-1);
    if ((val = mln_lang_val_new(ctx->pool, M_LANG_VAL_TYPE_REAL, &r)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if ((var = mln_lang_var_new(ctx, NULL, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_val_free(val);
        return -1;
    }
    if ((*ret = mln_lang_retExp_new(ctx, M_LANG_RETEXP_VAR, var)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_var_free(var);
        return -1;
    }
    return 0;
}

static int
mln_lang_real_sinc(mln_lang_ctx_t *ctx, mln_lang_retExp_t **ret, mln_lang_retExp_t *op1, mln_lang_retExp_t *op2)
{
    ASSERT(op1->type == M_LANG_RETEXP_VAR);
    mln_lang_var_t *var;
    mln_lang_val_t *val;
    double r = mln_lang_var_toReal(op1->data.var);
    mln_lang_var_setReal(op1->data.var, r+1);
    if ((val = mln_lang_val_new(ctx->pool, M_LANG_VAL_TYPE_REAL, &r)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if ((var = mln_lang_var_new(ctx, NULL, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_val_free(val);
        return -1;
    }
    if ((*ret = mln_lang_retExp_new(ctx, M_LANG_RETEXP_VAR, var)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_var_free(var);
        return -1;
    }
    return 0;
}

static int
mln_lang_real_negative(mln_lang_ctx_t *ctx, mln_lang_retExp_t **ret, mln_lang_retExp_t *op1, mln_lang_retExp_t *op2)
{
    ASSERT(op1->type == M_LANG_RETEXP_VAR);
    mln_lang_var_t *var;
    mln_lang_val_t *val;
    double r = -mln_lang_var_toReal(op1->data.var);
    if ((val = mln_lang_val_new(ctx->pool, M_LANG_VAL_TYPE_REAL, &r)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if ((var = mln_lang_var_new(ctx, NULL, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_val_free(val);
        return -1;
    }
    if ((*ret = mln_lang_retExp_new(ctx, M_LANG_RETEXP_VAR, var)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_var_free(var);
        return -1;
    }
    return 0;
}

static int
mln_lang_real_not(mln_lang_ctx_t *ctx, mln_lang_retExp_t **ret, mln_lang_retExp_t *op1, mln_lang_retExp_t *op2)
{
    ASSERT(op1->type == M_LANG_RETEXP_VAR);
    mln_lang_var_t *var;
    mln_lang_val_t *val;
    mln_u8_t b = !mln_lang_var_toReal(op1->data.var);
    if ((val = mln_lang_val_new(ctx->pool, M_LANG_VAL_TYPE_BOOL, &b)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if ((var = mln_lang_var_new(ctx, NULL, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_val_free(val);
        return -1;
    }
    if ((*ret = mln_lang_retExp_new(ctx, M_LANG_RETEXP_VAR, var)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_var_free(var);
        return -1;
    }
    return 0;
}

static int
mln_lang_real_pinc(mln_lang_ctx_t *ctx, mln_lang_retExp_t **ret, mln_lang_retExp_t *op1, mln_lang_retExp_t *op2)
{
    ASSERT(op1->type == M_LANG_RETEXP_VAR);
    mln_lang_var_t *var;
    mln_lang_val_t *val;
    double r = mln_lang_var_toReal(op1->data.var) + 1;
    mln_lang_var_setReal(op1->data.var, r);
    if ((val = mln_lang_val_new(ctx->pool, M_LANG_VAL_TYPE_REAL, &r)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if ((var = mln_lang_var_new(ctx, NULL, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_val_free(val);
        return -1;
    }
    if ((*ret = mln_lang_retExp_new(ctx, M_LANG_RETEXP_VAR, var)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_var_free(var);
        return -1;
    }
    return 0;
}

static int
mln_lang_real_pdec(mln_lang_ctx_t *ctx, mln_lang_retExp_t **ret, mln_lang_retExp_t *op1, mln_lang_retExp_t *op2)
{
    ASSERT(op1->type == M_LANG_RETEXP_VAR);
    mln_lang_var_t *var;
    mln_lang_val_t *val;
    double r = mln_lang_var_toReal(op1->data.var) - 1;
    mln_lang_var_setReal(op1->data.var, r);
    if ((val = mln_lang_val_new(ctx->pool, M_LANG_VAL_TYPE_REAL, &r)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if ((var = mln_lang_var_new(ctx, NULL, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_val_free(val);
        return -1;
    }
    if ((*ret = mln_lang_retExp_new(ctx, M_LANG_RETEXP_VAR, var)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_var_free(var);
        return -1;
    }
    return 0;
}

