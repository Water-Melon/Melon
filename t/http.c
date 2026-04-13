#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <unistd.h>
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

static void test_request_parse_get(void)
{
    mln_http_t *http;
    mln_tcp_conn_t conn;
    mln_alloc_t *pool;
    mln_chain_t *c;
    mln_buf_t *b;
    char req[] = "GET / HTTP/1.1\r\nHost: 127.0.0.1:8080\r\nUser-Agent: curl/7.81.0\r\n\r\n";

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
    assert(mln_http_type_get(http) == M_HTTP_REQUEST);
    assert(mln_http_method_get(http) == M_HTTP_GET);
    assert(mln_http_version_get(http) == M_HTTP_VERSION_1_1);
    assert(mln_http_uri_get(http) != NULL);

    if (c != NULL) mln_chain_pool_release(c);

    mln_http_destroy(http);
    mln_tcp_conn_destroy(&conn);

    printf("[PASS] test_request_parse_get\n");
}

static void test_request_parse_post(void)
{
    mln_http_t *http;
    mln_tcp_conn_t conn;
    mln_alloc_t *pool;
    mln_chain_t *c;
    mln_buf_t *b;
    char req[] = "POST /api/data HTTP/1.1\r\nHost: example.com\r\nContent-Length: 5\r\n\r\nhello";

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
    assert(mln_http_method_get(http) == M_HTTP_POST);

    if (c != NULL) mln_chain_pool_release(c);

    mln_http_destroy(http);
    mln_tcp_conn_destroy(&conn);

    printf("[PASS] test_request_parse_post\n");
}

static void test_all_http_methods(void)
{
    mln_http_t *http;
    mln_tcp_conn_t conn;
    mln_alloc_t *pool;
    mln_chain_t *c;
    mln_buf_t *b;
    char req[256];
    const char *methods[] = {"GET", "POST", "HEAD", "PUT", "DELETE", "TRACE", "CONNECT", "OPTIONS"};
    mln_u32_t method_vals[] = {M_HTTP_GET, M_HTTP_POST, M_HTTP_HEAD, M_HTTP_PUT, M_HTTP_DELETE, M_HTTP_TRACE, M_HTTP_CONNECT, M_HTTP_OPTIONS};
    int i;

    for (i = 0; i < 8; i++) {
        assert(mln_tcp_conn_init(&conn, -1) == 0);
        assert((http = mln_http_init(&conn, NULL, NULL)) != NULL);
        pool = mln_tcp_conn_pool_get(&conn);

        snprintf(req, sizeof(req), "%s / HTTP/1.1\r\nHost: test.com\r\n\r\n", methods[i]);

        assert((c = mln_chain_new(pool)) != NULL);
        assert((b = mln_buf_new(pool)) != NULL);
        c->buf = b;
        b->start = b->pos = b->left_pos = (mln_u8ptr_t)req;
        b->last = b->end = (mln_u8ptr_t)req + strlen(req);
        b->temporary = 1;

        assert(mln_http_parse(http, &c) == M_HTTP_RET_DONE);
        assert(mln_http_method_get(http) == method_vals[i]);

        if (c != NULL) mln_chain_pool_release(c);
        mln_http_destroy(http);
        mln_tcp_conn_destroy(&conn);
    }

    printf("[PASS] test_all_http_methods\n");
}

static void test_http_versions(void)
{
    mln_http_t *http;
    mln_tcp_conn_t conn;
    mln_alloc_t *pool;
    mln_chain_t *c;
    mln_buf_t *b;
    char req[256];

    assert(mln_tcp_conn_init(&conn, -1) == 0);
    assert((http = mln_http_init(&conn, NULL, NULL)) != NULL);
    pool = mln_tcp_conn_pool_get(&conn);

    snprintf(req, sizeof(req), "GET / HTTP/1.0\r\nHost: test.com\r\n\r\n");

    assert((c = mln_chain_new(pool)) != NULL);
    assert((b = mln_buf_new(pool)) != NULL);
    c->buf = b;
    b->start = b->pos = b->left_pos = (mln_u8ptr_t)req;
    b->last = b->end = (mln_u8ptr_t)req + strlen(req);
    b->temporary = 1;

    assert(mln_http_parse(http, &c) == M_HTTP_RET_DONE);
    assert(mln_http_version_get(http) == M_HTTP_VERSION_1_0);

    if (c != NULL) mln_chain_pool_release(c);
    mln_http_destroy(http);
    mln_tcp_conn_destroy(&conn);

    assert(mln_tcp_conn_init(&conn, -1) == 0);
    assert((http = mln_http_init(&conn, NULL, NULL)) != NULL);
    pool = mln_tcp_conn_pool_get(&conn);

    snprintf(req, sizeof(req), "GET / HTTP/1.1\r\nHost: test.com\r\n\r\n");

    assert((c = mln_chain_new(pool)) != NULL);
    assert((b = mln_buf_new(pool)) != NULL);
    c->buf = b;
    b->start = b->pos = b->left_pos = (mln_u8ptr_t)req;
    b->last = b->end = (mln_u8ptr_t)req + strlen(req);
    b->temporary = 1;

    assert(mln_http_parse(http, &c) == M_HTTP_RET_DONE);
    assert(mln_http_version_get(http) == M_HTTP_VERSION_1_1);

    if (c != NULL) mln_chain_pool_release(c);
    mln_http_destroy(http);
    mln_tcp_conn_destroy(&conn);

    printf("[PASS] test_http_versions\n");
}

static void test_request_with_query_string(void)
{
    mln_http_t *http;
    mln_tcp_conn_t conn;
    mln_alloc_t *pool;
    mln_chain_t *c;
    mln_buf_t *b;
    char req[] = "GET /search?q=test&page=1 HTTP/1.1\r\nHost: test.com\r\n\r\n";

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
    assert(mln_http_args_get(http) != NULL);

    if (c != NULL) mln_chain_pool_release(c);

    mln_http_destroy(http);
    mln_tcp_conn_destroy(&conn);

    printf("[PASS] test_request_with_query_string\n");
}

static void test_multiple_headers(void)
{
    mln_http_t *http;
    mln_tcp_conn_t conn;
    mln_alloc_t *pool;
    mln_chain_t *c;
    mln_buf_t *b;
    char req[] = "GET / HTTP/1.1\r\nHost: test.com\r\nUser-Agent: test\r\nContent-Type: text/html\r\nAccept: */*\r\n\r\n";

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

    if (c != NULL) mln_chain_pool_release(c);

    mln_http_destroy(http);
    mln_tcp_conn_destroy(&conn);

    printf("[PASS] test_multiple_headers\n");
}

static void test_response_parse(void)
{
    mln_http_t *http;
    mln_tcp_conn_t conn;
    mln_alloc_t *pool;
    mln_chain_t *c;
    mln_buf_t *b;
    char resp[] = "HTTP/1.1 200 OK\r\nContent-Length: 5\r\n\r\nhello";

    assert(mln_tcp_conn_init(&conn, -1) == 0);
    assert((http = mln_http_init(&conn, NULL, NULL)) != NULL);
    /* Do not pre-set type; parser auto-detects from HTTP/x.x prefix */

    pool = mln_tcp_conn_pool_get(&conn);

    assert((c = mln_chain_new(pool)) != NULL);
    assert((b = mln_buf_new(pool)) != NULL);
    c->buf = b;
    b->start = b->pos = b->left_pos = (mln_u8ptr_t)resp;
    b->last = b->end = (mln_u8ptr_t)resp + sizeof(resp) - 1;
    b->temporary = 1;

    assert(mln_http_parse(http, &c) == M_HTTP_RET_DONE);
    assert(mln_http_status_get(http) == M_HTTP_OK);

    if (c != NULL) mln_chain_pool_release(c);

    mln_http_destroy(http);
    mln_tcp_conn_destroy(&conn);

    printf("[PASS] test_response_parse\n");
}

static void test_response_status_codes(void)
{
    mln_http_t *http;
    mln_tcp_conn_t conn;
    mln_alloc_t *pool;
    mln_chain_t *c;
    mln_buf_t *b;
    char resp[256];
    struct {
        mln_u32_t code;
        const char *msg;
    } tests[] = {
        {M_HTTP_OK, "200 OK"},
        {M_HTTP_CREATED, "201 Created"},
        {M_HTTP_BAD_REQUEST, "400 Bad Request"},
        {M_HTTP_UNAUTHORIZED, "401 Unauthorized"},
        {M_HTTP_NOT_FOUND, "404 Not Found"},
        {M_HTTP_INTERNAL_SERVER_ERROR, "500 Internal Server Error"},
        {M_HTTP_SERVICE_UNAVAILABLE, "503 Service Unavailable"},
    };
    int i;

    for (i = 0; i < 7; i++) {
        assert(mln_tcp_conn_init(&conn, -1) == 0);
        assert((http = mln_http_init(&conn, NULL, NULL)) != NULL);
        /* Do not pre-set type; parser auto-detects */
        pool = mln_tcp_conn_pool_get(&conn);

        snprintf(resp, sizeof(resp), "HTTP/1.1 %s\r\n\r\n", tests[i].msg);

        assert((c = mln_chain_new(pool)) != NULL);
        assert((b = mln_buf_new(pool)) != NULL);
        c->buf = b;
        b->start = b->pos = b->left_pos = (mln_u8ptr_t)resp;
        b->last = b->end = (mln_u8ptr_t)resp + strlen(resp);
        b->temporary = 1;

        assert(mln_http_parse(http, &c) == M_HTTP_RET_DONE);
        assert(mln_http_status_get(http) == tests[i].code);

        if (c != NULL) mln_chain_pool_release(c);
        mln_http_destroy(http);
        mln_tcp_conn_destroy(&conn);
    }

    printf("[PASS] test_response_status_codes\n");
}

static void test_field_operations(void)
{
    mln_http_t *http;
    mln_tcp_conn_t conn;
    mln_string_t key, val, val2;
    mln_string_t *result;

    assert(mln_tcp_conn_init(&conn, -1) == 0);
    assert((http = mln_http_init(&conn, NULL, NULL)) != NULL);

    mln_string_set(&key, "Server");
    mln_string_set(&val, "Melon");
    mln_string_set(&val2, "Melon/1.0");

    assert(mln_http_field_set(http, &key, &val) == 0);
    result = mln_http_field_get(http, &key);
    assert(result != NULL);
    assert(result->len == val.len);

    assert(mln_http_field_set(http, &key, &val2) == 0);
    result = mln_http_field_get(http, &key);
    assert(result != NULL);
    assert(result->len == val2.len);

    mln_http_field_remove(http, &key);
    result = mln_http_field_get(http, &key);
    assert(result == NULL);

    mln_http_destroy(http);
    mln_tcp_conn_destroy(&conn);

    printf("[PASS] test_field_operations\n");
}

static void test_case_insensitive_headers(void)
{
    mln_http_t *http;
    mln_tcp_conn_t conn;
    mln_string_t key_lower, key_upper;
    mln_string_t val;
    mln_string_t *result;

    assert(mln_tcp_conn_init(&conn, -1) == 0);
    assert((http = mln_http_init(&conn, NULL, NULL)) != NULL);

    mln_string_set(&key_lower, "content-type");
    mln_string_set(&key_upper, "Content-Type");
    mln_string_set(&val, "text/html");

    assert(mln_http_field_set(http, &key_lower, &val) == 0);
    result = mln_http_field_get(http, &key_upper);
    assert(result != NULL);

    mln_http_destroy(http);
    mln_tcp_conn_destroy(&conn);

    printf("[PASS] test_case_insensitive_headers\n");
}

static void test_generate_request(void)
{
    mln_http_t *http;
    mln_tcp_conn_t conn;
    mln_alloc_t *pool;
    mln_chain_t *c, *head = NULL, *tail = NULL;
    mln_string_t key, val;

    assert(mln_tcp_conn_init(&conn, -1) == 0);
    assert((http = mln_http_init(&conn, NULL, NULL)) != NULL);

    pool = mln_tcp_conn_pool_get(&conn);

    mln_http_type_set(http, M_HTTP_REQUEST);
    mln_http_method_set(http, M_HTTP_GET);
    mln_http_version_set(http, M_HTTP_VERSION_1_1);

    mln_string_set(&key, "Host");
    mln_string_set(&val, "example.com");
    assert(mln_http_field_set(http, &key, &val) == 0);

    assert(mln_http_generate(http, &head, &tail) == M_HTTP_RET_DONE);
    assert(head != NULL);

    for (c = head; c != NULL; c = c->next) {
        if (mln_buf_size(c->buf)) {
            assert(c->buf->start != NULL);
        }
    }

    mln_chain_pool_release_all(head);
    mln_http_destroy(http);
    mln_tcp_conn_destroy(&conn);

    printf("[PASS] test_generate_request\n");
}

static void test_generate_response(void)
{
    mln_http_t *http;
    mln_tcp_conn_t conn;
    mln_alloc_t *pool;
    mln_chain_t *c, *head = NULL, *tail = NULL;
    mln_string_t key, val;

    assert(mln_tcp_conn_init(&conn, -1) == 0);
    assert((http = mln_http_init(&conn, NULL, NULL)) != NULL);

    pool = mln_tcp_conn_pool_get(&conn);

    mln_http_type_set(http, M_HTTP_RESPONSE);
    mln_http_status_set(http, M_HTTP_OK);
    mln_http_version_set(http, M_HTTP_VERSION_1_1);

    mln_string_set(&key, "Content-Type");
    mln_string_set(&val, "text/html");
    assert(mln_http_field_set(http, &key, &val) == 0);

    assert(mln_http_generate(http, &head, &tail) == M_HTTP_RET_DONE);
    assert(head != NULL);

    for (c = head; c != NULL; c = c->next) {
        if (mln_buf_size(c->buf)) {
            assert(c->buf->start != NULL);
        }
    }

    mln_chain_pool_release_all(head);
    mln_http_destroy(http);
    mln_tcp_conn_destroy(&conn);

    printf("[PASS] test_generate_response\n");
}

static void test_reset(void)
{
    mln_http_t *http;
    mln_tcp_conn_t conn;
    mln_alloc_t *pool;
    mln_chain_t *c;
    mln_buf_t *b;
    mln_string_t key, val;
    char req[] = "GET / HTTP/1.1\r\nHost: test.com\r\n\r\n";

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

    mln_string_set(&key, "Server");
    mln_string_set(&val, "Test");
    assert(mln_http_field_set(http, &key, &val) == 0);

    mln_http_reset(http);

    assert(mln_http_method_get(http) == M_HTTP_UNKNOWN);
    assert(mln_http_field_get(http, &key) == NULL);

    if (c != NULL) mln_chain_pool_release(c);

    mln_http_destroy(http);
    mln_tcp_conn_destroy(&conn);

    printf("[PASS] test_reset\n");
}

static void test_performance_parse_generate(void)
{
    mln_http_t *http;
    mln_tcp_conn_t conn;
    mln_alloc_t *pool;
    mln_chain_t *c, *head, *tail;
    mln_buf_t *b;
    char req[] = "GET /test HTTP/1.1\r\nHost: test.com\r\nUser-Agent: test\r\n\r\n";
    mln_string_t key, val;
    struct timespec start, end;
    long elapsed;
    int i;
    int iterations = 100000;

    assert(mln_tcp_conn_init(&conn, -1) == 0);
    assert((http = mln_http_init(&conn, NULL, NULL)) != NULL);
    pool = mln_tcp_conn_pool_get(&conn);

    mln_string_set(&key, "Server");
    mln_string_set(&val, "Melon");

    clock_gettime(CLOCK_MONOTONIC, &start);

    for (i = 0; i < iterations; i++) {
        assert((c = mln_chain_new(pool)) != NULL);
        assert((b = mln_buf_new(pool)) != NULL);
        c->buf = b;
        b->start = b->pos = b->left_pos = (mln_u8ptr_t)req;
        b->last = b->end = (mln_u8ptr_t)req + sizeof(req) - 1;
        b->temporary = 1;

        assert(mln_http_parse(http, &c) == M_HTTP_RET_DONE);

        mln_http_type_set(http, M_HTTP_RESPONSE);
        mln_http_status_set(http, M_HTTP_OK);
        assert(mln_http_field_set(http, &key, &val) == 0);

        head = NULL;
        tail = NULL;
        assert(mln_http_generate(http, &head, &tail) == M_HTTP_RET_DONE);
        if (head != NULL) mln_chain_pool_release_all(head);

        mln_http_reset(http);

        if (c != NULL) mln_chain_pool_release(c);
    }

    clock_gettime(CLOCK_MONOTONIC, &end);
    elapsed = elapsed_us(&start, &end);

    printf("[PERF] parse+generate: %d iterations in %ld us (%.2f ops/sec)\n",
           iterations, elapsed, (iterations * 1000000.0) / elapsed);

    mln_http_destroy(http);
    mln_tcp_conn_destroy(&conn);
}

static void test_stability_parse_multiple(void)
{
    mln_http_t *http;
    mln_tcp_conn_t conn;
    mln_alloc_t *pool;
    mln_chain_t *c;
    mln_buf_t *b;
    char req[256];
    int i;
    int count = 10000;

    assert(mln_tcp_conn_init(&conn, -1) == 0);
    assert((http = mln_http_init(&conn, NULL, NULL)) != NULL);
    pool = mln_tcp_conn_pool_get(&conn);

    for (i = 0; i < count; i++) {
        snprintf(req, sizeof(req), "GET /path%d HTTP/1.1\r\nHost: test.com\r\n\r\n", i);

        assert((c = mln_chain_new(pool)) != NULL);
        assert((b = mln_buf_new(pool)) != NULL);
        c->buf = b;
        b->start = b->pos = b->left_pos = (mln_u8ptr_t)req;
        b->last = b->end = (mln_u8ptr_t)req + strlen(req);
        b->temporary = 1;

        assert(mln_http_parse(http, &c) == M_HTTP_RET_DONE);
        mln_http_reset(http);

        if (c != NULL) mln_chain_pool_release(c);
    }

    printf("[PASS] test_stability_parse_multiple (%d requests)\n", count);

    mln_http_destroy(http);
    mln_tcp_conn_destroy(&conn);
}

static void test_stability_generate_multiple(void)
{
    mln_http_t *http;
    mln_tcp_conn_t conn;
    mln_alloc_t *pool;
    mln_chain_t *head, *tail;
    mln_string_t key, val;
    char val_str[256];
    int i;
    int count = 10000;

    assert(mln_tcp_conn_init(&conn, -1) == 0);
    assert((http = mln_http_init(&conn, NULL, NULL)) != NULL);
    pool = mln_tcp_conn_pool_get(&conn);

    mln_http_type_set(http, M_HTTP_RESPONSE);
    mln_http_version_set(http, M_HTTP_VERSION_1_1);
    mln_string_set(&key, "Server");

    for (i = 0; i < count; i++) {
        mln_http_type_set(http, M_HTTP_RESPONSE);
        mln_http_version_set(http, M_HTTP_VERSION_1_1);
        mln_http_status_set(http, (i % 5 == 0) ? M_HTTP_OK : M_HTTP_NOT_FOUND);

        snprintf(val_str, sizeof(val_str), "Melon-%d", i);
        val.data = (mln_u8ptr_t)val_str;
        val.len = strlen(val_str);
        val.data_ref = 1;
        val.pool = 0;
        val.ref = 1;

        assert(mln_http_field_set(http, &key, &val) == 0);
        head = NULL;
        tail = NULL;
        assert(mln_http_generate(http, &head, &tail) == M_HTTP_RET_DONE);

        if (head != NULL) mln_chain_pool_release_all(head);
        mln_http_reset(http);
    }

    printf("[PASS] test_stability_generate_multiple (%d responses)\n", count);

    mln_http_destroy(http);
    mln_tcp_conn_destroy(&conn);
}

int main(void)
{
    printf("===== HTTP Module Tests =====\n\n");

    printf("=== Request Parsing ===\n");
    test_request_parse_get();
    test_request_parse_post();
    test_all_http_methods();
    test_http_versions();
    test_request_with_query_string();
    test_multiple_headers();

    printf("\n=== Response Parsing ===\n");
    test_response_parse();
    test_response_status_codes();

    printf("\n=== Header Field Operations ===\n");
    test_field_operations();
    test_case_insensitive_headers();

    printf("\n=== HTTP Generation ===\n");
    test_generate_request();
    test_generate_response();

    printf("\n=== Reset and Reuse ===\n");
    test_reset();

    printf("\n=== Performance ===\n");
    test_performance_parse_generate();

    printf("\n=== Stability ===\n");
    test_stability_parse_multiple();
    test_stability_generate_multiple();

    printf("\n===== All tests passed! =====\n");
    return 0;
}
