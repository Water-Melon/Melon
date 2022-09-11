## I/O线程

I/O线程算是一种另类线程池结构。但是这个组件主要用于图形界面类的应用。通常情况下，图形界面应用都会存在一个用户线程和一个I/O线程，这样当I/O处理时就不会无法响应用户的操作（如：点击）了。



### 头文件

```c
#include "mln_iothread.h"
```



### 函数/宏



#### mln_iothread_init

```c
int mln_iothread_init(mln_iothread_t *t, struct mln_iothread_attr *attr);

struct mln_iothread_attr {
    mln_u32_t                   nthread; //几个I/O线程
    mln_iothread_entry_t        entry; //I/O线程入口函数
    void                       *args; //I/O线程入口参数
    mln_iothread_msg_process_t  handler; //消息处理函数
};

typedef void *(*mln_iothread_entry_t)(void *); //线程入口
typedef void (*mln_iothread_msg_process_t)(mln_iothread_t *t, mln_iothread_ep_type_t from, mln_u32_t type, void *data);//消息处理函数
```

描述：依据`attr`对`t`进行初始化。

返回值：成功返回`0`，否则返回`-1`


#### mln_iothread_destroy

```c
void mln_iothread_destroy(mln_iothread_t *t);
```

描述：销毁一个iothread实例。

返回值：无



#### mln_iothread_send

```c
extern int mln_iothread_send(mln_iothread_t *t, mln_u32_t type, void *data, mln_iothread_ep_type_t to, int feedback);
```

描述：发送一个消息类型为`type`，消息数据为`data`的消息给`to`的一端，并根据`feedback`来确定是否阻塞等待反馈。

返回值：

- `0` - 成功
- `-1` - 失败
- `1` - 发送缓冲区满



#### mln_iothread_recv

```c
int mln_iothread_recv(mln_iothread_t *t, mln_iothread_ep_type_t from);
```

描述：从`from`的一端接收消息。接收后会调用初始化时设置好的消息处理函数，对消息进行处理。

返回值：

- `0` - 成功
- `-1` - 失败



#### mln_iothread_sockfd_get

```c
 mln_iothread_iofd_get(p,t)
```

描述：从`p`所指代的`mln_iothread_t`结构中，根据`t`的值，获取I/O线程或用户线程的通信套接字。一般是为了将其加入到事件中。

返回值：套接字描述符

**注意**：套接字仅是用来通知对方线程（或线程组），另一端线程（或线程组）有消息发送过来，用户可以使用epoll、kqueue、select等事件机制进行监听。



### 示例

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

