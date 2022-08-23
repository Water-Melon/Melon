
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

static mln_lang_var_t *mln_lang_print_process(mln_lang_ctx_t *ctx);
static int mln_lang_print_array_cmp(const void *addr1, const void *addr2);
static void mln_lang_print_array(mln_lang_array_t *arr, mln_rbtree_t *check);
static int mln_lang_print_array_elem(mln_rbtree_node_t *node, void *rn_data, void *udata);

int mln_lang_print(mln_lang_ctx_t *ctx)
{
    mln_lang_val_t *val;
    mln_lang_var_t *var;
    mln_lang_func_detail_t *func;
    mln_string_t funcname = mln_string("mln_print");
    mln_string_t v = mln_string("var");
    if ((func = mln_lang_func_detail_new(ctx, M_FUNC_INTERNAL, mln_lang_print_process, NULL, NULL)) == NULL) {
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
    if (mln_lang_symbol_node_join(ctx, M_LANG_SYMBOL_VAR, var) < 0) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_var_free(var);
        return -1;
    }
    return 0;
}

static mln_lang_var_t *mln_lang_print_process(mln_lang_ctx_t *ctx)
{
    mln_s32_t type;
    mln_lang_val_t *val;
    mln_lang_var_t *ret_var;
    mln_string_t var = mln_string("var");
    mln_lang_symbol_node_t *sym;
    if ((sym = mln_lang_symbol_node_search(ctx, &var, 1)) == NULL) {
        ASSERT(0);
        mln_lang_errmsg(ctx, "Argument missing.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);

    type = mln_lang_var_val_type_get(sym->data.var);
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
        {
            struct mln_rbtree_attr rbattr;
            mln_rbtree_t *check;
            rbattr.pool = ctx->pool;
            rbattr.pool_alloc = (rbtree_pool_alloc_handler)mln_alloc_m;
            rbattr.pool_free = (rbtree_pool_free_handler)mln_alloc_free;
            rbattr.cmp = mln_lang_print_array_cmp;
            rbattr.data_free = NULL;
            rbattr.cache = 0;
            if ((check = mln_rbtree_init(&rbattr)) == NULL) {
                mln_lang_errmsg(ctx, "No memory.\n");
                return NULL;
            }
            mln_lang_print_array(val->data.array, check);
            mln_log(none, "\n");
            mln_rbtree_destroy(check);
            break;
        }
        default:
            mln_log(none, "<type error>\n");
            break;
    }

    if ((ret_var = mln_lang_var_create_true(ctx, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return NULL;
    }
    return ret_var;
}

static int mln_lang_print_array_cmp(const void *addr1, const void *addr2)
{
    return (mln_s8ptr_t)addr1 - (mln_s8ptr_t)addr2;
}

static void mln_lang_print_array(mln_lang_array_t *arr, mln_rbtree_t *check)
{
    mln_rbtree_node_t *rn = mln_rbtree_search(check, check->root, arr);
    if (!mln_rbtree_null(rn, check)) {
        mln_log(none, "[...]");
        return;
    }
    rn = mln_rbtree_node_new(check, arr);
    if (rn == NULL) return;
    mln_rbtree_insert(check, rn);
    mln_log(none, "[");
    mln_rbtree_scan_all(arr->elems_index, mln_lang_print_array_elem, check);
    mln_log(none, "]");
}

static int mln_lang_print_array_elem(mln_rbtree_node_t *node, void *rn_data, void *udata)
{
    mln_lang_array_elem_t *elem = (mln_lang_array_elem_t *)rn_data;
    mln_rbtree_t *check = (mln_rbtree_t *)udata;
    mln_lang_var_t *var = elem->value;
    mln_lang_val_t *val = var->val;
    mln_s32_t type = mln_lang_var_val_type_get(var);
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
            mln_lang_print_array(val->data.array, check);
            mln_log(none, ", ");
            break;
        default:
            mln_log(none, "<type error>, ");
            break;
    }
    return 0;
}

