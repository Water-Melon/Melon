## 脚本任务

脚本任务分为C相关的函数使用，以及脚本自身语法和函数库的使用。后者详情参见：[Melang.org](https://melang.org)。

本脚本是一个同步写法但纯异步实现的脚本。脚本可以在单一线程内实现多任务抢占式调度执行，且不会影响到该线程内其他异步事件的处理。换言之，脚本可以与异步网络IO同在一个线程内处理。

本文仅给出创建脚本任务、调度执行脚本任务的函数。关于扩展功能，可以参考后续的脚本开发文章。



### 头文件

```c
#include "mln_lang.h"
```



### 函数/宏



#### mln_lang_new

```c
mln_lang_t *mln_lang_new(mln_alloc_t *pool, mln_event_t *ev);
```

描述：创建脚本管理结构，该结构是脚本任务的管理结构，用于维护多个脚本任务的资源、调度、创建、删除等相关内容。其上每一个脚本任务被看作一个协程。`pool`为每个脚本任务所使用的内存池结构，`ev`为每个脚本任务所依赖的事件结构。换言之，脚本任务的执行是依赖于Melon的异步事件API的。

返回值：成功则返回脚本管理结构`mln_lang_t`指针，否则返回`NULL`



#### mln_lang_free

```c
void mln_lang_free(mln_lang_t *lang);
```

描述：销毁并释放脚本管理结构的所有资源。

返回值：无



#### mln_lang_job_new

```c
mln_lang_ctx_t *mln_lang_job_new(mln_lang_t *lang, mln_u32_t type, mln_string_t *data, void *udata, mln_lang_return_handler handler);

typedef void (*mln_lang_return_handler)(mln_lang_ctx_t *);
```

描述：创建脚本任务。参数含义如下：

- `lang` 脚本管理结构，由`mln_lang_new`创建而来。本任务创建好后将由该结构进行管理。
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



#### mln_lang_run

```c
void mln_lang_run(mln_lang_t *lang);
```

描述：执行脚本。本函数不需要每一个调用`mln_lang_job_new`后调用，可以新建多个脚本任务后调用一次即可。

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



### 示例

最好的示例就是Melang仓库的源代码，仅有一个文件不超过200行，关于脚本调用的代码仅35行。该仓库仅仅是Melon核心库的一个启动器。详情参见：[melang.c](https://github.com/Water-Melon/Melang/blob/master/melang.c)。

