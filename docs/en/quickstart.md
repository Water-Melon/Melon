## Quickstart

The use of Melon is not complicated, and the general steps can be divided into:

1. Initialize the library before use
2. Introduce the corresponding function header file and call the function to use it
3. Modify the configuration file as needed before starting the program

#### Component usage example

Here's an example of using a memory pool:

```c
#include <stdio.h>
#include "mln_core.h"
#include "mln_alloc.h"
#include <mln_log.h>

int main(int argc, char *argv[])
{
    char *ptr;
    mln_alloc_t *pool;
    struct mln_core_attr cattr;

    cattr.argc = argc;
    cattr.argv = argv;
    cattr.global_init = NULL;
    cattr.master_process = NULL;
    cattr.worker_process = NULL;

    if (mln_core_init(&cattr) < 0) {
       fprintf(stderr, "Melon init failed.\n");
       return -1;
    }

    pool = mln_alloc_init(NULL);

    ptr = mln_alloc_m(pool, 1024);
    mln_log(debug, "%X\n", ptr);
    mln_alloc_free(ptr);

    mln_alloc_destroy(pool);
    return 0;
}
```

Among them, `mln_core_init` is the initialization function of the Melon library, and the function parameter is a structure, which is used to pass in `program parameters`, `global variable initialization function` and `worker process processing function`. Since this example does not intend to enable the multi-process framework and does not need to initialize some global variables, both function pointers are nulled.

In the code that follows:

- `mln_alloc_init`: used to create a memory pool
- `mln_alloc_m`: used to allocate a block of memory of a specified size from the memory pool
- `mln_alloc_free`: used to free the memory allocated by the memory pool back to the pool
- `mln_alloc_destroy`: used to destroy the memory pool and free resources

Memory pool related functions and structure definitions are in `mln_alloc.h`.

`mln_log` is the log output function of Melon. In this example, it outputs the starting address of the memory allocated by the memory pool in hexadecimal.



Then compile the code, taking the UNIX system as an example:

```bash
$ cc -o test test.c -I /usr/local/melon/include/ -L /usr/local/melon/lib/ -lmelon
```

On Windows, users can also execute in git bash:

```bash
$ gcc -o test test.c -I $HOME/libmelon/include/ -L $HOME/libmelon/lib/ -llibmelon -lWs2_32
```



At this point, the `test` program cannot be started yet, because we first need to check the configuration file of the Melon library to ensure that the configuration does not cause the program to start a multi-process or multi-threaded framework (Windows users can ignore this step).

```
$ vim /usr/local/melon/conf/melon.conf

log_level "none";
//user "root";
daemon off;
core_file_size "unlimited";
//max_nofile 1024;
worker_proc 1;
thread_mode off;
framework off;
log_path "/usr/local/melon/logs/melon.log";
/*
 * Configurations in the 'exec_proc' are the
 * processes which are customized by user.
 *
 * Here is an example to show you how to
 * spawn a program.
 *     keepalive "/tmp/a.out" ["arg1" "arg2" ...]
 * The command in this example is 'keepalive' that
 * indicate master process to supervise this
 * process. If process is killed, master process
 * would restart this program.
 * If you don't want master to restart it, you can
 *     default "/tmp/a.out" ["arg1" "arg2" ...]
 *
 * But you should know that there is another
 * arugment after the last argument you write here.
 * That is the file descriptor which is used to
 * communicate with master process.
 */
exec_proc {
   // keepalive "/tmp/a";
}
thread_exec {
//    restart "hello" "hello" "world";
//    default "haha";
}
```

Here we need to make sure that the `framework` item is `off`, because this example does not need to start the framework function.



At this point, we can execute the example.

```bash
$ ./test
```

You should see output similar to the following:

```
03/27/2021 04:36:26 GMT DEBUG: test.c:main:25: PID:24077 1e29950
```



#### Example of using the multi-process framework

The framework function is not supported in Windows temporarily, this example will be demonstrated in UNIX system.

```c
#include <stdio.h>
#include "mln_core.h"
#include "mln_log.h"
#include "mln_event.h"

char text[1024];

static int global_init(void);
static void worker_process(mln_event_t *ev);
static void print_handler(mln_event_t *ev, void *data);

int main(int argc, char *argv[])
{
    struct mln_core_attr cattr;
    cattr.argc = argc;
    cattr.argv = argv;
    cattr.global_init = global_init;
    cattr.master_process = NULL;
    cattr.worker_process = worker_process;
    return mln_core_init(&cattr);
}

static int global_init(void)
{
    //global variable init function
    int n = snprintf(text, sizeof(text)-1, "hello world\n");
    text[n] = 0;
    return 0;
}

static void worker_process(mln_event_t *ev)
{
    //we can set event handler here
    //let's set a timer
    mln_event_timer_set(ev, 1000, text, print_handler);
}

static void print_handler(mln_event_t *ev, void *data)
{
    mln_log(debug, "%s\n", (char *)data);
    mln_event_timer_set(ev, 1000, data, print_handler);
}
```

In this example, we add handlers for `global_init` and `worker_process`. `global_init` is used to initialize a global character array `text`. And `worker_process` is the processing function of the child process (or called the worker process). In the worker process processing function, we use the timer function of the Melon event module to call the `print_handler` function every 1 second (1000 milliseconds) to log the contents of the character array `text`.

Generate executable program:

```bash
$ cc -o hello hello.c -I /usr/local/melon/include/ -L /usr/local/melon/lib/ -lmelon
```

Next, check the configuration file again, but this time we want to make sure that `framework` is `on` and `thread_mode` is `off`. Such a configuration indicates that if we enable Melon's framework functions, but do not enable multi-threaded mode, then Melon will enable multi-process mode. Then, modify the number of `worker_proc` as needed, for example: 3.

```
log_level "none";
//user "root";
daemon off;
core_file_size "unlimited";
//max_nofile 1024;
worker_proc 3;
thread_mode off;
framework on;
log_path "/usr/local/melon/logs/melon.log";
/*
 * Configurations in the 'exec_proc' are the
 * processes which are customized by user.
 *
 * Here is an example to show you how to
 * spawn a program.
 *     keepalive "/tmp/a.out" ["arg1" "arg2" ...]
 * The command in this example is 'keepalive' that
 * indicate master process to supervise this
 * process. If process is killed, master process
 * would restart this program.
 * If you don't want master to restart it, you can
 *     default "/tmp/a.out" ["arg1" "arg2" ...]
 *
 * But you should know that there is another
 * arugment after the last argument you write here.
 * That is the file descriptor which is used to
 * communicate with master process.
 */
exec_proc {
   // keepalive "/tmp/a";
}
thread_exec {
//    restart "hello" "hello" "world";
//    default "haha";
}
```

Now, we can start the program:

```bash
$ ./hello
```

We can see output similar to the following:

```
Start up worker process No.1
Start up worker process No.2
Start up worker process No.3
03/27/2021 04:53:44 GMT DEBUG: d.c:print_handler:39: PID:27620 hello world

03/27/2021 04:53:44 GMT DEBUG: d.c:print_handler:39: PID:27621 hello world

03/27/2021 04:53:44 GMT DEBUG: d.c:print_handler:39: PID:27622 hello world

03/27/2021 04:53:45 GMT DEBUG: d.c:print_handler:39: PID:27620 hello world

03/27/2021 04:53:45 GMT DEBUG: d.c:print_handler:39: PID:27621 hello world

03/27/2021 04:53:45 GMT DEBUG: d.c:print_handler:39: PID:27622 hello world

...
```



At this point, the quick start section has come to an end. We will explain and give examples of each function provided by Melon in other sections.
