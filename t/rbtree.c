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

/* Test 22: delete current node during iterate */
struct del_iter_ctx {
    mln_rbtree_t *t;
    int target;
    int visited;
    int visited_bitmap[11]; /* 0-10 */
};

static int delete_current_iterate_handler(mln_rbtree_node_t *node, void *udata)
{
    struct del_iter_ctx *ctx = (struct del_iter_ctx *)udata;
    int val = *(int *)mln_rbtree_node_data_get(node);
    ctx->visited++;
    ctx->visited_bitmap[val] = 1;
    if (val == ctx->target) {
        mln_rbtree_delete(ctx->t, node);
        mln_rbtree_node_free(ctx->t, node);
    }
    return 0;
}

static void test_delete_current_during_iterate(void)
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
    memset(&ctx, 0, sizeof(ctx));
    ctx.t = t;
    ctx.target = 5;

    int ret = mln_rbtree_iterate(t, delete_current_iterate_handler, &ctx);
    assert(ret == 0);
    assert(mln_rbtree_node_num(t) == (mln_uauto_t)(n - 1));
    /* all 10 nodes visited (5 was visited then deleted) */
    assert(ctx.visited == 10);
    for (int i = 1; i <= 10; i++)
        assert(ctx.visited_bitmap[i] == 1);

    /* verify target is gone */
    int key = 5;
    mln_rbtree_node_t *rn = mln_rbtree_search(t, &key);
    assert(mln_rbtree_null(rn, t));

    mln_rbtree_free(t);
    PASS();
}

/* Test 23: delete NEXT node during iterate */
struct del_next_ctx {
    mln_rbtree_t *t;
    int trigger_val;
    int visited;
    int visited_bitmap[11]; /* 0-10 */
};

static int delete_next_iterate_handler(mln_rbtree_node_t *node, void *udata)
{
    struct del_next_ctx *ctx = (struct del_next_ctx *)udata;
    int val = *(int *)mln_rbtree_node_data_get(node);
    ctx->visited++;
    ctx->visited_bitmap[val] = 1;
    if (val == ctx->trigger_val) {
        /* delete the successor (next node) */
        mln_rbtree_node_t *succ = mln_rbtree_successor(ctx->t, node);
        if (!mln_rbtree_null(succ, ctx->t)) {
            mln_rbtree_delete(ctx->t, succ);
            mln_rbtree_node_free(ctx->t, succ);
        }
    }
    return 0;
}

static void test_delete_next_during_iterate(void)
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

    struct del_next_ctx ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.t = t;
    ctx.trigger_val = 5; /* when visiting 5, delete 6 */

    int ret = mln_rbtree_iterate(t, delete_next_iterate_handler, &ctx);
    assert(ret == 0);
    assert(mln_rbtree_node_num(t) == (mln_uauto_t)(n - 1));
    /* 9 nodes visited: 1-5 and 7-10. Node 6 deleted before visited */
    assert(ctx.visited == 9);
    for (int i = 1; i <= 10; i++) {
        if (i == 6)
            assert(ctx.visited_bitmap[i] == 0);
        else
            assert(ctx.visited_bitmap[i] == 1);
    }

    /* verify 6 is gone, all others present */
    int key = 6;
    mln_rbtree_node_t *rn = mln_rbtree_search(t, &key);
    assert(mln_rbtree_null(rn, t));
    for (int i = 1; i <= 10; i++) {
        if (i == 6) continue;
        rn = mln_rbtree_search(t, &vals[i - 1]);
        assert(!mln_rbtree_null(rn, t));
    }

    mln_rbtree_free(t);
    PASS();
}

/* Test 24: delete BOTH current and next during iterate */
struct del_both_ctx {
    mln_rbtree_t *t;
    int trigger_val;
    int visited;
    int visited_bitmap[11]; /* 0-10 */
};

static int delete_both_iterate_handler(mln_rbtree_node_t *node, void *udata)
{
    struct del_both_ctx *ctx = (struct del_both_ctx *)udata;
    int val = *(int *)mln_rbtree_node_data_get(node);
    ctx->visited++;
    ctx->visited_bitmap[val] = 1;
    if (val == ctx->trigger_val) {
        /* delete the successor (next node) first */
        mln_rbtree_node_t *succ = mln_rbtree_successor(ctx->t, node);
        if (!mln_rbtree_null(succ, ctx->t)) {
            mln_rbtree_delete(ctx->t, succ);
            mln_rbtree_node_free(ctx->t, succ);
        }
        /* then delete the current node */
        mln_rbtree_delete(ctx->t, node);
        mln_rbtree_node_free(ctx->t, node);
    }
    return 0;
}

static void test_delete_both_during_iterate(void)
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

    struct del_both_ctx ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.t = t;
    ctx.trigger_val = 5; /* when visiting 5, delete 6 then 5 */

    int ret = mln_rbtree_iterate(t, delete_both_iterate_handler, &ctx);
    assert(ret == 0);
    assert(mln_rbtree_node_num(t) == (mln_uauto_t)(n - 2));
    /* 9 nodes visited: 1-5 and 7-10. 5 visited then deleted, 6 not visited */
    assert(ctx.visited == 9);
    for (int i = 1; i <= 10; i++) {
        if (i == 6)
            assert(ctx.visited_bitmap[i] == 0);
        else
            assert(ctx.visited_bitmap[i] == 1);
    }

    mln_rbtree_free(t);
    PASS();
}

/* ===== Traversal order verification helpers ===== */

static void preorder_collect(mln_rbtree_t *t, mln_rbtree_node_t *n, int *out, int *idx)
{
    if (mln_rbtree_null(n, t)) return;
    out[(*idx)++] = *(int *)mln_rbtree_node_data_get(n);
    preorder_collect(t, n->left, out, idx);
    preorder_collect(t, n->right, out, idx);
}

static void inorder_collect(mln_rbtree_t *t, mln_rbtree_node_t *n, int *out, int *idx)
{
    if (mln_rbtree_null(n, t)) return;
    inorder_collect(t, n->left, out, idx);
    out[(*idx)++] = *(int *)mln_rbtree_node_data_get(n);
    inorder_collect(t, n->right, out, idx);
}

static void postorder_collect(mln_rbtree_t *t, mln_rbtree_node_t *n, int *out, int *idx)
{
    if (mln_rbtree_null(n, t)) return;
    postorder_collect(t, n->left, out, idx);
    postorder_collect(t, n->right, out, idx);
    out[(*idx)++] = *(int *)mln_rbtree_node_data_get(n);
}

/* Verify red-black tree properties recursively. Returns the black-height, or -1 on error. */
static int verify_rb_properties(mln_rbtree_t *t, mln_rbtree_node_t *n)
{
    if (mln_rbtree_null(n, t)) return 1; /* nil nodes are black, contribute 1 */

    /* Property: BST ordering */
    if (!mln_rbtree_null(n->left, t)) {
        assert(*(int *)mln_rbtree_node_data_get(n->left) <= *(int *)mln_rbtree_node_data_get(n));
    }
    if (!mln_rbtree_null(n->right, t)) {
        assert(*(int *)mln_rbtree_node_data_get(n->right) >= *(int *)mln_rbtree_node_data_get(n));
    }

    /* Property: if node is red, both children must be black */
    if (n->color == M_RB_RED) {
        assert(mln_rbtree_null(n->left, t) || n->left->color == M_RB_BLACK);
        assert(mln_rbtree_null(n->right, t) || n->right->color == M_RB_BLACK);
    }

    int lh = verify_rb_properties(t, n->left);
    int rh = verify_rb_properties(t, n->right);

    /* Property: equal black-height on both sides */
    assert(lh == rh);

    return lh + (n->color == M_RB_BLACK ? 1 : 0);
}

/* Test 25: pre-order traversal verification */
static void test_preorder_traversal(void)
{
    /*
     * Insert {5, 3, 7, 1, 4, 6, 8} and verify pre-order output.
     * After RB balancing with these inserts:
     *       5(B)
     *      / \
     *    3(B) 7(B)
     *   / \   / \
     * 1(R)4(R)6(R)8(R)
     * Pre-order: 5,3,1,4,7,6,8
     */
    int vals[] = {5, 3, 7, 1, 4, 6, 8};
    int n = sizeof(vals) / sizeof(vals[0]);
    struct mln_rbtree_attr rbattr;
    rbattr.pool = NULL; rbattr.pool_alloc = NULL; rbattr.pool_free = NULL;
    rbattr.cmp = cmp_int; rbattr.data_free = NULL;

    mln_rbtree_t *t = mln_rbtree_new(&rbattr);
    assert(t != NULL);
    for (int i = 0; i < n; i++) {
        mln_rbtree_node_t *rn = mln_rbtree_node_new(t, &vals[i]);
        assert(rn != NULL);
        mln_rbtree_insert(t, rn);
    }

    /* Verify RB properties hold */
    assert(mln_rbtree_root(t)->color == M_RB_BLACK); /* root is black */
    verify_rb_properties(t, mln_rbtree_root(t));

    /* Collect and verify pre-order: values should be a valid pre-order of a BST */
    int pre[7]; int idx = 0;
    preorder_collect(t, mln_rbtree_root(t), pre, &idx);
    assert(idx == n);
    /* Pre-order of a BST: first element is root, all elements before first > root go left subtree */
    assert(pre[0] == 5); /* root */
    /* Every pre-order element must exist in original set */
    for (int i = 0; i < n; i++) {
        int found = 0;
        for (int j = 0; j < n; j++) {
            if (pre[i] == vals[j]) { found = 1; break; }
        }
        assert(found);
    }

    mln_rbtree_free(t);
    PASS();
}

/* Test 26: in-order traversal verification */
static void test_inorder_traversal(void)
{
    int vals[] = {5, 3, 7, 1, 4, 6, 8, 2, 9, 0};
    int n = sizeof(vals) / sizeof(vals[0]);
    struct mln_rbtree_attr rbattr;
    rbattr.pool = NULL; rbattr.pool_alloc = NULL; rbattr.pool_free = NULL;
    rbattr.cmp = cmp_int; rbattr.data_free = NULL;

    mln_rbtree_t *t = mln_rbtree_new(&rbattr);
    assert(t != NULL);
    for (int i = 0; i < n; i++) {
        mln_rbtree_node_t *rn = mln_rbtree_node_new(t, &vals[i]);
        assert(rn != NULL);
        mln_rbtree_insert(t, rn);
    }

    /* Verify RB properties */
    verify_rb_properties(t, mln_rbtree_root(t));

    /* In-order must produce sorted sequence */
    int inorder[10]; int idx = 0;
    inorder_collect(t, mln_rbtree_root(t), inorder, &idx);
    assert(idx == n);
    for (int i = 0; i < n; i++)
        assert(inorder[i] == i); /* should be 0,1,2,...,9 */

    mln_rbtree_free(t);
    PASS();
}

/* Test 27: post-order traversal verification */
static void test_postorder_traversal(void)
{
    int vals[] = {5, 3, 7, 1, 4, 6, 8};
    int n = sizeof(vals) / sizeof(vals[0]);
    struct mln_rbtree_attr rbattr;
    rbattr.pool = NULL; rbattr.pool_alloc = NULL; rbattr.pool_free = NULL;
    rbattr.cmp = cmp_int; rbattr.data_free = NULL;

    mln_rbtree_t *t = mln_rbtree_new(&rbattr);
    assert(t != NULL);
    for (int i = 0; i < n; i++) {
        mln_rbtree_node_t *rn = mln_rbtree_node_new(t, &vals[i]);
        assert(rn != NULL);
        mln_rbtree_insert(t, rn);
    }

    /* Verify RB properties */
    verify_rb_properties(t, mln_rbtree_root(t));

    /* Post-order: last element must be root */
    int post[7]; int idx = 0;
    postorder_collect(t, mln_rbtree_root(t), post, &idx);
    assert(idx == n);
    assert(post[n - 1] == *(int *)mln_rbtree_node_data_get(mln_rbtree_root(t))); /* last is root */
    /* Every element must exist in the original set */
    for (int i = 0; i < n; i++) {
        int found = 0;
        for (int j = 0; j < n; j++) {
            if (post[i] == vals[j]) { found = 1; break; }
        }
        assert(found);
    }

    mln_rbtree_free(t);
    PASS();
}

/* Test 28: full RB property verification on larger trees */
static void test_rb_properties(void)
{
    struct mln_rbtree_attr rbattr;
    rbattr.pool = NULL; rbattr.pool_alloc = NULL; rbattr.pool_free = NULL;
    rbattr.cmp = cmp_int; rbattr.data_free = NULL;

    /* Sequential insert */
    mln_rbtree_t *t = mln_rbtree_new(&rbattr);
    assert(t != NULL);
    int vals1[100];
    for (int i = 0; i < 100; i++) {
        vals1[i] = i;
        mln_rbtree_node_t *rn = mln_rbtree_node_new(t, &vals1[i]);
        assert(rn != NULL);
        mln_rbtree_insert(t, rn);
    }
    assert(mln_rbtree_root(t)->color == M_RB_BLACK);
    verify_rb_properties(t, mln_rbtree_root(t));
    /* In-order must be sorted */
    int inorder[100]; int idx = 0;
    inorder_collect(t, mln_rbtree_root(t), inorder, &idx);
    assert(idx == 100);
    for (int i = 0; i < 100; i++) assert(inorder[i] == i);
    mln_rbtree_free(t);

    /* Reverse insert */
    t = mln_rbtree_new(&rbattr);
    assert(t != NULL);
    int vals2[100];
    for (int i = 0; i < 100; i++) {
        vals2[i] = 99 - i;
        mln_rbtree_node_t *rn = mln_rbtree_node_new(t, &vals2[i]);
        assert(rn != NULL);
        mln_rbtree_insert(t, rn);
    }
    assert(mln_rbtree_root(t)->color == M_RB_BLACK);
    verify_rb_properties(t, mln_rbtree_root(t));
    idx = 0;
    inorder_collect(t, mln_rbtree_root(t), inorder, &idx);
    assert(idx == 100);
    for (int i = 0; i < 100; i++) assert(inorder[i] == i);
    mln_rbtree_free(t);

    /* Random insert */
    t = mln_rbtree_new(&rbattr);
    assert(t != NULL);
    int vals3[200];
    srand(9999);
    for (int i = 0; i < 200; i++) {
        vals3[i] = rand() % 10000;
        mln_rbtree_node_t *rn = mln_rbtree_node_new(t, &vals3[i]);
        assert(rn != NULL);
        mln_rbtree_insert(t, rn);
    }
    assert(mln_rbtree_root(t)->color == M_RB_BLACK);
    verify_rb_properties(t, mln_rbtree_root(t));
    mln_rbtree_free(t);

    PASS();
}

/* Test 29: RB properties hold after deletions */
static void test_rb_properties_after_delete(void)
{
    struct mln_rbtree_attr rbattr;
    rbattr.pool = NULL; rbattr.pool_alloc = NULL; rbattr.pool_free = NULL;
    rbattr.cmp = cmp_int; rbattr.data_free = NULL;

    mln_rbtree_t *t = mln_rbtree_new(&rbattr);
    assert(t != NULL);
    int vals[50];
    mln_rbtree_node_t *nodes[50];
    for (int i = 0; i < 50; i++) {
        vals[i] = i;
        nodes[i] = mln_rbtree_node_new(t, &vals[i]);
        assert(nodes[i] != NULL);
        mln_rbtree_insert(t, nodes[i]);
    }

    /* Delete every 3rd node and verify RB properties each time */
    for (int i = 0; i < 50; i += 3) {
        mln_rbtree_delete(t, nodes[i]);
        mln_rbtree_node_free(t, nodes[i]);
        if (mln_rbtree_node_num(t) > 0) {
            assert(mln_rbtree_root(t)->color == M_RB_BLACK);
            verify_rb_properties(t, mln_rbtree_root(t));
        }
    }

    mln_rbtree_free(t);
    PASS();
}

/* ===== Comprehensive iterate-with-delete tests ===== */

/*
 * Generic iterate-delete test helper.
 * Builds a tree with values 0..n-1, then iterates.
 * The callback deletes nodes whose value is in the delete_set.
 * After iterate, verifies:
 *   - Every non-deleted node was visited exactly once
 *   - Deleted nodes that were the current node were visited exactly once
 *   - Deleted nodes that were NOT the current node (i.e. deleted as "next") were NOT visited
 *   - Total visit count and final tree size are correct
 */

#define MAX_TREE_SIZE 128

struct iter_del_generic_ctx {
    mln_rbtree_t *t;
    int n;
    int delete_set[MAX_TREE_SIZE];   /* 1 = should delete when current */
    int delete_next_set[MAX_TREE_SIZE]; /* 1 = when visiting this val, delete its successor */
    int visit_count[MAX_TREE_SIZE];
};

static int iter_del_generic_handler(mln_rbtree_node_t *node, void *udata)
{
    struct iter_del_generic_ctx *ctx = (struct iter_del_generic_ctx *)udata;
    int val = *(int *)mln_rbtree_node_data_get(node);
    ctx->visit_count[val]++;

    /* Delete next node first (if requested) */
    if (ctx->delete_next_set[val]) {
        mln_rbtree_node_t *succ = mln_rbtree_successor(ctx->t, node);
        if (!mln_rbtree_null(succ, ctx->t)) {
            mln_rbtree_delete(ctx->t, succ);
            mln_rbtree_node_free(ctx->t, succ);
        }
    }

    /* Delete current node (if requested) */
    if (ctx->delete_set[val]) {
        mln_rbtree_delete(ctx->t, node);
        mln_rbtree_node_free(ctx->t, node);
    }

    return 0;
}

static mln_rbtree_t *build_int_tree(int *vals, int n)
{
    struct mln_rbtree_attr rbattr;
    rbattr.pool = NULL; rbattr.pool_alloc = NULL; rbattr.pool_free = NULL;
    rbattr.cmp = cmp_int; rbattr.data_free = NULL;

    mln_rbtree_t *t = mln_rbtree_new(&rbattr);
    assert(t != NULL);
    for (int i = 0; i < n; i++) {
        mln_rbtree_node_t *rn = mln_rbtree_node_new(t, &vals[i]);
        assert(rn != NULL);
        mln_rbtree_insert(t, rn);
    }
    return t;
}

/* Test 30: iterate-delete: delete the minimum (first visited) node */
static void test_iter_del_min(void)
{
    int vals[20];
    for (int i = 0; i < 20; i++) vals[i] = i;
    mln_rbtree_t *t = build_int_tree(vals, 20);

    struct iter_del_generic_ctx ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.t = t; ctx.n = 20;
    ctx.delete_set[0] = 1; /* delete min (value 0) */

    int ret = mln_rbtree_iterate(t, iter_del_generic_handler, &ctx);
    assert(ret == 0);
    assert(mln_rbtree_node_num(t) == 19);
    /* All 20 nodes visited exactly once (0 was visited then deleted) */
    for (int i = 0; i < 20; i++) assert(ctx.visit_count[i] == 1);

    mln_rbtree_free(t);
    PASS();
}

/* Test 31: iterate-delete: delete the maximum (last visited) node */
static void test_iter_del_max(void)
{
    int vals[20];
    for (int i = 0; i < 20; i++) vals[i] = i;
    mln_rbtree_t *t = build_int_tree(vals, 20);

    struct iter_del_generic_ctx ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.t = t; ctx.n = 20;
    ctx.delete_set[19] = 1; /* delete max (value 19) */

    int ret = mln_rbtree_iterate(t, iter_del_generic_handler, &ctx);
    assert(ret == 0);
    assert(mln_rbtree_node_num(t) == 19);
    for (int i = 0; i < 20; i++) assert(ctx.visit_count[i] == 1);

    mln_rbtree_free(t);
    PASS();
}

/* Test 32: iterate-delete: delete the root node */
static void test_iter_del_root(void)
{
    int vals[15];
    for (int i = 0; i < 15; i++) vals[i] = i;
    mln_rbtree_t *t = build_int_tree(vals, 15);

    int root_val = *(int *)mln_rbtree_node_data_get(mln_rbtree_root(t));

    struct iter_del_generic_ctx ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.t = t; ctx.n = 15;
    ctx.delete_set[root_val] = 1;

    int ret = mln_rbtree_iterate(t, iter_del_generic_handler, &ctx);
    assert(ret == 0);
    assert(mln_rbtree_node_num(t) == 14);
    for (int i = 0; i < 15; i++) assert(ctx.visit_count[i] == 1);

    mln_rbtree_free(t);
    PASS();
}

/* Test 33: iterate-delete: delete a leaf node (no children) */
static void test_iter_del_leaf(void)
{
    /* Insert in specific order so that certain nodes are leaves */
    int vals[] = {4, 2, 6, 1, 3, 5, 7};
    int n = 7;
    mln_rbtree_t *t = build_int_tree(vals, n);

    /* Find a leaf node: 1 should be a leaf (leftmost) */
    struct iter_del_generic_ctx ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.t = t; ctx.n = 8; /* values 0-7, using indices up to 7 */
    ctx.delete_set[1] = 1; /* delete leaf node with value 1 */

    int ret = mln_rbtree_iterate(t, iter_del_generic_handler, &ctx);
    assert(ret == 0);
    assert(mln_rbtree_node_num(t) == 6);
    for (int i = 0; i < n; i++) assert(ctx.visit_count[vals[i]] == 1);

    mln_rbtree_free(t);
    PASS();
}

/* Test 34: iterate-delete: delete node with one child */
static void test_iter_del_one_child(void)
{
    /* Build tree where a node has exactly one child */
    int vals[] = {10, 5, 15, 3, 7, 12, 20, 1};
    int n = sizeof(vals) / sizeof(vals[0]);
    mln_rbtree_t *t = build_int_tree(vals, n);

    /* Value 3 should have left child 1 but no right child in typical RB layout */
    struct iter_del_generic_ctx ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.t = t; ctx.n = 21;
    ctx.delete_set[3] = 1;

    int ret = mln_rbtree_iterate(t, iter_del_generic_handler, &ctx);
    assert(ret == 0);
    assert(mln_rbtree_node_num(t) == (mln_uauto_t)(n - 1));
    for (int i = 0; i < n; i++) assert(ctx.visit_count[vals[i]] == 1);

    mln_rbtree_free(t);
    PASS();
}

/* Test 35: iterate-delete: delete node with two children */
static void test_iter_del_two_children(void)
{
    int vals[] = {10, 5, 15, 3, 7, 12, 20};
    int n = sizeof(vals) / sizeof(vals[0]);
    mln_rbtree_t *t = build_int_tree(vals, n);

    /* Value 5 has two children (3 and 7). Delete it during iterate. */
    struct iter_del_generic_ctx ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.t = t; ctx.n = 21;
    ctx.delete_set[5] = 1;

    int ret = mln_rbtree_iterate(t, iter_del_generic_handler, &ctx);
    assert(ret == 0);
    assert(mln_rbtree_node_num(t) == (mln_uauto_t)(n - 1));
    for (int i = 0; i < n; i++) assert(ctx.visit_count[vals[i]] == 1);
    /* Verify 5 is gone */
    int key = 5;
    assert(mln_rbtree_null(mln_rbtree_search(t, &key), t));

    mln_rbtree_free(t);
    PASS();
}

/* Test 36: iterate-delete: delete next of first node (delete 2nd visited) */
static void test_iter_del_next_of_first(void)
{
    int vals[20];
    for (int i = 0; i < 20; i++) vals[i] = i;
    mln_rbtree_t *t = build_int_tree(vals, 20);

    struct iter_del_generic_ctx ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.t = t; ctx.n = 20;
    ctx.delete_next_set[0] = 1; /* when visiting 0 (min), delete its successor (1) */

    int ret = mln_rbtree_iterate(t, iter_del_generic_handler, &ctx);
    assert(ret == 0);
    assert(mln_rbtree_node_num(t) == 19);
    /* Node 0 visited, node 1 deleted before visit, nodes 2-19 visited */
    assert(ctx.visit_count[0] == 1);
    assert(ctx.visit_count[1] == 0);
    for (int i = 2; i < 20; i++) assert(ctx.visit_count[i] == 1);

    mln_rbtree_free(t);
    PASS();
}

/* Test 37: iterate-delete: delete next of second-to-last (delete max via next) */
static void test_iter_del_next_of_penultimate(void)
{
    int vals[20];
    for (int i = 0; i < 20; i++) vals[i] = i;
    mln_rbtree_t *t = build_int_tree(vals, 20);

    struct iter_del_generic_ctx ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.t = t; ctx.n = 20;
    ctx.delete_next_set[18] = 1; /* when visiting 18, delete 19 (max) */

    int ret = mln_rbtree_iterate(t, iter_del_generic_handler, &ctx);
    assert(ret == 0);
    assert(mln_rbtree_node_num(t) == 19);
    for (int i = 0; i < 19; i++) assert(ctx.visit_count[i] == 1);
    assert(ctx.visit_count[19] == 0); /* 19 was deleted before visit */

    mln_rbtree_free(t);
    PASS();
}

/* Test 38: iterate-delete: delete multiple consecutive nodes (current) */
static void test_iter_del_multiple_consecutive(void)
{
    int vals[20];
    for (int i = 0; i < 20; i++) vals[i] = i;
    mln_rbtree_t *t = build_int_tree(vals, 20);

    struct iter_del_generic_ctx ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.t = t; ctx.n = 20;
    /* Delete nodes 5,6,7,8,9 (consecutive range in the middle) */
    for (int i = 5; i <= 9; i++) ctx.delete_set[i] = 1;

    int ret = mln_rbtree_iterate(t, iter_del_generic_handler, &ctx);
    assert(ret == 0);
    assert(mln_rbtree_node_num(t) == 15);
    /* All 20 visited (deleted ones were visited then deleted) */
    for (int i = 0; i < 20; i++) assert(ctx.visit_count[i] == 1);

    mln_rbtree_free(t);
    PASS();
}

/* Test 39: iterate-delete: delete multiple via next (skip every other) */
static void test_iter_del_skip_every_other(void)
{
    int vals[20];
    for (int i = 0; i < 20; i++) vals[i] = i;
    mln_rbtree_t *t = build_int_tree(vals, 20);

    struct iter_del_generic_ctx ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.t = t; ctx.n = 20;
    /* When visiting even nodes, delete the next (odd) node */
    for (int i = 0; i < 20; i += 2) ctx.delete_next_set[i] = 1;

    int ret = mln_rbtree_iterate(t, iter_del_generic_handler, &ctx);
    assert(ret == 0);
    assert(mln_rbtree_node_num(t) == 10); /* 10 odd nodes deleted */
    /* Even nodes visited, odd nodes not visited (deleted before their turn) */
    for (int i = 0; i < 20; i++) {
        if (i % 2 == 0) assert(ctx.visit_count[i] == 1);
        else assert(ctx.visit_count[i] == 0);
    }

    mln_rbtree_free(t);
    PASS();
}

/* Test 40: iterate-delete: delete ALL nodes during iterate (current) */
static void test_iter_del_all(void)
{
    int vals[20];
    for (int i = 0; i < 20; i++) vals[i] = i;
    mln_rbtree_t *t = build_int_tree(vals, 20);

    struct iter_del_generic_ctx ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.t = t; ctx.n = 20;
    for (int i = 0; i < 20; i++) ctx.delete_set[i] = 1;

    int ret = mln_rbtree_iterate(t, iter_del_generic_handler, &ctx);
    assert(ret == 0);
    assert(mln_rbtree_node_num(t) == 0);
    for (int i = 0; i < 20; i++) assert(ctx.visit_count[i] == 1);

    mln_rbtree_free(t);
    PASS();
}

/* Test 41: iterate-delete: delete both current and next at multiple positions */
static void test_iter_del_both_multiple(void)
{
    int vals[30];
    for (int i = 0; i < 30; i++) vals[i] = i;
    mln_rbtree_t *t = build_int_tree(vals, 30);

    struct iter_del_generic_ctx ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.t = t; ctx.n = 30;
    /* At positions 5, 15, 25: delete next then current */
    ctx.delete_next_set[5] = 1;  ctx.delete_set[5] = 1;   /* deletes 6 then 5 */
    ctx.delete_next_set[15] = 1; ctx.delete_set[15] = 1;   /* deletes 16 then 15 */
    ctx.delete_next_set[25] = 1; ctx.delete_set[25] = 1;   /* deletes 26 then 25 */

    int ret = mln_rbtree_iterate(t, iter_del_generic_handler, &ctx);
    assert(ret == 0);
    assert(mln_rbtree_node_num(t) == 24); /* 6 nodes deleted */
    /* 5, 15, 25 were visited then deleted */
    assert(ctx.visit_count[5] == 1);
    assert(ctx.visit_count[15] == 1);
    assert(ctx.visit_count[25] == 1);
    /* 6, 16, 26 were deleted before visit */
    assert(ctx.visit_count[6] == 0);
    assert(ctx.visit_count[16] == 0);
    assert(ctx.visit_count[26] == 0);
    /* All others visited exactly once */
    for (int i = 0; i < 30; i++) {
        if (i == 6 || i == 16 || i == 26) continue;
        assert(ctx.visit_count[i] == 1);
    }

    mln_rbtree_free(t);
    PASS();
}

/* Test 42: iterate-delete: delete next when current is second-to-last */
static void test_iter_del_next_near_end(void)
{
    int vals[10];
    for (int i = 0; i < 10; i++) vals[i] = i;
    mln_rbtree_t *t = build_int_tree(vals, 10);

    struct iter_del_generic_ctx ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.t = t; ctx.n = 10;
    ctx.delete_next_set[8] = 1; /* visiting 8, delete 9 (last node) */

    int ret = mln_rbtree_iterate(t, iter_del_generic_handler, &ctx);
    assert(ret == 0);
    assert(mln_rbtree_node_num(t) == 9);
    for (int i = 0; i < 9; i++) assert(ctx.visit_count[i] == 1);
    assert(ctx.visit_count[9] == 0);

    mln_rbtree_free(t);
    PASS();
}

/* Test 43: iterate-delete single node tree - delete the only node */
static void test_iter_del_single_node(void)
{
    int vals[] = {42};
    mln_rbtree_t *t = build_int_tree(vals, 1);

    struct iter_del_generic_ctx ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.t = t; ctx.n = 43;
    ctx.delete_set[42] = 1;

    int ret = mln_rbtree_iterate(t, iter_del_generic_handler, &ctx);
    assert(ret == 0);
    assert(mln_rbtree_node_num(t) == 0);
    assert(ctx.visit_count[42] == 1);

    mln_rbtree_free(t);
    PASS();
}

/* Test 44: iterate-delete two node tree - delete first, verify second visited */
static void test_iter_del_two_nodes(void)
{
    int vals[] = {1, 2};
    mln_rbtree_t *t = build_int_tree(vals, 2);

    struct iter_del_generic_ctx ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.t = t; ctx.n = 3;
    ctx.delete_set[1] = 1; /* delete first visited */

    int ret = mln_rbtree_iterate(t, iter_del_generic_handler, &ctx);
    assert(ret == 0);
    assert(mln_rbtree_node_num(t) == 1);
    assert(ctx.visit_count[1] == 1);
    assert(ctx.visit_count[2] == 1);

    mln_rbtree_free(t);
    PASS();
}

/* Test 45: iterate-delete: large tree (100 nodes), delete scattered positions */
static void test_iter_del_large_scattered(void)
{
    int vals[100];
    for (int i = 0; i < 100; i++) vals[i] = i;
    mln_rbtree_t *t = build_int_tree(vals, 100);

    struct iter_del_generic_ctx ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.t = t; ctx.n = 100;
    /* Delete via current: 0 (min), 50 (mid), 99 (max), primes */
    ctx.delete_set[0] = 1;
    ctx.delete_set[50] = 1;
    ctx.delete_set[99] = 1;
    /* Delete via next: at 10 delete 11, at 30 delete 31, at 70 delete 71 */
    ctx.delete_next_set[10] = 1;
    ctx.delete_next_set[30] = 1;
    ctx.delete_next_set[70] = 1;

    int ret = mln_rbtree_iterate(t, iter_del_generic_handler, &ctx);
    assert(ret == 0);
    assert(mln_rbtree_node_num(t) == 94); /* 6 deleted */
    /* Current-deleted nodes were visited */
    assert(ctx.visit_count[0] == 1);
    assert(ctx.visit_count[50] == 1);
    assert(ctx.visit_count[99] == 1);
    /* Next-deleted nodes were NOT visited */
    assert(ctx.visit_count[11] == 0);
    assert(ctx.visit_count[31] == 0);
    assert(ctx.visit_count[71] == 0);
    /* Everything else visited exactly once */
    for (int i = 0; i < 100; i++) {
        if (i == 11 || i == 31 || i == 71) continue;
        assert(ctx.visit_count[i] == 1);
    }

    /* Verify RB properties still hold after all the deletions */
    if (mln_rbtree_node_num(t) > 0)
        verify_rb_properties(t, mln_rbtree_root(t));

    mln_rbtree_free(t);
    PASS();
}

/* Test 46: performance benchmark - insert + search */
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

/* Test 47: performance benchmark - inline insert + search */
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

/* Test 48: stability - repeated insert/delete/search cycles */
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

/* Test 49: single node tree */
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

/* Test 50: reverse insert order */
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
    test_delete_current_during_iterate();
    test_delete_next_during_iterate();
    test_delete_both_during_iterate();
    test_preorder_traversal();
    test_inorder_traversal();
    test_postorder_traversal();
    test_rb_properties();
    test_rb_properties_after_delete();
    test_iter_del_min();
    test_iter_del_max();
    test_iter_del_root();
    test_iter_del_leaf();
    test_iter_del_one_child();
    test_iter_del_two_children();
    test_iter_del_next_of_first();
    test_iter_del_next_of_penultimate();
    test_iter_del_multiple_consecutive();
    test_iter_del_skip_every_other();
    test_iter_del_all();
    test_iter_del_both_multiple();
    test_iter_del_next_near_end();
    test_iter_del_single_node();
    test_iter_del_two_nodes();
    test_iter_del_large_scattered();
    test_performance_benchmark();
    test_inline_performance_benchmark();
    test_stability();
    test_single_node();
    test_reverse_insert();
    printf("All %d tests passed.\n", test_nr);
    return 0;
}

