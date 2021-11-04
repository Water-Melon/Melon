
/*
 * Copyright (C) Niklaus F.Schen.
 */
#ifndef __MLN_LANG_NETWORK_H
#define __MLN_LANG_NETWORK_H

#include "mln_lang.h"
#include "mln_connection.h"
#if defined(WIN32)
#include <ws2tcpip.h>
#endif

#define MLN_LANG_NETWORK_TCP_CONNECT_RETRY 64

typedef struct mln_lang_tcp_s {
    mln_lang_t            *lang;
    mln_lang_ctx_t        *ctx;
    mln_tcp_conn_t         conn;
    char                   ip[128];
    mln_u16_t              port;
    mln_u16_t              send_closed:1;
    mln_u16_t              recv_closed:1;
    mln_u16_t              sending:1;
    mln_u16_t              recving:1;
    mln_u16_t              retry:12;
    mln_s32_t              timeout;
    mln_s32_t              connect_timeout;
    struct mln_lang_tcp_s *prev;
    struct mln_lang_tcp_s *next;
} mln_lang_tcp_t;

typedef struct mln_lang_ctx_tcp_s {
    mln_lang_ctx_t        *ctx;
    mln_lang_tcp_t        *head;
    mln_lang_tcp_t        *tail;
} mln_lang_ctx_tcp_t;

typedef struct mln_lang_udp_s {
    mln_lang_t            *lang;
    mln_lang_ctx_t        *ctx;
    struct sockaddr        addr;
    socklen_t              len;
    int                    fd;
    mln_s32_t              timeout;
    mln_string_t          *data;
    mln_u64_t              bufsize;
    mln_u64_t              sending:1;
    mln_u64_t              recving:1;
    mln_u64_t              padding:62;
    mln_lang_var_t        *ip;
    mln_lang_var_t        *port;
    struct mln_lang_udp_s *prev;
    struct mln_lang_udp_s *next;
} mln_lang_udp_t;

typedef struct mln_lang_ctx_udp_s {
    mln_lang_ctx_t        *ctx;
    mln_lang_udp_t        *head;
    mln_lang_udp_t        *tail;
} mln_lang_ctx_udp_t;

extern int mln_lang_network(mln_lang_ctx_t *ctx);

#endif
