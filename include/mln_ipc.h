
/*
 * Copyright (C) Niklaus F.Schen.
 */
#ifndef __MLN_IPC_H
#define __MLN_IPC_H

#if !defined(MSVC)

#include "mln_types.h"
#include "mln_fork.h"

typedef struct mln_ipc_cb_s {
    struct mln_ipc_cb_s  *next;
    struct mln_ipc_cb_s  *prev;
    ipc_handler           master_handler;
    ipc_handler           worker_handler;
    void                 *master_data;
    void                 *worker_data;
    mln_u32_t             type;
} mln_ipc_cb_t;

extern int mln_ipc_set_process_handlers(void);
/*
 * 'type' - 0~1024 is taken by melon
 */
extern mln_ipc_cb_t *
mln_ipc_handler_register(mln_u32_t type, \
                         ipc_handler master_handler, \
                         ipc_handler worker_handler, \
                         void *master_data, \
                         void *worker_data) __NONNULL2(2,3);
extern void mln_ipc_handler_unregister(mln_ipc_cb_t *cb);
#endif

#endif
