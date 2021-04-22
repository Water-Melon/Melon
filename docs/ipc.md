## IPC模块开发



**注意**：Windows下暂时不支持本功能。

Melon支持多进程框架，那么自然主子进程之间会涉及通信问题，这部分就由IPC进行处理。

Melon提供了一种简单的IPC扩展方式，它不需要对现有源文件进行修改即可完成（但需要重新编译）。为此，在Melon目录中有一个专门的目录提供给这个扩展之用——`ipc_handlers`。

在这个目录下的文件，每一个即为一个消息以及消息的处理函数。文件的命名是有规范的：`处理函数前缀.消息类型名`。而处理函数又分为：主进程处理函数 和 子进程处理函数，他们的名字分别为：`处理函数前缀_master`和`处理函数前缀_worker`。消息类型会在configure时自动加入到`mln_ipc.h`头文件中，用户自己的代码直接使用即可。

处理函数的函数原型如下：

```c
void prefix_master(mln_event_t *ev, void *f_ptr, void *buf, mln_u32_t len, void **udata_ptr);
```

和

```c
void prefix_worker(mln_event_t *ev, void *f_ptr, void *buf, mln_u32_t len, void **udata_ptr);
```

第一个参数为与本消息相关的事件结构。第二个参数是一个`mln_fork_t`类型的指针，我们可以通过这里个指针获取一些和子进程相关的信息，例如通信的链接结构。第三个参数为主/子进程本次收到的消息内容。第四个参数为消息的长度。第五个参数是一个可自定义的用户数据。我们可以在本次调用中对其进行赋值，而在下次调用时它会被传递回来继续使用，框架不会对其进行初始化和释放操作。

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
int mln_fork_scan_all(mln_event_t *ev, scan_handler handler, void *data);

typedef int (*scan_handler)(mln_event_t *, mln_fork_t *, void *);
```

- `ev`为主进程消息相关的事件处理结构。
- `handler`为每个子进程节点的处理函数，函数有三个参数，分别为：消息相关的事件处理结构、子进程的`mln_fork_t`结构以及用户自定义数据（即`mln_fork_scan_all`的第三个参数）。
- `data`用户自定义数据。

Melon中还提供了其他函数：

- mln_fork_get_master_connection

  ```c
  mln_tcp_conn_t *mln_fork_get_master_connection(void);
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



目前，在ipc_handlers中有一个已经定义了的消息，用于配置热重载的，极其简单，用户可以在阅读完本文后与之对照。