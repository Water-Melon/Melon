## 初始化

这个组件是用来对整个Melon库进行首次初始化的。

在Melon的初始化过程中的不同阶段，会调用用户提供的一些回调函数进行特定阶段的处理。

这些用户提供的回调函数包含：

- `global_init`
- `main_thread`
- `master_process`
- `worker_process`

这些回调函数大致的调用时序如下所示：

```
                                             |
                                          配置初始化
                                             |
                                    global_init回调函数调用
                                             |
                           一些框架的初始化，例如资源限制、常驻后台、fork等等
                                             |
                           main_thread/master_process/worker_process回调函数调用
                                             |
                                         处理各种事件
```

`global_init`是在配置初始化后，但在fork前（如果是启用多进程框架的话）被调用的。这个时候可以通过配置的函数调用到Melon的全部配置，然后依据这些配置做一些针对整个应用程序的初始化行为。

`main_thread`是在多线程模型下，在主线程内被调用的，用于对主线程设置一些常驻任务的。

`master_process`/`worker_process`是在多进程模型下，在主/工作进程中被调用的，用于针对单个进程进行一些全局设置。

`msvc`中，不包含`main_thread`，`master_process`和`worker_process`。初始化流程仅对配置和日志等初始化后返回。



### 头文件

```
#include "mln_framework.h"
```



### 模块名

`framework`



### 相关结构

```c
struct mln_framework_attr {
    int                            argc; //一般为main的argc
    char                         **argv; //一般为main的argv
    mln_framework_init_t           global_init; //初始化回调函数，一般用于初始化全局变量，该回调会在配置加载完成后被调用
#if !defined(MSVC)
    mln_framework_process_t        main_thread; //主线程处理函数，我们将在多线程框架部分深入
    mln_framework_process_t        master_process; //主进程处理函数，我们将在多进程框架部分深入
    mln_framework_process_t        worker_process; //工作进程处理函数，我们将在多进程框架部分深入
#endif
};

typedef int (*mln_framework_init_t)(void);
typedef void (*mln_framework_process_t)(mln_event_t *);
```

一般情况下，在Melon的各个组件中，被用来作为初始化参数的结构体都以`_attr`结尾，且不会被`typedef`定义为某一类型。



### 函数



#### mln_framework_init

```c
int mln_framework_init(struct mln_framework_attr *attr)；
```

描述：该函数是Melon库的整体初始化函数，会加载配置，并根据配置启用或停用Melon中框架部分功能，以及其他额外功能。

返回值：成功则返回0，否则返回-1。

举例：

```c
int main(int argc, char *argv[])
{
    struct mln_framework_attr cattr;
    cattr.argc = argc;
    cattr.argv = argv;
    cattr.global_init = NULL;
    cattr.main_thread = NULL;
    cattr.master_process = NULL;
    cattr.worker_process = NULL;
    return mln_framework_init(&cattr);
}
```

