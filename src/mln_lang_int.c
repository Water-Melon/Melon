
/*
 * Copyright (C) Niklaus F.Schen.
 */
#include "mln_lang_int.h"
#include <stdio.h>
#include <stdlib.h>

#ifdef __DEBUG__
#include <assert.h>
#define ASSERT(x) assert(x)
#else
#define ASSERT(x);
#endif

static int
mln_lang_int_assign(mln_lang_ctx_t *ctx, mln_lang_retExp_t **ret, mln_lang_retExp_t *op1, mln_lang_retExp_t *op2);
static int
mln_lang_int_pluseq(mln_lang_ctx_t *ctx, mln_lang_retExp_t **ret, mln_lang_retExp_t *op1, mln_lang_retExp_t *op2);
static int
mln_lang_int_subeq(mln_lang_ctx_t *ctx, mln_lang_retExp_t **ret, mln_lang_retExp_t *op1, mln_lang_retExp_t *op2);
static int
mln_lang_int_lmoveq(mln_lang_ctx_t *ctx, mln_lang_retExp_t **ret, mln_lang_retExp_t *op1, mln_lang_retExp_t *op2);
static int
mln_lang_int_rmoveq(mln_lang_ctx_t *ctx, mln_lang_retExp_t **ret, mln_lang_retExp_t *op1, mln_lang_retExp_t *op2);
static int
mln_lang_int_muleq(mln_lang_ctx_t *ctx, mln_lang_retExp_t **ret, mln_lang_retExp_t *op1, mln_lang_retExp_t *op2);
static int
mln_lang_int_diveq(mln_lang_ctx_t *ctx, mln_lang_retExp_t **ret, mln_lang_retExp_t *op1, mln_lang_retExp_t *op2);
static int
mln_lang_int_oreq(mln_lang_ctx_t *ctx, mln_lang_retExp_t **ret, mln_lang_retExp_t *op1, mln_lang_retExp_t *op2);
static int
mln_lang_int_andeq(mln_lang_ctx_t *ctx, mln_lang_retExp_t **ret, mln_lang_retExp_t *op1, mln_lang_retExp_t *op2);
static int
mln_lang_int_xoreq(mln_lang_ctx_t *ctx, mln_lang_retExp_t **ret, mln_lang_retExp_t *op1, mln_lang_retExp_t *op2);
static int
mln_lang_int_modeq(mln_lang_ctx_t *ctx, mln_lang_retExp_t **ret, mln_lang_retExp_t *op1, mln_lang_retExp_t *op2);
static int
mln_lang_int_cor(mln_lang_ctx_t *ctx, mln_lang_retExp_t **ret, mln_lang_retExp_t *op1, mln_lang_retExp_t *op2);
static int
mln_lang_int_cand(mln_lang_ctx_t *ctx, mln_lang_retExp_t **ret, mln_lang_retExp_t *op1, mln_lang_retExp_t *op2);
static int
mln_lang_int_cxor(mln_lang_ctx_t *ctx, mln_lang_retExp_t **ret, mln_lang_retExp_t *op1, mln_lang_retExp_t *op2);
static int
mln_lang_int_equal(mln_lang_ctx_t *ctx, mln_lang_retExp_t **ret, mln_lang_retExp_t *op1, mln_lang_retExp_t *op2);
static int
mln_lang_int_nonequal(mln_lang_ctx_t *ctx, mln_lang_retExp_t **ret, mln_lang_retExp_t *op1, mln_lang_retExp_t *op2);
static int
mln_lang_int_less(mln_lang_ctx_t *ctx, mln_lang_retExp_t **ret, mln_lang_retExp_t *op1, mln_lang_retExp_t *op2);
static int
mln_lang_int_lesseq(mln_lang_ctx_t *ctx, mln_lang_retExp_t **ret, mln_lang_retExp_t *op1, mln_lang_retExp_t *op2);
static int
mln_lang_int_grea(mln_lang_ctx_t *ctx, mln_lang_retExp_t **ret, mln_lang_retExp_t *op1, mln_lang_retExp_t *op2);
static int
mln_lang_int_greale(mln_lang_ctx_t *ctx, mln_lang_retExp_t **ret, mln_lang_retExp_t *op1, mln_lang_retExp_t *op2);
static int
mln_lang_int_lmov(mln_lang_ctx_t *ctx, mln_lang_retExp_t **ret, mln_lang_retExp_t *op1, mln_lang_retExp_t *op2);
static int
mln_lang_int_rmov(mln_lang_ctx_t *ctx, mln_lang_retExp_t **ret, mln_lang_retExp_t *op1, mln_lang_retExp_t *op2);
static int
mln_lang_int_plus(mln_lang_ctx_t *ctx, mln_lang_retExp_t **ret, mln_lang_retExp_t *op1, mln_lang_retExp_t *op2);
static int
mln_lang_int_sub(mln_lang_ctx_t *ctx, mln_lang_retExp_t **ret, mln_lang_retExp_t *op1, mln_lang_retExp_t *op2);
static int
mln_lang_int_mul(mln_lang_ctx_t *ctx, mln_lang_retExp_t **ret, mln_lang_retExp_t *op1, mln_lang_retExp_t *op2);
static int
mln_lang_int_div(mln_lang_ctx_t *ctx, mln_lang_retExp_t **ret, mln_lang_retExp_t *op1, mln_lang_retExp_t *op2);
static int
mln_lang_int_mod(mln_lang_ctx_t *ctx, mln_lang_retExp_t **ret, mln_lang_retExp_t *op1, mln_lang_retExp_t *op2);
static int
mln_lang_int_sdec(mln_lang_ctx_t *ctx, mln_lang_retExp_t **ret, mln_lang_retExp_t *op1, mln_lang_retExp_t *op2);
static int
mln_lang_int_sinc(mln_lang_ctx_t *ctx, mln_lang_retExp_t **ret, mln_lang_retExp_t *op1, mln_lang_retExp_t *op2);
static int
mln_lang_int_negative(mln_lang_ctx_t *ctx, mln_lang_retExp_t **ret, mln_lang_retExp_t *op1, mln_lang_retExp_t *op2);
static int
mln_lang_int_reverse(mln_lang_ctx_t *ctx, mln_lang_retExp_t **ret, mln_lang_retExp_t *op1, mln_lang_retExp_t *op2);
static int
mln_lang_int_not(mln_lang_ctx_t *ctx, mln_lang_retExp_t **ret, mln_lang_retExp_t *op1, mln_lang_retExp_t *op2);
static int
mln_lang_int_pinc(mln_lang_ctx_t *ctx, mln_lang_retExp_t **ret, mln_lang_retExp_t *op1, mln_lang_retExp_t *op2);
static int
mln_lang_int_pdec(mln_lang_ctx_t *ctx, mln_lang_retExp_t **ret, mln_lang_retExp_t *op1, mln_lang_retExp_t *op2);

mln_lang_method_t mln_lang_int_oprs = {
    mln_lang_int_assign,
    mln_lang_int_pluseq,
    mln_lang_int_subeq,
    mln_lang_int_lmoveq,
    mln_lang_int_rmoveq,
    mln_lang_int_muleq,
    mln_lang_int_diveq,
    mln_lang_int_oreq,
    mln_lang_int_andeq,
    mln_lang_int_xoreq,
    mln_lang_int_modeq,
    mln_lang_int_cor,
    mln_lang_int_cand,
    mln_lang_int_cxor,
    mln_lang_int_equal,
    mln_lang_int_nonequal,
    mln_lang_int_less,
    mln_lang_int_lesseq,
    mln_lang_int_grea,
    mln_lang_int_greale,
    mln_lang_int_lmov,
    mln_lang_int_rmov,
    mln_lang_int_plus,
    mln_lang_int_sub,
    mln_lang_int_mul,
    mln_lang_int_div,
    mln_lang_int_mod,
    mln_lang_int_sdec,
    mln_lang_int_sinc,
    NULL,
    NULL,
    mln_lang_int_negative,
    mln_lang_int_reverse,
    mln_lang_int_not,
    mln_lang_int_pinc,
    mln_lang_int_pdec
};

static int
mln_lang_int_assign(mln_lang_ctx_t *ctx, mln_lang_retExp_t **ret, mln_lang_retExp_t *op1, mln_lang_retExp_t *op2)
{
    ASSERT(op1->type == M_LANG_RETEXP_VAR && op2->type == M_LANG_RETEXP_VAR);
    mln_lang_var_t *var;
    if (mln_lang_var_setValue(ctx->pool, op1->data.var, op2->data.var) < 0) {
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
mln_lang_int_pluseq(mln_lang_ctx_t *ctx, mln_lang_retExp_t **ret, mln_lang_retExp_t *op1, mln_lang_retExp_t *op2)
{
    ASSERT(op1->type == M_LANG_RETEXP_VAR && op2->type == M_LANG_RETEXP_VAR);
    mln_s32_t type = mln_lang_var_getValType(op2->data.var);
    if (type == M_LANG_VAL_TYPE_OBJECT || \
        type == M_LANG_VAL_TYPE_FUNC || \
        type == M_LANG_VAL_TYPE_ARRAY)
    {
        mln_lang_errmsg(ctx, "Operation Not support.");
        return -1;
    }
    mln_lang_var_t *var;
    if (type == M_LANG_VAL_TYPE_REAL) {
        mln_lang_var_setReal(op1->data.var, \
                             mln_lang_var_toReal(op1->data.var) + \
                                 mln_lang_var_toReal(op2->data.var));
    } else {
        mln_lang_var_setInt(op1->data.var, \
                            mln_lang_var_toInt(op1->data.var) + \
                                mln_lang_var_toInt(op2->data.var));
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
mln_lang_int_subeq(mln_lang_ctx_t *ctx, mln_lang_retExp_t **ret, mln_lang_retExp_t *op1, mln_lang_retExp_t *op2)
{
    ASSERT(op1->type == M_LANG_RETEXP_VAR && op2->type == M_LANG_RETEXP_VAR);
    mln_s32_t type = mln_lang_var_getValType(op2->data.var);
    if (type == M_LANG_VAL_TYPE_OBJECT || \
        type == M_LANG_VAL_TYPE_FUNC || \
        type == M_LANG_VAL_TYPE_ARRAY)
    {
        mln_lang_errmsg(ctx, "Operation Not support.");
        return -1;
    }
    mln_lang_var_t *var;
    if (type == M_LANG_VAL_TYPE_REAL) {
        mln_lang_var_setReal(op1->data.var, \
                             mln_lang_var_toReal(op1->data.var) - \
                                 mln_lang_var_toReal(op2->data.var));
    } else {
        mln_lang_var_setInt(op1->data.var, \
                            mln_lang_var_toInt(op1->data.var) - \
                                mln_lang_var_toInt(op2->data.var));
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
mln_lang_int_lmoveq(mln_lang_ctx_t *ctx, mln_lang_retExp_t **ret, mln_lang_retExp_t *op1, mln_lang_retExp_t *op2)
{
    ASSERT(op1->type == M_LANG_RETEXP_VAR && op2->type == M_LANG_RETEXP_VAR);
    mln_s32_t type = mln_lang_var_getValType(op2->data.var);
    if (type == M_LANG_VAL_TYPE_OBJECT || \
        type == M_LANG_VAL_TYPE_FUNC || \
        type == M_LANG_VAL_TYPE_ARRAY || \
        type == M_LANG_VAL_TYPE_REAL)
    {
        mln_lang_errmsg(ctx, "Operation Not support.");
        return -1;
    }
    mln_lang_var_t *var;
    mln_lang_var_setInt(op1->data.var, \
                        mln_lang_var_toInt(op1->data.var) << \
                            mln_lang_var_toInt(op2->data.var));
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
mln_lang_int_rmoveq(mln_lang_ctx_t *ctx, mln_lang_retExp_t **ret, mln_lang_retExp_t *op1, mln_lang_retExp_t *op2)
{
    ASSERT(op1->type == M_LANG_RETEXP_VAR && op2->type == M_LANG_RETEXP_VAR);
    mln_s32_t type = mln_lang_var_getValType(op2->data.var);
    if (type == M_LANG_VAL_TYPE_OBJECT || \
        type == M_LANG_VAL_TYPE_FUNC || \
        type == M_LANG_VAL_TYPE_ARRAY || \
        type == M_LANG_VAL_TYPE_REAL)
    {
        mln_lang_errmsg(ctx, "Operation Not support.");
        return -1;
    }
    mln_lang_var_t *var;
    mln_lang_var_setInt(op1->data.var, \
                        mln_lang_var_toInt(op1->data.var) >> \
                            mln_lang_var_toInt(op2->data.var));
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
mln_lang_int_muleq(mln_lang_ctx_t *ctx, mln_lang_retExp_t **ret, mln_lang_retExp_t *op1, mln_lang_retExp_t *op2)
{
    ASSERT(op1->type == M_LANG_RETEXP_VAR && op2->type == M_LANG_RETEXP_VAR);
    mln_s32_t type = mln_lang_var_getValType(op2->data.var);
    if (type == M_LANG_VAL_TYPE_OBJECT || \
        type == M_LANG_VAL_TYPE_FUNC || \
        type == M_LANG_VAL_TYPE_ARRAY)
    {
        mln_lang_errmsg(ctx, "Operation Not support.");
        return -1;
    }
    mln_lang_var_t *var;
    if (type == M_LANG_VAL_TYPE_REAL) {
        mln_lang_var_setReal(op1->data.var, \
                             mln_lang_var_toReal(op1->data.var) * \
                                 mln_lang_var_toReal(op2->data.var));
    } else {
        mln_lang_var_setInt(op1->data.var, \
                            mln_lang_var_toInt(op1->data.var) * \
                                mln_lang_var_toInt(op2->data.var));
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
mln_lang_int_diveq(mln_lang_ctx_t *ctx, mln_lang_retExp_t **ret, mln_lang_retExp_t *op1, mln_lang_retExp_t *op2)
{
    ASSERT(op1->type == M_LANG_RETEXP_VAR && op2->type == M_LANG_RETEXP_VAR);
    mln_s32_t type = mln_lang_var_getValType(op2->data.var);
    if (type == M_LANG_VAL_TYPE_OBJECT || \
        type == M_LANG_VAL_TYPE_FUNC || \
        type == M_LANG_VAL_TYPE_ARRAY)
    {
        mln_lang_errmsg(ctx, "Operation Not support.");
        return -1;
    }
    mln_lang_var_t *var;
    if (type == M_LANG_VAL_TYPE_REAL) {
        double r = mln_lang_var_toReal(op2->data.var);
        double tmp = r < 0? -r: r;
        if (tmp <= 1e-15) {
            mln_lang_errmsg(ctx, "Division by zero.");
            return -1;
        }
        mln_lang_var_setReal(op1->data.var, mln_lang_var_toReal(op1->data.var) / r);
    } else {
        mln_s64_t i = mln_lang_var_toInt(op2->data.var);
        if (!i) {
            mln_lang_errmsg(ctx, "Division by zero.");
            return -1;
        }
        mln_lang_var_setInt(op1->data.var, mln_lang_var_toInt(op1->data.var) / i);
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
mln_lang_int_oreq(mln_lang_ctx_t *ctx, mln_lang_retExp_t **ret, mln_lang_retExp_t *op1, mln_lang_retExp_t *op2)
{
    ASSERT(op1->type == M_LANG_RETEXP_VAR && op2->type == M_LANG_RETEXP_VAR);
    mln_s32_t type = mln_lang_var_getValType(op2->data.var);
    if (type == M_LANG_VAL_TYPE_OBJECT || \
        type == M_LANG_VAL_TYPE_FUNC || \
        type == M_LANG_VAL_TYPE_ARRAY || \
        type == M_LANG_VAL_TYPE_REAL)
    {
        mln_lang_errmsg(ctx, "Operation Not support.");
        return -1;
    }
    mln_lang_var_t *var;
    mln_lang_var_setInt(op1->data.var, \
                        mln_lang_var_toInt(op1->data.var) | \
                            mln_lang_var_toInt(op2->data.var));
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
mln_lang_int_andeq(mln_lang_ctx_t *ctx, mln_lang_retExp_t **ret, mln_lang_retExp_t *op1, mln_lang_retExp_t *op2)
{
    ASSERT(op1->type == M_LANG_RETEXP_VAR && op2->type == M_LANG_RETEXP_VAR);
    mln_s32_t type = mln_lang_var_getValType(op2->data.var);
    if (type == M_LANG_VAL_TYPE_OBJECT || \
        type == M_LANG_VAL_TYPE_FUNC || \
        type == M_LANG_VAL_TYPE_ARRAY || \
        type == M_LANG_VAL_TYPE_REAL)
    {
        mln_lang_errmsg(ctx, "Operation Not support.");
        return -1;
    }
    mln_lang_var_t *var;
    mln_lang_var_setInt(op1->data.var, \
                        mln_lang_var_toInt(op1->data.var) & \
                            mln_lang_var_toInt(op2->data.var));
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
mln_lang_int_xoreq(mln_lang_ctx_t *ctx, mln_lang_retExp_t **ret, mln_lang_retExp_t *op1, mln_lang_retExp_t *op2)
{
    ASSERT(op1->type == M_LANG_RETEXP_VAR && op2->type == M_LANG_RETEXP_VAR);
    mln_s32_t type = mln_lang_var_getValType(op2->data.var);
    if (type == M_LANG_VAL_TYPE_OBJECT || \
        type == M_LANG_VAL_TYPE_FUNC || \
        type == M_LANG_VAL_TYPE_ARRAY || \
        type == M_LANG_VAL_TYPE_REAL)
    {
        mln_lang_errmsg(ctx, "Operation Not support.");
        return -1;
    }
    mln_lang_var_t *var;
    mln_lang_var_setInt(op1->data.var, \
                        mln_lang_var_toInt(op1->data.var) ^ \
                            mln_lang_var_toInt(op2->data.var));
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
mln_lang_int_modeq(mln_lang_ctx_t *ctx, mln_lang_retExp_t **ret, mln_lang_retExp_t *op1, mln_lang_retExp_t *op2)
{
    ASSERT(op1->type == M_LANG_RETEXP_VAR && op2->type == M_LANG_RETEXP_VAR);
    mln_s32_t type = mln_lang_var_getValType(op2->data.var);
    if (type == M_LANG_VAL_TYPE_OBJECT || \
        type == M_LANG_VAL_TYPE_FUNC || \
        type == M_LANG_VAL_TYPE_ARRAY || \
        type == M_LANG_VAL_TYPE_REAL)
    {
        mln_lang_errmsg(ctx, "Operation Not support.");
        return -1;
    }
    mln_lang_var_t *var;
    mln_s64_t i = mln_lang_var_toInt(op2->data.var);
    if (!i) {
        mln_lang_errmsg(ctx, "Modulo by zero.");
        return -1;
    }
    mln_lang_var_setInt(op1->data.var, mln_lang_var_toInt(op1->data.var) % i);
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
mln_lang_int_cor(mln_lang_ctx_t *ctx, mln_lang_retExp_t **ret, mln_lang_retExp_t *op1, mln_lang_retExp_t *op2)
{
    ASSERT(op1->type == M_LANG_RETEXP_VAR && op2->type == M_LANG_RETEXP_VAR);
    mln_s32_t type = mln_lang_var_getValType(op2->data.var);
    if (type == M_LANG_VAL_TYPE_OBJECT || \
        type == M_LANG_VAL_TYPE_FUNC || \
        type == M_LANG_VAL_TYPE_ARRAY || \
        type == M_LANG_VAL_TYPE_REAL)
    {
        mln_lang_errmsg(ctx, "Operation Not support.");
        return -1;
    }
    mln_lang_var_t *var;
    mln_lang_val_t *val;
    mln_s64_t i = mln_lang_var_toInt(op1->data.var) | mln_lang_var_toInt(op2->data.var);
    if ((val = mln_lang_val_new(ctx->pool, M_LANG_VAL_TYPE_INT, &i)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if ((var = mln_lang_var_new(ctx->pool, NULL, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_val_free(val);
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
mln_lang_int_cand(mln_lang_ctx_t *ctx, mln_lang_retExp_t **ret, mln_lang_retExp_t *op1, mln_lang_retExp_t *op2)
{
    ASSERT(op1->type == M_LANG_RETEXP_VAR && op2->type == M_LANG_RETEXP_VAR);
    mln_s32_t type = mln_lang_var_getValType(op2->data.var);
    if (type == M_LANG_VAL_TYPE_OBJECT || \
        type == M_LANG_VAL_TYPE_FUNC || \
        type == M_LANG_VAL_TYPE_ARRAY || \
        type == M_LANG_VAL_TYPE_REAL)
    {
        mln_lang_errmsg(ctx, "Operation Not support.");
        return -1;
    }
    mln_lang_var_t *var;
    mln_lang_val_t *val;
    mln_s64_t i = mln_lang_var_toInt(op1->data.var) & mln_lang_var_toInt(op2->data.var);
    if ((val = mln_lang_val_new(ctx->pool, M_LANG_VAL_TYPE_INT, &i)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if ((var = mln_lang_var_new(ctx->pool, NULL, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_val_free(val);
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
mln_lang_int_cxor(mln_lang_ctx_t *ctx, mln_lang_retExp_t **ret, mln_lang_retExp_t *op1, mln_lang_retExp_t *op2)
{
    ASSERT(op1->type == M_LANG_RETEXP_VAR && op2->type == M_LANG_RETEXP_VAR);
    mln_s32_t type = mln_lang_var_getValType(op2->data.var);
    if (type == M_LANG_VAL_TYPE_OBJECT || \
        type == M_LANG_VAL_TYPE_FUNC || \
        type == M_LANG_VAL_TYPE_ARRAY || \
        type == M_LANG_VAL_TYPE_REAL)
    {
        mln_lang_errmsg(ctx, "Operation Not support.");
        return -1;
    }
    mln_lang_var_t *var;
    mln_lang_val_t *val;
    mln_s64_t i = mln_lang_var_toInt(op1->data.var) ^ mln_lang_var_toInt(op2->data.var);
    if ((val = mln_lang_val_new(ctx->pool, M_LANG_VAL_TYPE_INT, &i)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if ((var = mln_lang_var_new(ctx->pool, NULL, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_val_free(val);
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
mln_lang_int_equal(mln_lang_ctx_t *ctx, mln_lang_retExp_t **ret, mln_lang_retExp_t *op1, mln_lang_retExp_t *op2)
{
    ASSERT(op1->type == M_LANG_RETEXP_VAR && op2->type == M_LANG_RETEXP_VAR);
    mln_s32_t type = mln_lang_var_getValType(op1->data.var);
    if (type == M_LANG_VAL_TYPE_OBJECT || \
        type == M_LANG_VAL_TYPE_FUNC || \
        type == M_LANG_VAL_TYPE_ARRAY)
    {
        if ((*ret = mln_lang_retExp_createTmpFalse(ctx->pool, NULL)) == NULL) {
            mln_lang_errmsg(ctx, "No memory.");
            return -1;
        }
        return 0;
    }
    mln_lang_var_t *var;
    mln_lang_val_t *val;
    mln_u8_t b = mln_lang_var_toInt(op1->data.var) == mln_lang_var_toInt(op2->data.var);
    if ((val = mln_lang_val_new(ctx->pool, M_LANG_VAL_TYPE_BOOL, &b)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if ((var = mln_lang_var_new(ctx->pool, NULL, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_val_free(val);
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
mln_lang_int_nonequal(mln_lang_ctx_t *ctx, mln_lang_retExp_t **ret, mln_lang_retExp_t *op1, mln_lang_retExp_t *op2)
{
    ASSERT(op1->type == M_LANG_RETEXP_VAR && op2->type == M_LANG_RETEXP_VAR);
    mln_s32_t type = mln_lang_var_getValType(op1->data.var);
    if (type == M_LANG_VAL_TYPE_OBJECT || \
        type == M_LANG_VAL_TYPE_FUNC || \
        type == M_LANG_VAL_TYPE_ARRAY)
    {
        if ((*ret = mln_lang_retExp_createTmpTrue(ctx->pool, NULL)) == NULL) {
            mln_lang_errmsg(ctx, "No memory.");
            return -1;
        }
        return 0;
    }
    mln_lang_var_t *var;
    mln_lang_val_t *val;
    mln_u8_t b = mln_lang_var_toInt(op1->data.var) != mln_lang_var_toInt(op2->data.var);
    if ((val = mln_lang_val_new(ctx->pool, M_LANG_VAL_TYPE_BOOL, &b)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if ((var = mln_lang_var_new(ctx->pool, NULL, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_val_free(val);
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
mln_lang_int_less(mln_lang_ctx_t *ctx, mln_lang_retExp_t **ret, mln_lang_retExp_t *op1, mln_lang_retExp_t *op2)
{
    ASSERT(op1->type == M_LANG_RETEXP_VAR && op2->type == M_LANG_RETEXP_VAR);
    mln_s32_t type = mln_lang_var_getValType(op2->data.var);
    if (type == M_LANG_VAL_TYPE_OBJECT || \
        type == M_LANG_VAL_TYPE_FUNC || \
        type == M_LANG_VAL_TYPE_ARRAY)
    {
        mln_lang_errmsg(ctx, "Operation Not support.");
        return -1;
    }
    mln_lang_var_t *var;
    mln_lang_val_t *val;
    mln_u8_t b;
    if (type == M_LANG_VAL_TYPE_REAL) {
        b = mln_lang_var_toReal(op1->data.var) < mln_lang_var_toReal(op2->data.var);
    } else {
        b = mln_lang_var_toInt(op1->data.var) < mln_lang_var_toInt(op2->data.var);
    }
    if ((val = mln_lang_val_new(ctx->pool, M_LANG_VAL_TYPE_BOOL, &b)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if ((var = mln_lang_var_new(ctx->pool, NULL, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_val_free(val);
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
mln_lang_int_lesseq(mln_lang_ctx_t *ctx, mln_lang_retExp_t **ret, mln_lang_retExp_t *op1, mln_lang_retExp_t *op2)
{
    ASSERT(op1->type == M_LANG_RETEXP_VAR && op2->type == M_LANG_RETEXP_VAR);
    mln_s32_t type = mln_lang_var_getValType(op2->data.var);
    if (type == M_LANG_VAL_TYPE_OBJECT || \
        type == M_LANG_VAL_TYPE_FUNC || \
        type == M_LANG_VAL_TYPE_ARRAY)
    {
        mln_lang_errmsg(ctx, "Operation Not support.");
        return -1;
    }
    mln_lang_var_t *var;
    mln_lang_val_t *val;
    mln_u8_t b;
    if (type == M_LANG_VAL_TYPE_REAL) {
        b = mln_lang_var_toReal(op1->data.var) < mln_lang_var_toReal(op2->data.var);
    } else {
        b = mln_lang_var_toInt(op1->data.var) < mln_lang_var_toInt(op2->data.var);
    }
    if ((val = mln_lang_val_new(ctx->pool, M_LANG_VAL_TYPE_BOOL, &b)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if ((var = mln_lang_var_new(ctx->pool, NULL, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_val_free(val);
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
mln_lang_int_grea(mln_lang_ctx_t *ctx, mln_lang_retExp_t **ret, mln_lang_retExp_t *op1, mln_lang_retExp_t *op2)
{
    ASSERT(op1->type == M_LANG_RETEXP_VAR && op2->type == M_LANG_RETEXP_VAR);
    mln_s32_t type = mln_lang_var_getValType(op2->data.var);
    if (type == M_LANG_VAL_TYPE_OBJECT || \
        type == M_LANG_VAL_TYPE_FUNC || \
        type == M_LANG_VAL_TYPE_ARRAY)
    {
        mln_lang_errmsg(ctx, "Operation Not support.");
        return -1;
    }
    mln_lang_var_t *var;
    mln_lang_val_t *val;
    mln_u8_t b;
    if (type == M_LANG_VAL_TYPE_REAL) {
        b = mln_lang_var_toReal(op1->data.var) < mln_lang_var_toReal(op2->data.var);
    } else {
        b = mln_lang_var_toInt(op1->data.var) < mln_lang_var_toInt(op2->data.var);
    }
    if ((val = mln_lang_val_new(ctx->pool, M_LANG_VAL_TYPE_BOOL, &b)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if ((var = mln_lang_var_new(ctx->pool, NULL, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_val_free(val);
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
mln_lang_int_greale(mln_lang_ctx_t *ctx, mln_lang_retExp_t **ret, mln_lang_retExp_t *op1, mln_lang_retExp_t *op2)
{
    ASSERT(op1->type == M_LANG_RETEXP_VAR && op2->type == M_LANG_RETEXP_VAR);
    mln_s32_t type = mln_lang_var_getValType(op2->data.var);
    if (type == M_LANG_VAL_TYPE_OBJECT || \
        type == M_LANG_VAL_TYPE_FUNC || \
        type == M_LANG_VAL_TYPE_ARRAY)
    {
        mln_lang_errmsg(ctx, "Operation Not support.");
        return -1;
    }
    mln_lang_var_t *var;
    mln_lang_val_t *val;
    mln_u8_t b;
    if (type == M_LANG_VAL_TYPE_REAL) {
        b = mln_lang_var_toReal(op1->data.var) < mln_lang_var_toReal(op2->data.var);
    } else {
        b = mln_lang_var_toInt(op1->data.var) < mln_lang_var_toInt(op2->data.var);
    }
    if ((val = mln_lang_val_new(ctx->pool, M_LANG_VAL_TYPE_BOOL, &b)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if ((var = mln_lang_var_new(ctx->pool, NULL, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_val_free(val);
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
mln_lang_int_lmov(mln_lang_ctx_t *ctx, mln_lang_retExp_t **ret, mln_lang_retExp_t *op1, mln_lang_retExp_t *op2)
{
    ASSERT(op1->type == M_LANG_RETEXP_VAR && op2->type == M_LANG_RETEXP_VAR);
    mln_s32_t type = mln_lang_var_getValType(op2->data.var);
    if (type == M_LANG_VAL_TYPE_OBJECT || \
        type == M_LANG_VAL_TYPE_FUNC || \
        type == M_LANG_VAL_TYPE_ARRAY || \
        type == M_LANG_VAL_TYPE_REAL)
    {
        mln_lang_errmsg(ctx, "Operation Not support.");
        return -1;
    }
    mln_lang_var_t *var;
    mln_lang_val_t *val;
    mln_s64_t i = mln_lang_var_toInt(op1->data.var) << mln_lang_var_toInt(op2->data.var);
    if ((val = mln_lang_val_new(ctx->pool, M_LANG_VAL_TYPE_INT, &i)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if ((var = mln_lang_var_new(ctx->pool, NULL, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_val_free(val);
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
mln_lang_int_rmov(mln_lang_ctx_t *ctx, mln_lang_retExp_t **ret, mln_lang_retExp_t *op1, mln_lang_retExp_t *op2)
{
    ASSERT(op1->type == M_LANG_RETEXP_VAR && op2->type == M_LANG_RETEXP_VAR);
    mln_s32_t type = mln_lang_var_getValType(op2->data.var);
    if (type == M_LANG_VAL_TYPE_OBJECT || \
        type == M_LANG_VAL_TYPE_FUNC || \
        type == M_LANG_VAL_TYPE_ARRAY || \
        type == M_LANG_VAL_TYPE_REAL)
    {
        mln_lang_errmsg(ctx, "Operation Not support.");
        return -1;
    }
    mln_lang_var_t *var;
    mln_lang_val_t *val;
    mln_s64_t i = mln_lang_var_toInt(op1->data.var) >> mln_lang_var_toInt(op2->data.var);
    if ((val = mln_lang_val_new(ctx->pool, M_LANG_VAL_TYPE_INT, &i)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if ((var = mln_lang_var_new(ctx->pool, NULL, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_val_free(val);
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
mln_lang_int_plus(mln_lang_ctx_t *ctx, mln_lang_retExp_t **ret, mln_lang_retExp_t *op1, mln_lang_retExp_t *op2)
{
    ASSERT(op1->type == M_LANG_RETEXP_VAR && op2->type == M_LANG_RETEXP_VAR);
    mln_s32_t type = mln_lang_var_getValType(op2->data.var);
    if (type == M_LANG_VAL_TYPE_OBJECT || \
        type == M_LANG_VAL_TYPE_FUNC || \
        type == M_LANG_VAL_TYPE_ARRAY)
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
    if (type == M_LANG_VAL_TYPE_REAL) {
        double r = mln_lang_var_toReal(op1->data.var) + mln_lang_var_toReal(op2->data.var);
        if ((val = mln_lang_val_new(ctx->pool, M_LANG_VAL_TYPE_REAL, &r)) == NULL) {
            mln_lang_errmsg(ctx, "No memory.");
            return -1;
        }
    } else {
        mln_s64_t i = mln_lang_var_toInt(op1->data.var) + mln_lang_var_toInt(op2->data.var);
        if ((val = mln_lang_val_new(ctx->pool, M_LANG_VAL_TYPE_INT, &i)) == NULL) {
            mln_lang_errmsg(ctx, "No memory.");
            return -1;
        }
    }
    if ((var = mln_lang_var_new(ctx->pool, NULL, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_val_free(val);
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
mln_lang_int_sub(mln_lang_ctx_t *ctx, mln_lang_retExp_t **ret, mln_lang_retExp_t *op1, mln_lang_retExp_t *op2)
{
    ASSERT(op1->type == M_LANG_RETEXP_VAR && op2->type == M_LANG_RETEXP_VAR);
    mln_s32_t type = mln_lang_var_getValType(op2->data.var);
    if (type == M_LANG_VAL_TYPE_OBJECT || \
        type == M_LANG_VAL_TYPE_FUNC || \
        type == M_LANG_VAL_TYPE_ARRAY)
    {
        mln_lang_errmsg(ctx, "Operation Not support.");
        return -1;
    }
    mln_lang_var_t *var;
    mln_lang_val_t *val;
    if (type == M_LANG_VAL_TYPE_REAL) {
        double r = mln_lang_var_toReal(op1->data.var) - mln_lang_var_toReal(op2->data.var);
        if ((val = mln_lang_val_new(ctx->pool, M_LANG_VAL_TYPE_REAL, &r)) == NULL) {
            mln_lang_errmsg(ctx, "No memory.");
            return -1;
        }
    } else {
        mln_s64_t i = mln_lang_var_toInt(op1->data.var) - mln_lang_var_toInt(op2->data.var);
        if ((val = mln_lang_val_new(ctx->pool, M_LANG_VAL_TYPE_INT, &i)) == NULL) {
            mln_lang_errmsg(ctx, "No memory.");
            return -1;
        }
    }
    if ((var = mln_lang_var_new(ctx->pool, NULL, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_val_free(val);
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
mln_lang_int_mul(mln_lang_ctx_t *ctx, mln_lang_retExp_t **ret, mln_lang_retExp_t *op1, mln_lang_retExp_t *op2)
{
    ASSERT(op1->type == M_LANG_RETEXP_VAR && op2->type == M_LANG_RETEXP_VAR);
    mln_s32_t type = mln_lang_var_getValType(op2->data.var);
    if (type == M_LANG_VAL_TYPE_OBJECT || \
        type == M_LANG_VAL_TYPE_FUNC || \
        type == M_LANG_VAL_TYPE_ARRAY)
    {
        mln_lang_errmsg(ctx, "Operation Not support.");
        return -1;
    }
    mln_lang_var_t *var;
    mln_lang_val_t *val;
    if (type == M_LANG_VAL_TYPE_REAL) {
        double r = mln_lang_var_toReal(op1->data.var) * mln_lang_var_toReal(op2->data.var);
        if ((val = mln_lang_val_new(ctx->pool, M_LANG_VAL_TYPE_REAL, &r)) == NULL) {
            mln_lang_errmsg(ctx, "No memory.");
            return -1;
        }
    } else {
        mln_s64_t i = mln_lang_var_toInt(op1->data.var) * mln_lang_var_toInt(op2->data.var);
        if ((val = mln_lang_val_new(ctx->pool, M_LANG_VAL_TYPE_INT, &i)) == NULL) {
            mln_lang_errmsg(ctx, "No memory.");
            return -1;
        }
    }
    if ((var = mln_lang_var_new(ctx->pool, NULL, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_val_free(val);
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
mln_lang_int_div(mln_lang_ctx_t *ctx, mln_lang_retExp_t **ret, mln_lang_retExp_t *op1, mln_lang_retExp_t *op2)
{
    ASSERT(op1->type == M_LANG_RETEXP_VAR && op2->type == M_LANG_RETEXP_VAR);
    mln_s32_t type = mln_lang_var_getValType(op2->data.var);
    if (type == M_LANG_VAL_TYPE_OBJECT || \
        type == M_LANG_VAL_TYPE_FUNC || \
        type == M_LANG_VAL_TYPE_ARRAY)
    {
        mln_lang_errmsg(ctx, "Operation Not support.");
        return -1;
    }
    mln_lang_var_t *var;
    mln_lang_val_t *val;
    if (type == M_LANG_VAL_TYPE_REAL) {
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
    } else {
        mln_s64_t tmp =  mln_lang_var_toInt(op2->data.var);
        if (!tmp) {
            mln_lang_errmsg(ctx, "Division by zero.");
            return -1;
        }
        mln_s64_t i = mln_lang_var_toInt(op1->data.var) / tmp;
        if ((val = mln_lang_val_new(ctx->pool, M_LANG_VAL_TYPE_INT, &i)) == NULL) {
            mln_lang_errmsg(ctx, "No memory.");
            return -1;
        }
    }
    if ((var = mln_lang_var_new(ctx->pool, NULL, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_val_free(val);
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
mln_lang_int_mod(mln_lang_ctx_t *ctx, mln_lang_retExp_t **ret, mln_lang_retExp_t *op1, mln_lang_retExp_t *op2)
{
    ASSERT(op1->type == M_LANG_RETEXP_VAR && op2->type == M_LANG_RETEXP_VAR);
    mln_s32_t type = mln_lang_var_getValType(op2->data.var);
    if (type == M_LANG_VAL_TYPE_OBJECT || \
        type == M_LANG_VAL_TYPE_FUNC || \
        type == M_LANG_VAL_TYPE_ARRAY || \
        type == M_LANG_VAL_TYPE_REAL)
    {
        mln_lang_errmsg(ctx, "Operation Not support.");
        return -1;
    }
    mln_lang_var_t *var;
    mln_lang_val_t *val;
    mln_s64_t tmp =  mln_lang_var_toInt(op2->data.var);
    if (!tmp) {
        mln_lang_errmsg(ctx, "Modulo by zero.");
        return -1;
    }
    mln_s64_t i = mln_lang_var_toInt(op1->data.var) % tmp;
    if ((val = mln_lang_val_new(ctx->pool, M_LANG_VAL_TYPE_INT, &i)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if ((var = mln_lang_var_new(ctx->pool, NULL, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_val_free(val);
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
mln_lang_int_sdec(mln_lang_ctx_t *ctx, mln_lang_retExp_t **ret, mln_lang_retExp_t *op1, mln_lang_retExp_t *op2)
{
    ASSERT(op1->type == M_LANG_RETEXP_VAR);
    mln_lang_var_t *var;
    mln_lang_val_t *val;
    mln_s64_t i = mln_lang_var_toInt(op1->data.var);
    mln_lang_var_setInt(op1->data.var, i-1);
    if ((val = mln_lang_val_new(ctx->pool, M_LANG_VAL_TYPE_INT, &i)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if ((var = mln_lang_var_new(ctx->pool, NULL, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_val_free(val);
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
mln_lang_int_sinc(mln_lang_ctx_t *ctx, mln_lang_retExp_t **ret, mln_lang_retExp_t *op1, mln_lang_retExp_t *op2)
{
    ASSERT(op1->type == M_LANG_RETEXP_VAR);
    mln_lang_var_t *var;
    mln_lang_val_t *val;
    mln_s64_t i = mln_lang_var_toInt(op1->data.var);
    mln_lang_var_setInt(op1->data.var, i+1);
    if ((val = mln_lang_val_new(ctx->pool, M_LANG_VAL_TYPE_INT, &i)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if ((var = mln_lang_var_new(ctx->pool, NULL, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_val_free(val);
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
mln_lang_int_negative(mln_lang_ctx_t *ctx, mln_lang_retExp_t **ret, mln_lang_retExp_t *op1, mln_lang_retExp_t *op2)
{
    ASSERT(op1->type == M_LANG_RETEXP_VAR);
    mln_lang_var_t *var;
    mln_lang_val_t *val;
    mln_s64_t i = -mln_lang_var_toInt(op1->data.var);
    if ((val = mln_lang_val_new(ctx->pool, M_LANG_VAL_TYPE_INT, &i)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if ((var = mln_lang_var_new(ctx->pool, NULL, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_val_free(val);
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
mln_lang_int_reverse(mln_lang_ctx_t *ctx, mln_lang_retExp_t **ret, mln_lang_retExp_t *op1, mln_lang_retExp_t *op2)
{
    ASSERT(op1->type == M_LANG_RETEXP_VAR);
    mln_lang_var_t *var;
    mln_lang_val_t *val;
    mln_s64_t i = ~mln_lang_var_toInt(op1->data.var);
    if ((val = mln_lang_val_new(ctx->pool, M_LANG_VAL_TYPE_INT, &i)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if ((var = mln_lang_var_new(ctx->pool, NULL, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_val_free(val);
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
mln_lang_int_not(mln_lang_ctx_t *ctx, mln_lang_retExp_t **ret, mln_lang_retExp_t *op1, mln_lang_retExp_t *op2)
{
    ASSERT(op1->type == M_LANG_RETEXP_VAR);
    mln_lang_var_t *var;
    mln_lang_val_t *val;
    mln_u8_t b = !mln_lang_var_toInt(op1->data.var);
    if ((val = mln_lang_val_new(ctx->pool, M_LANG_VAL_TYPE_BOOL, &b)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if ((var = mln_lang_var_new(ctx->pool, NULL, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_val_free(val);
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
mln_lang_int_pinc(mln_lang_ctx_t *ctx, mln_lang_retExp_t **ret, mln_lang_retExp_t *op1, mln_lang_retExp_t *op2)
{
    ASSERT(op1->type == M_LANG_RETEXP_VAR);
    mln_lang_var_t *var;
    mln_lang_val_t *val;
    mln_s64_t i = mln_lang_var_toInt(op1->data.var) + 1;
    mln_lang_var_setInt(op1->data.var, i);
    if ((val = mln_lang_val_new(ctx->pool, M_LANG_VAL_TYPE_INT, &i)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if ((var = mln_lang_var_new(ctx->pool, NULL, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_val_free(val);
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
mln_lang_int_pdec(mln_lang_ctx_t *ctx, mln_lang_retExp_t **ret, mln_lang_retExp_t *op1, mln_lang_retExp_t *op2)
{
    ASSERT(op1->type == M_LANG_RETEXP_VAR);
    mln_lang_var_t *var;
    mln_lang_val_t *val;
    mln_s64_t i = mln_lang_var_toInt(op1->data.var) - 1;
    mln_lang_var_setInt(op1->data.var, i);
    if ((val = mln_lang_val_new(ctx->pool, M_LANG_VAL_TYPE_INT, &i)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if ((var = mln_lang_var_new(ctx->pool, NULL, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_val_free(val);
        return -1;
    }
    if ((*ret = mln_lang_retExp_new(ctx->pool, M_LANG_RETEXP_VAR, var)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_var_free(var);
        return -1;
    }
    return 0;
}

