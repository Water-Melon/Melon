#define MLN_FUNC_FLAG

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include "mln_types.h"
#include "mln_span.h"
#include "mln_func.h"

static int tests_run = 0;
static int tests_passed = 0;

#define RUN_TEST(fn) do { \
    tests_run++; \
    fn(); \
    tests_passed++; \
    printf("  PASS: %s\n", #fn); \
} while (0)

/* ---- traced helper functions (use MLN_FUNC) ---- */

MLN_FUNC(static, int, traced_add, (int a, int b), (a, b), {
    return a + b;
})

MLN_FUNC(static, int, traced_mul, (int a, int b), (a, b), {
    return traced_add(a, b) * 2;
})

MLN_FUNC(static, int, traced_compute, (int a, int b), (a, b), {
    return traced_mul(a, b) + traced_add(a, b);
})

MLN_FUNC(static, int, traced_recurse, (int n), (n), {
    if (n <= 0) return 0;
    return traced_recurse(n - 1) + 1;
})

MLN_FUNC(static, int, traced_noop, (int x), (x), {
    return x;
})

/* ---- dump helpers ---- */

static int dump_count;
static int dump_max_level;

static void count_dump_cb(mln_span_t *s, int level, void *data)
{
    (void)s; (void)data;
    dump_count++;
    if (level > dump_max_level) dump_max_level = level;
}

static void verify_dump_cb(mln_span_t *s, int level, void *data)
{
    (void)data;
    assert(s != NULL);
    assert(mln_span_file(s) != NULL);
    assert(mln_span_func(s) != NULL);
    assert(level >= 0);
    dump_count++;
}

/* ---- tests ---- */

static void test_new_free(void)
{
    mln_span_t *s = mln_span_new(NULL, "test.c", "test_func", 42);
    assert(s != NULL);
    assert(strcmp(mln_span_file(s), "test.c") == 0);
    assert(strcmp(mln_span_func(s), "test_func") == 0);
    assert(mln_span_line(s) == 42);
    assert(s->subspans_head == NULL);
    assert(s->subspans_tail == NULL);
    assert(s->parent == NULL);
    assert(s->prev == NULL);
    assert(s->next == NULL);
    mln_span_free(s);
    mln_span_pool_free();
}

static void test_new_with_parent(void)
{
    mln_span_t *parent = mln_span_new(NULL, "p.c", "parent_fn", 10);
    assert(parent != NULL);

    mln_span_t *child1 = mln_span_new(parent, "c.c", "child1_fn", 20);
    assert(child1 != NULL);
    assert(child1->parent == parent);
    assert(parent->subspans_head == child1);
    assert(parent->subspans_tail == child1);

    mln_span_t *child2 = mln_span_new(parent, "c.c", "child2_fn", 30);
    assert(child2 != NULL);
    assert(parent->subspans_head == child1);
    assert(parent->subspans_tail == child2);
    assert(child1->next == child2);
    assert(child2->prev == child1);

    mln_span_t *grandchild = mln_span_new(child1, "g.c", "gc_fn", 40);
    assert(grandchild != NULL);
    assert(grandchild->parent == child1);
    assert(child1->subspans_head == grandchild);

    mln_span_free(parent);
    mln_span_pool_free();
}

static void test_start_stop_basic(void)
{
    mln_span_start();
    traced_add(1, 2);
    mln_span_stop();

    assert(mln_span_root != NULL);
    assert(mln_span_file(mln_span_root) != NULL);
    assert(mln_span_func(mln_span_root) != NULL);
    assert(mln_span_line(mln_span_root) > 0);
    mln_span_release();
}

static void test_dump_count(void)
{
    mln_span_start();
    traced_compute(3, 4);
    mln_span_stop();

    dump_count = 0;
    dump_max_level = 0;
    mln_span_dump(count_dump_cb, NULL);
    /*
     * root(start) -> traced_compute -> { traced_mul -> traced_add, traced_add }
     * = 1 root + 1 compute + 1 mul + 2 add = 5
     */
    assert(dump_count == 5);
    assert(dump_max_level >= 2);

    mln_span_release();
}

static void test_dump_verify(void)
{
    mln_span_start();
    traced_mul(10, 20);
    mln_span_stop();

    dump_count = 0;
    mln_span_dump(verify_dump_cb, NULL);
    assert(dump_count > 0);

    mln_span_release();
}

static void test_dump_null(void)
{
    /* NULL callback should not crash */
    mln_span_dump(NULL, NULL);

    /* NULL root should not crash */
    assert(mln_span_root == NULL);
    mln_span_dump(count_dump_cb, NULL);
}

static void test_move(void)
{
    mln_span_start();
    traced_add(1, 2);
    mln_span_stop();

    assert(mln_span_root != NULL);
    mln_span_t *moved = mln_span_move();
    assert(moved != NULL);
    assert(mln_span_root == NULL);

    /* verify moved span is still valid */
    dump_count = 0;
    mln_span_root = moved;
    mln_span_dump(count_dump_cb, NULL);
    assert(dump_count > 0);
    mln_span_root = NULL;

    mln_span_free(moved);
    mln_span_pool_free();
}

static void test_time_cost(void)
{
    mln_span_start();
    traced_add(1, 2);
    mln_span_stop();

    assert(mln_span_root != NULL);
    mln_u64_t cost = mln_span_time_cost(mln_span_root);
    /* cost should be non-negative (>= 0) in microseconds */
    (void)cost;

    /* check sub-spans also have valid times */
    if (mln_span_root->subspans_head != NULL) {
        mln_u64_t sub_cost = mln_span_time_cost(mln_span_root->subspans_head);
        (void)sub_cost;
    }

    mln_span_release();
}

static void test_file_func_line(void)
{
    mln_span_start();
    traced_add(5, 6);
    mln_span_stop();

    assert(mln_span_root != NULL);
    /* root span's file/func/line come from mln_span_start macro location */
    assert(mln_span_file(mln_span_root) != NULL);
    assert(mln_span_func(mln_span_root) != NULL);
    assert(mln_span_line(mln_span_root) > 0);

    /* first child should be traced_add */
    mln_span_t *child = mln_span_root->subspans_head;
    assert(child != NULL);
    assert(strstr(mln_span_file(child), "span.c") != NULL);

    mln_span_release();
}

static void test_deep_nesting(void)
{
    mln_span_start();
    int result = traced_recurse(200);
    assert(result == 200);
    mln_span_stop();

    dump_count = 0;
    dump_max_level = 0;
    mln_span_dump(count_dump_cb, NULL);
    /* root + 201 recursive calls = 202 */
    assert(dump_count == 202);
    assert(dump_max_level >= 200);

    mln_span_release();
}

static void test_many_siblings(void)
{
    mln_span_start();
    int i;
    for (i = 0; i < 2000; ++i) {
        traced_add(i, i + 1);
    }
    mln_span_stop();

    dump_count = 0;
    mln_span_dump(count_dump_cb, NULL);
    /* root + 2000 add calls = 2001 */
    assert(dump_count == 2001);

    mln_span_release();
}

static void test_mixed_tree(void)
{
    mln_span_start();
    int i;
    for (i = 0; i < 100; ++i) {
        traced_compute(i, i + 1);
    }
    mln_span_stop();

    dump_count = 0;
    mln_span_dump(count_dump_cb, NULL);
    /* root + 100 * (compute + mul + add_in_mul + add_in_compute) = 1 + 400 = 401 */
    assert(dump_count == 401);

    mln_span_release();
}

static void test_repeated_cycles(void)
{
    int cycle;
    for (cycle = 0; cycle < 200; ++cycle) {
        mln_span_start();
        traced_compute(cycle, cycle + 1);
        mln_span_stop();

        assert(mln_span_root != NULL);
        mln_span_release();
    }
}

static void test_free_null(void)
{
    mln_span_free(NULL);
    /* should not crash */
}

static void test_free_single(void)
{
    mln_span_t *s = mln_span_new(NULL, "f.c", "fn", 1);
    assert(s != NULL);
    mln_span_free(s);
    mln_span_pool_free();
}

static void test_free_deep_tree(void)
{
    /* Build a deep tree manually and verify iterative free handles it */
    mln_span_t *root = mln_span_new(NULL, "d.c", "root", 1);
    assert(root != NULL);
    mln_span_t *cur = root;
    int i;
    for (i = 0; i < 5000; ++i) {
        mln_span_t *child = mln_span_new(cur, "d.c", "child", i);
        assert(child != NULL);
        cur = child;
    }
    /* This would stack-overflow with recursive free; iterative handles it */
    mln_span_free(root);
    mln_span_pool_free();
}

static void test_free_wide_tree(void)
{
    mln_span_t *root = mln_span_new(NULL, "w.c", "root", 1);
    assert(root != NULL);
    int i;
    for (i = 0; i < 5000; ++i) {
        mln_span_t *child = mln_span_new(root, "w.c", "child", i);
        assert(child != NULL);
    }
    mln_span_free(root);
    mln_span_pool_free();
}

static void test_pool_reuse(void)
{
    /* Verify the pool recycles memory across cycles */
    int cycle;
    for (cycle = 0; cycle < 50; ++cycle) {
        mln_span_start();
        int i;
        for (i = 0; i < 100; ++i) {
            traced_add(i, i + 1);
        }
        mln_span_stop();

        /* free spans (recycles to pool) */
        mln_span_free(mln_span_root);
        mln_span_root = NULL;
        /* do NOT call mln_span_pool_free: spans stay in free list */
    }
    /* final cleanup */
    mln_span_pool_free();
}

static void test_move_then_free(void)
{
    mln_span_start();
    traced_compute(1, 2);
    traced_compute(3, 4);
    mln_span_stop();

    mln_span_t *moved = mln_span_move();
    assert(moved != NULL);
    assert(mln_span_root == NULL);

    dump_count = 0;
    mln_span_root = moved;
    mln_span_dump(count_dump_cb, NULL);
    mln_span_root = NULL;
    assert(dump_count > 0);

    mln_span_free(moved);
    mln_span_pool_free();
}

static void test_benchmark(void)
{
    const int N = 1000000;
    clock_t start, end;
    double elapsed;
    int i;

    mln_span_start();

    start = clock();
    for (i = 0; i < N; ++i) {
        traced_noop(i);
    }
    end = clock();

    mln_span_stop();

    elapsed = (double)(end - start) / CLOCKS_PER_SEC;
    if (elapsed > 0) {
        printf("    Benchmark: %d traced calls in %.4f sec (%.0f calls/sec)\n",
               N, elapsed, (double)N / elapsed);
    } else {
        printf("    Benchmark: %d traced calls in < 1ms\n", N);
    }

    mln_span_release();
}

static void test_stability(void)
{
    const int ROUNDS = 2000;
    int round;

    for (round = 0; round < ROUNDS; ++round) {
        mln_span_start();

        int count = (round % 50) + 1;
        int i;
        for (i = 0; i < count; ++i) {
            traced_compute(i, i + 1);
        }

        mln_span_stop();

        if (round % 7 == 0) {
            dump_count = 0;
            mln_span_dump(count_dump_cb, NULL);
            assert(dump_count > 0);
        }

        if (round % 3 == 0) {
            mln_span_t *moved = mln_span_move();
            assert(moved != NULL);
            mln_span_free(moved);
            mln_span_pool_free();
        } else {
            mln_span_release();
        }
    }
}

int main(void)
{
    printf("Span tests:\n");

    RUN_TEST(test_new_free);
    RUN_TEST(test_new_with_parent);
    RUN_TEST(test_start_stop_basic);
    RUN_TEST(test_dump_count);
    RUN_TEST(test_dump_verify);
    RUN_TEST(test_dump_null);
    RUN_TEST(test_move);
    RUN_TEST(test_time_cost);
    RUN_TEST(test_file_func_line);
    RUN_TEST(test_deep_nesting);
    RUN_TEST(test_many_siblings);
    RUN_TEST(test_mixed_tree);
    RUN_TEST(test_repeated_cycles);
    RUN_TEST(test_free_null);
    RUN_TEST(test_free_single);
    RUN_TEST(test_free_deep_tree);
    RUN_TEST(test_free_wide_tree);
    RUN_TEST(test_pool_reuse);
    RUN_TEST(test_move_then_free);
    RUN_TEST(test_benchmark);
    RUN_TEST(test_stability);

    printf("\nAll %d/%d tests passed.\n", tests_passed, tests_run);
    return 0;
}
