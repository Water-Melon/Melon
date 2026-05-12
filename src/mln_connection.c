
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
#if defined(MLN_TLS)
#if !defined(MSVC)
#include <pthread.h>
#endif
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/bio.h>
#include <openssl/x509v3.h>
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
#if defined(MLN_TLS)
static int mln_tcp_conn_send_tls(mln_tcp_conn_t *tc);
static int mln_tcp_conn_recv_tls(mln_tcp_conn_t *tc);
static int mln_tcp_conn_tls_flush_wbio(mln_tcp_conn_t *tc);
static int mln_tcp_conn_tls_drain_socket(mln_tcp_conn_t *tc);
#endif


static inline int mln_fd_is_nonblock(int fd)
{
#if defined(MSVC)
    return 0; /* no useful API for getting this flag from socket */
#else
    if (fd < 0) return 0;
    int flg = fcntl(fd, F_GETFL, 0);
    if (flg < 0) return 0;
    return (flg & O_NONBLOCK) != 0;
#endif
}


/*
 * mln_tcp_conn_t
 */

MLN_FUNC(, int, mln_tcp_conn_init, (mln_tcp_conn_t *tc, int sockfd), (tc, sockfd), {
    tc->pool = mln_alloc_init(NULL, 0);
    if (tc->pool == NULL) return -1;
    tc->rcv_head = tc->rcv_tail = NULL;
    tc->snd_head = tc->snd_tail = NULL;
    tc->sent_head = tc->sent_tail = NULL;
    tc->sockfd = sockfd;
    tc->nonblock = mln_fd_is_nonblock(sockfd);
#if defined(MLN_TLS)
    tc->ssl = NULL;
    tc->tls_conf = NULL;
    tc->tls_pending = NULL;
    tc->tls_pending_off = 0;
    tc->tls_pending_len = 0;
    tc->tls_done = 0;
    tc->tls_want_r = 0;
    tc->tls_want_w = 0;
    tc->tls_shut = 0;
#endif
    return 0;
})

MLN_FUNC_VOID(, void, mln_tcp_conn_fd_set, (mln_tcp_conn_t *tc, int fd), (tc, fd), {
    tc->sockfd = fd;
    tc->nonblock = mln_fd_is_nonblock(fd);
})

MLN_FUNC(, int, mln_tcp_conn_set_nonblock, (mln_tcp_conn_t *tc, int nb), (tc, nb), {
#if defined(MSVC)
    tc->nonblock = 0;
    return 0;
#else
    if (tc->sockfd < 0) {
        tc->nonblock = nb ? 1 : 0;
        return 0;
    }
    int flg = fcntl(tc->sockfd, F_GETFL, 0);
    if (flg < 0) return -1;
    if (nb) {
        if (fcntl(tc->sockfd, F_SETFL, flg | O_NONBLOCK) < 0) return -1;
        tc->nonblock = 1;
    } else {
        if (fcntl(tc->sockfd, F_SETFL, flg & ~O_NONBLOCK) < 0) return -1;
        tc->nonblock = 0;
    }
    return 0;
#endif
})

MLN_FUNC_VOID(, void, mln_tcp_conn_destroy, (mln_tcp_conn_t *tc), (tc), {
    if (tc == NULL) return;

#if defined(MLN_TLS)
    if (tc->ssl != NULL) {
        /* SSL_free releases the SSL object and any BIO attached to it. */
        SSL_free(tc->ssl);
        tc->ssl = NULL;
    }
#endif
    mln_chain_pool_release_all(mln_tcp_conn_remove(tc, M_C_SEND));
    mln_chain_pool_release_all(mln_tcp_conn_remove(tc, M_C_RECV));
    mln_chain_pool_release_all(mln_tcp_conn_remove(tc, M_C_SENT));
    if (tc->pool) mln_alloc_destroy(tc->pool);
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

#if defined(MLN_TLS)
    if (tc->ssl != NULL) return mln_tcp_conn_send_tls(tc);
#endif

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

    if (tc->nonblock) {
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
    mln_u8_t buf[16384], *p;
    mln_chain_t *c;
    mln_buf_t *b;
    mln_size_t left_size;
    register mln_size_t buf_left_size;
    int n, is_done = 0;

    if (tc->nonblock) {
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

    if (tc->nonblock) {
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
    mln_u8_t buf[16384];
    mln_size_t len, buf_left_size;

    if (tc->nonblock) {
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

MLN_FUNC_VOID(, void, mln_tcp_conn_move_sent, (mln_tcp_conn_t *tc), (tc), {
    if (tc->snd_head == NULL) return;

    if (tc->sent_tail == NULL) {
        tc->sent_head = tc->snd_head;
        tc->sent_tail = tc->snd_tail;
    } else {
        tc->sent_tail->next = tc->snd_head;
        tc->sent_tail = tc->snd_tail;
    }
    tc->snd_head = tc->snd_tail = NULL;
})

MLN_FUNC(, int, mln_tcp_conn_send_chain, (mln_tcp_conn_t *tc, mln_chain_t *chain), (tc, chain), {
    mln_tcp_conn_append_chain(tc, chain, NULL, M_C_SEND);
    return mln_tcp_conn_send(tc);
})

MLN_FUNC(, int, mln_tcp_conn_recv, (mln_tcp_conn_t *tc, mln_u32_t flag), (tc, flag), {
    ASSERT(flag == M_C_TYPE_MEMORY || flag == M_C_TYPE_FILE);

#if defined(MLN_TLS)
    if (tc->ssl != NULL) {
        /* TLS records always require an intermediate memory buffer for
         * decryption; the file-receive path is intentionally unavailable.
         */
        if (flag != M_C_TYPE_MEMORY) {
            errno = EINVAL;
            return M_C_ERROR;
        }
        return mln_tcp_conn_recv_tls(tc);
    }
#endif

    int n;

    if (tc->nonblock) {
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
        if (tc->nonblock) {
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
    mln_u8_t buf[8192];

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

    buf = (mln_u8ptr_t)mln_alloc_m(pool, 4096);
    if (buf == NULL) {
        errno = ENOMEM;
        return -1;
    }

#if defined(MSVC)
    n = recv(sockfd, (char *)buf, 4096, 0);
#else
    n = recv(sockfd, buf, 4096, 0);
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


#if defined(MLN_TLS)
/* ------------------------------------------------------------------ *
 *                              TLS path                              *
 *                                                                    *
 * The plain mln_tcp_conn_send / mln_tcp_conn_recv paths above remain *
 * unchanged for sockets that were initialized through                *
 * mln_tcp_conn_init().  A second initializer, mln_tcp_conn_tls_init, *
 * attaches an SSL handle plus two memory BIOs (rbio / wbio):         *
 *                                                                    *
 *     plaintext  -->  SSL_write  -->  wbio  -->  send(sockfd)        *
 *     recv(sockfd)  -->  rbio  -->  SSL_read  -->  plaintext         *
 *                                                                    *
 * The plaintext lands in rcv_* / snd_* as usual; the ciphertext is   *
 * only ever held by the BIOs (and a 16 KiB on-stack buffer while it  *
 * is being moved between BIO and the socket).  Because ciphertext is *
 * never queued into any mln_chain_t, there is no risk of the rcv_*   *
 * list mixing decrypted bytes with bytes that still have to be       *
 * decrypted.                                                         *
 * ------------------------------------------------------------------ */

#define M_TLS_CHUNK 16384


static void mln_tcp_tls_apply_versions(SSL_CTX *ctx, mln_u32_t versions)
{
    if (versions == 0) versions = M_TLS_VDEFAULT;
    int min_v = 0, max_v = 0;
    if (versions & M_TLS_V1_2) min_v = TLS1_2_VERSION;
    if (versions & M_TLS_V1_3) {
#ifdef TLS1_3_VERSION
        if (min_v == 0) min_v = TLS1_3_VERSION;
        max_v = TLS1_3_VERSION;
#else
        /* TLS 1.3 requested but this OpenSSL build doesn't support it.
         * Fall back to TLS 1.2 so we don't silently negotiate older
         * versions: treat M_TLS_V1_3-only as "at least TLS 1.2". */
        if (min_v == 0) min_v = TLS1_2_VERSION;
        max_v = TLS1_2_VERSION;
#endif
    } else {
        max_v = TLS1_2_VERSION;
    }
    if (min_v) SSL_CTX_set_min_proto_version(ctx, min_v);
    if (max_v) SSL_CTX_set_max_proto_version(ctx, max_v);
}

MLN_FUNC(, mln_tcp_tls_conf_t *, mln_tcp_tls_conf_new, \
         (mln_u32_t role, mln_string_t *cert_file, mln_string_t *key_file, \
          mln_string_t *ca_file, mln_string_t *ciphers, \
          mln_u32_t versions, mln_u32_t verify), \
         (role, cert_file, key_file, ca_file, ciphers, versions, verify), \
{
    mln_tcp_tls_conf_t *c;
    const SSL_METHOD   *method;

    /* Reject anything outside the documented role enum up front -- otherwise
     * we'd pick the client method for an unknown value but later set the
     * stored role flag to M_TLS_SERVER, and SSL_set_accept_state() would be
     * applied to a client-method SSL.  Be strict here. */
    if (role != M_TLS_SERVER && role != M_TLS_CLIENT) {
        errno = EINVAL;
        return NULL;
    }
    /* Reject unknown bits in the version mask so we never store a
     * value that would mislead future readers of conf->versions. */
    if (versions != 0 && (versions & ~(mln_u32_t)M_TLS_VDEFAULT) != 0) {
        errno = EINVAL;
        return NULL;
    }

    if (role == M_TLS_SERVER) {
        if (cert_file == NULL || cert_file->len == 0 ||
            key_file  == NULL || key_file->len  == 0) {
            errno = EINVAL;
            return NULL;
        }
        method = TLS_server_method();
    } else {
        method = TLS_client_method();
    }

    c = (mln_tcp_tls_conf_t *)calloc(1, sizeof(*c));
    if (c == NULL) { errno = ENOMEM; return NULL; }

    c->ctx = SSL_CTX_new(method);
    if (c->ctx == NULL) goto err;
    SSL_CTX_set_mode(c->ctx,
                     SSL_MODE_ENABLE_PARTIAL_WRITE |
                     SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER);
    mln_tcp_tls_apply_versions(c->ctx, versions);

    /* No silent truncation: mln_string_t is length-prefixed and may
     * legitimately exceed any fixed stack buffer; we allocate exactly
     * what is needed for the trailing NUL OpenSSL APIs require. */
    if (ciphers != NULL && ciphers->len > 0) {
        char *buf = (char *)malloc(ciphers->len + 1);
        if (buf == NULL) { errno = ENOMEM; goto err; }
        memcpy(buf, ciphers->data, ciphers->len);
        buf[ciphers->len] = '\0';
        int ok = SSL_CTX_set_cipher_list(c->ctx, buf);
        free(buf);
        if (!ok) goto err;
    }

    if (cert_file != NULL && cert_file->len > 0) {
        char *path = (char *)malloc(cert_file->len + 1);
        if (path == NULL) { errno = ENOMEM; goto err; }
        memcpy(path, cert_file->data, cert_file->len);
        path[cert_file->len] = '\0';
        int ok = (SSL_CTX_use_certificate_chain_file(c->ctx, path) == 1);
        free(path);
        if (!ok) goto err;
    }
    if (key_file != NULL && key_file->len > 0) {
        char *path = (char *)malloc(key_file->len + 1);
        if (path == NULL) { errno = ENOMEM; goto err; }
        memcpy(path, key_file->data, key_file->len);
        path[key_file->len] = '\0';
        int ok = (SSL_CTX_use_PrivateKey_file(c->ctx, path, SSL_FILETYPE_PEM) == 1);
        free(path);
        if (!ok) goto err;
        if (SSL_CTX_check_private_key(c->ctx) != 1) goto err;
    }
    if (ca_file != NULL && ca_file->len > 0) {
        char *path = (char *)malloc(ca_file->len + 1);
        if (path == NULL) { errno = ENOMEM; goto err; }
        memcpy(path, ca_file->data, ca_file->len);
        path[ca_file->len] = '\0';
        int ok = (SSL_CTX_load_verify_locations(c->ctx, path, NULL) == 1);
        free(path);
        if (!ok) goto err;
    }

    {
        int vmode = SSL_VERIFY_NONE;
        if (verify) {
            vmode = SSL_VERIFY_PEER;
            if (role == M_TLS_SERVER)
                vmode |= SSL_VERIFY_FAIL_IF_NO_PEER_CERT;
        }
        SSL_CTX_set_verify(c->ctx, vmode, NULL);
    }

    /* Deep-copy so the conf is self-contained: callers don't have to
     * keep their mln_string_t storage alive for the conf's lifetime.
     * Any dup failure tears the whole conf down. */
    if (cert_file != NULL && (c->cert_file = mln_string_dup(cert_file)) == NULL) goto err;
    if (key_file  != NULL && (c->key_file  = mln_string_dup(key_file))  == NULL) goto err;
    if (ca_file   != NULL && (c->ca_file   = mln_string_dup(ca_file))   == NULL) goto err;
    if (ciphers   != NULL && (c->ciphers   = mln_string_dup(ciphers))   == NULL) goto err;
    c->role      = role;  /* role was validated to be M_TLS_SERVER or M_TLS_CLIENT above */
    c->verify    = verify ? 1 : 0;
    c->versions  = versions ? versions : M_TLS_VDEFAULT;
    return c;

err:
    if (c->cert_file) mln_string_free(c->cert_file);
    if (c->key_file)  mln_string_free(c->key_file);
    if (c->ca_file)   mln_string_free(c->ca_file);
    if (c->ciphers)   mln_string_free(c->ciphers);
    if (c->ctx)       SSL_CTX_free(c->ctx);
    free(c);
    return NULL;
})

MLN_FUNC_VOID(, void, mln_tcp_tls_conf_free, (mln_tcp_tls_conf_t *conf), (conf), {
    if (conf == NULL) return;
    if (conf->cert_file) mln_string_free(conf->cert_file);
    if (conf->key_file)  mln_string_free(conf->key_file);
    if (conf->ca_file)   mln_string_free(conf->ca_file);
    if (conf->ciphers)   mln_string_free(conf->ciphers);
    if (conf->ctx)       SSL_CTX_free(conf->ctx);
    free(conf);
})

MLN_FUNC(, int, mln_tcp_conn_tls_init, \
         (mln_tcp_conn_t *tc, int sockfd, mln_tcp_tls_conf_t *conf), \
         (tc, sockfd, conf), \
{
    BIO *rbio = NULL, *wbio = NULL;

    /* Defensive: conf is annotated as required, but a partially-initialized
     * struct (NULL ctx) would otherwise segfault inside SSL_new.
     */
    if (conf == NULL || conf->ctx == NULL) {
        errno = EINVAL;
        return -1;
    }

    if (mln_tcp_conn_init(tc, sockfd) < 0) return -1;

    tc->ssl = SSL_new(conf->ctx);
    if (tc->ssl == NULL) goto err;
    rbio = BIO_new(BIO_s_mem());
    wbio = BIO_new(BIO_s_mem());
    if (rbio == NULL || wbio == NULL) goto err;
    /* mem BIOs must never return "no data" as a fatal EOF condition;
     * EOF should look like EAGAIN to OpenSSL while we wait for more
     * ciphertext to arrive on the socket.
     */
    BIO_set_mem_eof_return(rbio, -1);
    BIO_set_mem_eof_return(wbio, -1);
    SSL_set_bio(tc->ssl, rbio, wbio);
    rbio = wbio = NULL; /* now owned by SSL */

    if (conf->role == M_TLS_SERVER) SSL_set_accept_state(tc->ssl);
    else                            SSL_set_connect_state(tc->ssl);

    tc->tls_conf = conf;
    return 0;

err:
    if (rbio) BIO_free(rbio);
    if (wbio) BIO_free(wbio);
    if (tc->ssl) { SSL_free(tc->ssl); tc->ssl = NULL; }
    mln_chain_pool_release_all(mln_tcp_conn_remove(tc, M_C_SEND));
    mln_chain_pool_release_all(mln_tcp_conn_remove(tc, M_C_RECV));
    mln_chain_pool_release_all(mln_tcp_conn_remove(tc, M_C_SENT));
    if (tc->pool) mln_alloc_destroy(tc->pool);
    tc->pool = NULL;
    return -1;
})

MLN_FUNC(, int, mln_tcp_conn_tls_set_sni, \
         (mln_tcp_conn_t *tc, mln_string_t *hostname), (tc, hostname), \
{
    char buf[256];
    if (tc->ssl == NULL) { errno = EINVAL; return -1; }
    /* DNS names are bounded by RFC 1035 to 253 octets; anything longer
     * is either a bug or an attacker-controlled value, and silently
     * truncating would send an incorrect SNI value.  Reject up front. */
    if (hostname->len >= sizeof(buf)) { errno = EINVAL; return -1; }
    /* An embedded NUL would silently shorten the C-string sent as SNI,
     * making the verified name differ from what the caller intended. */
    if (memchr(hostname->data, '\0', hostname->len) != NULL) { errno = EINVAL; return -1; }
    memcpy(buf, hostname->data, hostname->len);
    buf[hostname->len] = '\0';
    if (SSL_set_tlsext_host_name(tc->ssl, buf) != 1) return -1;
    return 0;
})

MLN_FUNC(, int, mln_tcp_conn_tls_set_verify_host, \
         (mln_tcp_conn_t *tc, mln_string_t *hostname), (tc, hostname), \
{
    char buf[256];
    X509_VERIFY_PARAM *param;
    if (tc->ssl == NULL) { errno = EINVAL; return -1; }
    /* Same rationale as SNI: never verify a silently-truncated hostname,
     * that would let an attacker pass verification against a different
     * name than the caller intended. */
    if (hostname->len >= sizeof(buf)) { errno = EINVAL; return -1; }
    /* An embedded NUL would shorten the C-string and make the verified
     * name differ from what the caller passed in. */
    if (memchr(hostname->data, '\0', hostname->len) != NULL) { errno = EINVAL; return -1; }
    memcpy(buf, hostname->data, hostname->len);
    buf[hostname->len] = '\0';
    param = SSL_get0_param(tc->ssl);
    if (param == NULL) return -1;
    X509_VERIFY_PARAM_set_hostflags(param, X509_CHECK_FLAG_NO_PARTIAL_WILDCARDS);
    if (X509_VERIFY_PARAM_set1_host(param, buf, hostname->len) != 1) return -1;
    return 0;
})

/* ------------------------------------------------------------------ *
 * BIO <-> socket bridges                                             *
 * ------------------------------------------------------------------ */

/* Drain everything in wbio to the network.  Returns:
 *   M_C_FINISH  wbio (and pending buffer) fully empty after the call
 *   M_C_NOTYET  bytes still queued (EAGAIN on socket)
 *   M_C_ERROR   socket write error other than EAGAIN/EINTR
 *
 * Ordering note: BIO_read removes bytes from wbio.  If those bytes only
 * partially fit on the socket we must NOT BIO_write the tail back -- a
 * later BIO_read would put fresh ciphertext from SSL_write *before* it
 * in the stream and corrupt the TLS connection (peer MAC failure).
 * Instead we stash the unsent tail in tc->tls_pending and drain that
 * buffer in-order before reading any more from wbio.
 */
static int mln_tcp_conn_tls_flush_wbio(mln_tcp_conn_t *tc)
{
    int n;

    /* Lazy-allocate the pending buffer from the conn pool the first
     * time we have to bridge BIO -> socket.  Allocated from the pool
     * so it is freed together with the conn on destroy.
     */
    if (tc->tls_pending == NULL) {
        tc->tls_pending = (mln_u8ptr_t)mln_alloc_m(tc->pool, M_TLS_CHUNK);
        if (tc->tls_pending == NULL) { errno = ENOMEM; return M_C_ERROR; }
        tc->tls_pending_off = 0;
        tc->tls_pending_len = 0;
    }

    /* 1) Drain whatever was left from a previous EAGAIN. */
    while (tc->tls_pending_len > 0) {
#if defined(MSVC)
        n = send(tc->sockfd,
                 (char *)(tc->tls_pending + tc->tls_pending_off),
                 (int)tc->tls_pending_len, 0);
#else
        n = send(tc->sockfd,
                 tc->tls_pending + tc->tls_pending_off,
                 tc->tls_pending_len, 0);
#endif
        if (n > 0) {
            tc->tls_pending_off += (mln_size_t)n;
            tc->tls_pending_len -= (mln_size_t)n;
            continue;
        }
        if (n < 0 && errno == EINTR) continue;
        if (n < 0 && errno == EAGAIN) {
            tc->tls_want_w = 1;
            return M_C_NOTYET;
        }
        return M_C_ERROR;
    }

    /* 2) Now safe to pull more ciphertext from wbio. */
    for (;;) {
        int got = BIO_read(SSL_get_wbio(tc->ssl),
                           tc->tls_pending, M_TLS_CHUNK);
        if (got <= 0) {
            /* Empty BIO (eof_return=-1 makes it behave like EAGAIN). */
            tc->tls_pending_off = 0;
            tc->tls_pending_len = 0;
            return M_C_FINISH;
        }
        tc->tls_pending_off = 0;
        tc->tls_pending_len = (mln_size_t)got;

        while (tc->tls_pending_len > 0) {
#if defined(MSVC)
            n = send(tc->sockfd,
                     (char *)(tc->tls_pending + tc->tls_pending_off),
                     (int)tc->tls_pending_len, 0);
#else
            n = send(tc->sockfd,
                     tc->tls_pending + tc->tls_pending_off,
                     tc->tls_pending_len, 0);
#endif
            if (n > 0) {
                tc->tls_pending_off += (mln_size_t)n;
                tc->tls_pending_len -= (mln_size_t)n;
                continue;
            }
            if (n < 0 && errno == EINTR) continue;
            if (n < 0 && errno == EAGAIN) {
                tc->tls_want_w = 1;
                return M_C_NOTYET;
            }
            return M_C_ERROR;
        }
    }
}

/* Read available ciphertext off the socket and push into rbio.  Returns:
 *   M_C_FINISH  one or more reads succeeded (or socket would block)
 *   M_C_CLOSED  peer closed the connection
 *   M_C_ERROR   recv error
 *
 * For blocking sockets this performs exactly one recv() and then returns
 * so that SSL_read can make progress; for non-blocking sockets it loops
 * until EAGAIN.
 */
static int mln_tcp_conn_tls_drain_socket(mln_tcp_conn_t *tc)
{
    unsigned char buf[M_TLS_CHUNK];
    int n, got_any = 0;

    for (;;) {
#if defined(MSVC)
        n = recv(tc->sockfd, (char *)buf, sizeof buf, 0);
#else
        n = recv(tc->sockfd, buf, sizeof buf, 0);
#endif
        if (n > 0) {
            /* A mem-BIO write only fails if it can't allocate.  Treat
             * a short or failed write as a hard error -- dropping any
             * ciphertext here would desync the TLS stream and the peer
             * would close on the next MAC failure. */
            int w = BIO_write(SSL_get_rbio(tc->ssl), buf, n);
            if (w != n) { errno = ENOMEM; return M_C_ERROR; }
            got_any = 1;
            if (!tc->nonblock) break;
            continue;
        }
        if (n == 0) return got_any ? M_C_FINISH : M_C_CLOSED;
        if (errno == EINTR) continue;
        if (errno == EAGAIN) break;
        return M_C_ERROR;
    }
    return M_C_FINISH;
}

/* ------------------------------------------------------------------ *
 * Handshake                                                          *
 * ------------------------------------------------------------------ */

MLN_FUNC(, int, mln_tcp_conn_tls_handshake, (mln_tcp_conn_t *tc), (tc), {
    int r, err;

    if (tc->ssl == NULL) { errno = EINVAL; return M_C_ERROR; }
    /* Stale flags from a previous M_C_NOTYET return would otherwise
     * leak out through the tls_done short-circuit. */
    tc->tls_want_r = tc->tls_want_w = 0;
    if (tc->tls_done) return M_C_FINISH;

    for (;;) {
        tc->tls_want_r = tc->tls_want_w = 0;

        r = SSL_do_handshake(tc->ssl);
        /* Any handshake message produced (ClientHello, Finished, ...)
         * lives in wbio now; push it to the wire before reporting state.
         */
        {
            int fr = mln_tcp_conn_tls_flush_wbio(tc);
            if (fr == M_C_ERROR) return M_C_ERROR;
            if (fr == M_C_NOTYET) {
                /* Will retry on the next writable event. */
                return M_C_NOTYET;
            }
        }
        if (r == 1) {
            tc->tls_done = 1;
            return M_C_FINISH;
        }
        err = SSL_get_error(tc->ssl, r);
        if (err == SSL_ERROR_WANT_READ) {
            /* Capture rbio fill before and after the socket drain so we
             * can tell whether drain_socket actually made progress.
             * Checking only `BIO_pending == 0` was fragile -- if rbio
             * already held a stale partial record from a previous
             * iteration and the socket has no new bytes, we would
             * BIO_pending > 0 and loop forever in nonblock mode.
             * `post <= pre` is the correct "no progress" test. */
            size_t pre = (size_t)BIO_pending(SSL_get_rbio(tc->ssl));
            int dr = mln_tcp_conn_tls_drain_socket(tc);
            if (dr == M_C_ERROR || dr == M_C_CLOSED) return dr;
            size_t post = (size_t)BIO_pending(SSL_get_rbio(tc->ssl));
            if (tc->nonblock) {
                if (post <= pre) {
                    tc->tls_want_r = 1;
                    return M_C_NOTYET;
                }
                continue;
            }
            /* Blocking socket: recv() already blocked once; try the
             * handshake again now that we have bytes. */
            continue;
        }
        if (err == SSL_ERROR_WANT_WRITE) {
            /* wbio was flushed above; if it stalled we already returned
             * M_C_NOTYET.  Loop once more to drive forward.
             */
            tc->tls_want_w = 1;
            return M_C_NOTYET;
        }
        return M_C_ERROR;
    }
})

/* ------------------------------------------------------------------ *
 * Send                                                               *
 * ------------------------------------------------------------------ *
 * Per-chain semantics: a chain is only moved to sent_* once the      *
 * corresponding ciphertext has actually been transmitted on the      *
 * socket.  The loop below drains wbio after each successful          *
 * SSL_write, and any failure of that drain causes us to return       *
 * M_C_NOTYET with the buf left in snd_*.                              *
 * ------------------------------------------------------------------ */

static int mln_tcp_conn_tls_write_one_chunk(mln_tcp_conn_t *tc,
                                            const unsigned char *p,
                                            size_t n,
                                            size_t *written)
{
    int w;
    *written = 0;
    while (*written < n) {
        w = SSL_write(tc->ssl, p + *written, (int)(n - *written));
        if (w > 0) {
            *written += (size_t)w;
            continue;
        }
        int err = SSL_get_error(tc->ssl, w);
        if (err == SSL_ERROR_WANT_READ)  { tc->tls_want_r = 1; return M_C_NOTYET; }
        if (err == SSL_ERROR_WANT_WRITE) { tc->tls_want_w = 1; return M_C_NOTYET; }
        if (err == SSL_ERROR_ZERO_RETURN) return M_C_CLOSED;
        return M_C_ERROR;
    }
    return M_C_FINISH;
}

static int mln_tcp_conn_send_tls(mln_tcp_conn_t *tc)
{
    int r;
    mln_chain_t *c;
    mln_buf_t   *b;
    int is_done = 0;

    /* Clear stale event-wait flags from a previous M_C_NOTYET so the
     * caller does not keep registering POLLIN/POLLOUT we no longer
     * need.  Each branch below sets them only on its own return path.
     */
    tc->tls_want_r = tc->tls_want_w = 0;

    if (!tc->tls_done) {
        r = mln_tcp_conn_tls_handshake(tc);
        if (r != M_C_FINISH) return r;
    }

    /* First push any ciphertext that was queued by an earlier partial
     * write back out to the socket.
     */
    r = mln_tcp_conn_tls_flush_wbio(tc);
    if (r == M_C_ERROR) return r;
    if (r == M_C_NOTYET) return r;

    while ((c = tc->snd_head) != NULL) {
        b = c->buf;
        if (b == NULL) {
            c = mln_tcp_conn_pop_inline(tc, M_C_SEND);
            mln_tcp_conn_append(tc, c, M_C_SENT);
            continue;
        }

        size_t left = mln_buf_left_size(b);
        if (left == 0) {
            if (b->last_in_chain) is_done = 1;
            c = mln_tcp_conn_pop_inline(tc, M_C_SEND);
            mln_tcp_conn_append(tc, c, M_C_SENT);
            if (is_done) break;
            continue;
        }

        size_t written = 0;
        unsigned char stackbuf[M_TLS_CHUNK];
        const unsigned char *src;

        if (b->in_memory) {
            size_t n = left > M_TLS_CHUNK ? M_TLS_CHUNK : left;
            src = (const unsigned char *)b->left_pos;
            r = mln_tcp_conn_tls_write_one_chunk(tc, src, n, &written);
            if (written > 0) b->left_pos += written;
        } else if (b->in_file) {
            ssize_t rn;
            size_t n = left > M_TLS_CHUNK ? M_TLS_CHUNK : left;
            if (lseek(mln_file_fd(b->file), b->file_left_pos, SEEK_SET) < 0)
                return M_C_ERROR;
            /* Match the EINTR retry behaviour of the plain in_file path. */
            for (;;) {
                rn = read(mln_file_fd(b->file), stackbuf, n);
                if (rn > 0) break;
                if (rn < 0 && errno == EINTR) continue;
                return M_C_ERROR;
            }
            r = mln_tcp_conn_tls_write_one_chunk(tc, stackbuf, (size_t)rn, &written);
            if (written > 0) b->file_left_pos += written;
        } else {
            /* Unknown buffer flavour — treat as a caller error so
             * the chain is never silently discarded. */
            errno = EINVAL;
            return M_C_ERROR;
        }

        /* Flush whatever SSL_write produced before deciding the fate
         * of this chain.  Only after the ciphertext is actually on the
         * wire do we consider this buf "sent".
         */
        {
            int fr = mln_tcp_conn_tls_flush_wbio(tc);
            if (fr == M_C_ERROR) return M_C_ERROR;
            if (fr == M_C_NOTYET) return M_C_NOTYET;
        }
        if (r == M_C_ERROR || r == M_C_CLOSED) return r;
        if (r == M_C_NOTYET) return M_C_NOTYET;

        if (mln_buf_left_size(b) == 0) {
            if (b->last_in_chain) is_done = 1;
            c = mln_tcp_conn_pop_inline(tc, M_C_SEND);
            mln_tcp_conn_append(tc, c, M_C_SENT);
            if (is_done) break;
        }
    }

    return is_done ? M_C_FINISH : M_C_NOTYET;
}

/* ------------------------------------------------------------------ *
 * Recv                                                               *
 * ------------------------------------------------------------------ *
 * Decrypted plaintext is appended to rcv_* as fresh memory chains;   *
 * ciphertext only lives on the stack and inside rbio, so the rcv_*   *
 * queue is always pure plaintext from the caller's point of view.    *
 * ------------------------------------------------------------------ */

static int mln_tcp_conn_recv_tls(mln_tcp_conn_t *tc)
{
    int dr, p, err;

    tc->tls_want_r = tc->tls_want_w = 0;

    if (!tc->tls_done) {
        int hr = mln_tcp_conn_tls_handshake(tc);
        if (hr != M_C_FINISH) return hr;
    }

    /* Pull ciphertext off the wire and feed it to rbio.
     */
    dr = mln_tcp_conn_tls_drain_socket(tc);
    if (dr == M_C_ERROR) return dr;
    /* M_C_CLOSED is recorded but we still try SSL_read in case the
     * shutdown record gives us trailing plaintext / a clean close. */

    /* Decrypt everything currently available. */
    for (;;) {
        tc->tls_want_r = tc->tls_want_w = 0;
        mln_u8ptr_t buf = (mln_u8ptr_t)mln_alloc_m(tc->pool, 4096);
        if (buf == NULL) { errno = ENOMEM; return M_C_ERROR; }

        p = SSL_read(tc->ssl, buf, 4096);
        if (p > 0) {
            /* mln_chain_new_with_buf returns a chain node with its
             * mln_buf_t already attached.  Internally it makes two
             * pool allocations but cleans the chain node up if the
             * buf allocation fails -- callers always observe either
             * a fully-formed pair or NULL, so we cannot leak one
             * half.  The 4 KiB plaintext buffer `buf` below is owned
             * separately by the returned mln_buf_t. */
            mln_chain_t *nc = mln_chain_new_with_buf(tc->pool);
            if (nc == NULL) { mln_alloc_free(buf); errno = ENOMEM; return M_C_ERROR; }
            mln_buf_t *nb = nc->buf;
            nb->left_pos = nb->pos = nb->start = buf;
            nb->last     = nb->end             = buf + p;
            nb->in_memory = 1;
            nb->last_buf  = 1;
            mln_tcp_conn_append(tc, nc, M_C_RECV);
            continue;
        }
        mln_alloc_free(buf);

        err = SSL_get_error(tc->ssl, p);
        if (err == SSL_ERROR_WANT_READ) {
            tc->tls_want_r = 1;
            if (dr == M_C_CLOSED) return M_C_CLOSED;
            return M_C_NOTYET;
        }
        if (err == SSL_ERROR_WANT_WRITE) {
            /* Renegotiation: SSL_read wants to push ciphertext.  Flush
             * it.  If the flush stalled (returns NOTYET) flush_wbio
             * already set tls_want_w; if it fully drained we're now
             * waiting for the peer's response, so re-arm for read. */
            int fr = mln_tcp_conn_tls_flush_wbio(tc);
            if (fr == M_C_ERROR) return M_C_ERROR;
            if (fr == M_C_NOTYET) {
                /* tls_want_w already set by flush_wbio */
            } else {
                tc->tls_want_r = 1;
            }
            return M_C_NOTYET;
        }
        if (err == SSL_ERROR_ZERO_RETURN) return M_C_CLOSED;
        return M_C_ERROR;
    }
}

/* ------------------------------------------------------------------ *
 * Shutdown                                                           *
 * ------------------------------------------------------------------ */

MLN_FUNC(, int, mln_tcp_conn_tls_shutdown, (mln_tcp_conn_t *tc), (tc), {
    int r, err;

    if (tc->ssl == NULL) return M_C_FINISH;
    if (!tc->tls_done) {
        /* Nothing to gracefully close; let the caller close the fd. */
        tc->tls_want_r = tc->tls_want_w = 0;
        return M_C_FINISH;
    }

    tc->tls_want_r = tc->tls_want_w = 0;

    for (;;) {
        r = SSL_shutdown(tc->ssl);
        /* Flush close_notify (and possibly ack of peer's close_notify). */
        {
            int fr = mln_tcp_conn_tls_flush_wbio(tc);
            if (fr == M_C_ERROR) return M_C_ERROR;
            if (fr == M_C_NOTYET) {
                tc->tls_shut = 1;
                return M_C_NOTYET;
            }
        }
        if (r >= 1) { tc->tls_shut = 1; return M_C_FINISH; }
        if (r == 0) {
            /* Sent close_notify; wait for the peer's. */
            if (tc->nonblock) {
                tc->tls_want_r = 1;
                return M_C_NOTYET;
            }
            /* Blocking: drain a bit and loop. */
            int dr = mln_tcp_conn_tls_drain_socket(tc);
            if (dr == M_C_ERROR) return M_C_ERROR;
            if (dr == M_C_CLOSED) { tc->tls_shut = 1; return M_C_FINISH; }
            continue;
        }
        err = SSL_get_error(tc->ssl, r);
        if (err == SSL_ERROR_WANT_READ) {
            /* See the handshake path: use pre/post BIO_pending to decide
             * whether drain_socket actually made progress, instead of
             * trusting "rbio is empty" which leaves the spin window
             * open when a stale partial record sits in rbio. */
            size_t pre = (size_t)BIO_pending(SSL_get_rbio(tc->ssl));
            int dr = mln_tcp_conn_tls_drain_socket(tc);
            if (dr == M_C_ERROR) return M_C_ERROR;
            if (dr == M_C_CLOSED) { tc->tls_shut = 1; return M_C_FINISH; }
            size_t post = (size_t)BIO_pending(SSL_get_rbio(tc->ssl));
            if (tc->nonblock && post <= pre) {
                tc->tls_want_r = 1;
                return M_C_NOTYET;
            }
            continue;
        }
        if (err == SSL_ERROR_WANT_WRITE) {
            tc->tls_want_w = 1;
            return M_C_NOTYET;
        }
        return M_C_ERROR;
    }
})

#endif /* MLN_TLS */

