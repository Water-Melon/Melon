
/*
 * Copyright (C) Niklaus F.Schen.
 */
#ifndef __MLN_CONNECTION_H
#define __MLN_CONNECTION_H

#include <sys/types.h>
#include <sys/socket.h>
#include "mln_types.h"

/*buffer type*/
#define M_C_SEND 1
#define M_C_RECV 2
/*return value*/
#define M_C_FINISH 1
#define M_C_NOTYET 2
#define M_C_ERROR  3
#define M_C_CLOSED 4
/*functions*/
#define M_C_SND_EMPTY(c_ptr) (!(c_ptr)->send.pos)
#define M_C_RCV_EMPTY(c_ptr) (!(c_ptr)->recv.pos)
#define M_C_SND_NULL(c_ptr) ((c_ptr)->send.buf == NULL)
#define M_C_RCV_NULL(c_ptr) ((c_ptr)->recv.buf == NULL)

struct mln_conn_buf_s {
    void                    *buf;
    mln_u32_t                len;
    mln_u32_t                pos;
};

typedef struct {
    struct mln_conn_buf_s    recv;
    struct mln_conn_buf_s    send;
    int                      fd     __cacheline_aligned;
} mln_tcp_connection_t;

extern void
mln_tcp_connection_init(mln_tcp_connection_t *c, int fd) __NONNULL1(1);
extern void
mln_tcp_connection_destroy(mln_tcp_connection_t *c) __NONNULL1(1);
extern int
mln_tcp_connection_set_buf(mln_tcp_connection_t *c, \
                           void *buf, \
                           mln_u32_t len, \
                           int type) __NONNULL1(1);
extern void
mln_tcp_connection_clr_buf(mln_tcp_connection_t *c, int type) __NONNULL1(1);
extern void *
mln_tcp_connection_get_buf(mln_tcp_connection_t *c, int type) __NONNULL1(1);
extern int
mln_tcp_connection_send(mln_tcp_connection_t *c) __NONNULL1(1);
extern int
mln_tcp_connection_recv(mln_tcp_connection_t *c) __NONNULL1(1);

#endif

