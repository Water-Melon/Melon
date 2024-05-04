## I/O Thread

I/O thread is another kind of thread pool. But this component is mainly used for GUI applications. Usually, GUI applications will have one user thread and one I/O thread, so that when I/O is processed, it will not prevent to respond to user operations (such as click).

This module is not supported in the MSVC.


### Header file

```c
#include "mln_iothread.h"
```



### Module

`iothread`



### Functions/Macros



#### mln_iothread_init

```c
int mln_iothread_init(mln_iothread_t *t, mln_u32_t nthread, mln_iothread_entry_t entry, void *args, mln_iothread_msg_process_t handler);

typedef void *(*mln_iothread_entry_t)(void *); //I/O thread entry function
typedef void (*mln_iothread_msg_process_t)(mln_iothread_t *t, mln_iothread_ep_type_t from, mln_iothread_msg_t *msg);//message handler
```

Description: Initialize `t` according to `attr`.

- `nthread` Total number of I/O threads.
- `entry` I/O thread entry function.
- `args` I/O thread entry parameters.
- `handler` message handler.

Return value: return `0` on success, otherwise return `-1`


#### mln_iothread_destroy

```c
void mln_iothread_destroy(mln_iothread_t *t);
```

Description: Destroy an iothread instance.

Return value: none



#### mln_iothread_send

```c
extern int mln_iothread_send(mln_iothread_t *t, mln_u32_t type, void *data, mln_iothread_ep_type_t to, int feedback);

typedef enum {
    io_thread,
    user_thread
} mln_iothread_ep_type_t;
```

Description: Send a message with message type `type` and message data `data` to the destination `to`, and determine whether to block waiting for feedback according to `feedback`.

Return value:

- `0` - on success
- `-1` - on failure
- `1` - send buffer full



#### mln_iothread_recv

```c
int mln_iothread_recv(mln_iothread_t *t, mln_iothread_ep_type_t from);
```

Description: Receive a message from the side of `from`. After receiving, the message processing function set during initialization will be called to process the message.

Return value: The number of received messages.



#### mln_iothread_sockfd_get

```c
 mln_iothread_iofd_get(p)
```

Description: According to the value of `t`, get the communication socket of the I/O thread or user thread from the `mln_iothread_t` structure pointed to by `p` . Usually to add it to an event.

Return value: socket descriptor

**Note**: The socket is only used to notify the other thread(s) that the thread(s) at the other end has a message sent. User can use epoll, kqueue, select and other event mechanisms to monitor.




#### mln_iothread_msg_hold

```c
mln_iothread_msg_hold(m)
```

Description: Hold message `m`. Which means this message won't be freed if the message handler returned. This macro only work on `feedback` message.

Return value: None



#### mln_iothread_msg_release

```c
mln_iothread_msg_release(m)
```

Description: Release message `m`. This macro only work on `feedback` message.

Return value: None



#### mln_iothread_msg_type

```c
mln_iothread_msg_type(m)
```

Description: Get message type.

Return value: unsigned int type



#### mln_iothread_msg_data

```c
mln_iothread_msg_data(m)
```

Description: Get user data from message `m`.

Return value: user data pointer



### Example

```c
#include "mln_iothread.h"
#include <string.h>
#include <stdio.h>
#include <errno.h>

static void msg_handler(mln_iothread_t *t, mln_iothread_ep_type_t from, mln_iothread_msg_t *msg)
{
    mln_u32_t type = mln_iothread_msg_type(msg);
    printf("msg type: %u\n", type);
}

static void *entry(void *args)
{
    int n;
    mln_iothread_t *t = (mln_iothread_t *)args;
    while (1) {
        n = mln_iothread_recv(t, user_thread);
        printf("recv %d message(s)\n", n);
    }

    return NULL;
}

int main(void)
{
    int i, rc;
    mln_iothread_t t;

    if (mln_iothread_init(&t, 1, (mln_iothread_entry_t)entry, &t, (mln_iothread_msg_process_t)msg_handler) < 0) {
        fprintf(stderr, "iothread init failed\n");
        return -1;
    }
    for (i = 0; i < 1000000; ++i) {
        if ((rc = mln_iothread_send(&t, i, NULL, io_thread, 1)) < 0) {
            fprintf(stderr, "send failed\n");
            return -1;
        } else if (rc > 0)
            continue;
    }
    sleep(1);
    mln_iothread_destroy(&t);
    sleep(3);
    printf("DONE\n");

    return 0;
}
```

