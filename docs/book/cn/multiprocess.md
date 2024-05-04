## 多进程模型



本功能在MSVC环境中暂不支持。

Melon开发之初便是要支持多进程模型的，这一点也主要源于Nginx以及以往用户态网络程序开发经历。因此，Melon中的多进程也延续了类似Nginx的异步事件模式。




下面我们来使用Melon来完成一个多进程例子。这个例子虽然简单，但用户会发现，事实上Melon并不干涉甚至是给予用户极大的自由发挥空间。



在安装好后，我们首先要创建一个名为`hello.c`的源文件来完成我们期望的功能：

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

这段代码主要是初始化了一个全局变量，然后给每一个子进程创建了一个定时事件，即每一秒中输出一个 hello world 。

我们对该源文件进行编译链接生成可执行程序：

```bash
$ gcc -o hello hello.c -I /usr/local/melon/include/ -L /usr/local/melon/lib/ -lmelon
```

然后，我们需要先修改 Melon 库的配置文件：

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

我们做如下修改:

- `framework off;` --> `framework "multiprocess";`
- `worker_proc 1;` --> `worker_proc 3;`

这样，多进程框架将被启用，且会产生三个子进程。

最后，程序启动后如下：

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

这时，可以`ps`看一下，一共存在四个 hello 进程，一个为主，其余三个为子进程。



到此，我们的例子已经看完。我们可以看到，在全局的初始化函数中，我们可以自由的对全局变量进行处理，甚至可以在其中改写当前框架的配置以强制框架按照我们的期望进行初始化。而在工作进程处理函数中，我们可以自由的编写我们的程序逻辑，而不会担心会有其他繁杂的逻辑会对此产生干扰。
