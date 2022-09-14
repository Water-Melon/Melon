## Script task

The script tasks are divided into the use of C-related functions, and the use of the script's own syntax and function library. Details of the latter can be found at: [Melang.org](https://melang.org).

This script is a synchronously written but purely asynchronous script. Scripts can implement multitasking preemptive scheduling and execution in a single thread without affecting the processing of other asynchronous events in the thread. In other words, scripts can be processed in the same thread as asynchronous network IO.

This article only provides functions for creating script tasks and scheduling script tasks. For extended functions, please refer to the subsequent script development articles.



### Header file

```c
#include "mln_lang.h"
```



### Functions/Macros



#### mln_lang_new

```c
mln_lang_t *mln_lang_new(mln_event_t *ev);
```

Description: Create a script management structure, which is the management structure of script tasks, and is used to maintain the resources, scheduling, creation, deletion and other related content of multiple script tasks. Each script task on it is regarded as a coroutine. `ev` is the event structure each script task depends on. In other words, the execution of script tasks is dependent on Melon's asynchronous event API.

Return value: If successful, return the script management structure `mln_lang_t` pointer, otherwise return `NULL`



#### mln_lang_free

```c
void mln_lang_free(mln_lang_t *lang);
```

Description: Destroy and release all resources of the script management structure.

Return value: none



#### mln_lang_job_new

```c
mln_lang_ctx_t *mln_lang_job_new(mln_lang_t *lang, mln_u32_t type, mln_string_t *data, void *udata, mln_lang_return_handler handler);

typedef void (*mln_lang_return_handler)(mln_lang_ctx_t *);
```

Description: Create a script task. The meaning of the parameters is as follows:

- `lang` script management structure, created by `mln_lang_new`. Once created, this task will be managed by this structure.
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



### Example

The best example is the source code of the Melang repository, which has only one file with no more than 200 lines, and only 35 lines of code for script calls. This repository is just a starter for the Melon core library. For details, see: [melang.c](https://github.com/Water-Melon/Melang/blob/master/melang.c).

