
/*
 * Copyright (C) Niklaus F.Schen.
 */
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include "msgqueue/mln_lang_msgqueue.h"

#ifdef __DEBUG__
#include <assert.h>
#define ASSERT(x) assert(x)
#else
#define ASSERT(x);
#endif

static int mln_lang_msgqueue_resource_register(mln_lang_ctx_t *ctx);
static void mln_lang_msgqueue_resource_cancel(mln_lang_ctx_t *ctx);
static int mln_lang_msgqueue_mq_send(mln_lang_ctx_t *ctx);
static mln_lang_var_t *mln_lang_msgqueue_mq_send_process(mln_lang_ctx_t *ctx);
static int mln_lang_mq_msg_broadcast_ctx(mln_lang_ctx_t *ctx, mln_string_t *qname, int type, void *data);
static int mln_lang_msgqueue_mq_recv(mln_lang_ctx_t *ctx);
static mln_lang_var_t *mln_lang_msgqueue_mq_recv_process(mln_lang_ctx_t *ctx);
static mln_lang_var_t *mln_lang_mq_msg_get(mln_lang_ctx_t *ctx, mln_string_t *qname, mln_s64_t timeout);
static int mln_lang_mq_msg_subscribe_get(mln_lang_ctx_t *ctx, mln_string_t *qname, mln_lang_var_t **ret_var);
static void mln_lang_msgqueue_timeout_handler(mln_event_t *ev, void *data);
static mln_lang_var_t *mln_lang_mq_msg_set(mln_lang_ctx_t *ctx, mln_string_t *qname, int type, void *data);
static mln_lang_var_t *mln_lang_mq_msg_broadcast(mln_lang_ctx_t *ctx, mln_string_t *qname, int type, void *data);
static int mln_lang_msgqueue_topic_subscribe(mln_lang_ctx_t *ctx);
static mln_lang_var_t *mln_lang_msgqueue_topic_subscribe_process(mln_lang_ctx_t *ctx);
static int mln_lang_msgqueue_topic_unsubscribe(mln_lang_ctx_t *ctx);
static mln_lang_var_t *mln_lang_msgqueue_topic_unsubscribe_process(mln_lang_ctx_t *ctx);
/*components*/
MLN_CHAIN_FUNC_DECLARE(mln_lang_mq_msg, mln_lang_mq_msg_t, static inline void,);
MLN_CHAIN_FUNC_DECLARE(mln_lang_mq_wait, mln_lang_mq_wait_t, static inline void,);
static mln_lang_mq_msg_t *mln_lang_mq_msg_new(mln_lang_t *lang, int type, void *data);
static void mln_lang_mq_msg_free(mln_lang_mq_msg_t *msg);
static mln_lang_mq_wait_t *mln_lang_mq_wait_new(mln_lang_ctx_t *ctx, mln_lang_mq_t *mq);
static void mln_lang_mq_wait_free(mln_lang_mq_wait_t *lmw);
static int mln_lang_mq_wait_cmp(const mln_lang_mq_wait_t *lmw1, const mln_lang_mq_wait_t *lmw2);
static void mln_lang_mq_wait_copy(mln_lang_mq_wait_t *lmw1, mln_lang_mq_wait_t *lmw2);
static mln_lang_mq_t *mln_lang_mq_new(mln_string_t *name);
static int mln_lang_mq_cmp(const mln_lang_mq_t *lm1, const mln_lang_mq_t *lm2);
static void mln_lang_mq_free(mln_lang_mq_t *lm);
static mln_lang_ctx_mq_t *mln_lang_ctx_mq_new(mln_lang_ctx_t *ctx);
static void mln_lang_ctx_mq_free(mln_lang_ctx_mq_t *lcm);
static mln_lang_mq_t *mln_lang_mq_fetch(mln_lang_t *lang, mln_string_t *qname);
static void mln_lang_ctx_mq_join(mln_lang_ctx_t *ctx, mln_lang_mq_wait_t *wait);
static void mln_lang_ctx_mq_remove(mln_lang_ctx_t *ctx);
static mln_lang_ctx_mq_topic_t *mln_lang_ctx_mq_topic_new(mln_lang_ctx_t *ctx, mln_string_t *name);
static void mln_lang_ctx_mq_topic_free(mln_lang_ctx_mq_topic_t *lcmt);
static int mln_lang_ctx_mq_topic_cmp(const mln_lang_ctx_mq_topic_t *lcmt1, const mln_lang_ctx_mq_topic_t *lcmt2);

mln_lang_mq_wait_t mq_wait_min = {
    NULL, NULL, NULL, 0, 0, 0, NULL, NULL
};

int mln_lang_msgqueue(mln_lang_ctx_t *ctx)
{
    if (mln_lang_msgqueue_resource_register(ctx) < 0) {
        return -1;
    }
    if (mln_lang_msgqueue_mq_send(ctx) < 0) {
        mln_lang_msgqueue_resource_cancel(ctx);
        return -1;
    }
    if (mln_lang_msgqueue_mq_recv(ctx) < 0) {
        mln_lang_msgqueue_resource_cancel(ctx);
        return -1;
    }
    if (mln_lang_msgqueue_topic_subscribe(ctx) < 0) {
        mln_lang_msgqueue_resource_cancel(ctx);
        return -1;
    }
    if (mln_lang_msgqueue_topic_unsubscribe(ctx) < 0) {
        mln_lang_msgqueue_resource_cancel(ctx);
        return -1;
    }
    return 0;
}

static int mln_lang_msgqueue_resource_register(mln_lang_ctx_t *ctx)
{
    mln_rbtree_t *mq_set;
    mln_fheap_t *mq_timeout_set;
    if ((mq_set = mln_lang_resource_fetch(ctx->lang, "mq")) == NULL) {
        struct mln_rbtree_attr rbattr;
        rbattr.pool = ctx->lang->pool;
        rbattr.pool_alloc = (rbtree_pool_alloc_handler)mln_alloc_m;
        rbattr.pool_free = (rbtree_pool_free_handler)mln_alloc_free;
        rbattr.cmp = (rbtree_cmp)mln_lang_mq_cmp;
        rbattr.data_free = (rbtree_free_data)mln_lang_mq_free;
        rbattr.cache = 0;
        if ((mq_set = mln_rbtree_init(&rbattr)) == NULL) {
            mln_lang_errmsg(ctx, "No memory.");
            return -1;
        }
        if (mln_lang_resource_register(ctx->lang, "mq", mq_set, (mln_lang_resource_free)mln_rbtree_destroy) < 0) {
            mln_lang_errmsg(ctx, "No memory.");
            mln_rbtree_destroy(mq_set);
            return -1;
        }
    }
    if ((mq_timeout_set = mln_lang_resource_fetch(ctx->lang, "mq_timeout")) == NULL) {
        struct mln_fheap_attr fattr;
        fattr.pool = NULL;
        fattr.pool_alloc = NULL;
        fattr.pool_free = NULL;
        fattr.cmp = (fheap_cmp)mln_lang_mq_wait_cmp;
        fattr.copy = (fheap_copy)mln_lang_mq_wait_copy;
        fattr.key_free = NULL;
        fattr.min_val = &mq_wait_min;
        fattr.min_val_size = sizeof(mq_wait_min);
        if ((mq_timeout_set = mln_fheap_init(&fattr)) == NULL) {
            mln_lang_errmsg(ctx, "No memory.");
            return -1;
        }
        if (mln_lang_resource_register(ctx->lang, "mq_timeout", mq_timeout_set, (mln_lang_resource_free)mln_fheap_destroy) < 0) {
            mln_lang_errmsg(ctx, "No memory.");
            mln_fheap_destroy(mq_timeout_set);
            return -1;
        }
    }

    mln_lang_ctx_mq_t *lcm;
    if ((lcm = mln_lang_ctx_mq_new(ctx)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if (mln_lang_ctx_resource_register(ctx, "mq", lcm, (mln_lang_resource_free)mln_lang_ctx_mq_free) < 0) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_ctx_mq_free(lcm);
        return -1;
    }
    return 0;
}

static void mln_lang_msgqueue_resource_cancel(mln_lang_ctx_t *ctx)
{
    mln_rbtree_t *mq_set;
    mln_fheap_t *mq_timeout_set;
    if ((mq_set = mln_lang_resource_fetch(ctx->lang, "mq")) == NULL) {
        return;
    }
    if (!mq_set->nr_node) {
        mln_lang_resource_cancel(ctx->lang, "mq");
    }
    if ((mq_timeout_set = mln_lang_resource_fetch(ctx->lang, "mq_timeout")) == NULL) {
        return;
    }
    if (!mq_timeout_set->num) {
        mln_lang_resource_cancel(ctx->lang, "mq_timeout");
    }
}

static int mln_lang_msgqueue_mq_send(mln_lang_ctx_t *ctx)
{
    mln_lang_val_t *val;
    mln_lang_var_t *var;
    mln_lang_func_detail_t *func;
    mln_string_t v1 = mln_string("qname"), v2 = mln_string("msg"), v3 = mln_string("asTopic");
    mln_string_t funcname = mln_string("mln_msg_queue_send");

    if ((func = mln_lang_func_detail_new(ctx, M_FUNC_INTERNAL, mln_lang_msgqueue_mq_send_process, NULL, NULL)) == NULL) {
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

static mln_lang_var_t *mln_lang_msgqueue_mq_send_process(mln_lang_ctx_t *ctx)
{
    mln_string_t v1 = mln_string("qname");
    mln_string_t v2 = mln_string("msg");
    mln_string_t v3 = mln_string("asTopic");
    mln_string_t *qname;
    mln_lang_symbol_node_t *sym;
    mln_lang_val_t *val;
    mln_s32_t type, type2;
    mln_u8ptr_t data;
    mln_u8_t broadcast;
    mln_lang_var_t *ret_var;
    /*arg1*/
    if ((sym = mln_lang_symbol_node_search(ctx, &v1, 1)) == NULL) {
        ASSERT(0);
        mln_lang_errmsg(ctx, "Argument 1 missing.");
        return NULL;
    }
    if (sym->type != M_LANG_SYMBOL_VAR || mln_lang_var_val_type_get(sym->data.var) != M_LANG_VAL_TYPE_STRING) {
        mln_lang_errmsg(ctx, "Invalid type of argument 1.");
        return NULL;
    }
    qname = mln_lang_var_val_get(sym->data.var)->data.s;
    /*arg2*/
    if ((sym = mln_lang_symbol_node_search(ctx, &v2, 1)) == NULL) {
        ASSERT(0);
        mln_lang_errmsg(ctx, "Argument 2 missing.");
        return NULL;
    }
    if (sym->type != M_LANG_SYMBOL_VAR) {
        mln_lang_errmsg(ctx, "Invalid type of argument 2.");
        return NULL;
    }
    val = mln_lang_var_val_get(sym->data.var);
    type = mln_lang_var_val_type_get(sym->data.var);
    switch (type) {
        case M_LANG_VAL_TYPE_INT:
            data = (mln_u8ptr_t)&(val->data.i);
            break;
        case M_LANG_VAL_TYPE_BOOL:
            data = (mln_u8ptr_t)&(val->data.b);
            break;
        case M_LANG_VAL_TYPE_REAL:
            data = (mln_u8ptr_t)&(val->data.f);
            break;
        case M_LANG_VAL_TYPE_STRING:
            data = (mln_u8ptr_t)(val->data.s);
            break;
        default:
            mln_lang_errmsg(ctx, "Invalid type of argument 2");
            ASSERT(0);
            return NULL;
    }
    /*arg3*/
    if ((sym = mln_lang_symbol_node_search(ctx, &v3, 1)) == NULL) {
        ASSERT(0);
        mln_lang_errmsg(ctx, "Argument 3 missing.");
        return NULL;
    }
    if (sym->type != M_LANG_SYMBOL_VAR) {
        mln_lang_errmsg(ctx, "Invalid type of argument 3.");
        return NULL;
    }
    val = mln_lang_var_val_get(sym->data.var);
    type2 = mln_lang_var_val_type_get(sym->data.var);
    if (type2 == M_LANG_VAL_TYPE_NIL) {
        broadcast = 0;
    } else if (type2 == M_LANG_VAL_TYPE_BOOL) {
        broadcast = val->data.b;
    } else {
        mln_lang_errmsg(ctx, "Invalid type of argument 3.");
        return NULL;
    }

    if (type == M_LANG_VAL_TYPE_STRING) {
        mln_string_t *s;
        if ((s = mln_string_pool_dup(ctx->lang->pool, (mln_string_t *)data)) == NULL) {
            mln_lang_errmsg(ctx, "No memory.");
            return NULL;
        }
        data = (mln_u8ptr_t)s;
    }
    if (broadcast) {
        ret_var = mln_lang_mq_msg_broadcast(ctx, qname, type, data);
    } else {
        ret_var = mln_lang_mq_msg_set(ctx, qname, type, data);
    }
    if (type == M_LANG_VAL_TYPE_STRING) mln_string_free((mln_string_t *)data);
    return ret_var;
}

static int mln_lang_msgqueue_mq_recv(mln_lang_ctx_t *ctx)
{
    mln_lang_val_t *val;
    mln_lang_var_t *var;
    mln_lang_func_detail_t *func;
    mln_string_t v1 = mln_string("qname"), v2 = mln_string("timeout");
    mln_string_t funcname = mln_string("mln_msg_queue_recv");

    if ((func = mln_lang_func_detail_new(ctx, M_FUNC_INTERNAL, mln_lang_msgqueue_mq_recv_process, NULL, NULL)) == NULL) {
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

static mln_lang_var_t *mln_lang_msgqueue_mq_recv_process(mln_lang_ctx_t *ctx)
{
    mln_string_t v1 = mln_string("qname");
    mln_string_t v2 = mln_string("timeout");
    mln_string_t *qname;
    mln_lang_symbol_node_t *sym;
    mln_lang_val_t *val;
    mln_s32_t type;
    mln_s64_t timeout;
    mln_lang_var_t *ret_var;
    int rc;

    /*arg1*/
    if ((sym = mln_lang_symbol_node_search(ctx, &v1, 1)) == NULL) {
        ASSERT(0);
        mln_lang_errmsg(ctx, "Argument 1 missing.");
        return NULL;
    }
    if (sym->type != M_LANG_SYMBOL_VAR || mln_lang_var_val_type_get(sym->data.var) != M_LANG_VAL_TYPE_STRING) {
        mln_lang_errmsg(ctx, "Invalid type of argument 1.");
        return NULL;
    }
    qname = mln_lang_var_val_get(sym->data.var)->data.s;
    /*arg2*/
    if ((sym = mln_lang_symbol_node_search(ctx, &v2, 1)) == NULL) {
        ASSERT(0);
        mln_lang_errmsg(ctx, "Argument 2 missing.");
        return NULL;
    }
    if (sym->type != M_LANG_SYMBOL_VAR) {
        mln_lang_errmsg(ctx, "Invalid type of argument 2.");
        return NULL;
    }
    val = mln_lang_var_val_get(sym->data.var);
    type = mln_lang_var_val_type_get(sym->data.var);
    if (type == M_LANG_VAL_TYPE_NIL) {
        timeout = -1;
    } else if (type == M_LANG_VAL_TYPE_INT && val->data.i > 0) {
        timeout = val->data.i;
    } else {
        mln_lang_errmsg(ctx, "Invalid type of argument 2.");
        return NULL;
    }

    rc = mln_lang_mq_msg_subscribe_get(ctx, qname, &ret_var);
    if (!rc) return ret_var;
    else if (rc < 0) return NULL;
    return mln_lang_mq_msg_get(ctx, qname, timeout);
}

static int mln_lang_mq_msg_subscribe_get(mln_lang_ctx_t *ctx, mln_string_t *qname, mln_lang_var_t **ret_var)
{
    mln_lang_ctx_mq_topic_t tmp, *topic;
    mln_rbtree_node_t *rn;
    mln_lang_mq_msg_t *msg;
    mln_lang_ctx_mq_t *lcm;

    lcm  = mln_lang_ctx_resource_fetch(ctx, "mq");
    ASSERT(lcm != NULL);
    tmp.topic_name = qname;
    rn = mln_rbtree_search(lcm->topics, lcm->topics->root, &tmp);
    if (mln_rbtree_null(rn, lcm->topics)) return 1;

    topic = (mln_lang_ctx_mq_topic_t *)(rn->data);
    if ((msg = topic->msg_head) == NULL) return 1;

    switch (msg->type) {
        case M_LANG_VAL_TYPE_INT:
            *ret_var = mln_lang_var_create_int(ctx, msg->data.i, NULL);
            break;
        case M_LANG_VAL_TYPE_BOOL:
            *ret_var = mln_lang_var_create_bool(ctx, msg->data.b, NULL);
            break;
        case M_LANG_VAL_TYPE_REAL:
            *ret_var = mln_lang_var_create_real(ctx, msg->data.f, NULL);
            break;
        case M_LANG_VAL_TYPE_STRING:
            *ret_var = mln_lang_var_create_ref_string(ctx, msg->data.s, NULL);
            break;
        default:
            mln_lang_errmsg(ctx, "Invalid type.");
            ASSERT(0);
            abort();
    }
    if (*ret_var == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    mln_lang_mq_msg_chain_del(&(topic->msg_head), &(topic->msg_tail), msg);
    mln_lang_mq_msg_free(msg);

    return 0;
}

static mln_lang_var_t *mln_lang_mq_msg_get(mln_lang_ctx_t *ctx, mln_string_t *qname, mln_s64_t timeout)
{
    mln_rbtree_node_t *rn = NULL;
    mln_lang_t *lang = ctx->lang;
    mln_rbtree_t *mq_set = mln_lang_resource_fetch(lang, "mq");
    ASSERT(mq_set != NULL);
    mln_lang_mq_t *mq;
    mln_lang_mq_msg_t *msg;
    mln_lang_mq_wait_t *wait;
    mln_lang_var_t *ret_var;
    if ((mq = mln_lang_mq_fetch(lang, qname)) == NULL) {
        if ((mq = mln_lang_mq_new(qname)) == NULL) {
            mln_lang_errmsg(ctx, "No memory.");
            return NULL;
        }
        if ((rn = mln_rbtree_node_new(mq_set, mq)) == NULL) {
            mln_lang_mq_free(mq);
            mln_lang_errmsg(ctx, "No memory.");
            return NULL;
        }
        mln_rbtree_insert(mq_set, rn);
    }
    if ((ret_var = mln_lang_var_create_nil(ctx, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return NULL;
    }
    if ((wait = mln_lang_mq_wait_new(ctx, mq)) == NULL) {
        mln_lang_var_free(ret_var);
        mln_lang_errmsg(ctx, "No memory.");
        return NULL;
    }
    mln_lang_mq_wait_chain_add(&(mq->wait_head), &(mq->wait_tail), wait);
    if (mq->wait_head == wait && (msg = mq->msg_head) != NULL) {
        mln_lang_var_free(ret_var);
        switch (msg->type) {
            case M_LANG_VAL_TYPE_INT:
                ret_var = mln_lang_var_create_int(ctx, msg->data.i, NULL);
                break;
            case M_LANG_VAL_TYPE_BOOL:
                ret_var = mln_lang_var_create_bool(ctx, msg->data.b, NULL);
                break;
            case M_LANG_VAL_TYPE_REAL:
                ret_var = mln_lang_var_create_real(ctx, msg->data.f, NULL);
                break;
            case M_LANG_VAL_TYPE_STRING:
                ret_var = mln_lang_var_create_ref_string(ctx, msg->data.s, NULL);
                break;
            default:
                mln_lang_errmsg(ctx, "Invalid type.");
                ASSERT(0);
                abort();
        }
        mln_lang_mq_wait_chain_del(&(mq->wait_head), &(mq->wait_tail), wait);
        mln_lang_mq_wait_free(wait);
        if (ret_var == NULL) {
            mln_lang_errmsg(ctx, "No memory.");
            return NULL;
        }
        mln_lang_mq_msg_chain_del(&(mq->msg_head), &(mq->msg_tail), msg);
        mln_lang_mq_msg_free(msg);
        if (mq->msg_head == NULL && mq->wait_head == NULL) {
            if (rn == NULL) {
                rn = mln_rbtree_search(mq_set, mq_set->root, mq);
                if (mln_rbtree_null(rn, mq_set)) {
                    goto out;
                }
            }
            mln_rbtree_delete(mq_set, rn);
            mln_rbtree_node_free(mq_set, rn);
        }
    } else {
        if (timeout > 0) {
            struct timeval now;
            mln_fheap_t *mq_timeout_set = mln_lang_resource_fetch(ctx->lang, "mq_timeout");
            ASSERT(mq_timeout_set != NULL);
            gettimeofday(&now, NULL);
            wait->timestamp = now.tv_usec + now.tv_sec * 1000000 + timeout;
            mln_fheap_insert(mq_timeout_set, wait->fnode);
            wait->in_heap = 1;
            if (mq_timeout_set->num == 1) {
                if (mln_event_set_timer(ctx->lang->ev, 10, ctx->lang, mln_lang_msgqueue_timeout_handler) < 0) {
                    mln_lang_mq_wait_chain_del(&(mq->wait_head), &(mq->wait_tail), wait);
                    mln_lang_mq_wait_free(wait);
                    mln_lang_var_free(ret_var);
                    mln_lang_errmsg(ctx, "No memory.");
                    return NULL;
                }
                ++(ctx->lang->wait);
            }
        }
        mln_lang_ctx_mq_join(ctx, wait);
        mln_lang_ctx_suspend(ctx);
    }

out:
    return ret_var;
}

static void mln_lang_msgqueue_timeout_handler(mln_event_t *ev, void *data)
{
    mln_lang_t *lang = (mln_lang_t *)data;
    mln_fheap_t *mq_timeout_set = mln_lang_resource_fetch(lang, "mq_timeout");
    ASSERT(mq_timeout_set != NULL);
    mln_fheap_node_t *fn;
    struct timeval tv;
    mln_u64_t now;
    mln_lang_mq_wait_t *wait;
    mln_rbtree_t *mq_set = mln_lang_resource_fetch(lang, "mq");
    ASSERT(mq_set != NULL);
    mln_lang_mq_t *mq;

    --(lang->wait);
    if (lang->quit) {
        mln_lang_free(lang);
        return;
    }
    gettimeofday(&tv, NULL);
    now = tv.tv_usec + tv.tv_sec * 1000000;
    while (1) {
        fn = mln_fheap_minimum(mq_timeout_set);
        if (fn == NULL) break;
        if (((mln_lang_mq_wait_t *)(fn->key))->timestamp > now) {
            mln_event_set_timer(ev, 10, data, mln_lang_msgqueue_timeout_handler);
            ++(lang->wait);
            break;
        }
        fn = mln_fheap_extract_min(mq_timeout_set);
        wait = (mln_lang_mq_wait_t *)(fn->key);
        wait->in_heap = 0;
        mq = wait->mq;
        mln_lang_mq_wait_chain_del(&(mq->wait_head), &(mq->wait_tail), wait);
        mln_lang_ctx_mq_remove(wait->ctx);
        mln_lang_ctx_continue(wait->ctx);
        mln_lang_mq_wait_free(wait);

        if (mq->msg_head == NULL && mq->wait_head == NULL) {
            mln_rbtree_node_t *rn = mln_rbtree_search(mq_set, mq_set->root, mq);
            if (!mln_rbtree_null(rn, mq_set)) {
                mln_rbtree_delete(mq_set, rn);
                mln_rbtree_node_free(mq_set, rn);
            }
        }
    }
}

static mln_lang_var_t *mln_lang_mq_msg_broadcast(mln_lang_ctx_t *ctx, mln_string_t *qname, int type, void *data)
{
    mln_lang_ctx_t *c, *scan;
    mln_lang_var_t *ret_var;

    scan = ctx->lang->run_head;
    while (scan != NULL) {
        c = scan;
        scan = scan->next;
        if (mln_lang_mq_msg_broadcast_ctx(c, qname, type, data) < 0) {
            return NULL;
        }
    }
    scan = ctx->lang->wait_head;
    while (scan != NULL) {
        c = scan;
        scan = scan->next;
        if (mln_lang_mq_msg_broadcast_ctx(c, qname, type, data) < 0) {
            return NULL;
        }
    }
    if ((ret_var = mln_lang_var_create_nil(ctx, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return NULL;
    }
    return ret_var;
}

static int mln_lang_mq_msg_broadcast_ctx(mln_lang_ctx_t *ctx, mln_string_t *qname, int type, void *data)
{
    mln_lang_ctx_mq_topic_t tmp, *topic;
    mln_rbtree_node_t *rn;
    mln_lang_mq_msg_t *msg;
    mln_lang_ctx_mq_t *lcm;

    lcm  = mln_lang_ctx_resource_fetch(ctx, "mq");
    ASSERT(lcm != NULL);
    tmp.topic_name = qname;
    rn = mln_rbtree_search(lcm->topics, lcm->topics->root, &tmp);
    if (mln_rbtree_null(rn, lcm->topics)) return 0;

    topic = (mln_lang_ctx_mq_topic_t *)(rn->data);
    if ((msg = mln_lang_mq_msg_new(ctx->lang, type, data)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    mln_lang_mq_msg_chain_add(&(topic->msg_head), &(topic->msg_tail), msg);

    if (lcm->mq_wait != NULL) {
        mln_lang_var_t *ret_var;
        mln_lang_mq_wait_t *wait = lcm->mq_wait;
        mln_lang_mq_t *mq = wait->mq;
        switch (msg->type) {
            case M_LANG_VAL_TYPE_INT:
                ret_var = mln_lang_var_create_int(ctx, msg->data.i, NULL);
                break;
            case M_LANG_VAL_TYPE_BOOL:
                ret_var = mln_lang_var_create_bool(ctx, msg->data.b, NULL);
                break;
            case M_LANG_VAL_TYPE_REAL:
                ret_var = mln_lang_var_create_real(ctx, msg->data.f, NULL);
                break;
            case M_LANG_VAL_TYPE_STRING:
                ret_var = mln_lang_var_create_ref_string(ctx, msg->data.s, NULL);
                break;
            default:
                mln_lang_errmsg(ctx, "Invalid type.");
                ASSERT(0);
                abort();
        }
        if (ret_var == NULL) {
            mln_lang_errmsg(ctx, "No memory.");
            return -1;
        }
        mln_lang_ctx_set_ret_var(ctx, ret_var);
        mln_lang_mq_msg_chain_del(&(topic->msg_head), &(topic->msg_tail), msg);
        mln_lang_mq_msg_free(msg);
        mln_lang_ctx_mq_remove(ctx);
        mln_lang_ctx_continue(ctx);
        mln_lang_mq_wait_chain_del(&(mq->wait_head), &(mq->wait_tail), wait);
        mln_lang_mq_wait_free(wait);
    }
    return 0;
}

static mln_lang_var_t *mln_lang_mq_msg_set(mln_lang_ctx_t *ctx, mln_string_t *qname, int type, void *data)
{
    mln_lang_t *lang = ctx->lang;
    mln_rbtree_node_t *rn;
    mln_rbtree_t *mq_set = mln_lang_resource_fetch(lang, "mq");
    ASSERT(mq_set != NULL);
    mln_lang_mq_t *mq;
    mln_lang_mq_msg_t *msg;
    mln_lang_var_t *ret_var;
    mln_lang_mq_wait_t *wait;

    if ((msg = mln_lang_mq_msg_new(lang, type, data)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return NULL;
    }
    if ((mq = mln_lang_mq_fetch(lang, qname)) == NULL) {
        if ((mq = mln_lang_mq_new(qname)) == NULL) {
            mln_lang_mq_msg_free(msg);
            mln_lang_errmsg(ctx, "No memory.");
            return NULL;
        }
        if ((rn = mln_rbtree_node_new(mq_set, mq)) == NULL) {
            mln_lang_mq_free(mq);
            mln_lang_mq_msg_free(msg);
            mln_lang_errmsg(ctx, "No memory.");
            return NULL;
        }
        mln_rbtree_insert(mq_set, rn);
    }
    mln_lang_mq_msg_chain_add(&(mq->msg_head), &(mq->msg_tail), msg);

    if ((ret_var = mln_lang_var_create_nil(ctx, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return NULL;
    }
    msg = mq->msg_head;
    if ((wait = mq->wait_head) != NULL) {
        mln_lang_var_t *ret_var2;
        switch (msg->type) {
            case M_LANG_VAL_TYPE_INT:
                ret_var2 = mln_lang_var_create_int(wait->ctx, msg->data.i, NULL);
                break;
            case M_LANG_VAL_TYPE_BOOL:
                ret_var2 = mln_lang_var_create_bool(wait->ctx, msg->data.b, NULL);
                break;
            case M_LANG_VAL_TYPE_REAL:
                ret_var2 = mln_lang_var_create_real(wait->ctx, msg->data.f, NULL);
                break;
            case M_LANG_VAL_TYPE_STRING:
                ret_var2 = mln_lang_var_create_ref_string(wait->ctx, msg->data.s, NULL);
                break;
            default:
                mln_lang_errmsg(ctx, "Invalid type.");
                ASSERT(0);
                abort();
        }
        if (ret_var2 == NULL) {
            mln_lang_errmsg(wait->ctx, "No memory.");
        } else {
            mln_lang_ctx_set_ret_var(wait->ctx, ret_var2);
            mln_lang_mq_msg_chain_del(&(mq->msg_head), &(mq->msg_tail), msg);
            mln_lang_mq_msg_free(msg);
            mln_lang_ctx_mq_remove(wait->ctx);
            mln_lang_ctx_continue(wait->ctx);
            mln_lang_mq_wait_chain_del(&(mq->wait_head), &(mq->wait_tail), wait);
            mln_lang_mq_wait_free(wait);
        }
    }
    return ret_var;
}

static int mln_lang_msgqueue_topic_subscribe(mln_lang_ctx_t *ctx)
{
    mln_lang_val_t *val;
    mln_lang_var_t *var;
    mln_lang_func_detail_t *func;
    mln_string_t v1 = mln_string("qname");
    mln_string_t funcname = mln_string("mln_msg_topic_subscribe");

    if ((func = mln_lang_func_detail_new(ctx, M_FUNC_INTERNAL, mln_lang_msgqueue_topic_subscribe_process, NULL, NULL)) == NULL) {
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

static mln_lang_var_t *mln_lang_msgqueue_topic_subscribe_process(mln_lang_ctx_t *ctx)
{
    mln_lang_ctx_mq_t *lcm;
    mln_lang_var_t *ret_var = NULL;
    mln_string_t v1 = mln_string("qname");
    mln_string_t *qname;
    mln_lang_symbol_node_t *sym;
    mln_rbtree_node_t *rn;
    mln_lang_ctx_mq_topic_t tmp, *topic;

    if ((sym = mln_lang_symbol_node_search(ctx, &v1, 1)) == NULL) {
        ASSERT(0);
        mln_lang_errmsg(ctx, "Argument 1 missing.");
        return NULL;
    }
    if (sym->type != M_LANG_SYMBOL_VAR || mln_lang_var_val_type_get(sym->data.var) != M_LANG_VAL_TYPE_STRING) {
        mln_lang_errmsg(ctx, "Invalid type of argument 1.");
        return NULL;
    }
    qname = mln_lang_var_val_get(sym->data.var)->data.s;

    if ((ret_var = mln_lang_var_create_nil(ctx, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return NULL;
    }
    lcm  = mln_lang_ctx_resource_fetch(ctx, "mq");
    ASSERT(lcm != NULL);
    tmp.topic_name = qname;
    rn = mln_rbtree_search(lcm->topics, lcm->topics->root, &tmp);
    if (mln_rbtree_null(rn, lcm->topics)) {
        if ((topic = mln_lang_ctx_mq_topic_new(ctx, qname)) == NULL) {
            mln_lang_var_free(ret_var);
            mln_lang_errmsg(ctx, "No memory.");
            return NULL;
        }
        if ((rn = mln_rbtree_node_new(lcm->topics, topic)) == NULL) {
            mln_lang_ctx_mq_topic_free(topic);
            mln_lang_var_free(ret_var);
            mln_lang_errmsg(ctx, "No memory.");
            return NULL;
        }
        mln_rbtree_insert(lcm->topics, rn);
    }

    return ret_var;
}

static int mln_lang_msgqueue_topic_unsubscribe(mln_lang_ctx_t *ctx)
{
    mln_lang_val_t *val;
    mln_lang_var_t *var;
    mln_lang_func_detail_t *func;
    mln_string_t v1 = mln_string("qname");
    mln_string_t funcname = mln_string("mln_msg_topic_unsubscribe");

    if ((func = mln_lang_func_detail_new(ctx, M_FUNC_INTERNAL, mln_lang_msgqueue_topic_unsubscribe_process, NULL, NULL)) == NULL) {
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

static mln_lang_var_t *mln_lang_msgqueue_topic_unsubscribe_process(mln_lang_ctx_t *ctx)
{
    mln_lang_ctx_mq_t *lcm;
    mln_lang_var_t *ret_var = NULL;
    mln_string_t v1 = mln_string("qname");
    mln_string_t *qname;
    mln_lang_symbol_node_t *sym;
    mln_rbtree_node_t *rn;
    mln_lang_ctx_mq_topic_t tmp;

    if ((sym = mln_lang_symbol_node_search(ctx, &v1, 1)) == NULL) {
        ASSERT(0);
        mln_lang_errmsg(ctx, "Argument 1 missing.");
        return NULL;
    }
    if (sym->type != M_LANG_SYMBOL_VAR || mln_lang_var_val_type_get(sym->data.var) != M_LANG_VAL_TYPE_STRING) {
        mln_lang_errmsg(ctx, "Invalid type of argument 1.");
        return NULL;
    }
    qname = mln_lang_var_val_get(sym->data.var)->data.s;

    if ((ret_var = mln_lang_var_create_nil(ctx, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return NULL;
    }
    lcm  = mln_lang_ctx_resource_fetch(ctx, "mq");
    ASSERT(lcm != NULL);
    tmp.topic_name = qname;
    rn = mln_rbtree_search(lcm->topics, lcm->topics->root, &tmp);
    if (!mln_rbtree_null(rn, lcm->topics)) {
        mln_rbtree_delete(lcm->topics, rn);
        mln_rbtree_node_free(lcm->topics, rn);
    }

    return ret_var;
}


/*
 * components
 */
static mln_lang_mq_msg_t *mln_lang_mq_msg_new(mln_lang_t *lang, int type, void *data)
{
    mln_lang_mq_msg_t *msg;
    if ((msg = (mln_lang_mq_msg_t *)malloc(sizeof(mln_lang_mq_msg_t))) == NULL) {
        return NULL;
    }
    msg->prev = msg->next = NULL;
    msg->lang = lang;
    switch (type) {
        case M_LANG_VAL_TYPE_INT:
            msg->data.i = *((mln_s64_t *)data);
            break;
        case M_LANG_VAL_TYPE_BOOL:
            msg->data.b = *((mln_u8ptr_t)data);
            break;
        case M_LANG_VAL_TYPE_REAL:
            msg->data.f = *(double *)data;
            break;
        case M_LANG_VAL_TYPE_STRING:
            msg->data.s = mln_string_ref((mln_string_t *)data);
            break;
        default:
            ASSERT(0);
            free(msg);
            return NULL;
    }
    msg->type = type;
    return msg;
}

static void mln_lang_mq_msg_free(mln_lang_mq_msg_t *msg)
{
    if (msg == NULL) return;
    if (msg->type == M_LANG_VAL_TYPE_STRING && msg->data.s != NULL) {
        /*
         * Note:
         * must be identical with msg data allocation type
         */
        mln_string_free(msg->data.s);
    }
    free(msg);
}

static mln_lang_mq_wait_t *mln_lang_mq_wait_new(mln_lang_ctx_t *ctx, mln_lang_mq_t *mq)
{
    mln_lang_mq_wait_t *lmw;
    mln_fheap_t *mq_timeout_set = mln_lang_resource_fetch(ctx->lang, "mq_timeout");
    ASSERT(mq_timeout_set != NULL);
    if ((lmw = (mln_lang_mq_wait_t *)malloc(sizeof(mln_lang_mq_wait_t))) == NULL) {
        return NULL;
    }
    lmw->ctx = ctx;
    lmw->mq = mq;
    if ((lmw->fnode = mln_fheap_node_init(mq_timeout_set, lmw)) == NULL) {
        free(lmw);
        return NULL;
    }
    lmw->in_heap = 0;
    lmw->padding = 0;
    lmw->timestamp = 0;
    lmw->prev = lmw->next = NULL;
    return lmw;
}

static void mln_lang_mq_wait_free(mln_lang_mq_wait_t *lmw)
{
    if (lmw == NULL) return;
    if (lmw->fnode != NULL) {
        mln_fheap_t *mq_timeout_set = mln_lang_resource_fetch(lmw->ctx->lang, "mq_timeout");
        if (mq_timeout_set != NULL) {
            if (lmw->in_heap)
                mln_fheap_delete(mq_timeout_set, lmw->fnode);
            mln_fheap_node_destroy(mq_timeout_set, lmw->fnode);
        }
    }
    free(lmw);
}

static int mln_lang_mq_wait_cmp(const mln_lang_mq_wait_t *lmw1, const mln_lang_mq_wait_t *lmw2)
{
    if (lmw1->timestamp < lmw2->timestamp) return 0;
    return 1;
}

static void mln_lang_mq_wait_copy(mln_lang_mq_wait_t *lmw1, mln_lang_mq_wait_t *lmw2)
{
    lmw1->timestamp = lmw2->timestamp;
}

static mln_lang_mq_t *mln_lang_mq_new(mln_string_t *name)
{
    mln_lang_mq_t *lm;
    if ((lm = (mln_lang_mq_t *)malloc(sizeof(mln_lang_mq_t))) == NULL) {
        return NULL;
    }
    if ((lm->name = mln_string_dup(name)) == NULL) {
        free(lm);
        return NULL;
    }
    lm->msg_head = lm->msg_tail = NULL;
    lm->wait_head = lm->wait_tail = NULL;
    return lm;
}

static int mln_lang_mq_cmp(const mln_lang_mq_t *lm1, const mln_lang_mq_t *lm2)
{
    return mln_string_strcmp(lm1->name, lm2->name);
}

static void mln_lang_mq_free(mln_lang_mq_t *lm)
{
    if (lm == NULL) return;
    mln_lang_mq_msg_t *msg;
    mln_lang_mq_wait_t *lmw;
    if (lm->name != NULL) mln_string_free(lm->name);
    while ((msg = lm->msg_head) != NULL) {
        mln_lang_mq_msg_chain_del(&(lm->msg_head), &(lm->msg_tail), msg);
        mln_lang_mq_msg_free(msg);
    }
    while ((lmw = lm->wait_head) != NULL) {
        mln_lang_mq_wait_chain_del(&(lm->wait_head), &(lm->wait_tail), lmw);
        mln_lang_mq_wait_free(lmw);
    }
    free(lm);
}

static mln_lang_ctx_mq_t *mln_lang_ctx_mq_new(mln_lang_ctx_t *ctx)
{
    mln_lang_ctx_mq_t *lcm;
    struct mln_rbtree_attr rbattr;
    if ((lcm = (mln_lang_ctx_mq_t *)mln_alloc_m(ctx->pool, sizeof(mln_lang_ctx_mq_t))) == NULL) {
        return NULL;
    }
    lcm->mq_wait = NULL;
    rbattr.pool = ctx->pool;
    rbattr.pool_alloc = (rbtree_pool_alloc_handler)mln_alloc_m;
    rbattr.pool_free = (rbtree_pool_free_handler)mln_alloc_free;
    rbattr.cmp = (rbtree_cmp)mln_lang_ctx_mq_topic_cmp;
    rbattr.data_free = (rbtree_free_data)mln_lang_ctx_mq_topic_free;
    rbattr.cache = 0;
    if ((lcm->topics = mln_rbtree_init(&rbattr)) == NULL) {
        mln_alloc_free(lcm);
        return NULL;
    }
    return lcm;
}

static void mln_lang_ctx_mq_free(mln_lang_ctx_mq_t *lcm)
{
    if (lcm == NULL) return;
    if (lcm->topics != NULL) mln_rbtree_destroy(lcm->topics);
    if (lcm->mq_wait != NULL) {
        lcm->mq_wait->ctx = NULL;
        mln_lang_mq_wait_chain_del(&(lcm->mq_wait->mq->wait_head), &(lcm->mq_wait->mq->wait_tail), lcm->mq_wait);
        mln_lang_mq_wait_free(lcm->mq_wait);
    }
    mln_alloc_free(lcm);
}

static void mln_lang_ctx_mq_join(mln_lang_ctx_t *ctx, mln_lang_mq_wait_t *wait)
{
    mln_lang_ctx_mq_t *lcm = mln_lang_ctx_resource_fetch(ctx, "mq");
    ASSERT(lcm != NULL);
    lcm->mq_wait = wait;
}

static void mln_lang_ctx_mq_remove(mln_lang_ctx_t *ctx)
{
    mln_lang_ctx_mq_t *lcm = mln_lang_ctx_resource_fetch(ctx, "mq");
    ASSERT(lcm != NULL);
    lcm->mq_wait = NULL;
}

static mln_lang_mq_t *mln_lang_mq_fetch(mln_lang_t *lang, mln_string_t *qname)
{
    mln_lang_mq_t tmp;
    mln_rbtree_node_t *rn;
    mln_rbtree_t *mq_set = mln_lang_resource_fetch(lang, "mq");
    ASSERT(mq_set != NULL);
    tmp.name = qname;
    rn = mln_rbtree_search(mq_set, mq_set->root, &tmp);
    if (mln_rbtree_null(rn, mq_set)) return NULL;
    return (mln_lang_mq_t *)(rn->data);
}

static mln_lang_ctx_mq_topic_t *mln_lang_ctx_mq_topic_new(mln_lang_ctx_t *ctx, mln_string_t *name)
{
    mln_lang_ctx_mq_topic_t *lcmt;
    if ((lcmt = (mln_lang_ctx_mq_topic_t *)mln_alloc_m(ctx->pool, sizeof(mln_lang_ctx_mq_topic_t))) == NULL) {
        return NULL;
    }
    if ((lcmt->topic_name = mln_string_pool_dup(ctx->pool, name)) == NULL) {
        mln_alloc_free(lcmt);
        return NULL;
    }
    lcmt->ctx = ctx;
    lcmt->msg_head = lcmt->msg_tail = NULL;
    return lcmt;
}

static void mln_lang_ctx_mq_topic_free(mln_lang_ctx_mq_topic_t *lcmt)
{
    mln_lang_mq_msg_t *msg;
    if (lcmt == NULL) return;
    if (lcmt->topic_name != NULL) mln_string_free(lcmt->topic_name);
    while ((msg = lcmt->msg_head) != NULL) {
        mln_lang_mq_msg_chain_del(&(lcmt->msg_head), &(lcmt->msg_tail), msg);
        mln_lang_mq_msg_free(msg);
    }
    mln_alloc_free(lcmt);
}

static int mln_lang_ctx_mq_topic_cmp(const mln_lang_ctx_mq_topic_t *lcmt1, const mln_lang_ctx_mq_topic_t *lcmt2)
{
    return mln_string_strcmp(lcmt1->topic_name, lcmt2->topic_name);
}

MLN_CHAIN_FUNC_DEFINE(mln_lang_mq_msg, mln_lang_mq_msg_t, static inline void, prev, next);
MLN_CHAIN_FUNC_DEFINE(mln_lang_mq_wait, mln_lang_mq_wait_t, static inline void, prev, next);

