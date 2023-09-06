## Quickstart

The use of Melon is not complicated, and the general steps can be divided into:

1. Initialize the library before use
2. Introduce the corresponding function header file and call the function to use it
3. Modify the configuration file as needed before starting the program

#### Component usage example

Here's an example of using a memory pool:

```c
#include <stdio.h>
#include "mln_alloc.h"

int main(int argc, char *argv[])
{
    char *ptr;
    mln_alloc_t *pool;

    pool = mln_alloc_init(NULL);

    ptr = mln_alloc_m(pool, 1024);
    printf("%p\n", ptr);
    mln_alloc_free(ptr);

    mln_alloc_destroy(pool);
    return 0;
}
```

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

Let's execute the example.

```bash
$ ./test
```

The output can be seen on terminal:

```
0xaaaad0ea57d0
```



#### Example of using the multi-process framework

The framework function is not supported in Windows temporarily, this example will be demonstrated in UNIX system.

```c
#include <stdio.h>
#include "mln_framework.h"
#include "mln_log.h"
#include "mln_event.h"

char text[1024];

static int global_init(void);
static void worker_process(mln_event_t *ev);
static void print_handler(mln_event_t *ev, void *data);

int main(int argc, char *argv[])
{
    struct mln_framework_attr cattr;
    cattr.argc = argc;
    cattr.argv = argv;
    cattr.global_init = global_init;
    cattr.main_thread = NULL;
    cattr.master_process = NULL;
    cattr.worker_process = worker_process;
    return mln_framework_init(&cattr);
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

Among them, `mln_framework_init` is the initialization function of the Melon library, and the function parameter is a structure, which is used to pass in `program parameters`, `global variable initialization function` and `worker process processing function`.

In this example, we add handlers for `global_init` and `worker_process`. `global_init` is used to initialize a global character array `text`. And `worker_process` is the processing function of the child process (or called the worker process). In the worker process processing function, we use the timer function of the Melon event module to call the `print_handler` function every 1 second (1000 milliseconds) to log the contents of the character array `text`.

Generate executable program:

```bash
$ cc -o hello hello.c -I /usr/local/melon/include/ -L /usr/local/melon/lib/ -lmelon
```

Next, check the configuration file again, but this time we want to make sure that `framework` is `multiprocess`. Such a configuration indicates that if we enable Melon's framework functions, but do not enable multi-threaded mode, then Melon will enable multi-process mode. Then, modify the number of `worker_proc` as needed, for example: 3.

```
log_level "none";
//user "root";
daemon off;
core_file_size "unlimited";
//max_nofile 1024;
worker_proc 3;
framework "multiprocess";
log_path "/usr/local/melon/logs/melon.log";
/*
 * Configurations in the 'proc_exec' are the
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
proc_exec {
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
03/27/2021 04:53:44 UTC DEBUG: d.c:print_handler:39: PID:27620 hello world

03/27/2021 04:53:44 UTC DEBUG: d.c:print_handler:39: PID:27621 hello world

03/27/2021 04:53:44 UTC DEBUG: d.c:print_handler:39: PID:27622 hello world

03/27/2021 04:53:45 UTC DEBUG: d.c:print_handler:39: PID:27620 hello world

03/27/2021 04:53:45 UTC DEBUG: d.c:print_handler:39: PID:27621 hello world

03/27/2021 04:53:45 UTC DEBUG: d.c:print_handler:39: PID:27622 hello world

...
```



At this point, the quick start section has come to an end. We will explain and give examples of each function provided by Melon in other sections.
