
/*
 * Copyright (C) Niklaus F.Schen.
 */
#include "mln_lang_str.h"
#include "mln_string.h"
#include <stdio.h>

#ifdef __DEBUG__
#include <assert.h>
#define ASSERT(x) assert(x)
#else
#define ASSERT(x);
#endif

static int
mln_lang_str_assign(mln_lang_ctx_t *ctx, mln_lang_retExp_t **ret, mln_lang_retExp_t *op1, mln_lang_retExp_t *op2);
static int
mln_lang_str_pluseq(mln_lang_ctx_t *ctx, mln_lang_retExp_t **ret, mln_lang_retExp_t *op1, mln_lang_retExp_t *op2);
static int
mln_lang_str_equal(mln_lang_ctx_t *ctx, mln_lang_retExp_t **ret, mln_lang_retExp_t *op1, mln_lang_retExp_t *op2);
static int
mln_lang_str_nonequal(mln_lang_ctx_t *ctx, mln_lang_retExp_t **ret, mln_lang_retExp_t *op1, mln_lang_retExp_t *op2);
static int
mln_lang_str_less(mln_lang_ctx_t *ctx, mln_lang_retExp_t **ret, mln_lang_retExp_t *op1, mln_lang_retExp_t *op2);
static int
mln_lang_str_lesseq(mln_lang_ctx_t *ctx, mln_lang_retExp_t **ret, mln_lang_retExp_t *op1, mln_lang_retExp_t *op2);
static int
mln_lang_str_grea(mln_lang_ctx_t *ctx, mln_lang_retExp_t **ret, mln_lang_retExp_t *op1, mln_lang_retExp_t *op2);
static int
mln_lang_str_greale(mln_lang_ctx_t *ctx, mln_lang_retExp_t **ret, mln_lang_retExp_t *op1, mln_lang_retExp_t *op2);
static int
mln_lang_str_plus(mln_lang_ctx_t *ctx, mln_lang_retExp_t **ret, mln_lang_retExp_t *op1, mln_lang_retExp_t *op2);
static int
mln_lang_str_not(mln_lang_ctx_t *ctx, mln_lang_retExp_t **ret, mln_lang_retExp_t *op1, mln_lang_retExp_t *op2);

mln_lang_method_t mln_lang_str_oprs = {
    mln_lang_str_assign,
    mln_lang_str_pluseq,
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
    mln_lang_str_equal,
    mln_lang_str_nonequal,
    mln_lang_str_less,
    mln_lang_str_lesseq,
    mln_lang_str_grea,
    mln_lang_str_greale,
    NULL,
    NULL,
    mln_lang_str_plus,
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
    mln_lang_str_not,
    NULL,
    NULL
};

static int
mln_lang_str_assign(mln_lang_ctx_t *ctx, mln_lang_retExp_t **ret, mln_lang_retExp_t *op1, mln_lang_retExp_t *op2)
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
mln_lang_str_pluseq(mln_lang_ctx_t *ctx, mln_lang_retExp_t **ret, mln_lang_retExp_t *op1, mln_lang_retExp_t *op2)
{
    ASSERT(op1->type == M_LANG_RETEXP_VAR && op2->type == M_LANG_RETEXP_VAR);
    mln_string_t *s, *tmp;
    mln_lang_var_t *var;
    if ((tmp = mln_lang_var_toString(ctx->pool, op2->data.var)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if ((s = mln_string_pool_strcat(ctx->pool, mln_lang_var_getVal(op1->data.var)->data.s, tmp)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_string_pool_free(tmp);
        return -1;
    }
    mln_string_pool_free(tmp);
    mln_lang_var_setString(op1->data.var, s);
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
mln_lang_str_equal(mln_lang_ctx_t *ctx, mln_lang_retExp_t **ret, mln_lang_retExp_t *op1, mln_lang_retExp_t *op2)
{
    ASSERT(op1->type == M_LANG_RETEXP_VAR && op2->type == M_LANG_RETEXP_VAR);
    int rv;
    mln_string_t *tmp;
    if ((tmp = mln_lang_var_toString(ctx->pool, op2->data.var)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    rv = mln_string_strcmp(mln_lang_var_getVal(op1->data.var)->data.s, tmp);
    mln_string_pool_free(tmp);
    if (rv) {
        if ((*ret = mln_lang_retExp_createTmpFalse(ctx->pool, NULL)) == NULL) {
            mln_lang_errmsg(ctx, "No memory.");
            return -1;
        }
    } else {
        if ((*ret = mln_lang_retExp_createTmpTrue(ctx->pool, NULL)) == NULL) {
            mln_lang_errmsg(ctx, "No memory.");
            return -1;
        }
    }
    return 0;
}

static int
mln_lang_str_nonequal(mln_lang_ctx_t *ctx, mln_lang_retExp_t **ret, mln_lang_retExp_t *op1, mln_lang_retExp_t *op2)
{
    ASSERT(op1->type == M_LANG_RETEXP_VAR && op2->type == M_LANG_RETEXP_VAR);
    int rv;
    mln_string_t *tmp;
    if ((tmp = mln_lang_var_toString(ctx->pool, op2->data.var)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    rv = mln_string_strcmp(mln_lang_var_getVal(op1->data.var)->data.s, tmp);
    mln_string_pool_free(tmp);
    if (rv) {
        if ((*ret = mln_lang_retExp_createTmpTrue(ctx->pool, NULL)) == NULL) {
            mln_lang_errmsg(ctx, "No memory.");
            return -1;
        }
    } else {
        if ((*ret = mln_lang_retExp_createTmpFalse(ctx->pool, NULL)) == NULL) {
            mln_lang_errmsg(ctx, "No memory.");
            return -1;
        }
    }
    return 0;
}

static int
mln_lang_str_less(mln_lang_ctx_t *ctx, mln_lang_retExp_t **ret, mln_lang_retExp_t *op1, mln_lang_retExp_t *op2)
{
    ASSERT(op1->type == M_LANG_RETEXP_VAR && op2->type == M_LANG_RETEXP_VAR);
    int rv;
    mln_string_t *tmp;
    if ((tmp = mln_lang_var_toString(ctx->pool, op2->data.var)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    rv = mln_string_strcmpSeq(mln_lang_var_getVal(op1->data.var)->data.s, tmp);
    mln_string_pool_free(tmp);
    if (rv < 0) {
        if ((*ret = mln_lang_retExp_createTmpTrue(ctx->pool, NULL)) == NULL) {
            mln_lang_errmsg(ctx, "No memory.");
            return -1;
        }
    } else {
        if ((*ret = mln_lang_retExp_createTmpFalse(ctx->pool, NULL)) == NULL) {
            mln_lang_errmsg(ctx, "No memory.");
            return -1;
        }
    }
    return 0;
}

static int
mln_lang_str_lesseq(mln_lang_ctx_t *ctx, mln_lang_retExp_t **ret, mln_lang_retExp_t *op1, mln_lang_retExp_t *op2)
{
    ASSERT(op1->type == M_LANG_RETEXP_VAR && op2->type == M_LANG_RETEXP_VAR);
    int rv;
    mln_string_t *tmp;
    if ((tmp = mln_lang_var_toString(ctx->pool, op2->data.var)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    rv = mln_string_strcmpSeq(mln_lang_var_getVal(op1->data.var)->data.s, tmp);
    mln_string_pool_free(tmp);
    if (rv <= 0) {
        if ((*ret = mln_lang_retExp_createTmpTrue(ctx->pool, NULL)) == NULL) {
            mln_lang_errmsg(ctx, "No memory.");
            return -1;
        }
    } else {
        if ((*ret = mln_lang_retExp_createTmpFalse(ctx->pool, NULL)) == NULL) {
            mln_lang_errmsg(ctx, "No memory.");
            return -1;
        }
    }
    return 0;
}

static int
mln_lang_str_grea(mln_lang_ctx_t *ctx, mln_lang_retExp_t **ret, mln_lang_retExp_t *op1, mln_lang_retExp_t *op2)
{
    ASSERT(op1->type == M_LANG_RETEXP_VAR && op2->type == M_LANG_RETEXP_VAR);
    int rv;
    mln_string_t *tmp;
    if ((tmp = mln_lang_var_toString(ctx->pool, op2->data.var)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    rv = mln_string_strcmpSeq(mln_lang_var_getVal(op1->data.var)->data.s, tmp);
    mln_string_pool_free(tmp);
    if (rv > 0) {
        if ((*ret = mln_lang_retExp_createTmpTrue(ctx->pool, NULL)) == NULL) {
            mln_lang_errmsg(ctx, "No memory.");
            return -1;
        }
    } else {
        if ((*ret = mln_lang_retExp_createTmpFalse(ctx->pool, NULL)) == NULL) {
            mln_lang_errmsg(ctx, "No memory.");
            return -1;
        }
    }
    return 0;
}

static int
mln_lang_str_greale(mln_lang_ctx_t *ctx, mln_lang_retExp_t **ret, mln_lang_retExp_t *op1, mln_lang_retExp_t *op2)
{
    ASSERT(op1->type == M_LANG_RETEXP_VAR && op2->type == M_LANG_RETEXP_VAR);
    int rv;
    mln_string_t *tmp;
    if ((tmp = mln_lang_var_toString(ctx->pool, op2->data.var)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    rv = mln_string_strcmpSeq(mln_lang_var_getVal(op1->data.var)->data.s, tmp);
    mln_string_pool_free(tmp);
    if (rv >= 0) {
        if ((*ret = mln_lang_retExp_createTmpTrue(ctx->pool, NULL)) == NULL) {
            mln_lang_errmsg(ctx, "No memory.");
            return -1;
        }
    } else {
        if ((*ret = mln_lang_retExp_createTmpFalse(ctx->pool, NULL)) == NULL) {
            mln_lang_errmsg(ctx, "No memory.");
            return -1;
        }
    }
    return 0;
}

static int
mln_lang_str_plus(mln_lang_ctx_t *ctx, mln_lang_retExp_t **ret, mln_lang_retExp_t *op1, mln_lang_retExp_t *op2)
{
    ASSERT(op1->type == M_LANG_RETEXP_VAR && op2->type == M_LANG_RETEXP_VAR);
    mln_string_t *s, *tmp1, *tmp2;
    mln_lang_var_t *var;
    mln_lang_val_t *val;
    if ((tmp1 = mln_lang_var_toString(ctx->pool, op1->data.var)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if ((tmp2 = mln_lang_var_toString(ctx->pool, op2->data.var)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_string_pool_free(tmp1);
        return -1;
    }
    if ((s = mln_string_pool_strcat(ctx->pool, tmp1, tmp2)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_string_pool_free(tmp1);
        mln_string_pool_free(tmp2);
        return -1;
    }
    mln_string_pool_free(tmp1);
    mln_string_pool_free(tmp2);
    if ((val = mln_lang_val_new(ctx->pool, M_LANG_VAL_TYPE_STRING, s)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_string_pool_free(s);
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
mln_lang_str_not(mln_lang_ctx_t *ctx, mln_lang_retExp_t **ret, mln_lang_retExp_t *op1, mln_lang_retExp_t *op2)
{
    ASSERT(op1->type == M_LANG_RETEXP_VAR);
    if (mln_lang_condition_isTrue(op1->data.var)) {
        if ((*ret = mln_lang_retExp_createTmpFalse(ctx->pool, NULL)) == NULL) {
            mln_lang_errmsg(ctx, "No memory.");
            return -1;
        }
    } else {
        if ((*ret = mln_lang_retExp_createTmpTrue(ctx->pool, NULL)) == NULL) {
            mln_lang_errmsg(ctx, "No memory.");
            return -1;
        }
    }
    return 0;
}

