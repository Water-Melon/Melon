## Multi-thread Framework



Different from the thread pool described above, the multi-threaded framework is a modular thread. Modular threads mean that each thread is an independent code module with its own corresponding entry function (similar to the fact that every C language program has a main function).

> Since version 2.3.0, the multi-threading module is no longer integrated by writing code files in the threads directory under the Melon directory, but using the registration function to be registered and loaded by the user in the program. This decouples the thread module code from the Melon library in the directory structure.

This feature is not supported in the MSVC.


### Development steps

1. First, write a source file:

   ```c
   #include <stdio.h>
   #include <assert.h>
   #include <string.h>
   #include <errno.h>
   #include "mln_framework.h"
   #include "mln_log.h"
   #include "mln_thread.h"
   
   static int haha(int argc, char **argv)
   {
       int fd = atoi(argv[argc-1]);
       mln_thread_msg_t msg;
       int nfds;
       fd_set rdset;
       for (;;) {
           FD_ZERO(&rdset);
           FD_SET(fd, &rdset);
           nfds = select(fd+1, &rdset, NULL, NULL, NULL);
           if (nfds < 0) {
               if (errno == EINTR) continue;
               mln_log(error, "select error. %s\n", strerror(errno));
               return -1;
           }
           memset(&msg, 0, sizeof(msg));
           int n = recv(fd, &msg, sizeof(msg), 0);
           if (n != sizeof(msg)) {
               mln_log(debug, "recv error. n=%d. %s\n", n, strerror(errno));
               return -1;
           }
           mln_log(debug, "!!!src:%S auto:%l char:%c\n", msg.src, msg.sauto, msg.c);
           mln_thread_clear_msg(&msg);
       }
       return 0;
   }
   
   static void hello_cleanup(void *data)
   {
       mln_log(debug, "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n");
   }
   
   static int hello(int argc, char **argv)
   {
       mln_thread_cleanup_set(hello_cleanup, NULL);
       int i;
       for (i = 0; i < 1; ++i)  {
           int fd = atoi(argv[argc-1]);
           mln_thread_msg_t msg;
           memset(&msg, 0, sizeof(msg));
           msg.dest = mln_string_new("haha");
           assert(msg.dest);
           msg.sauto = 9736;
           msg.c = 'N';
           msg.type = ITC_REQUEST;
           msg.need_clear = 1;
           int n = send(fd, &msg, sizeof(msg), 0);
           if (n != sizeof(msg)) {
               mln_log(debug, "send error. n=%d. %s\n", n, strerror(errno));
               mln_string_free(msg.dest);
               return -1;
           }
       }
       usleep(100000);
       return 0;
   }
   
   int main(int argc, char *argv[])
   {
       mln_thread_module_t modules[] = {
         {"haha", haha},
         {"hello", hello},
       };
   
       struct mln_framework_attr cattr;
   
       cattr.argc = argc;
       cattr.argv = argv;
       cattr.global_init = NULL;
       cattr.main_thread = NULL;
       cattr.worker_process = NULL;
       cattr.master_process = NULL;
   
       mln_thread_module_set(modules, 2);
   
       if (mln_framework_init(&cattr) < 0) {
          fprintf(stderr, "Melon init failed.\n");
          return -1;
       }
   
       return 0;
   }
   ```

   There are a few caveats in this code:

   - In main, we use the `mln_thread_module_set` function to load the mapping between the entry of each thread module and the thread module name into the Melon library.
   - In `mln_framework_init`, the child thread will be automatically initialized according to the settings of the configuration file in the following steps, and enter the thread to start running.
   - It can be seen that the two thread modules can communicate. The way of communication is to use the last parameter (the last of `argv`) to send a message to the main thread, and the message contains the name of the destination thread. Note here that `argv` is given in the configuration file in the following steps, but the last parameter is a parameter automatically added by the program after the configuration item, which is the communication socket file descriptor between the main and child threads.
   - In the hello module, the `mln_thread_cleanup_set` function is called. The function of this function is: after returning from the entry function of the current thread module to the upper-level function, it will be called to clean up custom resources. Only one cleanup function can be set for each thread module, and multiple settings will be overwritten. The cleanup function is thread-independent, so it will not overwrite other thread processing functions (of course, you can also construct it on purpose, such as Pass a handler function pointer to another module, and then that module will set it).


2. Compile the file:

   ```shell
   $ cc -o test test.c -I /path/to/melon/include -L /path/to/melon/lib -lmelon -lpthread
   ```

3. Modify the configuration file

   ```
   log_level "none";
   //user "root";
   daemon off;
   core_file_size "unlimited";
   //max_nofile 1024;
   worker_proc 1;
   framework "multithread";
   log_path "/home/niklaus/melon/logs/melon.log";
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
       restart "hello" "hello" "world";
       default "haha";
   }
   ```

   Here we mainly focus on the configuration items of `framework` and `thread_exec`. The `thread_exec` configuration block is specially used for modular threads, and each of its internal configuration items is a thread module.

   Take hello as an example:

   ```
   restart "hello" "hello" "world";
   ```

   `restart` or `default` is an instruction, `restart` indicates that after the thread exits the main function, start the thread again. And `default` means that once it exits, it will not start again. The following `hello` string is the name of the module, and the rest are module parameters, that is, the `argc` and `argv` parts of the entry function. The socket that communicates with the main thread does not need to be written here, but is automatically added before entering the entry function after the thread is started.

4. Run

   ```shell
   $ ./test
   Start up worker process No.1
   Start thread 'hello'
   Start thread 'haha'
   04/14/2022 14:50:16 UTC DEBUG: a.c:haha:34: PID:552165 !!!src:hello auto:9736 char:N
   04/14/2022 14:50:16 UTC DEBUG: a.c:hello_cleanup:42: PID:552165 @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
   04/14/2022 14:50:16 UTC REPORT: PID:552165 Thread 'hello' return 0.
   04/14/2022 14:50:16 UTC REPORT: PID:552165 Child thread 'hello' exit.
   04/14/2022 14:50:16 UTC REPORT: PID:552165 child thread pthread_join's exit code: 0
   04/14/2022 14:50:16 UTC DEBUG: a.c:haha:34: PID:552165 !!!src:hello auto:9736 char:N
   04/14/2022 14:50:16 UTC DEBUG: a.c:hello_cleanup:42: PID:552165 @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
   04/14/2022 14:50:16 UTC REPORT: PID:552165 Thread 'hello' return 0.
   04/14/2022 14:50:16 UTC REPORT: PID:552165 Child thread 'hello' exit.
   04/14/2022 14:50:16 UTC REPORT: PID:552165 child thread pthread_join's exit code: 0
   04/14/2022 14:50:16 UTC DEBUG: a.c:haha:34: PID:552165 !!!src:hello auto:9736 char:N
   04/14/2022 14:50:17 UTC DEBUG: a.c:hello_cleanup:42: PID:552165 @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
   04/14/2022 14:50:17 UTC REPORT: PID:552165 Thread 'hello' return 0.
   04/14/2022 14:50:17 UTC REPORT: PID:552165 Child thread 'hello' exit.
   04/14/2022 14:50:17 UTC REPORT: PID:552165 child thread pthread_join's exit code: 0
   ...
   ```

   It can be seen that in fact Melon will start a worker process to pull up its child threads, and the number of worker processes is controlled by the `worker_proc` configuration item. If there is more than one, each worker process will pull up a set of haha and hello threads . In addition, we also see that after the hello thread exits, the cleanup function is called.

### Advanced

In addition to registering thread modules with `mln_thread_module_set`, threads can be dynamically added and removed using the API. This feature enables threads to be dynamically deployed and undeployed according to external requirements.

Let's look at a simple example:

```c
#include <stdio.h>
#include <errno.h>
#include "mln_framework.h"
#include "mln_log.h"
#include "mln_thread.h"
#include <unistd.h>

int sw = 0;
char name[] = "hello";
static void main_thread(mln_event_t *ev);

static int hello(int argc, char *argv[])
{
    while (1) {
        printf("%d: Hello\n", getpid());
        usleep(10);
    }
    return 0;
}

static void timer_handler(mln_event_t *ev, void *data)
{
    if (sw) {
        mln_string_t alias = mln_string("hello");
        mln_thread_kill(&alias);
        sw = !sw;
        mln_event_timer_set(ev, 1000, NULL, timer_handler);
    } else {
        main_thread(ev);
    }
}

static void main_thread(mln_event_t *ev)
{
    char **argv = (char **)calloc(3, sizeof(char *));
    if (argv != NULL) {
        argv[0] = name;
        argv[1] = NULL;
        argv[2] = NULL;
        mln_thread_create(ev, "hello", THREAD_DEFAULT, hello, 1, argv);
        sw = !sw;
        mln_event_timer_set(ev, 1000, NULL, timer_handler);
    }
}

int main(int argc, char *argv[])
{
    struct mln_framework_attr cattr;

    cattr.argc = argc;
    cattr.argv = argv;
    cattr.global_init = NULL;
    cattr.main_thread = main_thread;
    cattr.worker_process = NULL;
    cattr.master_process = NULL;

    if (mln_framework_init(&cattr) < 0) {
       fprintf(stderr, "Melon init failed.\n");
       return -1;
    }

    return 0;
}
```

In this code, we use the `main_thread` callback to add some initialization processing to the main thread of the worker process.

An array of pointers is allocated in `main_thread`, which is used as a thread entry parameter. And use the `mln_thread_create` function to create a thread named `hello`, the entry function of the thread is `hello`, the type of this thread is `THREAD_DEFAULT` (it will not restart after exiting). Then set a timer with a timeout of 1 second.

Enter a timing processing function every second, in which the global variable `sw` is used to control killing or creating `hello` thread.

The `hello` thread outputs the hello string in an endless loop.

**Note**: If you want to use `mln_thread_kill` to kill the child thread, you cannot use `mln_log` to output logs in the child thread, because the log function lock may not be released and the main thread may deadlock.
