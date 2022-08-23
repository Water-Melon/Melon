
/*
 * Copyright (C) Niklaus F.Schen.
 */
#if defined(WIN32)
#include <ws2tcpip.h>
#include <winsock2.h>
#else
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#endif
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
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
MLN_CHAIN_FUNC_DECLARE(mln_lang_tcp, mln_lang_tcp_t, static inline void,);
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
static mln_lang_var_t *mln_lang_network_tcp_listen_process(mln_lang_ctx_t *ctx);
static int mln_lang_network_get_addr(const void *in_addr, char *ip, mln_u16_t *port);
static mln_lang_var_t *mln_lang_network_tcp_close_process(mln_lang_ctx_t *ctx);
static mln_lang_var_t *mln_lang_network_tcp_shutdown_process(mln_lang_ctx_t *ctx);
static void mln_lang_network_tcp_accept_handler(mln_event_t *ev, int fd, void *data);
static void mln_lang_network_tcp_recv_handler(mln_event_t *ev, int fd, void *data);
static void mln_lang_network_tcp_send_handler(mln_event_t *ev, int fd, void *data);
static void mln_lang_network_tcp_timeout_handler(mln_event_t *ev, int fd, void *data);
static mln_lang_var_t *mln_lang_network_tcp_connect_process(mln_lang_ctx_t *ctx);
static void mln_lang_network_tcp_connect_handler(mln_event_t *ev, int fd, void *data);
static void mln_lang_network_tcp_connect_timeout_handler(mln_event_t *ev, int fd, void *data);
static mln_lang_var_t *mln_lang_network_tcp_accept_process(mln_lang_ctx_t *ctx);
static mln_lang_var_t *mln_lang_network_tcp_send_process(mln_lang_ctx_t *ctx);
static mln_lang_var_t *mln_lang_network_tcp_recv_process(mln_lang_ctx_t *ctx);
/*udp*/
MLN_CHAIN_FUNC_DECLARE(mln_lang_udp, mln_lang_udp_t, static inline void,);
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
static mln_lang_var_t *mln_lang_network_udp_create_process(mln_lang_ctx_t *ctx);
static int mln_lang_network_udp_close(mln_lang_ctx_t *ctx);
static mln_lang_var_t *mln_lang_network_udp_close_process(mln_lang_ctx_t *ctx);
static int mln_lang_network_udp_send(mln_lang_ctx_t *ctx);
static mln_lang_var_t *mln_lang_network_udp_send_process(mln_lang_ctx_t *ctx);
static void mln_lang_network_udp_send_handler(mln_event_t *ev, int fd, void *data);
static int mln_lang_network_udp_recv(mln_lang_ctx_t *ctx);
static mln_lang_var_t *mln_lang_network_udp_recv_process(mln_lang_ctx_t *ctx);
static void mln_lang_network_udp_recv_handler(mln_event_t *ev, int fd, void *data);
static void mln_lang_network_udp_timeout_handler(mln_event_t *ev, int fd, void *data);

#if defined(WIN32)
/*
 * Note
 * code in this if is copied and modified from webrtc
 */

char* inet_ntop_v4(const void* src, char* dst, socklen_t size) {
  if (size < INET_ADDRSTRLEN) {
    return NULL;
  }
  const struct in_addr* as_in_addr = (const struct in_addr *)src;
  snprintf(dst, size, "%d.%d.%d.%d",
                      as_in_addr->S_un.S_un_b.s_b1,
                      as_in_addr->S_un.S_un_b.s_b2,
                      as_in_addr->S_un.S_un_b.s_b3,
                      as_in_addr->S_un.S_un_b.s_b4);
  return dst;
}
// Helper function for inet_ntop for IPv6 addresses.
char* inet_ntop_v6(const void* src, char* dst, socklen_t size) {
  if (size < INET6_ADDRSTRLEN) {
    return NULL;
  }
  const mln_u16_t * as_shorts = (const mln_u16_t *)src;
  int runpos[8];
  int current = 1;
  int max = 0;
  int maxpos = -1;
  int run_array_size = sizeof(runpos)/sizeof(int);
  // Run over the address marking runs of 0s.
  for (int i = 0; i < run_array_size; ++i) {
    if (as_shorts[i] == 0) {
      runpos[i] = current;
      if (current > max) {
        maxpos = i;
        max = current;
      }
      ++current;
    } else {
      runpos[i] = -1;
      current = 1;
    }
  }

  if (max > 0) {
    int tmpmax = maxpos;
    // Run back through, setting -1 for all but the longest run.
    for (int i = run_array_size - 1; i >= 0; i--) {
      if (i > tmpmax) {
        runpos[i] = -1;
      } else if (runpos[i] == -1) {
        // We're less than maxpos, we hit a -1, so the 'good' run is done.
        // Setting tmpmax -1 means all remaining positions get set to -1.
        tmpmax = -1;
      }
    }
  }

  char* cursor = dst;
  // Print IPv4 compatible and IPv4 mapped addresses using the IPv4 helper.
  // These addresses have an initial run of either eight zero-bytes followed
  // by 0xFFFF, or an initial run of ten zero-bytes.
  if (runpos[0] == 1 && (maxpos == 5 ||
                         (maxpos == 4 && as_shorts[5] == 0xFFFF))) {
    *cursor++ = ':';
    *cursor++ = ':';
    if (maxpos == 4) {
      cursor += snprintf(cursor, INET6_ADDRSTRLEN - 2, "ffff:");
    }
    const struct in_addr* as_v4 = (const struct in_addr *)(&as_shorts[6]);
    inet_ntop_v4(as_v4, cursor, INET6_ADDRSTRLEN - (cursor - dst));
  } else {
    for (int i = 0; i < run_array_size; ++i) {
      if (runpos[i] == -1) {
        cursor += snprintf(cursor,
                           INET6_ADDRSTRLEN - (cursor - dst),
                           "%x", ntohs(as_shorts[i]));
        if (i != 7 && runpos[i + 1] != 1) {
          *cursor++ = ':';
        }
      } else if (runpos[i] == 1) {
        // Entered the run; print the colons and skip the run.
        *cursor++ = ':';
        *cursor++ = ':';
        i += (max - 1);
      }
    }
  }
  return dst;
}

char *inet_ntop(int af, const void *src, char* dst, socklen_t size){
  if (!src || !dst) {
    return NULL;
  }
  switch (af) {
    case AF_INET: {
      return inet_ntop_v4(src, dst, size);
    }
    case AF_INET6: {
      return inet_ntop_v6(src, dst, size);
    }
  }
  return NULL;
}
#endif

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
#if !defined(WIN32)
        signal(SIGPIPE, SIG_IGN);
#endif
        struct mln_rbtree_attr rbattr;
        rbattr.pool = ctx->lang->pool;
        rbattr.pool_alloc = (rbtree_pool_alloc_handler)mln_alloc_m;
        rbattr.pool_free = (rbtree_pool_free_handler)mln_alloc_free;
        rbattr.cmp = (rbtree_cmp)mln_lang_tcp_cmp;
        rbattr.data_free = (rbtree_free_data)mln_lang_tcp_free;
        rbattr.cache = 0;
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
        rbattr.pool = ctx->lang->pool;
        rbattr.pool_alloc = (rbtree_pool_alloc_handler)mln_alloc_m;
        rbattr.pool_free = (rbtree_pool_free_handler)mln_alloc_free;
        rbattr.cmp = (rbtree_cmp)mln_lang_udp_cmp;
        rbattr.data_free = (rbtree_free_data)mln_lang_udp_free;
        rbattr.cache = 0;
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
    mln_string_t funcname = mln_string("mln_tcp_listen");

    if ((func = mln_lang_func_detail_new(ctx, M_FUNC_INTERNAL, mln_lang_network_tcp_listen_process, NULL, NULL)) == NULL) {
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

static mln_lang_var_t *mln_lang_network_tcp_listen_process(mln_lang_ctx_t *ctx)
{
    mln_lang_val_t *val1, *val2;
    mln_lang_var_t *ret_var;
    mln_string_t v1 = mln_string("host"), v2 = mln_string("service");
    mln_lang_symbol_node_t *sym;
    struct addrinfo addr, *res = NULL;
    int fd, opt = 1;
    mln_u16_t port;
    char ip[128] = {0}, host[128] = {0}, service[64] = {0};
    mln_lang_tcp_t *tcp;

    if ((sym = mln_lang_symbol_node_search(ctx, &v1, 1)) == NULL) {
        ASSERT(0);
        mln_lang_errmsg(ctx, "Argument1 missing.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);
    if (mln_lang_var_val_type_get(sym->data.var) != M_LANG_VAL_TYPE_STRING) {
        mln_lang_errmsg(ctx, "Invalid type of argument 1.");
        return NULL;
    }
    val1 = sym->data.var->val;
    if (val1->data.s->len > sizeof(host)-1) {
        mln_lang_errmsg(ctx, "Invalid host.");
        return NULL;
    }
    memcpy(host, val1->data.s->data, val1->data.s->len);

    if ((sym = mln_lang_symbol_node_search(ctx, &v2, 1)) == NULL) {
        ASSERT(0);
        mln_lang_errmsg(ctx, "Argument2 missing.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);
    if (mln_lang_var_val_type_get(sym->data.var) != M_LANG_VAL_TYPE_STRING) {
        mln_lang_errmsg(ctx, "Invalid type of argument 2.");
        return NULL;
    }
    val2 = sym->data.var->val;
    if (val2->data.s->len > sizeof(service)-1) {
        mln_lang_errmsg(ctx, "Invalid service.");
        return NULL;
    }
    memcpy(service, val2->data.s->data, val2->data.s->len);

    memset(&addr, 0, sizeof(addr));
    addr.ai_flags = AI_PASSIVE;
    addr.ai_family = AF_UNSPEC;
    addr.ai_socktype = SOCK_STREAM;
    addr.ai_protocol = IPPROTO_IP;
    if (getaddrinfo(host, service, &addr, &res) != 0 || res == NULL) {
        if ((ret_var = mln_lang_var_create_false(ctx, NULL)) == NULL) {
            mln_lang_errmsg(ctx, "No memory.");
            return NULL;
        }
        return ret_var;
    }
    if ((fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) < 0) {
        freeaddrinfo(res);
        if ((ret_var = mln_lang_var_create_false(ctx, NULL)) == NULL) {
            mln_lang_errmsg(ctx, "No memory.");
            return NULL;
        }
        return ret_var;
    }
    if (mln_lang_network_get_addr(res->ai_addr, ip, &port) < 0) {
        mln_socket_close(fd);
        freeaddrinfo(res);
        if ((ret_var = mln_lang_var_create_false(ctx, NULL)) == NULL) {
            mln_lang_errmsg(ctx, "No memory.");
            return NULL;
        }
        return ret_var;
    }
    if ((tcp = mln_lang_tcp_new(ctx->lang, fd, ip, port)) == NULL) {
        mln_socket_close(fd);
        freeaddrinfo(res);
        mln_lang_errmsg(ctx, "No memory.");
        return NULL;
    }
#if defined(WIN32)
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char *)(&opt), sizeof(opt)) < 0) {
#else
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
#endif
        mln_lang_tcp_free(tcp);
        freeaddrinfo(res);
        if ((ret_var = mln_lang_var_create_false(ctx, NULL)) == NULL) {
            mln_lang_errmsg(ctx, "No memory.");
            return NULL;
        }
        return ret_var;
    }
    if (bind(fd, res->ai_addr, res->ai_addrlen) < 0) {
        mln_lang_tcp_free(tcp);
        freeaddrinfo(res);
        if ((ret_var = mln_lang_var_create_false(ctx, NULL)) == NULL) {
            mln_lang_errmsg(ctx, "No memory.");
            return NULL;
        }
        return ret_var;
    }
    freeaddrinfo(res);
    if (listen(fd, 32767) < 0) {
        mln_lang_tcp_free(tcp);
        if ((ret_var = mln_lang_var_create_false(ctx, NULL)) == NULL) {
            mln_lang_errmsg(ctx, "No memory.");
            return NULL;
        }
        return ret_var;
    }
    if ((ret_var = mln_lang_var_create_int(ctx, fd, NULL)) == NULL) {
        mln_lang_tcp_free(tcp);
        mln_lang_errmsg(ctx, "No memory.");
        return NULL;
    }
    if (mln_lang_network_tcp_resource_add(ctx->lang, tcp) < 0) {
        mln_lang_var_free(ret_var);
        mln_lang_tcp_free(tcp);
        mln_lang_errmsg(ctx, "No memory.");
        return NULL;
    }
    return ret_var;
}

static int mln_lang_network_get_addr(const void *in_addr, char *ip, mln_u16_t *port)
{
    char *numeric_addr = NULL;
    char addr_buf[128] = {0};
    struct sockaddr *saddr = (struct sockaddr *)in_addr;

    if (AF_INET == saddr->sa_family) {
        struct sockaddr_in *addr = (struct sockaddr_in *)in_addr;
        numeric_addr = (char *)(&addr->sin_addr);
        *port = ntohs(addr->sin_port);
    } else if (AF_INET6 == saddr->sa_family) {
        struct sockaddr_in6 *addr = (struct sockaddr_in6 *)in_addr;
        numeric_addr = (char *)(&addr->sin6_addr);
        *port = ntohs(addr->sin6_port);
    } else {
        return -1;
    }
    if (inet_ntop(saddr->sa_family, numeric_addr, addr_buf, sizeof(addr_buf)) == NULL)
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
    mln_string_t funcname = mln_string("mln_tcp_accept");

    if ((func = mln_lang_func_detail_new(ctx, M_FUNC_INTERNAL, mln_lang_network_tcp_accept_process, NULL, NULL)) == NULL) {
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

static mln_lang_var_t *mln_lang_network_tcp_accept_process(mln_lang_ctx_t *ctx)
{
    mln_lang_var_t *ret_var;
    mln_string_t v1 = mln_string("fd"), v2 = mln_string("timeout");
    mln_lang_symbol_node_t *sym;
    mln_lang_tcp_t *tcp;
    int fd, timeout, type;

    if ((sym = mln_lang_symbol_node_search(ctx, &v1, 1)) == NULL) {
        ASSERT(0);
        mln_lang_errmsg(ctx, "Argument missing.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);
    if (mln_lang_var_val_type_get(sym->data.var) != M_LANG_VAL_TYPE_INT) {
        mln_lang_errmsg(ctx, "Invalid type of argument 1.");
        return NULL;
    }
    fd = sym->data.var->val->data.i;

    if ((sym = mln_lang_symbol_node_search(ctx, &v2, 1)) == NULL) {
        ASSERT(0);
        mln_lang_errmsg(ctx, "Argument missing.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);
    type = mln_lang_var_val_type_get(sym->data.var);
    if (type == M_LANG_VAL_TYPE_NIL) {
        timeout = M_EV_UNLIMITED;
    } else if (type == M_LANG_VAL_TYPE_INT && sym->data.var->val->data.i >= 0) {
        timeout = sym->data.var->val->data.i;
    } else {
        mln_lang_errmsg(ctx, "Invalid type of argument 2.");
        return NULL;
    }

    if ((ret_var = mln_lang_var_create_false(ctx, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return NULL;
    }
    if ((tcp = mln_lang_network_tcp_resource_fetch(ctx->lang, fd)) == NULL) {
        return ret_var;
    }
    if (tcp->recving) {
        mln_lang_errmsg(ctx, "Socket used in other script task.");
        return ret_var;
    }
    if (tcp->recv_closed) {
        mln_lang_errmsg(ctx, "Socket recv shutdown.");
        return ret_var;
    }
    if (tcp->sending) {
        mln_lang_errmsg(ctx, "Listen socket sending data.");
        mln_lang_var_free(ret_var);
        return NULL;
    }
    tcp->timeout = timeout;
    if (mln_event_set_fd(ctx->lang->ev, fd, M_EV_RECV|M_EV_NONBLOCK|M_EV_ONESHOT,timeout, tcp, mln_lang_network_tcp_accept_handler) < 0) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_var_free(ret_var);
        return NULL;
    }
    if (timeout >= 0) {
        mln_event_set_fd_timeout_handler(ctx->lang->ev, fd, tcp, mln_lang_network_tcp_timeout_handler);
    }
    tcp->recving = 1;
    mln_lang_ctx_tcp_resource_add(ctx, tcp);
    mln_lang_ctx_suspend(ctx);

    return ret_var;
}

static int mln_lang_network_tcp_recv(mln_lang_ctx_t *ctx)
{
    mln_lang_val_t *val;
    mln_lang_var_t *var;
    mln_lang_func_detail_t *func;
    mln_string_t v1 = mln_string("fd"), v2 = mln_string("timeout");
    mln_string_t funcname = mln_string("mln_tcp_recv");

    if ((func = mln_lang_func_detail_new(ctx, M_FUNC_INTERNAL, mln_lang_network_tcp_recv_process, NULL, NULL)) == NULL) {
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

static mln_lang_var_t *mln_lang_network_tcp_recv_process(mln_lang_ctx_t *ctx)
{
    mln_lang_var_t *ret_var;
    mln_string_t v1 = mln_string("fd"), v2 = mln_string("timeout");
    mln_lang_symbol_node_t *sym;
    mln_lang_tcp_t *tcp;
    int fd, timeout, type;

    if ((sym = mln_lang_symbol_node_search(ctx, &v1, 1)) == NULL) {
        ASSERT(0);
        mln_lang_errmsg(ctx, "Argument missing.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);
    if (mln_lang_var_val_type_get(sym->data.var) != M_LANG_VAL_TYPE_INT) {
        mln_lang_errmsg(ctx, "Invalid type of argument 1.");
        return NULL;
    }
    fd = sym->data.var->val->data.i;

    if ((sym = mln_lang_symbol_node_search(ctx, &v2, 1)) == NULL) {
        ASSERT(0);
        mln_lang_errmsg(ctx, "Argument missing.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);
    type = mln_lang_var_val_type_get(sym->data.var);
    if (type == M_LANG_VAL_TYPE_NIL) {
        timeout = M_EV_UNLIMITED;
    } else if (type == M_LANG_VAL_TYPE_INT && sym->data.var->val->data.i >= 0) {
        timeout = sym->data.var->val->data.i;
    } else {
        mln_lang_errmsg(ctx, "Invalid type of argument 2.");
        return NULL;
    }

    if ((ret_var = mln_lang_var_create_false(ctx, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return NULL;
    }
    if ((tcp = mln_lang_network_tcp_resource_fetch(ctx->lang, fd)) == NULL) {
        return ret_var;
    }
    if (tcp->recving || tcp->sending) {
        mln_lang_errmsg(ctx, "Socket used in other script task.");
        return ret_var;
    }
    if (tcp->recv_closed) {
        mln_lang_errmsg(ctx, "Socket recv shutdown.");
        return ret_var;
    }
    if (mln_event_set_fd(ctx->lang->ev, fd, M_EV_RECV|M_EV_NONBLOCK|M_EV_ONESHOT, timeout, tcp, mln_lang_network_tcp_recv_handler) < 0) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_var_free(ret_var);
        return NULL;
    }
    if (tcp->timeout != M_EV_UNLIMITED)
        mln_event_set_fd_timeout_handler(tcp->lang->ev, fd, tcp, mln_lang_network_tcp_timeout_handler);

    tcp->recving = 1;
    mln_lang_ctx_tcp_resource_add(ctx, tcp);
    mln_lang_ctx_suspend(ctx);

    return ret_var;
}

static int mln_lang_network_tcp_send(mln_lang_ctx_t *ctx)
{
    mln_lang_val_t *val;
    mln_lang_var_t *var;
    mln_lang_func_detail_t *func;
    mln_string_t v1 = mln_string("fd"), v2 = mln_string("data");
    mln_string_t funcname = mln_string("mln_tcp_send");

    if ((func = mln_lang_func_detail_new(ctx, M_FUNC_INTERNAL, mln_lang_network_tcp_send_process, NULL, NULL)) == NULL) {
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

static mln_lang_var_t *mln_lang_network_tcp_send_process(mln_lang_ctx_t *ctx)
{
    mln_lang_var_t *ret_var;
    mln_string_t v1 = mln_string("fd"), v2 = mln_string("data");
    mln_lang_symbol_node_t *sym;
    mln_lang_tcp_t *tcp;
    int fd;
    mln_string_t *data;
    mln_chain_t *c;
    mln_buf_t *b;
    mln_u8ptr_t buf;

    if ((sym = mln_lang_symbol_node_search(ctx, &v1, 1)) == NULL) {
        ASSERT(0);
        mln_lang_errmsg(ctx, "Argument missing.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);
    if (mln_lang_var_val_type_get(sym->data.var) != M_LANG_VAL_TYPE_INT) {
        mln_lang_errmsg(ctx, "Invalid type of argument 1.");
        return NULL;
    }
    fd = sym->data.var->val->data.i;

    if ((sym = mln_lang_symbol_node_search(ctx, &v2, 1)) == NULL) {
        ASSERT(0);
        mln_lang_errmsg(ctx, "Argument missing.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);
    if (mln_lang_var_val_type_get(sym->data.var) != M_LANG_VAL_TYPE_STRING) {
        mln_lang_errmsg(ctx, "Invalid type of argument 2.");
        return NULL;
    }
    data = sym->data.var->val->data.s;

    if ((ret_var = mln_lang_var_create_false(ctx, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return NULL;
    }
    if ((tcp = mln_lang_network_tcp_resource_fetch(ctx->lang, fd)) == NULL) {
        return ret_var;
    }
    if (tcp->recving || tcp->sending) {
        mln_lang_errmsg(ctx, "Socket used in other script task.");
        return ret_var;
    }
    if (tcp->send_closed) {
        mln_lang_errmsg(ctx, "Socket send shutdown.");
        return ret_var;
    }
    if (mln_event_set_fd(ctx->lang->ev, fd, M_EV_SEND|M_EV_NONBLOCK|M_EV_ONESHOT, M_EV_UNLIMITED, tcp, mln_lang_network_tcp_send_handler) < 0) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_var_free(ret_var);
        return NULL;
    }
    if ((c = mln_chain_new(mln_tcp_conn_get_pool(&(tcp->conn)))) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_event_set_fd(ctx->lang->ev, fd, M_EV_CLR, M_EV_UNLIMITED, NULL, NULL);
        mln_lang_var_free(ret_var);
        return NULL;
    }
    if ((b = mln_buf_new(mln_tcp_conn_get_pool(&(tcp->conn)))) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_chain_pool_release(c);
        mln_event_set_fd(ctx->lang->ev, fd, M_EV_CLR, M_EV_UNLIMITED, NULL, NULL);
        mln_lang_var_free(ret_var);
        return NULL;
    }
    c->buf = b;
    if ((buf = (mln_u8ptr_t)mln_alloc_m(mln_tcp_conn_get_pool(&(tcp->conn)), data->len)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_chain_pool_release(c);
        mln_event_set_fd(ctx->lang->ev, fd, M_EV_CLR, M_EV_UNLIMITED, NULL, NULL);
        mln_lang_var_free(ret_var);
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

    return ret_var;
}

static int mln_lang_network_tcp_connect(mln_lang_ctx_t *ctx)
{
    mln_lang_val_t *val;
    mln_lang_var_t *var;
    mln_lang_func_detail_t *func;
    mln_string_t v1 = mln_string("host"), v2 = mln_string("service"), v3 = mln_string("timeout");
    mln_string_t funcname = mln_string("mln_tcp_connect");

    if ((func = mln_lang_func_detail_new(ctx, M_FUNC_INTERNAL, mln_lang_network_tcp_connect_process, NULL, NULL)) == NULL) {
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

static mln_lang_var_t *mln_lang_network_tcp_connect_process(mln_lang_ctx_t *ctx)
{
    mln_lang_val_t *val;
    mln_lang_var_t *ret_var;
    mln_string_t v1 = mln_string("host"), v2 = mln_string("service"), v3 = mln_string("timeout");
    mln_lang_symbol_node_t *sym;
    mln_lang_tcp_t *tcp;
    struct addrinfo addr, *res = NULL;
    int fd, type, timeout;
    mln_u16_t port;
    char host[128] = {0}, service[32] = {0}, ip[128] = {0};

    if ((sym = mln_lang_symbol_node_search(ctx, &v3, 1)) == NULL) {
        ASSERT(0);
        mln_lang_errmsg(ctx, "Argument missing.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);
    type = mln_lang_var_val_type_get(sym->data.var);
    if (type == M_LANG_VAL_TYPE_NIL) {
        timeout = M_EV_UNLIMITED;
    } else if (type == M_LANG_VAL_TYPE_INT && sym->data.var->val->data.i >= 0) {
        timeout = sym->data.var->val->data.i;
    } else {
        mln_lang_errmsg(ctx, "Invalid type of argument 3.");
        return NULL;
    }

    if ((sym = mln_lang_symbol_node_search(ctx, &v1, 1)) == NULL) {
        ASSERT(0);
        mln_lang_errmsg(ctx, "Argument missing.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);
    if (mln_lang_var_val_type_get(sym->data.var) != M_LANG_VAL_TYPE_STRING) {
        mln_lang_errmsg(ctx, "Invalid type of argument 1.");
        return NULL;
    }
    val = sym->data.var->val;
    if (val->data.s->len > sizeof(host)-1) {
        mln_lang_errmsg(ctx, "Invalid host.");
        return NULL;
    }
    memcpy(host, val->data.s->data, val->data.s->len);

    if ((sym = mln_lang_symbol_node_search(ctx, &v2, 1)) == NULL) {
        ASSERT(0);
        mln_lang_errmsg(ctx, "Argument missing.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);
    if (mln_lang_var_val_type_get(sym->data.var) != M_LANG_VAL_TYPE_STRING) {
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
        if ((ret_var = mln_lang_var_create_false(ctx, NULL)) == NULL) {
            mln_lang_errmsg(ctx, "No memory.");
            return NULL;
        }
        return ret_var;
    }
    if ((fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) < 0) {
        freeaddrinfo(res);
        if ((ret_var = mln_lang_var_create_false(ctx, NULL)) == NULL) {
            mln_lang_errmsg(ctx, "No memory.");
            return NULL;
        }
        return ret_var;
    }
    if (mln_lang_network_get_addr(res->ai_addr, ip, &port) < 0) {
        mln_socket_close(fd);
        freeaddrinfo(res);
        if ((ret_var = mln_lang_var_create_false(ctx, NULL)) == NULL) {
            mln_lang_errmsg(ctx, "No memory.");
            return NULL;
        }
        return ret_var;
    }
    if ((tcp = mln_lang_tcp_new(ctx->lang, fd, ip, port)) == NULL) {
        mln_socket_close(fd);
        freeaddrinfo(res);
        mln_lang_errmsg(ctx, "No memory.");
        return NULL;
    }
    tcp->connect_timeout = timeout;
    /*
     * M_EV_ERROR must be set, because epoll only trigger EPOLLERROR in sometime
     * while EPOLLOUT and EPOLLIN will not be triggered.
     */
    if (mln_event_set_fd(ctx->lang->ev, \
                         fd, \
                         M_EV_SEND|M_EV_ERROR|M_EV_NONBLOCK|M_EV_ONESHOT, \
                         timeout, \
                         tcp, \
                         mln_lang_network_tcp_connect_handler) < 0)
    {
        mln_lang_tcp_free(tcp);
        freeaddrinfo(res);
        mln_lang_errmsg(ctx, "No memory.");
        return NULL;
    }
#if defined(WIN32)
    if (connect(fd, res->ai_addr, res->ai_addrlen) == SOCKET_ERROR && WSAGetLastError() != WSAEWOULDBLOCK) {
#else
    if (connect(fd, res->ai_addr, res->ai_addrlen) < 0 && errno != EINPROGRESS) {
#endif
        mln_event_set_fd(ctx->lang->ev, fd, M_EV_CLR, M_EV_UNLIMITED, NULL, NULL);
        mln_lang_tcp_free(tcp);
        freeaddrinfo(res);
        if ((ret_var = mln_lang_var_create_false(ctx, NULL)) == NULL) {
            mln_lang_errmsg(ctx, "No memory.");
            return NULL;
        }
        return ret_var;
    }
    freeaddrinfo(res);
    if ((ret_var = mln_lang_var_create_false(ctx, NULL)) == NULL) {
        mln_event_set_fd(ctx->lang->ev, fd, M_EV_CLR, M_EV_UNLIMITED, NULL, NULL);
        mln_lang_tcp_free(tcp);
        mln_lang_errmsg(ctx, "No memory.");
        return NULL;
    }
    if (mln_lang_network_tcp_resource_add(ctx->lang, tcp) < 0) {
        mln_lang_var_free(ret_var);
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

    return ret_var;
}

static int mln_lang_network_tcp_shutdown(mln_lang_ctx_t *ctx)
{
    mln_lang_val_t *val;
    mln_lang_var_t *var;
    mln_lang_func_detail_t *func;
    mln_string_t v1 = mln_string("fd"), v2 = mln_string("mode");
    mln_string_t funcname = mln_string("mln_tcp_shutdown");

    if ((func = mln_lang_func_detail_new(ctx, M_FUNC_INTERNAL, mln_lang_network_tcp_shutdown_process, NULL, NULL)) == NULL) {
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

static mln_lang_var_t *mln_lang_network_tcp_shutdown_process(mln_lang_ctx_t *ctx)
{
    mln_lang_val_t *val;
    mln_lang_var_t *ret_var;
    mln_string_t v1 = mln_string("fd"), v2 = mln_string("mode");
    mln_lang_symbol_node_t *sym;
    mln_lang_tcp_t *tcp;
    mln_string_t send_mode = mln_string("send");
    mln_string_t recv_mode = mln_string("recv");
    int fd;

    if ((sym = mln_lang_symbol_node_search(ctx, &v1, 1)) == NULL) {
        ASSERT(0);
        mln_lang_errmsg(ctx, "Argument missing.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);
    if (mln_lang_var_val_type_get(sym->data.var) != M_LANG_VAL_TYPE_INT) {
        mln_lang_errmsg(ctx, "Invalid type of argument 1.");
        return NULL;
    }
    fd = sym->data.var->val->data.i;
    tcp = mln_lang_network_tcp_resource_fetch(ctx->lang, fd);
    if (tcp == NULL) {
        mln_lang_errmsg(ctx, "socket not existing.");
        return NULL;
    }

    if ((sym = mln_lang_symbol_node_search(ctx, &v2, 1)) == NULL) {
        ASSERT(0);
        mln_lang_errmsg(ctx, "Argument missing.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);
    if (mln_lang_var_val_type_get(sym->data.var) != M_LANG_VAL_TYPE_STRING) {
        mln_lang_errmsg(ctx, "Invalid type of argument 2.");
        return NULL;
    }
    val = sym->data.var->val;

    if ((ret_var = mln_lang_var_create_nil(ctx, NULL)) == NULL) {
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
#if defined (WIN32)
            shutdown(fd, SD_SEND);
#else
            shutdown(fd, SHUT_WR);
#endif
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
#if defined (WIN32)
            shutdown(fd, SD_RECEIVE);
#else
            shutdown(fd, SHUT_RD);
#endif
        }
    } else {
        mln_lang_errmsg(ctx, "Invalid type of argument 2.");
        mln_lang_var_free(ret_var);
        return NULL;
    }

    return ret_var;
}

static int mln_lang_network_tcp_close(mln_lang_ctx_t *ctx)
{
    mln_lang_val_t *val;
    mln_lang_var_t *var;
    mln_lang_func_detail_t *func;
    mln_string_t v1 = mln_string("fd");
    mln_string_t funcname = mln_string("mln_tcp_close");

    if ((func = mln_lang_func_detail_new(ctx, M_FUNC_INTERNAL, mln_lang_network_tcp_close_process, NULL, NULL)) == NULL) {
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

static mln_lang_var_t *mln_lang_network_tcp_close_process(mln_lang_ctx_t *ctx)
{
    mln_lang_var_t *ret_var;
    mln_string_t v1 = mln_string("fd");
    mln_lang_symbol_node_t *sym;

    if ((sym = mln_lang_symbol_node_search(ctx, &v1, 1)) == NULL) {
        ASSERT(0);
        mln_lang_errmsg(ctx, "Argument missing.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);
    if (mln_lang_var_val_type_get(sym->data.var) != M_LANG_VAL_TYPE_INT) {
        mln_lang_errmsg(ctx, "Invalid type of argument 1.");
        return NULL;
    }
    if ((ret_var = mln_lang_var_create_nil(ctx, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return NULL;
    }
    mln_lang_network_tcp_resource_remove(ctx->lang, sym->data.var->val->data.i);
    return ret_var;
}

static void mln_lang_network_tcp_connect_handler(mln_event_t *ev, int fd, void *data)
{
    mln_lang_tcp_t *tcp = (mln_lang_tcp_t *)data;
    mln_lang_ctx_t *ctx = tcp->ctx;
    int err = 0, failed = 0;
    socklen_t len = sizeof(err);
    mln_lang_var_t *ret_var;

#if defined (WIN32)
    if (getsockopt(fd, SOL_SOCKET, SO_ERROR, (char *)&err, &len) >= 0) {
#else
    if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &len) >= 0) {
#endif
        if (err) {
            if (err == EINPROGRESS && (++tcp->retry <= MLN_LANG_NETWORK_TCP_CONNECT_RETRY)) {
                mln_event_set_fd(ev, fd, M_EV_SEND|M_EV_ERROR|M_EV_NONBLOCK|M_EV_ONESHOT, tcp->connect_timeout, tcp, mln_lang_network_tcp_connect_handler);
                if (tcp->connect_timeout != M_EV_UNLIMITED) {
                    mln_event_set_fd_timeout_handler(ev, fd, tcp, mln_lang_network_tcp_connect_timeout_handler);
                }
                return;
            } else {
                failed = 1;
            }
        } else {
            if ((ret_var = mln_lang_var_create_int(tcp->ctx, fd, NULL)) != NULL) {
                mln_lang_ctx_set_ret_var(tcp->ctx, ret_var);
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
    mln_lang_var_t *ret_var;
    mln_lang_tcp_t *tcp = (mln_lang_tcp_t *)data;
    mln_lang_ctx_t *ctx = tcp->ctx;

    mln_event_set_fd(ev, fd, M_EV_CLR, M_EV_UNLIMITED, NULL, NULL);
    tcp->sending = 0;
    ASSERT(!tcp->recving);
    tcp->timeout = M_EV_UNLIMITED;
    mln_lang_ctx_tcp_resource_remove(tcp);
    mln_lang_network_tcp_resource_remove(ctx->lang, fd);
    if ((ret_var = mln_lang_var_create_nil(ctx, NULL)) != NULL) {
        mln_lang_ctx_set_ret_var(ctx, ret_var);
    }
    mln_lang_ctx_continue(ctx);
}

static void mln_lang_network_tcp_accept_handler(mln_event_t *ev, int fd, void *data)
{
    mln_lang_tcp_t *tcp = (mln_lang_tcp_t *)data, *newtcp;
    int connfd;
    mln_u8_t addr[sizeof(struct sockaddr) >= sizeof(struct sockaddr_in6)? \
                  sizeof(struct sockaddr): \
                  sizeof(struct sockaddr_in6)] = {0};
    char ip[128] = {0};
    mln_u16_t port;
    socklen_t len;
    mln_lang_var_t *ret_var;

    len = sizeof(addr);
    if ((connfd = accept(fd, (struct sockaddr *)addr, &len)) < 0) {
        if (errno == EINTR || errno == ECONNABORTED || errno == EAGAIN) {
            mln_event_set_fd(ev, fd, M_EV_RECV|M_EV_NONBLOCK|M_EV_ONESHOT, tcp->timeout, tcp, mln_lang_network_tcp_accept_handler);
            if (tcp->timeout >= 0)
                mln_event_set_fd_timeout_handler(ev, fd, tcp, mln_lang_network_tcp_timeout_handler);
            return;
        }
    } else {
        if (mln_lang_network_get_addr(addr, ip, &port) < 0) {
            mln_socket_close(connfd);
        } else {
            if ((newtcp = mln_lang_tcp_new(tcp->lang, connfd, ip, port)) == NULL) {
                mln_socket_close(connfd);
            } else {
                if ((ret_var = mln_lang_var_create_int(tcp->ctx, connfd, NULL)) == NULL) {
                    mln_lang_tcp_free(newtcp);
                } else {
                    if (mln_lang_network_tcp_resource_add(tcp->lang, newtcp) < 0) {
                        mln_lang_var_free(ret_var);
                        mln_lang_tcp_free(newtcp);
                    } else {
                        mln_lang_ctx_set_ret_var(tcp->ctx, ret_var);
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
    int rc = mln_tcp_conn_recv(&(tcp->conn), M_C_TYPE_MEMORY);
    if (rc == M_C_ERROR) {
        /* do nothing */
    } else if (rc == M_C_CLOSED && mln_tcp_conn_get_head(&(tcp->conn), M_C_RECV) == NULL) {
        mln_lang_var_t *ret_var = mln_lang_var_create_true(tcp->ctx, NULL);
        if (ret_var == NULL) {
            mln_event_set_fd(ev, fd, M_EV_SEND|M_EV_NONBLOCK|M_EV_ONESHOT, M_EV_UNLIMITED, tcp, mln_lang_network_tcp_recv_handler);
            return;
        }
        mln_lang_ctx_set_ret_var(tcp->ctx, ret_var);
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
        mln_string_nset(&tmp, buf, size);
        mln_lang_var_t *ret_var = mln_lang_var_create_string(tcp->ctx, &tmp, NULL);
        free(buf);
        if (ret_var == NULL) {
            mln_event_set_fd(ev, fd, M_EV_RECV|M_EV_NONBLOCK|M_EV_ONESHOT, M_EV_UNLIMITED, tcp, mln_lang_network_tcp_recv_handler);
            return;
        }
        mln_lang_ctx_set_ret_var(tcp->ctx, ret_var);
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
            mln_lang_var_t *ret_var = mln_lang_var_create_true(tcp->ctx, NULL);
            if (ret_var != NULL) mln_lang_ctx_set_ret_var(tcp->ctx, ret_var);
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
    mln_lang_var_t *ret_var;
    mln_event_set_fd(ev, fd, M_EV_CLR, M_EV_UNLIMITED, NULL, NULL);
    ret_var = mln_lang_var_create_nil(tcp->ctx, NULL);
    tcp->recving = 0;
    ASSERT(!tcp->sending);
    if (ret_var != NULL) {
        mln_lang_ctx_set_ret_var(tcp->ctx, ret_var);
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
    mln_socket_close(mln_tcp_conn_get_fd(&(lt->conn)));
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
    mln_string_t funcname = mln_string("mln_udp_create");

    if ((func = mln_lang_func_detail_new(ctx, M_FUNC_INTERNAL, mln_lang_network_udp_create_process, NULL, NULL)) == NULL) {
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

static mln_lang_var_t *mln_lang_network_udp_create_process(mln_lang_ctx_t *ctx)
{
    mln_lang_val_t *val1, *val2;
    mln_lang_var_t *ret_var;
    mln_string_t v1 = mln_string("host"), v2 = mln_string("service");
    mln_lang_symbol_node_t *sym;
    struct addrinfo addr, *res = NULL;
    int fd, opt = 1, type, notbind = 0;
    char host[128] = {0}, service[64] = {0};
    mln_lang_udp_t *udp;

    if ((sym = mln_lang_symbol_node_search(ctx, &v1, 1)) == NULL) {
        ASSERT(0);
        mln_lang_errmsg(ctx, "Argument missing.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);
    type = mln_lang_var_val_type_get(sym->data.var);
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

    if ((sym = mln_lang_symbol_node_search(ctx, &v2, 1)) == NULL) {
        ASSERT(0);
        mln_lang_errmsg(ctx, "Argument missing.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);
    type = mln_lang_var_val_type_get(sym->data.var);
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
            if ((ret_var = mln_lang_var_create_false(ctx, NULL)) == NULL) {
                mln_lang_errmsg(ctx, "No memory.");
                return NULL;
            }
            return ret_var;
        }
        if ((udp = mln_lang_udp_new(ctx->lang, fd)) == NULL) {
            mln_socket_close(fd);
            mln_lang_errmsg(ctx, "No memory.");
            return NULL;
        }
    } else {
        memset(&addr, 0, sizeof(addr));
        addr.ai_flags = AI_PASSIVE;
        addr.ai_family = AF_UNSPEC;
        addr.ai_socktype = SOCK_DGRAM;
        addr.ai_protocol = IPPROTO_IP;
        if (getaddrinfo(host, service, &addr, &res) != 0 || res == NULL) {
            if ((ret_var = mln_lang_var_create_false(ctx, NULL)) == NULL) {
                mln_lang_errmsg(ctx, "No memory.");
                return NULL;
            }
            return ret_var;
        }
        if ((fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) < 0) {
            freeaddrinfo(res);
            if ((ret_var = mln_lang_var_create_false(ctx, NULL)) == NULL) {
                mln_lang_errmsg(ctx, "No memory.");
                return NULL;
            }
            return ret_var;
        }
#if defined (WIN32)
        {
            u_long opt = 1;
            ioctlsocket(fd, FIONBIO, &opt);
        }
#endif
        if ((udp = mln_lang_udp_new(ctx->lang, fd)) == NULL) {
            mln_socket_close(fd);
            freeaddrinfo(res);
            mln_lang_errmsg(ctx, "No memory.");
            return NULL;
        }
#if defined (WIN32)
        if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0) {
#else
        if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
#endif
            mln_lang_udp_free(udp);
            freeaddrinfo(res);
            if ((ret_var = mln_lang_var_create_false(ctx, NULL)) == NULL) {
                mln_lang_errmsg(ctx, "No memory.");
                return NULL;
            }
            return ret_var;
        }
        if (bind(fd, res->ai_addr, res->ai_addrlen) < 0) {
            mln_lang_udp_free(udp);
            freeaddrinfo(res);
            if ((ret_var = mln_lang_var_create_false(ctx, NULL)) == NULL) {
                mln_lang_errmsg(ctx, "No memory.");
                return NULL;
            }
            return ret_var;
        }
        freeaddrinfo(res);
    }
    if ((ret_var = mln_lang_var_create_int(ctx, fd, NULL)) == NULL) {
        mln_lang_udp_free(udp);
        mln_lang_errmsg(ctx, "No memory.");
        return NULL;
    }
    if (mln_lang_network_udp_resource_add(ctx->lang, udp) < 0) {
        mln_lang_var_free(ret_var);
        mln_lang_udp_free(udp);
        mln_lang_errmsg(ctx, "No memory.");
        return NULL;
    }
    return ret_var;
}

static int mln_lang_network_udp_close(mln_lang_ctx_t *ctx)
{
    mln_lang_val_t *val;
    mln_lang_var_t *var;
    mln_lang_func_detail_t *func;
    mln_string_t v1 = mln_string("fd");
    mln_string_t funcname = mln_string("mln_udp_close");

    if ((func = mln_lang_func_detail_new(ctx, M_FUNC_INTERNAL, mln_lang_network_udp_close_process, NULL, NULL)) == NULL) {
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

static mln_lang_var_t *mln_lang_network_udp_close_process(mln_lang_ctx_t *ctx)
{
    mln_lang_var_t *ret_var;
    mln_string_t v1 = mln_string("fd");
    mln_lang_symbol_node_t *sym;

    if ((sym = mln_lang_symbol_node_search(ctx, &v1, 1)) == NULL) {
        ASSERT(0);
        mln_lang_errmsg(ctx, "Argument missing.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);
    if (mln_lang_var_val_type_get(sym->data.var) != M_LANG_VAL_TYPE_INT) {
        mln_lang_errmsg(ctx, "Invalid type of argument 1.");
        return NULL;
    }
    if ((ret_var = mln_lang_var_create_nil(ctx, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return NULL;
    }
    mln_lang_network_udp_resource_remove(ctx->lang, sym->data.var->val->data.i);
    return ret_var;
}

static int mln_lang_network_udp_send(mln_lang_ctx_t *ctx)
{
    mln_lang_val_t *val;
    mln_lang_var_t *var;
    mln_lang_func_detail_t *func;
    mln_string_t v1 = mln_string("fd"), v2 = mln_string("data"), v3 = mln_string("host"), v4 = mln_string("service");
    mln_string_t funcname = mln_string("mln_udp_send");

    if ((func = mln_lang_func_detail_new(ctx, M_FUNC_INTERNAL, mln_lang_network_udp_send_process, NULL, NULL)) == NULL) {
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

static mln_lang_var_t *mln_lang_network_udp_send_process(mln_lang_ctx_t *ctx)
{
    mln_lang_val_t *val;
    mln_lang_var_t *ret_var;
    mln_string_t v1 = mln_string("fd"), v2 = mln_string("data"), v3 = mln_string("host"), v4 = mln_string("service");
    mln_lang_symbol_node_t *sym;
    struct addrinfo addr, *res = NULL;
    int fd;
    mln_string_t *data;
    char host[128] = {0}, service[64] = {0};
    mln_lang_udp_t *udp;

    if ((sym = mln_lang_symbol_node_search(ctx, &v1, 1)) == NULL) {
        ASSERT(0);
        mln_lang_errmsg(ctx, "Argument missing.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);
    if (mln_lang_var_val_type_get(sym->data.var) != M_LANG_VAL_TYPE_INT) {
        mln_lang_errmsg(ctx, "Invalid type of argument 1.");
        return NULL;
    }
    fd = sym->data.var->val->data.i;

    if ((sym = mln_lang_symbol_node_search(ctx, &v2, 1)) == NULL) {
        ASSERT(0);
        mln_lang_errmsg(ctx, "Argument missing.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);
    if (mln_lang_var_val_type_get(sym->data.var) != M_LANG_VAL_TYPE_STRING) {
        mln_lang_errmsg(ctx, "Invalid type of argument 2.");
        return NULL;
    }
    data = sym->data.var->val->data.s;

    if ((sym = mln_lang_symbol_node_search(ctx, &v3, 1)) == NULL) {
        ASSERT(0);
        mln_lang_errmsg(ctx, "Argument missing.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);
    if (mln_lang_var_val_type_get(sym->data.var) != M_LANG_VAL_TYPE_STRING) {
        mln_lang_errmsg(ctx, "Invalid type of argument 3.");
        return NULL;
    }
    val = sym->data.var->val;
    if (val->data.s->len > sizeof(host)-1) {
        mln_lang_errmsg(ctx, "Invalid host.");
        return NULL;
    }
    memcpy(host, val->data.s->data, val->data.s->len);

    if ((sym = mln_lang_symbol_node_search(ctx, &v4, 1)) == NULL) {
        ASSERT(0);
        mln_lang_errmsg(ctx, "Argument missing.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);
    if (mln_lang_var_val_type_get(sym->data.var) != M_LANG_VAL_TYPE_STRING) {
        mln_lang_errmsg(ctx, "Invalid type of argument 4.");
        return NULL;
    }
    val = sym->data.var->val;
    if (val->data.s->len > sizeof(host)-1) {
        mln_lang_errmsg(ctx, "Invalid service.");
        return NULL;
    }
    memcpy(service, val->data.s->data, val->data.s->len);

    if ((ret_var = mln_lang_var_create_false(ctx, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return NULL;
    }
    udp = mln_lang_network_udp_resource_fetch(ctx->lang, fd);
    if (udp == NULL) {
        mln_lang_var_free(ret_var);
        mln_lang_errmsg(ctx, "socket not existing.");
        return NULL;
    }
    if (udp->recving || udp->sending) {
        mln_lang_errmsg(ctx, "Socket used in other script task.");
        return ret_var;
    }
    memset(&addr, 0, sizeof(addr));
    addr.ai_flags = AI_PASSIVE;
    addr.ai_family = AF_UNSPEC;
    addr.ai_socktype = SOCK_DGRAM;
    addr.ai_protocol = IPPROTO_IP;
    if (getaddrinfo(host, service, &addr, &res) != 0 || res == NULL) {
        return ret_var;
    }
    memcpy(&(udp->addr), res->ai_addr, sizeof(struct sockaddr));
    udp->len = res->ai_addrlen;
    freeaddrinfo(res);
    ASSERT(udp->data==NULL);
    if ((udp->data = mln_string_dup(data)) == NULL) {
        mln_lang_var_free(ret_var);
        mln_lang_errmsg(ctx, "No memory.");
        return NULL;
    }
    if (mln_event_set_fd(ctx->lang->ev, fd, M_EV_SEND|M_EV_NONBLOCK|M_EV_ONESHOT, M_EV_UNLIMITED, udp, mln_lang_network_udp_send_handler) < 0) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_var_free(ret_var);
        return NULL;
    }
    udp->sending = 1;
    mln_lang_ctx_udp_resource_add(ctx, udp);
    mln_lang_ctx_suspend(ctx);
    return ret_var;
}

static void mln_lang_network_udp_send_handler(mln_event_t *ev, int fd, void *data)
{
    mln_lang_udp_t *udp = (mln_lang_udp_t *)data;
#if defined(WIN32)
    int rc = sendto(fd, (char *)(udp->data->data), udp->data->len, 0, &(udp->addr), udp->len);
#else
    int rc = sendto(fd, udp->data->data, udp->data->len, MSG_DONTWAIT, &(udp->addr), udp->len);
#endif
    int err = errno;
    mln_string_free(udp->data);
    udp->data = NULL;
    if (rc < 0 && err == EINTR) {
        mln_event_set_fd(ev, fd, M_EV_SEND|M_EV_NONBLOCK|M_EV_ONESHOT, M_EV_UNLIMITED, udp, mln_lang_network_udp_send_handler);
        return;
    }
    if (rc >= 0) {
        mln_lang_var_t *ret_var = mln_lang_var_create_true(udp->ctx, NULL);
        if (ret_var != NULL) mln_lang_ctx_set_ret_var(udp->ctx, ret_var);
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
    mln_string_t funcname = mln_string("mln_udp_recv");

    if ((func = mln_lang_func_detail_new(ctx, M_FUNC_INTERNAL, mln_lang_network_udp_recv_process, NULL, NULL)) == NULL) {
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
    if ((var = mln_lang_var_new(ctx, &v3, M_LANG_VAR_REFER, val, NULL)) == NULL) {
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
    if ((var = mln_lang_var_new(ctx, &v4, M_LANG_VAR_REFER, val, NULL)) == NULL) {
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
    if ((var = mln_lang_var_new(ctx, &v5, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
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

static mln_lang_var_t *mln_lang_network_udp_recv_process(mln_lang_ctx_t *ctx)
{
    mln_lang_var_t *ret_var;
    mln_string_t v1 = mln_string("fd"), v2 = mln_string("bufsize"), v3 = mln_string("ip"), v4 = mln_string("port"), v5 = mln_string("timeout");
    mln_lang_symbol_node_t *sym;
    int fd, timeout, type;
    mln_s64_t size;
    mln_lang_udp_t *udp;
    mln_lang_var_t *var1, *var2;
    /*arg1*/
    if ((sym = mln_lang_symbol_node_search(ctx, &v1, 1)) == NULL) {
        ASSERT(0);
        mln_lang_errmsg(ctx, "Argument missing.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);
    if (mln_lang_var_val_type_get(sym->data.var) != M_LANG_VAL_TYPE_INT) {
        mln_lang_errmsg(ctx, "Invalid type of argument 1.");
        return NULL;
    }
    fd = sym->data.var->val->data.i;
    /*arg2*/
    if ((sym = mln_lang_symbol_node_search(ctx, &v2, 1)) == NULL) {
        ASSERT(0);
        mln_lang_errmsg(ctx, "Argument missing.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);
    if (mln_lang_var_val_type_get(sym->data.var) != M_LANG_VAL_TYPE_INT) {
        mln_lang_errmsg(ctx, "Invalid type of argument 2.");
        return NULL;
    }
    size = sym->data.var->val->data.i;
    if (size <= 0) {
        mln_lang_errmsg(ctx, "Invalid type of argument 2.");
        return NULL;
    }
    /*arg3*/
    if ((sym = mln_lang_symbol_node_search(ctx, &v3, 1)) == NULL) {
        ASSERT(0);
        mln_lang_errmsg(ctx, "Argument missing.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);
    var1 = sym->data.var;
    /*arg4*/
    if ((sym = mln_lang_symbol_node_search(ctx, &v4, 1)) == NULL) {
        ASSERT(0);
        mln_lang_errmsg(ctx, "Argument missing.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);
    var2 = sym->data.var;
    /*arg5*/
    if ((sym = mln_lang_symbol_node_search(ctx, &v5, 1)) == NULL) {
        ASSERT(0);
        mln_lang_errmsg(ctx, "Argument missing.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);
    if ((type = mln_lang_var_val_type_get(sym->data.var)) == M_LANG_VAL_TYPE_NIL) {
        timeout = M_EV_UNLIMITED;
    } else if (type == M_LANG_VAL_TYPE_INT && (timeout = sym->data.var->val->data.i) >= 0) {
        /*do nothing*/
    } else {
        mln_lang_errmsg(ctx, "Invalid type of argument 5.");
        return NULL;
    }

    if ((ret_var = mln_lang_var_create_false(ctx, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return NULL;
    }
    udp = mln_lang_network_udp_resource_fetch(ctx->lang, fd);
    if (udp == NULL) {
        mln_lang_var_free(ret_var);
        mln_lang_errmsg(ctx, "socket not existing.");
        return NULL;
    }
    if (udp->recving || udp->sending) {
        mln_lang_errmsg(ctx, "Socket used in other script task.");
        return ret_var;
    }
    udp->timeout = timeout;
    udp->bufsize = size;
    udp->ip = var1;
    udp->port = var2;

    if (mln_event_set_fd(ctx->lang->ev, fd, M_EV_RECV|M_EV_NONBLOCK|M_EV_ONESHOT, udp->timeout, udp, mln_lang_network_udp_recv_handler) < 0) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_var_free(ret_var);
        return NULL;
    }
    if (timeout != M_EV_UNLIMITED) {
        mln_event_set_fd_timeout_handler(ctx->lang->ev, fd, udp, mln_lang_network_udp_timeout_handler);
    }
    udp->recving = 1;
    mln_lang_ctx_udp_resource_add(ctx, udp);
    mln_lang_ctx_suspend(ctx);
    return ret_var;
}

static void mln_lang_network_udp_recv_handler(mln_event_t *ev, int fd, void *data)
{
    mln_lang_udp_t *udp = (mln_lang_udp_t *)data;
    mln_u8ptr_t buf = NULL;
    mln_u8_t addr[sizeof(struct sockaddr) >= sizeof(struct sockaddr_in6)? \
                  sizeof(struct sockaddr): \
                  sizeof(struct sockaddr_in6)] = {0};
    socklen_t len;
    int rc;
    mln_lang_var_t *ret_var;
    mln_lang_var_t var;
    mln_lang_val_t val;
    mln_string_t tmp;
    mln_u16_t port;
    char ip[128] = {0};

    len = sizeof(addr);
    if ((buf = (mln_u8ptr_t)malloc(udp->bufsize)) == NULL) {
        mln_event_set_fd(ev, fd, M_EV_RECV|M_EV_NONBLOCK|M_EV_ONESHOT, udp->timeout, udp, mln_lang_network_udp_recv_handler);
        if (udp->timeout != M_EV_UNLIMITED)
            mln_event_set_fd_timeout_handler(ev, fd, udp, mln_lang_network_udp_timeout_handler);
        return;
    }

#if defined(WIN32)
    rc = recvfrom(fd, (char *)buf, udp->bufsize, 0, (struct sockaddr *)addr, &len);
#else
    rc = recvfrom(fd, buf, udp->bufsize, MSG_DONTWAIT, (struct sockaddr *)addr, &len);
#endif
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
    if (mln_lang_network_get_addr(addr, ip, &port) < 0) {
        if (rc > 0) free(buf);
        goto out;
    }
    /*ip*/
    var.type = M_LANG_VAR_NORMAL;
    var.name = NULL;
    var.val = &val;
    var.in_set = NULL;
    var.prev = var.next = NULL;
    val.type = M_LANG_VAL_TYPE_STRING;
    val.ref = 1;
    mln_string_set(&tmp, ip);
    val.data.s = &tmp;
    if (mln_lang_var_value_set(udp->ctx, udp->ip, &var) < 0) {
        if (rc > 0) free(buf);
        goto out;
    }
    /*port*/
    var.type = M_LANG_VAR_NORMAL;
    var.name = NULL;
    var.val = &val;
    var.in_set = NULL;
    var.prev = var.next = NULL;
    val.data.i = port;
    val.type = M_LANG_VAL_TYPE_INT;
    val.ref = 1;
    if (mln_lang_var_value_set(udp->ctx, udp->port, &var) < 0) {
        if (rc > 0) free(buf);
        goto out;
    }
    /*data*/
    if (rc > 0) {
        mln_string_nset(&tmp, buf, rc);
        ret_var = mln_lang_var_create_string(udp->ctx, &tmp, NULL);
        free(buf);
        if (ret_var != NULL) mln_lang_ctx_set_ret_var(udp->ctx, ret_var);
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
    mln_lang_var_t *ret_var;
    mln_event_set_fd(ev, fd, M_EV_CLR, M_EV_UNLIMITED, NULL, NULL);
    ret_var = mln_lang_var_create_nil(udp->ctx, NULL);
    udp->recving = 0;
    ASSERT(!udp->sending);
    if (ret_var != NULL) {
        mln_lang_ctx_set_ret_var(udp->ctx, ret_var);
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
    mln_socket_close(lu->fd);
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

