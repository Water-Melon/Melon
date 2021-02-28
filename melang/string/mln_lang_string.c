
/*
 * Copyright (C) Niklaus F.Schen.
 */
#include "string/mln_lang_string.h"
#include <stdio.h>
#include <stdlib.h>

#ifdef __DEBUG__
#include <assert.h>
#define ASSERT(x) assert(x)
#else
#define ASSERT(x);
#endif

static int mln_lang_string_real2bin_handler(mln_lang_ctx_t *ctx);
static int mln_lang_string_int2bin_handler(mln_lang_ctx_t *ctx);
static int mln_lang_string_bin2real_handler(mln_lang_ctx_t *ctx);
static int mln_lang_string_bin2int_handler(mln_lang_ctx_t *ctx);
static int mln_lang_string_bin2hex_handler(mln_lang_ctx_t *ctx);
static int mln_lang_string_hex2bin_handler(mln_lang_ctx_t *ctx);
static int mln_lang_string_b2s_handler(mln_lang_ctx_t *ctx);
static int mln_lang_string_s2b_handler(mln_lang_ctx_t *ctx);
static int mln_lang_string_strlen_handler(mln_lang_ctx_t *ctx);
static int mln_lang_string_split_handler(mln_lang_ctx_t *ctx);
static int mln_lang_string_strncmp_handler(mln_lang_ctx_t *ctx);
static mln_lang_retExp_t *mln_strcmpSeq_process(mln_lang_ctx_t *ctx);
static mln_lang_retExp_t *mln_strcmp_process(mln_lang_ctx_t *ctx);
static mln_lang_retExp_t *mln_strncmp_process(mln_lang_ctx_t *ctx);
static mln_lang_retExp_t *mln_strstr_process(mln_lang_ctx_t *ctx);
static mln_lang_retExp_t *mln_kmp_process(mln_lang_ctx_t *ctx);
static mln_lang_retExp_t *mln_split_process(mln_lang_ctx_t *ctx);
static mln_lang_retExp_t *mln_slice_process(mln_lang_ctx_t *ctx);
static mln_lang_retExp_t *mln_strlen_process(mln_lang_ctx_t *ctx);
static mln_lang_retExp_t *mln_b2s_process(mln_lang_ctx_t *ctx);
static mln_lang_retExp_t *mln_s2b_process(mln_lang_ctx_t *ctx);
static mln_lang_retExp_t *mln_hex2bin_process(mln_lang_ctx_t *ctx);
static mln_lang_retExp_t *mln_bin2hex_process(mln_lang_ctx_t *ctx);
static mln_lang_retExp_t *mln_bin2int_process(mln_lang_ctx_t *ctx);
static mln_lang_retExp_t *mln_bin2real_process(mln_lang_ctx_t *ctx);
static mln_lang_retExp_t *mln_int2bin_process(mln_lang_ctx_t *ctx);
static mln_lang_retExp_t *mln_real2bin_process(mln_lang_ctx_t *ctx);
static mln_lang_retExp_t *mln_reg_equal_process(mln_lang_ctx_t *ctx);
static mln_lang_retExp_t *mln_reg_match_process(mln_lang_ctx_t *ctx);

int mln_lang_string(mln_lang_ctx_t *ctx)
{
    mln_lang_val_t *val;
    mln_lang_var_t *var;
    mln_lang_func_detail_t *func;
    mln_string_t v1 = mln_string("s1"), v2 = mln_string("s2");
    mln_string_t funcname;
    mln_s8ptr_t funcs[] = {
        "mln_strcmpSeq",
        "mln_strcmp",
        "mln_strstr",
        "mln_kmp",
        "mln_slice",
        "mln_regEqual",
        "mln_regMatch"
    };
    mln_lang_internal handlers[] = {
        mln_strcmpSeq_process,
        mln_strcmp_process,
        mln_strstr_process,
        mln_kmp_process,
        mln_slice_process,
        mln_reg_equal_process,
        mln_reg_match_process
    };
    mln_size_t n = sizeof(funcs)/sizeof(mln_u8ptr_t), i;

    for (i = 0; i < n; ++i) {
        mln_string_nSet(&funcname, funcs[i], strlen(funcs[i]));

        if ((func = mln_lang_func_detail_new(ctx->pool, M_FUNC_INTERNAL, handlers[i], NULL)) == NULL) {
            mln_lang_errmsg(ctx, "No memory.");
            return -1;
        }
        if ((val = mln_lang_val_new(ctx->pool, M_LANG_VAL_TYPE_NIL, NULL)) == NULL) {
            mln_lang_errmsg(ctx, "No memory.");
            mln_lang_func_detail_free(func);
            return -1;
        }
        if ((var = mln_lang_var_new(ctx->pool, &v1, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
            mln_lang_errmsg(ctx, "No memory.");
            mln_lang_val_free(val);
            mln_lang_func_detail_free(func);
            return -1;
        }
        mln_lang_var_chain_add(&(func->args_head), &(func->args_tail), var);
        ++func->nargs;
        if ((val = mln_lang_val_new(ctx->pool, M_LANG_VAL_TYPE_NIL, NULL)) == NULL) {
            mln_lang_errmsg(ctx, "No memory.");
            mln_lang_func_detail_free(func);
            return -1;
        }
        if ((var = mln_lang_var_new(ctx->pool, &v2, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
            mln_lang_errmsg(ctx, "No memory.");
            mln_lang_val_free(val);
            mln_lang_func_detail_free(func);
            return -1;
        }
        mln_lang_var_chain_add(&(func->args_head), &(func->args_tail), var);
        ++func->nargs;

        if ((val = mln_lang_val_new(ctx->pool, M_LANG_VAL_TYPE_FUNC, func)) == NULL) {
            mln_lang_errmsg(ctx, "No memory.");
            mln_lang_func_detail_free(func);
            return -1;
        }
        if ((var = mln_lang_var_new(ctx->pool, &funcname, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
            mln_lang_errmsg(ctx, "No memory.");
            mln_lang_val_free(val);
            return -1;
        }
        if (mln_lang_symbolNode_join(ctx, M_LANG_SYMBOL_VAR, var) < 0) {
            mln_lang_errmsg(ctx, "No memory.");
            mln_lang_var_free(var);
            return -1;
        }
    }

    if (mln_lang_string_strncmp_handler(ctx) < 0) {
        return -1;
    }
    if (mln_lang_string_split_handler(ctx) < 0) {
        return -1;
    }
    if (mln_lang_string_strlen_handler(ctx) < 0) {
        return -1;
    }
    if (mln_lang_string_b2s_handler(ctx) < 0) {
        return -1;
    }
    if (mln_lang_string_s2b_handler(ctx) < 0) {
        return -1;
    }
    if (mln_lang_string_hex2bin_handler(ctx) < 0) {
        return -1;
    }
    if (mln_lang_string_bin2hex_handler(ctx) < 0) {
        return -1;
    }
    if (mln_lang_string_bin2int_handler(ctx) < 0) {
        return -1;
    }
    if (mln_lang_string_int2bin_handler(ctx) < 0) {
        return -1;
    }
    if (mln_lang_string_bin2real_handler(ctx) < 0) {
        return -1;
    }
    if (mln_lang_string_real2bin_handler(ctx) < 0) {
        return -1;
    }
    return 0;
}

static mln_lang_retExp_t *mln_strcmpSeq_process(mln_lang_ctx_t *ctx)
{
    int ret;
    mln_lang_val_t *val1, *val2;
    mln_lang_retExp_t *retExp;
    mln_string_t v1 = mln_string("s1"), v2 = mln_string("s2");
    mln_lang_symbolNode_t *sym;

    if ((sym = mln_lang_symbolNode_search(ctx, &v1, 1)) == NULL) {
        ASSERT(0);
        mln_lang_errmsg(ctx, "Argument missing.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);
    if (mln_lang_var_getValType(sym->data.var) != M_LANG_VAL_TYPE_STRING) {
        mln_lang_errmsg(ctx, "Invalid type of argument 1.");
        return NULL;
    }
    val1 = sym->data.var->val;
    if ((sym = mln_lang_symbolNode_search(ctx, &v2, 1)) == NULL) {
        ASSERT(0);
        mln_lang_errmsg(ctx, "Argument missing.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);
    if (mln_lang_var_getValType(sym->data.var) != M_LANG_VAL_TYPE_STRING) {
        mln_lang_errmsg(ctx, "Invalid type of argument 2.");
        return NULL;
    }
    val2 = sym->data.var->val;

    if (val1->data.s == NULL) {
        if (val2->data.s == NULL) {
            retExp = mln_lang_retExp_createTmpInt(ctx, 0, NULL);
        } else {
            retExp = mln_lang_retExp_createTmpInt(ctx, -1, NULL);
        }
    } else {
        if (val2->data.s == NULL) {
            retExp = mln_lang_retExp_createTmpInt(ctx, 1, NULL);
        } else {
            ret = mln_string_strcmpSeq(val1->data.s, val2->data.s);
            retExp = mln_lang_retExp_createTmpInt(ctx, ret, NULL);
        }
    }
    if (retExp == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return NULL;
    }
    return retExp;
}

static mln_lang_retExp_t *mln_strcmp_process(mln_lang_ctx_t *ctx)
{
    mln_lang_val_t *val1, *val2;
    mln_lang_retExp_t *retExp;
    mln_string_t v1 = mln_string("s1"), v2 = mln_string("s2");
    mln_lang_symbolNode_t *sym;

    if ((sym = mln_lang_symbolNode_search(ctx, &v1, 1)) == NULL) {
        ASSERT(0);
        mln_lang_errmsg(ctx, "Argument missing.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);
    if (mln_lang_var_getValType(sym->data.var) != M_LANG_VAL_TYPE_STRING) {
        mln_lang_errmsg(ctx, "Invalid type of argument 1.");
        return NULL;
    }
    val1 = sym->data.var->val;
    if ((sym = mln_lang_symbolNode_search(ctx, &v2, 1)) == NULL) {
        ASSERT(0);
        mln_lang_errmsg(ctx, "Argument missing.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);
    if (mln_lang_var_getValType(sym->data.var) != M_LANG_VAL_TYPE_STRING) {
        mln_lang_errmsg(ctx, "Invalid type of argument 2.");
        return NULL;
    }
    val2 = sym->data.var->val;

    if (val1->data.s == NULL) {
        if (val2->data.s == NULL) {
            retExp = mln_lang_retExp_createTmpTrue(ctx, NULL);
        } else {
            retExp = mln_lang_retExp_createTmpFalse(ctx, NULL);
        }
    } else {
        if (val2->data.s == NULL) {
            retExp = mln_lang_retExp_createTmpFalse(ctx, NULL);
        } else {
            if (!mln_string_strcmp(val1->data.s, val2->data.s))
                retExp = mln_lang_retExp_createTmpTrue(ctx, NULL);
            else
                retExp = mln_lang_retExp_createTmpFalse(ctx, NULL);
        }
    }
    if (retExp == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return NULL;
    }
    return retExp;
}

static mln_lang_retExp_t *mln_strncmp_process(mln_lang_ctx_t *ctx)
{
    mln_lang_val_t *val1, *val2, *val3;
    mln_lang_retExp_t *retExp;
    mln_string_t v1 = mln_string("s1"), v2 = mln_string("s2"), v3 = mln_string("n");;
    mln_lang_symbolNode_t *sym;

    if ((sym = mln_lang_symbolNode_search(ctx, &v1, 1)) == NULL) {
        ASSERT(0);
        mln_lang_errmsg(ctx, "Argument missing.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);
    if (mln_lang_var_getValType(sym->data.var) != M_LANG_VAL_TYPE_STRING) {
        mln_lang_errmsg(ctx, "Invalid type of argument 1.");
        return NULL;
    }
    val1 = sym->data.var->val;

    if ((sym = mln_lang_symbolNode_search(ctx, &v2, 1)) == NULL) {
        ASSERT(0);
        mln_lang_errmsg(ctx, "Argument missing.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);
    if (mln_lang_var_getValType(sym->data.var) != M_LANG_VAL_TYPE_STRING) {
        mln_lang_errmsg(ctx, "Invalid type of argument 2.");
        return NULL;
    }
    val2 = sym->data.var->val;

    if ((sym = mln_lang_symbolNode_search(ctx, &v3, 1)) == NULL) {
        ASSERT(0);
        mln_lang_errmsg(ctx, "Argument missing.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);
    if (mln_lang_var_getValType(sym->data.var) != M_LANG_VAL_TYPE_INT) {
        mln_lang_errmsg(ctx, "Invalid type of argument 3.");
        return NULL;
    }
    val3 = sym->data.var->val;

    if (val1->data.s == NULL) {
        if (val2->data.s == NULL) {
            retExp = mln_lang_retExp_createTmpTrue(ctx, NULL);
        } else {
            retExp = mln_lang_retExp_createTmpFalse(ctx, NULL);
        }
    } else {
        if (val2->data.s == NULL) {
            retExp = mln_lang_retExp_createTmpFalse(ctx, NULL);
        } else {
            if (!mln_string_strncmp(val1->data.s, val2->data.s, val3->data.i))
                retExp = mln_lang_retExp_createTmpTrue(ctx, NULL);
            else
                retExp = mln_lang_retExp_createTmpFalse(ctx, NULL);
        }
    }
    if (retExp == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return NULL;
    }
    return retExp;
}

static int mln_lang_string_strncmp_handler(mln_lang_ctx_t *ctx)
{
    mln_lang_val_t *val;
    mln_lang_var_t *var;
    mln_lang_func_detail_t *func;
    mln_string_t v1 = mln_string("s1"), v2 = mln_string("s2"), v3 = mln_string("n");
    mln_string_t funcname = mln_string("mln_strncmp");

    if ((func = mln_lang_func_detail_new(ctx->pool, M_FUNC_INTERNAL, mln_strncmp_process, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if ((val = mln_lang_val_new(ctx->pool, M_LANG_VAL_TYPE_NIL, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_func_detail_free(func);
        return -1;
    }
    if ((var = mln_lang_var_new(ctx->pool, &v1, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_val_free(val);
        mln_lang_func_detail_free(func);
        return -1;
    }
    mln_lang_var_chain_add(&(func->args_head), &(func->args_tail), var);
    ++func->nargs;
    if ((val = mln_lang_val_new(ctx->pool, M_LANG_VAL_TYPE_NIL, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_func_detail_free(func);
        return -1;
    }
    if ((var = mln_lang_var_new(ctx->pool, &v2, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_val_free(val);
        mln_lang_func_detail_free(func);
        return -1;
    }
    mln_lang_var_chain_add(&(func->args_head), &(func->args_tail), var);
    ++func->nargs;
    if ((val = mln_lang_val_new(ctx->pool, M_LANG_VAL_TYPE_NIL, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_func_detail_free(func);
        return -1;
    }
    if ((var = mln_lang_var_new(ctx->pool, &v3, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_val_free(val);
        mln_lang_func_detail_free(func);
        return -1;
    }
    mln_lang_var_chain_add(&(func->args_head), &(func->args_tail), var);
    ++func->nargs;

    if ((val = mln_lang_val_new(ctx->pool, M_LANG_VAL_TYPE_FUNC, func)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_func_detail_free(func);
        return -1;
    }
    if ((var = mln_lang_var_new(ctx->pool, &funcname, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_val_free(val);
        return -1;
    }
    if (mln_lang_symbolNode_join(ctx, M_LANG_SYMBOL_VAR, var) < 0) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_var_free(var);
        return -1;
    }

    return 0;
}

static mln_lang_retExp_t *mln_strstr_process(mln_lang_ctx_t *ctx)
{
    mln_lang_val_t *val1, *val2;
    mln_lang_retExp_t *retExp;
    mln_string_t v1 = mln_string("s1"), v2 = mln_string("s2"), *ret;
    mln_lang_symbolNode_t *sym;

    if ((sym = mln_lang_symbolNode_search(ctx, &v1, 1)) == NULL) {
        ASSERT(0);
        mln_lang_errmsg(ctx, "Argument missing.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);
    if (mln_lang_var_getValType(sym->data.var) != M_LANG_VAL_TYPE_STRING) {
        mln_lang_errmsg(ctx, "Invalid type of argument 1.");
        return NULL;
    }
    val1 = sym->data.var->val;
    if ((sym = mln_lang_symbolNode_search(ctx, &v2, 1)) == NULL) {
        ASSERT(0);
        mln_lang_errmsg(ctx, "Argument missing.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);
    if (mln_lang_var_getValType(sym->data.var) != M_LANG_VAL_TYPE_STRING) {
        mln_lang_errmsg(ctx, "Invalid type of argument 2.");
        return NULL;
    }
    val2 = sym->data.var->val;
    if (val1->data.s == NULL || val2->data.s == NULL) {
        mln_lang_errmsg(ctx, "Invalid argument.");
        return NULL;
    }

    if ((ret = mln_string_S_strstr(val1->data.s, val2->data.s)) == NULL) {
        retExp = mln_lang_retExp_createTmpNil(ctx, NULL);
    } else {
        retExp = mln_lang_retExp_createTmpInt(ctx, \
                                              (mln_s64_t)(ret->data-val1->data.s->data), \
                                              NULL);
        mln_string_free(ret);
    }

    if (retExp == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return NULL;
    }
    return retExp;
}

static mln_lang_retExp_t *mln_kmp_process(mln_lang_ctx_t *ctx)
{
    mln_lang_val_t *val1, *val2;
    mln_lang_retExp_t *retExp;
    mln_string_t v1 = mln_string("s1"), v2 = mln_string("s2"), *ret;
    mln_lang_symbolNode_t *sym;

    if ((sym = mln_lang_symbolNode_search(ctx, &v1, 1)) == NULL) {
        ASSERT(0);
        mln_lang_errmsg(ctx, "Argument missing.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);
    if (mln_lang_var_getValType(sym->data.var) != M_LANG_VAL_TYPE_STRING) {
        mln_lang_errmsg(ctx, "Invalid type of argument 1.");
        return NULL;
    }
    val1 = sym->data.var->val;
    if ((sym = mln_lang_symbolNode_search(ctx, &v2, 1)) == NULL) {
        ASSERT(0);
        mln_lang_errmsg(ctx, "Argument missing.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);
    if (mln_lang_var_getValType(sym->data.var) != M_LANG_VAL_TYPE_STRING) {
        mln_lang_errmsg(ctx, "Invalid type of argument 2.");
        return NULL;
    }
    val2 = sym->data.var->val;
    if (val1->data.s == NULL || val2->data.s == NULL) {
        mln_lang_errmsg(ctx, "Invalid argument.");
        return NULL;
    }

    if ((ret = mln_string_S_KMPStrstr(val1->data.s, val2->data.s)) == NULL) {
        retExp = mln_lang_retExp_createTmpNil(ctx, NULL);
    } else {
        retExp = mln_lang_retExp_createTmpInt(ctx, \
                                              (mln_s64_t)(ret->data-val1->data.s->data), \
                                              NULL);
        mln_string_free(ret);
    }

    if (retExp == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return NULL;
    }
    return retExp;
}

static int mln_lang_string_split_handler(mln_lang_ctx_t *ctx)
{
    mln_lang_val_t *val;
    mln_lang_var_t *var;
    mln_lang_func_detail_t *func;
    mln_string_t v1 = mln_string("s"), v2 = mln_string("offset"), v3 = mln_string("len");
    mln_string_t funcname = mln_string("mln_split");

    if ((func = mln_lang_func_detail_new(ctx->pool, M_FUNC_INTERNAL, mln_split_process, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if ((val = mln_lang_val_new(ctx->pool, M_LANG_VAL_TYPE_NIL, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_func_detail_free(func);
        return -1;
    }
    if ((var = mln_lang_var_new(ctx->pool, &v1, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_val_free(val);
        mln_lang_func_detail_free(func);
        return -1;
    }
    mln_lang_var_chain_add(&(func->args_head), &(func->args_tail), var);
    ++func->nargs;
    if ((val = mln_lang_val_new(ctx->pool, M_LANG_VAL_TYPE_NIL, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_func_detail_free(func);
        return -1;
    }
    if ((var = mln_lang_var_new(ctx->pool, &v2, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_val_free(val);
        mln_lang_func_detail_free(func);
        return -1;
    }
    mln_lang_var_chain_add(&(func->args_head), &(func->args_tail), var);
    ++func->nargs;

    if ((val = mln_lang_val_new(ctx->pool, M_LANG_VAL_TYPE_NIL, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_func_detail_free(func);
        return -1;
    }
    if ((var = mln_lang_var_new(ctx->pool, &v3, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_val_free(val);
        mln_lang_func_detail_free(func);
        return -1;
    }
    mln_lang_var_chain_add(&(func->args_head), &(func->args_tail), var);
    ++func->nargs;

    if ((val = mln_lang_val_new(ctx->pool, M_LANG_VAL_TYPE_FUNC, func)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_func_detail_free(func);
        return -1;
    }
    if ((var = mln_lang_var_new(ctx->pool, &funcname, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_val_free(val);
        return -1;
    }
    if (mln_lang_symbolNode_join(ctx, M_LANG_SYMBOL_VAR, var) < 0) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_var_free(var);
        return -1;
    }

    return 0;
}

static mln_lang_retExp_t *mln_split_process(mln_lang_ctx_t *ctx)
{
    mln_lang_val_t *val1, *val2, *val3;
    mln_lang_retExp_t *retExp;
    mln_string_t v1 = mln_string("s"), v2 = mln_string("offset");
    mln_string_t v3 = mln_string("len"), ret;
    mln_lang_symbolNode_t *sym;
    mln_u8ptr_t p;
    mln_s64_t len;

    if ((sym = mln_lang_symbolNode_search(ctx, &v1, 1)) == NULL) {
        ASSERT(0);
        mln_lang_errmsg(ctx, "Argument missing.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);
    if (mln_lang_var_getValType(sym->data.var) != M_LANG_VAL_TYPE_STRING) {
        mln_lang_errmsg(ctx, "Invalid type of argument 1.");
        return NULL;
    }
    val1 = sym->data.var->val;

    if ((sym = mln_lang_symbolNode_search(ctx, &v2, 1)) == NULL) {
        ASSERT(0);
        mln_lang_errmsg(ctx, "Argument missing.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);
    if (mln_lang_var_getValType(sym->data.var) != M_LANG_VAL_TYPE_INT) {
        mln_lang_errmsg(ctx, "Invalid type of argument 2.");
        return NULL;
    }
    val2 = sym->data.var->val;

    if ((sym = mln_lang_symbolNode_search(ctx, &v3, 1)) == NULL) {
        ASSERT(0);
        mln_lang_errmsg(ctx, "Argument missing.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);
    if (mln_lang_var_getValType(sym->data.var) == M_LANG_VAL_TYPE_NIL) {
        len = -1;
    } else if (mln_lang_var_getValType(sym->data.var) == M_LANG_VAL_TYPE_INT) {
        val3 = sym->data.var->val;
        len = val3->data.i;
        if (len < 0) {
            mln_lang_errmsg(ctx, "Invalid type of argument 3.");
            return NULL;
        }
    } else {
        mln_lang_errmsg(ctx, "Invalid type of argument 3.");
        return NULL;
    }

    if (val2->data.i < 0) {
        mln_s64_t off = -(val2->data.i);
        p = val1->data.s->data + (val1->data.s->len - (mln_u64_t)off);
        if (len >= 0 && len < off) off = len;
        mln_string_nSet(&ret, p, (mln_u64_t)off);
    } else {
        p = val1->data.s->data + val2->data.i;
        if (len >= 0 && len < val1->data.s->len-val2->data.i) {
            mln_string_nSet(&ret, p, len);
        } else {
            mln_string_nSet(&ret, p, val1->data.s->len - val2->data.i);
        }
    }
    if ((retExp = mln_lang_retExp_createTmpString(ctx, &ret, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return NULL;
    }
    return retExp;
}

static mln_lang_retExp_t *mln_reg_equal_process(mln_lang_ctx_t *ctx)
{
    mln_lang_val_t *val1, *val2;
    mln_lang_retExp_t *retExp;
    mln_string_t v1 = mln_string("s1"), v2 = mln_string("s2");
    mln_lang_symbolNode_t *sym;

    if ((sym = mln_lang_symbolNode_search(ctx, &v1, 1)) == NULL) {
        ASSERT(0);
        mln_lang_errmsg(ctx, "Argument missing.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);
    if (mln_lang_var_getValType(sym->data.var) != M_LANG_VAL_TYPE_STRING) {
        mln_lang_errmsg(ctx, "Invalid type of argument 1.");
        return NULL;
    }
    val1 = sym->data.var->val;

    if ((sym = mln_lang_symbolNode_search(ctx, &v2, 1)) == NULL) {
        ASSERT(0);
        mln_lang_errmsg(ctx, "Argument missing.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);
    if (mln_lang_var_getValType(sym->data.var) != M_LANG_VAL_TYPE_STRING) {
        mln_lang_errmsg(ctx, "Invalid type of argument 2.");
        return NULL;
    }
    val2 = sym->data.var->val;

    if (mln_reg_equal(val1->data.s, val2->data.s)) {
        retExp = mln_lang_retExp_createTmpTrue(ctx, NULL);
    } else {
        retExp = mln_lang_retExp_createTmpFalse(ctx, NULL);
    }
    if (retExp == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return NULL;
    }
    return retExp;
}

static mln_lang_retExp_t *mln_reg_match_process(mln_lang_ctx_t *ctx)
{
    mln_lang_val_t *val1, *val2;
    mln_lang_retExp_t *retExp;
    mln_string_t v1 = mln_string("s1"), v2 = mln_string("s2");
    mln_lang_symbolNode_t *sym;
    mln_lang_array_t *array;
    mln_lang_var_t *array_val;
    mln_lang_var_t var;
    mln_lang_val_t val;
    mln_reg_match_t *scan, *head = NULL, *tail = NULL;

    if ((sym = mln_lang_symbolNode_search(ctx, &v1, 1)) == NULL) {
        ASSERT(0);
        mln_lang_errmsg(ctx, "Argument missing.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);
    if (mln_lang_var_getValType(sym->data.var) != M_LANG_VAL_TYPE_STRING) {
        mln_lang_errmsg(ctx, "Invalid type of argument 1.");
        return NULL;
    }
    val1 = sym->data.var->val;
    if ((sym = mln_lang_symbolNode_search(ctx, &v2, 1)) == NULL) {
        ASSERT(0);
        mln_lang_errmsg(ctx, "Argument missing.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);
    if (mln_lang_var_getValType(sym->data.var) != M_LANG_VAL_TYPE_STRING) {
        mln_lang_errmsg(ctx, "Invalid type of argument 2.");
        return NULL;
    }
    val2 = sym->data.var->val;

    if (mln_reg_match(val1->data.s, val2->data.s, &head, &tail) < 0) {
        if ((retExp = mln_lang_retExp_createTmpFalse(ctx, NULL)) == NULL) {
            mln_lang_errmsg(ctx, "No memory.");
            return NULL;
        }
        return retExp;
    }

    if ((retExp = mln_lang_retExp_createTmpArray(ctx, NULL)) == NULL) {
        mln_reg_match_result_free(head);
        mln_lang_errmsg(ctx, "No memory.");
        return NULL;
    }
    array = retExp->data.var->val->data.array;
    for (scan = head; scan != NULL; scan = scan->next) {
        if ((array_val = mln_lang_array_getAndNew(ctx, array, NULL)) == NULL) {
            mln_lang_retExp_free(retExp);
            mln_reg_match_result_free(head);
            mln_lang_errmsg(ctx, "No memory.");
            return NULL;
        }
        var.type = M_LANG_VAR_NORMAL;
        var.name = NULL;
        var.val = &val;
        var.inSet = NULL;
        var.prev = var.next = NULL;
        val.data.s = &(scan->data);
        val.type = M_LANG_VAL_TYPE_STRING;
        val.ref = 1;
        if (mln_lang_var_setValue(ctx, array_val, &var) < 0) {
            mln_lang_retExp_free(retExp);
            mln_reg_match_result_free(head);
            mln_lang_errmsg(ctx, "No memory.");
            return NULL;
        }
    }
    mln_reg_match_result_free(head);
    return retExp;
}

static mln_lang_retExp_t *mln_slice_process(mln_lang_ctx_t *ctx)
{
    mln_lang_val_t *val1, *val2;
    mln_lang_retExp_t *retExp;
    mln_s8ptr_t seps;
    mln_string_t v1 = mln_string("s1"), v2 = mln_string("s2"), *ret;
    mln_lang_symbolNode_t *sym;
    mln_lang_array_t *array;
    mln_string_t *scan;
    mln_lang_var_t *array_val;
    mln_lang_var_t var;
    mln_lang_val_t val;

    if ((sym = mln_lang_symbolNode_search(ctx, &v1, 1)) == NULL) {
        ASSERT(0);
        mln_lang_errmsg(ctx, "Argument missing.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);
    if (mln_lang_var_getValType(sym->data.var) != M_LANG_VAL_TYPE_STRING) {
        mln_lang_errmsg(ctx, "Invalid type of argument 1.");
        return NULL;
    }
    val1 = sym->data.var->val;
    if ((sym = mln_lang_symbolNode_search(ctx, &v2, 1)) == NULL) {
        ASSERT(0);
        mln_lang_errmsg(ctx, "Argument missing.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);
    if (mln_lang_var_getValType(sym->data.var) != M_LANG_VAL_TYPE_STRING) {
        mln_lang_errmsg(ctx, "Invalid type of argument 2.");
        return NULL;
    }
    val2 = sym->data.var->val;

    if ((seps = (mln_s8ptr_t)malloc(val2->data.s->len+1)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return NULL;
    }
    memcpy(seps, val2->data.s->data, val2->data.s->len);
    seps[val2->data.s->len] = 0;
    ret = mln_string_slice(val1->data.s, seps);
    free(seps);
    if (ret == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return NULL;
    }
    if ((retExp = mln_lang_retExp_createTmpArray(ctx, NULL)) == NULL) {
        mln_string_slice_free(ret);
        mln_lang_errmsg(ctx, "No memory.");
        return NULL;
    }
    array = retExp->data.var->val->data.array;
    for (scan = ret; scan->data != NULL; ++scan) {
        if ((array_val = mln_lang_array_getAndNew(ctx, array, NULL)) == NULL) {
            mln_lang_retExp_free(retExp);
            mln_string_slice_free(ret);
            mln_lang_errmsg(ctx, "No memory.");
            return NULL;
        }
        var.type = M_LANG_VAR_NORMAL;
        var.name = NULL;
        var.val = &val;
        var.inSet = NULL;
        var.prev = var.next = NULL;
        val.data.s = scan;
        val.type = M_LANG_VAL_TYPE_STRING;
        val.ref = 1;
        if (mln_lang_var_setValue(ctx, array_val, &var) < 0) {
            mln_lang_retExp_free(retExp);
            mln_string_slice_free(ret);
            mln_lang_errmsg(ctx, "No memory.");
            return NULL;
        }
    }
    mln_string_slice_free(ret);
    return retExp;
}

static int mln_lang_string_strlen_handler(mln_lang_ctx_t *ctx)
{
    mln_lang_val_t *val;
    mln_lang_var_t *var;
    mln_lang_func_detail_t *func;
    mln_string_t v1 = mln_string("s");
    mln_string_t funcname = mln_string("mln_strlen");

    if ((func = mln_lang_func_detail_new(ctx->pool, M_FUNC_INTERNAL, mln_strlen_process, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if ((val = mln_lang_val_new(ctx->pool, M_LANG_VAL_TYPE_NIL, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_func_detail_free(func);
        return -1;
    }
    if ((var = mln_lang_var_new(ctx->pool, &v1, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_val_free(val);
        mln_lang_func_detail_free(func);
        return -1;
    }
    mln_lang_var_chain_add(&(func->args_head), &(func->args_tail), var);
    ++func->nargs;

    if ((val = mln_lang_val_new(ctx->pool, M_LANG_VAL_TYPE_FUNC, func)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_func_detail_free(func);
        return -1;
    }
    if ((var = mln_lang_var_new(ctx->pool, &funcname, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_val_free(val);
        return -1;
    }
    if (mln_lang_symbolNode_join(ctx, M_LANG_SYMBOL_VAR, var) < 0) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_var_free(var);
        return -1;
    }

    return 0;
}

static mln_lang_retExp_t *mln_strlen_process(mln_lang_ctx_t *ctx)
{
    mln_lang_val_t *val1;
    mln_lang_retExp_t *retExp;
    mln_string_t v1 = mln_string("s");
    mln_lang_symbolNode_t *sym;

    if ((sym = mln_lang_symbolNode_search(ctx, &v1, 1)) == NULL) {
        ASSERT(0);
        mln_lang_errmsg(ctx, "Argument missing.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);
    if (mln_lang_var_getValType(sym->data.var) != M_LANG_VAL_TYPE_STRING) {
        mln_lang_errmsg(ctx, "Invalid type of argument 1.");
        return NULL;
    }
    val1 = sym->data.var->val;
    if (val1->data.s == NULL) {
        mln_lang_errmsg(ctx, "Invalid argument.");
        return NULL;
    }
    if ((retExp = mln_lang_retExp_createTmpInt(ctx, val1->data.s->len, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return NULL;
    }
    return retExp;
}

static int mln_lang_string_b2s_handler(mln_lang_ctx_t *ctx)
{
    mln_lang_val_t *val;
    mln_lang_var_t *var;
    mln_lang_func_detail_t *func;
    mln_string_t v1 = mln_string("bin");
    mln_string_t funcname = mln_string("mln_b2s");

    if ((func = mln_lang_func_detail_new(ctx->pool, M_FUNC_INTERNAL, mln_b2s_process, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if ((val = mln_lang_val_new(ctx->pool, M_LANG_VAL_TYPE_NIL, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_func_detail_free(func);
        return -1;
    }
    if ((var = mln_lang_var_new(ctx->pool, &v1, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_val_free(val);
        mln_lang_func_detail_free(func);
        return -1;
    }
    mln_lang_var_chain_add(&(func->args_head), &(func->args_tail), var);
    ++func->nargs;

    if ((val = mln_lang_val_new(ctx->pool, M_LANG_VAL_TYPE_FUNC, func)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_func_detail_free(func);
        return -1;
    }
    if ((var = mln_lang_var_new(ctx->pool, &funcname, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_val_free(val);
        return -1;
    }
    if (mln_lang_symbolNode_join(ctx, M_LANG_SYMBOL_VAR, var) < 0) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_var_free(var);
        return -1;
    }

    return 0;
}

static mln_lang_retExp_t *mln_b2s_process(mln_lang_ctx_t *ctx)
{
    mln_s32_t t;
    mln_lang_val_t *val1;
    mln_lang_retExp_t *retExp;
    mln_string_t v1 = mln_string("bin"), tmp;
    mln_u8_t data[16];
    mln_lang_symbolNode_t *sym;

    if ((sym = mln_lang_symbolNode_search(ctx, &v1, 1)) == NULL) {
        ASSERT(0);
        mln_lang_errmsg(ctx, "Argument missing.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);
    t = mln_lang_var_getValType(sym->data.var);
    val1 = sym->data.var->val;
    if (t == M_LANG_VAL_TYPE_INT) {
        memcpy(data, &(val1->data.i), sizeof(mln_s64_t));
        mln_string_nSet(&tmp, data, sizeof(mln_s64_t));
    } else if (t == M_LANG_VAL_TYPE_REAL) {
        memcpy(data, &(val1->data.f), sizeof(double));
        mln_string_nSet(&tmp, data, sizeof(double));
    } else if (t == M_LANG_VAL_TYPE_BOOL) {
        data[0] = val1->data.b;
        mln_string_nSet(&tmp, data, 1);
    } else {
        mln_lang_errmsg(ctx, "Invalid type of argument 1.");
        return NULL;
    }
    if ((retExp = mln_lang_retExp_createTmpString(ctx, &tmp, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return NULL;
    }
    return retExp;
}

static int mln_lang_string_s2b_handler(mln_lang_ctx_t *ctx)
{
    mln_lang_val_t *val;
    mln_lang_var_t *var;
    mln_lang_func_detail_t *func;
    mln_string_t v1 = mln_string("s"), v2 = mln_string("type");
    mln_string_t funcname = mln_string("mln_s2b");

    if ((func = mln_lang_func_detail_new(ctx->pool, M_FUNC_INTERNAL, mln_s2b_process, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if ((val = mln_lang_val_new(ctx->pool, M_LANG_VAL_TYPE_NIL, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_func_detail_free(func);
        return -1;
    }
    if ((var = mln_lang_var_new(ctx->pool, &v1, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_val_free(val);
        mln_lang_func_detail_free(func);
        return -1;
    }
    mln_lang_var_chain_add(&(func->args_head), &(func->args_tail), var);
    ++func->nargs;

    if ((val = mln_lang_val_new(ctx->pool, M_LANG_VAL_TYPE_NIL, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_func_detail_free(func);
        return -1;
    }
    if ((var = mln_lang_var_new(ctx->pool, &v2, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_val_free(val);
        mln_lang_func_detail_free(func);
        return -1;
    }
    mln_lang_var_chain_add(&(func->args_head), &(func->args_tail), var);
    ++func->nargs;

    if ((val = mln_lang_val_new(ctx->pool, M_LANG_VAL_TYPE_FUNC, func)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_func_detail_free(func);
        return -1;
    }
    if ((var = mln_lang_var_new(ctx->pool, &funcname, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_val_free(val);
        return -1;
    }
    if (mln_lang_symbolNode_join(ctx, M_LANG_SYMBOL_VAR, var) < 0) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_var_free(var);
        return -1;
    }

    return 0;
}

static mln_lang_retExp_t *mln_s2b_process(mln_lang_ctx_t *ctx)
{
    mln_lang_val_t *val1, *val2;
    mln_lang_retExp_t *retExp;
    mln_string_t v1 = mln_string("s"), v2 = mln_string("type");
    mln_lang_symbolNode_t *sym;

    if ((sym = mln_lang_symbolNode_search(ctx, &v1, 1)) == NULL) {
        ASSERT(0);
        mln_lang_errmsg(ctx, "Argument missing.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);
    if (mln_lang_var_getValType(sym->data.var) != M_LANG_VAL_TYPE_STRING) {
        mln_lang_errmsg(ctx, "Invalid type of argument 1.");
        return NULL;
    }
    val1 = sym->data.var->val;

    if ((sym = mln_lang_symbolNode_search(ctx, &v2, 1)) == NULL) {
        ASSERT(0);
        mln_lang_errmsg(ctx, "Argument missing.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);
    if (mln_lang_var_getValType(sym->data.var) != M_LANG_VAL_TYPE_STRING) {
        mln_lang_errmsg(ctx, "Invalid type of argument 2.");
        return NULL;
    }
    val2 = sym->data.var->val;

    if (val1->data.s == NULL || val2->data.s == NULL) {
        mln_lang_errmsg(ctx, "Invalid argument.");
        return NULL;
    }
    if (!mln_string_constStrcmp(val2->data.s, "int")) {
        mln_s64_t i = 0;
        memcpy(&i, val1->data.s->data, \
               val1->data.s->len>sizeof(mln_s64_t)? \
                   sizeof(mln_s64_t): val1->data.s->len);
        retExp = mln_lang_retExp_createTmpInt(ctx, i, NULL);
    } else if (!mln_string_constStrcmp(val2->data.s, "real")) {
        double f = 0;
        memcpy(&f, val1->data.s->data, \
               val1->data.s->len>sizeof(double)? \
                   sizeof(double): val1->data.s->len);
        retExp = mln_lang_retExp_createTmpReal(ctx, f, NULL);
    } else if (!mln_string_constStrcmp(val2->data.s, "bool")) {
        retExp = mln_lang_retExp_createTmpBool(ctx, val1->data.s->data[0], NULL);
    } else {
        mln_lang_errmsg(ctx, "Invalid argument 2.");
        return NULL;
    }

    if (retExp == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return NULL;
    }
    return retExp;
}

static int mln_lang_string_hex2bin_handler(mln_lang_ctx_t *ctx)
{
    mln_lang_val_t *val;
    mln_lang_var_t *var;
    mln_lang_func_detail_t *func;
    mln_string_t v1 = mln_string("hex");
    mln_string_t funcname = mln_string("mln_hex2bin");

    if ((func = mln_lang_func_detail_new(ctx->pool, M_FUNC_INTERNAL, mln_hex2bin_process, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if ((val = mln_lang_val_new(ctx->pool, M_LANG_VAL_TYPE_NIL, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_func_detail_free(func);
        return -1;
    }
    if ((var = mln_lang_var_new(ctx->pool, &v1, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_val_free(val);
        mln_lang_func_detail_free(func);
        return -1;
    }
    mln_lang_var_chain_add(&(func->args_head), &(func->args_tail), var);
    ++func->nargs;

    if ((val = mln_lang_val_new(ctx->pool, M_LANG_VAL_TYPE_FUNC, func)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_func_detail_free(func);
        return -1;
    }
    if ((var = mln_lang_var_new(ctx->pool, &funcname, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_val_free(val);
        return -1;
    }
    if (mln_lang_symbolNode_join(ctx, M_LANG_SYMBOL_VAR, var) < 0) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_var_free(var);
        return -1;
    }

    return 0;
}

static mln_lang_retExp_t *mln_hex2bin_process(mln_lang_ctx_t *ctx)
{
    mln_s32_t t;
    mln_u32_t i, j, n;
    mln_lang_val_t *val1;
    mln_lang_retExp_t *retExp;
    mln_string_t v1 = mln_string("hex"), tmp;
    mln_lang_symbolNode_t *sym;
    mln_u8ptr_t buf, p;
    mln_u8_t v;

    if ((sym = mln_lang_symbolNode_search(ctx, &v1, 1)) == NULL) {
        ASSERT(0);
        mln_lang_errmsg(ctx, "Argument missing.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);
    t = mln_lang_var_getValType(sym->data.var);
    val1 = sym->data.var->val;
    if (t != M_LANG_VAL_TYPE_STRING || val1->data.s->len % 2) {
        mln_lang_errmsg(ctx, "Invalid type of argument 1.");
        return NULL;
    }

    if ((buf = (mln_u8ptr_t)malloc(val1->data.s->len / 2 + 1)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return NULL;
    }
    for (p = val1->data.s->data, i = j = 0, n = val1->data.s->len; i < n; ++i, ++p) {
        v = *p;
        if (v >= '0' && v <= '9') {
            v = v - '0';
        } else if (v >= 'a' && v <= 'f') {
            v = v - 'a' + 10;
        } else if (v >= 'A' && v <= 'F') {
            v = v - 'A' + 10;
        } else {
            mln_lang_errmsg(ctx, "Invalid type of argument 1.");
            free(buf);
            return NULL;
        }
        if (i % 2) {
            buf[j++] |= v;
        } else {
            buf[j] = (v << 4) & 0xf0;
        }
    }
    buf[val1->data.s->len / 2] = 0;
    mln_string_nSet(&tmp, buf, val1->data.s->len / 2);

    if ((retExp = mln_lang_retExp_createTmpString(ctx, &tmp, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        free(buf);
        return NULL;
    }
    free(buf);
    return retExp;
}

static int mln_lang_string_bin2hex_handler(mln_lang_ctx_t *ctx)
{
    mln_lang_val_t *val;
    mln_lang_var_t *var;
    mln_lang_func_detail_t *func;
    mln_string_t v1 = mln_string("bin");
    mln_string_t funcname = mln_string("mln_bin2hex");

    if ((func = mln_lang_func_detail_new(ctx->pool, M_FUNC_INTERNAL, mln_bin2hex_process, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if ((val = mln_lang_val_new(ctx->pool, M_LANG_VAL_TYPE_NIL, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_func_detail_free(func);
        return -1;
    }
    if ((var = mln_lang_var_new(ctx->pool, &v1, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_val_free(val);
        mln_lang_func_detail_free(func);
        return -1;
    }
    mln_lang_var_chain_add(&(func->args_head), &(func->args_tail), var);
    ++func->nargs;

    if ((val = mln_lang_val_new(ctx->pool, M_LANG_VAL_TYPE_FUNC, func)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_func_detail_free(func);
        return -1;
    }
    if ((var = mln_lang_var_new(ctx->pool, &funcname, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_val_free(val);
        return -1;
    }
    if (mln_lang_symbolNode_join(ctx, M_LANG_SYMBOL_VAR, var) < 0) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_var_free(var);
        return -1;
    }

    return 0;
}

static mln_lang_retExp_t *mln_bin2hex_process(mln_lang_ctx_t *ctx)
{
    mln_s32_t t;
    mln_u32_t i;
    mln_lang_val_t *val1;
    mln_lang_retExp_t *retExp;
    mln_string_t v1 = mln_string("bin"), tmp;
    mln_lang_symbolNode_t *sym;
    mln_u8ptr_t buf, p, end;
    mln_u8_t v;

    if ((sym = mln_lang_symbolNode_search(ctx, &v1, 1)) == NULL) {
        ASSERT(0);
        mln_lang_errmsg(ctx, "Argument missing.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);
    t = mln_lang_var_getValType(sym->data.var);
    val1 = sym->data.var->val;
    if (t != M_LANG_VAL_TYPE_STRING) {
        mln_lang_errmsg(ctx, "Invalid type of argument 1.");
        return NULL;
    }

    if ((buf = (mln_u8ptr_t)malloc(val1->data.s->len * 2 + 1)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return NULL;
    }
    for (p = val1->data.s->data, i = 0, end = val1->data.s->data + val1->data.s->len; p < end; ++p) {
        v = (*p >> 4) & 0xf;
        if (v >= 0 && v <= 9) {
            buf[i++] = '0' + v;
        } else {
            buf[i++] = v - 10 + 'A';
        }
        v = *p & 0xf;
        if (v >= 0 && v <= 9) {
            buf[i++] = '0' + v;
        } else {
            buf[i++] = v - 10 + 'A';
        }
    }
    buf[val1->data.s->len * 2] = 0;
    mln_string_nSet(&tmp, buf, val1->data.s->len * 2);

    if ((retExp = mln_lang_retExp_createTmpString(ctx, &tmp, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        free(buf);
        return NULL;
    }
    free(buf);
    return retExp;
}

static int mln_lang_string_bin2int_handler(mln_lang_ctx_t *ctx)
{
    mln_lang_val_t *val;
    mln_lang_var_t *var;
    mln_lang_func_detail_t *func;
    mln_string_t v1 = mln_string("bin");
    mln_string_t funcname = mln_string("mln_bin2int");

    if ((func = mln_lang_func_detail_new(ctx->pool, M_FUNC_INTERNAL, mln_bin2int_process, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if ((val = mln_lang_val_new(ctx->pool, M_LANG_VAL_TYPE_NIL, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_func_detail_free(func);
        return -1;
    }
    if ((var = mln_lang_var_new(ctx->pool, &v1, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_val_free(val);
        mln_lang_func_detail_free(func);
        return -1;
    }
    mln_lang_var_chain_add(&(func->args_head), &(func->args_tail), var);
    ++func->nargs;

    if ((val = mln_lang_val_new(ctx->pool, M_LANG_VAL_TYPE_FUNC, func)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_func_detail_free(func);
        return -1;
    }
    if ((var = mln_lang_var_new(ctx->pool, &funcname, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_val_free(val);
        return -1;
    }
    if (mln_lang_symbolNode_join(ctx, M_LANG_SYMBOL_VAR, var) < 0) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_var_free(var);
        return -1;
    }

    return 0;
}

static mln_lang_retExp_t *mln_bin2int_process(mln_lang_ctx_t *ctx)
{
    mln_s32_t t;
    mln_lang_val_t *val1;
    mln_lang_retExp_t *retExp;
    mln_string_t v1 = mln_string("bin");
    mln_lang_symbolNode_t *sym;
    mln_u8ptr_t p, end;
    mln_s64_t i = 0, j;

    if ((sym = mln_lang_symbolNode_search(ctx, &v1, 1)) == NULL) {
        ASSERT(0);
        mln_lang_errmsg(ctx, "Argument missing.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);
    t = mln_lang_var_getValType(sym->data.var);
    val1 = sym->data.var->val;
    if (t != M_LANG_VAL_TYPE_STRING) {
        mln_lang_errmsg(ctx, "Invalid type of argument 1.");
        return NULL;
    }

    for (j = 0, p = val1->data.s->data, end = val1->data.s->data + val1->data.s->len - 1; \
         end >= p && j < sizeof(i); \
         --end, ++j)
    {
        i |= ((mln_s64_t)(*end) << (j * 8));
    }

    if ((retExp = mln_lang_retExp_createTmpInt(ctx, i, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return NULL;
    }
    return retExp;
}

static int mln_lang_string_int2bin_handler(mln_lang_ctx_t *ctx)
{
    mln_lang_val_t *val;
    mln_lang_var_t *var;
    mln_lang_func_detail_t *func;
    mln_string_t v1 = mln_string("i");
    mln_string_t funcname = mln_string("mln_int2bin");

    if ((func = mln_lang_func_detail_new(ctx->pool, M_FUNC_INTERNAL, mln_int2bin_process, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if ((val = mln_lang_val_new(ctx->pool, M_LANG_VAL_TYPE_NIL, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_func_detail_free(func);
        return -1;
    }
    if ((var = mln_lang_var_new(ctx->pool, &v1, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_val_free(val);
        mln_lang_func_detail_free(func);
        return -1;
    }
    mln_lang_var_chain_add(&(func->args_head), &(func->args_tail), var);
    ++func->nargs;

    if ((val = mln_lang_val_new(ctx->pool, M_LANG_VAL_TYPE_FUNC, func)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_func_detail_free(func);
        return -1;
    }
    if ((var = mln_lang_var_new(ctx->pool, &funcname, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_val_free(val);
        return -1;
    }
    if (mln_lang_symbolNode_join(ctx, M_LANG_SYMBOL_VAR, var) < 0) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_var_free(var);
        return -1;
    }

    return 0;
}

static mln_lang_retExp_t *mln_int2bin_process(mln_lang_ctx_t *ctx)
{
    mln_s32_t t;
    mln_lang_val_t *val1;
    mln_lang_retExp_t *retExp;
    mln_string_t v1 = mln_string("i"), tmp;
    mln_lang_symbolNode_t *sym;
    mln_u8_t buf[sizeof(mln_s64_t)];

    if ((sym = mln_lang_symbolNode_search(ctx, &v1, 1)) == NULL) {
        ASSERT(0);
        mln_lang_errmsg(ctx, "Argument missing.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);
    t = mln_lang_var_getValType(sym->data.var);
    val1 = sym->data.var->val;
    if (t != M_LANG_VAL_TYPE_INT) {
        mln_lang_errmsg(ctx, "Invalid type of argument 1.");
        return NULL;
    }

    for (t = 0; t < sizeof(buf); ++t) {
        buf[sizeof(buf)-1-t] = (val1->data.i >> (t << 3)) & 0xff;
    }
    mln_string_nSet(&tmp, buf, sizeof(buf));
    if ((retExp = mln_lang_retExp_createTmpString(ctx, &tmp, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return NULL;
    }
    return retExp;
}

static int mln_lang_string_bin2real_handler(mln_lang_ctx_t *ctx)
{
    mln_lang_val_t *val;
    mln_lang_var_t *var;
    mln_lang_func_detail_t *func;
    mln_string_t v1 = mln_string("bin");
    mln_string_t funcname = mln_string("mln_bin2real");

    if ((func = mln_lang_func_detail_new(ctx->pool, M_FUNC_INTERNAL, mln_bin2real_process, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if ((val = mln_lang_val_new(ctx->pool, M_LANG_VAL_TYPE_NIL, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_func_detail_free(func);
        return -1;
    }
    if ((var = mln_lang_var_new(ctx->pool, &v1, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_val_free(val);
        mln_lang_func_detail_free(func);
        return -1;
    }
    mln_lang_var_chain_add(&(func->args_head), &(func->args_tail), var);
    ++func->nargs;

    if ((val = mln_lang_val_new(ctx->pool, M_LANG_VAL_TYPE_FUNC, func)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_func_detail_free(func);
        return -1;
    }
    if ((var = mln_lang_var_new(ctx->pool, &funcname, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_val_free(val);
        return -1;
    }
    if (mln_lang_symbolNode_join(ctx, M_LANG_SYMBOL_VAR, var) < 0) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_var_free(var);
        return -1;
    }

    return 0;
}

static mln_lang_retExp_t *mln_bin2real_process(mln_lang_ctx_t *ctx)
{
    mln_s32_t t, i;
    mln_lang_val_t *val1;
    mln_lang_retExp_t *retExp;
    mln_string_t v1 = mln_string("bin");
    mln_lang_symbolNode_t *sym;
    mln_u8ptr_t p, end;
    mln_u8_t buf[sizeof(double)];
    double f;

    if ((sym = mln_lang_symbolNode_search(ctx, &v1, 1)) == NULL) {
        ASSERT(0);
        mln_lang_errmsg(ctx, "Argument missing.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);
    t = mln_lang_var_getValType(sym->data.var);
    val1 = sym->data.var->val;
    if (t != M_LANG_VAL_TYPE_STRING) {
        mln_lang_errmsg(ctx, "Invalid type of argument 1.");
        return NULL;
    }

    for (i = 0, p = val1->data.s->data, end = val1->data.s->data + val1->data.s->len - 1; \
         end >= p && i < sizeof(f); \
         --end, ++i)
    {
        buf[sizeof(double)-i-1] = *end;
    }
    memcpy(&f, buf, sizeof(f));

    if ((retExp = mln_lang_retExp_createTmpReal(ctx, f, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return NULL;
    }
    return retExp;
}

static int mln_lang_string_real2bin_handler(mln_lang_ctx_t *ctx)
{
    mln_lang_val_t *val;
    mln_lang_var_t *var;
    mln_lang_func_detail_t *func;
    mln_string_t v1 = mln_string("r");
    mln_string_t funcname = mln_string("mln_real2bin");

    if ((func = mln_lang_func_detail_new(ctx->pool, M_FUNC_INTERNAL, mln_real2bin_process, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if ((val = mln_lang_val_new(ctx->pool, M_LANG_VAL_TYPE_NIL, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_func_detail_free(func);
        return -1;
    }
    if ((var = mln_lang_var_new(ctx->pool, &v1, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_val_free(val);
        mln_lang_func_detail_free(func);
        return -1;
    }
    mln_lang_var_chain_add(&(func->args_head), &(func->args_tail), var);
    ++func->nargs;

    if ((val = mln_lang_val_new(ctx->pool, M_LANG_VAL_TYPE_FUNC, func)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_func_detail_free(func);
        return -1;
    }
    if ((var = mln_lang_var_new(ctx->pool, &funcname, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_val_free(val);
        return -1;
    }
    if (mln_lang_symbolNode_join(ctx, M_LANG_SYMBOL_VAR, var) < 0) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_var_free(var);
        return -1;
    }

    return 0;
}

static mln_lang_retExp_t *mln_real2bin_process(mln_lang_ctx_t *ctx)
{
    mln_s32_t t;
    mln_lang_val_t *val1;
    mln_lang_retExp_t *retExp;
    mln_string_t v1 = mln_string("r"), tmp;
    mln_lang_symbolNode_t *sym;
    mln_u8ptr_t buf[sizeof(double)];

    if ((sym = mln_lang_symbolNode_search(ctx, &v1, 1)) == NULL) {
        ASSERT(0);
        mln_lang_errmsg(ctx, "Argument missing.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);
    t = mln_lang_var_getValType(sym->data.var);
    val1 = sym->data.var->val;
    if (t != M_LANG_VAL_TYPE_REAL) {
        mln_lang_errmsg(ctx, "Invalid type of argument 1.");
        return NULL;
    }

    memcpy(buf, &(val1->data.f), sizeof(double));
    mln_string_nSet(&tmp, buf, sizeof(double));
    if ((retExp = mln_lang_retExp_createTmpString(ctx, &tmp, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return NULL;
    }
    return retExp;
}

