
/*
 * Copyright (C) Niklaus F.Schen.
 */

#include "mln_list.h"
#include "mln_utils.h"
#include "mln_func.h"

MLN_CHAIN_FUNC_DECLARE(static inline, mln_list, mln_list_t,);
MLN_CHAIN_FUNC_DEFINE(static inline, mln_list, mln_list_t, prev, next);

MLN_FUNC_VOID(, void, mln_list_add, (mln_list_t *sentinel, mln_list_t *node), (sentinel, node), {
    mln_list_chain_add(&mln_list_head(sentinel), &mln_list_tail(sentinel), node);
})

MLN_FUNC_VOID(, void, mln_list_remove, (mln_list_t *sentinel, mln_list_t *node), (sentinel, node), {
    mln_list_chain_del(&mln_list_head(sentinel), &mln_list_tail(sentinel), node);
})

