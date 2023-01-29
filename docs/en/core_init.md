## Initialization



### Header file

```
#include "mln_core.h"
```



### Structure

```c
struct mln_core_attr {
    int                       argc; //Usually the argc of main
    char                    **argv; //Usually the argv of main
    mln_core_init_t           global_init; //The initialization callback function is generally used to initialize global variables. The callback will be called after the configuration is loaded.
    mln_core_process_t        main_thread; //The main thread handler function, which we will dive into in the mutli-thread framework section
    mln_core_process_t        master_process; //The main process handler function, which we will dive into in the multi-process framework section
    mln_core_process_t        worker_process; //Worker process handlers, which we'll dive into in the multiprocessing framework section
};

typedef int (*mln_core_init_t)(void);
typedef void (*mln_core_process_t)(mln_event_t *);
```

In general, in each component of Melon, the structures used as initialization parameters end with `_attr`, and are not defined as a certain type by `typedef`.



### Functions



#### mln_core_init

```c
int mln_core_init(struct mln_core_attr *attr)；
```

Description: This function is the overall initialization function of the Melon library. It loads the configuration and enables or disables some functions of the framework in Melon according to the configuration, as well as other additional functions.

Return value: Returns 0 if successful, otherwise returns -1.



### Example：

```c
int main(int argc, char *argv[])
{
    struct mln_core_attr cattr;
    cattr.argc = argc;
    cattr.argv = argv;
    cattr.global_init = NULL;
    cattr.main_thread = NULL;
    cattr.master_process = NULL;
    cattr.worker_process = NULL;
    return mln_core_init(&cattr);
}
```

