
/*
 * Copyright (C) Niklaus F.Schen.
 */

#include "print/mln_lang_print.h"
#include "mln_log.h"

#ifdef __DEBUG__
#include <assert.h>
#define ASSERT(x) assert(x)
#else
#define ASSERT(x);
#endif

static mln_lang_retExp_t *mln_lang_print_process(mln_lang_ctx_t *ctx);

int mln_lang_print(mln_lang_ctx_t *ctx)
{
    mln_lang_val_t *val;
    mln_lang_var_t *var;
    mln_lang_func_detail_t *func;
    mln_string_t funcname = mln_string("mln_print");
    mln_string_t v = mln_string("var");
    if ((func = mln_lang_func_detail_new(ctx->pool, M_FUNC_INTERNAL, mln_lang_print_process, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if ((val = mln_lang_val_new(ctx->pool, M_LANG_VAL_TYPE_NIL, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_func_detail_free(func);
        return -1;
    }
    if ((var = mln_lang_var_new(ctx->pool, &v, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_val_free(val);
        mln_lang_func_detail_free(func);
        return -1;
    }
    func->args_head = func->args_tail = var;
    func->nargs = 1;
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

static mln_lang_retExp_t *mln_lang_print_process(mln_lang_ctx_t *ctx)
{
    mln_s32_t type;
    mln_lang_val_t *val;
    mln_lang_retExp_t *retExp;
    mln_string_t var = mln_string("var");
    mln_lang_symbolNode_t *sym;
    if ((sym = mln_lang_symbolNode_search(ctx, &var, 1)) == NULL) {
        ASSERT(0);
        mln_lang_errmsg(ctx, "Argument missing.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);

    type = mln_lang_var_getValType(sym->data.var);
    val = sym->data.var->val;
    switch (type) {
        case M_LANG_VAL_TYPE_NIL:
            mln_log(none, "NIL\n");
            break;
        case M_LANG_VAL_TYPE_INT:
            mln_log(none, "%i\n", val->data.i);
            break;
        case M_LANG_VAL_TYPE_BOOL:
            mln_log(none, "%s\n", val->data.b?"True":"False");
            break;
        case M_LANG_VAL_TYPE_REAL:
            mln_log(none, "%lf\n", val->data.f);
            break;
        case M_LANG_VAL_TYPE_STRING:
            mln_log(none, "%S\n", val->data.s);
            break;
        case M_LANG_VAL_TYPE_OBJECT:
            mln_log(none, "OBJECT\n");
            break;
        case M_LANG_VAL_TYPE_FUNC:
            mln_log(none, "FUNCTION\n");
            break;
        case M_LANG_VAL_TYPE_ARRAY:
            mln_log(none, "ARRAY\n");
            break;
        default:
            mln_log(none, "type error\n");
            break;
    }

    if ((retExp = mln_lang_retExp_createTmpTrue(ctx->pool, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return NULL;
    }
    return retExp;
}

