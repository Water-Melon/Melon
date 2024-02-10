
/*
 * Copyright (C) Niklaus F.Schen.
 */
#include "mln_lang_real.h"
#include <stdlib.h>
#include <stdio.h>
#include "mln_func.h"

#ifdef __DEBUG__
#include <assert.h>
#define ASSERT(x) assert(x)
#else
#define ASSERT(x);
#endif

static inline double mln_lang_real_var_toreal(mln_lang_var_t *var);
static int
mln_lang_real_assign(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_real_pluseq(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_real_subeq(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_real_lmoveq(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_real_rmoveq(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_real_muleq(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_real_diveq(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_real_oreq(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_real_andeq(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_real_xoreq(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_real_modeq(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_real_cor(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_real_cand(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_real_cxor(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_real_equal(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_real_nonequal(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_real_less(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_real_lesseq(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_real_grea(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_real_greale(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_real_lmov(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_real_rmov(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_real_plus(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_real_sub(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_real_mul(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_real_div(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_real_mod(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_real_sdec(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_real_sinc(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_real_index(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_real_property(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_real_negative(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_real_reverse(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_real_not(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_real_pinc(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_real_pdec(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);

mln_lang_method_t mln_lang_real_oprs = {
    mln_lang_real_assign,
    mln_lang_real_pluseq,
    mln_lang_real_subeq,
    mln_lang_real_lmoveq,
    mln_lang_real_rmoveq,
    mln_lang_real_muleq,
    mln_lang_real_diveq,
    mln_lang_real_oreq,
    mln_lang_real_andeq,
    mln_lang_real_xoreq,
    mln_lang_real_modeq,
    mln_lang_real_cor,
    mln_lang_real_cand,
    mln_lang_real_cxor,
    mln_lang_real_equal,
    mln_lang_real_nonequal,
    mln_lang_real_less,
    mln_lang_real_lesseq,
    mln_lang_real_grea,
    mln_lang_real_greale,
    mln_lang_real_lmov,
    mln_lang_real_rmov,
    mln_lang_real_plus,
    mln_lang_real_sub,
    mln_lang_real_mul,
    mln_lang_real_div,
    mln_lang_real_mod,
    mln_lang_real_sdec,
    mln_lang_real_sinc,
    mln_lang_real_index,
    mln_lang_real_property,
    mln_lang_real_negative,
    mln_lang_real_reverse,
    mln_lang_real_not,
    mln_lang_real_pinc,
    mln_lang_real_pdec
};

static mln_string_t mln_lang_real_opr_names[] = {
    mln_string("__real_assign_operator__"),
    mln_string("__real_pluseq_operator__"),
    mln_string("__real_subeq_operator__"),
    mln_string("__real_lmoveq_operator__"),
    mln_string("__real_rmoveq_operator__"),
    mln_string("__real_muleq_operator__"),
    mln_string("__real_diveq_operator__"),
    mln_string("__real_oreq_operator__"),
    mln_string("__real_andeq_operator__"),
    mln_string("__real_xoreq_operator__"),
    mln_string("__real_modeq_operator__"),
    mln_string("__real_cor_operator__"),
    mln_string("__real_cand_operator__"),
    mln_string("__real_cxor_operator__"),
    mln_string("__real_equal_operator__"),
    mln_string("__real_nonequal_operator__"),
    mln_string("__real_lt_operator__"),
    mln_string("__real_le_operator__"),
    mln_string("__real_gt_operator__"),
    mln_string("__real_ge_operator__"),
    mln_string("__real_lmov_operator__"),
    mln_string("__real_rmov_operator__"),
    mln_string("__real_plus_operator__"),
    mln_string("__real_sub_operator__"),
    mln_string("__real_mul_operator__"),
    mln_string("__real_div_operator__"),
    mln_string("__real_mod_operator__"),
    mln_string("__real_sdec_operator__"),
    mln_string("__real_sinc_operator__"),
    mln_string("__real_index_operator__"),
    mln_string("__real_property_operator__"),
    mln_string("__real_negative_operator__"),
    mln_string("__real_reverse_operator__"),
    mln_string("__real_not_operator__"),
    mln_string("__real_pinc_operator__"),
    mln_string("__real_pdec_operator__"),
};

MLN_FUNC(static inline, double, mln_lang_real_var_toreal, (mln_lang_var_t *var), (var), {
    ASSERT(var != NULL && var->val != NULL);
    double r = 0;
    mln_lang_val_t *val = var->val;
    switch (val->type) {
        case M_LANG_VAL_TYPE_NIL:
        case M_LANG_VAL_TYPE_OBJECT:
        case M_LANG_VAL_TYPE_FUNC:
        case M_LANG_VAL_TYPE_ARRAY:
        case M_LANG_VAL_TYPE_CALL:
            break;
        case M_LANG_VAL_TYPE_INT:
            r = (double)val->data.i;
            break;
        case M_LANG_VAL_TYPE_BOOL:
            r = val->data.b? 1.0: 0;
            break;
        case M_LANG_VAL_TYPE_REAL:
            r = val->data.f;
            break;
        case M_LANG_VAL_TYPE_STRING:
        {
            mln_string_t *s = val->data.s;
            mln_u8ptr_t buf = (mln_u8ptr_t)malloc(s->len + 1);
            if (buf == NULL) break;
            memcpy(buf, s->data, s->len);
            buf[s->len] = 0;
            sscanf((char *)buf, "%lf", &r);
            free(buf);
            break;
        }
        default:
            ASSERT(0);
            break;
    }
    return r;
})

MLN_FUNC(static, int, mln_lang_real_assign, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_real_flag) {
        int rc = mln_lang_funccall_val_operator(ctx, &mln_lang_real_opr_names[0], ret, op1, op2);
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

MLN_FUNC(static, int, mln_lang_real_pluseq, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_real_flag) {
        int rc = mln_lang_funccall_val_operator(ctx, &mln_lang_real_opr_names[1], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }

    mln_s32_t type = mln_lang_var_val_type_get(op1);
    if (type == M_LANG_VAL_TYPE_OBJECT || \
        type == M_LANG_VAL_TYPE_FUNC || \
        type == M_LANG_VAL_TYPE_ARRAY || \
        type == M_LANG_VAL_TYPE_BOOL || \
        type == M_LANG_VAL_TYPE_NIL)
    {
        mln_lang_errmsg(ctx, "Operation NOT support.");
        return -1;
    }
    if (type == M_LANG_VAL_TYPE_STRING) {
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
    }
    mln_lang_var_set_real(op1, \
                         mln_lang_real_var_toreal(op1) + \
                             mln_lang_real_var_toreal(op2));
    *ret = mln_lang_var_ref(op1);
    return 0;
})

MLN_FUNC(static, int, mln_lang_real_subeq, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_real_flag) {
        int rc = mln_lang_funccall_val_operator(ctx, &mln_lang_real_opr_names[2], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }

    mln_s32_t type = mln_lang_var_val_type_get(op1);
    if (type == M_LANG_VAL_TYPE_OBJECT || \
        type == M_LANG_VAL_TYPE_FUNC || \
        type == M_LANG_VAL_TYPE_ARRAY || \
        type == M_LANG_VAL_TYPE_BOOL || \
        type == M_LANG_VAL_TYPE_NIL || \
        type == M_LANG_VAL_TYPE_STRING)
    {
        mln_lang_errmsg(ctx, "Operation NOT support.");
        return -1;
    }
    mln_lang_var_set_real(op1, \
                         mln_lang_real_var_toreal(op1) - \
                             mln_lang_real_var_toreal(op2));
    *ret = mln_lang_var_ref(op1);
    return 0;
})

MLN_FUNC(static, int, mln_lang_real_lmoveq, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_real_flag) {
        int rc = mln_lang_funccall_val_operator(ctx, &mln_lang_real_opr_names[3], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }
    mln_lang_errmsg(ctx, "Operation NOT support.");
    return -1;
})

MLN_FUNC(static, int, mln_lang_real_rmoveq, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_real_flag) {
        int rc = mln_lang_funccall_val_operator(ctx, &mln_lang_real_opr_names[4], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }
    mln_lang_errmsg(ctx, "Operation NOT support.");
    return -1;
})

MLN_FUNC(static, int, mln_lang_real_muleq, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_real_flag) {
        int rc = mln_lang_funccall_val_operator(ctx, &mln_lang_real_opr_names[5], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }

    mln_s32_t type = mln_lang_var_val_type_get(op1);
    if (type == M_LANG_VAL_TYPE_OBJECT || \
        type == M_LANG_VAL_TYPE_FUNC || \
        type == M_LANG_VAL_TYPE_ARRAY || \
        type == M_LANG_VAL_TYPE_BOOL || \
        type == M_LANG_VAL_TYPE_NIL || \
        type == M_LANG_VAL_TYPE_STRING)
    {
        mln_lang_errmsg(ctx, "Operation NOT support.");
        return -1;
    }
    mln_lang_var_set_real(op1, \
                         mln_lang_real_var_toreal(op1) * \
                             mln_lang_real_var_toreal(op2));
    *ret = mln_lang_var_ref(op1);
    return 0;
})

MLN_FUNC(static, int, mln_lang_real_diveq, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_real_flag) {
        int rc = mln_lang_funccall_val_operator(ctx, &mln_lang_real_opr_names[6], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }

    mln_s32_t type = mln_lang_var_val_type_get(op1);
    if (type == M_LANG_VAL_TYPE_OBJECT || \
        type == M_LANG_VAL_TYPE_FUNC || \
        type == M_LANG_VAL_TYPE_ARRAY || \
        type == M_LANG_VAL_TYPE_BOOL || \
        type == M_LANG_VAL_TYPE_NIL || \
        type == M_LANG_VAL_TYPE_STRING)
    {
        mln_lang_errmsg(ctx, "Operation NOT support.");
        return -1;
    }
    double r = mln_lang_real_var_toreal(op2);
    double tmp = r < 0? -r: r;
    if (tmp <= 1e-15) {
        mln_lang_errmsg(ctx, "Division by zero.");
        return -1;
    }
    mln_lang_var_set_real(op1, mln_lang_real_var_toreal(op1) / r);
    *ret = mln_lang_var_ref(op1);
    return 0;
})

MLN_FUNC(static, int, mln_lang_real_oreq, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_real_flag) {
        int rc = mln_lang_funccall_val_operator(ctx, &mln_lang_real_opr_names[7], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }
    mln_lang_errmsg(ctx, "Operation NOT support.");
    return -1;
})

MLN_FUNC(static, int, mln_lang_real_andeq, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_real_flag) {
        int rc = mln_lang_funccall_val_operator(ctx, &mln_lang_real_opr_names[8], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }
    mln_lang_errmsg(ctx, "Operation NOT support.");
    return -1;
})

MLN_FUNC(static, int, mln_lang_real_xoreq, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_real_flag) {
        int rc = mln_lang_funccall_val_operator(ctx, &mln_lang_real_opr_names[9], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }
    mln_lang_errmsg(ctx, "Operation NOT support.");
    return -1;
})

MLN_FUNC(static, int, mln_lang_real_modeq, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_real_flag) {
        int rc = mln_lang_funccall_val_operator(ctx, &mln_lang_real_opr_names[10], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }
    mln_lang_errmsg(ctx, "Operation NOT support.");
    return -1;
})

MLN_FUNC(static, int, mln_lang_real_cor, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_real_flag) {
        int rc = mln_lang_funccall_val_operator(ctx, &mln_lang_real_opr_names[11], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }
    mln_lang_errmsg(ctx, "Operation NOT support.");
    return -1;
})

MLN_FUNC(static, int, mln_lang_real_cand, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_real_flag) {
        int rc = mln_lang_funccall_val_operator(ctx, &mln_lang_real_opr_names[12], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }
    mln_lang_errmsg(ctx, "Operation NOT support.");
    return -1;
})

MLN_FUNC(static, int, mln_lang_real_cxor, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_real_flag) {
        int rc = mln_lang_funccall_val_operator(ctx, &mln_lang_real_opr_names[13], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }
    mln_lang_errmsg(ctx, "Operation NOT support.");
    return -1;
})

MLN_FUNC(static, int, mln_lang_real_equal, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_real_flag) {
        int rc = mln_lang_funccall_val_operator(ctx, &mln_lang_real_opr_names[14], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }

    mln_s32_t type = mln_lang_var_val_type_get(op2);
    if (type == M_LANG_VAL_TYPE_OBJECT || \
        type == M_LANG_VAL_TYPE_FUNC || \
        type == M_LANG_VAL_TYPE_ARRAY || \
        type == M_LANG_VAL_TYPE_BOOL || \
        type == M_LANG_VAL_TYPE_NIL)
    {
        if ((*ret = mln_lang_var_create_false(ctx, NULL)) == NULL) {
            mln_lang_errmsg(ctx, "No memory.");
            return -1;
        }
        return 0;
    }
    mln_lang_val_t *val;
    mln_u8_t b = mln_lang_real_var_toreal(op1) == mln_lang_real_var_toreal(op2);
    if ((val = mln_lang_val_new(ctx, M_LANG_VAL_TYPE_BOOL, &b)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if ((*ret = mln_lang_var_new(ctx, NULL, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_val_free(val);
        return -1;
    }
    return 0;
})

MLN_FUNC(static, int, mln_lang_real_nonequal, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_real_flag) {
        int rc = mln_lang_funccall_val_operator(ctx, &mln_lang_real_opr_names[15], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }

    mln_s32_t type = mln_lang_var_val_type_get(op2);
    if (type == M_LANG_VAL_TYPE_OBJECT || \
        type == M_LANG_VAL_TYPE_FUNC || \
        type == M_LANG_VAL_TYPE_ARRAY || \
        type == M_LANG_VAL_TYPE_BOOL || \
        type == M_LANG_VAL_TYPE_NIL)
    {
        if ((*ret = mln_lang_var_create_true(ctx, NULL)) == NULL) {
            mln_lang_errmsg(ctx, "No memory.");
            return -1;
        }
        return 0;
    }
    mln_lang_val_t *val;
    mln_u8_t b = mln_lang_real_var_toreal(op1) != mln_lang_real_var_toreal(op2);
    if ((val = mln_lang_val_new(ctx, M_LANG_VAL_TYPE_BOOL, &b)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if ((*ret = mln_lang_var_new(ctx, NULL, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_val_free(val);
        return -1;
    }
    return 0;
})

MLN_FUNC(static, int, mln_lang_real_less, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_real_flag) {
        int rc = mln_lang_funccall_val_operator(ctx, &mln_lang_real_opr_names[16], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }

    mln_lang_val_t *val;
    mln_u8_t b;
    mln_s32_t type = mln_lang_var_val_type_get(op2);
    if (type == M_LANG_VAL_TYPE_OBJECT || \
        type == M_LANG_VAL_TYPE_FUNC || \
        type == M_LANG_VAL_TYPE_ARRAY || \
        type == M_LANG_VAL_TYPE_BOOL || \
        type == M_LANG_VAL_TYPE_NIL)
    {
        mln_lang_errmsg(ctx, "Operation NOT support.");
        return -1;
    }
    if (type == M_LANG_VAL_TYPE_STRING) {
        mln_lang_method_t *method = mln_lang_methods[type];
        if (method == NULL) {
            mln_lang_errmsg(ctx, "Operation NOT support.");
            return -1;
        }
        mln_lang_op handler = method->less_handler;
        if (handler == NULL) {
            mln_lang_errmsg(ctx, "Operation NOT support.");
            return -1;
        }
        return handler(ctx, ret, op1, op2);
    }
    b = mln_lang_real_var_toreal(op1) < mln_lang_real_var_toreal(op2);
    if ((val = mln_lang_val_new(ctx, M_LANG_VAL_TYPE_BOOL, &b)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if ((*ret = mln_lang_var_new(ctx, NULL, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_val_free(val);
        return -1;
    }
    return 0;
})

MLN_FUNC(static, int, mln_lang_real_lesseq, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_real_flag) {
        int rc = mln_lang_funccall_val_operator(ctx, &mln_lang_real_opr_names[17], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }

    mln_lang_val_t *val;
    mln_u8_t b;
    mln_s32_t type = mln_lang_var_val_type_get(op2);
    if (type == M_LANG_VAL_TYPE_OBJECT || \
        type == M_LANG_VAL_TYPE_FUNC || \
        type == M_LANG_VAL_TYPE_ARRAY || \
        type == M_LANG_VAL_TYPE_BOOL || \
        type == M_LANG_VAL_TYPE_NIL)
    {
        mln_lang_errmsg(ctx, "Operation NOT support.");
        return -1;
    }
    if (type == M_LANG_VAL_TYPE_STRING) {
        mln_lang_method_t *method = mln_lang_methods[type];
        if (method == NULL) {
            mln_lang_errmsg(ctx, "Operation NOT support.");
            return -1;
        }
        mln_lang_op handler = method->lesseq_handler;
        if (handler == NULL) {
            mln_lang_errmsg(ctx, "Operation NOT support.");
            return -1;
        }
        return handler(ctx, ret, op1, op2);
    }
    b = mln_lang_real_var_toreal(op1) <= mln_lang_real_var_toreal(op2);
    if ((val = mln_lang_val_new(ctx, M_LANG_VAL_TYPE_BOOL, &b)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if ((*ret = mln_lang_var_new(ctx, NULL, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_val_free(val);
        return -1;
    }
    return 0;
})

MLN_FUNC(static, int, mln_lang_real_grea, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_real_flag) {
        int rc = mln_lang_funccall_val_operator(ctx, &mln_lang_real_opr_names[18], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }

    mln_lang_val_t *val;
    mln_u8_t b;
    mln_s32_t type = mln_lang_var_val_type_get(op2);
    if (type == M_LANG_VAL_TYPE_OBJECT || \
        type == M_LANG_VAL_TYPE_FUNC || \
        type == M_LANG_VAL_TYPE_ARRAY || \
        type == M_LANG_VAL_TYPE_BOOL || \
        type == M_LANG_VAL_TYPE_NIL)
    {
        mln_lang_errmsg(ctx, "Operation NOT support.");
        return -1;
    }
    if (type == M_LANG_VAL_TYPE_STRING) {
        mln_lang_method_t *method = mln_lang_methods[type];
        if (method == NULL) {
            mln_lang_errmsg(ctx, "Operation NOT support.");
            return -1;
        }
        mln_lang_op handler = method->grea_handler;
        if (handler == NULL) {
            mln_lang_errmsg(ctx, "Operation NOT support.");
            return -1;
        }
        return handler(ctx, ret, op1, op2);
    }
    b = mln_lang_real_var_toreal(op1) > mln_lang_real_var_toreal(op2);
    if ((val = mln_lang_val_new(ctx, M_LANG_VAL_TYPE_BOOL, &b)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if ((*ret = mln_lang_var_new(ctx, NULL, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_val_free(val);
        return -1;
    }
    return 0;
})

MLN_FUNC(static, int, mln_lang_real_greale, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_real_flag) {
        int rc = mln_lang_funccall_val_operator(ctx, &mln_lang_real_opr_names[19], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }

    mln_lang_val_t *val;
    mln_u8_t b;
    mln_s32_t type = mln_lang_var_val_type_get(op2);
    if (type == M_LANG_VAL_TYPE_OBJECT || \
        type == M_LANG_VAL_TYPE_FUNC || \
        type == M_LANG_VAL_TYPE_ARRAY || \
        type == M_LANG_VAL_TYPE_BOOL || \
        type == M_LANG_VAL_TYPE_NIL)
    {
        mln_lang_errmsg(ctx, "Operation NOT support.");
        return -1;
    }
    if (type == M_LANG_VAL_TYPE_STRING) {
        mln_lang_method_t *method = mln_lang_methods[type];
        if (method == NULL) {
            mln_lang_errmsg(ctx, "Operation NOT support.");
            return -1;
        }
        mln_lang_op handler = method->greale_handler;
        if (handler == NULL) {
            mln_lang_errmsg(ctx, "Operation NOT support.");
            return -1;
        }
        return handler(ctx, ret, op1, op2);
    }
    b = mln_lang_real_var_toreal(op1) >= mln_lang_real_var_toreal(op2);
    if ((val = mln_lang_val_new(ctx, M_LANG_VAL_TYPE_BOOL, &b)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if ((*ret = mln_lang_var_new(ctx, NULL, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_val_free(val);
        return -1;
    }
    return 0;
})

MLN_FUNC(static, int, mln_lang_real_lmov, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_real_flag) {
        int rc = mln_lang_funccall_val_operator(ctx, &mln_lang_real_opr_names[20], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }
    mln_lang_errmsg(ctx, "Operation NOT support.");
    return -1;
})

MLN_FUNC(static, int, mln_lang_real_rmov, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_real_flag) {
        int rc = mln_lang_funccall_val_operator(ctx, &mln_lang_real_opr_names[21], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }
    mln_lang_errmsg(ctx, "Operation NOT support.");
    return -1;
})

MLN_FUNC(static, int, mln_lang_real_plus, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_real_flag) {
        int rc = mln_lang_funccall_val_operator(ctx, &mln_lang_real_opr_names[22], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }

    mln_s32_t type = mln_lang_var_val_type_get(op2);
    if (type == M_LANG_VAL_TYPE_OBJECT || \
        type == M_LANG_VAL_TYPE_FUNC || \
        type == M_LANG_VAL_TYPE_ARRAY || \
        type == M_LANG_VAL_TYPE_BOOL || \
        type == M_LANG_VAL_TYPE_NIL)
    {
        mln_lang_errmsg(ctx, "Operation NOT support.");
        return -1;
    }
    if (type == M_LANG_VAL_TYPE_STRING) {
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
    }
    mln_lang_val_t *val;
    double r = mln_lang_real_var_toreal(op1) + mln_lang_real_var_toreal(op2);
    if ((val = mln_lang_val_new(ctx, M_LANG_VAL_TYPE_REAL, &r)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if ((*ret = mln_lang_var_new(ctx, NULL, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_val_free(val);
        return -1;
    }
    return 0;
})

MLN_FUNC(static, int, mln_lang_real_sub, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_real_flag) {
        int rc = mln_lang_funccall_val_operator(ctx, &mln_lang_real_opr_names[23], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }

    mln_s32_t type = mln_lang_var_val_type_get(op2);
    if (type == M_LANG_VAL_TYPE_OBJECT || \
        type == M_LANG_VAL_TYPE_FUNC || \
        type == M_LANG_VAL_TYPE_ARRAY || \
        type == M_LANG_VAL_TYPE_BOOL || \
        type == M_LANG_VAL_TYPE_NIL)
    {
        mln_lang_errmsg(ctx, "Operation NOT support.");
        return -1;
    }
    mln_lang_val_t *val;
    double r = mln_lang_real_var_toreal(op1) - mln_lang_real_var_toreal(op2);
    if ((val = mln_lang_val_new(ctx, M_LANG_VAL_TYPE_REAL, &r)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if ((*ret = mln_lang_var_new(ctx, NULL, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_val_free(val);
        return -1;
    }
    return 0;
})

MLN_FUNC(static, int, mln_lang_real_mul, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_real_flag) {
        int rc = mln_lang_funccall_val_operator(ctx, &mln_lang_real_opr_names[24], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }

    mln_s32_t type = mln_lang_var_val_type_get(op2);
    if (type == M_LANG_VAL_TYPE_OBJECT || \
        type == M_LANG_VAL_TYPE_FUNC || \
        type == M_LANG_VAL_TYPE_ARRAY || \
        type == M_LANG_VAL_TYPE_BOOL || \
        type == M_LANG_VAL_TYPE_NIL)
    {
        mln_lang_errmsg(ctx, "Operation NOT support.");
        return -1;
    }
    mln_lang_val_t *val;
    double r = mln_lang_real_var_toreal(op1) * mln_lang_real_var_toreal(op2);
    if ((val = mln_lang_val_new(ctx, M_LANG_VAL_TYPE_REAL, &r)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if ((*ret = mln_lang_var_new(ctx, NULL, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_val_free(val);
        return -1;
    }
    return 0;
})

MLN_FUNC(static, int, mln_lang_real_div, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_real_flag) {
        int rc = mln_lang_funccall_val_operator(ctx, &mln_lang_real_opr_names[25], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }

    mln_s32_t type = mln_lang_var_val_type_get(op2);
    if (type == M_LANG_VAL_TYPE_OBJECT || \
        type == M_LANG_VAL_TYPE_FUNC || \
        type == M_LANG_VAL_TYPE_ARRAY || \
        type == M_LANG_VAL_TYPE_BOOL || \
        type == M_LANG_VAL_TYPE_NIL)
    {
        mln_lang_errmsg(ctx, "Operation NOT support.");
        return -1;
    }
    mln_lang_val_t *val;
    double tmp = mln_lang_real_var_toreal(op2);
    double tmpr = tmp < 0? -tmp: tmp;
    if (tmpr <= 1e-15) {
        mln_lang_errmsg(ctx, "Division by zero.");
        return -1;
    }
    double r = mln_lang_real_var_toreal(op1) / tmp;
    if ((val = mln_lang_val_new(ctx, M_LANG_VAL_TYPE_REAL, &r)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if ((*ret = mln_lang_var_new(ctx, NULL, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_val_free(val);
        return -1;
    }
    return 0;
})

MLN_FUNC(static, int, mln_lang_real_mod, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_real_flag) {
        int rc = mln_lang_funccall_val_operator(ctx, &mln_lang_real_opr_names[26], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }
    mln_lang_errmsg(ctx, "Operation NOT support.");
    return -1;
})

MLN_FUNC(static, int, mln_lang_real_sdec, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_real_flag) {
        int rc = mln_lang_funccall_val_operator(ctx, &mln_lang_real_opr_names[27], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }

    mln_lang_val_t *val;
    double r = mln_lang_real_var_toreal(op1);
    mln_lang_var_set_real(op1, r-1);
    if ((val = mln_lang_val_new(ctx, M_LANG_VAL_TYPE_REAL, &r)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if ((*ret = mln_lang_var_new(ctx, NULL, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_val_free(val);
        return -1;
    }
    return 0;
})

MLN_FUNC(static, int, mln_lang_real_sinc, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_real_flag) {
        int rc = mln_lang_funccall_val_operator(ctx, &mln_lang_real_opr_names[28], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }

    mln_lang_val_t *val;
    double r = mln_lang_real_var_toreal(op1);
    mln_lang_var_set_real(op1, r+1);
    if ((val = mln_lang_val_new(ctx, M_LANG_VAL_TYPE_REAL, &r)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if ((*ret = mln_lang_var_new(ctx, NULL, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_val_free(val);
        return -1;
    }
    return 0;
})

MLN_FUNC(static, int, mln_lang_real_index, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_real_flag) {
        int rc = mln_lang_funccall_val_operator(ctx, &mln_lang_real_opr_names[29], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }
    mln_lang_errmsg(ctx, "Operation NOT support.");
    return -1;
})

MLN_FUNC(static, int, mln_lang_real_property, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_real_flag) {
        int rc = mln_lang_funccall_val_operator(ctx, &mln_lang_real_opr_names[30], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }
    mln_lang_errmsg(ctx, "Operation NOT support.");
    return -1;
})

MLN_FUNC(static, int, mln_lang_real_negative, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_real_flag) {
        int rc = mln_lang_funccall_val_operator(ctx, &mln_lang_real_opr_names[31], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }

    mln_lang_val_t *val;
    double r = -mln_lang_real_var_toreal(op1);
    if ((val = mln_lang_val_new(ctx, M_LANG_VAL_TYPE_REAL, &r)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if ((*ret = mln_lang_var_new(ctx, NULL, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_val_free(val);
        return -1;
    }
    return 0;
})

MLN_FUNC(static, int, mln_lang_real_reverse, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_real_flag) {
        int rc = mln_lang_funccall_val_operator(ctx, &mln_lang_real_opr_names[32], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }
    mln_lang_errmsg(ctx, "Operation NOT support.");
    return -1;
})

MLN_FUNC(static, int, mln_lang_real_not, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_real_flag) {
        int rc = mln_lang_funccall_val_operator(ctx, &mln_lang_real_opr_names[33], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }

    mln_lang_val_t *val;
    mln_u8_t b = !mln_lang_real_var_toreal(op1);
    if ((val = mln_lang_val_new(ctx, M_LANG_VAL_TYPE_BOOL, &b)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if ((*ret = mln_lang_var_new(ctx, NULL, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_val_free(val);
        return -1;
    }
    return 0;
})

MLN_FUNC(static, int, mln_lang_real_pinc, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_real_flag) {
        int rc = mln_lang_funccall_val_operator(ctx, &mln_lang_real_opr_names[34], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }

    mln_lang_val_t *val;
    double r = mln_lang_real_var_toreal(op1) + 1;
    mln_lang_var_set_real(op1, r);
    if ((val = mln_lang_val_new(ctx, M_LANG_VAL_TYPE_REAL, &r)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if ((*ret = mln_lang_var_new(ctx, NULL, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_val_free(val);
        return -1;
    }
    return 0;
})

MLN_FUNC(static, int, mln_lang_real_pdec, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_real_flag) {
        int rc = mln_lang_funccall_val_operator(ctx, &mln_lang_real_opr_names[35], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }

    mln_lang_val_t *val;
    double r = mln_lang_real_var_toreal(op1) - 1;
    mln_lang_var_set_real(op1, r);
    if ((val = mln_lang_val_new(ctx, M_LANG_VAL_TYPE_REAL, &r)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if ((*ret = mln_lang_var_new(ctx, NULL, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_val_free(val);
        return -1;
    }
    return 0;
})

