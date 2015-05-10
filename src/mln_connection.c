
/*
 * Copyright (C) Niklaus F.Schen.
 */
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include "mln_connection.h"
#include "mln_log.h"

static inline int mln_fd_is_nonblock(int fd);
static int mln_connection_return(mln_tcp_connection_t *c, int flag) __NONNULL1(1);


void mln_tcp_connection_init(mln_tcp_connection_t *c, int fd)
{
    c->fd = fd;
    c->recv.buf = NULL;
    c->recv.len = 0;
    c->recv.pos = 0;
    c->recv.is_referred = 0;
    c->send.buf = NULL;
    c->send.len = 0;
    c->send.pos = 0;
    c->send.is_referred = 0;
}

void mln_tcp_connection_destroy(mln_tcp_connection_t *c)
{
    mln_connection_return(c, M_C_CLOSED);
    memset(c, 0, sizeof(mln_tcp_connection_t));
    c->fd = -1;
}

int mln_tcp_connection_set_buf(mln_tcp_connection_t *c, \
                               void *buf, \
                               mln_u32_t len, \
                               int type, \
                               int is_referred)
{
    if (type == M_C_SEND) {
        if (is_referred) {
            c->send.buf = buf;
            c->send.is_referred = 1;
        } else {
            c->send.buf = malloc(len);
            if (c->send.buf == NULL) return -1;
            memcpy(c->send.buf, buf, len);
            c->send.is_referred = 0;
        }
        c->send.len = len;
        c->send.pos = 0;
    } else if (type == M_C_RECV) {
        if (is_referred) {
            c->recv.buf = buf;
            c->recv.is_referred = 1;
        } else {
            c->recv.buf = malloc(len);
            if (c->recv.buf == NULL) return -1;
            c->recv.is_referred = 0;
        }
        c->recv.len = len;
        c->recv.pos = 0;
    } else {
        mln_log(error, "No such type.\n");
        abort();
    }
    return 0;
}

void *mln_tcp_connection_get_buf(mln_tcp_connection_t *c, int type)
{
    if (type != M_C_SEND && type != M_C_RECV) {
        mln_log(error, "No such type.\n");
        abort();
    }
    return (type == M_C_SEND)?c->send.buf:c->recv.buf;
}

void mln_tcp_connection_clr_buf(mln_tcp_connection_t *c, int type)
{
    if (type == M_C_SEND) {
        if (c->send.buf != NULL && !c->send.is_referred)
            free(c->send.buf);
        c->send.buf = NULL;
        c->send.len = 0;
        c->send.pos = 0;
    } else if (type == M_C_RECV) {
        if (c->recv.buf != NULL && !c->recv.is_referred)
            free(c->recv.buf);
        c->recv.buf = NULL;
        c->recv.len = 0;
        c->recv.pos = 0;
    } else {
        mln_log(error, "No such type.\n");
        abort();
    }
}

static inline int mln_fd_is_nonblock(int fd)
{
    int flg;
    if ((flg = fcntl(fd, F_GETFL, NULL)) < 0) {
        mln_log(error, "fcntl F_GETFL failed. %s\n", strerror(errno));
        abort();
    }
    return flg & O_NONBLOCK;
}

static int mln_connection_return(mln_tcp_connection_t *c, int flag)
{
    switch (flag) {
        case M_C_FINISH:
        case M_C_NOTYET: return flag;
        case M_C_ERROR:
        case M_C_CLOSED:
            mln_tcp_connection_clr_buf(c, M_C_SEND);
            mln_tcp_connection_clr_buf(c, M_C_RECV);
            close(c->fd);
            break;
        default:
            mln_log(error, "No such flag.\n");
            abort();
    }
    return flag;
}

int mln_tcp_connection_send(mln_tcp_connection_t *c)
{
    if (c->send.pos >= c->send.len)
        return mln_connection_return(c, M_C_FINISH);
    int n;
    if (mln_fd_is_nonblock(c->fd)) {
lp1:
        while ((n = send(c->fd, \
                         c->send.buf+c->send.pos, \
                         c->send.len-c->send.pos, 0)) > 0)
        {
            c->send.pos += n;
            if (c->send.pos >= c->send.len)
                return mln_connection_return(c, M_C_FINISH);
        }
        if (n == 0) {
            mln_log(error, "send return 0.\n");
            abort();
        }
        if (errno == EINTR) goto lp1;
        if (errno == EAGAIN)
            return mln_connection_return(c, M_C_NOTYET);
    } else {
lp2:
        n = send(c->fd, c->send.buf+c->send.pos, c->send.len-c->send.pos, 0);
        if (n > 0) {
            c->send.pos += n;
            if (c->send.pos >= c->send.len)
                return mln_connection_return(c, M_C_FINISH);
            return mln_connection_return(c, M_C_NOTYET);
        } else if (n == 0) {
            mln_log(error, "send return 0.\n");
            abort();
        }
        if (errno == EINTR) goto lp2;
    }
    return mln_connection_return(c, M_C_ERROR);
}

int mln_tcp_connection_recv(mln_tcp_connection_t *c)
{
    if (c->recv.pos >= c->recv.len)
        return mln_connection_return(c, M_C_FINISH);
    int n;
    if (mln_fd_is_nonblock(c->fd)) {
lp1:
        while ((n = recv(c->fd, \
                         c->recv.buf+c->recv.pos, \
                         c->recv.len-c->recv.pos, 0)) > 0)
        {
            c->recv.pos += n;
            if (c->recv.pos >= c->recv.len)
                return mln_connection_return(c, M_C_FINISH);
        }
        if (n == 0) return mln_connection_return(c, M_C_CLOSED);
        if (errno == EINTR) goto lp1;
        if (errno == EAGAIN)
            return mln_connection_return(c, M_C_NOTYET);
    } else {
lp2:
        n = recv(c->fd, c->recv.buf+c->recv.pos, c->recv.len-c->recv.pos, 0);
        if (n > 0) {
            c->recv.pos += n;
            if (c->recv.pos >= c->recv.len)
                return mln_connection_return(c, M_C_FINISH);
            return mln_connection_return(c, M_C_NOTYET);
        } else if (n == 0) {
            return mln_connection_return(c, M_C_CLOSED);
        }
        if (errno == EINTR) goto lp2;
    }
    return mln_connection_return(c, M_C_ERROR);
}

