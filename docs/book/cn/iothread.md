## I/O线程

I/O线程算是一种另类线程池结构。但是这个组件主要用于图形界面类的应用。通常情况下，图形界面应用都会存在一个用户线程和一个I/O线程，这样当I/O处理时就不会无法响应用户的操作（如：点击）了。

本模块在MSVC环境中暂不支持。




### 头文件

```c
#include "mln_iothread.h"
```



### 模块名

`iothread`



### 函数/宏



#### mln_iothread_init

```c
int mln_iothread_init(mln_iothread_t *t, mln_u32_t nthread, mln_iothread_entry_t entry, void *args, mln_iothread_msg_process_t handler);

typedef void *(*mln_iothread_entry_t)(void *); //线程入口
typedef void (*mln_iothread_msg_process_t)(mln_iothread_t *t, mln_iothread_ep_type_t from, mln_iothread_msg_t *msg);//消息处理函数
```

描述：依据`attr`对`t`进行初始化，参数：

- `nthread` I/O线程数量
- `entry` I/O线程入口函数
- `args` I/O线程入口参数
- `handler` 消息处理函数

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

返回值：已接收并处理的消息个数



#### mln_iothread_sockfd_get

```c
 mln_iothread_iofd_get(p,t)
```

描述：从`p`所指代的`mln_iothread_t`结构中，根据`t`的值，获取I/O线程或用户线程的通信套接字。一般是为了将其加入到事件中。

返回值：套接字描述符

**注意**：套接字仅是用来通知对方线程（或线程组），另一端线程（或线程组）有消息发送过来，用户可以使用epoll、kqueue、select等事件机制进行监听。



#### mln_iothread_msg_hold

```c
mln_iothread_msg_hold(m)
```

描述：将消息`m`持有，此时消息处理函数返回，该消息也不会被释放。主要用于流程较长的场景。该函数仅作用于`feedback`类型的消息，无需反馈的消息理论上也不需要持有，因为不在乎消息被处理的结果。

返回值：无



#### mln_iothread_msg_release

```c
mln_iothread_msg_release(m)
```

描述：释放持有的消息。该消息应该是`feedback`类型消息，非该类型消息则可能导致执行流程异常。

返回值：无



#### mln_iothread_msg_type

```c
mln_iothread_msg_type(m)
```

描述：获取消息的消息类型。

返回值：无符号整型



#### mln_iothread_msg_data

```c
mln_iothread_msg_data(m)
```

描述：获取消息的用户自定义数据。

返回值：用户自定义数据结构指针



### 示例

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

