#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mln_alloc.h"

int main(int argc, char *argv[])
{
    char *p;
    mln_alloc_t *pool;

    pool = mln_alloc_init(NULL);
    if (pool == NULL) {
        fprintf(stderr, "pool init failed\n");
        return -1;
    }

    p = (char *)mln_alloc_m(pool, 6);
    if (p == NULL) {
        fprintf(stderr, "alloc failed\n");
        return -1;
    }

    memcpy(p, "hello", 5);
    p[5] = 0;
    printf("%s\n", p);

    mln_alloc_free(p);
    mln_alloc_destroy(pool);

    return 0;
}
