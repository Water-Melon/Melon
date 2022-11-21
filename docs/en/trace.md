## Trace Mode

Trace mode is used to collect program information using the Melon library. The general process is as follows:

1. Use the specified function to set the tracepoint in the program
2. Enable the trace mode configuration, and set the path of the Melang script file that receives the tracepoint information
3. Call the `pipe` function in the Melang script to receive the information from the trace function

Melon provides a Melang script example for processing trace information, which is located in `trace/trace.m` in the source code directory.

Unlike ebpf, trace mode in Melon is designed to collect application-related information, that is, (business or functional) information specified by developers. ebpf can collect a large amount of program and kernel information, but it cannot collect business information. Therefore, the trace mode is to complement this part.



### Header

```c
#include "mln_trace.h"
```



### Function/Macro



#### mln_trace

```c
mln_trace(fmt, ...);
```

- Description: Send some data to the specified processing script. The parameters here are exactly the same as `fmt` and its variable parameters of the `mln_lang_ctx_pipe_send` function ([see this chapter for details](https://water-melon.github.io/Melon/en/melang.html)), Because the inside of this macro is to call the function to complete the message delivery.
- return value:
  - `0` - success
  - `-1` - failed



### Example

After installing Melon, we proceed as follows:

1. Enable trace mode configuration, edit `conf/melon.conf` under the installation path, and delete the comment (`//`) before `trace_mode`.

    > If you want to disable the trace mode, you only need to comment the configuration or change the configuration item to trace_mode off;.

2. Create a new file called `a.c`

    ```c
    #include <stdio.h>
    #include "mln_log.h"
    #include "mln_core.h"
    #include "mln_trace.h"

    int main(int argc, char *argv[])
    {
        struct mln_core_attr cattr;

        cattr.argc = argc;
        cattr.argv = argv;
        cattr.global_init = NULL;
        cattr.master_process = NULL;
        cattr.worker_process = NULL;

        if (mln_core_init(&cattr) < 0) {
           fprintf(stderr, "Melon init failed.\n");
           return -1;
        }

        while (1) {
            mln_trace("sir", "Hello", 2, 3.1);
            usleep(10000);
        }
        return 0;
    }
    ```



3. Compile the a.c file, and then execute the generated executable program, you can see the following output

    ```
    [Hello, 2, 3.100000, ]
    [Hello, 2, 3.100000, ]
    [Hello, 2, 3.100000, ]
    [Hello, 2, 3.100000, ]
    [Hello, 2, 3.100000, ]
    [Hello, 2, 3.100000, ]
    [Hello, 2, 3.100000, ]
    [Hello, 2, 3.100000, ]
    [Hello, 2, 3.100000, ]
    [Hello, 2, 3.100000, ]
    ...
    ```

