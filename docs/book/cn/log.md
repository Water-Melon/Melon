## 日志


日志模块在`msvc`环境中将不是多线程安全的。



### 头文件

```c
#include "mln_log.h"
```



### 模块名

`log`



### 函数



#### mln_log_init

```c
int mln_log_init(mln_conf_t *cf);
```

描述：初始化全局日志模块，`cf`若为`NULL`，则使用Melon配置文件中配置进行初始化，否则将使用参数`cf`的配置对日志进行初始化。

返回值：成功则返回`0`，否则返回`-1`



#### mln_log

```c
enum log_level {
    none,
    report,
    debug,
    warn,
    error
};

void _mln_sys_log(enum log_level level, const char *file, const char *func, int line, char *msg, ...);

#define mln_log(err_lv,msg,...) _mln_sys_log(err_lv, __FILE__, __FUNCTION__, __LINE__, msg, ## __VA_ARGS__)
```

描述：

日常开发中，经常被用到的是宏`mln_log`，它会将输出日志的文件、函数、行数都自行附加上。

日志分为5个等级，其级别由上至下依次增加。在配置文件中，有一配置项用于控制输出日志的级别，低于该级别的日志将不会进行输出：

```
log_level "none";
```

默认情况下为最低级别`none`。

`none`与其他级别有所不同，该级别下，所有日志输出的内容完全为`msg`的内容，而不带有任何前缀信息，如：日期、进程号、文件名、函数名、行号等。

该函数需要在`mln_framework_init`之后或其回调函数中使用，在`mln_framework_init`之前使用将会出错，因为此时日志相关组件尚未被初始化。



#### mln_log_dir_path

```c
char *mln_log_dir_path(void);
```

描述：获取日志文件所在目录路径。

返回值：日志目录路径字符串



#### mln_log_logfile_path

```c
char *mln_log_logfile_path(void);
```

描述：获取日志文集路径。

返回值：日志文件路径字符串



#### mln_log_pid_path

```c
char *mln_log_pid_path(void);
```

描述：获取PID文件路径，该文件记录了主进程的进程ID。

返回值：PID文件路径字符串



#### mln_log_logger_set

```c
void mln_log_logger_set(mln_logger_t logger);

typedef void (*mln_logger_t)(mln_log_t *log, mln_log_level_t level, const char *filename, const char *funcname, int line, char *fmt, va_list args);
```

描述：设置自定义的日志记录函数。`mln_logger_t`为函数原型，所有参数均为日志模块传入，其中：`args`为可变参数部分。

返回值：无



#### mln_log_logger_get

```c
mln_logger_t mln_log_logger_get(void);
```

描述：获取当前日志处理函数指针，可用于在自定义处理函数时，串联处理函数。

返回值：返回当前日志处理函数指针




### 示例

```c
#include "mln_log.h"

int main(int argc, char *argv[])
{
    mln_log(debug, "This will be outputted to stderr\n");
    mln_conf_load();
    mln_log_init(NULL);
    mln_log(debug, "This will be outputted to stderr and log file\n");
    return 0;
}
```

