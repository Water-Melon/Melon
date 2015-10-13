
/*
 * Copyright (C) Niklaus F.Schen.
 */
#ifndef __MLN_HTTP_H
#define __MLN_HTTP_H

#include <sys/types.h>
#include <sys/socket.h>
#include "mln_http_header.h"

/*status_code*/
#define M_HTTP_STATUS_CODE_DONE   1
#define M_HTTP_STATUS_CODE_OK     0
#define M_HTTP_STATUS_CODE_ERROR -1

typedef struct mln_http_s mln_http_t;
typedef mln_chain_t *(*http_handle)(mln_http_t *, mln_chain_t *, void *);

struct mln_http_s {
    mln_alloc_t                      *pool;
    void                             *data;
    mln_tcp_conn_t                   *conn;
    http_handle                       handle;
    mln_http_header_t                *header;
    mln_chain_t                      *body_head;
    mln_chain_t                      *body_tail;
    struct sockaddr                   peer_addr;
    mln_u32_t                         header_done:1;
    mln_s32_t                         status_code;
};

extern mln_http_t *
mln_http_init(mln_alloc_t *pool) __NONNULL1(1);
extern void
mln_http_destroy(mln_http_t *http);
extern mln_chain_t *
mln_http_packup(mln_http_t *http) __NONNULL1(1);
extern mln_chain_t *
mln_http_parse(mln_http_t *http, mln_chain_t *in) __NONNULL1(1);
extern void
mln_http_set_send_default_handle(mln_http_t *http) __NONNULL1(1);

#define mln_http_get_pool(http) (http)->pool
#define mln_http_set_pool(http,p) (http)->pool = (p)

#define mln_http_get_data(http) (http)->data
#define mln_http_set_data(http,pdata) (http)->data = (pdata)

#define mln_http_get_connection(http) (http)->conn
#define mln_http_set_connection(http,con) (http)->conn = (con)

#define mln_http_get_handle(http) (http)->handle
#define mln_http_set_handle(http,h) (http)->handle = (h)

#define mln_http_get_header(http) (http)->header
#define mln_http_set_header(http,h) (http)->header = (h)

#define mln_http_body_head(http) (http)->body_head
#define mln_http_body_append(http,chain); \
{\
    if ((http)->body_head == NULL) {\
        (http)->body_head = (http)->body_tail = (chain);\
    } else {\
        (http)->body_tail->next = (chain);\
        (http)->body_tail = (chain);\
    }\
}

#define mln_http_get_peer_address(http) &((http)->peer_addr)
#define mln_http_set_peer_address(http,addr) \
    memcpy(&((http)->peer_addr), &addr, sizeof(struct sockaddr))

#define mln_http_get_header_done(http) (((http)->header_done) & 0x1)
#define mln_http_set_header_done(http) ((http)->header_done |= 0x1)
#define mln_http_reset_header_done(http) (http)->header_done &= (~((mln_u32_t)0x1))

#define mln_http_get_status_code(http) (http)->status_code
#define mln_http_set_status_code(http,code) (http)->status_code = (code)

#endif

