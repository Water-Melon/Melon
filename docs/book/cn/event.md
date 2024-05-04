## 事件

事件所用系统调用根据不同操作系统平台有所不同，现支持：

- epoll
- kqueue
- select

`msvc`环境中，本模块不是线程安全的。


### 头文件

```c
#include "mln_event.h"
```



### 模块名

`event`



### 函数



#### mln_event_new

```c
mln_event_t *mln_event_new(void);
```

描述：创建事件结构。

返回值：成功则返回事件结构指针，否则返回`NULL`



#### mln_event_free

```c
void mln_event_free(mln_event_t *ev);
```

描述：销毁事件结构。

返回值：无



#### mln_event_dispatch

```c
void mln_event_dispatch(mln_event_t *event);
```

描述：调度事件集`event`上的事件。当有事件触发时，则会调用相应的回调函数进行处理。

**注意**：本函数在不调用`mln_event_break_set`的情况下是不会返回的。

返回值：无



#### mln_event_fd_set

```c
int mln_event_fd_set(mln_event_t *event, int fd, mln_u32_t flag, int timeout_ms, void *data, ev_fd_handler fd_handler);

typedef void (*ev_fd_handler)  (mln_event_t *, int, void *);s
```

描述：设置文件描述符事件，其中：

- `fd`为事件关注的文件描述符。

- `flag`分为如下几类：

  - `M_EV_RECV` 读事件
  - `M_EV_SEND` 写事件
  - `M_EV_ERROR` 错误事件
  - `M_EV_ONESHOT `仅触发一次
  - `M_EV_NONBLOCK` 非阻塞模式
  - `M_EV_BLOCK `阻塞模式
  - `M_EV_APPEND` 追加事件，即原本已设置了某个事件，如读事件，此时想再追加监听一类事件，如写事件，则可以使用该flag
  - `M_EV_CLR` 清除所有事件

  这些flag之间可以使用或运算符进行同时设置。

- `timeout_ms`事件超时时间，毫秒级，该字段值为：

  - `M_EV_UNLIMITED` 永不超时
  - `M_EV_UNMODIFIED` 保留之前的超时设置
  - `毫秒值` 超时时长

- `data` 为事件处理相关的用户数据结构，可自行定义。

- `ev_fd_handler`为事件处理函数，函数有三个参数分别为：事件结构、文件描述符以及自定义的用户数据结构。

返回值：成功则返回`0`，否则返回`-1`



#### mln_event_fd_timeout_handler_set

```c
void mln_event_fd_timeout_handler_set(mln_event_t *event, int fd, void *data, ev_fd_handler timeout_handler);
```

描述：设置描述符事件超时处理函数，其中：

- `fd`为文件描述符。
- `data`为超时时间处理相关的用户数据结构，可自行定义。
- `timeout_handler`与`mln_event_fd_set`函数的回调函数类型一致，用于处理超时事件。

该函数需要与`mln_event_fd_set`函数配合使用，先使用`mln_event_fd_set`设置事件，后使用本函数设置超时处理函数。

之所以这样做，是因为有些事件超时后可能不需要特殊函数进行处理，而如果全部放在`mln_event_fd_set`函数中设置，会导致参数过多过于繁杂。

返回值：无



#### mln_event_timer_set

```c
mln_event_timer_t *mln_event_timer_set(mln_event_t *event, mln_u32_t msec, void *data, ev_tm_handler tm_handler);

typedef void (*ev_tm_handler)  (mln_event_t *, void *);
```

描述：设置定时器事件，其中：

- `msec`为定时毫秒值
- `data` 为定时事件用户自定义数据结构
- `tm_handler` 定时事件处理函数，其参数依次为：事件结构和用户自定义数据

定时事件每一次出发后，会自动从事件集中删除。若需要一直触发定时事件，则需要在处理函数内自行调用本函数进行设置。

返回值：成功则返回定时器句柄指针，否则返回`NULL`



#### mln_event_timer_cancel

```c
void mln_event_timer_cancel(mln_event_t *event, mln_event_timer_t *timer);
```

描述：取消已设置的定时器。本函数需要保证`timer`是已设置但未触发的超时事件句柄。

返回值：无



#### mln_event_signal_set

```c
typedef void (*sig_t)(int);

sig_t mln_event_signal_set(int signo, sig_t handle);
```

描述：设置信号处理函数，系统提供的signal函数的别名。

返回值：返回设置前的信号处理函数



#### mln_event_break_set

```c
mln_event_break_set(ev);
```

描述：中断事件处理，即使得`mln_event_dispatch`函数返回。

返回值：无



#### mln_event_break_reset

```c
mln_event_break_reset(ev);
```

描述：重置break标记。

返回值：无



#### mln_event_callback_set

```c
void (mln_event_t *ev, dispatch_callback dc, void *dc_data);

typedef void (*dispatch_callback) (mln_event_t *, void *);
```

描述：设置事件处理回调函数，该函数会在每次时间循环的最开始被调用一次。目前主要用于处理配置热重载。

`dc_data`为用户自定义数据结构。

`dc`为回调函数，其参数依次为：事件结构和用户自定义数据结构。

返回值：无



### 示例

```c
#include <stdio.h>
#include <stdlib.h>
#include "mln_event.h"

static void timer_handler(mln_event_t *ev, void *data)
{
    printf("timer\n");
    mln_event_timer_set(ev, 1000, NULL, timer_handler);
}

static void mln_fd_write(mln_event_t *ev, int fd, void *data)
{
    printf("write handler\n");
    write(fd, "hello\n", 6);
    mln_event_fd_set(ev, fd, M_EV_CLR, M_EV_UNLIMITED, NULL, NULL);
}

int main(int argc, char *argv[])
{
    mln_event_t *ev;

    ev = mln_event_new();
    if (ev == NULL) {
        fprintf(stderr, "event init failed.\n");
        return -1;
    }

    if (mln_event_timer_set(ev, 1000, NULL, timer_handler) < 0) {
        fprintf(stderr, "timer set failed.\n");
        return -1;
    }

    if (mln_event_fd_set(ev, STDOUT_FILENO, M_EV_SEND, M_EV_UNLIMITED, NULL, mln_fd_write) < 0) {
        fprintf(stderr, "fd handler set failed.\n");
        return -1;
    }

    mln_event_dispatch(ev);

    return 0;
}
```

