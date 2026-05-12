## TCP连接及网络I/O链

TCP的收发会根据其是否为阻塞或非阻塞而有所差异。而是否为非阻塞，则有用户自行设置（事件接口中可以设置）。

Windows中性能会相对低一些，因为win并未提供相关系统接口获取套接字是否为非阻塞模式。



### 头文件

```c
#include "mln_connection.h"
```



### 模块名

`connection`



### 相关结构

```c
typedef struct mln_buf_s {//用于存放数据，且根据不同标识量指定数据存放位置（文件还是内存），同时还标出当前数据被处理的位置
    mln_u8ptr_t         left_pos;//当前数据被处理到的位置
    mln_u8ptr_t         pos;//数据在本块内存的起始位置
    mln_u8ptr_t         last;//数据在本块内存的结束位置
    mln_u8ptr_t         start;//本块内存起始位置
    mln_u8ptr_t         end;//本块内存结束位置
    struct mln_buf_s   *shadow;//是否存在其他buf结构指向相同内存块
    mln_off_t           file_left_pos;//当前数据被处理到的文件偏移
    mln_off_t           file_pos;//数据在本文件内的起始偏移
    mln_off_t           file_last;//数据在本文件内的结束偏移
    mln_file_t         *file;//文件结构，参见文件集合部分的介绍
    mln_u32_t           temporary:1;//start、pos等内存指针指向的内存是否是临时的（即不需要释放的）
#if !defined(MSVC)
    mln_u32_t           mmap:1;//是否是mmap创建的内存，win下暂不支持
#endif
    mln_u32_t           in_memory:1;//数据是否在内存中
    mln_u32_t           in_file:1;//数据是否在文件中
    mln_u32_t           flush:1;//是否将数据立刻发出（本标记暂时未被使用）
    mln_u32_t           sync:1;//本标记暂时未被使用
    mln_u32_t           last_buf:1;//本buf是否是shadow替身中的最后一个buf，当无替身时，自己就是最后一个
    mln_u32_t           last_in_chain:1;//标记本buf是否是链上的最后一的buf，该标记被用于tcp发送部分。当遇到此标记时，
                        //哪怕本buf所在链节点后还有节点，也会立刻返回给上层，并表示数据发送完成。
                        //若还要继续发送，需要再次调用发送函数
} mln_buf_t;

typedef struct mln_chain_s { //buf单链表，用于tcp发送数据和接收数据
    mln_buf_t          *buf;
    struct mln_chain_s *next;
} mln_chain_t;
```



### 函数/宏



#### mln_buf_size

```c
mln_buf_size(pbuf)
```

描述：获取本`pbuf`所指数据的大小，即`last-pos`/`file_last-file_last`的大小。

返回值：字节数



#### mln_buf_left_size

```c
mln_buf_left_size(pbuf)
```

描述：获取本`pbuf`尚未被处理的数据大小，即`last-left_pos`/``file_last-file_left_pos`的大小，

返回值：字节数



#### mln_chain_add

```c
mln_chain_add(pphead,pptail,c)
```

描述：将链节点`c`加入到链`pphead``pptail`中，其中：`pphead`和`pptail`分别为单链表的首和尾的二级指针。

返回值：无



#### mln_buf_new

```c
mln_buf_t *mln_buf_new(mln_alloc_t *pool);
```

描述：从内存池`pool`中创建buf结构。

返回值：成功则返回buf结构指针，否则返回`NULL`



#### mln_chain_new

```c
mln_chain_t *mln_chain_new(mln_alloc_t *pool);
```

描述：从内存池`pool`中创建链结构。

返回值：成功则返回链结构指针，否则返回`NULL`



#### mln_chain_new_with_buf

```c
mln_chain_t *mln_chain_new_with_buf(mln_alloc_t *pool);
```

描述：从内存池`pool`中创建链结构，并为其分配一个`buf`结构。相当于先调用`mln_chain_new`再调用`mln_buf_new`的便捷函数。

返回值：成功则返回链结构指针（其`buf`成员已被初始化），否则返回`NULL`



#### mln_buf_pool_release

```c
void mln_buf_pool_release(mln_buf_t *b);
```

描述：释放buf及其内部资源。

返回值：无



#### mln_chain_pool_release

```c
void mln_chain_pool_release(mln_chain_t *c);
```

描述：释放链节点`c`及其内部资源。

返回值：无



#### mln_chain_pool_release_all

```c
void mln_chain_pool_release_all(mln_chain_t *c);
```

描述：释放`c`所指代的整条链及其内部资源。

返回值：无



#### mln_tcp_conn_init

```c
int mln_tcp_conn_init(mln_tcp_conn_t *tc, int sockfd);
```

描述：初始化TCP结构，其中：

- `tc`为TCP结构指针，所指向内存根据需要可能来源于栈中，也可能来源于堆或者共享内存等等，由用户自行决定。
- `sockfd`为TCP的socket套接字描述符。

返回值：成功则返回`0`，否则返回`-1`



#### mln_tcp_conn_destroy

```c
void mln_tcp_conn_destroy(mln_tcp_conn_t *tc);
```

描述：销毁`tc`内的资源结构。**注意**：本函数不会关闭套接字，也不会释放`tc`本身的内存，需要由外部自行处理。

返回值：无



#### mln_tcp_conn_append_chain

```c
void mln_tcp_conn_append_chain(mln_tcp_conn_t *tc, mln_chain_t *c_head, mln_chain_t *c_tail, int type);
```

描述：将由`c_head`和`c_tail`表示的链追加到`tc`结构中的制定链上。`type`有如下值：

- `M_C_SEND`表示发送链，即发送队列
- `M_C_RECV`表示接收队列
- `M_C_SENT`表示已发送队列

返回值：无



#### mln_tcp_conn_append

```c
void mln_tcp_conn_append(mln_tcp_conn_t *tc, mln_chain_t *c, int type);
```

描述：将链`c`追加到`tc`中的指定队列中，`type`的值与`mln_tcp_conn_append_chain`一致。本函数仅省去了追加链的尾指针，因此本函数的开销会比`mln_tcp_conn_append_chain`稍大一些。

返回值：无



#### mln_tcp_conn_head

```c
mln_chain_t *mln_tcp_conn_head(mln_tcp_conn_t *tc, int type);
```

描述：获取`tc`中指定链（队列）的头指针。`type`的值与`mln_tcp_conn_append_chain`一致。

返回值：若有数据则为链节点指针，没数据则为`NULL`



#### mln_tcp_conn_remove

```c
mln_chain_t *mln_tcp_conn_remove(mln_tcp_conn_t *tc, int type);
```

描述：将`tc`中指定链（队列）整个拆卸并返回其头指针。`type`的值与`mln_tcp_conn_append_chain`一致。

返回值：若有数据则为链节点指针，没数据则为`NULL`



#### mln_tcp_conn_pop

```c
mln_chain_t *mln_tcp_conn_pop(mln_tcp_conn_t *tc, int type);
```

描述：获取`tc`中指定链的第一个链节点。`type`的值与`mln_tcp_conn_append_chain`一致。

返回值：若有数据则为链节点指针，没数据则为`NULL`



#### mln_tcp_conn_tail

```c
mln_chain_t *mln_tcp_conn_tail(mln_tcp_conn_t *tc, int type);
```

描述：获取`tc`中指定链（队列）的尾指针。`type`的值与`mln_tcp_conn_append_chain`一致。

返回值：若有数据则为链节点指针，没数据则为`NULL`



#### mln_tcp_conn_send

```c
int mln_tcp_conn_send(mln_tcp_conn_t *tc);
```

描述：将`tc`中的发送队列上的数据通过套接字发送出去。

这里需要注意，本函数返回并不代表发送队列已经完全发完，这取决于发送队列中每个节点内buf的标识设定，参见本章前面`相关结构`小节。

发送后，已发送数据会被移至已发送队列。用户可以在上层代码自行对以发送队列内的数据进行处理，例如将其释放。

返回值：

- `M_C_FINISH`表示发送完成，当buf的`last_in_chain`被设置时，即便后续还有数据在链上，依旧会返回该值。
- `M_C_NOTYET`表示还有数据未发完
- `M_C_ERROR`表示发送失败



#### mln_tcp_conn_recv

```c
int mln_tcp_conn_recv(mln_tcp_conn_t *tc, mln_u32_t flag);
```

描述：从`tc`中的socket套接字上接收数据，并将数据存放于接收队列（链）中。

`flag`用于指出接收到的数据是存放于内存中还是文件中，其值如下：

- `M_C_TYPE_MEMORY`存放在内存中
- `M_C_TYPE_FILE`存放在文件中
- `M_C_TYPE_FOLLOW`与上一次调用保持一致

返回值：

- `M_C_NOTYET`表示已接收，但可能未收完。但当暂时没有数据可接收时，也会返回此值
- `M_C_ERROR`表示接收出错
- `M_C_CLOSED`表示对方已关闭链接



#### mln_tcp_conn_move_sent

```c
void mln_tcp_conn_move_sent(mln_tcp_conn_t *tc);
```

描述：将发送队列（send queue）中的所有链节点一次性移至已发送队列（sent queue）。移动后发送队列为空。此操作不释放任何资源，仅调整队列指针。

返回值：无



#### mln_tcp_conn_send_chain

```c
int mln_tcp_conn_send_chain(mln_tcp_conn_t *tc, mln_chain_t *chain);
```

描述：将`chain`追加到`tc`的发送队列后立刻调用`mln_tcp_conn_send`进行发送。是`mln_tcp_conn_append` + `mln_tcp_conn_send`的便捷函数。

返回值：与`mln_tcp_conn_send`一致：

- `M_C_FINISH`表示发送完成
- `M_C_NOTYET`表示还有数据未发完
- `M_C_ERROR`表示发送失败



#### mln_tcp_conn_send_empty

```c
mln_tcp_conn_send_empty(pconn)
```

描述：判断发送队列是否为空。

返回值：为空则返回`非0`，否则返回`0`



#### mln_tcp_conn_recv_empty

```c
mln_tcp_conn_recv_empty(pconn)
```

描述：判断接收队列是否为空。

返回值：为空则返回`非0`，否则返回`0`



#### mln_tcp_conn_sent_empty

```c
mln_tcp_conn_sent_empty(pconn)
```

描述：判断已发送队列是否为空。

返回值：为空则返回`非0`，否则返回`0`



#### mln_tcp_conn_fd_get

```c
mln_tcp_conn_fd_get(pconn)
```

描述：获取TCP结构中的套接字描述符。

返回值：套接字描述符



#### mln_tcp_conn_fd_set

```c
void mln_tcp_conn_fd_set(mln_tcp_conn_t *tc, int fd);
```

描述：设置TCP结构中的套接字描述符为`fd`，并自动更新非阻塞标志位。

返回值：无



#### mln_tcp_conn_set_nonblock

```c
int mln_tcp_conn_set_nonblock(mln_tcp_conn_t *tc, int nb);
```

描述：设置或清除TCP连接的非阻塞模式。`nb`为非零值表示设置为非阻塞，为`0`表示设置为阻塞。本函数会通过`fcntl`更新底层套接字标志并同步`tc`内的`nonblock`标志位。

返回值：成功则返回`0`，失败返回`-1`



#### mln_tcp_conn_pool_get

```c
mln_tcp_conn_pool_get(pconn)
```

描述：获取TCP结构中内存池结构。TCP结构中自行创建了内存池，因为输入输出链使用了内存池进行分配和释放。

返回值：`mln_alloc_t`结构指针



### TLS支持

`mln_connection`内建对TLS（HTTPS）的支持，由OpenSSL作为后端实现。整套TLS逻辑被集成在`mln_tcp_conn_t`内部，**任何上层代码（包括`mln_http`）都无需感知或修改**——只要将连接通过`mln_tcp_conn_tls_init`初始化，后续的`mln_tcp_conn_send`、`mln_tcp_conn_recv`即会自动完成加密与解密。

未启用TLS的连接（通过`mln_tcp_conn_init`初始化）走原有路径，性能与历史版本一致：开启`MLN_TLS`编译宏后，明文路径仅在入口处多出一次空指针判断与条件跳转。

#### 启用编译

```bash
./configure --enable-tls
```

`configure`会自动探测常见OpenSSL安装位置（含macOS Homebrew的`/opt/homebrew/opt/openssl@3`等），也可通过`OPENSSL_INCLUDE`和`OPENSSL_LIB`环境变量手动指定。**要求OpenSSL 1.1.0或更高版本；不支持1.0.x**。未找到合适的OpenSSL时会跳过TLS并给出提示，不会让整个构建失败。

未启用`--enable-tls`时，下述接口与字段全部不存在，二进制与历史版本字节一致。

#### 相关常量

```c
/* 角色 */
#define M_TLS_SERVER   0
#define M_TLS_CLIENT   1

/* 协议版本掩码 */
#define M_TLS_V1_2     0x4
#define M_TLS_V1_3     0x8
#define M_TLS_VDEFAULT (M_TLS_V1_2 | M_TLS_V1_3)
```

#### 相关结构

```c
typedef struct mln_tcp_tls_conf_s {
    SSL_CTX        *ctx;
    mln_string_t   *cert_file;
    mln_string_t   *key_file;
    mln_string_t   *ca_file;
    mln_string_t   *ciphers;
    mln_u32_t       versions;    /* M_TLS_V* 标志位掩码 */
    mln_u32_t       role:1;      /* M_TLS_SERVER / M_TLS_CLIENT */
    mln_u32_t       verify:1;
} mln_tcp_tls_conf_t;
```

配置结构可在多个连接之间共享，由调用方负责管理其生命周期。`cert_file`/`key_file`/`ca_file`/`ciphers` 字段由 `mln_tcp_tls_conf_new` 深拷贝持有，调用方传入的 `mln_string_t` 在 `conf_new` 返回后即可释放。



#### mln_tcp_tls_conf_new

```c
mln_tcp_tls_conf_t *
mln_tcp_tls_conf_new(mln_u32_t role,
                     mln_string_t *cert_file,
                     mln_string_t *key_file,
                     mln_string_t *ca_file,
                     mln_string_t *ciphers,
                     mln_u32_t versions,
                     mln_u32_t verify);
```

描述：构造一份TLS配置，封装一个`SSL_CTX`和证书等信息。各参数：

- `role`：`M_TLS_SERVER`时必须提供`cert_file`和`key_file`；`M_TLS_CLIENT`时两者可为`NULL`。
- `ca_file`：PEM格式的受信CA证书集合。开启客户端证书校验时使用。
- `ciphers`：TLSv1.2 cipher list；`NULL`使用OpenSSL默认。
- `versions`：`M_TLS_V1_2 | M_TLS_V1_3`的位掩码，`0`等同于`M_TLS_VDEFAULT`。
- `verify`：非零表示要求验证对端证书。

返回值：成功返回结构指针，失败返回`NULL`



#### mln_tcp_tls_conf_free

```c
void mln_tcp_tls_conf_free(mln_tcp_tls_conf_t *conf);
```

描述：释放配置及其内部`SSL_CTX`。



#### mln_tcp_conn_tls_init

```c
int mln_tcp_conn_tls_init(mln_tcp_conn_t *tc, int sockfd, mln_tcp_tls_conf_t *conf);
```

描述：与`mln_tcp_conn_init`平行的TLS初始化函数。`tc`完成初始化后就是一条TLS连接，后续`mln_tcp_conn_send/recv`自动完成加解密。`conf`必须在`tc`销毁前持续有效（不会被`tc`接管或释放）。

返回值：成功返回`0`，失败返回`-1`



#### mln_tcp_conn_tls_handshake

```c
int mln_tcp_conn_tls_handshake(mln_tcp_conn_t *tc);
```

描述：显式驱动TLS握手。**非必须**——首次调用`mln_tcp_conn_send`或`mln_tcp_conn_recv`时会自动触发。提供该函数主要是为了在事件循环里把"握手完成"和"开始读写"分作两个独立的回调处理。

返回值：

- `M_C_FINISH`握手完成
- `M_C_NOTYET`仍需更多I/O，调用方可参考`mln_tcp_conn_tls_want_read`/`mln_tcp_conn_tls_want_write`重新注册事件
- `M_C_ERROR`握手失败
- `M_C_CLOSED`对端在握手过程中关闭



#### mln_tcp_conn_tls_shutdown

```c
int mln_tcp_conn_tls_shutdown(mln_tcp_conn_t *tc);
```

描述：发送TLS `close_notify`，进行优雅关闭。非阻塞模式下可能返回`M_C_NOTYET`，需待socket可写后重试。

返回值：`M_C_FINISH`/`M_C_NOTYET`/`M_C_ERROR`



#### mln_tcp_conn_tls_set_sni

```c
int mln_tcp_conn_tls_set_sni(mln_tcp_conn_t *tc, mln_string_t *hostname);
```

描述：客户端使用，设置握手时发送的SNI主机名。必须在握手开始之前调用。

返回值：成功返回`0`，失败返回`-1`



#### mln_tcp_conn_tls_set_verify_host

```c
int mln_tcp_conn_tls_set_verify_host(mln_tcp_conn_t *tc, mln_string_t *hostname);
```

描述：客户端使用，对服务端证书启用RFC 6125主机名校验。需配合`mln_tcp_tls_conf_new`时`verify=1`使用。

返回值：成功返回`0`，失败返回`-1`



#### 状态查询宏

```c
mln_tcp_conn_tls_enabled(tc)    /* 当前连接是否为TLS */
mln_tcp_conn_tls_done(tc)       /* 握手是否完成 */
mln_tcp_conn_tls_want_read(tc)  /* 上一次I/O是否在等可读事件 */
mln_tcp_conn_tls_want_write(tc) /* 上一次I/O是否在等可写事件 */
```

非阻塞模式下，`mln_tcp_conn_send/recv/tls_handshake`返回`M_C_NOTYET`时，调用方应根据`want_read`/`want_write`重新设置`mln_event_fd_set`关注的事件——TLS存在"想读其实是想写"（重协商）的场景，必须如实反映到事件层。



#### 行为说明

- 队列内容仅为明文：`snd_*`、`rcv_*`、`sent_*`三条队列中始终只存放明文`mln_chain_t`；密文只在内部栈缓冲与OpenSSL的内存BIO中流动，不会与明文混在同一队列里。
- `sent_*`语义严格：一条chain只有在其对应的密文已经被`writev/send`真正写到socket之后，才会被移入`sent_*`；网络写阻塞时该chain仍然停留在`snd_*`头部，下次调用继续从断点处续传。
- `in_file`类型的buf：会被`mln_tcp_conn_send`内部按16 KiB为单位先`read`到栈缓冲再交给`SSL_write`。注意TLS路径下零拷贝`sendfile`是不可用的。
- `mln_tcp_conn_recv`在TLS连接上的`flag`必须是`M_C_TYPE_MEMORY`；`M_C_TYPE_FILE`本期不支持。
- `mln_tcp_conn_destroy`会自动`SSL_free`，无需调用方额外处理；但不会释放共享的`mln_tcp_tls_conf_t`。



#### 简单示例

服务端：

```c
mln_string_t cert = mln_string("/etc/melon/server.pem");
mln_string_t key  = mln_string("/etc/melon/server.key");
mln_tcp_tls_conf_t *conf = mln_tcp_tls_conf_new(
    M_TLS_SERVER, &cert, &key, NULL, NULL, M_TLS_VDEFAULT, 0);

int fd = accept(listen_fd, NULL, NULL);
mln_tcp_conn_t conn;
mln_tcp_conn_tls_init(&conn, fd, conf);
/* 之后 mln_tcp_conn_send / mln_tcp_conn_recv 即自动走TLS */
```

客户端：

```c
mln_tcp_tls_conf_t *conf = mln_tcp_tls_conf_new(
    M_TLS_CLIENT, NULL, NULL, &ca_bundle, NULL, M_TLS_VDEFAULT, 1);

int fd = ...; /* connect 之后 */
mln_tcp_conn_t conn;
mln_tcp_conn_tls_init(&conn, fd, conf);

mln_string_t host = mln_string("example.com");
mln_tcp_conn_tls_set_sni(&conn, &host);
mln_tcp_conn_tls_set_verify_host(&conn, &host);
```



### 示例

本篇示例碍于篇幅，仅给出部分片段展示如何使用。

片段摘自Melang脚本语言中TCP收发部分。

```c
//发送数据
static void mln_lang_network_tcp_send_handler(mln_event_t *ev, int fd, void *data)
{
    mln_lang_tcp_t *tcp = (mln_lang_tcp_t *)data;
    int rc = mln_tcp_conn_send(&(tcp->conn));//发送数据

    if (rc == M_C_FINISH || rc == M_C_NOTYET) {//发送成功
        mln_chain_pool_release_all(mln_tcp_conn_remove(&(tcp->conn), M_C_SENT));//清除已发送队列
        if (mln_tcp_conn_head(&(tcp->conn), M_C_SEND) != NULL) {//判断是否还有未发送数据
            ...
        } else {
            ...
        }
    }
    ...
}

//接收数据
static void mln_lang_network_tcp_recv_handler(mln_event_t *ev, int fd, void *data)
{   
    mln_lang_tcp_t *tcp = (mln_lang_tcp_t *)data;
    mln_s64_t size = 0;
    mln_u8ptr_t buf, p;
    mln_string_t tmp;
    mln_chain_t *c;
    int rc = mln_tcp_conn_recv(&(tcp->conn), M_C_TYPE_MEMORY);//接收数据
    if (rc == M_C_ERROR) {//如果出错
        ...
    } else if (rc == M_C_CLOSED && mln_tcp_conn_head(&(tcp->conn), M_C_RECV) == NULL) {//如果对端关闭，且本地已收到数据
        ...
    } else {
        c = mln_tcp_conn_head(&(tcp->conn), M_C_RECV);//获取接收队列
        for (; c != NULL; c = c->next) {
            if (c->buf == NULL) continue;
            size += mln_buf_left_size(c->buf);//计算接收队列数据总大小
        }
        ...//一些处理
        mln_chain_pool_release_all(mln_tcp_conn_remove(&(tcp->conn), M_C_RECV));//释放接收队列
    }
    ...
}
```

