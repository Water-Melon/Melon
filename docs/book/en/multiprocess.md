## Multi-Process Framework



At the beginning of Melon's development, it was necessary to support the multi-process model, which was mainly due to Nginx and previous user-mode network program development experience. Therefore, multiprocessing in Melon also continues the asynchronous event pattern similar to Nginx.

Let's use Melon to complete a multi-process example. Although this example is simple, users will find that in fact Melon does not interfere or even gives to the user great freedom of development.

This feature is not supported in the MSVC.


After installation, we first need to create a source file called `hello.c` to complete the desired function:

```cpp
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

This code mainly initializes a global variable, and then creates a timed event for each child process, that is, outputs a hello world every second.

We compile and link the source file to generate an executable program:

```bash
$ gcc -o hello hello.c -I /usr/local/melon/include/ -L /usr/local/melon/lib/ -lmelon
```

Then, we need to modify the configuration file of the Melon library first:

```
$ sudo vim /usr/local/melon/conf/melon.conf

log_level "none";
//user "root";
daemon off;
core_file_size "unlimited";
//max_nofile 1024;
worker_proc 1;
framework off;
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

We make the following modifications:

- `framework off;` --> `framework "multiprocess";`
- `worker_proc 1;` --> `worker_proc 3;`

In this way, the multi-process framework will be enabled and three child processes will be spawned.

Finally, the program starts as follows:

```
$ ./hello
Start up worker process No.1
Start up worker process No.2
Start up worker process No.3
02/08/2021 09:34:46 UTC DEBUG: hello.c:print_handler:39: PID:25322 hello world

02/08/2021 09:34:46 UTC DEBUG: hello.c:print_handler:39: PID:25323 hello world

02/08/2021 09:34:46 UTC DEBUG: hello.c:print_handler:39: PID:25324 hello world

02/08/2021 09:34:47 UTC DEBUG: hello.c:print_handler:39: PID:25322 hello world

02/08/2021 09:34:47 UTC DEBUG: hello.c:print_handler:39: PID:25323 hello world

02/08/2021 09:34:47 UTC DEBUG: hello.c:print_handler:39: PID:25324 hello world

...
```

At this time, you can `ps` to see that there are four hello processes in total, one is the main process, and the other three are child processes.



We can see that in the global initialization function, we can freely deal with global variables, and even rewrite the configuration of the current framework in it to force the framework to initialize according to our expectations. In the worker process processing function, we can freely write our program logic without worrying that other complicated logic will interfere with it.
