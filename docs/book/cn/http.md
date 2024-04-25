## HTTP



### 头文件

```c
#include "mln_http.h"
```



### 模块名

`http`



### 函数/宏



#### mln_http_init

```c
mln_http_t *mln_http_init(mln_tcp_conn_t *connection, void *data, mln_http_handler body_handler);

typedef int (*mln_http_handler)(mln_http_t *, mln_chain_t **, mln_chain_t **);
```

描述：创建并初始化`mln_http_t`结构。`connection`是TCP结构，内含TCP套接字。`data`为体处理函数的用户自定义数据部分，用于辅助请求或响应体的处理。`body_handler`是体处理函数，如果该指针非`NULL`，则该函数会在每次调用`mln_http_parse`或`mln_http_generate`函数时被调用。体处理函数有三个参数，分别为：http结构，用于解析或生成HTTP报文的双向链表的头和尾节点。

返回值：成功则返回`mln_http_t`结构指针，否则返回`NULL`



#### mln_http_destroy

```c
void mln_http_destroy(mln_http_t *http);
```

描述：销毁`http`结构并释放资源。

返回值：无



#### mln_http_reset

```c
void mln_http_reset(mln_http_t *http);
```

描述：重置`http`结构，但不会将结构释放，可用于下一次处理。

返回值：无



#### mln_http_parse

```c
int mln_http_parse(mln_http_t *http, mln_chain_t **in);
```

描述：用于解析HTTP报文，并将解析的结果写入`http`中。

返回值：

- `M_HTTP_RET_DONE` 解析完成
- `M_HTTP_RET_OK` 解析未完成但未出错，继续传入新的数据使解析完成
- `M_HTTP_RET_ERROR` 解析失败



#### mln_http_generate

```c
int mln_http_generate(mln_http_t *http, mln_chain_t **out_head, mln_chain_t **out_tail);
```

描述：将`http`中HTTP相关信息生成HTTP报文。报文可能不会一次性生成完全，因此可以多次调用。已生成的报文将会存放在`out_head`和`out_tail`指定的双向链表中。

返回值：

- `M_HTTP_RET_DONE` 生成完成
- `M_HTTP_RET_OK` 生成未完成但未出错
- `M_HTTP_RET_ERROR` 生成失败



#### mln_http_field_set

```c
int mln_http_field_set(mln_http_t *http, mln_string_t *key, mln_string_t *val);
```

描述：设置HTTP头字段。若头字段`key`存在，则会将`val`替换原有值。

返回值：

- `M_HTTP_RET_OK` 处理成功
- `M_HTTP_RET_ERROR`处理失败



#### mln_http_field_get

```c
mln_string_t *mln_http_field_get(mln_http_t *http, mln_string_t *key);
```

描述：获取HTTP头字段中键为`key`的值。

返回值：成功则返回值字符串结构指针，否则返回`NULL`



#### mln_http_field_iterator

```c
mln_string_t *mln_http_field_iterator(mln_http_t *http, mln_string_t *key);
```

描述：每次返回一个键为`key`的头字段值（即假设存在多个相同键名的头字段）。

返回值：成功则返回值字符串结构指针，否则返回`NULL`



#### mln_http_field_remove

```c
void mln_http_field_remove(mln_http_t *http, mln_string_t *key);
```

描述：移除头字段`key`及其值。

返回值：无



#### mln_http_dump

```c
void mln_http_dump(mln_http_t *http);
```

描述：将HTTP信息输出到标准输出，用于调试之用。

返回值：无



#### mln_http_connection_get

```c
mln_http_connection_get(h)
```

描述：获取类型为`mln_http_t`的`h`中TCP链接结构。

返回值：`mln_tcp_conn_t`类型指针



#### mln_http_connection_set

```c
mln_http_connection_set(h,c)
```

描述：将`mln_http_t`类型的`h`中TCP链接结构设置为`mln_tcp_conn_t`类型的`c`。

返回值：无



#### mln_http_pool_get

```c
mln_http_pool_get(h)
```

描述：获取类型为`mln_http_t`的`h`中内存池结构。

返回值：`mln_alloc_t`类型指针



#### mln_http_pool_set

```c
mln_http_pool_set(h,p)
```

描述：将`mln_http_t`类型的`h`中内存池设置为`mln_alloc_t`类型的`p`。

返回值：无



#### mln_http_data_get

```c
mln_http_data_get(h)
```

描述：获取类型为`mln_http_t`的`h`中辅助体处理函数的用户自定义数据。

返回值：用户自定义数据指针



#### mln_http_data_set

```c
mln_http_data_set(h,d)
```

描述：将`mln_http_t`类型的`h`中辅助体处理函数的用户自定义数据设置为`d`。

返回值：无



#### mln_http_uri_get

```c
mln_http_uri_get(h)
```

描述：获取类型为`mln_http_t`的`h`中URI字符串。

返回值：`mln_string_t`类型指针



#### mln_http_uri_set

```c
mln_http_uri_set(h,u)
```

描述：将`mln_http_t`类型的`h`中URI设置为`mln_string_t`类型指针的`u`。

返回值：无



#### mln_http_args_get

```c
mln_http_args_get(h)
```

描述：获取类型为`mln_http_t`的`h`中参数字符串。

返回值：`mln_string_t`类型指针



#### mln_http_args_set

```c
mln_http_args_set(h,a)
```

描述：将`mln_http_t`类型的`h`中参数设置为`mln_string_t`类型指针的`a`。

返回值：无



#### mln_http_status_get

```c
mln_http_status_get(h)


```

描述：获取类型为`mln_http_t`的`h`中响应状态字，例如200 400等。

返回值：整型状态字



#### mln_http_status_set

```c
mln_http_status_set(h,s)
```

描述：将`mln_http_t`类型的`h`中响应状态字设置为整型的`s`。

返回值：无



#### mln_http_method_get

```c
mln_http_method_get(h)
```

描述：获取类型为`mln_http_t`的`h`中方法字段

返回值：

- `M_HTTP_GET`
- `M_HTTP_POST`
- `M_HTTP_HEAD`
- `M_HTTP_PUT`
- `M_HTTP_DELETE`
- `M_HTTP_TRACE`
- `M_HTTP_CONNECT`
- `M_HTTP_OPTIONS`



#### mln_http_method_set

```c
mln_http_method_set(h,m)
```

描述：将`mln_http_t`类型的`h`中请求方法设置为`m`，`m`的可用值参考`mln_http_get_method`的返回值部分。

返回值：无



#### mln_http_version_get

```c
mln_http_version_get(h)
```

描述：获取类型为`mln_http_t`的`h`中HTTP版本

返回值：

- `M_HTTP_VERSION_1_0` HTTP 1.0
- `M_HTTP_VERSION_1_1` HTTP 1.1



#### mln_http_version_set

```c
mln_http_version_set(h,v)
```

描述：将`mln_http_t`类型的`h`中的HTTP版本号为`v`，`v`的取值参考`mln_http_version_get`的返回值。

返回值：无



#### mln_http_type_get

```c
mln_http_type_get(h)
```

描述：获取类型为`mln_http_t`的`h`中HTTP类型，即请求还是响应。

返回值：

- `M_HTTP_UNKNOWN`未知类型
- `M_HTTP_REQUEST`请求
- `M_HTTP_RESPONSE`响应



#### mln_http_type_set

```c
mln_http_type_set(h,t)
```

描述：将`mln_http_t`类型的`h`中报文类型设置为`t`，`t`的取值参考`mln_http_type_get`的返回值。

返回值：无



#### mln_http_handler_get

```c
mln_http_handler_get(h)
```

描述：获取类型为`mln_http_t`的`h`中体处理函数指针。

返回值：类型为`mln_http_handler`的函数指针



#### mln_http_handler_set

```c
mln_http_handler_set(h,hlr)
```

描述：将`mln_http_t`类型的`h`中提处理函数设置为`mln_http_handler`类型的`hlr`。

返回值：无



#### mln_http_response_msg_get

```c
mln_http_response_msg_get(h)
```

描述：获取类型为`mln_http_t`的`h`中响应信息，即类似：Bad Request 或 Internal Server Error等字符串。

返回值：`mln_string_t`类型指针



#### mln_http_response_msg_set

```c
mln_http_response_msg_set(h,m)
```

描述：将`mln_http_t`类型的`h`中响应信息设置为`mln_string_t`类型指针的`m`。

返回值：无



#### mln_http_error_get

```c
mln_http_error_get(h)

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
```

描述：获取类型为`mln_http_t`的`h`中错误信息。

返回值：宏定义的错误值



#### mln_http_error_set

```c
mln_http_error_set(h,e)
```

描述：将`mln_http_t`类型的`h`中错误信息设置为`e`，`e`的取值参见`mln_http_error_get`中的宏定义。

返回值：无



#### mln_http_header_get

```c
mln_http_header_get(h)
```

描述：获取类型为`mln_http_t`的`h`中头字段结构。

返回值：`mln_hash_t`类型结构



### 示例

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
#include "mln_http.h"
#include "mln_file.h"


static void mln_accept(mln_event_t *ev, int fd, void *data);
static int mln_http_recv_body_handler(mln_http_t *http, mln_chain_t **in, mln_chain_t **nil);
static void mln_recv(mln_event_t *ev, int fd, void *data);
static void mln_quit(mln_event_t *ev, int fd, void *data);
static void mln_send(mln_event_t *ev, int fd, void *data);
static int mln_http_send_body_handler(mln_http_t *http, mln_chain_t **body_head, mln_chain_t **body_tail);

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
    int ret, rc;
    mln_chain_t *c;

    while (1) {
        ret = mln_tcp_conn_recv(connection, M_C_TYPE_MEMORY);
        if (ret == M_C_FINISH) {
            continue;
        } else if (ret == M_C_NOTYET) {
            c = mln_tcp_conn_remove(connection, M_C_RECV);
            if (c != NULL) {
                rc = mln_http_parse(http, &c);
                if (c != NULL) {
                    mln_tcp_conn_append_chain(connection, c, NULL, M_C_RECV);
                }
                if (rc == M_HTTP_RET_OK) {
                    return;
                } else if (rc == M_HTTP_RET_DONE) {
                    mln_send(ev, fd, data);
                } else {
                    fprintf(stderr, "Http parse error. error_code:%u\n", mln_http_error_get(http));
                    mln_quit(ev, fd, data);
                    return;
                }
            }
            break;
        } else if (ret == M_C_CLOSED) {
            c = mln_tcp_conn_remove(connection, M_C_RECV);
            if (c != NULL) {
                rc = mln_http_parse(http, &c);
                if (c != NULL) {
                    mln_tcp_conn_append_chain(connection, c, NULL, M_C_RECV);
                }
                if (rc == M_HTTP_RET_ERROR) {
                    fprintf(stderr, "Http parse error. error_code:%u\n", mln_http_error_get(http));
                }
            }
            mln_quit(ev, fd, data);
            return;
        } else if (ret == M_C_ERROR) {
            mln_quit(ev, fd, data);
            return;
        }
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
    mln_chain_t *c = mln_tcp_conn_head(connection, M_C_SEND);
    int ret;

    if (c == NULL) {
        mln_http_reset(http);
        mln_http_status_set(http, M_HTTP_OK);
        mln_http_version_set(http, M_HTTP_VERSION_1_0);
        mln_http_type_set(http, M_HTTP_RESPONSE);
        mln_http_handler_set(http, mln_http_send_body_handler);
        mln_chain_t *body_head = NULL, *body_tail = NULL;
        if (mln_http_generate(http, &body_head, &body_tail) == M_HTTP_RET_ERROR) {
            fprintf(stderr, "mln_http_generate() failed. %u\n", mln_http_error_get(http));
            mln_quit(ev, fd, data);
            return;
        }
        mln_tcp_conn_append_chain(connection, body_head, body_tail, M_C_SEND);
    }

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

