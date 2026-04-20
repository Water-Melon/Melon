
/*
 * Copyright (C) Niklaus F.Schen.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

/*
 * Optimized matrix multiplication:
 *   1) Transpose B so both A-row and Bt-row scans are sequential (cache-friendly).
 *   2) Process 4 rows of A per Bt-row to reuse Bt data across 4 dot products.
 *   3) 4-wide unrolled accumulation with 4 independent sums per dot product.
 *   4) For small K, fallback to ikj with unrolling to avoid transpose overhead.
 */
#define MLN_MATRIX_TRANSPOSE_THRESHOLD 16

MLN_FUNC(static, void, mln_matrix_transpose_buf, \
         (double *dst, const double *src, mln_size_t rows, mln_size_t cols), \
         (dst, src, rows, cols), \
{
    mln_size_t i, j;
    for (i = 0; i < rows; ++i) {
        const double *srow = src + i * cols;
        for (j = 0; j < cols; ++j) {
            dst[j * rows + i] = srow[j];
        }
    }
})

MLN_FUNC(, mln_matrix_t *, mln_matrix_mul, (mln_matrix_t *m1, mln_matrix_t *m2), (m1, m2), {
    if (m1->col != m2->row) {
        errno = EINVAL;
        return NULL;
    }
    double *data;
    mln_matrix_t *ret;
    mln_size_t m1row = m1->row, K = m1->col, m2col = m2->col;
    double *m1data = m1->data, *m2data = m2->data;
    mln_size_t i, j, k;

    if ((data = (double *)calloc(m1row * m2col, sizeof(double))) == NULL) {
        errno = ENOMEM;
        return NULL;
    }
    if ((ret = mln_matrix_new(m1row, m2col, data, 0)) == NULL) {
        free(data);
        errno = ENOMEM;
        return NULL;
    }

    if (K >= MLN_MATRIX_TRANSPOSE_THRESHOLD) {
        /* Transpose B for sequential access on both operands */
        double *bt = (double *)malloc(K * m2col * sizeof(double));
        if (bt == NULL) {
            mln_matrix_free(ret);
            errno = ENOMEM;
            return NULL;
        }
        mln_matrix_transpose_buf(bt, m2data, m2->row, m2col);

        /* Process 4 rows of A at a time, reusing each Bt row across all 4 */
        i = 0;
        for (; i + 3 < m1row; i += 4) {
            const double *arow0 = m1data + i * K;
            const double *arow1 = m1data + (i + 1) * K;
            const double *arow2 = m1data + (i + 2) * K;
            const double *arow3 = m1data + (i + 3) * K;
            double *crow0 = data + i * m2col;
            double *crow1 = data + (i + 1) * m2col;
            double *crow2 = data + (i + 2) * m2col;
            double *crow3 = data + (i + 3) * m2col;
            for (j = 0; j < m2col; ++j) {
                const double *brow = bt + j * K;
                double s0 = 0, s1 = 0, s2 = 0, s3 = 0;
                k = 0;
                for (; k + 3 < K; k += 4) {
                    double b0 = brow[k], b1 = brow[k+1], b2 = brow[k+2], b3 = brow[k+3];
                    s0 += arow0[k]*b0 + arow0[k+1]*b1 + arow0[k+2]*b2 + arow0[k+3]*b3;
                    s1 += arow1[k]*b0 + arow1[k+1]*b1 + arow1[k+2]*b2 + arow1[k+3]*b3;
                    s2 += arow2[k]*b0 + arow2[k+1]*b1 + arow2[k+2]*b2 + arow2[k+3]*b3;
                    s3 += arow3[k]*b0 + arow3[k+1]*b1 + arow3[k+2]*b2 + arow3[k+3]*b3;
                }
                for (; k < K; ++k) {
                    double bk = brow[k];
                    s0 += arow0[k] * bk;
                    s1 += arow1[k] * bk;
                    s2 += arow2[k] * bk;
                    s3 += arow3[k] * bk;
                }
                crow0[j] = s0;
                crow1[j] = s1;
                crow2[j] = s2;
                crow3[j] = s3;
            }
        }
        /* Handle remaining rows */
        for (; i < m1row; ++i) {
            const double *arow = m1data + i * K;
            double *crow = data + i * m2col;
            for (j = 0; j < m2col; ++j) {
                const double *brow = bt + j * K;
                double s0 = 0, s1 = 0, s2 = 0, s3 = 0;
                k = 0;
                for (; k + 3 < K; k += 4) {
                    s0 += arow[k]   * brow[k];
                    s1 += arow[k+1] * brow[k+1];
                    s2 += arow[k+2] * brow[k+2];
                    s3 += arow[k+3] * brow[k+3];
                }
                for (; k < K; ++k) {
                    s0 += arow[k] * brow[k];
                }
                crow[j] = s0 + s1 + s2 + s3;
            }
        }
        free(bt);
    } else {
        /* Small K: ikj with unrolling, no transpose overhead */
        for (i = 0; i < m1row; ++i) {
            double *crow = data + i * m2col;
            for (k = 0; k < K; ++k) {
                double a_ik = m1data[i * K + k];
                const double *m2row = m2data + k * m2col;
                j = 0;
                for (; j + 3 < m2col; j += 4) {
                    crow[j]     += a_ik * m2row[j];
                    crow[j + 1] += a_ik * m2row[j + 1];
                    crow[j + 2] += a_ik * m2row[j + 2];
                    crow[j + 3] += a_ik * m2row[j + 3];
                }
                for (; j < m2col; ++j) {
                    crow[j] += a_ik * m2row[j];
                }
            }
        }
    }

    return ret;
})

/*
 * Optimized matrix inverse using Gauss-Jordan elimination:
 *   1) Work on a copy of the input (non-destructive).
 *   2) Pointer arithmetic with row pointers instead of index multiplication.
 *   3) Multiply by reciprocal instead of dividing in a loop.
 *   4) Skip near-zero rows during elimination.
 *   5) Exploit triangular structure: only update origin columns >= m during elimination.
 *   6) 4-wide loop unrolling for all row operations.
 */
MLN_FUNC(, mln_matrix_t *, mln_matrix_inverse, (mln_matrix_t *matrix), (matrix), {
    if (matrix == NULL || matrix->row != matrix->col) {
        errno = EINVAL;
        return NULL;
    }
    mln_matrix_t *ret;
    mln_size_t len = matrix->row;
    mln_size_t n = len * len;
    double *data, *origin;
    double tmp, inv_pivot;
    mln_size_t i, j, m, best, p;
    double *ori_row_i, *dat_row_i, *ori_row_j, *dat_row_j;
    double *ori_row_best;

    if ((origin = (double *)malloc(n * sizeof(double))) == NULL) {
        errno = ENOMEM;
        return NULL;
    }
    memcpy(origin, matrix->data, n * sizeof(double));

    if ((data = (double *)malloc(n * sizeof(double))) == NULL) {
        free(origin);
        errno = ENOMEM;
        return NULL;
    }
    if ((ret = mln_matrix_new(len, len, data, 0)) == NULL) {
        free(origin);
        free(data);
        errno = ENOMEM;
        return NULL;
    }

    /* Initialize identity matrix */
    memset(data, 0, n * sizeof(double));
    for (i = 0; i < len; ++i)
        data[i * len + i] = 1.0;

    for (m = 0; m < len; ++m) {
        ori_row_i = origin + m * len;

        /* Partial pivoting */
        best = m;
        tmp = fabs(ori_row_i[m]);
        ori_row_j = ori_row_i + len;
        for (j = m + 1; j < len; ++j, ori_row_j += len) {
            if (fabs(ori_row_j[m]) > tmp) {
                tmp = fabs(ori_row_j[m]);
                best = j;
            }
        }

        if (best != m) {
            ori_row_best = origin + best * len;
            dat_row_i = data + m * len;
            dat_row_j = data + best * len;
            /* Swap origin rows: only columns m..len-1 matter (earlier are structurally 0 or handled) */
            p = m;
            for (; p + 3 < len; p += 4) {
                tmp = ori_row_i[p]; ori_row_i[p] = ori_row_best[p]; ori_row_best[p] = tmp;
                tmp = ori_row_i[p+1]; ori_row_i[p+1] = ori_row_best[p+1]; ori_row_best[p+1] = tmp;
                tmp = ori_row_i[p+2]; ori_row_i[p+2] = ori_row_best[p+2]; ori_row_best[p+2] = tmp;
                tmp = ori_row_i[p+3]; ori_row_i[p+3] = ori_row_best[p+3]; ori_row_best[p+3] = tmp;
            }
            for (; p < len; ++p) {
                tmp = ori_row_i[p]; ori_row_i[p] = ori_row_best[p]; ori_row_best[p] = tmp;
            }
            /* Swap data rows: full width */
            p = 0;
            for (; p + 3 < len; p += 4) {
                tmp = dat_row_i[p]; dat_row_i[p] = dat_row_j[p]; dat_row_j[p] = tmp;
                tmp = dat_row_i[p+1]; dat_row_i[p+1] = dat_row_j[p+1]; dat_row_j[p+1] = tmp;
                tmp = dat_row_i[p+2]; dat_row_i[p+2] = dat_row_j[p+2]; dat_row_j[p+2] = tmp;
                tmp = dat_row_i[p+3]; dat_row_i[p+3] = dat_row_j[p+3]; dat_row_j[p+3] = tmp;
            }
            for (; p < len; ++p) {
                tmp = dat_row_i[p]; dat_row_i[p] = dat_row_j[p]; dat_row_j[p] = tmp;
            }
        }

        if (fabs(ori_row_i[m]) < 1e-6) {
            free(origin);
            mln_matrix_free(ret);
            errno = EINVAL;
            return NULL;
        }

        /* Scale pivot row: multiply by reciprocal */
        inv_pivot = 1.0 / ori_row_i[m];
        dat_row_i = data + m * len;
        /* Origin: only columns m..len-1 need scaling */
        ori_row_i[m] = 1.0; /* exact */
        p = m + 1;
        for (; p + 3 < len; p += 4) {
            ori_row_i[p]   *= inv_pivot;
            ori_row_i[p+1] *= inv_pivot;
            ori_row_i[p+2] *= inv_pivot;
            ori_row_i[p+3] *= inv_pivot;
        }
        for (; p < len; ++p)
            ori_row_i[p] *= inv_pivot;
        /* Data: full width */
        p = 0;
        for (; p + 3 < len; p += 4) {
            dat_row_i[p]   *= inv_pivot;
            dat_row_i[p+1] *= inv_pivot;
            dat_row_i[p+2] *= inv_pivot;
            dat_row_i[p+3] *= inv_pivot;
        }
        for (; p < len; ++p)
            dat_row_i[p] *= inv_pivot;

        /* Eliminate column m from all other rows */
        ori_row_j = origin;
        dat_row_j = data;
        for (j = 0; j < len; ++j, ori_row_j += len, dat_row_j += len) {
            if (j == m) continue;
            tmp = ori_row_j[m];
            if (fabs(tmp) < 1e-15) continue;
            ori_row_j[m] = 0.0; /* exact */
            /* Origin: only columns m+1..len-1 (earlier columns are 0 in pivot row) */
            p = m + 1;
            for (; p + 3 < len; p += 4) {
                ori_row_j[p]   -= ori_row_i[p]   * tmp;
                ori_row_j[p+1] -= ori_row_i[p+1] * tmp;
                ori_row_j[p+2] -= ori_row_i[p+2] * tmp;
                ori_row_j[p+3] -= ori_row_i[p+3] * tmp;
            }
            for (; p < len; ++p)
                ori_row_j[p] -= ori_row_i[p] * tmp;
            /* Data: full width */
            p = 0;
            for (; p + 3 < len; p += 4) {
                dat_row_j[p]   -= dat_row_i[p]   * tmp;
                dat_row_j[p+1] -= dat_row_i[p+1] * tmp;
                dat_row_j[p+2] -= dat_row_i[p+2] * tmp;
                dat_row_j[p+3] -= dat_row_i[p+3] * tmp;
            }
            for (; p < len; ++p)
                dat_row_j[p] -= dat_row_i[p] * tmp;
        }
    }

    free(origin);
    return ret;
})

MLN_FUNC(, mln_matrix_t *, mln_matrix_add, (mln_matrix_t *m1, mln_matrix_t *m2), (m1, m2), {
    if (m1->row != m2->row || m1->col != m2->col) {
        errno = EINVAL;
        return NULL;
    }
    mln_size_t total = m1->row * m1->col;
    double *data, *d1 = m1->data, *d2 = m2->data;
    mln_matrix_t *ret;
    mln_size_t i;

    if ((data = (double *)malloc(total * sizeof(double))) == NULL) {
        errno = ENOMEM;
        return NULL;
    }
    if ((ret = mln_matrix_new(m1->row, m1->col, data, 0)) == NULL) {
        free(data);
        errno = ENOMEM;
        return NULL;
    }

    i = 0;
    for (; i + 3 < total; i += 4) {
        data[i]     = d1[i]     + d2[i];
        data[i + 1] = d1[i + 1] + d2[i + 1];
        data[i + 2] = d1[i + 2] + d2[i + 2];
        data[i + 3] = d1[i + 3] + d2[i + 3];
    }
    for (; i < total; ++i)
        data[i] = d1[i] + d2[i];

    return ret;
})

MLN_FUNC(, mln_matrix_t *, mln_matrix_sub, (mln_matrix_t *m1, mln_matrix_t *m2), (m1, m2), {
    if (m1->row != m2->row || m1->col != m2->col) {
        errno = EINVAL;
        return NULL;
    }
    mln_size_t total = m1->row * m1->col;
    double *data, *d1 = m1->data, *d2 = m2->data;
    mln_matrix_t *ret;
    mln_size_t i;

    if ((data = (double *)malloc(total * sizeof(double))) == NULL) {
        errno = ENOMEM;
        return NULL;
    }
    if ((ret = mln_matrix_new(m1->row, m1->col, data, 0)) == NULL) {
        free(data);
        errno = ENOMEM;
        return NULL;
    }

    i = 0;
    for (; i + 3 < total; i += 4) {
        data[i]     = d1[i]     - d2[i];
        data[i + 1] = d1[i + 1] - d2[i + 1];
        data[i + 2] = d1[i + 2] - d2[i + 2];
        data[i + 3] = d1[i + 3] - d2[i + 3];
    }
    for (; i < total; ++i)
        data[i] = d1[i] - d2[i];

    return ret;
})

MLN_FUNC(, mln_matrix_t *, mln_matrix_scalar_mul, (double scalar, mln_matrix_t *matrix), (scalar, matrix), {
    mln_size_t total = matrix->row * matrix->col;
    double *data, *src = matrix->data;
    mln_matrix_t *ret;
    mln_size_t i;

    if ((data = (double *)malloc(total * sizeof(double))) == NULL) {
        errno = ENOMEM;
        return NULL;
    }
    if ((ret = mln_matrix_new(matrix->row, matrix->col, data, 0)) == NULL) {
        free(data);
        errno = ENOMEM;
        return NULL;
    }

    i = 0;
    for (; i + 3 < total; i += 4) {
        data[i]     = scalar * src[i];
        data[i + 1] = scalar * src[i + 1];
        data[i + 2] = scalar * src[i + 2];
        data[i + 3] = scalar * src[i + 3];
    }
    for (; i < total; ++i)
        data[i] = scalar * src[i];

    return ret;
})

MLN_FUNC(, mln_matrix_t *, mln_matrix_transpose, (mln_matrix_t *matrix), (matrix), {
    mln_size_t row = matrix->row, col = matrix->col;
    mln_size_t total = row * col;
    double *data, *src = matrix->data;
    mln_matrix_t *ret;
    mln_size_t i, j;

    if ((data = (double *)malloc(total * sizeof(double))) == NULL) {
        errno = ENOMEM;
        return NULL;
    }
    if ((ret = mln_matrix_new(col, row, data, 0)) == NULL) {
        free(data);
        errno = ENOMEM;
        return NULL;
    }

    for (i = 0; i < row; ++i) {
        for (j = 0; j < col; ++j) {
            data[j * row + i] = src[i * col + j];
        }
    }

    return ret;
})

/*
 * Determinant via LU decomposition with partial pivoting.
 * Uses a working copy to avoid modifying the input.
 */
MLN_FUNC(, double, mln_matrix_det, (mln_matrix_t *matrix), (matrix), {
    if (matrix == NULL || matrix->row != matrix->col) {
        errno = EINVAL;
        return 0.0;
    }
    mln_size_t len = matrix->row;
    mln_size_t n = len * len;
    double *work;
    double det = 1.0, tmp;
    mln_size_t i, j, best;
    double *row_i, *row_j;

    if ((work = (double *)malloc(n * sizeof(double))) == NULL) {
        errno = ENOMEM;
        return 0.0;
    }
    memcpy(work, matrix->data, n * sizeof(double));

    for (i = 0; i < len; ++i) {
        row_i = work + i * len;

        /* Partial pivoting */
        best = i;
        tmp = fabs(row_i[i]);
        for (j = i + 1; j < len; ++j) {
            row_j = work + j * len;
            if (fabs(row_j[i]) > tmp) {
                tmp = fabs(row_j[i]);
                best = j;
            }
        }
        if (best != i) {
            row_j = work + best * len;
            for (j = 0; j < len; ++j) {
                tmp = row_i[j]; row_i[j] = row_j[j]; row_j[j] = tmp;
            }
            det = -det;
        }

        if (fabs(row_i[i]) < 1e-15) {
            free(work);
            return 0.0;
        }

        det *= row_i[i];

        /* Eliminate below */
        for (j = i + 1; j < len; ++j) {
            mln_size_t p;
            row_j = work + j * len;
            tmp = row_j[i] / row_i[i];
            p = i + 1;
            for (; p + 3 < len; p += 4) {
                row_j[p]   -= row_i[p]   * tmp;
                row_j[p+1] -= row_i[p+1] * tmp;
                row_j[p+2] -= row_i[p+2] * tmp;
                row_j[p+3] -= row_i[p+3] * tmp;
            }
            for (; p < len; ++p) {
                row_j[p] -= row_i[p] * tmp;
            }
        }
    }

    free(work);
    return det;
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

