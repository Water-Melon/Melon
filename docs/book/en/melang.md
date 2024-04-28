## Script task

The script tasks are divided into the use of C-related functions, and the use of the script's own syntax and function library. Details of the latter can be found at: [Melang.org](https://melang.org).

This script is a synchronously written but purely asynchronous script. Scripts can implement multitasking step-sharing scheduling and execution in a single thread without affecting the processing of other asynchronous events in the thread. In other words, scripts can be processed in the same thread as asynchronous network IO.

This article only provides functions for creating script tasks and scheduling script tasks. For extended functions, please refer to the subsequent script development articles.



### Header file

```c
#include "mln_lang.h"
```



### Module

`lang`



### Functions/Macros



#### mln_lang_new

```c
mln_lang_t *mln_lang_new(mln_event_t *ev, mln_lang_run_ctl_t signal, mln_lang_run_ctl_t clear);

typedef int (*mln_lang_run_ctl_t)(mln_lang_t *);
```

Description: Create a script management structure, which is the management structure of script tasks, and is used to maintain the resources, scheduling, creation, deletion and other related content of multiple script tasks. Each script task on it is regarded as a coroutine. `ev` is the event structure each script task depends on. In other words, the execution of script tasks is dependent on Melon's asynchronous event API. `signal` is used to trigger event to run tasks. `clear` is used to remove event and prevent task to run.

Return value: If successful, return the script management structure `mln_lang_t` pointer, otherwise return `NULL`



#### mln_lang_free

```c
void mln_lang_free(mln_lang_t *lang);
```

Description: Destroy and release all resources of the script management structure.

Return value: none



#### mln_lang_job_new

```c
mln_lang_ctx_t *mln_lang_job_new(mln_lang_t *lang, mln_string_t *alias, mln_u32_t type, mln_string_t *data, void *udata, mln_lang_return_handler handler);

typedef void (*mln_lang_return_handler)(mln_lang_ctx_t *);
```

Description: Create a script task. The meaning of the parameters is as follows:

- `lang` script management structure, created by `mln_lang_new`. Once created, this task will be managed by this structure.
- `alias`: An alias for the script task. Set to `NULL` if not needed.
- `type` Type of script task code: `M_INPUT_T_FILE` (file), `M_INPUT_T_BUF` (string).
- `data` This parameter has different meanings depending on the `type`. When it is a file, this parameter is the file path; when it is a string, this parameter is a code string.
- `udata` user-defined structure, generally used for third-party library function implementation and `handler` to obtain the task return value.
- `handler` Gets the task return value when the task is completed normally. This function has only one parameter, which is the script task structure. If you need to customize the structure to help implement some functions, you can set the `udata` field.

Return value: If successful, return the script structure `mln_lang_ctx_t` pointer, otherwise return `NULL`



#### mln_lang_job_free

```c
void mln_lang_job_free(mln_lang_ctx_t *ctx);
```

Description: Destroy and release all resources of the script task structure.

Return value: none



#### mln_lang_cache_set

```c
mln_lang_cache_set(lang)
```

Description: Set whether to cache abstract syntax tree structures. After setting this cache, when creating tasks with the same script code, the abstract syntax tree generation process can be omitted and used directly to improve performance.

Return value: none



#### mln_lang_ctx_data_get

```c
mln_lang_ctx_data_get(ctx)
```

Description: Get user-defined data in `ctx` of a pointer of type `mln_lang_ctx_t` of script task.

Return value: user-defined data pointer



#### mln_lang_ctx_data_set

```c
mln_lang_ctx_data_set(ctx, d)
```

Description: Set user-defined data 'd' to script task `ctx`.

Return value: none



#### mln_lang_launcher_get

```c
mln_lang_launcher_get(lang)
```

Description: Get the launcher function pointer in the `lang` structure, which is the handler for the file descriptor event in `mln_event_t`. The purpose of allowing access to this function here is to allow the caller to obtain the handler function in the custom `signal` function pointer for setting file descriptor events.

Return value: launcher function pointer



#### mln_lang_event_get

```c
mln_lang_event_get(lang)
```

Description: Get the `mln_event_t` pointer in `lang` for setting/clearing events in `signal` and `clear` callbacks.

Return value: `mln_event_t` pointer



#### mln_lang_signal_get

```c
mln_lang_signal_get(lang)
```

Description: Get the `signal` function pointer in the `lang` structure, which is used to trigger file descriptor events in script development, so that the script task can continue to execute.

Return value: `signal` function pointer



#### mln_lang_task_empty

```c
mln_lang_task_empty(lang)
```

Description: Check if there are no script tasks in `lang` anymore.

Return value:

- `0` has tasks
- `non-0` no task



#### mln_lang_mutex_lock

```c
mln_lang_mutex_lock(lang)
```

Description: Lock the resources in `lang`, for example, it should be locked before calling `mln_lang_task_empty` in script development.

Return value:

- `0` on success
- `non-0` on fail



#### mln_lang_mutex_unlock

```c
mln_lang_mutex_unlock(lang)
```

Description: Unlock resources in `lang`, for example, it should be unlocked after calling `mln_lang_task_empty` in script development.

Return value:

- `0` on success
- `non-0` on fail



#### mln_lang_ctx_pipe_send

````c
int mln_lang_ctx_pipe_send(mln_lang_ctx_t *ctx, char *fmt, ...)
````

Description: Send a message to the specified script task in C code. This message can be received by the `Pipe` function of the script layer, where:

- `ctx` is the context structure pointer corresponding to the script task
- `fmt` is used for the interpretation of variable parameters, `fmt` supports four characters:
   - `i` integer, which should be an `mln_s64_t` type integer
   - `r` real number, which should be of type `double`
   - `s` string, which should correspond to the `char` pointer parameter
   - `S` string, which should correspond to the `mln_string_t` pointer parameter

return value:

- `0` on success
- `-1` fails

Example:

````c
mln_string_t s = mln_string("hello");
mln_lang_ctx_pipe_send(ctx, "sir", &s, 1, 3.14);
````



#### mln_lang_ctx_pipe_recv_handler_set

```c
int mln_lang_ctx_pipe_recv_handler_set(mln_lang_ctx_t *ctx, mln_lang_ctx_pipe_recv_cb_t recv_handler);
```

Description: Set the callback function `recv_handler` that receives the data sent from the script layer `Pipe` function, which is defined as:

```c
typedef int (*mln_lang_ctx_pipe_recv_cb_t)(mln_lang_ctx_t *, mln_lang_val_t *);
```

- The first parameter is the context structure pointer of the script task
- The second is the value of variable, which is of type `mln_lang_val_t`

return value:

- `0` for success
- `-1` fails




#### mln_lang_ctx_is_quit

````c
mln_lang_ctx_is_quit(ctx)
````

Description: Indicates whether the program exited without completion.

return value: none



### Example

The best example is the source code of the Melang repository, which has only one file with no more than 200 lines, and only 35 lines of code for script calls. This repository is just a starter for the Melon core library. For details, see: [melang.c](https://github.com/Water-Melon/Melang/blob/master/melang.c).

