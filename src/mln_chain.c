
/*
 * Copyright (C) Niklaus F.Schen.
 */

#include "mln_chain.h"
#include "mln_func.h"

MLN_FUNC(, mln_chain_t *, mln_chain_new, (mln_alloc_t *pool), (pool), {
    mln_chain_t *c = mln_alloc_m(pool, sizeof(mln_chain_t));
    c->buf = NULL;
    c->next = NULL;
    return c;
})

MLN_FUNC(, mln_chain_t *, mln_chain_new_with_buf, (mln_alloc_t *pool), (pool), {
    mln_chain_t *c = mln_chain_new(pool);
    if (c == NULL) return NULL;
    c->buf = mln_buf_new(pool);
    if (c->buf == NULL) {
        mln_alloc_free(c);
        return NULL;
    }
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
        if (fr->buf != NULL) {
            mln_buf_pool_release(fr->buf);
        }
        mln_alloc_free(fr);
    }
})
