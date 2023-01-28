
/*
 * Copyright (C) Niklaus F.Schen.
 */
#ifndef __MLN_CORE_H
#define __MLN_CORE_H

#include "mln_event.h"

typedef int (*mln_core_init_t)(void);
#if !defined(WIN32)
typedef void (*mln_core_process_t)(mln_event_t *);
#endif

struct mln_core_attr {
    int                       argc;
    char                    **argv;
    mln_core_init_t           global_init;
#if !defined(WIN32)
    mln_core_process_t        main_thread;
    mln_core_process_t        master_process;
    mln_core_process_t        worker_process;
#endif
};

extern int mln_core_init(struct mln_core_attr *attr) __NONNULL1(1);
#endif
