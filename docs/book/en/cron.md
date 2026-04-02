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

Description: Calculate the next available timestamp based on the given cron expression `exp` and the base timestamp `base`.

Supported cron syntax:

- `*` wildcard, matches all values
- `N` specific numeric value
- `N,N,N` comma-separated value list
- `N-N` range (supports wrap-around, e.g. `22-3` in the hour field means 22,23,0,1,2,3)
- `*/N` step
- `N-N/N` range combined with step
- Month names: `JAN`, `FEB`, `MAR`, `APR`, `MAY`, `JUN`, `JUL`, `AUG`, `SEP`, `OCT`, `NOV`, `DEC` (case-insensitive)
- Weekday names: `SUN`, `MON`, `TUE`, `WED`, `THU`, `FRI`, `SAT` (case-insensitive)
- Predefined macros: `@yearly` (or `@annually`), `@monthly`, `@weekly`, `@daily` (or `@midnight`), `@hourly`

Return value: timestamp of time_t type, or 0 if there is an error.



### Example

```c
#include "mln_cron.h"
#include <stdio.h>

int main(void)
{
    /* Basic wildcard */
    char p[] = "* * * * *";
    mln_string_t s;
    mln_string_nset(&s, p, sizeof(p)-1);
    time_t now = time(NULL);
    time_t next = mln_cron_parse(&s, now);
    printf("%lu %lu %s\n", (unsigned long)now, (unsigned long)next, ctime(&next));

    /* Using month and weekday names */
    char p2[] = "0 9 * JAN-JUN MON-FRI";
    mln_string_nset(&s, p2, sizeof(p2)-1);
    next = mln_cron_parse(&s, now);
    printf("next: %lu %s\n", (unsigned long)next, ctime(&next));

    /* Using predefined macros */
    char p3[] = "@daily";
    mln_string_nset(&s, p3, sizeof(p3)-1);
    next = mln_cron_parse(&s, now);
    printf("daily: %lu %s\n", (unsigned long)next, ctime(&next));

    return 0;
}
```

