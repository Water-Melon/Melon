
/*
 * Copyright (C) Niklaus F.Schen.
 */
#include <stdlib.h>
#include "des/mln_lang_des.h"
#include "mln_des.h"

#ifdef __DEBUG__
#include <assert.h>
#define ASSERT(x) assert(x)
#else
#define ASSERT(x);
#endif

static int mln_lang_des_handler(mln_lang_ctx_t *ctx);
static int mln_lang_3des_handler(mln_lang_ctx_t *ctx);
static mln_lang_var_t *mln_lang_des_process(mln_lang_ctx_t *ctx);
static mln_lang_var_t *mln_lang_3des_process(mln_lang_ctx_t *ctx);

int mln_lang_des(mln_lang_ctx_t *ctx)
{
    if (mln_lang_des_handler(ctx) < 0) return -1;
    if (mln_lang_3des_handler(ctx) < 0) return -1;
    return 0;
}

static int mln_lang_des_handler(mln_lang_ctx_t *ctx)
{
    mln_lang_val_t *val;
    mln_lang_var_t *var;
    mln_lang_func_detail_t *func;
    mln_string_t funcname = mln_string("mln_des");
    mln_string_t v1 = mln_string("data");
    mln_string_t v2 = mln_string("key");
    mln_string_t v3 = mln_string("op");
    if ((func = mln_lang_func_detail_new(ctx, M_FUNC_INTERNAL, mln_lang_des_process, NULL, NULL)) == NULL) {
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
    if ((val = mln_lang_val_new(ctx, M_LANG_VAL_TYPE_NIL, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_func_detail_free(func);
        return -1;
    }
    if ((var = mln_lang_var_new(ctx, &v3, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
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

static mln_lang_var_t *mln_lang_des_process(mln_lang_ctx_t *ctx)
{
    mln_s32_t encode = 0;
    mln_lang_val_t *val;
    mln_string_t *tmp, t;
    mln_lang_var_t *ret_var;
    mln_string_t v1 = mln_string("data");
    mln_string_t v2 = mln_string("key");
    mln_string_t v3 = mln_string("op");
    mln_lang_symbol_node_t *sym;
    mln_u8ptr_t outbuf;
    mln_des_t des;

    if ((sym = mln_lang_symbol_node_search(ctx, &v3, 1)) == NULL) {
        ASSERT(0);
        mln_lang_errmsg(ctx, "Argument missing.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);
    val = sym->data.var->val;
    if (mln_lang_var_val_type_get(sym->data.var) != M_LANG_VAL_TYPE_STRING) {
        mln_lang_errmsg(ctx, "Invalid argument 3.");
        return NULL;
    }
    if ((tmp = val->data.s) == NULL) {
        mln_lang_errmsg(ctx, "Invalid argument 3.");
        return NULL;
    }
    if (!mln_string_const_strcmp(tmp, "encode")) {
        encode = 1;
    } else if (!mln_string_const_strcmp(tmp, "decode")) {
        /*do nothing, encode = 0*/
    } else {
        mln_lang_errmsg(ctx, "Invalid argument 3.");
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
    if (tmp->len % 8) {
        mln_lang_errmsg(ctx, "Invalid argument 1.");
        return NULL;
    }

    if ((sym = mln_lang_symbol_node_search(ctx, &v2, 1)) == NULL) {
        ASSERT(0);
        mln_lang_errmsg(ctx, "Argument missing.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);
    if (mln_lang_var_val_type_get(sym->data.var) != M_LANG_VAL_TYPE_INT) {
        mln_lang_errmsg(ctx, "Invalid argument 2.");
        return NULL;
    }
    val = sym->data.var->val;

    mln_des_init(&des, (mln_u64_t)(val->data.i));
    if ((outbuf = (mln_u8ptr_t)malloc(tmp->len)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return NULL;
    }
    mln_des_buf(&des, tmp->data, tmp->len, outbuf, tmp->len, 0, encode);
    mln_string_nset(&t, outbuf, tmp->len);

    ret_var = mln_lang_var_create_string(ctx, &t, NULL);
    free(outbuf);
    if (ret_var == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return NULL;
    }
    return ret_var;
}

static int mln_lang_3des_handler(mln_lang_ctx_t *ctx)
{
    mln_lang_val_t *val;
    mln_lang_var_t *var;
    mln_lang_func_detail_t *func;
    mln_string_t funcname = mln_string("mln_3des");
    mln_string_t v1 = mln_string("data");
    mln_string_t v2 = mln_string("key1");
    mln_string_t v3 = mln_string("key2");
    mln_string_t v4 = mln_string("op");
    if ((func = mln_lang_func_detail_new(ctx, M_FUNC_INTERNAL, mln_lang_3des_process, NULL, NULL)) == NULL) {
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
    if ((val = mln_lang_val_new(ctx, M_LANG_VAL_TYPE_NIL, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_func_detail_free(func);
        return -1;
    }
    if ((var = mln_lang_var_new(ctx, &v3, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
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
    if ((var = mln_lang_var_new(ctx, &v4, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
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

static mln_lang_var_t *mln_lang_3des_process(mln_lang_ctx_t *ctx)
{
    mln_s32_t encode = 0;
    mln_u64_t k1, k2;
    mln_lang_val_t *val;
    mln_string_t *tmp, t;
    mln_lang_var_t *ret_var;
    mln_string_t v1 = mln_string("data");
    mln_string_t v2 = mln_string("key1");
    mln_string_t v3 = mln_string("key2");
    mln_string_t v4 = mln_string("op");
    mln_lang_symbol_node_t *sym;
    mln_u8ptr_t outbuf;
    mln_3des_t _3des;

    if ((sym = mln_lang_symbol_node_search(ctx, &v4, 1)) == NULL) {
        ASSERT(0);
        mln_lang_errmsg(ctx, "Argument missing.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);
    val = sym->data.var->val;
    if (mln_lang_var_val_type_get(sym->data.var) != M_LANG_VAL_TYPE_STRING) {
        mln_lang_errmsg(ctx, "Invalid argument 4.");
        return NULL;
    }
    if ((tmp = val->data.s) == NULL) {
        mln_lang_errmsg(ctx, "Invalid argument 4.");
        return NULL;
    }
    if (!mln_string_const_strcmp(tmp, "encode")) {
        encode = 1;
    } else if (!mln_string_const_strcmp(tmp, "decode")) {
        /*do nothing, encode = 0*/
    } else {
        mln_lang_errmsg(ctx, "Invalid argument 4.");
        return NULL;
    }

    if ((sym = mln_lang_symbol_node_search(ctx, &v2, 1)) == NULL) {
        ASSERT(0);
        mln_lang_errmsg(ctx, "Argument missing.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);
    val = sym->data.var->val;
    if (mln_lang_var_val_type_get(sym->data.var) != M_LANG_VAL_TYPE_INT) {
        mln_lang_errmsg(ctx, "Invalid argument 2.");
        return NULL;
    }
    k1 = (mln_u64_t)(val->data.i);

    if ((sym = mln_lang_symbol_node_search(ctx, &v3, 1)) == NULL) {
        ASSERT(0);
        mln_lang_errmsg(ctx, "Argument missing.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);
    val = sym->data.var->val;
    if (mln_lang_var_val_type_get(sym->data.var) != M_LANG_VAL_TYPE_INT) {
        mln_lang_errmsg(ctx, "Invalid argument 3.");
        return NULL;
    }
    k2 = (mln_u64_t)(val->data.i);

    if ((sym = mln_lang_symbol_node_search(ctx, &v1, 1)) == NULL) {
        ASSERT(0);
        mln_lang_errmsg(ctx, "Argument missing.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);
    if (mln_lang_var_val_type_get(sym->data.var) != M_LANG_VAL_TYPE_STRING) {
        mln_lang_errmsg(ctx, "Invalid argument 1.");
        return NULL;
    }
    val = sym->data.var->val;
    if ((tmp = val->data.s) == NULL) {
        mln_lang_errmsg(ctx, "Invalid argument 1.");
        return NULL;
    }
    if (tmp->len % 8) {
        mln_lang_errmsg(ctx, "Invalid argument 1.");
        return NULL;
    }

    mln_3des_init(&_3des, k1, k2);
    if ((outbuf = (mln_u8ptr_t)malloc(tmp->len)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return NULL;
    }
    mln_3des_buf(&_3des, tmp->data, tmp->len, outbuf, tmp->len, 0, encode);
    mln_string_nset(&t, outbuf, tmp->len);

    ret_var = mln_lang_var_create_string(ctx, &t, NULL);
    free(outbuf);
    if (ret_var == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return NULL;
    }
    return ret_var;
}

