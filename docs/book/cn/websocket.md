## Websocket



### 头文件

```c
#include "mln_websocket.h"
```



### 模块名

`websocket`



### 函数/宏



#### mln_websocket_init

```c
int mln_websocket_init(mln_websocket_t *ws, mln_http_t *http);
```

描述：初始化websocket结构`ws`，其依赖于`mln_http_t`结构`http`。ws可能是动态分配而来，也可能是栈内自动变量。

返回值：成功则返回`0`，否则返回`-1`



#### mln_websocket_new

```c
mln_websocket_t *mln_websocket_new(mln_http_t *http);
```

描述：创建并初始化`mln_websocket_t`结构，其依赖于`mln_http_t`结构`http`。

返回值：成功则返回结构指针，否则返回`NULL`



#### mln_websocket_destroy

```c
void mln_websocket_destroy(mln_websocket_t *ws);
```

描述：销毁`mln_websocket_t`结构，`ws`应由`mln_websocket_init`初始化而来。

返回值：无



#### mln_websocket_free

```c
void mln_websocket_free(mln_websocket_t *ws);
```

描述：销毁并释放`mln_websocket_t`结构`ws`。

返回值：无



#### mln_websocket_reset

```c
void mln_websocket_reset(mln_websocket_t *ws);
```

描述：重置`ws`内的所有内容。

返回值：无



#### mln_websocket_is_websocket

```c
int mln_websocket_is_websocket(mln_http_t *http);
```

描述：判断本`http`是否是一个websocket。

返回值：

- `M_WS_RET_NOTWS` 不是websocket
- `M_WS_RET_ERROR` 不是HTTP请求
- `M_WS_RET_OK` 是websocket



#### mln_websocket_validate

```c
int mln_websocket_validate(mln_websocket_t *ws);
```

描述：判断`ws`是否是一个有效的websocket连接。

返回值：

- `M_WS_RET_NOTWS` 不是
- `M_WS_RET_ERROR` 不是HTTP响应
- `M_WS_RET_OK` 是



#### mln_websocket_set_field

```c
int mln_websocket_set_field(mln_websocket_t *ws, mln_string_t *key, mln_string_t *val);
```

描述：设置websocket首次交互的HTTP头。若`key`存在，则替换值为`val`。

返回值：

- `M_WS_RET_FAILED` 失败
- `M_WS_RET_OK` 成功



#### mln_websocket_get_field

```c
mln_string_t *mln_websocket_get_field(mln_websocket_t *ws, mln_string_t *key);
```

描述：获取websocket首次交互的HTTP头字段`key`的值。

返回值：存在则返回`mln_string_t`类型指针，否则返回`NULL`



#### mln_websocket_match

```c
int mln_websocket_match(mln_websocket_t *ws);
```

描述：将websocket中的头字段与其http结构中的头字段进行比较。

返回值：

- `M_WS_RET_ERROR` 失败
- `M_WS_RET_OK` 成功



#### mln_websocket_handshake_response_generate

```c
int mln_websocket_handshake_response_generate(mln_websocket_t *ws, mln_chain_t **chead, mln_chain_t **ctail);
```

描述：生成一个websocket握手响应报文，报文内容为`chead`和`ctail`指定的链表中的内容。

返回值：

- `M_WS_RET_FAILED` 失败
- `M_WS_RET_OK` 成功



#### mln_websocket_handshake_request_generate

```c
int mln_websocket_handshake_request_generate(mln_websocket_t *ws, mln_chain_t **chead, mln_chain_t **ctail);
```

描述：生成一个websocket握手请求报文，报文内容为`chead`和`ctail`指定的链表中的内容。

返回值：

- `M_WS_RET_FAILED` 失败
- `M_WS_RET_OK` 成功



#### mln_websocket_text_generate

```c
int mln_websocket_text_generate(mln_websocket_t *ws, mln_chain_t **out_cnode, mln_u8ptr_t buf, mln_size_t len, mln_u32_t flags);
```

描述：生成一个文本数据帧。`out_cnode`为生成的帧数据，`buf`和`len`为文本内容。flags有若干值，且这些值可以使用或运算符一同赋予：

- `M_WS_FLAG_NONE` 无含义
- `M_WS_FLAG_NEW` 标记第一个数据分片
- `M_WS_FLAG_END` 标记最后一个数据分片
- `M_WS_FLAG_CLIENT` 标记为客户端生成
- `M_WS_FLAG_SERVER` 标记为服务端生成

返回值：

- `M_WS_RET_ERROR`数据有错
- `M_WS_RET_FAILED`失败，例如内存不足等
- `M_WS_RET_OK`成功



#### mln_websocket_binary_generate

```c
int mln_websocket_binary_generate(mln_websocket_t *ws, mln_chain_t **out_cnode, void *buf, mln_size_t len, mln_u32_t flags);
```

描述：生成一个二进制数据帧。`out_cnode`为生成的帧数据，`buf`和`len`为二进制数据。flags有若干值，且这些值可以使用或运算符一同赋予：

- `M_WS_FLAG_NONE` 无含义
- `M_WS_FLAG_NEW` 标记第一个数据分片
- `M_WS_FLAG_END` 标记最后一个数据分片
- `M_WS_FLAG_CLIENT` 标记为客户端生成
- `M_WS_FLAG_SERVER` 标记为服务端生成

返回值：

- `M_WS_RET_ERROR`数据有错
- `M_WS_RET_FAILED`失败，例如内存不足等
- `M_WS_RET_OK`成功



#### mln_websocket_close_generate

```c
int mln_websocket_close_generate(mln_websocket_t *ws, mln_chain_t **out_cnode, char *reason, mln_u16_t status, mln_u32_t flags);
```

描述：生成一个websocket关闭报文。`out_cnode`为本函数生成的报文，`reason`为关闭原因，`status`为关闭状态字：

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

flags有若干值，且这些值可以使用或运算符一同赋予：

- `M_WS_FLAG_NONE` 无含义
- `M_WS_FLAG_NEW` 标记第一个数据分片
- `M_WS_FLAG_END` 标记最后一个数据分片
- `M_WS_FLAG_CLIENT` 标记为客户端生成
- `M_WS_FLAG_SERVER` 标记为服务端生成

返回值：

- `M_WS_RET_ERROR`数据有错
- `M_WS_RET_FAILED`失败，例如内存不足等
- `M_WS_RET_OK`成功



#### mln_websocket_ping_generate

```c
int mln_websocket_ping_generate(mln_websocket_t *ws, mln_chain_t **out_cnode, mln_u32_t flags);
```

描述：生成ping报文，`out_cnode`为生成的报文，flags有若干值，且这些值可以使用或运算符一同赋予：

- `M_WS_FLAG_NONE` 无含义
- `M_WS_FLAG_NEW` 标记第一个数据分片
- `M_WS_FLAG_END` 标记最后一个数据分片
- `M_WS_FLAG_CLIENT` 标记为客户端生成
- `M_WS_FLAG_SERVER` 标记为服务端生成

返回值：

- `M_WS_RET_ERROR`数据有错
- `M_WS_RET_FAILED`失败，例如内存不足等
- `M_WS_RET_OK`成功



#### mln_websocket_pong_generate

```c
int mln_websocket_pong_generate(mln_websocket_t *ws, mln_chain_t **out_cnode, mln_u32_t flags);
```

描述：生成pong报文，`out_cnode`为生成的报文，flags有若干值，且这些值可以使用或运算符一同赋予：

- `M_WS_FLAG_NONE` 无含义
- `M_WS_FLAG_NEW` 标记第一个数据分片
- `M_WS_FLAG_END` 标记最后一个数据分片
- `M_WS_FLAG_CLIENT` 标记为客户端生成
- `M_WS_FLAG_SERVER` 标记为服务端生成

返回值：

- `M_WS_RET_ERROR`数据有错
- `M_WS_RET_FAILED`失败，例如内存不足等
- `M_WS_RET_OK`成功



#### mln_websocket_generate

```c
int mln_websocket_generate(mln_websocket_t *ws, mln_chain_t **out_cnode);
```

描述：根据`ws`结构的当前内容生成报文。如上生成函数皆为该函数的上层封装。`out_cnode`为生成的报文。当想自定义发送的格式时，需要配合后续的宏函数对`ws`的一些成员进行设置。

返回值：

- `M_WS_RET_ERROR`数据有错
- `M_WS_RET_FAILED`失败，例如内存不足等
- `M_WS_RET_OK`成功



#### mln_websocket_parse

```c
int mln_websocket_parse(mln_websocket_t *ws, mln_chain_t **in);
```

描述：解析`in`中的数据，并将数据放入`ws`中的对应位置。

返回值：

- `M_WS_RET_ERROR`报文出错
- `M_WS_RET_OK`解析成功
- `M_WS_RET_FAILED`解析失败，例如内存不足等问题
- `M_WS_RET_NOTYET`成功但数据不完全，需要继续处理



#### mln_websocket_get_http

```c
mln_websocket_get_http(ws)
```

描述：获取`ws`所属的`http`结构。

返回值：`mln_http_t`类型指针



#### mln_websocket_get_pool

```c
mln_websocket_get_pool(ws)
```

描述：获取`ws`所使用的内存池结构。

返回值：`mln_alloc_t`类型指针



#### mln_websocket_get_connection

```c
mln_websocket_get_connection(ws)
```

描述：获取`ws`所使用的TCP链接结构。

返回值：`mln_tcp_conn_t`类型指针



#### mln_websocket_get_uri

```c
mln_websocket_get_uri(ws)
```

描述：获取`ws`当前的HTTP URI。

返回值：`mln_string_t`类型指针



#### mln_websocket_set_uri

```c
mln_websocket_set_uri(ws,u)
```

描述：设置`ws`要访问的URI为`mln_string_t`类型指针的`u`。

返回值：无



#### mln_websocket_get_args

```c
mln_websocket_get_args(ws)
```

描述：获取`ws`的HTTP请求参数。

返回值：`mln_string_t`类型指针



#### mln_websocket_set_args

```c
mln_websocket_set_args(ws,a)
```

描述：设置`ws`的HTTP请求参数为`mln_string_t`类型指针的`a`。

返回值：无



#### mln_websocket_get_key

```c
mln_websocket_get_key(ws)
```

描述：获取HTTP头字段`Sec-Websocket-Key`的值。

返回值：`mln_string_t`类型指针



#### mln_websocket_set_key

```c
 mln_websocket_set_key(ws,k)
```

描述：设置websocket HTTP头字段`Sec-Websocket-Key`的值为`mln_string_t`类型指针`k`。

返回值：无



#### mln_websocket_set_data

```c
mln_websocket_set_data(ws,d)
```

描述：设置用户自定义结构`d`。

返回值：无



#### mln_websocket_get_data

```c
mln_websocket_get_data(ws)
```

描述：获取用户自定义结构。

返回值：自定义结构指针



#### mln_websocket_set_content

```c
mln_websocket_set_content(ws,c)
```

描述：设置数据帧，`c`的数据类型取决于要发送的内容，可以是结构体，也可以是整数，也可以是其他数据类型。本函数仅用于自定义websocket报文时使用。

返回值：无



#### mln_websocket_get_content

```c
mln_websocket_get_content(ws)
```

描述：获取数据帧内容。

返回值：`void *`类型数据



#### mln_websocket_set_content_len

```c
mln_websocket_set_content_len(ws,l)
```

描述：设置数据长度。

返回值：无



#### mln_websocket_get_content_len

```c
mln_websocket_get_content_len(ws)
```

描述：获取数据长度。

返回值：无符号整型



#### mln_websocket_set_ext_handler

```c
mln_websocket_set_ext_handler(ws,h)

typedef int (*mln_ws_extension_handle)(mln_websocket_t *);
```

描述：设置extension handler `h`到`ws`中。该函数会在`mln_websocket_generate`和`mln_websocket_parse`中被调用，用于增加一写自定义处理内容。

返回值：无



#### mln_websocket_get_ext_handler

```c
mln_websocket_get_ext_handler(ws)
```

描述：获取extension handler。该函数会在`mln_websocket_generate`和`mln_websocket_parse`中被调用，用于增加一写自定义处理内容。

返回值：`mln_ws_extension_handle`类型指针



#### mln_websocket_set_rsv1

```c
 mln_websocket_set_rsv1(ws)
```

描述：设置保留位rsv1位1。

返回值：无



#### mln_websocket_reset_rsv1

```c
mln_websocket_reset_rsv1(ws)
```

描述：复位保留位rsv1。

返回值：无



#### mln_websocket_get_rsv1

```c
mln_websocket_get_rsv1(ws)
```

描述：获取保留位rsv1的值。

返回值：无符号整型值



#### mln_websocket_set_rsv2

```c
mln_websocket_set_rsv2(ws)
```

描述：设置保留位rsv2位1。

返回值：无



#### mln_websocket_reset_rsv2

```c
mln_websocket_reset_rsv2(ws)
```

描述：复位保留位rsv2。

返回值：无



#### mln_websocket_get_rsv2

```c
mln_websocket_get_rsv2(ws)
```

描述：获取保留位rsv2的值。

返回值：无符号整型值



#### mln_websocket_set_rsv3

```c
mln_websocket_set_rsv3(ws)
```

描述：设置保留位rsv3位1。

返回值：无



#### mln_websocket_reset_rsv3

```c
mln_websocket_reset_rsv3(ws)
```

描述：复位保留位rsv3。

返回值：无



#### mln_websocket_get_rsv3

```c
mln_websocket_get_rsv3(ws)
```

描述：获取保留位rsv3的值。

返回值：无符号整型值



#### mln_websocket_set_opcode

```c
mln_websocket_set_opcode(ws,op)
```

描述：设置操作码，该操作码用于告知`mln_websocket_generate`函数生成什么类型报文，`op`的值如下：

- `M_WS_OPCODE_CONTINUE` 由于类型在首个数据包中设定，因此后续数据都为本类型。
- `M_WS_OPCODE_TEXT` 文本类型
- `M_WS_OPCODE_BINARY` 二进制类型
- `M_WS_OPCODE_CLOSE` 断开
- `M_WS_OPCODE_PING` 存活检测请求
- `M_WS_OPCODE_PONG` 存活检测响应

返回值：无



#### mln_websocket_get_opcode

```c
mln_websocket_get_opcode(ws)
```

描述：获取操作码，操作码详情见`mln_websocket_set_opcode`函数。

返回值：无符号整型



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

描述：设置websocket状态码，状态码数值如宏定义所示。

返回值：无



#### mln_websocket_get_status

```c
mln_websocket_get_status(ws)
```

描述：获取websocket状态码。

返回值：无符号整型



#### mln_websocket_set_content_free

```c
mln_websocket_set_content_free(ws)
```

描述：置位`content_free`标记。该标记控制是否在`ws`结构销毁或重置时释放content成员（数据帧数据）的内存，该内存必须由`ws`的内存池分配而来。

返回值：无



#### mln_websocket_reset_content_free

```c
mln_websocket_reset_content_free(ws)
```

描述：复位`content_free`标记值。

返回值：无



#### mln_websocket_get_content_free

```c
mln_websocket_get_content_free(ws)
```

描述：获取`content_free`标记值。

返回值：无符号整型



#### mln_websocket_set_fin

```c
mln_websocket_set_fin(ws)
```

描述：置位`fin`标记，该标记控制是否为最后一个数据帧。

返回值：无



#### mln_websocket_reset_fin

```c
mln_websocket_reset_fin(ws)
```

描述：复位`fin`标记。

返回值：无



#### mln_websocket_get_fin

```c
mln_websocket_get_fin(ws)
```

描述：获取`fin`的值。

返回值：无符号整型



#### mln_websocket_set_maskbit

```c
mln_websocket_set_maskbit(ws)
```

描述：置位websocket报文的`mask`位。置位表示会携带有4字节masking-key。

返回值：无



#### mln_websocket_reset_maskbit

```c
mln_websocket_reset_maskbit(ws)
```

描述：复位websocket报文的`mask`位。

返回值：无



#### mln_websocket_get_maskbit

```c
mln_websocket_get_maskbit(ws)
```

描述：获取websocket报文的`mask`位。

返回值：无符号整型



#### mln_websocket_set_masking_key

```c
mln_websocket_set_masking_key(ws,k)
```

描述：设置`masking-key`字段值为`k`。`k`为无符号32位整型。

返回值：无



#### mln_websocket_get_masking_key

```c
mln_websocket_get_masking_key(ws)
```

描述：获取websocket的`masking-key`值。

返回值：无符号32位整型



###示例

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

