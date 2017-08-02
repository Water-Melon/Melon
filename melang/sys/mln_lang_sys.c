
/*
 * Copyright (C) Niklaus F.Schen.
 */
#include "sys/mln_lang_sys.h"
#include <stdio.h>
#include <stdlib.h>

#ifdef __DEBUG__
#include <assert.h>
#define ASSERT(x) assert(x)
#else
#define ASSERT(x);
#endif

static int mln_lang_sys_size_handler(mln_lang_ctx_t *ctx);
static mln_lang_retExp_t *mln_lang_sys_size_process(mln_lang_ctx_t *ctx);

int mln_lang_sys(mln_lang_ctx_t *ctx)
{
    if (mln_lang_sys_size_handler(ctx) < 0) return -1;
    return 0;
}

static int mln_lang_sys_size_handler(mln_lang_ctx_t *ctx)
{
    mln_lang_val_t *val;
    mln_lang_var_t *var;
    mln_lang_func_detail_t *func;
    mln_string_t funcname = mln_string("mln_size");
    mln_string_t v1 = mln_string("array");
    if ((func = mln_lang_func_detail_new(ctx->pool, M_FUNC_INTERNAL, mln_lang_sys_size_process, NULL)) == NULL) {
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

static mln_lang_retExp_t *mln_lang_sys_size_process(mln_lang_ctx_t *ctx)
{
    mln_lang_retExp_t *retExp;
    mln_string_t v1 = mln_string("array");
    mln_lang_symbolNode_t *sym;

    if ((sym = mln_lang_symbolNode_search(ctx, &v1, 1)) == NULL) {
        ASSERT(0);
        mln_lang_errmsg(ctx, "Argument missing.");
        return NULL;
    }
    if (sym->type != M_LANG_SYMBOL_VAR || \
        mln_lang_var_getValType(sym->data.var) != M_LANG_VAL_TYPE_ARRAY)
    {
        mln_lang_errmsg(ctx, "Invalid type of argument 1.");
        return NULL;
    }
    if ((retExp = mln_lang_retExp_createTmpInt(ctx->pool, \
                                               sym->data.var->val->data.array->elems_index->nr_node, \
                                               NULL)) == NULL)
    {
        mln_lang_errmsg(ctx, "No memory.");
        return NULL;
    }
    return retExp;
}

