## 初始化



### 头文件

```
#include "mln_core.h"
```



### 相关结构

```c
struct mln_core_attr {
    int                       argc; //一般为main的argc
    char                    **argv; //一般为main的argv
    mln_core_init_t           global_init; //初始化回调函数，一般用于初始化全局变量，该回调会在配置加载完成后被调用
    mln_core_process_t        master_process; //主进程处理函数，我们将在多进程框架部分深入
    mln_core_process_t        worker_process; //工作进程处理函数，我们将在多进程框架部分深入
};

typedef int (*mln_core_init_t)(void);
typedef void (*mln_core_process_t)(mln_event_t *);
```

一般情况下，在Melon的各个组件中，被用来作为初始化参数的结构体都以`_attr`结尾，且不会被`typedef`定义为某一类型。



### 函数



#### mln_core_init

```c
int mln_core_init(struct mln_core_attr *attr)；
```

描述：该函数是Melon库的整体初始化函数，会加载配置，并根据配置启用或停用Melon中框架部分功能，以及其他额外功能。

返回值：成功则返回0，否则返回-1。

举例：

```c
int main(int argc, char *argv[])
{
    struct mln_core_attr cattr;
    cattr.argc = argc;
    cattr.argv = argv;
    cattr.global_init = NULL;
    cattr.master_process = NULL;
    cattr.worker_process = NULL;
    return mln_core_init(&cattr);
}
```

