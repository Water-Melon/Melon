## 跟踪模式



跟踪模式用于收集使用Melon库的程序信息。大致过程如下：

1. 在程序中使用指定函数设置跟踪点
2. 开启跟踪模式配置，并设置好接收跟踪信息的Melang脚本文件路径
3. 在接收跟踪信息的Melang脚本中调用`pipe`函数接收跟踪传来的信息

目前，Melon中给出了一个处理跟踪信息的Melang脚本示例，在源码目录的`trace/trace.m`。

与ebpf不同，跟踪模块旨在收集应用相关的信息，即由开发人员指定的（业务或功能）信息。ebpf可以收集大量程序及内核信息，但无法针对与业务信息进行收集。因此，Melon中的trace模式就是为了补全这部分内容。

关于Melang及其配套库，请参考[Melang仓库](https://github.com/Water-Melon/Melang)。

> **注意事项**：
>
> 1. 配置中的`trace_mode`只有在开启框架模式（`framework`为`"multiprocess"`或`"multithread"`）时才会有效。
> 2. 若在非框架模式下，或在配置中未启用`trace_mode`，但仍想使用该功能，则可以通过调用`mln_trace_init`进行初始化。
> 3. 该模式下，系统存在一定的额外开销。



### 头文件

```c
#include "mln_trace.h"
```



### 模块名

`trace`



### 函数/宏



#### mln_trace

```c
mln_trace(fmt, ...);

//msvc中，原型为
mln_trace(ret, fmt, ...);
```

描述：发送若干数据给指定的处理脚本。这里的参数，与`mln_lang_ctx_pipe_send`函数([详见此章节](https://water-melon.github.io/Melon/cn/melang.html))的`fmt`及其可变参数的内容完全一致，因为本宏内部就是调用该函数完成的消息传递。msvc中，返回值会被赋予给参数`ret`。

返回值：

- `0` - 成功
- `-1` - 失败



#### mln_trace_path

```c
 mln_string_t *mln_trace_path(void);
```

描述：返回配置中`trace_mode`配置项设置的跟踪脚本路径。

返回值：

- `NULL` - 若配置不存在或者配置项参数为`off`
- 跟踪脚本的文件路径



#### mln_trace_init

```c
int mln_trace_init(mln_event_t *ev, mln_string_t *path);
```

描述：初始化全局跟踪模块。其中：

- `ev` 是跟踪脚本所依赖的事件结构
- `path`是跟踪脚本的文件路径

若跟踪脚本在主进程中初始化成功，则会向其增加一个全局`MASTER`变量，且值为`true`。

返回值：

- `0` - 成功
- `-1` - 失败



#### mln_trace_task_get

```c
mln_lang_ctx_t *mln_trace_task_get(void);
```

描述：获取全局跟踪模块中用于获取跟踪信息的脚本任务对象。

返回值：

- `NULL` - 跟踪模块未被初始化或脚本任务已退出
- 脚本任务对象



#### mln_trace_finalize

```c
void mln_trace_finalize(void);
```

描述：销毁所有trace结构，并将全局指针复位。

返回值：无



#### mln_trace_init_callback_set

```c
void mln_trace_init_callback_set(mln_trace_init_cb_t cb);
typedef int (*mln_trace_init_cb_t)(mln_lang_ctx_t *ctx);
```

描述：这个函数用于设置跟踪脚本的初始化回调，这个回调会在`mln_trace_init`中被调用。

返回值：无



#### mln_trace_recv_handler_set

```c
int mln_trace_recv_handler_set(mln_lang_ctx_pipe_recv_cb_t recv_handler);
typedef int (*mln_lang_ctx_pipe_recv_cb_t)(mln_lang_ctx_t *, mln_lang_val_t *);
```

描述：这个函数用于设置跟踪脚本中`Pipe`函数传递给C层的数据的处理函数。

返回值：成功返回`0`，否则返回`-1`



### 示例

安装Melon后，我们按如下步骤操作：

1. 开启trace模式配置，编辑安装路径下的`conf/melon.conf`，将`framework`设置为`"multiprocess"`，且将`trace_mode`前的注释（`//`）删掉。本例中`worker_proc`设置为`1`。

   > 如果想要禁用trace模式，只需要将配置注释或者将配置项改为 trace_mode off;即可。

2. 新建一个名为`a.c`的文件

   ```c
   #include <stdio.h>
   #include "mln_log.h"
   #include "mln_framework.h"
   #include "mln_trace.h"
   #include "mln_conf.h"
   
   static void timeout_handler(mln_event_t *ev, void *data)
   {
       mln_trace("sir", "Hello", getpid(), 3.1);
       mln_event_timer_set(ev, 1000, NULL, timeout_handler);
   }
   
   static int recv_handler(mln_lang_ctx_t *ctx, mln_lang_val_t *val)
   {
       mln_log(debug, "%d\n", val->data.i);
       return 0;
   }
   
   static void worker_process(mln_event_t *ev)
   {
       mln_event_timer_set(ev, 1000, NULL, timeout_handler);
       mln_log(debug, "%d\n", mln_trace_recv_handler_set(recv_handler));
   }
   
   int main(int argc, char *argv[])
   {
       struct mln_framework_attr cattr;
   
       cattr.argc = argc;
       cattr.argv = argv;
       cattr.global_init = NULL;
       cattr.main_thread = NULL;
       cattr.master_process = NULL;
       cattr.worker_process = worker_process;
   
       if (mln_framework_init(&cattr) < 0) {
          fprintf(stderr, "Melon init failed.\n");
          return -1;
       }
   
       return 0;
   }
   ```



3. 编译a.c文件，然后执行生成的可执行程序，可看到如下输出

   ```
   Start up worker process No.1
   02/02/2023 07:08:54 UTC DEBUG: a.c:worker_process:22: PID:58451 0
   master process
   worker process
   [Hello, 58451, 3.100000, ]
   02/02/2023 07:08:56 UTC DEBUG: a.c:recv_handler:15: PID:58451 1
   [Hello, 58451, 3.100000, ]
   02/02/2023 07:08:57 UTC DEBUG: a.c:recv_handler:15: PID:58451 1
   [Hello, 58451, 3.100000, ]
   02/02/2023 07:08:58 UTC DEBUG: a.c:recv_handler:15: PID:58451 1
   ...
   ```

