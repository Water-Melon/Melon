## 多线程框架



**注意**：Windows下目前不支持本功能。

多线程框架与前面介绍的线程池不同，是一种模块化线程。模块化线程是指，每一个线程都是一个独立的代码模块，都有各自对应的入口函数（类似于每一个 C 语言程序有一个 main 函数一样）。

模块要存放于 `Melon/threads/`目录下。在现有的Melon代码中，包含了两个示例模块——haha 和 hello （名字起得有点随意）。下面，我们以这两个模块为例说明模块化线程的开发和使用流程。



### **开发流程**

这里有几点注意事项：

1. 模块的名字：模块的名字将被用于两个地方，一个是配置文件中，一个是模块入口函数名。前者将在使用流程中说明，后者我们马上将以 haha 为例进行说明。
2. 模块的参数：参数是在配置文件中给出的，这一点我们在使用流程中将会说明。但是需要注意一点，最后一个参数并不是配置文件中给出的，而是框架自动追加的，是主线程与该线程模块通信的 socketpair 套接字。

```c
//haha 模块

int haha_main(int argc, char **argv)
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
#if defined(WIN32)
        int n = recv(fd, (char *)&msg, sizeof(msg), 0);
#else
        int n = recv(fd, &msg, sizeof(msg), 0);
#endif
        if (n != sizeof(msg)) {
            mln_log(debug, "recv error. n=%d. %s\n", n, strerror(errno));
            return -1;
        }
        mln_log(debug, "!!!src:%S auto:%l char:%c\n", msg.src, msg.sauto, msg.c);
        mln_thread_clearMsg(&msg);
    }
    return 0;
}
```

可以看到，在这个例子中，模块的入口函数名为`haha_main`。对于每一个线程模块来说，他们的入口函数就是他们`模块的名称（即文件名）+下划线+main`组成的。

这个例子也很简单，就是利用 select 持续关注主线程消息，当从主线程接收到消息后，就进行日志输出，然后释放资源。

与之功能对应的就是 hello 这个模块：

```c
//hello 模块
#include <assert.h>

static void hello_cleanup(void *data)
{
    mln_log(debug, "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n");
}

int hello_main(int argc, char **argv)
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
#if defined(WIN32)
        int n = send(fd, (char *)&msg, sizeof(msg), 0);
#else
        int n = send(fd, &msg, sizeof(msg), 0);
#endif
        if (n != sizeof(msg)) {
            mln_log(debug, "send error. n=%d. %s\n", n, strerror(errno));
            mln_string_free(msg.dest);
            return -1;
        }
    }
    usleep(100000);
    return 0;
}
```

这个模块的功能也很简单，就是向主线程发送消息，而消息的接收方是 haha 模块，即主线程是一个中转站，它将 hello 模块的消息转发给 haha 模块。

在 hello 这个模块中，调用了`mln_thread_cleanup_set`函数，这个函数的作用是：在从当前线程模块的入口函数返回至上层函数后，将会被调用，用于清理自定义资源。

每一个线程模块的清理函数只能被设置一个，多次设置会被覆盖，清理函数是线程独立的，因此不会出现覆盖其他线程处理函数的情况（当然，你也可以故意这样来构造，比如传一个处理函数指针给别的模块，然后那个模块再进行设置）。



### **使用流程**

使用流程遵循如下步骤：

1. 编写框架启动器
2. 编译链接生成可执行程序
3. 修改配置文件
4. 启动程序

我们逐个步骤进行操作。

我们先编写启动器：

```c
//launcher.c

#include "mln_core.h"

int main(int argc, char *argv[])
{
    struct mln_core_attr cattr;
    cattr.argc = argc;
    cattr.argv = argv;
    cattr.global_init = NULL;
    cattr.worker_process = NULL;
    return mln_core_init(&cattr);
}
```

这里，我们不初始化任何全局变量，也不需要工作进程，因此都置空即可。

```shell
$ cc -o launcher launcher.c -I /usr/local/melon/include/ -L /usr/local/melon/lib/ -lmelon -lpthread
```

生成名为 launcher 的可执行程序。

此时，我们的线程尚不能执行，我们需要修改配置文件：

```
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

上面是默认配置文件，我们要进行如下修改：

- `thread_mode off;` -> `thread_mode on;`
- `framework off;` -> `framework on;`
- `thread_exec`配置块中的两项注释去掉

这里，需要额外说明一下：

`thread_exec`配置块专门用于模块化线程之用，其内部每一个配置项均为线程模块。

以 hello 为例：

```
restart "hello" "hello" "world";
```

`restart`或者`default`是指令，`restart`表示线程退出主函数后，再次启动线程。而`default`则表示一旦退出便不再启动。其后的`hello`字符串就是模块的名称，其余则为模块参数，即入口函数的`argc`和`argv`的部分。而与主线程通信的套接字则不必写在此处，而是线程启动后进入入口函数前自动添加的。

现在，就来启动程序吧。

```shell
$ ./launcher

Start up worker process No.1
Start thread 'hello'
Start thread 'haha'
02/14/2021 04:07:48 GMT DEBUG: ./src/mln_thread_module.c:haha_main:42: PID:9309 !!!src:hello auto:9736 char:N
02/14/2021 04:07:49 GMT DEBUG: ./src/mln_thread_module.c:hello_cleanup:53: PID:9309 @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
02/14/2021 04:07:49 GMT REPORT: PID:9309 Thread 'hello' return 0.
02/14/2021 04:07:49 GMT REPORT: PID:9309 Child thread 'hello' exit.
02/14/2021 04:07:49 GMT REPORT: PID:9309 child thread pthread_join's exit code: 0
02/14/2021 04:07:49 GMT DEBUG: ./src/mln_thread_module.c:haha_main:42: PID:9309 !!!src:hello auto:9736 char:N
02/14/2021 04:07:49 GMT DEBUG: ./src/mln_thread_module.c:hello_cleanup:53: PID:9309 @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
02/14/2021 04:07:49 GMT REPORT: PID:9309 Thread 'hello' return 0.
02/14/2021 04:07:49 GMT REPORT: PID:9309 Child thread 'hello' exit.
02/14/2021 04:07:49 GMT REPORT: PID:9309 child thread pthread_join's exit code: 0
...
```

可以看到，事实上 Melon 中会启动工作进程来拉起其子线程，而工作进程数量由`worker_proc`配置项控制，如果多于一个，则每个工作进程都会拉起一组haha和hello线程。此外，我们也看到，hello线程退出后，清理函数被调用。
