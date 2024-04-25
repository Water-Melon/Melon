#include <stdio.h>
#include "mln_http.h"
#include <assert.h>

int main(void)
{
    mln_http_t *http;
    mln_tcp_conn_t conn;
    mln_alloc_t *pool;
    mln_chain_t *c, *head = NULL, *tail = NULL;
    mln_buf_t *b;
    char req[] = "GET / HTTP/1.1\r\nHost: 127.0.0.1:8080\r\nUser-Agent: curl/7.81.0\r\n\r\n";
    mln_string_t key = mln_string("Server");
    mln_string_t val = mln_string("Melon");

    assert(mln_tcp_conn_init(&conn, -1) == 0);
    assert((http = mln_http_init(&conn, NULL, NULL)) != NULL); //no body

    pool = mln_tcp_conn_pool_get(&conn);

    assert((c = mln_chain_new(pool)) != NULL);
    assert((b = mln_buf_new(pool)) != NULL);
    c->buf = b;
    b->start = b->pos = b->left_pos = (mln_u8ptr_t)req;
    b->last = b->end = (mln_u8ptr_t)req + sizeof(req) - 1;
    b->temporary = 1;

    assert(mln_http_parse(http, &c) == M_HTTP_RET_DONE);

    mln_http_type_set(http, M_HTTP_RESPONSE);

    assert(mln_http_field_set(http, &key, &val) == 0);

    if (c != NULL) mln_chain_pool_release(c);

    assert(mln_http_generate(http, &head, &tail) == M_HTTP_RET_DONE);

    for (c = head; c != NULL; c = c->next) {
        if (mln_buf_size(c->buf)) {
            write(STDOUT_FILENO, c->buf->start, mln_buf_size(c->buf));
        }
    }

    mln_chain_pool_release_all(head);
    mln_http_destroy(http);
    mln_tcp_conn_destroy(&conn);

    return 0;
}

