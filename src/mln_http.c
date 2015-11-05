
/*
 * Copyright (C) Niklaus F.Schen.
 */

#include "mln_http.h"
#include "mln_log.h"

static mln_chain_t *
mln_http_header_send_default_handle(mln_string_t *key, \
                                    mln_string_t **val, \
                                    mln_http_header_t *header, \
                                    void *data);

mln_http_t *mln_http_init(mln_alloc_t *pool)
{
    mln_http_t *http;
    http = (mln_http_t *)mln_alloc_m(pool, sizeof(mln_http_t));
    if (http == NULL) return NULL;
    http->pool = pool;
    http->data = NULL;
    http->conn = NULL;
    http->handle = NULL;
    http->header = NULL;
    http->body_head = http->body_tail = NULL;
    memset(&(http->peer_addr), 0, sizeof(struct sockaddr));
    http->header_done = 0;
    http->status_code = 0;

    return http;
}

void mln_http_destroy(mln_http_t *http)
{
    if (http == NULL) return;

    mln_alloc_free(http);
}

void mln_http_set_send_default_handle(mln_http_t *http)
{
    mln_http_header_t *header = mln_http_get_header(http);
    int i;
    http_header handle;

    for (i = 0; i < M_HTTP_HEADER_NR_FIELDS; i++) {
        handle = mln_http_header_get_handle(header, i);
        if (handle == NULL) {
            mln_http_header_set_handle(header, i, mln_http_header_send_default_handle);
        }
    }
}

static mln_chain_t *
mln_http_header_send_default_handle(mln_string_t *key, \
                                    mln_string_t **val, \
                                    mln_http_header_t *header, \
                                    void *data)
{
    if (*val == NULL) {
        mln_http_header_set_retval(header, M_HTTP_HEADER_RET_OK);
        return NULL;
    }

    mln_alloc_t *pool = mln_http_header_get_pool(header);
    mln_size_t n = 0;
    mln_size_t val_len = (*val)->len;
    mln_size_t len = key->len + 4 + val_len;
    mln_chain_t *c = mln_chain_new(pool);
    mln_buf_t *b = mln_buf_new(pool);
    mln_u8ptr_t buf = (mln_u8ptr_t)mln_alloc_m(pool, len);

    if (c == NULL || b == NULL || buf == NULL) {
        mln_http_header_set_retval(header, M_HTTP_HEADER_RET_ERROR);
        return NULL;
    }

    memcpy(buf, key->str, key->len);
    n += key->len;
    buf[n++] = ':';
    buf[n++] = ' ';
    memcpy(buf + n, (*val)->str, val_len);
    n += val_len;
    buf[n++] = '\r';
    buf[n++] = '\n';

    c->buf = b;
    b->send_pos = b->pos = b->start = buf;
    b->last = b->end = buf + n;
    b->in_memory = 1;
    b->last_buf = 1;

    mln_http_header_set_retval(header, M_HTTP_HEADER_RET_OK);
    return c;
}

mln_chain_t *mln_http_packup(mln_http_t *http)
{
    http_handle create_body_handle;
    mln_chain_t *out = NULL, *c, **ll;
    mln_http_header_t *header = mln_http_get_header(http);

    if (!mln_http_get_header_done(http)) {
        if (header == NULL) {
            mln_http_set_status_code(http, M_HTTP_STATUS_CODE_ERROR);
            return NULL;
        }

        out = mln_http_header_packup(header);
        if (mln_http_header_get_retval(header) != M_HTTP_HEADER_RET_OK) {
            mln_http_set_status_code(http, M_HTTP_STATUS_CODE_ERROR);
            return NULL;
        }

        mln_http_set_header_done(http);
    }

    ll = &out;
    for (c = out; c != NULL; c = c->next) {
        ll = &(c->next);
    }

    create_body_handle = mln_http_get_handle(http);
    if (create_body_handle != NULL) {
        c = create_body_handle(http, NULL, mln_http_get_data(http));
        mln_http_body_append(http, c);
    } else {
        mln_http_set_status_code(http, M_HTTP_STATUS_CODE_DONE);
    }

    *ll = mln_http_body_head(http);

    return out;
}

mln_chain_t *mln_http_parse(mln_http_t *http, mln_chain_t *in)
{
    if (in == NULL) {
        mln_http_set_status_code(http, M_HTTP_STATUS_CODE_OK);
        return in;
    }

    mln_s32_t rc, status;
    mln_chain_t *header_left = NULL;
    mln_http_header_t *header = mln_http_get_header(http);
    http_handle handle;

    if (!mln_http_get_header_done(http)) {
        header_left = mln_http_header_parse(header, in);
        rc = mln_http_header_get_retval(header);
        if (rc == M_HTTP_HEADER_RET_OK) {
            mln_http_set_status_code(http, M_HTTP_STATUS_CODE_OK);
            return header_left;
        } else if (rc == M_HTTP_HEADER_RET_ERROR) {
            mln_http_set_status_code(http, M_HTTP_STATUS_CODE_ERROR);
            return header_left;
        }

        mln_http_set_header_done(http);

        status = mln_http_header_get_status(header);
        if (!mln_http_header_is_request(header) && \
            (status < M_HTTP_HEADER_OK || \
             status == M_HTTP_HEADER_NO_CONTENT || \
             status == M_HTTP_HEADER_NOT_MODIFIED))
        {
            mln_http_set_status_code(http, M_HTTP_STATUS_CODE_DONE);
            return header_left;
        }
    }

    handle = mln_http_get_handle(http);
    if (handle != NULL) {
        return handle(http, header_left, mln_http_get_data(http));
    }

    mln_http_set_status_code(http, M_HTTP_STATUS_CODE_OK);
    return header_left;
}

