## Log


This module is not thread-safe in the `MSVC`.



### Header file

```c
#include "mln_log.h"
```



### Module

`log`



### Functions



#### mln_log_init

```c
int mln_log_init(mln_conf_t *cf);
```

Description: Initialize global log module. If `cf` is `NULL`, log will be initialized with Melon configuration file. Otherwise, log will be initialized with given `cf`.

Return value: `0` on success, otherwise `-1` returned



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

This function needs to be used after `mln_framework_init` or its callback function. It will be an error to use it before `mln_framework_init`, because the log related components have not been initialized at this time.



#### mln_log_dir_path

```c
char *mln_log_dir_path(void);
```

Description: Get the path of the directory where the log file is located.

Return value: log directory path string



#### mln_log_logfile_path

```c
char *mln_log_logfile_path(void);
```

Description: Get the log file path.

Return value: log file path string



#### mln_log_pid_path

```c
char *mln_log_pid_path(void);
```

Description: Get the path of the PID file, which records the process ID of the main process.

Return value: PID file path string



#### mln_log_logger_set

```c
void mln_log_logger_set(mln_logger_t logger);

typedef void (*mln_logger_t)(mln_log_t *log, mln_log_level_t level, const char *filename, const char *funcname, int line, char *fmt, va_list args);
```

Description: Set a custom logging function. `mln_logger_t` is the function prototype, and all parameters are passed in by the log module. `args` is the variable parameter part.

Return value: None



#### mln_log_logger_get

```c
mln_logger_t mln_log_logger_get(void);
```

Description: Get the current log processing function pointer, which can be used to link processing functions when customizing processing functions.

Return value: Returns the current log processing function pointer




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

