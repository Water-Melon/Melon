
/*
 * Copyright (C) Niklaus F.Schen.
 */

#include "mln_chain.h"

mln_buf_t *mln_buf_new(mln_alloc_t *pool)
{
    mln_buf_t *b = mln_alloc_m(pool, sizeof(mln_buf_t));
    b->send_pos = b->pos = b->last = NULL;
    b->start = b->end = NULL;
    b->shadow = NULL;
    b->file_send_pos = b->file_pos = b->file_last = 0;
    b->file = NULL;
    b->temporary = b->mmap = b->in_memory = b->in_file = 0;
    b->flush = b->sync = b->last_buf = b->last_in_chain = 0;
    return b;
}

mln_chain_t *mln_chain_new(mln_alloc_t *pool)
{
    mln_chain_t *c = mln_alloc_m(pool, sizeof(mln_chain_t));
    c->buf = NULL;
    c->next = NULL;
    return c;
}

void mln_buf_pool_release(mln_buf_t *b)
{
    if (b == NULL) return;

    if (b->shadow != NULL || b->temporary) {
        mln_alloc_free(b);
        return;
    }

    if (b->in_memory) {
        if (b->start != NULL) {
            mln_alloc_free(b->start);
        } else {
            mln_alloc_free(b->pos);
        }
        mln_alloc_free(b);
        return;
    }

    if (b->in_file) {
        mln_file_close(b->file);
        mln_alloc_free(b);
        return;
    }

    if (b->mmap) {
        if (b->start != NULL) {
            munmap(b->start, b->end - b->start);
        } else {
            munmap(b->pos, b->last - b->pos);
        }
        mln_alloc_free(b);
        return;
    }

    mln_alloc_free(b);
}

void mln_chain_pool_release(mln_chain_t *c)
{
    if (c == NULL) return;

    if (c->buf != NULL) {
        mln_buf_pool_release(c->buf);
    }
    mln_alloc_free(c);
}

void mln_chain_pool_release_all(mln_chain_t *c)
{
    mln_chain_t *fr;

    while (c != NULL) {
        fr = c;
        c = c->next;
        mln_chain_pool_release(fr);
    }
}

