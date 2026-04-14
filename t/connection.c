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

    printf("\n=== All connection tests passed! ===\n");
    return 0;
}
