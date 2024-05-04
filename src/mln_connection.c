
/*
 * Copyright (C) Niklaus F.Schen.
 */
#include "mln_connection.h"
#include <stdio.h>
#include <stdlib.h>
#if !defined(MSVC)
#include <unistd.h>
#endif
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#if defined(MSVC)
#include "mln_utils.h"
#else
#include <sys/time.h>
#endif
#include <sys/stat.h>
#include <sys/types.h>
#include "mln_utils.h"
#include "mln_file.h"
#include "mln_func.h"
#if defined(MLN_WRITEV)
#include <sys/uio.h>
#endif
#if defined(MLN_SENDFILE)
#include <sys/sendfile.h>
#endif


static inline int mln_fd_is_nonblock(int fd);
static inline mln_chain_t *
mln_tcp_conn_pop_inline(mln_tcp_conn_t *tc, int type);
static inline int
mln_tcp_conn_recv_chain(mln_tcp_conn_t *tc, mln_u32_t flag);
static inline int
mln_tcp_conn_recv_chain_file(int sockfd, \
                             mln_alloc_t *pool, \
                             mln_buf_t *b, \
                             mln_buf_t *last);
static inline int
mln_tcp_conn_recv_chain_mem(int sockfd, mln_alloc_t *pool, mln_buf_t *b);
static inline int
mln_tcp_conn_send_chain_memory(mln_tcp_conn_t *tc);
static inline int
mln_tcp_conn_send_chain_file(mln_tcp_conn_t *tc);


static inline int mln_fd_is_nonblock(int fd)
{
#if defined(MSVC)
    return 0; /* no useful API for getting this flag from socket */
#else
    int flg = fcntl(fd, F_GETFL, NULL);
    ASSERT(flg >= 0);
    return flg & O_NONBLOCK;
#endif
}


/*
 * mln_tcp_conn_t
 */

MLN_FUNC(, int, mln_tcp_conn_init, (mln_tcp_conn_t *tc, int sockfd), (tc, sockfd), {
    tc->pool = mln_alloc_init(NULL);
    if (tc->pool == NULL) return -1;
    tc->rcv_head = tc->rcv_tail = NULL;
    tc->snd_head = tc->snd_tail = NULL;
    tc->sent_head = tc->sent_tail = NULL;
    tc->sockfd = sockfd;
    return 0;
})

MLN_FUNC_VOID(, void, mln_tcp_conn_destroy, (mln_tcp_conn_t *tc), (tc), {
    if (tc == NULL) return;

    mln_chain_pool_release_all(mln_tcp_conn_remove(tc, M_C_SEND));
    mln_chain_pool_release_all(mln_tcp_conn_remove(tc, M_C_RECV));
    mln_chain_pool_release_all(mln_tcp_conn_remove(tc, M_C_SENT));
    mln_alloc_destroy(tc->pool);
})

MLN_FUNC_VOID(, void, mln_tcp_conn_append_chain, \
              (mln_tcp_conn_t *tc, mln_chain_t *c_head, mln_chain_t *c_tail, int type), \
              (tc, c_head, c_tail, type), \
{
    if (c_head == NULL) return;

    mln_chain_t **head = NULL, **tail = NULL;
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
        ASSERT(0);
    }

    if (c_tail == NULL) {
        for (c_tail = c_head; c_tail->next != NULL; c_tail = c_tail->next)
            ;
    }
    if (*head == NULL) {
        *head = c_head;
        *tail = c_tail;
    } else {
        (*tail)->next = c_head;
        *tail = c_tail;
    }
})

MLN_FUNC_VOID(, void, mln_tcp_conn_append, (mln_tcp_conn_t *tc, mln_chain_t *c, int type), (tc, c, type), {
    mln_chain_t **head = NULL, **tail = NULL;
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
        ASSERT(0);
    }

    if (*head == NULL) {
        *head = *tail = c;
    } else {
        (*tail)->next = c;
        *tail = c;
    }
})

MLN_FUNC(, mln_chain_t *, mln_tcp_conn_head, (mln_tcp_conn_t *tc, int type), (tc, type), {
    mln_chain_t *rc = NULL;

    if (type == M_C_SEND) {
        rc = tc->snd_head;
    } else if (type == M_C_RECV) {
        rc = tc->rcv_head;
    } else if (type == M_C_SENT) {
        rc = tc->sent_head;
    } else {
        ASSERT(0);
    }

    return rc;
})

MLN_FUNC(, mln_chain_t *, mln_tcp_conn_remove, (mln_tcp_conn_t *tc, int type), (tc, type), {
    mln_chain_t *rc = NULL;

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
        ASSERT(0);
    }

    return rc;
})

MLN_FUNC(, mln_chain_t *, mln_tcp_conn_pop, (mln_tcp_conn_t *tc, int type), (tc, type), {
    mln_chain_t **head = NULL, **tail = NULL;
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
        ASSERT(0);
    }

    mln_chain_t *rc = *head;
    if (rc == *tail) {
        *head = *tail = NULL;
        return rc;
    }

    *head = rc->next;
    rc->next = NULL;
    return rc;
})

MLN_FUNC(, mln_chain_t *, mln_tcp_conn_tail, (mln_tcp_conn_t *tc, int type), (tc, type), {
    mln_chain_t *rc = NULL;

    if (type == M_C_SEND) {
        rc = tc->snd_tail;
    } else if (type == M_C_RECV) {
        rc = tc->rcv_tail;
    } else if (type == M_C_SENT) {
        rc = tc->sent_tail;
    } else {
        ASSERT(0);
    }

    return rc;
})

MLN_FUNC(, int, mln_tcp_conn_send, (mln_tcp_conn_t *tc), (tc), {
    int n;

    if (tc->snd_head == NULL) return M_C_NOTYET;

me:
    n = mln_tcp_conn_send_chain_memory(tc);
    if (n == 0 && \
        tc->snd_head != NULL && \
        tc->snd_head->buf != NULL && \
        tc->snd_head->buf->in_file)
    {
        goto fi;
    }
    if (n == 0) return M_C_NOTYET;
    if (n > 0) return M_C_FINISH;
    return M_C_ERROR;

fi:
    n = mln_tcp_conn_send_chain_file(tc);
    if (n == 0 && \
        tc->snd_head != NULL && \
        tc->snd_head->buf != NULL && \
        tc->snd_head->buf->in_memory)
    {
        goto me;
    }
    if (n == 0) return M_C_NOTYET;
    if (n > 0) return M_C_FINISH;
    return M_C_ERROR;
})


#if defined(MLN_WRITEV)
MLN_FUNC(static inline, int, mln_tcp_conn_send_chain_memory, (mln_tcp_conn_t *tc), (tc), {
    mln_chain_t *c;
    mln_buf_t *b;
    int n, is_done = 0;
    register mln_size_t buf_left_size;
    int proc_vec, nvec = 256;
    struct iovec vector[256];

    if (mln_fd_is_nonblock(tc->sockfd)) {
        while (1) {
            proc_vec = 0;
            for (c = tc->snd_head; c != NULL; c = c->next) {
                if (proc_vec >= nvec) break;
                if ((b = c->buf) == NULL) continue;
                if (!b->in_memory) break;
                buf_left_size = mln_buf_left_size(b);
                if (buf_left_size) {
                    vector[proc_vec].iov_base = b->left_pos;
                    vector[proc_vec].iov_len = buf_left_size;
                    ++proc_vec;
                }
                if (b->last_in_chain) break;
            }

            if (!proc_vec) {
                if (tc->snd_head != NULL) mln_chain_pool_release_all(tc->snd_head);
                tc->snd_head = tc->snd_tail = NULL;
                return 0;
            }

non:
            n = writev(tc->sockfd, vector, proc_vec);
            if (n <= 0) {
                if (errno == EINTR) goto non;
                if (errno == EAGAIN) return 0;
                return -1;
            }

            while ((c = tc->snd_head) != NULL) {
                if ((b = c->buf) == NULL) {
                    c = mln_tcp_conn_pop_inline(tc, M_C_SEND);
                    mln_tcp_conn_append(tc, c, M_C_SENT);
                    continue;
                }
                if (!b->in_memory) break;
                if (b->last_in_chain) is_done = 1;

                buf_left_size = mln_buf_left_size(b);
                if (n >= buf_left_size) {
                    b->left_pos += buf_left_size;
                    n -= buf_left_size;
                    c = mln_tcp_conn_pop_inline(tc, M_C_SEND);
                    mln_tcp_conn_append(tc, c, M_C_SENT);
                } else {
                    b->left_pos += n;
                    n = 0;
                }
                if (is_done || n == 0) break;
            }

            if (is_done) break;
        }
        return 1;
    }

    proc_vec = 0;
    for (c = tc->snd_head; c != NULL; c = c->next) {
        if (proc_vec >= nvec) break;
        if ((b = c->buf) == NULL) continue;
        if (!b->in_memory) break;
        buf_left_size = mln_buf_left_size(b);
        if (buf_left_size) {
            vector[proc_vec].iov_base = b->left_pos;
            vector[proc_vec].iov_len = buf_left_size;
            ++proc_vec;
        }
        if (b->last_in_chain) break;
    }

    if (!proc_vec) {
        if (tc->snd_head != NULL) mln_chain_pool_release_all(tc->snd_head);
        tc->snd_head = tc->snd_tail = NULL;
        return 0;
    }

blk:
    n = writev(tc->sockfd, vector, proc_vec);
    if (n <= 0) {
        if (errno == EINTR) goto blk;
        return -1;
    }

    while ((c = tc->snd_head) != NULL) {
        if ((b = c->buf) == NULL) {
            c = mln_tcp_conn_pop_inline(tc, M_C_SEND);
            mln_tcp_conn_append(tc, c, M_C_SENT);
            continue;
        }
        if (!b->in_memory) break;
        if (b->last_in_chain) is_done = 1;

        buf_left_size = mln_buf_left_size(b);
        if (n >= buf_left_size) {
            b->left_pos += buf_left_size;
            n -= buf_left_size;
            c = mln_tcp_conn_pop_inline(tc, M_C_SEND);
            mln_tcp_conn_append(tc, c, M_C_SENT);
        } else {
            b->left_pos += n;
            n = 0;
        }
        if (is_done || n == 0) break;
    }

    return is_done;
})
#else
static inline int mln_tcp_conn_send_chain_memory(mln_tcp_conn_t *tc)
{
    mln_u8_t buf[8192], *p;
    mln_chain_t *c;
    mln_buf_t *b;
    mln_size_t left_size;
    register mln_size_t buf_left_size;
    int n, is_done = 0;

    if (mln_fd_is_nonblock(tc->sockfd)) {
        while (1) {
            p = buf;
            left_size = sizeof(buf);

            for (c = tc->snd_head; c != NULL; c = c->next) {
                if ((b = c->buf) == NULL) continue;
                if (!b->in_memory) break;
                buf_left_size = mln_buf_left_size(b);

                if (buf_left_size > left_size) {
                    memcpy(p, b->left_pos, left_size);
                    p += left_size;
                    left_size = 0; 
                    break;
                } else {
                    if (buf_left_size > 0) {
                        memcpy(p, b->left_pos, buf_left_size);
                        p += buf_left_size;
                        left_size -= buf_left_size;
                    }
                    if (b->last_in_chain) break;
                }
            }

            if (left_size == sizeof(buf)) return 0;

non:
#if defined(MSVC)
            n = send(tc->sockfd, (char *)buf, sizeof(buf) - left_size, 0);
#else
            n = send(tc->sockfd, buf, sizeof(buf) - left_size, 0);
#endif
            if (n <= 0) {
                if (errno == EINTR) goto non;
                if (errno == EAGAIN) return 0;
                return -1;
            }

            while ((c = tc->snd_head) != NULL) {
                if ((b = c->buf) == NULL) {
                    c = mln_tcp_conn_pop_inline(tc, M_C_SEND);
                    mln_tcp_conn_append(tc, c, M_C_SENT);
                    continue;
                }
                buf_left_size = mln_buf_left_size(b);
                if (buf_left_size > n) {
                    b->left_pos += n;
                    n = 0;
                } else {
                    if (b->last_in_chain) is_done = 1;
                    n -= buf_left_size;
                    b->left_pos += buf_left_size;
                    c = mln_tcp_conn_pop_inline(tc, M_C_SEND);
                    mln_tcp_conn_append(tc, c, M_C_SENT);
                }
                if (is_done || n == 0) break;
            }

            if (is_done) break;
        }

        return 1;
    }

    p = buf;
    left_size = sizeof(buf);

    for (c = tc->snd_head; c != NULL; c = c->next) {
        if ((b = c->buf) == NULL) continue;
        if (!b->in_memory) break;
        buf_left_size = mln_buf_left_size(b);

        if (buf_left_size > left_size) {
            memcpy(p, b->left_pos, left_size);
            p += left_size;
            left_size = 0;
            break;
        } else {
            if (buf_left_size > 0) {
                memcpy(p, b->left_pos, buf_left_size);
                p += buf_left_size;
                left_size -= buf_left_size;
            }
            if (b->last_in_chain) break;
        }
    }

    if (left_size == sizeof(buf)) return 0;

blk:
#if defined(MSVC)
    n = send(tc->sockfd, (char *)buf, sizeof(buf) - left_size, 0);
#else
    n = send(tc->sockfd, buf, sizeof(buf) - left_size, 0);
#endif
    if (n <= 0) {
        if (errno == EINTR) goto blk;
        return -1;
    }

    while ((c = tc->snd_head) != NULL) {
        if ((b = c->buf) == NULL) {
            c = mln_tcp_conn_pop_inline(tc, M_C_SEND);
            mln_tcp_conn_append(tc, c, M_C_SENT);
            continue;
        }
        buf_left_size = mln_buf_left_size(b);
        if (buf_left_size > n) {
            b->left_pos += n;
            n = 0;
        } else {
            if (b->last_in_chain) is_done = 1;
            n -= buf_left_size;
            b->left_pos += buf_left_size;
            c = mln_tcp_conn_pop_inline(tc, M_C_SEND);
            mln_tcp_conn_append(tc, c, M_C_SENT);
        }
        if (is_done || n == 0) break;
    }

    return is_done;
}
#endif


#if defined(MLN_SENDFILE)
MLN_FUNC(static inline, int, mln_tcp_conn_send_chain_file, (mln_tcp_conn_t *tc), (tc), {
    int sockfd = tc->sockfd;
    int n, is_done = 0;
    mln_chain_t *c;
    mln_buf_t *b;
    mln_size_t buf_left_size;

    if (mln_fd_is_nonblock(sockfd)) {
        while ((c = tc->snd_head) != NULL) {
            if ((b = c->buf) == NULL) {
                c = mln_tcp_conn_pop_inline(tc, M_C_SEND);
                mln_tcp_conn_append(tc, c, M_C_SENT);
                continue;
            }
            if (!b->in_file) break;
            buf_left_size = mln_buf_left_size(b);
            if (b->last_in_chain) is_done = 1;

            if (buf_left_size) {
non:
                n = sendfile(sockfd, \
                             mln_file_fd(b->file), \
                             &b->file_left_pos, \
                             buf_left_size);
                if (n <= 0) {
                    if (errno == EINTR) goto non;
                    if (errno == EAGAIN) return 0;
                    return -1;
                }

                if (mln_buf_left_size(b)) {
                    goto non;
                }
            }
            c = mln_tcp_conn_pop_inline(tc, M_C_SEND);
            mln_tcp_conn_append(tc, c, M_C_SENT);
            if (is_done) break;
        }
        return 1;
    }

    while ((c = tc->snd_head) != NULL) {
        if ((b = c->buf) != NULL) {
            if (mln_buf_left_size(b)) break;
            if (b->last_in_chain) is_done = 1;
        }
        c = mln_tcp_conn_pop_inline(tc, M_C_SEND);
        mln_tcp_conn_append(tc, c, M_C_SENT);
        if (is_done) return 1;
    }
    if (tc->snd_head == NULL) return 0;
    if (!b->in_file) return 0;

blk:
    n = sendfile(sockfd, \
                 mln_file_fd(b->file), \
                 &b->file_left_pos, \
                 mln_buf_left_size(b));
    if (n <= 0) {
        if (errno == EINTR) goto blk;
        return -1;
    }

    if (mln_buf_left_size(b)) goto blk;
    if (b->last_in_chain) is_done = 1;
    c = mln_tcp_conn_pop_inline(tc, M_C_SEND);
    mln_tcp_conn_append(tc, c, M_C_SENT);

    return is_done;
})
#else
static inline int mln_tcp_conn_send_chain_file(mln_tcp_conn_t *tc)
{
    int sockfd = tc->sockfd;
    int n;
    mln_buf_t *b;
    mln_chain_t *c;
    mln_u8_t buf[4096];
    mln_size_t len, buf_left_size;

    if (mln_fd_is_nonblock(sockfd)) {
        while ((c = tc->snd_head) != NULL) {
            if ((b = c->buf) == NULL) {
                c = mln_tcp_conn_pop_inline(tc, M_C_SEND);
                mln_tcp_conn_append(tc, c, M_C_SENT);
                continue;
            }
            if (!b->in_file) break;

            buf_left_size = mln_buf_left_size(b);
            if (buf_left_size == 0) {
                c = mln_tcp_conn_pop_inline(tc, M_C_SEND);
                mln_tcp_conn_append(tc, c, M_C_SENT);
                if (b->last_in_chain) return 1;
                continue;
            }

            lseek(mln_file_fd(b->file), b->file_left_pos, SEEK_SET);
            len = buf_left_size > sizeof(buf)? sizeof(buf): buf_left_size;
non_rd:
            n = read(mln_file_fd(b->file), buf, len);
            if (n <= 0) {
                if (errno == EINTR) goto non_rd;
                return -1;
            }

            len = n;
non_snd:
#if defined(MSVC)
            n = send(sockfd, (char *)buf, len, 0);
#else
            n = send(sockfd, buf, len, 0);
#endif
            if (n <= 0) {
                if (errno == EINTR) goto non_snd;
                if (errno == EAGAIN) return 0;
                return -1;
            }
            b->file_left_pos += n;
            if (mln_buf_left_size(b)) continue;

            c = mln_tcp_conn_pop_inline(tc, M_C_SEND);
            mln_tcp_conn_append(tc, c, M_C_SENT);
            if (b->last_in_chain) return 1;
        }
        return 0;
    }

    while ((c = tc->snd_head) != NULL) {
        if ((b = c->buf) == NULL) {
            c = mln_tcp_conn_pop_inline(tc, M_C_SEND);
            mln_tcp_conn_append(tc, c, M_C_SENT);
            continue;
        }
        if (!b->in_file) return 0;

        buf_left_size = mln_buf_left_size(b);
        if (buf_left_size == 0) {
            c = mln_tcp_conn_pop_inline(tc, M_C_SEND);
            mln_tcp_conn_append(tc, c, M_C_SENT);
            if (b->last_in_chain) return 1;
            continue;
        }

        break;
    }
    if (c == NULL) return 0;

    lseek(mln_file_fd(b->file), b->file_left_pos, SEEK_SET);
    len = buf_left_size > sizeof(buf)? sizeof(buf): buf_left_size;
blk_rd:
    n = read(mln_file_fd(b->file), buf, len);
    if (n <= 0) {
        if (errno == EINTR) goto blk_rd;
        return -1;
    }

    len = n;
blk_snd:
#if defined(MSVC)
    n = send(sockfd, (char *)buf, len, 0);
#else
    n = send(sockfd, buf, len, 0);
#endif
    if (n <= 0) {
        if (errno == EINTR) goto blk_snd;
        return -1;
    }
    b->file_left_pos += n;
    if (mln_buf_left_size(b)) return 0;

    c = mln_tcp_conn_pop_inline(tc, M_C_SEND);
    mln_tcp_conn_append(tc, c, M_C_SENT);

    return b->last_in_chain == 0? 0: 1;
}
#endif

MLN_FUNC(static inline, mln_chain_t *, mln_tcp_conn_pop_inline, \
         (mln_tcp_conn_t *tc, int type), (tc, type), \
{
    mln_chain_t **head = NULL, **tail = NULL;
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
        ASSERT(0);
    }

    mln_chain_t *rc = *head;
    if (rc == *tail) {
        *head = *tail = NULL;
        return rc;
    }

    *head = rc->next;
    rc->next = NULL;
    return rc;
})

MLN_FUNC(, int, mln_tcp_conn_recv, (mln_tcp_conn_t *tc, mln_u32_t flag), (tc, flag), {
    ASSERT(flag == M_C_TYPE_MEMORY || flag == M_C_TYPE_FILE);

    int n;

    if (mln_fd_is_nonblock(tc->sockfd)) {
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
        if (mln_fd_is_nonblock(tc->sockfd)) {
            goto goon_non;
        } else {
            goto goon_blk;
        }
    } else if (errno == EAGAIN) {
        return M_C_NOTYET;
    }
    return M_C_ERROR;
})

MLN_FUNC(static inline, int, mln_tcp_conn_recv_chain, \
         (mln_tcp_conn_t *tc, mln_u32_t flag), (tc, flag), \
{
    mln_buf_t *last = NULL;
    int n = -1;
    mln_buf_t *b;
    mln_chain_t *c;
    mln_alloc_t *pool = mln_tcp_conn_pool_get(tc);

    c = mln_chain_new(pool);
    b = mln_buf_new(pool);
    if (c == NULL || b == NULL) {
        errno = ENOMEM;
        return -1;
    }
    c->buf = b;

    if (flag & M_C_TYPE_FILE) {
        if (flag & M_C_TYPE_FOLLOW && tc->rcv_tail != NULL && tc->rcv_tail->buf != NULL) {
            last = tc->rcv_tail->buf;
            if (!last->in_file) {
                last = NULL;
            }
        }
        n = mln_tcp_conn_recv_chain_file(tc->sockfd, pool, b, last);
    } else if (flag & M_C_TYPE_MEMORY) {
        n = mln_tcp_conn_recv_chain_mem(tc->sockfd, pool, b);
    } else {
        ASSERT(0);
    }

    if (n <= 0) {
        mln_chain_pool_release(c);
    } else {
        mln_tcp_conn_append(tc, c, M_C_RECV);
    }

    return n;
})

static inline int mln_tcp_conn_recv_chain_file(int sockfd, mln_alloc_t *pool, mln_buf_t *b, mln_buf_t *last)
{
    int n;
    mln_u8_t buf[1024];

#if defined(MSVC)
    n = recv(sockfd, (char *)buf, sizeof(buf), 0);
#else
    n = recv(sockfd, buf, sizeof(buf), 0);
#endif
    if (n <= 0) return n;

    if (last == NULL) {
        if ((b->file = mln_file_tmp_open(pool)) == NULL) {
            return -1;
        }
        b->file_left_pos = b->file_pos = 0;
    } else {
        b->file_left_pos = b->file_pos = last->file_last;
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

static inline int mln_tcp_conn_recv_chain_mem(int sockfd, mln_alloc_t *pool, mln_buf_t *b)
{
    mln_u8ptr_t buf;
    int n;

    buf = (mln_u8ptr_t)mln_alloc_m(pool, 1024);
    if (buf == NULL) {
        errno = ENOMEM;
        return -1;
    }

#if defined(MSVC)
    n = recv(sockfd, (char *)buf, 1024, 0);
#else
    n = recv(sockfd, buf, 1024, 0);
#endif
    if (n <= 0) {
        mln_alloc_free(buf);
        return n;
    }

    b->left_pos = b->pos = b->start = buf;
    b->last = b->end = buf + n;
    b->in_memory = 1;
    b->last_buf = 1;

    return n;
}

