#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include "mln_rbtree.h"

static int test_nr = 0;

#define PASS() do { printf("  #%02d PASS\n", ++test_nr); } while (0)

static int cmp_int(const void *data1, const void *data2)
{
    int a = *(int *)data1, b = *(int *)data2;
    if (a < b) return -1;
    if (a > b) return 1;
    return 0;
}

static inline int cmp_int_inline(const void *data1, const void *data2)
{
    int a = *(int *)data1, b = *(int *)data2;
    if (a < b) return -1;
    if (a > b) return 1;
    return 0;
}

static void free_int(void *data)
{
    free(data);
}

/* Test 1: basic new/free */
static void test_new_free(void)
{
    mln_rbtree_t *t;
    struct mln_rbtree_attr rbattr;

    rbattr.pool = NULL;
    rbattr.pool_alloc = NULL;
    rbattr.pool_free = NULL;
    rbattr.cmp = cmp_int;
    rbattr.data_free = NULL;

    t = mln_rbtree_new(&rbattr);
    assert(t != NULL);
    assert(mln_rbtree_node_num(t) == 0);
    mln_rbtree_free(t);

    /* NULL attr */
    t = mln_rbtree_new(NULL);
    assert(t != NULL);
    mln_rbtree_free(t);

    /* free NULL is safe */
    mln_rbtree_free(NULL);
    PASS();
}

/* Test 2: insert and search basic */
static void test_insert_search(void)
{
    int vals[] = {5, 3, 7, 1, 4, 6, 8, 2, 9, 0};
    int n = sizeof(vals) / sizeof(vals[0]);
    struct mln_rbtree_attr rbattr;
    mln_rbtree_node_t *rn;

    rbattr.pool = NULL;
    rbattr.pool_alloc = NULL;
    rbattr.pool_free = NULL;
    rbattr.cmp = cmp_int;
    rbattr.data_free = NULL;

    mln_rbtree_t *t = mln_rbtree_new(&rbattr);
    assert(t != NULL);

    for (int i = 0; i < n; i++) {
        rn = mln_rbtree_node_new(t, &vals[i]);
        assert(rn != NULL);
        mln_rbtree_insert(t, rn);
    }
    assert(mln_rbtree_node_num(t) == (mln_uauto_t)n);

    /* search all values */
    for (int i = 0; i < n; i++) {
        rn = mln_rbtree_search(t, &vals[i]);
        assert(!mln_rbtree_null(rn, t));
        assert(*(int *)mln_rbtree_node_data_get(rn) == vals[i]);
    }

    /* search non-existent */
    int missing = 99;
    rn = mln_rbtree_search(t, &missing);
    assert(mln_rbtree_null(rn, t));

    mln_rbtree_free(t);
    PASS();
}

/* Test 3: delete */
static void test_delete(void)
{
    int vals[] = {10, 20, 30, 40, 50, 25, 35, 15, 5, 45};
    int n = sizeof(vals) / sizeof(vals[0]);
    struct mln_rbtree_attr rbattr;
    mln_rbtree_node_t *rn;

    rbattr.pool = NULL;
    rbattr.pool_alloc = NULL;
    rbattr.pool_free = NULL;
    rbattr.cmp = cmp_int;
    rbattr.data_free = NULL;

    mln_rbtree_t *t = mln_rbtree_new(&rbattr);
    assert(t != NULL);

    mln_rbtree_node_t *nodes[10];
    for (int i = 0; i < n; i++) {
        nodes[i] = mln_rbtree_node_new(t, &vals[i]);
        assert(nodes[i] != NULL);
        mln_rbtree_insert(t, nodes[i]);
    }

    /* delete some nodes */
    mln_rbtree_delete(t, nodes[0]); /* 10 */
    mln_rbtree_node_free(t, nodes[0]);
    assert(mln_rbtree_node_num(t) == (mln_uauto_t)(n - 1));

    rn = mln_rbtree_search(t, &vals[0]);
    assert(mln_rbtree_null(rn, t));

    /* remaining still found */
    for (int i = 1; i < n; i++) {
        rn = mln_rbtree_search(t, &vals[i]);
        assert(!mln_rbtree_null(rn, t));
    }

    /* delete all remaining */
    for (int i = 1; i < n; i++) {
        mln_rbtree_delete(t, nodes[i]);
        mln_rbtree_node_free(t, nodes[i]);
    }
    assert(mln_rbtree_node_num(t) == 0);

    mln_rbtree_free(t);
    PASS();
}

/* Test 4: successor */
static void test_successor(void)
{
    int vals[] = {3, 1, 5, 2, 4};
    int n = sizeof(vals) / sizeof(vals[0]);
    struct mln_rbtree_attr rbattr;

    rbattr.pool = NULL;
    rbattr.pool_alloc = NULL;
    rbattr.pool_free = NULL;
    rbattr.cmp = cmp_int;
    rbattr.data_free = NULL;

    mln_rbtree_t *t = mln_rbtree_new(&rbattr);
    assert(t != NULL);

    for (int i = 0; i < n; i++) {
        mln_rbtree_node_t *rn = mln_rbtree_node_new(t, &vals[i]);
        assert(rn != NULL);
        mln_rbtree_insert(t, rn);
    }

    /* find node 3 and its successor (4) */
    mln_rbtree_node_t *rn = mln_rbtree_search(t, &vals[0]);
    assert(!mln_rbtree_null(rn, t));
    mln_rbtree_node_t *succ = mln_rbtree_successor(t, rn);
    assert(!mln_rbtree_null(succ, t));
    assert(*(int *)mln_rbtree_node_data_get(succ) == 4);

    /* successor of max element is nil */
    int maxval = 5;
    rn = mln_rbtree_search(t, &maxval);
    succ = mln_rbtree_successor(t, rn);
    assert(mln_rbtree_null(succ, t));

    mln_rbtree_free(t);
    PASS();
}

/* Test 5: min */
static void test_min(void)
{
    int vals[] = {50, 30, 70, 10, 40};
    int n = sizeof(vals) / sizeof(vals[0]);
    struct mln_rbtree_attr rbattr;

    rbattr.pool = NULL;
    rbattr.pool_alloc = NULL;
    rbattr.pool_free = NULL;
    rbattr.cmp = cmp_int;
    rbattr.data_free = NULL;

    mln_rbtree_t *t = mln_rbtree_new(&rbattr);
    assert(t != NULL);

    /* min of empty tree */
    mln_rbtree_node_t *rn = mln_rbtree_min(t);
    assert(mln_rbtree_null(rn, t));

    for (int i = 0; i < n; i++) {
        mln_rbtree_node_t *node = mln_rbtree_node_new(t, &vals[i]);
        assert(node != NULL);
        mln_rbtree_insert(t, node);
    }

    rn = mln_rbtree_min(t);
    assert(!mln_rbtree_null(rn, t));
    assert(*(int *)mln_rbtree_node_data_get(rn) == 10);

    mln_rbtree_free(t);
    PASS();
}

/* Test 6: iterate (in-order) */
static int iterate_check_sorted(mln_rbtree_node_t *node, void *udata)
{
    int **prev = (int **)udata;
    int val = *(int *)mln_rbtree_node_data_get(node);
    if (*prev != NULL) {
        assert(val >= **prev);
    }
    *prev = (int *)mln_rbtree_node_data_get(node);
    return 0;
}

static void test_iterate(void)
{
    int vals[] = {5, 3, 7, 1, 4, 6, 8, 2, 9, 0};
    int n = sizeof(vals) / sizeof(vals[0]);
    struct mln_rbtree_attr rbattr;

    rbattr.pool = NULL;
    rbattr.pool_alloc = NULL;
    rbattr.pool_free = NULL;
    rbattr.cmp = cmp_int;
    rbattr.data_free = NULL;

    mln_rbtree_t *t = mln_rbtree_new(&rbattr);
    assert(t != NULL);

    for (int i = 0; i < n; i++) {
        mln_rbtree_node_t *rn = mln_rbtree_node_new(t, &vals[i]);
        assert(rn != NULL);
        mln_rbtree_insert(t, rn);
    }

    int *prev = NULL;
    int ret = mln_rbtree_iterate(t, iterate_check_sorted, &prev);
    assert(ret == 0);

    mln_rbtree_free(t);
    PASS();
}

/* Test 7: iterate early termination */
static int iterate_early_stop(mln_rbtree_node_t *node, void *udata)
{
    int *count = (int *)udata;
    (void)node;
    (*count)++;
    if (*count >= 3) return -1;
    return 0;
}

static void test_iterate_early_stop(void)
{
    int vals[] = {1, 2, 3, 4, 5};
    struct mln_rbtree_attr rbattr;

    rbattr.pool = NULL;
    rbattr.pool_alloc = NULL;
    rbattr.pool_free = NULL;
    rbattr.cmp = cmp_int;
    rbattr.data_free = NULL;

    mln_rbtree_t *t = mln_rbtree_new(&rbattr);
    assert(t != NULL);

    for (int i = 0; i < 5; i++) {
        mln_rbtree_node_t *rn = mln_rbtree_node_new(t, &vals[i]);
        assert(rn != NULL);
        mln_rbtree_insert(t, rn);
    }

    int count = 0;
    int ret = mln_rbtree_iterate(t, iterate_early_stop, &count);
    assert(ret == -1);
    assert(count == 3);

    mln_rbtree_free(t);
    PASS();
}

/* Test 8: iterate empty tree */
static void test_iterate_empty(void)
{
    struct mln_rbtree_attr rbattr;

    rbattr.pool = NULL;
    rbattr.pool_alloc = NULL;
    rbattr.pool_free = NULL;
    rbattr.cmp = cmp_int;
    rbattr.data_free = NULL;

    mln_rbtree_t *t = mln_rbtree_new(&rbattr);
    assert(t != NULL);

    int count = 0;
    int ret = mln_rbtree_iterate(t, iterate_early_stop, &count);
    assert(ret == 0);
    assert(count == 0);

    mln_rbtree_free(t);
    PASS();
}

/* Test 9: reset */
static void test_reset(void)
{
    struct mln_rbtree_attr rbattr;

    rbattr.pool = NULL;
    rbattr.pool_alloc = NULL;
    rbattr.pool_free = NULL;
    rbattr.cmp = cmp_int;
    rbattr.data_free = free_int;

    mln_rbtree_t *t = mln_rbtree_new(&rbattr);
    assert(t != NULL);

    for (int i = 0; i < 100; i++) {
        int *val = (int *)malloc(sizeof(int));
        assert(val != NULL);
        *val = i;
        mln_rbtree_node_t *rn = mln_rbtree_node_new(t, val);
        assert(rn != NULL);
        mln_rbtree_insert(t, rn);
    }
    assert(mln_rbtree_node_num(t) == 100);

    mln_rbtree_reset(t);
    assert(mln_rbtree_node_num(t) == 0);
    assert(mln_rbtree_null(mln_rbtree_min(t), t));

    /* can reuse after reset */
    for (int i = 0; i < 50; i++) {
        int *val = (int *)malloc(sizeof(int));
        assert(val != NULL);
        *val = i;
        mln_rbtree_node_t *rn = mln_rbtree_node_new(t, val);
        assert(rn != NULL);
        mln_rbtree_insert(t, rn);
    }
    assert(mln_rbtree_node_num(t) == 50);

    mln_rbtree_free(t);
    PASS();
}

/* Test 10: data_free callback */
static int free_counter = 0;

static void counting_free(void *data)
{
    free_counter++;
    free(data);
}

static void test_data_free_callback(void)
{
    struct mln_rbtree_attr rbattr;

    rbattr.pool = NULL;
    rbattr.pool_alloc = NULL;
    rbattr.pool_free = NULL;
    rbattr.cmp = cmp_int;
    rbattr.data_free = counting_free;

    mln_rbtree_t *t = mln_rbtree_new(&rbattr);
    assert(t != NULL);

    free_counter = 0;
    for (int i = 0; i < 10; i++) {
        int *val = (int *)malloc(sizeof(int));
        assert(val != NULL);
        *val = i;
        mln_rbtree_node_t *rn = mln_rbtree_node_new(t, val);
        assert(rn != NULL);
        mln_rbtree_insert(t, rn);
    }

    mln_rbtree_free(t);
    assert(free_counter == 10);
    PASS();
}

/* Test 11: inline insert/search */
static void test_inline_insert_search(void)
{
    int vals[] = {5, 3, 7, 1, 4, 6, 8};
    int n = sizeof(vals) / sizeof(vals[0]);
    mln_rbtree_node_t *rn;

    mln_rbtree_t *t = mln_rbtree_new(NULL);
    assert(t != NULL);

    for (int i = 0; i < n; i++) {
        rn = mln_rbtree_node_new(t, &vals[i]);
        assert(rn != NULL);
        mln_rbtree_inline_insert(t, rn, cmp_int_inline);
    }
    assert(mln_rbtree_node_num(t) == (mln_uauto_t)n);

    for (int i = 0; i < n; i++) {
        rn = mln_rbtree_inline_search(t, &vals[i], cmp_int_inline);
        assert(!mln_rbtree_null(rn, t));
        assert(*(int *)mln_rbtree_node_data_get(rn) == vals[i]);
    }

    int missing = 99;
    rn = mln_rbtree_inline_search(t, &missing, cmp_int_inline);
    assert(mln_rbtree_null(rn, t));

    mln_rbtree_free(t);
    PASS();
}

/* Test 12: inline root_search */
static void test_inline_root_search(void)
{
    int vals[] = {4, 2, 6, 1, 3, 5, 7};
    int n = sizeof(vals) / sizeof(vals[0]);

    mln_rbtree_t *t = mln_rbtree_new(NULL);
    assert(t != NULL);

    for (int i = 0; i < n; i++) {
        mln_rbtree_node_t *rn = mln_rbtree_node_new(t, &vals[i]);
        assert(rn != NULL);
        mln_rbtree_inline_insert(t, rn, cmp_int_inline);
    }

    mln_rbtree_node_t *rn = mln_rbtree_inline_root_search(t, mln_rbtree_root(t), &vals[3], cmp_int_inline);
    assert(!mln_rbtree_null(rn, t));
    assert(*(int *)mln_rbtree_node_data_get(rn) == vals[3]);

    mln_rbtree_free(t);
    PASS();
}

/* Test 13: container usage (mln_rbtree_node_init) */
typedef struct user_defined_s {
    int val;
    mln_rbtree_node_t node;
} ud_t;

static int cmp_ud(const void *data1, const void *data2)
{
    int a = ((ud_t *)data1)->val, b = ((ud_t *)data2)->val;
    if (a < b) return -1;
    if (a > b) return 1;
    return 0;
}

static void test_container_usage(void)
{
    mln_rbtree_t *t;
    ud_t data1, data2, data3;
    mln_rbtree_node_t *rn;
    struct mln_rbtree_attr rbattr;

    rbattr.pool = NULL;
    rbattr.pool_alloc = NULL;
    rbattr.pool_free = NULL;
    rbattr.cmp = cmp_ud;
    rbattr.data_free = NULL;

    t = mln_rbtree_new(&rbattr);
    assert(t != NULL);

    data1.val = 10;
    data2.val = 20;
    data3.val = 5;

    mln_rbtree_node_init(&data1.node, &data1);
    mln_rbtree_node_init(&data2.node, &data2);
    mln_rbtree_node_init(&data3.node, &data3);

    mln_rbtree_insert(t, &data1.node);
    mln_rbtree_insert(t, &data2.node);
    mln_rbtree_insert(t, &data3.node);
    assert(mln_rbtree_node_num(t) == 3);

    rn = mln_rbtree_search(t, &data1);
    assert(!mln_rbtree_null(rn, t));
    assert(((ud_t *)mln_rbtree_node_data_get(rn))->val == 10);
    assert(mln_container_of(rn, ud_t, node)->val == 10);

    /* min should be data3 (val=5) */
    rn = mln_rbtree_min(t);
    assert(!mln_rbtree_null(rn, t));
    assert(((ud_t *)mln_rbtree_node_data_get(rn))->val == 5);

    mln_rbtree_delete(t, &data1.node);
    mln_rbtree_node_free(t, &data1.node);
    assert(mln_rbtree_node_num(t) == 2);

    mln_rbtree_free(t);
    PASS();
}

/* Test 14: inline free */
static inline void inline_free_int(void *data)
{
    free(data);
}

static void test_inline_free(void)
{
    mln_rbtree_t *t = mln_rbtree_new(NULL);
    assert(t != NULL);

    for (int i = 0; i < 100; i++) {
        int *val = (int *)malloc(sizeof(int));
        assert(val != NULL);
        *val = i;
        mln_rbtree_node_t *rn = mln_rbtree_node_new(t, val);
        assert(rn != NULL);
        mln_rbtree_inline_insert(t, rn, cmp_int_inline);
    }

    mln_rbtree_inline_free(t, inline_free_int);
    PASS();
}

/* Test 15: inline reset */
static void test_inline_reset(void)
{
    mln_rbtree_t *t = mln_rbtree_new(NULL);
    assert(t != NULL);

    for (int i = 0; i < 50; i++) {
        int *val = (int *)malloc(sizeof(int));
        assert(val != NULL);
        *val = i;
        mln_rbtree_node_t *rn = mln_rbtree_node_new(t, val);
        assert(rn != NULL);
        mln_rbtree_inline_insert(t, rn, cmp_int_inline);
    }

    mln_rbtree_inline_reset(t, inline_free_int);
    assert(mln_rbtree_node_num(t) == 0);

    /* reuse after reset */
    for (int i = 0; i < 20; i++) {
        int *val = (int *)malloc(sizeof(int));
        assert(val != NULL);
        *val = i;
        mln_rbtree_node_t *rn = mln_rbtree_node_new(t, val);
        assert(rn != NULL);
        mln_rbtree_inline_insert(t, rn, cmp_int_inline);
    }
    assert(mln_rbtree_node_num(t) == 20);

    mln_rbtree_inline_free(t, inline_free_int);
    PASS();
}

/* Test 16: inline node_free */
static void test_inline_node_free(void)
{
    struct mln_rbtree_attr rbattr;

    rbattr.pool = NULL;
    rbattr.pool_alloc = NULL;
    rbattr.pool_free = NULL;
    rbattr.cmp = cmp_int;
    rbattr.data_free = NULL;

    mln_rbtree_t *t = mln_rbtree_new(&rbattr);
    assert(t != NULL);

    int *val = (int *)malloc(sizeof(int));
    assert(val != NULL);
    *val = 42;
    mln_rbtree_node_t *rn = mln_rbtree_node_new(t, val);
    assert(rn != NULL);
    mln_rbtree_insert(t, rn);

    mln_rbtree_delete(t, rn);
    mln_rbtree_inline_node_free(t, rn, inline_free_int);

    mln_rbtree_free(t);
    PASS();
}

/* Test 17: node_data_set */
static void test_node_data_set(void)
{
    int a = 10, b = 20;
    struct mln_rbtree_attr rbattr;

    rbattr.pool = NULL;
    rbattr.pool_alloc = NULL;
    rbattr.pool_free = NULL;
    rbattr.cmp = cmp_int;
    rbattr.data_free = NULL;

    mln_rbtree_t *t = mln_rbtree_new(&rbattr);
    assert(t != NULL);

    mln_rbtree_node_t *rn = mln_rbtree_node_new(t, &a);
    assert(rn != NULL);
    assert(*(int *)mln_rbtree_node_data_get(rn) == 10);

    mln_rbtree_node_data_set(rn, &b);
    assert(*(int *)mln_rbtree_node_data_get(rn) == 20);

    mln_rbtree_node_free(t, rn);
    mln_rbtree_free(t);
    PASS();
}

/* Test 18: root macro */
static void test_root_macro(void)
{
    int vals[] = {5, 3, 7};
    struct mln_rbtree_attr rbattr;

    rbattr.pool = NULL;
    rbattr.pool_alloc = NULL;
    rbattr.pool_free = NULL;
    rbattr.cmp = cmp_int;
    rbattr.data_free = NULL;

    mln_rbtree_t *t = mln_rbtree_new(&rbattr);
    assert(t != NULL);

    /* root of empty tree is nil */
    assert(mln_rbtree_null(mln_rbtree_root(t), t));

    for (int i = 0; i < 3; i++) {
        mln_rbtree_node_t *rn = mln_rbtree_node_new(t, &vals[i]);
        assert(rn != NULL);
        mln_rbtree_insert(t, rn);
    }

    assert(!mln_rbtree_null(mln_rbtree_root(t), t));

    mln_rbtree_free(t);
    PASS();
}

/* Test 19: duplicate values */
static void test_duplicate_values(void)
{
    int vals[] = {5, 5, 5, 3, 3, 7, 7};
    int n = sizeof(vals) / sizeof(vals[0]);
    struct mln_rbtree_attr rbattr;

    rbattr.pool = NULL;
    rbattr.pool_alloc = NULL;
    rbattr.pool_free = NULL;
    rbattr.cmp = cmp_int;
    rbattr.data_free = NULL;

    mln_rbtree_t *t = mln_rbtree_new(&rbattr);
    assert(t != NULL);

    for (int i = 0; i < n; i++) {
        mln_rbtree_node_t *rn = mln_rbtree_node_new(t, &vals[i]);
        assert(rn != NULL);
        mln_rbtree_insert(t, rn);
    }
    assert(mln_rbtree_node_num(t) == (mln_uauto_t)n);

    /* search should find at least one */
    int key = 5;
    mln_rbtree_node_t *rn = mln_rbtree_search(t, &key);
    assert(!mln_rbtree_null(rn, t));

    mln_rbtree_free(t);
    PASS();
}

/* Test 20: large tree (1000 nodes) */
static void test_large_tree(void)
{
    struct mln_rbtree_attr rbattr;

    rbattr.pool = NULL;
    rbattr.pool_alloc = NULL;
    rbattr.pool_free = NULL;
    rbattr.cmp = cmp_int;
    rbattr.data_free = free_int;

    mln_rbtree_t *t = mln_rbtree_new(&rbattr);
    assert(t != NULL);

    int count = 1000;
    for (int i = 0; i < count; i++) {
        int *val = (int *)malloc(sizeof(int));
        assert(val != NULL);
        *val = i;
        mln_rbtree_node_t *rn = mln_rbtree_node_new(t, val);
        assert(rn != NULL);
        mln_rbtree_insert(t, rn);
    }
    assert(mln_rbtree_node_num(t) == (mln_uauto_t)count);

    /* verify all present */
    for (int i = 0; i < count; i++) {
        mln_rbtree_node_t *rn = mln_rbtree_search(t, &i);
        assert(!mln_rbtree_null(rn, t));
    }

    /* min is 0 */
    mln_rbtree_node_t *rn = mln_rbtree_min(t);
    assert(!mln_rbtree_null(rn, t));
    assert(*(int *)mln_rbtree_node_data_get(rn) == 0);

    mln_rbtree_free(t);
    PASS();
}

/* Test 21: sequential insert/delete stress */
static void test_sequential_delete(void)
{
    struct mln_rbtree_attr rbattr;

    rbattr.pool = NULL;
    rbattr.pool_alloc = NULL;
    rbattr.pool_free = NULL;
    rbattr.cmp = cmp_int;
    rbattr.data_free = NULL;

    mln_rbtree_t *t = mln_rbtree_new(&rbattr);
    assert(t != NULL);

    int vals[100];
    mln_rbtree_node_t *nodes[100];
    for (int i = 0; i < 100; i++) {
        vals[i] = i;
        nodes[i] = mln_rbtree_node_new(t, &vals[i]);
        assert(nodes[i] != NULL);
        mln_rbtree_insert(t, nodes[i]);
    }

    /* delete every other node */
    for (int i = 0; i < 100; i += 2) {
        mln_rbtree_delete(t, nodes[i]);
        mln_rbtree_node_free(t, nodes[i]);
    }
    assert(mln_rbtree_node_num(t) == 50);

    /* verify odd values still present */
    for (int i = 1; i < 100; i += 2) {
        mln_rbtree_node_t *rn = mln_rbtree_search(t, &vals[i]);
        assert(!mln_rbtree_null(rn, t));
    }

    /* verify even values removed */
    for (int i = 0; i < 100; i += 2) {
        mln_rbtree_node_t *rn = mln_rbtree_search(t, &vals[i]);
        assert(mln_rbtree_null(rn, t));
    }

    mln_rbtree_free(t);
    PASS();
}

/* Test 22: delete during iterate */
struct del_iter_ctx {
    mln_rbtree_t *t;
    int target;
    int visited;
};

static int delete_during_iterate_handler(mln_rbtree_node_t *node, void *udata)
{
    struct del_iter_ctx *ctx = (struct del_iter_ctx *)udata;
    int val = *(int *)mln_rbtree_node_data_get(node);
    ctx->visited++;
    if (val == ctx->target) {
        mln_rbtree_delete(ctx->t, node);
        mln_rbtree_node_free(ctx->t, node);
    }
    return 0;
}

static void test_delete_during_iterate(void)
{
    int vals[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    int n = sizeof(vals) / sizeof(vals[0]);
    struct mln_rbtree_attr rbattr;

    rbattr.pool = NULL;
    rbattr.pool_alloc = NULL;
    rbattr.pool_free = NULL;
    rbattr.cmp = cmp_int;
    rbattr.data_free = NULL;

    mln_rbtree_t *t = mln_rbtree_new(&rbattr);
    assert(t != NULL);

    for (int i = 0; i < n; i++) {
        mln_rbtree_node_t *rn = mln_rbtree_node_new(t, &vals[i]);
        assert(rn != NULL);
        mln_rbtree_insert(t, rn);
    }

    struct del_iter_ctx ctx;
    ctx.t = t;
    ctx.target = 5;
    ctx.visited = 0;

    int ret = mln_rbtree_iterate(t, delete_during_iterate_handler, &ctx);
    assert(ret == 0);
    assert(mln_rbtree_node_num(t) == (mln_uauto_t)(n - 1));

    /* verify target is gone */
    int key = 5;
    mln_rbtree_node_t *rn = mln_rbtree_search(t, &key);
    assert(mln_rbtree_null(rn, t));

    mln_rbtree_free(t);
    PASS();
}

/* Test 23: performance benchmark - insert + search */
static void test_performance_benchmark(void)
{
    int count = 500000;
    struct mln_rbtree_attr rbattr;

    rbattr.pool = NULL;
    rbattr.pool_alloc = NULL;
    rbattr.pool_free = NULL;
    rbattr.cmp = cmp_int;
    rbattr.data_free = NULL;

    int *vals = (int *)malloc(count * sizeof(int));
    assert(vals != NULL);
    mln_rbtree_node_t **nodes = (mln_rbtree_node_t **)malloc(count * sizeof(mln_rbtree_node_t *));
    assert(nodes != NULL);

    srand(42);
    for (int i = 0; i < count; i++) {
        vals[i] = rand();
    }

    mln_rbtree_t *t = mln_rbtree_new(&rbattr);
    assert(t != NULL);

    /* benchmark insert */
    clock_t start = clock();
    for (int i = 0; i < count; i++) {
        nodes[i] = mln_rbtree_node_new(t, &vals[i]);
        assert(nodes[i] != NULL);
        mln_rbtree_insert(t, nodes[i]);
    }
    clock_t insert_time = clock() - start;
    assert(mln_rbtree_node_num(t) == (mln_uauto_t)count);

    /* benchmark search */
    start = clock();
    for (int i = 0; i < count; i++) {
        mln_rbtree_node_t *rn = mln_rbtree_search(t, &vals[i]);
        assert(!mln_rbtree_null(rn, t));
    }
    clock_t search_time = clock() - start;

    /* benchmark delete */
    start = clock();
    for (int i = 0; i < count; i++) {
        mln_rbtree_delete(t, nodes[i]);
        mln_rbtree_node_free(t, nodes[i]);
    }
    clock_t delete_time = clock() - start;
    assert(mln_rbtree_node_num(t) == 0);

    printf("    Performance (%d ops): insert=%.3fms search=%.3fms delete=%.3fms\n",
           count,
           (double)insert_time / CLOCKS_PER_SEC * 1000,
           (double)search_time / CLOCKS_PER_SEC * 1000,
           (double)delete_time / CLOCKS_PER_SEC * 1000);

    mln_rbtree_free(t);
    free(vals);
    free(nodes);
    PASS();
}

/* Test 24: performance benchmark - inline insert + search */
static void test_inline_performance_benchmark(void)
{
    int count = 500000;
    int *vals = (int *)malloc(count * sizeof(int));
    assert(vals != NULL);
    mln_rbtree_node_t **nodes = (mln_rbtree_node_t **)malloc(count * sizeof(mln_rbtree_node_t *));
    assert(nodes != NULL);

    srand(42);
    for (int i = 0; i < count; i++) {
        vals[i] = rand();
    }

    mln_rbtree_t *t = mln_rbtree_new(NULL);
    assert(t != NULL);

    /* benchmark inline insert */
    clock_t start = clock();
    for (int i = 0; i < count; i++) {
        nodes[i] = mln_rbtree_node_new(t, &vals[i]);
        assert(nodes[i] != NULL);
        mln_rbtree_inline_insert(t, nodes[i], cmp_int_inline);
    }
    clock_t insert_time = clock() - start;

    /* benchmark inline search */
    start = clock();
    for (int i = 0; i < count; i++) {
        mln_rbtree_node_t *rn = mln_rbtree_inline_search(t, &vals[i], cmp_int_inline);
        assert(!mln_rbtree_null(rn, t));
    }
    clock_t search_time = clock() - start;

    /* benchmark delete */
    start = clock();
    for (int i = 0; i < count; i++) {
        mln_rbtree_delete(t, nodes[i]);
        mln_rbtree_node_free(t, nodes[i]);
    }
    clock_t delete_time = clock() - start;

    printf("    Inline Performance (%d ops): insert=%.3fms search=%.3fms delete=%.3fms\n",
           count,
           (double)insert_time / CLOCKS_PER_SEC * 1000,
           (double)search_time / CLOCKS_PER_SEC * 1000,
           (double)delete_time / CLOCKS_PER_SEC * 1000);

    mln_rbtree_free(t);
    free(vals);
    free(nodes);
    PASS();
}

/* Test 25: stability - repeated insert/delete/search cycles */
static void test_stability(void)
{
    int rounds = 1000;
    int ops_per_round = 100;
    struct mln_rbtree_attr rbattr;

    rbattr.pool = NULL;
    rbattr.pool_alloc = NULL;
    rbattr.pool_free = NULL;
    rbattr.cmp = cmp_int;
    rbattr.data_free = NULL;

    mln_rbtree_t *t = mln_rbtree_new(&rbattr);
    assert(t != NULL);

    int *vals = (int *)malloc(ops_per_round * sizeof(int));
    mln_rbtree_node_t **nodes = (mln_rbtree_node_t **)malloc(ops_per_round * sizeof(mln_rbtree_node_t *));
    assert(vals != NULL && nodes != NULL);

    srand(123);
    for (int r = 0; r < rounds; r++) {
        /* insert */
        for (int i = 0; i < ops_per_round; i++) {
            vals[i] = rand();
            nodes[i] = mln_rbtree_node_new(t, &vals[i]);
            assert(nodes[i] != NULL);
            mln_rbtree_insert(t, nodes[i]);
        }

        /* search */
        for (int i = 0; i < ops_per_round; i++) {
            mln_rbtree_node_t *rn = mln_rbtree_search(t, &vals[i]);
            assert(!mln_rbtree_null(rn, t));
        }

        /* delete all */
        for (int i = 0; i < ops_per_round; i++) {
            mln_rbtree_delete(t, nodes[i]);
            mln_rbtree_node_free(t, nodes[i]);
        }
        assert(mln_rbtree_node_num(t) == 0);
    }

    free(vals);
    free(nodes);
    mln_rbtree_free(t);
    PASS();
}

/* Test 26: single node tree */
static void test_single_node(void)
{
    int val = 42;
    struct mln_rbtree_attr rbattr;

    rbattr.pool = NULL;
    rbattr.pool_alloc = NULL;
    rbattr.pool_free = NULL;
    rbattr.cmp = cmp_int;
    rbattr.data_free = NULL;

    mln_rbtree_t *t = mln_rbtree_new(&rbattr);
    assert(t != NULL);

    mln_rbtree_node_t *rn = mln_rbtree_node_new(t, &val);
    assert(rn != NULL);
    mln_rbtree_insert(t, rn);

    assert(mln_rbtree_node_num(t) == 1);
    assert(!mln_rbtree_null(mln_rbtree_min(t), t));
    assert(*(int *)mln_rbtree_node_data_get(mln_rbtree_min(t)) == 42);

    mln_rbtree_node_t *found = mln_rbtree_search(t, &val);
    assert(!mln_rbtree_null(found, t));

    mln_rbtree_node_t *succ = mln_rbtree_successor(t, found);
    assert(mln_rbtree_null(succ, t));

    mln_rbtree_delete(t, rn);
    mln_rbtree_node_free(t, rn);
    assert(mln_rbtree_node_num(t) == 0);

    mln_rbtree_free(t);
    PASS();
}

/* Test 27: reverse insert order */
static void test_reverse_insert(void)
{
    struct mln_rbtree_attr rbattr;

    rbattr.pool = NULL;
    rbattr.pool_alloc = NULL;
    rbattr.pool_free = NULL;
    rbattr.cmp = cmp_int;
    rbattr.data_free = free_int;

    mln_rbtree_t *t = mln_rbtree_new(&rbattr);
    assert(t != NULL);

    /* insert in descending order */
    for (int i = 999; i >= 0; i--) {
        int *val = (int *)malloc(sizeof(int));
        assert(val != NULL);
        *val = i;
        mln_rbtree_node_t *rn = mln_rbtree_node_new(t, val);
        assert(rn != NULL);
        mln_rbtree_insert(t, rn);
    }

    assert(mln_rbtree_node_num(t) == 1000);
    assert(*(int *)mln_rbtree_node_data_get(mln_rbtree_min(t)) == 0);

    /* iterate and verify sorted order */
    int *prev = NULL;
    int ret = mln_rbtree_iterate(t, iterate_check_sorted, &prev);
    assert(ret == 0);

    mln_rbtree_free(t);
    PASS();
}

int main(void)
{
    printf("rbtree tests:\n");
    test_new_free();
    test_insert_search();
    test_delete();
    test_successor();
    test_min();
    test_iterate();
    test_iterate_early_stop();
    test_iterate_empty();
    test_reset();
    test_data_free_callback();
    test_inline_insert_search();
    test_inline_root_search();
    test_container_usage();
    test_inline_free();
    test_inline_reset();
    test_inline_node_free();
    test_node_data_set();
    test_root_macro();
    test_duplicate_values();
    test_large_tree();
    test_sequential_delete();
    test_delete_during_iterate();
    test_performance_benchmark();
    test_inline_performance_benchmark();
    test_stability();
    test_single_node();
    test_reverse_insert();
    printf("All %d tests passed.\n", test_nr);
    return 0;
}

