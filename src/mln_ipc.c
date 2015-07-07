
/*
 * Copyright (C) Niklaus F.Schen.
 */
#include "mln_lex.h"
#include "mln_fork.h"
#include "mln_ipc.h"
#include "mln_log.h"
#include "mln_connection.h"
#include "mln_event.h"
#include "mln_fheap.h"
#include "mln_global.h"
#include "mln_hash.h"
#include "mln_prime_generator.h"
#include "mln_rbtree.h"
#include "mln_string.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>

/*
 * IPC only act on A child process and the parent process.
 * If there are some threads in a child process,
 * IPC only act on the control thread (main thread) and the parent process.
 * If you need to send something to the peer,
 * you can call mln_tcp_connection_set_buf() to set connection send buffer
 * and call mln_event_set_fd() to set send event, which means you can
 * customize the send routine.
 */

void conf_reload_master(mln_event_t *ev, void *f_ptr, void *buf, mln_u32_t len, void **udata_ptr)
{
    /*
     * do nothing.
     */
}

void conf_reload_worker(mln_event_t *ev, void *f_ptr, void *buf, mln_u32_t len, void **udata_ptr)
{
    if (mln_conf_reload() < 0) {
        mln_log(error, "mln_conf_reload() failed.\n");
        exit(1);
    }
}


mln_ipc_handler_t ipc_master_handlers[] = {
{conf_reload_master, NULL, M_IPC_CONF_RELOAD}
};
mln_ipc_handler_t ipc_worker_handlers[] = {
{conf_reload_worker, NULL, M_IPC_CONF_RELOAD}
};
void mln_set_ipc_handlers(void)
{
    int i;
    for (i = 0; i < sizeof(ipc_master_handlers)/sizeof(mln_ipc_handler_t); i++) {
        mln_set_master_ipc_handler(&ipc_master_handlers[i]);
    }
    for (i = 0; i < sizeof(ipc_worker_handlers)/sizeof(mln_ipc_handler_t); i++) {
        mln_set_worker_ipc_handler(&ipc_worker_handlers[i]);
    }
}
