
/*
 * Copyright (C) Niklaus F.Schen.
 */
#ifndef __MLN_CONNECTION_H
#define __MLN_CONNECTION_H

#include <sys/types.h>
#include <sys/socket.h>
#include "mln_types.h"
#include "mln_chain.h"
#include "mln_alloc.h"

/*buffer type*/
#define M_C_SEND 1
#define M_C_RECV 2
#define M_C_SENT 3
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
    mln_u32_t                is_referred:1;
};

typedef struct {
    struct mln_conn_buf_s    recv;
    struct mln_conn_buf_s    send;
    int                      fd     __cacheline_aligned;
} mln_tcp_connection_t;

#define mln_tcp_connection_get_fd(pconn) ((pconn)->fd)
extern void
mln_tcp_connection_init(mln_tcp_connection_t *c, int fd) __NONNULL1(1);
extern void
mln_tcp_connection_destroy(mln_tcp_connection_t *c) __NONNULL1(1);
extern int
mln_tcp_connection_set_buf(mln_tcp_connection_t *c, \
                           void *buf, \
                           mln_u32_t len, \
                           int type, \
                           int is_referred) __NONNULL1(1);
extern void
mln_tcp_connection_clr_buf(mln_tcp_connection_t *c, int type) __NONNULL1(1);
extern void *
mln_tcp_connection_get_buf(mln_tcp_connection_t *c, int type) __NONNULL1(1);
extern int
mln_tcp_connection_send(mln_tcp_connection_t *c) __NONNULL1(1);
extern int
mln_tcp_connection_recv(mln_tcp_connection_t *c) __NONNULL1(1);


/*
 * another tcp I/O
 */

#define M_C_TYPE_FOLLOW 0x8
#define M_C_TYPE_MEMORY 0x1
#define M_C_TYPE_FILE   0x2

typedef struct {
    mln_alloc_t *pool;
    mln_chain_t *rcv_head;
    mln_chain_t *rcv_tail;
    mln_chain_t *snd_head;
    mln_chain_t *snd_tail;
    mln_chain_t *sent_head;
    mln_chain_t *sent_tail;
    int          sockfd;
} mln_tcp_conn_t;


#define mln_tcp_conn_send_empty(pconn) ((pconn)->snd_head == NULL)
#define mln_tcp_conn_recv_empty(pconn) ((pconn)->rcv_head == NULL)
#define mln_tcp_conn_sent_empty(pconn) ((pconn)->sent_head == NULL)
#define mln_tcp_conn_get_fd(pconn) ((pconn)->sockfd)
extern int mln_tcp_conn_init(mln_tcp_conn_t *tc, int sockfd) __NONNULL1(1);
extern void mln_tcp_conn_destroy(mln_tcp_conn_t *tc);
extern int mln_tcp_conn_getsock(mln_tcp_conn_t *tc) __NONNULL1(1);
extern void
mln_tcp_conn_append(mln_tcp_conn_t *tc, mln_chain_t *c, int type) __NONNULL2(1,2);
extern void
mln_tcp_conn_set(mln_tcp_conn_t *tc, mln_chain_t *c, int type) __NONNULL2(1,2);
extern mln_chain_t *mln_tcp_conn_get(mln_tcp_conn_t *tc, int type) __NONNULL1(1);
extern mln_chain_t *mln_tcp_conn_remove(mln_tcp_conn_t *tc, int type) __NONNULL1(1);
extern int mln_tcp_conn_send(mln_tcp_conn_t *tc) __NONNULL1(1);
extern int mln_tcp_conn_recv(mln_tcp_conn_t *tc, mln_u32_t flag) __NONNULL1(1);

#endif

