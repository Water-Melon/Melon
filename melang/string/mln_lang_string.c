
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
        "mln_slice"
    };
    mln_lang_internal handlers[] = {
        mln_strcmpSeq_process,
        mln_strcmp_process,
        mln_strstr_process,
        mln_kmp_process,
        mln_slice_process
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
    return 0;
}

static mln_lang_retExp_t *mln_strcmpSeq_process(mln_lang_ctx_t *ctx)
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
            retExp = mln_lang_retExp_createTmpTrue(ctx->pool, NULL);
        } else {
            retExp = mln_lang_retExp_createTmpFalse(ctx->pool, NULL);
        }
    } else {
        if (val2->data.s == NULL) {
            retExp = mln_lang_retExp_createTmpFalse(ctx->pool, NULL);
        } else {
            if (!mln_string_strcmpSeq(val1->data.s, val2->data.s))
                retExp = mln_lang_retExp_createTmpTrue(ctx->pool, NULL);
            else
                retExp = mln_lang_retExp_createTmpFalse(ctx->pool, NULL);
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
            retExp = mln_lang_retExp_createTmpTrue(ctx->pool, NULL);
        } else {
            retExp = mln_lang_retExp_createTmpFalse(ctx->pool, NULL);
        }
    } else {
        if (val2->data.s == NULL) {
            retExp = mln_lang_retExp_createTmpFalse(ctx->pool, NULL);
        } else {
            if (!mln_string_strcmp(val1->data.s, val2->data.s))
                retExp = mln_lang_retExp_createTmpTrue(ctx->pool, NULL);
            else
                retExp = mln_lang_retExp_createTmpFalse(ctx->pool, NULL);
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
        mln_lang_errmsg(ctx, "Invalid type of argument 2.");
        return NULL;
    }
    val3 = sym->data.var->val;

    if (val1->data.s == NULL) {
        if (val2->data.s == NULL) {
            retExp = mln_lang_retExp_createTmpTrue(ctx->pool, NULL);
        } else {
            retExp = mln_lang_retExp_createTmpFalse(ctx->pool, NULL);
        }
    } else {
        if (val2->data.s == NULL) {
            retExp = mln_lang_retExp_createTmpFalse(ctx->pool, NULL);
        } else {
            if (!mln_string_strncmp(val1->data.s, val2->data.s, val3->data.i))
                retExp = mln_lang_retExp_createTmpTrue(ctx->pool, NULL);
            else
                retExp = mln_lang_retExp_createTmpFalse(ctx->pool, NULL);
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
        retExp = mln_lang_retExp_createTmpNil(ctx->pool, NULL);
    } else {
        retExp = mln_lang_retExp_createTmpInt(ctx->pool, \
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
        retExp = mln_lang_retExp_createTmpNil(ctx->pool, NULL);
    } else {
        retExp = mln_lang_retExp_createTmpInt(ctx->pool, \
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
    if (mln_lang_var_getValType(sym->data.var) != M_LANG_VAL_TYPE_INT) {
        mln_lang_errmsg(ctx, "Invalid type of argument 3.");
        return NULL;
    }
    val3 = sym->data.var->val;

    if (val2->data.i < 0) {
        mln_s64_t off = -(val2->data.i);
        p = val1->data.s->data + (val1->data.s->len - (mln_u64_t)off);
        if (val3->data.i >= 0 && val3->data.i < off) off = val3->data.i;
        mln_string_nSet(&ret, p, (mln_u64_t)off);
    } else {
        p = val1->data.s->data + val2->data.i;
        if (val3->data.i >= 0 && val3->data.i < val1->data.s->len-val2->data.i) {
            mln_string_nSet(&ret, p, val3->data.i);
        } else {
            mln_string_nSet(&ret, p, val1->data.s->len - val2->data.i);
        }
    }
    if ((retExp = mln_lang_retExp_createTmpString(ctx->pool, &ret, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return NULL;
    }
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
    if ((retExp = mln_lang_retExp_createTmpArray(ctx->pool, NULL)) == NULL) {
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
        if (mln_lang_var_setValue(ctx->pool, array_val, &var) < 0) {
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
    if ((retExp = mln_lang_retExp_createTmpInt(ctx->pool, val1->data.s->len, NULL)) == NULL) {
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
    if ((retExp = mln_lang_retExp_createTmpString(ctx->pool, &tmp, NULL)) == NULL) {
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
        retExp = mln_lang_retExp_createTmpInt(ctx->pool, i, NULL);
    } else if (!mln_string_constStrcmp(val2->data.s, "real")) {
        double f = 0;
        memcpy(&f, val1->data.s->data, \
               val1->data.s->len>sizeof(double)? \
                   sizeof(double): val1->data.s->len);
        retExp = mln_lang_retExp_createTmpReal(ctx->pool, f, NULL);
    } else if (!mln_string_constStrcmp(val2->data.s, "bool")) {
        retExp = mln_lang_retExp_createTmpBool(ctx->pool, val1->data.s->data[0], NULL);
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

