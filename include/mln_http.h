
/*
 * Copyright (C) Niklaus F.Schen.
 */
#ifndef __MLN_HTTP_H
#define __MLN_HTTP_H

#include "mln_connection.h"
#include "mln_hash.h"
#include "mln_string.h"
#include "mln_alloc.h"

#define M_HTTP_HASH_LEN                        31
#define M_HTTP_GENERATE_ALLOC_SIZE             1024

/*http type*/
#define M_HTTP_UNKNOWN                         0
#define M_HTTP_REQUEST                         1
#define M_HTTP_RESPONSE                        2
/*return value*/
#define M_HTTP_RET_OK                          0
#define M_HTTP_RET_DONE                        1
#define M_HTTP_RET_ERROR                       2

/*request method*/
#define M_HTTP_GET                             0
#define M_HTTP_POST                            1
#define M_HTTP_HEAD                            2
#define M_HTTP_PUT                             3
#define M_HTTP_DELETE                          4
#define M_HTTP_TRACE                           5
#define M_HTTP_CONNECT                         6
#define M_HTTP_OPTIONS                         7

/*version*/
#define M_HTTP_VERSION_1_0                     0
#define M_HTTP_VERSION_1_1                     1

/*status*/
#define M_HTTP_CONTINUE                        100
#define M_HTTP_SWITCHING_PROTOCOLS             101
#define M_HTTP_PROCESSING                      102
#define M_HTTP_OK                              200
#define M_HTTP_CREATED                         201
#define M_HTTP_ACCEPTED                        202
#define M_HTTP_NON_AUTHORITATIVE_INFORMATION   203
#define M_HTTP_NO_CONTENT                      204
#define M_HTTP_RESET_CONTENT                   205
#define M_HTTP_PARTIAL_CONTENT                 206
#define M_HTTP_MULTI_STATUS                    207
#define M_HTTP_MULTIPLE_CHOICES                300
#define M_HTTP_MOVED_PERMANENTLY               301
#define M_HTTP_MOVED_TEMPORARILY               302
#define M_HTTP_SEE_OTHER                       303
#define M_HTTP_NOT_MODIFIED                    304
#define M_HTTP_USE_PROXY                       305
#define M_HTTP_SWITCH_PROXY                    306
#define M_HTTP_TEMPORARY_REDIRECT              307
#define M_HTTP_BAD_REQUEST                     400
#define M_HTTP_UNAUTHORIZED                    401
#define M_HTTP_PAYMENT_REQUIRED                402
#define M_HTTP_FORBIDDEN                       403
#define M_HTTP_NOT_FOUND                       404
#define M_HTTP_METHOD_NOT_ALLOWED              405
#define M_HTTP_NOT_ACCEPTABLE                  406
#define M_HTTP_PROXY_AUTHENTICATION_REQUIRED   407
#define M_HTTP_REQUEST_TIMEOUT                 408
#define M_HTTP_CONFLICT                        409
#define M_HTTP_GONE                            410
#define M_HTTP_LENGTH_REQUIRED                 411
#define M_HTTP_PRECONDITION_FAILED             412
#define M_HTTP_REQUEST_ENTITY_TOO_LARGE        413
#define M_HTTP_REQUEST_URI_TOO_LARGE           414
#define M_HTTP_UNSUPPORTED_MEDIA_TYPE          415
#define M_HTTP_REQUESTED_RANGE_NOT_SATISFIABLE 416
#define M_HTTP_EXPECTATION_FAILED              417
#define M_HTTP_TOO_MANY_CONNECTIONS            421
#define M_HTTP_UNPROCESSABLE_ENTITY            422
#define M_HTTP_LOCKED                          423
#define M_HTTP_FAILED_DEPENDENCY               424
#define M_HTTP_UNORDERED_COLLECTION            425
#define M_HTTP_UPGRADE_REQUIRED                426
#define M_HTTP_RETRY_WITH                      449
#define M_HTTP_INTERNAL_SERVER_ERROR           500
#define M_HTTP_NOT_IMPLEMENTED                 501
#define M_HTTP_BAD_GATEWAY                     502
#define M_HTTP_SERVICE_UNAVAILABLE             503
#define M_HTTP_GATEWAY_TIMEOUT                 504
#define M_HTTP_VERSION_NOT_SUPPORTED           505
#define M_HTTP_VARIANT_ALSO_NEGOTIATES         506
#define M_HTTP_INSUFFICIENT_STORAGE            507
#define M_HTTP_BANDWIDTH_LIMIT_EXCEEDED        509
#define M_HTTP_NOT_EXTENDED                    510
#define M_HTTP_UNPARSEABLE_RESPONSE_HEADERS    600

typedef struct mln_http_s mln_http_t;
typedef int (*mln_http_handler)(mln_http_t *, mln_chain_t **, mln_chain_t **);

typedef struct {
    mln_string_t            msg_str;
    mln_string_t            code_str;
    mln_u32_t               code;
} mln_http_map_t;

struct mln_http_s {
    mln_tcp_conn_t         *connection;
    mln_alloc_t            *pool;
    mln_hash_t             *header_fields;
    mln_chain_t            *body_head;
    mln_chain_t            *body_tail;
    mln_http_handler        body_handler;
    void                   *data;
    mln_string_t           *uri;
    mln_string_t           *args;
    mln_string_t           *response_msg;
    mln_u32_t               error;
    mln_u32_t               status;
    mln_u32_t               method;
    mln_u32_t               version;
    mln_u32_t               type:2;
    mln_u32_t               done:1;
};

/*for internal*/
#define mln_http_done_get(h)             ((h)->done)
#define mln_http_done_set(h,hd)          (h)->done = (hd)

/*for external*/
#define mln_http_connection_get(h)       ((h)->connection)
#define mln_http_connection_set(h,c)     (h)->connection = (c)
#define mln_http_pool_get(h)             ((h)->pool)
#define mln_http_pool_set(h,p)           (h)->pool = (p)
#define mln_http_data_get(h)             ((h)->data)
#define mln_http_data_set(h,d)           (h)->data = (d)
#define mln_http_uri_get(h)              ((h)->uri)
#define mln_http_uri_set(h,u)            (h)->uri = (u)
#define mln_http_args_get(h)             ((h)->args)
#define mln_http_args_set(h,a)           (h)->args = (a)
#define mln_http_status_get(h)           ((h)->status)
#define mln_http_status_set(h,s)         (h)->status = (s)
#define mln_http_method_get(h)           ((h)->method)
#define mln_http_method_set(h,m)         (h)->method = (m)
#define mln_http_version_get(h)          ((h)->version)
#define mln_http_version_set(h,v)        (h)->version = (v)
#define mln_http_type_get(h)             ((h)->type)
#define mln_http_type_set(h,t)           (h)->type = (t)
#define mln_http_handler_get(h)          ((h)->body_handler)
#define mln_http_handler_set(h,hlr)      (h)->body_handler = (hlr)
#define mln_http_response_msg_get(h)     ((h)->response_msg)
#define mln_http_response_msg_set(h,m)   (h)->response_msg = (m)
#define mln_http_error_get(h)            ((h)->error)
#define mln_http_error_set(h,e)          (h)->error = (e)
#define mln_http_header_get(h)           ((h)->header_fields)

extern mln_http_t *
mln_http_init(mln_tcp_conn_t *connection, void *data, mln_http_handler body_handler);
extern void mln_http_destroy(mln_http_t *http);
extern void mln_http_reset(mln_http_t *http);
/*
 * mln_http_parse():
 * If return M_HTTP_RET_OK, that means input not enough.
 * If return M_HTTP_RET_DONE, that means parse done.
 * M_HTTP_RET_ERROR means parse error, and you can get
 * the error_code via mln_http_error_get().
 * The 'body_handler' will be called in this function to
 * process HTTP body stuff. And the second argument's type
 * is mln_chain_t **, which means that the input chain that
 * you have processed should be freed in this callback function.
 * And the third argument of this callback function will be
 * set NULL. Just ignore it.
 */
extern int mln_http_parse(mln_http_t *http, mln_chain_t **in);
/*
 * mln_http_generate():
 * Return value is the same s mln_http_parse().
 * When you processed HTTP body in function 'body_handler' called
 * in mln_http_generate(), the body chain should be returned
 * via the second and third arguments of 'body_handler'.
 */
extern int mln_http_generate(mln_http_t *http, mln_chain_t **out_head, mln_chain_t **out_tail);
extern int mln_http_field_set(mln_http_t *http, mln_string_t *key, mln_string_t *val);
extern mln_string_t *mln_http_field_get(mln_http_t *http, mln_string_t *key);
extern mln_string_t *mln_http_field_iterator(mln_http_t *http, mln_string_t *key);
extern void mln_http_field_remove(mln_http_t *http, mln_string_t *key);

extern void mln_http_dump(mln_http_t *http);

#endif

