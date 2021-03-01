
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
static void mln_lang_print_array(mln_lang_array_t *arr);
static int mln_lang_print_array_elem(mln_rbtree_node_t *node, void *rn_data, void *udata);

int mln_lang_print(mln_lang_ctx_t *ctx)
{
    mln_lang_val_t *val;
    mln_lang_var_t *var;
    mln_lang_func_detail_t *func;
    mln_string_t funcname = mln_string("mln_print");
    mln_string_t v = mln_string("var");
    if ((func = mln_lang_func_detail_new(ctx, M_FUNC_INTERNAL, mln_lang_print_process, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if ((val = mln_lang_val_new(ctx, M_LANG_VAL_TYPE_NIL, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_func_detail_free(func);
        return -1;
    }
    if ((var = mln_lang_var_new(ctx, &v, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_val_free(val);
        mln_lang_func_detail_free(func);
        return -1;
    }
    func->args_head = func->args_tail = var;
    func->nargs = 1;
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
            mln_log(none, "nil\n");
            break;
        case M_LANG_VAL_TYPE_INT:
            mln_log(none, "%i\n", val->data.i);
            break;
        case M_LANG_VAL_TYPE_BOOL:
            mln_log(none, "%s\n", val->data.b?"true":"false");
            break;
        case M_LANG_VAL_TYPE_REAL:
            mln_log(none, "%f\n", val->data.f);
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
            mln_lang_print_array(val->data.array);
            mln_log(none, "\n");
            break;
        default:
            mln_log(none, "<type error>\n");
            break;
    }

    if ((retExp = mln_lang_retExp_createTmpTrue(ctx, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return NULL;
    }
    return retExp;
}

static void mln_lang_print_array(mln_lang_array_t *arr)
{
    mln_log(none, "[");
    mln_rbtree_scan_all(arr->elems_index, mln_lang_print_array_elem, NULL);
    mln_log(none, "]");
}

static int mln_lang_print_array_elem(mln_rbtree_node_t *node, void *rn_data, void *udata)
{
    mln_lang_array_elem_t *elem = (mln_lang_array_elem_t *)rn_data;
    mln_lang_var_t *var = elem->value;
    mln_lang_val_t *val = var->val;
    mln_s32_t type = mln_lang_var_getValType(var);
    switch (type) {
        case M_LANG_VAL_TYPE_NIL:
            mln_log(none, "nil, ");
            break;
        case M_LANG_VAL_TYPE_INT:
            mln_log(none, "%i, ", val->data.i);
            break;
        case M_LANG_VAL_TYPE_BOOL:
            mln_log(none, "%s, ", val->data.b?"true":"false");
            break;
        case M_LANG_VAL_TYPE_REAL:
            mln_log(none, "%f, ", val->data.f);
            break;
        case M_LANG_VAL_TYPE_STRING:
            mln_log(none, "%S, ", val->data.s);
            break;
        case M_LANG_VAL_TYPE_OBJECT:
            mln_log(none, "OBJECT, ");
            break;
        case M_LANG_VAL_TYPE_FUNC:
            mln_log(none, "FUNCTION, ");
            break;
        case M_LANG_VAL_TYPE_ARRAY:
            mln_lang_print_array(val->data.array);
            mln_log(none, ", ");
            break;
        default:
            mln_log(none, "<type error>, ");
            break;
    }
    return 0;
}

