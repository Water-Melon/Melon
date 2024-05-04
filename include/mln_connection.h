
/*
 * Copyright (C) Niklaus F.Schen.
 */
#ifndef __MLN_CONNECTION_H
#define __MLN_CONNECTION_H

#if defined(MSVC)
#include <winsock2.h>
#else
#include <sys/socket.h>
#endif
#include <sys/types.h>
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
#define mln_tcp_conn_fd_get(pconn) ((pconn)->sockfd)
#define mln_tcp_conn_fd_set(pconn,fd) (pconn)->sockfd = (fd)
#define mln_tcp_conn_pool_get(pconn) ((pconn)->pool)
extern int mln_tcp_conn_init(mln_tcp_conn_t *tc, int sockfd) __NONNULL1(1);
extern void mln_tcp_conn_destroy(mln_tcp_conn_t *tc);
extern void
mln_tcp_conn_append_chain(mln_tcp_conn_t *tc, \
                          mln_chain_t *c_head, \
                          mln_chain_t *c_tail, \
                          int type) __NONNULL1(1);
extern void
mln_tcp_conn_append(mln_tcp_conn_t *tc, mln_chain_t *c, int type) __NONNULL2(1,2);
extern mln_chain_t *mln_tcp_conn_head(mln_tcp_conn_t *tc, int type) __NONNULL1(1);
extern mln_chain_t *mln_tcp_conn_remove(mln_tcp_conn_t *tc, int type) __NONNULL1(1);
extern mln_chain_t *mln_tcp_conn_pop(mln_tcp_conn_t *tc, int type) __NONNULL1(1);
extern mln_chain_t *mln_tcp_conn_tail(mln_tcp_conn_t *tc, int type) __NONNULL1(1);
extern int mln_tcp_conn_send(mln_tcp_conn_t *tc) __NONNULL1(1);
extern int mln_tcp_conn_recv(mln_tcp_conn_t *tc, mln_u32_t flag) __NONNULL1(1);

#endif

