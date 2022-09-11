## I/O Thread

I/O thread is another kind of thread pool. But this component is mainly used for GUI applications. Usually, GUI applications will have one user thread and one I/O thread, so that when I/O is processed, it will not prevent to respond to user operations (such as click).



### Header file

```c
#include "mln_iothread.h"
```



### Functions/Macros



#### mln_iothread_init

```c
int mln_iothread_init(mln_iothread_t *t, struct mln_iothread_attr *attr);

struct mln_iothread_attr {
    mln_u32_t                   nthread; //Total number of I/O threads
    mln_iothread_entry_t        entry; //I/O thread entry function
    void                       *args; //I/O thread entry parameters
    mln_iothread_msg_process_t  handler; //message handler
};

typedef void *(*mln_iothread_entry_t)(void *); //I/O thread entry function
typedef void (*mln_iothread_msg_process_t)(mln_iothread_t *t, mln_iothread_ep_type_t from, mln_u32_t type, void *data);//message handler
```

Description: Initialize `t` according to `attr`.

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

- `0` - success
- `-1` - failed
- `1` - send buffer full



#### mln_iothread_recv

```c
int mln_iothread_recv(mln_iothread_t *t, mln_iothread_ep_type_t from);
```

Description: Receive a message from the side of `from`. After receiving, the message processing function set during initialization will be called to process the message.

Return value:

- `0` - success
- `-1` - failed



#### mln_iothread_sockfd_get

```c
 mln_iothread_iofd_get(p)
```

Description: According to the value of `t`, get the communication socket of the I/O thread or user thread from the `mln_iothread_t` structure pointed to by `p` . Usually to add it to an event.

Return value: socket descriptor

**Note**: The socket is only used to notify the other thread(s) that the thread(s) at the other end has a message sent. User can use epoll, kqueue, select and other event mechanisms to monitor.




### Example

```c
#include "mln_iothread.h"
#include <string.h>
#include <stdio.h>
#include <errno.h>

static void msg_handler(mln_iothread_t *t, mln_iothread_ep_type_t from, mln_u32_t type, void *data)
{
    printf("msg type: %u\n", type);
}

static void *entry(void *args)
{
    mln_iothread_t *t = (mln_iothread_t *)args;
    while (1) {
        if (mln_iothread_recv(t, user_thread) < 0) {
            fprintf(stderr, "recv failed\n");
            sleep(3);
        }
    }

    return NULL;
}

int main(void)
{
    int i, rc;
    mln_iothread_t t;
    struct mln_iothread_attr tattr;

    tattr.nthread = 1;
    tattr.entry = (mln_iothread_entry_t)entry;
    tattr.args = &t;
    tattr.handler = (mln_iothread_msg_process_t)msg_handler;
    if (mln_iothread_init(&t, &tattr) < 0) {
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

