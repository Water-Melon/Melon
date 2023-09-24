## Matrix



### Header file

```c
#include "mln_matrix.h"
```



### Module

`matrix`



### Structure

```c
typedef struct {                 
    mln_size_t  row;//the number of rows in the matrix
    mln_size_t  col;//the number of columns in the matrix
    double     *data;//A one-dimensional array containing all the elements in the matrix, arranged row by row
    mln_u32_t   is_ref:1;//Identifies whether data is an external reference, this flag is used to ignore the release of data when releasing the matrix structure
} mln_matrix_t;
```



### Functions



#### mln_matrix_new

```c
mln_matrix_t *mln_matrix_new(mln_size_t row, mln_size_t col, double *data, mln_u32_t is_ref);
```

Description: Create a matrix with `row` rows and `col` columns, and the data is `data`. If `is_ref` is `0`, it means that the matrix structure will completely copy a `data` in it, otherwise directly refer to `data`.

Return value: Returns the matrix structure pointer if successful, otherwise returns `NULL`



#### mln_matrix_free

```c
void mln_matrix_free(mln_matrix_t *matrix);
```

Description: Free the matrix structure memory.

Return value: none



#### mln_matrix_mul

```c
mln_matrix_t *mln_matrix_mul(mln_matrix_t *m1, mln_matrix_t *m2);
```

Description: Matrix multiplication.

Return value: Returns the result matrix pointer if successful, otherwise returns `NULL`



#### mln_matrix_inverse

```c
mln_matrix_t *mln_matrix_inverse(mln_matrix_t *matrix);
```

Description: Matrix inversion. **Note**: Matrix inversion requires that the matrix is a square matrix.

Return value: Returns the result matrix pointer if successful, otherwise returns `NULL`



#### mln_matrix_dump

```c
void mln_matrix_dump(mln_matrix_t *matrix);
```

Description: Print information about the matrix to standard output. For debugging only.

Return value: none



### Example

```c
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
```

