#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <math.h>
#include <time.h>
#if !defined(MSVC)
#include <sys/time.h>
#endif
#include "mln_matrix.h"

#define EPSILON 1e-6

static int test_new_free(void)
{
    mln_matrix_t *m;
    double data[] = {1, 2, 3, 4, 5, 6};

    /* Test ref mode */
    m = mln_matrix_new(2, 3, data, 1);
    if (m == NULL) {
        fprintf(stderr, "test_new_free: ref alloc failed\n");
        return -1;
    }
    if (m->row != 2 || m->col != 3 || m->is_ref != 1 || m->data != data) {
        fprintf(stderr, "test_new_free: ref fields mismatch\n");
        mln_matrix_free(m);
        return -1;
    }
    mln_matrix_free(m);

    /* Test non-ref mode */
    m = mln_matrix_new(2, 3, data, 0);
    if (m == NULL) {
        fprintf(stderr, "test_new_free: non-ref alloc failed\n");
        return -1;
    }
    if (m->row != 2 || m->col != 3 || m->is_ref != 0) {
        fprintf(stderr, "test_new_free: non-ref fields mismatch\n");
        mln_matrix_free(m);
        return -1;
    }
    /* data pointer assigned directly; free must not crash */
    m->data = NULL; /* prevent double free since data is stack */
    mln_matrix_free(m);

    /* Free NULL should not crash */
    mln_matrix_free(NULL);

    return 0;
}

static int test_mul_basic(void)
{
    /* [1 2] * [5 6] = [19 22]
       [3 4]   [7 8]   [43 50] */
    double d1[] = {1, 2, 3, 4};
    double d2[] = {5, 6, 7, 8};
    double expect[] = {19, 22, 43, 50};
    mln_matrix_t *a, *b, *c;
    mln_size_t i;

    a = mln_matrix_new(2, 2, d1, 1);
    b = mln_matrix_new(2, 2, d2, 1);
    if (a == NULL || b == NULL) {
        fprintf(stderr, "test_mul_basic: alloc failed\n");
        return -1;
    }
    c = mln_matrix_mul(a, b);
    if (c == NULL) {
        fprintf(stderr, "test_mul_basic: mul failed\n");
        mln_matrix_free(a);
        mln_matrix_free(b);
        return -1;
    }
    for (i = 0; i < 4; ++i) {
        if (fabs(c->data[i] - expect[i]) > EPSILON) {
            fprintf(stderr, "test_mul_basic: mismatch at %lu: %f != %f\n",
                    (unsigned long)i, c->data[i], expect[i]);
            mln_matrix_free(a);
            mln_matrix_free(b);
            mln_matrix_free(c);
            return -1;
        }
    }
    mln_matrix_free(a);
    mln_matrix_free(b);
    mln_matrix_free(c);
    return 0;
}

static int test_mul_non_square(void)
{
    /* [1 2 3] * [7  8 ]   [58  64 ]
       [4 5 6]   [9  10] = [139 154]
                  [11 12]                */
    double d1[] = {1, 2, 3, 4, 5, 6};
    double d2[] = {7, 8, 9, 10, 11, 12};
    double expect[] = {58, 64, 139, 154};
    mln_matrix_t *a, *b, *c;
    mln_size_t i;

    a = mln_matrix_new(2, 3, d1, 1);
    b = mln_matrix_new(3, 2, d2, 1);
    c = mln_matrix_mul(a, b);
    if (c == NULL) {
        fprintf(stderr, "test_mul_non_square: mul failed\n");
        mln_matrix_free(a);
        mln_matrix_free(b);
        return -1;
    }
    if (c->row != 2 || c->col != 2) {
        fprintf(stderr, "test_mul_non_square: dimension mismatch\n");
        mln_matrix_free(a);
        mln_matrix_free(b);
        mln_matrix_free(c);
        return -1;
    }
    for (i = 0; i < 4; ++i) {
        if (fabs(c->data[i] - expect[i]) > EPSILON) {
            fprintf(stderr, "test_mul_non_square: mismatch at %lu\n", (unsigned long)i);
            mln_matrix_free(a);
            mln_matrix_free(b);
            mln_matrix_free(c);
            return -1;
        }
    }
    mln_matrix_free(a);
    mln_matrix_free(b);
    mln_matrix_free(c);
    return 0;
}

static int test_mul_dimension_error(void)
{
    double d1[] = {1, 2, 3, 4};
    double d2[] = {1, 2, 3, 4, 5, 6};
    mln_matrix_t *a, *b, *c;

    a = mln_matrix_new(2, 2, d1, 1);
    b = mln_matrix_new(3, 2, d2, 1);
    c = mln_matrix_mul(a, b);
    if (c != NULL) {
        fprintf(stderr, "test_mul_dimension_error: expected NULL\n");
        mln_matrix_free(a);
        mln_matrix_free(b);
        mln_matrix_free(c);
        return -1;
    }
    mln_matrix_free(a);
    mln_matrix_free(b);
    return 0;
}

static int test_inverse_basic(void)
{
    /* 3x3 matrix inverse, then multiply: A * A^-1 = I */
    double data[] = {1, 1, 1, 1, 2, 4, 2, 8, 64};
    double datacopy[9];
    mln_matrix_t *a, *inv, *prod;
    mln_size_t i, j;

    memcpy(datacopy, data, sizeof(data));
    a = mln_matrix_new(3, 3, datacopy, 1);
    if (a == NULL) {
        fprintf(stderr, "test_inverse_basic: alloc failed\n");
        return -1;
    }
    inv = mln_matrix_inverse(a);
    if (inv == NULL) {
        fprintf(stderr, "test_inverse_basic: inverse failed: %s\n", strerror(errno));
        mln_matrix_free(a);
        return -1;
    }

    /* Restore original data (inverse may modify it in old code, but new code uses a copy) */
    memcpy(datacopy, data, sizeof(data));
    prod = mln_matrix_mul(a, inv);
    if (prod == NULL) {
        fprintf(stderr, "test_inverse_basic: mul failed\n");
        mln_matrix_free(a);
        mln_matrix_free(inv);
        return -1;
    }

    /* Check identity */
    for (i = 0; i < 3; ++i) {
        for (j = 0; j < 3; ++j) {
            double expected = (i == j) ? 1.0 : 0.0;
            if (fabs(prod->data[i * 3 + j] - expected) > 1e-4) {
                fprintf(stderr, "test_inverse_basic: not identity at [%lu][%lu]: %f\n",
                        (unsigned long)i, (unsigned long)j, prod->data[i * 3 + j]);
                mln_matrix_free(a);
                mln_matrix_free(inv);
                mln_matrix_free(prod);
                return -1;
            }
        }
    }
    mln_matrix_free(a);
    mln_matrix_free(inv);
    mln_matrix_free(prod);
    return 0;
}

static int test_inverse_singular(void)
{
    double data[] = {1, 2, 3, 4, 5, 6, 7, 8, 9}; /* singular */
    double datacopy[9];
    mln_matrix_t *a, *inv;

    memcpy(datacopy, data, sizeof(data));
    a = mln_matrix_new(3, 3, datacopy, 1);
    inv = mln_matrix_inverse(a);
    if (inv != NULL) {
        fprintf(stderr, "test_inverse_singular: expected NULL for singular matrix\n");
        mln_matrix_free(a);
        mln_matrix_free(inv);
        return -1;
    }
    mln_matrix_free(a);
    return 0;
}

static int test_inverse_non_square(void)
{
    double data[] = {1, 2, 3, 4, 5, 6};
    mln_matrix_t *a, *inv;

    a = mln_matrix_new(2, 3, data, 1);
    inv = mln_matrix_inverse(a);
    if (inv != NULL) {
        fprintf(stderr, "test_inverse_non_square: expected NULL\n");
        mln_matrix_free(a);
        mln_matrix_free(inv);
        return -1;
    }
    mln_matrix_free(a);
    return 0;
}

static int test_inverse_preserves_input(void)
{
    /* The optimized inverse should NOT modify the original matrix */
    double orig[] = {2, 1, 1, 3};
    double saved[4];
    mln_matrix_t *a, *inv;
    mln_size_t i;

    memcpy(saved, orig, sizeof(orig));
    a = mln_matrix_new(2, 2, orig, 1);
    inv = mln_matrix_inverse(a);
    if (inv == NULL) {
        fprintf(stderr, "test_inverse_preserves_input: inverse failed\n");
        mln_matrix_free(a);
        return -1;
    }
    for (i = 0; i < 4; ++i) {
        if (fabs(orig[i] - saved[i]) > EPSILON) {
            fprintf(stderr, "test_inverse_preserves_input: input modified at %lu\n", (unsigned long)i);
            mln_matrix_free(a);
            mln_matrix_free(inv);
            return -1;
        }
    }
    mln_matrix_free(a);
    mln_matrix_free(inv);
    return 0;
}

static int test_add(void)
{
    double d1[] = {1, 2, 3, 4, 5, 6};
    double d2[] = {6, 5, 4, 3, 2, 1};
    mln_matrix_t *a, *b, *c;
    mln_size_t i;

    a = mln_matrix_new(2, 3, d1, 1);
    b = mln_matrix_new(2, 3, d2, 1);
    c = mln_matrix_add(a, b);
    if (c == NULL) {
        fprintf(stderr, "test_add: add failed\n");
        mln_matrix_free(a);
        mln_matrix_free(b);
        return -1;
    }
    for (i = 0; i < 6; ++i) {
        if (fabs(c->data[i] - 7.0) > EPSILON) {
            fprintf(stderr, "test_add: mismatch at %lu\n", (unsigned long)i);
            mln_matrix_free(a);
            mln_matrix_free(b);
            mln_matrix_free(c);
            return -1;
        }
    }
    mln_matrix_free(a);
    mln_matrix_free(b);
    mln_matrix_free(c);
    return 0;
}

static int test_add_dimension_error(void)
{
    double d1[] = {1, 2, 3, 4};
    double d2[] = {1, 2, 3, 4, 5, 6};
    mln_matrix_t *a, *b, *c;

    a = mln_matrix_new(2, 2, d1, 1);
    b = mln_matrix_new(2, 3, d2, 1);
    c = mln_matrix_add(a, b);
    if (c != NULL) {
        fprintf(stderr, "test_add_dimension_error: expected NULL\n");
        mln_matrix_free(a);
        mln_matrix_free(b);
        mln_matrix_free(c);
        return -1;
    }
    mln_matrix_free(a);
    mln_matrix_free(b);
    return 0;
}

static int test_sub(void)
{
    double d1[] = {10, 20, 30, 40};
    double d2[] = {1, 2, 3, 4};
    double expect[] = {9, 18, 27, 36};
    mln_matrix_t *a, *b, *c;
    mln_size_t i;

    a = mln_matrix_new(2, 2, d1, 1);
    b = mln_matrix_new(2, 2, d2, 1);
    c = mln_matrix_sub(a, b);
    if (c == NULL) {
        fprintf(stderr, "test_sub: sub failed\n");
        mln_matrix_free(a);
        mln_matrix_free(b);
        return -1;
    }
    for (i = 0; i < 4; ++i) {
        if (fabs(c->data[i] - expect[i]) > EPSILON) {
            fprintf(stderr, "test_sub: mismatch at %lu\n", (unsigned long)i);
            mln_matrix_free(a);
            mln_matrix_free(b);
            mln_matrix_free(c);
            return -1;
        }
    }
    mln_matrix_free(a);
    mln_matrix_free(b);
    mln_matrix_free(c);
    return 0;
}

static int test_scalar_mul(void)
{
    double d[] = {1, 2, 3, 4, 5, 6};
    mln_matrix_t *a, *c;
    mln_size_t i;

    a = mln_matrix_new(2, 3, d, 1);
    c = mln_matrix_scalar_mul(3.0, a);
    if (c == NULL) {
        fprintf(stderr, "test_scalar_mul: failed\n");
        mln_matrix_free(a);
        return -1;
    }
    for (i = 0; i < 6; ++i) {
        if (fabs(c->data[i] - d[i] * 3.0) > EPSILON) {
            fprintf(stderr, "test_scalar_mul: mismatch at %lu\n", (unsigned long)i);
            mln_matrix_free(a);
            mln_matrix_free(c);
            return -1;
        }
    }
    mln_matrix_free(a);
    mln_matrix_free(c);
    return 0;
}

static int test_transpose(void)
{
    /* [1 2 3]T = [1 4]
       [4 5 6]    [2 5]
                   [3 6] */
    double d[] = {1, 2, 3, 4, 5, 6};
    double expect[] = {1, 4, 2, 5, 3, 6};
    mln_matrix_t *a, *t;
    mln_size_t i;

    a = mln_matrix_new(2, 3, d, 1);
    t = mln_matrix_transpose(a);
    if (t == NULL) {
        fprintf(stderr, "test_transpose: failed\n");
        mln_matrix_free(a);
        return -1;
    }
    if (t->row != 3 || t->col != 2) {
        fprintf(stderr, "test_transpose: dimension wrong\n");
        mln_matrix_free(a);
        mln_matrix_free(t);
        return -1;
    }
    for (i = 0; i < 6; ++i) {
        if (fabs(t->data[i] - expect[i]) > EPSILON) {
            fprintf(stderr, "test_transpose: mismatch at %lu\n", (unsigned long)i);
            mln_matrix_free(a);
            mln_matrix_free(t);
            return -1;
        }
    }
    mln_matrix_free(a);
    mln_matrix_free(t);
    return 0;
}

static int test_det(void)
{
    /* det([[1,2],[3,4]]) = -2 */
    double d1[] = {1, 2, 3, 4};
    mln_matrix_t *a;
    double det;

    a = mln_matrix_new(2, 2, d1, 1);
    det = mln_matrix_det(a);
    if (fabs(det - (-2.0)) > EPSILON) {
        fprintf(stderr, "test_det: 2x2 det mismatch: %f\n", det);
        mln_matrix_free(a);
        return -1;
    }
    mln_matrix_free(a);

    /* det([[1,1,1],[1,2,4],[2,8,64]]) = 12 */
    {
        double d2[] = {1, 1, 1, 1, 2, 4, 2, 8, 64};
        a = mln_matrix_new(3, 3, d2, 1);
        det = mln_matrix_det(a);
        if (fabs(det - 44.0) > EPSILON) {
            fprintf(stderr, "test_det: 3x3 det mismatch: %f\n", det);
            mln_matrix_free(a);
            return -1;
        }
        mln_matrix_free(a);
    }

    /* Singular matrix det = 0 */
    {
        double d3[] = {1, 2, 3, 4, 5, 6, 7, 8, 9};
        a = mln_matrix_new(3, 3, d3, 1);
        det = mln_matrix_det(a);
        if (fabs(det) > EPSILON) {
            fprintf(stderr, "test_det: singular det mismatch: %f\n", det);
            mln_matrix_free(a);
            return -1;
        }
        mln_matrix_free(a);
    }

    /* NULL and non-square */
    det = mln_matrix_det(NULL);
    if (fabs(det) > EPSILON) {
        fprintf(stderr, "test_det: NULL should return 0\n");
        return -1;
    }

    {
        double d4[] = {1, 2, 3, 4, 5, 6};
        a = mln_matrix_new(2, 3, d4, 1);
        det = mln_matrix_det(a);
        if (fabs(det) > EPSILON) {
            fprintf(stderr, "test_det: non-square should return 0\n");
            mln_matrix_free(a);
            return -1;
        }
        mln_matrix_free(a);
    }

    return 0;
}

static int test_dump(void)
{
    double data[] = {1, 2, 3, 4};
    mln_matrix_t *m;

    m = mln_matrix_new(2, 2, data, 1);
    mln_matrix_dump(m);
    mln_matrix_dump(NULL); /* should not crash */
    mln_matrix_free(m);
    return 0;
}

static double get_time_ms(void)
{
#if defined(MSVC)
    return (double)clock() * 1000.0 / CLOCKS_PER_SEC;
#else
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000.0 + tv.tv_usec / 1000.0;
#endif
}

static double *random_matrix_data(mln_size_t n)
{
    double *data = (double *)malloc(n * sizeof(double));
    mln_size_t i;
    if (data == NULL) return NULL;
    for (i = 0; i < n; ++i)
        data[i] = (double)(rand() % 1000) / 100.0;
    return data;
}

static int test_performance_mul(void)
{
    mln_size_t size = 256;
    mln_size_t iters = 5;
    mln_size_t i;
    double *d1, *d2;
    mln_matrix_t *a, *b, *c;
    double start, elapsed, total = 0;

    fprintf(stdout, "\n--- Performance: mul %lux%lu ---\n",
            (unsigned long)size, (unsigned long)size);

    d1 = random_matrix_data(size * size);
    d2 = random_matrix_data(size * size);
    if (d1 == NULL || d2 == NULL) {
        fprintf(stderr, "test_performance_mul: alloc failed\n");
        free(d1);
        free(d2);
        return -1;
    }

    a = mln_matrix_new(size, size, d1, 1);
    b = mln_matrix_new(size, size, d2, 1);

    for (i = 0; i < iters; ++i) {
        start = get_time_ms();
        c = mln_matrix_mul(a, b);
        elapsed = get_time_ms() - start;
        total += elapsed;
        fprintf(stdout, "  iter %lu: %.2f ms\n", (unsigned long)(i + 1), elapsed);
        mln_matrix_free(c);
    }

    fprintf(stdout, "  avg: %.2f ms\n", total / iters);
    mln_matrix_free(a);
    mln_matrix_free(b);
    free(d1);
    free(d2);
    return 0;
}

static int test_performance_inverse(void)
{
    mln_size_t size = 128;
    mln_size_t iters = 5;
    mln_size_t i, j;
    double *d;
    mln_matrix_t *a, *inv;
    double start, elapsed, total = 0;

    fprintf(stdout, "\n--- Performance: inverse %lux%lu ---\n",
            (unsigned long)size, (unsigned long)size);

    d = random_matrix_data(size * size);
    if (d == NULL) {
        fprintf(stderr, "test_performance_inverse: alloc failed\n");
        return -1;
    }
    /* Make diagonally dominant to ensure invertibility */
    for (i = 0; i < size; ++i)
        d[i * size + i] += (double)size * 10.0;

    for (i = 0; i < iters; ++i) {
        a = mln_matrix_new(size, size, d, 1);
        start = get_time_ms();
        inv = mln_matrix_inverse(a);
        elapsed = get_time_ms() - start;
        total += elapsed;
        fprintf(stdout, "  iter %lu: %.2f ms\n", (unsigned long)(i + 1), elapsed);
        if (inv == NULL) {
            fprintf(stderr, "test_performance_inverse: inverse failed at iter %lu\n",
                    (unsigned long)i);
            mln_matrix_free(a);
            free(d);
            return -1;
        }
        mln_matrix_free(inv);
        mln_matrix_free(a);
    }

    fprintf(stdout, "  avg: %.2f ms\n", total / iters);
    free(d);
    (void)j;
    return 0;
}

static int test_stability_inverse(void)
{
    /* Test that inverse of inverse returns the original */
    double orig[] = {4, 7, 2, 6};
    double saved[4];
    mln_matrix_t *a, *inv, *inv2;
    mln_size_t i;

    memcpy(saved, orig, sizeof(orig));
    a = mln_matrix_new(2, 2, orig, 1);
    inv = mln_matrix_inverse(a);
    if (inv == NULL) {
        fprintf(stderr, "test_stability_inverse: first inverse failed\n");
        mln_matrix_free(a);
        return -1;
    }
    inv2 = mln_matrix_inverse(inv);
    if (inv2 == NULL) {
        fprintf(stderr, "test_stability_inverse: second inverse failed\n");
        mln_matrix_free(a);
        mln_matrix_free(inv);
        return -1;
    }
    for (i = 0; i < 4; ++i) {
        if (fabs(inv2->data[i] - saved[i]) > 1e-4) {
            fprintf(stderr, "test_stability_inverse: inv(inv(A)) != A at %lu: %f != %f\n",
                    (unsigned long)i, inv2->data[i], saved[i]);
            mln_matrix_free(a);
            mln_matrix_free(inv);
            mln_matrix_free(inv2);
            return -1;
        }
    }
    mln_matrix_free(a);
    mln_matrix_free(inv);
    mln_matrix_free(inv2);
    return 0;
}

static int test_stability_mul_identity(void)
{
    /* A * I = A */
    double da[] = {1, 2, 3, 4, 5, 6, 7, 8, 9};
    double di[] = {1, 0, 0, 0, 1, 0, 0, 0, 1};
    mln_matrix_t *a, *id, *c;
    mln_size_t i;

    a = mln_matrix_new(3, 3, da, 1);
    id = mln_matrix_new(3, 3, di, 1);
    c = mln_matrix_mul(a, id);
    if (c == NULL) {
        fprintf(stderr, "test_stability_mul_identity: mul failed\n");
        mln_matrix_free(a);
        mln_matrix_free(id);
        return -1;
    }
    for (i = 0; i < 9; ++i) {
        if (fabs(c->data[i] - da[i]) > EPSILON) {
            fprintf(stderr, "test_stability_mul_identity: A*I != A at %lu\n", (unsigned long)i);
            mln_matrix_free(a);
            mln_matrix_free(id);
            mln_matrix_free(c);
            return -1;
        }
    }
    mln_matrix_free(a);
    mln_matrix_free(id);
    mln_matrix_free(c);
    return 0;
}

static int test_performance_add(void)
{
    mln_size_t size = 512;
    mln_size_t iters = 20;
    mln_size_t i;
    double *d1, *d2;
    mln_matrix_t *a, *b, *c;
    double start, elapsed, total = 0;

    fprintf(stdout, "\n--- Performance: add %lux%lu ---\n",
            (unsigned long)size, (unsigned long)size);

    d1 = random_matrix_data(size * size);
    d2 = random_matrix_data(size * size);
    if (d1 == NULL || d2 == NULL) {
        free(d1);
        free(d2);
        return -1;
    }
    a = mln_matrix_new(size, size, d1, 1);
    b = mln_matrix_new(size, size, d2, 1);

    for (i = 0; i < iters; ++i) {
        start = get_time_ms();
        c = mln_matrix_add(a, b);
        elapsed = get_time_ms() - start;
        total += elapsed;
        mln_matrix_free(c);
    }

    fprintf(stdout, "  avg over %lu iters: %.2f ms\n", (unsigned long)iters, total / iters);
    mln_matrix_free(a);
    mln_matrix_free(b);
    free(d1);
    free(d2);
    return 0;
}

int main(int argc, char *argv[])
{
    int ret = 0;

    srand((unsigned int)time(NULL));

    fprintf(stdout, "=== Matrix Test Suite ===\n\n");

    /* Correctness tests */
    fprintf(stdout, "[new/free] ");
    if (test_new_free() < 0) { fprintf(stdout, "FAIL\n"); ret = -1; }
    else fprintf(stdout, "PASS\n");

    fprintf(stdout, "[mul basic] ");
    if (test_mul_basic() < 0) { fprintf(stdout, "FAIL\n"); ret = -1; }
    else fprintf(stdout, "PASS\n");

    fprintf(stdout, "[mul non-square] ");
    if (test_mul_non_square() < 0) { fprintf(stdout, "FAIL\n"); ret = -1; }
    else fprintf(stdout, "PASS\n");

    fprintf(stdout, "[mul dimension error] ");
    if (test_mul_dimension_error() < 0) { fprintf(stdout, "FAIL\n"); ret = -1; }
    else fprintf(stdout, "PASS\n");

    fprintf(stdout, "[inverse basic] ");
    if (test_inverse_basic() < 0) { fprintf(stdout, "FAIL\n"); ret = -1; }
    else fprintf(stdout, "PASS\n");

    fprintf(stdout, "[inverse singular] ");
    if (test_inverse_singular() < 0) { fprintf(stdout, "FAIL\n"); ret = -1; }
    else fprintf(stdout, "PASS\n");

    fprintf(stdout, "[inverse non-square] ");
    if (test_inverse_non_square() < 0) { fprintf(stdout, "FAIL\n"); ret = -1; }
    else fprintf(stdout, "PASS\n");

    fprintf(stdout, "[inverse preserves input] ");
    if (test_inverse_preserves_input() < 0) { fprintf(stdout, "FAIL\n"); ret = -1; }
    else fprintf(stdout, "PASS\n");

    fprintf(stdout, "[add] ");
    if (test_add() < 0) { fprintf(stdout, "FAIL\n"); ret = -1; }
    else fprintf(stdout, "PASS\n");

    fprintf(stdout, "[add dimension error] ");
    if (test_add_dimension_error() < 0) { fprintf(stdout, "FAIL\n"); ret = -1; }
    else fprintf(stdout, "PASS\n");

    fprintf(stdout, "[sub] ");
    if (test_sub() < 0) { fprintf(stdout, "FAIL\n"); ret = -1; }
    else fprintf(stdout, "PASS\n");

    fprintf(stdout, "[scalar_mul] ");
    if (test_scalar_mul() < 0) { fprintf(stdout, "FAIL\n"); ret = -1; }
    else fprintf(stdout, "PASS\n");

    fprintf(stdout, "[transpose] ");
    if (test_transpose() < 0) { fprintf(stdout, "FAIL\n"); ret = -1; }
    else fprintf(stdout, "PASS\n");

    fprintf(stdout, "[det] ");
    if (test_det() < 0) { fprintf(stdout, "FAIL\n"); ret = -1; }
    else fprintf(stdout, "PASS\n");

    fprintf(stdout, "[dump] ");
    if (test_dump() < 0) { fprintf(stdout, "FAIL\n"); ret = -1; }
    else fprintf(stdout, "PASS\n");

    /* Stability tests */
    fprintf(stdout, "\n[stability: inv(inv(A))==A] ");
    if (test_stability_inverse() < 0) { fprintf(stdout, "FAIL\n"); ret = -1; }
    else fprintf(stdout, "PASS\n");

    fprintf(stdout, "[stability: A*I==A] ");
    if (test_stability_mul_identity() < 0) { fprintf(stdout, "FAIL\n"); ret = -1; }
    else fprintf(stdout, "PASS\n");

    /* Performance tests */
    if (test_performance_mul() < 0) ret = -1;
    if (test_performance_inverse() < 0) ret = -1;
    if (test_performance_add() < 0) ret = -1;

    fprintf(stdout, "\n=== %s ===\n", ret == 0 ? "ALL TESTS PASSED" : "SOME TESTS FAILED");
    return ret;
}
