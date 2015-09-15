
/*
 * Copyright (C) Niklaus F.Schen.
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "mln_connection.h"
#include "mln_log.h"
#include "mln_file.h"
#if defined(MLN_SENDFILE)
#include <sys/sendfile.h>
#endif


static inline int mln_fd_is_nonblock(int fd);
static int mln_connection_return(mln_tcp_connection_t *c, int flag) __NONNULL1(1);
static inline int
mln_tcp_conn_send_chain(int sockfd, mln_chain_t *out);
static inline mln_chain_t *
mln_tcp_conn_pop(mln_tcp_conn_t *tc, int type);
static inline int
mln_tcp_conn_recv_chain(mln_tcp_conn_t *tc, mln_u32_t flag);
static inline int
mln_tcp_conn_recv_chain_file(int sockfd, \
                             mln_alloc_t *pool, \
                             mln_buf_t *b, \
                             mln_buf_t *last);
static inline int
mln_tcp_conn_recv_chain_mem(int sockfd, mln_alloc_t *pool, mln_buf_t *b);


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



/*
 * mln_tcp_conn_t
 */

int mln_tcp_conn_init(mln_tcp_conn_t *tc, int sockfd)
{
    tc->pool = mln_alloc_init();
    if (tc->pool == NULL) return -1;
    tc->rcv_head = tc->rcv_tail = NULL;
    tc->snd_head = tc->snd_tail = NULL;
    tc->sent_head = tc->sent_tail = NULL;
    tc->sockfd = sockfd;
    return 0;
}

void mln_tcp_conn_destroy(mln_tcp_conn_t *tc)
{
    if (tc == NULL) return;

    mln_chain_pool_release_all(mln_tcp_conn_remove(tc, M_C_SEND));
    mln_chain_pool_release_all(mln_tcp_conn_remove(tc, M_C_RECV));
    mln_chain_pool_release_all(mln_tcp_conn_remove(tc, M_C_SENT));
    mln_alloc_destroy(tc->pool);
}

int mln_tcp_conn_getsock(mln_tcp_conn_t *tc)
{
    return tc->sockfd;
}

void mln_tcp_conn_set(mln_tcp_conn_t *tc, mln_chain_t *c, int type)
{
    mln_chain_t **head, **tail;
    if (type == M_C_SEND) {
        head = &(tc->snd_head);
        tail = &(tc->snd_tail);
    } else if (type == M_C_RECV) {
        head = &(tc->rcv_head);
        tail = &(tc->rcv_tail);
    } else if (type == M_C_SENT) {
        head = &(tc->sent_head);
        tail = &(tc->sent_tail);
    } else {
        mln_log(error, "flag error.\n");
        abort();
    }

    if (*head == NULL) {
        *head = *tail = c;
    } else {
        (*tail)->next = c;
        *tail = c;
    }
}

mln_chain_t *mln_tcp_conn_get(mln_tcp_conn_t *tc, int type)
{
    mln_chain_t *rc = NULL;

    if (type == M_C_SEND) {
        rc = tc->snd_head;
    } else if (type == M_C_RECV) {
        rc = tc->rcv_head;
    } else if (type == M_C_SENT) {
        rc = tc->sent_head;
    } else {
        mln_log(error, "flag error.\n");
        abort();
    }

    return rc;
}

mln_chain_t *mln_tcp_conn_remove(mln_tcp_conn_t *tc, int type)
{
    mln_chain_t *rc;

    if (type == M_C_SEND) {
        rc = tc->snd_head;
        tc->snd_head = tc->snd_tail = NULL;
    } else if (type == M_C_RECV) {
        rc = tc->rcv_head;
        tc->rcv_head = tc->rcv_tail = NULL;
    } else if (type == M_C_SENT) {
        rc = tc->sent_head;
        tc->sent_head = tc->sent_tail = NULL;
    } else {
        mln_log(error, "flag error.\n");
        abort();
    }

    return rc;
}

int mln_tcp_conn_send(mln_tcp_conn_t *tc)
{
    int n, nonblock = 0;
    mln_chain_t *c;

    nonblock = mln_fd_is_nonblock(tc->sockfd);

    if (nonblock) {
goon_non:
        while ((n = mln_tcp_conn_send_chain(tc->sockfd, tc->snd_head)) > 0) {
            if (!mln_buf_left_size(tc->snd_head->buf)) {
                c = mln_tcp_conn_pop(tc, M_C_SEND);
                mln_tcp_conn_set(tc, c, M_C_SENT);
                if (c->buf != NULL && c->buf->last_in_chain) {
                    return M_C_FINISH;
                }
            }
        }
    } else {
goon_blk:
        if ((n = mln_tcp_conn_send_chain(tc->sockfd, tc->snd_head)) > 0) {
            if (!mln_buf_left_size(tc->snd_head->buf)) {
                c = mln_tcp_conn_pop(tc, M_C_SEND);
                mln_tcp_conn_set(tc, c, M_C_SENT);
                if (c->buf != NULL && c->buf->last_in_chain) {
                    return M_C_FINISH;
                }
            }
            return M_C_NOTYET;
        }
    }
    if (n == 0) {
        return M_C_NOTYET;
    }
    if (errno == EINTR) {
        if (nonblock) {
            goto goon_non;
        } else {
            goto goon_blk;
        }
    } else if (errno == EAGAIN) {
        return M_C_NOTYET;
    }
    return M_C_ERROR;
}

static inline int
mln_tcp_conn_send_chain(int sockfd, mln_chain_t *out)
{
    int n;
    mln_buf_t *b;

    if (out == NULL) return 0;
    if ((b = out->buf) == NULL) {
        mln_log(error, "Buf shouldn't be NULL.\n");
        return 1;
    }

    if (b->in_file) {
#if defined(MLN_SENDFILE)
        return sendfile(sockfd, mln_file_fd(b->file), &b->file_send_pos, b->file_last - b->file_send_pos);
#else
        mln_u8_t buf[4096];
        mln_size_t left_size, len;
        left_size = b->file_last - b->file_send_pos;
        len = left_size >= 4096? 4096: left_size;

        lseek(mln_file_fd(b->file), b->file_send_pos, SEEK_SET);
        n = read(mln_file_fd(b->file), buf, len);
        if (n <= 0) return n;

        n = send(sockfd, buf, n, 0);
        if (n > 0) {
            b->file_send_pos += n;
        }
        return n;
#endif
    }

    n = send(sockfd, b->send_pos, b->last - b->send_pos, 0);
    if (n > 0) {
        b->send_pos += n;
    }
    return n;
}

static inline mln_chain_t *
mln_tcp_conn_pop(mln_tcp_conn_t *tc, int type)
{
    mln_chain_t **head, **tail;
    if (type == M_C_SEND) {
        head = &(tc->snd_head);
        tail = &(tc->snd_tail);
    } else if (type == M_C_RECV) {
        head = &(tc->rcv_head);
        tail = &(tc->rcv_tail);
    } else if (type == M_C_SENT) {
        head = &(tc->sent_head);
        tail = &(tc->sent_tail);
    } else {
        mln_log(error, "flag error.\n");
        abort();
    }

    mln_chain_t *rc = *head;
    if (rc == *tail) {
        *head = *tail = NULL;
        return rc;
    }

    *head = rc->next;
    rc->next = NULL;
    return rc;
}

int mln_tcp_conn_recv(mln_tcp_conn_t *tc, mln_u32_t flag)
{
    if (flag != M_C_TYPE_MEMORY && flag != M_C_TYPE_FILE) {
        mln_log(error, "Flag error.\n");
        abort();
    }

    int n, nonblock = 0;
    nonblock = mln_fd_is_nonblock(tc->sockfd);

    if (nonblock) {
goon_non:
        while ((n = mln_tcp_conn_recv_chain(tc, flag)) > 0) {
            /*do nothing*/
        }
    } else {
goon_blk:
        if ((n = mln_tcp_conn_recv_chain(tc, flag)) > 0) {
            return M_C_NOTYET;
        }
    }

    if (n == 0) {
        return M_C_CLOSED;
    }

    if (errno == EINTR) {
        if (nonblock) {
            goto goon_non;
        } else {
            goto goon_blk;
        }
    } else if (errno == EAGAIN) {
        return M_C_NOTYET;
    }
    return M_C_ERROR;
}

static inline int
mln_tcp_conn_recv_chain(mln_tcp_conn_t *tc, mln_u32_t flag)
{
    mln_buf_t *last = NULL;
    int n = -1;
    mln_buf_t *b;
    mln_chain_t *c;

    c = mln_chain_new(tc->pool);
    b = mln_buf_new(tc->pool);
    if (c == NULL || b == NULL) {
        errno = ENOMEM;
        return -1;
    }
    c->buf = b;

    if (flag == M_C_TYPE_FILE) {
fil:
        n = mln_tcp_conn_recv_chain_file(tc->sockfd, tc->pool, b, last);
    } else if (flag == M_C_TYPE_MEMORY) {
mem:
        n = mln_tcp_conn_recv_chain_mem(tc->sockfd, tc->pool, b);
    } else {
        if (tc->rcv_tail == NULL || tc->rcv_tail->buf == NULL) {
            mln_log(error, "Flag error. shouldn't be M_C_TYPE_FOLLOW, nothing can be followed.\n");
            abort();
        }
        last = tc->rcv_head->buf;
        if (last->in_file) {
            goto fil;
        } else {
            goto mem;
        }
    }

    if (n <= 0) {
        mln_chain_pool_release(c);
    } else {
        mln_tcp_conn_set(tc, c, M_C_RECV);
    }

    return n;
}

static inline int
mln_tcp_conn_recv_chain_file(int sockfd, \
                             mln_alloc_t *pool, \
                             mln_buf_t *b, \
                             mln_buf_t *last)
{
    int n;
    mln_u8_t buf[1024];

    n = recv(sockfd, buf, sizeof(buf), 0);
    if (n <= 0) return n;

    if (last == NULL) {
        if ((b->file = mln_file_tmp_open(pool)) == NULL) {
            return -1;
        }
        b->file_pos = 0;
    } else {
        b->file_pos = last->file_last;
        b->file = last->file;
        last->shadow = b;
    }
    b->file_last = b->file_pos + n;
    b->in_file = 1;
    b->last_buf = 1;

    if (write(mln_file_fd(b->file), buf, n) < 0) {
        return -1;
    }

    return n;
}

static inline int
mln_tcp_conn_recv_chain_mem(int sockfd, mln_alloc_t *pool, mln_buf_t *b)
{
    mln_u8ptr_t buf;
    int n;

    buf = (mln_u8ptr_t)mln_alloc_m(pool, 1024);
    if (buf == NULL) {
        errno = ENOMEM;
        return -1;
    }

    n = recv(sockfd, buf, 1024, 0);
    if (n <= 0) {
        mln_alloc_free(buf);
        return n;
    }

    b->pos = buf;
    b->last = buf + n;
    b->in_memory = 1;
    b->last_buf = 1;

    return n;
}

