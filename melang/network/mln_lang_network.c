
/*
 * Copyright (C) Niklaus F.Schen.
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <signal.h>
#include "network/mln_lang_network.h"

#ifdef __DEBUG__
#include <assert.h>
#define ASSERT(x) assert(x)
#else
#define ASSERT(x);
#endif


static int mln_lang_network_resource_register(mln_lang_ctx_t *ctx);
static void mln_lang_network_resource_cancel(mln_lang_ctx_t *ctx);
/*tcp*/
MLN_CHAIN_FUNC_DECLARE(mln_lang_tcp, mln_lang_tcp_t, static inline void, __NONNULL3(1,2,3));
MLN_CHAIN_FUNC_DEFINE(mln_lang_tcp, mln_lang_tcp_t, static inline void, prev, next);
static void mln_lang_ctx_tcp_free(mln_lang_ctx_tcp_t *lct);
static mln_lang_ctx_tcp_t *mln_lang_ctx_tcp_new(mln_lang_ctx_t *ctx);
static int mln_lang_tcp_cmp(const mln_lang_tcp_t *lt1, const mln_lang_tcp_t *lt2);
static void mln_lang_tcp_free(mln_lang_tcp_t *lt);
static mln_lang_tcp_t *mln_lang_tcp_new(mln_lang_t *lang, int fd, char *ip, mln_u16_t port);
static int mln_lang_network_tcp_resource_add(mln_lang_t *lang, mln_lang_tcp_t *tcp);
static mln_lang_tcp_t *mln_lang_network_tcp_resource_fetch(mln_lang_t *lang, int fd);
static void mln_lang_ctx_tcp_resource_add(mln_lang_ctx_t *ctx, mln_lang_tcp_t *tcp);
static void mln_lang_ctx_tcp_resource_remove(mln_lang_tcp_t *tcp);
static void mln_lang_network_tcp_resource_remove(mln_lang_t *lang, int fd);
static int mln_lang_network_tcp_listen(mln_lang_ctx_t *ctx);
static int mln_lang_network_tcp_accept(mln_lang_ctx_t *ctx);
static int mln_lang_network_tcp_send(mln_lang_ctx_t *ctx);
static int mln_lang_network_tcp_recv(mln_lang_ctx_t *ctx);
static int mln_lang_network_tcp_connect(mln_lang_ctx_t *ctx);
static int mln_lang_network_tcp_shutdown(mln_lang_ctx_t *ctx);
static int mln_lang_network_tcp_close(mln_lang_ctx_t *ctx);
static mln_lang_retExp_t *mln_lang_network_tcp_listen_process(mln_lang_ctx_t *ctx);
static int mln_lang_network_get_addr(const struct sockaddr *addr, char *ip, mln_u16_t *port);
static mln_lang_retExp_t *mln_lang_network_tcp_close_process(mln_lang_ctx_t *ctx);
static mln_lang_retExp_t *mln_lang_network_tcp_shutdown_process(mln_lang_ctx_t *ctx);
static void mln_lang_network_tcp_accept_handler(mln_event_t *ev, int fd, void *data);
static void mln_lang_network_tcp_recv_handler(mln_event_t *ev, int fd, void *data);
static void mln_lang_network_tcp_send_handler(mln_event_t *ev, int fd, void *data);
static void mln_lang_network_tcp_timeout_handler(mln_event_t *ev, int fd, void *data);
static mln_lang_retExp_t *mln_lang_network_tcp_connect_process(mln_lang_ctx_t *ctx);
static void mln_lang_network_tcp_connect_handler(mln_event_t *ev, int fd, void *data);
static void mln_lang_network_tcp_connect_timeout_handler(mln_event_t *ev, int fd, void *data);
static mln_lang_retExp_t *mln_lang_network_tcp_accept_process(mln_lang_ctx_t *ctx);
static mln_lang_retExp_t *mln_lang_network_tcp_send_process(mln_lang_ctx_t *ctx);
static mln_lang_retExp_t *mln_lang_network_tcp_recv_process(mln_lang_ctx_t *ctx);
/*udp*/
MLN_CHAIN_FUNC_DECLARE(mln_lang_udp, mln_lang_udp_t, static inline void, __NONNULL3(1,2,3));
MLN_CHAIN_FUNC_DEFINE(mln_lang_udp, mln_lang_udp_t, static inline void, prev, next);
static int mln_lang_network_udp_resource_add(mln_lang_t *lang, mln_lang_udp_t *udp);
static mln_lang_udp_t *mln_lang_network_udp_resource_fetch(mln_lang_t *lang, int fd);
static void mln_lang_network_udp_resource_remove(mln_lang_t *lang, int fd);
static mln_lang_udp_t *mln_lang_udp_new(mln_lang_t *lang, int fd);
static void mln_lang_udp_free(mln_lang_udp_t *lu);
static int mln_lang_udp_cmp(const mln_lang_udp_t *lu1, const mln_lang_udp_t *lu2);
static mln_lang_ctx_udp_t *mln_lang_ctx_udp_new(mln_lang_ctx_t *ctx);
static void mln_lang_ctx_udp_free(mln_lang_ctx_udp_t *lcu);
static void mln_lang_ctx_udp_resource_add(mln_lang_ctx_t *ctx, mln_lang_udp_t *udp);
static void mln_lang_ctx_udp_resource_remove(mln_lang_udp_t *udp);
static int mln_lang_network_udp_create(mln_lang_ctx_t *ctx);
static mln_lang_retExp_t *mln_lang_network_udp_create_process(mln_lang_ctx_t *ctx);
static int mln_lang_network_udp_close(mln_lang_ctx_t *ctx);
static mln_lang_retExp_t *mln_lang_network_udp_close_process(mln_lang_ctx_t *ctx);
static int mln_lang_network_udp_send(mln_lang_ctx_t *ctx);
static mln_lang_retExp_t *mln_lang_network_udp_send_process(mln_lang_ctx_t *ctx);
static void mln_lang_network_udp_send_handler(mln_event_t *ev, int fd, void *data);
static int mln_lang_network_udp_recv(mln_lang_ctx_t *ctx);
static mln_lang_retExp_t *mln_lang_network_udp_recv_process(mln_lang_ctx_t *ctx);
static void mln_lang_network_udp_recv_handler(mln_event_t *ev, int fd, void *data);
static void mln_lang_network_udp_timeout_handler(mln_event_t *ev, int fd, void *data);

int mln_lang_network(mln_lang_ctx_t *ctx)
{
    if (mln_lang_network_resource_register(ctx) < 0) {
        return -1;
    }
    if (mln_lang_network_tcp_listen(ctx) < 0) {
        mln_lang_network_resource_cancel(ctx);
        return -1;
    }
    if (mln_lang_network_tcp_accept(ctx) < 0) {
        mln_lang_network_resource_cancel(ctx);
        return -1;
    }
    if (mln_lang_network_tcp_send(ctx) < 0) {
        mln_lang_network_resource_cancel(ctx);
        return -1;
    }
    if (mln_lang_network_tcp_recv(ctx) < 0) {
        mln_lang_network_resource_cancel(ctx);
        return -1;
    }
    if (mln_lang_network_tcp_connect(ctx) < 0) {
        mln_lang_network_resource_cancel(ctx);
        return -1;
    }
    if (mln_lang_network_tcp_shutdown(ctx) < 0) {
        mln_lang_network_resource_cancel(ctx);
        return -1;
    }
    if (mln_lang_network_tcp_close(ctx) < 0) {
        mln_lang_network_resource_cancel(ctx);
        return -1;
    }
    if (mln_lang_network_udp_create(ctx) < 0) {
        mln_lang_network_resource_cancel(ctx);
        return -1;
    }
    if (mln_lang_network_udp_close(ctx) < 0) {
        mln_lang_network_resource_cancel(ctx);
        return -1;
    }
    if (mln_lang_network_udp_send(ctx) < 0) {
        mln_lang_network_resource_cancel(ctx);
        return -1;
    }
    if (mln_lang_network_udp_recv(ctx) < 0) {
        mln_lang_network_resource_cancel(ctx);
        return -1;
    }
    return 0;
}

static int mln_lang_network_resource_register(mln_lang_ctx_t *ctx)
{
    mln_rbtree_t *tcp_set, *udp_set;
    if ((tcp_set = mln_lang_resource_fetch(ctx->lang, "tcp")) == NULL) {
        signal(SIGPIPE, SIG_IGN);
        struct mln_rbtree_attr rbattr;
        rbattr.cmp = (rbtree_cmp)mln_lang_tcp_cmp;
        rbattr.data_free = (rbtree_free_data)mln_lang_tcp_free;
        if ((tcp_set = mln_rbtree_init(&rbattr)) == NULL) {
            mln_lang_errmsg(ctx, "No memory.");
            return -1;
        }
        if (mln_lang_resource_register(ctx->lang, "tcp", tcp_set, (mln_lang_resource_free)mln_rbtree_destroy) < 0) {
            mln_lang_errmsg(ctx, "No memory.");
            mln_rbtree_destroy(tcp_set);
            return -1;
        }
    }
    if ((udp_set = mln_lang_resource_fetch(ctx->lang, "udp")) == NULL) {
        struct mln_rbtree_attr rbattr;
        rbattr.cmp = (rbtree_cmp)mln_lang_udp_cmp;
        rbattr.data_free = (rbtree_free_data)mln_lang_udp_free;
        if ((udp_set = mln_rbtree_init(&rbattr)) == NULL) {
            mln_lang_errmsg(ctx, "No memory.");
            return -1;
        }
        if (mln_lang_resource_register(ctx->lang, "udp", udp_set, (mln_lang_resource_free)mln_rbtree_destroy) < 0) {
            mln_lang_errmsg(ctx, "No memory.");
            mln_rbtree_destroy(udp_set);
            return -1;
        }
    }

    mln_lang_ctx_tcp_t *lct;
    if ((lct = mln_lang_ctx_tcp_new(ctx)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if (mln_lang_ctx_resource_register(ctx, "tcp", lct, (mln_lang_resource_free)mln_lang_ctx_tcp_free) < 0) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_ctx_tcp_free(lct);
        return -1;
    }
    mln_lang_ctx_udp_t *lcu;
    if ((lcu = mln_lang_ctx_udp_new(ctx)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if (mln_lang_ctx_resource_register(ctx, "udp", lcu, (mln_lang_resource_free)mln_lang_ctx_udp_free) < 0) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_ctx_udp_free(lcu);
        return -1;
    }
    return 0;
}

static void mln_lang_network_resource_cancel(mln_lang_ctx_t *ctx)
{
    mln_rbtree_t *tcp_set, *udp_set;
    if ((tcp_set = mln_lang_resource_fetch(ctx->lang, "tcp")) == NULL) {
        return;
    }
    if (!tcp_set->nr_node) {
        mln_lang_resource_cancel(ctx->lang, "tcp");
    }
    if ((udp_set = mln_lang_resource_fetch(ctx->lang, "udp")) == NULL) {
        return;
    }
    if (!udp_set->nr_node) {
        mln_lang_resource_cancel(ctx->lang, "udp");
    }
}

/*
 * tcp
 */
static int mln_lang_network_tcp_resource_add(mln_lang_t *lang, mln_lang_tcp_t *tcp)
{
    mln_rbtree_node_t *rn;
    mln_rbtree_t *tcp_set = mln_lang_resource_fetch(lang, "tcp");
    ASSERT(tcp_set != NULL);
    if ((rn = mln_rbtree_node_new(tcp_set, tcp)) == NULL) {
        return -1;
    }
    mln_rbtree_insert(tcp_set, rn);
    return 0;
}

static mln_lang_tcp_t *mln_lang_network_tcp_resource_fetch(mln_lang_t *lang, int fd)
{
    mln_lang_tcp_t tmp;
    mln_rbtree_node_t *rn;
    mln_rbtree_t *tcp_set = mln_lang_resource_fetch(lang, "tcp");
    ASSERT(tcp_set != NULL);
    mln_tcp_conn_set_fd(&(tmp.conn), fd);
    rn = mln_rbtree_search(tcp_set, tcp_set->root, &tmp);
    if (mln_rbtree_null(rn, tcp_set)) return NULL;
    return (mln_lang_tcp_t *)(rn->data);
}

static void mln_lang_network_tcp_resource_remove(mln_lang_t *lang, int fd)
{
    mln_lang_tcp_t tmp;
    mln_rbtree_node_t *rn;
    mln_rbtree_t *tcp_set = mln_lang_resource_fetch(lang, "tcp");
    ASSERT(tcp_set != NULL);
    mln_tcp_conn_set_fd(&(tmp.conn), fd);
    rn = mln_rbtree_search(tcp_set, tcp_set->root, &tmp);
    if (mln_rbtree_null(rn, tcp_set)) return;
    mln_rbtree_delete(tcp_set, rn);
    mln_rbtree_node_free(tcp_set, rn);
}

static int mln_lang_network_tcp_listen(mln_lang_ctx_t *ctx)
{
    mln_lang_val_t *val;
    mln_lang_var_t *var;
    mln_lang_func_detail_t *func;
    mln_string_t v1 = mln_string("host"), v2 = mln_string("service");
    mln_string_t funcname = mln_string("mln_tcpListen");

    if ((func = mln_lang_func_detail_new(ctx->pool, M_FUNC_INTERNAL, mln_lang_network_tcp_listen_process, NULL)) == NULL) {
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
    if ((val = mln_lang_val_new(ctx->pool, M_LANG_VAL_TYPE_NIL, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_func_detail_free(func);
        return -1;
    }
    if ((var = mln_lang_var_new(ctx->pool, &v2, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
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

static mln_lang_retExp_t *mln_lang_network_tcp_listen_process(mln_lang_ctx_t *ctx)
{
    mln_lang_val_t *val1, *val2;
    mln_lang_retExp_t *retExp;
    mln_string_t v1 = mln_string("host"), v2 = mln_string("service");
    mln_lang_symbolNode_t *sym;
    struct addrinfo addr, *res = NULL;
    int fd, opt = 1;
    mln_u16_t port;
    char ip[128] = {0}, host[128] = {0}, service[64] = {0};
    mln_lang_tcp_t *tcp;

    if ((sym = mln_lang_symbolNode_search(ctx, &v1, 1)) == NULL) {
        ASSERT(0);
        mln_lang_errmsg(ctx, "Argument missing.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);
    if (mln_lang_var_getValType(sym->data.var) != M_LANG_VAL_TYPE_STRING) {
        mln_lang_errmsg(ctx, "Invalid type of argument 1.");
        return NULL;
    }
    val1 = sym->data.var->val;
    if (val1->data.s->len > sizeof(host)-1) {
        mln_lang_errmsg(ctx, "Invalid host.");
        return NULL;
    }
    memcpy(host, val1->data.s->data, val1->data.s->len);

    if ((sym = mln_lang_symbolNode_search(ctx, &v2, 1)) == NULL) {
        ASSERT(0);
        mln_lang_errmsg(ctx, "Argument missing.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);
    if (mln_lang_var_getValType(sym->data.var) != M_LANG_VAL_TYPE_STRING) {
        mln_lang_errmsg(ctx, "Invalid type of argument 2.");
        return NULL;
    }
    val2 = sym->data.var->val;
    if (val2->data.s->len > sizeof(service)-1) {
        mln_lang_errmsg(ctx, "Invalid service.");
        return NULL;
    }
    memcpy(service, val2->data.s->data, val2->data.s->len);

    addr.ai_flags = AI_PASSIVE;
    addr.ai_family = AF_UNSPEC;
    addr.ai_socktype = SOCK_STREAM;
    addr.ai_protocol = IPPROTO_IP;
    if (getaddrinfo(host, service, &addr, &res) != 0 || res == NULL) {
        if ((retExp = mln_lang_retExp_createTmpFalse(ctx->pool, NULL)) == NULL) {
            mln_lang_errmsg(ctx, "No memory.");
            return NULL;
        }
        return retExp;
    }
    if ((fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) < 0) {
        freeaddrinfo(res);
        if ((retExp = mln_lang_retExp_createTmpFalse(ctx->pool, NULL)) == NULL) {
            mln_lang_errmsg(ctx, "No memory.");
            return NULL;
        }
        return retExp;
    }
    if (mln_lang_network_get_addr(res->ai_addr, ip, &port) < 0) {
        close(fd);
        freeaddrinfo(res);
        if ((retExp = mln_lang_retExp_createTmpFalse(ctx->pool, NULL)) == NULL) {
            mln_lang_errmsg(ctx, "No memory.");
            return NULL;
        }
        return retExp;
    }
    if ((tcp = mln_lang_tcp_new(ctx->lang, fd, ip, port)) == NULL) {
        close(fd);
        freeaddrinfo(res);
        mln_lang_errmsg(ctx, "No memory.");
        return NULL;
    }
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        mln_lang_tcp_free(tcp);
        freeaddrinfo(res);
        if ((retExp = mln_lang_retExp_createTmpFalse(ctx->pool, NULL)) == NULL) {
            mln_lang_errmsg(ctx, "No memory.");
            return NULL;
        }
        return retExp;
    }
    if (bind(fd, res->ai_addr, res->ai_addrlen) < 0) {
        mln_lang_tcp_free(tcp);
        freeaddrinfo(res);
        if ((retExp = mln_lang_retExp_createTmpFalse(ctx->pool, NULL)) == NULL) {
            mln_lang_errmsg(ctx, "No memory.");
            return NULL;
        }
        return retExp;
    }
    freeaddrinfo(res);
    if (listen(fd, 32767) < 0) {
        mln_lang_tcp_free(tcp);
        if ((retExp = mln_lang_retExp_createTmpFalse(ctx->pool, NULL)) == NULL) {
            mln_lang_errmsg(ctx, "No memory.");
            return NULL;
        }
        return retExp;
    }
    if ((retExp = mln_lang_retExp_createTmpInt(ctx->pool, fd, NULL)) == NULL) {
        mln_lang_tcp_free(tcp);
        mln_lang_errmsg(ctx, "No memory.");
        return NULL;
    }
    if (mln_lang_network_tcp_resource_add(ctx->lang, tcp) < 0) {
        mln_lang_retExp_free(retExp);
        mln_lang_tcp_free(tcp);
        mln_lang_errmsg(ctx, "No memory.");
        return NULL;
    }
    return retExp;
}

static int mln_lang_network_get_addr(const struct sockaddr *addr, char *ip, mln_u16_t *port)
{
    char *numeric_addr = NULL;
    char addr_buf[128] = {0};

    if (AF_INET == addr->sa_family) {
        numeric_addr = (char *)(&((struct sockaddr_in*)addr)->sin_addr);
        *port = ntohs(((struct sockaddr_in *)addr)->sin_port);
    } else if (AF_INET6 == addr->sa_family) {
        numeric_addr = (char *)(&((struct sockaddr_in6*)addr)->sin6_addr);
        *port = ntohs(((struct sockaddr_in6 *)addr)->sin6_port);
    } else {
        return -1;
    }
    if (inet_ntop(addr->sa_family, numeric_addr, addr_buf, sizeof(addr_buf)) == NULL)
        return -1;
    memcpy(ip, addr_buf, sizeof(addr_buf));
    return 0;
}

static int mln_lang_network_tcp_accept(mln_lang_ctx_t *ctx)
{
    mln_lang_val_t *val;
    mln_lang_var_t *var;
    mln_lang_func_detail_t *func;
    mln_string_t v1 = mln_string("fd"), v2 = mln_string("timeout");
    mln_string_t funcname = mln_string("mln_tcpAccept");

    if ((func = mln_lang_func_detail_new(ctx->pool, M_FUNC_INTERNAL, mln_lang_network_tcp_accept_process, NULL)) == NULL) {
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
    if ((val = mln_lang_val_new(ctx->pool, M_LANG_VAL_TYPE_NIL, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_func_detail_free(func);
        return -1;
    }
    if ((var = mln_lang_var_new(ctx->pool, &v2, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
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

static mln_lang_retExp_t *mln_lang_network_tcp_accept_process(mln_lang_ctx_t *ctx)
{
    mln_lang_retExp_t *retExp;
    mln_string_t v1 = mln_string("fd"), v2 = mln_string("timeout");
    mln_lang_symbolNode_t *sym;
    mln_lang_tcp_t *tcp;
    int fd, timeout, type;

    if ((sym = mln_lang_symbolNode_search(ctx, &v1, 1)) == NULL) {
        ASSERT(0);
        mln_lang_errmsg(ctx, "Argument missing.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);
    if (mln_lang_var_getValType(sym->data.var) != M_LANG_VAL_TYPE_INT) {
        mln_lang_errmsg(ctx, "Invalid type of argument 1.");
        return NULL;
    }
    fd = sym->data.var->val->data.i;

    if ((sym = mln_lang_symbolNode_search(ctx, &v2, 1)) == NULL) {
        ASSERT(0);
        mln_lang_errmsg(ctx, "Argument missing.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);
    type = mln_lang_var_getValType(sym->data.var);
    if (type == M_LANG_VAL_TYPE_NIL) {
        timeout = M_EV_UNLIMITED;
    } else if (type == M_LANG_VAL_TYPE_INT && sym->data.var->val->data.i >= 0) {
        timeout = sym->data.var->val->data.i;
    } else {
        mln_lang_errmsg(ctx, "Invalid type of argument 2.");
        return NULL;
    }

    if ((retExp = mln_lang_retExp_createTmpFalse(ctx->pool, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return NULL;
    }
    if ((tcp = mln_lang_network_tcp_resource_fetch(ctx->lang, fd)) == NULL) {
        return retExp;
    }
    if (tcp->recving) {
        mln_lang_errmsg(ctx, "Socket used in other script task.");
        return retExp;
    }
    if (tcp->recv_closed) {
        mln_lang_errmsg(ctx, "Socket recv shutdown.");
        return retExp;
    }
    if (tcp->sending) {
        mln_lang_errmsg(ctx, "Listen socket sending data.");
        mln_lang_retExp_free(retExp);
        return NULL;
    }
    tcp->timeout = timeout;
    if (mln_event_set_fd(ctx->lang->ev, fd, M_EV_RECV|M_EV_NONBLOCK|M_EV_ONESHOT,timeout, tcp, mln_lang_network_tcp_accept_handler) < 0) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_retExp_free(retExp);
        return NULL;
    }
    if (timeout >= 0) {
        mln_event_set_fd_timeout_handler(ctx->lang->ev, fd, tcp, mln_lang_network_tcp_timeout_handler);
    }
    tcp->recving = 1;
    mln_lang_ctx_tcp_resource_add(ctx, tcp);
    mln_lang_ctx_suspend(ctx);

    return retExp;
}

static int mln_lang_network_tcp_recv(mln_lang_ctx_t *ctx)
{
    mln_lang_val_t *val;
    mln_lang_var_t *var;
    mln_lang_func_detail_t *func;
    mln_string_t v1 = mln_string("fd"), v2 = mln_string("timeout");
    mln_string_t funcname = mln_string("mln_tcpRecv");

    if ((func = mln_lang_func_detail_new(ctx->pool, M_FUNC_INTERNAL, mln_lang_network_tcp_recv_process, NULL)) == NULL) {
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
    if ((val = mln_lang_val_new(ctx->pool, M_LANG_VAL_TYPE_NIL, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_func_detail_free(func);
        return -1;
    }
    if ((var = mln_lang_var_new(ctx->pool, &v2, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
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

static mln_lang_retExp_t *mln_lang_network_tcp_recv_process(mln_lang_ctx_t *ctx)
{
    mln_lang_retExp_t *retExp;
    mln_string_t v1 = mln_string("fd"), v2 = mln_string("timeout");
    mln_lang_symbolNode_t *sym;
    mln_lang_tcp_t *tcp;
    int fd, timeout, type;

    if ((sym = mln_lang_symbolNode_search(ctx, &v1, 1)) == NULL) {
        ASSERT(0);
        mln_lang_errmsg(ctx, "Argument missing.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);
    if (mln_lang_var_getValType(sym->data.var) != M_LANG_VAL_TYPE_INT) {
        mln_lang_errmsg(ctx, "Invalid type of argument 1.");
        return NULL;
    }
    fd = sym->data.var->val->data.i;

    if ((sym = mln_lang_symbolNode_search(ctx, &v2, 1)) == NULL) {
        ASSERT(0);
        mln_lang_errmsg(ctx, "Argument missing.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);
    type = mln_lang_var_getValType(sym->data.var);
    if (type == M_LANG_VAL_TYPE_NIL) {
        timeout = M_EV_UNLIMITED;
    } else if (type == M_LANG_VAL_TYPE_INT && sym->data.var->val->data.i >= 0) {
        timeout = sym->data.var->val->data.i;
    } else {
        mln_lang_errmsg(ctx, "Invalid type of argument 2.");
        return NULL;
    }

    if ((retExp = mln_lang_retExp_createTmpFalse(ctx->pool, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return NULL;
    }
    if ((tcp = mln_lang_network_tcp_resource_fetch(ctx->lang, fd)) == NULL) {
        return retExp;
    }
    if (tcp->recving || tcp->sending) {
        mln_lang_errmsg(ctx, "Socket used in other script task.");
        return retExp;
    }
    if (tcp->recv_closed) {
        mln_lang_errmsg(ctx, "Socket recv shutdown.");
        return retExp;
    }
    if (mln_event_set_fd(ctx->lang->ev, fd, M_EV_RECV|M_EV_NONBLOCK|M_EV_ONESHOT, timeout, tcp, mln_lang_network_tcp_recv_handler) < 0) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_retExp_free(retExp);
        return NULL;
    }
    if (tcp->timeout != M_EV_UNLIMITED)
        mln_event_set_fd_timeout_handler(tcp->lang->ev, fd, tcp, mln_lang_network_tcp_timeout_handler);

    tcp->recving = 1;
    mln_lang_ctx_tcp_resource_add(ctx, tcp);
    mln_lang_ctx_suspend(ctx);

    return retExp;
}

static int mln_lang_network_tcp_send(mln_lang_ctx_t *ctx)
{
    mln_lang_val_t *val;
    mln_lang_var_t *var;
    mln_lang_func_detail_t *func;
    mln_string_t v1 = mln_string("fd"), v2 = mln_string("data");
    mln_string_t funcname = mln_string("mln_tcpSend");

    if ((func = mln_lang_func_detail_new(ctx->pool, M_FUNC_INTERNAL, mln_lang_network_tcp_send_process, NULL)) == NULL) {
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
    if ((val = mln_lang_val_new(ctx->pool, M_LANG_VAL_TYPE_NIL, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_func_detail_free(func);
        return -1;
    }
    if ((var = mln_lang_var_new(ctx->pool, &v2, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
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

static mln_lang_retExp_t *mln_lang_network_tcp_send_process(mln_lang_ctx_t *ctx)
{
    mln_lang_retExp_t *retExp;
    mln_string_t v1 = mln_string("fd"), v2 = mln_string("data");
    mln_lang_symbolNode_t *sym;
    mln_lang_tcp_t *tcp;
    int fd;
    mln_string_t *data;
    mln_chain_t *c;
    mln_buf_t *b;
    mln_u8ptr_t buf;

    if ((sym = mln_lang_symbolNode_search(ctx, &v1, 1)) == NULL) {
        ASSERT(0);
        mln_lang_errmsg(ctx, "Argument missing.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);
    if (mln_lang_var_getValType(sym->data.var) != M_LANG_VAL_TYPE_INT) {
        mln_lang_errmsg(ctx, "Invalid type of argument 1.");
        return NULL;
    }
    fd = sym->data.var->val->data.i;

    if ((sym = mln_lang_symbolNode_search(ctx, &v2, 1)) == NULL) {
        ASSERT(0);
        mln_lang_errmsg(ctx, "Argument missing.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);
    if (mln_lang_var_getValType(sym->data.var) != M_LANG_VAL_TYPE_STRING) {
        mln_lang_errmsg(ctx, "Invalid type of argument 2.");
        return NULL;
    }
    data = sym->data.var->val->data.s;

    if ((retExp = mln_lang_retExp_createTmpFalse(ctx->pool, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return NULL;
    }
    if ((tcp = mln_lang_network_tcp_resource_fetch(ctx->lang, fd)) == NULL) {
        return retExp;
    }
    if (tcp->recving || tcp->sending) {
        mln_lang_errmsg(ctx, "Socket used in other script task.");
        return retExp;
    }
    if (tcp->send_closed) {
        mln_lang_errmsg(ctx, "Socket send shutdown.");
        return retExp;
    }
    if (mln_event_set_fd(ctx->lang->ev, fd, M_EV_SEND|M_EV_NONBLOCK|M_EV_ONESHOT, M_EV_UNLIMITED, tcp, mln_lang_network_tcp_send_handler) < 0) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_retExp_free(retExp);
        return NULL;
    }
    if ((c = mln_chain_new(mln_tcp_conn_get_pool(&(tcp->conn)))) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_event_set_fd(ctx->lang->ev, fd, M_EV_CLR, M_EV_UNLIMITED, NULL, NULL);
        mln_lang_retExp_free(retExp);
        return NULL;
    }
    if ((b = mln_buf_new(mln_tcp_conn_get_pool(&(tcp->conn)))) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_chain_pool_release(c);
        mln_event_set_fd(ctx->lang->ev, fd, M_EV_CLR, M_EV_UNLIMITED, NULL, NULL);
        mln_lang_retExp_free(retExp);
        return NULL;
    }
    c->buf = b;
    if ((buf = (mln_u8ptr_t)mln_alloc_m(mln_tcp_conn_get_pool(&(tcp->conn)), data->len)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_chain_pool_release(c);
        mln_event_set_fd(ctx->lang->ev, fd, M_EV_CLR, M_EV_UNLIMITED, NULL, NULL);
        mln_lang_retExp_free(retExp);
        return NULL;
    }
    memcpy(buf, data->data, data->len);
    b->left_pos = b->pos = b->start = buf;
    b->last = b->end = buf + data->len;
    b->in_memory = 1;
    b->last_buf = 1;
    mln_tcp_conn_append(&(tcp->conn), c, M_C_SEND);

    tcp->sending = 1;
    mln_lang_ctx_tcp_resource_add(ctx, tcp);
    mln_lang_ctx_suspend(ctx);

    return retExp;
}

static int mln_lang_network_tcp_connect(mln_lang_ctx_t *ctx)
{
    mln_lang_val_t *val;
    mln_lang_var_t *var;
    mln_lang_func_detail_t *func;
    mln_string_t v1 = mln_string("host"), v2 = mln_string("service"), v3 = mln_string("timeout");
    mln_string_t funcname = mln_string("mln_tcpConnect");

    if ((func = mln_lang_func_detail_new(ctx->pool, M_FUNC_INTERNAL, mln_lang_network_tcp_connect_process, NULL)) == NULL) {
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
    if ((val = mln_lang_val_new(ctx->pool, M_LANG_VAL_TYPE_NIL, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_func_detail_free(func);
        return -1;
    }
    if ((var = mln_lang_var_new(ctx->pool, &v2, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_val_free(val);
        mln_lang_func_detail_free(func);
        return -1;
    }
    mln_lang_var_chain_add(&(func->args_head), &(func->args_tail), var);
    ++func->nargs;
    if ((val = mln_lang_val_new(ctx->pool, M_LANG_VAL_TYPE_NIL, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_func_detail_free(func);
        return -1;
    }
    if ((var = mln_lang_var_new(ctx->pool, &v3, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
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

static mln_lang_retExp_t *mln_lang_network_tcp_connect_process(mln_lang_ctx_t *ctx)
{
    mln_lang_val_t *val;
    mln_lang_retExp_t *retExp;
    mln_string_t v1 = mln_string("host"), v2 = mln_string("service"), v3 = mln_string("timeout");
    mln_lang_symbolNode_t *sym;
    mln_lang_tcp_t *tcp;
    struct addrinfo addr, *res = NULL;
    int fd, type, timeout;
    mln_u16_t port;
    char host[128] = {0}, service[32] = {0}, ip[128] = {0};

    if ((sym = mln_lang_symbolNode_search(ctx, &v3, 1)) == NULL) {
        ASSERT(0);
        mln_lang_errmsg(ctx, "Argument missing.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);
    type = mln_lang_var_getValType(sym->data.var);
    if (type == M_LANG_VAL_TYPE_NIL) {
        timeout = M_EV_UNLIMITED;
    } else if (type == M_LANG_VAL_TYPE_INT && sym->data.var->val->data.i >= 0) {
        timeout = sym->data.var->val->data.i;
    } else {
        mln_lang_errmsg(ctx, "Invalid type of argument 3.");
        return NULL;
    }

    if ((sym = mln_lang_symbolNode_search(ctx, &v1, 1)) == NULL) {
        ASSERT(0);
        mln_lang_errmsg(ctx, "Argument missing.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);
    if (mln_lang_var_getValType(sym->data.var) != M_LANG_VAL_TYPE_STRING) {
        mln_lang_errmsg(ctx, "Invalid type of argument 1.");
        return NULL;
    }
    val = sym->data.var->val;
    if (val->data.s->len > sizeof(host)-1) {
        mln_lang_errmsg(ctx, "Invalid host.");
        return NULL;
    }
    memcpy(host, val->data.s->data, val->data.s->len);

    if ((sym = mln_lang_symbolNode_search(ctx, &v2, 1)) == NULL) {
        ASSERT(0);
        mln_lang_errmsg(ctx, "Argument missing.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);
    if (mln_lang_var_getValType(sym->data.var) != M_LANG_VAL_TYPE_STRING) {
        mln_lang_errmsg(ctx, "Invalid type of argument 2.");
        return NULL;
    }
    val = sym->data.var->val;
    if (val->data.s->len > sizeof(service)-1) {
        mln_lang_errmsg(ctx, "Invalid service.");
        return NULL;
    }
    memcpy(service, val->data.s->data, val->data.s->len);

    memset(&addr, 0, sizeof(addr));
    addr.ai_flags = AI_PASSIVE;
    addr.ai_family = AF_UNSPEC;
    addr.ai_socktype = SOCK_STREAM;
    addr.ai_protocol = IPPROTO_IP;
    if (getaddrinfo(host, service, &addr, &res) != 0 || res == NULL) {
        if ((retExp = mln_lang_retExp_createTmpFalse(ctx->pool, NULL)) == NULL) {
            mln_lang_errmsg(ctx, "No memory.");
            return NULL;
        }
        return retExp;
    }
    if ((fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) < 0) {
        freeaddrinfo(res);
        if ((retExp = mln_lang_retExp_createTmpFalse(ctx->pool, NULL)) == NULL) {
            mln_lang_errmsg(ctx, "No memory.");
            return NULL;
        }
        return retExp;
    }
    if (mln_lang_network_get_addr(res->ai_addr, ip, &port) < 0) {
        close(fd);
        freeaddrinfo(res);
        if ((retExp = mln_lang_retExp_createTmpFalse(ctx->pool, NULL)) == NULL) {
            mln_lang_errmsg(ctx, "No memory.");
            return NULL;
        }
        return retExp;
    }
    if ((tcp = mln_lang_tcp_new(ctx->lang, fd, ip, port)) == NULL) {
        close(fd);
        freeaddrinfo(res);
        mln_lang_errmsg(ctx, "No memory.");
        return NULL;
    }
    tcp->connect_timeout = timeout;
    if (mln_event_set_fd(ctx->lang->ev, \
                         fd, \
                         M_EV_SEND|M_EV_NONBLOCK|M_EV_ONESHOT, \
                         timeout, \
                         tcp, \
                         mln_lang_network_tcp_connect_handler) < 0)
    {
        mln_lang_tcp_free(tcp);
        freeaddrinfo(res);
        mln_lang_errmsg(ctx, "No memory.");
        return NULL;
    }
    if (connect(fd, res->ai_addr, res->ai_addrlen) < 0 && errno != EINPROGRESS) {
        mln_event_set_fd(ctx->lang->ev, fd, M_EV_CLR, M_EV_UNLIMITED, NULL, NULL);
        mln_lang_tcp_free(tcp);
        freeaddrinfo(res);
        if ((retExp = mln_lang_retExp_createTmpFalse(ctx->pool, NULL)) == NULL) {
            mln_lang_errmsg(ctx, "No memory.");
            return NULL;
        }
        return retExp;
    }
    freeaddrinfo(res);
    if ((retExp = mln_lang_retExp_createTmpFalse(ctx->pool, NULL)) == NULL) {
        mln_event_set_fd(ctx->lang->ev, fd, M_EV_CLR, M_EV_UNLIMITED, NULL, NULL);
        mln_lang_tcp_free(tcp);
        mln_lang_errmsg(ctx, "No memory.");
        return NULL;
    }
    if (mln_lang_network_tcp_resource_add(ctx->lang, tcp) < 0) {
        mln_lang_retExp_free(retExp);
        mln_event_set_fd(ctx->lang->ev, fd, M_EV_CLR, M_EV_UNLIMITED, NULL, NULL);
        mln_lang_tcp_free(tcp);
        mln_lang_errmsg(ctx, "No memory.");
        return NULL;
    }
    tcp->sending = 1;
    if (timeout != M_EV_UNLIMITED) {
        mln_event_set_fd_timeout_handler(tcp->lang->ev, fd, tcp, mln_lang_network_tcp_connect_timeout_handler);
    }
    mln_lang_ctx_tcp_resource_add(ctx, tcp);
    mln_lang_ctx_suspend(ctx);

    return retExp;
}

static int mln_lang_network_tcp_shutdown(mln_lang_ctx_t *ctx)
{
    mln_lang_val_t *val;
    mln_lang_var_t *var;
    mln_lang_func_detail_t *func;
    mln_string_t v1 = mln_string("fd"), v2 = mln_string("mode");
    mln_string_t funcname = mln_string("mln_tcpShutdown");

    if ((func = mln_lang_func_detail_new(ctx->pool, M_FUNC_INTERNAL, mln_lang_network_tcp_shutdown_process, NULL)) == NULL) {
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
    if ((val = mln_lang_val_new(ctx->pool, M_LANG_VAL_TYPE_NIL, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_func_detail_free(func);
        return -1;
    }
    if ((var = mln_lang_var_new(ctx->pool, &v2, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
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

static mln_lang_retExp_t *mln_lang_network_tcp_shutdown_process(mln_lang_ctx_t *ctx)
{
    mln_lang_val_t *val;
    mln_lang_retExp_t *retExp;
    mln_string_t v1 = mln_string("fd"), v2 = mln_string("mode");
    mln_lang_symbolNode_t *sym;
    mln_lang_tcp_t *tcp;
    mln_string_t send_mode = mln_string("send");
    mln_string_t recv_mode = mln_string("recv");
    int fd;

    if ((sym = mln_lang_symbolNode_search(ctx, &v1, 1)) == NULL) {
        ASSERT(0);
        mln_lang_errmsg(ctx, "Argument missing.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);
    if (mln_lang_var_getValType(sym->data.var) != M_LANG_VAL_TYPE_INT) {
        mln_lang_errmsg(ctx, "Invalid type of argument 1.");
        return NULL;
    }
    fd = sym->data.var->val->data.i;
    tcp = mln_lang_network_tcp_resource_fetch(ctx->lang, fd);
    if (tcp == NULL) {
        mln_lang_errmsg(ctx, "socket not existing.");
        return NULL;
    }

    if ((sym = mln_lang_symbolNode_search(ctx, &v2, 1)) == NULL) {
        ASSERT(0);
        mln_lang_errmsg(ctx, "Argument missing.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);
    if (mln_lang_var_getValType(sym->data.var) != M_LANG_VAL_TYPE_STRING) {
        mln_lang_errmsg(ctx, "Invalid type of argument 2.");
        return NULL;
    }
    val = sym->data.var->val;

    if ((retExp = mln_lang_retExp_createTmpNil(ctx->pool, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return NULL;
    }

    if (!mln_string_strcasecmp(val->data.s, &send_mode)) {
        if (tcp->recv_closed) {
            mln_lang_network_tcp_resource_remove(ctx->lang, fd);
        } else {
            if (tcp->sending) {
                if (tcp->recving) {
                    tcp->sending = 0;
                    tcp->send_closed = 1;
                    mln_event_set_fd(tcp->lang->ev, fd, M_EV_RECV|M_EV_NONBLOCK|M_EV_ONESHOT, tcp->timeout, tcp, mln_lang_network_tcp_recv_handler);
                    if (tcp->timeout != M_EV_UNLIMITED)
                        mln_event_set_fd_timeout_handler(tcp->lang->ev, fd, tcp, mln_lang_network_tcp_timeout_handler);
                } else {
                    mln_event_set_fd(tcp->lang->ev, fd, M_EV_CLR, M_EV_UNLIMITED, NULL, NULL);
                    mln_lang_ctx_continue(tcp->ctx);
                    mln_lang_ctx_tcp_resource_remove(tcp);
                }
            } else {
                tcp->send_closed = 1;
            }
            shutdown(fd, SHUT_WR);
        }
    } else if (!mln_string_strcasecmp(val->data.s, &recv_mode)) {
        if (tcp->send_closed) {
            mln_lang_network_tcp_resource_remove(ctx->lang, fd);
        } else {
            if (tcp->recving) {
                if (tcp->sending) {
                    tcp->recving = 0;
                    tcp->recv_closed = 1;
                    mln_event_set_fd(tcp->lang->ev, fd, M_EV_SEND|M_EV_NONBLOCK|M_EV_ONESHOT, M_EV_UNLIMITED, tcp, mln_lang_network_tcp_send_handler);
                } else {
                    mln_event_set_fd(tcp->lang->ev, fd, M_EV_CLR, M_EV_UNLIMITED, NULL, NULL);
                    mln_lang_ctx_continue(tcp->ctx);
                    mln_lang_ctx_tcp_resource_remove(tcp);
                }
            } else {
                tcp->recv_closed = 1;
            }
            shutdown(fd, SHUT_RD);
        }
    } else {
        mln_lang_errmsg(ctx, "Invalid type of argument 2.");
        mln_lang_retExp_free(retExp);
        return NULL;
    }

    return retExp;
}

static int mln_lang_network_tcp_close(mln_lang_ctx_t *ctx)
{
    mln_lang_val_t *val;
    mln_lang_var_t *var;
    mln_lang_func_detail_t *func;
    mln_string_t v1 = mln_string("fd");
    mln_string_t funcname = mln_string("mln_tcpClose");

    if ((func = mln_lang_func_detail_new(ctx->pool, M_FUNC_INTERNAL, mln_lang_network_tcp_close_process, NULL)) == NULL) {
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

static mln_lang_retExp_t *mln_lang_network_tcp_close_process(mln_lang_ctx_t *ctx)
{
    mln_lang_retExp_t *retExp;
    mln_string_t v1 = mln_string("fd");
    mln_lang_symbolNode_t *sym;

    if ((sym = mln_lang_symbolNode_search(ctx, &v1, 1)) == NULL) {
        ASSERT(0);
        mln_lang_errmsg(ctx, "Argument missing.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);
    if (mln_lang_var_getValType(sym->data.var) != M_LANG_VAL_TYPE_INT) {
        mln_lang_errmsg(ctx, "Invalid type of argument 1.");
        return NULL;
    }
    if ((retExp = mln_lang_retExp_createTmpNil(ctx->pool, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return NULL;
    }
    mln_lang_network_tcp_resource_remove(ctx->lang, sym->data.var->val->data.i);
    return retExp;
}

static void mln_lang_network_tcp_connect_handler(mln_event_t *ev, int fd, void *data)
{
    mln_lang_tcp_t *tcp = (mln_lang_tcp_t *)data;
    mln_lang_ctx_t *ctx = tcp->ctx;
    int err = 0, failed = 0;
    socklen_t len = sizeof(err);
    mln_lang_retExp_t *retExp;

    if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &len) >= 0) {
        if (err) {
            if (err == EINPROGRESS && (++tcp->retry <= MLN_LANG_NETWORK_TCP_CONNECT_RETRY)) {
                mln_event_set_fd(ev, fd, M_EV_SEND|M_EV_NONBLOCK|M_EV_ONESHOT, tcp->connect_timeout, tcp, mln_lang_network_tcp_connect_handler);
                if (tcp->connect_timeout != M_EV_UNLIMITED) {
                    mln_event_set_fd_timeout_handler(ev, fd, tcp, mln_lang_network_tcp_connect_timeout_handler);
                }
                return;
            } else {
                failed = 1;
            }
        } else {
            if ((retExp = mln_lang_retExp_createTmpInt(tcp->ctx->pool, fd, NULL)) != NULL) {
                mln_lang_ctx_setRetExp(tcp->ctx, retExp);
            } else {
                failed = 1;
            }
        }
    }
    mln_event_set_fd(ev, fd, M_EV_CLR, M_EV_UNLIMITED, NULL, NULL);
    tcp->sending = 0;
    mln_lang_ctx_continue(tcp->ctx);
    mln_lang_ctx_tcp_resource_remove(tcp);
    if (failed)
        mln_lang_network_tcp_resource_remove(ctx->lang, fd);
}

static void mln_lang_network_tcp_connect_timeout_handler(mln_event_t *ev, int fd, void *data)
{
    mln_lang_retExp_t *retExp;
    mln_lang_tcp_t *tcp = (mln_lang_tcp_t *)data;
    mln_lang_ctx_t *ctx = tcp->ctx;

    mln_event_set_fd(ev, fd, M_EV_CLR, M_EV_UNLIMITED, NULL, NULL);
    tcp->sending = 0;
    ASSERT(!tcp->recving);
    tcp->timeout = M_EV_UNLIMITED;
    mln_lang_ctx_tcp_resource_remove(tcp);
    mln_lang_network_tcp_resource_remove(ctx->lang, fd);
    if ((retExp = mln_lang_retExp_createTmpNil(ctx->pool, NULL)) != NULL) {
        mln_lang_ctx_setRetExp(ctx, retExp);
    }
    mln_lang_ctx_continue(ctx);
}

static void mln_lang_network_tcp_accept_handler(mln_event_t *ev, int fd, void *data)
{
    mln_lang_tcp_t *tcp = (mln_lang_tcp_t *)data, *newtcp;
    int connfd;
    struct sockaddr addr;
    char ip[128] = {0};
    mln_u16_t port;
    socklen_t len;
    mln_lang_retExp_t *retExp;

    memset(&addr, 0, sizeof(addr));
    len = sizeof(addr);
    if ((connfd = accept(fd, &addr, &len)) < 0) {
        if (errno == EINTR || errno == ECONNABORTED || errno == EAGAIN) {
            mln_event_set_fd(ev, fd, M_EV_RECV|M_EV_NONBLOCK|M_EV_ONESHOT, tcp->timeout, tcp, mln_lang_network_tcp_accept_handler);
            if (tcp->timeout >= 0)
                mln_event_set_fd_timeout_handler(ev, fd, tcp, mln_lang_network_tcp_timeout_handler);
            return;
        }
    } else {
        if (mln_lang_network_get_addr(&addr, ip, &port) < 0) {
            close(connfd);
        } else {
            if ((newtcp = mln_lang_tcp_new(tcp->lang, connfd, ip, port)) == NULL) {
                close(connfd);
            } else {
                if ((retExp = mln_lang_retExp_createTmpInt(tcp->ctx->pool, connfd, NULL)) == NULL) {
                    mln_lang_tcp_free(newtcp);
                } else {
                    if (mln_lang_network_tcp_resource_add(tcp->lang, newtcp) < 0) {
                        mln_lang_retExp_free(retExp);
                        mln_lang_tcp_free(newtcp);
                    } else {
                        mln_lang_ctx_setRetExp(tcp->ctx, retExp);
                    }
                }
            }
        }
    }
    mln_event_set_fd(ev, fd, M_EV_CLR, M_EV_UNLIMITED, NULL, NULL);
    tcp->recving = 0;
    mln_lang_ctx_continue(tcp->ctx);
    mln_lang_ctx_tcp_resource_remove(tcp);
}

static void mln_lang_network_tcp_recv_handler(mln_event_t *ev, int fd, void *data)
{
    mln_lang_tcp_t *tcp = (mln_lang_tcp_t *)data;
    mln_s64_t size = 0;
    mln_u8ptr_t buf, p;
    mln_string_t tmp;
    mln_chain_t *c;
    int rc = mln_tcp_conn_recv(&(tcp->conn), M_C_TYPE_MEMORY);;
    if (rc == M_C_ERROR) {
        /* do nothing */
    } else if (rc == M_C_CLOSED && mln_tcp_conn_get_head(&(tcp->conn), M_C_RECV) == NULL) {
        mln_lang_retExp_t *retExp = mln_lang_retExp_createTmpTrue(tcp->ctx->pool, NULL);
        if (retExp == NULL) {
            mln_event_set_fd(ev, fd, M_EV_SEND|M_EV_NONBLOCK|M_EV_ONESHOT, M_EV_UNLIMITED, tcp, mln_lang_network_tcp_recv_handler);
            return;
        }
        mln_lang_ctx_setRetExp(tcp->ctx, retExp);
    } else {
        c = mln_tcp_conn_get_head(&(tcp->conn), M_C_RECV);
        for (; c != NULL; c = c->next) {
            if (c->buf == NULL) continue;
            size += mln_buf_left_size(c->buf);
        }
        if ((buf = (mln_u8ptr_t)malloc(size)) == NULL) {
            mln_event_set_fd(ev, fd, M_EV_SEND|M_EV_NONBLOCK|M_EV_ONESHOT, M_EV_UNLIMITED, tcp, mln_lang_network_tcp_recv_handler);
            return;
        }
        for (p = buf, c = mln_tcp_conn_get_head(&(tcp->conn), M_C_RECV); c != NULL; c = c->next) {
            if (c->buf == NULL) continue;
            memcpy(p, c->buf->left_pos, mln_buf_left_size(c->buf));
            p += mln_buf_left_size(c->buf);
        }
        mln_string_nSet(&tmp, buf, size);
        mln_lang_retExp_t *retExp = mln_lang_retExp_createTmpString(tcp->ctx->pool, &tmp, NULL);
        free(buf);
        if (retExp == NULL) {
            mln_event_set_fd(ev, fd, M_EV_RECV|M_EV_NONBLOCK|M_EV_ONESHOT, M_EV_UNLIMITED, tcp, mln_lang_network_tcp_recv_handler);
            return;
        }
        mln_lang_ctx_setRetExp(tcp->ctx, retExp);
        mln_chain_pool_release_all(mln_tcp_conn_remove(&(tcp->conn), M_C_RECV));
    }
    mln_event_set_fd(ev, fd, M_EV_CLR, M_EV_UNLIMITED, NULL, NULL);
    tcp->recving = 0;
    mln_lang_ctx_continue(tcp->ctx);
    mln_lang_ctx_tcp_resource_remove(tcp);
}

static void mln_lang_network_tcp_send_handler(mln_event_t *ev, int fd, void *data)
{
    mln_lang_tcp_t *tcp = (mln_lang_tcp_t *)data;
    int rc = mln_tcp_conn_send(&(tcp->conn));

    if (rc == M_C_FINISH || rc == M_C_NOTYET) {
        mln_chain_pool_release_all(mln_tcp_conn_remove(&(tcp->conn), M_C_SENT));
        if (mln_tcp_conn_get_head(&(tcp->conn), M_C_SEND) != NULL) {
            mln_event_set_fd(ev, fd, M_EV_SEND|M_EV_NONBLOCK|M_EV_ONESHOT, M_EV_UNLIMITED, tcp, mln_lang_network_tcp_send_handler);
            return;
        } else {
            mln_lang_retExp_t *retExp = mln_lang_retExp_createTmpTrue(tcp->ctx->pool, NULL);
            if (retExp != NULL) mln_lang_ctx_setRetExp(tcp->ctx, retExp);
        }
    }
    mln_event_set_fd(ev, fd, M_EV_CLR, M_EV_UNLIMITED, NULL, NULL);
    tcp->sending = 0;
    mln_lang_ctx_continue(tcp->ctx);
    mln_lang_ctx_tcp_resource_remove(tcp);
}

static void mln_lang_network_tcp_timeout_handler(mln_event_t *ev, int fd, void *data)
{
    mln_lang_tcp_t *tcp = (mln_lang_tcp_t *)data;
    mln_lang_retExp_t *retExp;
    mln_event_set_fd(ev, fd, M_EV_CLR, M_EV_UNLIMITED, NULL, NULL);
    retExp = mln_lang_retExp_createTmpNil(tcp->ctx->pool, NULL);
    tcp->recving = 0;
    ASSERT(!tcp->sending);
    if (retExp != NULL) {
        mln_lang_ctx_setRetExp(tcp->ctx, retExp);
    }
    mln_lang_ctx_continue(tcp->ctx);
    mln_lang_ctx_tcp_resource_remove(tcp);
}

/*
 * tcp components
 */
static mln_lang_tcp_t *mln_lang_tcp_new(mln_lang_t *lang, int fd, char *ip, mln_u16_t port)
{
    int n;
    mln_lang_tcp_t *lt;
    if ((lt = (mln_lang_tcp_t *)malloc(sizeof(mln_lang_tcp_t))) == NULL) {
        return NULL;
    }
    lt->lang = lang;
    lt->ctx = NULL;
    if (mln_tcp_conn_init(&(lt->conn), fd) < 0) {
        free(lt);
        return NULL;
    }
    n = snprintf(lt->ip, 127, "%s", ip);
    lt->ip[n] = 0;
    lt->port = port;
    lt->send_closed = lt->recv_closed = 0;
    lt->sending = lt->recving = 0;
    lt->retry = 0;
    lt->timeout = 0;
    lt->connect_timeout = 0;
    lt->prev = lt->next = NULL;
    return lt;
}

static void mln_lang_tcp_free(mln_lang_tcp_t *lt)
{
    if (lt == NULL) return;
    if (lt->ctx != NULL) {
        mln_lang_ctx_tcp_t *lct = mln_lang_ctx_resource_fetch(lt->ctx, "tcp");
        if (lct != NULL) {
            mln_lang_tcp_chain_del(&(lct->head), &(lct->tail), lt);
            mln_lang_ctx_continue(lt->ctx);
            lt->ctx = NULL;
            lt->sending = lt->recving = 0;
        }
    }
    mln_event_set_fd(lt->lang->ev, mln_tcp_conn_get_fd(&(lt->conn)), M_EV_CLR, M_EV_UNLIMITED, NULL, NULL);
    close(mln_tcp_conn_get_fd(&(lt->conn)));
    mln_tcp_conn_destroy(&(lt->conn));
    free(lt);
}

static int mln_lang_tcp_cmp(const mln_lang_tcp_t *lt1, const mln_lang_tcp_t *lt2)
{
    return mln_tcp_conn_get_fd(&(lt1->conn)) - mln_tcp_conn_get_fd(&(lt2->conn));
}

static mln_lang_ctx_tcp_t *mln_lang_ctx_tcp_new(mln_lang_ctx_t *ctx)
{
    mln_lang_ctx_tcp_t *lct;
    if ((lct = (mln_lang_ctx_tcp_t *)mln_alloc_m(ctx->pool, sizeof(mln_lang_ctx_tcp_t))) == NULL) {
        return NULL;
    }
    lct->ctx = ctx;
    lct->head = lct->tail = NULL;
    return lct;
}

static void mln_lang_ctx_tcp_free(mln_lang_ctx_tcp_t *lct)
{
    if (lct == NULL) return;
    mln_lang_tcp_t *lt;
    while ((lt = lct->head) != NULL) {
        mln_lang_tcp_chain_del(&(lct->head), &(lct->tail), lt);
        mln_event_set_fd(lt->lang->ev, mln_tcp_conn_get_fd(&(lt->conn)), M_EV_CLR, M_EV_UNLIMITED, NULL, NULL);
        lt->ctx = NULL;
    }
    mln_alloc_free(lct);
}

static void mln_lang_ctx_tcp_resource_add(mln_lang_ctx_t *ctx, mln_lang_tcp_t *tcp)
{
    mln_lang_ctx_tcp_t *lct = mln_lang_ctx_resource_fetch(ctx, "tcp");
    ASSERT(lct != NULL);
    mln_lang_tcp_chain_add(&(lct->head), &(lct->tail), tcp);
    tcp->ctx = ctx;
}

static void mln_lang_ctx_tcp_resource_remove(mln_lang_tcp_t *tcp)
{
    mln_lang_ctx_tcp_t *lct = mln_lang_ctx_resource_fetch(tcp->ctx, "tcp");
    ASSERT(lct != NULL);
    mln_lang_tcp_chain_del(&(lct->head), &(lct->tail), tcp);
    tcp->ctx = NULL;
}

/*
 * udp
 */
static int mln_lang_network_udp_resource_add(mln_lang_t *lang, mln_lang_udp_t *udp)
{
    mln_rbtree_node_t *rn;
    mln_rbtree_t *udp_set = mln_lang_resource_fetch(lang, "udp");
    ASSERT(udp_set != NULL);
    if ((rn = mln_rbtree_node_new(udp_set, udp)) == NULL) {
        return -1;
    }
    mln_rbtree_insert(udp_set, rn);
    return 0;
}

static mln_lang_udp_t *mln_lang_network_udp_resource_fetch(mln_lang_t *lang, int fd)
{
    mln_lang_udp_t tmp;
    mln_rbtree_node_t *rn;
    mln_rbtree_t *udp_set = mln_lang_resource_fetch(lang, "udp");
    ASSERT(udp_set != NULL);
    tmp.fd = fd;
    rn = mln_rbtree_search(udp_set, udp_set->root, &tmp);
    if (mln_rbtree_null(rn, udp_set)) return NULL;
    return (mln_lang_udp_t *)(rn->data);
}

static void mln_lang_network_udp_resource_remove(mln_lang_t *lang, int fd)
{
    mln_lang_udp_t tmp;
    mln_rbtree_node_t *rn;
    mln_rbtree_t *udp_set = mln_lang_resource_fetch(lang, "udp");
    ASSERT(udp_set != NULL);
    tmp.fd = fd;
    rn = mln_rbtree_search(udp_set, udp_set->root, &tmp);
    if (mln_rbtree_null(rn, udp_set)) return;
    mln_rbtree_delete(udp_set, rn);
    mln_rbtree_node_free(udp_set, rn);
}

static int mln_lang_network_udp_create(mln_lang_ctx_t *ctx)
{
    mln_lang_val_t *val;
    mln_lang_var_t *var;
    mln_lang_func_detail_t *func;
    mln_string_t v1 = mln_string("host"), v2 = mln_string("service");
    mln_string_t funcname = mln_string("mln_udpCreate");

    if ((func = mln_lang_func_detail_new(ctx->pool, M_FUNC_INTERNAL, mln_lang_network_udp_create_process, NULL)) == NULL) {
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
    if ((val = mln_lang_val_new(ctx->pool, M_LANG_VAL_TYPE_NIL, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_func_detail_free(func);
        return -1;
    }
    if ((var = mln_lang_var_new(ctx->pool, &v2, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
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

static mln_lang_retExp_t *mln_lang_network_udp_create_process(mln_lang_ctx_t *ctx)
{
    mln_lang_val_t *val1, *val2;
    mln_lang_retExp_t *retExp;
    mln_string_t v1 = mln_string("host"), v2 = mln_string("service");
    mln_lang_symbolNode_t *sym;
    struct addrinfo addr, *res = NULL;
    int fd, opt = 1, type, notbind = 0;
    char host[128] = {0}, service[64] = {0};
    mln_lang_udp_t *udp;

    if ((sym = mln_lang_symbolNode_search(ctx, &v1, 1)) == NULL) {
        ASSERT(0);
        mln_lang_errmsg(ctx, "Argument missing.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);
    type = mln_lang_var_getValType(sym->data.var);
    if (type == M_LANG_VAL_TYPE_STRING) {
        val1 = sym->data.var->val;
        if (val1->data.s->len > sizeof(host)-1) {
            mln_lang_errmsg(ctx, "Invalid host.");
            return NULL;
        }
        memcpy(host, val1->data.s->data, val1->data.s->len);
    } else if (type == M_LANG_VAL_TYPE_NIL) {
        notbind = 1;
    } else {
        mln_lang_errmsg(ctx, "Invalid type of argument 1.");
        return NULL;
    }

    if ((sym = mln_lang_symbolNode_search(ctx, &v2, 1)) == NULL) {
        ASSERT(0);
        mln_lang_errmsg(ctx, "Argument missing.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);
    type = mln_lang_var_getValType(sym->data.var);
    if (type == M_LANG_VAL_TYPE_STRING) {
        if (notbind) {
            mln_lang_errmsg(ctx, "host cannot be nil.");
            return NULL;
        }
        val2 = sym->data.var->val;
        if (val2->data.s->len > sizeof(service)-1) {
            mln_lang_errmsg(ctx, "Invalid service.");
            return NULL;
        }
        memcpy(service, val2->data.s->data, val2->data.s->len);
    } else if (type == M_LANG_VAL_TYPE_NIL) {
        if (!notbind) {
            mln_lang_errmsg(ctx, "service cannot be nil.");
            return NULL;
        }
    } else {
        mln_lang_errmsg(ctx, "Invalid type of argument 2.");
        return NULL;
    }

    if (notbind) {
        if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
            if ((retExp = mln_lang_retExp_createTmpFalse(ctx->pool, NULL)) == NULL) {
                mln_lang_errmsg(ctx, "No memory.");
                return NULL;
            }
            return retExp;
        }
        if ((udp = mln_lang_udp_new(ctx->lang, fd)) == NULL) {
            close(fd);
            mln_lang_errmsg(ctx, "No memory.");
            return NULL;
        }
    } else {
        addr.ai_flags = AI_PASSIVE;
        addr.ai_family = AF_UNSPEC;
        addr.ai_socktype = SOCK_DGRAM;
        addr.ai_protocol = IPPROTO_IP;
        if (getaddrinfo(host, service, &addr, &res) != 0 || res == NULL) {
            if ((retExp = mln_lang_retExp_createTmpFalse(ctx->pool, NULL)) == NULL) {
                mln_lang_errmsg(ctx, "No memory.");
                return NULL;
            }
            return retExp;
        }
        if ((fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) < 0) {
            freeaddrinfo(res);
            if ((retExp = mln_lang_retExp_createTmpFalse(ctx->pool, NULL)) == NULL) {
                mln_lang_errmsg(ctx, "No memory.");
                return NULL;
            }
            return retExp;
        }
        if ((udp = mln_lang_udp_new(ctx->lang, fd)) == NULL) {
            close(fd);
            freeaddrinfo(res);
            mln_lang_errmsg(ctx, "No memory.");
            return NULL;
        }
        if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
            mln_lang_udp_free(udp);
            freeaddrinfo(res);
            if ((retExp = mln_lang_retExp_createTmpFalse(ctx->pool, NULL)) == NULL) {
                mln_lang_errmsg(ctx, "No memory.");
                return NULL;
            }
            return retExp;
        }
        if (bind(fd, res->ai_addr, res->ai_addrlen) < 0) {
            mln_lang_udp_free(udp);
            freeaddrinfo(res);
            if ((retExp = mln_lang_retExp_createTmpFalse(ctx->pool, NULL)) == NULL) {
                mln_lang_errmsg(ctx, "No memory.");
                return NULL;
            }
            return retExp;
        }
        freeaddrinfo(res);
    }
    if ((retExp = mln_lang_retExp_createTmpInt(ctx->pool, fd, NULL)) == NULL) {
        mln_lang_udp_free(udp);
        mln_lang_errmsg(ctx, "No memory.");
        return NULL;
    }
    if (mln_lang_network_udp_resource_add(ctx->lang, udp) < 0) {
        mln_lang_retExp_free(retExp);
        mln_lang_udp_free(udp);
        mln_lang_errmsg(ctx, "No memory.");
        return NULL;
    }
    return retExp;
}

static int mln_lang_network_udp_close(mln_lang_ctx_t *ctx)
{
    mln_lang_val_t *val;
    mln_lang_var_t *var;
    mln_lang_func_detail_t *func;
    mln_string_t v1 = mln_string("fd");
    mln_string_t funcname = mln_string("mln_udpClose");

    if ((func = mln_lang_func_detail_new(ctx->pool, M_FUNC_INTERNAL, mln_lang_network_udp_close_process, NULL)) == NULL) {
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

static mln_lang_retExp_t *mln_lang_network_udp_close_process(mln_lang_ctx_t *ctx)
{
    mln_lang_retExp_t *retExp;
    mln_string_t v1 = mln_string("fd");
    mln_lang_symbolNode_t *sym;

    if ((sym = mln_lang_symbolNode_search(ctx, &v1, 1)) == NULL) {
        ASSERT(0);
        mln_lang_errmsg(ctx, "Argument missing.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);
    if (mln_lang_var_getValType(sym->data.var) != M_LANG_VAL_TYPE_INT) {
        mln_lang_errmsg(ctx, "Invalid type of argument 1.");
        return NULL;
    }
    if ((retExp = mln_lang_retExp_createTmpNil(ctx->pool, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return NULL;
    }
    mln_lang_network_udp_resource_remove(ctx->lang, sym->data.var->val->data.i);
    return retExp;
}

static int mln_lang_network_udp_send(mln_lang_ctx_t *ctx)
{
    mln_lang_val_t *val;
    mln_lang_var_t *var;
    mln_lang_func_detail_t *func;
    mln_string_t v1 = mln_string("fd"), v2 = mln_string("data"), v3 = mln_string("host"), v4 = mln_string("service");
    mln_string_t funcname = mln_string("mln_udpSend");

    if ((func = mln_lang_func_detail_new(ctx->pool, M_FUNC_INTERNAL, mln_lang_network_udp_send_process, NULL)) == NULL) {
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
    if ((val = mln_lang_val_new(ctx->pool, M_LANG_VAL_TYPE_NIL, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_func_detail_free(func);
        return -1;
    }
    if ((var = mln_lang_var_new(ctx->pool, &v2, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_val_free(val);
        mln_lang_func_detail_free(func);
        return -1;
    }
    mln_lang_var_chain_add(&(func->args_head), &(func->args_tail), var);
    ++func->nargs;
    if ((val = mln_lang_val_new(ctx->pool, M_LANG_VAL_TYPE_NIL, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_func_detail_free(func);
        return -1;
    }
    if ((var = mln_lang_var_new(ctx->pool, &v3, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_val_free(val);
        mln_lang_func_detail_free(func);
        return -1;
    }
    mln_lang_var_chain_add(&(func->args_head), &(func->args_tail), var);
    ++func->nargs;
    if ((val = mln_lang_val_new(ctx->pool, M_LANG_VAL_TYPE_NIL, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_func_detail_free(func);
        return -1;
    }
    if ((var = mln_lang_var_new(ctx->pool, &v4, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
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

static mln_lang_retExp_t *mln_lang_network_udp_send_process(mln_lang_ctx_t *ctx)
{
    mln_lang_val_t *val;
    mln_lang_retExp_t *retExp;
    mln_string_t v1 = mln_string("fd"), v2 = mln_string("data"), v3 = mln_string("host"), v4 = mln_string("service");
    mln_lang_symbolNode_t *sym;
    struct addrinfo addr, *res = NULL;
    int fd;
    mln_string_t *data;
    char host[128] = {0}, service[64] = {0};
    mln_lang_udp_t *udp;

    if ((sym = mln_lang_symbolNode_search(ctx, &v1, 1)) == NULL) {
        ASSERT(0);
        mln_lang_errmsg(ctx, "Argument missing.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);
    if (mln_lang_var_getValType(sym->data.var) != M_LANG_VAL_TYPE_INT) {
        mln_lang_errmsg(ctx, "Invalid type of argument 1.");
        return NULL;
    }
    fd = sym->data.var->val->data.i;

    if ((sym = mln_lang_symbolNode_search(ctx, &v2, 1)) == NULL) {
        ASSERT(0);
        mln_lang_errmsg(ctx, "Argument missing.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);
    if (mln_lang_var_getValType(sym->data.var) != M_LANG_VAL_TYPE_STRING) {
        mln_lang_errmsg(ctx, "Invalid type of argument 2.");
        return NULL;
    }
    data = sym->data.var->val->data.s;

    if ((sym = mln_lang_symbolNode_search(ctx, &v3, 1)) == NULL) {
        ASSERT(0);
        mln_lang_errmsg(ctx, "Argument missing.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);
    if (mln_lang_var_getValType(sym->data.var) != M_LANG_VAL_TYPE_STRING) {
        mln_lang_errmsg(ctx, "Invalid type of argument 3.");
        return NULL;
    }
    val = sym->data.var->val;
    if (val->data.s->len > sizeof(host)-1) {
        mln_lang_errmsg(ctx, "Invalid host.");
        return NULL;
    }
    memcpy(host, val->data.s->data, val->data.s->len);

    if ((sym = mln_lang_symbolNode_search(ctx, &v4, 1)) == NULL) {
        ASSERT(0);
        mln_lang_errmsg(ctx, "Argument missing.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);
    if (mln_lang_var_getValType(sym->data.var) != M_LANG_VAL_TYPE_STRING) {
        mln_lang_errmsg(ctx, "Invalid type of argument 4.");
        return NULL;
    }
    val = sym->data.var->val;
    if (val->data.s->len > sizeof(host)-1) {
        mln_lang_errmsg(ctx, "Invalid service.");
        return NULL;
    }
    memcpy(service, val->data.s->data, val->data.s->len);

    if ((retExp = mln_lang_retExp_createTmpFalse(ctx->pool, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return NULL;
    }
    udp = mln_lang_network_udp_resource_fetch(ctx->lang, fd);
    if (udp == NULL) {
        mln_lang_retExp_free(retExp);
        mln_lang_errmsg(ctx, "socket not existing.");
        return NULL;
    }
    if (udp->recving || udp->sending) {
        mln_lang_errmsg(ctx, "Socket used in other script task.");
        return retExp;
    }
    addr.ai_flags = AI_PASSIVE;
    addr.ai_family = AF_UNSPEC;
    addr.ai_socktype = SOCK_DGRAM;
    addr.ai_protocol = IPPROTO_IP;
    if (getaddrinfo(host, service, &addr, &res) != 0 || res == NULL) {
        return retExp;
    }
    memcpy(&(udp->addr), res->ai_addr, sizeof(struct sockaddr));
    udp->len = res->ai_addrlen;
    freeaddrinfo(res);
    ASSERT(udp->data==NULL);
    if ((udp->data = mln_string_dup(data)) == NULL) {
        mln_lang_retExp_free(retExp);
        mln_lang_errmsg(ctx, "No memory.");
        return NULL;
    }
    if (mln_event_set_fd(ctx->lang->ev, fd, M_EV_SEND|M_EV_NONBLOCK|M_EV_ONESHOT, M_EV_UNLIMITED, udp, mln_lang_network_udp_send_handler) < 0) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_retExp_free(retExp);
        return NULL;
    }
    udp->sending = 1;
    mln_lang_ctx_udp_resource_add(ctx, udp);
    mln_lang_ctx_suspend(ctx);
    return retExp;
}

static void mln_lang_network_udp_send_handler(mln_event_t *ev, int fd, void *data)
{
    mln_lang_udp_t *udp = (mln_lang_udp_t *)data;
    int rc = sendto(fd, udp->data->data, udp->data->len, MSG_DONTWAIT, &(udp->addr), udp->len);
    int err = errno;
    mln_string_free(udp->data);
    udp->data = NULL;
    if (rc < 0 && err == EINTR) {
        mln_event_set_fd(ev, fd, M_EV_SEND|M_EV_NONBLOCK|M_EV_ONESHOT, M_EV_UNLIMITED, udp, mln_lang_network_udp_send_handler);
        return;
    }
    if (rc >= 0) {
        mln_lang_retExp_t *retExp = mln_lang_retExp_createTmpTrue(udp->ctx->pool, NULL);
        if (retExp != NULL) mln_lang_ctx_setRetExp(udp->ctx, retExp);
    }
    mln_event_set_fd(ev, fd, M_EV_CLR, M_EV_UNLIMITED, NULL, NULL);
    udp->sending = 0;
    mln_lang_ctx_continue(udp->ctx);
    mln_lang_ctx_udp_resource_remove(udp);
}

static int mln_lang_network_udp_recv(mln_lang_ctx_t *ctx)
{
    mln_lang_val_t *val;
    mln_lang_var_t *var;
    mln_lang_func_detail_t *func;
    mln_string_t v1 = mln_string("fd"), v2 = mln_string("bufsize"), v3 = mln_string("ip"), v4 = mln_string("port"), v5 = mln_string("timeout");
    mln_string_t funcname = mln_string("mln_udpRecv");

    if ((func = mln_lang_func_detail_new(ctx->pool, M_FUNC_INTERNAL, mln_lang_network_udp_recv_process, NULL)) == NULL) {
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

    if ((val = mln_lang_val_new(ctx->pool, M_LANG_VAL_TYPE_NIL, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_func_detail_free(func);
        return -1;
    }
    if ((var = mln_lang_var_new(ctx->pool, &v2, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_val_free(val);
        mln_lang_func_detail_free(func);
        return -1;
    }
    mln_lang_var_chain_add(&(func->args_head), &(func->args_tail), var);
    ++func->nargs;

    if ((val = mln_lang_val_new(ctx->pool, M_LANG_VAL_TYPE_NIL, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_func_detail_free(func);
        return -1;
    }
    if ((var = mln_lang_var_new(ctx->pool, &v3, M_LANG_VAR_REFER, val, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_val_free(val);
        mln_lang_func_detail_free(func);
        return -1;
    }
    mln_lang_var_chain_add(&(func->args_head), &(func->args_tail), var);
    ++func->nargs;

    if ((val = mln_lang_val_new(ctx->pool, M_LANG_VAL_TYPE_NIL, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_func_detail_free(func);
        return -1;
    }
    if ((var = mln_lang_var_new(ctx->pool, &v4, M_LANG_VAR_REFER, val, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_val_free(val);
        mln_lang_func_detail_free(func);
        return -1;
    }
    mln_lang_var_chain_add(&(func->args_head), &(func->args_tail), var);
    ++func->nargs;

    if ((val = mln_lang_val_new(ctx->pool, M_LANG_VAL_TYPE_NIL, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_func_detail_free(func);
        return -1;
    }
    if ((var = mln_lang_var_new(ctx->pool, &v5, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
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

static mln_lang_retExp_t *mln_lang_network_udp_recv_process(mln_lang_ctx_t *ctx)
{
    mln_lang_retExp_t *retExp;
    mln_string_t v1 = mln_string("fd"), v2 = mln_string("bufsize"), v3 = mln_string("ip"), v4 = mln_string("port"), v5 = mln_string("timeout");
    mln_lang_symbolNode_t *sym;
    int fd, timeout, type;
    mln_s64_t size;
    mln_lang_udp_t *udp;
    mln_lang_var_t *var1, *var2;
    /*arg1*/
    if ((sym = mln_lang_symbolNode_search(ctx, &v1, 1)) == NULL) {
        ASSERT(0);
        mln_lang_errmsg(ctx, "Argument missing.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);
    if (mln_lang_var_getValType(sym->data.var) != M_LANG_VAL_TYPE_INT) {
        mln_lang_errmsg(ctx, "Invalid type of argument 1.");
        return NULL;
    }
    fd = sym->data.var->val->data.i;
    /*arg2*/
    if ((sym = mln_lang_symbolNode_search(ctx, &v2, 1)) == NULL) {
        ASSERT(0);
        mln_lang_errmsg(ctx, "Argument missing.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);
    if (mln_lang_var_getValType(sym->data.var) != M_LANG_VAL_TYPE_INT) {
        mln_lang_errmsg(ctx, "Invalid type of argument 2.");
        return NULL;
    }
    size = sym->data.var->val->data.i;
    if (size <= 0) {
        mln_lang_errmsg(ctx, "Invalid type of argument 2.");
        return NULL;
    }
    /*arg3*/
    if ((sym = mln_lang_symbolNode_search(ctx, &v3, 1)) == NULL) {
        ASSERT(0);
        mln_lang_errmsg(ctx, "Argument missing.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);
    var1 = sym->data.var;
    /*arg4*/
    if ((sym = mln_lang_symbolNode_search(ctx, &v4, 1)) == NULL) {
        ASSERT(0);
        mln_lang_errmsg(ctx, "Argument missing.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);
    var2 = sym->data.var;
    /*arg5*/
    if ((sym = mln_lang_symbolNode_search(ctx, &v5, 1)) == NULL) {
        ASSERT(0);
        mln_lang_errmsg(ctx, "Argument missing.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);
    if ((type = mln_lang_var_getValType(sym->data.var)) == M_LANG_VAL_TYPE_NIL) {
        timeout = M_EV_UNLIMITED;
    } else if (type == M_LANG_VAL_TYPE_INT && (timeout = sym->data.var->val->data.i) >= 0) {
        /*do nothing*/
    } else {
        mln_lang_errmsg(ctx, "Invalid type of argument 5.");
        return NULL;
    }

    if ((retExp = mln_lang_retExp_createTmpFalse(ctx->pool, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return NULL;
    }
    udp = mln_lang_network_udp_resource_fetch(ctx->lang, fd);
    if (udp == NULL) {
        mln_lang_retExp_free(retExp);
        mln_lang_errmsg(ctx, "socket not existing.");
        return NULL;
    }
    if (udp->recving || udp->sending) {
        mln_lang_errmsg(ctx, "Socket used in other script task.");
        return retExp;
    }
    udp->timeout = timeout;
    udp->bufsize = size;
    udp->ip = var1;
    udp->port = var2;

    if (mln_event_set_fd(ctx->lang->ev, fd, M_EV_RECV|M_EV_NONBLOCK|M_EV_ONESHOT, udp->timeout, udp, mln_lang_network_udp_recv_handler) < 0) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_retExp_free(retExp);
        return NULL;
    }
    if (timeout != M_EV_UNLIMITED) {
        mln_event_set_fd_timeout_handler(ctx->lang->ev, fd, udp, mln_lang_network_udp_timeout_handler);
    }
    udp->recving = 1;
    mln_lang_ctx_udp_resource_add(ctx, udp);
    mln_lang_ctx_suspend(ctx);
    return retExp;
}

static void mln_lang_network_udp_recv_handler(mln_event_t *ev, int fd, void *data)
{
    mln_lang_udp_t *udp = (mln_lang_udp_t *)data;
    mln_u8ptr_t buf = NULL;
    struct sockaddr addr;
    socklen_t len;
    int rc;
    mln_lang_retExp_t *retExp;
    mln_lang_var_t var;
    mln_lang_val_t val;
    mln_string_t tmp;
    mln_u16_t port;
    char ip[128] = {0};

    memset(&addr, 0, sizeof(addr));
    len = sizeof(addr);
    if ((buf = (mln_u8ptr_t)malloc(udp->bufsize)) == NULL) {
        mln_event_set_fd(ev, fd, M_EV_RECV|M_EV_NONBLOCK|M_EV_ONESHOT, udp->timeout, udp, mln_lang_network_udp_recv_handler);
        if (udp->timeout != M_EV_UNLIMITED)
            mln_event_set_fd_timeout_handler(ev, fd, udp, mln_lang_network_udp_timeout_handler);
        return;
    }

    rc = recvfrom(fd, buf, udp->bufsize, MSG_DONTWAIT, &addr, &len);
    if (rc < 0) {
        free(buf);
        if (errno == EINTR) {
            mln_event_set_fd(ev, fd, M_EV_RECV|M_EV_NONBLOCK|M_EV_ONESHOT, udp->timeout, udp, mln_lang_network_udp_recv_handler);
            if (udp->timeout != M_EV_UNLIMITED)
                mln_event_set_fd_timeout_handler(ev, fd, udp, mln_lang_network_udp_timeout_handler);
            return;
        }
    } else if (rc == 0) {
        free(buf);
    }
    if (mln_lang_network_get_addr(&addr, ip, &port) < 0) {
        if (rc > 0) free(buf);
        goto out;
    }
    /*ip*/
    var.type = M_LANG_VAR_NORMAL;
    var.name = NULL;
    var.val = &val;
    var.inSet = NULL;
    var.prev = var.next = NULL;
    val.type = M_LANG_VAL_TYPE_STRING;
    val.ref = 1;
    mln_string_set(&tmp, ip);
    val.data.s = &tmp;
    if (mln_lang_var_setValue(udp->ctx, udp->ip, &var) < 0) {
        if (rc > 0) free(buf);
        goto out;
    }
    /*port*/
    var.type = M_LANG_VAR_NORMAL;
    var.name = NULL;
    var.val = &val;
    var.inSet = NULL;
    var.prev = var.next = NULL;
    val.data.i = port;
    val.type = M_LANG_VAL_TYPE_INT;
    val.ref = 1;
    if (mln_lang_var_setValue(udp->ctx, udp->port, &var) < 0) {
        if (rc > 0) free(buf);
        goto out;
    }
    /*data*/
    if (rc > 0) {
        mln_string_nSet(&tmp, buf, rc);
        retExp = mln_lang_retExp_createTmpString(udp->ctx->pool, &tmp, NULL);
        free(buf);
        if (retExp != NULL) mln_lang_ctx_setRetExp(udp->ctx, retExp);
    }
out:
    mln_event_set_fd(ev, fd, M_EV_CLR, M_EV_UNLIMITED, NULL, NULL);
    udp->recving = 0;
    mln_lang_ctx_continue(udp->ctx);
    mln_lang_ctx_udp_resource_remove(udp);
}

static void mln_lang_network_udp_timeout_handler(mln_event_t *ev, int fd, void *data)
{
    mln_lang_udp_t *udp = (mln_lang_udp_t *)data;
    mln_lang_retExp_t *retExp;
    mln_event_set_fd(ev, fd, M_EV_CLR, M_EV_UNLIMITED, NULL, NULL);
    retExp = mln_lang_retExp_createTmpNil(udp->ctx->pool, NULL);
    udp->recving = 0;
    ASSERT(!udp->sending);
    if (retExp != NULL) {
        mln_lang_ctx_setRetExp(udp->ctx, retExp);
    }
    mln_lang_ctx_continue(udp->ctx);
    mln_lang_ctx_udp_resource_remove(udp);
}

/*
 * udp components
 */
static mln_lang_udp_t *mln_lang_udp_new(mln_lang_t *lang, int fd)
{
    mln_lang_udp_t *lu;
    if ((lu = (mln_lang_udp_t *)malloc(sizeof(mln_lang_udp_t))) == NULL) {
        return NULL;
    }
    lu->lang = lang;
    lu->ctx = NULL;
    memset(&(lu->addr), 0, sizeof(struct sockaddr));
    lu->len = 0;
    lu->fd = fd;
    lu->timeout = 0;
    lu->data = NULL;
    lu->bufsize = 0;
    lu->sending = lu->recving = 0;
    lu->padding = 0;
    lu->ip = NULL;
    lu->port = NULL;
    lu->prev = lu->next = NULL;
    return lu;
}

static void mln_lang_udp_free(mln_lang_udp_t *lu)
{
    if (lu == NULL) return;
    if (lu->ctx != NULL) {
        mln_lang_ctx_udp_t *lcu = mln_lang_ctx_resource_fetch(lu->ctx, "udp");
        if (lcu != NULL) {
            mln_lang_udp_chain_del(&(lcu->head), &(lcu->tail), lu);
            mln_lang_ctx_continue(lu->ctx);
            lu->ctx = NULL;
            lu->sending = lu->recving = 0;
        }
    }
    mln_event_set_fd(lu->lang->ev, lu->fd, M_EV_CLR, M_EV_UNLIMITED, NULL, NULL);
    close(lu->fd);
    if (lu->data != NULL) mln_string_free(lu->data);
    free(lu);
}

static int mln_lang_udp_cmp(const mln_lang_udp_t *lu1, const mln_lang_udp_t *lu2)
{
    return lu1->fd - lu2->fd;
}

static mln_lang_ctx_udp_t *mln_lang_ctx_udp_new(mln_lang_ctx_t *ctx)
{
    mln_lang_ctx_udp_t *lcu;
    if ((lcu = (mln_lang_ctx_udp_t *)mln_alloc_m(ctx->pool, sizeof(mln_lang_ctx_udp_t))) == NULL) {
        return NULL;
    }
    lcu->ctx = ctx;
    lcu->head = lcu->tail = NULL;
    return lcu;
}

static void mln_lang_ctx_udp_free(mln_lang_ctx_udp_t *lcu)
{
    if (lcu == NULL) return;
    mln_lang_udp_t *lu;
    while ((lu = lcu->head) != NULL) {
        mln_lang_udp_chain_del(&(lcu->head), &(lcu->tail), lu);
        mln_event_set_fd(lu->lang->ev, lu->fd, M_EV_CLR, M_EV_UNLIMITED, NULL, NULL);
        lu->ctx = NULL;
    }
    mln_alloc_free(lcu);
}

static void mln_lang_ctx_udp_resource_add(mln_lang_ctx_t *ctx, mln_lang_udp_t *udp)
{
    mln_lang_ctx_udp_t *lcu = mln_lang_ctx_resource_fetch(ctx, "udp");
    ASSERT(lcu != NULL);
    mln_lang_udp_chain_add(&(lcu->head), &(lcu->tail), udp);
    udp->ctx = ctx;
}

static void mln_lang_ctx_udp_resource_remove(mln_lang_udp_t *udp)
{
    mln_lang_ctx_udp_t *lcu = mln_lang_ctx_resource_fetch(udp->ctx, "udp");
    ASSERT(lcu != NULL);
    mln_lang_udp_chain_del(&(lcu->head), &(lcu->tail), udp);
    udp->ctx = NULL;
}

