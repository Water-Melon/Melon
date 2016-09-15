
/*
 * Copyright (C) Niklaus F.Schen.
 */
#ifndef __MLN_THREAD_H
#define __MLN_THREAD_H
#include "mln_types.h"
#include "mln_string.h"
#include "mln_connection.h"
#include "mln_event.h"
#include "mln_rbtree.h"

#define THREAD_SOCKFD_LEN 128

typedef int (*tentrance)(int, char **);
typedef struct mln_thread_s mln_thread_t;

enum thread_stype {
    THREAD_RESTART,
    THREAD_DEFAULT
};

struct mln_thread_attr {
    tentrance                  thread_main;
    char                      *alias;
    char                     **argv;
    int                        argc;
    int                        peerfd;
    int                        sockfd;
    enum thread_stype          stype;
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
    tentrance                  thread_main;
    mln_string_t              *alias;
    char                     **argv;
    int                        argc;
    int                        peerfd;
    int                        is_created;
    enum thread_stype          stype;
    pthread_t                  tid;
    mln_tcp_conn_t             conn;
    mln_thread_msgq_t         *local_head;
    mln_thread_msgq_t         *local_tail;
    mln_thread_msgq_t         *dest_head;
    mln_thread_msgq_t         *dest_tail;
    mln_rbtree_node_t         *node;
};

extern void
mln_thread_clearMsg(mln_thread_msg_t *msg);
extern int
mln_load_thread(mln_event_t *ev) __NONNULL1(1);
extern int
mln_thread_create(mln_thread_t *t, mln_event_t *ev) __NONNULL2(1,2);
/*
 * mln_thread_exit() called by child thread only.
 */
extern void mln_thread_exit(int exit_code);
/*
 * mln_thread_kill() called by main thread only.
 */
extern void mln_thread_kill(mln_string_t *alias);
extern void mln_thread_setCleanup(void (*tcleanup)(void *), void *data);
#endif

