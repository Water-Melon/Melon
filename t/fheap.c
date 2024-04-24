#include <stdio.h>
#include <stdlib.h>
#include "mln_fheap.h"
#include <assert.h>

typedef struct user_defined_s {
    int val;
    mln_fheap_node_t node; //自定义数据结构的成员
} ud_t;

static inline int container_cmp_handler(const void *key1, const void *key2)
{
    return ((ud_t *)key1)->val < ((ud_t *)key2)->val? 0: 1;
}

int fheap_container_test(void)
{
    mln_fheap_t *fh;
    ud_t min = {0, };
    ud_t data1 = {1, };
    ud_t data2 = {2, };
    mln_fheap_node_t *fn;

    fh = mln_fheap_new(&min, NULL);
    if (fh == NULL) {
        fprintf(stderr, "fheap init failed.\n");
        return -1;
    }

    mln_fheap_node_init(&data1.node, &data1); //初始化堆结点
    mln_fheap_node_init(&data2.node, &data2);
    mln_fheap_inline_insert(fh, &data1.node, container_cmp_handler); //插入堆结点
    mln_fheap_inline_insert(fh, &data2.node, container_cmp_handler);

    fn = mln_fheap_minimum(fh);
    //两种方式获取自定义数据
    printf("%d\n", ((ud_t *)mln_fheap_node_key(fn))->val);
    printf("%d\n", mln_container_of(fn, ud_t, node)->val);

    mln_fheap_inline_free(fh, container_cmp_handler, NULL);

    return 0;
}

static inline int cmp_handler(const void *key1, const void *key2) //inline
{
    return *(int *)key1 < *(int *)key2? 0: 1;
}

int fheap_test(void)
{
    int i = 10, min = 0;
    mln_fheap_t *fh;
    mln_fheap_node_t *fn;

    fh = mln_fheap_new(&min, NULL);
    if (fh == NULL) {
        fprintf(stderr, "fheap init failed.\n");
        return -1;
    }

    fn = mln_fheap_node_new(fh, &i);
    if (fn == NULL) {
        fprintf(stderr, "fheap node init failed.\n");
        return -1;
    }
    mln_fheap_inline_insert(fh, fn, cmp_handler); //inline insert

    fn = mln_fheap_minimum(fh);
    printf("%d\n", *((int *)mln_fheap_node_key(fn)));

    mln_fheap_inline_free(fh, cmp_handler, NULL); //inline free

    return 0;
}

int main(void)
{
    assert(fheap_test() == 0);
    assert(fheap_container_test() == 0);
    return 0;
}

