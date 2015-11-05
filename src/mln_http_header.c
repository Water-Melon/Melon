
/*
 * Copyright (C) Niklaus F.Schen.
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "mln_http_header.h"
#include "mln_conf.h"
#include "mln_log.h"

static inline mln_size_t mln_http_header_get_header_line_size(void);
static mln_chain_t *mln_http_header_packup_head(mln_http_header_t *header);
static mln_string_t *mln_http_header_map_status(mln_u32_t status_code);
static inline void
mln_http_header_process_field(mln_http_header_t *header, mln_u8ptr_t buf, mln_size_t length);
static inline mln_s16_t
mln_http_header_map_version(mln_string_t *version);
static inline mln_s16_t
mln_http_header_map_method(mln_string_t *method);
static inline void
mln_http_header_map_resource(mln_http_header_t *header, mln_string_t *resource);
static mln_http_header_cookie_t *
mln_http_header_cookie_parse_processor(mln_alloc_t *pool, mln_string_t *val);
static inline int
mln_http_header_map_field(mln_string_t *key);

char mln_http_header_domain[] = "http";
char mln_http_header_line_size_cmd[] = "header_line_size";

mln_string_t mln_http_header_fields[] = {
mln_string("Cache-Control"),
mln_string("Pragma"),
mln_string("Connection"),
mln_string("Proxy-Connection"),
mln_string("Date"),
mln_string("Transfer-Encoding"),
mln_string("Upgrade"),
mln_string("Via"),
mln_string("Keep-Alive"),
mln_string("Accept"),
mln_string("Accept-Charset"),
mln_string("Accept-Encoding"),
mln_string("Accept-Language"),
mln_string("Authorization"),
mln_string("If-Match"),
mln_string("If-Modified-Since"),
mln_string("If-Unmodified-Since"),
mln_string("If-Range"),
mln_string("Range"),
mln_string("Proxy-Authenticate"),
mln_string("Proxy-Authorization"),
mln_string("Host"),
mln_string("Referer"),
mln_string("User-Agent"),
mln_string("Cookie"),
mln_string("Upgrade-Insecure-Requests"),
mln_string("Age"),
mln_string("Server"),
mln_string("Accept-Ranges"),
mln_string("Vary"),
mln_string("Set-Cookie"),
mln_string("Allow"),
mln_string("Location"),
mln_string("Content-Base"),
mln_string("Content-Encoding"),
mln_string("Content-Language"),
mln_string("Content-Location"),
mln_string("Content-MD5"),
mln_string("Content-Range"),
mln_string("Content-Type"),
mln_string("Etag"),
mln_string("Expires"),
mln_string("Last-Modified"),
mln_string("Content-Length")
};

mln_http_header_map_t mln_http_header_method[] = {
{mln_string("GET"),     M_HTTP_HEADER_GET},
{mln_string("POST"),    M_HTTP_HEADER_POST},
{mln_string("HEAD"),    M_HTTP_HEADER_HEAD},
{mln_string("PUT"),     M_HTTP_HEADER_PUT},
{mln_string("DELETE"),  M_HTTP_HEADER_DELETE},
{mln_string("TRACE"),   M_HTTP_HEADER_TRACE},
{mln_string("CONNECT"), M_HTTP_HEADER_CONNECT},
{mln_string("OPTIONS"), M_HTTP_HEADER_OPTIONS}
};

mln_http_header_map_t mln_http_header_version[] = {
{mln_string("HTTP/1.0"),    M_HTTP_HEADER_VERSION_1_0},
{mln_string("HTTP/1.1"),    M_HTTP_HEADER_VERSION_1_1}
};

mln_http_header_map_t mln_http_header_status[] = {
{mln_string("Continue"),                        M_HTTP_HEADER_CONTINUE},
{mln_string("Switching Protocols"),             M_HTTP_HEADER_SWITCHING_PROTOCOLS},
{mln_string("Processing"),                      M_HTTP_HEADER_PROCESSING},
{mln_string("OK"),                              M_HTTP_HEADER_OK},
{mln_string("Created"),                         M_HTTP_HEADER_CREATED},
{mln_string("Accepted"),                        M_HTTP_HEADER_ACCEPTED},
{mln_string("Non-Authoritative Information"),   M_HTTP_HEADER_NON_AUTHORITATIVE_INFORMATION},
{mln_string("No Content"),                      M_HTTP_HEADER_NO_CONTENT},
{mln_string("Reset Content"),                   M_HTTP_HEADER_RESET_CONTENT},
{mln_string("Partial Content"),                 M_HTTP_HEADER_PARTIAL_CONTENT},
{mln_string("Multi-Status"),                    M_HTTP_HEADER_MULTI_STATUS},
{mln_string("Multiple Choices"),                M_HTTP_HEADER_MULTIPLE_CHOICES},
{mln_string("Moved Permanently"),               M_HTTP_HEADER_MOVED_PERMANENTLY},
{mln_string("Move temporarily"),                M_HTTP_HEADER_MOVED_TEMPORARILY},
{mln_string("See Other"),                       M_HTTP_HEADER_SEE_OTHER},
{mln_string("Not Modified"),                    M_HTTP_HEADER_NOT_MODIFIED},
{mln_string("Use Proxy"),                       M_HTTP_HEADER_USE_PROXY},
{mln_string("Switch Proxy"),                    M_HTTP_HEADER_SWITCH_PROXY},
{mln_string("Temporary Redirect"),              M_HTTP_HEADER_TEMPORARY_REDIRECT},
{mln_string("Bad Request"),                     M_HTTP_HEADER_BAD_REQUEST},
{mln_string("Unauthorized"),                    M_HTTP_HEADER_UNAUTHORIZED},
{mln_string("Payment Required"),                M_HTTP_HEADER_PAYMENT_REQUIRED},
{mln_string("Forbidden"),                       M_HTTP_HEADER_FORBIDDEN},
{mln_string("Not Found"),                       M_HTTP_HEADER_NOT_FOUND},
{mln_string("Method Not Allowed"),              M_HTTP_HEADER_METHOD_NOT_ALLOWED},
{mln_string("Not Acceptable"),                  M_HTTP_HEADER_NOT_ACCEPTABLE},
{mln_string("Proxy Authentication Required"),   M_HTTP_HEADER_PROXY_AUTHENTICATION_REQUIRED},
{mln_string("Request Timeout"),                 M_HTTP_HEADER_REQUEST_TIMEOUT},
{mln_string("Conflict"),                        M_HTTP_HEADER_CONFLICT},
{mln_string("Gone"),                            M_HTTP_HEADER_GONE},
{mln_string("Length Required"),                 M_HTTP_HEADER_LENGTH_REQUIRED},
{mln_string("Precondition Failed"),             M_HTTP_HEADER_PRECONDITION_FAILED},
{mln_string("Request Entity Too Large"),        M_HTTP_HEADER_REQUEST_ENTITY_TOO_LARGE},
{mln_string("Request-URI Too Long"),            M_HTTP_HEADER_REQUEST_URI_TOO_LARGE},
{mln_string("Unsupported Media Type"),          M_HTTP_HEADER_UNSUPPORTED_MEDIA_TYPE},
{mln_string("Requested Range Not Satisfiable"), M_HTTP_HEADER_REQUESTED_RANGE_NOT_SATISFIABLE},
{mln_string("Expectation Failed"),              M_HTTP_HEADER_EXPECTATION_FAILED},
{mln_string("There are too many connections from your internet address"), \
                                                M_HTTP_HEADER_TOO_MANY_CONNECTIONS},
{mln_string("Unprocessable Entity"),            M_HTTP_HEADER_UNPROCESSABLE_ENTITY},
{mln_string("Locked"),                          M_HTTP_HEADER_LOCKED},
{mln_string("Failed Dependency"),               M_HTTP_HEADER_FAILED_DEPENDENCY},
{mln_string("Unordered Collection"),            M_HTTP_HEADER_UNORDERED_COLLECTION},
{mln_string("Upgrade Required"),                M_HTTP_HEADER_UPGRADE_REQUIRED},
{mln_string("Retry With"),                      M_HTTP_HEADER_RETRY_WITH},
{mln_string("Internal Server Error"),           M_HTTP_HEADER_INTERNAL_SERVER_ERROR},
{mln_string("Not Implemented"),                 M_HTTP_HEADER_NOT_IMPLEMENTED},
{mln_string("Bad Gateway"),                     M_HTTP_HEADER_BAD_GATEWAY},
{mln_string("Service Unavailable"),             M_HTTP_HEADER_SERVICE_UNAVAILABLE},
{mln_string("Gateway Timeout"),                 M_HTTP_HEADER_GATEWAY_TIMEOUT},
{mln_string("HTTP Version Not Supported"),      M_HTTP_HEADER_VERSION_NOT_SUPPORTED},
{mln_string("Variant Also Negotiates"),         M_HTTP_HEADER_VARIANT_ALSO_NEGOTIATES},
{mln_string("Insufficient Storage"),            M_HTTP_HEADER_INSUFFICIENT_STORAGE},
{mln_string("Bandwidth Limit Exceeded"),        M_HTTP_HEADER_BANDWIDTH_LIMIT_EXCEEDED},
{mln_string("Not Extended"),                    M_HTTP_HEADER_NOT_EXTENDED},
{mln_string("Unparseable Response Headers"),    M_HTTP_HEADER_UNPARSEABLE_RESPONSE_HEADERS}
};

mln_http_header_t *
mln_http_header_init(mln_alloc_t *pool)
{
    int i;
    mln_http_header_t *header;

    header = (mln_http_header_t *)mln_alloc_m(pool, sizeof(mln_http_header_t));
    if (header == NULL) return NULL;

    header->pool = pool;
    header->data = NULL;

    header->header_line_size = mln_http_header_get_header_line_size();
    header->is_request = 0;
    header->is_notfirst = 0;
    header->field_done = 0;
    header->retval = M_HTTP_HEADER_RET_OK;

    header->uri = NULL;
    header->args = NULL;
    header->version = M_HTTP_HEADER_UNSET;
    header->method = M_HTTP_HEADER_UNSET;
    header->status = M_HTTP_HEADER_UNSET;

    for (i = 0; i < M_HTTP_HEADER_NR_FIELDS; i++) {
        header->fields[i] = NULL;
        header->value_map[i] = &(header->fields[i]);
        header->handle_map[i] = NULL;
    }

    return header;
}

static inline mln_size_t
mln_http_header_get_header_line_size(void)
{
    mln_conf_t *cf = mln_get_conf();
    if (cf != NULL) {
        mln_conf_domain_t *cd = cf->search(cf, mln_http_header_domain);
        if (cd != NULL) {
            mln_conf_cmd_t *cc = cd->search(cd, mln_http_header_line_size_cmd);
            if (cc != NULL) {
                mln_conf_item_t *ci = cc->search(cc, 1);
                if (ci->type != CONF_INT) {
                    mln_log(error, "Configuration error. Invalid type of command '%s' in domain '%s'.\n", \
                            mln_http_header_line_size_cmd, mln_http_header_domain);
                    exit(1);
                }
                if (ci->val.i <= 0) {
                    mln_log(error, "Configuration error. Invalid value of command '%s' in domain '%s'.\n", \
                            mln_http_header_line_size_cmd, mln_http_header_domain);
                    exit(1);
                }
                return ci->val.i;
            }
        }
    }
    return M_HTTP_HEADER_DEFAULT_LINE_SIZE;
}

void mln_http_header_destroy(mln_http_header_t *header)
{
    if (header == NULL) return;

    mln_alloc_free(header);
}

mln_chain_t *mln_http_header_packup(mln_http_header_t *header)
{
    mln_alloc_t *pool = mln_http_header_get_pool(header);
    mln_chain_t *out_head = NULL, *out_tail = NULL;
    mln_chain_t *c;
    mln_buf_t *b;
    mln_string_t **val;
    mln_u8ptr_t buf;
    http_header handle;
    mln_size_t i;

    c = mln_http_header_packup_head(header);
    if (mln_http_header_get_retval(header) != M_HTTP_HEADER_RET_OK) {
        return NULL;
    }
    if (c != NULL) {
        mln_chain_add(&out_head, &out_tail, c);
    }

    for (i = 0; i < M_HTTP_HEADER_NR_FIELDS; i++) {
        val = mln_http_header_get_value(header, i);
        handle = mln_http_header_get_handle(header, i);
        if (handle != NULL) {
            c = handle(&mln_http_header_fields[i], val, header, header->data);
            if (mln_http_header_get_retval(header) != M_HTTP_HEADER_RET_OK) {
                mln_chain_pool_release_all(out_head);
                return NULL;
            }
            if (c != NULL) {
                mln_chain_add(&out_head, &out_tail, c);
            }
        }
    }

    buf = (mln_u8ptr_t)mln_alloc_m(pool, 2);
    b = mln_buf_new(pool);
    c = mln_chain_new(pool);
    if (buf == NULL || b == NULL || c == NULL) {
        mln_chain_pool_release_all(out_head);
        mln_http_header_set_retval(header, M_HTTP_HEADER_RET_ERROR);
        return NULL;
    }
    c->buf = b;
    b->send_pos = b->pos = b->start = buf;
    b->last = b->end = buf + 2;
    b->in_memory = 1;
    b->last_buf = 1;
    buf[0] = '\r';
    buf[1] = '\n';
    mln_chain_add(&out_head, &out_tail, c);

    mln_http_header_set_retval(header, M_HTTP_HEADER_RET_OK);
    return out_head;
}

static mln_chain_t *mln_http_header_packup_head(mln_http_header_t *header)
{
    mln_chain_t *c;
    mln_buf_t *b;
    mln_u8ptr_t buf;
    mln_size_t len = 0, n = 0;
    mln_http_header_map_t *method = NULL;
    mln_http_header_map_t *ver = NULL;
    mln_alloc_t *pool = mln_http_header_get_pool(header);
    mln_string_t *status;
    char status_str[64] = {0};

    switch (mln_http_header_get_version(header)) {
        case M_HTTP_HEADER_VERSION_1_0:
            ver = &mln_http_header_version[M_HTTP_HEADER_VERSION_1_0];
            break;
        case M_HTTP_HEADER_VERSION_1_1:
            ver = &mln_http_header_version[M_HTTP_HEADER_VERSION_1_1];
            break;
        default:
            mln_http_header_set_retval(header, M_HTTP_HEADER_RET_ERROR);
            return NULL;
    }

    if (mln_http_header_is_request(header)) {
        switch (mln_http_header_get_method(header)) {
            case M_HTTP_HEADER_GET:
                method = &mln_http_header_method[M_HTTP_HEADER_GET];
                break;
            case M_HTTP_HEADER_POST:
                method = &mln_http_header_method[M_HTTP_HEADER_POST];
                break;
            case M_HTTP_HEADER_HEAD:
                method = &mln_http_header_method[M_HTTP_HEADER_HEAD];
                break;
            case M_HTTP_HEADER_PUT:
                method = &mln_http_header_method[M_HTTP_HEADER_PUT];
                break;
            case M_HTTP_HEADER_DELETE:
                method = &mln_http_header_method[M_HTTP_HEADER_DELETE];
                break;
            case M_HTTP_HEADER_TRACE:
                method = &mln_http_header_method[M_HTTP_HEADER_TRACE];
                break;
            case M_HTTP_HEADER_CONNECT:
                method = &mln_http_header_method[M_HTTP_HEADER_CONNECT];
                break;
            case M_HTTP_HEADER_OPTIONS:
                method = &mln_http_header_method[M_HTTP_HEADER_OPTIONS];
                break;
            default:
                mln_http_header_set_retval(header, M_HTTP_HEADER_RET_ERROR);
                return NULL;
        }

        len = method->type_string.len;

        if (header->uri == NULL) {
            len += 1;
        } else {
            len += header->uri->len;
        }
        if (header->args != NULL) {
            len += header->args->len;
        }
        len += (ver->type_string.len + 4);

        buf = (mln_u8ptr_t)mln_alloc_m(pool, len);
        b = mln_buf_new(pool);
        c = mln_chain_new(pool);
        if (buf == NULL || b == NULL || c == NULL) {
            mln_http_header_set_retval(header, M_HTTP_HEADER_RET_ERROR);
            return NULL;
        }

        memcpy(buf+n, method->type_string.str, method->type_string.len);
        n += method->type_string.len;
        buf[n++] = ' ';
        memcpy(buf+n, \
               header->uri==NULL? (mln_s8ptr_t)"/": header->uri->str, header->uri==NULL? 1: header->uri->len);
        n += (header->uri==NULL? 1: header->uri->len);
        if (header->args != NULL) {
            memcpy(buf+n, header->args->str, header->args->len);
            n += header->args->len;
        }
        buf[n++] = ' ';
        memcpy(buf+n, ver->type_string.str, ver->type_string.len);
        n += ver->type_string.len;
        buf[n++] = '\r';
        buf[n++] = '\n';

        c->buf = b;
        b->send_pos = b->pos = b->start = buf;
        b->last = b->end = buf + n;
        b->in_memory = 1;
        b->last_buf = 1;

        return c;
    }

    status = mln_http_header_map_status(mln_http_header_get_status(header));
    if (status == NULL) {
        mln_http_header_set_retval(header, M_HTTP_HEADER_RET_ERROR);
        return NULL;
    }

    n = snprintf(status_str, sizeof(status_str)-1, "%u", \
                 mln_http_header_get_status(header));
    len = ver->type_string.len + n + status->len + 4;

    buf = (mln_u8ptr_t)mln_alloc_m(pool, len);
    b = mln_buf_new(pool);
    c = mln_chain_new(pool);
    if (buf == NULL || b == NULL || c == NULL) {
        mln_http_header_set_retval(header, M_HTTP_HEADER_RET_ERROR);
        return NULL;
    }

    memcpy(buf, ver->type_string.str, ver->type_string.len);
    buf[ver->type_string.len] = ' ';
    memcpy(buf + ver->type_string.len + 1, status_str, n);
    n += (ver->type_string.len + 1);
    buf[n++] = ' ';
    memcpy(buf + n, status->str, status->len);
    n += status->len;
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

static mln_string_t *mln_http_header_map_status(mln_u32_t status_code)
{
    mln_http_header_map_t *st, *stend;

    st = mln_http_header_status;
    stend = mln_http_header_status + \
                sizeof(mln_http_header_status)/sizeof(mln_http_header_map_t);

    for (; st < stend; st++) {
        if (status_code == st->type_code)
            return &st->type_string;
    }

    return NULL;
}

mln_chain_t *mln_http_header_parse(mln_http_header_t *header, mln_chain_t *in)
{
    int not_done = 1, ready = 0;
    mln_chain_t *c = in, *save_c = NULL;
    mln_buf_t *b;
    mln_u8ptr_t p, save_pos = NULL;
    mln_u8ptr_t buf = NULL;
    mln_size_t header_size;

    header_size = mln_http_header_get_max_size(header);

    buf = (mln_u8ptr_t)mln_alloc_m(mln_http_header_get_pool(header), header_size);
    if (buf == NULL) {
        mln_http_header_set_retval(header, M_HTTP_HEADER_RET_ERROR);
        return c;
    }
    p = buf;

    while (c != NULL && not_done) {
        b = c->buf;
        if (b == NULL) {
            c = c->next;
            continue;
        }
        if (!b->in_memory) {
            mln_http_header_set_retval(header, M_HTTP_HEADER_RET_ERROR);
            mln_alloc_free(buf);
            return c;
        }

        if (b->send_pos == NULL) {
            b->send_pos = b->pos;
        }
        for (; b->send_pos < b->last; b->send_pos++) {
            if (*(b->send_pos) == '\r') {
                ready = 1;
                continue;
            }
            if (*(b->send_pos) == '\n') {
                save_c = c;
                save_pos = b->send_pos + 1;
                if (mln_http_header_field_done(header)) {
                    b->send_pos++;
                    not_done = 0;
                    break;
                }
                mln_http_header_field_done_set(header);
                mln_http_header_process_field(header, buf, p - buf);
                if (mln_http_header_get_retval(header) != M_HTTP_HEADER_RET_OK) {
                    mln_alloc_free(buf);
                    mln_http_header_set_retval(header, M_HTTP_HEADER_RET_ERROR);
                    return c;
                }
                memset(buf, 0, p - buf);
                p = buf;
                ready = 0;
                continue;
            }
            if (ready == 1) {
                if (p >= buf + header_size - 1) {
                    mln_http_header_set_retval(header, M_HTTP_HEADER_RET_ERROR);
                    return c;
                }
                *p++ = '\r';
                ready = 0;
            }
            if (p >= buf + header_size) {
                mln_http_header_set_retval(header, M_HTTP_HEADER_RET_ERROR);
                return c;
            }
            *p++ = *(b->send_pos);
            mln_http_header_field_done_reset(header);
        }
        if (b->send_pos >= b->last) {
            c = c->next;
            continue;
        }
    }

    if (!not_done) {
        mln_alloc_free(buf);
        mln_http_header_set_retval(header, M_HTTP_HEADER_RET_DONE);
        return c;
    }

    if (save_c == NULL) {
        mln_alloc_free(buf);
        mln_http_header_set_retval(header, M_HTTP_HEADER_RET_OK);
        return c;
    }
    save_c->buf->send_pos = save_pos;
    mln_alloc_free(buf);
    mln_http_header_set_retval(header, M_HTTP_HEADER_RET_OK);
    return save_c;
}

static inline void
mln_http_header_process_field(mln_http_header_t *header, mln_u8ptr_t buf, mln_size_t length)
{
    mln_string_t key, val, *value, **pstore;
    mln_u8ptr_t p = buf, pend = buf + length;
    http_header handle;
    mln_s16_t sval16;
    int index;

    if (!mln_http_header_is_nonfirst(header)) {
        mln_string_t line, *parts = NULL;

        line.str = (mln_s8ptr_t)buf;
        line.len = length;
        line.is_referred = 1;

        parts = mln_slice(&line, " \r\n");
        if (parts == NULL || \
            parts[0].str == NULL || \
            parts[1].str == NULL || \
            parts[2].str == NULL)
        {
            mln_http_header_set_retval(header, M_HTTP_HEADER_RET_ERROR);
            return;
        }

        sval16 = mln_http_header_map_version(&parts[0]);
        if (sval16 != M_HTTP_HEADER_UNSET) {
            mln_http_header_response_set(header);
            mln_http_header_set_version(header, sval16);
            sval16 = atoi(parts[1].str);
            mln_http_header_set_status(header, sval16);
        } else {
            mln_http_header_request_set(header);
            sval16 = mln_http_header_map_method(&parts[0]);
            if (sval16 == M_HTTP_HEADER_UNSET) {
                mln_http_header_set_retval(header, M_HTTP_HEADER_RET_ERROR);
                mln_slice_free(parts);
                return;
            }
            mln_http_header_set_method(header, sval16);

            mln_http_header_map_resource(header, &parts[1]);
            if (mln_http_header_get_retval(header) != M_HTTP_HEADER_RET_OK) {
                return;
            }

            sval16 = mln_http_header_map_version(&parts[2]);
            if (sval16 == M_HTTP_HEADER_UNSET) {
                mln_http_header_set_retval(header, M_HTTP_HEADER_RET_ERROR);
                mln_slice_free(parts);
                return;
            }
            mln_http_header_set_version(header, sval16);
        }

        mln_slice_free(parts);
        mln_http_header_nonfirst_set(header);
        return;
    }

    for (; p < pend; p++) {
        if (*p == ':') {
            break;
        }
    }

    if (p >= pend) {
        mln_http_header_set_retval(header, M_HTTP_HEADER_RET_ERROR);
        return;
    }

    key.str = (mln_s8ptr_t)buf;
    key.len = p - buf;
    key.is_referred = 1;
    val.str = *(p+1) == ' '? (mln_s8ptr_t)(p + 2): (mln_s8ptr_t)(p + 1);
    val.len = *(p+1) == ' '? pend - (p + 2): pend - (p + 1);
    val.is_referred = 1;

    index = mln_http_header_map_field(&key);
    if (index < 0) {
        mln_http_header_set_retval(header, M_HTTP_HEADER_RET_ERROR);
        return;
    }

    value = mln_dup_string_pool(mln_http_header_get_pool(header), &val);
    if (value == NULL) {
        mln_http_header_set_retval(header, M_HTTP_HEADER_RET_ERROR);
        return;
    }

    pstore = mln_http_header_get_value(header, index);
    if (pstore == NULL) {
        mln_http_header_set_retval(header, M_HTTP_HEADER_RET_ERROR);
        mln_free_string_pool(value);
        return;
    }
    *pstore = value;
    handle = mln_http_header_get_handle(header, index);
    if (handle != NULL) {
        (void)handle(&key, pstore, header, header->data);
        if (mln_http_header_get_retval(header) != M_HTTP_HEADER_RET_OK) {
            mln_http_header_set_retval(header, M_HTTP_HEADER_RET_ERROR);
            return;
        }
    }
    mln_http_header_set_retval(header, M_HTTP_HEADER_RET_OK);
}

static inline int
mln_http_header_map_field(mln_string_t *key)
{
    int i;

    for (i = 0; i < M_HTTP_HEADER_NR_FIELDS; i++) {
        if (!mln_strcmp(key, &mln_http_header_fields[i])) {
            return i;
        }
    }

    return -1;
}

static inline mln_s16_t
mln_http_header_map_version(mln_string_t *version)
{
    mln_http_header_map_t *map, *end;
    end = mln_http_header_version + sizeof(mln_http_header_version)/sizeof(mln_http_header_map_t);

    for (map = mln_http_header_version; map < end; map++) {
        if (!mln_strcmp(version, &(map->type_string))) {
            return map->type_code;
        }
    }

    return M_HTTP_HEADER_UNSET;
}

static inline mln_s16_t
mln_http_header_map_method(mln_string_t *method)
{
    mln_http_header_map_t *map, *end;
    end = mln_http_header_version + sizeof(mln_http_header_method)/sizeof(mln_http_header_map_t);

    for (map = mln_http_header_method; map < end; map++) {
        if (!mln_strcmp(method, &(map->type_string))) {
            return map->type_code;
        }
    }

    return M_HTTP_HEADER_UNSET;
}

static inline void
mln_http_header_map_resource(mln_http_header_t *header, mln_string_t *resource)
{
    mln_string_t *parts, *uri, *args = NULL;
    mln_alloc_t *pool = mln_http_header_get_pool(header);

    parts = mln_slice(resource, "?");
    uri = mln_dup_string_pool(pool, &parts[0]);
    if (uri == NULL) {
        mln_http_header_set_retval(header, M_HTTP_HEADER_RET_ERROR);
        return;
    }
    if (parts[1].str != NULL) {
        args = mln_dup_string_pool(pool, &parts[1]);
        if (args == NULL) {
            mln_free_string_pool(uri);
            mln_http_header_set_retval(header, M_HTTP_HEADER_RET_ERROR);
            return;
        }
    }

    mln_slice_free(parts);
    mln_http_header_set_uri(header, uri);
    mln_http_header_set_args(header, args);
}

void mln_http_header_reset(mln_http_header_t *header, http_header *hooks)
{
    int i;
    mln_string_t **s;
    header->is_request = 0;
    mln_http_header_first_set(header);
    header->field_done = 0;
    header->retval = M_HTTP_HEADER_RET_OK;
    header->version = M_HTTP_HEADER_UNSET;
    header->method = M_HTTP_HEADER_UNSET;
    header->status = M_HTTP_HEADER_UNSET;
    header->uri = NULL;
    header->args = NULL;

    for (i = 0; i < M_HTTP_HEADER_NR_FIELDS; i++) {
        s = mln_http_header_get_value(header, i);
        *s = NULL;
        if (hooks != NULL) {
            header->handle_map[i] = hooks[i];
        } else {
            header->handle_map[i] = NULL;
        }
    }
}

/*
 * cookies
 */
mln_http_header_cookie_t *mln_http_header_cookie_parse(mln_http_header_t *header)
{
    mln_string_t **val = mln_http_header_get_value(header, M_HTTP_HEADER_COOKIE);
    mln_alloc_t *pool = mln_http_header_get_pool(header);
    mln_http_header_cookie_t *ret = NULL;

    if (val == NULL || (*val) == NULL) {
        mln_http_header_set_retval(header, M_HTTP_HEADER_RET_ERROR);
        return NULL;
    }

    ret = mln_http_header_cookie_parse_processor(pool, *val);
    if (ret == NULL) {
        mln_http_header_set_retval(header, M_HTTP_HEADER_RET_ERROR);
        return NULL;
    }
    return ret;
}

mln_http_header_cookie_t *mln_http_header_setcookie_parse(mln_http_header_t *header)
{
    mln_string_t **val = mln_http_header_get_value(header, M_HTTP_HEADER_SET_COOKIE);
    mln_alloc_t *pool = mln_http_header_get_pool(header);
    mln_http_header_cookie_t *ret = NULL;

    if (val == NULL || (*val) == NULL) {
        mln_http_header_set_retval(header, M_HTTP_HEADER_RET_ERROR);
        return NULL;
    }

    ret = mln_http_header_cookie_parse_processor(pool, *val);
    if (ret == NULL) {
        mln_http_header_set_retval(header, M_HTTP_HEADER_RET_ERROR);
        return NULL;
    }
    return ret;
}

static mln_http_header_cookie_t *
mln_http_header_cookie_parse_processor(mln_alloc_t *pool, mln_string_t *val)
{
    mln_u32_t nr_cookies = 0, i = 0;
    mln_string_t *copy = mln_dup_string_pool(pool, val);
    mln_string_t *scan, *array, *kv;
    mln_http_header_cookie_t *cookies;
    if (copy == NULL) return NULL;

    array = mln_slice(copy, "; ");
    if (array == NULL) {
        mln_free_string_pool(copy);
        return NULL;
    }

    for (scan = array; scan->str != NULL; scan++) {
         nr_cookies++;
    }

    cookies = (mln_http_header_cookie_t *)mln_alloc_c(pool, \
                  (nr_cookies+1)*sizeof(mln_http_header_cookie_t));
    if (cookies == NULL) {
        mln_slice_free(array);
        mln_free_string_pool(copy);
        return NULL;
    }

    for (scan = array; scan->str != NULL; scan++) {
        kv = mln_slice(scan, "=");
        if (kv == NULL) {
            mln_http_header_cookies_destroy(cookies);
            mln_slice_free(array);
            mln_free_string_pool(copy);
            return NULL;
        }

        if (kv[0].str == NULL || kv[1].str == NULL || kv[2].str != NULL) {
            mln_slice_free(kv);
            mln_http_header_cookies_destroy(cookies);
            mln_slice_free(array);
            mln_free_string_pool(copy);
            return NULL;
        }
        cookies[i].key = mln_dup_string_pool(pool, &kv[0]);
        cookies[i].value = mln_dup_string_pool(pool, &kv[1]);
        if (cookies[i].key == NULL || cookies[i].value == NULL) {
            mln_slice_free(kv);
            mln_http_header_cookies_destroy(cookies);
            mln_slice_free(array);
            mln_free_string_pool(copy);
            return NULL;
        }

        mln_slice_free(kv);
        i++;
    }

    mln_slice_free(array);
    mln_free_string_pool(copy);
    return cookies;
}

void mln_http_header_cookies_destroy(mln_http_header_cookie_t *cookies)
{
    if (cookies == NULL) return;

    mln_http_header_cookie_t *scan;
    int _break = 0;

    for (scan = cookies; !_break; scan++) {
        if (scan->key == NULL) {
            _break = 1;
        } else {
            mln_free_string_pool(scan->key);
        }
        if (scan->value != NULL) {
            mln_free_string_pool(scan->value);
        }
    }

    mln_alloc_free(cookies);
}

mln_string_t *mln_http_header_cookie_packup(mln_http_header_t *header, \
                                            mln_http_header_cookie_t *cookies)
{
    if (cookies == NULL) return NULL;

    mln_size_t length = 0, n = 0;
    mln_http_header_cookie_t *scan;
    mln_alloc_t *pool = mln_http_header_get_pool(header);
    mln_string_t *c;
    mln_u8ptr_t buf;

    for (scan = cookies; scan->key != NULL; scan++) {
        if (scan->value == NULL) {
            mln_http_header_set_retval(header, M_HTTP_HEADER_RET_ERROR);
            return NULL;
        }
        length += (scan->key->len + scan->value->len + 3);
    }
    if (!length) return NULL;

    c = (mln_string_t *)mln_alloc_m(pool, sizeof(mln_string_t));
    if (c == NULL) {
        mln_http_header_set_retval(header, M_HTTP_HEADER_RET_ERROR);
        return NULL;
    }
    buf = (mln_u8ptr_t)mln_alloc_m(pool, length);
    if (buf == NULL) {
        mln_alloc_free(c);
        return NULL;
    }

    for (scan = cookies; scan->key != NULL; scan++) {
        memcpy(buf + n, scan->key->str, scan->key->len);
        n += scan->key->len;
        buf[n++] = '=';
        memcpy(buf + n, scan->value->str, scan->value->len);
        n += scan->value->len;
        buf[n++] = ';';
        buf[n++] = ' ';
    }
    buf[--n] = 0;
    buf[--n] = 0;
    c->str = (mln_s8ptr_t)buf;
    c->len = n;

    return c;
}

