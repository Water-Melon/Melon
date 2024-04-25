#include <stdio.h>
#include <stdlib.h>
#include "mln_hash.h"

typedef struct mln_hash_test_s {
    int    key;
    int    val;
} mln_hash_test_t;

static mln_u64_t calc_handler(mln_hash_t *h, void *key)
{
    return *((int *)key) % h->len;
}

static int cmp_handler(mln_hash_t *h, void *key1, void *key2)
{
    return !(*((int *)key1) - *((int *)key2));
}

static void free_handler(void *val)
{
    free(val);
}

int main(int argc, char *argv[])
{
    mln_hash_t *h;
    struct mln_hash_attr hattr;
    mln_hash_test_t *item, *ret;

    hattr.pool = NULL;
    hattr.pool_alloc = NULL;
    hattr.pool_free = NULL;
    hattr.hash = calc_handler;
    hattr.cmp = cmp_handler;
    hattr.key_freer = NULL;
    hattr.val_freer = free_handler;
    hattr.len_base = 97;
    hattr.expandable = 0;
    hattr.calc_prime = 0;

    if ((h = mln_hash_new(&hattr)) == NULL) {
        fprintf(stderr, "Hash init failed.\n");
        return -1;
    }

    item = (mln_hash_test_t *)malloc(sizeof(mln_hash_test_t));
    if (item == NULL) {
        fprintf(stderr, "malloc failed.\n");
        return -1;
    }
    item->key = 1;
    item->val = 10;
    if (mln_hash_insert(h, &(item->key), item) < 0) {
        fprintf(stderr, "insert failed.\n");
        return -1;
    }

    ret = mln_hash_search(h, &(item->key));
    printf("%p %p\n", ret, item);

    mln_hash_free(h, M_HASH_F_VAL);

    return 0;
}
