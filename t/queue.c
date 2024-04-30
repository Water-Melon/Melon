#include <stdio.h>
#include <stdlib.h>
#include "mln_queue.h"

int main(int argc, char *argv[])
{
    int i = 10;
    mln_queue_t *q;

    q = mln_queue_init(10, NULL);
    if (q == NULL) {
        fprintf(stderr, "queue init failed.\n");
        return -1;
    }
    mln_queue_append(q, &i);
    printf("%d\n", *(int *)mln_queue_get(q));
    mln_queue_destroy(q);

    return 0;
}

