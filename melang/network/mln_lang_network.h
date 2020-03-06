
/*
 * Copyright (C) Niklaus F.Schen.
 */
#ifndef __MLN_LANG_NETWORK_H
#define __MLN_LANG_NETWORK_H

#include "mln_lang.h"
#include "mln_connection.h"

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
    mln_s32_t              timeout;
    struct mln_lang_tcp_s *prev;
    struct mln_lang_tcp_s *next;
} mln_lang_tcp_t;

typedef struct mln_lang_ctx_tcp_s {
    mln_lang_ctx_t        *ctx;
    mln_lang_tcp_t        *head;
    mln_lang_tcp_t        *tail;
} mln_lang_ctx_tcp_t;

extern int mln_lang_network(mln_lang_ctx_t *ctx);

#endif
