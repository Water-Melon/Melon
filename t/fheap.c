#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include "mln_fheap.h"

/*
 * Helper types
 */
typedef struct user_defined_s {
    int val;
    mln_fheap_node_t node;
} ud_t;

/*
 * Callback functions
 */
static int cmp_handler(const void *key1, const void *key2)
{
    return *(int *)key1 < *(int *)key2 ? 0 : 1;
}

static void copy_handler(void *old_key, void *new_key)
{
    *(int *)old_key = *(int *)new_key;
}

static void key_free_handler(void *key)
{
    free(key);
}

static inline int inline_cmp_handler(const void *key1, const void *key2)
{
    return *(int *)key1 < *(int *)key2 ? 0 : 1;
}

static inline void inline_copy_handler(void *old_key, void *new_key)
{
    *(int *)old_key = *(int *)new_key;
}

static inline int container_cmp_handler(const void *key1, const void *key2)
{
    return ((ud_t *)key1)->val < ((ud_t *)key2)->val ? 0 : 1;
}

static inline void container_copy_handler(void *old_key, void *new_key)
{
    ((ud_t *)old_key)->val = ((ud_t *)new_key)->val;
}

/*
 * Test 1: Basic API - new, insert, minimum, extract_min, node_new, node_free, free
 */
int test_basic_api(void)
{
    int min = 0;
    int vals[] = {5, 3, 7, 1, 9, 2, 8, 4, 6, 10};
    int n = sizeof(vals) / sizeof(vals[0]);
    int i;
    mln_fheap_t *fh;
    mln_fheap_node_t *fn;
    struct mln_fheap_attr fattr;

    fattr.pool = NULL;
    fattr.pool_alloc = NULL;
    fattr.pool_free = NULL;
    fattr.cmp = cmp_handler;
    fattr.copy = copy_handler;
    fattr.key_free = NULL;

    fh = mln_fheap_new(&min, &fattr);
    assert(fh != NULL);

    for (i = 0; i < n; i++) {
        fn = mln_fheap_node_new(fh, &vals[i]);
        assert(fn != NULL);
        mln_fheap_insert(fh, fn);
    }

    /* minimum should be 1 */
    fn = mln_fheap_minimum(fh);
    assert(fn != NULL);
    assert(*(int *)mln_fheap_node_key(fn) == 1);

    /* extract all in sorted order */
    int prev = -1;
    for (i = 0; i < n; i++) {
        fn = mln_fheap_extract_min(fh);
        assert(fn != NULL);
        assert(*(int *)mln_fheap_node_key(fn) >= prev);
        prev = *(int *)mln_fheap_node_key(fn);
        mln_fheap_node_free(fh, fn);
    }

    /* heap empty */
    assert(mln_fheap_minimum(fh) == NULL);
    assert(mln_fheap_extract_min(fh) == NULL);

    mln_fheap_free(fh);
    printf("  test_basic_api PASSED\n");
    return 0;
}

/*
 * Test 2: new_fast API
 */
int test_new_fast(void)
{
    int min = 0;
    int vals[] = {30, 10, 20};
    int i;
    mln_fheap_t *fh;
    mln_fheap_node_t *fn;

    fh = mln_fheap_new_fast(&min, cmp_handler, copy_handler, NULL);
    assert(fh != NULL);

    for (i = 0; i < 3; i++) {
        fn = mln_fheap_node_new(fh, &vals[i]);
        assert(fn != NULL);
        mln_fheap_insert(fh, fn);
    }

    fn = mln_fheap_extract_min(fh);
    assert(fn != NULL);
    assert(*(int *)mln_fheap_node_key(fn) == 10);
    mln_fheap_node_free(fh, fn);

    mln_fheap_free(fh);
    printf("  test_new_fast PASSED\n");
    return 0;
}

/*
 * Test 3: decrease_key
 */
int test_decrease_key(void)
{
    int min = 0;
    int v1 = 10, v2 = 20, v3 = 30;
    int new_key = 5;
    int bad_key = 25;
    mln_fheap_t *fh;
    mln_fheap_node_t *fn1, *fn2, *fn3, *fn;
    struct mln_fheap_attr fattr;

    fattr.pool = NULL;
    fattr.pool_alloc = NULL;
    fattr.pool_free = NULL;
    fattr.cmp = cmp_handler;
    fattr.copy = copy_handler;
    fattr.key_free = NULL;

    fh = mln_fheap_new(&min, &fattr);
    assert(fh != NULL);

    fn1 = mln_fheap_node_new(fh, &v1);
    fn2 = mln_fheap_node_new(fh, &v2);
    fn3 = mln_fheap_node_new(fh, &v3);
    assert(fn1 && fn2 && fn3);

    mln_fheap_insert(fh, fn1);
    mln_fheap_insert(fh, fn2);
    mln_fheap_insert(fh, fn3);

    /* decrease fn3 (30) to 5, should become new min */
    assert(mln_fheap_decrease_key(fh, fn3, &new_key) == 0);
    fn = mln_fheap_minimum(fh);
    assert(fn == fn3);
    assert(*(int *)mln_fheap_node_key(fn) == 5);

    /* decrease with larger key should fail */
    assert(mln_fheap_decrease_key(fh, fn2, &bad_key) == -1);

    mln_fheap_free(fh);
    printf("  test_decrease_key PASSED\n");
    return 0;
}

/*
 * Test 4: delete
 */
int test_delete(void)
{
    int min = 0;
    int v1 = 10, v2 = 20, v3 = 30;
    mln_fheap_t *fh;
    mln_fheap_node_t *fn1, *fn2, *fn3, *fn;
    struct mln_fheap_attr fattr;

    fattr.pool = NULL;
    fattr.pool_alloc = NULL;
    fattr.pool_free = NULL;
    fattr.cmp = cmp_handler;
    fattr.copy = copy_handler;
    fattr.key_free = NULL;

    fh = mln_fheap_new(&min, &fattr);
    assert(fh != NULL);

    fn1 = mln_fheap_node_new(fh, &v1);
    fn2 = mln_fheap_node_new(fh, &v2);
    fn3 = mln_fheap_node_new(fh, &v3);
    assert(fn1 && fn2 && fn3);

    mln_fheap_insert(fh, fn1);
    mln_fheap_insert(fh, fn2);
    mln_fheap_insert(fh, fn3);

    /* delete the middle element */
    mln_fheap_delete(fh, fn2);
    mln_fheap_node_free(fh, fn2);

    /* min should still be 10 */
    fn = mln_fheap_minimum(fh);
    assert(*(int *)mln_fheap_node_key(fn) == 10);

    /* extract should give 10 then 30 */
    fn = mln_fheap_extract_min(fh);
    assert(*(int *)mln_fheap_node_key(fn) == 10);
    mln_fheap_node_free(fh, fn);

    fn = mln_fheap_extract_min(fh);
    assert(*(int *)mln_fheap_node_key(fn) == 30);
    mln_fheap_node_free(fh, fn);

    assert(mln_fheap_extract_min(fh) == NULL);

    mln_fheap_free(fh);
    printf("  test_delete PASSED\n");
    return 0;
}

/*
 * Test 5: Inline operations
 */
#if !defined(MSVC)
int test_inline_ops(void)
{
    int min = 0;
    int vals[] = {50, 30, 70, 10, 90, 20};
    int n = sizeof(vals) / sizeof(vals[0]);
    int i, prev;
    mln_fheap_t *fh;
    mln_fheap_node_t *fn;

    fh = mln_fheap_new(&min, NULL);
    assert(fh != NULL);

    for (i = 0; i < n; i++) {
        fn = mln_fheap_node_new(fh, &vals[i]);
        assert(fn != NULL);
        mln_fheap_inline_insert(fh, fn, inline_cmp_handler);
    }

    fn = mln_fheap_minimum(fh);
    assert(*(int *)mln_fheap_node_key(fn) == 10);

    prev = -1;
    for (i = 0; i < n; i++) {
        fn = mln_fheap_inline_extract_min(fh, inline_cmp_handler);
        assert(fn != NULL);
        assert(*(int *)mln_fheap_node_key(fn) >= prev);
        prev = *(int *)mln_fheap_node_key(fn);
        mln_fheap_inline_node_free(fh, fn, NULL);
    }

    assert(mln_fheap_minimum(fh) == NULL);
    mln_fheap_inline_free(fh, inline_cmp_handler, NULL);
    printf("  test_inline_ops PASSED\n");
    return 0;
}
#else
int test_inline_ops(void)
{
    printf("  test_inline_ops SKIPPED (MSVC)\n");
    return 0;
}
#endif

/*
 * Test 6: Inline decrease_key and delete
 */
#if !defined(MSVC)
int test_inline_decrease_delete(void)
{
    int min = 0;
    int v1 = 100, v2 = 200, v3 = 300;
    int new_key = 50;
    mln_fheap_t *fh;
    mln_fheap_node_t *fn1, *fn2, *fn3, *fn;

    fh = mln_fheap_new(&min, NULL);
    assert(fh != NULL);

    fn1 = mln_fheap_node_new(fh, &v1);
    fn2 = mln_fheap_node_new(fh, &v2);
    fn3 = mln_fheap_node_new(fh, &v3);
    assert(fn1 && fn2 && fn3);

    mln_fheap_inline_insert(fh, fn1, inline_cmp_handler);
    mln_fheap_inline_insert(fh, fn2, inline_cmp_handler);
    mln_fheap_inline_insert(fh, fn3, inline_cmp_handler);

    /* inline decrease key */
    assert(mln_fheap_inline_decrease_key(fh, fn3, &new_key, inline_copy_handler, inline_cmp_handler) == 0);
    fn = mln_fheap_minimum(fh);
    assert(*(int *)mln_fheap_node_key(fn) == 50);

    /* inline delete */
    mln_fheap_inline_delete(fh, fn2, inline_copy_handler, inline_cmp_handler);
    mln_fheap_inline_node_free(fh, fn2, NULL);

    /* extract remaining sorted: 50, 100 */
    fn = mln_fheap_inline_extract_min(fh, inline_cmp_handler);
    assert(*(int *)mln_fheap_node_key(fn) == 50);
    mln_fheap_inline_node_free(fh, fn, NULL);

    fn = mln_fheap_inline_extract_min(fh, inline_cmp_handler);
    assert(*(int *)mln_fheap_node_key(fn) == 100);
    mln_fheap_inline_node_free(fh, fn, NULL);

    assert(mln_fheap_inline_extract_min(fh, inline_cmp_handler) == NULL);
    mln_fheap_inline_free(fh, inline_cmp_handler, NULL);
    printf("  test_inline_decrease_delete PASSED\n");
    return 0;
}
#else
int test_inline_decrease_delete(void)
{
    printf("  test_inline_decrease_delete SKIPPED (MSVC)\n");
    return 0;
}
#endif

/*
 * Test 7: Container usage
 */
#if !defined(MSVC)
int test_container_usage(void)
{
    mln_fheap_t *fh;
    ud_t min_ud = {0, };
    ud_t data[5];
    int vals[] = {50, 10, 30, 20, 40};
    int i;
    mln_fheap_node_t *fn;

    fh = mln_fheap_new(&min_ud, NULL);
    assert(fh != NULL);

    for (i = 0; i < 5; i++) {
        data[i].val = vals[i];
        mln_fheap_node_init(&data[i].node, &data[i]);
        mln_fheap_inline_insert(fh, &data[i].node, container_cmp_handler);
    }

    /* minimum should be 10 */
    fn = mln_fheap_minimum(fh);
    assert(fn != NULL);
    assert(((ud_t *)mln_fheap_node_key(fn))->val == 10);
    assert(mln_container_of(fn, ud_t, node)->val == 10);

    /* extract all in sorted order */
    int prev = -1;
    for (i = 0; i < 5; i++) {
        fn = mln_fheap_inline_extract_min(fh, container_cmp_handler);
        assert(fn != NULL);
        ud_t *ud = mln_container_of(fn, ud_t, node);
        assert(ud->val >= prev);
        prev = ud->val;
    }

    /* container decrease_key */
    ud_t d1 = {100, }, d2 = {200, };
    ud_t new_k = {50, };
    mln_fheap_node_init(&d1.node, &d1);
    mln_fheap_node_init(&d2.node, &d2);
    mln_fheap_inline_insert(fh, &d1.node, container_cmp_handler);
    mln_fheap_inline_insert(fh, &d2.node, container_cmp_handler);

    assert(mln_fheap_inline_decrease_key(fh, &d2.node, &new_k, container_copy_handler, container_cmp_handler) == 0);
    fn = mln_fheap_minimum(fh);
    assert(mln_container_of(fn, ud_t, node)->val == 50);

    mln_fheap_inline_free(fh, container_cmp_handler, NULL);
    printf("  test_container_usage PASSED\n");
    return 0;
}
#else
int test_container_usage(void)
{
    printf("  test_container_usage SKIPPED (MSVC)\n");
    return 0;
}
#endif

/*
 * Test 8: Empty heap operations
 */
int test_empty_heap(void)
{
    int min = 0;
    mln_fheap_t *fh;

    fh = mln_fheap_new(&min, NULL);
    assert(fh != NULL);

    assert(mln_fheap_minimum(fh) == NULL);
    assert(mln_fheap_extract_min(fh) == NULL);

    mln_fheap_free(fh);

    /* free NULL should not crash */
    mln_fheap_free(NULL);

    printf("  test_empty_heap PASSED\n");
    return 0;
}

/*
 * Test 9: Single element
 */
int test_single_element(void)
{
    int min = 0, val = 42;
    mln_fheap_t *fh;
    mln_fheap_node_t *fn;
    struct mln_fheap_attr fattr;

    fattr.pool = NULL;
    fattr.pool_alloc = NULL;
    fattr.pool_free = NULL;
    fattr.cmp = cmp_handler;
    fattr.copy = copy_handler;
    fattr.key_free = NULL;

    fh = mln_fheap_new(&min, &fattr);
    assert(fh != NULL);

    fn = mln_fheap_node_new(fh, &val);
    assert(fn != NULL);
    mln_fheap_insert(fh, fn);

    assert(mln_fheap_minimum(fh) == fn);
    assert(*(int *)mln_fheap_node_key(fn) == 42);

    fn = mln_fheap_extract_min(fh);
    assert(fn != NULL);
    assert(*(int *)mln_fheap_node_key(fn) == 42);
    mln_fheap_node_free(fh, fn);

    assert(mln_fheap_minimum(fh) == NULL);
    mln_fheap_free(fh);
    printf("  test_single_element PASSED\n");
    return 0;
}

/*
 * Test 10: Ordering correctness with many elements
 */
int test_ordering(void)
{
    int min = 0;
    int n = 1000;
    int *vals;
    int i, prev;
    mln_fheap_t *fh;
    mln_fheap_node_t *fn;
    struct mln_fheap_attr fattr;

    fattr.pool = NULL;
    fattr.pool_alloc = NULL;
    fattr.pool_free = NULL;
    fattr.cmp = cmp_handler;
    fattr.copy = copy_handler;
    fattr.key_free = NULL;

    vals = (int *)malloc(sizeof(int) * n);
    assert(vals != NULL);
    srand(12345);
    for (i = 0; i < n; i++) vals[i] = rand() % 10000;

    fh = mln_fheap_new(&min, &fattr);
    assert(fh != NULL);

    for (i = 0; i < n; i++) {
        fn = mln_fheap_node_new(fh, &vals[i]);
        assert(fn != NULL);
        mln_fheap_insert(fh, fn);
    }

    prev = -1;
    for (i = 0; i < n; i++) {
        fn = mln_fheap_extract_min(fh);
        assert(fn != NULL);
        assert(*(int *)mln_fheap_node_key(fn) >= prev);
        prev = *(int *)mln_fheap_node_key(fn);
        mln_fheap_node_free(fh, fn);
    }

    assert(mln_fheap_extract_min(fh) == NULL);
    mln_fheap_free(fh);
    free(vals);
    printf("  test_ordering PASSED\n");
    return 0;
}

/*
 * Test 11: Duplicate keys
 */
int test_duplicate_keys(void)
{
    int min = 0;
    int vals[] = {5, 5, 5, 3, 3, 7, 7, 1, 1};
    int n = sizeof(vals) / sizeof(vals[0]);
    int i, prev;
    mln_fheap_t *fh;
    mln_fheap_node_t *fn;
    struct mln_fheap_attr fattr;

    fattr.pool = NULL;
    fattr.pool_alloc = NULL;
    fattr.pool_free = NULL;
    fattr.cmp = cmp_handler;
    fattr.copy = copy_handler;
    fattr.key_free = NULL;

    fh = mln_fheap_new(&min, &fattr);
    assert(fh != NULL);

    for (i = 0; i < n; i++) {
        fn = mln_fheap_node_new(fh, &vals[i]);
        assert(fn != NULL);
        mln_fheap_insert(fh, fn);
    }

    prev = -1;
    for (i = 0; i < n; i++) {
        fn = mln_fheap_extract_min(fh);
        assert(fn != NULL);
        assert(*(int *)mln_fheap_node_key(fn) >= prev);
        prev = *(int *)mln_fheap_node_key(fn);
        mln_fheap_node_free(fh, fn);
    }

    mln_fheap_free(fh);
    printf("  test_duplicate_keys PASSED\n");
    return 0;
}

/*
 * Test 12: key_free callback
 */
int test_key_free(void)
{
    int min_val = 0;
    mln_fheap_t *fh;
    mln_fheap_node_t *fn;
    struct mln_fheap_attr fattr;
    int *v;

    fattr.pool = NULL;
    fattr.pool_alloc = NULL;
    fattr.pool_free = NULL;
    fattr.cmp = cmp_handler;
    fattr.copy = copy_handler;
    fattr.key_free = key_free_handler;

    fh = mln_fheap_new(&min_val, &fattr);
    assert(fh != NULL);

    v = (int *)malloc(sizeof(int)); *v = 100;
    fn = mln_fheap_node_new(fh, v);
    assert(fn != NULL);
    mln_fheap_insert(fh, fn);

    v = (int *)malloc(sizeof(int)); *v = 50;
    fn = mln_fheap_node_new(fh, v);
    assert(fn != NULL);
    mln_fheap_insert(fh, fn);

    v = (int *)malloc(sizeof(int)); *v = 200;
    fn = mln_fheap_node_new(fh, v);
    assert(fn != NULL);
    mln_fheap_insert(fh, fn);

    /* free should invoke key_free for each key */
    mln_fheap_free(fh);
    printf("  test_key_free PASSED\n");
    return 0;
}

/*
 * Test 13: Mixed insert/extract operations
 */
int test_mixed_operations(void)
{
    int min = 0;
    int vals[20];
    int i;
    mln_fheap_t *fh;
    mln_fheap_node_t *fn;
    struct mln_fheap_attr fattr;

    fattr.pool = NULL;
    fattr.pool_alloc = NULL;
    fattr.pool_free = NULL;
    fattr.cmp = cmp_handler;
    fattr.copy = copy_handler;
    fattr.key_free = NULL;

    for (i = 0; i < 20; i++) vals[i] = 20 - i;

    fh = mln_fheap_new(&min, &fattr);
    assert(fh != NULL);

    /* insert 10 elements */
    for (i = 0; i < 10; i++) {
        fn = mln_fheap_node_new(fh, &vals[i]);
        assert(fn != NULL);
        mln_fheap_insert(fh, fn);
    }

    /* extract 5 */
    for (i = 0; i < 5; i++) {
        fn = mln_fheap_extract_min(fh);
        assert(fn != NULL);
        mln_fheap_node_free(fh, fn);
    }

    /* insert 10 more */
    for (i = 10; i < 20; i++) {
        fn = mln_fheap_node_new(fh, &vals[i]);
        assert(fn != NULL);
        mln_fheap_insert(fh, fn);
    }

    /* extract all remaining - should be sorted */
    int prev = -1;
    while ((fn = mln_fheap_extract_min(fh)) != NULL) {
        assert(*(int *)mln_fheap_node_key(fn) >= prev);
        prev = *(int *)mln_fheap_node_key(fn);
        mln_fheap_node_free(fh, fn);
    }

    mln_fheap_free(fh);
    printf("  test_mixed_operations PASSED\n");
    return 0;
}

/*
 * Test 14: Performance benchmark
 */
#if !defined(MSVC)
int test_performance(void)
{
    int min_val = 0;
    int N = 500000;
    int *vals;
    int i, prev;
    mln_fheap_t *fh;
    mln_fheap_node_t *fn;
    clock_t start, end;
    double elapsed;

    vals = (int *)malloc(sizeof(int) * N);
    assert(vals != NULL);
    srand(42);
    for (i = 0; i < N; i++) vals[i] = rand();

    fh = mln_fheap_new(&min_val, NULL);
    assert(fh != NULL);

    start = clock();

    /* insert all */
    for (i = 0; i < N; i++) {
        fn = mln_fheap_node_new(fh, &vals[i]);
        assert(fn != NULL);
        mln_fheap_inline_insert(fh, fn, inline_cmp_handler);
    }

    /* extract all and verify order */
    prev = -1;
    for (i = 0; i < N; i++) {
        fn = mln_fheap_inline_extract_min(fh, inline_cmp_handler);
        assert(fn != NULL);
        assert(*(int *)mln_fheap_node_key(fn) >= prev);
        prev = *(int *)mln_fheap_node_key(fn);
        mln_fheap_inline_node_free(fh, fn, NULL);
    }

    end = clock();
    elapsed = (double)(end - start) / CLOCKS_PER_SEC;

    assert(mln_fheap_minimum(fh) == NULL);
    mln_fheap_inline_free(fh, inline_cmp_handler, NULL);

    printf("  test_performance: %d insert+extract in %.4f sec PASSED\n", N, elapsed);
    free(vals);
    return 0;
}
#else
int test_performance(void)
{
    int min_val = 0;
    int N = 500000;
    int *vals;
    int i, prev;
    mln_fheap_t *fh;
    mln_fheap_node_t *fn;
    clock_t start, end;
    double elapsed;
    struct mln_fheap_attr fattr;

    fattr.pool = NULL;
    fattr.pool_alloc = NULL;
    fattr.pool_free = NULL;
    fattr.cmp = cmp_handler;
    fattr.copy = copy_handler;
    fattr.key_free = NULL;

    vals = (int *)malloc(sizeof(int) * N);
    assert(vals != NULL);
    srand(42);
    for (i = 0; i < N; i++) vals[i] = rand();

    fh = mln_fheap_new(&min_val, &fattr);
    assert(fh != NULL);

    start = clock();

    for (i = 0; i < N; i++) {
        fn = mln_fheap_node_new(fh, &vals[i]);
        assert(fn != NULL);
        mln_fheap_insert(fh, fn);
    }

    prev = -1;
    for (i = 0; i < N; i++) {
        fn = mln_fheap_extract_min(fh);
        assert(fn != NULL);
        assert(*(int *)mln_fheap_node_key(fn) >= prev);
        prev = *(int *)mln_fheap_node_key(fn);
        mln_fheap_node_free(fh, fn);
    }

    end = clock();
    elapsed = (double)(end - start) / CLOCKS_PER_SEC;

    assert(mln_fheap_minimum(fh) == NULL);
    mln_fheap_free(fh);

    printf("  test_performance: %d insert+extract in %.4f sec PASSED\n", N, elapsed);
    free(vals);
    return 0;
}
#endif

/*
 * Test 15: Stability - large scale with decrease_key and delete
 */
int test_stability(void)
{
    int min_val = 0;
    int N = 10000;
    int *vals;
    int i;
    mln_fheap_t *fh;
    mln_fheap_node_t **nodes;
    struct mln_fheap_attr fattr;

    fattr.pool = NULL;
    fattr.pool_alloc = NULL;
    fattr.pool_free = NULL;
    fattr.cmp = cmp_handler;
    fattr.copy = copy_handler;
    fattr.key_free = NULL;

    vals = (int *)malloc(sizeof(int) * N);
    nodes = (mln_fheap_node_t **)malloc(sizeof(mln_fheap_node_t *) * N);
    assert(vals != NULL && nodes != NULL);

    srand(99);
    for (i = 0; i < N; i++) vals[i] = rand() % 100000 + 100;

    fh = mln_fheap_new(&min_val, &fattr);
    assert(fh != NULL);

    /* insert all */
    for (i = 0; i < N; i++) {
        nodes[i] = mln_fheap_node_new(fh, &vals[i]);
        assert(nodes[i] != NULL);
        mln_fheap_insert(fh, nodes[i]);
    }

    /* decrease key on every 10th node */
    for (i = 0; i < N; i += 10) {
        int new_val = 1 + (i / 10);
        vals[i] = new_val;
        /* node already has key pointing to vals[i], copy_handler copies value */
        int tmp = new_val;
        if (mln_fheap_decrease_key(fh, nodes[i], &tmp) != 0) {
            /* key might already be smaller; that's ok */
        }
    }

    /* delete every 5th node (that wasn't decreased) */
    for (i = 5; i < N; i += 10) {
        mln_fheap_delete(fh, nodes[i]);
        mln_fheap_node_free(fh, nodes[i]);
        nodes[i] = NULL;
    }

    /* extract all remaining and verify sorted order */
    int prev = -1;
    mln_fheap_node_t *fn;
    int count = 0;
    while ((fn = mln_fheap_extract_min(fh)) != NULL) {
        assert(*(int *)mln_fheap_node_key(fn) >= prev);
        prev = *(int *)mln_fheap_node_key(fn);
        mln_fheap_node_free(fh, fn);
        count++;
    }

    /* verify count: N - N/10 deleted */
    assert(count == N - N / 10);

    mln_fheap_free(fh);
    free(vals);
    free(nodes);
    printf("  test_stability PASSED\n");
    return 0;
}

/*
 * Test 16: Free performance (O(n) destroy_list vs O(n log n) extract loop)
 */
#if !defined(MSVC)
int test_free_performance(void)
{
    int min_val = 0;
    int N = 500000;
    int *vals;
    int i;
    mln_fheap_t *fh;
    mln_fheap_node_t *fn;
    clock_t start, end;
    double elapsed;

    vals = (int *)malloc(sizeof(int) * N);
    assert(vals != NULL);
    for (i = 0; i < N; i++) vals[i] = i;

    fh = mln_fheap_new(&min_val, NULL);
    assert(fh != NULL);

    for (i = 0; i < N; i++) {
        fn = mln_fheap_node_new(fh, &vals[i]);
        assert(fn != NULL);
        mln_fheap_inline_insert(fh, fn, inline_cmp_handler);
    }

    /* Force some consolidation so tree has structure */
    for (i = 0; i < 100; i++) {
        fn = mln_fheap_inline_extract_min(fh, inline_cmp_handler);
        assert(fn != NULL);
        mln_fheap_inline_node_free(fh, fn, NULL);
    }

    start = clock();
    mln_fheap_inline_free(fh, inline_cmp_handler, NULL);
    end = clock();
    elapsed = (double)(end - start) / CLOCKS_PER_SEC;

    printf("  test_free_performance: free %d nodes in %.4f sec PASSED\n", N - 100, elapsed);
    free(vals);
    return 0;
}
#else
int test_free_performance(void)
{
    printf("  test_free_performance SKIPPED (MSVC)\n");
    return 0;
}
#endif

/*
 * ================================================================
 * Test 17: Prove this IS a Fibonacci Heap
 *
 * A Fibonacci heap is identified by the following structural
 * invariants that distinguish it from binary heaps, binomial heaps,
 * pairing heaps, etc. We verify ALL of them by walking the internal
 * tree structure after a mixed workload of insert, extract-min, and
 * decrease-key (which exercises consolidation AND cascading cut).
 *
 * Invariants verified:
 *   P1  Root-degree uniqueness  (post-consolidation)
 *   P2  Degree upper bound      D(n) <= floor(log_phi(n))
 *   P3  Fibonacci subtree size  size(x) >= F(degree(x)+2)
 *   P4  Mark invariant          root.mark==0; non-root.mark in {0,1}
 *   P5  Min-heap order          parent.key <= child.key
 *   P6  Circular doubly-linked list integrity
 *   P7  Parent pointer consistency
 *   P8  Stored degree == actual child count
 *   P9  Node count == fh->num
 * ================================================================
 */

/* --- helper: count nodes in a single subtree rooted at `node` --- */
static mln_size_t fhv_count_subtree(mln_fheap_node_t *node)
{
    mln_size_t cnt = 1;
    if (node->child != NULL) {
        mln_fheap_node_t *c = node->child;
        do { cnt += fhv_count_subtree(c); c = c->right; } while (c != node->child);
    }
    return cnt;
}

/* --- helper: count all nodes reachable from a circular list --- */
static mln_size_t fhv_count_list(mln_fheap_node_t *head)
{
    if (head == NULL) return 0;
    mln_size_t cnt = 0;
    mln_fheap_node_t *n = head;
    do { cnt += fhv_count_subtree(n); n = n->right; } while (n != head);
    return cnt;
}

/* --- helper: count direct children --- */
static mln_size_t fhv_count_children(mln_fheap_node_t *node)
{
    if (node->child == NULL) return 0;
    mln_size_t cnt = 0;
    mln_fheap_node_t *c = node->child;
    do { cnt++; c = c->right; } while (c != node->child);
    return cnt;
}

/* --- helper: k-th Fibonacci number, F(0)=0, F(1)=1, ... --- */
static mln_size_t fhv_fibonacci(int k)
{
    if (k <= 0) return 0;
    if (k == 1) return 1;
    mln_size_t a = 0, b = 1;
    int i;
    for (i = 2; i <= k; i++) { mln_size_t c = a + b; a = b; b = c; }
    return b;
}

/* --- helper: floor(log_phi(n)) via Fibonacci --- */
static int fhv_degree_upper_bound(mln_size_t n)
{
    /*
     * In a Fibonacci heap, the max degree D(n) satisfies
     *   F(D(n) + 2) <= n
     * We find the largest k such that F(k+2) <= n.
     */
    if (n <= 1) return 0;
    mln_size_t a = 1, b = 2; /* F(2)=1, F(3)=2 */
    int k = 0;
    while (b <= n) {
        mln_size_t c = a + b;
        a = b; b = c;
        k++;
    }
    return k;
}

/* P1: after consolidation, all root degrees are distinct */
static int fhv_root_degrees_unique(mln_fheap_node_t *root_list)
{
    if (root_list == NULL) return 1;
    int seen[FH_LGN];
    memset(seen, 0, sizeof(seen));
    mln_fheap_node_t *n = root_list;
    do {
        if (n->degree >= FH_LGN) return 0;
        if (seen[n->degree]) return 0;
        seen[n->degree] = 1;
        n = n->right;
    } while (n != root_list);
    return 1;
}

/* P2: max degree across the whole heap */
static int fhv_max_degree(mln_fheap_node_t *head)
{
    if (head == NULL) return -1;
    int md = -1;
    mln_fheap_node_t *n = head;
    do {
        if ((int)n->degree > md) md = (int)n->degree;
        int cd = fhv_max_degree(n->child);
        if (cd > md) md = cd;
        n = n->right;
    } while (n != head);
    return md;
}

/* P3: subtree(x) >= F(degree(x)+2) for every node */
static int fhv_fibonacci_sizes(mln_fheap_node_t *head)
{
    if (head == NULL) return 1;
    mln_fheap_node_t *n = head;
    do {
        mln_size_t sz  = fhv_count_subtree(n);
        mln_size_t fib = fhv_fibonacci((int)n->degree + 2);
        if (sz < fib) return 0;
        if (!fhv_fibonacci_sizes(n->child)) return 0;
        n = n->right;
    } while (n != head);
    return 1;
}

/* P4: mark invariant */
static int fhv_marks_valid(mln_fheap_node_t *head, int is_root_level)
{
    if (head == NULL) return 1;
    mln_fheap_node_t *n = head;
    do {
        if (is_root_level && n->mark != FHEAP_FALSE) return 0;
        if (!is_root_level && n->mark != FHEAP_FALSE && n->mark != FHEAP_TRUE) return 0;
        if (!fhv_marks_valid(n->child, 0)) return 0;
        n = n->right;
    } while (n != head);
    return 1;
}

/* P5: min-heap order – parent.key <= every child.key
 *     cmp(a,b)==0 means a<b; so cmp(child,parent)==0 => child<parent => violation */
static int fhv_heap_order(mln_fheap_node_t *node, fheap_cmp cmp)
{
    if (node->child == NULL) return 1;
    mln_fheap_node_t *c = node->child;
    do {
        if (!cmp(c->key, node->key)) return 0;   /* child < parent */
        if (!fhv_heap_order(c, cmp)) return 0;
        c = c->right;
    } while (c != node->child);
    return 1;
}

static int fhv_heap_order_all(mln_fheap_node_t *head, fheap_cmp cmp)
{
    if (head == NULL) return 1;
    mln_fheap_node_t *n = head;
    do {
        if (!fhv_heap_order(n, cmp)) return 0;
        n = n->right;
    } while (n != head);
    return 1;
}

/* P6: circular doubly-linked list integrity */
static int fhv_circular_list_ok(mln_fheap_node_t *head)
{
    if (head == NULL) return 1;
    mln_fheap_node_t *n = head;
    do {
        if (n->right->left != n) return 0;
        if (n->left->right != n) return 0;
        n = n->right;
    } while (n != head);
    return 1;
}

/* P7: parent pointers consistent, and recurse into children */
static int fhv_parent_ptrs(mln_fheap_node_t *head, mln_fheap_node_t *expected_parent)
{
    if (head == NULL) return 1;
    mln_fheap_node_t *n = head;
    do {
        if (n->parent != expected_parent) return 0;
        if (!fhv_circular_list_ok(n->child)) return 0;
        if (!fhv_parent_ptrs(n->child, n)) return 0;
        n = n->right;
    } while (n != head);
    return 1;
}

/* P8: stored degree == actual child count, for every node */
static int fhv_degrees_correct(mln_fheap_node_t *head)
{
    if (head == NULL) return 1;
    mln_fheap_node_t *n = head;
    do {
        if (fhv_count_children(n) != n->degree) return 0;
        if (!fhv_degrees_correct(n->child)) return 0;
        n = n->right;
    } while (n != head);
    return 1;
}

int test_fibonacci_heap_properties(void)
{
    int min_val = 0;
    int N = 5000;
    int *vals;
    int i;
    mln_fheap_t *fh;
    mln_fheap_node_t **nodes;
    struct mln_fheap_attr fattr;

    printf("  test_fibonacci_heap_properties ...\n");

    fattr.pool = NULL;
    fattr.pool_alloc = NULL;
    fattr.pool_free = NULL;
    fattr.cmp = cmp_handler;
    fattr.copy = copy_handler;
    fattr.key_free = NULL;

    vals = (int *)malloc(sizeof(int) * N);
    nodes = (mln_fheap_node_t **)malloc(sizeof(mln_fheap_node_t *) * N);
    assert(vals != NULL && nodes != NULL);

    for (i = 0; i < N; i++) vals[i] = i + 1;  /* keys 1..N */

    fh = mln_fheap_new(&min_val, &fattr);
    assert(fh != NULL);

    /*
     * Phase 1: Insert all N elements.
     * After pure inserts, root list is a flat list of N singleton trees.
     */
    for (i = 0; i < N; i++) {
        nodes[i] = mln_fheap_node_new(fh, &vals[i]);
        assert(nodes[i] != NULL);
        mln_fheap_insert(fh, nodes[i]);
    }

    /*
     * Phase 2: Extract 2000 smallest elements.
     * Each extract-min triggers consolidation, which merges roots
     * with the same degree — the hallmark of Fibonacci heap behavior.
     */
    for (i = 0; i < 2000; i++) {
        mln_fheap_node_t *fn = mln_fheap_extract_min(fh);
        assert(fn != NULL);
        nodes[*(int *)fn->key - 1] = NULL;
        mln_fheap_node_free(fh, fn);
    }

    /*
     * Phase 3: Decrease keys of ~500 nodes to very small values.
     * This triggers cut and cascading-cut (the mechanism that
     * maintains the Fibonacci size bound via marks).
     */
    {
        int dk_count = 0;
        for (i = N - 1; i >= 0 && dk_count < 500; i--) {
            if (nodes[i] == NULL) continue;
            int new_val = -(dk_count + 1);
            int tmp = new_val;
            if (mln_fheap_decrease_key(fh, nodes[i], &tmp) == 0) {
                vals[i] = new_val;
                dk_count++;
            }
        }
        printf("    triggered %d decrease-key operations\n", dk_count);
    }

    /*
     * Phase 4: One more extract-min to trigger a fresh consolidation,
     * so that the root-degree-uniqueness invariant holds when we check.
     */
    {
        mln_fheap_node_t *fn = mln_fheap_extract_min(fh);
        assert(fn != NULL);
        mln_fheap_node_free(fh, fn);
    }

    /*
     * ============================================================
     * VERIFY ALL FIBONACCI HEAP STRUCTURAL INVARIANTS
     * ============================================================
     */
    mln_size_t n_remaining = fh->num;
    printf("    heap contains %lu nodes, verifying invariants ...\n",
           (unsigned long)n_remaining);

    /* P1: Root-degree uniqueness (post-consolidation) */
    assert(fhv_root_degrees_unique(fh->root_list));
    printf("    P1 root-degree uniqueness           OK\n");

    /* P2: Degree upper bound: max_degree <= floor(log_phi(n)) */
    {
        int md    = fhv_max_degree(fh->root_list);
        int bound = fhv_degree_upper_bound(n_remaining);
        printf("    P2 degree bound: max_degree=%d, "
               "floor(log_phi(%lu))=%d  ", md, (unsigned long)n_remaining, bound);
        assert(md <= bound);
        printf("OK\n");
    }

    /* P3: Fibonacci subtree size: subtree(x) >= F(degree(x)+2) */
    assert(fhv_fibonacci_sizes(fh->root_list));
    printf("    P3 Fibonacci subtree size bound      OK\n");

    /* P4: Mark invariant: roots unmarked; non-roots mark in {0,1} */
    assert(fhv_marks_valid(fh->root_list, 1));
    printf("    P4 mark invariant                    OK\n");

    /* P5: Min-heap order */
    assert(fhv_heap_order_all(fh->root_list, cmp_handler));
    printf("    P5 min-heap order                    OK\n");

    /* P6: Circular doubly-linked list integrity (root list) */
    assert(fhv_circular_list_ok(fh->root_list));
    printf("    P6 circular list integrity           OK\n");

    /* P7: Parent pointers + child-list integrity (recursive) */
    assert(fhv_parent_ptrs(fh->root_list, NULL));
    printf("    P7 parent pointer consistency        OK\n");

    /* P8: Stored degree == actual child count for every node */
    assert(fhv_degrees_correct(fh->root_list));
    printf("    P8 degree field correctness          OK\n");

    /* P9: Total node count matches fh->num */
    {
        mln_size_t counted = fhv_count_list(fh->root_list);
        assert(counted == n_remaining);
    }
    printf("    P9 node count consistency            OK\n");

    mln_fheap_free(fh);
    free(vals);
    free(nodes);
    printf("  test_fibonacci_heap_properties PASSED\n");
    return 0;
}

int main(void)
{
    printf("=== Fibonacci Heap Tests ===\n");

    assert(test_basic_api() == 0);
    assert(test_new_fast() == 0);
    assert(test_decrease_key() == 0);
    assert(test_delete() == 0);
    assert(test_inline_ops() == 0);
    assert(test_inline_decrease_delete() == 0);
    assert(test_container_usage() == 0);
    assert(test_empty_heap() == 0);
    assert(test_single_element() == 0);
    assert(test_ordering() == 0);
    assert(test_duplicate_keys() == 0);
    assert(test_key_free() == 0);
    assert(test_mixed_operations() == 0);
    assert(test_performance() == 0);
    assert(test_stability() == 0);
    assert(test_free_performance() == 0);
    assert(test_fibonacci_heap_properties() == 0);

    printf("=== ALL TESTS PASSED ===\n");
    return 0;
}
