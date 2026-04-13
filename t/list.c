#include "mln_list.h"
#include "mln_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>

typedef struct {
    int        val;
    mln_list_t node;
} test_t;

static double now_sec(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

/* ============================================================ */
/*                      Feature Tests                           */
/* ============================================================ */

static void test_init_and_empty(void)
{
    mln_list_t s;
    mln_list_init(&s);
    assert(mln_list_head(&s) == NULL);
    assert(mln_list_tail(&s) == NULL);
    printf("  PASS: init and empty\n");
}

static void test_null_init_and_empty(void)
{
    mln_list_t s = mln_list_null();
    assert(mln_list_head(&s) == NULL);
    assert(mln_list_tail(&s) == NULL);

    /* for_each on empty null-initialized list must not crash */
    int count = 0;
    mln_list_t *lnode;
    mln_list_for_each(lnode, &s) {
        count++;
    }
    assert(count == 0);

    mln_list_t *tmp;
    mln_list_for_each_safe(lnode, tmp, &s) {
        count++;
    }
    assert(count == 0);

    printf("  PASS: null init and empty\n");
}

static void test_add_single(void)
{
    mln_list_t s;
    mln_list_init(&s);
    test_t t;
    t.val = 42;
    mln_list_add(&s, &t.node);
    assert(mln_list_head(&s) == &t.node);
    assert(mln_list_tail(&s) == &t.node);
    assert(mln_list_next(&s, &t.node) == NULL);
    assert(mln_list_prev(&s, &t.node) == NULL);
    printf("  PASS: add single\n");
}

static void test_add_multiple(void)
{
    mln_list_t s;
    mln_list_init(&s);
    test_t a, b, c;
    a.val = 1; b.val = 2; c.val = 3;

    mln_list_add(&s, &a.node);
    mln_list_add(&s, &b.node);
    mln_list_add(&s, &c.node);

    /* head is first added, tail is last added */
    assert(mln_list_head(&s) == &a.node);
    assert(mln_list_tail(&s) == &c.node);

    /* forward traversal: a -> b -> c */
    assert(mln_list_next(&s, &a.node) == &b.node);
    assert(mln_list_next(&s, &b.node) == &c.node);
    assert(mln_list_next(&s, &c.node) == NULL);

    /* backward traversal: c -> b -> a */
    assert(mln_list_prev(&s, &c.node) == &b.node);
    assert(mln_list_prev(&s, &b.node) == &a.node);
    assert(mln_list_prev(&s, &a.node) == NULL);

    printf("  PASS: add multiple\n");
}

static void test_remove_head(void)
{
    mln_list_t s;
    mln_list_init(&s);
    test_t a, b, c;
    a.val = 1; b.val = 2; c.val = 3;
    mln_list_add(&s, &a.node);
    mln_list_add(&s, &b.node);
    mln_list_add(&s, &c.node);

    mln_list_remove(&s, &a.node);
    assert(mln_list_head(&s) == &b.node);
    assert(mln_list_tail(&s) == &c.node);
    assert(mln_list_prev(&s, &b.node) == NULL);
    printf("  PASS: remove head\n");
}

static void test_remove_tail(void)
{
    mln_list_t s;
    mln_list_init(&s);
    test_t a, b, c;
    a.val = 1; b.val = 2; c.val = 3;
    mln_list_add(&s, &a.node);
    mln_list_add(&s, &b.node);
    mln_list_add(&s, &c.node);

    mln_list_remove(&s, &c.node);
    assert(mln_list_head(&s) == &a.node);
    assert(mln_list_tail(&s) == &b.node);
    assert(mln_list_next(&s, &b.node) == NULL);
    printf("  PASS: remove tail\n");
}

static void test_remove_middle(void)
{
    mln_list_t s;
    mln_list_init(&s);
    test_t a, b, c;
    a.val = 1; b.val = 2; c.val = 3;
    mln_list_add(&s, &a.node);
    mln_list_add(&s, &b.node);
    mln_list_add(&s, &c.node);

    mln_list_remove(&s, &b.node);
    assert(mln_list_head(&s) == &a.node);
    assert(mln_list_tail(&s) == &c.node);
    assert(mln_list_next(&s, &a.node) == &c.node);
    assert(mln_list_prev(&s, &c.node) == &a.node);
    printf("  PASS: remove middle\n");
}

static void test_remove_all(void)
{
    mln_list_t s;
    mln_list_init(&s);
    test_t a, b;
    a.val = 1; b.val = 2;
    mln_list_add(&s, &a.node);
    mln_list_add(&s, &b.node);

    mln_list_remove(&s, &a.node);
    mln_list_remove(&s, &b.node);
    assert(mln_list_head(&s) == NULL);
    assert(mln_list_tail(&s) == NULL);
    printf("  PASS: remove all\n");
}

static void test_remove_single(void)
{
    mln_list_t s;
    mln_list_init(&s);
    test_t a;
    a.val = 1;
    mln_list_add(&s, &a.node);
    mln_list_remove(&s, &a.node);
    assert(mln_list_head(&s) == NULL);
    assert(mln_list_tail(&s) == NULL);
    printf("  PASS: remove single\n");
}

static void test_for_each(void)
{
    mln_list_t s;
    mln_list_init(&s);
    test_t nodes[5];
    int i;

    for (i = 0; i < 5; ++i) {
        nodes[i].val = i;
        mln_list_add(&s, &nodes[i].node);
    }

    /* forward iteration using for_each */
    i = 0;
    mln_list_t *lnode;
    mln_list_for_each(lnode, &s) {
        test_t *t = mln_container_of(lnode, test_t, node);
        assert(t->val == i);
        ++i;
    }
    assert(i == 5);
    printf("  PASS: for_each\n");
}

static void test_for_each_safe(void)
{
    mln_list_t s;
    mln_list_init(&s);
    int i;

    test_t *nodes[5];
    for (i = 0; i < 5; ++i) {
        nodes[i] = (test_t *)calloc(1, sizeof(test_t));
        assert(nodes[i] != NULL);
        nodes[i]->val = i;
        mln_list_add(&s, &nodes[i]->node);
    }

    /* remove all during iteration */
    mln_list_t *lnode, *tmp;
    i = 0;
    mln_list_for_each_safe(lnode, tmp, &s) {
        test_t *t = mln_container_of(lnode, test_t, node);
        assert(t->val == i);
        mln_list_remove(&s, &t->node);
        free(t);
        ++i;
    }
    assert(i == 5);
    assert(mln_list_head(&s) == NULL);
    printf("  PASS: for_each_safe\n");
}

static void test_container_of_traversal(void)
{
    mln_list_t s;
    mln_list_init(&s);
    test_t nodes[3];
    int i;

    for (i = 0; i < 3; ++i) {
        nodes[i].val = i * 10;
        mln_list_add(&s, &nodes[i].node);
    }

    /* Traditional traversal using container_of and head/next */
    test_t *t;
    i = 0;
    for (t = mln_container_of(mln_list_head(&s), test_t, node);
         t != NULL;
         t = mln_container_of(mln_list_next(&s, &t->node), test_t, node))
    {
        assert(t->val == i * 10);
        ++i;
    }
    assert(i == 3);
    printf("  PASS: container_of traversal\n");
}

static void test_null_init_add(void)
{
    /* Test backward-compatible mln_list_null() initialization */
    mln_list_t s = mln_list_null();
    test_t a, b;
    a.val = 1; b.val = 2;

    mln_list_add(&s, &a.node);
    assert(mln_list_head(&s) == &a.node);
    assert(mln_list_tail(&s) == &a.node);

    mln_list_add(&s, &b.node);
    assert(mln_list_head(&s) == &a.node);
    assert(mln_list_tail(&s) == &b.node);

    mln_list_remove(&s, &a.node);
    mln_list_remove(&s, &b.node);
    assert(mln_list_head(&s) == NULL);
    assert(mln_list_tail(&s) == NULL);
    printf("  PASS: null init add\n");
}

static void test_add_remove_readd(void)
{
    mln_list_t s;
    mln_list_init(&s);
    test_t a;
    a.val = 1;

    mln_list_add(&s, &a.node);
    assert(mln_list_head(&s) == &a.node);

    mln_list_remove(&s, &a.node);
    assert(mln_list_head(&s) == NULL);

    /* Re-add after removal */
    mln_list_add(&s, &a.node);
    assert(mln_list_head(&s) == &a.node);
    assert(mln_list_tail(&s) == &a.node);
    printf("  PASS: add-remove-readd\n");
}

/* ============================================================ */
/*                     Performance Test                         */
/* ============================================================ */

static void test_performance(void)
{
    int N = 5000000;
    int i, rounds = 3;
    double total = 0;
    double t0, t1;

    test_t *nodes = (test_t *)calloc(N, sizeof(test_t));
    assert(nodes != NULL);

    for (int r = 0; r < rounds; ++r) {
        mln_list_t s;
        mln_list_init(&s);
        t0 = now_sec();
        for (i = 0; i < N; ++i) {
            nodes[i].val = i;
            mln_list_add(&s, &nodes[i].node);
        }
        for (i = 0; i < N; ++i) {
            mln_list_remove(&s, &nodes[i].node);
        }
        t1 = now_sec();
        total += t1 - t0;
    }

    printf("  add/remove (%d ops x %d rounds):\n", N, rounds);
    printf("    Total: %.4f sec\n", total);
    printf("    Avg:   %.4f sec per round\n", total / rounds);
    printf("    Ops:   %.0f ops/sec\n", (double)N * 2 * rounds / total);

    /* Benchmark for_each iteration */
    mln_list_t s;
    mln_list_init(&s);
    for (i = 0; i < N; ++i) {
        nodes[i].val = i;
        mln_list_add(&s, &nodes[i].node);
    }

    total = 0;
    for (int r = 0; r < rounds; ++r) {
        volatile long sum = 0;
        mln_list_t *lnode;
        t0 = now_sec();
        mln_list_for_each(lnode, &s) {
            test_t *t = mln_container_of(lnode, test_t, node);
            sum += t->val;
        }
        t1 = now_sec();
        total += t1 - t0;
    }

    printf("  for_each (%d elements x %d rounds):\n", N, rounds);
    printf("    Total: %.4f sec\n", total);
    printf("    Avg:   %.4f sec per round\n", total / rounds);

    double safe_total = 0;
    for (int r = 0; r < rounds; ++r) {
        volatile long sum = 0;
        mln_list_t *lnode, *tmp;
        t0 = now_sec();
        mln_list_for_each_safe(lnode, tmp, &s) {
            test_t *t = mln_container_of(lnode, test_t, node);
            sum += t->val;
        }
        t1 = now_sec();
        safe_total += t1 - t0;
    }

    printf("  for_each_safe (%d elements x %d rounds):\n", N, rounds);
    printf("    Total: %.4f sec\n", safe_total);
    printf("    Avg:   %.4f sec per round\n", safe_total / rounds);

    for (i = 0; i < N; ++i) {
        mln_list_remove(&s, &nodes[i].node);
    }

    free(nodes);
}

/* ============================================================ */
/*                      Stability Test                          */
/* ============================================================ */

static void test_stability(void)
{
    int N = 100000;
    int i, round;
    mln_list_t s;
    mln_list_init(&s);

    test_t *nodes = (test_t *)calloc(N, sizeof(test_t));
    assert(nodes != NULL);

    for (round = 0; round < 10; ++round) {
        /* Add all */
        for (i = 0; i < N; ++i) {
            nodes[i].val = round * N + i;
            mln_list_add(&s, &nodes[i].node);
        }

        /* Verify count via for_each */
        int count = 0;
        mln_list_t *lnode;
        mln_list_for_each(lnode, &s) {
            count++;
        }
        assert(count == N);

        /* Remove even-indexed in forward pass */
        mln_list_t *tmp;
        mln_list_for_each_safe(lnode, tmp, &s) {
            test_t *t = mln_container_of(lnode, test_t, node);
            if ((t - nodes) % 2 == 0) {
                mln_list_remove(&s, &t->node);
            }
        }

        /* Count remaining */
        count = 0;
        mln_list_for_each(lnode, &s) {
            count++;
        }
        assert(count == N / 2);

        /* Remove rest */
        mln_list_for_each_safe(lnode, tmp, &s) {
            mln_list_remove(&s, lnode);
        }
        assert(mln_list_head(&s) == NULL);
        assert(mln_list_tail(&s) == NULL);
    }

    free(nodes);
    printf("  PASS: stability (%d elements x %d rounds)\n", N, 10);
}

/* ============================================================ */
/*                          Main                                */
/* ============================================================ */

int main(void)
{
    printf("=== Feature Tests ===\n");
    test_init_and_empty();
    test_null_init_and_empty();
    test_add_single();
    test_add_multiple();
    test_remove_head();
    test_remove_tail();
    test_remove_middle();
    test_remove_all();
    test_remove_single();
    test_for_each();
    test_for_each_safe();
    test_container_of_traversal();
    test_null_init_add();
    test_add_remove_readd();

    printf("\n=== Stability Test ===\n");
    test_stability();

    printf("\n=== Performance Test ===\n");
    test_performance();

    printf("\nAll tests passed.\n");
    return 0;
}

