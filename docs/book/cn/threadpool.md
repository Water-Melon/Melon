## 线程池

在Melon中支持两种多线程模式，线程池是其中一种，另一种请参见后续的多线程框架文章。

**注意**：在每个进程中仅允许存在一个线程池。

本模块在MSVC环境中暂不支持。



### 头文件

```c
#include "mln_thread_pool.h"
```



### 模块名

`thread_pool`



### 函数



#### mln_thread_pool_run

```c
int mln_thread_pool_run(struct mln_thread_pool_attr *tpattr);

struct mln_thread_pool_attr {
    void                              *main_data;
    mln_thread_process                 child_process_handler;
    mln_thread_process                 main_process_handler;
    mln_thread_data_free                free_handler;
    mln_u64_t                          cond_timeout; /*ms*/
    mln_u32_t                          max;
    mln_u32_t                          concurrency;
};
typedef int  (*mln_thread_process)(void *);
typedef void (*mln_thread_data_free)(void *);
```

描述：创建并运行内存池。

线程池由主线程进行管理和做一部分处理后下发任务，子线程组则接受任务进行处理。

初始状态下，是不存在子线程的，当有任务需要下发时会自动创建子线程。当任务处理完后，子线程会延迟释放，避免频繁分配释放资源。

其中参数结构体的每个成员含义如下：

- `main_data` 为主线程的用户自定义数据。
- `child_process_handler` 每个子线程的处理函数，该函数有一个参数为主线程下发任务时给出的数据结构指针，返回值为`0`表示处理正常，`非0`表示处理异常，异常时会有日志输出。
- `main_process_handler` 主线程的处理函数，该函数有一个参数为`main_data`，返回值为`0`表示处理正常，`非0`表示处理异常，异常时会有日志输出。**一般情况下，主线程处理函数不应随意自行返回，一旦返回代表线程池处理结束，线程池会被销毁**。
- `free_handler` 资源释放函数。其资源为主线程下发给子线程的数据结构指针所指向的内容。注意：这个释放函数仅在子线程退出或线程池销毁时，用于释放尚未被处理或处理完的资源。也就是说，任何经过了`child_process_handler`回调的资源，其释放将完全交由子线程负责，在`child_process_handler`中被释放。
- `cond_timeout`为闲置子线程回收定时器，单位为毫秒。当子线程无任务处理，且等待时间超过该定时器时长后，会自行退出。
- `max`线程池允许的最大子线程数量。
- `concurrency`用于`pthread_setconcurrency`设置并行级别参考值，但部分系统并为实现该功能，因此不应该过多依赖该值。在Linux下，该值设为零表示交由本系统实现自行确定并行度。

返回值：本函数返回值与主线程处理函数的返回值保持一致



#### mln_thread_pool_resource_add

```c
int mln_thread_pool_resource_add(void *data);
```

描述：将资源`data`放入到资源池中。本函数仅应由主线程调用，用于主线程向子线程下发任务所用。

返回值：成功则返回`0`，否则返回`非0`



#### mln_thread_quit

```c
void mln_thread_quit(void);
```

描述：本函数用于告知线程池，关闭并销毁线程池。

返回值：无



#### mln_thread_resource_info

```c
void mln_thread_resource_info(struct mln_thread_pool_info *info);

struct mln_thread_pool_info {
    mln_u32_t                          max_num;
    mln_u32_t                          idle_num;
    mln_u32_t                          cur_num;
    mln_size_t                         res_num;
};
```

描述：获取当前线程池信息。信息会写入参数结构体中，结构体每个参数含义如下：

- `max_num`：线程池最大子线程数量
- `idle_num`：当前闲置子线程数量
- `cur_num`：当前子线程数量（包含闲置和工作中的子线程）
- `res_num`：当前尚未被处理的资源数量

返回值：无



### 示例

```c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "mln_thread_pool.h"

static int main_process_handler(void *data);
static int child_process_handler(void *data);
static void free_handler(void *data);

int main(int argc, char *argv[])
{
    struct mln_thread_pool_attr tpattr;

    tpattr.main_data = NULL;
    tpattr.child_process_handler = child_process_handler;
    tpattr.main_process_handler = main_process_handler;
    tpattr.free_handler = free_handler;
    tpattr.cond_timeout = 10;
    tpattr.max = 10;
    tpattr.concurrency = 10;
    return mln_thread_pool_run(&tpattr);
}

static int child_process_handler(void *data)
{
    printf("%s\n", (char *)data);
    free(data); //data已经被子线程接管，因此子线程的这个回调函数一旦被调用，资源释放就不再由主线程负责了
    return 0;
}

static int main_process_handler(void *data)
{
    int n;
    char *text;

    while (1) {
        if ((text = (char *)malloc(16)) == NULL) {
            return -1;
        }
        n = snprintf(text, 15, "hello world");
        text[n] = 0;
        mln_thread_pool_resource_add(text);
        usleep(1000);
    }

    return 0;
}

static void free_handler(void *data)
{
    free(data);
}
```

