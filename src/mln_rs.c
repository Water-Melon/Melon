
/*
 * Copyright (C) Niklaus F.Schen.
 * Reed-Solomon Code.
 */
#include "mln_rs.h"
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include "mln_func.h"

/*GF operations*/
static mln_u8_t mln_rs_gfilog[] = {
  1,   2,   4,   8,  16,  32,  64, 128,
 45,  90, 180,  69, 138,  57, 114, 228,
229, 231, 227, 235, 251, 219, 155,  27,
 54, 108, 216, 157,  23,  46,  92, 184,
 93, 186,  89, 178,  73, 146,   9,  18,
 36,  72, 144,  13,  26,  52, 104, 208,
141,  55, 110, 220, 149,   7,  14,  28,
 56, 112, 224, 237, 247, 195, 171, 123,
246, 193, 175, 115, 230, 225, 239, 243,
203, 187,  91, 182,  65, 130,  41,  82,
164, 101, 202, 185,  95, 190,  81, 162,
105, 210, 137,  63, 126, 252, 213, 135,
 35,  70, 140,  53, 106, 212, 133,  39,
 78, 156,  21,  42,  84, 168, 125, 250,
217, 159,  19,  38,  76, 152,  29,  58,
116, 232, 253, 215, 131,  43,  86, 172,
117, 234, 249, 223, 147,  11,  22,  44,
 88, 176,  77, 154,  25,  50, 100, 200,
189,  87, 174, 113, 226, 233, 255, 211,
139,  59, 118, 236, 245, 199, 163, 107,
214, 129,  47,  94, 188,  85, 170, 121,
242, 201, 191,  83, 166,  97, 194, 169,
127, 254, 209, 143,  51, 102, 204, 181,
 71, 142,  49,  98, 196, 165, 103, 206,
177,  79, 158,  17,  34,  68, 136,  61,
122, 244, 197, 167,  99, 198, 161, 111,
222, 145,  15,  30,  60, 120, 240, 205,
183,  67, 134,  33,  66, 132,  37,  74,
148,   5,  10,  20,  40,  80, 160, 109,
218, 153,  31,  62, 124, 248, 221, 151,
  3,   6,  12,  24,  48,  96, 192, 173,
119, 238, 241, 207, 179,  75, 150,   0
};

static mln_u8_t mln_rs_gflog[] = {
255,   0,   1, 240,   2, 225, 241,  53,
  3,  38, 226, 133, 242,  43,  54, 210,
  4, 195,  39, 114, 227, 106, 134,  28,
243, 140,  44,  23,  55, 118, 211, 234,
  5, 219, 196,  96,  40, 222, 115, 103,
228,  78, 107, 125, 135,   8,  29, 162,
244, 186, 141, 180,  45,  99,  24,  49,
 56,  13, 119, 153, 212, 199, 235,  91,
  6,  76, 220, 217, 197,  11,  97, 184,
 41,  36, 223, 253, 116, 138, 104, 193,
229,  86,  79, 171, 108, 165, 126, 145,
136,  34,   9,  74,  30,  32, 163,  84,
245, 173, 187, 204, 142,  81, 181, 190,
 46,  88, 100, 159,  25, 231,  50, 207,
 57, 147,  14,  67, 120, 128, 154, 248,
213, 167, 200,  63, 236, 110,  92, 176,
  7, 161,  77, 124, 221, 102, 218,  95,
198,  90,  12, 152,  98,  48, 185, 179,
 42, 209,  37, 132, 224,  52, 254, 239,
117, 233, 139,  22, 105,  27, 194, 113,
230, 206,  87, 158,  80, 189, 172, 203,
109, 175, 166,  62, 127, 247, 146,  66,
137, 192,  35, 252,  10, 183,  75, 216,
 31,  83,  33,  73, 164, 144,  85, 170,
246,  65, 174,  61, 188, 202, 205, 157,
143, 169,  82,  72, 182, 215, 191, 251,
 47, 178,  89, 151, 101,  94, 160, 123,
 26, 112, 232,  21,  51, 238, 208, 131,
 58,  69, 148,  18,  15,  16,  68,  17,
121, 149, 129,  19, 155,  59, 249,  70,
214, 250, 168,  71, 201, 156,  64,  60,
237, 130, 111,  20,  93, 122, 177, 150
};

#define M_RS_GF_ADDSUB(dst,src) ((dst) ^= (src))
#define M_RS_GF_MUL(dst,src) \
  ((dst) = (((dst) == 0 || (src) == 0)? \
             0: \
             mln_rs_gfilog[(mln_rs_gflog[(dst)]+mln_rs_gflog[(src)])%255]))
#define M_RS_GF_DIV(dst,src) \
  ((dst) = (((dst) == 0 || (src) == 0)? \
            0: \
            mln_rs_gfilog[ (mln_rs_gflog[(dst)]<mln_rs_gflog[(src)]? \
                             255-(mln_rs_gflog[(src)]-mln_rs_gflog[(dst)]): \
                             mln_rs_gflog[(dst)]-mln_rs_gflog[(src)]) ]))

MLN_FUNC(static inline, mln_size_t, mln_rs_power_calc, (mln_size_t base, mln_size_t exp), (base, exp), {
    mln_s32_t i;
    mln_size_t save = base;
    for (i = (sizeof(base)<<3)-1; i >= 0; --i) {
        if (exp & ((mln_size_t)1 << i)) break;
    }
    if (i-- < 0) return 1;

    for (; i >= 0; --i) {
        base *= base;
        if (exp & ((mln_size_t)1 << i)) {
            base *= save;
        }
    }
    return base;
})

/*matrix*/
MLN_FUNC(static, mln_rs_matrix_t *, mln_rs_matrix_new, \
         (mln_size_t row, mln_size_t col, mln_u8ptr_t data, mln_u32_t is_ref), \
         (row, col, data, is_ref), \
{
    mln_rs_matrix_t *rm;
    if ((rm = (mln_rs_matrix_t *)malloc(sizeof(mln_rs_matrix_t))) == NULL) {
        return NULL;
    }
    rm->row = row;
    rm->col = col;
    rm->data = data;
    rm->is_ref = is_ref;
    return rm;
})

MLN_FUNC_VOID(static, void, mln_rs_matrix_free, (mln_rs_matrix_t *matrix), (matrix), {
    if (matrix == NULL) return;
    if (matrix->data != NULL && !matrix->is_ref)
        free(matrix->data);
    free(matrix);
})

MLN_FUNC(static inline, mln_rs_matrix_t *, mln_rs_matrix_mul, \
         (mln_rs_matrix_t *m1, mln_rs_matrix_t *m2), (m1, m2), \
{
    if (m1->col != m2->row) {
        errno = EINVAL;
        return NULL;
    }
    mln_u8_t dst, src, tmp;
    mln_u8ptr_t data;
    mln_size_t i, j, k;
    mln_rs_matrix_t *ret;
    mln_size_t m1row = m1->row, m1col = m1->col, m2col = m2->col;
    mln_u8ptr_t m1data = m1->data, m2data = m2->data;

    if ((data = (mln_u8ptr_t)calloc(m1row, m2col)) == NULL) {
        errno = ENOMEM;
        return NULL;
    }
    if ((ret = mln_rs_matrix_new(m1row, m2col, data, 0)) == NULL) {
        free(data);
        errno = ENOMEM;
        return NULL;
    }

    for (i = 0; i < m1row; ++i) {
        for (k = 0; k < m1col; ++k) {
            tmp = m1data[i*m1col+k];
            for (j = 0; j < m2col; ++j) {
                dst = tmp;
                src = m2data[k*m2col+j];
                M_RS_GF_MUL(dst, src);
                M_RS_GF_ADDSUB(data[i*m2col+j], dst);
            }
        }
    }

    return ret;
})

MLN_FUNC(static, mln_rs_matrix_t *, mln_rs_matrix_inverse, \
         (mln_rs_matrix_t *matrix), (matrix), \
{
    if (matrix == NULL || matrix->row != matrix->col) {
        errno = EINVAL;
        return NULL;
    }
    mln_rs_matrix_t *ret;
    register mln_u8ptr_t data, origin = matrix->data;
    mln_u8_t tmp, dst;
    mln_size_t i, j, k, m;
    mln_size_t n = matrix->row * matrix->col;
    mln_size_t len = matrix->row;

    if ((data = (mln_u8ptr_t)malloc(n)) == NULL) {
        errno = ENOMEM;
        return NULL;
    }
    if ((ret = mln_rs_matrix_new(matrix->row, matrix->col, data, 0)) == NULL) {
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
            if (origin[j + m] > tmp) {
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
        if (!origin[i + m]) {
            mln_rs_matrix_free(ret);
            errno = EINVAL;
            return NULL;
        }

        tmp = origin[i + m];
        for (j = 0; j < len; ++j) {
            M_RS_GF_DIV(origin[i + j], tmp);
            M_RS_GF_DIV(data[i + j], tmp);
        }
        for (j = 0; j < n; j += len) {
            if (j != i) {
                tmp = origin[j + m];
                for (k = 0; k < len; ++k) {
                    dst = origin[i + k];
                    M_RS_GF_MUL(dst, tmp);
                    M_RS_GF_ADDSUB(origin[j + k], dst);
                    dst = data[i + k];
                    M_RS_GF_MUL(dst, tmp);
                    M_RS_GF_ADDSUB(data[j + k], dst);
                }
            }
        }
    }

    return ret;
})

MLN_FUNC(static, mln_rs_matrix_t *, mln_rs_matrix_co_matrix, \
         (mln_size_t row, mln_size_t addition_row), (row, addition_row), \
{
    mln_u8ptr_t data, p;
    mln_size_t i, j, k;
    mln_rs_matrix_t *matrix;

    if ((data = (mln_u8ptr_t)malloc((row+addition_row)*row)) == NULL) {
        errno = ENOMEM;
        return NULL;
    }

    for (i = 0, p = data; i < row; ++i) {
        for (k = 0; k < row; ++k) {
            *p++ = k==i? 1: 0;
        }
    }
    for (j = 1; i < row+addition_row; ++i, ++j) {
        for (k = 1; k <= row; ++k) {
            *p++ = (mln_u8_t)mln_rs_power_calc(k, j-1);
        }
    }

    if ((matrix = mln_rs_matrix_new(row+addition_row, row, data, 0)) == NULL) {
        free(data);
        errno = ENOMEM;
        return NULL;
    }
    return matrix;
})

MLN_FUNC(static, mln_rs_matrix_t *, mln_rs_matrix_co_inverse_matrix, \
         (uint8_t **data_vector, size_t len, size_t n, size_t k), \
         (data_vector, len, n, k), \
{
    mln_size_t i, j, row = 0;
    mln_rs_matrix_t *matrix, *inverse;
    mln_u8ptr_t p, *pend, data;

    if ((data = (mln_u8ptr_t)malloc((n + k) * n)) == NULL) {
        errno = ENOMEM;
        return NULL;
    }

    p = data;
    pend = data_vector + n;
    for (i = 0; data_vector < pend; ++data_vector, ++i) {
        if (*data_vector == NULL) continue;
        for (j = 0; j < n; ++j) {
            *p++ = (i==j)? 1: 0;
        }
        ++row;
    }
    pend = data_vector + n + k;
    for (i = 1; row < n && data_vector < pend; ++data_vector, ++i) {
        if (*data_vector == NULL) continue;
        for (j = 1; j <= n; ++j) {
            *p++ = (mln_u8_t)mln_rs_power_calc(j, i-1);
        }
        ++row;
    }

    if ((matrix = mln_rs_matrix_new(n, n, data, 0)) == NULL) {
        free(data);
        errno = ENOMEM;
        return NULL;
    }
    if ((inverse = mln_rs_matrix_inverse(matrix)) == NULL) {
        int err = errno;
        mln_rs_matrix_free(matrix);
        errno = err;
        return NULL;
    }
    mln_rs_matrix_free(matrix);
    return inverse;
})

MLN_FUNC(static, mln_rs_matrix_t *, mln_rs_matrix_data_matrix, \
         (uint8_t **data_vector, size_t len, size_t n, size_t k), \
         (data_vector, len, n, k), \
{
    mln_size_t row = 0;
    mln_u8ptr_t data, p, *pend;
    mln_rs_matrix_t *matrix;

    if ((data = (mln_u8ptr_t)malloc(n*len)) == NULL) {
        errno = ENOMEM;
        return NULL;
    }
    p = data;
    pend = data_vector + n + k;
    for (; row < n && data_vector < pend; ++data_vector) {
        if (*data_vector == NULL) continue;
        memcpy(p, *data_vector, len);
        p += len;
        ++row;
    }

    if ((matrix = mln_rs_matrix_new(n, len, data, 0)) == NULL) {
        free(data);
        errno = EINVAL;
        return NULL;
    }
    return matrix;
})

void mln_rs_matrix_dump(mln_rs_matrix_t *matrix)
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
        printf("%u ", matrix->data[i]);
    }
    printf("\n");
}

/*mln_rs_result_t*/
MLN_FUNC(static, mln_rs_result_t *, mln_rs_result_new, \
         (mln_u8ptr_t data, mln_size_t num, mln_size_t len), (data, num, len), \
{
    mln_rs_result_t *rr;
    if ((rr = (mln_rs_result_t *)malloc(sizeof(mln_rs_result_t))) == NULL) {
        errno = ENOMEM;
        return NULL;
    }
    rr->data = data;
    rr->len = len;
    rr->num = num;
    return rr;
})

MLN_FUNC_VOID(, void, mln_rs_result_free, (mln_rs_result_t *result), (result), {
    if (result == NULL) return;
    if (result->data != NULL)
        free(result->data);
    free(result);
})

/*
 * Reed-Solomon operations
 */
MLN_FUNC(, mln_rs_result_t *, mln_rs_encode, \
         (uint8_t *data_vector, size_t len, size_t n, size_t k), \
         (data_vector, len, n, k), \
{
    mln_rs_result_t *result;
    mln_rs_matrix_t *matrix, *co_matrix, *res_matrix;

    if (data_vector == NULL || !len || !n || !k) {
        errno = EINVAL;
        return NULL;
    }

    if ((matrix = mln_rs_matrix_new(n, len, data_vector, 1)) == NULL) {
        errno = ENOMEM;
        return NULL;
    }
    if ((co_matrix = mln_rs_matrix_co_matrix(n, k)) == NULL) {
        mln_rs_matrix_free(matrix);
        errno = ENOMEM;
        return NULL;
    }
    res_matrix = mln_rs_matrix_mul(co_matrix, matrix);
    mln_rs_matrix_free(matrix);
    mln_rs_matrix_free(co_matrix);
    if (res_matrix == NULL) {
        errno = ENOMEM;
        return NULL;
    }

    if ((result = mln_rs_result_new(res_matrix->data, n+k, (n+k)*len)) == NULL) {
        mln_rs_matrix_free(res_matrix);
        errno = ENOMEM;
        return NULL;
    }
    res_matrix->is_ref = 1;
    mln_rs_matrix_free(res_matrix);
    return result;
})

MLN_FUNC(, mln_rs_result_t *, mln_rs_decode, \
         (uint8_t **data_vector, size_t len, size_t n, size_t k), \
         (data_vector, len, n, k), \
{
    mln_u8ptr_t data, p, *pp, *pend;
    mln_rs_result_t *result;
    mln_rs_matrix_t *co_matrix, *data_matrix, *result_matrix;
    if (!n && !k) {
        errno = EINVAL;
        return NULL;
    }
    if (!k) {
        if ((data = (mln_u8ptr_t)malloc(len*n)) == NULL) {
            errno = ENOMEM;
            return NULL;
        }
        p = data;
        for (pp = data_vector, pend = data_vector+n; pp < pend; ++pp) {
            memcpy(p, *pp, len);
            p += len;
        }
        if ((result = mln_rs_result_new(data, n, len*n)) == NULL) {
            free(data);
            errno = ENOMEM;
            return NULL;
        }
        return result;
    }

    if ((co_matrix = mln_rs_matrix_co_inverse_matrix(data_vector, len, n, k)) == NULL) {
        return NULL;
    }
    if ((data_matrix = mln_rs_matrix_data_matrix(data_vector, len, n, k)) == NULL) {
        int err = errno;
        mln_rs_matrix_free(co_matrix);
        errno = err;
        return NULL;
    }
    result_matrix = mln_rs_matrix_mul(co_matrix, data_matrix);
    mln_rs_matrix_free(co_matrix);
    mln_rs_matrix_free(data_matrix);
    if (result_matrix == NULL) {
        errno = ENOMEM;
        return NULL;
    }
    if ((result = mln_rs_result_new(result_matrix->data, \
                                    result_matrix->row, \
                                    result_matrix->row * result_matrix->col)) == NULL)
    {
        mln_rs_matrix_free(result_matrix);
        errno = ENOMEM;
        return NULL;
    }
    result_matrix->data = NULL;
    mln_rs_matrix_free(result_matrix);
    return result;
})

