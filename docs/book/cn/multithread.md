## 多线程框架



本功能在MSVC环境中暂不支持。

多线程框架与前面介绍的线程池不同，是一种模块化线程。模块化线程是指，每一个线程都是一个独立的代码模块，都有各自对应的入口函数（类似于每一个 C 语言程序有一个 main 函数一样）。

> 自2.3.0版本起，多线程模块不再是以在Melon目录下的threads目录中编写代码文件的方式进行集成，而是采用注册函数，由使用者在程序中进行注册加载。这样可以解除线程模块代码与Melon库在目录结构上的耦合。
>



### **开发流程**

1. 首先，编写一个源文件：

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

   这段代码中有几个注意事项：

   - 在main中，我们使用`mln_thread_module_set`函数将每个线程模块的入口与线程模块名映射关系加载到Melon库中。
   - 在`mln_framework_init`会根据下面步骤中对配置文件的设置来自动初始化子线程，并进入线程开始运行。
   - 可以看到，两个线程模块之间是可以通信的，通信的方式是使用最后一个参数（`argv`的最后一个），来向主线程发送消息，消息中包含了目的线程的名字。这里注意，`argv`是在后面步骤中在配置文件中给出的，但是最后一个参数是程序自动在配置项后增加的一个参数，为主子线程间的通信套接字文件描述符。
   - 在 hello 这个模块中，调用了`mln_thread_cleanup_set`函数，这个函数的作用是：在从当前线程模块的入口函数返回至上层函数后，将会被调用，用于清理自定义资源。每一个线程模块的清理函数只能被设置一个，多次设置会被覆盖，清理函数是线程独立的，因此不会出现覆盖其他线程处理函数的情况（当然，你也可以故意这样来构造，比如传一个处理函数指针给别的模块，然后那个模块再进行设置）。


2. 对该文件进行编译：

   ```shell
   $ cc -o test test.c -I /path/to/melon/include -L /path/to/melon/lib -lmelon -lpthread
   ```

3. 修改配置文件

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

   这里主要关注`framework`以及`thread_exec`的配置项。`thread_exec`配置块专门用于模块化线程之用，其内部每一个配置项均为线程模块。

   以 hello 为例：

   ```
   restart "hello" "hello" "world";
   ```

   `restart`或者`default`是指令，`restart`表示线程退出主函数后，再次启动线程。而`default`则表示一旦退出便不再启动。其后的`hello`字符串就是模块的名称，其余则为模块参数，即入口函数的`argc`和`argv`的部分。而与主线程通信的套接字则不必写在此处，而是线程启动后进入入口函数前自动添加的。

4. 运行程序

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

   可以看到，事实上 Melon 中会启动工作进程来拉起其子线程，而工作进程数量由`worker_proc`配置项控制，如果多于一个，则每个工作进程都会拉起一组haha和hello线程。此外，我们也看到，hello线程退出后，清理函数被调用。

### 高级使用

除了使用`mln_thread_module_set`注册线程模块外，还可以使用API动态添加和删除线程。这一功能可以使得线程可以根据外部需求进行动态部署和下线。

我们来看一个简单的例子：

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

这段代码中，我们使用`main_thread`这个回调来让worker进程的主线程增加一些初始化处理。

在`main_thread`中分配了一个指针数组，用来作为线程入口参数。并且利用`mln_thread_create`函数创建了一个名为`hello`的线程，线程的入口函数是`hello`，这个线程的类型是`THREAD_DEFAULT`（退出后不会重启）。然后设置了一个1秒的定时器。

每秒钟进入一次定时处理函数，函数中通过全局变量`sw`来杀掉和创建`hello`线程。

`hello`线程则是死循环输出hello字符串。

这里有一个点要**注意**：如果要使用`mln_thread_kill`杀掉子线程，则子线程内不能使用`mln_log`来打印日志，因为可能会导致日志函数锁无法释放而致使主线程死锁。
