
/*
 * Copyright (C) Niklaus F.Schen.
 */

#include <stdio.h>
#include <stdlib.h>
#include "mln_websocket.h"
#include "mln_regexp.h"
#include "mln_sha.h"
#include "mln_base64.h"
#if defined(MSVC)
#include "mln_utils.h"
#else
#include <sys/time.h>
#endif
#include "mln_func.h"

static mln_u64_t mln_websocket_hash_calc(mln_hash_t *h, void *key);
static int mln_websocket_hash_cmp(mln_hash_t *h, void *key1, void *key2);
static void mln_websocket_hash_free(void *data);
static int mln_websocket_match_iterate_handler(mln_hash_t *h, void *key, void *val, void *data);
static int mln_websocket_validate_accept(mln_http_t *http, mln_string_t *wskey);
static mln_string_t *mln_websocket_accept_field(mln_http_t *http);
static int mln_websocket_iterate_set_fields(mln_hash_t *h, void *key, void *val, void *data);
static mln_string_t *mln_websocket_client_handshake_key_generate(mln_alloc_t *pool);
static mln_string_t *mln_websocket_extension_tokens(mln_alloc_t *pool, mln_string_t *in);
static mln_u32_t mln_websocket_masking_key_generate(void);
static int mln_websocket_is_valid_status_code(mln_u16_t status);
static int mln_websocket_is_valid_utf8(mln_u8ptr_t data, mln_size_t len);
static void mln_websocket_unmask_xor4(mln_u8ptr_t data, mln_size_t len, mln_u32_t masking_key);

MLN_FUNC(, int, mln_websocket_init, (mln_websocket_t *ws, mln_http_t *http), (ws, http), {
    struct mln_hash_attr hattr;
    hattr.pool = mln_http_pool_get(http);
    hattr.pool_alloc = (hash_pool_alloc_handler)mln_alloc_m;
    hattr.pool_free = (hash_pool_free_handler)mln_alloc_free;
    hattr.hash = mln_websocket_hash_calc;
    hattr.cmp = mln_websocket_hash_cmp;
    hattr.key_freer = mln_websocket_hash_free;
    hattr.val_freer = mln_websocket_hash_free;
    hattr.len_base = 37;
    hattr.expandable = 0;
    hattr.calc_prime = 0;

    ws->http = http;
    ws->pool = mln_http_pool_get(http);
    ws->connection = mln_http_connection_get(http);
    if ((ws->fields = mln_hash_new(&hattr)) == NULL) return -1;
    ws->uri = ws->args = ws->key = NULL;

    ws->data = NULL;
    ws->content = NULL;
    ws->extension_handler = NULL;
    ws->content_len = 0;
    ws->content_free = 0;
    ws->fin = 0;
    ws->rsv1 = ws->rsv2 = ws->rsv3 = 0;
    ws->opcode = 0;
    ws->mask = 0;
    ws->status = 0;
    ws->masking_key = 0;

    return 0;
})

MLN_FUNC(static, mln_u64_t, mln_websocket_hash_calc, (mln_hash_t *h, void *key), (h, key), {
    mln_string_t *k = (mln_string_t *)key;
    mln_u64_t index = 0;
    mln_u8ptr_t p, end = k->data + k->len;

    for (p = k->data; p < end; ++p) {
        index += (*p * 3);
    }

    return index % h->len;
})

MLN_FUNC(static, int, mln_websocket_hash_cmp, \
         (mln_hash_t *h, void *key1, void *key2), (h, key1, key2), \
{
    return !mln_string_strcasecmp((mln_string_t *)key1, (mln_string_t *)key2);
})

MLN_FUNC_VOID(static, void, mln_websocket_hash_free, (void *data), (data), {
    if (data == NULL) return;
    mln_string_free((mln_string_t *)data);
})

MLN_FUNC(, mln_websocket_t *, mln_websocket_new, (mln_http_t *http), (http), {
    mln_websocket_t *ws = (mln_websocket_t *)mln_alloc_m(mln_http_pool_get(http), sizeof(mln_websocket_t));
    if (ws == NULL) return NULL;
    if (mln_websocket_init(ws, http) < 0) {
        mln_alloc_free(ws);
        return NULL;
    }
    return ws;
})

MLN_FUNC_VOID(, void, mln_websocket_destroy, (mln_websocket_t *ws), (ws), {
    if (ws == NULL) return;
    if (ws->fields != NULL) mln_hash_free(ws->fields, M_HASH_F_KV);
    if (ws->uri != NULL) mln_string_free(ws->uri);
    if (ws->args != NULL) mln_string_free(ws->args);
    if (ws->key != NULL) mln_string_free(ws->key);
    if (ws->content_free) mln_alloc_free(ws->content);
})

MLN_FUNC_VOID(, void, mln_websocket_free, (mln_websocket_t *ws), (ws), {
    if (ws == NULL) return;
    mln_websocket_destroy(ws);
    mln_alloc_free(ws);
})

MLN_FUNC_VOID(, void, mln_websocket_reset, (mln_websocket_t *ws), (ws), {
    if (ws->fields != NULL) {
        mln_hash_reset(ws->fields, M_HASH_F_KV);
    }
    if (ws->uri != NULL) {
        mln_string_free(ws->uri);
        ws->uri = NULL;
    }
    if (ws->args != NULL) {
        mln_string_free(ws->args);
        ws->args = NULL;
    }
    if (ws->key != NULL) {
        mln_string_free(ws->key);
        ws->key = NULL;
    }

    ws->data = NULL;
    if (ws->content_free) {
        ws->content_free = 0;
        mln_alloc_free(ws->content);
        ws->content = NULL;
    } else {
        ws->content = NULL;
    }
    ws->extension_handler = NULL;
    ws->content_len = 0;
    ws->fin = 0;
    ws->rsv1 = ws->rsv2 = ws->rsv3 = 0;
    ws->opcode = 0;
    ws->mask = 0;
    ws->status = 0;
    ws->masking_key = 0;
})


MLN_FUNC(, int, mln_websocket_is_websocket, (mln_http_t *http), (http), {
    mln_string_t key = mln_string("Upgrade");
    mln_string_t val = mln_string("websocket");
    mln_string_t *tmp = mln_http_field_get(http, &key);
    if (tmp == NULL || mln_string_strcasecmp(&val, tmp)) return M_WS_RET_NOTWS;
    if (mln_http_type_get(http) != M_HTTP_REQUEST) return M_WS_RET_ERROR;
    return M_WS_RET_OK;
})

static int mln_websocket_conn_has_upgrade_token(mln_string_t *val);

MLN_FUNC(, int, mln_websocket_validate, (mln_websocket_t *ws), (ws), {
    mln_http_t *http = ws->http;
    if (mln_http_status_get(http) != M_HTTP_SWITCHING_PROTOCOLS) return M_WS_RET_NOTWS;

    mln_string_t upgrade_key = mln_string("Upgrade");
    mln_string_t upgrade_val = mln_string("websocket");
    mln_string_t connection_key = mln_string("Connection");
    mln_string_t *tmp;

    tmp = mln_http_field_get(http, &upgrade_key);
    if (tmp == NULL || mln_string_strcasecmp(tmp, &upgrade_val)) return M_WS_RET_NOTWS;
    tmp = mln_http_field_get(http, &connection_key);
    if (tmp == NULL || !mln_websocket_conn_has_upgrade_token(tmp)) return M_WS_RET_NOTWS;
    int ret = mln_websocket_validate_accept(http, ws->key);
    if (ret != M_WS_RET_OK) return ret;
    if (mln_http_type_get(http) != M_HTTP_RESPONSE) return M_WS_RET_ERROR;

    return M_WS_RET_OK;
})

MLN_FUNC(static, int, mln_websocket_validate_accept, \
         (mln_http_t *http, mln_string_t *wskey), (http, wskey), \
{
    mln_sha1_t s;
    mln_sha1_init(&s);
    mln_alloc_t *pool = mln_http_pool_get(http);
    mln_string_t key = mln_string("Sec-WebSocket-Accept");
    mln_string_t *val = mln_http_field_get(http, &key);
    if (val == NULL) return M_WS_RET_NOTWS;
    char guid[] = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

    mln_u8ptr_t buf = (mln_u8ptr_t)mln_alloc_m(pool, wskey->len+sizeof(guid));
    if (buf == NULL) return -1;
    memcpy(buf, wskey->data, wskey->len);
    memcpy(buf+wskey->len, guid, sizeof(guid));
    mln_sha1_calc(&s, buf, wskey->len+sizeof(guid)-1, 1);
    mln_alloc_free(buf);
    buf = NULL;
    mln_u8_t tmp[20];
    mln_uauto_t len = 0;
    mln_sha1_tobytes(&s, tmp, sizeof(tmp));
    if (mln_base64_pool_encode(pool, tmp, sizeof(tmp), &buf, &len) < 0) return -1;
    mln_string_nset(&key, buf, len);
    int ret = mln_string_strcasecmp(&key, val);
    mln_base64_pool_free(buf);
    return ret? M_WS_RET_ERROR: M_WS_RET_OK;
})

MLN_FUNC(, int, mln_websocket_set_field, \
         (mln_websocket_t *ws, mln_string_t *key, mln_string_t *val), \
         (ws, key, val), \
{
    mln_string_t *dup_key, *dup_val;
    dup_key = mln_string_pool_dup(ws->pool, key);
    if (dup_key == NULL) return M_WS_RET_FAILED;
    dup_val = mln_string_pool_dup(ws->pool, val);
    if (dup_val == NULL) {
        mln_string_free(dup_key);
        return M_WS_RET_FAILED;
    }
    int ret = mln_hash_update(ws->fields, &dup_key, &dup_val);
    mln_string_free(dup_key);
    mln_string_free(dup_val);
    return ret<0? M_WS_RET_FAILED: M_WS_RET_OK;
})

MLN_FUNC(, mln_string_t *, mln_websocket_get_field, \
         (mln_websocket_t *ws, mln_string_t *key), (ws, key), \
{
    return (mln_string_t *)mln_hash_search(ws->fields, key);
})

int mln_websocket_match(mln_websocket_t *ws)
{
    if (mln_hash_iterate(ws->fields, mln_websocket_match_iterate_handler, ws->http) < 0)
        return M_WS_RET_ERROR;
    return M_WS_RET_OK;
}

MLN_FUNC(static, int, mln_websocket_match_iterate_handler, \
         (mln_hash_t *h, void *key, void *val, void *data), (h, key, val, data), \
{
    mln_reg_match_result_t *res = NULL;
    mln_string_t *tmp = mln_http_field_get((mln_http_t *)data, (mln_string_t *)key);
    if (tmp == NULL) return -1;
    if ((res = mln_reg_match_result_new(1)) == NULL) {
        return -1;
    }
    if (val != NULL && mln_reg_match((mln_string_t *)val, tmp, res) <= 0) {
        mln_reg_match_result_free(res);
        return -1;
    }
    mln_reg_match_result_free(res);
    return 0;
})

MLN_FUNC(, int, mln_websocket_handshake_response_generate, \
         (mln_websocket_t *ws, mln_chain_t **chead, mln_chain_t **ctail), \
         (ws, chead, ctail), \
{
    mln_string_t *tmp;
    mln_http_t *http = ws->http;

    mln_string_t protocol_key = mln_string("Sec-WebSocket-Protocol");
    mln_string_t *protocol_val = NULL;
    tmp = mln_http_field_iterator(http, &protocol_key);
    if (tmp) {
        protocol_val = mln_string_pool_dup(ws->pool, tmp);
        if (protocol_val == NULL) return M_WS_RET_FAILED;
    }

    mln_string_t extension_key = mln_string("Sec-WebSocket-Extensions");
    mln_string_t *extension_val = NULL;
    tmp = mln_http_field_iterator(http, &extension_key);
    if (tmp) {
        extension_val = mln_websocket_extension_tokens(ws->pool, tmp);
    }

    mln_string_t *accept = mln_websocket_accept_field(http);
    if (accept == NULL) {
        if (protocol_val != NULL) mln_string_free(protocol_val);
        if (extension_val != NULL) mln_string_free(extension_val);
        return M_WS_RET_FAILED;
    }

    mln_http_reset(http);
    mln_http_status_set(http, M_HTTP_SWITCHING_PROTOCOLS);
    mln_http_version_set(http, M_HTTP_VERSION_1_1);
    mln_http_type_set(http, M_HTTP_RESPONSE);
    mln_http_handler_set(http, NULL);

    mln_string_t upgrade_key = mln_string("Upgrade");
    mln_string_t upgrade_val = mln_string("websocket");
    mln_string_t connection_key = mln_string("Connection");
    mln_string_t accept_key = mln_string("Sec-WebSocket-Accept");


    if (protocol_val != NULL) {
        if (mln_http_field_set(http, &protocol_key, protocol_val) != M_HTTP_RET_OK) {
            if (protocol_val != NULL) mln_string_free(protocol_val);
            if (extension_val != NULL) mln_string_free(extension_val);
            if (accept != NULL) mln_string_free(accept);
            return M_WS_RET_FAILED;
        }
    }
    if (extension_val != NULL) {
        if (mln_http_field_set(http, &extension_key, extension_val) != M_HTTP_RET_OK) {
            if (extension_val != NULL) mln_string_free(extension_val);
            if (accept != NULL) mln_string_free(accept);
            return M_WS_RET_FAILED;
        }
    }
    if (mln_http_field_set(http, &accept_key, accept) != M_HTTP_RET_OK) {
        mln_string_free(accept);
        return M_WS_RET_FAILED;
    }
    if (mln_http_field_set(http, &upgrade_key, &upgrade_val) != M_HTTP_RET_OK) return M_WS_RET_FAILED;
    if (mln_http_field_set(http, &connection_key, &upgrade_key) != M_HTTP_RET_OK) return M_WS_RET_FAILED;

    if (mln_hash_iterate(ws->fields, mln_websocket_iterate_set_fields, http) < 0)
        return M_WS_RET_FAILED;

    if (mln_http_generate(http, chead, ctail) == M_HTTP_RET_ERROR) return M_WS_RET_FAILED;

    return M_WS_RET_OK;
})

MLN_FUNC(static, mln_string_t *, mln_websocket_extension_tokens, \
         (mln_alloc_t *pool, mln_string_t *in), (pool, in), \
{
    mln_string_t *tmp = mln_string_pool_dup(pool, in);
    if (tmp == NULL) return NULL;
    mln_string_t *array = mln_string_slice(tmp, ",");
    if (array == NULL) {
        mln_string_free(tmp);
        return NULL;
    }
    mln_string_t *p = array;
    mln_s8ptr_t pos, buf;
    mln_uauto_t size = 0;
    for (; p->len; ++p) {
        if ((pos = strchr((char *)(p->data), ';')) == NULL) {
            size += (p->len + 1);
        } else {
            size += (pos - (char *)(p->data) + 1);
        }
    }
    if (!size) return NULL;

    --size;
    buf = (mln_s8ptr_t)mln_alloc_m(pool, size);
    if (buf == NULL) {
        mln_string_slice_free(array);
        mln_string_free(tmp);
        return NULL;
    }
    for (size = 0, p = array; p->len; ++p) {
        if ((pos = strchr((char *)(p->data), ';')) == NULL) {
            memcpy(buf+size, p->data, p->len);
            size += p->len;
        } else {
            memcpy(buf+size, p->data, pos-(char *)(p->data));
            size += (pos - (char *)(p->data));
        }
        buf[size++] = ',';
    }
    buf[--size] = '\0';
    mln_string_slice_free(array);
    mln_string_free(tmp);

    mln_string_t t;
    mln_string_nset(&t, buf, size);
    tmp = mln_string_pool_dup(pool, &t);
    mln_alloc_free(buf);
    return tmp;
})

MLN_FUNC(static, int, mln_websocket_iterate_set_fields, \
         (mln_hash_t *h, void *key, void *val, void *data), (h, key, val, data), \
{
    return mln_http_field_set((mln_http_t *)data, (mln_string_t *)key, (mln_string_t *)val)==M_HTTP_RET_OK?0:-1;
})

MLN_FUNC(static, mln_string_t *, mln_websocket_accept_field, (mln_http_t *http), (http), {
    mln_sha1_t s;
    mln_sha1_init(&s);
    mln_alloc_t *pool = mln_http_pool_get(http);
    mln_string_t key = mln_string("Sec-WebSocket-Key");
    mln_string_t *val = mln_http_field_get(http, &key);
    char guid[] = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

    mln_u8ptr_t buf = (mln_u8ptr_t)mln_alloc_m(pool, val->len+sizeof(guid));
    if (buf == NULL) return NULL;
    memcpy(buf, val->data, val->len);
    memcpy(buf+val->len, guid, sizeof(guid));
    mln_sha1_calc(&s, buf, val->len+sizeof(guid)-1, 1);
    mln_alloc_free(buf);
    buf = NULL;
    mln_u8_t tmp[20];
    mln_uauto_t len = 0;
    mln_sha1_tobytes(&s, tmp, sizeof(tmp));
    if (mln_base64_pool_encode(pool, tmp, sizeof(tmp), &buf, &len) < 0) return NULL;
    mln_string_nset(&key, buf, len);
    val = mln_string_pool_dup(pool, &key);
    mln_base64_pool_free(buf);
    return val;
})

MLN_FUNC(, int, mln_websocket_handshake_request_generate, \
         (mln_websocket_t *ws, mln_chain_t **chead, mln_chain_t **ctail), \
         (ws, chead, ctail), \
{
    mln_http_t *http = ws->http;
    mln_alloc_t *pool = ws->pool;

    mln_string_t *dup_uri = NULL;
    if (ws->uri == NULL) {
        mln_string_t tmp = mln_string("/");
        dup_uri = mln_string_pool_dup(pool, &tmp);
    } else {
        dup_uri = mln_string_pool_dup(pool, ws->uri);
    }
    if (dup_uri == NULL) return M_WS_RET_FAILED;
    mln_string_t *dup_args = NULL;
    if (ws->args != NULL) {
        dup_args = mln_string_pool_dup(pool, ws->args);
        if (dup_args == NULL) {
            mln_string_free(dup_uri);
            return M_WS_RET_FAILED;
        }
    }

    mln_http_reset(http);
    mln_http_method_set(http, M_HTTP_GET);
    mln_http_version_set(http, M_HTTP_VERSION_1_1);
    mln_http_type_set(http, M_HTTP_REQUEST);
    mln_http_uri_set(http, dup_uri);
    if (dup_args != NULL) mln_http_args_set(http, dup_args);
    mln_http_handler_set(http, NULL);

    mln_string_t upgrade_key = mln_string("Upgrade");
    mln_string_t upgrade_val = mln_string("websocket");
    mln_string_t connection_key = mln_string("Connection");
    mln_string_t version_key = mln_string("Sec-WebSocket-Version");
    mln_string_t version_val = mln_string("13");
    mln_string_t key_key = mln_string("Sec-WebSocket-Key");
    mln_string_t *key_val = mln_websocket_client_handshake_key_generate(pool);
    if (key_val == NULL) return M_WS_RET_FAILED;
    if (mln_http_field_set(http, &key_key, key_val) < 0) {
        mln_string_free(key_val);
        return M_WS_RET_FAILED;
    }
    if (mln_http_field_set(http, &upgrade_key, &upgrade_val) < 0) return M_WS_RET_FAILED;
    if (mln_http_field_set(http, &connection_key, &upgrade_key) < 0) return M_WS_RET_FAILED;
    if (mln_http_field_set(http, &version_key, &version_val) < 0) return M_WS_RET_FAILED;

    if (mln_hash_iterate(ws->fields, mln_websocket_iterate_set_fields, http) < 0)
        return M_WS_RET_FAILED;

    if (mln_http_generate(http, chead, ctail) == M_HTTP_RET_ERROR) return M_WS_RET_FAILED;

    return M_WS_RET_OK;
})

MLN_FUNC(static, mln_string_t *, mln_websocket_client_handshake_key_generate, (mln_alloc_t *pool), (pool), {
    struct timeval tv;
    mln_u8_t buf[16];
    mln_u32_t i, tmp;
    mln_u8ptr_t out = NULL;
    mln_uauto_t outlen = 0;
    mln_string_t s, *sdup;

    gettimeofday(&tv, NULL);
    srand(tv.tv_sec*1000000+tv.tv_usec);
    for (i = 0; i < 4; ++i) {
        tmp = (mln_u32_t)rand();
        buf[i*4] = (tmp >> 24) & 0xff;
        buf[i*4+1] = (tmp >> 16) & 0xff;
        buf[i*4+2] = (tmp >> 8) & 0xff;
        buf[i*4+3] = tmp & 0xff;
        srand(tmp>>16|tmp<<16);
    }

    if (mln_base64_pool_encode(pool, buf, sizeof(buf), &out, &outlen) < 0) return NULL;
    mln_string_nset(&s, out, outlen);
    sdup = mln_string_pool_dup(pool, &s);
    mln_base64_pool_free(out);
    return sdup;
})


MLN_FUNC(, int, mln_websocket_text_generate, \
         (mln_websocket_t *ws, mln_chain_t **out_cnode, mln_u8ptr_t buf, \
          mln_size_t len, mln_u32_t flags), \
         (ws, out_cnode, buf, len, flags), \
{
    if ((flags & M_WS_FLAG_CLIENT) && (flags & M_WS_FLAG_SERVER)) return M_WS_RET_ERROR;

    if (mln_websocket_get_content_free(ws)) {
        mln_alloc_free(mln_websocket_get_content(ws));
        mln_websocket_reset_content_free(ws);
    }
    mln_websocket_set_content(ws, buf);
    mln_websocket_set_content_len(ws, len);
    mln_websocket_reset_content_free(ws);
    if (flags & M_WS_FLAG_END) mln_websocket_set_fin(ws);
    else mln_websocket_reset_fin(ws);
    mln_websocket_reset_rsv1(ws);
    mln_websocket_reset_rsv2(ws);
    mln_websocket_reset_rsv3(ws);
    if (flags & M_WS_FLAG_NEW) mln_websocket_set_opcode(ws, M_WS_OPCODE_TEXT);
    else mln_websocket_set_opcode(ws, M_WS_OPCODE_CONTINUE);
    if (flags & M_WS_FLAG_CLIENT) {
        mln_websocket_set_maskbit(ws);
        mln_websocket_set_masking_key(ws, mln_websocket_masking_key_generate());
    } else {
        mln_websocket_reset_maskbit(ws);
    }

    return mln_websocket_generate(ws, out_cnode);
})

MLN_FUNC(, int, mln_websocket_binary_generate, \
         (mln_websocket_t *ws, mln_chain_t **out_cnode, void *buf, \
          mln_size_t len, mln_u32_t flags), \
         (ws, out_cnode, buf, len, flags), \
{
    if ((flags & M_WS_FLAG_CLIENT) && (flags & M_WS_FLAG_SERVER)) return M_WS_RET_ERROR;

    if (mln_websocket_get_content_free(ws)) {
        mln_alloc_free(mln_websocket_get_content(ws));
        mln_websocket_reset_content_free(ws);
    }
    mln_websocket_set_content(ws, buf);
    mln_websocket_set_content_len(ws, len);
    mln_websocket_reset_content_free(ws);
    if (flags & M_WS_FLAG_END) mln_websocket_set_fin(ws);
    else mln_websocket_reset_fin(ws);
    mln_websocket_reset_rsv1(ws);
    mln_websocket_reset_rsv2(ws);
    mln_websocket_reset_rsv3(ws);
    if (flags & M_WS_FLAG_NEW) mln_websocket_set_opcode(ws, M_WS_OPCODE_BINARY);
    else mln_websocket_set_opcode(ws, M_WS_OPCODE_CONTINUE);
    if (flags & M_WS_FLAG_CLIENT) {
        mln_websocket_set_maskbit(ws);
        mln_websocket_set_masking_key(ws, mln_websocket_masking_key_generate());
    } else {
        mln_websocket_reset_maskbit(ws);
    }

    return mln_websocket_generate(ws, out_cnode);
})

MLN_FUNC(, int, mln_websocket_close_generate, \
         (mln_websocket_t *ws, mln_chain_t **out_cnode, \
          char *reason, mln_u16_t status, mln_u32_t flags), \
         (ws, out_cnode, reason, status, flags), \
{
    if ((flags & M_WS_FLAG_CLIENT) && (flags & M_WS_FLAG_SERVER)) return M_WS_RET_ERROR;

    if (mln_websocket_get_content_free(ws)) {
        mln_alloc_free(mln_websocket_get_content(ws));
        mln_websocket_reset_content_free(ws);
    }
    mln_websocket_set_content(ws, reason);
    if (reason == NULL) mln_websocket_set_content_len(ws, 0);
    else mln_websocket_set_content_len(ws, strlen(reason));
    mln_websocket_set_fin(ws);
    mln_websocket_reset_rsv1(ws);
    mln_websocket_reset_rsv2(ws);
    mln_websocket_reset_rsv3(ws);
    mln_websocket_set_opcode(ws, M_WS_OPCODE_CLOSE);

    if (flags & M_WS_FLAG_CLIENT) {
        mln_websocket_set_maskbit(ws);
        mln_websocket_set_masking_key(ws, mln_websocket_masking_key_generate());
    } else {
        mln_websocket_reset_maskbit(ws);
    }

    return mln_websocket_generate(ws, out_cnode);
})

MLN_FUNC(, int, mln_websocket_ping_generate, \
         (mln_websocket_t *ws, mln_chain_t **out_cnode, mln_u32_t flags), \
         (ws, out_cnode, flags), \
{
    if ((flags & M_WS_FLAG_CLIENT) && (flags & M_WS_FLAG_SERVER)) return M_WS_RET_ERROR;

    if (mln_websocket_get_content_free(ws)) {
        mln_alloc_free(mln_websocket_get_content(ws));
    }
    mln_websocket_set_content(ws, NULL);
    mln_websocket_set_content_len(ws, 0);
    mln_websocket_reset_content_free(ws);
    mln_websocket_set_fin(ws);
    mln_websocket_reset_rsv1(ws);
    mln_websocket_reset_rsv2(ws);
    mln_websocket_reset_rsv3(ws);
    mln_websocket_set_opcode(ws, M_WS_OPCODE_PING);

    if (flags & M_WS_FLAG_CLIENT) {
        mln_websocket_set_maskbit(ws);
        mln_websocket_set_masking_key(ws, mln_websocket_masking_key_generate());
    } else {
        mln_websocket_reset_maskbit(ws);
    }

    return mln_websocket_generate(ws, out_cnode);
})

MLN_FUNC(, int, mln_websocket_pong_generate, \
         (mln_websocket_t *ws, mln_chain_t **out_cnode, mln_u32_t flags), \
         (ws, out_cnode, flags), \
{
    if ((flags & M_WS_FLAG_CLIENT) && (flags & M_WS_FLAG_SERVER)) return M_WS_RET_ERROR;

    if (mln_websocket_get_content_free(ws)) {
        mln_alloc_free(mln_websocket_get_content(ws));
    }
    mln_websocket_set_content(ws, NULL);
    mln_websocket_set_content_len(ws, 0);
    mln_websocket_reset_content_free(ws);
    mln_websocket_set_fin(ws);
    mln_websocket_reset_rsv1(ws);
    mln_websocket_reset_rsv2(ws);
    mln_websocket_reset_rsv3(ws);
    mln_websocket_set_opcode(ws, M_WS_OPCODE_PONG);

    if (flags & M_WS_FLAG_CLIENT) {
        mln_websocket_set_maskbit(ws);
        mln_websocket_set_masking_key(ws, mln_websocket_masking_key_generate());
    } else {
        mln_websocket_reset_maskbit(ws);
    }

    return mln_websocket_generate(ws, out_cnode);
})

static int mln_websocket_conn_has_upgrade_token(mln_string_t *val)
{
    mln_u8ptr_t p = val->data;
    mln_u8ptr_t end = val->data + val->len;
    while (p < end) {
        while (p < end && (*p == ' ' || *p == '\t')) p++;
        mln_u8ptr_t tok_start = p;
        while (p < end && *p != ',') p++;
        mln_u8ptr_t tok_end = p;
        while (tok_end > tok_start && (tok_end[-1] == ' ' || tok_end[-1] == '\t')) tok_end--;
        if ((mln_size_t)(tok_end - tok_start) == 7) {
            mln_string_t tok;
            tok.data = tok_start;
            tok.len = 7;
            if (!mln_string_const_strcasecmp(&tok, "Upgrade")) return 1;
        }
        if (p < end) p++;
    }
    return 0;
}

MLN_FUNC(static, int, mln_websocket_is_valid_status_code, (mln_u16_t status), (status), {
    if (status >= 1000 && status <= 1003) return 1;
    if (status == 1005 || status == 1006) return 0;
    if (status >= 1007 && status <= 1011) return 1;
    if (status >= 1012 && status <= 1014) return 1;
    if (status == 1015) return 0;
    if (status >= 3000 && status <= 3999) return 1;
    if (status >= 4000 && status <= 4999) return 1;
    return 0;
})

MLN_FUNC(static, int, mln_websocket_is_valid_utf8, (mln_u8ptr_t data, mln_size_t len), (data, len), {
    mln_size_t i = 0;
    mln_u8_t b0, b1, b2, b3;
    mln_u32_t cp;
    while (i < len) {
        b0 = data[i];
        if (b0 < 0x80) {
            i += 1;
        } else if ((b0 & 0xE0) == 0xC0) {
            if (i + 1 >= len) return 0;
            b1 = data[i+1];
            if ((b1 & 0xC0) != 0x80) return 0;
            cp = ((mln_u32_t)(b0 & 0x1F) << 6) | (b1 & 0x3F);
            if (cp < 0x80) return 0;           /* overlong */
            i += 2;
        } else if ((b0 & 0xF0) == 0xE0) {
            if (i + 2 >= len) return 0;
            b1 = data[i+1]; b2 = data[i+2];
            if ((b1 & 0xC0) != 0x80) return 0;
            if ((b2 & 0xC0) != 0x80) return 0;
            cp = ((mln_u32_t)(b0 & 0x0F) << 12) | ((mln_u32_t)(b1 & 0x3F) << 6) | (b2 & 0x3F);
            if (cp < 0x800) return 0;          /* overlong */
            if (cp >= 0xD800 && cp <= 0xDFFF) return 0; /* surrogate halves */
            i += 3;
        } else if ((b0 & 0xF8) == 0xF0) {
            if (i + 3 >= len) return 0;
            b1 = data[i+1]; b2 = data[i+2]; b3 = data[i+3];
            if ((b1 & 0xC0) != 0x80) return 0;
            if ((b2 & 0xC0) != 0x80) return 0;
            if ((b3 & 0xC0) != 0x80) return 0;
            cp = ((mln_u32_t)(b0 & 0x07) << 18) | ((mln_u32_t)(b1 & 0x3F) << 12) |
                 ((mln_u32_t)(b2 & 0x3F) << 6) | (b3 & 0x3F);
            if (cp < 0x10000) return 0;        /* overlong */
            if (cp > 0x10FFFF) return 0;       /* out of range */
            i += 4;
        } else {
            return 0;
        }
    }
    return 1;
})

MLN_FUNC(static, void, mln_websocket_unmask_xor4, (mln_u8ptr_t data, mln_size_t len, mln_u32_t masking_key), (data, len, masking_key), {
    mln_size_t i = 0;
    mln_u8_t tmpkey[4];
    tmpkey[0] = (masking_key >> 24) & 0xff;
    tmpkey[1] = (masking_key >> 16) & 0xff;
    tmpkey[2] = (masking_key >> 8) & 0xff;
    tmpkey[3] = masking_key & 0xff;

    mln_size_t aligned = len & ~3;
    mln_u32_t *ptr32 = (mln_u32_t *)data;
    mln_u32_t *key32 = (mln_u32_t *)tmpkey;
    mln_size_t j;
    for (j = 0; j < aligned; j += 4) {
        ptr32[j/4] ^= *key32;
    }

    for (i = aligned; i < len; ++i) {
        data[i] ^= tmpkey[i % 4];
    }
})

MLN_FUNC(static, mln_u32_t, mln_websocket_masking_key_generate, (void), (), {
    struct timeval tv;
    mln_uauto_t tmp = (mln_uauto_t)&tv;
    gettimeofday(&tv, NULL);
    srand(tv.tv_sec*1000000+tv.tv_usec);
    return ((mln_u32_t)tmp | (mln_u32_t)rand());
})

MLN_FUNC(, int, mln_websocket_generate, \
         (mln_websocket_t *ws, mln_chain_t **out_cnode), (ws, out_cnode), \
{
    mln_size_t size = 2;
    mln_u8ptr_t buf, p;
    mln_buf_t *b;
    mln_chain_t *c;
    mln_alloc_t *pool = ws->pool;
    mln_u8_t payload_length = 0;
    mln_u8ptr_t content = NULL;
    mln_u64_t clen = 0;
    mln_u32_t opcode = mln_websocket_get_opcode(ws);

    if (mln_websocket_get_ext_handler(ws) != NULL) {
        int ret = mln_websocket_get_ext_handler(ws)(ws);
        if (ret != M_WS_RET_OK) return ret;
    }

    content = (mln_u8ptr_t)mln_websocket_get_content(ws);
    clen = mln_websocket_get_content_len(ws);
    if (content == NULL && clen) return M_WS_RET_ERROR;

    if (opcode == M_WS_OPCODE_CLOSE) {
        clen += 2;
    }
    if ((opcode == M_WS_OPCODE_CLOSE || \
         opcode == M_WS_OPCODE_PING || \
         opcode == M_WS_OPCODE_PONG) && \
        clen > 125)
        return M_WS_RET_ERROR;

    if (clen > 125) {
        if ((clen >> 16)) {
            size += 8;
            payload_length = 127;
        } else {
            size += 2;
            payload_length = 126;
        }
    } else {
        payload_length = clen;
    }
    size += clen;

    if (mln_websocket_get_maskbit(ws)) size += 4;

    c = mln_chain_new(pool);
    if (c == NULL) return M_WS_RET_FAILED;
    b = mln_buf_new(pool);
    if (b == NULL) {
        mln_chain_pool_release(c);
        return M_WS_RET_FAILED;
    }
    c->buf = b;
    buf = (mln_u8ptr_t)mln_alloc_m(pool, size);
    if (buf == NULL) {
        mln_chain_pool_release(c);
        return M_WS_RET_FAILED;
    }
    b->left_pos = b->pos = b->start = buf;
    b->end = b->last = buf + size;
    b->in_memory = 1;
    b->last_buf = 1;
    if (mln_websocket_get_fin(ws)) b->last_in_chain = 1;
    *out_cnode = c;

    p = buf;
    *p = 0;
    if (mln_websocket_get_fin(ws)) *p |= 0x80;
    if (mln_websocket_get_rsv1(ws)) *p |= 0x40;
    if (mln_websocket_get_rsv2(ws)) *p |= 0x20;
    if (mln_websocket_get_rsv3(ws)) *p |= 0x10;
    *p++ |= (opcode & 0xf);

    *p = 0;
    if (mln_websocket_get_maskbit(ws)) *p |= 0x80;
    *p++ |= (payload_length & 0x7f);

    if (payload_length == 126) {
        *p++ = ((clen >> 8) & 0xff);
        *p++ = (clen & 0xff);
    } else if (payload_length == 127) {
        *p++ = ((clen >> 56) & 0xff);
        *p++ = ((clen >> 48) & 0xff);
        *p++ = ((clen >> 40) & 0xff);
        *p++ = ((clen >> 32) & 0xff);
        *p++ = ((clen >> 24) & 0xff);
        *p++ = ((clen >> 16) & 0xff);
        *p++ = ((clen >> 8) & 0xff);
        *p++ = (clen & 0xff);
    }

    if (mln_websocket_get_maskbit(ws)) {
        mln_u8_t tmpkey[4];
        mln_u32_t m = mln_websocket_get_masking_key(ws);
        mln_u8ptr_t pc = (mln_u8ptr_t)content;
        *p++ = tmpkey[0] = ((m >> 24) & 0xff);
        *p++ = tmpkey[1] = ((m >> 16) & 0xff);
        *p++ = tmpkey[2] = ((m >> 8) & 0xff);
        *p++ = tmpkey[3] = (m & 0xff);

        mln_u32_t mask_offset = 0;
        if (opcode == M_WS_OPCODE_CLOSE) {
            *p++ = ((mln_websocket_get_status(ws) >> 8) & 0xff) ^ tmpkey[0];
            *p++ = (mln_websocket_get_status(ws) & 0xff) ^ tmpkey[1];
            mask_offset = 2;
        }

        mln_size_t remaining = clen - (opcode == M_WS_OPCODE_CLOSE ? 2 : 0);
        /* Build key rotated by mask_offset (0 or 2) for remaining payload */
        mln_u8_t rotkey[4];
        if (mask_offset == 0) {
            memcpy(rotkey, tmpkey, 4);
        } else {
            rotkey[0] = tmpkey[2]; rotkey[1] = tmpkey[3];
            rotkey[2] = tmpkey[0]; rotkey[3] = tmpkey[1];
        }
        mln_size_t aligned = remaining & ~3;
        mln_size_t k;
        mln_u32_t key32_rot, word;
        memcpy(&key32_rot, rotkey, 4);
        for (k = 0; k < aligned; k += 4) {
            memcpy(&word, pc + k, 4);
            word ^= key32_rot;
            memcpy(p + k, &word, 4);
        }
        p += aligned;
        mln_size_t j;
        for (j = aligned; j < remaining; ++j) {
            *p++ = pc[j] ^ rotkey[j % 4];
        }
    } else {
        if (opcode == M_WS_OPCODE_CLOSE) {
            *p++ = (mln_websocket_get_status(ws) >> 8) & 0xff;
            *p++ = mln_websocket_get_status(ws) & 0xff;
            clen -= 2;
        }
        if (content != NULL) memcpy(p, content, clen);
    }

    return M_WS_RET_OK;
})

MLN_FUNC(, int, mln_websocket_parse, (mln_websocket_t *ws, mln_chain_t **in), (ws, in), {
    mln_chain_t *c = *in;
    mln_u8ptr_t p = NULL, end = NULL, content = NULL;
    mln_u64_t len, i, tmp;
    mln_u32_t masking_key = 0;
    mln_u8_t b1 = 0, b2 = 0;

    for (i = 0; c != NULL; c = c->next) {
        if (c->buf == NULL || mln_buf_left_size(c->buf) == 0) continue;
        p = c->buf->left_pos;
        for (end = c->buf->end; p < end; ++p) {
             if (i == 0) {
                 b1 = *p;
                 ++i;
             } else {
                 b2 = *p++;
                 ++i;
                 break;
             }
        }
        if (i >= 2) break;
    }
    if (i < 2) return M_WS_RET_NOTYET;

    len = b2 & 0x7f;
    if (len == 127) {
        tmp = p - end;
        if (tmp > 8) tmp = 8;
        len = 0;
        i = 0;
again127:
        for (; i < tmp; ++i) {
            len |= ((*p++) << ((7 - i)<<3));
        }
        if (tmp < 8) {
            for (c = c->next; c != NULL; c = c->next) {
                if (c->buf == NULL || mln_buf_left_size(c->buf) == 0) continue;
                p = c->buf->left_pos;
                end = c->buf->end;
                break;
            }
            if (c == NULL) return M_WS_RET_NOTYET;
            tmp = (8-tmp > p-end)? tmp+(mln_u32_t)(p-end): 8;
            goto again127;
        }
    } else if (len == 126) {
        tmp = p - end;
        if (tmp > 2) tmp = 2;
        len = 0;
        i = 0;
again126:
        for (; i < tmp; ++i) {
            len |= ((*p++) << ((1 - i)<<3));
        }
        if (tmp < 2) {
            for (c = c->next; c != NULL; c = c->next) {
                if (c->buf == NULL || mln_buf_left_size(c->buf) == 0) continue;
                p = c->buf->left_pos;
                end = c->buf->end;
                break;
            }
            if (c == NULL) return M_WS_RET_NOTYET;
            tmp = (2-tmp > p-end)? tmp+(mln_u32_t)(p-end): 2;
            goto again126;
        }
    }

    if (b2 & 0x80) {
        tmp = p - end;
        if (tmp > 4) tmp = 4;
        i = 0;
againm:
        for (; i < tmp; ++i) {
            masking_key |= ((*p++) << ((3 - i) << 3));
        }
        if (tmp < 4) {
            for (c = c->next; c != NULL; c = c->next) {
                if (c->buf == NULL || mln_buf_left_size(c->buf) == 0) continue;
                p = c->buf->left_pos;
                end = c->buf->end;
                break;
            }
            if (c == NULL) return M_WS_RET_NOTYET;
            tmp = (4-tmp > p-end)? tmp+(mln_u32_t)(p-end): 4;
            goto againm;
        }
    }

    if (len) {
        if ((b1 & 0xf) >= M_WS_OPCODE_CLOSE && len > 125) {
            return M_WS_RET_ERROR;
        }
        if ((b1&0xf) == M_WS_OPCODE_CLOSE && len > 1) {
            mln_u16_t status = 0;

            tmp = p - end;
            if (tmp > 2) tmp = 2;
            i = 0;
against:
            for (; i < tmp; ++i) {
                status |= ((*p++) << ((1 - i)<<3));
            }
            if (tmp < 2) {
                for (c = c->next; c != NULL; c = c->next) {
                    if (c->buf == NULL || mln_buf_left_size(c->buf) == 0) continue;
                    p = c->buf->left_pos;
                    end = c->buf->end;
                    break;
                }
                if (c == NULL) return M_WS_RET_NOTYET;
                tmp = (2-tmp > p-end)? tmp+(mln_u32_t)(p-end): 2;
                goto against;
            }

            mln_websocket_set_status(ws, status);
            len -= 2;
        } else {
            mln_websocket_set_status(ws, 0);
        }
        content = (mln_u8ptr_t)mln_alloc_m(mln_websocket_get_pool(ws), len);
        if (content == NULL) return M_WS_RET_FAILED;
        tmp = p - end;
        if (tmp > len) tmp = len;
        i = 0;
againc:
        if (tmp > 0) memcpy(content+i, p, tmp);
        i += tmp;
        p += tmp;
        if (tmp < len) {
            for (c = c->next; c != NULL; c = c->next) {
                if (c->buf == NULL || mln_buf_left_size(c->buf) == 0) continue;
                p = c->buf->left_pos;
                end = c->buf->end;
                break;
            }
            if (c == NULL) {
                mln_alloc_free(content);
                return M_WS_RET_NOTYET;
            }
            tmp = (len-tmp > p-end)? p-end: len-tmp;
            goto againc;
        }
    }

    if (mln_websocket_get_content_free(ws)) {
        mln_alloc_free(mln_websocket_get_content(ws));
        mln_websocket_reset_content_free(ws);
    }
    mln_websocket_set_content(ws, content);
    if (content != NULL) mln_websocket_set_content_free(ws);
    mln_websocket_set_content_len(ws, len);
    if (b1 & 0x80) mln_websocket_set_fin(ws);
    else mln_websocket_reset_fin(ws);
    if (b1 & 0x40) mln_websocket_set_rsv1(ws);
    else mln_websocket_reset_rsv1(ws);
    if (b1 & 0x20) mln_websocket_set_rsv2(ws);
    else mln_websocket_reset_rsv2(ws);
    if (b1 & 0x10) mln_websocket_set_rsv3(ws);
    else mln_websocket_reset_rsv3(ws);
    mln_websocket_set_opcode(ws, b1&0xf);
    if (b2 & 0x80) mln_websocket_set_maskbit(ws);
    else mln_websocket_reset_maskbit(ws);
    if (b2 & 0x80) mln_websocket_set_masking_key(ws, masking_key);
    else mln_websocket_set_masking_key(ws, 0);

    if (mln_websocket_get_maskbit(ws)) {
        mln_u8_t tmpkey[4];
        mln_u16_t close_status = mln_websocket_get_status(ws);
        tmpkey[0] = (masking_key >> 24) & 0xff;
        tmpkey[1] = (masking_key >> 16) & 0xff;
        tmpkey[2] = (masking_key >> 8) & 0xff;
        tmpkey[3] = masking_key & 0xff;
        if (mln_websocket_get_opcode(ws) == M_WS_OPCODE_CLOSE && close_status != 0) {
            mln_u16_t unmasked_status = (mln_u16_t)((((close_status >> 8) & 0xff) ^ tmpkey[0]) << 8) | \
                                        (mln_u16_t)(((close_status & 0xff) ^ tmpkey[1]) & 0xff);
            if (!mln_websocket_is_valid_status_code(unmasked_status)) {
                if (content != NULL) mln_alloc_free(content);
                return M_WS_RET_ERROR;
            }
            mln_websocket_set_status(ws, unmasked_status);
        }
        if (content != NULL && len > 0) {
            mln_u32_t unmask_key = masking_key;
            if (mln_websocket_get_opcode(ws) == M_WS_OPCODE_CLOSE && close_status != 0) {
                /* 2 status bytes consumed 2 mask positions; rotate key by 2 to continue */
                unmask_key = (masking_key << 16) | (masking_key >> 16);
            }
            mln_websocket_unmask_xor4(content, len, unmask_key);
        }
    } else {
        if ((b1 & 0x40) || (b1 & 0x20) || (b1 & 0x10)) {
            if (mln_websocket_get_ext_handler(ws) == NULL) {
                if (content != NULL) mln_alloc_free(content);
                return M_WS_RET_ERROR;
            }
        }
        if ((b1 & 0xf) == M_WS_OPCODE_CLOSE && mln_websocket_get_status(ws) != 0) {
            if (!mln_websocket_is_valid_status_code(mln_websocket_get_status(ws))) {
                if (content != NULL) mln_alloc_free(content);
                return M_WS_RET_ERROR;
            }
        }
    }

    if ((b1 & 0xf) == M_WS_OPCODE_TEXT && content != NULL && len > 0) {
        if (!mln_websocket_is_valid_utf8(content, len)) {
            if (content != NULL) mln_alloc_free(content);
            return M_WS_RET_ERROR;
        }
    }

    if ((b1 & 0xf) >= M_WS_OPCODE_CLOSE && !mln_websocket_get_fin(ws)) {
        if (content != NULL) mln_alloc_free(content);
        return M_WS_RET_ERROR;
    }

    if (mln_websocket_get_ext_handler(ws) != NULL) {
        int ret = mln_websocket_get_ext_handler(ws)(ws);
        if (ret != M_WS_RET_OK) return ret;
    }

    if (c != NULL && c->buf != NULL) c->buf->left_pos = p;
    for (; c != NULL; c = c->next) {
        if (c->buf == NULL || mln_buf_left_size(c->buf)==0) continue;
        break;
    }

    if (c == NULL) {
        mln_chain_pool_release_all(*in);
        *in = NULL;
    } else {
        if (c != *in) {
            mln_chain_t *tmpc = *in;
            *in = c;
            for (c = tmpc; c->next != *in; c = c->next)
                ;
            c->next = NULL;
            mln_chain_pool_release_all(tmpc);
        }
    }

    return M_WS_RET_OK;
})

