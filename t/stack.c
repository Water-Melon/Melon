#include "mln_stack.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>

static int tests_run = 0;
static int tests_passed = 0;

#define RUN_TEST(fn) do { \
    tests_run++; \
    fn(); \
    tests_passed++; \
    printf("  PASS: %s\n", #fn); \
} while (0)

/* ---- helpers ---- */

typedef struct {
    int a;
    int b;
} data_t;

static void data_free(void *ptr)
{
    free(ptr);
}

static void *data_copy(void *src, void *udata)
{
    data_t *d = (data_t *)src;
    data_t *dup = (data_t *)malloc(sizeof(data_t));
    if (dup == NULL) return NULL;
    *dup = *d;
    (void)udata;
    return dup;
}

/* ---- tests ---- */

static void test_init_destroy(void)
{
    mln_stack_t *st = mln_stack_init(NULL, NULL);
    assert(st != NULL);
    assert(mln_stack_empty(st));
    mln_stack_destroy(st);

    st = mln_stack_init(data_free, data_copy);
    assert(st != NULL);
    mln_stack_destroy(st);

    mln_stack_destroy(NULL);
}

static void test_push_pop_basic(void)
{
    mln_stack_t *st = mln_stack_init(NULL, NULL);
    assert(st != NULL);

    int a = 1, b = 2, c = 3;
    assert(mln_stack_push(st, &a) == 0);
    assert(mln_stack_push(st, &b) == 0);
    assert(mln_stack_push(st, &c) == 0);
    assert(!mln_stack_empty(st));

    assert(mln_stack_pop(st) == &c);
    assert(mln_stack_pop(st) == &b);
    assert(mln_stack_pop(st) == &a);
    assert(mln_stack_empty(st));
    assert(mln_stack_pop(st) == NULL);

    mln_stack_destroy(st);
}

static void test_top(void)
{
    mln_stack_t *st = mln_stack_init(NULL, NULL);
    assert(st != NULL);
    assert(mln_stack_top(st) == NULL);

    int a = 10, b = 20;
    mln_stack_push(st, &a);
    assert(mln_stack_top(st) == &a);

    mln_stack_push(st, &b);
    assert(mln_stack_top(st) == &b);

    mln_stack_pop(st);
    assert(mln_stack_top(st) == &a);

    mln_stack_pop(st);
    assert(mln_stack_top(st) == NULL);

    mln_stack_destroy(st);
}

static void test_empty(void)
{
    mln_stack_t *st = mln_stack_init(NULL, NULL);
    assert(st != NULL);
    assert(mln_stack_empty(st));

    int x = 1;
    mln_stack_push(st, &x);
    assert(!mln_stack_empty(st));

    mln_stack_pop(st);
    assert(mln_stack_empty(st));

    mln_stack_destroy(st);
}

static void test_push_pop_with_free(void)
{
    mln_stack_t *st = mln_stack_init(data_free, NULL);
    assert(st != NULL);

    int i;
    for (i = 0; i < 5; ++i) {
        data_t *d = (data_t *)malloc(sizeof(data_t));
        assert(d != NULL);
        d->a = i;
        d->b = i * 10;
        assert(mln_stack_push(st, d) == 0);
    }

    data_t *top = (data_t *)mln_stack_pop(st);
    assert(top != NULL);
    assert(top->a == 4 && top->b == 40);
    free(top);

    mln_stack_destroy(st);
}

static void test_grow(void)
{
    mln_stack_t *st = mln_stack_init(NULL, NULL);
    assert(st != NULL);

    int vals[1000];
    int i;
    for (i = 0; i < 1000; ++i) {
        vals[i] = i;
        assert(mln_stack_push(st, &vals[i]) == 0);
    }

    for (i = 999; i >= 0; --i) {
        int *p = (int *)mln_stack_pop(st);
        assert(p != NULL);
        assert(*p == i);
    }
    assert(mln_stack_empty(st));

    mln_stack_destroy(st);
}

static void test_dup_with_copy(void)
{
    mln_stack_t *st = mln_stack_init(data_free, data_copy);
    assert(st != NULL);

    int i;
    for (i = 0; i < 5; ++i) {
        data_t *d = (data_t *)malloc(sizeof(data_t));
        assert(d != NULL);
        d->a = i;
        d->b = i * 100;
        assert(mln_stack_push(st, d) == 0);
    }

    mln_stack_t *dup = mln_stack_dup(st, NULL);
    assert(dup != NULL);

    for (i = 4; i >= 0; --i) {
        data_t *d1 = (data_t *)mln_stack_pop(st);
        data_t *d2 = (data_t *)mln_stack_pop(dup);
        assert(d1 != NULL && d2 != NULL);
        assert(d1 != d2);
        assert(d1->a == d2->a && d1->b == d2->b);
        free(d1);
        free(d2);
    }

    mln_stack_destroy(st);
    mln_stack_destroy(dup);
}

static void test_dup_without_copy(void)
{
    mln_stack_t *st = mln_stack_init(NULL, NULL);
    assert(st != NULL);

    int vals[3] = {10, 20, 30};
    int i;
    for (i = 0; i < 3; ++i)
        assert(mln_stack_push(st, &vals[i]) == 0);

    mln_stack_t *dup = mln_stack_dup(st, NULL);
    assert(dup != NULL);

    for (i = 2; i >= 0; --i) {
        void *p1 = mln_stack_pop(st);
        void *p2 = mln_stack_pop(dup);
        assert(p1 == p2);
        assert(p1 == &vals[i]);
    }

    mln_stack_destroy(st);
    mln_stack_destroy(dup);
}

static int iterate_sum_handler(void *data, void *udata)
{
    int *sum = (int *)udata;
    int *val = (int *)data;
    *sum += *val;
    return 0;
}

static void test_iterate(void)
{
    mln_stack_t *st = mln_stack_init(NULL, NULL);
    assert(st != NULL);

    int vals[5] = {1, 2, 3, 4, 5};
    int i;
    for (i = 0; i < 5; ++i)
        assert(mln_stack_push(st, &vals[i]) == 0);

    int sum = 0;
    assert(mln_stack_iterate(st, iterate_sum_handler, &sum) == 0);
    assert(sum == 15);

    mln_stack_destroy(st);
}

static int iterate_order_handler(void *data, void *udata)
{
    int **cursor = (int **)udata;
    int val = *(int *)data;
    assert(val == **cursor);
    (*cursor)++;
    return 0;
}

static void test_iterate_order(void)
{
    mln_stack_t *st = mln_stack_init(NULL, NULL);
    assert(st != NULL);

    int vals[5] = {1, 2, 3, 4, 5};
    int expected[5] = {5, 4, 3, 2, 1};
    int i;
    for (i = 0; i < 5; ++i)
        assert(mln_stack_push(st, &vals[i]) == 0);

    int *cursor = expected;
    assert(mln_stack_iterate(st, iterate_order_handler, &cursor) == 0);

    mln_stack_destroy(st);
}

static int iterate_stop_handler(void *data, void *udata)
{
    int *count = (int *)udata;
    (*count)++;
    if (*count >= 3) return -1;
    return 0;
}

static void test_iterate_early_stop(void)
{
    mln_stack_t *st = mln_stack_init(NULL, NULL);
    assert(st != NULL);

    int vals[10];
    int i;
    for (i = 0; i < 10; ++i) {
        vals[i] = i;
        assert(mln_stack_push(st, &vals[i]) == 0);
    }

    int count = 0;
    assert(mln_stack_iterate(st, iterate_stop_handler, &count) == -1);
    assert(count == 3);

    mln_stack_destroy(st);
}

static void test_iterate_empty(void)
{
    mln_stack_t *st = mln_stack_init(NULL, NULL);
    assert(st != NULL);

    int sum = 0;
    assert(mln_stack_iterate(st, iterate_sum_handler, &sum) == 0);
    assert(sum == 0);

    mln_stack_destroy(st);
}

static void test_push_pop_interleaved(void)
{
    mln_stack_t *st = mln_stack_init(NULL, NULL);
    assert(st != NULL);

    int vals[100];
    int i;
    for (i = 0; i < 100; ++i) {
        vals[i] = i;
        assert(mln_stack_push(st, &vals[i]) == 0);
    }

    for (i = 0; i < 50; ++i)
        mln_stack_pop(st);

    for (i = 0; i < 50; ++i) {
        vals[i] = i + 1000;
        assert(mln_stack_push(st, &vals[i]) == 0);
    }

    assert(!mln_stack_empty(st));

    for (i = 0; i < 100; ++i)
        assert(mln_stack_pop(st) != NULL);

    assert(mln_stack_empty(st));

    mln_stack_destroy(st);
}

static void test_single_element(void)
{
    mln_stack_t *st = mln_stack_init(NULL, NULL);
    assert(st != NULL);

    int x = 42;
    assert(mln_stack_push(st, &x) == 0);
    assert(mln_stack_top(st) == &x);
    assert(!mln_stack_empty(st));

    void *p = mln_stack_pop(st);
    assert(p == &x);
    assert(mln_stack_empty(st));
    assert(mln_stack_top(st) == NULL);

    mln_stack_destroy(st);
}

static void test_destroy_with_elements(void)
{
    mln_stack_t *st = mln_stack_init(data_free, NULL);
    assert(st != NULL);

    int i;
    for (i = 0; i < 100; ++i) {
        data_t *d = (data_t *)malloc(sizeof(data_t));
        assert(d != NULL);
        d->a = i;
        d->b = i;
        assert(mln_stack_push(st, d) == 0);
    }

    mln_stack_destroy(st);
}

static void test_benchmark(void)
{
    const int N = 2000000;
    mln_stack_t *st = mln_stack_init(NULL, NULL);
    assert(st != NULL);

    int dummy = 0;
    clock_t start = clock();

    int i;
    for (i = 0; i < N; ++i)
        mln_stack_push(st, &dummy);
    for (i = 0; i < N; ++i)
        mln_stack_pop(st);

    clock_t end = clock();
    double elapsed = (double)(end - start) / CLOCKS_PER_SEC;
    printf("    Benchmark: %d push+pop in %.4f sec (%.0f ops/sec)\n",
           N, elapsed, (2.0 * N) / elapsed);

    assert(mln_stack_empty(st));
    mln_stack_destroy(st);
}

static void test_stability(void)
{
    const int ROUNDS = 10000;
    mln_stack_t *st = mln_stack_init(data_free, data_copy);
    assert(st != NULL);

    int round;
    for (round = 0; round < ROUNDS; ++round) {
        int count = (round % 50) + 1;
        int i;
        for (i = 0; i < count; ++i) {
            data_t *d = (data_t *)malloc(sizeof(data_t));
            assert(d != NULL);
            d->a = round;
            d->b = i;
            assert(mln_stack_push(st, d) == 0);
        }

        if (round % 7 == 0) {
            mln_stack_t *dup = mln_stack_dup(st, NULL);
            assert(dup != NULL);
            mln_stack_destroy(dup);
        }

        for (i = 0; i < count; ++i) {
            data_t *d = (data_t *)mln_stack_pop(st);
            assert(d != NULL);
            assert(d->a == round);
            free(d);
        }
    }

    assert(mln_stack_empty(st));
    mln_stack_destroy(st);
}

static void test_dup_empty(void)
{
    mln_stack_t *st = mln_stack_init(data_free, data_copy);
    assert(st != NULL);

    mln_stack_t *dup = mln_stack_dup(st, NULL);
    assert(dup != NULL);
    assert(mln_stack_empty(dup));

    mln_stack_destroy(dup);
    mln_stack_destroy(st);
}

static void test_large_grow(void)
{
    mln_stack_t *st = mln_stack_init(NULL, NULL);
    assert(st != NULL);

    const int N = 100000;
    int i;
    int dummy = 0;
    for (i = 0; i < N; ++i)
        assert(mln_stack_push(st, &dummy) == 0);

    for (i = 0; i < N; ++i)
        assert(mln_stack_pop(st) == &dummy);

    assert(mln_stack_empty(st));
    mln_stack_destroy(st);
}

int main(void)
{
    printf("Stack tests:\n");

    RUN_TEST(test_init_destroy);
    RUN_TEST(test_push_pop_basic);
    RUN_TEST(test_top);
    RUN_TEST(test_empty);
    RUN_TEST(test_push_pop_with_free);
    RUN_TEST(test_grow);
    RUN_TEST(test_dup_with_copy);
    RUN_TEST(test_dup_without_copy);
    RUN_TEST(test_dup_empty);
    RUN_TEST(test_iterate);
    RUN_TEST(test_iterate_order);
    RUN_TEST(test_iterate_early_stop);
    RUN_TEST(test_iterate_empty);
    RUN_TEST(test_push_pop_interleaved);
    RUN_TEST(test_single_element);
    RUN_TEST(test_destroy_with_elements);
    RUN_TEST(test_large_grow);
    RUN_TEST(test_benchmark);
    RUN_TEST(test_stability);

    printf("\nAll %d/%d tests passed.\n", tests_passed, tests_run);
    return 0;
}
