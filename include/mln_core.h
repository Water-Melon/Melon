
/*
 * Copyright (C) Niklaus F.Schen.
 */
#ifndef __MLN_CORE_H
#define __MLN_CORE_H

#include "mln_event.h"

typedef int (*mln_core_init_t)(void);
typedef void (*mln_core_worker_process_t)(mln_event_t *);

struct mln_core_attr {
    int                       argc;
    char                    **argv;
    mln_core_init_t           global_init;
    mln_core_worker_process_t worker_process;
};

extern int mln_core_init(struct mln_core_attr *attr) __NONNULL1(1);
#if defined(__GNUC__)
extern int mln_simple_init;
extern int __attribute__((constructor)) mln_core_simple_init(void);
#endif
#endif
