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

描述：根据给定的cron表达式`exp`和基准时间戳`base`，计算出下一次可用的时间戳。

支持的cron语法：

- `*` 通配符，匹配所有值
- `N` 具体数值
- `N,N,N` 逗号分隔的值列表
- `N-N` 范围（支持环绕，如小时字段`22-3`表示22,23,0,1,2,3）
- `*/N` 步进
- `N-N/N` 范围与步进的组合
- 月份名称：`JAN`, `FEB`, `MAR`, `APR`, `MAY`, `JUN`, `JUL`, `AUG`, `SEP`, `OCT`, `NOV`, `DEC`（不区分大小写）
- 星期名称：`SUN`, `MON`, `TUE`, `WED`, `THU`, `FRI`, `SAT`（不区分大小写）
- 预定义宏：`@yearly`（或`@annually`）, `@monthly`, `@weekly`, `@daily`（或`@midnight`）, `@hourly`

返回值：time_t类型时间戳，若出错则返回0。



### 示例

```c
#include "mln_cron.h"
#include <stdio.h>

int main(void)
{
    /* 基本通配符 */
    char p[] = "* * * * *";
    mln_string_t s;
    mln_string_nset(&s, p, sizeof(p)-1);
    time_t now = time(NULL);
    time_t next = mln_cron_parse(&s, now);
    printf("%lu %lu %s\n", (unsigned long)now, (unsigned long)next, ctime(&next));

    /* 使用月份和星期名称 */
    char p2[] = "0 9 * JAN-JUN MON-FRI";
    mln_string_nset(&s, p2, sizeof(p2)-1);
    next = mln_cron_parse(&s, now);
    printf("next: %lu %s\n", (unsigned long)next, ctime(&next));

    /* 使用预定义宏 */
    char p3[] = "@daily";
    mln_string_nset(&s, p3, sizeof(p3)-1);
    next = mln_cron_parse(&s, now);
    printf("daily: %lu %s\n", (unsigned long)next, ctime(&next));

    return 0;
}
```

