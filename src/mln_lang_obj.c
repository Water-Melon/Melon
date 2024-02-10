
/*
 * Copyright (C) Niklaus F.Schen.
 */
#include "mln_lang_obj.h"
#include <stdio.h>
#include "mln_func.h"

#ifdef __DEBUG__
#include <assert.h>
#define ASSERT(x) assert(x)
#else
#define ASSERT(x);
#endif

static int
mln_lang_obj_assign(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_obj_pluseq(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_obj_subeq(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_obj_lmoveq(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_obj_rmoveq(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_obj_muleq(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_obj_diveq(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_obj_oreq(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_obj_andeq(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_obj_xoreq(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_obj_modeq(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_obj_cor(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_obj_cand(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_obj_cxor(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_obj_equal(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_obj_nonequal(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_obj_less(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_obj_lesseq(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_obj_grea(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_obj_greale(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_obj_lmov(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_obj_rmov(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_obj_plus(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_obj_sub(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_obj_mul(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_obj_div(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_obj_mod(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_obj_sdec(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_obj_sinc(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_obj_index(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_obj_property(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_obj_negative(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_obj_reverse(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_obj_not(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_obj_pinc(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_obj_pdec(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);

mln_lang_method_t mln_lang_obj_oprs = {
    mln_lang_obj_assign,
    mln_lang_obj_pluseq,
    mln_lang_obj_subeq,
    mln_lang_obj_lmoveq,
    mln_lang_obj_rmoveq,
    mln_lang_obj_muleq,
    mln_lang_obj_diveq,
    mln_lang_obj_oreq,
    mln_lang_obj_andeq,
    mln_lang_obj_xoreq,
    mln_lang_obj_modeq,
    mln_lang_obj_cor,
    mln_lang_obj_cand,
    mln_lang_obj_cxor,
    mln_lang_obj_equal,
    mln_lang_obj_nonequal,
    mln_lang_obj_less,
    mln_lang_obj_lesseq,
    mln_lang_obj_grea,
    mln_lang_obj_greale,
    mln_lang_obj_lmov,
    mln_lang_obj_rmov,
    mln_lang_obj_plus,
    mln_lang_obj_sub,
    mln_lang_obj_mul,
    mln_lang_obj_div,
    mln_lang_obj_mod,
    mln_lang_obj_sdec,
    mln_lang_obj_sinc,
    mln_lang_obj_index,
    mln_lang_obj_property,
    mln_lang_obj_negative,
    mln_lang_obj_reverse,
    mln_lang_obj_not,
    mln_lang_obj_pinc,
    mln_lang_obj_pdec
};

static mln_string_t mln_lang_obj_opr_names[] = {
    mln_string("__obj_assign_operator__"),
    mln_string("__obj_pluseq_operator__"),
    mln_string("__obj_subeq_operator__"),
    mln_string("__obj_lmoveq_operator__"),
    mln_string("__obj_rmoveq_operator__"),
    mln_string("__obj_muleq_operator__"),
    mln_string("__obj_diveq_operator__"),
    mln_string("__obj_oreq_operator__"),
    mln_string("__obj_andeq_operator__"),
    mln_string("__obj_xoreq_operator__"),
    mln_string("__obj_modeq_operator__"),
    mln_string("__obj_cor_operator__"),
    mln_string("__obj_cand_operator__"),
    mln_string("__obj_cxor_operator__"),
    mln_string("__obj_equal_operator__"),
    mln_string("__obj_nonequal_operator__"),
    mln_string("__obj_lt_operator__"),
    mln_string("__obj_le_operator__"),
    mln_string("__obj_gt_operator__"),
    mln_string("__obj_ge_operator__"),
    mln_string("__obj_lmov_operator__"),
    mln_string("__obj_rmov_operator__"),
    mln_string("__obj_plus_operator__"),
    mln_string("__obj_sub_operator__"),
    mln_string("__obj_mul_operator__"),
    mln_string("__obj_div_operator__"),
    mln_string("__obj_mod_operator__"),
    mln_string("__obj_sdec_operator__"),
    mln_string("__obj_sinc_operator__"),
    mln_string("__obj_index_operator__"),
    mln_string("__obj_property_operator__"),
    mln_string("__obj_negative_operator__"),
    mln_string("__obj_reverse_operator__"),
    mln_string("__obj_not_operator__"),
    mln_string("__obj_pinc_operator__"),
    mln_string("__obj_pdec_operator__"),
};

MLN_FUNC(static, int, mln_lang_obj_assign, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_obj_flag) {
        int rc = mln_lang_funccall_val_obj_operator(ctx, op2, &mln_lang_obj_opr_names[0], ret, op1, op2);
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

MLN_FUNC(static, int, mln_lang_obj_pluseq, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_obj_flag) {
        int rc = mln_lang_funccall_val_obj_operator(ctx, op2, &mln_lang_obj_opr_names[1], ret, op1, op2);
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

MLN_FUNC(static, int, mln_lang_obj_subeq, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_obj_flag) {
        int rc = mln_lang_funccall_val_obj_operator(ctx, op2, &mln_lang_obj_opr_names[2], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }
    mln_lang_errmsg(ctx, "Operation NOT support.");
    return -1;
})

MLN_FUNC(static, int, mln_lang_obj_lmoveq, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_obj_flag) {
        int rc = mln_lang_funccall_val_obj_operator(ctx, op2, &mln_lang_obj_opr_names[3], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }
    mln_lang_errmsg(ctx, "Operation NOT support.");
    return -1;
})

MLN_FUNC(static, int, mln_lang_obj_rmoveq, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_obj_flag) {
        int rc = mln_lang_funccall_val_obj_operator(ctx, op2, &mln_lang_obj_opr_names[4], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }
    mln_lang_errmsg(ctx, "Operation NOT support.");
    return -1;
})

MLN_FUNC(static, int, mln_lang_obj_muleq, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_obj_flag) {
        int rc = mln_lang_funccall_val_obj_operator(ctx, op2, &mln_lang_obj_opr_names[5], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }
    mln_lang_errmsg(ctx, "Operation NOT support.");
    return -1;
})

MLN_FUNC(static, int, mln_lang_obj_diveq, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_obj_flag) {
        int rc = mln_lang_funccall_val_obj_operator(ctx, op2, &mln_lang_obj_opr_names[6], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }
    mln_lang_errmsg(ctx, "Operation NOT support.");
    return -1;
})

MLN_FUNC(static, int, mln_lang_obj_oreq, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_obj_flag) {
        int rc = mln_lang_funccall_val_obj_operator(ctx, op2, &mln_lang_obj_opr_names[7], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }
    mln_lang_errmsg(ctx, "Operation NOT support.");
    return -1;
})

MLN_FUNC(static, int, mln_lang_obj_andeq, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_obj_flag) {
        int rc = mln_lang_funccall_val_obj_operator(ctx, op2, &mln_lang_obj_opr_names[8], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }
    mln_lang_errmsg(ctx, "Operation NOT support.");
    return -1;
})

MLN_FUNC(static, int, mln_lang_obj_xoreq, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_obj_flag) {
        int rc = mln_lang_funccall_val_obj_operator(ctx, op2, &mln_lang_obj_opr_names[9], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }
    mln_lang_errmsg(ctx, "Operation NOT support.");
    return -1;
})

MLN_FUNC(static, int, mln_lang_obj_modeq, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_obj_flag) {
        int rc = mln_lang_funccall_val_obj_operator(ctx, op2, &mln_lang_obj_opr_names[10], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }
    mln_lang_errmsg(ctx, "Operation NOT support.");
    return -1;
})

MLN_FUNC(static, int, mln_lang_obj_cor, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_obj_flag) {
        int rc = mln_lang_funccall_val_obj_operator(ctx, op1, &mln_lang_obj_opr_names[11], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }
    mln_lang_errmsg(ctx, "Operation NOT support.");
    return -1;
})

MLN_FUNC(static, int, mln_lang_obj_cand, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_obj_flag) {
        int rc = mln_lang_funccall_val_obj_operator(ctx, op1, &mln_lang_obj_opr_names[12], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }
    mln_lang_errmsg(ctx, "Operation NOT support.");
    return -1;
})

MLN_FUNC(static, int, mln_lang_obj_cxor, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_obj_flag) {
        int rc = mln_lang_funccall_val_obj_operator(ctx, op1, &mln_lang_obj_opr_names[13], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }
    mln_lang_errmsg(ctx, "Operation NOT support.");
    return -1;
})

MLN_FUNC(static, int, mln_lang_obj_equal, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_obj_flag) {
        int rc = mln_lang_funccall_val_obj_operator(ctx, op1, &mln_lang_obj_opr_names[14], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }

    if (mln_lang_var_val_type_get(op1) != mln_lang_var_val_type_get(op2)) {
f:
        if ((*ret = mln_lang_var_create_false(ctx, NULL)) == NULL) {
            mln_lang_errmsg(ctx, "No memory.");
            return -1;
        }
        return 0;
    }
    if (mln_lang_var_val_get(op1)->data.obj != mln_lang_var_val_get(op2)->data.obj)
        goto f;
    if ((*ret = mln_lang_var_create_true(ctx, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    return 0;
})

MLN_FUNC(static, int, mln_lang_obj_nonequal, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_obj_flag) {
        int rc = mln_lang_funccall_val_obj_operator(ctx, op1, &mln_lang_obj_opr_names[15], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }

    if (mln_lang_var_val_type_get(op1) != mln_lang_var_val_type_get(op2)) {
t:
        if ((*ret = mln_lang_var_create_true(ctx, NULL)) == NULL) {
            mln_lang_errmsg(ctx, "No memory.");
            return -1;
        }
        return 0;
    }
    if (mln_lang_var_val_get(op1)->data.obj != mln_lang_var_val_get(op2)->data.obj)
        goto t;
    if ((*ret = mln_lang_var_create_false(ctx, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    return 0;
})

MLN_FUNC(static, int, mln_lang_obj_less, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_obj_flag) {
        int rc = mln_lang_funccall_val_obj_operator(ctx, op1, &mln_lang_obj_opr_names[16], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }
    mln_lang_errmsg(ctx, "Operation NOT support.");
    return -1;
})

MLN_FUNC(static, int, mln_lang_obj_lesseq, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_obj_flag) {
        int rc = mln_lang_funccall_val_obj_operator(ctx, op1, &mln_lang_obj_opr_names[17], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }
    mln_lang_errmsg(ctx, "Operation NOT support.");
    return -1;
})

MLN_FUNC(static, int, mln_lang_obj_grea, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_obj_flag) {
        int rc = mln_lang_funccall_val_obj_operator(ctx, op1, &mln_lang_obj_opr_names[18], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }
    mln_lang_errmsg(ctx, "Operation NOT support.");
    return -1;
})

MLN_FUNC(static, int, mln_lang_obj_greale, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_obj_flag) {
        int rc = mln_lang_funccall_val_obj_operator(ctx, op1, &mln_lang_obj_opr_names[19], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }
    mln_lang_errmsg(ctx, "Operation NOT support.");
    return -1;
})

MLN_FUNC(static, int, mln_lang_obj_lmov, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_obj_flag) {
        int rc = mln_lang_funccall_val_obj_operator(ctx, op1, &mln_lang_obj_opr_names[20], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }
    mln_lang_errmsg(ctx, "Operation NOT support.");
    return -1;
})

MLN_FUNC(static, int, mln_lang_obj_rmov, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_obj_flag) {
        int rc = mln_lang_funccall_val_obj_operator(ctx, op1, &mln_lang_obj_opr_names[21], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }
    mln_lang_errmsg(ctx, "Operation NOT support.");
    return -1;
})

MLN_FUNC(static, int, mln_lang_obj_plus, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_obj_flag) {
        int rc = mln_lang_funccall_val_obj_operator(ctx, op1, &mln_lang_obj_opr_names[22], ret, op1, op2);
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

MLN_FUNC(static, int, mln_lang_obj_sub, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_obj_flag) {
        int rc = mln_lang_funccall_val_obj_operator(ctx, op1, &mln_lang_obj_opr_names[23], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }
    mln_lang_errmsg(ctx, "Operation NOT support.");
    return -1;
})

MLN_FUNC(static, int, mln_lang_obj_mul, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_obj_flag) {
        int rc = mln_lang_funccall_val_obj_operator(ctx, op1, &mln_lang_obj_opr_names[24], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }
    mln_lang_errmsg(ctx, "Operation NOT support.");
    return -1;
})

MLN_FUNC(static, int, mln_lang_obj_div, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_obj_flag) {
        int rc = mln_lang_funccall_val_obj_operator(ctx, op1, &mln_lang_obj_opr_names[25], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }
    mln_lang_errmsg(ctx, "Operation NOT support.");
    return -1;
})

MLN_FUNC(static, int, mln_lang_obj_mod, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_obj_flag) {
        int rc = mln_lang_funccall_val_obj_operator(ctx, op1, &mln_lang_obj_opr_names[26], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }
    mln_lang_errmsg(ctx, "Operation NOT support.");
    return -1;
})

MLN_FUNC(static, int, mln_lang_obj_sdec, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_obj_flag) {
        int rc = mln_lang_funccall_val_obj_operator(ctx, op1, &mln_lang_obj_opr_names[27], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }
    mln_lang_errmsg(ctx, "Operation NOT support.");
    return -1;
})

MLN_FUNC(static, int, mln_lang_obj_sinc, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_obj_flag) {
        int rc = mln_lang_funccall_val_obj_operator(ctx, op1, &mln_lang_obj_opr_names[28], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }
    mln_lang_errmsg(ctx, "Operation NOT support.");
    return -1;
})

MLN_FUNC(static, int, mln_lang_obj_index, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_obj_flag) {
        int rc = mln_lang_funccall_val_obj_operator(ctx, op1, &mln_lang_obj_opr_names[29], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }
    mln_lang_errmsg(ctx, "Operation NOT support.");
    return -1;
})

MLN_FUNC(static, int, mln_lang_obj_property, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_obj_flag) {
        int rc = mln_lang_funccall_val_obj_operator(ctx, op1, &mln_lang_obj_opr_names[30], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }

    mln_lang_object_t *obj = mln_lang_var_val_get(op1)->data.obj;
    if (mln_lang_var_val_get(op2)->type != M_LANG_VAL_TYPE_CALL) {
        ASSERT(mln_lang_var_val_type_get(op2) == M_LANG_VAL_TYPE_STRING);
        mln_lang_var_t *var = mln_lang_set_member_search(obj->members, mln_lang_var_val_get(op2)->data.s);
        if (var == NULL) {
            mln_lang_var_t tmpvar;
            mln_lang_val_t tmpval;
            mln_rbtree_node_t *rn;
            tmpvar.type = M_LANG_VAR_NORMAL;
            tmpvar.name = mln_lang_var_val_get(op2)->data.s;
            tmpvar.val = &tmpval;
            tmpvar.in_set = obj->in_set;
            tmpvar.prev = tmpvar.next = NULL;
            tmpval.data.s = NULL;
            tmpval.type = M_LANG_VAL_TYPE_NIL;
            tmpval.ref = 1;
            if ((var = mln_lang_var_dup(ctx, &tmpvar)) == NULL) {
                mln_lang_errmsg(ctx, "No memory.");
                return -1;
            }
            if ((rn = mln_rbtree_node_new(obj->members, var)) == NULL) {
                mln_lang_var_free(var);
                mln_lang_errmsg(ctx, "No memory.");
                return -1;
            }
            mln_rbtree_insert(obj->members, rn);
        }
        *ret = mln_lang_var_ref(var);
    } else {
        if ((*ret = mln_lang_var_create_call(ctx, op2->val->data.call)) == NULL) {
            mln_lang_errmsg(ctx, "No memory.");
            return -1;
        }
        mln_lang_funccall_val_object_add(op2->val->data.call, mln_lang_var_val_get(op1));
        op2->val->data.call = NULL;
    }
    return 0;
})

MLN_FUNC(static, int, mln_lang_obj_negative, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_obj_flag) {
        int rc = mln_lang_funccall_val_obj_operator(ctx, op1, &mln_lang_obj_opr_names[31], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }
    mln_lang_errmsg(ctx, "Operation NOT support.");
    return -1;
})

MLN_FUNC(static, int, mln_lang_obj_reverse, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_obj_flag) {
        int rc = mln_lang_funccall_val_obj_operator(ctx, op1, &mln_lang_obj_opr_names[32], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }
    mln_lang_errmsg(ctx, "Operation NOT support.");
    return -1;
})

MLN_FUNC(static, int, mln_lang_obj_not, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_obj_flag) {
        int rc = mln_lang_funccall_val_obj_operator(ctx, op1, &mln_lang_obj_opr_names[33], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }

    ASSERT(op1->val->data.obj != NULL);
    if ((*ret = mln_lang_var_create_false(ctx, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    return 0;
})

MLN_FUNC(static, int, mln_lang_obj_pinc, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_obj_flag) {
        int rc = mln_lang_funccall_val_obj_operator(ctx, op1, &mln_lang_obj_opr_names[34], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }
    mln_lang_errmsg(ctx, "Operation NOT support.");
    return -1;
})

MLN_FUNC(static, int, mln_lang_obj_pdec, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_obj_flag) {
        int rc = mln_lang_funccall_val_obj_operator(ctx, op1, &mln_lang_obj_opr_names[35], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }
    mln_lang_errmsg(ctx, "Operation NOT support.");
    return -1;
})

