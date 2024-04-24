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

