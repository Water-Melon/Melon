#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include "mln_queue.h"

static int test_nr = 0;

#define PASS() do { printf("  #%02d PASS\n", ++test_nr); } while (0)

static int g_free_count = 0;

static void free_counter_fn(void *data)
{
    (void)data;
    g_free_count++;
}

static void test_init_destroy(void)
{
    mln_queue_t *q = mln_queue_init(10, NULL);
    assert(q != NULL);
    assert(mln_queue_empty(q));
    assert(!mln_queue_full(q));
    assert(mln_queue_length(q) == 10);
    assert(mln_queue_element(q) == 0);
    mln_queue_destroy(q);

    q = mln_queue_init(0, NULL);
    assert(q != NULL);
    assert(mln_queue_empty(q));
    assert(mln_queue_full(q));
    assert(mln_queue_length(q) == 0);
    mln_queue_destroy(q);

    mln_queue_destroy(NULL);
    PASS();
}

static void test_append_get_remove(void)
{
    int a = 1, b = 2, c = 3;
    mln_queue_t *q = mln_queue_init(3, NULL);
    assert(q != NULL);

    assert(mln_queue_append(q, &a) == 0);
    assert(mln_queue_append(q, &b) == 0);
    assert(mln_queue_append(q, &c) == 0);
    assert(mln_queue_full(q));
    assert(mln_queue_element(q) == 3);

    assert(*(int *)mln_queue_get(q) == 1);
    mln_queue_remove(q);
    assert(*(int *)mln_queue_get(q) == 2);
    mln_queue_remove(q);
    assert(*(int *)mln_queue_get(q) == 3);
    mln_queue_remove(q);
    assert(mln_queue_empty(q));
    assert(mln_queue_get(q) == NULL);

    mln_queue_destroy(q);
    PASS();
}

static void test_full_queue_reject(void)
{
    int a = 1, b = 2;
    mln_queue_t *q = mln_queue_init(1, NULL);
    assert(q != NULL);
    assert(mln_queue_append(q, &a) == 0);
    assert(mln_queue_append(q, &b) == -1);
    assert(mln_queue_element(q) == 1);
    assert(*(int *)mln_queue_get(q) == 1);
    mln_queue_destroy(q);
    PASS();
}

static void test_remove_empty(void)
{
    mln_queue_t *q = mln_queue_init(4, NULL);
    assert(q != NULL);
    mln_queue_remove(q);
    assert(mln_queue_empty(q));
    assert(mln_queue_get(q) == NULL);
    mln_queue_destroy(q);
    PASS();
}

static void test_wraparound(void)
{
    int vals[] = {10, 20, 30, 40, 50};
    mln_queue_t *q = mln_queue_init(4, NULL);
    assert(q != NULL);

    assert(mln_queue_append(q, &vals[0]) == 0);
    assert(mln_queue_append(q, &vals[1]) == 0);
    assert(mln_queue_append(q, &vals[2]) == 0);
    mln_queue_remove(q);
    mln_queue_remove(q);

    assert(mln_queue_append(q, &vals[3]) == 0);
    assert(mln_queue_append(q, &vals[4]) == 0);
    assert(mln_queue_element(q) == 3);

    assert(*(int *)mln_queue_get(q) == 30);
    mln_queue_remove(q);
    assert(*(int *)mln_queue_get(q) == 40);
    mln_queue_remove(q);
    assert(*(int *)mln_queue_get(q) == 50);
    mln_queue_remove(q);
    assert(mln_queue_empty(q));

    mln_queue_destroy(q);
    PASS();
}

static void test_search(void)
{
    int vals[] = {100, 200, 300, 400};
    mln_queue_t *q = mln_queue_init(4, NULL);
    assert(q != NULL);

    int i;
    for (i = 0; i < 4; ++i)
        assert(mln_queue_append(q, &vals[i]) == 0);

    assert(*(int *)mln_queue_search(q, 0) == 100);
    assert(*(int *)mln_queue_search(q, 1) == 200);
    assert(*(int *)mln_queue_search(q, 2) == 300);
    assert(*(int *)mln_queue_search(q, 3) == 400);
    assert(mln_queue_search(q, 4) == NULL);
    assert(mln_queue_search(q, 100) == NULL);

    mln_queue_destroy(q);
    PASS();
}

static void test_search_wraparound(void)
{
    int vals[] = {1, 2, 3, 4, 5};
    mln_queue_t *q = mln_queue_init(4, NULL);
    assert(q != NULL);

    assert(mln_queue_append(q, &vals[0]) == 0);
    assert(mln_queue_append(q, &vals[1]) == 0);
    assert(mln_queue_append(q, &vals[2]) == 0);
    mln_queue_remove(q);
    mln_queue_remove(q);
    assert(mln_queue_append(q, &vals[3]) == 0);
    assert(mln_queue_append(q, &vals[4]) == 0);

    assert(*(int *)mln_queue_search(q, 0) == 3);
    assert(*(int *)mln_queue_search(q, 1) == 4);
    assert(*(int *)mln_queue_search(q, 2) == 5);

    mln_queue_destroy(q);
    PASS();
}

static int iterate_sum_handler(void *data, void *udata)
{
    *(int *)udata += *(int *)data;
    return 0;
}

static int iterate_abort_handler(void *data, void *udata)
{
    int val = *(int *)data;
    (void)udata;
    if (val > 2) return -1;
    return 0;
}

static void test_iterate(void)
{
    int vals[] = {1, 2, 3, 4};
    int sum = 0;
    mln_queue_t *q = mln_queue_init(4, NULL);
    assert(q != NULL);

    int i;
    for (i = 0; i < 4; ++i)
        assert(mln_queue_append(q, &vals[i]) == 0);

    assert(mln_queue_iterate(q, iterate_sum_handler, &sum) == 0);
    assert(sum == 10);

    assert(mln_queue_iterate(q, iterate_abort_handler, NULL) == -1);

    assert(mln_queue_iterate(q, NULL, NULL) == 0);

    mln_queue_destroy(q);
    PASS();
}

static void test_iterate_wraparound(void)
{
    int vals[] = {10, 20, 30, 40, 50};
    int sum = 0;
    mln_queue_t *q = mln_queue_init(4, NULL);
    assert(q != NULL);

    int i;
    for (i = 0; i < 3; ++i)
        assert(mln_queue_append(q, &vals[i]) == 0);
    mln_queue_remove(q);
    mln_queue_remove(q);
    assert(mln_queue_append(q, &vals[3]) == 0);
    assert(mln_queue_append(q, &vals[4]) == 0);

    assert(mln_queue_iterate(q, iterate_sum_handler, &sum) == 0);
    assert(sum == 30 + 40 + 50);

    mln_queue_destroy(q);
    PASS();
}

static void test_free_index_head(void)
{
    int vals[] = {1, 2, 3, 4};
    int i;

    g_free_count = 0;
    mln_queue_t *q = mln_queue_init(4, free_counter_fn);
    assert(q != NULL);
    for (i = 0; i < 4; ++i)
        assert(mln_queue_append(q, &vals[i]) == 0);

    mln_queue_free_index(q, 0);
    assert(mln_queue_element(q) == 3);
    assert(g_free_count == 1);
    assert(*(int *)mln_queue_search(q, 0) == 2);
    assert(*(int *)mln_queue_search(q, 1) == 3);
    assert(*(int *)mln_queue_search(q, 2) == 4);

    mln_queue_destroy(q);
    PASS();
}

static void test_free_index_tail(void)
{
    int vals[] = {1, 2, 3, 4};
    int i;

    g_free_count = 0;
    mln_queue_t *q = mln_queue_init(4, free_counter_fn);
    assert(q != NULL);
    for (i = 0; i < 4; ++i)
        assert(mln_queue_append(q, &vals[i]) == 0);

    mln_queue_free_index(q, 3);
    assert(mln_queue_element(q) == 3);
    assert(g_free_count == 1);
    assert(*(int *)mln_queue_search(q, 0) == 1);
    assert(*(int *)mln_queue_search(q, 1) == 2);
    assert(*(int *)mln_queue_search(q, 2) == 3);

    mln_queue_destroy(q);
    PASS();
}

static void test_free_index_middle(void)
{
    int vals[] = {1, 2, 3, 4};
    int i;

    g_free_count = 0;
    mln_queue_t *q = mln_queue_init(4, free_counter_fn);
    assert(q != NULL);
    for (i = 0; i < 4; ++i)
        assert(mln_queue_append(q, &vals[i]) == 0);

    mln_queue_free_index(q, 1);
    assert(mln_queue_element(q) == 3);
    assert(g_free_count == 1);
    assert(*(int *)mln_queue_search(q, 0) == 1);
    assert(*(int *)mln_queue_search(q, 1) == 3);
    assert(*(int *)mln_queue_search(q, 2) == 4);

    mln_queue_destroy(q);
    PASS();
}

static void test_free_index_out_of_range(void)
{
    int a = 1;
    mln_queue_t *q = mln_queue_init(4, NULL);
    assert(q != NULL);
    assert(mln_queue_append(q, &a) == 0);
    mln_queue_free_index(q, 5);
    assert(mln_queue_element(q) == 1);
    mln_queue_destroy(q);
    PASS();
}

static void test_free_index_wraparound(void)
{
    int vals[] = {1, 2, 3, 4, 5, 6};
    mln_queue_t *q = mln_queue_init(4, NULL);
    assert(q != NULL);

    int i;
    for (i = 0; i < 3; ++i)
        assert(mln_queue_append(q, &vals[i]) == 0);
    mln_queue_remove(q);
    mln_queue_remove(q);
    for (i = 3; i < 6; ++i)
        assert(mln_queue_append(q, &vals[i]) == 0);
    assert(mln_queue_element(q) == 4);

    mln_queue_free_index(q, 1);
    assert(mln_queue_element(q) == 3);
    assert(*(int *)mln_queue_search(q, 0) == 3);
    assert(*(int *)mln_queue_search(q, 1) == 5);
    assert(*(int *)mln_queue_search(q, 2) == 6);

    mln_queue_destroy(q);
    PASS();
}

static void test_destroy_with_free_handler(void)
{
    int dummy = 0;
    g_free_count = 0;
    mln_queue_t *q = mln_queue_init(3, free_counter_fn);
    assert(q != NULL);
    assert(mln_queue_append(q, &dummy) == 0);
    assert(mln_queue_append(q, &dummy) == 0);
    assert(mln_queue_append(q, &dummy) == 0);
    mln_queue_destroy(q);
    assert(g_free_count == 3);
    PASS();
}

static void test_single_element(void)
{
    int a = 42;
    mln_queue_t *q = mln_queue_init(1, NULL);
    assert(q != NULL);
    assert(mln_queue_append(q, &a) == 0);
    assert(mln_queue_full(q));
    assert(*(int *)mln_queue_get(q) == 42);
    assert(*(int *)mln_queue_search(q, 0) == 42);
    mln_queue_remove(q);
    assert(mln_queue_empty(q));
    mln_queue_destroy(q);
    PASS();
}

static void test_refill_after_drain(void)
{
    int vals[] = {1, 2, 3, 4, 5, 6};
    mln_queue_t *q = mln_queue_init(3, NULL);
    assert(q != NULL);

    int i;
    for (i = 0; i < 3; ++i)
        assert(mln_queue_append(q, &vals[i]) == 0);
    for (i = 0; i < 3; ++i)
        mln_queue_remove(q);
    assert(mln_queue_empty(q));

    for (i = 3; i < 6; ++i)
        assert(mln_queue_append(q, &vals[i]) == 0);
    assert(mln_queue_full(q));

    assert(*(int *)mln_queue_get(q) == 4);
    mln_queue_remove(q);
    assert(*(int *)mln_queue_get(q) == 5);
    mln_queue_remove(q);
    assert(*(int *)mln_queue_get(q) == 6);

    mln_queue_destroy(q);
    PASS();
}

static void test_large_queue(void)
{
    mln_uauto_t n = 10000;
    mln_uauto_t i;
    mln_queue_t *q = mln_queue_init(n, NULL);
    assert(q != NULL);

    for (i = 0; i < n; ++i)
        assert(mln_queue_append(q, (void *)(mln_uauto_t)(i + 1)) == 0);
    assert(mln_queue_full(q));

    for (i = 0; i < n; ++i) {
        void *v = mln_queue_search(q, i);
        assert((mln_uauto_t)v == i + 1);
    }

    for (i = 0; i < n; ++i) {
        void *v = mln_queue_get(q);
        assert((mln_uauto_t)v == i + 1);
        mln_queue_remove(q);
    }
    assert(mln_queue_empty(q));

    mln_queue_destroy(q);
    PASS();
}

static void test_iterate_empty(void)
{
    mln_queue_t *q = mln_queue_init(4, NULL);
    assert(q != NULL);
    assert(mln_queue_iterate(q, iterate_sum_handler, NULL) == 0);
    mln_queue_destroy(q);
    PASS();
}

static void test_free_index_then_append(void)
{
    int vals[] = {1, 2, 3};
    int val4 = 4;
    mln_queue_t *q = mln_queue_init(3, NULL);
    assert(q != NULL);

    int i;
    for (i = 0; i < 3; ++i)
        assert(mln_queue_append(q, &vals[i]) == 0);

    mln_queue_free_index(q, 1);
    assert(mln_queue_element(q) == 2);

    assert(mln_queue_append(q, &val4) == 0);
    assert(mln_queue_element(q) == 3);
    assert(*(int *)mln_queue_search(q, 0) == 1);
    assert(*(int *)mln_queue_search(q, 1) == 3);
    assert(*(int *)mln_queue_search(q, 2) == 4);

    mln_queue_destroy(q);
    PASS();
}

static double elapsed_ns(struct timespec *t0, struct timespec *t1)
{
    return (t1->tv_sec - t0->tv_sec) * 1e9 + (t1->tv_nsec - t0->tv_nsec);
}

static int noop_handler(void *data, void *udata)
{
    (void)data; (void)udata;
    return 0;
}

static void test_benchmark(void)
{
    struct timespec t0, t1;
    int iters = 5000000;
    int i;
    double ns;
    mln_queue_t *q;
    int dummy = 42;
    volatile void *sink;

    printf("  Performance benchmark:\n");

    q = mln_queue_init(1024, NULL);
    clock_gettime(CLOCK_MONOTONIC, &t0);
    for (i = 0; i < iters; ++i) {
        mln_queue_append(q, &dummy);
        mln_queue_remove(q);
    }
    clock_gettime(CLOCK_MONOTONIC, &t1);
    ns = elapsed_ns(&t0, &t1);
    printf("    append+remove:  %.1f ns/op (%d iters)\n", ns / iters, iters);
    mln_queue_destroy(q);

    q = mln_queue_init(1024, NULL);
    for (i = 0; i < 1024; ++i) mln_queue_append(q, &dummy);
    clock_gettime(CLOCK_MONOTONIC, &t0);
    for (i = 0; i < iters; ++i) {
        sink = mln_queue_search(q, i & 1023);
    }
    clock_gettime(CLOCK_MONOTONIC, &t1);
    ns = elapsed_ns(&t0, &t1);
    printf("    search:         %.1f ns/op (%d iters)\n", ns / iters, iters);
    (void)sink;
    mln_queue_destroy(q);

    q = mln_queue_init(1024, NULL);
    for (i = 0; i < 1024; ++i) mln_queue_append(q, &dummy);
    iters = 100000;
    clock_gettime(CLOCK_MONOTONIC, &t0);
    for (i = 0; i < iters; ++i) {
        mln_queue_iterate(q, noop_handler, NULL);
    }
    clock_gettime(CLOCK_MONOTONIC, &t1);
    ns = elapsed_ns(&t0, &t1);
    printf("    iterate(1024):  %.1f ns/op (%d iters)\n", ns / iters, iters);
    mln_queue_destroy(q);

    PASS();
}

static void test_stability(void)
{
    int rounds = 100000;
    int i;
    mln_uauto_t j;
    mln_queue_t *q = mln_queue_init(64, NULL);
    assert(q != NULL);

    for (i = 0; i < rounds; ++i) {
        for (j = 0; j < 64; ++j)
            assert(mln_queue_append(q, (void *)(mln_uauto_t)(j + 1)) == 0);
        assert(mln_queue_full(q));
        for (j = 0; j < 64; ++j) {
            assert((mln_uauto_t)mln_queue_get(q) == j + 1);
            mln_queue_remove(q);
        }
        assert(mln_queue_empty(q));
    }

    for (j = 0; j < 32; ++j)
        assert(mln_queue_append(q, (void *)(mln_uauto_t)(j + 1)) == 0);
    for (i = 0; i < rounds; ++i) {
        mln_queue_free_index(q, 16);
        assert(mln_queue_append(q, (void *)(mln_uauto_t)99) == 0);
        assert(mln_queue_element(q) == 32);
    }

    mln_queue_destroy(q);
    PASS();
}

static mln_queue_t *g_iter_q = NULL;

/* Callback that removes the current element using free_index */
static int iterate_remove_current_handler(void *data, void *udata)
{
    int val = *(int *)data;
    int **visited = (int **)udata;
    **visited = val;
    (*visited)++;

    /* Find and remove the current element by searching for it */
    mln_uauto_t i;
    for (i = 0; i < mln_queue_element(g_iter_q); ++i) {
        if (mln_queue_search(g_iter_q, i) == data) {
            mln_queue_free_index(g_iter_q, i);
            break;
        }
    }
    return 0;
}

static void test_iterate_remove_current(void)
{
    int vals[] = {1, 2, 3, 4, 5};
    int visited[5] = {0};
    int *vp = visited;
    int i;

    mln_queue_t *q = mln_queue_init(5, NULL);
    assert(q != NULL);
    for (i = 0; i < 5; ++i)
        assert(mln_queue_append(q, &vals[i]) == 0);

    g_iter_q = q;
    assert(mln_queue_iterate(q, iterate_remove_current_handler, &vp) == 0);

    /* All 5 elements should have been visited */
    assert(vp - visited == 5);
    assert(visited[0] == 1);
    assert(visited[1] == 2);
    assert(visited[2] == 3);
    assert(visited[3] == 4);
    assert(visited[4] == 5);
    /* Queue should be empty now */
    assert(mln_queue_empty(q));

    mln_queue_destroy(q);
    g_iter_q = NULL;
    PASS();
}

/* Callback that removes the next element */
static int iterate_remove_next_handler(void *data, void *udata)
{
    int val = *(int *)data;
    int **visited = (int **)udata;
    **visited = val;
    (*visited)++;

    /* Remove element at index 1 (next) if more than 1 element remains */
    if (mln_queue_element(g_iter_q) > 1) {
        mln_queue_free_index(g_iter_q, 1);
    }
    return 0;
}

static void test_iterate_remove_next(void)
{
    int vals[] = {1, 2, 3, 4, 5};
    int visited[5] = {0};
    int *vp = visited;
    int i;

    mln_queue_t *q = mln_queue_init(5, NULL);
    assert(q != NULL);
    for (i = 0; i < 5; ++i)
        assert(mln_queue_append(q, &vals[i]) == 0);

    g_iter_q = q;
    assert(mln_queue_iterate(q, iterate_remove_next_handler, &vp) == 0);

    /*
     * Iteration visits: 1 (removes 2), 3 (removes 4), 5.
     * Visited: [1, 3, 5], Remaining in queue: [1, 3, 5] with the removed ones gone.
     * Wait — removing from position 1 while iterating at position 0 means
     * position 1 shifts, but index > iter_i so iter_i is NOT adjusted.
     * After callback at index 0 (val 1): ++iter_i -> 1. Element at 1 was 3 (since 2 was removed).
     * Callback at index 1 (val 3): removes index 1 (which is now 3 itself? No!)
     *
     * Let me re-trace: queue starts [1,2,3,4,5]
     * iter_i=0: visit 1. Remove index 1 (value 2). Queue: [1,3,4,5]. index(1) > iter_i(0) -> no adjust.
     * ++iter_i -> 1.
     * iter_i=1: visit 3. Remove index 1 (value 3 itself). Queue: [1,4,5].
     *   free_index(q, 1): index(1) <= iter_i(1) -> --iter_i -> 0.
     * ++iter_i -> 1.
     * iter_i=1: visit 4. Remove index 1 (value 4 itself). Queue: [1,5].
     *   free_index(q, 1): index(1) <= iter_i(1) -> --iter_i -> 0.
     * ++iter_i -> 1.
     * iter_i=1: visit 5. Remove index 1? Only 2 elements [1,5]. Remove index 1 = value 5.
     *   Queue: [1]. free_index(q,1): index(1) <= iter_i(1) -> --iter_i -> 0.
     * ++iter_i -> 1.
     * 1 < 1? No. Exit.
     * Visited: 1, 3, 4, 5. Value 2 was removed without visiting.
     * That's correct! Element at index 1 (the "next" after current head) was removed,
     * and the element that slid in was visited next.
     */
    int count = (int)(vp - visited);
    assert(count == 4); /* 1, 3, 4, 5 — value 2 removed before being visited */
    assert(visited[0] == 1);
    assert(visited[1] == 3);
    assert(visited[2] == 4);
    assert(visited[3] == 5);
    assert(mln_queue_element(q) == 1); /* only value 1 remains */
    assert(*(int *)mln_queue_get(q) == 1);

    mln_queue_destroy(q);
    g_iter_q = NULL;
    PASS();
}

/* Callback that uses mln_queue_remove (removes head) during iteration */
static int iterate_remove_head_handler(void *data, void *udata)
{
    int val = *(int *)data;
    int **visited = (int **)udata;
    **visited = val;
    (*visited)++;

    /* Always remove from head */
    mln_queue_remove(g_iter_q);
    return 0;
}

static void test_iterate_remove_via_remove(void)
{
    int vals[] = {1, 2, 3, 4, 5};
    int visited[5] = {0};
    int *vp = visited;
    int i;

    mln_queue_t *q = mln_queue_init(5, NULL);
    assert(q != NULL);
    for (i = 0; i < 5; ++i)
        assert(mln_queue_append(q, &vals[i]) == 0);

    g_iter_q = q;
    assert(mln_queue_iterate(q, iterate_remove_head_handler, &vp) == 0);

    /*
     * iter_i=0: visit 1. remove() -> head advances, --iter_i -> -1. ++iter_i -> 0.
     * Queue: [2,3,4,5]. iter_i=0: visit 2. remove() -> --iter_i -> -1. ++iter_i -> 0.
     * Queue: [3,4,5]. iter_i=0: visit 3. ... etc.
     * All 5 visited, queue empty.
     */
    assert(vp - visited == 5);
    assert(visited[0] == 1);
    assert(visited[1] == 2);
    assert(visited[2] == 3);
    assert(visited[3] == 4);
    assert(visited[4] == 5);
    assert(mln_queue_empty(q));

    mln_queue_destroy(q);
    g_iter_q = NULL;
    PASS();
}

/* Callback that removes a previous element (before current) */
static int iterate_remove_prev_handler(void *data, void *udata)
{
    int val = *(int *)data;
    int **visited = (int **)udata;
    **visited = val;
    (*visited)++;

    /* If visiting element with value >= 3, remove index 0 (head) via free_index */
    if (val >= 3 && mln_queue_element(g_iter_q) > 1) {
        mln_queue_free_index(g_iter_q, 0);
    }
    return 0;
}

static void test_iterate_remove_previous(void)
{
    int vals[] = {1, 2, 3, 4, 5};
    int visited[10] = {0};
    int *vp = visited;
    int i;

    mln_queue_t *q = mln_queue_init(5, NULL);
    assert(q != NULL);
    for (i = 0; i < 5; ++i)
        assert(mln_queue_append(q, &vals[i]) == 0);

    g_iter_q = q;
    assert(mln_queue_iterate(q, iterate_remove_prev_handler, &vp) == 0);

    /*
     * Queue: [1,2,3,4,5]
     * iter_i=0: visit 1 (val<3, no remove). ++iter_i -> 1.
     * iter_i=1: visit 2 (val<3, no remove). ++iter_i -> 2.
     * iter_i=2: visit 3 (val>=3). free_index(q,0) removes 1. Queue: [2,3,4,5].
     *   index(0) <= iter_i(2) -> --iter_i -> 1. ++iter_i -> 2.
     * iter_i=2: visit 4 (val>=3). free_index(q,0) removes 2. Queue: [3,4,5].
     *   index(0) <= iter_i(2) -> --iter_i -> 1. ++iter_i -> 2.
     * iter_i=2: visit 5 (val>=3). free_index(q,0) removes 3. Queue: [4,5].
     *   index(0) <= iter_i(2) -> --iter_i -> 1. ++iter_i -> 2.
     * 2 < 2? No. Exit.
     * Visited: 1, 2, 3, 4, 5. All visited!
     */
    int count = (int)(vp - visited);
    assert(count == 5);
    assert(visited[0] == 1);
    assert(visited[1] == 2);
    assert(visited[2] == 3);
    assert(visited[3] == 4);
    assert(visited[4] == 5);
    assert(mln_queue_element(q) == 2); /* 4 and 5 remain */

    mln_queue_destroy(q);
    g_iter_q = NULL;
    PASS();
}

/* Callback that removes both current and next simultaneously */
static int iterate_remove_current_and_next_handler(void *data, void *udata)
{
    int val = *(int *)data;
    int **visited = (int **)udata;
    **visited = val;
    (*visited)++;

    /* Remove current (index 0) and then "next" (new index 0 after shift) */
    if (mln_queue_element(g_iter_q) > 0) {
        /* Find current by scanning */
        mln_uauto_t i;
        for (i = 0; i < mln_queue_element(g_iter_q); ++i) {
            if (mln_queue_search(g_iter_q, i) == data) {
                mln_queue_free_index(g_iter_q, i);
                break;
            }
        }
    }
    /* Remove the element that is now at the same position (was "next") */
    if (mln_queue_element(g_iter_q) > 0) {
        mln_uauto_t i;
        for (i = 0; i < mln_queue_element(g_iter_q); ++i) {
            void *p = mln_queue_search(g_iter_q, i);
            if (p != NULL && *(int *)p == val + 1) {
                mln_queue_free_index(g_iter_q, i);
                break;
            }
        }
    }
    return 0;
}

static void test_iterate_remove_current_and_next(void)
{
    int vals[] = {1, 2, 3, 4, 5, 6};
    int visited[6] = {0};
    int *vp = visited;
    int i;

    mln_queue_t *q = mln_queue_init(6, NULL);
    assert(q != NULL);
    for (i = 0; i < 6; ++i)
        assert(mln_queue_append(q, &vals[i]) == 0);

    g_iter_q = q;
    assert(mln_queue_iterate(q, iterate_remove_current_and_next_handler, &vp) == 0);

    /*
     * Queue: [1,2,3,4,5,6]
     * iter_i=0: visit 1. Remove 1 (index 0, <= iter_i(0) -> --iter_i=-1).
     *   Queue: [2,3,4,5,6]. Remove 2 (search finds at index 0, <= iter_i(-1 as signed)? 
     *   No! iter_i < 0 so no adjustment). Queue: [3,4,5,6].
     *   ++iter_i -> 0.
     * iter_i=0: visit 3. Remove 3 and 4. Queue: [5,6]. iter_i -> 0 after adjustments.
     *   ++iter_i -> ... depends on exact adjustments. Let me just check visited count.
     */
    int count = (int)(vp - visited);
    /* Should visit 1, 3, 5 (each removes self and next) */
    assert(count == 3);
    assert(visited[0] == 1);
    assert(visited[1] == 3);
    assert(visited[2] == 5);
    assert(mln_queue_empty(q));

    mln_queue_destroy(q);
    g_iter_q = NULL;
    PASS();
}

int main(int argc, char *argv[])
{
    (void)argc; (void)argv;

    printf("Queue test suite:\n");

    test_init_destroy();
    test_append_get_remove();
    test_full_queue_reject();
    test_remove_empty();
    test_wraparound();
    test_search();
    test_search_wraparound();
    test_iterate();
    test_iterate_wraparound();
    test_iterate_empty();
    test_free_index_head();
    test_free_index_tail();
    test_free_index_middle();
    test_free_index_out_of_range();
    test_free_index_wraparound();
    test_free_index_then_append();
    test_destroy_with_free_handler();
    test_single_element();
    test_refill_after_drain();
    test_large_queue();
    test_iterate_remove_current();
    test_iterate_remove_next();
    test_iterate_remove_via_remove();
    test_iterate_remove_previous();
    test_iterate_remove_current_and_next();
    test_benchmark();
    test_stability();

    printf("All %d tests passed.\n", test_nr);
    return 0;
}

