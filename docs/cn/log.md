## 日志



### 头文件

```c
#include "mln_log.h"
```



### 函数



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

该函数需要在`mln_core_init`之后或其回调函数中使用，在`mln_core_init`之前使用将会出错，因为此时日志相关组件尚未被初始化。