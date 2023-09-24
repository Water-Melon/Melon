## Cron format parser



### Header file

```c
#include "mln_cron.h"
```



### Module

`cron`



### Functions

#### mln_cron_parse

```c
time_t mln_cron_parse(mln_string_t *exp, time_t base);
```

Description: Calculate the next available timestamp based on the given cron expression `exp` and the base timestamp `base`. Currently, the range operator `-` is not supported.

Return value: timestamp of time_t type, or 0 if there is an error.



### Example

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

