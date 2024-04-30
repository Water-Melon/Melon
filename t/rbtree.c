#include <stdio.h>
#include <stdlib.h>
#include "mln_rbtree.h"
#include <assert.h>

static inline int cmp_handler(const void *data1, const void *data2)
{
    return *(int *)data1 - *(int *)data2;
}

int test1(void)
{
    int n = 10;
    mln_rbtree_t *t;
    mln_rbtree_node_t *rn;

    assert((t = mln_rbtree_new(NULL)) != NULL);
    assert((rn = mln_rbtree_node_new(t, &n)) != NULL);
    mln_rbtree_inline_insert(t, rn, cmp_handler);

    rn = mln_rbtree_inline_search(t, &n, cmp_handler);
    assert(!mln_rbtree_null(rn, t));
    printf("%d\n", *((int *)mln_rbtree_node_data_get(rn)));

    mln_rbtree_delete(t, rn);
    mln_rbtree_node_free(t, rn);

    mln_rbtree_free(t);

    return 0;
}

typedef struct user_defined_s {
    int val;
    mln_rbtree_node_t node;
} ud_t;

static int cmp2_handler(const void *data1, const void *data2)
{
    return ((ud_t *)data1)->val - ((ud_t *)data2)->val;
}

int test2(void)
{
    mln_rbtree_t *t;
    ud_t data1, data2;
    mln_rbtree_node_t *rn;
    struct mln_rbtree_attr rbattr;

    rbattr.pool = NULL;
    rbattr.pool_alloc = NULL;
    rbattr.pool_free = NULL;
    rbattr.cmp = cmp2_handler;
    rbattr.data_free = NULL;

    assert((t = mln_rbtree_new(&rbattr)) != NULL);

    data1.val = 1;
    data2.val = 2;

    mln_rbtree_node_init(&data1.node, &data1);
    mln_rbtree_node_init(&data2.node, &data2);

    mln_rbtree_insert(t, &data1.node);
    mln_rbtree_insert(t, &data2.node);

    rn = mln_rbtree_search(t, &data1);
    assert(!mln_rbtree_null(rn, t));

    printf("%d\n", ((ud_t *)mln_rbtree_node_data_get(rn))->val);
    printf("%d\n", mln_container_of(rn, ud_t, node)->val);

    mln_rbtree_delete(t, &data1.node);
    mln_rbtree_node_free(t, &data1.node);

    mln_rbtree_free(t);

    return 0;
}

int main(void)
{
    assert(test1() == 0);
    assert(test2() == 0);
    return 0;
}

