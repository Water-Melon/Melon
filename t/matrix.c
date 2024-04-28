#include <stdio.h>
#include <errno.h>
#include <string.h>
#include "mln_matrix.h"

int main(int argc, char *argv[])
{
    mln_matrix_t *a, *b;
    double data[] = {1, 1, 1, 1, 2, 4, 2, 8, 64};

    a = mln_matrix_new(3, 3, data, 1);
    if (a == NULL) {
        fprintf(stderr, "init matrix failed\n");
        return -1;
    }
    mln_matrix_dump(a);

    b = mln_matrix_inverse(a);
    mln_matrix_free(a);
    if (b == NULL) {
        fprintf(stderr, "inverse failed: %s\n", strerror(errno));
        return -1;
    }
    mln_matrix_dump(b);
    mln_matrix_free(b);

    return 0;
}

