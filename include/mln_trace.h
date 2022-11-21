
/*
 * Copyright (C) Niklaus F.Schen.
 */
#ifndef __MLN_TRACE_H
#define __MLN_TRACE_H

#include "mln_lang.h"

#define mln_trace(fmt, ...) \
    ({\
        int ret = 0;\
        mln_lang_ctx_t *__trace_ctx = mln_trace_task_get();\
        if (__trace_ctx != NULL) ret = mln_lang_ctx_pipe_send(__trace_ctx, fmt, ## __VA_ARGS__);\
        ret;\
    })

extern int mln_trace_init(void);
extern mln_lang_ctx_t *mln_trace_task_get(void);

#endif

