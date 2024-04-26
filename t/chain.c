#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mln_chain.h"

int main(int argc, char *argv[])
{
    char *p;
    char *buf;
    mln_alloc_t *pool;
    mln_chain_t *c;
    mln_buf_t *b;

    pool = mln_alloc_init(NULL);
    if (pool == NULL) {
        fprintf(stderr, "pool init failed.\n");
        return -1;
    }

    if ((c = mln_chain_new(pool)) == NULL) {
        fprintf(stderr, "chain_new failed.\n");
        return -1;
    }
    if ((b = mln_buf_new(pool)) == NULL) {
        fprintf(stderr, "buf_new failed.\n");
        return -1;
    }
    c->buf = b;
    if ((buf = (char *)mln_alloc_m(pool, 1024)) == NULL) {
        fprintf(stderr, "buffer allocate failed.\n");
        return -1;
    }
    b->left_pos = b->pos = b->start = (mln_u8ptr_t)buf;
    b->last = b->end = (mln_u8ptr_t)buf + 1024;
    b->in_memory = 1;

    buf[0] = 'H', buf[1] = 'i';

    mln_chain_pool_release(c);
    mln_alloc_destroy(pool);

    return 0;
}
