## 探针模式

探针模式用于收集使用Melon库的程序信息。大致过程如下：

1. 在程序中使用指定函数设置跟踪点
2. 开启跟踪模式配置，并设置好接收跟踪信息的Melang脚本文件路径
3. 在接收跟踪信息的Melang脚本中调用`pipe`函数接收探针传来的信息

目前，Melon中给出了一个处理跟踪信息的Melang脚本示例，在源码目录的`trace/trace.m`。

与ebpf不同，探针旨在收集应用相关的信息，即由开发人员指定的（业务或功能）信息。ebpf可以收集大量程序及内核信息，但无法针对与业务信息进行收集。因此，Melon中的trace模式就是为了补全这部分内容。



### 头文件

```c
#include "mln_trace.h"
```



### 函数/宏



#### mln_trace

```c
mln_trace(fmt, ...);
```

描述：发送若干数据给指定的处理脚本。这里的参数，与`mln_lang_ctx_pipe_send`函数([详见此章节](https://water-melon.github.io/Melon/cn/melang.html))的`fmt`及其可变参数的内容完全一致，因为本宏内部就是调用该函数完成的消息传递。

返回值：

- `0` - 成功
- `-1` - 失败



### 示例

安装Melon后，我们按如下步骤操作：

1. 开启trace模式配置，编辑安装路径下的`conf/melon.conf`，将`trace_mode`前的注释（`//`）删掉。

   > 如果想要禁用trace模式，只需要将配置注释或者将配置项改为 trace_mode off;即可。

2. 新建一个名为`a.c`的文件

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



3. 编译a.c文件，然后执行生成的可执行程序，可看到如下输出

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

