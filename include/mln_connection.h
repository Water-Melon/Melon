
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
#if defined(MLN_TLS)
#include "mln_string.h"
#include <openssl/ssl.h>
#endif


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

#if defined(MLN_TLS)
/*TLS role*/
#define M_TLS_SERVER   0
#define M_TLS_CLIENT   1

/*TLS protocol version mask*/
#define M_TLS_V1_2     0x4
#define M_TLS_V1_3     0x8
#define M_TLS_VDEFAULT (M_TLS_V1_2 | M_TLS_V1_3)

typedef struct mln_tcp_tls_conf_s {
    SSL_CTX        *ctx;
    mln_string_t   *cert_file;
    mln_string_t   *key_file;
    mln_string_t   *ca_file;
    mln_string_t   *ciphers;
    /* Full-width versions field instead of a narrow bitfield: lets
     * future protocol additions (M_TLS_V1_4 etc.) fit without an
     * ABI-breaking widen, and stops mln_tcp_tls_conf_new from
     * silently truncating unknown bits. */
    mln_u32_t       versions;
    mln_u32_t       role:1;
    mln_u32_t       verify:1;
} mln_tcp_tls_conf_t;
#endif

typedef struct {
    mln_alloc_t *pool;
    mln_chain_t *rcv_head;
    mln_chain_t *rcv_tail;
    mln_chain_t *snd_head;
    mln_chain_t *snd_tail;
    mln_chain_t *sent_head;
    mln_chain_t *sent_tail;
    int          sockfd;
    mln_u32_t    nonblock:1;
#if defined(MLN_TLS)
    /* TLS state: all zero/NULL on a plain TCP connection. */
    SSL                 *ssl;
    mln_tcp_tls_conf_t  *tls_conf;  /* not owned */
    /* Pending ciphertext that was read out of wbio but the socket
     * could not yet swallow.  Must be drained in-order before any
     * further BIO_read, otherwise the TLS byte stream gets reordered
     * and the peer drops the connection on MAC failure.  Allocated
     * lazily from `pool` on the first call to mln_tcp_conn_tls_flush_wbio.
     */
    mln_u8ptr_t          tls_pending;
    mln_size_t           tls_pending_off;
    mln_size_t           tls_pending_len;
    mln_u32_t            tls_done:1;
    mln_u32_t            tls_want_r:1;
    mln_u32_t            tls_want_w:1;
    mln_u32_t            tls_shut:1;
#endif
} mln_tcp_conn_t;


#define mln_tcp_conn_send_empty(pconn) ((pconn)->snd_head == NULL)
#define mln_tcp_conn_recv_empty(pconn) ((pconn)->rcv_head == NULL)
#define mln_tcp_conn_sent_empty(pconn) ((pconn)->sent_head == NULL)
#define mln_tcp_conn_fd_get(pconn) ((pconn)->sockfd)
extern void mln_tcp_conn_fd_set(mln_tcp_conn_t *tc, int fd) __NONNULL1(1);
#define mln_tcp_conn_pool_get(pconn) ((pconn)->pool)
extern int mln_tcp_conn_set_nonblock(mln_tcp_conn_t *tc, int nb) __NONNULL1(1);
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
extern void mln_tcp_conn_move_sent(mln_tcp_conn_t *tc) __NONNULL1(1);
extern int mln_tcp_conn_send_chain(mln_tcp_conn_t *tc, mln_chain_t *chain) __NONNULL2(1,2);

#if defined(MLN_TLS)
/*
 * TLS support
 *
 * A TLS connection is just an mln_tcp_conn_t initialized through
 * mln_tcp_conn_tls_init() instead of mln_tcp_conn_init(); after that
 * the regular mln_tcp_conn_send/recv functions transparently encrypt
 * and decrypt.
 *
 * Configuration objects (mln_tcp_tls_conf_t) wrap an SSL_CTX and may
 * be shared across many connections.
 *
 * Return value conventions match mln_tcp_conn_send/recv:
 *   M_C_FINISH / M_C_NOTYET / M_C_ERROR / M_C_CLOSED.
 */


/*
 * Build a config.  Returns NULL on failure (bad files, OpenSSL error...).
 *
 *   role:        M_TLS_SERVER requires non-NULL cert_file and key_file.
 *                M_TLS_CLIENT may pass NULL for both.
 *   ca_file:     PEM bundle of trusted CAs (client side); enables peer
 *                verification when 'verify' is non-zero.
 *   ciphers:     TLSv1.2 cipher list, NULL for OpenSSL default.
 *   versions:    bitmask of M_TLS_V1_2 / M_TLS_V1_3; 0 means default.
 *   verify:      non-zero to require peer certificate verification.
 */
extern mln_tcp_tls_conf_t *
mln_tcp_tls_conf_new(mln_u32_t role,
                     mln_string_t *cert_file,
                     mln_string_t *key_file,
                     mln_string_t *ca_file,
                     mln_string_t *ciphers,
                     mln_u32_t versions,
                     mln_u32_t verify);
extern void mln_tcp_tls_conf_free(mln_tcp_tls_conf_t *conf);

/*
 * Initialize a TCP connection that speaks TLS, parallel to
 * mln_tcp_conn_init().  conf must outlive tc; it is not consumed.
 * Returns 0 on success, -1 on failure (errno set when meaningful).
 */
extern int
mln_tcp_conn_tls_init(mln_tcp_conn_t *tc, int sockfd, mln_tcp_tls_conf_t *conf)
    __NONNULL1(1);

/*
 * Drive the TLS handshake explicitly.  Optional: the first send/recv
 * call will trigger it automatically.  Returns M_C_FINISH when done,
 * M_C_NOTYET when more I/O is needed (check tls_want_r/tls_want_w),
 * M_C_ERROR on failure, M_C_CLOSED if the peer hung up mid-handshake.
 */
extern int mln_tcp_conn_tls_handshake(mln_tcp_conn_t *tc) __NONNULL1(1);

/*
 * Send TLS close_notify.  Non-blocking callers may receive M_C_NOTYET
 * and need to retry once the socket is writable.
 */
extern int mln_tcp_conn_tls_shutdown(mln_tcp_conn_t *tc) __NONNULL1(1);

/* Client-side: set SNI hostname.  Must be called before handshake. */
extern int mln_tcp_conn_tls_set_sni(mln_tcp_conn_t *tc, mln_string_t *hostname)
    __NONNULL2(1,2);

/* Client-side: enable hostname verification (RFC 6125). */
extern int mln_tcp_conn_tls_set_verify_host(mln_tcp_conn_t *tc, mln_string_t *hostname)
    __NONNULL2(1,2);

#define mln_tcp_conn_tls_enabled(tc)    ((tc)->ssl != NULL)
#define mln_tcp_conn_tls_done(tc)       ((tc)->tls_done)
#define mln_tcp_conn_tls_want_read(tc)  ((tc)->tls_want_r)
#define mln_tcp_conn_tls_want_write(tc) ((tc)->tls_want_w)

#endif /* MLN_TLS */

#endif

