
/*
 * Copyright (C) Niklaus F.Schen.
 */
#ifndef __MLN_IOTHREAD_H
#define __MLN_IOTHREAD_H

#if !defined(MSVC)
#include "mln_types.h"
#include <pthread.h>

typedef struct mln_iothread_msg_s mln_iothread_msg_t;
typedef struct mln_iothread_s     mln_iothread_t;

typedef enum {
    io_thread,
    user_thread
} mln_iothread_ep_type_t;

typedef void *(*mln_iothread_entry_t)(void *);
typedef void (*mln_iothread_msg_process_t)(mln_iothread_t *, mln_iothread_ep_type_t, mln_iothread_msg_t *);

struct mln_iothread_msg_s {
    struct mln_iothread_msg_s  *prev;
    struct mln_iothread_msg_s  *next;
    mln_u32_t                   feedback:1;
    mln_u32_t                   hold:1;
    mln_u32_t                   padding:30;
    mln_u32_t                   type;
    void                       *data;
    pthread_mutex_t             mutex;
};

struct mln_iothread_s {
    pthread_mutex_t             io_lock;
    pthread_mutex_t             user_lock;
    int                         io_fd;
    int                         user_fd;
    mln_iothread_msg_process_t  handler;
    mln_iothread_msg_t         *io_head;
    mln_iothread_msg_t         *io_tail;
    mln_iothread_entry_t        entry;
    void                       *args;
    pthread_t                  *tids;
    mln_u32_t                   nthread;
    mln_iothread_msg_t         *user_head;
    mln_iothread_msg_t         *user_tail;
};

#define mln_iothread_sockfd_get(p,t)   ((t) == io_thread? (p)->io_fd: (p)->user_fd)
#define mln_iothread_msg_hold(m)       ((m)->hold = 1)
#define mln_iothread_msg_release(m)    pthread_mutex_unlock(&(m)->mutex)
#define mln_iothread_msg_type(m)       ((m)->type)
#define mln_iothread_msg_data(m)       ((m)->data)

extern int mln_iothread_init(mln_iothread_t *t, mln_u32_t nthread, mln_iothread_entry_t entry, void *args, mln_iothread_msg_process_t handler);
extern void mln_iothread_destroy(mln_iothread_t *t);
extern int mln_iothread_send(mln_iothread_t *t, mln_u32_t type, void *data, mln_iothread_ep_type_t to, mln_u32_t feedback);
extern int mln_iothread_recv(mln_iothread_t *t, mln_iothread_ep_type_t from);

#endif

#endif
