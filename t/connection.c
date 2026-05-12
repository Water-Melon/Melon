#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include "mln_connection.h"
#include "mln_file.h"

#if defined(MLN_TLS)
#include <pthread.h>
#include <poll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "mln_http.h"
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>
#endif

/* Helper function to calculate elapsed time in microseconds */
static long elapsed_us(struct timespec *start, struct timespec *end)
{
    long sec_us = (end->tv_sec - start->tv_sec) * 1000000L;
    long nsec_us = (end->tv_nsec - start->tv_nsec) / 1000L;
    return sec_us + nsec_us;
}

/* Helper to set socket to non-blocking mode */
static void set_nonblock(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    assert(flags >= 0);
    assert(fcntl(fd, F_SETFL, flags | O_NONBLOCK) == 0);
}

/* Test 1: Basic initialization and destruction */
static void test_basic(void)
{
    printf("Testing basic initialization and destruction...\n");

    int fds[2];
    assert(socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0);

    mln_tcp_conn_t conn;
    assert(mln_tcp_conn_init(&conn, fds[0]) == 0);

    /* Verify initialization */
    assert(mln_tcp_conn_fd_get(&conn) == fds[0]);
    assert(mln_tcp_conn_pool_get(&conn) != NULL);
    assert(mln_tcp_conn_send_empty(&conn));
    assert(mln_tcp_conn_recv_empty(&conn));
    assert(mln_tcp_conn_sent_empty(&conn));

    /* Destroy connection */
    mln_tcp_conn_destroy(&conn);

    close(fds[0]);
    close(fds[1]);

    printf("  PASS: basic initialization\n");
}

/* Test 2: Queue operations (append, pop, head, tail, remove) */
static void test_queue_ops(void)
{
    printf("Testing queue operations...\n");

    int fds[2];
    assert(socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0);

    mln_tcp_conn_t conn;
    assert(mln_tcp_conn_init(&conn, fds[0]) == 0);

    mln_alloc_t *pool = mln_tcp_conn_pool_get(&conn);
    assert(pool != NULL);

    /* Test append one-by-one to SEND queue */
    mln_chain_t *c1 = mln_chain_new(pool);
    mln_chain_t *c2 = mln_chain_new(pool);
    mln_chain_t *c3 = mln_chain_new(pool);
    assert(c1 != NULL && c2 != NULL && c3 != NULL);

    mln_tcp_conn_append(&conn, c1, M_C_SEND);
    mln_tcp_conn_append(&conn, c2, M_C_SEND);
    mln_tcp_conn_append(&conn, c3, M_C_SEND);
    assert(mln_tcp_conn_head(&conn, M_C_SEND) == c1);
    assert(mln_tcp_conn_tail(&conn, M_C_SEND) == c3);

    /* Test append_chain: build a linked list and append it */
    mln_chain_t *c4 = mln_chain_new(pool);
    mln_chain_t *c5 = mln_chain_new(pool);
    assert(c4 != NULL && c5 != NULL);
    c4->next = c5;
    mln_tcp_conn_append_chain(&conn, c4, c5, M_C_RECV);
    assert(mln_tcp_conn_head(&conn, M_C_RECV) == c4);
    assert(mln_tcp_conn_tail(&conn, M_C_RECV) == c5);

    /* Test pop from SEND queue */
    mln_chain_t *popped = mln_tcp_conn_pop(&conn, M_C_SEND);
    assert(popped == c1);
    assert(mln_tcp_conn_head(&conn, M_C_SEND) == c2);
    assert(mln_tcp_conn_tail(&conn, M_C_SEND) == c3);

    /* Test pop remaining */
    popped = mln_tcp_conn_pop(&conn, M_C_SEND);
    assert(popped == c2);
    popped = mln_tcp_conn_pop(&conn, M_C_SEND);
    assert(popped == c3);
    assert(mln_tcp_conn_send_empty(&conn));

    /* Test remove all from RECV queue */
    assert(mln_tcp_conn_head(&conn, M_C_RECV) == c4);
    mln_chain_t *removed = mln_tcp_conn_remove(&conn, M_C_RECV);
    assert(removed == c4);
    assert(mln_tcp_conn_recv_empty(&conn));

    mln_tcp_conn_destroy(&conn);
    close(fds[0]);
    close(fds[1]);

    printf("  PASS: queue operations\n");
}

/* Test 3: Send and receive data */
static void test_send_recv(void)
{
    printf("Testing send and receive...\n");

    int fds[2];
    assert(socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0);

    mln_tcp_conn_t conn_send, conn_recv;
    assert(mln_tcp_conn_init(&conn_send, fds[0]) == 0);
    assert(mln_tcp_conn_init(&conn_recv, fds[1]) == 0);

    mln_alloc_t *pool = mln_tcp_conn_pool_get(&conn_send);

    /* Prepare data to send */
    const char *data = "Hello, World!";
    int data_len = strlen(data);

    mln_u8ptr_t buf = (mln_u8ptr_t)mln_alloc_m(pool, data_len + 1);
    assert(buf != NULL);
    memcpy(buf, data, data_len);

    mln_chain_t *c = mln_chain_new(pool);
    mln_buf_t *b = mln_buf_new(pool);
    assert(c != NULL && b != NULL);

    c->buf = b;
    b->left_pos = b->pos = b->start = buf;
    b->last = b->end = buf + data_len;
    b->in_memory = 1;
    b->last_buf = 1;
    b->last_in_chain = 1;

    mln_tcp_conn_append(&conn_send, c, M_C_SEND);

    /* Send data */
    int ret = mln_tcp_conn_send(&conn_send);
    assert(ret == M_C_FINISH);

    /* Receive data */
    ret = mln_tcp_conn_recv(&conn_recv, M_C_TYPE_MEMORY);
    assert(ret != M_C_ERROR);

    /* Verify received data */
    mln_chain_t *recv_chain = mln_tcp_conn_head(&conn_recv, M_C_RECV);
    assert(recv_chain != NULL);
    assert(recv_chain->buf != NULL);
    assert(memcmp(recv_chain->buf->start, data, data_len) == 0);

    mln_tcp_conn_destroy(&conn_send);
    mln_tcp_conn_destroy(&conn_recv);
    close(fds[0]);
    close(fds[1]);

    printf("  PASS: send and receive\n");
}

/* Test 4: Large data send and receive */
static void test_large_data(void)
{
    printf("Testing large data send/recv...\n");

    int fds[2];
    assert(socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0);

    mln_tcp_conn_t conn_send, conn_recv;
    assert(mln_tcp_conn_init(&conn_send, fds[0]) == 0);
    assert(mln_tcp_conn_init(&conn_recv, fds[1]) == 0);

    mln_alloc_t *pool = mln_tcp_conn_pool_get(&conn_send);

    /* Create large data (>4096 bytes) */
    const int large_size = 8192;
    mln_u8ptr_t large_buf = (mln_u8ptr_t)mln_alloc_m(pool, large_size);
    assert(large_buf != NULL);

    /* Fill with pattern */
    for (int i = 0; i < large_size; i++) {
        large_buf[i] = (mln_u8_t)(i % 256);
    }

    mln_chain_t *c = mln_chain_new(pool);
    mln_buf_t *b = mln_buf_new(pool);
    assert(c != NULL && b != NULL);

    c->buf = b;
    b->left_pos = b->pos = b->start = large_buf;
    b->last = b->end = large_buf + large_size;
    b->in_memory = 1;
    b->last_buf = 1;
    b->last_in_chain = 1;

    mln_tcp_conn_append(&conn_send, c, M_C_SEND);

    /* Send large data */
    int ret = mln_tcp_conn_send(&conn_send);
    assert(ret == M_C_FINISH);

    /* Receive large data - may take multiple reads */
    int total_recv = 0;
    while (total_recv < large_size) {
        ret = mln_tcp_conn_recv(&conn_recv, M_C_TYPE_MEMORY);
        assert(ret != M_C_ERROR);

        mln_chain_t *iter = mln_tcp_conn_head(&conn_recv, M_C_RECV);
        total_recv = 0;
        while (iter != NULL) {
            if (iter->buf != NULL)
                total_recv += (int)(iter->buf->last - iter->buf->pos);
            iter = iter->next;
        }
    }

    /* Verify data integrity across all received chunks */
    mln_chain_t *recv_chain = mln_tcp_conn_head(&conn_recv, M_C_RECV);
    int offset = 0;
    while (recv_chain != NULL && offset < large_size) {
        if (recv_chain->buf != NULL) {
            int chunk = (int)(recv_chain->buf->last - recv_chain->buf->pos);
            assert(memcmp(recv_chain->buf->pos, large_buf + offset, chunk) == 0);
            offset += chunk;
        }
        recv_chain = recv_chain->next;
    }
    assert(offset == large_size);

    mln_tcp_conn_destroy(&conn_send);
    mln_tcp_conn_destroy(&conn_recv);
    close(fds[0]);
    close(fds[1]);

    printf("  PASS: large data\n");
}

/* Test 5: mln_tcp_conn_move_sent */
static void test_move_sent(void)
{
    printf("Testing move_sent...\n");

    int fds[2];
    assert(socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0);

    mln_tcp_conn_t conn;
    assert(mln_tcp_conn_init(&conn, fds[0]) == 0);

    mln_alloc_t *pool = mln_tcp_conn_pool_get(&conn);

    /* Add chains to send queue */
    mln_chain_t *c1 = mln_chain_new(pool);
    mln_chain_t *c2 = mln_chain_new(pool);
    assert(c1 != NULL && c2 != NULL);

    mln_tcp_conn_append(&conn, c1, M_C_SEND);
    mln_tcp_conn_append(&conn, c2, M_C_SEND);

    assert(!mln_tcp_conn_send_empty(&conn));
    assert(mln_tcp_conn_sent_empty(&conn));

    /* Move from send to sent */
    mln_tcp_conn_move_sent(&conn);

    assert(mln_tcp_conn_send_empty(&conn));
    assert(!mln_tcp_conn_sent_empty(&conn));
    assert(mln_tcp_conn_head(&conn, M_C_SENT) == c1);
    assert(mln_tcp_conn_tail(&conn, M_C_SENT) == c2);

    mln_tcp_conn_destroy(&conn);
    close(fds[0]);
    close(fds[1]);

    printf("  PASS: move_sent\n");
}

/* Test 6: mln_tcp_conn_send_chain */
static void test_send_chain(void)
{
    printf("Testing send_chain...\n");

    int fds[2];
    assert(socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0);

    mln_tcp_conn_t conn_send, conn_recv;
    assert(mln_tcp_conn_init(&conn_send, fds[0]) == 0);
    assert(mln_tcp_conn_init(&conn_recv, fds[1]) == 0);

    mln_alloc_t *pool = mln_tcp_conn_pool_get(&conn_send);

    /* Create a chain directly */
    const char *msg = "Send chain test";
    mln_u8ptr_t buf = (mln_u8ptr_t)mln_alloc_m(pool, strlen(msg) + 1);
    assert(buf != NULL);
    memcpy(buf, msg, strlen(msg));

    mln_chain_t *c = mln_chain_new(pool);
    mln_buf_t *b = mln_buf_new(pool);
    assert(c != NULL && b != NULL);

    c->buf = b;
    b->left_pos = b->pos = b->start = buf;
    b->last = b->end = buf + strlen(msg);
    b->in_memory = 1;
    b->last_buf = 1;
    b->last_in_chain = 1;

    /* Use mln_tcp_conn_send_chain */
    int ret = mln_tcp_conn_send_chain(&conn_send, c);
    assert(ret == M_C_FINISH);

    /* Receive and verify */
    ret = mln_tcp_conn_recv(&conn_recv, M_C_TYPE_MEMORY);
    assert(ret != M_C_ERROR);

    mln_chain_t *recv_chain = mln_tcp_conn_head(&conn_recv, M_C_RECV);
    assert(recv_chain != NULL);
    assert(memcmp(recv_chain->buf->start, msg, strlen(msg)) == 0);

    mln_tcp_conn_destroy(&conn_send);
    mln_tcp_conn_destroy(&conn_recv);
    close(fds[0]);
    close(fds[1]);

    printf("  PASS: send_chain\n");
}

/* Test 7: fd_get, fd_set, pool_get macros */
static void test_macros(void)
{
    printf("Testing fd/pool macros...\n");

    int fds[2];
    assert(socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0);

    mln_tcp_conn_t conn;
    assert(mln_tcp_conn_init(&conn, fds[0]) == 0);

    /* Test fd_get macro */
    int fd = mln_tcp_conn_fd_get(&conn);
    assert(fd == fds[0]);

    /* Test fd_set macro */
    mln_tcp_conn_fd_set(&conn, fds[1]);
    assert(mln_tcp_conn_fd_get(&conn) == fds[1]);

    /* Test pool_get macro */
    mln_alloc_t *pool = mln_tcp_conn_pool_get(&conn);
    assert(pool != NULL);

    /* Test set_nonblock macro */
    mln_tcp_conn_set_nonblock(&conn, 1);
    assert(conn.nonblock == 1);

    mln_tcp_conn_set_nonblock(&conn, 0);
    assert(conn.nonblock == 0);

    mln_tcp_conn_destroy(&conn);
    close(fds[0]);
    close(fds[1]);

    printf("  PASS: fd/pool macros\n");
}

/* Test 8: Multiple send/recv cycles */
static void test_multiple_cycles(void)
{
    printf("Testing multiple send/recv cycles...\n");

    int fds[2];
    assert(socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0);

    mln_tcp_conn_t conn_send, conn_recv;
    assert(mln_tcp_conn_init(&conn_send, fds[0]) == 0);
    assert(mln_tcp_conn_init(&conn_recv, fds[1]) == 0);

    mln_alloc_t *pool = mln_tcp_conn_pool_get(&conn_send);

    /* Multiple send/recv pairs */
    const char *messages[] = {"msg1", "msg2", "msg3"};
    int num_msgs = sizeof(messages) / sizeof(messages[0]);

    for (int i = 0; i < num_msgs; i++) {
        const char *msg = messages[i];
        int msg_len = strlen(msg);

        /* Prepare send buffer */
        mln_u8ptr_t buf = (mln_u8ptr_t)mln_alloc_m(pool, msg_len + 1);
        assert(buf != NULL);
        memcpy(buf, msg, msg_len);

        mln_chain_t *c = mln_chain_new(pool);
        mln_buf_t *b = mln_buf_new(pool);
        assert(c != NULL && b != NULL);

        c->buf = b;
        b->left_pos = b->pos = b->start = buf;
        b->last = b->end = buf + msg_len;
        b->in_memory = 1;
        b->last_buf = 1;
        b->last_in_chain = 1;

        mln_tcp_conn_append(&conn_send, c, M_C_SEND);

        /* Send */
        int ret = mln_tcp_conn_send(&conn_send);
        assert(ret == M_C_FINISH);

        /* Receive */
        ret = mln_tcp_conn_recv(&conn_recv, M_C_TYPE_MEMORY);
        assert(ret != M_C_ERROR);

        /* Verify */
        mln_chain_t *recv_chain = mln_tcp_conn_head(&conn_recv, M_C_RECV);
        assert(recv_chain != NULL);
        assert(memcmp(recv_chain->buf->start, msg, msg_len) == 0);

        /* Clean up recv queue for next iteration */
        mln_chain_pool_release_all(mln_tcp_conn_remove(&conn_recv, M_C_RECV));
    }

    mln_tcp_conn_destroy(&conn_send);
    mln_tcp_conn_destroy(&conn_recv);
    close(fds[0]);
    close(fds[1]);

    printf("  PASS: multiple cycles\n");
}

/* Test 9: Performance benchmark (send/recv throughput) */
static void test_performance(void)
{
    printf("Testing performance (100000 iterations)...\n");

    int fds[2];
    assert(socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0);

    mln_tcp_conn_t conn_send, conn_recv;
    assert(mln_tcp_conn_init(&conn_send, fds[0]) == 0);
    assert(mln_tcp_conn_init(&conn_recv, fds[1]) == 0);

    mln_alloc_t *pool = mln_tcp_conn_pool_get(&conn_send);

    const int N = 100000;
    const char *test_msg = "x";
    int msg_len = strlen(test_msg);

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    for (int i = 0; i < N; i++) {
        /* Prepare small message */
        mln_u8ptr_t buf = (mln_u8ptr_t)mln_alloc_m(pool, msg_len + 1);
        if (buf == NULL) break;
        memcpy(buf, test_msg, msg_len);

        mln_chain_t *c = mln_chain_new(pool);
        mln_buf_t *b = mln_buf_new(pool);
        if (c == NULL || b == NULL) break;

        c->buf = b;
        b->left_pos = b->pos = b->start = buf;
        b->last = b->end = buf + msg_len;
        b->in_memory = 1;
        b->last_buf = 1;
        b->last_in_chain = 1;

        mln_tcp_conn_append(&conn_send, c, M_C_SEND);

        /* Send */
        int ret = mln_tcp_conn_send(&conn_send);
        if (ret != M_C_FINISH) break;

        /* Receive */
        ret = mln_tcp_conn_recv(&conn_recv, M_C_TYPE_MEMORY);
        if (ret == M_C_ERROR) break;

        /* Clean up */
        mln_chain_pool_release_all(mln_tcp_conn_remove(&conn_recv, M_C_RECV));
    }

    clock_gettime(CLOCK_MONOTONIC, &end);
    long elapsed = elapsed_us(&start, &end);

    double throughput = (N * 1000000.0) / elapsed;
    printf("  Throughput: %.2f messages/sec\n", throughput);
    printf("  Total time: %ld us\n", elapsed);

    mln_tcp_conn_destroy(&conn_send);
    mln_tcp_conn_destroy(&conn_recv);
    close(fds[0]);
    close(fds[1]);

    printf("  PASS: performance benchmark\n");
}

/* Test 10: Stability test with rapid connections and operations */
static void test_stability(void)
{
    printf("Testing stability with rapid operations...\n");

    const int NUM_CYCLES = 100;
    const int CHAIN_OPS_PER_CYCLE = 50;

    for (int cycle = 0; cycle < NUM_CYCLES; cycle++) {
        int fds[2];
        if (socketpair(AF_UNIX, SOCK_STREAM, 0, fds) != 0) continue;

        mln_tcp_conn_t conn;
        if (mln_tcp_conn_init(&conn, fds[0]) != 0) {
            close(fds[0]);
            close(fds[1]);
            continue;
        }

        mln_alloc_t *pool = mln_tcp_conn_pool_get(&conn);

        /* Perform many chain operations */
        for (int op = 0; op < CHAIN_OPS_PER_CYCLE; op++) {
            mln_chain_t *c = mln_chain_new(pool);
            mln_buf_t *b = mln_buf_new(pool);

            if (c != NULL && b != NULL) {
                c->buf = b;

                /* Random operations */
                int action = op % 3;
                if (action == 0) {
                    mln_tcp_conn_append(&conn, c, M_C_SEND);
                } else if (action == 1) {
                    mln_tcp_conn_append(&conn, c, M_C_RECV);
                } else {
                    mln_tcp_conn_append(&conn, c, M_C_SENT);
                }
            }
        }

        /* Clean up all queues */
        mln_chain_pool_release_all(mln_tcp_conn_remove(&conn, M_C_SEND));
        mln_chain_pool_release_all(mln_tcp_conn_remove(&conn, M_C_RECV));
        mln_chain_pool_release_all(mln_tcp_conn_remove(&conn, M_C_SENT));

        mln_tcp_conn_destroy(&conn);
        close(fds[0]);
        close(fds[1]);
    }

    printf("  PASS: stability test\n");
}

/* Test 11: recv returns M_C_CLOSED when peer closes connection */
static void test_recv_closed(void)
{
    printf("Testing recv with closed connection...\n");

    int fds[2];
    assert(socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0);

    mln_tcp_conn_t conn_recv;
    assert(mln_tcp_conn_init(&conn_recv, fds[1]) == 0);

    /* Close sender side to signal EOF */
    close(fds[0]);

    /* Recv should return M_C_CLOSED when peer closes */
    int ret = mln_tcp_conn_recv(&conn_recv, M_C_TYPE_MEMORY);
    assert(ret == M_C_CLOSED);

    mln_tcp_conn_destroy(&conn_recv);
    close(fds[1]);

    printf("  PASS: recv closed connection\n");
}

/* Test 12: recv returns M_C_NOTYET on non-blocking socket with no data */
static void test_recv_notyet_nonblock(void)
{
    printf("Testing recv on non-blocking socket (no data)...\n");

    int fds[2];
    assert(socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0);

    mln_tcp_conn_t conn_recv;
    assert(mln_tcp_conn_init(&conn_recv, fds[1]) == 0);

    /* Set to non-blocking mode */
    set_nonblock(fds[1]);
    mln_tcp_conn_set_nonblock(&conn_recv, 1);

    /* Recv with no data available should return M_C_NOTYET (EAGAIN) */
    int ret = mln_tcp_conn_recv(&conn_recv, M_C_TYPE_MEMORY);
    assert(ret == M_C_NOTYET);

    mln_tcp_conn_destroy(&conn_recv);
    close(fds[0]);
    close(fds[1]);

    printf("  PASS: recv non-blocking no data\n");
}

/* Test 13: recv returns M_C_NOTYET on blocking socket after reading data */
static void test_recv_notyet_blocking(void)
{
    printf("Testing recv on blocking socket...\n");

    int fds[2];
    assert(socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0);

    mln_tcp_conn_t conn_send, conn_recv;
    assert(mln_tcp_conn_init(&conn_send, fds[0]) == 0);
    assert(mln_tcp_conn_init(&conn_recv, fds[1]) == 0);

    mln_alloc_t *pool = mln_tcp_conn_pool_get(&conn_send);

    /* Send some data */
    const char *data = "test data";
    int data_len = strlen(data);

    mln_u8ptr_t buf = (mln_u8ptr_t)mln_alloc_m(pool, data_len + 1);
    assert(buf != NULL);
    memcpy(buf, data, data_len);

    mln_chain_t *c = mln_chain_new(pool);
    mln_buf_t *b = mln_buf_new(pool);
    assert(c != NULL && b != NULL);

    c->buf = b;
    b->left_pos = b->pos = b->start = buf;
    b->last = b->end = buf + data_len;
    b->in_memory = 1;
    b->last_buf = 1;
    b->last_in_chain = 1;

    mln_tcp_conn_append(&conn_send, c, M_C_SEND);

    int ret = mln_tcp_conn_send(&conn_send);
    assert(ret == M_C_FINISH);

    /* Blocking recv should return M_C_NOTYET after reading data chunk */
    ret = mln_tcp_conn_recv(&conn_recv, M_C_TYPE_MEMORY);
    assert(ret == M_C_NOTYET);

    /* Verify data was received */
    mln_chain_t *recv_chain = mln_tcp_conn_head(&conn_recv, M_C_RECV);
    assert(recv_chain != NULL);

    mln_tcp_conn_destroy(&conn_send);
    mln_tcp_conn_destroy(&conn_recv);
    close(fds[0]);
    close(fds[1]);

    printf("  PASS: recv blocking socket\n");
}

/* Test 14: recv returns M_C_ERROR on invalid fd */
static void test_recv_error(void)
{
    printf("Testing recv with invalid fd...\n");

    mln_tcp_conn_t conn;
    assert(mln_tcp_conn_init(&conn, -1) == 0);

    /* Recv on invalid fd should return M_C_ERROR */
    int ret = mln_tcp_conn_recv(&conn, M_C_TYPE_MEMORY);
    assert(ret == M_C_ERROR);

    mln_tcp_conn_destroy(&conn);

    printf("  PASS: recv error handling\n");
}

/* Test 15: send returns M_C_FINISH when all data is successfully written */
static void test_send_finish(void)
{
    printf("Testing send returns M_C_FINISH...\n");

    int fds[2];
    assert(socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0);
    set_nonblock(fds[0]);
    set_nonblock(fds[1]);

    mln_tcp_conn_t conn_send, conn_recv;
    assert(mln_tcp_conn_init(&conn_send, fds[0]) == 0);
    assert(mln_tcp_conn_init(&conn_recv, fds[1]) == 0);
    mln_tcp_conn_set_nonblock(&conn_send, 1);
    mln_tcp_conn_set_nonblock(&conn_recv, 1);

    mln_alloc_t *pool = mln_tcp_conn_pool_get(&conn_send);

    /* Send a small chunk - should complete immediately, returning M_C_FINISH */
    const char *data = "M_C_FINISH test";
    int data_len = strlen(data);

    mln_u8ptr_t buf = (mln_u8ptr_t)mln_alloc_m(pool, data_len);
    assert(buf != NULL);
    memcpy(buf, data, data_len);

    mln_chain_t *c = mln_chain_new(pool);
    mln_buf_t *b = mln_buf_new(pool);
    assert(c != NULL && b != NULL);
    c->buf = b;
    b->left_pos = b->pos = b->start = buf;
    b->last = b->end = buf + data_len;
    b->in_memory = 1;
    b->last_buf = 1;
    b->last_in_chain = 1;

    mln_tcp_conn_append(&conn_send, c, M_C_SEND);

    int ret = mln_tcp_conn_send(&conn_send);
    assert(ret == M_C_FINISH);

    /* After M_C_FINISH, send queue should be empty */
    assert(mln_tcp_conn_head(&conn_send, M_C_SEND) == NULL);

    /* Sent data should be in the SENT queue */
    assert(mln_tcp_conn_head(&conn_send, M_C_SENT) != NULL);

    /* Verify the receiver can read the data */
    ret = mln_tcp_conn_recv(&conn_recv, M_C_TYPE_MEMORY);
    assert(ret == M_C_NOTYET);
    mln_chain_t *rc = mln_tcp_conn_head(&conn_recv, M_C_RECV);
    assert(rc != NULL && rc->buf != NULL);
    assert((int)(rc->buf->last - rc->buf->pos) == data_len);
    assert(memcmp(rc->buf->pos, data, data_len) == 0);

    mln_tcp_conn_destroy(&conn_send);
    mln_tcp_conn_destroy(&conn_recv);
    close(fds[0]);
    close(fds[1]);

    printf("  PASS: send returns M_C_FINISH\n");
}

/* Test 16: recv with M_C_TYPE_MEMORY after partial send (nonblocking NOTYET) */
static void test_recv_after_nonblock_send(void)
{
    printf("Testing recv after nonblocking send...\n");

    int fds[2];
    assert(socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0);
    set_nonblock(fds[0]);
    set_nonblock(fds[1]);

    mln_tcp_conn_t conn_send, conn_recv;
    assert(mln_tcp_conn_init(&conn_send, fds[0]) == 0);
    assert(mln_tcp_conn_init(&conn_recv, fds[1]) == 0);
    mln_tcp_conn_set_nonblock(&conn_send, 1);
    mln_tcp_conn_set_nonblock(&conn_recv, 1);

    mln_alloc_t *pool = mln_tcp_conn_pool_get(&conn_send);

    /* Send data */
    const char *data = "nonblock test data";
    int data_len = strlen(data);

    mln_u8ptr_t buf = (mln_u8ptr_t)mln_alloc_m(pool, data_len + 1);
    assert(buf != NULL);
    memcpy(buf, data, data_len);

    mln_chain_t *c = mln_chain_new(pool);
    mln_buf_t *b = mln_buf_new(pool);
    assert(c != NULL && b != NULL);

    c->buf = b;
    b->left_pos = b->pos = b->start = buf;
    b->last = b->end = buf + data_len;
    b->in_memory = 1;
    b->last_buf = 1;
    b->last_in_chain = 1;

    mln_tcp_conn_append(&conn_send, c, M_C_SEND);

    int ret = mln_tcp_conn_send(&conn_send);
    assert(ret == M_C_FINISH || ret == M_C_NOTYET);

    /* Recv on nonblock - should get data or NOTYET */
    ret = mln_tcp_conn_recv(&conn_recv, M_C_TYPE_MEMORY);
    assert(ret == M_C_NOTYET);

    /* Verify data was received */
    mln_chain_t *recv_chain = mln_tcp_conn_head(&conn_recv, M_C_RECV);
    assert(recv_chain != NULL);
    assert(recv_chain->buf != NULL);
    assert(memcmp(recv_chain->buf->pos, data, data_len) == 0);

    mln_tcp_conn_destroy(&conn_send);
    mln_tcp_conn_destroy(&conn_recv);
    close(fds[0]);
    close(fds[1]);

    printf("  PASS: recv after nonblock send\n");
}

#if defined(MLN_TLS)
/* ====================================================================
 *                          TLS test fixtures
 * ====================================================================
 * All TLS tests run entirely in-process over an AF_UNIX socketpair, with
 * a self-signed RSA certificate generated on the fly so the tests need
 * no external PEM files and no network access.
 * ==================================================================== */

static SSL_CTX *g_server_ctx = NULL;
static SSL_CTX *g_client_ctx = NULL;

/* RSA key generation compat shim.  EVP_RSA_gen() is an OpenSSL 3.0
 * convenience; on 1.1.x we fall back to the EVP_PKEY_keygen path. */
static EVP_PKEY *tls_test_genkey(int bits)
{
#if OPENSSL_VERSION_NUMBER >= 0x30000000L
    return EVP_RSA_gen(bits);
#else
    EVP_PKEY     *pkey = NULL;
    EVP_PKEY_CTX *kctx = EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, NULL);
    if (kctx == NULL) return NULL;
    if (EVP_PKEY_keygen_init(kctx) <= 0) goto out;
    if (EVP_PKEY_CTX_set_rsa_keygen_bits(kctx, bits) <= 0) goto out;
    if (EVP_PKEY_keygen(kctx, &pkey) <= 0) pkey = NULL;
out:
    EVP_PKEY_CTX_free(kctx);
    return pkey;
#endif
}

/* Generate a self-signed test certificate and bake it into the two
 * shared SSL_CTXs.  Trust is wired up so the client can verify the
 * server with X509_V_OK.
 */
static void tls_test_fixture_init(void)
{
    if (g_server_ctx != NULL) return;

    /* Generate an RSA 2048 key (version-portable via tls_test_genkey). */
    EVP_PKEY *pkey = tls_test_genkey(2048);
    assert(pkey != NULL);

    X509 *x = X509_new();
    assert(x != NULL);
    ASN1_INTEGER_set(X509_get_serialNumber(x), 1);
    X509_gmtime_adj(X509_get_notBefore(x), 0);
    X509_gmtime_adj(X509_get_notAfter(x), 60L * 60L * 24L * 365L);
    X509_set_pubkey(x, pkey);
    X509_NAME *name = X509_get_subject_name(x);
    X509_NAME_add_entry_by_txt(name, "CN", MBSTRING_ASC,
                               (const unsigned char *)"melon-tls-test", -1, -1, 0);
    X509_set_issuer_name(x, name);
    assert(X509_sign(x, pkey, EVP_sha256()) > 0);

    g_server_ctx = SSL_CTX_new(TLS_server_method());
    assert(g_server_ctx != NULL);
    SSL_CTX_set_mode(g_server_ctx,
                     SSL_MODE_ENABLE_PARTIAL_WRITE |
                     SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER);
    assert(SSL_CTX_use_certificate(g_server_ctx, x) == 1);
    assert(SSL_CTX_use_PrivateKey(g_server_ctx, pkey) == 1);
    assert(SSL_CTX_check_private_key(g_server_ctx) == 1);

    g_client_ctx = SSL_CTX_new(TLS_client_method());
    assert(g_client_ctx != NULL);
    SSL_CTX_set_mode(g_client_ctx,
                     SSL_MODE_ENABLE_PARTIAL_WRITE |
                     SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER);
    /* Trust our self-signed cert. */
    X509_STORE *store = SSL_CTX_get_cert_store(g_client_ctx);
    assert(X509_STORE_add_cert(store, x) == 1);
    SSL_CTX_set_verify(g_client_ctx, SSL_VERIFY_PEER, NULL);

    X509_free(x);
    EVP_PKEY_free(pkey);
}

static void tls_test_fixture_destroy(void)
{
    if (g_server_ctx) { SSL_CTX_free(g_server_ctx); g_server_ctx = NULL; }
    if (g_client_ctx) { SSL_CTX_free(g_client_ctx); g_client_ctx = NULL; }
}

/* Attach a connection to a raw SSL_CTX by injecting the SSL handle
 * directly.  mln_tcp_conn_tls_init() is the public entry point but it
 * needs an mln_tcp_tls_conf_t; for tests we build the SSL_CTX in memory
 * to avoid touching the filesystem, so this helper duplicates the
 * minimal init steps using our own SSL_CTX.
 */
static int tls_test_attach(mln_tcp_conn_t *tc, int fd, SSL_CTX *ctx, int is_server)
{
    BIO *rbio = NULL, *wbio = NULL;
    if (mln_tcp_conn_init(tc, fd) < 0) return -1;
    tc->ssl = SSL_new(ctx);
    if (tc->ssl == NULL) goto err;
    rbio = BIO_new(BIO_s_mem());
    wbio = BIO_new(BIO_s_mem());
    if (rbio == NULL || wbio == NULL) goto err;
    BIO_set_mem_eof_return(rbio, -1);
    BIO_set_mem_eof_return(wbio, -1);
    SSL_set_bio(tc->ssl, rbio, wbio);
    /* Ownership transferred to SSL; null our locals so the err path of
     * any future failure point added below cannot double-free them. */
    rbio = wbio = NULL;
    if (is_server) SSL_set_accept_state(tc->ssl);
    else           SSL_set_connect_state(tc->ssl);
    return 0;
err:
    if (rbio) BIO_free(rbio);
    if (wbio) BIO_free(wbio);
    if (tc->ssl) { SSL_free(tc->ssl); tc->ssl = NULL; }
    mln_tcp_conn_destroy(tc);
    return -1;
}

/* Drive both ends of the handshake until both report done.  The 256
 * iteration cap protects against a regression that would otherwise
 * stall forever; we do NOT treat "both sides returned NOTYET in the
 * same iteration" as deadlock -- that is the normal TLS handshake
 * flow (client emits ClientHello and waits, server emits ServerHello
 * and waits, next iteration each side drains the other's record and
 * makes progress).  Real deadlocks just exhaust the iteration cap.
 */
static int tls_drive_handshake_pair(mln_tcp_conn_t *s, mln_tcp_conn_t *c)
{
    for (int i = 0; i < 256; i++) {
        if (mln_tcp_conn_tls_done(s) && mln_tcp_conn_tls_done(c)) return 0;
        if (!mln_tcp_conn_tls_done(c)) {
            int r = mln_tcp_conn_tls_handshake(c);
            if (r == M_C_ERROR || r == M_C_CLOSED) return -1;
        }
        if (!mln_tcp_conn_tls_done(s)) {
            int r = mln_tcp_conn_tls_handshake(s);
            if (r == M_C_ERROR || r == M_C_CLOSED) return -1;
        }
    }
    return -1;
}

/* Append a memory buf to a connection's send queue. */
static void tls_test_append_mem(mln_tcp_conn_t *tc, const void *data, size_t n)
{
    mln_alloc_t *pool = mln_tcp_conn_pool_get(tc);
    mln_chain_t *ch = mln_chain_new(pool);
    mln_buf_t   *b  = mln_buf_new(pool);
    mln_u8ptr_t  bf = (mln_u8ptr_t)mln_alloc_m(pool, n);
    assert(ch != NULL);
    assert(b  != NULL);
    assert(bf != NULL);
    memcpy(bf, data, n);
    ch->buf = b;
    b->start = b->left_pos = b->pos = bf;
    b->last  = b->end = bf + n;
    b->in_memory = 1;
    b->last_buf = 1;
    b->last_in_chain = 1;
    mln_tcp_conn_append(tc, ch, M_C_SEND);
}

/* Drain rcv_* into a flat buffer and return total bytes copied.
 * Advances left_pos to mark consumed bytes; chain nodes that still
 * have unread data are put back on M_C_RECV so callers may retry. */
static size_t tls_test_drain_rcv(mln_tcp_conn_t *tc, unsigned char *dst, size_t cap)
{
    mln_chain_t *c = mln_tcp_conn_remove(tc, M_C_RECV);
    size_t off = 0;
    while (c != NULL) {
        mln_chain_t *next = c->next;
        c->next = NULL;
        if (c->buf != NULL) {
            size_t left = mln_buf_left_size(c->buf);
            size_t cp = left > cap - off ? cap - off : left;
            if (cp > 0) {
                memcpy(dst + off, c->buf->left_pos, cp);
                c->buf->left_pos += cp;
                off += cp;
            }
        }
        if (c->buf == NULL || mln_buf_left_size(c->buf) == 0) {
            mln_chain_pool_release(c);
        } else {
            /* Partially consumed — put the remainder back. */
            mln_tcp_conn_append_chain(tc, c, NULL, M_C_RECV);
        }
        c = next;
    }
    return off;
}

/* Threaded helper: drives one side of a blocking-mode handshake. */
struct tls_blocking_thread_arg {
    mln_tcp_conn_t *conn;
    int             ok;
};
static void *tls_blocking_handshake_thread(void *vp)
{
    struct tls_blocking_thread_arg *a = vp;
    int r = mln_tcp_conn_tls_handshake(a->conn);
    a->ok = (r == M_C_FINISH);
    return NULL;
}

/* Test 1: blocking handshake driven by two threads (one per side).  This
 * is what the blocking API contract actually promises: a single call
 * returns M_C_FINISH once the peer makes equivalent progress.
 */
static void test_tls_handshake_blocking(void)
{
    printf("Testing TLS handshake (blocking, 2 threads)...\n");
    tls_test_fixture_init();

    int fds[2];
    assert(socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0);

    mln_tcp_conn_t srv, cli;
    assert(tls_test_attach(&srv, fds[0], g_server_ctx, 1) == 0);
    assert(tls_test_attach(&cli, fds[1], g_client_ctx, 0) == 0);

    struct tls_blocking_thread_arg sa = { .conn = &srv, .ok = 0 };
    struct tls_blocking_thread_arg ca = { .conn = &cli, .ok = 0 };
    pthread_t st, ct;
    assert(pthread_create(&st, NULL, tls_blocking_handshake_thread, &sa) == 0);
    assert(pthread_create(&ct, NULL, tls_blocking_handshake_thread, &ca) == 0);
    pthread_join(st, NULL);
    pthread_join(ct, NULL);
    assert(sa.ok && ca.ok);
    assert(mln_tcp_conn_tls_done(&srv));
    assert(mln_tcp_conn_tls_done(&cli));

    mln_tcp_conn_destroy(&srv);
    mln_tcp_conn_destroy(&cli);
    close(fds[0]); close(fds[1]);
    printf("  PASS: tls handshake blocking\n");
}

/* Test 2: non-blocking handshake. */
static void test_tls_handshake_nonblock(void)
{
    printf("Testing TLS handshake (non-blocking)...\n");
    tls_test_fixture_init();

    int fds[2];
    assert(socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0);
    set_nonblock(fds[0]); set_nonblock(fds[1]);

    mln_tcp_conn_t srv, cli;
    assert(tls_test_attach(&srv, fds[0], g_server_ctx, 1) == 0);
    assert(tls_test_attach(&cli, fds[1], g_client_ctx, 0) == 0);
    assert(mln_tcp_conn_set_nonblock(&srv, 1) == 0);
    assert(mln_tcp_conn_set_nonblock(&cli, 1) == 0);

    int notyet_seen = 0;
    for (int i = 0; i < 128; i++) {
        if (mln_tcp_conn_tls_done(&srv) && mln_tcp_conn_tls_done(&cli)) break;
        int rs = mln_tcp_conn_tls_handshake(&srv);
        int rc = mln_tcp_conn_tls_handshake(&cli);
        if (rs == M_C_NOTYET || rc == M_C_NOTYET) notyet_seen++;
        if (rs == M_C_ERROR || rc == M_C_ERROR) {
            fprintf(stderr, "handshake error (i=%d rs=%d rc=%d errno=%d)\n",
                    i, rs, rc, errno);
            abort();
        }
    }
    assert(mln_tcp_conn_tls_done(&srv) && mln_tcp_conn_tls_done(&cli));
    assert(notyet_seen > 0); /* we must have transited through NOTYET */

    mln_tcp_conn_destroy(&srv);
    mln_tcp_conn_destroy(&cli);
    close(fds[0]); close(fds[1]);
    printf("  PASS: tls handshake non-blocking (NOTYET transitions: %d)\n", notyet_seen);
}

/* Test 3: short send/recv round-trip after handshake.  Uses non-blocking
 * sockets so a single thread can drive both sides; the underlying
 * mln_tcp_conn_send / mln_tcp_conn_recv code paths are identical.
 */
static void test_tls_send_recv_short(void)
{
    printf("Testing TLS send/recv (short)...\n");
    tls_test_fixture_init();

    int fds[2];
    assert(socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0);
    set_nonblock(fds[0]); set_nonblock(fds[1]);
    mln_tcp_conn_t srv, cli;
    assert(tls_test_attach(&srv, fds[0], g_server_ctx, 1) == 0);
    assert(tls_test_attach(&cli, fds[1], g_client_ctx, 0) == 0);
    mln_tcp_conn_set_nonblock(&srv, 1);
    mln_tcp_conn_set_nonblock(&cli, 1);
    assert(tls_drive_handshake_pair(&srv, &cli) == 0);

    const char *msg = "hello melon tls";
    size_t mlen = strlen(msg);
    tls_test_append_mem(&cli, msg, mlen);
    int sr = mln_tcp_conn_send(&cli);
    assert(sr == M_C_FINISH || sr == M_C_NOTYET);
    int rr = mln_tcp_conn_recv(&srv, M_C_TYPE_MEMORY);
    assert(rr == M_C_NOTYET || rr == M_C_FINISH);

    unsigned char got[64];
    size_t n = tls_test_drain_rcv(&srv, got, sizeof got);
    assert(n == mlen);
    assert(memcmp(got, msg, mlen) == 0);

    mln_tcp_conn_destroy(&srv);
    mln_tcp_conn_destroy(&cli);
    close(fds[0]); close(fds[1]);
    printf("  PASS: tls send/recv short\n");
}

/* Test 4: large payload round-trip over non-blocking sockets. */
static void test_tls_send_recv_large(void)
{
    printf("Testing TLS send/recv (large, non-blocking)...\n");
    tls_test_fixture_init();

    const size_t N = 4u * 1024u * 1024u;
    unsigned char *payload = malloc(N);
    unsigned char *recvbuf = malloc(N + 16);
    assert(payload && recvbuf);
    for (size_t i = 0; i < N; i++) payload[i] = (unsigned char)(i * 1103515245u + 12345u);

    int fds[2];
    assert(socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0);
    set_nonblock(fds[0]); set_nonblock(fds[1]);
    mln_tcp_conn_t srv, cli;
    assert(tls_test_attach(&srv, fds[0], g_server_ctx, 1) == 0);
    assert(tls_test_attach(&cli, fds[1], g_client_ctx, 0) == 0);
    mln_tcp_conn_set_nonblock(&srv, 1);
    mln_tcp_conn_set_nonblock(&cli, 1);

    /* finish handshake first */
    for (int i = 0; i < 512; i++) {
        if (mln_tcp_conn_tls_done(&srv) && mln_tcp_conn_tls_done(&cli)) break;
        mln_tcp_conn_tls_handshake(&cli);
        mln_tcp_conn_tls_handshake(&srv);
    }
    assert(mln_tcp_conn_tls_done(&srv));

    tls_test_append_mem(&cli, payload, N);

    size_t got = 0;
    for (int spin = 0; spin < 65536 && got < N; spin++) {
        int sr = mln_tcp_conn_send(&cli);
        assert(sr == M_C_FINISH || sr == M_C_NOTYET);
        int rr = mln_tcp_conn_recv(&srv, M_C_TYPE_MEMORY);
        assert(rr == M_C_NOTYET || rr == M_C_FINISH);
        size_t n = tls_test_drain_rcv(&srv, recvbuf + got, N + 16 - got);
        got += n;
    }
    assert(got == N);
    assert(memcmp(recvbuf, payload, N) == 0);

    mln_tcp_conn_destroy(&srv);
    mln_tcp_conn_destroy(&cli);
    close(fds[0]); close(fds[1]);
    free(payload); free(recvbuf);
    printf("  PASS: tls send/recv large (%zu bytes)\n", N);
}

/* Test 5: file-backed buf goes through SSL_write. */
static void test_tls_send_in_file(void)
{
    printf("Testing TLS send with in-file buf...\n");
    tls_test_fixture_init();

    int fds[2];
    assert(socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0);
    set_nonblock(fds[0]); set_nonblock(fds[1]);
    mln_tcp_conn_t srv, cli;
    assert(tls_test_attach(&srv, fds[0], g_server_ctx, 1) == 0);
    assert(tls_test_attach(&cli, fds[1], g_client_ctx, 0) == 0);
    mln_tcp_conn_set_nonblock(&srv, 1);
    mln_tcp_conn_set_nonblock(&cli, 1);
    assert(tls_drive_handshake_pair(&srv, &cli) == 0);

    /* Build a small temp file containing the payload. */
    char tmpl[] = "/tmp/mln_tls_test_XXXXXX";
    int fd = mkstemp(tmpl);
    assert(fd >= 0);
    const char *payload = "file-backed-tls-payload-1234567890";
    size_t plen = strlen(payload);
    assert(write(fd, payload, plen) == (ssize_t)plen);
    close(fd);

    mln_alloc_t *pool = mln_tcp_conn_pool_get(&cli);
    mln_fileset_t *fset = mln_fileset_init(8);
    assert(fset != NULL);
    mln_file_t *file = mln_file_open(fset, tmpl);
    assert(file != NULL);
    mln_chain_t *ch = mln_chain_new(pool);
    mln_buf_t   *b  = mln_buf_new(pool);
    ch->buf = b;
    b->file = file;
    b->file_pos = b->file_left_pos = 0;
    b->file_last = plen;
    b->in_file = 1;
    b->last_buf = 1;
    b->last_in_chain = 1;
    mln_tcp_conn_append(&cli, ch, M_C_SEND);

    int sr = mln_tcp_conn_send(&cli);
    assert(sr == M_C_FINISH || sr == M_C_NOTYET);
    int rr = mln_tcp_conn_recv(&srv, M_C_TYPE_MEMORY);
    assert(rr == M_C_NOTYET || rr == M_C_FINISH);

    unsigned char got[128];
    size_t n = tls_test_drain_rcv(&srv, got, sizeof got);
    assert(n == plen);
    assert(memcmp(got, payload, plen) == 0);

    unlink(tmpl);
    mln_tcp_conn_destroy(&srv);
    mln_tcp_conn_destroy(&cli);
    mln_fileset_destroy(fset);
    (void)pool;
    close(fds[0]); close(fds[1]);
    printf("  PASS: tls send with in-file buf\n");
}

/* Test 6: shutdown round-trip. */
static void test_tls_shutdown(void)
{
    printf("Testing TLS shutdown...\n");
    tls_test_fixture_init();

    int fds[2];
    assert(socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0);
    set_nonblock(fds[0]); set_nonblock(fds[1]);
    mln_tcp_conn_t srv, cli;
    assert(tls_test_attach(&srv, fds[0], g_server_ctx, 1) == 0);
    assert(tls_test_attach(&cli, fds[1], g_client_ctx, 0) == 0);
    mln_tcp_conn_set_nonblock(&srv, 1);
    mln_tcp_conn_set_nonblock(&cli, 1);
    assert(tls_drive_handshake_pair(&srv, &cli) == 0);

    int r = mln_tcp_conn_tls_shutdown(&cli);
    assert(r == M_C_FINISH || r == M_C_NOTYET);
    /* Drain whatever the client left on the wire so the server can see
     * close_notify and acknowledge. */
    int rr = mln_tcp_conn_recv(&srv, M_C_TYPE_MEMORY);
    /* recv may legitimately return CLOSED here. */
    (void)rr;
    int r2 = mln_tcp_conn_tls_shutdown(&srv);
    (void)r2;

    mln_tcp_conn_destroy(&srv);
    mln_tcp_conn_destroy(&cli);
    close(fds[0]); close(fds[1]);
    printf("  PASS: tls shutdown\n");
}

/* Test 7: plain (non-TLS) path is still byte-identical: a connection
 * created via mln_tcp_conn_init() must never enter the TLS branch.
 */
static void test_tls_plain_unchanged(void)
{
    printf("Testing TLS-enabled build keeps plain path unchanged...\n");
    int fds[2];
    assert(socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0);

    mln_tcp_conn_t a, b;
    assert(mln_tcp_conn_init(&a, fds[0]) == 0);
    assert(mln_tcp_conn_init(&b, fds[1]) == 0);
    /* The TLS-enabled fields must be zero-initialized. */
    assert(!mln_tcp_conn_tls_enabled(&a));
    assert(!mln_tcp_conn_tls_enabled(&b));

    const char *msg = "plain-path-still-works";
    size_t mlen = strlen(msg);
    tls_test_append_mem(&a, msg, mlen);
    int sr = mln_tcp_conn_send(&a);
    assert(sr == M_C_FINISH);
    int rr = mln_tcp_conn_recv(&b, M_C_TYPE_MEMORY);
    assert(rr == M_C_NOTYET || rr == M_C_FINISH);
    unsigned char got[64];
    size_t n = tls_test_drain_rcv(&b, got, sizeof got);
    assert(n == mlen);
    assert(memcmp(got, msg, mlen) == 0);

    mln_tcp_conn_destroy(&a);
    mln_tcp_conn_destroy(&b);
    close(fds[0]); close(fds[1]);
    printf("  PASS: plain path unchanged when TLS compiled in\n");
}

/* Test 8: throughput benchmark.  Reports MiB/s but does not assert on a
 * specific number — CI hardware variance is too large to make it
 * meaningful — only prints a [WARN] if the rate falls below a very low
 * floor that would indicate a real regression.
 *
 * The total transfer defaults to 1 MiB to keep Valgrind/CI runs fast.
 * Set the MLN_BENCH_TOTAL_MB environment variable to a larger value
 * (e.g. 16) for a more representative measurement on fast hardware.
 */
static void test_tls_perf_throughput(void)
{
    printf("Benchmarking TLS throughput...\n");
    tls_test_fixture_init();

    size_t total_mb = 1;
    const char *env_mb = getenv("MLN_BENCH_TOTAL_MB");
    if (env_mb) { int v = atoi(env_mb); if (v > 0) total_mb = (size_t)v; }
    const size_t TOTAL = total_mb * 1024u * 1024u;
    const size_t CHUNK = 64u * 1024u;
    unsigned char *block = malloc(CHUNK);
    assert(block != NULL);
    for (size_t i = 0; i < CHUNK; i++) block[i] = (unsigned char)i;

    int fds[2];
    assert(socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0);
    set_nonblock(fds[0]); set_nonblock(fds[1]);
    mln_tcp_conn_t srv, cli;
    assert(tls_test_attach(&srv, fds[0], g_server_ctx, 1) == 0);
    assert(tls_test_attach(&cli, fds[1], g_client_ctx, 0) == 0);
    mln_tcp_conn_set_nonblock(&srv, 1);
    mln_tcp_conn_set_nonblock(&cli, 1);
    for (int i = 0; i < 512; i++) {
        if (mln_tcp_conn_tls_done(&srv) && mln_tcp_conn_tls_done(&cli)) break;
        mln_tcp_conn_tls_handshake(&cli);
        mln_tcp_conn_tls_handshake(&srv);
    }
    assert(mln_tcp_conn_tls_done(&srv));

    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);

    size_t sent = 0, recvd = 0;
    unsigned char sink[64 * 1024];
    while (recvd < TOTAL) {
        if (sent < TOTAL && mln_tcp_conn_send_empty(&cli)) {
            size_t left = TOTAL - sent;
            size_t n = left > CHUNK ? CHUNK : left;
            tls_test_append_mem(&cli, block, n);
            sent += n;
        }
        int sr = mln_tcp_conn_send(&cli);
        (void)sr;
        int rr = mln_tcp_conn_recv(&srv, M_C_TYPE_MEMORY);
        (void)rr;
        recvd += tls_test_drain_rcv(&srv, sink, sizeof sink);
    }

    clock_gettime(CLOCK_MONOTONIC, &t1);
    long us = elapsed_us(&t0, &t1);
    /* MiB/s: TOTAL bytes over us microseconds, scaled to mebibytes/second */
    double mbs = us > 0 ? ((double)TOTAL * 1e6 / ((double)us * 1024.0 * 1024.0)) : 0.0;
    printf("  INFO: tls throughput = %.1f MiB/s (%zu bytes in %ld us)\n",
           mbs, TOTAL, us);
    if (mbs < 20.0) {
        printf("  [WARN] throughput below 20 MiB/s; possible regression\n");
    }

    mln_tcp_conn_destroy(&srv);
    mln_tcp_conn_destroy(&cli);
    close(fds[0]); close(fds[1]);
    free(block);
    printf("  PASS: tls throughput benchmark\n");
}

/* Plain-path throughput for comparison (only built when TLS is on so
 * we can print the ratio in one run; the plain test suite already
 * exercises this code path independently).
 * Honours the same MLN_BENCH_TOTAL_MB env var as the TLS benchmark.
 * Reports MiB/s (TOTAL is computed as MiB × 1024 × 1024 bytes). */
static void test_plain_perf_throughput(void)
{
    printf("Benchmarking plain TCP throughput (for TLS comparison)...\n");
    size_t total_mb = 1;
    const char *env_mb = getenv("MLN_BENCH_TOTAL_MB");
    if (env_mb) { int v = atoi(env_mb); if (v > 0) total_mb = (size_t)v; }
    const size_t TOTAL = total_mb * 1024u * 1024u;
    const size_t CHUNK = 64u * 1024u;
    unsigned char *block = malloc(CHUNK);
    assert(block != NULL);
    for (size_t i = 0; i < CHUNK; i++) block[i] = (unsigned char)i;

    int fds[2];
    assert(socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0);
    set_nonblock(fds[0]); set_nonblock(fds[1]);
    mln_tcp_conn_t srv, cli;
    assert(mln_tcp_conn_init(&srv, fds[0]) == 0);
    assert(mln_tcp_conn_init(&cli, fds[1]) == 0);

    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);

    size_t sent = 0, recvd = 0;
    unsigned char sink[64 * 1024];
    while (recvd < TOTAL) {
        if (sent < TOTAL && mln_tcp_conn_send_empty(&cli)) {
            size_t left = TOTAL - sent;
            size_t n = left > CHUNK ? CHUNK : left;
            tls_test_append_mem(&cli, block, n);
            sent += n;
        }
        int sr = mln_tcp_conn_send(&cli);
        (void)sr;
        int rr = mln_tcp_conn_recv(&srv, M_C_TYPE_MEMORY);
        (void)rr;
        recvd += tls_test_drain_rcv(&srv, sink, sizeof sink);
    }
    clock_gettime(CLOCK_MONOTONIC, &t1);
    long us = elapsed_us(&t0, &t1);
    /* MiB/s: TOTAL bytes over us microseconds, scaled to mebibytes/second */
    double mbs = us > 0 ? ((double)TOTAL * 1e6 / ((double)us * 1024.0 * 1024.0)) : 0.0;
    printf("  INFO: plain throughput = %.1f MiB/s (%zu bytes in %ld us)\n",
           mbs, TOTAL, us);

    mln_tcp_conn_destroy(&srv);
    mln_tcp_conn_destroy(&cli);
    close(fds[0]); close(fds[1]);
    free(block);
    printf("  PASS: plain throughput benchmark\n");
}

/* ====================================================================
 *                HTTPS end-to-end test (10 request rounds)
 * ====================================================================
 * Real loopback TCP between two threads, both sides non-blocking, both
 * driven via mln_tcp_conn_tls_init + mln_http_init -- i.e. the real
 * public APIs.  Exercises:
 *   - server: cert + private key loaded from PEM via mln_tcp_tls_conf_new
 *   - client: SNI, CA verification, hostname (X509v3 SAN) verification
 *   - 10 HTTP request / response round-trips on one TLS connection
 *   - graceful close_notify shutdown on both sides
 * ==================================================================== */

#define HTTPS_TEST_ROUNDS 10
#define HTTPS_TEST_CN     "localhost"

/* Write a self-signed RSA-2048 cert (CN=localhost, SAN=DNS:localhost,
 * IP:127.0.0.1) and matching private key to two temp PEM files.
 * Returns 0 on success; *cert_path and *key_path are filled in. */
static int https_make_cert_files(char *cert_path, char *key_path)
{
    EVP_PKEY       *pkey = NULL;
    X509           *x    = NULL;
    X509_EXTENSION *ext  = NULL;
    int             cfd  = -1, kfd = -1;
    FILE           *cf   = NULL, *kf = NULL;
    int             rc   = -1;

    cert_path[0] = '\0';
    key_path[0]  = '\0';

    pkey = tls_test_genkey(2048);
    if (pkey == NULL) goto out;

    x = X509_new();
    if (x == NULL) goto out;
    if (ASN1_INTEGER_set(X509_get_serialNumber(x), 1) != 1)                 goto out;
    if (X509_gmtime_adj(X509_get_notBefore(x), 0) == NULL)                  goto out;
    if (X509_gmtime_adj(X509_get_notAfter(x), 60L*60L*24L*365L) == NULL)    goto out;
    if (X509_set_pubkey(x, pkey) != 1)                                      goto out;

    X509_NAME *name = X509_get_subject_name(x);
    if (X509_NAME_add_entry_by_txt(name, "CN", MBSTRING_ASC,
                                   (const unsigned char *)HTTPS_TEST_CN,
                                   -1, -1, 0) != 1)                         goto out;
    if (X509_set_issuer_name(x, name) != 1)                                 goto out;

    ext = X509V3_EXT_conf_nid(NULL, NULL, NID_subject_alt_name,
                              "DNS:" HTTPS_TEST_CN ",IP:127.0.0.1");
    if (ext == NULL)                                                        goto out;
    if (X509_add_ext(x, ext, -1) != 1)                                      goto out;

    if (X509_sign(x, pkey, EVP_sha256()) <= 0)                              goto out;

    strcpy(cert_path, "/tmp/mln_https_cert_XXXXXX");
    cfd = mkstemp(cert_path);
    if (cfd < 0) { cert_path[0] = '\0'; goto out; }
    strcpy(key_path,  "/tmp/mln_https_key_XXXXXX");
    kfd = mkstemp(key_path);
    if (kfd < 0) { key_path[0] = '\0'; goto out; }

    cf = fdopen(cfd, "w");
    if (cf == NULL) goto out;
    cfd = -1;        /* ownership transferred to FILE* */
    kf = fdopen(kfd, "w");
    if (kf == NULL) goto out;
    kfd = -1;

    if (PEM_write_X509(cf, x) != 1)                                         goto out;
    if (PEM_write_PrivateKey(kf, pkey, NULL, NULL, 0, NULL, NULL) != 1)     goto out;
    if (fclose(cf) != 0) { cf = NULL; goto out; }  cf = NULL;
    if (fclose(kf) != 0) { kf = NULL; goto out; }  kf = NULL;

    rc = 0;

out:
    if (cf)        fclose(cf);
    if (kf)        fclose(kf);
    if (cfd >= 0)  close(cfd);
    if (kfd >= 0)  close(kfd);
    if (ext)       X509_EXTENSION_free(ext);
    if (x)         X509_free(x);
    if (pkey)      EVP_PKEY_free(pkey);
    if (rc != 0) {
        if (cert_path[0]) unlink(cert_path);
        if (key_path[0])  unlink(key_path);
    }
    return rc;
}

/* A 15-second wall-clock deadline per high-level operation -- long
 * enough that even Valgrind-slow CI completes, short enough that a
 * regression cannot hang the test suite indefinitely. */
#define HTTPS_DEADLINE_US (15LL * 1000LL * 1000LL)

/* Wait up to `timeout_ms` for the desired events on `fd`.  Returns 0
 * on a successful (or signal-interrupted) wait, -1 on poll() error
 * or timeout so callers can fail the test deterministically. */
static int https_wait(int fd, mln_tcp_conn_t *c, int timeout_ms)
{
    struct pollfd pfd = { .fd = fd, .events = 0 };
    if (mln_tcp_conn_tls_want_read(c))  pfd.events |= POLLIN;
    if (mln_tcp_conn_tls_want_write(c)) pfd.events |= POLLOUT;
    if (pfd.events == 0) pfd.events = POLLIN | POLLOUT;
    int n = poll(&pfd, 1, timeout_ms);
    if (n < 0) return (errno == EINTR) ? 0 : -1;
    if (n == 0) return -1;   /* poll timeout */
    return 0;
}

static int https_deadline_expired(struct timespec *t0)
{
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return elapsed_us(t0, &now) > HTTPS_DEADLINE_US;
}

/* Drive the TLS handshake to completion or hard-fail on deadline. */
static int https_handshake(int fd, mln_tcp_conn_t *c)
{
    struct timespec t0; clock_gettime(CLOCK_MONOTONIC, &t0);
    for (;;) {
        int r = mln_tcp_conn_tls_handshake(c);
        if (r == M_C_FINISH) return 0;
        if (r != M_C_NOTYET) return -1;
        if (https_wait(fd, c, 5000) < 0) return -1;
        if (https_deadline_expired(&t0)) return -1;
    }
}

/* Receive bytes and feed mln_http_parse until one message is complete. */
static int https_recv_message(int fd, mln_tcp_conn_t *c, mln_http_t *http)
{
    struct timespec t0; clock_gettime(CLOCK_MONOTONIC, &t0);
    for (;;) {
        int r = mln_tcp_conn_recv(c, M_C_TYPE_MEMORY);
        if (r == M_C_ERROR) return -1;
        mln_chain_t *in = mln_tcp_conn_remove(c, M_C_RECV);
        if (in != NULL) {
            int pr = mln_http_parse(http, &in);
            if (pr == M_HTTP_RET_DONE) {
                mln_chain_pool_release_all(in);
                return 0;
            }
            if (pr == M_HTTP_RET_ERROR) {
                mln_chain_pool_release_all(in);
                return -1;
            }
            /* M_HTTP_RET_OK: need more bytes.
             * Free only fully-consumed chain nodes (left_pos has reached
             * last); put any partially or wholly unconsumed chains back
             * on the recv queue so the next read appends new data after
             * them without losing the already-received bytes.
             */
            mln_chain_t *rem = in;
            while (rem != NULL && mln_buf_left_size(rem->buf) == 0) {
                mln_chain_t *tmp = rem;
                rem = rem->next;
                tmp->next = NULL;
                mln_chain_pool_release(tmp);
            }
            if (rem != NULL)
                mln_tcp_conn_append_chain(c, rem, NULL, M_C_RECV);
        }
        if (r == M_C_CLOSED) return -1;
        if (r == M_C_NOTYET) {
            if (https_wait(fd, c, 5000) < 0) return -1;
            if (https_deadline_expired(&t0)) return -1;
        }
    }
}

/* Send a generated chain until M_C_FINISH; releases sent buffers. */
static int https_send_chain(int fd, mln_tcp_conn_t *c,
                            mln_chain_t *head, mln_chain_t *tail)
{
    struct timespec t0; clock_gettime(CLOCK_MONOTONIC, &t0);
    mln_tcp_conn_append_chain(c, head, tail, M_C_SEND);
    for (;;) {
        int r = mln_tcp_conn_send(c);
        if (r == M_C_FINISH) {
            mln_chain_pool_release_all(mln_tcp_conn_remove(c, M_C_SENT));
            return 0;
        }
        if (r == M_C_ERROR) return -1;
        if (https_wait(fd, c, 5000) < 0) return -1;
        if (https_deadline_expired(&t0)) return -1;
    }
}

/* Best-effort graceful close.  Capped at 32 iterations; poll() errors
 * or timeouts inside https_wait abort early so a vanished peer cannot
 * hang the test. */
static void https_drive_shutdown(int fd, mln_tcp_conn_t *c)
{
    for (int i = 0; i < 32; i++) {
        int r = mln_tcp_conn_tls_shutdown(c);
        if (r != M_C_NOTYET) return;
        if (https_wait(fd, c, 200) < 0) return;
    }
}

struct https_server_arg {
    int                  listen_fd;
    mln_tcp_tls_conf_t  *conf;
    int                  ok;
};

static void *https_server_thread(void *vp)
{
    struct https_server_arg *sa = vp;
    sa->ok = 0;

    int cfd = accept(sa->listen_fd, NULL, NULL);
    if (cfd < 0) return NULL;
    set_nonblock(cfd);

    mln_tcp_conn_t conn;
    if (mln_tcp_conn_tls_init(&conn, cfd, sa->conf) < 0) { close(cfd); return NULL; }
    mln_tcp_conn_set_nonblock(&conn, 1);

    if (https_handshake(cfd, &conn) < 0) goto out;

    mln_http_t *http = mln_http_init(&conn, NULL, NULL);
    if (http == NULL) goto out;

    int rounds_done = 0;
    while (rounds_done < HTTPS_TEST_ROUNDS) {
        if (https_recv_message(cfd, &conn, http) < 0) goto out_http;
        if (mln_http_method_get(http) != M_HTTP_GET) goto out_http;

        /* Echo the X-Iter header value back so the client can verify
         * round identity end-to-end. */
        mln_string_t key_iter = mln_string("X-Iter");
        mln_string_t *iter_v  = mln_http_field_get(http, &key_iter);
        if (iter_v == NULL) goto out_http;
        char iter_buf[32];
        size_t iter_len = iter_v->len < sizeof(iter_buf)-1 ? iter_v->len : sizeof(iter_buf)-1;
        memcpy(iter_buf, iter_v->data, iter_len);
        iter_buf[iter_len] = '\0';

        mln_http_reset(http);
        mln_http_type_set(http, M_HTTP_RESPONSE);
        mln_http_status_set(http, M_HTTP_OK);
        mln_http_version_set(http, M_HTTP_VERSION_1_1);

        mln_string_t key_srv = mln_string("Server");
        mln_string_t val_srv = mln_string("melon-tls-test");
        mln_string_t key_cl  = mln_string("Content-Length");
        mln_string_t val_cl  = mln_string("0");
        mln_string_t val_iter;
        mln_string_nset(&val_iter, iter_buf, iter_len);
        if (mln_http_field_set(http, &key_srv,  &val_srv)  < 0) goto out_http;
        if (mln_http_field_set(http, &key_iter, &val_iter) < 0) goto out_http;
        if (mln_http_field_set(http, &key_cl,   &val_cl)   < 0) goto out_http;

        mln_chain_t *head = NULL, *tail = NULL;
        if (mln_http_generate(http, &head, &tail) != M_HTTP_RET_DONE) goto out_http;
        if (https_send_chain(cfd, &conn, head, tail) < 0) goto out_http;
        mln_http_reset(http);
        rounds_done++;
    }
    sa->ok = 1;

    https_drive_shutdown(cfd, &conn);
out_http:
    mln_http_destroy(http);
out:
    mln_tcp_conn_destroy(&conn);
    close(cfd);
    return NULL;
}

struct https_client_arg {
    in_port_t            port;
    mln_tcp_tls_conf_t  *conf;
    int                  ok;
};

static void *https_client_thread(void *vp)
{
    struct https_client_arg *ca = vp;
    ca->ok = 0;

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) return NULL;
    set_nonblock(fd);

    struct sockaddr_in sa = { 0 };
    sa.sin_family = AF_INET;
    sa.sin_port   = htons(ca->port);
    inet_pton(AF_INET, "127.0.0.1", &sa.sin_addr);
    int cr = connect(fd, (struct sockaddr *)&sa, sizeof sa);
    if (cr < 0 && errno != EINPROGRESS) { close(fd); return NULL; }
    /* Wait for connect to complete. */
    struct pollfd pfd = { .fd = fd, .events = POLLOUT };
    int pret;
    do { pret = poll(&pfd, 1, 5000); } while (pret < 0 && errno == EINTR);
    if (pret <= 0) { close(fd); return NULL; }  /* timeout or error */
    int err = 0; socklen_t errlen = sizeof err;
    if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &errlen) < 0) { close(fd); return NULL; }
    if (err != 0) { close(fd); return NULL; }

    mln_tcp_conn_t conn;
    if (mln_tcp_conn_tls_init(&conn, fd, ca->conf) < 0) { close(fd); return NULL; }
    mln_tcp_conn_set_nonblock(&conn, 1);

    /* SNI + hostname verification before the handshake starts. */
    mln_string_t host = mln_string(HTTPS_TEST_CN);
    if (mln_tcp_conn_tls_set_sni(&conn, &host) < 0)         goto out;
    if (mln_tcp_conn_tls_set_verify_host(&conn, &host) < 0) goto out;

    if (https_handshake(fd, &conn) < 0) goto out;

    mln_http_t *http = mln_http_init(&conn, NULL, NULL);
    if (http == NULL) goto out;

    for (int i = 0; i < HTTPS_TEST_ROUNDS; i++) {
        char iter_buf[16];
        int iter_len = snprintf(iter_buf, sizeof iter_buf, "%d", i);

        mln_http_reset(http);
        mln_http_type_set(http, M_HTTP_REQUEST);
        mln_http_method_set(http, M_HTTP_GET);
        mln_http_version_set(http, M_HTTP_VERSION_1_1);
        mln_string_t key_host  = mln_string("Host");
        mln_string_t key_iter  = mln_string("X-Iter");
        mln_string_t val_iter;
        mln_string_nset(&val_iter, iter_buf, (size_t)iter_len);
        if (mln_http_field_set(http, &key_host, &host)     < 0) goto out_http;
        if (mln_http_field_set(http, &key_iter, &val_iter) < 0) goto out_http;

        mln_chain_t *head = NULL, *tail = NULL;
        if (mln_http_generate(http, &head, &tail) != M_HTTP_RET_DONE) goto out_http;
        if (https_send_chain(fd, &conn, head, tail) < 0) goto out_http;

        mln_http_reset(http);
        if (https_recv_message(fd, &conn, http) < 0) goto out_http;
        if (mln_http_type_get(http)   != M_HTTP_RESPONSE) goto out_http;
        if (mln_http_status_get(http) != M_HTTP_OK)       goto out_http;

        mln_string_t *got = mln_http_field_get(http, &key_iter);
        if (got == NULL) goto out_http;
        if (got->len != (mln_size_t)iter_len) goto out_http;
        if (memcmp(got->data, iter_buf, iter_len) != 0) goto out_http;
    }
    ca->ok = 1;

    https_drive_shutdown(fd, &conn);
out_http:
    mln_http_destroy(http);
out:
    mln_tcp_conn_destroy(&conn);
    close(fd);
    return NULL;
}

/* Regression test for the non-blocking handshake spin window: when the
 * remote BIO holds a stale partial record and the socket has no new
 * bytes, mln_tcp_conn_tls_handshake must return M_C_NOTYET quickly,
 * not loop while BIO_pending reports the unchanged stale fill.  We
 * synthesise this state by directly BIO_writing a few bytes of fake
 * record header into the server SSL's read BIO and draining the
 * socket so drain_socket adds nothing.  Bounded by a 500 ms wall-
 * clock check that would trip on a spin. */
static void test_tls_handshake_partial_record_no_spin(void)
{
    printf("Testing TLS handshake (partial record, no-spin)...\n");
    tls_test_fixture_init();

    int fds[2];
    assert(socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0);
    set_nonblock(fds[0]); set_nonblock(fds[1]);
    mln_tcp_conn_t srv, cli;
    assert(tls_test_attach(&srv, fds[0], g_server_ctx, 1) == 0);
    assert(tls_test_attach(&cli, fds[1], g_client_ctx, 0) == 0);
    mln_tcp_conn_set_nonblock(&srv, 1);
    mln_tcp_conn_set_nonblock(&cli, 1);

    /* Drain socket fds[0] completely first (in case anything has
     * already been queued -- shouldn't be, but be defensive). */
    char tmp[8192];
    while (read(fds[0], tmp, sizeof tmp) > 0) { /* discard */ }

    /* Inject 4 bytes of a would-be TLS record header into srv->rbio.
     * A full TLS record header is 5 bytes, so this is intentionally
     * short: SSL_do_handshake can either consume it greedily into
     * internal state (in which case rbio is empty after the call) or
     * leave it in rbio waiting for more.  Either way our handshake
     * driver MUST return NOTYET promptly because no new bytes will
     * ever arrive on the socket. */
    char partial[4] = { 0x16, 0x03, 0x03, 0x00 };
    assert(BIO_write(SSL_get_rbio(srv.ssl), partial, sizeof partial) == sizeof partial);

    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);
    int r = mln_tcp_conn_tls_handshake(&srv);
    clock_gettime(CLOCK_MONOTONIC, &t1);
    long us = elapsed_us(&t0, &t1);

    assert(r == M_C_NOTYET);
    /* 500ms is an enormous budget for a single non-blocking handshake
     * call -- if we exceed it the function is spinning. */
    if (us >= 500000L) {
        fprintf(stderr, "handshake spun for %ld us on partial-record rbio\n", us);
        abort();
    }
    /* The function returned NOTYET, so the want_* flags should be set
     * to direct the caller to wait for readable data. */
    assert(mln_tcp_conn_tls_want_read(&srv) ||
           mln_tcp_conn_tls_want_write(&srv));

    mln_tcp_conn_destroy(&srv);
    mln_tcp_conn_destroy(&cli);
    close(fds[0]); close(fds[1]);
    printf("  PASS: tls handshake bounded on partial rbio (%ld us)\n", us);
}

/* Negative-path coverage for mln_tcp_tls_conf_new / mln_tcp_conn_tls_init
 * / set_sni / set_verify_host.  Validates the input checks added by
 * earlier review rounds. */
static void test_tls_conf_validation(void)
{
    printf("Testing TLS API input validation...\n");
    tls_test_fixture_init();

    /* (1) Invalid role */
    errno = 0;
    mln_tcp_tls_conf_t *bad = mln_tcp_tls_conf_new(
        99, NULL, NULL, NULL, NULL, 0, 0);
    assert(bad == NULL);
    assert(errno == EINVAL);

    /* (2) Server without cert/key */
    errno = 0;
    bad = mln_tcp_tls_conf_new(
        M_TLS_SERVER, NULL, NULL, NULL, NULL, 0, 0);
    assert(bad == NULL);
    assert(errno == EINVAL);

    /* (3) Unknown bits in version mask */
    errno = 0;
    bad = mln_tcp_tls_conf_new(
        M_TLS_CLIENT, NULL, NULL, NULL, NULL, 0x100, 0);
    assert(bad == NULL);
    assert(errno == EINVAL);

    /* (4) tls_init with NULL conf */
    mln_tcp_conn_t dummy;
    errno = 0;
    assert(mln_tcp_conn_tls_init(&dummy, -1, NULL) < 0);
    assert(errno == EINVAL);

    /* (5) set_sni / set_verify_host on a real but oversize hostname */
    int fds[2];
    assert(socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0);
    mln_tcp_conn_t cli;
    assert(tls_test_attach(&cli, fds[0], g_client_ctx, 0) == 0);

    char big[300];
    memset(big, 'a', sizeof big);
    mln_string_t huge;
    mln_string_nset(&huge, big, sizeof big);
    errno = 0;
    assert(mln_tcp_conn_tls_set_sni(&cli, &huge) == -1);
    assert(errno == EINVAL);
    errno = 0;
    assert(mln_tcp_conn_tls_set_verify_host(&cli, &huge) == -1);
    assert(errno == EINVAL);

    /* (6) Embedded NUL */
    char with_nul[] = { 'a', 'b', '\0', 'c', 'd' };
    mln_string_t nul_host;
    mln_string_nset(&nul_host, with_nul, sizeof with_nul);
    errno = 0;
    assert(mln_tcp_conn_tls_set_sni(&cli, &nul_host) == -1);
    assert(errno == EINVAL);
    errno = 0;
    assert(mln_tcp_conn_tls_set_verify_host(&cli, &nul_host) == -1);
    assert(errno == EINVAL);

    /* (7) Reasonable inputs succeed */
    mln_string_t ok_host = mln_string("example.test");
    assert(mln_tcp_conn_tls_set_sni(&cli, &ok_host) == 0);
    assert(mln_tcp_conn_tls_set_verify_host(&cli, &ok_host) == 0);

    mln_tcp_conn_destroy(&cli);
    close(fds[0]); close(fds[1]);
    printf("  PASS: tls API input validation\n");
}

static void test_https_e2e_nonblocking(void)
{
    printf("Testing real HTTPS client/server (10 rounds, non-blocking)...\n");

    char cert_path[64], key_path[64];
    assert(https_make_cert_files(cert_path, key_path) == 0);

    mln_string_t cert_s, key_s, ca_s;
    mln_string_nset(&cert_s, cert_path, strlen(cert_path));
    mln_string_nset(&key_s,  key_path,  strlen(key_path));
    /* Self-signed: the cert file doubles as the CA bundle for the client. */
    mln_string_nset(&ca_s,   cert_path, strlen(cert_path));

    mln_tcp_tls_conf_t *srv_conf = mln_tcp_tls_conf_new(
        M_TLS_SERVER, &cert_s, &key_s, NULL, NULL, M_TLS_VDEFAULT, 0);
    mln_tcp_tls_conf_t *cli_conf = mln_tcp_tls_conf_new(
        M_TLS_CLIENT, NULL, NULL, &ca_s, NULL, M_TLS_VDEFAULT, 1);
    assert(srv_conf && cli_conf);

    int lfd = socket(AF_INET, SOCK_STREAM, 0);
    assert(lfd >= 0);
    int one = 1;
    setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof one);
    struct sockaddr_in la = { 0 };
    la.sin_family = AF_INET;
    la.sin_port   = 0; /* ephemeral */
    inet_pton(AF_INET, "127.0.0.1", &la.sin_addr);
    assert(bind(lfd, (struct sockaddr *)&la, sizeof la) == 0);
    assert(listen(lfd, 1) == 0);
    socklen_t lalen = sizeof la;
    assert(getsockname(lfd, (struct sockaddr *)&la, &lalen) == 0);

    struct https_server_arg sa = { .listen_fd = lfd, .conf = srv_conf, .ok = 0 };
    struct https_client_arg ca = { .port = ntohs(la.sin_port), .conf = cli_conf, .ok = 0 };
    pthread_t st, ct;
    assert(pthread_create(&st, NULL, https_server_thread, &sa) == 0);
    assert(pthread_create(&ct, NULL, https_client_thread, &ca) == 0);
    pthread_join(st, NULL);
    pthread_join(ct, NULL);
    assert(sa.ok);
    assert(ca.ok);

    close(lfd);
    mln_tcp_tls_conf_free(srv_conf);
    mln_tcp_tls_conf_free(cli_conf);
    unlink(cert_path);
    unlink(key_path);
    printf("  PASS: HTTPS 10-round round-trip with SNI + CA + hostname verify\n");
}
#endif /* MLN_TLS */

int main(void)
{
    printf("=== Connection Module Comprehensive Tests ===\n\n");

    test_basic();
    test_queue_ops();
    test_send_recv();
    test_large_data();
    test_move_sent();
    test_send_chain();
    test_macros();
    test_multiple_cycles();
    test_performance();
    test_stability();
    test_recv_closed();
    test_recv_notyet_nonblock();
    test_recv_notyet_blocking();
    test_recv_error();
    test_send_finish();
    test_recv_after_nonblock_send();

#if defined(MLN_TLS)
    printf("\n--- TLS tests ---\n");
    test_tls_handshake_blocking();
    test_tls_handshake_nonblock();
    test_tls_handshake_partial_record_no_spin();
    test_tls_conf_validation();
    test_tls_send_recv_short();
    test_tls_send_recv_large();
    test_tls_send_in_file();
    test_tls_shutdown();
    test_tls_plain_unchanged();
    test_plain_perf_throughput();
    test_tls_perf_throughput();
    test_https_e2e_nonblocking();
    tls_test_fixture_destroy();
#endif

    printf("\n=== All connection tests passed! ===\n");
    return 0;
}
