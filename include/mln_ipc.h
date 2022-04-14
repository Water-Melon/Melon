
/*
 * Copyright (C) Niklaus F.Schen.
 */
#ifndef __MLN_IPC_H
#define __MLN_IPC_H

#if !defined(WIN32)

#include "mln_types.h"
#include "mln_fork.h"

typedef struct mln_ipc_set_s {
    struct mln_ipc_set_s *next;
    ipc_handler           master_handler;
    ipc_handler           worker_handler;
    void                 *master_data;
    void                 *worker_data;
    mln_u32_t             type;
} mln_ipc_set_t;

extern int mln_set_ipc_handlers(void);
/*
 * 'type' - 0~1024 is taken by melon
 */
extern int
mln_ipc_handler_register(mln_u32_t type, \
                         ipc_handler master_handler, \
                         ipc_handler worker_handler, \
                         void *master_data, \
                         void *worker_data) __NONNULL2(2,3);
#endif

#endif
