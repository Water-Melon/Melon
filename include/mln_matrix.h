
/*
 * Copyright (C) Niklaus F.Schen.
 */
#ifndef __MLN_MATRIX_H
#define __MLN_MATRIX_H

#include "mln_types.h"

typedef struct {
    mln_size_t  row;
    mln_size_t  col;
    double     *data;
    mln_u32_t   is_ref:1;
} mln_matrix_t;

extern mln_matrix_t *
mln_matrix_new(mln_size_t row, mln_size_t col, double *data, mln_u32_t is_ref) __NONNULL1(3);
extern void mln_matrix_free(mln_matrix_t *matrix);
extern mln_matrix_t *
mln_matrix_mul(mln_matrix_t *m1, mln_matrix_t *m2) __NONNULL2(1,2);
extern mln_matrix_t *mln_matrix_inverse(mln_matrix_t *matrix);
extern void mln_matrix_dump(mln_matrix_t *matrix);
#endif

