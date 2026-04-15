
/*
 * Copyright (C) Niklaus F.Schen.
 */
#if !defined(MSVC)
#include "mln_ipc.h"
#include "mln_func.h"

/*
 * IPC only act on A child process and the parent process.
 * If there are some threads in a child process,
 * IPC only act on the control thread (main thread) and the parent process.
 * If you need to send something to the peer,
 * you can call mln_tcp_conn_init to initialize a connection and set its
 * send buffer, then call mln_event_fd_set() to set send event.
 * All above operations mean that you can customize the send routine.
 */

/*
 * Hash table for O(1) handler lookup by type, replacing linked list.
 */
#define IPC_CB_HASH_BUCKETS 32
#define IPC_CB_HASH_MASK (IPC_CB_HASH_BUCKETS - 1)

static mln_ipc_cb_t *ipc_cb_hash[IPC_CB_HASH_BUCKETS];

MLN_FUNC(, mln_ipc_cb_t *, mln_ipc_handler_register, \
         (mln_u32_t type, ipc_handler master_handler, ipc_handler worker_handler, \
          void *master_data, void *worker_data), \
         (type, master_handler, worker_handler, master_data, worker_data), \
{
    mln_ipc_cb_t *cb;
    if ((cb = (mln_ipc_cb_t *)malloc(sizeof(mln_ipc_cb_t))) == NULL)
        return NULL;

    cb->master_handler = master_handler;
    cb->worker_handler = worker_handler;
    cb->master_data = master_data;
    cb->worker_data = worker_data;
    cb->type = type;
    cb->prev = NULL;

    mln_u32_t idx = type & IPC_CB_HASH_MASK;
    cb->next = ipc_cb_hash[idx];
    if (cb->next != NULL) cb->next->prev = cb;
    ipc_cb_hash[idx] = cb;

    return cb;
})

MLN_FUNC_VOID(, void, mln_ipc_handler_unregister, (mln_ipc_cb_t *cb), (cb), {
    if (cb == NULL) return;

    mln_u32_t idx = cb->type & IPC_CB_HASH_MASK;
    if (cb->prev != NULL) {
        cb->prev->next = cb->next;
    } else {
        ipc_cb_hash[idx] = cb->next;
    }
    if (cb->next != NULL) {
        cb->next->prev = cb->prev;
    }
    free(cb);
})

MLN_FUNC(, int, mln_ipc_set_process_handlers, (void), (), {
    mln_u32_t i;
    mln_ipc_cb_t *cb;
    for (i = 0; i < IPC_CB_HASH_BUCKETS; ++i) {
        for (cb = ipc_cb_hash[i]; cb != NULL; cb = cb->next) {
            if (mln_fork_master_ipc_handler_set(cb->type, cb->master_handler, cb->master_data) < 0)
                return -1;
            if (mln_fork_worker_ipc_handler_set(cb->type, cb->worker_handler, cb->worker_data) < 0)
                return -1;
        }
    }
    return 0;
})

#endif
