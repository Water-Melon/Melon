## Websocket



### Header file

```c
#include "mln_websocket.h"
```



### Module

`websocket`



### Functions/Macros



#### mln_websocket_init

```c
int mln_websocket_init(mln_websocket_t *ws, mln_http_t *http);
```

Description: Initialize the websocket structure `ws`, which depends on the `mln_http_t` structure `http`. ws may be dynamically allocated, or it may be an automatic variable on the stack.

Return value: return `0` if successful, otherwise return `-1`



#### mln_websocket_new

```c
mln_websocket_t *mln_websocket_new(mln_http_t *http);
```

Description: Create and initialize the `mln_websocket_t` structure, which depends on the `mln_http_t` structure `http`.

Return value: return the structure pointer if successful, otherwise return `NULL`



#### mln_websocket_destroy

```c
void mln_websocket_destroy(mln_websocket_t *ws);
```

Description: Destroy the `mln_websocket_t` structure, `ws` should be initialized by `mln_websocket_init`.

Return value: none



#### mln_websocket_free

```c
void mln_websocket_free(mln_websocket_t *ws);
```

Description: Destroy and free the `mln_websocket_t` structure `ws`.

Return value: none



#### mln_websocket_reset

```c
void mln_websocket_reset(mln_websocket_t *ws);
```

Description: Reset everything inside `ws`.

Return value: none



#### mln_websocket_is_websocket

```c
int mln_websocket_is_websocket(mln_http_t *http);
```

Description: Determine whether this `http` is a websocket.

return value:

- `M_WS_RET_NOTWS` is not a websocket
- `M_WS_RET_ERROR` is not an HTTP request
- `M_WS_RET_OK` is websocket



#### mln_websocket_validate

```c
int mln_websocket_validate(mln_websocket_t *ws);
```

- Description: Determine if `ws` is a valid websocket connection.

  return value:

  - `M_WS_RET_NOTWS` no
  - `M_WS_RET_ERROR` is not an HTTP response
  - `M_WS_RET_OK` yes



#### mln_websocket_set_field

```c
int mln_websocket_set_field(mln_websocket_t *ws, mln_string_t *key, mln_string_t *val);
```

Description: Set the HTTP header for the first interaction of the websocket. If `key` exists, the replacement value is `val`.

return value:

- `M_WS_RET_FAILED` on failure
- `M_WS_RET_OK` on success



#### mln_websocket_get_field

```c
mln_string_t *mln_websocket_get_field(mln_websocket_t *ws, mln_string_t *key);
```

Description: Get the value of the HTTP header field `key` of the first websocket interaction.

Return value: Returns a pointer of type `mln_string_t` if it exists, otherwise returns `NULL`



#### mln_websocket_match

```c
int mln_websocket_match(mln_websocket_t *ws);
```

Description: Compare the header fields in the websocket with the header fields in the http structure.

return value:

- `M_WS_RET_ERROR` on failure
- `M_WS_RET_OK` on success



#### mln_websocket_handshake_response_generate

```c
int mln_websocket_handshake_response_generate(mln_websocket_t *ws, mln_chain_t **chead, mln_chain_t **ctail);
```

Description: Generate a websocket handshake response message, the content of the message is the content in the linked list specified by `chead` and `ctail`.

return value:

- `M_WS_RET_FAILED` on failure
- `M_WS_RET_OK` on success



#### mln_websocket_handshake_request_generate

```c
int mln_websocket_handshake_request_generate(mln_websocket_t *ws, mln_chain_t **chead, mln_chain_t **ctail);
```

Description: Generate a websocket handshake request message, the content of the message is the content in the linked list specified by `chead` and `ctail`.

return value:

- `M_WS_RET_FAILED` on failure
- `M_WS_RET_OK` on success



#### mln_websocket_text_generate

```c
int mln_websocket_text_generate(mln_websocket_t *ws, mln_chain_t **out_cnode, mln_u8ptr_t buf, mln_size_t len, mln_u32_t flags);
```

Description: Generates a text data frame. `out_cnode` is the generated frame data, `buf` and `len` are the text content. flags has several values, and these values can be assigned together using the OR operator:

- `M_WS_FLAG_NONE` has no meaning
- `M_WS_FLAG_NEW` marks the first data fragment
- `M_WS_FLAG_END` marks the last data fragment
- `M_WS_FLAG_CLIENT` is flagged for client generation
- `M_WS_FLAG_SERVER` flag for server generation

return value:

- `M_WS_RET_ERROR` data is wrong
- `M_WS_RET_FAILED` on failure, e.g. out of memory, etc.
- `M_WS_RET_OK` on success



#### mln_websocket_binary_generate

```c
int mln_websocket_binary_generate(mln_websocket_t *ws, mln_chain_t **out_cnode, void *buf, mln_size_t len, mln_u32_t flags);
```

Description: Generates a binary data frame. `out_cnode` is the generated frame data, `buf` and `len` are binary data. flags has several values, and these values can be assigned together using the OR operator:

- `M_WS_FLAG_NONE` has no meaning
- `M_WS_FLAG_NEW` marks the first data fragment
- `M_WS_FLAG_END` marks the last data fragment
- `M_WS_FLAG_CLIENT` is flagged for client generation
- `M_WS_FLAG_SERVER` flag for server generation

return value:

- `M_WS_RET_ERROR` data is wrong
- `M_WS_RET_FAILED` on failure, e.g. out of memory, etc.
- `M_WS_RET_OK` on success



#### mln_websocket_close_generate

```c
int mln_websocket_close_generate(mln_websocket_t *ws, mln_chain_t **out_cnode, char *reason, mln_u16_t status, mln_u32_t flags);
```

Description: Generate a websocket close message. `out_cnode` is the message generated by this function, `reason` is the shutdown reason, and `status` is the shutdown status word:

- `M_WS_STATUS_NORMAL_CLOSURE`
- `M_WS_STATUS_GOING_AWAY`
- `M_WS_STATUS_PROTOCOL_ERROR`
- `M_WS_STATUS_UNSOPPORTED_DATA`
- `M_WS_STATUS_RESERVED`
- `M_WS_STATUS_NO_STATUS_RCVD`
- `M_WS_STATUS_ABNOMAIL_CLOSURE`
- `M_WS_STATUS_INVALID_PAYLOAD_DATA`
- `M_WS_STATUS_POLICY_VIOLATION`
- `M_WS_STATUS_MESSAGE_TOO_BIG`
- `M_WS_STATUS_MANDATORY_EXT`
- `M_WS_STATUS_INTERNAL_SERVER_ERROR`
- `M_WS_STATUS_TLS_HANDSHAKE`

flags has several values, and these values can be assigned together using the OR operator:

- `M_WS_FLAG_NONE` has no meaning
- `M_WS_FLAG_NEW` marks the first data fragment
- `M_WS_FLAG_END` marks the last data fragment
- `M_WS_FLAG_CLIENT` is flagged for client generation
- `M_WS_FLAG_SERVER` flag for server generation

return value:

- `M_WS_RET_ERROR` data is wrong
- `M_WS_RET_FAILED` on failure, e.g. out of memory, etc.
- `M_WS_RET_OK` on success



#### mln_websocket_ping_generate

```c
int mln_websocket_ping_generate(mln_websocket_t *ws, mln_chain_t **out_cnode, mln_u32_t flags);
```

Description: Generate a ping message, `out_cnode` is the generated message, flags has several values, and these values can be assigned together with the OR operator:

- `M_WS_FLAG_NONE` has no meaning
- `M_WS_FLAG_NEW` marks the first data fragment
- `M_WS_FLAG_END` marks the last data fragment
- `M_WS_FLAG_CLIENT` is flagged for client generation
- `M_WS_FLAG_SERVER` flag for server generation

return value:

- `M_WS_RET_ERROR` data is wrong
- `M_WS_RET_FAILED` on failure, e.g. out of memory, etc.
- `M_WS_RET_OK` on success



#### mln_websocket_pong_generate

```c
int mln_websocket_pong_generate(mln_websocket_t *ws, mln_chain_t **out_cnode, mln_u32_t flags);
```

Description: Generate a pong message, `out_cnode` is the generated message, flags has several values, and these values can be assigned together using the OR operator:

- `M_WS_FLAG_NONE` has no meaning
- `M_WS_FLAG_NEW` marks the first data fragment
- `M_WS_FLAG_END` marks the last data fragment
- `M_WS_FLAG_CLIENT` is flagged for client generation
- `M_WS_FLAG_SERVER` flag for server generation

return value:

- `M_WS_RET_ERROR` data is wrong
- `M_WS_RET_FAILED` on failure, e.g. out of memory, etc.
- `M_WS_RET_OK` on success



#### mln_websocket_generate

```c
int mln_websocket_generate(mln_websocket_t *ws, mln_chain_t **out_cnode);
```

Description: Generate a message based on the current contents of the `ws` structure. The above generated functions are all upper-level encapsulations of this function. `out_cnode` is the generated message. When you want to customize the sending format, you need to set some members of `ws` with the subsequent macro functions.

return value:

- `M_WS_RET_ERROR` data is wrong
- `M_WS_RET_FAILED` on failure, e.g. out of memory, etc.
- `M_WS_RET_OK` on success



#### mln_websocket_parse

```c
int mln_websocket_parse(mln_websocket_t *ws, mln_chain_t **in);
```

Description: Parse the data in `in` and put the data into the corresponding position in `ws`.

return value:

- `M_WS_RET_ERROR` message error
- `M_WS_RET_OK` parsed successfully
- `M_WS_RET_FAILED` parse failed, such as out of memory, etc.
- `M_WS_RET_NOTYET` on success but the data is incomplete and needs to continue processing



#### mln_websocket_get_http

```c
mln_websocket_get_http(ws)
```

Description: Get the `http` structure to which `ws` belongs.

Return value: `mln_http_t` type pointer



#### mln_websocket_get_pool

```c
mln_websocket_get_pool(ws)
```

Description: Get the memory pool structure used by `ws`.

Return value: pointer of type `mln_alloc_t`



#### mln_websocket_get_connection

```c
mln_websocket_get_connection(ws)
```

Description: Get the TCP link structure used by `ws`.

Return value: `mln_tcp_conn_t` type pointer



#### mln_websocket_get_uri

```c
mln_websocket_get_uri(ws)
```

Description: Get the current HTTP URI of `ws`.

Return value: pointer of type `mln_string_t`



#### mln_websocket_set_uri

```c
mln_websocket_set_uri(ws,u)
```

Description: Set the URI to be accessed by `ws` to `u` of type `mln_string_t` pointer.

Return value: none



#### mln_websocket_get_args

```c
mln_websocket_get_args(ws)
```

Description: Get HTTP request parameters for `ws`.

Return value: pointer of type `mln_string_t`



#### mln_websocket_set_args

```c
mln_websocket_set_args(ws,a)
```

Description: Set the HTTP request parameter of `ws` to `a` of type `mln_string_t` pointer.

Return value: none



#### mln_websocket_get_key

```c
mln_websocket_get_key(ws)
```

Description: Get the value of the HTTP header field `Sec-Websocket-Key`.

Return value: pointer of type `mln_string_t`



#### mln_websocket_set_key

```c
 mln_websocket_set_key(ws,k)
```

Description: Set the value of the websocket HTTP header field `Sec-Websocket-Key` to a `mln_string_t` type pointer `k`.

Return value: none



#### mln_websocket_set_data

```c
mln_websocket_set_data(ws,d)
```

Description: Set user-defined structure `d`.

Return value: none



#### mln_websocket_get_data

```c
mln_websocket_get_data(ws)
```

Description: Get user-defined structure.

Return value: custom structure pointer



#### mln_websocket_set_content

```c
mln_websocket_set_content(ws,c)
```

Description: Set the data frame. The data type of `c` depends on the content to be sent. It can be a structure, an integer, or other data types. This function is only used when customizing websocket messages.

Return value: none



#### mln_websocket_get_content

```c
mln_websocket_get_content(ws)
```

Description: Get the data frame content.

Return value: `void *` type data



#### mln_websocket_set_content_len

```c
mln_websocket_set_content_len(ws,l)
```

Description: Set the data length.

Return value: none



#### mln_websocket_get_content_len

```c
mln_websocket_get_content_len(ws)
```

Description: Get the data length.

Return value: unsigned integer



#### mln_websocket_set_ext_handler

```c
mln_websocket_set_ext_handler(ws,h)

typedef int (*mln_ws_extension_handle)(mln_websocket_t *);
```

Description: Set extension handler `h` to `ws`. This function will be called in `mln_websocket_generate` and `mln_websocket_parse` to add a custom processing content.

Return value: none



#### mln_websocket_get_ext_handler

```c
mln_websocket_get_ext_handler(ws)
```

Description: Get the extension handler. This function will be called in `mln_websocket_generate` and `mln_websocket_parse` to add a custom processing content.

Return value: pointer of type `mln_ws_extension_handle`



#### mln_websocket_set_rsv1

```c
 mln_websocket_set_rsv1(ws)
```

Description: Set reserved bit rsv1 bit 1.

Return value: none



#### mln_websocket_reset_rsv1

```c
mln_websocket_reset_rsv1(ws)
```

Description: Reset reserved bit rsv1.

Return value: none



#### mln_websocket_get_rsv1

```c
mln_websocket_get_rsv1(ws)
```

Description: Get the value of reserved bit rsv1.

Return value: unsigned integer value



#### mln_websocket_set_rsv2

```c
mln_websocket_set_rsv2(ws)
```

Description: Set reserved bit rsv2 bit 1.

Return value: none



#### mln_websocket_reset_rsv2

```c
mln_websocket_reset_rsv2(ws)
```

Description: Reset reserved bit rsv2.

Return value: none



#### mln_websocket_get_rsv2

```c
mln_websocket_get_rsv2(ws)
```

Description: Get the value of reserved bit rsv2.

Return value: unsigned integer value



#### mln_websocket_set_rsv3

```c
mln_websocket_set_rsv3(ws)
```

Description: Set reserved bit rsv3 bit 1.

Return value: none



#### mln_websocket_reset_rsv3

```c
mln_websocket_reset_rsv3(ws)
```

Description: Reset reserved bit rsv3.

Return value: none



#### mln_websocket_get_rsv3

```c
mln_websocket_get_rsv3(ws)
```

Description: Get the value of reserved bit rsv3.

Return value: unsigned integer value



#### mln_websocket_set_opcode

```c
mln_websocket_set_opcode(ws,op)
```

Description: Set the opcode, which is used to tell the `mln_websocket_generate` function what type of message to generate. The value of `op` is as follows:

- `M_WS_OPCODE_CONTINUE` Since the type is set in the first packet, subsequent data are all of this type.
- `M_WS_OPCODE_TEXT` text type
- `M_WS_OPCODE_BINARY` binary type
- `M_WS_OPCODE_CLOSE` disconnect
- `M_WS_OPCODE_PING` liveness check request
- `M_WS_OPCODE_PONG` liveness check response

Return value: none



#### mln_websocket_get_opcode

```c
mln_websocket_get_opcode(ws)
```

Description: Get the opcode. For details of the opcode, see the `mln_websocket_set_opcode` function.

Return value: unsigned integer



#### mln_websocket_set_status

```c
mln_websocket_set_status(ws,s)

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
```

Description: Set the websocket status code, the value of the status code is shown in the macro definition.

Return value: none



#### mln_websocket_get_status

```c
mln_websocket_get_status(ws)
```

Description: Get the websocket status code.

Return value: unsigned integer



#### mln_websocket_set_content_free

```c
mln_websocket_set_content_free(ws)
```

Description: Set the `content_free` flag. This flag controls whether to free the memory of the content member (data frame data) when the `ws` structure is destroyed or reset, which must be allocated from the `ws` memory pool.

Return value: none



#### mln_websocket_reset_content_free

```c
mln_websocket_reset_content_free(ws)
```

Description: Reset the `content_free` flag value.

Return value: none



#### mln_websocket_get_content_free

```c
mln_websocket_get_content_free(ws)
```

Description: Get the `content_free` tag value.

Return value: unsigned integer



#### mln_websocket_set_fin

```c
mln_websocket_set_fin(ws)
```

Description: Set the `fin` flag, which controls whether it is the last data frame.

Return value: none



#### mln_websocket_reset_fin

```c
mln_websocket_reset_fin(ws)
```

Description: Reset the `fin` flag.

Return value: none



#### mln_websocket_get_fin

```c
mln_websocket_get_fin(ws)
```

Description: Get the value of `fin`.

Return value: unsigned integer



#### mln_websocket_set_maskbit

```c
mln_websocket_set_maskbit(ws)
```

Description: Set the `mask` bit of websocket packets. When set, it will carry a 4-byte masking-key.

Return value: none



#### mln_websocket_reset_maskbit

```c
mln_websocket_reset_maskbit(ws)
```

Description: Reset the `mask` bit of websocket packets.

Return value: none



#### mln_websocket_get_maskbit

```c
mln_websocket_get_maskbit(ws)
```

Description: Get the `mask` bit of the websocket packet.

Return value: unsigned integer



#### mln_websocket_set_masking_key

```c
mln_websocket_set_masking_key(ws,k)
```

Description: Set the `masking-key` field to `k`. `k` is an unsigned 32-bit integer.

Return value: none



#### mln_websocket_get_masking_key

```c
mln_websocket_get_masking_key(ws)
```

Description: Get the `masking-key` value of websocket.

Return value: unsigned 32-bit integer



### Example

```c
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include "mln_framework.h"
#include "mln_log.h"
#include "mln_websocket.h"


static void mln_accept(mln_event_t *ev, int fd, void *data);
static int mln_http_recv_body_handler(mln_http_t *http, mln_chain_t **in, mln_chain_t **nil);
static void mln_recv(mln_event_t *ev, int fd, void *data);
static void mln_quit(mln_event_t *ev, int fd, void *data);
static void mln_send(mln_event_t *ev, int fd, void *data);
static int mln_http_send_body_handler(mln_http_t *http, mln_chain_t **body_head, mln_chain_t **body_tail);
static void mln_websocket_send(mln_event_t *ev, int fd, void *data);
static void mln_websocket_recv(mln_event_t *ev, int fd, void *data);

static void worker_process(mln_event_t *ev)
{
    mln_u16_t port = 1234;
    mln_s8_t ip[] = "0.0.0.0";
    struct sockaddr_in addr;
    int val = 1;
    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd < 0) {
        mln_log(error, "listen socket error\n");
        return;
    }
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val)) < 0) {
        mln_log(error, "setsockopt error\n");
        close(listenfd);
        return;
    }
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(ip);
    if (bind(listenfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        mln_log(error, "bind error\n");
        close(listenfd);
        return;
    }
    if (listen(listenfd, 511) < 0) {
        mln_log(error, "listen error\n");
        close(listenfd);
        return;
    }

    if (mln_event_fd_set(ev, \
                         listenfd, \
                         M_EV_RECV|M_EV_NONBLOCK, \
                         M_EV_UNLIMITED, \
                         NULL, \
                         mln_accept) < 0)
    {
        mln_log(error, "listen sock set event error\n");
        close(listenfd);
        return;
    }
}


static void mln_accept(mln_event_t *ev, int fd, void *data)
{
    mln_tcp_conn_t *connection;
    mln_http_t *http;
    int connfd;
    socklen_t len;
    struct sockaddr_in addr;

    while (1) {
        memset(&addr, 0, sizeof(addr));
        len = sizeof(addr);
        connfd = accept(fd, (struct sockaddr *)&addr, &len);
        if (connfd < 0) {
            if (errno == EAGAIN) break;
            if (errno == EINTR) continue;
            perror("accept");
            exit(1);
        }

        connection = (mln_tcp_conn_t *)malloc(sizeof(mln_tcp_conn_t));
        if (connection == NULL) {
            fprintf(stderr, "3No memory.\n");
            close(connfd);
            continue;
        }
        if (mln_tcp_conn_init(connection, connfd) < 0) {
            fprintf(stderr, "4No memory.\n");
            close(connfd);
            free(connection);
            continue;
        }

        http = mln_http_init(connection, NULL, mln_http_recv_body_handler);
        if (http == NULL) {
            fprintf(stderr, "5No memory.\n");
            mln_tcp_conn_destroy(connection);
            free(connection);
            close(connfd);
            continue;
        }

        if (mln_event_fd_set(ev, \
                             connfd, \
                             M_EV_RECV|M_EV_NONBLOCK, \
                             M_EV_UNLIMITED, \
                             http, \
                             mln_recv) < 0)
        {
            fprintf(stderr, "6No memory.\n");
            mln_http_destroy(http);
            mln_tcp_conn_destroy(connection);
            free(connection);
            close(connfd);
            continue;
        }
    }
}

static void mln_quit(mln_event_t *ev, int fd, void *data)
{
    mln_http_t *http = (mln_http_t *)data;
    mln_tcp_conn_t *connection = mln_http_connection_get(http);

    mln_event_fd_set(ev, fd, M_EV_CLR, M_EV_UNLIMITED, NULL, NULL);
    mln_http_destroy(http);
    mln_tcp_conn_destroy(connection);
    free(connection);
    close(fd);
}

static void mln_recv(mln_event_t *ev, int fd, void *data)
{
    mln_http_t *http = (mln_http_t *)data;
    mln_tcp_conn_t *connection = mln_http_connection_get(http);
    int ret, rc = M_HTTP_RET_DONE;
    mln_chain_t *c;

    ret = mln_tcp_conn_recv(connection, M_C_TYPE_MEMORY);
    /* debug info */
    printf("recv:\n");
    mln_chain_t *cc = mln_tcp_conn_head(connection, M_C_RECV);
    for (; cc != NULL; cc = cc->next) {
        if (cc->buf == NULL || mln_buf_left_size(cc->buf)==0) continue;
        write(STDOUT_FILENO, cc->buf->left_pos, mln_buf_left_size(cc->buf));
    }
    printf("recv end\n");
    /* debug info end */
    c = mln_tcp_conn_remove(connection, M_C_RECV);
    if (c != NULL) {
        rc = mln_http_parse(http, &c);
        if (c != NULL) mln_tcp_conn_append_chain(connection, c, NULL, M_C_RECV);
    }
    if (ret == M_C_ERROR || ret == M_C_CLOSED || rc == M_HTTP_RET_ERROR) {
        mln_quit(ev, fd, data);
        return;
    }
    if (rc == M_HTTP_RET_OK) {
        return;
    }

    if (mln_websocket_is_websocket(http) == M_WS_RET_OK) {
        mln_string_t ext = mln_string("Sec-WebSocket-Extensions");
        mln_http_field_remove(http, &ext);
        mln_websocket_t *ws = mln_websocket_new(http);
        if (ws == NULL) {
            mln_quit(ev, fd, data);
            return;
        }
        mln_chain_t *body_head = NULL, *body_tail = NULL;
        if (mln_websocket_handshake_response_generate(ws, &body_head, &body_tail) == M_WS_RET_ERROR) {
            mln_websocket_free(ws);
            mln_quit(ev, fd, data);
            return;
        }
        mln_tcp_conn_append_chain(connection, body_head, body_tail, M_C_SEND);
        mln_websocket_send(ev, fd, ws);
    } else {
        mln_http_reset(http);
        mln_http_status_set(http, M_HTTP_OK);
        mln_http_version_set(http, M_HTTP_VERSION_1_1);
        mln_http_type_set(http, M_HTTP_RESPONSE);
        mln_http_handler_set(http, mln_http_send_body_handler);
        mln_chain_t *body_head = NULL, *body_tail = NULL;
        if (mln_http_generate(http, &body_head, &body_tail) == M_HTTP_RET_ERROR) {
            fprintf(stderr, "mln_http_generate() failed. %u\n", mln_http_error_get(http));
            mln_quit(ev, fd, data);
            return;
        }
        mln_tcp_conn_append_chain(connection, body_head, body_tail, M_C_SEND);
        mln_send(ev, fd, data);
    }
}

static int mln_http_recv_body_handler(mln_http_t *http, mln_chain_t **in, mln_chain_t **nil)
{
    mln_u32_t method = mln_http_method_get(http);
    if (method == M_HTTP_GET)
        return M_HTTP_RET_DONE;
    mln_http_error_set(http, M_HTTP_NOT_IMPLEMENTED);
    return M_HTTP_RET_ERROR;
}

static void mln_send(mln_event_t *ev, int fd, void *data)
{
    mln_http_t *http = (mln_http_t *)data;
    mln_tcp_conn_t *connection = mln_http_connection_get(http);
    mln_chain_t *c;
    int ret;

    while ((c = mln_tcp_conn_head(connection, M_C_SEND)) != NULL) {
        ret = mln_tcp_conn_send(connection);
        if (ret == M_C_FINISH) {
            mln_quit(ev, fd, data);
            break;
        } else if (ret == M_C_NOTYET) {
            mln_chain_pool_release_all(mln_tcp_conn_remove(connection, M_C_SENT));
            mln_event_fd_set(ev, fd, M_EV_SEND|M_EV_APPEND|M_EV_NONBLOCK, M_EV_UNLIMITED, data, mln_send);
            return;
        } else if (ret == M_C_ERROR) {
            mln_quit(ev, fd, data);
            return;
        } else {
            fprintf(stderr, "Shouldn't be here.\n");
            abort();
        }
    }
}

static void mln_websocket_send(mln_event_t *ev, int fd, void *data)
{
    mln_websocket_t *ws = (mln_websocket_t *)data;
    mln_tcp_conn_t *connection = mln_websocket_get_connection(ws);
    mln_chain_t *c;
    int ret;

    while ((c = mln_tcp_conn_head(connection, M_C_SEND)) != NULL) {
        ret = mln_tcp_conn_send(connection);
        if (ret == M_C_FINISH) {
            continue;
        } else if (ret == M_C_NOTYET) {
            break;
        } else if (ret == M_C_ERROR) {
            mln_http_t *http = mln_websocket_get_http(ws);
            mln_websocket_free(ws);
            mln_quit(ev, fd, http);
            return;
        } else {
            fprintf(stderr, "Shouldn't be here.\n");
            abort();
        }
    }
    if (c == NULL) {
        mln_chain_pool_release_all(mln_tcp_conn_remove(connection, M_C_SENT));
        mln_event_fd_set(ev, fd, M_EV_RECV|M_EV_NONBLOCK, M_EV_UNLIMITED, data, mln_websocket_recv);
    } else {
        mln_chain_pool_release_all(mln_tcp_conn_remove(connection, M_C_SENT));
        mln_event_fd_set(ev, fd, M_EV_SEND|M_EV_APPEND|M_EV_NONBLOCK|M_EV_ONESHOT, M_EV_UNLIMITED, data, mln_send);
    }
}

static void mln_websocket_recv(mln_event_t *ev, int fd, void *data)
{
    mln_websocket_t *ws = (mln_websocket_t *)data;
    mln_http_t *http = mln_websocket_get_http(ws);
    mln_tcp_conn_t *connection = mln_http_connection_get(ws);
    int ret, rc;
    mln_chain_t *c;

    ret = mln_tcp_conn_recv(connection, M_C_TYPE_MEMORY);
lp:
    /* debug info */
    printf("websocket recv:[");
    mln_chain_t *cc = mln_tcp_conn_head(connection, M_C_RECV);
    mln_u8ptr_t uc;
    for (; cc != NULL; cc = cc->next) {
        if (cc->buf == NULL || mln_buf_left_size(cc->buf)==0) continue;
        for (uc = cc->buf->left_pos; uc < cc->buf->end; uc++) {
            printf("%02x ", *uc);
        }
    }
    printf("]\n");
    /* debug info end */
    c = mln_tcp_conn_remove(connection, M_C_RECV);
    rc = mln_websocket_parse(ws, &c);
    if (c != NULL) mln_tcp_conn_append_chain(connection, c, NULL, M_C_RECV);
    if (ret == M_C_ERROR || ret == M_C_CLOSED || rc == M_WS_RET_ERROR || rc == M_WS_RET_FAILED) {
        mln_websocket_free(ws);
        mln_quit(ev, fd, http);
        return;
    }
    if (rc == M_WS_RET_NOTYET) return;
    /* debug info */
    if (mln_websocket_get_content(ws) != NULL)
        write(STDOUT_FILENO, mln_websocket_get_content(ws), mln_websocket_get_content_len(ws));
    printf("\n%d\nc:%lx\n", rc, (unsigned long)c);
    /* debug info end */
    if (mln_websocket_get_opcode(ws) == M_WS_OPCODE_TEXT) {
        mln_chain_t *out = NULL;
        if (mln_websocket_text_generate(ws, &out, (mln_u8ptr_t)"hello", 5, M_WS_FLAG_NEW|M_WS_FLAG_SERVER) != M_WS_RET_OK) {
            mln_log(error, "text generate error.\n");
            mln_websocket_free(ws);
            mln_quit(ev, fd, http);
            return;
        }
        mln_tcp_conn_append(connection, out, M_C_SEND);
        if (mln_websocket_text_generate(ws, &out, (mln_u8ptr_t)"world", 5, M_WS_FLAG_NONE|M_WS_FLAG_SERVER) != M_WS_RET_OK) {
            mln_log(error, "text generate error.\n");
            mln_websocket_free(ws);
            mln_quit(ev, fd, http);
            return;
        }
        mln_tcp_conn_append(connection, out, M_C_SEND);
        if (mln_websocket_text_generate(ws, &out, (mln_u8ptr_t)"nobody", 6, M_WS_FLAG_END|M_WS_FLAG_SERVER) != M_WS_RET_OK) {
            mln_log(error, "text generate error.\n");
            mln_websocket_free(ws);
            mln_quit(ev, fd, http);
            return;
        }
        mln_tcp_conn_append(connection, out, M_C_SEND);
        if (mln_websocket_binary_generate(ws, &out, (mln_u8ptr_t)"a", 1, M_WS_FLAG_NEW|M_WS_FLAG_SERVER) != M_WS_RET_OK) {
            mln_log(error, "text generate error.\n");
            mln_websocket_free(ws);
            mln_quit(ev, fd, http);
            return;
        }
        mln_tcp_conn_append(connection, out, M_C_SEND);
        if (mln_websocket_binary_generate(ws, &out, (mln_u8ptr_t)"b", 1, M_WS_FLAG_NONE|M_WS_FLAG_SERVER) != M_WS_RET_OK) {
            mln_log(error, "text generate error.\n");
            mln_websocket_free(ws);
            mln_quit(ev, fd, http);
            return;
        }
        mln_tcp_conn_append(connection, out, M_C_SEND);
        if (mln_websocket_binary_generate(ws, &out, (mln_u8ptr_t)"c", 1, M_WS_FLAG_END|M_WS_FLAG_SERVER) != M_WS_RET_OK) {
            mln_log(error, "text generate error.\n");
            mln_websocket_free(ws);
            mln_quit(ev, fd, http);
            return;
        }
        mln_tcp_conn_append(connection, out, M_C_SEND);
        if (mln_websocket_ping_generate(ws, &out, M_WS_FLAG_SERVER) != M_WS_RET_OK) {
            mln_log(error, "ping generate error.\n");
            mln_websocket_free(ws);
            mln_quit(ev, fd, http);
            return;
        }
        mln_tcp_conn_append(connection, out, M_C_SEND);
        mln_event_fd_set(ev, fd, M_EV_SEND|M_EV_APPEND|M_EV_NONBLOCK|M_EV_ONESHOT, M_EV_UNLIMITED, data, mln_websocket_send);
    } else if (mln_websocket_get_opcode(ws) == M_WS_OPCODE_CLOSE) {
        mln_chain_t *out = NULL;
        if (mln_websocket_close_generate(ws, &out, "just fun", M_WS_STATUS_NORMAL_CLOSURE, M_WS_FLAG_SERVER) != M_WS_RET_OK) {
            mln_log(error, "close generate error.\n");
            mln_websocket_free(ws);
            mln_quit(ev, fd, http);
            return;
        }
        mln_tcp_conn_append(connection, out, M_C_SEND);
        mln_event_fd_set(ev, fd, M_EV_SEND|M_EV_APPEND|M_EV_NONBLOCK|M_EV_ONESHOT, M_EV_UNLIMITED, data, mln_websocket_send);
    } else {
        mln_chain_t *out = NULL;
        if (mln_websocket_pong_generate(ws, &out, M_WS_FLAG_SERVER) != M_WS_RET_OK) {
            mln_log(error, "pong generate error.\n");
            mln_websocket_free(ws);
            mln_quit(ev, fd, http);
            return;
        }
        mln_tcp_conn_append(connection, out, M_C_SEND);
        if (mln_websocket_close_generate(ws, &out, "just fun", M_WS_STATUS_NORMAL_CLOSURE, M_WS_FLAG_SERVER) != M_WS_RET_OK) {
            mln_log(error, "close generate error.\n");
            mln_websocket_free(ws);
            mln_quit(ev, fd, http);
            return;
        }
        mln_tcp_conn_append(connection, out, M_C_SEND);
        mln_event_fd_set(ev, fd, M_EV_SEND|M_EV_APPEND|M_EV_NONBLOCK|M_EV_ONESHOT, M_EV_UNLIMITED, data, mln_websocket_send);
    }
    if (c != NULL) goto lp;
}

static int mln_http_send_body_handler(mln_http_t *http, mln_chain_t **body_head, mln_chain_t **body_tail)
{
    mln_u8ptr_t buf;
    mln_alloc_t *pool = mln_http_pool_get(http);
    mln_string_t cttype_key = mln_string("Content-Type");
    mln_string_t cttype_val = mln_string("text/html");

    buf = mln_alloc_m(pool, 5);
    if (buf == NULL) {
        mln_http_error_set(http, M_HTTP_INTERNAL_SERVER_ERROR);
        return M_HTTP_RET_ERROR;
    }
    memcpy(buf, "hello", 5);

    if (mln_http_field_set(http, &cttype_key, &cttype_val) == M_HTTP_RET_ERROR) {
        mln_http_error_set(http, M_HTTP_INTERNAL_SERVER_ERROR);
        return M_HTTP_RET_ERROR;
    }

    mln_string_t ctlen_key = mln_string("Content-Length");
    mln_string_t ctlen_val = mln_string("5");
    if (mln_http_field_set(http, &ctlen_key, &ctlen_val) == M_HTTP_RET_ERROR) {
        mln_http_error_set(http, M_HTTP_INTERNAL_SERVER_ERROR);
        return M_HTTP_RET_ERROR;
    }

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
    b->left_pos = b->pos = b->start = buf;
    b->last = b->end = buf + 5;
    b->in_memory = 1;
    b->last_buf = 1;
    b->last_in_chain = 1;

    if (*body_head == NULL) {
        *body_head = *body_tail = c;
    } else {
        (*body_tail)->next = c;
        *body_tail = c;
    }

    return M_HTTP_RET_DONE;
}

int main(int argc, char *argv[])
{
    struct mln_framework_attr cattr;

    cattr.argc = argc;
    cattr.argv = argv;
    cattr.global_init = NULL;
    cattr.main_thread = NULL;
    cattr.master_process = NULL;
    cattr.worker_process = worker_process;
    return mln_framework_init(&cattr);
}
```

