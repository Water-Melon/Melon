
/*
 * Copyright (C) Niklaus F.Schen.
 */
#ifndef __MLN_FORK_H
#define __MLN_FORK_H
#if !defined(MSVC)
#include <sys/types.h>
#include "mln_types.h"
#include "mln_event.h"
#include "mln_connection.h"

#define STATE_IDLE    0
#define STATE_LENGTH  1
#define M_F_TYPELEN   sizeof(mln_u32_t)
#define M_F_LENLEN    sizeof(mln_u32_t)

typedef struct mln_fork_s mln_fork_t;

typedef void (*clr_handler)(void *);

typedef int (*fork_iterate_handler)(mln_event_t *, mln_fork_t *, void *);
/*ipc handler*/
typedef void (*ipc_handler)(mln_event_t *, \
                            void *, /*mln_fork_t or mln_tcp_conn_t*/\
                            void *,/*buffer*/\
                            mln_u32_t,/*buffer length*/\
                            void **);/*user data pointer*/
typedef struct {
    ipc_handler              handler;
    void                    *data;
    mln_u32_t                type;
} mln_ipc_handler_t;

enum proc_state_type {
    M_PST_DFL,
    M_PST_SUP /*supervise*/
};

enum proc_exec_type {
    M_PET_DFL,
    M_PET_EXE /*call exec*/
};

struct mln_fork_attr {
    mln_s8ptr_t             *args;
    mln_u32_t                n_args;
    int                      fd;
    pid_t                    pid;
    enum proc_exec_type      etype;
    enum proc_state_type     stype;
};

struct mln_fork_s {
    struct mln_fork_s       *prev;
    struct mln_fork_s       *next;
    mln_s8ptr_t             *args;
    mln_tcp_conn_t           conn;
    pid_t                    pid;
    mln_u32_t                n_args;
    mln_u32_t                state;
    mln_u32_t                msg_len;
    mln_u32_t                msg_type;
    mln_size_t               error_bytes;
    void                    *msg_content;
    enum proc_exec_type      etype;
    enum proc_state_type     stype;
};

extern int mln_fork_prepare(void);
extern int mln_fork_master_ipc_handler_set(mln_u32_t type, ipc_handler handler, void *data) __NONNULL1(2);
extern int mln_fork_worker_ipc_handler_set(mln_u32_t type, ipc_handler handler, void *data) __NONNULL1(2);
extern int
mln_fork_iterate(mln_event_t *ev, fork_iterate_handler handler, void *data) __NONNULL1(1);
extern mln_tcp_conn_t *mln_fork_master_connection_get(void);
extern int mln_fork(void);
/*
 * Only master process can call 'mln_fork_spawn'.
 * argument 'handler()' is used to
 * clear process resources.
 */
extern int
mln_fork_spawn(enum proc_state_type stype, \
               mln_s8ptr_t *args, \
               mln_u32_t n_args, \
               mln_event_t *master_ev);
extern int
mln_fork_restart(mln_event_t *master_ev);
extern void
mln_fork_master_events_set(mln_event_t *ev) __NONNULL1(1);
extern void
mln_fork_worker_events_set(mln_event_t *ev) __NONNULL1(1);
extern void
mln_fork_resource_clear_handler_set(clr_handler handler, void *data);
extern void
mln_ipc_fd_handler_master(mln_event_t *ev, int fd, void *data);
extern void
mln_ipc_fd_handler_worker(mln_event_t *ev, int fd, void *data);
extern void
mln_fork_socketpair_close_handler(mln_event_t *ev, mln_fork_t *f, int fd) __NONNULL2(1,2);
extern int
mln_ipc_master_send_prepare(mln_event_t *ev, \
                            mln_u32_t type, \
                            void *buf, \
                            mln_size_t len, \
                            mln_fork_t *f_child) __NONNULL3(1,3,5);
extern int
mln_ipc_worker_send_prepare(mln_event_t *ev, \
                            mln_u32_t type, \
                            void *msg, \
                            mln_size_t len) __NONNULL2(1,3);

#endif
#endif

