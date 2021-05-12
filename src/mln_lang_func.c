
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
mln_lang_func_assign(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_func_pluseq(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_func_equal(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_func_nonequal(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_func_plus(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);
static int
mln_lang_func_not(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2);

mln_lang_method_t mln_lang_func_oprs = {
    mln_lang_func_assign,
    mln_lang_func_pluseq,
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
mln_lang_func_assign(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2)
{
    if (mln_lang_var_value_set(ctx, op1, op2) < 0) {
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    *ret = mln_lang_var_ref(op1);
    return 0;
}

static int
mln_lang_func_equal(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2)
{
    mln_u8ptr_t data1, data2;
    mln_lang_func_detail_t *f1, *f2;
    if (mln_lang_var_val_type_get(op1) != mln_lang_var_val_type_get(op2)) {
f:
        if ((*ret = mln_lang_var_create_false(ctx, NULL)) == NULL) {
            mln_lang_errmsg(ctx, "No memory.");
            return -1;
        }
        return 0;
    }
    f1 = mln_lang_var_val_get(op1)->data.func;
    f2 = mln_lang_var_val_get(op2)->data.func;
    if (f1->type != f2->type) goto f;
    data1 = f1->type == M_FUNC_INTERNAL? (mln_u8ptr_t)f1->data.process: (mln_u8ptr_t)f1->data.stm;
    data2 = f2->type == M_FUNC_INTERNAL? (mln_u8ptr_t)f2->data.process: (mln_u8ptr_t)f2->data.stm;
    if (data1 != data2) goto f;
    if ((*ret = mln_lang_var_create_true(ctx, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    return 0;
}

static int
mln_lang_func_pluseq(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2)
{
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
}

static int
mln_lang_func_nonequal(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2)
{
    mln_u8ptr_t data1, data2;
    mln_lang_func_detail_t *f1, *f2;
    if (mln_lang_var_val_type_get(op1) != mln_lang_var_val_type_get(op2)) {
t:
        if ((*ret = mln_lang_var_create_true(ctx, NULL)) == NULL) {
            mln_lang_errmsg(ctx, "No memory.");
            return -1;
        }
        return 0;
    }
    f1 = mln_lang_var_val_get(op1)->data.func;
    f2 = mln_lang_var_val_get(op2)->data.func;
    if (f1->type != f2->type) goto t;
    data1 = f1->type == M_FUNC_INTERNAL? (mln_u8ptr_t)f1->data.process: (mln_u8ptr_t)f1->data.stm;
    data2 = f2->type == M_FUNC_INTERNAL? (mln_u8ptr_t)f2->data.process: (mln_u8ptr_t)f2->data.stm;
    if (data1 != data2) goto t;
    if ((*ret = mln_lang_var_create_false(ctx, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    return 0;
}

static int
mln_lang_func_plus(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2)
{
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
}

static int
mln_lang_func_not(mln_lang_ctx_t *ctx, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2)
{
#ifdef __DEBUG__
    if (op1->val->data.func->type == M_FUNC_INTERNAL)
        ASSERT(op1->val->data.func->data.process);
    else
        ASSERT(op1->val->data.func->data.stm);
#endif
    if ((*ret = mln_lang_var_create_false(ctx, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    return 0;
}

