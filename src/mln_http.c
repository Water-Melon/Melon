
/*
 * Copyright (C) Niklaus F.Schen.
 */

#include "mln_http.h"
#include <stdio.h>
#if !defined(MSVC)
#include <unistd.h>
#endif
#include "mln_types.h"
#include "mln_func.h"


struct mln_http_chain_s {
    mln_http_t  *http;
    mln_chain_t *head;
    mln_chain_t *tail;
    mln_u8ptr_t  pos;
    mln_size_t   left_size;
};

static inline int mln_http_line_length(mln_http_t *http, mln_chain_t *in, mln_size_t *len);
static inline int mln_http_process_line(mln_http_t *http, mln_chain_t **in, mln_size_t len);
static inline int mln_http_parse_headline(mln_http_t *http, mln_u8ptr_t buf, mln_size_t len);
static inline int mln_http_parse_field(mln_http_t *http, mln_u8ptr_t buf, mln_size_t len);
static void mln_http_hash_free(void *data);
static mln_u64_t mln_http_hash_calc(mln_hash_t *h, void *key);
static int mln_http_hash_cmp(mln_hash_t *h, void *key1, void *key2);
static inline int mln_http_atou(mln_string_t *s, mln_u32_t *status);
static int mln_http_dump_iterate_handler(mln_hash_t *h, void *key, void *val, void *data);
static inline int
mln_http_generate_version(struct mln_http_chain_s *hc);
static inline int
mln_http_generate_status(struct mln_http_chain_s *hc);
static inline int
mln_http_generate_method(struct mln_http_chain_s *hc);
static inline int
mln_http_generate_uri(struct mln_http_chain_s *hc);
static int
mln_http_generate_fields_hash_iterate_handler(mln_hash_t *h, void *key, void *val, void *data);
static inline int
#if defined(MSVC)
mln_http_generate_write(struct mln_http_chain_s *hc, mln_u8ptr_t buf, mln_size_t size);
#else
mln_http_generate_write(struct mln_http_chain_s *hc, void *buf, mln_size_t size);
#endif
static inline int
mln_http_generate_set_last_in_chain(struct mln_http_chain_s *hc);

mln_string_t http_version[] = {
    mln_string("HTTP/1.0"),
    mln_string("HTTP/1.1")
};

mln_string_t http_method[] = {
    mln_string("GET"),
    mln_string("POST"),
    mln_string("HEAD"),
    mln_string("PUT"),
    mln_string("DELETE"),
    mln_string("TRACE"),
    mln_string("CONNECT"),
    mln_string("OPTIONS")
};

mln_http_map_t mln_http_status[] = {
{mln_string("Continue"),                        mln_string("100"), M_HTTP_CONTINUE},
{mln_string("Switching Protocols"),             mln_string("101"), M_HTTP_SWITCHING_PROTOCOLS},
{mln_string("Processing"),                      mln_string("102"), M_HTTP_PROCESSING},
{mln_string("OK"),                              mln_string("200"), M_HTTP_OK},
{mln_string("Created"),                         mln_string("201"), M_HTTP_CREATED},
{mln_string("Accepted"),                        mln_string("202"), M_HTTP_ACCEPTED},
{mln_string("Non-Authoritative Information"),   mln_string("203"), M_HTTP_NON_AUTHORITATIVE_INFORMATION},
{mln_string("No Content"),                      mln_string("204"), M_HTTP_NO_CONTENT},
{mln_string("Reset Content"),                   mln_string("205"), M_HTTP_RESET_CONTENT},
{mln_string("Partial Content"),                 mln_string("206"), M_HTTP_PARTIAL_CONTENT},
{mln_string("Multi-Status"),                    mln_string("207"), M_HTTP_MULTI_STATUS},
{mln_string("Multiple Choices"),                mln_string("300"), M_HTTP_MULTIPLE_CHOICES},
{mln_string("Moved Permanently"),               mln_string("301"), M_HTTP_MOVED_PERMANENTLY},
{mln_string("Move temporarily"),                mln_string("302"), M_HTTP_MOVED_TEMPORARILY},
{mln_string("See Other"),                       mln_string("303"), M_HTTP_SEE_OTHER},
{mln_string("Not Modified"),                    mln_string("304"), M_HTTP_NOT_MODIFIED},
{mln_string("Use Proxy"),                       mln_string("305"), M_HTTP_USE_PROXY},
{mln_string("Switch Proxy"),                    mln_string("306"), M_HTTP_SWITCH_PROXY},
{mln_string("Temporary Redirect"),              mln_string("307"), M_HTTP_TEMPORARY_REDIRECT},
{mln_string("Bad Request"),                     mln_string("400"), M_HTTP_BAD_REQUEST},
{mln_string("Unauthorized"),                    mln_string("401"), M_HTTP_UNAUTHORIZED},
{mln_string("Payment Required"),                mln_string("402"), M_HTTP_PAYMENT_REQUIRED},
{mln_string("Forbidden"),                       mln_string("403"), M_HTTP_FORBIDDEN},
{mln_string("Not Found"),                       mln_string("404"), M_HTTP_NOT_FOUND},
{mln_string("Method Not Allowed"),              mln_string("405"), M_HTTP_METHOD_NOT_ALLOWED},
{mln_string("Not Acceptable"),                  mln_string("406"), M_HTTP_NOT_ACCEPTABLE},
{mln_string("Proxy Authentication Required"),   mln_string("407"), M_HTTP_PROXY_AUTHENTICATION_REQUIRED},
{mln_string("Request Timeout"),                 mln_string("408"), M_HTTP_REQUEST_TIMEOUT},
{mln_string("Conflict"),                        mln_string("409"), M_HTTP_CONFLICT},
{mln_string("Gone"),                            mln_string("410"), M_HTTP_GONE},
{mln_string("Length Required"),                 mln_string("411"), M_HTTP_LENGTH_REQUIRED},
{mln_string("Precondition Failed"),             mln_string("412"), M_HTTP_PRECONDITION_FAILED},
{mln_string("Request Entity Too Large"),        mln_string("413"), M_HTTP_REQUEST_ENTITY_TOO_LARGE},
{mln_string("Request-URI Too Long"),            mln_string("414"), M_HTTP_REQUEST_URI_TOO_LARGE},
{mln_string("Unsupported Media Type"),          mln_string("415"), M_HTTP_UNSUPPORTED_MEDIA_TYPE},
{mln_string("Requested Range Not Satisfiable"), mln_string("416"), M_HTTP_REQUESTED_RANGE_NOT_SATISFIABLE},
{mln_string("Expectation Failed"),              mln_string("417"), M_HTTP_EXPECTATION_FAILED},
{mln_string("There are too many connections from your internet address"), \
                                                mln_string("421"), M_HTTP_TOO_MANY_CONNECTIONS},
{mln_string("Unprocessable Entity"),            mln_string("422"), M_HTTP_UNPROCESSABLE_ENTITY},
{mln_string("Locked"),                          mln_string("423"), M_HTTP_LOCKED},
{mln_string("Failed Dependency"),               mln_string("424"), M_HTTP_FAILED_DEPENDENCY},
{mln_string("Unordered Collection"),            mln_string("425"), M_HTTP_UNORDERED_COLLECTION},
{mln_string("Upgrade Required"),                mln_string("426"), M_HTTP_UPGRADE_REQUIRED},
{mln_string("Retry With"),                      mln_string("449"), M_HTTP_RETRY_WITH},
{mln_string("Internal Server Error"),           mln_string("500"), M_HTTP_INTERNAL_SERVER_ERROR},
{mln_string("Not Implemented"),                 mln_string("501"), M_HTTP_NOT_IMPLEMENTED},
{mln_string("Bad Gateway"),                     mln_string("502"), M_HTTP_BAD_GATEWAY},
{mln_string("Service Unavailable"),             mln_string("503"), M_HTTP_SERVICE_UNAVAILABLE},
{mln_string("Gateway Timeout"),                 mln_string("504"), M_HTTP_GATEWAY_TIMEOUT},
{mln_string("HTTP Version Not Supported"),      mln_string("505"), M_HTTP_VERSION_NOT_SUPPORTED},
{mln_string("Variant Also Negotiates"),         mln_string("506"), M_HTTP_VARIANT_ALSO_NEGOTIATES},
{mln_string("Insufficient Storage"),            mln_string("507"), M_HTTP_INSUFFICIENT_STORAGE},
{mln_string("Bandwidth Limit Exceeded"),        mln_string("509"), M_HTTP_BANDWIDTH_LIMIT_EXCEEDED},
{mln_string("Not Extended"),                    mln_string("510"), M_HTTP_NOT_EXTENDED},
{mln_string("Unparseable Response Headers"),    mln_string("600"), M_HTTP_UNPARSEABLE_RESPONSE_HEADERS}
};


MLN_FUNC(, int, mln_http_parse, (mln_http_t *http, mln_chain_t **in), (http, in), {
    if (http == NULL) return M_HTTP_RET_ERROR;

    int ret = M_HTTP_RET_DONE, rc;
    mln_size_t len = 0;
    mln_http_handler handler = mln_http_handler_get(http);

    while (!mln_http_done_get(http) && \
           (ret = mln_http_line_length(http, *in, &len)) == M_HTTP_RET_DONE)
    {
        if ((rc = mln_http_process_line(http, in, len) == M_HTTP_RET_ERROR))
            return rc;
    }
    if (ret == M_HTTP_RET_OK || ret == M_HTTP_RET_ERROR) return ret;

    if (handler != NULL) ret = handler(http, in, NULL);
    if (ret == M_HTTP_RET_DONE) {
        mln_http_done_set(http, 0);
    }
    return ret;
})

MLN_FUNC(static inline, int, mln_http_line_length, \
         (mln_http_t *http, mln_chain_t *in, mln_size_t *len), \
         (http, in, len), \
{
    mln_buf_t *b;
    mln_u8ptr_t p, end;
    mln_size_t length = 0;

    while (in != NULL) {
        b = in->buf;
        if (b == NULL || b->in_file || mln_buf_left_size(b) <= 0) {
            in = in->next;
            continue;
        }
        for (p = b->left_pos, end = b->last; p < end; ++p) {
            if (*p == (mln_u8_t)'\n') break;
            ++length;
        }
        if (p >= end) {
            in = in->next;
            continue;
        }
        break;
    }
    if (in == NULL) return M_HTTP_RET_OK;

    *len = length;
    return M_HTTP_RET_DONE;
})

MLN_FUNC(static inline, int, mln_http_process_line, \
         (mln_http_t *http, mln_chain_t **in, mln_size_t len), \
         (http, in, len), \
{
    int ret;
    mln_buf_t *b;
    mln_chain_t *c;
    mln_u8ptr_t buf, p, last;
    mln_alloc_t *pool = mln_http_pool_get(http);
    mln_u32_t type = mln_http_type_get(http);

    buf = (mln_u8ptr_t)mln_alloc_m(pool, len+1);
    if (buf == NULL) {
        mln_http_error_set(http, M_HTTP_INTERNAL_SERVER_ERROR);
        return M_HTTP_RET_ERROR;
    }
    buf[len] = 0;
    p = buf;

    while ((c = *in) != NULL) {
        b = c->buf;
        if (b == NULL || b->in_file || mln_buf_left_size(b) <= 0) {
            *in = (*in)->next;
            mln_chain_pool_release(c);
            continue;
        }
        while (mln_buf_left_size(b) > 0) {
            if (*(b->left_pos) == '\n') break;
            *p++ = *(b->left_pos++);
        }
        if (mln_buf_left_size(b) > 0) {
            ++(b->left_pos);
            break;
        }
    }

    for (last = buf + len - 1; last > buf; --last, --len) {
        if (*last != 0) break;
    }
    if (len == 0 || (len == 1 && buf[0] == '\r')) {
        mln_alloc_free(buf);
        mln_http_done_set(http, 1);
        return M_HTTP_RET_OK;
    }

    if (buf[len-1] == '\r') {
        buf[len-1] = 0;
        --len;
    }

    if (type == M_HTTP_UNKNOWN) {
        ret = mln_http_parse_headline(http, buf, len);
    } else {
        ret = mln_http_parse_field(http, buf, len);
    }

    mln_alloc_free(buf);

    return ret;
})

MLN_FUNC(static inline, int, mln_http_parse_headline, \
         (mln_http_t *http, mln_u8ptr_t buf, mln_size_t len), \
         (http, buf, len), \
{
    mln_u8ptr_t p, end = buf + len, ques;
    mln_string_t tmp, *s, *scan, *send;
    mln_u32_t type, status = 0;
    mln_alloc_t *pool = mln_http_pool_get(http);

    /*first part*/
    for (; buf < end; ++buf) {
        if (*buf != (mln_u8_t)' ' && *buf != (mln_u8_t)'\t')
            break;
    }
    if (buf >= end) {
        mln_http_done_set(http, 1);
        return M_HTTP_RET_OK;
    }
    for (p = buf; p < end; ++p) {
        if (*p == (mln_u8_t)' ' || *p == (mln_u8_t)'\t')
            break;
    }
    mln_string_nset(&tmp, buf, p-buf);
    send = http_version + sizeof(http_version)/sizeof(mln_string_t);
    for (scan = http_version; scan < send; ++scan) {
        if (!mln_string_strcasecmp(&tmp, scan)) break;
    }
    if (scan < send) {
        type = M_HTTP_RESPONSE;
        mln_http_type_set(http, M_HTTP_RESPONSE);
        mln_http_version_set(http, scan - http_version);
    } else {
        send = http_method + sizeof(http_method)/sizeof(mln_string_t);
        for (scan = http_method; scan < send; ++scan) {
            if (!mln_string_strcasecmp(&tmp, scan)) break;
        }
        if (scan < send) {
            type = M_HTTP_REQUEST;
            mln_http_type_set(http, M_HTTP_REQUEST);
            mln_http_method_set(http, scan - http_method);
        } else { 
            mln_http_error_set(http, M_HTTP_BAD_REQUEST);
            return M_HTTP_RET_ERROR;
        }
    }
    buf = p;

    /*second*/
    for (; buf < end; ++buf) {
        if (*buf != (mln_u8_t)' ' && *buf != (mln_u8_t)'\t')
            break;
    }
    if (buf >= end) {
        if (type == M_HTTP_REQUEST) {
            mln_http_error_set(http, M_HTTP_BAD_REQUEST);
        } else {
            mln_http_error_set(http, M_HTTP_UNPARSEABLE_RESPONSE_HEADERS);
        }
        return M_HTTP_RET_ERROR;
    }
    if (type == M_HTTP_REQUEST) {
        for (ques = NULL, p = buf; p < end; ++p) {
            if (ques == NULL && *p == (mln_u8_t)'?') ques = p;
            if (*p == (mln_u8_t)' ' || *p == (mln_u8_t)'\t')
                break;
        }
        if (ques == NULL || ques+1 >= p) {
            mln_string_nset(&tmp, buf, (ques == NULL)? p-buf: ques-buf);
            s = mln_string_pool_dup(pool, &tmp);
            if (s == NULL) {
                mln_http_error_set(http, M_HTTP_INTERNAL_SERVER_ERROR);
                return M_HTTP_RET_ERROR;
            }
            mln_http_uri_set(http, s);
            mln_http_args_set(http, NULL);
        } else {
            mln_string_nset(&tmp, buf, ques-buf);
            s = mln_string_pool_dup(pool, &tmp);
            if (s == NULL) {
                mln_http_error_set(http, M_HTTP_INTERNAL_SERVER_ERROR);
                return M_HTTP_RET_ERROR;
            }
            mln_http_uri_set(http, s);
            mln_string_nset(&tmp, ++ques, p - ques);
            s = mln_string_pool_dup(pool, &tmp);
            if (s == NULL) {
                mln_http_error_set(http, M_HTTP_INTERNAL_SERVER_ERROR);
                return M_HTTP_RET_ERROR;
            }
            mln_http_args_set(http, s);
        }
    } else {
        for (p = buf; p < end; ++p) {
            if (*p == (mln_u8_t)' ' || *p == (mln_u8_t)'\t')
                break;
        }
        mln_string_nset(&tmp, buf, p-buf);
        if (mln_http_atou(&tmp, &status) == M_HTTP_RET_ERROR) {
            mln_http_error_set(http, M_HTTP_UNPARSEABLE_RESPONSE_HEADERS);
            return M_HTTP_RET_ERROR;
        }
        mln_http_status_set(http, status);
    }
    buf = p;

    /*third*/
    for (; buf < end; ++buf) {
        if (*buf != (mln_u8_t)' ' && *buf != (mln_u8_t)'\t')
            break;
    }
    if (buf >= end) {
        if (type == M_HTTP_REQUEST) {
            mln_http_version_set(http, M_HTTP_VERSION_1_0);
        } else {
            mln_http_response_msg_set(http, NULL);
        }
        return M_HTTP_RET_OK;
    }

    if (type == M_HTTP_REQUEST) {
        mln_string_nset(&tmp, buf, end-buf);
        send = http_version + sizeof(http_version)/sizeof(mln_string_t);
        for (scan = http_version; scan < send; ++scan) {
            if (!mln_string_strcasecmp(&tmp, scan))
                break;
        }
        if (scan >= send) {
            mln_http_error_set(http, M_HTTP_BAD_REQUEST);
            return M_HTTP_RET_ERROR;
        }
        mln_http_version_set(http, scan - http_version);
        return M_HTTP_RET_OK;
    }
    mln_string_nset(&tmp, buf, end-buf);
    s = mln_string_pool_dup(pool, &tmp);
    if (s == NULL) {
        mln_http_error_set(http, M_HTTP_INTERNAL_SERVER_ERROR);
        return M_HTTP_RET_ERROR;
    }
    mln_http_response_msg_set(http, s);
    return M_HTTP_RET_OK;
})

MLN_FUNC(static inline, int, mln_http_parse_field, \
         (mln_http_t *http, mln_u8ptr_t buf, mln_size_t len), \
         (http, buf, len), \
{
    mln_u8ptr_t p, end = buf + len;
    mln_string_t tmp, *s, *v;
    mln_alloc_t *pool = mln_http_pool_get(http);
    mln_u32_t type = mln_http_type_get(http);
    mln_hash_t *header_fields = mln_http_header_get(http);

    /*field name*/
    for (; buf < end; ++buf) {
        if (*buf != (mln_u8_t)' ' && *buf != (mln_u8_t)'\t')
            break;
    }
    if (buf >= end) {
        mln_http_done_set(http, 1);
        return M_HTTP_RET_OK;
    }
    for (p = buf; p < end; ++p) {
        if (*p == (mln_u8_t)' ' || *p == (mln_u8_t)'\t' || *p == (mln_u8_t)':')
            break;
    }
    if (p - buf <= 0) {
        if (type == M_HTTP_REQUEST) {
            mln_http_error_set(http, M_HTTP_BAD_REQUEST);
        } else {
            mln_http_error_set(http, M_HTTP_UNPARSEABLE_RESPONSE_HEADERS);
        }
        return M_HTTP_RET_ERROR;
    }
    mln_string_nset(&tmp, buf, p-buf);
    s = mln_string_pool_dup(pool, &tmp);
    if (s == NULL) {
        mln_http_error_set(http, M_HTTP_INTERNAL_SERVER_ERROR);
        return M_HTTP_RET_ERROR;
    }
    buf = p;

    /* : */
    for (; buf < end; ++buf) {
        if (*buf != (mln_u8_t)' ' && *buf != (mln_u8_t)'\t')
            break;
    }
    if (buf >= end) {
        if (mln_hash_insert(header_fields, s, NULL) < 0) {
            mln_string_free(s);
            mln_http_error_set(http, M_HTTP_INTERNAL_SERVER_ERROR);
            return M_HTTP_RET_ERROR;
        }
        return M_HTTP_RET_OK;
    }
    if (buf[0] != (mln_u8_t)':') {
        mln_string_free(s);
        if (type == M_HTTP_REQUEST) {
            mln_http_error_set(http, M_HTTP_BAD_REQUEST);
        } else {
            mln_http_error_set(http, M_HTTP_UNPARSEABLE_RESPONSE_HEADERS);
        }
        return M_HTTP_RET_ERROR;
    }
    ++buf;

    /*field value*/
    for (; buf < end; ++buf) {
        if (*buf != (mln_u8_t)' ' && *buf != (mln_u8_t)'\t')
            break;
    }
    if (buf >= end) {
        if (mln_hash_insert(header_fields, s, NULL) < 0) {
            mln_string_free(s);
            mln_http_error_set(http, M_HTTP_INTERNAL_SERVER_ERROR);
            return M_HTTP_RET_ERROR;
        }
        return M_HTTP_RET_OK;
    }
    mln_string_nset(&tmp, buf, end-buf);
    v = mln_string_pool_dup(pool, &tmp);
    if (v == NULL) {
        mln_string_free(s);
        mln_http_error_set(http, M_HTTP_INTERNAL_SERVER_ERROR);
        return M_HTTP_RET_ERROR;
    }
    if (mln_hash_insert(header_fields, s, v) < 0) {
        mln_string_free(v);
        mln_string_free(s);
        mln_http_error_set(http, M_HTTP_INTERNAL_SERVER_ERROR);
        return M_HTTP_RET_ERROR;
    }

    return M_HTTP_RET_OK;
})

MLN_FUNC(, int, mln_http_generate, \
         (mln_http_t *http, mln_chain_t **out_head, mln_chain_t **out_tail), \
         (http, out_head, out_tail), \
{
    if (http == NULL || out_head == NULL || out_tail == NULL)
        return M_HTTP_RET_ERROR;

    mln_u32_t type = mln_http_type_get(http);
    mln_hash_t *header_fields = mln_http_header_get(http);
    mln_http_handler handler = mln_http_handler_get(http);
    struct mln_http_chain_s hc;
    int ret;

    if (type == M_HTTP_UNKNOWN) {
        mln_http_error_set(http, M_HTTP_INTERNAL_SERVER_ERROR);
        goto err;
    }

    hc.http = http;
    if (*out_head != NULL) {
        hc.head = *out_head;
        hc.tail = *out_tail;
    } else {
        hc.head = hc.tail = NULL;
    }
    hc.pos = NULL;
    hc.left_size = 0;

    if (!mln_http_done_get(http)) {
        if (handler != NULL) {
            if ((ret = handler(http, &http->body_head, &http->body_tail)) == M_HTTP_RET_ERROR) {
                goto err;
            } else if (ret == M_HTTP_RET_OK) {
                mln_http_error_set(http, M_HTTP_OK);
                *out_head = hc.head;
                *out_tail = hc.tail;
                return M_HTTP_RET_OK;
            }
        }
        mln_http_done_set(http, 1);
    }

    if (type == M_HTTP_RESPONSE) {
        if (mln_http_generate_version(&hc) == M_HTTP_RET_ERROR)
            goto err;
        if (mln_http_generate_write(&hc, " ", 1) == M_HTTP_RET_ERROR)
            goto err;
        if (mln_http_generate_status(&hc) == M_HTTP_RET_ERROR)
            goto err;
    } else {
        if (mln_http_generate_method(&hc) == M_HTTP_RET_ERROR)
            goto err;
        if (mln_http_generate_write(&hc, " ", 1) == M_HTTP_RET_ERROR)
            goto err;
        if (mln_http_generate_uri(&hc) == M_HTTP_RET_ERROR)
            goto err;
        if (mln_http_generate_write(&hc, " ", 1) == M_HTTP_RET_ERROR)
            goto err;
        if (mln_http_generate_version(&hc) == M_HTTP_RET_ERROR)
            goto err;
    }
    if (mln_http_generate_write(&hc, "\r\n", 2) == M_HTTP_RET_ERROR)
        goto err;

    if (header_fields != NULL) {
        if (mln_hash_iterate(header_fields, \
                              mln_http_generate_fields_hash_iterate_handler, \
                              &hc) < 0)
            goto err;
    }

    if (mln_http_generate_write(&hc, "\r\n", 2) == M_HTTP_RET_ERROR)
        goto err;


    if (http->body_head == NULL) {
        if (mln_http_generate_set_last_in_chain(&hc) == M_HTTP_RET_ERROR)
            goto err;
    }
    if (hc.head == NULL) {
        hc.head = http->body_head;
        hc.tail = http->body_tail;
    } else {
        hc.tail->next = http->body_head;
        hc.tail = http->body_tail;
    }
    http->body_head = http->body_tail = NULL;

    mln_http_done_set(http, 0);
    mln_http_error_set(http, M_HTTP_OK);
    *out_head = hc.head;
    *out_tail = hc.tail;
    return M_HTTP_RET_DONE;

err:
    mln_chain_pool_release_all(hc.head);
    *out_head = *out_tail = NULL;
    return M_HTTP_RET_ERROR;
})

MLN_FUNC(static inline, int, mln_http_generate_set_last_in_chain, \
         (struct mln_http_chain_s *hc), (hc), \
{
    if (hc->tail != NULL && hc->tail->buf != NULL) {
        hc->tail->buf->last_in_chain = 1;
        return M_HTTP_RET_OK;
    }

    mln_http_t *http = hc->http;
    mln_alloc_t *pool = mln_http_pool_get(http);
    mln_chain_t *c = mln_chain_new(pool);
    if (c == NULL) {
        mln_http_error_set(http, M_HTTP_INTERNAL_SERVER_ERROR);
        return M_HTTP_RET_ERROR;
    }
    mln_buf_t *b = mln_buf_new(pool);
    if (b == NULL) {
        mln_chain_pool_release(c);
        mln_http_error_set(http, M_HTTP_INTERNAL_SERVER_ERROR);
        return M_HTTP_RET_ERROR;
    }
    c->buf = b;
    b->in_memory = 1;
    b->last_in_chain = 1;
    if (hc->head == NULL) {
        hc->head = hc->tail;
    } else {
        hc->tail->next = c;
        hc->tail = c;
    }

    return M_HTTP_RET_OK;
})

#if defined(MSVC)
static inline int mln_http_generate_write(struct mln_http_chain_s *hc, mln_u8ptr_t buf, mln_size_t size)
#else
static inline int mln_http_generate_write(struct mln_http_chain_s *hc, void *buf, mln_size_t size)
#endif
{
    mln_buf_t *cur;
    mln_http_t *http = hc->http;
    mln_alloc_t *pool = mln_http_pool_get(http);

    while (size > 0) {
        if (hc->left_size == 0) {
            mln_chain_t *c = mln_chain_new(pool);
            if (c == NULL) {
                mln_http_error_set(http, M_HTTP_INTERNAL_SERVER_ERROR);
                return M_HTTP_RET_ERROR;
            }
            mln_buf_t *b = mln_buf_new(pool);
            if (b == NULL) {
                mln_chain_pool_release(c);
                mln_http_error_set(http, M_HTTP_INTERNAL_SERVER_ERROR);
                return M_HTTP_RET_ERROR;
            }
            c->buf = b;
            mln_u8ptr_t buffer = (mln_u8ptr_t)mln_alloc_m(pool, M_HTTP_GENERATE_ALLOC_SIZE);
            if (buffer == NULL) {
                mln_chain_pool_release(c);
                mln_http_error_set(http, M_HTTP_INTERNAL_SERVER_ERROR);
                return M_HTTP_RET_ERROR;
            }

            b->left_pos = b->pos = b->start = buffer;
            b->last = b->end = buffer;
            b->in_memory = 1;
            b->last_buf = 1;

            hc->pos = buffer;
            hc->left_size = M_HTTP_GENERATE_ALLOC_SIZE;
            if (hc->head == NULL) {
                hc->head = hc->tail = c;
            } else {
                hc->tail->next = c;
                hc->tail = c;
            }
        }

        if (hc->tail == NULL || hc->tail->buf == NULL) {
            hc->left_size = 0;
            continue;
        }

        cur = hc->tail->buf;
        if (size <= hc->left_size) {
            memcpy(hc->pos, buf, size);
            hc->left_size -= size;
            hc->pos += size;
            buf += size;
            size = 0;
        } else {
            memcpy(hc->pos, buf, hc->left_size);
            size -= hc->left_size;
            buf += hc->left_size;
            hc->pos += hc->left_size;
            hc->left_size = 0;
        }
        cur->last = cur->end = hc->pos;
    }

    return M_HTTP_RET_OK;
}

MLN_FUNC(static inline, int, mln_http_generate_version, \
         (struct mln_http_chain_s *hc), (hc), \
{
    mln_u32_t version = mln_http_version_get(hc->http);

    if (version >= sizeof(http_version)/sizeof(mln_string_t)) {
        if (mln_http_type_get(hc->http) == M_HTTP_REQUEST)
            mln_http_error_set(hc->http, M_HTTP_BAD_REQUEST);
        else
            mln_http_error_set(hc->http, M_HTTP_UNPARSEABLE_RESPONSE_HEADERS);
        return M_HTTP_RET_ERROR;
    }

    mln_string_t *p = &http_version[version];
    if (mln_http_generate_write(hc, p->data, p->len) == M_HTTP_RET_ERROR) {
        return M_HTTP_RET_ERROR;
    }
    return M_HTTP_RET_OK;
})

MLN_FUNC(static inline, int, mln_http_generate_status, (struct mln_http_chain_s *hc), (hc), {
    mln_u32_t status = mln_http_status_get(hc->http);
    mln_http_map_t *map = mln_http_status;
    mln_http_map_t *end = mln_http_status + sizeof(mln_http_status)/sizeof(mln_http_map_t);

    for (; map < end; ++map) {
        if (status == map->code) break;
    }
    if (map >= end) {
        mln_http_error_set(hc->http, M_HTTP_UNPARSEABLE_RESPONSE_HEADERS);
        return M_HTTP_RET_ERROR;
    }

    if (mln_http_generate_write(hc, map->code_str.data, map->code_str.len) == M_HTTP_RET_ERROR)
        return M_HTTP_RET_ERROR;
    if (mln_http_generate_write(hc, " ", 1) == M_HTTP_RET_ERROR)
        return M_HTTP_RET_ERROR;
    if (mln_http_generate_write(hc, map->msg_str.data, map->msg_str.len) == M_HTTP_RET_ERROR)
        return M_HTTP_RET_ERROR;

    return M_HTTP_RET_OK;
})

MLN_FUNC(static inline, int, mln_http_generate_method, (struct mln_http_chain_s *hc), (hc), {
    mln_u32_t method = mln_http_method_get(hc->http);
    if (method >= sizeof(http_method)/sizeof(mln_string_t)) {
        mln_http_error_set(hc->http, M_HTTP_BAD_REQUEST);
        return M_HTTP_RET_ERROR;
    }

    mln_string_t *p = &http_method[method];
    if (mln_http_generate_write(hc, p->data, p->len) == M_HTTP_RET_ERROR)
        return M_HTTP_RET_ERROR;

    return M_HTTP_RET_OK;
})

MLN_FUNC(static inline, int, mln_http_generate_uri, (struct mln_http_chain_s *hc), (hc), {
    mln_string_t *uri = mln_http_uri_get(hc->http);
    if (uri == NULL) {
        if (mln_http_generate_write(hc, "/", 1) == M_HTTP_RET_ERROR)
            return M_HTTP_RET_ERROR;
    } else {
        if (mln_http_generate_write(hc, uri->data, uri->len) == M_HTTP_RET_ERROR)
            return M_HTTP_RET_ERROR;
    }

    mln_string_t *args = mln_http_args_get(hc->http);
    if (args != NULL) {
        if (mln_http_generate_write(hc, "?", 1) == M_HTTP_RET_ERROR)
            return M_HTTP_RET_ERROR;
        if (mln_http_generate_write(hc, args->data, args->len) == M_HTTP_RET_ERROR)
            return M_HTTP_RET_ERROR;
    }

    return M_HTTP_RET_OK;
})

MLN_FUNC(static, int, mln_http_generate_fields_hash_iterate_handler, \
         (mln_hash_t *h, void *key, void *val, void *data), (h, key, val, data), \
{
    mln_string_t *k = (mln_string_t *)key;
    mln_string_t *v = (mln_string_t *)val;
    struct mln_http_chain_s *hc = (struct mln_http_chain_s *)data;

    if (mln_http_generate_write(hc, k->data, k->len) == M_HTTP_RET_ERROR)
        return -1;
    if (mln_http_generate_write(hc, ": ", 2) == M_HTTP_RET_ERROR)
        return -1;
    if (val != NULL) {
        if (mln_http_generate_write(hc, v->data, v->len) == M_HTTP_RET_ERROR)
            return -1;
    }
    if (mln_http_generate_write(hc, "\r\n", 2) == M_HTTP_RET_ERROR)
        return -1;

    return 0;
})


MLN_FUNC(, int, mln_http_field_set, \
         (mln_http_t *http, mln_string_t *key, mln_string_t *val), \
         (http, key, val), \
{
    if (http == NULL || key == NULL) {
        return M_HTTP_RET_ERROR;
    }

    mln_hash_t *header_fields = mln_http_header_get(http);
    if (header_fields == NULL) return M_HTTP_RET_ERROR;

    mln_alloc_t *pool = mln_http_pool_get(http);
    mln_string_t *dup_key, *dup_val;
    dup_key = mln_string_pool_dup(pool, key);
    if (dup_key == NULL) return M_HTTP_RET_ERROR;
    dup_val = mln_string_pool_dup(pool, val);
    if (dup_val == NULL) {
        mln_string_free(dup_key);
        return M_HTTP_RET_ERROR;
    }
    int ret = mln_hash_update(header_fields, &dup_key, &dup_val);
    mln_string_free(dup_key);
    mln_string_free(dup_val);

    if (ret < 0) return M_HTTP_RET_ERROR;
    return M_HTTP_RET_OK;
})

MLN_FUNC(, mln_string_t *, mln_http_field_get, \
         (mln_http_t *http, mln_string_t *key), (http, key), \
{
    if (http == NULL) return NULL;

    mln_hash_t *header_fields = mln_http_header_get(http);
    if (header_fields == NULL) return NULL;

    return (mln_string_t *)mln_hash_search(header_fields, key);
})

MLN_FUNC(, mln_string_t *, mln_http_field_iterator, \
         (mln_http_t *http, mln_string_t *key), (http, key), \
{
    int *ctx = NULL;
    mln_string_t *val;
    mln_u8ptr_t buf;
    mln_u32_t size = 0, cnt = 0;
    mln_alloc_t *pool = mln_http_pool_get(http);
    mln_hash_t *header = mln_http_header_get(http);

    do {
        val = mln_hash_search_iterator(header, key, &ctx);
        if (val != NULL) {
            size += (val->len + 1);
            ++cnt;
        }
    } while (ctx != NULL);
    if (cnt < 1) return NULL;

    buf = (mln_u8ptr_t)mln_alloc_m(pool, size+1);
    if (buf == NULL) return NULL;
    size = 0;
    do {
        val = mln_hash_search_iterator(header, key, &ctx);
        if (val != NULL) {
            memcpy(buf+size, val->data, val->len);
            size += val->len;
            if (cnt-- > 1) buf[size++] = ',';
        }
    } while (ctx != NULL);

    mln_string_t tmp;
    mln_string_nset(&tmp, buf, size);
    val = mln_string_pool_dup(pool, &tmp);
    mln_alloc_free(buf);

    return val;
})

MLN_FUNC_VOID(, void, mln_http_field_remove, (mln_http_t *http, mln_string_t *key), (http, key), {
    if (http == NULL || key == NULL) return;

    mln_string_t *val;
    mln_hash_t *header = mln_http_header_get(http);
    while ((val = (mln_string_t *)mln_hash_search(header, key)) != NULL) {
        mln_hash_remove(header, key, M_HASH_F_KV);
    }
})

MLN_FUNC(static inline, int, mln_http_atou, (mln_string_t *s, mln_u32_t *status), (s, status), {
    mln_u32_t st = 0;
    mln_u8ptr_t p, end = s->data + s->len;

    for (p = s->data; p < end; ++p) {
        if (!mln_isdigit(*p)) return M_HTTP_RET_ERROR;
        st *= 10;
        st += (*p - '0');
    }

    *status = st;

    return M_HTTP_RET_OK;
})

MLN_FUNC(, mln_http_t *, mln_http_init, \
         (mln_tcp_conn_t *connection, void *data, mln_http_handler body_handler), \
         (connection, data, body_handler), \
{
    if (connection == NULL) return NULL;

    mln_http_t *http;
    struct mln_hash_attr hattr;
    mln_alloc_t *pool = mln_tcp_conn_pool_get(connection);
    if (pool == NULL) return NULL;

    http = (mln_http_t *)mln_alloc_m(pool, sizeof(mln_http_t));
    if (http == NULL) return NULL;

    http->connection = connection;
    http->pool = pool;

    hattr.pool = pool;
    hattr.pool_alloc = (hash_pool_alloc_handler)mln_alloc_m;
    hattr.pool_free = (hash_pool_free_handler)mln_alloc_free;
    hattr.hash = mln_http_hash_calc;
    hattr.cmp = mln_http_hash_cmp;
    hattr.key_freer = mln_http_hash_free;
    hattr.val_freer = mln_http_hash_free;
    hattr.len_base = M_HTTP_HASH_LEN;
    hattr.expandable = 0;
    hattr.calc_prime = 0;
    http->header_fields = mln_hash_new(&hattr);
    if (http->header_fields == NULL) {
        mln_alloc_free(http);
        return NULL;
    }
    http->body_head = http->body_tail = NULL;
    http->body_handler = body_handler;
    http->data = data;
    http->uri = NULL;
    http->args = NULL;
    http->response_msg = NULL;
    http->error = M_HTTP_OK;
    http->status = M_HTTP_OK;
    http->method = 0;
    http->version = 0;
    http->type = M_HTTP_UNKNOWN;
    http->done = 0;

    return http;
})

MLN_FUNC_VOID(, void, mln_http_destroy, (mln_http_t *http), (http), {
    if (http == NULL) return;

    if (http->header_fields != NULL) {
        mln_hash_free(http->header_fields, M_HASH_F_KV);
    }
    if (http->body_head != NULL) {
        mln_chain_pool_release_all(http->body_head);
    }
    if (http->uri != NULL) {
        mln_string_free(http->uri);
    }
    if (http->args != NULL) {
        mln_string_free(http->args);
    }
    if (http->response_msg != NULL) {
        mln_string_free(http->response_msg);
    }

    mln_alloc_free(http);
})

MLN_FUNC_VOID(, void, mln_http_reset, (mln_http_t *http), (http), {
    if (http == NULL) return;

    if (http->header_fields != NULL) {
        mln_hash_reset(http->header_fields, M_HASH_F_KV);
    }
    if (http->body_head != NULL) {
        mln_chain_pool_release_all(http->body_head);
        http->body_head = http->body_tail = NULL;
    }
    if (http->uri != NULL) {
        mln_string_free(http->uri);
        http->uri = NULL;
    }
    if (http->args != NULL) {
        mln_string_free(http->args);
        http->args = NULL;
    }
    if (http->response_msg != NULL) {
        mln_string_free(http->response_msg);
        http->response_msg = NULL;
    }
    http->error = M_HTTP_OK;
    http->status = M_HTTP_OK;
    http->method = 0;
    http->version = 0;
    http->type = M_HTTP_UNKNOWN;
    http->done = 0;
})

MLN_FUNC_VOID(static, void, mln_http_hash_free, (void *data), (data), {
    mln_string_free((mln_string_t *)data);
})

MLN_FUNC(static, mln_u64_t, mln_http_hash_calc, (mln_hash_t *h, void *key), (h, key), {
    mln_u64_t index = 0;
    mln_string_t *s = (mln_string_t *)key;
    mln_u8ptr_t p, end = s->data + s->len;

    for (p = s->data; p < end; ++p) {
        index += (((mln_u64_t)(*p)) * 3);
    }

    return index % h->len;
})

MLN_FUNC(static, int, mln_http_hash_cmp, \
         (mln_hash_t *h, void *key1, void *key2), (h, key1, key2), \
{
    return !mln_string_strcasecmp((mln_string_t *)key1, (mln_string_t *)key2);
})

/*
 * dump
 */
void mln_http_dump(mln_http_t *http)
{
    int rc = 1;
    printf("HTTP Dump:\n");
    if (http == NULL) return;

    if (http->uri != NULL) {
        printf("\tURI:[");fflush(stdout);
#if defined(MSVC)
        rc = write(_fileno(stdout), http->uri->data, http->uri->len);
#else
        rc = write(STDOUT_FILENO, http->uri->data, http->uri->len);
#endif
        printf("]\n");
    }
    if (http->args != NULL) {
        printf("\tARGS:[");fflush(stdout);
#if defined(MSVC)
        rc = write(_fileno(stdout), http->args->data, http->args->len);
#else
        rc = write(STDOUT_FILENO, http->args->data, http->args->len);
#endif
        printf("]\n");
    }
    if (http->response_msg != NULL) {
        printf("\tRESPONSE_MSG:[");fflush(stdout);
#if defined(MSVC)
        rc = write(_fileno(stdout), http->response_msg->data, http->response_msg->len);
#else
        rc = write(STDOUT_FILENO, http->response_msg->data, http->response_msg->len);
#endif
        printf("]\n");
    }
    printf("\tstatus_code:%u\n", http->status);
    printf("\tMethod_code:%u\n", http->method);
    printf("\tversion_code:%u\n", http->version);
    printf("\ttype_code:%u\n", http->type);
    printf("\tfields:\n");
    if (rc <= 0) rc = 1;/*do nothing*/
    mln_hash_iterate(http->header_fields, mln_http_dump_iterate_handler, NULL);
}

MLN_FUNC(static, int, mln_http_dump_iterate_handler, \
         (mln_hash_t *h, void *key, void *val, void *data), (h, key, val, data), \
{
    printf("\t\tkey:[%s] value:[%s]\n", \
           (char *)(((mln_string_t *)key)->data), \
           val == NULL?"NULL":(char *)(((mln_string_t *)val)->data));

    return 0;
})

