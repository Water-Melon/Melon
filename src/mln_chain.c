
/*
 * Copyright (C) Niklaus F.Schen.
 */

#include "mln_chain.h"
#include "mln_func.h"

mln_buf_t *mln_buf_new(mln_alloc_t *pool)
{
    mln_buf_t *b = mln_alloc_m(pool, sizeof(mln_buf_t));
    b->left_pos = b->pos = b->last = NULL;
    b->start = b->end = NULL;
    b->shadow = NULL;
    b->file_left_pos = b->file_pos = b->file_last = 0;
    b->file = NULL;
    b->temporary = b->in_memory = b->in_file = 0;
#if !defined(MSVC) && defined(MLN_MMAP)
    b->mmap = 0;
#endif
    b->flush = b->sync = b->last_buf = b->last_in_chain = 0;
    return b;
}

MLN_FUNC(, mln_chain_t *, mln_chain_new, (mln_alloc_t *pool), (pool), {
    mln_chain_t *c = mln_alloc_m(pool, sizeof(mln_chain_t));
    c->buf = NULL;
    c->next = NULL;
    return c;
})

void mln_buf_pool_release(mln_buf_t *b)
{
    if (b == NULL) return;

    if (b->shadow != NULL || b->temporary) {
        mln_alloc_free(b);
        return;
    }

    if (b->in_memory) {
#if !defined(MSVC) && defined(MLN_MMAP)
        if (b->mmap) {
            if (b->start != NULL) {
                munmap(b->start, b->end - b->start);
            } else {
                munmap(b->pos, b->last - b->pos);
            }
        } else {
#endif
            if (b->start != NULL) {
                mln_alloc_free(b->start);
            } else {
                mln_alloc_free(b->pos);
            }
#if !defined(MSVC) && defined(MLN_MMAP)
        }
#endif
        mln_alloc_free(b);
        return;
    }

    if (b->in_file) {
        mln_file_close(b->file);
        mln_alloc_free(b);
        return;
    }

    mln_alloc_free(b);
}

MLN_FUNC_VOID(, void, mln_chain_pool_release, (mln_chain_t *c), (c), {
    if (c == NULL) return;

    if (c->buf != NULL) {
        mln_buf_pool_release(c->buf);
    }
    mln_alloc_free(c);
})

MLN_FUNC_VOID(, void, mln_chain_pool_release_all, (mln_chain_t *c), (c), {
    mln_chain_t *fr;

    while (c != NULL) {
        fr = c;
        c = c->next;
        mln_chain_pool_release(fr);
    }
})

