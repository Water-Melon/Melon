
/*
 * Copyright (C) Niklaus F.Schen.
 */
#include "mln_lang_bool.h"
#include <stdio.h>
#include "mln_func.h"

#ifdef __DEBUG__
#include <assert.h>
#define ASSERT(x) assert(x)
#else
#define ASSERT(x);
#endif

static int
mln_lang_bool_assign(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_bool_pluseq(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_bool_subeq(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_bool_lmoveq(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_bool_rmoveq(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_bool_muleq(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_bool_diveq(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_bool_oreq(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_bool_andeq(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_bool_xoreq(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_bool_modeq(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_bool_cor(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_bool_cand(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_bool_cxor(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_bool_equal(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_bool_nonequal(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_bool_less(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_bool_lesseq(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_bool_grea(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_bool_greale(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_bool_lmov(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_bool_rmov(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_bool_plus(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_bool_sub(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_bool_mul(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_bool_div(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_bool_mod(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_bool_sdec(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_bool_sinc(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_bool_index(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_bool_property(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_bool_negative(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_bool_reverse(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_bool_not(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_bool_pinc(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_bool_pdec(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);

mln_lang_method_t mln_lang_bool_oprs = {
    mln_lang_bool_assign,
    mln_lang_bool_pluseq,
    mln_lang_bool_subeq,
    mln_lang_bool_lmoveq,
    mln_lang_bool_rmoveq,
    mln_lang_bool_muleq,
    mln_lang_bool_diveq,
    mln_lang_bool_oreq,
    mln_lang_bool_andeq,
    mln_lang_bool_xoreq,
    mln_lang_bool_modeq,
    mln_lang_bool_cor,
    mln_lang_bool_cand,
    mln_lang_bool_cxor,
    mln_lang_bool_equal,
    mln_lang_bool_nonequal,
    mln_lang_bool_less,
    mln_lang_bool_lesseq,
    mln_lang_bool_grea,
    mln_lang_bool_greale,
    mln_lang_bool_lmov,
    mln_lang_bool_rmov,
    mln_lang_bool_plus,
    mln_lang_bool_sub,
    mln_lang_bool_mul,
    mln_lang_bool_div,
    mln_lang_bool_mod,
    mln_lang_bool_sdec,
    mln_lang_bool_sinc,
    mln_lang_bool_index,
    mln_lang_bool_property,
    mln_lang_bool_negative,
    mln_lang_bool_reverse,
    mln_lang_bool_not,
    mln_lang_bool_pinc,
    mln_lang_bool_pdec
};

static mln_string_t mln_lang_bool_opr_names[] = {
    mln_string("__bool_assign_operator__"),
    mln_string("__bool_pluseq_operator__"),
    mln_string("__bool_subeq_operator__"),
    mln_string("__bool_lmoveq_operator__"),
    mln_string("__bool_rmoveq_operator__"),
    mln_string("__bool_muleq_operator__"),
    mln_string("__bool_diveq_operator__"),
    mln_string("__bool_oreq_operator__"),
    mln_string("__bool_andeq_operator__"),
    mln_string("__bool_xoreq_operator__"),
    mln_string("__bool_modeq_operator__"),
    mln_string("__bool_cor_operator__"),
    mln_string("__bool_cand_operator__"),
    mln_string("__bool_cxor_operator__"),
    mln_string("__bool_equal_operator__"),
    mln_string("__bool_nonequal_operator__"),
    mln_string("__bool_lt_operator__"),
    mln_string("__bool_le_operator__"),
    mln_string("__bool_gt_operator__"),
    mln_string("__bool_ge_operator__"),
    mln_string("__bool_lmov_operator__"),
    mln_string("__bool_rmov_operator__"),
    mln_string("__bool_plus_operator__"),
    mln_string("__bool_sub_operator__"),
    mln_string("__bool_mul_operator__"),
    mln_string("__bool_div_operator__"),
    mln_string("__bool_mod_operator__"),
    mln_string("__bool_sdec_operator__"),
    mln_string("__bool_sinc_operator__"),
    mln_string("__bool_index_operator__"),
    mln_string("__bool_property_operator__"),
    mln_string("__bool_negative_operator__"),
    mln_string("__bool_reverse_operator__"),
    mln_string("__bool_not_operator__"),
    mln_string("__bool_pinc_operator__"),
    mln_string("__bool_pdec_operator__"),
};

MLN_FUNC(static, int, mln_lang_bool_assign, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_bool_flag) {
        int rc = mln_lang_funccall_val_operator(ctx, &mln_lang_bool_opr_names[0], ret, op1, op2);
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

MLN_FUNC(static, int, mln_lang_bool_pluseq, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_bool_flag) {
        int rc = mln_lang_funccall_val_operator(ctx, &mln_lang_bool_opr_names[1], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }

    mln_s32_t type = mln_lang_var_val_type_get(op1);
    if (type == M_LANG_VAL_TYPE_BOOL) {
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

MLN_FUNC(static, int, mln_lang_bool_subeq, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_bool_flag) {
        int rc = mln_lang_funccall_val_operator(ctx, &mln_lang_bool_opr_names[2], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }
    mln_lang_errmsg(ctx, "Operation NOT support.");
    return -1;
})

MLN_FUNC(static, int, mln_lang_bool_lmoveq, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_bool_flag) {
        int rc = mln_lang_funccall_val_operator(ctx, &mln_lang_bool_opr_names[3], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }
    mln_lang_errmsg(ctx, "Operation NOT support.");
    return -1;
})

MLN_FUNC(static, int, mln_lang_bool_rmoveq, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_bool_flag) {
        int rc = mln_lang_funccall_val_operator(ctx, &mln_lang_bool_opr_names[4], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }
    mln_lang_errmsg(ctx, "Operation NOT support.");
    return -1;
})

MLN_FUNC(static, int, mln_lang_bool_muleq, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_bool_flag) {
        int rc = mln_lang_funccall_val_operator(ctx, &mln_lang_bool_opr_names[5], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }
    mln_lang_errmsg(ctx, "Operation NOT support.");
    return -1;
})

MLN_FUNC(static, int, mln_lang_bool_diveq, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_bool_flag) {
        int rc = mln_lang_funccall_val_operator(ctx, &mln_lang_bool_opr_names[6], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }
    mln_lang_errmsg(ctx, "Operation NOT support.");
    return -1;
})

MLN_FUNC(static, int, mln_lang_bool_oreq, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_bool_flag) {
        int rc = mln_lang_funccall_val_operator(ctx, &mln_lang_bool_opr_names[7], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }
    mln_lang_errmsg(ctx, "Operation NOT support.");
    return -1;
})

MLN_FUNC(static, int, mln_lang_bool_andeq, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_bool_flag) {
        int rc = mln_lang_funccall_val_operator(ctx, &mln_lang_bool_opr_names[8], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }
    mln_lang_errmsg(ctx, "Operation NOT support.");
    return -1;
})

MLN_FUNC(static, int, mln_lang_bool_xoreq, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_bool_flag) {
        int rc = mln_lang_funccall_val_operator(ctx, &mln_lang_bool_opr_names[9], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }
    mln_lang_errmsg(ctx, "Operation NOT support.");
    return -1;
})

MLN_FUNC(static, int, mln_lang_bool_modeq, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_bool_flag) {
        int rc = mln_lang_funccall_val_operator(ctx, &mln_lang_bool_opr_names[10], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }
    mln_lang_errmsg(ctx, "Operation NOT support.");
    return -1;
})

MLN_FUNC(static, int, mln_lang_bool_cor, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_bool_flag) {
        int rc = mln_lang_funccall_val_operator(ctx, &mln_lang_bool_opr_names[11], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }
    mln_lang_errmsg(ctx, "Operation NOT support.");
    return -1;
})

MLN_FUNC(static, int, mln_lang_bool_cand, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_bool_flag) {
        int rc = mln_lang_funccall_val_operator(ctx, &mln_lang_bool_opr_names[12], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }
    mln_lang_errmsg(ctx, "Operation NOT support.");
    return -1;
})

MLN_FUNC(static, int, mln_lang_bool_cxor, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_bool_flag) {
        int rc = mln_lang_funccall_val_operator(ctx, &mln_lang_bool_opr_names[13], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }
    mln_lang_errmsg(ctx, "Operation NOT support.");
    return -1;
})

MLN_FUNC(static, int, mln_lang_bool_equal, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_bool_flag) {
        int rc = mln_lang_funccall_val_operator(ctx, &mln_lang_bool_opr_names[14], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }

    if (mln_lang_var_val_type_get(op1) != mln_lang_var_val_type_get(op2) || \
        mln_lang_var_val_get(op1)->data.b != mln_lang_var_val_get(op2)->data.b)
    {
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

MLN_FUNC(static, int, mln_lang_bool_nonequal, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_bool_flag) {
        int rc = mln_lang_funccall_val_operator(ctx, &mln_lang_bool_opr_names[15], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }

    if (mln_lang_var_val_type_get(op1) != mln_lang_var_val_type_get(op2) || \
        mln_lang_var_val_get(op1)->data.b != mln_lang_var_val_get(op2)->data.b)
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

MLN_FUNC(static, int, mln_lang_bool_less, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_bool_flag) {
        int rc = mln_lang_funccall_val_operator(ctx, &mln_lang_bool_opr_names[16], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }
    mln_lang_errmsg(ctx, "Operation NOT support.");
    return -1;
})

MLN_FUNC(static, int, mln_lang_bool_lesseq, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_bool_flag) {
        int rc = mln_lang_funccall_val_operator(ctx, &mln_lang_bool_opr_names[17], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }
    mln_lang_errmsg(ctx, "Operation NOT support.");
    return -1;
})

MLN_FUNC(static, int, mln_lang_bool_grea, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_bool_flag) {
        int rc = mln_lang_funccall_val_operator(ctx, &mln_lang_bool_opr_names[18], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }
    mln_lang_errmsg(ctx, "Operation NOT support.");
    return -1;
})

MLN_FUNC(static, int, mln_lang_bool_greale, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_bool_flag) {
        int rc = mln_lang_funccall_val_operator(ctx, &mln_lang_bool_opr_names[19], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }
    mln_lang_errmsg(ctx, "Operation NOT support.");
    return -1;
})

MLN_FUNC(static, int, mln_lang_bool_lmov, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_bool_flag) {
        int rc = mln_lang_funccall_val_operator(ctx, &mln_lang_bool_opr_names[20], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }
    mln_lang_errmsg(ctx, "Operation NOT support.");
    return -1;
})

MLN_FUNC(static, int, mln_lang_bool_rmov, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_bool_flag) {
        int rc = mln_lang_funccall_val_operator(ctx, &mln_lang_bool_opr_names[21], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }
    mln_lang_errmsg(ctx, "Operation NOT support.");
    return -1;
})

MLN_FUNC(static, int, mln_lang_bool_plus, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_bool_flag) {
        int rc = mln_lang_funccall_val_operator(ctx, &mln_lang_bool_opr_names[22], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }

    mln_s32_t type = mln_lang_var_val_type_get(op2);
    if (type != M_LANG_VAL_TYPE_STRING) {
        mln_lang_errmsg(ctx, "Operation NOT support.");
        return -1;
    }
    mln_lang_method_t *method = mln_lang_methods[type];
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

MLN_FUNC(static, int, mln_lang_bool_sub, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_bool_flag) {
        int rc = mln_lang_funccall_val_operator(ctx, &mln_lang_bool_opr_names[23], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }
    mln_lang_errmsg(ctx, "Operation NOT support.");
    return -1;
})

MLN_FUNC(static, int, mln_lang_bool_mul, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_bool_flag) {
        int rc = mln_lang_funccall_val_operator(ctx, &mln_lang_bool_opr_names[24], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }
    mln_lang_errmsg(ctx, "Operation NOT support.");
    return -1;
})

MLN_FUNC(static, int, mln_lang_bool_div, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_bool_flag) {
        int rc = mln_lang_funccall_val_operator(ctx, &mln_lang_bool_opr_names[25], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }
    mln_lang_errmsg(ctx, "Operation NOT support.");
    return -1;
})

MLN_FUNC(static, int, mln_lang_bool_mod, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_bool_flag) {
        int rc = mln_lang_funccall_val_operator(ctx, &mln_lang_bool_opr_names[26], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }
    mln_lang_errmsg(ctx, "Operation NOT support.");
    return -1;
})

MLN_FUNC(static, int, mln_lang_bool_sdec, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_bool_flag) {
        int rc = mln_lang_funccall_val_operator(ctx, &mln_lang_bool_opr_names[27], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }
    mln_lang_errmsg(ctx, "Operation NOT support.");
    return -1;
})

MLN_FUNC(static, int, mln_lang_bool_sinc, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_bool_flag) {
        int rc = mln_lang_funccall_val_operator(ctx, &mln_lang_bool_opr_names[28], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }
    mln_lang_errmsg(ctx, "Operation NOT support.");
    return -1;
})

MLN_FUNC(static, int, mln_lang_bool_index, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_bool_flag) {
        int rc = mln_lang_funccall_val_operator(ctx, &mln_lang_bool_opr_names[29], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }
    mln_lang_errmsg(ctx, "Operation NOT support.");
    return -1;
})

MLN_FUNC(static, int, mln_lang_bool_property, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_bool_flag) {
        int rc = mln_lang_funccall_val_operator(ctx, &mln_lang_bool_opr_names[30], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }
    mln_lang_errmsg(ctx, "Operation NOT support.");
    return -1;
})

MLN_FUNC(static, int, mln_lang_bool_negative, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_bool_flag) {
        int rc = mln_lang_funccall_val_operator(ctx, &mln_lang_bool_opr_names[31], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }
    mln_lang_errmsg(ctx, "Operation NOT support.");
    return -1;
})

MLN_FUNC(static, int, mln_lang_bool_reverse, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_bool_flag) {
        int rc = mln_lang_funccall_val_operator(ctx, &mln_lang_bool_opr_names[32], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }
    mln_lang_errmsg(ctx, "Operation NOT support.");
    return -1;
})

MLN_FUNC(static, int, mln_lang_bool_not, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_bool_flag) {
        int rc = mln_lang_funccall_val_operator(ctx, &mln_lang_bool_opr_names[33], ret, op1, op2);
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

MLN_FUNC(static, int, mln_lang_bool_pinc, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_bool_flag) {
        int rc = mln_lang_funccall_val_operator(ctx, &mln_lang_bool_opr_names[34], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }
    mln_lang_errmsg(ctx, "Operation NOT support.");
    return -1;
})

MLN_FUNC(static, int, mln_lang_bool_pdec, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_bool_flag) {
        int rc = mln_lang_funccall_val_operator(ctx, &mln_lang_bool_opr_names[35], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }
    mln_lang_errmsg(ctx, "Operation NOT support.");
    return -1;
})

