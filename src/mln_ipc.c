
/*
 * Copyright (C) Niklaus F.Schen.
 */
#include "mln_lex.h"
#include "mln_fork.h"
#include "mln_ipc.h"
#include "mln_log.h"
#include "mln_connection.h"
#include "mln_event.h"
#include "mln_fheap.h"
#include "mln_global.h"
#include "mln_hash.h"
#include "mln_prime_generator.h"
#include "mln_rbtree.h"
#include "mln_string.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>

/*
 * IPC only act on A child process and the parent process.
 * If there are some threads in a child process,
 * IPC only act on the control thread (main thread) and the parent process.
 * If you need to send something to the peer,
 * you can call mln_tcp_connection_set_buf() to set connection send buffer
 * and call mln_event_set_fd() to set send event, which means you can
 * customize the send routine.
 */


char send_buffer[] = "Hello world!";

void master_send_handler(mln_event_t *ev, int fd, void *data)
{
    int ret;
    mln_fork_t *f = (mln_fork_t *)data;
    mln_tcp_connection_t *c = &(f->conn);
    if ((ret = mln_tcp_connection_send(c)) == M_C_FINISH) {
        mln_tcp_connection_clr_buf(c, M_C_SEND);
        mln_log(report, "Send succeed!\n");
    } else if (ret == M_C_NOTYET) {
        if (mln_event_set_fd(ev, fd, M_EV_SEND|M_EV_APPEND|M_EV_ONESHOT, c, master_send_handler) < 0) {
            mln_log(error, "mln_event_set_fd() failed.\n");
            abort();
        }
    } else if (ret == M_C_ERROR) {
        mln_log(error, "mln_tcp_connection_send error. %s\n", strerror(errno));
    } else {
        mln_log(error, "Shouldn't be here.\n");
        abort();
    }
}

void worker_send_handler(mln_event_t *ev, int fd, void *data)
{
    int ret;
    mln_tcp_connection_t *c = (mln_tcp_connection_t *)data;
    if ((ret = mln_tcp_connection_send(c)) == M_C_FINISH) {
        mln_tcp_connection_clr_buf(c, M_C_SEND);
        mln_log(report, "Send succeed!\n");
    } else if (ret == M_C_NOTYET) {
        if (mln_event_set_fd(ev, fd, M_EV_SEND|M_EV_APPEND|M_EV_ONESHOT, c, worker_send_handler) < 0) {
            mln_log(error, "mln_event_set_fd() failed.\n");
            abort();
        }
    } else if (ret == M_C_ERROR) {
        mln_log(error, "mln_tcp_connection_send error. %s\n", strerror(errno));
    } else {
        mln_log(error, "Shouldn't be here.\n");
        abort();
    }
}

void test_master(mln_event_t *ev, void *f_ptr, void *buf, mln_u32_t len, void **udata_ptr)
{
    mln_fork_t *f = (mln_fork_t *)f_ptr;
    mln_tcp_connection_t *conn = &(f->conn);
    if (strncmp(send_buffer, (char *)buf, len)) {
        mln_log(error, "Content incorrect. [%s]\n", (char *)buf);
        abort();
    }
    mln_log(report, "Parent ipc received!\n");
    char b[32] = {0};
    mln_u32_t le = sizeof(mln_u32_t) + len;
    memcpy(b, &le, sizeof(mln_u32_t));
    mln_u32_t type = M_IPC_1;
    memcpy(b+sizeof(mln_u32_t), &type, sizeof(mln_u32_t));
    memcpy(b+2*sizeof(mln_u32_t), buf, len);
    mln_tcp_connection_set_buf(conn, b, len+2*sizeof(mln_u32_t), M_C_SEND);
    if (mln_event_set_fd(ev, conn->fd, M_EV_SEND|M_EV_APPEND|M_EV_ONESHOT, f, master_send_handler) < 0) {
        mln_log(error, "mln_event_set_fd() failed.\n");
        abort();
    }
}

void test_worker(mln_event_t *ev, void *c, void *buf, mln_u32_t len, void **udata_ptr)
{
    mln_tcp_connection_t *conn = (mln_tcp_connection_t *)c;
    if (strncmp(send_buffer, (char *)buf, len)) {
        mln_log(error, "Content incorrect. [%s]\n", (char *)buf);
        abort();
    }
    mln_log(report, "Child ipc received!\n");
    char b[32] = {0};
    mln_u32_t le = sizeof(mln_u32_t) + len;
    memcpy(b, &le, sizeof(mln_u32_t));
    mln_u32_t type = M_IPC_1;
    memcpy(b+sizeof(mln_u32_t), &type, sizeof(mln_u32_t));
    memcpy(b+2*sizeof(mln_u32_t), buf, len);
    mln_tcp_connection_set_buf(conn, b, len+2*sizeof(mln_u32_t), M_C_SEND);
    if (mln_event_set_fd(ev, conn->fd, M_EV_SEND|M_EV_APPEND|M_EV_ONESHOT, conn, worker_send_handler) < 0) {
        mln_log(error, "mln_event_set_fd() failed.\n");
        abort();
    }
}


mln_ipc_handler_t ipc_master_handlers[] = {
{test_master, NULL, M_IPC_1}
};
mln_ipc_handler_t ipc_worker_handlers[] = {
{test_worker, NULL, M_IPC_1}
};
void mln_set_ipc_handlers(void)
{
    int i;
    for (i = 0; i < sizeof(ipc_master_handlers)/sizeof(mln_ipc_handler_t); i++) {
        mln_set_master_ipc_handler(&ipc_master_handlers[i]);
    }
    for (i = 0; i < sizeof(ipc_worker_handlers)/sizeof(mln_ipc_handler_t); i++) {
        mln_set_worker_ipc_handler(&ipc_worker_handlers[i]);
    }
}
