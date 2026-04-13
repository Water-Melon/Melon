#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include "mln_websocket.h"
#include "mln_http.h"

static long elapsed_us(struct timespec *start, struct timespec *end)
{
    long sec_diff = (long)(end->tv_sec - start->tv_sec);
    long nsec_diff = (long)(end->tv_nsec - start->tv_nsec);
    return sec_diff * 1000000 + nsec_diff / 1000;
}

static mln_chain_t *create_chain_from_string(mln_alloc_t *pool, const char *str)
{
    mln_chain_t *c;
    mln_buf_t *b;

    assert((c = mln_chain_new(pool)) != NULL);
    assert((b = mln_buf_new(pool)) != NULL);
    c->buf = b;
    b->start = b->pos = b->left_pos = (mln_u8ptr_t)(void *)str;
    b->last = b->end = (mln_u8ptr_t)(void *)str + strlen(str);
    b->temporary = 1;
    return c;
}

static void test_is_websocket_detection(void)
{
    mln_http_t *http;
    mln_tcp_conn_t conn;
    mln_alloc_t *pool;
    mln_chain_t *c;
    mln_buf_t *b;
    char req[] = "GET / HTTP/1.1\r\nHost: test.com\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\nSec-WebSocket-Version: 13\r\n\r\n";

    assert(mln_tcp_conn_init(&conn, -1) == 0);
    assert((http = mln_http_init(&conn, NULL, NULL)) != NULL);

    pool = mln_tcp_conn_pool_get(&conn);

    assert((c = mln_chain_new(pool)) != NULL);
    assert((b = mln_buf_new(pool)) != NULL);
    c->buf = b;
    b->start = b->pos = b->left_pos = (mln_u8ptr_t)req;
    b->last = b->end = (mln_u8ptr_t)req + sizeof(req) - 1;
    b->temporary = 1;

    assert(mln_http_parse(http, &c) == M_HTTP_RET_DONE);

    assert(mln_websocket_is_websocket(http) >= 0);

    if (c != NULL) mln_chain_pool_release(c);

    mln_http_destroy(http);
    mln_tcp_conn_destroy(&conn);

    printf("[PASS] test_is_websocket_detection\n");
}

static void test_websocket_init(void)
{
    mln_http_t *http;
    mln_websocket_t *ws;
    mln_tcp_conn_t conn;
    mln_alloc_t *pool;
    mln_chain_t *c;
    mln_buf_t *b;
    char req[] = "GET / HTTP/1.1\r\nHost: test.com\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\nSec-WebSocket-Version: 13\r\n\r\n";

    assert(mln_tcp_conn_init(&conn, -1) == 0);
    assert((http = mln_http_init(&conn, NULL, NULL)) != NULL);

    pool = mln_tcp_conn_pool_get(&conn);

    assert((c = mln_chain_new(pool)) != NULL);
    assert((b = mln_buf_new(pool)) != NULL);
    c->buf = b;
    b->start = b->pos = b->left_pos = (mln_u8ptr_t)req;
    b->last = b->end = (mln_u8ptr_t)req + sizeof(req) - 1;
    b->temporary = 1;

    assert(mln_http_parse(http, &c) == M_HTTP_RET_DONE);

    assert((ws = mln_websocket_new(http)) != NULL);

    assert(mln_websocket_get_http(ws) == http);
    assert(mln_websocket_get_pool(ws) != NULL);
    assert(mln_websocket_get_connection(ws) != NULL);

    if (c != NULL) mln_chain_pool_release(c);

    mln_websocket_free(ws);
    mln_http_destroy(http);
    mln_tcp_conn_destroy(&conn);

    printf("[PASS] test_websocket_init\n");
}

static void test_websocket_text_frame_generate_simple(void)
{
    mln_http_t *http;
    mln_websocket_t *ws;
    mln_tcp_conn_t conn;
    mln_alloc_t *pool;
    mln_chain_t *out = NULL;
    mln_u8_t data[] = "hello";

    assert(mln_tcp_conn_init(&conn, -1) == 0);
    assert((http = mln_http_init(&conn, NULL, NULL)) != NULL);
    pool = mln_tcp_conn_pool_get(&conn);

    assert((ws = mln_websocket_new(http)) != NULL);
    mln_websocket_set_opcode(ws, M_WS_OPCODE_TEXT);
    mln_websocket_set_fin(ws);

    assert(mln_websocket_text_generate(ws, &out, data, sizeof(data) - 1, M_WS_FLAG_NONE) == M_WS_RET_OK);
    assert(out != NULL);

    mln_chain_pool_release_all(out);
    mln_websocket_free(ws);
    mln_http_destroy(http);
    mln_tcp_conn_destroy(&conn);

    printf("[PASS] test_websocket_text_frame_generate_simple\n");
}

static void test_websocket_text_frame_client_mode(void)
{
    mln_http_t *http;
    mln_websocket_t *ws;
    mln_tcp_conn_t conn;
    mln_alloc_t *pool;
    mln_chain_t *out = NULL;
    mln_u8_t data[] = "test";

    assert(mln_tcp_conn_init(&conn, -1) == 0);
    assert((http = mln_http_init(&conn, NULL, NULL)) != NULL);
    pool = mln_tcp_conn_pool_get(&conn);

    assert((ws = mln_websocket_new(http)) != NULL);
    mln_websocket_set_opcode(ws, M_WS_OPCODE_TEXT);
    mln_websocket_set_fin(ws);
    mln_websocket_set_maskbit(ws);

    assert(mln_websocket_text_generate(ws, &out, data, sizeof(data) - 1, M_WS_FLAG_CLIENT) == M_WS_RET_OK);
    assert(out != NULL);

    mln_chain_pool_release_all(out);
    mln_websocket_free(ws);
    mln_http_destroy(http);
    mln_tcp_conn_destroy(&conn);

    printf("[PASS] test_websocket_text_frame_client_mode\n");
}

static void test_websocket_binary_frame_generate(void)
{
    mln_http_t *http;
    mln_websocket_t *ws;
    mln_tcp_conn_t conn;
    mln_alloc_t *pool;
    mln_chain_t *out = NULL;
    mln_u8_t data[] = {0x00, 0x01, 0x02, 0x03, 0x04};

    assert(mln_tcp_conn_init(&conn, -1) == 0);
    assert((http = mln_http_init(&conn, NULL, NULL)) != NULL);
    pool = mln_tcp_conn_pool_get(&conn);

    assert((ws = mln_websocket_new(http)) != NULL);
    mln_websocket_set_opcode(ws, M_WS_OPCODE_BINARY);
    mln_websocket_set_fin(ws);

    assert(mln_websocket_binary_generate(ws, &out, data, sizeof(data), M_WS_FLAG_NONE) == M_WS_RET_OK);
    assert(out != NULL);

    mln_chain_pool_release_all(out);
    mln_websocket_free(ws);
    mln_http_destroy(http);
    mln_tcp_conn_destroy(&conn);

    printf("[PASS] test_websocket_binary_frame_generate\n");
}

static void test_websocket_close_frame_with_status(void)
{
    mln_http_t *http;
    mln_websocket_t *ws;
    mln_tcp_conn_t conn;
    mln_alloc_t *pool;
    mln_chain_t *out = NULL;

    assert(mln_tcp_conn_init(&conn, -1) == 0);
    assert((http = mln_http_init(&conn, NULL, NULL)) != NULL);
    pool = mln_tcp_conn_pool_get(&conn);

    assert((ws = mln_websocket_new(http)) != NULL);
    mln_websocket_set_opcode(ws, M_WS_OPCODE_CLOSE);
    mln_websocket_set_fin(ws);
    mln_websocket_set_status(ws, M_WS_STATUS_NORMAL_CLOSURE);

    assert(mln_websocket_close_generate(ws, &out, "Goodbye", 7, M_WS_FLAG_NONE) == M_WS_RET_OK);
    assert(out != NULL);

    mln_chain_pool_release_all(out);
    mln_websocket_free(ws);
    mln_http_destroy(http);
    mln_tcp_conn_destroy(&conn);

    printf("[PASS] test_websocket_close_frame_with_status\n");
}

static void test_websocket_ping_frame(void)
{
    mln_http_t *http;
    mln_websocket_t *ws;
    mln_tcp_conn_t conn;
    mln_alloc_t *pool;
    mln_chain_t *out = NULL;

    assert(mln_tcp_conn_init(&conn, -1) == 0);
    assert((http = mln_http_init(&conn, NULL, NULL)) != NULL);
    pool = mln_tcp_conn_pool_get(&conn);

    assert((ws = mln_websocket_new(http)) != NULL);
    mln_websocket_set_opcode(ws, M_WS_OPCODE_PING);
    mln_websocket_set_fin(ws);

    assert(mln_websocket_ping_generate(ws, &out, M_WS_FLAG_NONE) == M_WS_RET_OK);
    assert(out != NULL);

    mln_chain_pool_release_all(out);
    mln_websocket_free(ws);
    mln_http_destroy(http);
    mln_tcp_conn_destroy(&conn);

    printf("[PASS] test_websocket_ping_frame\n");
}

static void test_websocket_pong_frame(void)
{
    mln_http_t *http;
    mln_websocket_t *ws;
    mln_tcp_conn_t conn;
    mln_alloc_t *pool;
    mln_chain_t *out = NULL;

    assert(mln_tcp_conn_init(&conn, -1) == 0);
    assert((http = mln_http_init(&conn, NULL, NULL)) != NULL);
    pool = mln_tcp_conn_pool_get(&conn);

    assert((ws = mln_websocket_new(http)) != NULL);
    mln_websocket_set_opcode(ws, M_WS_OPCODE_PONG);
    mln_websocket_set_fin(ws);

    assert(mln_websocket_pong_generate(ws, &out, M_WS_FLAG_NONE) == M_WS_RET_OK);
    assert(out != NULL);

    mln_chain_pool_release_all(out);
    mln_websocket_free(ws);
    mln_http_destroy(http);
    mln_tcp_conn_destroy(&conn);

    printf("[PASS] test_websocket_pong_frame\n");
}

static void test_websocket_fragmentation_start(void)
{
    mln_http_t *http;
    mln_websocket_t *ws;
    mln_tcp_conn_t conn;
    mln_alloc_t *pool;
    mln_chain_t *out = NULL;
    mln_u8_t data[] = "fragment1";

    assert(mln_tcp_conn_init(&conn, -1) == 0);
    assert((http = mln_http_init(&conn, NULL, NULL)) != NULL);
    pool = mln_tcp_conn_pool_get(&conn);

    assert((ws = mln_websocket_new(http)) != NULL);
    mln_websocket_set_opcode(ws, M_WS_OPCODE_TEXT);
    mln_websocket_reset_fin(ws);

    assert(mln_websocket_text_generate(ws, &out, data, sizeof(data) - 1, M_WS_FLAG_NEW) == M_WS_RET_OK);
    assert(out != NULL);

    mln_chain_pool_release_all(out);
    mln_websocket_free(ws);
    mln_http_destroy(http);
    mln_tcp_conn_destroy(&conn);

    printf("[PASS] test_websocket_fragmentation_start\n");
}

static void test_websocket_fragmentation_continue(void)
{
    mln_http_t *http;
    mln_websocket_t *ws;
    mln_tcp_conn_t conn;
    mln_alloc_t *pool;
    mln_chain_t *out = NULL;
    mln_u8_t data[] = "fragment2";

    assert(mln_tcp_conn_init(&conn, -1) == 0);
    assert((http = mln_http_init(&conn, NULL, NULL)) != NULL);
    pool = mln_tcp_conn_pool_get(&conn);

    assert((ws = mln_websocket_new(http)) != NULL);
    mln_websocket_set_opcode(ws, M_WS_OPCODE_CONTINUE);
    mln_websocket_reset_fin(ws);

    assert(mln_websocket_text_generate(ws, &out, data, sizeof(data) - 1, M_WS_FLAG_NONE) == M_WS_RET_OK);
    assert(out != NULL);

    mln_chain_pool_release_all(out);
    mln_websocket_free(ws);
    mln_http_destroy(http);
    mln_tcp_conn_destroy(&conn);

    printf("[PASS] test_websocket_fragmentation_continue\n");
}

static void test_websocket_fragmentation_end(void)
{
    mln_http_t *http;
    mln_websocket_t *ws;
    mln_tcp_conn_t conn;
    mln_alloc_t *pool;
    mln_chain_t *out = NULL;
    mln_u8_t data[] = "fragment3";

    assert(mln_tcp_conn_init(&conn, -1) == 0);
    assert((http = mln_http_init(&conn, NULL, NULL)) != NULL);
    pool = mln_tcp_conn_pool_get(&conn);

    assert((ws = mln_websocket_new(http)) != NULL);
    mln_websocket_set_opcode(ws, M_WS_OPCODE_CONTINUE);
    mln_websocket_set_fin(ws);

    assert(mln_websocket_text_generate(ws, &out, data, sizeof(data) - 1, M_WS_FLAG_END) == M_WS_RET_OK);
    assert(out != NULL);

    mln_chain_pool_release_all(out);
    mln_websocket_free(ws);
    mln_http_destroy(http);
    mln_tcp_conn_destroy(&conn);

    printf("[PASS] test_websocket_fragmentation_end\n");
}

static void test_websocket_field_operations(void)
{
    mln_http_t *http;
    mln_websocket_t *ws;
    mln_tcp_conn_t conn;
    mln_string_t key, val, val2;
    mln_string_t *result;

    assert(mln_tcp_conn_init(&conn, -1) == 0);
    assert((http = mln_http_init(&conn, NULL, NULL)) != NULL);

    assert((ws = mln_websocket_new(http)) != NULL);

    mln_string_set(&key, "X-Custom-Header");
    mln_string_set(&val, "CustomValue");
    mln_string_set(&val2, "NewValue");

    assert(mln_websocket_set_field(ws, &key, &val) == 0);
    result = mln_websocket_get_field(ws, &key);
    assert(result != NULL);

    assert(mln_websocket_set_field(ws, &key, &val2) == 0);
    result = mln_websocket_get_field(ws, &key);
    assert(result != NULL);

    mln_websocket_free(ws);
    mln_http_destroy(http);
    mln_tcp_conn_destroy(&conn);

    printf("[PASS] test_websocket_field_operations\n");
}

static void test_websocket_reset(void)
{
    mln_http_t *http;
    mln_websocket_t *ws;
    mln_tcp_conn_t conn;
    mln_alloc_t *pool;
    mln_chain_t *out = NULL;
    mln_u8_t data[] = "test";

    assert(mln_tcp_conn_init(&conn, -1) == 0);
    assert((http = mln_http_init(&conn, NULL, NULL)) != NULL);
    pool = mln_tcp_conn_pool_get(&conn);

    assert((ws = mln_websocket_new(http)) != NULL);

    mln_websocket_set_opcode(ws, M_WS_OPCODE_TEXT);
    mln_websocket_set_fin(ws);
    assert(mln_websocket_text_generate(ws, &out, data, sizeof(data) - 1, M_WS_FLAG_NONE) == M_WS_RET_OK);

    mln_chain_pool_release_all(out);
    mln_websocket_reset(ws);

    assert(mln_websocket_get_opcode(ws) == 0);
    assert(mln_websocket_get_fin(ws) == 0);

    mln_websocket_free(ws);
    mln_http_destroy(http);
    mln_tcp_conn_destroy(&conn);

    printf("[PASS] test_websocket_reset\n");
}

static void test_websocket_masking_operations(void)
{
    mln_http_t *http;
    mln_websocket_t *ws;
    mln_tcp_conn_t conn;
    mln_u32_t mask_key = 0x12345678;

    assert(mln_tcp_conn_init(&conn, -1) == 0);
    assert((http = mln_http_init(&conn, NULL, NULL)) != NULL);

    assert((ws = mln_websocket_new(http)) != NULL);

    mln_websocket_set_masking_key(ws, mask_key);
    assert(mln_websocket_get_masking_key(ws) == mask_key);

    mln_websocket_set_maskbit(ws);
    assert(mln_websocket_get_maskbit(ws) == 1);

    mln_websocket_reset_maskbit(ws);
    assert(mln_websocket_get_maskbit(ws) == 0);

    mln_websocket_free(ws);
    mln_http_destroy(http);
    mln_tcp_conn_destroy(&conn);

    printf("[PASS] test_websocket_masking_operations\n");
}

static void test_websocket_rsv_bits(void)
{
    mln_http_t *http;
    mln_websocket_t *ws;
    mln_tcp_conn_t conn;

    assert(mln_tcp_conn_init(&conn, -1) == 0);
    assert((http = mln_http_init(&conn, NULL, NULL)) != NULL);

    assert((ws = mln_websocket_new(http)) != NULL);

    mln_websocket_set_rsv1(ws);
    assert(mln_websocket_get_rsv1(ws) == 1);
    mln_websocket_reset_rsv1(ws);
    assert(mln_websocket_get_rsv1(ws) == 0);

    mln_websocket_set_rsv2(ws);
    assert(mln_websocket_get_rsv2(ws) == 1);
    mln_websocket_reset_rsv2(ws);
    assert(mln_websocket_get_rsv2(ws) == 0);

    mln_websocket_set_rsv3(ws);
    assert(mln_websocket_get_rsv3(ws) == 1);
    mln_websocket_reset_rsv3(ws);
    assert(mln_websocket_get_rsv3(ws) == 0);

    mln_websocket_free(ws);
    mln_http_destroy(http);
    mln_tcp_conn_destroy(&conn);

    printf("[PASS] test_websocket_rsv_bits\n");
}

static void test_websocket_content_operations(void)
{
    mln_http_t *http;
    mln_websocket_t *ws;
    mln_tcp_conn_t conn;
    void *test_data = (void *)0xdeadbeef;
    mln_u64_t content_len = 1024;

    assert(mln_tcp_conn_init(&conn, -1) == 0);
    assert((http = mln_http_init(&conn, NULL, NULL)) != NULL);

    assert((ws = mln_websocket_new(http)) != NULL);

    mln_websocket_set_content(ws, test_data);
    assert(mln_websocket_get_content(ws) == test_data);

    mln_websocket_set_content_len(ws, content_len);
    assert(mln_websocket_get_content_len(ws) == content_len);

    mln_websocket_set_content_free(ws);
    assert(mln_websocket_get_content_free(ws) == 1);

    mln_websocket_reset_content_free(ws);
    assert(mln_websocket_get_content_free(ws) == 0);

    mln_websocket_free(ws);
    mln_http_destroy(http);
    mln_tcp_conn_destroy(&conn);

    printf("[PASS] test_websocket_content_operations\n");
}

static void test_websocket_status_codes(void)
{
    mln_http_t *http;
    mln_websocket_t *ws;
    mln_tcp_conn_t conn;

    assert(mln_tcp_conn_init(&conn, -1) == 0);
    assert((http = mln_http_init(&conn, NULL, NULL)) != NULL);

    assert((ws = mln_websocket_new(http)) != NULL);

    mln_websocket_set_status(ws, M_WS_STATUS_NORMAL_CLOSURE);
    assert(mln_websocket_get_status(ws) == M_WS_STATUS_NORMAL_CLOSURE);

    mln_websocket_set_status(ws, M_WS_STATUS_GOING_AWAY);
    assert(mln_websocket_get_status(ws) == M_WS_STATUS_GOING_AWAY);

    mln_websocket_set_status(ws, M_WS_STATUS_PROTOCOL_ERROR);
    assert(mln_websocket_get_status(ws) == M_WS_STATUS_PROTOCOL_ERROR);

    mln_websocket_free(ws);
    mln_http_destroy(http);
    mln_tcp_conn_destroy(&conn);

    printf("[PASS] test_websocket_status_codes\n");
}

static void test_websocket_uri_args_operations(void)
{
    mln_http_t *http;
    mln_websocket_t *ws;
    mln_tcp_conn_t conn;
    mln_string_t test_uri, test_args;

    assert(mln_tcp_conn_init(&conn, -1) == 0);
    assert((http = mln_http_init(&conn, NULL, NULL)) != NULL);

    assert((ws = mln_websocket_new(http)) != NULL);

    mln_string_set(&test_uri, "/ws");
    mln_string_set(&test_args, "token=abc123");

    /* Must use pool-duplicated strings since websocket_destroy frees them */
    mln_alloc_t *pool = mln_tcp_conn_pool_get(&conn);
    mln_string_t *dup_uri = mln_string_pool_dup(pool, &test_uri);
    mln_string_t *dup_args = mln_string_pool_dup(pool, &test_args);
    assert(dup_uri != NULL && dup_args != NULL);

    mln_websocket_set_uri(ws, dup_uri);
    assert(mln_websocket_get_uri(ws) != NULL);

    mln_websocket_set_args(ws, dup_args);
    assert(mln_websocket_get_args(ws) != NULL);

    mln_websocket_free(ws);
    mln_http_destroy(http);
    mln_tcp_conn_destroy(&conn);

    printf("[PASS] test_websocket_uri_args_operations\n");
}

static void test_websocket_data_operations(void)
{
    mln_http_t *http;
    mln_websocket_t *ws;
    mln_tcp_conn_t conn;
    void *user_data = (void *)0xcafebabe;

    assert(mln_tcp_conn_init(&conn, -1) == 0);
    assert((http = mln_http_init(&conn, NULL, NULL)) != NULL);

    assert((ws = mln_websocket_new(http)) != NULL);

    mln_websocket_set_data(ws, user_data);
    assert(mln_websocket_get_data(ws) == user_data);

    mln_websocket_free(ws);
    mln_http_destroy(http);
    mln_tcp_conn_destroy(&conn);

    printf("[PASS] test_websocket_data_operations\n");
}

static void test_websocket_performance_generate_parse_roundtrip(void)
{
    mln_http_t *http;
    mln_websocket_t *ws;
    mln_tcp_conn_t conn;
    mln_alloc_t *pool;
    mln_chain_t *out = NULL;
    mln_u8_t data[128];
    struct timespec start, end;
    long elapsed;
    int i;
    int iterations = 100000;

    assert(mln_tcp_conn_init(&conn, -1) == 0);
    assert((http = mln_http_init(&conn, NULL, NULL)) != NULL);
    pool = mln_tcp_conn_pool_get(&conn);

    assert((ws = mln_websocket_new(http)) != NULL);

    memset(data, 'A', sizeof(data));

    clock_gettime(CLOCK_MONOTONIC, &start);

    for (i = 0; i < iterations; i++) {
        mln_websocket_set_opcode(ws, M_WS_OPCODE_TEXT);
        mln_websocket_set_fin(ws);

        assert(mln_websocket_text_generate(ws, &out, data, sizeof(data), M_WS_FLAG_NONE) == M_WS_RET_OK);

        mln_chain_pool_release_all(out);
        out = NULL;
        mln_websocket_reset(ws);
    }

    clock_gettime(CLOCK_MONOTONIC, &end);
    elapsed = elapsed_us(&start, &end);

    printf("[PERF] websocket generate: %d iterations in %ld us (%.2f ops/sec)\n",
           iterations, elapsed, (iterations * 1000000.0) / elapsed);

    mln_websocket_free(ws);
    mln_http_destroy(http);
    mln_tcp_conn_destroy(&conn);
}

static void test_websocket_stability_large_frames(void)
{
    mln_http_t *http;
    mln_websocket_t *ws;
    mln_tcp_conn_t conn;
    mln_alloc_t *pool;
    mln_chain_t *out = NULL;
    mln_u8_t *data;
    int i;
    int count = 10000;
    int data_size = 256;

    assert(mln_tcp_conn_init(&conn, -1) == 0);
    assert((http = mln_http_init(&conn, NULL, NULL)) != NULL);
    pool = mln_tcp_conn_pool_get(&conn);

    assert((ws = mln_websocket_new(http)) != NULL);

    assert((data = (mln_u8_t *)malloc(data_size)) != NULL);
    memset(data, 0x42, data_size);

    for (i = 0; i < count; i++) {
        mln_websocket_set_opcode(ws, (i % 2 == 0) ? M_WS_OPCODE_TEXT : M_WS_OPCODE_BINARY);
        mln_websocket_set_fin(ws);

        if (i % 2 == 0) {
            assert(mln_websocket_text_generate(ws, &out, data, data_size, M_WS_FLAG_NONE) == M_WS_RET_OK);
        } else {
            assert(mln_websocket_binary_generate(ws, &out, data, data_size, M_WS_FLAG_NONE) == M_WS_RET_OK);
        }

        assert(out != NULL);
        mln_chain_pool_release_all(out);
        out = NULL;
        mln_websocket_reset(ws);
    }

    free(data);
    mln_websocket_free(ws);
    mln_http_destroy(http);
    mln_tcp_conn_destroy(&conn);

    printf("[PASS] test_websocket_stability_large_frames (%d frames)\n", count);
}

static void test_websocket_stability_mixed_frames(void)
{
    mln_http_t *http;
    mln_websocket_t *ws;
    mln_tcp_conn_t conn;
    mln_alloc_t *pool;
    mln_chain_t *out = NULL;
    mln_u8_t data[] = "test payload";
    int i;
    int count = 10000;

    assert(mln_tcp_conn_init(&conn, -1) == 0);
    assert((http = mln_http_init(&conn, NULL, NULL)) != NULL);
    pool = mln_tcp_conn_pool_get(&conn);

    assert((ws = mln_websocket_new(http)) != NULL);

    for (i = 0; i < count; i++) {
        int frame_type = i % 6;

        mln_websocket_reset(ws);
        mln_websocket_set_fin(ws);

        switch (frame_type) {
            case 0:
                mln_websocket_set_opcode(ws, M_WS_OPCODE_TEXT);
                assert(mln_websocket_text_generate(ws, &out, data, sizeof(data) - 1, M_WS_FLAG_NONE) == M_WS_RET_OK);
                break;
            case 1:
                mln_websocket_set_opcode(ws, M_WS_OPCODE_BINARY);
                assert(mln_websocket_binary_generate(ws, &out, data, sizeof(data) - 1, M_WS_FLAG_NONE) == M_WS_RET_OK);
                break;
            case 2:
                mln_websocket_set_opcode(ws, M_WS_OPCODE_PING);
                assert(mln_websocket_ping_generate(ws, &out, M_WS_FLAG_NONE) == M_WS_RET_OK);
                break;
            case 3:
                mln_websocket_set_opcode(ws, M_WS_OPCODE_PONG);
                assert(mln_websocket_pong_generate(ws, &out, M_WS_FLAG_NONE) == M_WS_RET_OK);
                break;
            case 4:
                mln_websocket_set_opcode(ws, M_WS_OPCODE_CLOSE);
                mln_websocket_set_status(ws, M_WS_STATUS_NORMAL_CLOSURE);
                assert(mln_websocket_close_generate(ws, &out, "", 0, M_WS_FLAG_NONE) == M_WS_RET_OK);
                break;
            case 5:
                mln_websocket_set_opcode(ws, M_WS_OPCODE_TEXT);
                mln_websocket_reset_fin(ws);
                assert(mln_websocket_text_generate(ws, &out, data, sizeof(data) - 1, M_WS_FLAG_NEW) == M_WS_RET_OK);
                break;
        }

        assert(out != NULL);
        mln_chain_pool_release_all(out);
        out = NULL;
    }

    mln_websocket_free(ws);
    mln_http_destroy(http);
    mln_tcp_conn_destroy(&conn);

    printf("[PASS] test_websocket_stability_mixed_frames (%d frames)\n", count);
}

int main(void)
{
    printf("===== WebSocket Module Tests =====\n\n");

    printf("=== WebSocket Detection and Init ===\n");
    test_is_websocket_detection();
    test_websocket_init();

    printf("\n=== Frame Generation ===\n");
    test_websocket_text_frame_generate_simple();
    test_websocket_text_frame_client_mode();
    test_websocket_binary_frame_generate();
    test_websocket_close_frame_with_status();
    test_websocket_ping_frame();
    test_websocket_pong_frame();

    printf("\n=== Fragmentation ===\n");
    test_websocket_fragmentation_start();
    test_websocket_fragmentation_continue();
    test_websocket_fragmentation_end();

    printf("\n=== Field Operations ===\n");
    test_websocket_field_operations();
    test_websocket_reset();

    printf("\n=== Masking Operations ===\n");
    test_websocket_masking_operations();

    printf("\n=== RSV Bits ===\n");
    test_websocket_rsv_bits();

    printf("\n=== Content Operations ===\n");
    test_websocket_content_operations();
    test_websocket_status_codes();

    printf("\n=== URI and Args Operations ===\n");
    test_websocket_uri_args_operations();

    printf("\n=== Data Operations ===\n");
    test_websocket_data_operations();

    printf("\n=== Performance ===\n");
    test_websocket_performance_generate_parse_roundtrip();

    printf("\n=== Stability ===\n");
    test_websocket_stability_large_frames();
    test_websocket_stability_mixed_frames();

    printf("\n===== All WebSocket tests passed! =====\n");
    return 0;
}
