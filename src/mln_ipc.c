
/*
 * Copyright (C) Niklaus F.Schen.
 */
#if !defined(WIN32)
#include "mln_ipc.h"
#include "mln_log.h"

/*
 * IPC only act on A child process and the parent process.
 * If there are some threads in a child process,
 * IPC only act on the control thread (main thread) and the parent process.
 * If you need to send something to the peer,
 * you can call mln_tcp_conn_init to initialize a connection and set its
 * send buffer, then call mln_event_fd_set() to set send event.
 * All above operations mean that you can customize the send routine.
 */

static mln_ipc_set_t *ipcs = NULL;


int mln_ipc_handler_register(mln_u32_t type, \
                             ipc_handler master_handler, \
                             ipc_handler worker_handler, \
                             void *master_data, \
                             void *worker_data)
{
    mln_ipc_set_t *is;
    if ((is = (mln_ipc_set_t *)malloc(sizeof(mln_ipc_set_t))) == NULL)
        return -1;

    is->master_handler = master_handler;
    is->worker_handler = worker_handler;
    is->master_data = master_data;
    is->worker_data = worker_data;
    is->type = type;

    if (ipcs == NULL) {
        is->next = NULL;
        ipcs = is;
    } else {
        is->next = ipcs;
        ipcs = is;
    }

    return 0;
}


int mln_set_ipc_handlers(void)
{
    mln_ipc_set_t *is;
    for (is = ipcs; is != NULL; is = is->next) {
        if (mln_set_master_ipc_handler(is->type, is->master_handler, is->master_data) < 0)
            return -1;
        if (mln_set_worker_ipc_handler(is->type, is->worker_handler, is->worker_data) < 0)
            return -1;
    }
    return 0;
}

#endif
