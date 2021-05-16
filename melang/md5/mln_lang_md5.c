
/*
 * Copyright (C) Niklaus F.Schen.
 */
#include "md5/mln_lang_md5.h"
#include "mln_md5.h"

#ifdef __DEBUG__
#include <assert.h>
#define ASSERT(x) assert(x)
#else
#define ASSERT(x);
#endif

static mln_lang_var_t *mln_lang_md5_process(mln_lang_ctx_t *ctx);

int mln_lang_md5(mln_lang_ctx_t *ctx)
{
    mln_lang_val_t *val;
    mln_lang_var_t *var;
    mln_lang_func_detail_t *func;
    mln_string_t funcname = mln_string("mln_md5");
    mln_string_t v1 = mln_string("data");
    if ((func = mln_lang_func_detail_new(ctx, M_FUNC_INTERNAL, mln_lang_md5_process, NULL, NULL)) == NULL) {
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

static mln_lang_var_t *mln_lang_md5_process(mln_lang_ctx_t *ctx)
{
    mln_s8_t res[33];
    mln_md5_t md5;
    mln_s32_t type;
    mln_lang_val_t *val;
    mln_lang_var_t *var;
    mln_string_t *tmp, t;
    mln_lang_var_t *ret_var;
    mln_string_t v1 = mln_string("data");
    mln_lang_symbol_node_t *sym;

    if ((sym = mln_lang_symbol_node_search(ctx, &v1, 1)) == NULL) {
        ASSERT(0);
        mln_lang_errmsg(ctx, "Argument missing.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);
    val = sym->data.var->val;
    type = mln_lang_var_val_type_get(sym->data.var);
    if (type == M_LANG_VAL_TYPE_STRING) {
        if ((tmp = val->data.s) == NULL) {
            mln_lang_errmsg(ctx, "Invalid argument.");
            return NULL;
        }
        mln_md5_init(&md5);
        mln_md5_calc(&md5, val->data.s->data, val->data.s->len, 1);
        mln_md5_tostring(&md5, res, sizeof(res));
    } else if (type == M_LANG_VAL_TYPE_OBJECT && \
               !mln_string_const_strcmp(val->data.obj->in_set->name, "File"))
    {
        int n, fd;
        mln_string_t v = mln_string("fd");
        mln_u8_t buf[4096];

        if ((var = mln_lang_set_member_search(val->data.obj->members, &v)) == NULL) {
            mln_lang_errmsg(ctx, "Lack of member 'fd'.");
            return NULL;
        }
        if (mln_lang_var_val_type_get(var) != M_LANG_VAL_TYPE_INT) {
            mln_lang_errmsg(ctx, "Invalid type of 'fd'.");
            return NULL;
        }
        val = var->val;
        fd = val->data.i;

        mln_md5_init(&md5);
        while ((n = read(fd, buf, sizeof(buf))) > 0) {
            mln_md5_calc(&md5, buf, n, 0);
        }
        if (n < 0) {
            if ((ret_var = mln_lang_var_create_false(ctx, NULL)) == NULL) {
                mln_lang_errmsg(ctx, "No memory.");
                return NULL;
            }
            return ret_var;
        }
        mln_md5_calc(&md5, NULL, 0, 1);
        mln_md5_tostring(&md5, res, sizeof(res));
    } else {
        mln_lang_errmsg(ctx, "Invalid argument.");
        return NULL;
    }
    mln_string_nset(&t, res, sizeof(res)-1);
    if ((ret_var = mln_lang_var_create_string(ctx, &t, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return NULL;
    }
    return ret_var;
}

