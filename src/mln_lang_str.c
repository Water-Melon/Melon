
/*
 * Copyright (C) Niklaus F.Schen.
 */
#include "mln_lang_str.h"
#include "mln_string.h"
#include <stdio.h>
#include "mln_func.h"

#ifdef __DEBUG__
#include <assert.h>
#define ASSERT(x) assert(x)
#else
#define ASSERT(x);
#endif

static inline mln_string_t *__mln_lang_str_var_tostring(mln_alloc_t *pool, mln_lang_var_t *var);
static int
mln_lang_str_assign(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_str_pluseq(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_str_subeq(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_str_lmoveq(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_str_rmoveq(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_str_muleq(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_str_diveq(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_str_oreq(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_str_andeq(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_str_xoreq(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_str_modeq(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_str_cor(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_str_cand(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_str_cxor(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_str_equal(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_str_nonequal(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_str_less(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_str_lesseq(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_str_grea(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_str_greale(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_str_lmov(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_str_rmov(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_str_plus(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_str_sub(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_str_mul(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_str_div(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_str_mod(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_str_sdec(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_str_sinc(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_str_index(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_str_property(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_str_negative(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_str_reverse(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_str_not(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_str_pinc(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_str_pdec(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);

mln_lang_method_t mln_lang_str_oprs = {
    mln_lang_str_assign,
    mln_lang_str_pluseq,
    mln_lang_str_subeq,
    mln_lang_str_lmoveq,
    mln_lang_str_rmoveq,
    mln_lang_str_muleq,
    mln_lang_str_diveq,
    mln_lang_str_oreq,
    mln_lang_str_andeq,
    mln_lang_str_xoreq,
    mln_lang_str_modeq,
    mln_lang_str_cor,
    mln_lang_str_cand,
    mln_lang_str_cxor,
    mln_lang_str_equal,
    mln_lang_str_nonequal,
    mln_lang_str_less,
    mln_lang_str_lesseq,
    mln_lang_str_grea,
    mln_lang_str_greale,
    mln_lang_str_lmov,
    mln_lang_str_rmov,
    mln_lang_str_plus,
    mln_lang_str_sub,
    mln_lang_str_mul,
    mln_lang_str_div,
    mln_lang_str_mod,
    mln_lang_str_sdec,
    mln_lang_str_sinc,
    mln_lang_str_index,
    mln_lang_str_property,
    mln_lang_str_negative,
    mln_lang_str_reverse,
    mln_lang_str_not,
    mln_lang_str_pinc,
    mln_lang_str_pdec
};

static mln_string_t mln_lang_str_opr_names[] = {
    mln_string("__str_assign_operator__"),
    mln_string("__str_pluseq_operator__"),
    mln_string("__str_subeq_operator__"),
    mln_string("__str_lmoveq_operator__"),
    mln_string("__str_rmoveq_operator__"),
    mln_string("__str_muleq_operator__"),
    mln_string("__str_diveq_operator__"),
    mln_string("__str_oreq_operator__"),
    mln_string("__str_andeq_operator__"),
    mln_string("__str_xoreq_operator__"),
    mln_string("__str_modeq_operator__"),
    mln_string("__str_cor_operator__"),
    mln_string("__str_cand_operator__"),
    mln_string("__str_cxor_operator__"),
    mln_string("__str_equal_operator__"),
    mln_string("__str_nonequal_operator__"),
    mln_string("__str_lt_operator__"),
    mln_string("__str_le_operator__"),
    mln_string("__str_gt_operator__"),
    mln_string("__str_ge_operator__"),
    mln_string("__str_lmov_operator__"),
    mln_string("__str_rmov_operator__"),
    mln_string("__str_plus_operator__"),
    mln_string("__str_sub_operator__"),
    mln_string("__str_mul_operator__"),
    mln_string("__str_div_operator__"),
    mln_string("__str_mod_operator__"),
    mln_string("__str_sdec_operator__"),
    mln_string("__str_sinc_operator__"),
    mln_string("__str_index_operator__"),
    mln_string("__str_property_operator__"),
    mln_string("__str_negative_operator__"),
    mln_string("__str_reverse_operator__"),
    mln_string("__str_not_operator__"),
    mln_string("__str_pinc_operator__"),
    mln_string("__str_pdec_operator__"),
};

MLN_FUNC(, mln_string_t *, mln_lang_str_var_tostring, \
         (mln_alloc_t *pool, mln_lang_var_t *var), (pool, var), \
{
    return __mln_lang_str_var_tostring(pool, var);
})

static inline mln_string_t *__mln_lang_str_var_tostring(mln_alloc_t *pool, mln_lang_var_t *var)
{
    ASSERT(var != NULL && var->val != NULL);
    char buf[1024] = {0};
    mln_lang_val_t *val = var->val;
    int n = 0;
    switch (val->type) {
        case M_LANG_VAL_TYPE_NIL:
            n = snprintf(buf, sizeof(buf)-1, "nil");
            break;
        case M_LANG_VAL_TYPE_OBJECT:
            n = snprintf(buf, sizeof(buf)-1, "Object");
            break;
        case M_LANG_VAL_TYPE_FUNC:
            n = snprintf(buf, sizeof(buf)-1, "Function");
            break;
        case M_LANG_VAL_TYPE_ARRAY:
            n = snprintf(buf, sizeof(buf)-1, "Array");
            break;
        case M_LANG_VAL_TYPE_INT:
#if defined(MSVC) || defined(i386) || defined(__arm__) || defined(__wasm__)
            n = snprintf(buf, sizeof(buf)-1, "%lld", val->data.i);
#else
            n = snprintf(buf, sizeof(buf)-1, "%ld", val->data.i);
#endif
            break;
        case M_LANG_VAL_TYPE_BOOL:
            n = snprintf(buf, sizeof(buf)-1, "%s", val->data.b?"true":"false");
            break;
        case M_LANG_VAL_TYPE_REAL:
            n = snprintf(buf, sizeof(buf)-1, "%lf", val->data.f);
            break;
        case M_LANG_VAL_TYPE_STRING:
        {
            mln_string_t *s = mln_string_ref(val->data.s);
            return s;
        }
        default:
            ASSERT(0);
            break;
    }
    mln_string_t tmp;
    mln_string_nset(&tmp, buf, n);
    return mln_string_pool_dup(pool, &tmp);
}

MLN_FUNC(static, int, mln_lang_str_assign, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_str_flag) {
        int rc = mln_lang_funccall_val_operator(ctx, &mln_lang_str_opr_names[0], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }

    if (mln_lang_var_value_set_string_ref(ctx, op1, op2) < 0) {
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    *ret = mln_lang_var_ref(op1);
    return 0;
})

MLN_FUNC(static, int, mln_lang_str_pluseq, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_str_flag) {
        int rc = mln_lang_funccall_val_operator(ctx, &mln_lang_str_opr_names[1], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }

    mln_string_t *s, *tmp1, *tmp2;
    if ((tmp1 = __mln_lang_str_var_tostring(ctx->pool, op1)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if ((tmp2 = __mln_lang_str_var_tostring(ctx->pool, op2)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_string_free(tmp1);
        return -1;
    }
    if ((s = mln_string_pool_strcat(ctx->pool, tmp1, tmp2)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_string_free(tmp1);
        mln_string_free(tmp2);
        return -1;
    }
    mln_string_free(tmp1);
    mln_string_free(tmp2);
    mln_lang_var_set_string(op1, s);
    *ret = mln_lang_var_ref(op1);
    return 0;
})

MLN_FUNC(static, int, mln_lang_str_subeq, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_str_flag) {
        int rc = mln_lang_funccall_val_operator(ctx, &mln_lang_str_opr_names[2], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }
    mln_lang_errmsg(ctx, "Operation NOT support.");
    return -1;
})

MLN_FUNC(static, int, mln_lang_str_lmoveq, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_str_flag) {
        int rc = mln_lang_funccall_val_operator(ctx, &mln_lang_str_opr_names[3], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }
    mln_lang_errmsg(ctx, "Operation NOT support.");
    return -1;
})

MLN_FUNC(static, int, mln_lang_str_rmoveq, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_str_flag) {
        int rc = mln_lang_funccall_val_operator(ctx, &mln_lang_str_opr_names[4], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }
    mln_lang_errmsg(ctx, "Operation NOT support.");
    return -1;
})

MLN_FUNC(static, int, mln_lang_str_muleq, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_str_flag) {
        int rc = mln_lang_funccall_val_operator(ctx, &mln_lang_str_opr_names[5], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }
    mln_lang_errmsg(ctx, "Operation NOT support.");
    return -1;
})

MLN_FUNC(static, int, mln_lang_str_diveq, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_str_flag) {
        int rc = mln_lang_funccall_val_operator(ctx, &mln_lang_str_opr_names[6], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }
    mln_lang_errmsg(ctx, "Operation NOT support.");
    return -1;
})

MLN_FUNC(static, int, mln_lang_str_oreq, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_str_flag) {
        int rc = mln_lang_funccall_val_operator(ctx, &mln_lang_str_opr_names[7], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }
    mln_lang_errmsg(ctx, "Operation NOT support.");
    return -1;
})

MLN_FUNC(static, int, mln_lang_str_andeq, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_str_flag) {
        int rc = mln_lang_funccall_val_operator(ctx, &mln_lang_str_opr_names[8], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }
    mln_lang_errmsg(ctx, "Operation NOT support.");
    return -1;
})

MLN_FUNC(static, int, mln_lang_str_xoreq, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_str_flag) {
        int rc = mln_lang_funccall_val_operator(ctx, &mln_lang_str_opr_names[9], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }
    mln_lang_errmsg(ctx, "Operation NOT support.");
    return -1;
})

MLN_FUNC(static, int, mln_lang_str_modeq, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_str_flag) {
        int rc = mln_lang_funccall_val_operator(ctx, &mln_lang_str_opr_names[10], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }
    mln_lang_errmsg(ctx, "Operation NOT support.");
    return -1;
})

MLN_FUNC(static, int, mln_lang_str_cor, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_str_flag) {
        int rc = mln_lang_funccall_val_operator(ctx, &mln_lang_str_opr_names[11], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }
    mln_lang_errmsg(ctx, "Operation NOT support.");
    return -1;
})

MLN_FUNC(static, int, mln_lang_str_cand, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_str_flag) {
        int rc = mln_lang_funccall_val_operator(ctx, &mln_lang_str_opr_names[12], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }
    mln_lang_errmsg(ctx, "Operation NOT support.");
    return -1;
})

MLN_FUNC(static, int, mln_lang_str_cxor, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_str_flag) {
        int rc = mln_lang_funccall_val_operator(ctx, &mln_lang_str_opr_names[13], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }
    mln_lang_errmsg(ctx, "Operation NOT support.");
    return -1;
})

MLN_FUNC(static, int, mln_lang_str_equal, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_str_flag) {
        int rc = mln_lang_funccall_val_operator(ctx, &mln_lang_str_opr_names[14], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }

    int rv;
    mln_string_t *tmp;
    mln_s32_t type = mln_lang_var_val_type_get(op2);

    if (type != M_LANG_VAL_TYPE_INT && type != M_LANG_VAL_TYPE_REAL && type != M_LANG_VAL_TYPE_STRING) {
        if ((*ret = mln_lang_var_create_false(ctx, NULL)) == NULL) {
            mln_lang_errmsg(ctx, "No memory.");
            return -1;
        }
        return 0;
    }
    if ((tmp = __mln_lang_str_var_tostring(ctx->pool, op2)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    rv = mln_string_strcmp(mln_lang_var_val_get(op1)->data.s, tmp);
    mln_string_free(tmp);
    if (rv) {
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

MLN_FUNC(static, int, mln_lang_str_nonequal, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_str_flag) {
        int rc = mln_lang_funccall_val_operator(ctx, &mln_lang_str_opr_names[15], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }

    int rv;
    mln_string_t *tmp;
    mln_s32_t type = mln_lang_var_val_type_get(op2);

    if (type != M_LANG_VAL_TYPE_INT && type != M_LANG_VAL_TYPE_REAL && type != M_LANG_VAL_TYPE_STRING) {
        if ((*ret = mln_lang_var_create_true(ctx, NULL)) == NULL) {
            mln_lang_errmsg(ctx, "No memory.");
            return -1;
        }
        return 0;
    }
    if ((tmp = __mln_lang_str_var_tostring(ctx->pool, op2)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    rv = mln_string_strcmp(mln_lang_var_val_get(op1)->data.s, tmp);
    mln_string_free(tmp);
    if (rv) {
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

MLN_FUNC(static, int, mln_lang_str_less, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_str_flag) {
        int rc = mln_lang_funccall_val_operator(ctx, &mln_lang_str_opr_names[16], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }

    int rv;
    mln_string_t *tmp1, *tmp2;
    mln_s32_t type = mln_lang_var_val_type_get(op2);

    if (type != M_LANG_VAL_TYPE_INT && type != M_LANG_VAL_TYPE_REAL && type != M_LANG_VAL_TYPE_STRING) {
        mln_lang_errmsg(ctx, "Operation NOT support.");
        return -1;
    }
    if ((tmp1 = __mln_lang_str_var_tostring(ctx->pool, op1)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if ((tmp2 = __mln_lang_str_var_tostring(ctx->pool, op2)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_string_free(tmp1);
        return -1;
    }
    rv = mln_string_strseqcmp(tmp1, tmp2);
    mln_string_free(tmp1);
    mln_string_free(tmp2);
    if (rv < 0) {
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

MLN_FUNC(static, int, mln_lang_str_lesseq, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_str_flag) {
        int rc = mln_lang_funccall_val_operator(ctx, &mln_lang_str_opr_names[17], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }

    int rv;
    mln_string_t *tmp1, *tmp2;
    mln_s32_t type = mln_lang_var_val_type_get(op2);

    if (type != M_LANG_VAL_TYPE_INT && type != M_LANG_VAL_TYPE_REAL && type != M_LANG_VAL_TYPE_STRING) {
        mln_lang_errmsg(ctx, "Operation NOT support.");
        return -1;
    }
    if ((tmp1 = __mln_lang_str_var_tostring(ctx->pool, op1)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if ((tmp2 = __mln_lang_str_var_tostring(ctx->pool, op2)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_string_free(tmp1);
        return -1;
    }
    rv = mln_string_strseqcmp(tmp1, tmp2);
    mln_string_free(tmp1);
    mln_string_free(tmp2);
    if (rv <= 0) {
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

MLN_FUNC(static, int, mln_lang_str_grea, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_str_flag) {
        int rc = mln_lang_funccall_val_operator(ctx, &mln_lang_str_opr_names[18], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }

    int rv;
    mln_string_t *tmp1, *tmp2;
    mln_s32_t type = mln_lang_var_val_type_get(op2);

    if (type != M_LANG_VAL_TYPE_INT && type != M_LANG_VAL_TYPE_REAL && type != M_LANG_VAL_TYPE_STRING) {
        mln_lang_errmsg(ctx, "Operation NOT support.");
        return -1;
    }
    if ((tmp1 = __mln_lang_str_var_tostring(ctx->pool, op1)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if ((tmp2 = __mln_lang_str_var_tostring(ctx->pool, op2)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_string_free(tmp1);
        return -1;
    }
    rv = mln_string_strseqcmp(tmp1, tmp2);
    mln_string_free(tmp1);
    mln_string_free(tmp2);
    if (rv > 0) {
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

MLN_FUNC(static, int, mln_lang_str_greale, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_str_flag) {
        int rc = mln_lang_funccall_val_operator(ctx, &mln_lang_str_opr_names[19], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }

    int rv;
    mln_string_t *tmp1, *tmp2;
    mln_s32_t type = mln_lang_var_val_type_get(op2);

    if (type != M_LANG_VAL_TYPE_INT && type != M_LANG_VAL_TYPE_REAL && type != M_LANG_VAL_TYPE_STRING) {
        mln_lang_errmsg(ctx, "Operation NOT support.");
        return -1;
    }
    if ((tmp1 = __mln_lang_str_var_tostring(ctx->pool, op1)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if ((tmp2 = __mln_lang_str_var_tostring(ctx->pool, op2)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_string_free(tmp1);
        return -1;
    }
    rv = mln_string_strseqcmp(tmp1, tmp2);
    mln_string_free(tmp1);
    mln_string_free(tmp2);
    if (rv >= 0) {
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

MLN_FUNC(static, int, mln_lang_str_lmov, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_str_flag) {
        int rc = mln_lang_funccall_val_operator(ctx, &mln_lang_str_opr_names[20], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }
    mln_lang_errmsg(ctx, "Operation NOT support.");
    return -1;
})

MLN_FUNC(static, int, mln_lang_str_rmov, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_str_flag) {
        int rc = mln_lang_funccall_val_operator(ctx, &mln_lang_str_opr_names[21], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }
    mln_lang_errmsg(ctx, "Operation NOT support.");
    return -1;
})

MLN_FUNC(static, int, mln_lang_str_plus, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_str_flag) {
        int rc = mln_lang_funccall_val_operator(ctx, &mln_lang_str_opr_names[22], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }

    mln_string_t *s, *tmp1, *tmp2;
    mln_lang_val_t *val;
    if ((tmp1 = __mln_lang_str_var_tostring(ctx->pool, op1)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if ((tmp2 = __mln_lang_str_var_tostring(ctx->pool, op2)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_string_free(tmp1);
        return -1;
    }
    if ((s = mln_string_pool_strcat(ctx->pool, tmp1, tmp2)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_string_free(tmp1);
        mln_string_free(tmp2);
        return -1;
    }
    mln_string_free(tmp1);
    mln_string_free(tmp2);
    if ((val = mln_lang_val_new(ctx, M_LANG_VAL_TYPE_STRING, s)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_string_free(s);
        return -1;
    }
    if ((*ret = mln_lang_var_new(ctx, NULL, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_val_free(val);
        return -1;
    }
    return 0;
})

MLN_FUNC(static, int, mln_lang_str_sub, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_str_flag) {
        int rc = mln_lang_funccall_val_operator(ctx, &mln_lang_str_opr_names[23], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }
    mln_lang_errmsg(ctx, "Operation NOT support.");
    return -1;
})

MLN_FUNC(static, int, mln_lang_str_mul, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_str_flag) {
        int rc = mln_lang_funccall_val_operator(ctx, &mln_lang_str_opr_names[24], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }
    mln_lang_errmsg(ctx, "Operation NOT support.");
    return -1;
})

MLN_FUNC(static, int, mln_lang_str_div, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_str_flag) {
        int rc = mln_lang_funccall_val_operator(ctx, &mln_lang_str_opr_names[25], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }
    mln_lang_errmsg(ctx, "Operation NOT support.");
    return -1;
})

MLN_FUNC(static, int, mln_lang_str_mod, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_str_flag) {
        int rc = mln_lang_funccall_val_operator(ctx, &mln_lang_str_opr_names[26], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }
    mln_lang_errmsg(ctx, "Operation NOT support.");
    return -1;
})

MLN_FUNC(static, int, mln_lang_str_sdec, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_str_flag) {
        int rc = mln_lang_funccall_val_operator(ctx, &mln_lang_str_opr_names[27], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }
    mln_lang_errmsg(ctx, "Operation NOT support.");
    return -1;
})

MLN_FUNC(static, int, mln_lang_str_sinc, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_str_flag) {
        int rc = mln_lang_funccall_val_operator(ctx, &mln_lang_str_opr_names[28], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }
    mln_lang_errmsg(ctx, "Operation NOT support.");
    return -1;
})

MLN_FUNC(static, int, mln_lang_str_index, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_str_flag) {
        int rc = mln_lang_funccall_val_operator(ctx, &mln_lang_str_opr_names[29], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }

    mln_s64_t offset;
    mln_string_t c;
    mln_string_t *s = mln_lang_var_val_get(op1)->data.s;
    if (mln_lang_var_val_type_get(op2) != M_LANG_VAL_TYPE_INT) {
        mln_lang_errmsg(ctx, "Offset must be an integer.");
        return -1;
    }
    offset = mln_lang_var_val_get(op2)->data.i;
    if (offset < 0) {
        offset = (mln_s64_t)(s->len) + offset;
    }
    if (offset < 0 || offset >= s->len) {
        mln_lang_errmsg(ctx, "Invalid offset.");
        return -1;
    }
    mln_string_nset(&c, &(s->data[offset]), 1);
    if ((*ret = mln_lang_var_create_string(ctx, &c, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    return 0;
})

MLN_FUNC(static, int, mln_lang_str_property, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_str_flag) {
        int rc = mln_lang_funccall_val_operator(ctx, &mln_lang_str_opr_names[30], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }
    mln_lang_errmsg(ctx, "Operation NOT support.");
    return -1;
})

MLN_FUNC(static, int, mln_lang_str_negative, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_str_flag) {
        int rc = mln_lang_funccall_val_operator(ctx, &mln_lang_str_opr_names[31], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }
    mln_lang_errmsg(ctx, "Operation NOT support.");
    return -1;
})

MLN_FUNC(static, int, mln_lang_str_reverse, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_str_flag) {
        int rc = mln_lang_funccall_val_operator(ctx, &mln_lang_str_opr_names[32], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }
    mln_lang_errmsg(ctx, "Operation NOT support.");
    return -1;
})

MLN_FUNC(static, int, mln_lang_str_not, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_str_flag) {
        int rc = mln_lang_funccall_val_operator(ctx, &mln_lang_str_opr_names[33], ret, op1, op2);
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

MLN_FUNC(static, int, mln_lang_str_pinc, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_str_flag) {
        int rc = mln_lang_funccall_val_operator(ctx, &mln_lang_str_opr_names[34], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }
    mln_lang_errmsg(ctx, "Operation NOT support.");
    return -1;
})

MLN_FUNC(static, int, mln_lang_str_pdec, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, ret, op1, op2), \
{
    if (ctx->op_str_flag) {
        int rc = mln_lang_funccall_val_operator(ctx, &mln_lang_str_opr_names[35], ret, op1, op2);
        if (rc < 0) return rc;
        if (rc > 0) return 0;
    }
    mln_lang_errmsg(ctx, "Operation NOT support.");
    return -1;
})

