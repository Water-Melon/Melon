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
mln_tcp_conn_fd_set(pconn,fd)
```

描述：设置TCP结构中的套接字描述符为`fd`。

返回值：无



#### mln_tcp_conn_pool_get

```c
mln_tcp_conn_pool_get(pconn)
```

描述：获取TCP结构中内存池结构。TCP结构中自行创建了内存池，因为输入输出链使用了内存池进行分配和释放。

返回值：`mln_alloc_t`结构指针



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

