## IPC模块开发

本模块在MSVC环境中暂不支持。



### 头文件

```c
#include "mln_ipc.h"
```



### 模块名

`ipc`



**注意**：Windows下暂时不支持本功能。

Melon支持多进程框架，那么自然主子进程之间会涉及通信问题，这部分就由IPC进行处理。

> 自2.3.0版本开始，IPC部分将不在以向ipc_handlers子目录下添加代码文件的方式进行集成，而是提供了注册函数来完成IPC的注册和加载，以此将用户自定义类型处理函数与Melon从代码目录层面解耦。

开发者可以在框架初始化前，调用

```c
mln_ipc_cb_t *mln_ipc_handler_register(mln_u32_t type, ipc_handler master_handler, ipc_handler worker_handler, void *master_data, void *worker_data);
```

来注册自己的IPC消息处理函数，其中：

- `type` 为消息的类型，作为约定 `0`~`1024`将被作为Melon内部使用的消息类型值，开发者尽量不要与之重叠，如有类型字段重叠的情况，则新的处理函数将覆盖旧处理函数。

- `master_handler`为主进程的处理函数，其函数原型为：

  ```c
  void (*ipc_handler)(mln_event_t *ev, void *f_ptr, void *buf, mln_u32_t len, void **udata_ptr);
  ```

  - `ev`为与本消息相关的事件结构
  - `f_ptr`为一个`mln_fork_t`类型指针（是的你没看错，我也没写错），我们可以通过这里个指针获取一些和子进程相关的信息，例如通信的链接结构。
  - `buf`为主/子进程本次收到的消息内容。
  - `len`为消息的长度
  - `udata_ptr`是一个可自定义的用户数据。我们可以在本次调用中对其进行赋值，而在下次调用时它会被传递回来继续使用，框架不会对其进行初始化和释放操作。

- `worker_handler`为子进程的处理函数，其函数原型与`master_handler`一致。

- `master_data`为主进程处理函数配套的用户自定义数据，若不需要则可传入`NULL`。

- `worker_data`为子进程处理函数配套的用户自定义数据，若不需要则可传入`NULL`。

- 返回值：`mln_ipc_cb_t *`指针，记录了IPC消息处理函数及消息类型等信息。


还可以调用

```c
void mln_ipc_handler_unregister(mln_ipc_cb_t *cb);
```
来反注册消息处理函数。



关于`mln_fork_t`类型：

```c
struct mln_fork_s {
    struct mln_fork_s       *prev;
    struct mln_fork_s       *next;
    mln_s8ptr_t             *args; //子进程参数
    mln_tcp_conn_t           conn; //通信的链接结构，socketpair被当作tcp使用
    pid_t                    pid; //子进程的pid
    mln_u32_t                n_args;//参数个数
    mln_u32_t                state;//子进程状态
    mln_u32_t                msg_len;//消息长度
    mln_u32_t                msg_type;//消息类型
    mln_size_t               error_bytes;//错误消息类型数据大小
    void                    *msg_content;//消息内容
    enum proc_exec_type      etype;//子进程是需要被替换执行映像的（exec）还是不需要的
    enum proc_state_type     stype;//子进程退出后是否需要被重新拉起
};
```



由于Melon是一主多从模式，因此需要一个方法能够遍历子进程列表，并对其下发消息，因此提供了一个函数用于遍历所有子进程结构：

```c
int mln_fork_iterate(mln_event_t *ev, fork_iterate_handler handler, void *data);

typedef int (*fork_iterate_handler)(mln_event_t *, mln_fork_t *, void *);
```

- `ev`为主进程消息相关的事件处理结构。
- `handler`为每个子进程节点的处理函数，函数有三个参数，分别为：消息相关的事件处理结构、子进程的`mln_fork_t`结构以及用户自定义数据（即`mln_fork_iterate`的第三个参数）。
- `data`用户自定义数据。



Melon中还提供了其他函数：

- mln_fork_master_connection_get

  ```c
  mln_tcp_conn_t *mln_fork_master_connection_get(void);
  ```

  用于在子进程中获取与主进程通信的TCP链接结构（socketpair被当作TCP处理）。

- mln_ipc_master_send_prepare

  ```c
  int mln_ipc_master_send_prepare(mln_event_t *ev, mln_u32_t type, void *buf, mln_size_t len, mln_fork_t *f_child);
  ```

  用于主进程将长度为`len`类行为`type`的消息`buf`发送给`f_child`指定的子进程。

- mln_ipc_worker_send_prepare

  ```c
  int mln_ipc_worker_send_prepare(mln_event_t *ev, mln_u32_t type, void *msg, mln_size_t len);
  ```

  用于子进程将长度为`len`类行为`type`的消息`buf`发送给主进程。

- mln_fork_master_ipc_handler_set

  ```c
  int mln_fork_master_ipc_handler_set(mln_u32_t type, ipc_handler handler, void *data);
  ```

  这个函数用于设置主进程的IPC处理函数。实际上，在`mln_ipc_handler_register`内部正是调用该函数进行设置的，但与之有区别的就是，本函数既可以在框架初始化前调用，也可以在框架初始化后调用。

  但是需要确保一点，本函数需要在主进程内被调用，在子进程中设置会出现因访问`NULL`内存而导致的段错误。

- mln_fork_worker_ipc_handler_set

  ```c
  int mln_fork_worker_ipc_handler_set(mln_u32_t type, ipc_handler handler, void *data);
  ```

  这个函数用于设置子进程的IPC处理函数。实际上，在`mln_ipc_handler_register`内部正是调用该函数进行设置的，但与之有区别的就是，本函数既可以在框架初始化前调用，也可以在框架初始化后调用。

  但是需要确保一点，本函数需要在子进程内被调用，在主进程中设置则不会被使用到。
