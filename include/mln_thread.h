
/*
 * Copyright (C) Niklaus F.Schen.
 */
#ifndef __MLN_THREAD_H
#define __MLN_THREAD_H

#if !defined(MSVC)

#include "mln_connection.h"
#include "mln_types.h"
#include "mln_string.h"
#include "mln_event.h"
#include "mln_rbtree.h"

#define THREAD_SOCKFD_LEN 128

typedef int (*mln_thread_entrance_t)(int, char **);

typedef struct mln_thread_s mln_thread_t;

typedef struct {
    char                      *alias;
    mln_thread_entrance_t      thread_main;
} mln_thread_module_t;

typedef enum {
    THREAD_RESTART,
    THREAD_DEFAULT
} mln_thread_stype_t;

struct mln_thread_attr {
    mln_event_t               *ev;
    mln_thread_entrance_t      thread_main;
    char                      *alias;
    char                     **argv;
    int                        argc;
    int                        peerfd;
    int                        sockfd;
    mln_thread_stype_t         stype;
};

typedef struct {
    mln_string_t              *dest;
    mln_string_t              *src;
    double                     f;
    void                      *pfunc;
    void                      *pdata;
    mln_sauto_t                sauto;
    mln_uauto_t                uauto;
    mln_s8_t                   c;
    mln_s8_t                   padding[7];
    enum {
        ITC_REQUEST,
        ITC_RESPONSE
    }                          type;
    int                        need_clear;
} mln_thread_msg_t;

typedef struct mln_thread_msgq_s {
    mln_thread_t              *sender;
    struct mln_thread_msgq_s  *dest_prev;
    struct mln_thread_msgq_s  *dest_next;
    struct mln_thread_msgq_s  *local_prev;
    struct mln_thread_msgq_s  *local_next;
    mln_thread_msg_t           msg;
} mln_thread_msgq_t;

struct mln_thread_s {
    mln_event_t               *ev;
    mln_thread_entrance_t      thread_main;
    mln_string_t              *alias;
    char                     **argv;
    int                        argc;
    int                        peerfd;
    int                        is_created;
    mln_thread_stype_t         stype;
    pthread_t                  tid;
    mln_tcp_conn_t             conn;
    mln_thread_msgq_t         *local_head;
    mln_thread_msgq_t         *local_tail;
    mln_thread_msgq_t         *dest_head;
    mln_thread_msgq_t         *dest_tail;
    mln_rbtree_node_t         *node;
};

extern void mln_thread_clear_msg(mln_thread_msg_t *msg);
extern int mln_load_thread(mln_event_t *ev) __NONNULL1(1);
extern int mln_thread_create(mln_event_t *ev, \
                             char *alias, \
                             mln_thread_stype_t stype, \
                             mln_thread_entrance_t entrance, \
                             int argc, \
                             char *argv[]) __NONNULL4(1,2,4,6);
/*
 * mln_thread_exit() called by child thread only.
 */
extern void mln_thread_exit(int exit_code);
/*
 * mln_thread_kill() called by main thread only.
 */
extern void mln_thread_kill(mln_string_t *alias);
extern void mln_thread_cleanup_set(void (*tcleanup)(void *), void *data);
extern void mln_thread_module_set(mln_thread_module_t *modules, mln_size_t num);
#endif

#endif

