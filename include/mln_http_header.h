
/*
 * Copyright (C) Niklaus F.Schen.
 */
#ifndef __MLN_HTTP_HEADER_H
#define __MLN_HTTP_HEADER_H

#include "mln_types.h"
#include "mln_alloc.h"
#include "mln_chain.h"
#include "mln_connection.h"


#define M_HTTP_HEADER_DEFAULT_LINE_SIZE               1024

/*return value*/
#define M_HTTP_HEADER_RET_DONE                        1
#define M_HTTP_HEADER_RET_OK                          0
#define M_HTTP_HEADER_RET_ERROR                       -1

/*common unset*/
#define M_HTTP_HEADER_UNSET                           -1

/*request method*/
#define M_HTTP_HEADER_GET                             0
#define M_HTTP_HEADER_POST                            1
#define M_HTTP_HEADER_HEAD                            2
#define M_HTTP_HEADER_PUT                             3
#define M_HTTP_HEADER_DELETE                          4
#define M_HTTP_HEADER_TRACE                           5
#define M_HTTP_HEADER_CONNECT                         6
#define M_HTTP_HEADER_OPTIONS                         7

/*version*/
#define M_HTTP_HEADER_VERSION_1_0                     0
#define M_HTTP_HEADER_VERSION_1_1                     1

/*status*/
#define M_HTTP_HEADER_CONTINUE                        100
#define M_HTTP_HEADER_SWITCHING_PROTOCOLS             101
#define M_HTTP_HEADER_PROCESSING                      102
#define M_HTTP_HEADER_OK                              200
#define M_HTTP_HEADER_CREATED                         201
#define M_HTTP_HEADER_ACCEPTED                        202
#define M_HTTP_HEADER_NON_AUTHORITATIVE_INFORMATION   203
#define M_HTTP_HEADER_NO_CONTENT                      204
#define M_HTTP_HEADER_RESET_CONTENT                   205
#define M_HTTP_HEADER_PARTIAL_CONTENT                 206
#define M_HTTP_HEADER_MULTI_STATUS                    207
#define M_HTTP_HEADER_MULTIPLE_CHOICES                300
#define M_HTTP_HEADER_MOVED_PERMANENTLY               301
#define M_HTTP_HEADER_MOVED_TEMPORARILY               302
#define M_HTTP_HEADER_SEE_OTHER                       303
#define M_HTTP_HEADER_NOT_MODIFIED                    304
#define M_HTTP_HEADER_USE_PROXY                       305
#define M_HTTP_HEADER_SWITCH_PROXY                    306
#define M_HTTP_HEADER_TEMPORARY_REDIRECT              307
#define M_HTTP_HEADER_BAD_REQUEST                     400
#define M_HTTP_HEADER_UNAUTHORIZED                    401
#define M_HTTP_HEADER_PAYMENT_REQUIRED                402
#define M_HTTP_HEADER_FORBIDDEN                       403
#define M_HTTP_HEADER_NOT_FOUND                       404
#define M_HTTP_HEADER_METHOD_NOT_ALLOWED              405
#define M_HTTP_HEADER_NOT_ACCEPTABLE                  406
#define M_HTTP_HEADER_PROXY_AUTHENTICATION_REQUIRED   407
#define M_HTTP_HEADER_REQUEST_TIMEOUT                 408
#define M_HTTP_HEADER_CONFLICT                        409
#define M_HTTP_HEADER_GONE                            410
#define M_HTTP_HEADER_LENGTH_REQUIRED                 411
#define M_HTTP_HEADER_PRECONDITION_FAILED             412
#define M_HTTP_HEADER_REQUEST_ENTITY_TOO_LARGE        413
#define M_HTTP_HEADER_REQUEST_URI_TOO_LARGE           414
#define M_HTTP_HEADER_UNSUPPORTED_MEDIA_TYPE          415
#define M_HTTP_HEADER_REQUESTED_RANGE_NOT_SATISFIABLE 416
#define M_HTTP_HEADER_EXPECTATION_FAILED              417
#define M_HTTP_HEADER_TOO_MANY_CONNECTIONS            421
#define M_HTTP_HEADER_UNPROCESSABLE_ENTITY            422
#define M_HTTP_HEADER_LOCKED                          423
#define M_HTTP_HEADER_FAILED_DEPENDENCY               424
#define M_HTTP_HEADER_UNORDERED_COLLECTION            425
#define M_HTTP_HEADER_UPGRADE_REQUIRED                426
#define M_HTTP_HEADER_RETRY_WITH                      449
#define M_HTTP_HEADER_INTERNAL_SERVER_ERROR           500
#define M_HTTP_HEADER_NOT_IMPLEMENTED                 501
#define M_HTTP_HEADER_BAD_GATEWAY                     502
#define M_HTTP_HEADER_SERVICE_UNAVAILABLE             503
#define M_HTTP_HEADER_GATEWAY_TIMEOUT                 504
#define M_HTTP_HEADER_VERSION_NOT_SUPPORTED           505
#define M_HTTP_HEADER_VARIANT_ALSO_NEGOTIATES         506
#define M_HTTP_HEADER_INSUFFICIENT_STORAGE            507
#define M_HTTP_HEADER_BANDWIDTH_LIMIT_EXCEEDED        509
#define M_HTTP_HEADER_NOT_EXTENDED                    510
#define M_HTTP_HEADER_UNPARSEABLE_RESPONSE_HEADERS    600
/*fields*/
#define M_HTTP_HEADER_CACHE_CONTORL                   0
#define M_HTTP_HEADER_PRAGMA                          1
#define M_HTTP_HEADER_CONNECTION                      2
#define M_HTTP_HEADER_PROXY_CONNECTION                3
#define M_HTTP_HEADER_DATE                            4
#define M_HTTP_HEADER_TRANSFER_ENCODING               5
#define M_HTTP_HEADER_UPGRADE                         6
#define M_HTTP_HEADER_VIA                             7
#define M_HTTP_HEADER_KEEP_ALIVE                      8
#define M_HTTP_HEADER_ACCEPT                          9
#define M_HTTP_HEADER_ACCEPT_CHARSET                  10
#define M_HTTP_HEADER_ACCEPT_ENCODING                 11
#define M_HTTP_HEADER_ACCEPT_LANGUAGE                 12
#define M_HTTP_HEADER_AUTHORIZATION                   13
#define M_HTTP_HEADER_IF_MATCH                        14
#define M_HTTP_HEADER_IF_MODIFIED_SINCE               15
#define M_HTTP_HEADER_IF_UNMODIFIED_SINCE             16
#define M_HTTP_HEADER_IF_RANGE                        17
#define M_HTTP_HEADER_RANGE                           18
#define M_HTTP_HEADER_PROXY_AUTHENTICATE              19
#define M_HTTP_HEADER_PROXY_AUTHORIZATION             20
#define M_HTTP_HEADER_HOST                            21
#define M_HTTP_HEADER_REFERER                         22
#define M_HTTP_HEADER_USER_AGENT                      23
#define M_HTTP_HEADER_COOKIE                          24
#define M_HTTP_HEADER_UPGRADE_INSECURE_REQUESTS       25
#define M_HTTP_HEADER_AGE                             26
#define M_HTTP_HEADER_SERVER                          27
#define M_HTTP_HEADER_ACCEPT_RANGES                   28
#define M_HTTP_HEADER_VARY                            29
#define M_HTTP_HEADER_SET_COOKIE                      30
#define M_HTTP_HEADER_ALLOW                           31
#define M_HTTP_HEADER_LOCATION                        32
#define M_HTTP_HEADER_CONTENT_BASE                    33
#define M_HTTP_HEADER_CONTENT_ENCODING                34
#define M_HTTP_HEADER_CONTENT_LANGUAGE                35
#define M_HTTP_HEADER_CONTENT_LOCATION                36
#define M_HTTP_HEADER_CONTENT_MD5                     37
#define M_HTTP_HEADER_CONTENT_RANGE                   38
#define M_HTTP_HEADER_CONTENT_TYPE                    39
#define M_HTTP_HEADER_ETAG                            40
#define M_HTTP_HEADER_EXPIRES                         41
#define M_HTTP_HEADER_LAST_MODIFIED                   42
#define M_HTTP_HEADER_CONTENT_LENGTH                  43

#define M_HTTP_HEADER_NR_FIELDS                       44

typedef struct {
    mln_string_t                     *key;
    mln_string_t                     *value;
} mln_http_header_cookie_t;

typedef struct {
    mln_string_t                      type_string;
    mln_s32_t                         type_code;
} mln_http_header_map_t;

typedef struct mln_http_header_s mln_http_header_t;

typedef mln_chain_t *(*http_header)(mln_string_t *, mln_string_t **, mln_http_header_t *, void *);

struct mln_http_header_s {
    mln_alloc_t                      *pool;
    void                             *data;
    mln_string_t                    **value_map[M_HTTP_HEADER_NR_FIELDS];
    http_header                       handle_map[M_HTTP_HEADER_NR_FIELDS];
    mln_size_t                        header_line_size;
    mln_u32_t                         is_request:1;
    mln_u32_t                         is_notfirst:1;
    mln_u32_t                         field_done:1;
    mln_s32_t                         retval;

    mln_s16_t                         version;
    mln_s16_t                         method;
    mln_s32_t                         status;
    mln_string_t                     *uri;
    mln_string_t                     *args;

    mln_string_t                     *fields[M_HTTP_HEADER_NR_FIELDS];
};

extern void
mln_http_header_reset(mln_http_header_t *header, http_header *hooks) __NONNULL1(1);
extern mln_http_header_t *
mln_http_header_init(mln_alloc_t *pool) __NONNULL1(1);
extern void
mln_http_header_destroy(mln_http_header_t *header);
extern mln_chain_t *
mln_http_header_packup(mln_http_header_t *header) __NONNULL1(1);
extern mln_chain_t *
mln_http_header_parse(mln_http_header_t *header, mln_chain_t *in) __NONNULL1(1);

#define mln_http_header_get_pool(header) \
    ((header)->pool)
#define mln_http_header_set_pool(header,p) \
    (header)->pool = (p)

#define mln_http_header_get_data(header) \
    ((header)->data)
#define mln_http_header_set_data(header,pdata) \
    (header)->data = (pdata)

#define mln_http_header_get_value(header,key) \
    ((header)->value_map[key])
#define mln_http_header_set_value(header,key,val) \
    *((header)->value_map[key]) = (val)

#define mln_http_header_get_handle(header,key) \
    ((header)->handle_map[key])
#define mln_http_header_set_handle(header,key,handle) \
    (header)->handle_map[key] = (handle)

#define mln_http_header_get_max_size(header) \
    ((header)->header_line_size)

#define mln_http_header_request_set(header) \
    (header)->is_request = 1
#define mln_http_header_response_set(header) \
    (header)->is_request = 0
#define mln_http_header_is_request(header) \
    (header)->is_request

#define mln_http_header_is_nonfirst(header) \
    ((header)->is_notfirst)
#define mln_http_header_first_set(header) \
    (header)->is_notfirst = 0
#define mln_http_header_nonfirst_set(header) \
    (header)->is_notfirst = 1

#define mln_http_header_field_done(header) \
    ((header)->field_done)
#define mln_http_header_field_done_set(header) \
    (header)->field_done = 1
#define mln_http_header_field_done_reset(header) \
    (header)->field_done = 0

#define mln_http_header_get_retval(header) \
    (header)->retval
#define mln_http_header_set_retval(header,rv) \
    (header)->retval = (rv)

#define mln_http_header_set_uri(header,/*mln_string_t **/puri) \
    (header)->uri = (puri)
#define mln_http_header_get_uri(header) \
    (header)->uri

#define mln_http_header_set_args(header, /*mln_string_t **/pargs) \
    (header)->args = (pargs)
#define mln_http_header_get_args(header) \
    (header)->args

#define mln_http_header_set_version(header,val) \
    (header)->version = (val)
#define mln_http_header_get_version(header) \
    (header)->version

#define mln_http_header_set_status(header,val) \
    (header)->status = (val)
#define mln_http_header_get_status(header) \
    (header)->status

#define mln_http_header_set_method(header,val) \
    (header)->method = (val)
#define mln_http_header_get_method(header) \
    (header)->method

extern mln_http_header_cookie_t *
mln_http_header_cookie_parse(mln_http_header_t *header) __NONNULL1(1);
extern mln_http_header_cookie_t *
mln_http_header_setcookie_parse(mln_http_header_t *header) __NONNULL1(1);
extern void
mln_http_header_cookies_destroy(mln_http_header_cookie_t *cookies);
extern mln_string_t *
mln_http_header_cookie_packup(mln_http_header_t *header, \
                              mln_http_header_cookie_t *cookies) __NONNULL1(1);

#endif

