
/*
 * Copyright (C) Niklaus F.Schen.
 */
#include "json/mln_lang_json.h"
#include "mln_json.h"

#ifdef __DEBUG__
#include <assert.h>
#define ASSERT(x) assert(x)
#else
#define ASSERT(x);
#endif

struct mln_lang_json_scan_s {
    mln_lang_ctx_t   *ctx;
    mln_lang_array_t *array;
};

static int mln_lang_json_encode_handler(mln_lang_ctx_t *ctx);
static int mln_lang_json_decode_handler(mln_lang_ctx_t *ctx);
static mln_lang_var_t *mln_lang_json_encode_process(mln_lang_ctx_t *ctx);
static inline mln_json_t *mln_lang_json_encode_generate(mln_lang_array_t *array);
static mln_json_t *mln_lang_json_encode_generate_array(mln_lang_array_t *array);
static inline mln_json_t *
mln_lang_json_encode_generate_obj(mln_lang_array_t *array);
static inline mln_json_t *mln_lang_json_encode_generate_nil(void);
static inline mln_json_t *mln_lang_json_encode_generate_bool(mln_u8_t b);
static inline mln_json_t *mln_lang_json_encode_generate_int(mln_s64_t i);
static inline mln_json_t *mln_lang_json_encode_generate_real(double f);
static inline mln_json_t *mln_lang_json_encode_generate_string(mln_string_t *s);
static int mln_lang_json_encode_obj_scan(mln_rbtree_node_t *node, void *rn_data, void *udata);
static mln_lang_var_t *mln_lang_json_decode_process(mln_lang_ctx_t *ctx);
static inline int
mln_lang_json_decode_obj(mln_lang_ctx_t *ctx, mln_lang_array_t *array, mln_hash_t *obj);
static inline int
mln_lang_json_decode_array(mln_lang_ctx_t *ctx, mln_lang_array_t *array, mln_rbtree_t *a);
static inline int
mln_lang_json_decode_string(mln_lang_ctx_t *ctx, mln_lang_array_t *array, mln_string_t *s, mln_lang_var_t *key);
static inline int
mln_lang_json_decode_number(mln_lang_ctx_t *ctx, mln_lang_array_t *array, double num, mln_lang_var_t *key);
static inline int
mln_lang_json_decode_true(mln_lang_ctx_t *ctx, mln_lang_array_t *array, mln_lang_var_t *key);
static inline int
mln_lang_json_decode_false(mln_lang_ctx_t *ctx, mln_lang_array_t *array, mln_lang_var_t *key);
static inline int
mln_lang_json_decode_null(mln_lang_ctx_t *ctx, mln_lang_array_t *array, mln_lang_var_t *key);
static int mln_lang_json_decode_array_scan(mln_rbtree_node_t *node, void *rn_data, void *udata);
static int mln_lang_json_decode_obj_scan(void *key, void *val, void *data);


int mln_lang_json(mln_lang_ctx_t *ctx)
{
    if (mln_lang_json_encode_handler(ctx) < 0) return -1;
    if (mln_lang_json_decode_handler(ctx) < 0) return -1;
    return 0;
}

static int mln_lang_json_encode_handler(mln_lang_ctx_t *ctx)
{
    mln_lang_val_t *val;
    mln_lang_var_t *var;
    mln_lang_func_detail_t *func;
    mln_string_t funcname = mln_string("mln_json_encode");
    mln_string_t v1 = mln_string("array");
    if ((func = mln_lang_func_detail_new(ctx, M_FUNC_INTERNAL, mln_lang_json_encode_process, NULL, NULL)) == NULL) {
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

static mln_lang_var_t *mln_lang_json_encode_process(mln_lang_ctx_t *ctx)
{
    mln_lang_val_t *val;
    mln_lang_var_t *ret_var;
    mln_string_t v1 = mln_string("array");
    mln_lang_symbol_node_t *sym;
    mln_lang_array_t *array;
    mln_json_t *json;

    if ((sym = mln_lang_symbol_node_search(ctx, &v1, 1)) == NULL) {
        ASSERT(0);
        mln_lang_errmsg(ctx, "Argument missing.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);
    if (mln_lang_var_val_type_get(sym->data.var) != M_LANG_VAL_TYPE_ARRAY) {
        goto fout;
    }
    val = sym->data.var->val;
    if ((array = val->data.array) == NULL) {
        mln_lang_errmsg(ctx, "Invalid argument.");
        return NULL;
    }

    if ((json = mln_lang_json_encode_generate(array)) == NULL) {
fout:
        if ((ret_var = mln_lang_var_create_false(ctx, NULL)) == NULL) {
            mln_lang_errmsg(ctx, "No memory.");
            return NULL;
        }
    } else {
        mln_string_t *s = mln_json_generate(json);
        mln_json_free(json);
        if (s == NULL) {
            mln_lang_errmsg(ctx, "No memory.");
            return NULL;
        }
        if ((ret_var = mln_lang_var_create_ref_string(ctx, s, NULL)) == NULL) {
            mln_string_free(s);
            mln_lang_errmsg(ctx, "No memory.");
            return NULL;
        }
        mln_string_free(s);
    }

    return ret_var;
}

static inline mln_json_t *mln_lang_json_encode_generate(mln_lang_array_t *array)
{
    mln_json_t *json;
    if (!array->elems_key->nr_node) {
        json = mln_lang_json_encode_generate_array(array);
    } else if (array->elems_index->nr_node == array->elems_key->nr_node) {
        json = mln_lang_json_encode_generate_obj(array);
    } else {
        return NULL;
    }
    return json;
}

static mln_json_t *mln_lang_json_encode_generate_array(mln_lang_array_t *array)
{
    mln_s32_t type;
    mln_json_t *json, *j;
    mln_rbtree_node_t *rn;
    mln_lang_var_t *var;
    mln_lang_array_elem_t elem;
    int i, n = array->elems_index->nr_node;
    if ((json = mln_json_new()) == NULL) {
        return NULL;
    }
    M_JSON_SET_TYPE_ARRAY(json);
    for (i = 0; i < n; ++i) {
        elem.index = i;
        rn = mln_rbtree_search(array->elems_index, array->elems_index->root, &elem);
        if (mln_rbtree_null(rn, array->elems_index)) {
            mln_json_free(json);
            return NULL;
        }
        var = ((mln_lang_array_elem_t *)(rn->data))->value;
        type = mln_lang_var_val_type_get(var);
        switch (type) {
            case M_LANG_VAL_TYPE_NIL:
                j = mln_lang_json_encode_generate_nil();
                break;
            case M_LANG_VAL_TYPE_BOOL:
                j = mln_lang_json_encode_generate_bool(var->val->data.b);
                break;
            case M_LANG_VAL_TYPE_INT:
                j = mln_lang_json_encode_generate_int(var->val->data.i);
                break;
            case M_LANG_VAL_TYPE_REAL:
                j = mln_lang_json_encode_generate_real(var->val->data.f);
                break;
            case M_LANG_VAL_TYPE_STRING:
                if (var->val->data.s == NULL) {
                    mln_json_free(json);
                    return NULL;
                }
                j = mln_lang_json_encode_generate_string(var->val->data.s);
                break;
            case M_LANG_VAL_TYPE_OBJECT:
            case M_LANG_VAL_TYPE_FUNC:
                mln_json_free(json);
                return NULL;
            case M_LANG_VAL_TYPE_ARRAY:
            {
                mln_lang_array_t *tmpa;
                if ((tmpa = var->val->data.array) == NULL) {
                    mln_json_free(json);
                    return NULL;
                }
                if (!tmpa->elems_key->nr_node) {
                    j = mln_lang_json_encode_generate_array(tmpa);
                } else if (tmpa->elems_index->nr_node == tmpa->elems_key->nr_node) {
                    j = mln_lang_json_encode_generate_obj(tmpa);
                } else {
                    mln_json_free(json);
                    return NULL;
                }
                break;
            }
            default:
                ASSERT(0);
                j = NULL;
                break;
        }
        if (j == NULL) continue;
        M_JSON_SET_INDEX(j, i);
        if (mln_json_add_element(json, j) < 0) {
            mln_json_free(j);
            mln_json_free(json);
            return NULL;
        }
    }
    return json;
}

static inline mln_json_t *
mln_lang_json_encode_generate_obj(mln_lang_array_t *array)
{
    mln_json_t *json;
    if ((json = mln_json_new()) == NULL) {
        return NULL;
    }
    if (mln_rbtree_scan_all(array->elems_key, \
                            mln_lang_json_encode_obj_scan, \
                            json) < 0)
    {
        mln_json_free(json);
        return NULL;
    }
    return json;
}

static int mln_lang_json_encode_obj_scan(mln_rbtree_node_t *node, void *rn_data, void *udata)
{
    mln_s32_t type;
    mln_string_t *k, *dup;
    mln_lang_var_t *var;
    mln_json_t *jparent = (mln_json_t *)udata, *j, *kj;
    mln_lang_array_elem_t *lae = (mln_lang_array_elem_t *)rn_data;
    if (mln_lang_var_val_type_get(lae->key) != M_LANG_VAL_TYPE_STRING || \
        (k = lae->key->val->data.s) == NULL)
    {
        return -1;
    }
    var = lae->value;
    type = mln_lang_var_val_type_get(var);
    switch (type) {
        case M_LANG_VAL_TYPE_NIL:
            j = mln_lang_json_encode_generate_nil();
            break;
        case M_LANG_VAL_TYPE_BOOL:
            j = mln_lang_json_encode_generate_bool(var->val->data.b);
            break;
        case M_LANG_VAL_TYPE_INT:
            j = mln_lang_json_encode_generate_int(var->val->data.i);
            break;
        case M_LANG_VAL_TYPE_REAL:
            j = mln_lang_json_encode_generate_real(var->val->data.f);
            break;
        case M_LANG_VAL_TYPE_STRING:
            if (var->val->data.s == NULL) return -1;
            j = mln_lang_json_encode_generate_string(var->val->data.s);
            break;
        case M_LANG_VAL_TYPE_OBJECT:
        case M_LANG_VAL_TYPE_FUNC:
            return -1;
        case M_LANG_VAL_TYPE_ARRAY:
        {
            mln_lang_array_t *tmpa;
            if ((tmpa = var->val->data.array) == NULL) {
                return -1;
            }
            if (!tmpa->elems_key->nr_node) {
                j = mln_lang_json_encode_generate_array(tmpa);
            } else if (tmpa->elems_index->nr_node == tmpa->elems_key->nr_node) {
                j = mln_lang_json_encode_generate_obj(tmpa);
            } else {
                return -1;
            }
            break;
        }
        default:
            ASSERT(0);
            j = NULL;
            break;
    }
    if (j == NULL) return 0;

    if ((dup = mln_string_dup(k)) == NULL) {
        mln_json_free(j);
        return -1;
    }
    if ((kj = mln_json_new()) == NULL) {
        mln_string_free(dup);
        mln_json_free(j);
        return -1;
    }
    M_JSON_SET_TYPE_STRING(kj);
    M_JSON_SET_DATA_STRING(kj, dup);
    if (mln_json_update_obj(jparent, kj, j) < 0) {
        mln_json_free(kj);
        mln_json_free(j);
        return -1;
    }
    return 0;
}

static inline mln_json_t *mln_lang_json_encode_generate_nil(void)
{
    mln_json_t *json;
    if ((json = mln_json_new()) == NULL) return NULL;
    M_JSON_SET_TYPE_NULL(json);
    M_JSON_SET_DATA_NULL(json);
    return json;
}

static inline mln_json_t *mln_lang_json_encode_generate_bool(mln_u8_t b)
{
    mln_json_t *json;
    if ((json = mln_json_new()) == NULL) return NULL;
    if (b) {
        M_JSON_SET_TYPE_TRUE(json);
        M_JSON_SET_DATA_TRUE(json);
    } else {
        M_JSON_SET_TYPE_FALSE(json);
        M_JSON_SET_DATA_FALSE(json);
    }
    return json;
}

static inline mln_json_t *mln_lang_json_encode_generate_int(mln_s64_t i)
{
    mln_json_t *json;
    if ((json = mln_json_new()) == NULL) return NULL;
    M_JSON_SET_TYPE_NUMBER(json);
    M_JSON_SET_DATA_NUMBER(json, i);
    return json;
}

static inline mln_json_t *mln_lang_json_encode_generate_real(double f)
{
    mln_json_t *json;
    if ((json = mln_json_new()) == NULL) return NULL;
    M_JSON_SET_TYPE_NUMBER(json);
    M_JSON_SET_DATA_NUMBER(json, f);
    return json;
}

static inline mln_json_t *mln_lang_json_encode_generate_string(mln_string_t *s)
{
    mln_json_t *json;
    mln_string_t *dup;
    if ((json = mln_json_new()) == NULL) return NULL;
    if ((dup = mln_string_dup(s)) == NULL) {
        mln_json_free(json);
        return NULL;
    }
    M_JSON_SET_TYPE_STRING(json);
    M_JSON_SET_DATA_STRING(json, dup);
    return json;
}


static int mln_lang_json_decode_handler(mln_lang_ctx_t *ctx)
{
    mln_lang_val_t *val;
    mln_lang_var_t *var;
    mln_lang_func_detail_t *func;
    mln_string_t funcname = mln_string("mln_json_decode");
    mln_string_t v1 = mln_string("s");
    if ((func = mln_lang_func_detail_new(ctx, M_FUNC_INTERNAL, mln_lang_json_decode_process, NULL, NULL)) == NULL) {
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

static mln_lang_var_t *mln_lang_json_decode_process(mln_lang_ctx_t *ctx)
{
    int rc = 0;
    mln_lang_val_t *val;
    mln_lang_var_t *ret_var;
    mln_string_t v1 = mln_string("s");
    mln_lang_symbol_node_t *sym;
    mln_lang_array_t *array;
    mln_json_t *json;

    if ((sym = mln_lang_symbol_node_search(ctx, &v1, 1)) == NULL) {
        ASSERT(0);
        mln_lang_errmsg(ctx, "Argument missing.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);
    if (mln_lang_var_val_type_get(sym->data.var) != M_LANG_VAL_TYPE_STRING) {
        mln_lang_errmsg(ctx, "Invalid type of argument.");
        return NULL;
    }
    val = sym->data.var->val;
    if (val->data.s == NULL) {
        mln_lang_errmsg(ctx, "Invalid argument.");
        return NULL;
    }
    if ((ret_var = mln_lang_var_create_array(ctx, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return NULL;
    }
    array = ret_var->val->data.array;

    if ((json = mln_json_parse(val->data.s)) == NULL) {
        mln_lang_var_free(ret_var);
        if ((ret_var = mln_lang_var_create_false(ctx, NULL)) == NULL) {
            mln_lang_errmsg(ctx, "No memory.");
            return NULL;
        }
    } else {
        if (M_JSON_IS_OBJECT(json)) {
            rc = mln_lang_json_decode_obj(ctx, array, json->data.m_j_obj);
        } else if (M_JSON_IS_ARRAY(json)) {
            rc = mln_lang_json_decode_array(ctx, array, json->data.m_j_array);
        } else if (M_JSON_IS_STRING(json)) {
            rc = mln_lang_json_decode_string(ctx, array, json->data.m_j_string, NULL);
        } else if (M_JSON_IS_NUMBER(json)) {
            rc = mln_lang_json_decode_number(ctx, array, json->data.m_j_number, NULL);
        } else if (M_JSON_IS_TRUE(json)) {
            rc = mln_lang_json_decode_true(ctx, array, NULL);
        } else if (M_JSON_IS_FALSE(json)) {
            rc = mln_lang_json_decode_false(ctx, array, NULL);
        } else if (M_JSON_IS_NULL(json)) {
            rc = mln_lang_json_decode_null(ctx, array, NULL);
        } else { /*M_JSON_IS_NONE*/
            /*do nothing*/
        }
        mln_json_free(json);
        if (rc < 0) {
            mln_lang_var_free(ret_var);
            if ((ret_var = mln_lang_var_create_false(ctx, NULL)) == NULL) {
                mln_lang_errmsg(ctx, "No memory.");
                return NULL;
            }
        }
    }
    return ret_var;
}

static int
mln_lang_json_decode_obj(mln_lang_ctx_t *ctx, mln_lang_array_t *array, mln_hash_t *obj)
{
    struct mln_lang_json_scan_s ljs;
    ljs.ctx = ctx;
    ljs.array = array;
    if (mln_hash_scan_all(obj, mln_lang_json_decode_obj_scan, &ljs) < 0) {
        return -1;
    }
    return 0;
}

static int mln_lang_json_decode_obj_scan(void *key, void *val, void *data)
{
    int rc = 0;
    mln_json_obj_t *jo = (mln_json_obj_t *)val;
    struct mln_lang_json_scan_s *ljs = (struct mln_lang_json_scan_s *)data;
    mln_lang_var_t kvar;
    mln_lang_val_t kval;
    kvar.type = M_LANG_VAR_NORMAL;
    kvar.name = NULL;
    kvar.val = &kval;
    kvar.in_set = NULL;
    kvar.prev = kvar.next = NULL;
    kval.data.s = (mln_string_t *)key;
    kval.type = M_LANG_VAL_TYPE_STRING;
    kval.ref = 1;

    if (M_JSON_IS_OBJECT(jo->val)) {
        mln_lang_array_t *tmpa;
        mln_lang_var_t *array_val, var;
        mln_lang_val_t val;
        if ((tmpa = mln_lang_array_new(ljs->ctx)) == NULL) {
            return -1;
        }
        ++(tmpa->ref);
        if ((rc = mln_lang_json_decode_obj(ljs->ctx, tmpa, jo->val->data.m_j_obj)) < 0) {
            mln_lang_array_free(tmpa);
            return rc;
        }
        if ((array_val = mln_lang_array_get(ljs->ctx, ljs->array, &kvar)) == NULL) {
            mln_lang_array_free(tmpa);
            return -1;
        }
        var.type = M_LANG_VAR_NORMAL;
        var.name = NULL;
        var.val = &val;
        var.in_set = NULL;
        var.prev = var.next = NULL;
        val.data.array = tmpa;
        val.type = M_LANG_VAL_TYPE_ARRAY;
        val.ref = 1;
        if (mln_lang_var_value_set(ljs->ctx, array_val, &var) < 0) {
            mln_lang_array_free(tmpa);
            return -1;
        }
    } else if (M_JSON_IS_ARRAY(jo->val)) {
        mln_lang_array_t *tmpa;
        mln_lang_var_t *array_val, var;
        mln_lang_val_t val;
        if ((tmpa = mln_lang_array_new(ljs->ctx)) == NULL) {
            return -1;
        }
        ++(tmpa->ref);
        if ((rc = mln_lang_json_decode_array(ljs->ctx, tmpa, jo->val->data.m_j_array)) < 0) {
            mln_lang_array_free(tmpa);
            return rc;
        }
        if ((array_val = mln_lang_array_get(ljs->ctx, ljs->array, &kvar)) == NULL) {
            mln_lang_array_free(tmpa);
            return -1;
        }
        var.type = M_LANG_VAR_NORMAL;
        var.name = NULL;
        var.val = &val;
        var.in_set = NULL;
        var.prev = var.next = NULL;
        val.data.array = tmpa;
        val.type = M_LANG_VAL_TYPE_ARRAY;
        val.ref = 1;
        if (mln_lang_var_value_set(ljs->ctx, array_val, &var) < 0) {
            mln_lang_array_free(tmpa);
            return -1;
        }
    } else if (M_JSON_IS_STRING(jo->val)) {
        rc = mln_lang_json_decode_string(ljs->ctx, ljs->array, jo->val->data.m_j_string, &kvar);
    } else if (M_JSON_IS_NUMBER(jo->val)) {
        rc = mln_lang_json_decode_number(ljs->ctx, ljs->array, jo->val->data.m_j_number, &kvar);
    } else if (M_JSON_IS_TRUE(jo->val)) {
        rc = mln_lang_json_decode_true(ljs->ctx, ljs->array, &kvar);
    } else if (M_JSON_IS_FALSE(jo->val)) {
        rc = mln_lang_json_decode_false(ljs->ctx, ljs->array, &kvar);
    } else if (M_JSON_IS_NULL(jo->val)) {
        rc = mln_lang_json_decode_null(ljs->ctx, ljs->array, &kvar);
    } else { /*M_JSON_IS_NONE*/
        /*do nothing*/
    }

    return rc;
}

static int
mln_lang_json_decode_array(mln_lang_ctx_t *ctx, mln_lang_array_t *array, mln_rbtree_t *a)
{
    struct mln_lang_json_scan_s ljs;
    ljs.ctx = ctx;
    ljs.array = array;
    if (mln_rbtree_scan_all(a, mln_lang_json_decode_array_scan, &ljs) < 0) {
        return -1;
    }
    return 0;
}

static int mln_lang_json_decode_array_scan(mln_rbtree_node_t *node, void *rn_data, void *udata)
{
    int rc = 0;
    mln_json_t *json = (mln_json_t *)rn_data;
    struct mln_lang_json_scan_s *ljs = (struct mln_lang_json_scan_s *)udata;
    mln_lang_var_t kvar;
    mln_lang_val_t kval;
    kvar.type = M_LANG_VAR_NORMAL;
    kvar.name = NULL;
    kvar.val = &kval;
    kvar.in_set = NULL;
    kvar.prev = kvar.next = NULL;
    kval.data.i = json->index - 1;
    ASSERT(kval.data.i >= 0);
    kval.type = M_LANG_VAL_TYPE_INT;
    kval.ref = 1;

    if (M_JSON_IS_OBJECT(json)) {
        mln_lang_array_t *tmpa;
        mln_lang_var_t *array_val, var;
        mln_lang_val_t val;
        if ((tmpa = mln_lang_array_new(ljs->ctx)) == NULL) {
            return -1;
        }
        ++(tmpa->ref);
        if ((rc = mln_lang_json_decode_obj(ljs->ctx, tmpa, json->data.m_j_obj)) < 0) {
            mln_lang_array_free(tmpa);
            return rc;
        }
        if ((array_val = mln_lang_array_get(ljs->ctx, ljs->array, &kvar)) == NULL) {
            mln_lang_array_free(tmpa);
            return -1;
        }
        var.type = M_LANG_VAR_NORMAL;
        var.name = NULL;
        var.val = &val;
        var.in_set = NULL;
        var.prev = var.next = NULL;
        val.data.array = tmpa;
        val.type = M_LANG_VAL_TYPE_ARRAY;
        val.ref = 1;
        if (mln_lang_var_value_set(ljs->ctx, array_val, &var) < 0) {
            mln_lang_array_free(tmpa);
            return -1;
        }
    } else if (M_JSON_IS_ARRAY(json)) {
        mln_lang_array_t *tmpa;
        mln_lang_var_t *array_val, var;
        mln_lang_val_t val;
        if ((tmpa = mln_lang_array_new(ljs->ctx)) == NULL) {
            return -1;
        }
        ++(tmpa->ref);
        if ((rc = mln_lang_json_decode_array(ljs->ctx, tmpa, json->data.m_j_array)) < 0) {
            mln_lang_array_free(tmpa);
            return rc;
        }
        if ((array_val = mln_lang_array_get(ljs->ctx, ljs->array, &kvar)) == NULL) {
            mln_lang_array_free(tmpa);
            return -1;
        }
        var.type = M_LANG_VAR_NORMAL;
        var.name = NULL;
        var.val = &val;
        var.in_set = NULL;
        var.prev = var.next = NULL;
        val.data.array = tmpa;
        val.type = M_LANG_VAL_TYPE_ARRAY;
        val.ref = 1;
        if (mln_lang_var_value_set(ljs->ctx, array_val, &var) < 0) {
            mln_lang_array_free(tmpa);
            return -1;
        }
    } else if (M_JSON_IS_STRING(json)) {
        rc = mln_lang_json_decode_string(ljs->ctx, ljs->array, json->data.m_j_string, &kvar);
    } else if (M_JSON_IS_NUMBER(json)) {
        rc = mln_lang_json_decode_number(ljs->ctx, ljs->array, json->data.m_j_number, &kvar);
    } else if (M_JSON_IS_TRUE(json)) {
        rc = mln_lang_json_decode_true(ljs->ctx, ljs->array, &kvar);
    } else if (M_JSON_IS_FALSE(json)) {
        rc = mln_lang_json_decode_false(ljs->ctx, ljs->array, &kvar);
    } else if (M_JSON_IS_NULL(json)) {
        rc = mln_lang_json_decode_null(ljs->ctx, ljs->array, &kvar);
    } else { /*M_JSON_IS_NONE*/
        /*do nothing*/
    }
    return rc;
}

static int
mln_lang_json_decode_string(mln_lang_ctx_t *ctx, mln_lang_array_t *array, mln_string_t *s, mln_lang_var_t *key)
{
    mln_lang_var_t *array_val, var;
    mln_lang_val_t val;
    if ((array_val = mln_lang_array_get(ctx, array, key)) == NULL) {
        return -1;
    }
    var.type = M_LANG_VAR_NORMAL;
    var.name = NULL;
    var.val = &val;
    var.in_set = NULL;
    var.prev = var.next = NULL;
    val.data.s = s;
    val.type = M_LANG_VAL_TYPE_STRING;
    val.ref = 1;
    if (mln_lang_var_value_set(ctx, array_val, &var) < 0) {
        return -1;
    }
    return 0;
}

static int
mln_lang_json_decode_number(mln_lang_ctx_t *ctx, mln_lang_array_t *array, double num, mln_lang_var_t *key)
{
    mln_lang_var_t *array_val, var;
    mln_lang_val_t val;
    mln_s64_t i = (mln_s64_t)num;
    if ((array_val = mln_lang_array_get(ctx, array, key)) == NULL) {
        return -1;
    }
    var.type = M_LANG_VAR_NORMAL;
    var.name = NULL;
    var.val = &val;
    var.in_set = NULL;
    var.prev = var.next = NULL;
    if (i == num) {
        val.data.i = num;
        val.type = M_LANG_VAL_TYPE_INT;
    } else {
        val.data.f = num;
        val.type = M_LANG_VAL_TYPE_REAL;
    }
    val.ref = 1;
    if (mln_lang_var_value_set(ctx, array_val, &var) < 0) {
        return -1;
    }
    return 0;
}

static int
mln_lang_json_decode_true(mln_lang_ctx_t *ctx, mln_lang_array_t *array, mln_lang_var_t *key)
{
    mln_lang_var_t *array_val, var;
    mln_lang_val_t val;
    if ((array_val = mln_lang_array_get(ctx, array, key)) == NULL) {
        return -1;
    }
    var.type = M_LANG_VAR_NORMAL;
    var.name = NULL;
    var.val = &val;
    var.in_set = NULL;
    var.prev = var.next = NULL;
    val.data.b = 1;
    val.type = M_LANG_VAL_TYPE_BOOL;
    val.ref = 1;
    if (mln_lang_var_value_set(ctx, array_val, &var) < 0) {
        return -1;
    }
    return 0;
}

static int
mln_lang_json_decode_false(mln_lang_ctx_t *ctx, mln_lang_array_t *array, mln_lang_var_t *key)
{
    mln_lang_var_t *array_val, var;
    mln_lang_val_t val;
    if ((array_val = mln_lang_array_get(ctx, array, key)) == NULL) {
        return -1;
    }
    var.type = M_LANG_VAR_NORMAL;
    var.name = NULL;
    var.val = &val;
    var.in_set = NULL;
    var.prev = var.next = NULL;
    val.data.b = 0;
    val.type = M_LANG_VAL_TYPE_BOOL;
    val.ref = 1;
    if (mln_lang_var_value_set(ctx, array_val, &var) < 0) {
        return -1;
    }
    return 0;
}

static int
mln_lang_json_decode_null(mln_lang_ctx_t *ctx, mln_lang_array_t *array, mln_lang_var_t *key)
{
    mln_lang_var_t *array_val;
    if ((array_val = mln_lang_array_get(ctx, array, key)) == NULL) {
        return -1;
    }
    return 0;
}

