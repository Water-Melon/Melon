## HTTP



### Header file

```c
#include "mln_http.h"
```



### Module

`http`



### Functions/Macros



#### mln_http_init

```c
mln_http_t *mln_http_init(mln_tcp_conn_t *connection, void *data, mln_http_handler body_handler);

typedef int (*mln_http_handler)(mln_http_t *, mln_chain_t **, mln_chain_t **);
```

Description: Create and initialize the `mln_http_t` structure. `connection` is a TCP structure, which contains a TCP socket. `data` is the user-defined data part of the body processing function, which is used to assist the processing of the request or response body. `body_handler` is the body handler function, which is called every time the `mln_http_parse` or `mln_http_generate` function is called if it is not `NULL`. The body processing function has three parameters: http structure, the head and tail nodes of the doubly linked list used to parse or generate HTTP packets.

Return value: return `mln_http_t` structure pointer if successful, otherwise return `NULL`



#### mln_http_destroy

```c
void mln_http_destroy(mln_http_t *http);
```

Description: Destroy the `http` structure and free resources.

Return value: none



#### mln_http_reset

```c
void mln_http_reset(mln_http_t *http);
```

Description: Resets the `http` structure, but does not free the structure for the next processing.

Return value: none



#### mln_http_parse

```c
int mln_http_parse(mln_http_t *http, mln_chain_t **in);
```

- Description: Used to parse HTTP packets and write the parsed results into `http`.

  return value:

  - `M_HTTP_RET_DONE` parsing completed
  - `M_HTTP_RET_OK` parsing is not completed but no error occurs, continue to pass in new data to complete the parsing
  - `M_HTTP_RET_ERROR` parsing failed



#### mln_http_generate

```c
int mln_http_generate(mln_http_t *http, mln_chain_t **out_head, mln_chain_t **out_tail);
```

Description: Generate HTTP messages from HTTP related information in `http`. The message may not be generated all at once, so it can be called multiple times. The generated packets will be stored in the doubly linked list specified by `out_head` and `out_tail`.

return value:

- `M_HTTP_RET_DONE` generation complete
- `M_HTTP_RET_OK` generation did not complete without errors
- `M_HTTP_RET_ERROR` failed to generate



#### mln_http_field_set

```c
int mln_http_field_set(mln_http_t *http, mln_string_t *key, mln_string_t *val);
```

Description: Set the HTTP header field. If the header field `key` exists, `val` will be replaced with the original value.

return value:

- `M_HTTP_RET_OK` is processed successfully
- `M_HTTP_RET_ERROR` processing failed



#### mln_http_field_get

```c
mln_string_t *mln_http_field_get(mln_http_t *http, mln_string_t *key);
```

Description: Get the value of the key `key` in the HTTP header field.

Return value: return value string structure pointer if successful, otherwise return `NULL`



#### mln_http_field_iterator

```c
mln_string_t *mln_http_field_iterator(mln_http_t *http, mln_string_t *key);
```

Description: Returns a header field value with key `key` each time (that is, assuming that there are multiple header fields with the same key name).

Return value: return value string structure pointer if successful, otherwise return `NULL`



#### mln_http_field_remove

```c
void mln_http_field_remove(mln_http_t *http, mln_string_t *key);
```

Description: Remove header field `key` and its value.

Return value: none



#### mln_http_dump

```c
void mln_http_dump(mln_http_t *http);
```

Description: Print HTTP messages to standard output for debugging purposes.

Return value: none



#### mln_http_connection_get

```c
mln_http_connection_get(h)
```

Description: Get the TCP link structure in `h` of type `mln_http_t`.

Return value: `mln_tcp_conn_t` type pointer



#### mln_http_connection_set

```c
mln_http_connection_set(h,c)
```

Description: Set the TCP connection structure in `h` of type `mln_http_t` to `c` of type `mln_tcp_conn_t`.

Return value: none



#### mln_http_pool_get

```c
mln_http_pool_get(h)
```

Description: Get the memory pool structure in `h` of type `mln_http_t`.

Return value: pointer of type `mln_alloc_t`



#### mln_http_pool_set

```c
mln_http_pool_set(h,p)
```

Description: Set the memory pool in `h` of type `mln_http_t` to `p` of type `mln_alloc_t`.

Return value: none



#### mln_http_data_get

```c
mln_http_data_get(h)
```

Description: Get the user-defined data of the auxiliary body handler function in `h` of type `mln_http_t`.

Return value: user-defined data pointer



#### mln_http_data_set

```c
mln_http_data_set(h,d)
```

Description: Set the user-defined data of the auxiliary body handler in `h` of type `mln_http_t` to `d`.

Return value: none



#### mln_http_uri_get

```c
mln_http_uri_get(h)
```

Description: Get the URI string in `h` of type `mln_http_t`.

Return value: pointer of type `mln_string_t`



#### mln_http_uri_set

```c
mln_http_uri_set(h,u)
```

Description: Set the URI in `h` of type `mln_http_t` to `u` of pointer of type `mln_string_t`.

Return value: none



#### mln_http_args_get

```c
mln_http_args_get(h)
```

Description: Get the parameter string in `h` of type `mln_http_t`.

Return value: pointer of type `mln_string_t`



#### mln_http_args_set

```c
mln_http_args_set(h,a)
```

Description: Set the parameter in `h` of type `mln_http_t` to `a` of type pointer of `mln_string_t`.

Return value: none



#### mln_http_status_get

```c
mln_http_status_get(h)


```

Description: Get the response status word in `h` of type `mln_http_t`, such as 200 400, etc.

Return value: Integer status word



#### mln_http_status_set

```c
mln_http_status_set(h,s)
```

Description: Set the response status word in `h` of type `mln_http_t` to `s` of type integer.

Return value: none



#### mln_http_method_get

```c
mln_http_method_get(h)
```

Description: Get the method field in `h` of type `mln_http_t`

return value:

- `M_HTTP_GET`
- `M_HTTP_POST`
- `M_HTTP_HEAD`
- `M_HTTP_PUT`
- `M_HTTP_DELETE`
- `M_HTTP_TRACE`
- `M_HTTP_CONNECT`
- `M_HTTP_OPTIONS`



#### mln_http_set_method

```c
mln_http_set_method(h,m)
```

Description: Set the request method in `h` of type `mln_http_t` to `m`. For the available values of `m`, refer to the return value section of `mln_http_get_method`.

Return value: none



#### mln_http_version_get

```c
mln_http_version_get(h)
```

Description: Get the HTTP version in `h` of type `mln_http_t`

return value:

- `M_HTTP_VERSION_1_0` HTTP 1.0
- `M_HTTP_VERSION_1_1` HTTP 1.1



#### mln_http_version_set

```c
mln_http_version_set(h,v)
```

Description: Set the HTTP version number in `h` of type `mln_http_t` to `v`, and refer to the return value of `mln_http_version_get` for the value of `v`.

Return value: none



#### mln_http_type_get

```c
mln_http_type_get(h)
```

Description: Get the HTTP type in `h` of type `mln_http_t`, that is, request or response.

return value:

- `M_HTTP_UNKNOWN` unknown type
- `M_HTTP_REQUEST` request
- `M_HTTP_RESPONSE` response



#### mln_http_type_set

```c
mln_http_type_set(h,t)
```

Description: Set the packet type in `h` of type `mln_http_t` to `t`. The value of `t` refers to the return value of `mln_http_type_get`.

Return value: none



#### mln_http_handler_get

```c
mln_http_handler_get(h)
```

Description: Get the `h` body handler pointer of type `mln_http_t`.

Return value: function pointer of type `mln_http_handler`



#### mln_http_handler_set

```c
mln_http_handler_set(h,hlr)
```

Description: Set the `h` handler function of type `mln_http_t` to `hlr` of type `mln_http_handler`.

Return value: none



#### mln_http_response_msg_get

```c
mln_http_response_msg_get(h)
```

Description: Get the response information in `h` of type `mln_http_t`, that is, strings like: Bad Request or Internal Server Error.

Return value: pointer of type `mln_string_t`



#### mln_http_response_msg_set

```c
mln_http_response_msg_set(h,m)
```

Description: Set the response information in `h` of type `mln_http_t` to `m` of type `mln_string_t` pointer.

Return value: none



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

Description: Get error information in `h` of type `mln_http_t`.

Return value: the error value defined by the macro



#### mln_http_error_set

```c
mln_http_error_set(h,e)
```

Description: Set the error message in `h` of type `mln_http_t` to `e`. For the value of `e`, see the macro definition in `mln_http_error_get`.

Return value: none



#### mln_http_header_get

```c
mln_http_header_get(h)
```

Description: Get the header field structure in `h` of type `mln_http_t`.

Return value: `mln_hash_t` type structure



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

