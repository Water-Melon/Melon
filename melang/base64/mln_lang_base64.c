
/*
 * Copyright (C) Niklaus F.Schen.
 */
#include "base64/mln_lang_base64.h"
#include "mln_base64.h"

#ifdef __DEBUG__
#include <assert.h>
#define ASSERT(x) assert(x)
#else
#define ASSERT(x);
#endif

static mln_lang_var_t *mln_lang_base64_process(mln_lang_ctx_t *ctx);

int mln_lang_base64(mln_lang_ctx_t *ctx)
{
    mln_lang_val_t *val;
    mln_lang_var_t *var;
    mln_lang_func_detail_t *func;
    mln_string_t funcname = mln_string("mln_base64");
    mln_string_t v1 = mln_string("data");
    mln_string_t v2 = mln_string("op");
    if ((func = mln_lang_func_detail_new(ctx, M_FUNC_INTERNAL, mln_lang_base64_process, NULL, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if ((val = mln_lang_val_new(ctx, M_LANG_VAL_TYPE_NIL, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_func_detail_free(func);
        return -1;
    }
    if ((var = mln_lang_var_new(ctx, &v1, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_val_free(val);
        mln_lang_func_detail_free(func);
        return -1;
    }
    mln_lang_var_chain_add(&(func->args_head), &(func->args_tail), var);
    ++func->nargs;
    if ((val = mln_lang_val_new(ctx, M_LANG_VAL_TYPE_NIL, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_func_detail_free(func);
        return -1;
    }
    if ((var = mln_lang_var_new(ctx, &v2, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_val_free(val);
        mln_lang_func_detail_free(func);
        return -1;
    }
    mln_lang_var_chain_add(&(func->args_head), &(func->args_tail), var);
    ++func->nargs;
    if ((val = mln_lang_val_new(ctx, M_LANG_VAL_TYPE_FUNC, func)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_func_detail_free(func);
        return -1;
    }
    if ((var = mln_lang_var_new(ctx, &funcname, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_val_free(val);
        return -1;
    }
    if (mln_lang_symbol_node_join(ctx, M_LANG_SYMBOL_VAR, var) < 0) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_var_free(var);
        return -1;
    }
    return 0;
}

static mln_lang_var_t *mln_lang_base64_process(mln_lang_ctx_t *ctx)
{
    mln_s32_t encode = 0;
    mln_lang_val_t *val;
    mln_string_t *tmp, t;
    mln_lang_var_t *ret_var;
    mln_string_t v1 = mln_string("data");
    mln_string_t v2 = mln_string("op");
    mln_lang_symbol_node_t *sym;
    mln_u8ptr_t outbuf;
    mln_uauto_t outlen;

    if ((sym = mln_lang_symbol_node_search(ctx, &v2, 1)) == NULL) {
        ASSERT(0);
        mln_lang_errmsg(ctx, "Argument missing.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);
    val = sym->data.var->val;
    if (mln_lang_var_val_type_get(sym->data.var) != M_LANG_VAL_TYPE_STRING) {
        mln_lang_errmsg(ctx, "Invalid argument 2.");
        return NULL;
    }
    if ((tmp = val->data.s) == NULL) {
        mln_lang_errmsg(ctx, "Invalid argument 2.");
        return NULL;
    }
    if (!mln_string_const_strcmp(tmp, "encode")) {
        encode = 1;
    } else if (!mln_string_const_strcmp(tmp, "decode")) {
        /*do nothing, encode = 0*/
    } else {
        mln_lang_errmsg(ctx, "Invalid argument 2.");
        return NULL;
    }

    if ((sym = mln_lang_symbol_node_search(ctx, &v1, 1)) == NULL) {
        ASSERT(0);
        mln_lang_errmsg(ctx, "Argument missing.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);
    val = sym->data.var->val;
    if (mln_lang_var_val_type_get(sym->data.var) != M_LANG_VAL_TYPE_STRING) {
        mln_lang_errmsg(ctx, "Invalid argument 1.");
        return NULL;
    }
    if ((tmp = val->data.s) == NULL) {
        mln_lang_errmsg(ctx, "Invalid argument 1.");
        return NULL;
    }

    if (encode) {
        if (mln_base64_encode(tmp->data, tmp->len, &outbuf, &outlen) < 0) {
            mln_lang_errmsg(ctx, "No memory.");
            return NULL;
        }
    } else {
        if (mln_base64_decode(tmp->data, tmp->len, &outbuf, &outlen) < 0) {
            mln_lang_errmsg(ctx, "No memory.");
            return NULL;
        }
    }
    mln_string_nset(&t, outbuf, outlen);

    if ((ret_var = mln_lang_var_create_string(ctx, &t, NULL)) == NULL) {
        mln_base64_free(outbuf);
        mln_lang_errmsg(ctx, "No memory.");
        return NULL;
    }
    mln_base64_free(outbuf);
    return ret_var;
}

