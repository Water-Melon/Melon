
/*
 * Copyright (C) Niklaus F.Schen.
 */
#ifndef __MLN_WEBSOCKET_H
#define __MLN_WEBSOCKET_H

#include "mln_connection.h"
#include "mln_string.h"
#include "mln_http.h"
#include "mln_chain.h"
#include "mln_alloc.h"
#include "mln_hash.h"

/*
 * return value
 */
#define M_WS_RET_ERROR                   -1
#define M_WS_RET_OK                       0
#define M_WS_RET_FAILED                   1
#define M_WS_RET_NOTWS                    2
#define M_WS_RET_NOTYET                   3
/*
 * status code
 */
#define M_WS_STATUS_NORMAL_CLOSURE        1000
#define M_WS_STATUS_GOING_AWAY            1001
#define M_WS_STATUS_PROTOCOL_ERROR        1002
#define M_WS_STATUS_UNSOPPORTED_DATA      1003
#define M_WS_STATUS_RESERVED              1004
#define M_WS_STATUS_NO_STATUS_RCVD        1005
#define M_WS_STATUS_ABNOMAIL_CLOSURE      1006
#define M_WS_STATUS_INVALID_PAYLOAD_DATA  1007
#define M_WS_STATUS_POLICY_VIOLATION      1008
#define M_WS_STATUS_MESSAGE_TOO_BIG       1009
#define M_WS_STATUS_MANDATORY_EXT         1010
#define M_WS_STATUS_INTERNAL_SERVER_ERROR 1011
#define M_WS_STATUS_TLS_HANDSHAKE         1015
/*
 * opcode
 */
#define M_WS_OPCODE_CONTINUE              0x0
#define M_WS_OPCODE_TEXT                  0x1
#define M_WS_OPCODE_BINARY                0x2
#define M_WS_OPCODE_CLOSE                 0x8
#define M_WS_OPCODE_PING                  0x9
#define M_WS_OPCODE_PONG                  0xa
/*
 * flags
 */
#define M_WS_FLAG_NONE                    0
#define M_WS_FLAG_NEW                     0x1
#define M_WS_FLAG_END                     0x2
#define M_WS_FLAG_CLIENT                  0x4
#define M_WS_FLAG_SERVER                  0x8

typedef struct mln_websocket_s mln_websocket_t;
typedef int (*mln_ws_extension_handle)(mln_websocket_t *);

struct mln_websocket_s {
    mln_http_t              *http;
    mln_alloc_t             *pool;
    mln_tcp_conn_t          *connection;
    mln_hash_t              *fields;
    mln_string_t            *uri;
    mln_string_t            *args;
    mln_string_t            *key;/*Only for websocket client*/

    void                    *data;
    void                    *content;
    mln_ws_extension_handle  extension_handler;
    mln_u64_t                content_len;
    mln_u16_t                content_free:1;
    mln_u16_t                fin:1;
    mln_u16_t                rsv1:1;
    mln_u16_t                rsv2:1;
    mln_u16_t                rsv3:1;
    mln_u16_t                opcode:4;
    mln_u16_t                mask:1;
    mln_u16_t                padding:6;
    mln_u16_t                status;
    mln_u32_t                masking_key;
};

#define mln_websocket_get_http(ws)             ((ws)->http)
#define mln_websocket_get_pool(ws)             ((ws)->pool)
#define mln_websocket_get_connection(ws)       ((ws)->connection)
#define mln_websocket_get_uri(ws)              ((ws)->uri)
#define mln_websocket_set_uri(ws,u)            ((ws)->uri = (u))
#define mln_websocket_get_args(ws)             ((ws)->args)
#define mln_websocket_set_args(ws,a)           ((ws)->args = (a))
#define mln_websocket_get_key(ws)              ((ws)->key)
#define mln_websocket_set_key(ws,k)            ((ws)->key = (k))

#define mln_websocket_set_data(ws,d)           ((ws)->data = (d))
#define mln_websocket_get_data(ws)             ((ws)->data)
#define mln_websocket_set_content(ws,c)        ((ws)->content = (c))
#define mln_websocket_get_content(ws)          ((ws)->content)
#define mln_websocket_set_ext_handler(ws,h)    ((ws)->extension_handler = (h))
#define mln_websocket_get_ext_handler(ws)      ((ws)->extension_handler)
#define mln_websocket_set_rsv1(ws)             ((ws)->rsv1 = 1)
#define mln_websocket_reset_rsv1(ws)           ((ws)->rsv1 = 0)
#define mln_websocket_get_rsv1(ws)             ((ws)->rsv1)
#define mln_websocket_set_rsv2(ws)             ((ws)->rsv2 = 1)
#define mln_websocket_reset_rsv2(ws)           ((ws)->rsv2 = 0)
#define mln_websocket_get_rsv2(ws)             ((ws)->rsv2)
#define mln_websocket_set_rsv3(ws)             ((ws)->rsv3 = 1)
#define mln_websocket_reset_rsv3(ws)           ((ws)->rsv3 = 0)
#define mln_websocket_get_rsv3(ws)             ((ws)->rsv3)
#define mln_websocket_set_opcode(ws,op)        ((ws)->opcode = (op))
#define mln_websocket_get_opcode(ws)           ((ws)->opcode)
#define mln_websocket_set_status(ws,s)         ((ws)->status = (s))
#define mln_websocket_get_status(ws)           ((ws)->status)
#define mln_websocket_set_content_len(ws,l)    ((ws)->content_len = (l))
#define mln_websocket_get_content_len(ws)      ((ws)->content_len)
#define mln_websocket_set_content_free(ws)     ((ws)->content_free = 1)
#define mln_websocket_reset_content_free(ws)   ((ws)->content_free = 0)
#define mln_websocket_get_content_free(ws)     ((ws)->content_free)
#define mln_websocket_set_fin(ws)              ((ws)->fin = 1)
#define mln_websocket_reset_fin(ws)            ((ws)->fin = 0)
#define mln_websocket_get_fin(ws)              ((ws)->fin)
#define mln_websocket_set_maskbit(ws)          ((ws)->mask = 1)
#define mln_websocket_reset_maskbit(ws)        ((ws)->mask = 0)
#define mln_websocket_get_maskbit(ws)          ((ws)->mask)
#define mln_websocket_set_masking_key(ws,k)    ((ws)->masking_key = (k))
#define mln_websocket_get_masking_key(ws)      ((ws)->masking_key)

extern int mln_websocket_init(mln_websocket_t *ws, mln_http_t *http) __NONNULL2(1,2);
extern mln_websocket_t *mln_websocket_new(mln_http_t *http) __NONNULL1(1);
extern void mln_websocket_destroy(mln_websocket_t *ws);
extern void mln_websocket_free(mln_websocket_t *ws);
extern void mln_websocket_reset(mln_websocket_t *ws) __NONNULL1(1);
extern int mln_websocket_is_websocket(mln_http_t *http) __NONNULL1(1);
extern int mln_websocket_validate(mln_websocket_t *ws) __NONNULL1(1);
extern int mln_websocket_set_field(mln_websocket_t *ws, mln_string_t *key, mln_string_t *val) __NONNULL2(1,2);
extern mln_string_t *mln_websocket_get_field(mln_websocket_t *ws, mln_string_t *key) __NONNULL2(1,2);
extern int mln_websocket_match(mln_websocket_t *ws) __NONNULL1(1);
extern int mln_websocket_handshake_response_generate(mln_websocket_t *ws, \
                                                     mln_chain_t **chead, \
                                                     mln_chain_t **ctail) __NONNULL3(1,2,3);
extern int mln_websocket_handshake_request_generate(mln_websocket_t *ws, \
                                                    mln_chain_t **chead, \
                                                    mln_chain_t **ctail) __NONNULL3(1,2,3);

extern int mln_websocket_text_generate(mln_websocket_t *ws, \
                                       mln_chain_t **out_cnode, \
                                       mln_u8ptr_t buf, \
                                       mln_size_t len, \
                                       mln_u32_t flags) __NONNULL3(1,2,3);
extern int mln_websocket_binary_generate(mln_websocket_t *ws, \
                                         mln_chain_t **out_cnode, \
                                         void *buf, \
                                         mln_size_t len, \
                                         mln_u32_t flags) __NONNULL3(1,2,3);
extern int mln_websocket_close_generate(mln_websocket_t *ws, \
                                        mln_chain_t **out_cnode, \
                                        char *reason, \
                                        mln_u16_t status, \
                                        mln_u32_t flags) __NONNULL2(1,2);
extern int mln_websocket_ping_generate(mln_websocket_t *ws, mln_chain_t **out_cnode, mln_u32_t flags) __NONNULL2(1,2);
extern int mln_websocket_pong_generate(mln_websocket_t *ws, mln_chain_t **out_cnode, mln_u32_t flags) __NONNULL2(1,2);
extern int mln_websocket_generate(mln_websocket_t *ws, mln_chain_t **out_cnode) __NONNULL1(1);
extern int mln_websocket_parse(mln_websocket_t *ws, mln_chain_t **in) __NONNULL1(1);

#endif
