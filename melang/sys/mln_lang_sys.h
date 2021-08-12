
/*
 * Copyright (C) Niklaus F.Schen.
 */
#ifndef __MLN_LANG_SYS_H
#define __MLN_LANG_SYS_H

#include "mln_lang.h"

struct mln_sys_diff_s {
    mln_lang_array_t *dest;
    mln_lang_array_t *notin;
};

extern int mln_lang_sys(mln_lang_ctx_t *ctx);

#endif

