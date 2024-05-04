
/*
 * Copyright (C) Niklaus F.Schen.
 */
#ifndef __MLN_FRAMEWORK_H
#define __MLN_FRAMEWORK_H

#include "mln_event.h"

typedef int (*mln_framework_init_t)(void);
#if !defined(MSVC)
typedef void (*mln_framework_process_t)(mln_event_t *);
#endif

struct mln_framework_attr {
    int                            argc;
    char                         **argv;
    mln_framework_init_t           global_init;
#if !defined(MSVC)
    mln_framework_process_t        main_thread;
    mln_framework_process_t        master_process;
    mln_framework_process_t        worker_process;
#endif
};

extern int mln_framework_init(struct mln_framework_attr *attr) __NONNULL1(1);
#endif
