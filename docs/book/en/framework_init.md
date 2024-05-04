## Initialization

This component is used to initialize the entire Melon library for the first time.

At different phases in the initialization process of Melon, some callback functions provided by the user will be called for specific phases of processing.

These user-supplied callback functions include:

- `global_init`
- `main_thread`
- `master_process`
- `worker_process`

The approximate calling sequence of these callback functions is as follows:

```
                                             |
                               Configuration initialization
                                             |
                                     call global_init()
                                             |
    Initialization of some frameworks, such as resource limitations, resident tasks, fork, etc.
                                             |
                       call main_thread/master_process/worker_process
                                             |
                                    Handle various events
```

`global_init` is called after configuration initialization, but before fork (if multi-process framework is enabled). At this time, you can get all the configurations of Melon by calling the configuration function, and then do some initialization behaviors for the entire application based on these configurations.

`main_thread` is called in the main thread under the multi-threading model, and is used to set some resident tasks for the main thread.

`master_process`/`worker_process` is called in the master/worker process under the multi-process model, and is used to make some global settings for a single process.

In `MSVC`, there is no `main_thread`, `master_process`, or `worker_process`. The initialization process only returns after initializing configurations, logging, etc.


### Header file

```
#include "mln_framework.h"
```



### Module

`framework`



### Structure

```c
struct mln_framework_attr {
    int                            argc; //Usually the argc of main
    char                         **argv; //Usually the argv of main
    mln_framework_init_t           global_init; //The initialization callback function is generally used to initialize global variables. The callback will be called after the configuration is loaded.
#if !defined(MSVC)
    mln_framework_process_t        main_thread; //The main thread handler function, which we will dive into in the mutli-thread framework section
    mln_framework_process_t        master_process; //The main process handler function, which we will dive into in the multi-process framework section
    mln_framework_process_t        worker_process; //Worker process handlers, which we'll dive into in the multiprocessing framework section
#endif
};

typedef int (*mln_framework_init_t)(void);
typedef void (*mln_framework_process_t)(mln_event_t *);
```

In general, in each component of Melon, the structures used as initialization parameters end with `_attr`, and are not defined as a certain type by `typedef`.



### Functions



#### mln_framework_init

```c
int mln_framework_init(struct mln_framework_attr *attr)；
```

Description: This function is the overall initialization function of the Melon library. It loads the configuration and enables or disables some functions of the framework in Melon according to the configuration, as well as other additional functions.

Return value: Returns 0 if successful, otherwise returns -1.



### Example：

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

