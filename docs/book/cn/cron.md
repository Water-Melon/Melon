## cron格式解析器




### 头文件

```c
#include "mln_cron.h"
```



### 模块名

`cron`



### 函数

#### mln_cron_parse

```c
time_t mln_cron_parse(mln_string_t *exp, time_t base);
```

描述：根据给定的cron表达式`exp`和基准时间戳`base`，计算出下一次可用的时间戳。目前，暂不支持范围操作符`-`。

返回值：time_t类型时间戳，若出错则返回0。



### 示例

```c
#include "mln_cron.h"
#include <stdio.h>

int main(void)
{
    char p[] = "* * * * *";
    mln_string_t s;
    mln_string_nset(&s, p, sizeof(p)-1);
    time_t now = time(NULL);
    time_t next = mln_cron_parse(&s, now);
    printf("%lu %lu %s\n", (unsigned long)now, (unsigned long)next, ctime(&next));
    return 0;
}
```

