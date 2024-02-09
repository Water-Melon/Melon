
/*
 * Copyright (C) Niklaus F.Schen.
 */
#include "mln_lang_array.h"
#include <stdio.h>
#include "mln_func.h"

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
mln_lang_array_subeq(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_array_lmoveq(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_array_rmoveq(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_array_muleq(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_array_diveq(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_array_oreq(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_array_andeq(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_array_xoreq(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_array_modeq(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_array_cor(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_array_cand(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_array_cxor(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_array_equal(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_array_nonequal(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_array_less(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_array_lesseq(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_array_grea(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_array_greale(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_array_lmov(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_array_rmov(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_array_plus(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_array_sub(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_array_mul(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_array_div(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_array_mod(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_array_sdec(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_array_sinc(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_array_index(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_array_property(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_array_negative(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_array_reverse(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_array_not(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_array_pinc(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_array_pdec(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);

mln_lang_method_t mln_lang_array_oprs = {
    mln_lang_array_assign,
    mln_lang_array_pluseq,
    mln_lang_array_subeq,
    mln_lang_array_lmoveq,
    mln_lang_array_rmoveq,
    mln_lang_array_muleq,
    mln_lang_array_diveq,
    mln_lang_array_oreq,
    mln_lang_array_andeq,
    mln_lang_array_xoreq,
    mln_lang_array_modeq,
    mln_lang_array_cor,
    mln_lang_array_cand,
    mln_lang_array_cxor,
    mln_lang_array_equal,
    mln_lang_array_nonequal,
    mln_lang_array_less,
    mln_lang_array_lesseq,
    mln_lang_array_grea,
    mln_lang_array_greale,
    mln_lang_array_lmov,
    mln_lang_array_rmov,
    mln_lang_array_plus,
    mln_lang_array_sub,
    mln_lang_array_mul,
    mln_lang_array_div,
    mln_lang_array_mod,
    mln_lang_array_sdec,
    mln_lang_array_sinc,
    mln_lang_array_index,
    mln_lang_array_property,
    mln_lang_array_negative,
    mln_lang_array_reverse,
    mln_lang_array_not,
    mln_lang_array_pinc,
    mln_lang_array_pdec
};

static mln_string_t mln_lang_array_opr_names[] = {
    mln_string("__array_assign_operator__"),
    mln_string("__array_pluseq_operator__"),
    mln_string("__array_subeq_operator__"),
    mln_string("__array_lmoveq_operator__"),
    mln_string("__array_rmoveq_operator__"),
    mln_string("__array_muleq_operator__"),
    mln_string("__array_diveq_operator__"),
    mln_string("__array_oreq_operator__"),
    mln_string("__array_andeq_operator__"),
    mln_string("__array_xoreq_operator__"),
    mln_string("__array_modeq_operator__"),
    mln_string("__array_cor_operator__"),
    mln_string("__array_cand_operator__"),
    mln_string("__array_cxor_operator__"),
    mln_string("__array_equal_operator__"),
    mln_string("__array_nonequal_operator__"),
    mln_string("__array_lt_operator__"),
    mln_string("__array_le_operator__"),
    mln_string("__array_gt_operator__"),
    mln_string("__array_ge_operator__"),
    mln_string("__array_lmov_operator__"),
    mln_string("__array_rmov_operator__"),
    mln_string("__array_plus_operator__"),
    mln_string("__array_sub_operator__"),
    mln_string("__array_mul_operator__"),
    mln_string("__array_div_operator__"),
    mln_string("__array_mod_operator__"),
    mln_string("__array_sdec_operator__"),
    mln_string("__array_sinc_operator__"),
    mln_string("__array_index_operator__"),
    mln_string("__array_property_operator__"),
    mln_string("__array_negative_operator__"),
    mln_string("__array_reverse_operator__"),
    mln_string("__array_not_operator__"),
    mln_string("__array_pinc_operator__"),
    mln_string("__array_pdec_operator__"),
};

MLN_FUNC(static, int, mln_lang_array_assign, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_array_flag) {
        int rc = mln_lang_funccall_val_operator(ctx, &mln_lang_array_opr_names[0], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }

    if (mln_lang_var_value_set(ctx, op1, op2) < 0) {
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    *ret = mln_lang_var_ref(op1);
    return 0;
})

MLN_FUNC(static, int, mln_lang_array_pluseq, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_array_flag) {
        int rc = mln_lang_funccall_val_operator(ctx, &mln_lang_array_opr_names[1], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }

    mln_s32_t type = mln_lang_var_val_type_get(op1);
    if (type != M_LANG_VAL_TYPE_STRING) {
        mln_lang_errmsg(ctx, "Operation NOT support.");
        return -1;
    }
    mln_lang_method_t *method = mln_lang_methods[type];
    if (method == NULL) {
        mln_lang_errmsg(ctx, "Operation NOT support.");
        return -1;
    }
    mln_lang_op handler = method->pluseq_handler;
    if (handler == NULL) {
        mln_lang_errmsg(ctx, "Operation NOT support.");
        return -1;
    }
    return handler(ctx, ret, op1, op2);
})

MLN_FUNC(static, int, mln_lang_array_subeq, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_array_flag) {
        int rc = mln_lang_funccall_val_operator(ctx, &mln_lang_array_opr_names[2], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }
    mln_lang_errmsg(ctx, "Operation NOT support.");
    return -1;
})

MLN_FUNC(static, int, mln_lang_array_lmoveq, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_array_flag) {
        int rc = mln_lang_funccall_val_operator(ctx, &mln_lang_array_opr_names[3], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }
    mln_lang_errmsg(ctx, "Operation NOT support.");
    return -1;
})

MLN_FUNC(static, int, mln_lang_array_rmoveq, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_array_flag) {
        int rc = mln_lang_funccall_val_operator(ctx, &mln_lang_array_opr_names[4], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }
    mln_lang_errmsg(ctx, "Operation NOT support.");
    return -1;
})

MLN_FUNC(static, int, mln_lang_array_muleq, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_array_flag) {
        int rc = mln_lang_funccall_val_operator(ctx, &mln_lang_array_opr_names[5], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }
    mln_lang_errmsg(ctx, "Operation NOT support.");
    return -1;
})

MLN_FUNC(static, int, mln_lang_array_diveq, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_array_flag) {
        int rc = mln_lang_funccall_val_operator(ctx, &mln_lang_array_opr_names[6], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }
    mln_lang_errmsg(ctx, "Operation NOT support.");
    return -1;
})

MLN_FUNC(static, int, mln_lang_array_oreq, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_array_flag) {
        int rc = mln_lang_funccall_val_operator(ctx, &mln_lang_array_opr_names[7], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }
    mln_lang_errmsg(ctx, "Operation NOT support.");
    return -1;
})

MLN_FUNC(static, int, mln_lang_array_andeq, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_array_flag) {
        int rc = mln_lang_funccall_val_operator(ctx, &mln_lang_array_opr_names[8], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }
    mln_lang_errmsg(ctx, "Operation NOT support.");
    return -1;
})

MLN_FUNC(static, int, mln_lang_array_xoreq, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_array_flag) {
        int rc = mln_lang_funccall_val_operator(ctx, &mln_lang_array_opr_names[9], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }
    mln_lang_errmsg(ctx, "Operation NOT support.");
    return -1;
})

MLN_FUNC(static, int, mln_lang_array_modeq, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_array_flag) {
        int rc = mln_lang_funccall_val_operator(ctx, &mln_lang_array_opr_names[10], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }
    mln_lang_errmsg(ctx, "Operation NOT support.");
    return -1;
})

MLN_FUNC(static, int, mln_lang_array_cor, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_array_flag) {
        int rc = mln_lang_funccall_val_operator(ctx, &mln_lang_array_opr_names[11], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }
    mln_lang_errmsg(ctx, "Operation NOT support.");
    return -1;
})

MLN_FUNC(static, int, mln_lang_array_cand, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_array_flag) {
        int rc = mln_lang_funccall_val_operator(ctx, &mln_lang_array_opr_names[12], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }
    mln_lang_errmsg(ctx, "Operation NOT support.");
    return -1;
})

MLN_FUNC(static, int, mln_lang_array_cxor, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_array_flag) {
        int rc = mln_lang_funccall_val_operator(ctx, &mln_lang_array_opr_names[13], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }
    mln_lang_errmsg(ctx, "Operation NOT support.");
    return -1;
})

MLN_FUNC(static, int, mln_lang_array_equal, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_array_flag) {
        int rc = mln_lang_funccall_val_operator(ctx, &mln_lang_array_opr_names[14], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }

    mln_s32_t type = mln_lang_var_val_type_get(op2);
    if (type == mln_lang_var_val_type_get(op1) && \
        mln_lang_var_val_get(op1)->data.array == mln_lang_var_val_get(op2)->data.array)
    {
        if ((*ret = mln_lang_var_create_true(ctx, NULL)) == NULL) {
            mln_lang_errmsg(ctx, "No memory.");
            return -1;
        }
    } else {
        if ((*ret = mln_lang_var_create_false(ctx, NULL)) == NULL) {
            mln_lang_errmsg(ctx, "No memory.");
            return -1;
        }
    }
    return 0;
})

MLN_FUNC(static, int, mln_lang_array_nonequal, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_array_flag) {
        int rc = mln_lang_funccall_val_operator(ctx, &mln_lang_array_opr_names[15], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }

    mln_s32_t type = mln_lang_var_val_type_get(op2);
    if (type != mln_lang_var_val_type_get(op1) || \
        mln_lang_var_val_get(op1)->data.array != mln_lang_var_val_get(op2)->data.array)
    {
        if ((*ret = mln_lang_var_create_true(ctx, NULL)) == NULL) {
            mln_lang_errmsg(ctx, "No memory.");
            return -1;
        }
    } else {
        if ((*ret = mln_lang_var_create_false(ctx, NULL)) == NULL) {
            mln_lang_errmsg(ctx, "No memory.");
            return -1;
        }
    }
    return 0;
})

MLN_FUNC(static, int, mln_lang_array_less, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_array_flag) {
        int rc = mln_lang_funccall_val_operator(ctx, &mln_lang_array_opr_names[16], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }
    mln_lang_errmsg(ctx, "Operation NOT support.");
    return -1;
})

MLN_FUNC(static, int, mln_lang_array_lesseq, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_array_flag) {
        int rc = mln_lang_funccall_val_operator(ctx, &mln_lang_array_opr_names[17], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }
    mln_lang_errmsg(ctx, "Operation NOT support.");
    return -1;
})

MLN_FUNC(static, int, mln_lang_array_grea, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_array_flag) {
        int rc = mln_lang_funccall_val_operator(ctx, &mln_lang_array_opr_names[18], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }
    mln_lang_errmsg(ctx, "Operation NOT support.");
    return -1;
})

MLN_FUNC(static, int, mln_lang_array_greale, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_array_flag) {
        int rc = mln_lang_funccall_val_operator(ctx, &mln_lang_array_opr_names[19], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }
    mln_lang_errmsg(ctx, "Operation NOT support.");
    return -1;
})

MLN_FUNC(static, int, mln_lang_array_lmov, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_array_flag) {
        int rc = mln_lang_funccall_val_operator(ctx, &mln_lang_array_opr_names[20], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }
    mln_lang_errmsg(ctx, "Operation NOT support.");
    return -1;
})

MLN_FUNC(static, int, mln_lang_array_rmov, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_array_flag) {
        int rc = mln_lang_funccall_val_operator(ctx, &mln_lang_array_opr_names[21], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }
    mln_lang_errmsg(ctx, "Operation NOT support.");
    return -1;
})

MLN_FUNC(static, int, mln_lang_array_plus, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_array_flag) {
        int rc = mln_lang_funccall_val_operator(ctx, &mln_lang_array_opr_names[22], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }

    if (mln_lang_var_val_type_get(op2) != M_LANG_VAL_TYPE_STRING) {
        mln_lang_errmsg(ctx, "Operation NOT support.");
        return -1;
    }
    mln_lang_method_t *method = mln_lang_methods[M_LANG_VAL_TYPE_STRING];
    if (method == NULL) {
        mln_lang_errmsg(ctx, "Operation NOT support.");
        return -1;
    }
    mln_lang_op handler = method->plus_handler;
    if (handler == NULL) {
        mln_lang_errmsg(ctx, "Operation NOT support.");
        return -1;
    }
    return handler(ctx, ret, op1, op2);
})

MLN_FUNC(static, int, mln_lang_array_sub, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_array_flag) {
        int rc = mln_lang_funccall_val_operator(ctx, &mln_lang_array_opr_names[23], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }
    mln_lang_errmsg(ctx, "Operation NOT support.");
    return -1;
})

MLN_FUNC(static, int, mln_lang_array_mul, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_array_flag) {
        int rc = mln_lang_funccall_val_operator(ctx, &mln_lang_array_opr_names[24], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }
    mln_lang_errmsg(ctx, "Operation NOT support.");
    return -1;
})

MLN_FUNC(static, int, mln_lang_array_div, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_array_flag) {
        int rc = mln_lang_funccall_val_operator(ctx, &mln_lang_array_opr_names[25], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }
    mln_lang_errmsg(ctx, "Operation NOT support.");
    return -1;
})

MLN_FUNC(static, int, mln_lang_array_mod, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_array_flag) {
        int rc = mln_lang_funccall_val_operator(ctx, &mln_lang_array_opr_names[26], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }
    mln_lang_errmsg(ctx, "Operation NOT support.");
    return -1;
})

MLN_FUNC(static, int, mln_lang_array_sdec, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_array_flag) {
        int rc = mln_lang_funccall_val_operator(ctx, &mln_lang_array_opr_names[27], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }
    mln_lang_errmsg(ctx, "Operation NOT support.");
    return -1;
})

MLN_FUNC(static, int, mln_lang_array_sinc, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_array_flag) {
        int rc = mln_lang_funccall_val_operator(ctx, &mln_lang_array_opr_names[28], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }
    mln_lang_errmsg(ctx, "Operation NOT support.");
    return -1;
})

MLN_FUNC(static, int, mln_lang_array_index, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_array_flag) {
        int rc = mln_lang_funccall_val_operator(ctx, &mln_lang_array_opr_names[29], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }

    mln_lang_array_t *array = mln_lang_var_val_get(op1)->data.array;
    mln_lang_var_t *rv;
    mln_s64_t save = ~((mln_s64_t)0);
    if (mln_lang_var_val_type_get(op2) == M_LANG_VAL_TYPE_INT && mln_lang_var_val_get(op2)->data.i < 0) {
        save = mln_lang_var_val_get(op2)->data.i;
        mln_lang_var_val_get(op2)->data.i += array->index;
        if (mln_lang_var_val_get(op2)->data.i < 0) {
            mln_lang_errmsg(ctx, "Invalid offset");
            return -1;
        }
    }
    if ((rv = mln_lang_array_get(ctx, array, op2)) == NULL) {
        return -1;
    }
    if (save != ~((mln_s64_t)0)) {
        mln_lang_var_val_get(op2)->data.i = save;
    }
    *ret = mln_lang_var_ref(rv);
    return 0;
})

MLN_FUNC(static, int, mln_lang_array_property, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_array_flag) {
        int rc = mln_lang_funccall_val_operator(ctx, &mln_lang_array_opr_names[30], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }
    mln_lang_errmsg(ctx, "Operation NOT support.");
    return -1;
})

static int
mln_lang_array_negative(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2)
{
    if (ctx->op_array_flag) {
        int rc = mln_lang_funccall_val_operator(ctx, &mln_lang_array_opr_names[31], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }
    mln_lang_errmsg(ctx, "Operation NOT support.");
    return -1;
}

MLN_FUNC(static, int, mln_lang_array_reverse, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_array_flag) {
        int rc = mln_lang_funccall_val_operator(ctx, &mln_lang_array_opr_names[32], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }
    mln_lang_errmsg(ctx, "Operation NOT support.");
    return -1;
})

MLN_FUNC(static, int, mln_lang_array_not, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_array_flag) {
        int rc = mln_lang_funccall_val_operator(ctx, &mln_lang_array_opr_names[33], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }

    if (mln_lang_condition_is_true(op1)) {
        if ((*ret = mln_lang_var_create_false(ctx, NULL)) == NULL) {
            mln_lang_errmsg(ctx, "No memory.");
            return -1;
        }
    } else {
        if ((*ret = mln_lang_var_create_true(ctx, NULL)) == NULL) {
            mln_lang_errmsg(ctx, "No memory.");
            return -1;
        }
    }
    return 0;
})

MLN_FUNC(static, int, mln_lang_array_pinc, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_array_flag) {
        int rc = mln_lang_funccall_val_operator(ctx, &mln_lang_array_opr_names[34], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }
    mln_lang_errmsg(ctx, "Operation NOT support.");
    return -1;
})

MLN_FUNC(static, int, mln_lang_array_pdec, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_array_flag) {
        int rc = mln_lang_funccall_val_operator(ctx, &mln_lang_array_opr_names[35], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }
    mln_lang_errmsg(ctx, "Operation NOT support.");
    return -1;
})

