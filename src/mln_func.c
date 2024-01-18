
/*
 * Copyright (C) Niklaus F.Schen.
 */

#include "mln_func.h"

mln_func_cb_t mln_func_entry = NULL;
mln_func_cb_t mln_func_exit = NULL;

void mln_func_entry_callback_set(mln_func_cb_t cb)
{
    mln_func_entry = cb;
}

mln_func_cb_t mln_func_entry_callback_get(void)
{
    return mln_func_entry;
}

void mln_func_exit_callback_set(mln_func_cb_t cb)
{
    mln_func_exit = cb;
}

mln_func_cb_t mln_func_exit_callback_get(void)
{
    return mln_func_exit;
}
