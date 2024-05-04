## TCP I/O

The sending and receiving of the CP will vary depending on whether it is blocking or non-blocking. Whether it is non-blocking or not is set by the user (it can be set in the event interface).

In Windows, the performance will be relatively low, because win does not provide the relevant system interface to obtain whether the socket is in non-blocking mode.



### Header file

```c
#include "mln_connection.h"
```



### Module

`connection`



### Structures

```c
typedef struct mln_buf_s {//Used to store data, and specify the data storage location (file or memory) according to different identifiers, and also mark the location where the current data is processed
    mln_u8ptr_t         left_pos;//The location to which the current data is processed
    mln_u8ptr_t         pos;//The starting position of the data in this block of memory
    mln_u8ptr_t         last;//The end position of the data in this block of memory
    mln_u8ptr_t         start;//The starting position of this block of memory
    mln_u8ptr_t         end;//The end of this block of memory
    struct mln_buf_s   *shadow;//Whether there are other buf structures pointing to the same memory block
    mln_off_t           file_left_pos;//The file offset to which the current data is processed
    mln_off_t           file_pos;//The starting offset of the data within this file
    mln_off_t           file_last;//end offset of data within this file
    mln_file_t         *file;//File structure, see the introduction of the file collection section
    mln_u32_t           temporary:1;//Whether the memory pointed to by memory pointers such as start and pos is temporary (that is, does not need to be released)
#if !defined(MSVC)
    mln_u32_t           mmap:1;//Whether it is the memory created by mmap, it is not supported under win
#endif
    mln_u32_t           in_memory:1;//Is the data in memory
    mln_u32_t           in_file:1;//Is the data in the file
    mln_u32_t           flush:1;//Whether to send the data immediately (this flag has not been used for now)
    mln_u32_t           sync:1;//This tag has not been used at this time
    mln_u32_t           last_buf:1;//Whether this buf is the last buf in the shadow substitute, when there is no substitute, I am the last one
    mln_u32_t           last_in_chain:1;//Marks whether this buf is the last buf on the chain, this mark is used for the tcp sending part. When this tag is encountered, even if there are nodes after the chain node where this buf is located, it will immediately return to the upper layer and indicate that the data transmission is complete. If you want to continue sending, you need to call the send function again
} mln_buf_t;

typedef struct mln_chain_s { //buf singly linked list for tcp sending and receiving data
    mln_buf_t          *buf;
    struct mln_chain_s *next;
} mln_chain_t;
```



### Functions/Macros



#### mln_buf_size

```c
mln_buf_size(pbuf)
```

Description: Get the size of the data pointed to by this `pbuf`, that is, the size of `last-pos`/`file_last-file_last`.

Return value: number of bytes



#### mln_buf_left_size

```c
mln_buf_left_size(pbuf)
```

Description: Get the size of the data that has not been processed by this `pbuf`, that is, the size of `last-left_pos`/``file_last-file_left_pos`,

Return value: number of bytes



#### mln_chain_add

```c
mln_chain_add(pphead,pptail,c)
```

Description: Add the chain node `c` to the chain `pphead` `pptail`, where: `pphead` and `pptail` are secondary pointers to the head and tail of the singly linked list, respectively.

Return value: none



#### mln_buf_new

```c
mln_buf_t *mln_buf_new(mln_alloc_t *pool);
```

Description: Create a buf structure from the memory pool `pool`.

Return value: return the buf structure pointer if successful, otherwise return `NULL`



#### mln_chain_new

```c
mln_chain_t *mln_chain_new(mln_alloc_t *pool);
```

Description: Create a chain structure from the memory pool `pool`.

Return value: If successful, return the chain structure pointer, otherwise return `NULL`



#### mln_buf_pool_release

```c
void mln_buf_pool_release(mln_buf_t *b);
```

Description: Release buf and its internal resources.

Return value: none



#### mln_chain_pool_release

```c
void mln_chain_pool_release(mln_chain_t *c);
```

Description: Release the chain node `c` and its internal resources.

Return value: none



#### mln_chain_pool_release_all

```c
void mln_chain_pool_release_all(mln_chain_t *c);
```

Description: Release the entire chain and its internal resources referred to by `c`.

Return value: none



#### mln_tcp_conn_init

```c
int mln_tcp_conn_init(mln_tcp_conn_t *tc, int sockfd);
```

Description: Initialize the TCP structure, where:

- `tc` is a pointer to a TCP structure. The memory pointed to may come from the stack, heap or shared memory, etc., as needed, and it is up to the user to decide.
- `sockfd` is the TCP socket socket descriptor.

Return value: return `0` if successful, otherwise return `-1`



#### mln_tcp_conn_destroy

```c
void mln_tcp_conn_destroy(mln_tcp_conn_t *tc);
```

Description: Destroy the resource structure inside `tc`. **Note**: This function will not close the socket, nor will it release the memory of `tc` itself, which needs to be handled externally.

Return value: none



#### mln_tcp_conn_append_chain

```c
void mln_tcp_conn_append_chain(mln_tcp_conn_t *tc, mln_chain_t *c_head, mln_chain_t *c_tail, int type);
```

Description: Append the chain represented by `c_head` and `c_tail` to the specified chain in the `tc` structure. `type` has the following values:

- `M_C_SEND` represents the sending chain, that is, the sending queue
- `M_C_RECV` indicates the receive queue
- `M_C_SENT` means sent queue

Return value: none



#### mln_tcp_conn_append

```c
void mln_tcp_conn_append(mln_tcp_conn_t *tc, mln_chain_t *c, int type);
```

Description: Append the chain `c` to the specified queue in `tc`, the value of `type` is the same as `mln_tcp_conn_append_chain`. This function only omits the tail pointer of the append chain, so the overhead of this function will be slightly larger than `mln_tcp_conn_append_chain`.

Return value: none



#### mln_tcp_conn_head

```c
mln_chain_t *mln_tcp_conn_head(mln_tcp_conn_t *tc, int type);
```

Description: Get the head pointer of the specified chain (queue) in `tc`. The value of `type` is the same as `mln_tcp_conn_append_chain`.

Return value: If there is data, it is the chain node pointer, if there is no data, it is `NULL`



#### mln_tcp_conn_remove

```c
mln_chain_t *mln_tcp_conn_remove(mln_tcp_conn_t *tc, int type);
```

Description: Disassemble the entire chain (queue) specified in `tc` and return its head pointer. The value of `type` is the same as `mln_tcp_conn_append_chain`.

Return value: If there is data, it is the chain node pointer, if there is no data, it is `NULL`



#### mln_tcp_conn_pop

```c
mln_chain_t *mln_tcp_conn_pop(mln_tcp_conn_t *tc, int type);
```

Description: Get the first chain node of the specified chain in `tc`. The value of `type` is the same as `mln_tcp_conn_append_chain`.

Return value: If there is data, it is the chain node pointer, if there is no data, it is `NULL`



#### mln_tcp_conn_tail

```c
mln_chain_t *mln_tcp_conn_tail(mln_tcp_conn_t *tc, int type);
```

Description: Get the tail pointer of the specified chain (queue) in `tc`. The value of `type` is the same as `mln_tcp_conn_append_chain`.

Return value: If there is data, it is the chain node pointer, if there is no data, it is `NULL`



#### mln_tcp_conn_send

```c
int mln_tcp_conn_send(mln_tcp_conn_t *tc);
```

Description: Send the data on the send queue in `tc` through the socket.

It should be noted here that the return of this function does not mean that the sending queue has been completely sent, which depends on the setting of the buf identifier in each node in the sending queue, see the `Related Structures` section earlier in this chapter.

After sending, sent data is moved to the sent queue. Users can process the data in the sending queue by themselves in the upper-level code, such as releasing it.

return value:

- `M_C_FINISH` indicates that the transmission is completed. When the `last_in_chain` of buf is set, even if there is still data on the chain, this value will still be returned.
- `M_C_NOTYET` indicates that there is still data to be sent
- `M_C_ERROR` means sending failed



#### mln_tcp_conn_recv

```c
int mln_tcp_conn_recv(mln_tcp_conn_t *tc, mln_u32_t flag);
```

Description: Receive data from the socket socket in `tc` and store the data in the receive queue (chain).

`flag` is used to indicate whether the received data is stored in memory or in a file, and its values are as follows:

- `M_C_TYPE_MEMORY` is stored in memory
- `M_C_TYPE_FILE` is stored in a file
- `M_C_TYPE_FOLLOW` is consistent with the last call

return value:

- `M_C_NOTYET` indicates that it has been received, but may not have been received. But when there is no data to receive temporarily, this value will also be returned
- `M_C_ERROR` indicates a receive error
- `M_C_CLOSED` indicates that the other party has closed the link



#### mln_tcp_conn_send_empty

```c
mln_tcp_conn_send_empty(pconn)
```

Description: Determine whether the send queue is empty.

Return value: if empty, return `non-0`, otherwise return `0`



#### mln_tcp_conn_recv_empty

```c
mln_tcp_conn_recv_empty(pconn)
```

Description: Determine whether the receive queue is empty.

Return value: if empty, return `non-0`, otherwise return `0`



#### mln_tcp_conn_sent_empty

```c
mln_tcp_conn_sent_empty(pconn)
```

Description: Determine if the sent queue is empty.

Return value: if empty, return `non-0`, otherwise return `0`



#### mln_tcp_conn_fd_get

```c
mln_tcp_conn_fd_get(pconn)
```

Description: Get the socket descriptor in the TCP structure.

Return value: socket descriptor



#### mln_tcp_conn_fd_set

```c
mln_tcp_conn_fd_set(pconn,fd)
```

Description: Set the socket descriptor in the TCP structure to `fd`.

Return value: none



#### mln_tcp_conn_pool_get

```c
mln_tcp_conn_pool_get(pconn)
```

Description: Get the memory pool structure in the TCP structure. The memory pool is created by itself in the TCP structure, because the input and output chain uses the memory pool for allocation and release.

Return value: `mln_alloc_t` structure pointer



### Example

Due to the space of this example, only some fragments are given to show how to use it.

The snippet is taken from the TCP Transceiver section in the Melang scripting language.

```c
//send data
static void mln_lang_network_tcp_send_handler(mln_event_t *ev, int fd, void *data)
{
    mln_lang_tcp_t *tcp = (mln_lang_tcp_t *)data;
    int rc = mln_tcp_conn_send(&(tcp->conn));//send

    if (rc == M_C_FINISH || rc == M_C_NOTYET) {//on success
        mln_chain_pool_release_all(mln_tcp_conn_remove(&(tcp->conn), M_C_SENT));//clean sent queue
        if (mln_tcp_conn_head(&(tcp->conn), M_C_SEND) != NULL) {//any data not sent
            ...
        } else {
            ...
        }
    }
    ...
}

//receive
static void mln_lang_network_tcp_recv_handler(mln_event_t *ev, int fd, void *data)
{   
    mln_lang_tcp_t *tcp = (mln_lang_tcp_t *)data;
    mln_s64_t size = 0;
    mln_u8ptr_t buf, p;
    mln_string_t tmp;
    mln_chain_t *c;
    int rc = mln_tcp_conn_recv(&(tcp->conn), M_C_TYPE_MEMORY);//receive
    if (rc == M_C_ERROR) {//on error
        ...
    } else if (rc == M_C_CLOSED && mln_tcp_conn_head(&(tcp->conn), M_C_RECV) == NULL) {//closed
        ...
    } else {
        c = mln_tcp_conn_head(&(tcp->conn), M_C_RECV);//get receive queue
        for (; c != NULL; c = c->next) {
            if (c->buf == NULL) continue;
            size += mln_buf_left_size(c->buf);//calculate queue size in byte
        }
        ...
        mln_chain_pool_release_all(mln_tcp_conn_remove(&(tcp->conn), M_C_RECV));//clean receive queue
    }
    ...
}
```

