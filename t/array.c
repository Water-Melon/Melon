#include <stdio.h>
#include "mln_array.h"

typedef struct {
    int i1;
    int i2;
} test_t;

int main(void)
{
    test_t *t;
    mln_size_t i, n;
    mln_array_t arr;

    mln_array_init(&arr, NULL, sizeof(test_t), 1);

    t = mln_array_push(&arr);
    if (t == NULL)
        return -1;
    t->i1 = 0;

    t = mln_array_pushn(&arr, 9);
    for (i = 0; i < 9; ++i) {
        t[i].i1 = i + 1;
    }

    for (t = mln_array_elts(&arr), i = 0; i < mln_array_nelts(&arr); ++i) {
        printf("%d\n", t[i].i1);
    }

    mln_array_destroy(&arr);

    return 0;
}

