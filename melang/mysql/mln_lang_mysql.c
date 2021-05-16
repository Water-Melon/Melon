
/*
 * Copyright (C) Niklaus F.Schen.
 */
#include "mysql/mln_lang_mysql.h"

#ifdef MLN_MYSQL

#ifdef __DEBUG__
#include <assert.h>
#define ASSERT(x) assert(x)
#else
#define ASSERT(x);
#endif

MLN_CHAIN_FUNC_DECLARE(mln_lang_mysql, mln_lang_mysql_t, static inline void, __NONNULL3(1,2,3));
MLN_CHAIN_FUNC_DEFINE(mln_lang_mysql, mln_lang_mysql_t, static inline void, prev, next);
MLN_CHAIN_FUNC_DECLARE(mln_lang_ctx, mln_lang_ctx_t, static inline void, __NONNULL3(1,2,3));
MLN_CHAIN_FUNC_DEFINE(mln_lang_ctx, mln_lang_ctx_t, static inline void, prev, next);

static mln_lang_mysql_t *mln_lang_mysql_new(mln_lang_ctx_t *ctx, mln_string_t *db, mln_string_t *username, mln_string_t *password, mln_string_t *host, int port)
{
    mln_lang_mysql_t *lm;
    int fds[2];

    if (pipe(fds) < 0) {
        return NULL;
    }
    if ((lm = (mln_lang_mysql_t *)mln_alloc_m(ctx->pool, sizeof(mln_lang_mysql_t))) == NULL) {
        mln_socket_close(fds[0]);
        mln_socket_close(fds[1]);
        return NULL;
    }
    lm->ctx = ctx;
    lm->ret_var = NULL;
    lm->mysql = NULL;
    lm->result = NULL;
    lm->nfield = 0;
    lm->row = NULL;
    if ((lm->db = mln_string_pool_dup(ctx->pool, db)) == NULL) {
        mln_alloc_free(lm);
        mln_socket_close(fds[0]);
        mln_socket_close(fds[1]);
        return NULL;
    }
    if ((lm->username = mln_string_pool_dup(ctx->pool, username)) == NULL) {
        mln_string_free(lm->db);
        mln_alloc_free(lm);
        mln_socket_close(fds[0]);
        mln_socket_close(fds[1]);
        return NULL;
    }
    if ((lm->password = mln_string_pool_dup(ctx->pool, password)) == NULL) {
        mln_string_free(lm->username);
        mln_string_free(lm->db);
        mln_alloc_free(lm);
        mln_socket_close(fds[0]);
        mln_socket_close(fds[1]);
        return NULL;
    }
    if ((lm->host = mln_string_pool_dup(ctx->pool, host)) == NULL) {
        mln_string_free(lm->password);
        mln_string_free(lm->username);
        mln_string_free(lm->db);
        mln_alloc_free(lm);
        mln_socket_close(fds[0]);
        mln_socket_close(fds[1]);
        return NULL;
    }
    lm->sql = NULL;
    lm->port = port;
    lm->fd_signal = fds[1];
    lm->fd_useless = fds[0];
    lm->prev = lm->next = NULL;
    return lm;
}

static void mln_lang_mysql_free(mln_lang_mysql_t *lm)
{
    if (lm == NULL) return;
    if (lm->ret_var != NULL) mln_lang_var_free(lm->ret_var);
    ASSERT(lm->result == NULL);
    if (lm->mysql != NULL) mysql_close(lm->mysql);
    if (lm->db != NULL) mln_string_free(lm->db);
    if (lm->username != NULL) mln_string_free(lm->username);
    if (lm->password != NULL) mln_string_free(lm->password);
    if (lm->host != NULL) mln_string_free(lm->host);
    if (lm->sql != NULL) mln_string_free(lm->sql);
    mln_socket_close(lm->fd_signal);
    mln_socket_close(lm->fd_useless);
    mln_alloc_free(lm);
}

static mln_lang_mysql_timeout_t *mln_lang_mysql_timeout_new(mln_lang_ctx_t *ctx)
{
    mln_lang_mysql_timeout_t *lmt;
    if ((lmt = (mln_lang_mysql_timeout_t *)mln_alloc_m(ctx->pool, sizeof(mln_lang_mysql_timeout_t))) == NULL) {
        return NULL;
    }
    lmt->ctx = ctx;
    lmt->head = lmt->tail = NULL;
    return lmt;
};

static void mln_lang_mysql_timeout_free(mln_lang_mysql_timeout_t *lmt)
{
    if (lmt == NULL) return;
    mln_lang_mysql_t *lm;
    while ((lm = lmt->head) != NULL) {
        mln_lang_mysql_chain_del(&(lmt->head), &(lmt->tail), lm);
        mln_event_set_fd(lmt->ctx->lang->ev, lm->fd_signal, M_EV_CLR, M_EV_UNLIMITED, NULL, NULL);
        mln_lang_mysql_free(lm);
    }
    mln_alloc_free(lmt);
}

static int mln_lang_mysql_add_fd(mln_lang_ctx_t *ctx, mln_lang_set_detail_t *set)
{
    mln_s64_t fd = 0;
    mln_lang_val_t *val;
    mln_lang_var_t *var;
    mln_string_t varname = mln_string("fd");

    if ((val = mln_lang_val_new(ctx, M_LANG_VAL_TYPE_INT, &fd)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if ((var = mln_lang_var_new(ctx, &varname, M_LANG_VAR_NORMAL, val, set)) == NULL) {
        mln_lang_val_free(val);
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if (mln_lang_set_member_add(ctx->pool, set->members, var) < 0) {
        mln_lang_var_free(var);
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    return 0;
}

static void mln_lang_mysql_connect_test(mln_event_t *ev, int fd, void *data)
{
    enum net_async_status status;
    mln_lang_mysql_timeout_t *lmt;
    mln_lang_mysql_t *mysql = (mln_lang_mysql_t *)data;
    mln_lang_ctx_t *ctx = mysql->ctx;
    mln_lang_var_t *ret_var;

    lmt = (mln_lang_mysql_timeout_t *)mln_lang_ctx_resource_fetch(ctx, "mysql");
    ASSERT(lmt != NULL);

    status = mysql_real_connect_nonblocking(mysql->mysql, \
                                            (char *)(mysql->host->data), \
                                            (char *)(mysql->username->data), \
                                            (char *)(mysql->password->data), \
                                            (char *)(mysql->db->data), \
                                            mysql->port, \
                                            NULL, \
                                            0);
    if (status == NET_ASYNC_ERROR) {
        mln_event_set_fd(ctx->lang->ev, mysql->fd_signal, M_EV_CLR, M_EV_UNLIMITED, NULL, NULL);
        mln_lang_mysql_chain_del(&(lmt->head), &(lmt->tail), mysql);
        mln_lang_mysql_free(mysql);
        mln_lang_ctx_continue(ctx);
    } else if (status == NET_ASYNC_NOT_READY) {
        mln_event_set_fd(ctx->lang->ev, mysql->fd_signal, M_EV_SEND|M_EV_ONESHOT|M_EV_NONBLOCK, M_EV_UNLIMITED, mysql, mln_lang_mysql_connect_test);
    } else {
        mln_event_set_fd(ctx->lang->ev, mysql->fd_signal, M_EV_CLR, M_EV_UNLIMITED, NULL, NULL);
        mln_lang_mysql_chain_del(&(lmt->head), &(lmt->tail), mysql);
        if ((ret_var = mln_lang_var_create_true(ctx, NULL)) != NULL) {
            mln_s32_t type;
            mln_lang_symbol_node_t *sym;
            mln_string_t _this = mln_string("this");
            mln_string_t v = mln_string("fd");
            mln_lang_val_t *val;
            mln_lang_var_t *var;
            if ((sym = mln_lang_symbol_node_search(ctx, &_this, 1)) == NULL) {
                mln_lang_errmsg(ctx, "Lack of 'this'.");
                mln_lang_var_free(ret_var);
                mln_lang_mysql_free(mysql);
                goto out;
            }
            ASSERT(sym->type == M_LANG_SYMBOL_VAR);
            type = mln_lang_var_val_type_get(sym->data.var);
            val = sym->data.var->val;
            if (type != M_LANG_VAL_TYPE_OBJECT) {
                mln_lang_errmsg(ctx, "'this' not object.");
                mln_lang_var_free(ret_var);
                mln_lang_mysql_free(mysql);
                goto out;
            }
            /*fd*/
            if ((var = mln_lang_set_member_search(val->data.obj->members, &v)) == NULL) {
                mln_lang_errmsg(ctx, "Lack of member 'fd'.");
                mln_lang_var_free(ret_var);
                mln_lang_mysql_free(mysql);
                goto out;
            }
            if (mln_lang_var_val_type_get(var) != M_LANG_VAL_TYPE_INT) {
                mln_lang_errmsg(ctx, "Invalid type of 'fd'.");
                mln_lang_var_free(ret_var);
                mln_lang_mysql_free(mysql);
                goto out;
            }
            val = var->val;
            mln_lang_ctx_set_ret_var(mysql->ctx, ret_var);
            memcpy(&(val->data.s), &mysql, sizeof(mln_lang_mysql_t *));
            mln_lang_val_not_modify_set(val);
        } else {
            mln_lang_errmsg(ctx, "No memory.");
            mln_lang_mysql_free(mysql);
        }
out:
        mln_lang_ctx_continue(ctx);
    }
}

static mln_lang_var_t *mln_lang_mysql_connect_process(mln_lang_ctx_t *ctx)
{
    mln_s32_t type;
    mln_lang_val_t *val1, *val2, *val3, *val4, *val5, *val;
    mln_string_t _this = mln_string("this");
    mln_string_t v1 = mln_string("host");
    mln_string_t v2 = mln_string("port");
    mln_string_t v3 = mln_string("db");
    mln_string_t v4 = mln_string("username");
    mln_string_t v5 = mln_string("password");
    mln_string_t v = mln_string("fd");
    mln_string_t typename = mln_string("Mysql");
    mln_lang_symbol_node_t *sym;
    mln_lang_var_t *var;
    mln_lang_var_t *ret_var;
    mln_lang_mysql_t *mysql;
    mln_lang_mysql_timeout_t *lmt;

    if ((sym = mln_lang_symbol_node_search(ctx, &_this, 1)) == NULL) {
        ASSERT(0);
        mln_lang_errmsg(ctx, "Lack of 'this'.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);
    type = mln_lang_var_val_type_get(sym->data.var);
    val = sym->data.var->val;
    if (type != M_LANG_VAL_TYPE_OBJECT) {
        mln_lang_errmsg(ctx, "'this' not object.");
        return NULL;
    }
    if (val->data.obj->in_set == NULL || val->data.obj->in_set->name == NULL || mln_string_strcmp(val->data.obj->in_set->name, &typename)) {
        mln_lang_errmsg(ctx, "Invalid set type, Mysql object required.");
        return NULL;
    }
    /*fd*/
    if ((var = mln_lang_set_member_search(val->data.obj->members, &v)) == NULL) {
        mln_lang_errmsg(ctx, "Lack of member 'fd'.");
        return NULL;
    }
    if (mln_lang_var_val_type_get(var) != M_LANG_VAL_TYPE_INT) {
        mln_lang_errmsg(ctx, "Invalid type of 'fd'.");
        return NULL;
    }
    val = var->val;
    /*arg1*/
    if ((sym = mln_lang_symbol_node_search(ctx, &v1, 1)) == NULL) {
        mln_lang_errmsg(ctx, "Argument missing.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);
    type = mln_lang_var_val_type_get(sym->data.var);
    val1 = sym->data.var->val;
    if (type != M_LANG_VAL_TYPE_STRING || val1->data.s == NULL) {
        mln_lang_errmsg(ctx, "'host' must be string.");
        return NULL;
    }
    /*arg2*/
    if ((sym = mln_lang_symbol_node_search(ctx, &v2, 1)) == NULL) {
        ASSERT(0);
        mln_lang_errmsg(ctx, "Argument missing.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);
    type = mln_lang_var_val_type_get(sym->data.var);
    val2 = sym->data.var->val;
    if (type != M_LANG_VAL_TYPE_INT) {
        mln_lang_errmsg(ctx, "'port' must be integer.");
        return NULL;
    }
    /*arg3*/
    if ((sym = mln_lang_symbol_node_search(ctx, &v3, 1)) == NULL) {
        ASSERT(0);
        mln_lang_errmsg(ctx, "Argument missing.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);
    type = mln_lang_var_val_type_get(sym->data.var);
    val3 = sym->data.var->val;
    if (type != M_LANG_VAL_TYPE_STRING || val3->data.s == NULL) {
        mln_lang_errmsg(ctx, "'db' must be string.");
        return NULL;
    }
    /*arg4*/
    if ((sym = mln_lang_symbol_node_search(ctx, &v4, 1)) == NULL) {
        ASSERT(0);
        mln_lang_errmsg(ctx, "Argument missing.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);
    type = mln_lang_var_val_type_get(sym->data.var);
    val4 = sym->data.var->val;
    if (type != M_LANG_VAL_TYPE_STRING || val4->data.s == NULL) {
        mln_lang_errmsg(ctx, "'username' must be string.");
        return NULL;
    }
    /*arg5*/
    if ((sym = mln_lang_symbol_node_search(ctx, &v5, 1)) == NULL) {
        ASSERT(0);
        mln_lang_errmsg(ctx, "Argument missing.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);
    type = mln_lang_var_val_type_get(sym->data.var);
    val5 = sym->data.var->val;
    if (type != M_LANG_VAL_TYPE_STRING || val5->data.s == NULL) {
        mln_lang_errmsg(ctx, "'password' must be string.");
        return NULL;
    }

    if ((mysql = mln_lang_mysql_new(ctx, val3->data.s, val4->data.s, val5->data.s, val1->data.s, val2->data.i)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return NULL;
    }
    if ((mysql->mysql = mysql_init(NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_mysql_free(mysql);
        return NULL;
    }
    lmt = (mln_lang_mysql_timeout_t *)mln_lang_ctx_resource_fetch(ctx, "mysql");
    ASSERT(lmt != NULL);
    if (mln_event_set_fd(ctx->lang->ev, mysql->fd_signal, M_EV_SEND|M_EV_ONESHOT|M_EV_NONBLOCK, M_EV_UNLIMITED, mysql, mln_lang_mysql_connect_test) < 0) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_mysql_free(mysql);
        return NULL;
    }
    if ((ret_var = mln_lang_var_create_false(ctx, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_event_set_fd(ctx->lang->ev, mysql->fd_signal, M_EV_CLR, M_EV_UNLIMITED, NULL, NULL);
        mln_lang_mysql_free(mysql);
        return NULL;
    }
    mln_lang_mysql_chain_add(&(lmt->head), &(lmt->tail), mysql);
    mln_lang_ctx_suspend(ctx);
    return ret_var;
}

static int mln_lang_mysql_add_connect(mln_lang_ctx_t *ctx, mln_lang_set_detail_t *set)
{
    mln_lang_val_t *val;
    mln_lang_var_t *var;
    mln_lang_func_detail_t *func;
    mln_string_t funcname = mln_string("connect");
    int i;
    mln_string_t arr[] = {
        mln_string("host"),
        mln_string("port"),
        mln_string("db"),
        mln_string("username"),
        mln_string("password")
    };

    if ((func = mln_lang_func_detail_new(ctx, M_FUNC_INTERNAL, mln_lang_mysql_connect_process, NULL, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    for (i = 0; i < sizeof(arr)/sizeof(mln_string_t); ++i) {
        if ((val = mln_lang_val_new(ctx, M_LANG_VAL_TYPE_NIL, NULL)) == NULL) {
            mln_lang_errmsg(ctx, "No memory.");
            mln_lang_func_detail_free(func);
            return -1;
        }
        if ((var = mln_lang_var_new(ctx, &arr[i], M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
            mln_lang_errmsg(ctx, "No memory.");
            mln_lang_val_free(val);
            mln_lang_func_detail_free(func);
            return -1;
        }
        mln_lang_var_chain_add(&(func->args_head), &(func->args_tail), var);
        ++func->nargs;
    }
    if ((val = mln_lang_val_new(ctx, M_LANG_VAL_TYPE_FUNC, func)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_func_detail_free(func);
        return -1;
    }
    if ((var = mln_lang_var_new(ctx, &funcname, M_LANG_VAR_NORMAL, val, set)) == NULL) {
        mln_lang_val_free(val);
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if (mln_lang_set_member_add(ctx->pool, set->members, var) < 0) {
        mln_lang_var_free(var);
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }

    return 0;
}

static mln_lang_var_t *mln_lang_mysql_close_process(mln_lang_ctx_t *ctx)
{
    mln_s32_t type;
    mln_lang_val_t *val;
    mln_string_t _this = mln_string("this");
    mln_string_t v = mln_string("fd"), typename = mln_string("Mysql");
    mln_lang_symbol_node_t *sym;
    mln_lang_var_t *var;
    mln_lang_var_t *ret_var;
    mln_lang_mysql_t *mysql;

    if ((sym = mln_lang_symbol_node_search(ctx, &_this, 1)) == NULL) {
        ASSERT(0);
        mln_lang_errmsg(ctx, "Lack of 'this'.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);

    type = mln_lang_var_val_type_get(sym->data.var);
    val = sym->data.var->val;
    if (type != M_LANG_VAL_TYPE_OBJECT) {
        mln_lang_errmsg(ctx, "'this' not object.");
        return NULL;
    }
    if (val->data.obj->in_set == NULL || val->data.obj->in_set->name == NULL || mln_string_strcmp(val->data.obj->in_set->name, &typename)) {
        mln_lang_errmsg(ctx, "Invalid set type, Mysql object required.");
        return NULL;
    }
    if ((var = mln_lang_set_member_search(val->data.obj->members, &v)) == NULL) {
        mln_lang_errmsg(ctx, "Lack of member 'fd'.");
        return NULL;
    }
    if (mln_lang_var_val_type_get(var) != M_LANG_VAL_TYPE_INT) {
        mln_lang_errmsg(ctx, "Invalid type of 'fd'.");
        return NULL;
    }
    val = var->val;
    memcpy(&mysql, &(val->data.s), sizeof(mln_lang_mysql_t *));
    mln_lang_mysql_free(mysql);
    val->data.i = 0;
    if ((ret_var = mln_lang_var_create_nil(ctx, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return NULL;
    }
    return ret_var;
}

static int mln_lang_mysql_add_close(mln_lang_ctx_t *ctx, mln_lang_set_detail_t *set)
{
    mln_lang_val_t *val;
    mln_lang_var_t *var;
    mln_lang_func_detail_t *func;
    mln_string_t funcname = mln_string("close");

    if ((func = mln_lang_func_detail_new(ctx, M_FUNC_INTERNAL, mln_lang_mysql_close_process, NULL, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if ((val = mln_lang_val_new(ctx, M_LANG_VAL_TYPE_FUNC, func)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_func_detail_free(func);
        return -1;
    }
    if ((var = mln_lang_var_new(ctx, &funcname, M_LANG_VAR_NORMAL, val, set)) == NULL) {
        mln_lang_val_free(val);
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if (mln_lang_set_member_add(ctx->pool, set->members, var) < 0) {
        mln_lang_var_free(var);
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    return 0;
}

static mln_lang_var_t *mln_lang_mysql_commit_process(mln_lang_ctx_t *ctx)
{
    mln_s32_t type;
    mln_lang_val_t *val;
    mln_string_t _this = mln_string("this");
    mln_string_t v = mln_string("fd"), typename = mln_string("Mysql");
    mln_lang_symbol_node_t *sym;
    mln_lang_var_t *var;
    mln_lang_var_t *ret_var;
    mln_lang_mysql_t *mysql;

    if ((sym = mln_lang_symbol_node_search(ctx, &_this, 1)) == NULL) {
        ASSERT(0);
        mln_lang_errmsg(ctx, "Lack of 'this'.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);

    type = mln_lang_var_val_type_get(sym->data.var);
    val = sym->data.var->val;
    if (type != M_LANG_VAL_TYPE_OBJECT) {
        mln_lang_errmsg(ctx, "'this' not object.");
        return NULL;
    }
    if (val->data.obj->in_set == NULL || val->data.obj->in_set->name == NULL || mln_string_strcmp(val->data.obj->in_set->name, &typename)) {
        mln_lang_errmsg(ctx, "Invalid set type, Mysql object required.");
        return NULL;
    }
    if ((var = mln_lang_set_member_search(val->data.obj->members, &v)) == NULL) {
        mln_lang_errmsg(ctx, "Lack of member 'fd'.");
        return NULL;
    }
    if (mln_lang_var_val_type_get(var) != M_LANG_VAL_TYPE_INT) {
        mln_lang_errmsg(ctx, "Invalid type of 'fd'.");
        return NULL;
    }
    val = var->val;
    memcpy(&mysql, &(val->data.s), sizeof(mln_lang_mysql_t *));
    if (mysql == NULL || mysql_commit(mysql->mysql)) {
        if ((ret_var = mln_lang_var_create_false(ctx, NULL)) == NULL) {
            mln_lang_errmsg(ctx, "No memory.");
            return NULL;
        }
    } else {
        if ((ret_var = mln_lang_var_create_true(ctx, NULL)) == NULL) {
            mln_lang_errmsg(ctx, "No memory.");
            return NULL;
        }
    }
    return ret_var;
}

static int mln_lang_mysql_add_commit(mln_lang_ctx_t *ctx, mln_lang_set_detail_t *set)
{
    mln_lang_val_t *val;
    mln_lang_var_t *var;
    mln_lang_func_detail_t *func;
    mln_string_t funcname = mln_string("commit");

    if ((func = mln_lang_func_detail_new(ctx, M_FUNC_INTERNAL, mln_lang_mysql_commit_process, NULL, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if ((val = mln_lang_val_new(ctx, M_LANG_VAL_TYPE_FUNC, func)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_func_detail_free(func);
        return -1;
    }
    if ((var = mln_lang_var_new(ctx, &funcname, M_LANG_VAR_NORMAL, val, set)) == NULL) {
        mln_lang_val_free(val);
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if (mln_lang_set_member_add(ctx->pool, set->members, var) < 0) {
        mln_lang_var_free(var);
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    return 0;
}

static mln_lang_var_t *mln_lang_mysql_rollback_process(mln_lang_ctx_t *ctx)
{
    mln_s32_t type;
    mln_lang_val_t *val;
    mln_string_t _this = mln_string("this");
    mln_string_t v = mln_string("fd"), typename = mln_string("Mysql");
    mln_lang_symbol_node_t *sym;
    mln_lang_var_t *var;
    mln_lang_var_t *ret_var;
    mln_lang_mysql_t *mysql;

    if ((sym = mln_lang_symbol_node_search(ctx, &_this, 1)) == NULL) {
        ASSERT(0);
        mln_lang_errmsg(ctx, "Lack of 'this'.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);

    type = mln_lang_var_val_type_get(sym->data.var);
    val = sym->data.var->val;
    if (type != M_LANG_VAL_TYPE_OBJECT) {
        mln_lang_errmsg(ctx, "'this' not object.");
        return NULL;
    }
    if (val->data.obj->in_set == NULL || val->data.obj->in_set->name == NULL || mln_string_strcmp(val->data.obj->in_set->name, &typename)) {
        mln_lang_errmsg(ctx, "Invalid set type, Mysql object required.");
        return NULL;
    }
    if ((var = mln_lang_set_member_search(val->data.obj->members, &v)) == NULL) {
        mln_lang_errmsg(ctx, "Lack of member 'fd'.");
        return NULL;
    }
    if (mln_lang_var_val_type_get(var) != M_LANG_VAL_TYPE_INT) {
        mln_lang_errmsg(ctx, "Invalid type of 'fd'.");
        return NULL;
    }
    val = var->val;
    memcpy(&mysql, &(val->data.s), sizeof(mln_lang_mysql_t *));
    if (mysql == NULL || mysql_rollback(mysql->mysql)) {
        if ((ret_var = mln_lang_var_create_false(ctx, NULL)) == NULL) {
            mln_lang_errmsg(ctx, "No memory.");
            return NULL;
        }
    } else {
        if ((ret_var = mln_lang_var_create_true(ctx, NULL)) == NULL) {
            mln_lang_errmsg(ctx, "No memory.");
            return NULL;
        }
    }
    return ret_var;
}

static int mln_lang_mysql_add_rollback(mln_lang_ctx_t *ctx, mln_lang_set_detail_t *set)
{
    mln_lang_val_t *val;
    mln_lang_var_t *var;
    mln_lang_func_detail_t *func;
    mln_string_t funcname = mln_string("rollback");

    if ((func = mln_lang_func_detail_new(ctx, M_FUNC_INTERNAL, mln_lang_mysql_rollback_process, NULL, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if ((val = mln_lang_val_new(ctx, M_LANG_VAL_TYPE_FUNC, func)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_func_detail_free(func);
        return -1;
    }
    if ((var = mln_lang_var_new(ctx, &funcname, M_LANG_VAR_NORMAL, val, set)) == NULL) {
        mln_lang_val_free(val);
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if (mln_lang_set_member_add(ctx->pool, set->members, var) < 0) {
        mln_lang_var_free(var);
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    return 0;
}

static mln_lang_var_t *mln_lang_mysql_error_process(mln_lang_ctx_t *ctx)
{
    mln_s32_t type;
    mln_lang_val_t *val;
    mln_string_t _this = mln_string("this");
    mln_string_t v = mln_string("fd"), typename = mln_string("Mysql"), tmp;
    mln_lang_symbol_node_t *sym;
    mln_lang_var_t *var;
    mln_lang_var_t *ret_var;
    mln_lang_mysql_t *mysql;
    const char *err;

    if ((sym = mln_lang_symbol_node_search(ctx, &_this, 1)) == NULL) {
        ASSERT(0);
        mln_lang_errmsg(ctx, "Lack of 'this'.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);

    type = mln_lang_var_val_type_get(sym->data.var);
    val = sym->data.var->val;
    if (type != M_LANG_VAL_TYPE_OBJECT) {
        mln_lang_errmsg(ctx, "'this' not object.");
        return NULL;
    }
    if (val->data.obj->in_set == NULL || val->data.obj->in_set->name == NULL || mln_string_strcmp(val->data.obj->in_set->name, &typename)) {
        mln_lang_errmsg(ctx, "Invalid set type, Mysql object required.");
        return NULL;
    }
    if ((var = mln_lang_set_member_search(val->data.obj->members, &v)) == NULL) {
        mln_lang_errmsg(ctx, "Lack of member 'fd'.");
        return NULL;
    }
    if (mln_lang_var_val_type_get(var) != M_LANG_VAL_TYPE_INT) {
        mln_lang_errmsg(ctx, "Invalid type of 'fd'.");
        return NULL;
    }
    val = var->val;
    memcpy(&mysql, &(val->data.s), sizeof(mln_lang_mysql_t *));
    if (mysql == NULL) {
        if ((ret_var = mln_lang_var_create_false(ctx, NULL)) == NULL) {
            mln_lang_errmsg(ctx, "No memory.");
            return NULL;
        }
    } else {
        err = mysql_error(mysql->mysql);
        mln_string_set(&tmp, err);
        if ((ret_var = mln_lang_var_create_string(ctx, &tmp, NULL)) == NULL) {
            mln_lang_errmsg(ctx, "No memory.");
            return NULL;
        }
    }
    return ret_var;
}

static int mln_lang_mysql_add_error(mln_lang_ctx_t *ctx, mln_lang_set_detail_t *set)
{
    mln_lang_val_t *val;
    mln_lang_var_t *var;
    mln_lang_func_detail_t *func;
    mln_string_t funcname = mln_string("error");

    if ((func = mln_lang_func_detail_new(ctx, M_FUNC_INTERNAL, mln_lang_mysql_error_process, NULL, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if ((val = mln_lang_val_new(ctx, M_LANG_VAL_TYPE_FUNC, func)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_func_detail_free(func);
        return -1;
    }
    if ((var = mln_lang_var_new(ctx, &funcname, M_LANG_VAR_NORMAL, val, set)) == NULL) {
        mln_lang_val_free(val);
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if (mln_lang_set_member_add(ctx->pool, set->members, var) < 0) {
        mln_lang_var_free(var);
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    return 0;
}

static mln_lang_var_t *mln_lang_mysql_errno_process(mln_lang_ctx_t *ctx)
{
    mln_s32_t type;
    mln_lang_val_t *val;
    mln_string_t _this = mln_string("this");
    mln_string_t v = mln_string("fd"), typename = mln_string("Mysql");
    mln_lang_symbol_node_t *sym;
    mln_lang_var_t *var;
    mln_lang_var_t *ret_var;
    mln_lang_mysql_t *mysql;
    unsigned int err;

    if ((sym = mln_lang_symbol_node_search(ctx, &_this, 1)) == NULL) {
        ASSERT(0);
        mln_lang_errmsg(ctx, "Lack of 'this'.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);

    type = mln_lang_var_val_type_get(sym->data.var);
    val = sym->data.var->val;
    if (type != M_LANG_VAL_TYPE_OBJECT) {
        mln_lang_errmsg(ctx, "'this' not object.");
        return NULL;
    }
    if (val->data.obj->in_set == NULL || val->data.obj->in_set->name == NULL || mln_string_strcmp(val->data.obj->in_set->name, &typename)) {
        mln_lang_errmsg(ctx, "Invalid set type, Mysql object required.");
        return NULL;
    }
    if ((var = mln_lang_set_member_search(val->data.obj->members, &v)) == NULL) {
        mln_lang_errmsg(ctx, "Lack of member 'fd'.");
        return NULL;
    }
    if (mln_lang_var_val_type_get(var) != M_LANG_VAL_TYPE_INT) {
        mln_lang_errmsg(ctx, "Invalid type of 'fd'.");
        return NULL;
    }
    val = var->val;
    memcpy(&mysql, &(val->data.s), sizeof(mln_lang_mysql_t *));
    if (mysql == NULL) {
        if ((ret_var = mln_lang_var_create_false(ctx, NULL)) == NULL) {
            mln_lang_errmsg(ctx, "No memory.");
            return NULL;
        }
    } else {
        err = mysql_errno(mysql->mysql);
        if ((ret_var = mln_lang_var_create_int(ctx, err, NULL)) == NULL) {
            mln_lang_errmsg(ctx, "No memory.");
            return NULL;
        }
    }
    return ret_var;
}

static int mln_lang_mysql_add_errno(mln_lang_ctx_t *ctx, mln_lang_set_detail_t *set)
{
    mln_lang_val_t *val;
    mln_lang_var_t *var;
    mln_lang_func_detail_t *func;
    mln_string_t funcname = mln_string("errno");

    if ((func = mln_lang_func_detail_new(ctx, M_FUNC_INTERNAL, mln_lang_mysql_errno_process, NULL, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if ((val = mln_lang_val_new(ctx, M_LANG_VAL_TYPE_FUNC, func)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_func_detail_free(func);
        return -1;
    }
    if ((var = mln_lang_var_new(ctx, &funcname, M_LANG_VAR_NORMAL, val, set)) == NULL) {
        mln_lang_val_free(val);
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if (mln_lang_set_member_add(ctx->pool, set->members, var) < 0) {
        mln_lang_var_free(var);
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    return 0;
}

static void mln_lang_mysql_free_test(mln_event_t *ev, int fd, void *data)
{
    enum net_async_status status = NET_ASYNC_COMPLETE;
    mln_lang_mysql_timeout_t *lmt;
    mln_lang_mysql_t *mysql = (mln_lang_mysql_t *)data;
    mln_lang_ctx_t *ctx = mysql->ctx;

    lmt = (mln_lang_mysql_timeout_t *)mln_lang_ctx_resource_fetch(ctx, "mysql");
    ASSERT(lmt != NULL);

    if (mysql->result != NULL) {
        status = mysql_free_result_nonblocking(mysql->result);
    }
    if (status == NET_ASYNC_NOT_READY) {
        mln_event_set_fd(ctx->lang->ev, mysql->fd_signal, M_EV_SEND|M_EV_ONESHOT|M_EV_NONBLOCK, M_EV_UNLIMITED, mysql, mln_lang_mysql_free_test);
    } else {
        mysql->result = NULL;
        mln_event_set_fd(ctx->lang->ev, mysql->fd_signal, M_EV_CLR, M_EV_UNLIMITED, NULL, NULL);
        mln_lang_mysql_chain_del(&(lmt->head), &(lmt->tail), mysql);
        mln_lang_ctx_continue(ctx);
    }
}

static int mln_lang_mysql_build_return_value(mln_lang_mysql_t *mysql)
{
    mln_lang_array_t *array, *newarr;
    mln_lang_var_t *array_val, var;
    mln_lang_val_t val;
    mln_size_t i;
    mln_string_t tmp;
    mln_size_t *length;
    mln_lang_ctx_t *ctx = mysql->ctx;
    if (mysql->ret_var == NULL) {
        if ((mysql->ret_var = mln_lang_var_create_array(ctx, NULL)) == NULL) {
            mln_lang_errmsg(ctx, "No memory.");
            return -1;
        }
    }
    array = mysql->ret_var->val->data.array;

    if ((newarr = mln_lang_array_new(ctx)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    ++(newarr->ref);
    if ((array_val = mln_lang_array_get(ctx, array, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_array_free(newarr);
        return -1;
    }
    var.type = M_LANG_VAR_NORMAL;
    var.name = NULL;
    var.val = &val;
    var.in_set = NULL;
    var.prev = var.next = NULL;
    val.data.array = newarr;
    val.type = M_LANG_VAL_TYPE_ARRAY;
    val.ref = 1;
    if (mln_lang_var_value_set(ctx, array_val, &var) < 0) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_array_free(newarr);
        return -1;
    }
    length = mysql_fetch_lengths(mysql->result);
    for (i = 0; i < mysql->nfield; ++i) {
        mln_string_nset(&tmp, mysql->row[i], length[i]);
        if ((array_val = mln_lang_array_get(ctx, newarr, NULL)) == NULL) {
            mln_lang_errmsg(ctx, "No memory.");
            return -1;
        }
        var.type = M_LANG_VAR_NORMAL;
        var.name = NULL;
        var.val = &val;
        var.in_set = NULL;
        var.prev = var.next = NULL;
        val.data.s = &tmp;
        val.type = M_LANG_VAL_TYPE_STRING;
        val.ref = 1;
        if (mln_lang_var_value_set(ctx, array_val, &var) < 0) {
            mln_lang_errmsg(ctx, "No memory.");
            return -1;
        }
    }

    return 0;
}

static void mln_lang_mysql_fetch_test(mln_event_t *ev, int fd, void *data)
{
    enum net_async_status status;
    mln_lang_mysql_t *mysql = (mln_lang_mysql_t *)data;
    mln_lang_ctx_t *ctx = mysql->ctx;

    status = mysql_fetch_row_nonblocking(mysql->result, &(mysql->row));
    if (status != NET_ASYNC_COMPLETE) {
        mln_event_set_fd(ctx->lang->ev, mysql->fd_signal, M_EV_SEND|M_EV_ONESHOT|M_EV_NONBLOCK, M_EV_UNLIMITED, mysql, mln_lang_mysql_fetch_test);
    } else if (status == NET_ASYNC_ERROR) {
        mln_event_set_fd(ctx->lang->ev, mysql->fd_signal, M_EV_SEND|M_EV_ONESHOT|M_EV_NONBLOCK, M_EV_UNLIMITED, mysql, mln_lang_mysql_free_test);
    } else {
        if (mysql->row) {
            if (mln_lang_mysql_build_return_value(mysql) < 0) {
                mln_event_set_fd(ctx->lang->ev, mysql->fd_signal, M_EV_SEND|M_EV_ONESHOT|M_EV_NONBLOCK, M_EV_UNLIMITED, mysql, mln_lang_mysql_free_test);
            } else {
                mln_event_set_fd(ctx->lang->ev, mysql->fd_signal, M_EV_SEND|M_EV_ONESHOT|M_EV_NONBLOCK, M_EV_UNLIMITED, mysql, mln_lang_mysql_fetch_test);
            }
        } else {
            if (mysql->ret_var == NULL && (mysql->ret_var = mln_lang_var_create_array(ctx, NULL)) == NULL) {
                mln_lang_errmsg(ctx, "No memory.");
            }
            if (mysql->ret_var != NULL) {
                mln_lang_ctx_set_ret_var(mysql->ctx, mysql->ret_var);
                mysql->ret_var = NULL;
            }
            mln_event_set_fd(ctx->lang->ev, mysql->fd_signal, M_EV_SEND|M_EV_ONESHOT|M_EV_NONBLOCK, M_EV_UNLIMITED, mysql, mln_lang_mysql_free_test);
        }
    }
}

static void mln_lang_mysql_result_test(mln_event_t *ev, int fd, void *data)
{
    enum net_async_status status;
    mln_lang_mysql_timeout_t *lmt;
    mln_lang_mysql_t *mysql = (mln_lang_mysql_t *)data;
    mln_lang_ctx_t *ctx = mysql->ctx;

    lmt = (mln_lang_mysql_timeout_t *)mln_lang_ctx_resource_fetch(ctx, "mysql");
    ASSERT(lmt != NULL);

    status = mysql_store_result_nonblocking(mysql->mysql, &(mysql->result));
    if (status == NET_ASYNC_NOT_READY) {
        mln_event_set_fd(ctx->lang->ev, mysql->fd_signal, M_EV_SEND|M_EV_ONESHOT|M_EV_NONBLOCK, M_EV_UNLIMITED, mysql, mln_lang_mysql_result_test);
    } else if (status == NET_ASYNC_ERROR) {
        mln_event_set_fd(ctx->lang->ev, mysql->fd_signal, M_EV_CLR, M_EV_UNLIMITED, NULL, NULL);
        mln_lang_mysql_chain_del(&(lmt->head), &(lmt->tail), mysql);
        mln_lang_ctx_continue(ctx);
    } else {
        if (mysql->result == NULL) {
            if ((mysql->ret_var = mln_lang_var_create_true(ctx, NULL)) == NULL) {
                mln_lang_errmsg(ctx, "No memory.");
            } else {
                mln_lang_ctx_set_ret_var(mysql->ctx, mysql->ret_var);
                mysql->ret_var = NULL;
            }
            mln_event_set_fd(ctx->lang->ev, mysql->fd_signal, M_EV_CLR, M_EV_UNLIMITED, NULL, NULL);
            mln_lang_mysql_chain_del(&(lmt->head), &(lmt->tail), mysql);
            mln_lang_ctx_continue(ctx);
        } else {
            mysql->nfield = mysql_num_fields(mysql->result);
            mln_event_set_fd(ctx->lang->ev, mysql->fd_signal, M_EV_SEND|M_EV_ONESHOT|M_EV_NONBLOCK, M_EV_UNLIMITED, mysql, mln_lang_mysql_fetch_test);
        }
    }
}

static void mln_lang_mysql_query_test(mln_event_t *ev, int fd, void *data)
{
    enum net_async_status status;
    mln_lang_mysql_timeout_t *lmt;
    mln_lang_mysql_t *mysql = (mln_lang_mysql_t *)data;
    mln_lang_ctx_t *ctx = mysql->ctx;

    lmt = (mln_lang_mysql_timeout_t *)mln_lang_ctx_resource_fetch(ctx, "mysql");
    ASSERT(lmt != NULL);

    status = mysql_real_query_nonblocking(mysql->mysql, (char *)(mysql->sql->data), mysql->sql->len);
    if (status == NET_ASYNC_ERROR) {
        mln_event_set_fd(ctx->lang->ev, mysql->fd_signal, M_EV_CLR, M_EV_UNLIMITED, NULL, NULL);
        mln_lang_mysql_chain_del(&(lmt->head), &(lmt->tail), mysql);
        mln_lang_ctx_continue(ctx);
    } else if (status == NET_ASYNC_NOT_READY) {
        mln_event_set_fd(ctx->lang->ev, mysql->fd_signal, M_EV_SEND|M_EV_ONESHOT|M_EV_NONBLOCK, M_EV_UNLIMITED, mysql, mln_lang_mysql_query_test);
    } else {
        mln_event_set_fd(ctx->lang->ev, mysql->fd_signal, M_EV_SEND|M_EV_ONESHOT|M_EV_NONBLOCK, M_EV_UNLIMITED, mysql, mln_lang_mysql_result_test);
    }
}

static mln_lang_var_t *mln_lang_mysql_execute_process(mln_lang_ctx_t *ctx)
{
    mln_s32_t type;
    mln_lang_val_t *val1, *val;
    mln_string_t _this = mln_string("this");
    mln_string_t v1 = mln_string("sql");
    mln_string_t v = mln_string("fd");
    mln_string_t typename = mln_string("Mysql");
    mln_lang_symbol_node_t *sym;
    mln_lang_var_t *var;
    mln_lang_var_t *ret_var;
    mln_lang_mysql_t *mysql;
    mln_lang_mysql_timeout_t *lmt;

    if ((sym = mln_lang_symbol_node_search(ctx, &_this, 1)) == NULL) {
        ASSERT(0);
        mln_lang_errmsg(ctx, "Lack of 'this'.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);
    type = mln_lang_var_val_type_get(sym->data.var);
    val = sym->data.var->val;
    if (type != M_LANG_VAL_TYPE_OBJECT) {
        mln_lang_errmsg(ctx, "'this' not object.");
        return NULL;
    }
    if (val->data.obj->in_set == NULL || val->data.obj->in_set->name == NULL || mln_string_strcmp(val->data.obj->in_set->name, &typename)) {
        mln_lang_errmsg(ctx, "Invalid set type, Mysql object required.");
        return NULL;
    }
    /*fd*/
    if ((var = mln_lang_set_member_search(val->data.obj->members, &v)) == NULL) {
        mln_lang_errmsg(ctx, "Lack of member 'fd'.");
        return NULL;
    }
    if (mln_lang_var_val_type_get(var) != M_LANG_VAL_TYPE_INT) {
        mln_lang_errmsg(ctx, "Invalid type of 'fd'.");
        return NULL;
    }
    val = var->val;
    memcpy(&mysql, &(val->data.s), sizeof(mln_lang_mysql_t *));
    if (mysql == NULL) {
        if ((ret_var = mln_lang_var_create_false(ctx, NULL)) == NULL) {
            mln_lang_errmsg(ctx, "No memory.");
            return NULL;
        }
        return ret_var;
    }
    /*arg1*/
    if ((sym = mln_lang_symbol_node_search(ctx, &v1, 1)) == NULL) {
        mln_lang_errmsg(ctx, "Argument missing.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);
    type = mln_lang_var_val_type_get(sym->data.var);
    val1 = sym->data.var->val;
    if (type != M_LANG_VAL_TYPE_STRING || val1->data.s == NULL) {
        mln_lang_errmsg(ctx, "'sql' must be string.");
        return NULL;
    }

    if (mysql->sql != NULL) {
        mln_string_free(mysql->sql);
        mysql->sql = NULL;
    }
    if ((mysql->sql = mln_string_pool_dup(ctx->pool, val1->data.s)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return NULL;
    }

    lmt = (mln_lang_mysql_timeout_t *)mln_lang_ctx_resource_fetch(ctx, "mysql");
    ASSERT(lmt != NULL);
    if (mln_event_set_fd(ctx->lang->ev, mysql->fd_signal, M_EV_SEND|M_EV_ONESHOT|M_EV_NONBLOCK, M_EV_UNLIMITED, mysql, mln_lang_mysql_query_test) < 0) {
        mln_lang_errmsg(ctx, "No memory.");
        return NULL;
    }
    if ((ret_var = mln_lang_var_create_false(ctx, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_event_set_fd(ctx->lang->ev, mysql->fd_signal, M_EV_CLR, M_EV_UNLIMITED, NULL, NULL);
        return NULL;
    }
    mln_lang_mysql_chain_add(&(lmt->head), &(lmt->tail), mysql);
    mln_lang_ctx_suspend(ctx);

    return ret_var;
}

static int mln_lang_mysql_add_execute(mln_lang_ctx_t *ctx, mln_lang_set_detail_t *set)
{
    mln_lang_val_t *val;
    mln_lang_var_t *var;
    mln_lang_func_detail_t *func;
    mln_string_t funcname = mln_string("execute");
    mln_string_t v1 = mln_string("sql");

    if ((func = mln_lang_func_detail_new(ctx, M_FUNC_INTERNAL, mln_lang_mysql_execute_process, NULL, NULL)) == NULL) {
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
    if ((var = mln_lang_var_new(ctx, &funcname, M_LANG_VAR_NORMAL, val, set)) == NULL) {
        mln_lang_val_free(val);
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if (mln_lang_set_member_add(ctx->pool, set->members, var) < 0) {
        mln_lang_var_free(var);
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }

    return 0;
}

static mln_lang_var_t *mln_lang_mysql_autocommit_process(mln_lang_ctx_t *ctx)
{
    mln_s32_t type;
    mln_lang_val_t *val1, *val;
    mln_string_t _this = mln_string("this");
    mln_string_t v1 = mln_string("mode");
    mln_string_t v = mln_string("fd");
    mln_string_t typename = mln_string("Mysql");
    mln_lang_symbol_node_t *sym;
    mln_lang_var_t *var;
    mln_lang_var_t *ret_var;
    mln_lang_mysql_t *mysql;

    if ((sym = mln_lang_symbol_node_search(ctx, &_this, 1)) == NULL) {
        ASSERT(0);
        mln_lang_errmsg(ctx, "Lack of 'this'.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);
    type = mln_lang_var_val_type_get(sym->data.var);
    val = sym->data.var->val;
    if (type != M_LANG_VAL_TYPE_OBJECT) {
        mln_lang_errmsg(ctx, "'this' not object.");
        return NULL;
    }
    if (val->data.obj->in_set == NULL || val->data.obj->in_set->name == NULL || mln_string_strcmp(val->data.obj->in_set->name, &typename)) {
        mln_lang_errmsg(ctx, "Invalid set type, Mysql object required.");
        return NULL;
    }
    /*fd*/
    if ((var = mln_lang_set_member_search(val->data.obj->members, &v)) == NULL) {
        mln_lang_errmsg(ctx, "Lack of member 'fd'.");
        return NULL;
    }
    if (mln_lang_var_val_type_get(var) != M_LANG_VAL_TYPE_INT) {
        mln_lang_errmsg(ctx, "Invalid type of 'fd'.");
        return NULL;
    }
    val = var->val;
    memcpy(&mysql, &(val->data.s), sizeof(mln_lang_mysql_t *));
    if (mysql == NULL) {
        if ((ret_var = mln_lang_var_create_false(ctx, NULL)) == NULL) {
            mln_lang_errmsg(ctx, "No memory.");
            return NULL;
        }
        return ret_var;
    }
    /*arg1*/
    if ((sym = mln_lang_symbol_node_search(ctx, &v1, 1)) == NULL) {
        mln_lang_errmsg(ctx, "Argument missing.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);
    type = mln_lang_var_val_type_get(sym->data.var);
    val1 = sym->data.var->val;
    if (type != M_LANG_VAL_TYPE_BOOL) {
        mln_lang_errmsg(ctx, "'mode' must be boolean.");
        return NULL;
    }

    if (mysql_autocommit(mysql->mysql, val1->data.b)) {
        if ((ret_var = mln_lang_var_create_false(ctx, NULL)) == NULL) {
            mln_lang_errmsg(ctx, "No memory.");
            return NULL;
        }
    } else {
        if ((ret_var = mln_lang_var_create_true(ctx, NULL)) == NULL) {
            mln_lang_errmsg(ctx, "No memory.");
            return NULL;
        }
    }
    return ret_var;
}

static int mln_lang_mysql_add_autocommit(mln_lang_ctx_t *ctx, mln_lang_set_detail_t *set)
{
    mln_lang_val_t *val;
    mln_lang_var_t *var;
    mln_lang_func_detail_t *func;
    mln_string_t funcname = mln_string("autocommit");
    mln_string_t v1 = mln_string("mode");

    if ((func = mln_lang_func_detail_new(ctx, M_FUNC_INTERNAL, mln_lang_mysql_autocommit_process, NULL, NULL)) == NULL) {
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
    if ((var = mln_lang_var_new(ctx, &funcname, M_LANG_VAR_NORMAL, val, set)) == NULL) {
        mln_lang_val_free(val);
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if (mln_lang_set_member_add(ctx->pool, set->members, var) < 0) {
        mln_lang_var_free(var);
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }

    return 0;
}

#endif

int mln_lang_mysql(mln_lang_ctx_t *ctx)
{
#ifdef MLN_MYSQL
    mln_lang_mysql_timeout_t *lmt = mln_lang_mysql_timeout_new(ctx);
    if (lmt == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if (mln_lang_ctx_resource_register(ctx, "mysql", lmt, (mln_lang_resource_free)mln_lang_mysql_timeout_free) < 0) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_mysql_timeout_free(lmt);
        return -1;
    }

    mln_lang_set_detail_t *set;
    mln_string_t setname = mln_string("Mysql");
    if ((set = mln_lang_set_detail_new(ctx->pool, &setname)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    ++(set->ref);
    if (mln_lang_symbol_node_join(ctx, M_LANG_SYMBOL_SET, set) < 0) {
        mln_lang_set_detail_self_free(set);
        return -1;
    }

    if (mln_lang_mysql_add_fd(ctx, set) < 0) {
        return -1;
    }
    if (mln_lang_mysql_add_connect(ctx, set) < 0) {
        return -1;
    }
    if (mln_lang_mysql_add_close(ctx, set) < 0) {
        return -1;
    }
    if (mln_lang_mysql_add_commit(ctx, set) < 0) {
        return -1;
    }
    if (mln_lang_mysql_add_rollback(ctx, set) < 0) {
        return -1;
    }
    if (mln_lang_mysql_add_error(ctx, set) < 0) {
        return -1;
    }
    if (mln_lang_mysql_add_errno(ctx, set) < 0) {
        return -1;
    }
    if (mln_lang_mysql_add_execute(ctx, set) < 0) {
        return -1;
    }
    if (mln_lang_mysql_add_autocommit(ctx, set) < 0) {
        return -1;
    }
#endif
    return 0;
}

