## 脚本任务

脚本任务分为C相关的函数使用，以及脚本自身语法和函数库的使用。后者详情参见：[Melang.org](https://melang.org)。

本脚本是一个同步写法但纯异步实现的脚本。脚本可以在单一线程内实现多任务抢占式调度执行，且不会影响到该线程内其他异步事件的处理。换言之，脚本可以与异步网络IO同在一个线程内处理。

本文仅给出创建脚本任务、调度执行脚本任务的函数。关于扩展功能，可以参考后续的脚本开发文章。



### 头文件

```c
#include "mln_lang.h"
```



### 模块名

`lang`



### 函数/宏



#### mln_lang_new

```c
mln_lang_t *mln_lang_new(mln_event_t *ev, mln_lang_run_ctl_t signal, mln_lang_run_ctl_t clear);

typedef int (*mln_lang_run_ctl_t)(mln_lang_t *);
```

描述：创建脚本管理结构，该结构是脚本任务的管理结构，用于维护多个脚本任务的资源、调度、创建、删除等相关内容。其上每一个脚本任务被看作一个协程。`ev`为每个脚本任务所依赖的事件结构。换言之，脚本任务的执行是依赖于Melon的异步事件API的。`signal`用于设置事件，来使其执行脚本任务。`clear`用于清除事件，来阻止脚本任务继续执行。

返回值：成功则返回脚本管理结构`mln_lang_t`指针，否则返回`NULL`



#### mln_lang_free

```c
void mln_lang_free(mln_lang_t *lang);
```

描述：销毁并释放脚本管理结构的所有资源。

返回值：无



#### mln_lang_job_new

```c
mln_lang_ctx_t *mln_lang_job_new(mln_lang_t *lang, mln_string_t *alias, mln_u32_t type, mln_string_t *data, void *udata, mln_lang_return_handler handler);

typedef void (*mln_lang_return_handler)(mln_lang_ctx_t *);
```

描述：创建脚本任务。参数含义如下：

- `lang` 脚本管理结构，由`mln_lang_new`创建而来。本任务创建好后将由该结构进行管理。
- `alias` 脚本任务的别名，若不需要可置`NULL`。
- `type`脚本任务代码的类型：`M_INPUT_T_FILE`（文件）、`M_INPUT_T_BUF`（字符串）。
- `data` 根据`type`不同，本参数含义不同。文件时本参数为文件路径；字符串时本参数为代码字符串。
- `udata` 用户自定义结构，一般用于第三方库函数实现以及`handler`内获取任务返回值之用。
- `handler`当任务正常完结时，获取任务返回值。本函数只有一个参数，即脚本任务结构，若需要自定义结构辅助实现一些功能，可以设置`udata`字段。

返回值：成功则返回脚本结构`mln_lang_ctx_t`指针，否则返回`NULL`



#### mln_lang_job_free

```c
void mln_lang_job_free(mln_lang_ctx_t *ctx);
```

描述：销毁并释放脚本任务结构所有资源。

返回值：无



#### mln_lang_cache_set

```c
mln_lang_cache_set(lang)
```

描述：设置是否缓存抽象语法树结构。设置该缓存后，在后续创建脚本代码相同的任务时，可略去抽象语法树生成过程而直接使用，以提升性能。

返回值：无



#### mln_lang_ctx_data_get

```c
mln_lang_ctx_data_get(ctx)
```

描述：获取脚本任务`mln_lang_ctx_t`类型指针的`ctx`中的用户自定义数据。

返回值：用户自定义数据指针



#### mln_lang_ctx_data_set

```c
mln_lang_ctx_data_set(ctx, d)
```

描述：为脚本任务`ctx`设置用户自定义数据`d`。

返回值：无



#### mln_lang_launcher_get

```c
mln_lang_launcher_get(lang)
```

描述：获取`lang`结构中的launcher函数指针，该指针是`mln_event_t`中文件描述符事件的处理函数。这里允许获取这个函数的目的是为了让调用方可以在自定义的`signal`函数指针内获取到该处理函数，以便用于设置文件描述符事件。

返回值：launcher函数指针



#### mln_lang_event_get

```c
mln_lang_event_get(lang)
```

描述：获取`lang`中的`mln_event_t`指针，用于在`signal`以及`clear`回调中设置/清理事件。

返回值：`mln_event_t`指针



#### mln_lang_signal_get

```c
mln_lang_signal_get(lang)
```

描述：获取`lang`结构中的`signal`函数指针，用于在脚本开发中触发文件描述符事件，来使脚本任务得以继续执行。

返回值：`signal`函数指针



#### mln_lang_task_empty

```c
mln_lang_task_empty(lang)
```

描述：检查`lang`中是否已经没有脚本任务了。

返回值：

- `0` 有任务
- `非0` 无任务



#### mln_lang_mutex_lock

```c
mln_lang_mutex_lock(lang)
```

描述：对`lang`中资源进行加锁，例如在脚本开发中调用`mln_lang_task_empty`前应当加锁。

返回值：

- `0` 成功
- `非0` 失败



#### mln_lang_mutex_unlock

```c
mln_lang_mutex_unlock(lang)
```

描述：对`lang`中资源解除锁定，例如在脚本开发中调用`mln_lang_task_empty`后应当解锁。

返回值：

- `0` 成功
- `非0` 失败



#### mln_lang_ctx_pipe_send

```c
int mln_lang_ctx_pipe_send(mln_lang_ctx_t *ctx, char *fmt, ...)
```

描述：在C代码中向指定的脚本任务发送一个消息。这个消息可以被脚本层的`Pipe`函数接收，其中：

- `ctx`是对应脚本任务的上下文结构指针
- `fmt`是用于对可变参数的解释，`fmt`支持四种字符：
  - `i`整数，该整数应该是`mln_s64_t`类型整数
  - `r`实数，该实数应该是`double`类型
  - `s`字符串，该字符串应对应`char`指针参数
  - `S`字符串，该字符串应对应`mln_string_t`指针参数

返回值：

- `0` 成功
- `-1` 失败

示例：

```c
mln_string_t s = mln_string("hello");
mln_lang_ctx_pipe_send(ctx, "sir", &s, 1, 3.14);
```



#### mln_lang_ctx_pipe_recv_handler_set

```c
int mln_lang_ctx_pipe_recv_handler_set(mln_lang_ctx_t *ctx, mln_lang_ctx_pipe_recv_cb_t recv_handler);
```

描述：设置接收从脚本层`Pipe`函数发送来的数据的回调函数`recv_handler`，其定义为：

```c
typedef int (*mln_lang_ctx_pipe_recv_cb_t)(mln_lang_ctx_t *, mln_lang_val_t *);
```

- 第一个参数是脚本任务的上下文结构指针
- 第二个是传递的变量数据部分，该数据为`mln_lang_val_t`类型

返回值：

- `0` 成功
- `-1` 失败




#### mln_lang_ctx_is_quit

````c
mln_lang_ctx_is_quit(ctx)
````

描述：判断程序是否未执行完退出。

返回值：无



### 示例

最好的示例就是Melang仓库的源代码，仅有一个文件不超过200行，关于脚本调用的代码仅35行。该仓库仅仅是Melon核心库的一个启动器。详情参见：[melang.c](https://github.com/Water-Melon/Melang/blob/master/melang.c)。

