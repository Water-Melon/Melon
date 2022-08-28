## Log



### Header file

```c
#include "mln_log.h"
```



### Functions



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

Description:

In daily development, the macro `mln_log` is often used, which will append the file, function, and line number of the output log by itself.

The logs are divided into 5 levels, and the levels increase sequentially from top to bottom. In the configuration file, there is a configuration item used to control the level of output logs, logs below this level will not be output:

```
log_level "none";
```

Defaults to the lowest level `none`.

`none` is different from other levels. Under this level, the content of all log output is completely the content of `msg` without any prefix information, such as: date, process number, file name, function name, line number Wait.

This function needs to be used after `mln_core_init` or its callback function. It will be an error to use it before `mln_core_init`, because the log related components have not been initialized at this time.