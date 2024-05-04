
/*
 * Copyright (C) Niklaus F.Schen.
 */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <math.h>
#include "mln_matrix.h"
#include "mln_func.h"

MLN_FUNC(, mln_matrix_t *, mln_matrix_new, \
         (mln_size_t row, mln_size_t col, double *data, mln_u32_t is_ref), \
         (row, col, data, is_ref), \
{
    mln_matrix_t *matrix;
    if ((matrix = (mln_matrix_t *)malloc(sizeof(mln_matrix_t))) == NULL) {
        return NULL;
    }
    matrix->row = row;
    matrix->col = col;
    matrix->data = data;
    matrix->is_ref = is_ref;
    return matrix;
})

MLN_FUNC_VOID(, void, mln_matrix_free, (mln_matrix_t *matrix), (matrix), {
    if (matrix == NULL) return;
    if (matrix->data != NULL && !matrix->is_ref)
        free(matrix->data);
    free(matrix);
})

MLN_FUNC(, mln_matrix_t *, mln_matrix_mul, (mln_matrix_t *m1, mln_matrix_t *m2), (m1, m2), {
    if (m1->col != m2->row) {
        errno = EINVAL;
        return NULL;
    }
    double *data, tmp;
    mln_size_t i, j, k;
    mln_matrix_t *ret;
    mln_size_t m1row = m1->row, m1col = m1->col, m2col = m2->col;
    double *m1data = m1->data, *m2data = m2->data;

    if ((data = (double *)calloc(m1row, m2col*sizeof(double))) == NULL) {
        errno = ENOMEM;
        return NULL;
    }
    if ((ret = mln_matrix_new(m1row, m2col, data, 0)) == NULL) {
        free(data);
        errno = ENOMEM;
        return NULL;
    }

    for (i = 0; i < m1row; ++i) {
        for (k = 0; k < m1col; ++k) {
            tmp = m1data[i*m1col+k];
            for (j = 0; j < m2col; ++j) {
                data[i*m2col+j] += (tmp * m2data[k*m2col+j]);
            }
        }
    }

    return ret;
})

MLN_FUNC(, mln_matrix_t *, mln_matrix_inverse, (mln_matrix_t *matrix), (matrix), {
    if (matrix == NULL || matrix->row != matrix->col) {
        errno = EINVAL;
        return NULL;
    }
    mln_matrix_t *ret;
    double *data, *origin = matrix->data, tmp;
    mln_size_t i, j, k, m;
    mln_size_t n = matrix->row * matrix->col;
    mln_size_t len = matrix->row;

    if ((data = (double *)malloc(n*sizeof(double))) == NULL) {
        errno = ENOMEM;
        return NULL;
    }
    if ((ret = mln_matrix_new(matrix->row, matrix->col, data, 0)) == NULL) {
        free(data);
        errno = ENOMEM;
        return NULL;
    }

    for (i = 0, k = 0; i < n; i += ret->col, k++) {
        for (j = 0; j < ret->col; ++j) {
            data[i + j] = j == k? 1: 0;
        }
    }

    for (m = 0, i = 0; i < n; i += len, m++) {
        tmp = origin[i + m];
        k = i;
        for (j = i + len; j < n; j += len) {
            if (fabs(origin[j + m]) > fabs(tmp)) {
                tmp = origin[j + m];
                k = j;
            }
        }

        if (k != i) {
            for (j = 0; j < len; ++j) {
                tmp = origin[i + j];
                origin[i + j] = origin[k + j];
                origin[k + j] = tmp;
                tmp = data[i + j];
                data[i + j] = data[k + j];
                data[k + j] = tmp;
            }
        }
        if (fabs(origin[i + m]) < 1e-6) {/*is zero*/
            mln_matrix_free(ret);
            errno = EINVAL;
            return NULL;
        }

        tmp = origin[i + m];
        for (j = 0; j < len; ++j) {
            origin[i + j] /= tmp;
            data[i + j] /= tmp;
        }
        for (j = 0; j < n; j += len) {
            if (j != i) {
                tmp = origin[j + m];
                for (k = 0; k < len; ++k) {
                    origin[j + k] -= (origin[i + k] * tmp);
                    data[j + k] -= (data[i + k] * tmp);
                }
            }
        }
    }

    return ret;
})

void mln_matrix_dump(mln_matrix_t *matrix)
{
    if (matrix == NULL) return;
    mln_size_t i, sum = matrix->row * matrix->col;
#if defined(i386) || defined(__arm__) || defined(MSVC)
    printf("Matrix row:%u col:%u\n ", matrix->row, matrix->col);
#else
    printf("Matrix row:%lu col:%lu\n ", matrix->row, matrix->col);
#endif
    for (i = 0; i < sum; ++i) {
        if (i && !(i % matrix->col)) {
            printf("\n ");
        }
        printf("%f ", matrix->data[i]);
    }
    printf("\n");
}

