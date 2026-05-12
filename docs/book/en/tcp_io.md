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



#### mln_chain_new_with_buf

```c
mln_chain_t *mln_chain_new_with_buf(mln_alloc_t *pool);
```

Description: Create a chain structure from the memory pool `pool` and allocate a `buf` structure for it. This is a convenience function equivalent to calling `mln_chain_new` followed by `mln_buf_new`.

Return value: If successful, return the chain structure pointer (with its `buf` member initialized), otherwise return `NULL`



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



#### mln_tcp_conn_move_sent

```c
void mln_tcp_conn_move_sent(mln_tcp_conn_t *tc);
```

Description: Move all chain nodes from the send queue to the sent queue in one operation. After this call the send queue is empty. This operation does not release any resources, it only adjusts the queue pointers.

Return value: none



#### mln_tcp_conn_send_chain

```c
int mln_tcp_conn_send_chain(mln_tcp_conn_t *tc, mln_chain_t *chain);
```

Description: Append `chain` to the send queue of `tc` and immediately call `mln_tcp_conn_send` to send. This is a convenience function combining `mln_tcp_conn_append` and `mln_tcp_conn_send`.

Return value: Same as `mln_tcp_conn_send`:

- `M_C_FINISH` indicates that the transmission is completed
- `M_C_NOTYET` indicates that there is still data to be sent
- `M_C_ERROR` means sending failed



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
void mln_tcp_conn_fd_set(mln_tcp_conn_t *tc, int fd);
```

Description: Set the socket descriptor in the TCP structure to `fd` and automatically update the non-blocking flag.

Return value: none



#### mln_tcp_conn_set_nonblock

```c
int mln_tcp_conn_set_nonblock(mln_tcp_conn_t *tc, int nb);
```

Description: Set or clear the non-blocking mode on the TCP connection. A non-zero `nb` sets non-blocking mode, `0` sets blocking mode. This function updates the underlying socket flags via `fcntl` and synchronizes the `nonblock` flag in `tc`.

Return value: return `0` on success, `-1` on failure



#### mln_tcp_conn_pool_get

```c
mln_tcp_conn_pool_get(pconn)
```

Description: Get the memory pool structure in the TCP structure. The memory pool is created by itself in the TCP structure, because the input and output chain uses the memory pool for allocation and release.

Return value: `mln_alloc_t` structure pointer



### TLS support

`mln_connection` has built-in TLS (HTTPS) support, backed by OpenSSL.
All TLS logic is folded into `mln_tcp_conn_t`, so **no upper-layer code
needs to change** — `mln_http` and any other consumer keeps using
`mln_tcp_conn_send` / `mln_tcp_conn_recv`. A connection initialized with
`mln_tcp_conn_tls_init` is automatically a TLS connection; subsequent
sends and receives transparently encrypt and decrypt.

Connections initialized through the plain `mln_tcp_conn_init` path are
byte-for-byte unchanged on the hot path. When the `MLN_TLS` macro is
enabled, the plain path only pays the cost of one extra load and one
conditional branch at the very top of `mln_tcp_conn_send/recv`.

#### Enable at build time

```bash
./configure --enable-tls
```

`configure` probes common OpenSSL install locations (including macOS
Homebrew layouts such as `/opt/homebrew/opt/openssl@3`), and accepts
`OPENSSL_INCLUDE` and `OPENSSL_LIB` env vars for overrides. OpenSSL
**1.1.0 or later is required**; OpenSSL 1.0.x is not supported. If
a suitable OpenSSL cannot be found the flag is reported as ignored and
the rest of the build proceeds normally.

When `--enable-tls` is **not** passed, none of the APIs and fields
below exist, and the resulting binary is identical to historical
builds.

#### Constants

```c
/* Role */
#define M_TLS_SERVER   0
#define M_TLS_CLIENT   1

/* Protocol version bitmask */
#define M_TLS_V1_2     0x4
#define M_TLS_V1_3     0x8
#define M_TLS_VDEFAULT (M_TLS_V1_2 | M_TLS_V1_3)
```

#### Structures

```c
typedef struct mln_tcp_tls_conf_s {
    SSL_CTX        *ctx;
    mln_string_t   *cert_file;
    mln_string_t   *key_file;
    mln_string_t   *ca_file;
    mln_string_t   *ciphers;
    mln_u32_t       versions;    /* bitmask of M_TLS_V* constants */
    mln_u32_t       role:1;      /* M_TLS_SERVER / M_TLS_CLIENT */
    mln_u32_t       verify:1;
} mln_tcp_tls_conf_t;
```

A configuration object is intended to be shared across many
connections; the caller owns its lifetime.  The `cert_file`,
`key_file`, `ca_file` and `ciphers` fields are deep-copied by
`mln_tcp_tls_conf_new`, so the caller may free the `mln_string_t`
arguments immediately after construction.



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

Description: Build a TLS configuration that wraps an `SSL_CTX` and
certificate material. Parameters:

- `role`: `M_TLS_SERVER` requires both `cert_file` and `key_file`;
  `M_TLS_CLIENT` may pass `NULL` for both.
- `ca_file`: PEM bundle of trusted CAs; enables peer verification when
  `verify` is non-zero.
- `ciphers`: TLSv1.2 cipher list; `NULL` for OpenSSL's default.
- `versions`: bitmask of `M_TLS_V1_2 | M_TLS_V1_3`; `0` is treated as
  `M_TLS_VDEFAULT`.
- `verify`: non-zero requires peer certificate verification.

Return value: structure pointer on success, `NULL` on failure.



#### mln_tcp_tls_conf_free

```c
void mln_tcp_tls_conf_free(mln_tcp_tls_conf_t *conf);
```

Description: Free the configuration along with its `SSL_CTX`.



#### mln_tcp_conn_tls_init

```c
int mln_tcp_conn_tls_init(mln_tcp_conn_t *tc, int sockfd, mln_tcp_tls_conf_t *conf);
```

Description: The TLS counterpart of `mln_tcp_conn_init`. After this
returns successfully `tc` represents a TLS connection; subsequent
`mln_tcp_conn_send` / `mln_tcp_conn_recv` calls handle encryption and
decryption automatically. `conf` must outlive `tc`; it is not
consumed.

Return value: `0` on success, `-1` on failure.



#### mln_tcp_conn_tls_handshake

```c
int mln_tcp_conn_tls_handshake(mln_tcp_conn_t *tc);
```

Description: Drive the TLS handshake explicitly. **Optional** — the
first call to `mln_tcp_conn_send` or `mln_tcp_conn_recv` will do it
automatically. The standalone entry point is convenient when an event
loop wants to handle "handshake done" and "ready for I/O" as two
separate callbacks.

Return value:

- `M_C_FINISH` handshake finished
- `M_C_NOTYET` more I/O needed; the caller should consult
  `mln_tcp_conn_tls_want_read` / `mln_tcp_conn_tls_want_write` and
  re-register the appropriate event
- `M_C_ERROR` handshake failed
- `M_C_CLOSED` peer hung up mid-handshake



#### mln_tcp_conn_tls_shutdown

```c
int mln_tcp_conn_tls_shutdown(mln_tcp_conn_t *tc);
```

Description: Send TLS `close_notify` for a graceful shutdown.
Non-blocking callers may receive `M_C_NOTYET` and need to retry once
the socket is writable.

Return value: `M_C_FINISH` / `M_C_NOTYET` / `M_C_ERROR`.



#### mln_tcp_conn_tls_set_sni

```c
int mln_tcp_conn_tls_set_sni(mln_tcp_conn_t *tc, mln_string_t *hostname);
```

Description: Client side. Set the SNI hostname sent during the
handshake. Must be called before the handshake starts.

Return value: `0` on success, `-1` on failure.



#### mln_tcp_conn_tls_set_verify_host

```c
int mln_tcp_conn_tls_set_verify_host(mln_tcp_conn_t *tc, mln_string_t *hostname);
```

Description: Client side. Enable RFC 6125 hostname verification of the
server certificate. Use together with `verify=1` in
`mln_tcp_tls_conf_new`.

Return value: `0` on success, `-1` on failure.



#### Status macros

```c
mln_tcp_conn_tls_enabled(tc)    /* is this a TLS connection? */
mln_tcp_conn_tls_done(tc)       /* has the handshake completed? */
mln_tcp_conn_tls_want_read(tc)  /* did the last I/O ask for a readable event? */
mln_tcp_conn_tls_want_write(tc) /* did the last I/O ask for a writable event? */
```

In non-blocking mode, when `mln_tcp_conn_send`, `mln_tcp_conn_recv` or
`mln_tcp_conn_tls_handshake` returns `M_C_NOTYET`, the caller must
re-register fd interest using `want_read` / `want_write`. TLS has a
legitimate "want read while doing a write" condition (renegotiation),
and the event loop has to honour that faithfully.



#### Behaviour notes

- Queues hold plaintext only: `snd_*`, `rcv_*` and `sent_*` always
  contain plaintext `mln_chain_t`. Ciphertext only ever lives on the
  stack and inside OpenSSL's memory BIOs, so the receive queue can
  never become a mixture of decrypted and yet-to-be-decrypted bytes.
- Strict `sent_*` semantics: a chain only moves to `sent_*` after its
  matching ciphertext has actually been written to the socket. If the
  socket fills up, the chain stays at the head of `snd_*` and the next
  call resumes from where it left off.
- `in_file` bufs: read into a 16 KiB stack buffer in 16 KiB chunks
  before being fed to `SSL_write`. Zero-copy `sendfile` is not
  available on the TLS path.
- `mln_tcp_conn_recv` on a TLS connection accepts only
  `M_C_TYPE_MEMORY`; `M_C_TYPE_FILE` is not supported in this release.
- `mln_tcp_conn_destroy` calls `SSL_free` for you; it does not free
  the shared `mln_tcp_tls_conf_t`.



#### Quick examples

Server:

```c
mln_string_t cert = mln_string("/etc/melon/server.pem");
mln_string_t key  = mln_string("/etc/melon/server.key");
mln_tcp_tls_conf_t *conf = mln_tcp_tls_conf_new(
    M_TLS_SERVER, &cert, &key, NULL, NULL, M_TLS_VDEFAULT, 0);

int fd = accept(listen_fd, NULL, NULL);
mln_tcp_conn_t conn;
mln_tcp_conn_tls_init(&conn, fd, conf);
/* Now mln_tcp_conn_send / mln_tcp_conn_recv transparently use TLS. */
```

Client:

```c
mln_tcp_tls_conf_t *conf = mln_tcp_tls_conf_new(
    M_TLS_CLIENT, NULL, NULL, &ca_bundle, NULL, M_TLS_VDEFAULT, 1);

int fd = ...; /* after connect() */
mln_tcp_conn_t conn;
mln_tcp_conn_tls_init(&conn, fd, conf);

mln_string_t host = mln_string("example.com");
mln_tcp_conn_tls_set_sni(&conn, &host);
mln_tcp_conn_tls_set_verify_host(&conn, &host);
```



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

