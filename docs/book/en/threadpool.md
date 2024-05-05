## Thread Pool

There are two multi-threading modes supported in Melon, one of which is the thread pool, and the other, please refer to the subsequent multi-threading framework articles.

**Note**: Only one thread pool is allowed per process.

This module is not supported in the MSVC.


### Header file

```c
#include "mln_thread_pool.h"
```



### Module

`thread_pool`



### Functions



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

Description: Create and run a memory pool.

The thread pool is managed by the main thread and issues tasks after doing some processing, and the sub-thread group accepts the tasks for processing.

In the initial state, there are no sub-threads, and sub-threads are automatically created when tasks need to be delivered. When the task is processed, the child thread will delay the release to avoid frequent allocation and release of resources.

The meaning of each member of the parameter structure is as follows:

- `main_data` User-defined data for the main thread.
- `child_process_handler` The processing function of each child thread. This function has a parameter of the data structure pointer given when the main thread sends the task. The return value is `0` to indicate normal processing, and `non-0` to indicate abnormal processing. There will be log output.
- `main_process_handler` The processing function of the main thread, this function has a parameter `main_data`, the return value `0` means normal processing, `non-0` means processing exceptions, and there will be log output when exceptions occur. **Under normal circumstances, the main thread processing function should not return at will. Once the return represents the end of the thread pool processing, the thread pool will be destroyed**.
- `free_handler` is the resource release function. Its resource is the content pointed to by the data structure pointer issued by the main thread to the child thread. Note: This release function is only used to release resources that have not been processed or completed when the child thread exits or the thread pool is destroyed. In other words, any resource that has passed the callback of `child_process_handler` will be completely released by the child thread and will be released in `child_process_handler`.
- `cond_timeout` is the idle sub-thread recycling timer, in milliseconds. When the child thread has no task processing and the waiting time exceeds the timer duration, it will exit by itself.
- The maximum number of child threads allowed by the `max` thread pool.
- `concurrency` is used for `pthread_setconcurrency` to set the parallel level reference value, but some systems do not implement this function, so this value should not be relied on too much. Under Linux, setting this value to zero means that the system can determine the degree of parallelism by itself.

Return value: The return value of this function is consistent with the return value of the main thread processing function



#### mln_thread_pool_resource_add

```c
int mln_thread_pool_resource_add(void *data);
```

Description: Put the resource `data` into the resource pool. This function should only be called by the main thread, and is used by the main thread to issue tasks to the child threads.

Return value: return `0` if successful, otherwise return `not 0`



#### mln_thread_quit

```c
void mln_thread_quit(void);
```

Description: This function is used to inform the thread pool to close and destroy the thread pool.

Return value: none



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

Description: Get current thread pool information. The information will be written into the parameter structure. The meaning of each parameter of the structure is as follows:

- `max_num`: the maximum number of child threads in the thread pool
- `idle_num`: the current number of idle child threads
- `cur_num`: the current number of child threads (including idle and working child threads)
- `res_num`: the number of resources that have not yet been processed

Return value: none



### Example

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
    free(data); //The data has been taken over by the child thread,
                //so once this callback function is called,
                //the resource release is no longer the responsibility of the main thread.
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

