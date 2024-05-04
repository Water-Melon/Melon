## Trace Mode

Trace mode is used to collect program information using the Melon library. The general process is as follows:

1. Use the specified function to set the tracepoint in the program
2. Enable the trace mode configuration, and set the path of the Melang script file that receives the tracepoint information
3. Call the `pipe` function in the Melang script to receive the information from the trace function

Melon provides a Melang script example for processing trace information, which is located in `trace/trace.m` in the source code directory.

Unlike ebpf, trace mode in Melon is designed to collect application-related information, that is, (business or functional) information specified by developers. ebpf can collect a large amount of program and kernel information, but it cannot collect business information. Therefore, the trace mode is to complement this part.

For Melang and its supporting library, please refer to [Melang Warehouse](https://github.com/Water-Melon/Melang).

> **NOTE**:
>
> 1. `trace_mode` in the configuration will only be valid when the framework mode is turned on (`framework` is `"multiprocess"` or `"multithread"`).
> 2. If you are in non-framework mode, or `trace_mode` is not enabled in the configuration, but you still want to use this function, you can initialize it by calling `mln_trace_init`.
> 3. In this mode, the system has a certain performance loss, so try not to enable this mode in a production environment. If it must be enabled, you can use a separate thread to run scripts to collect data, which can effectively reduce the performance loss of worker threads.



### Header file

```c
#include "mln_trace.h"
```



### Module

`trace`



### Functions/Macros



#### mln_trace

```c
mln_trace(fmt, ...);

//in msvc, the prototype will be
mln_trace(ret, fmt, ...);
```

Description: Send some data to the specified processing script. The parameters here are exactly the same as `fmt` and its variable parameters of the `mln_lang_ctx_pipe_send` function ([see this chapter for details](https://water-melon.github.io/Melon/en/melang.html)), Because the inside of this macro is to call the function to complete the message delivery. In MSVC, the return value will be assigned to the parameter `ret`.

return value:
- `0` - on success
- `-1` - on failure



#### mln_trace_path

```c
 mln_string_t *mln_trace_path(void);
```

Description: Returns the trace script path set by the `trace_mode` configuration item in the configuration.

return value:

- `NULL` - if the configuration does not exist or the parameter of the configuration item is `off`
- the file path of the trace script



#### mln_trace_init

```c
int mln_trace_init(mln_event_t *ev, mln_string_t *path);
```

Description: Initialize the global trace module.

- `ev` is the event structure that the trace script depends on
- `path` is the file path of the trace script

If the tracing script is successfully initialized in the main process, a global `MASTER` variable will be added to it with a value of `true`.

return value:

- `0` - on success
- `-1` - on failure



#### mln_trace_task_get

```c
mln_lang_ctx_t *mln_trace_task_get(void);
```

Description: Get the script task object used to get trace information in the global trace module.

return value:

- `NULL` - the tracking module has not been initialized or the script task has exited
- script task object



#### mln_trace_finalize

```c
void mln_trace_finalize(void);
```

Description: Destroy all trace structures and reset the global pointer.

return value: none



#### mln_trace_init_callback_set

```c
void mln_trace_init_callback_set(mln_trace_init_cb_t cb);
typedef int (*mln_trace_init_cb_t)(mln_lang_ctx_t *ctx);
```

Description: This function is used to set the initialization callback of the tracing script, which will be called in `mln_trace_init`.

return value: none



#### mln_trace_recv_handler_set

```c
int mln_trace_recv_handler_set(mln_lang_ctx_pipe_recv_cb_t recv_handler);
typedef int (*mln_lang_ctx_pipe_recv_cb_t)(mln_lang_ctx_t *, mln_lang_val_t *);
```

Description: This function is used to set the processing function of the data passed to the C layer by the `Pipe` function in the trace script.

Return value: returns `0` on success, otherwise returns `-1`



### Example

After installing Melon, we proceed as follows:

1. Enable trace mode configuration, edit `conf/melon.conf` under the installation path, set `framework` to `multiprocess`, and delete the comment (`//`) before `trace_mode`. In this example `worker_proc` is set to `1`.

   > If you want to disable the trace mode, you only need to comment the configuration or change the configuration item to `trace_mode off;`.

2. Create a new file named `a.c`

   ```c
   #include <stdio.h>
   #include "mln_log.h"
   #include "mln_framework.h"
   #include "mln_trace.h"
   #include "mln_conf.h"
   
   static void timeout_handler(mln_event_t *ev, void *data)
   {
       mln_trace("sir", "Hello", getpid(), 3.1);
       mln_event_timer_set(ev, 1000, NULL, timeout_handler);
   }
   
   static int recv_handler(mln_lang_ctx_t *ctx, mln_lang_val_t *val)
   {
       mln_log(debug, "%d\n", val->data.i);
       return 0;
   }

   static void worker_process(mln_event_t *ev)
   {
       mln_event_timer_set(ev, 1000, NULL, timeout_handler);
       mln_log(debug, "%d\n", mln_trace_recv_handler_set(recv_handler));
   }
   
   int main(int argc, char *argv[])
   {
       struct mln_framework_attr cattr;
   
       cattr.argc = argc;
       cattr.argv = argv;
       cattr.global_init = NULL;
       cattr.main_thread = NULL;
       cattr.master_process = NULL;
       cattr.worker_process = worker_process;
   
       if (mln_framework_init(&cattr) < 0) {
          fprintf(stderr, "Melon init failed.\n");
          return -1;
       }
   
       return 0;
   }
   ```



3. Compile the a.c file, and then execute the generated executable program, you can see the following output

   ```
   Start up worker process No.1
   02/02/2023 07:08:54 UTC DEBUG: a.c:worker_process:22: PID:58451 0
   master process
   worker process
   [Hello, 58451, 3.100000, ]
   02/02/2023 07:08:56 UTC DEBUG: a.c:recv_handler:15: PID:58451 1
   [Hello, 58451, 3.100000, ]
   02/02/2023 07:08:57 UTC DEBUG: a.c:recv_handler:15: PID:58451 1
   [Hello, 58451, 3.100000, ]
   02/02/2023 07:08:58 UTC DEBUG: a.c:recv_handler:15: PID:58451 1
   ...
   ```

