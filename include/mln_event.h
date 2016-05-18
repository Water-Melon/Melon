
/*
 * Copyright (C) Niklaus F.Schen.
 */

#ifndef __MLN_EVENT_H
#define __MLN_EVENT_H

#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include <signal.h>
#include "mln_rbtree.h"
#include "mln_fheap.h"
#if defined(MLN_EPOLL)
#include <sys/epoll.h>
#elif defined(MLN_KQUEUE)
#include<sys/event.h>
#else
#include <sys/select.h>
#endif

/*common*/
#define M_EV_HASH_LEN 64
#define M_EV_EPOLL_SIZE 1024 /*already ignored, see man epoll_create*/
/*for fd*/
#define M_EV_RECV ((mln_u32_t)0x1)
#define M_EV_SEND ((mln_u32_t)0x2)
#define M_EV_ERROR ((mln_u32_t)0x4)
#define M_EV_RSE_MASK ((mln_u32_t)0x7)
#define M_EV_ONESHOT ((mln_u32_t)0x8)
#define M_EV_NONBLOCK ((mln_u32_t)0x10)
#define M_EV_BLOCK ((mln_u32_t)0x20)
#define M_EV_APPEND ((mln_u32_t)0x40)
#define M_EV_CLR ((mln_u32_t)0x80)
#define M_EV_FD_MASK ((mln_u32_t)0xff)
#define M_EV_UNLIMITED -1
#define M_EV_UNMODIFIED -2
/*for signal*/
#define M_EV_SET ((mln_u32_t)0x100)
#define M_EV_UNSET ((mln_u32_t)0x200)
/*for epool, kqueue, select*/
#define M_EV_TIMEOUT_US 10000 /*10ms*/
#define M_EV_TIMEOUT_MS 10
#define M_EV_TIMEOUT_NS 10000000 /*10ms*/

typedef struct mln_event_s      mln_event_t;
typedef struct mln_event_desc_s mln_event_desc_t;

typedef void (*ev_fd_handler)  (mln_event_t *, int, void *);
typedef void (*ev_tm_handler)  (mln_event_t *, void *);
typedef void (*ev_sig_handler) (mln_event_t *, int, void *);
/*
 * return value: 0 - no active, 1 - active
 */
typedef int  (*ev_cust_check) (mln_event_desc_t *, void *);
typedef void (*dispatch_callback) (mln_event_t *, void *);

enum mln_event_type {
    M_EV_FD,
    M_EV_TM,
    M_EV_SIG
};

typedef struct mln_event_fd_s {
    void                    *rcv_data;
    ev_fd_handler            rcv_handler;
    void                    *snd_data;
    ev_fd_handler            snd_handler;
    void                    *err_data;
    ev_fd_handler            err_handler;
    void                    *timeout_data;
    ev_fd_handler            timeout_handler;
    mln_fheap_node_t        *timeout_node;
    mln_uauto_t              end_us;
    int                      fd;
    mln_u32_t                active_flag;
    mln_u32_t                in_process:1;
    mln_u32_t                is_clear:1;
    mln_u32_t                in_active:1;
    mln_u32_t                rd_oneshot:1;
    mln_u32_t                wr_oneshot:1;
    mln_u32_t                err_oneshot:1;
    mln_u32_t                padding:26;
} mln_event_fd_t;

typedef struct mln_event_tm_s {
    void                    *data;
    ev_tm_handler            handler;
    mln_uauto_t              end_tm;/*us*/
} mln_event_tm_t;

typedef struct mln_event_sig_s {
    void                    *data;
    ev_sig_handler           handler;
    int                      signo;
} mln_event_sig_t;

struct mln_event_desc_s {
    enum mln_event_type      type;
    mln_u32_t                flag;
    union {
        mln_event_fd_t       fd;
        mln_event_tm_t       tm;
        mln_event_sig_t      sig;
    } data;
    struct mln_event_desc_s *prev;
    struct mln_event_desc_s *next;
    struct mln_event_desc_s *act_prev;
    struct mln_event_desc_s *act_next;
};

typedef struct {
    mln_event_desc_t        *sig_head;
    mln_event_desc_t        *sig_tail;
    int                      signo;
} mln_event_sig_chain_t;

typedef struct {
    int                      signo;
    mln_u32_t                refer_cnt;
} mln_event_sig_refer_t;

struct mln_event_s {
    struct mln_event_s      *next;
    struct mln_event_s      *prev;
    dispatch_callback        callback;
    void                    *callback_data;
    mln_rbtree_t            *ev_fd_tree;
    mln_event_desc_t        *ev_fd_wait_head;
    mln_event_desc_t        *ev_fd_wait_tail;
    mln_event_desc_t        *ev_fd_active_head;
    mln_event_desc_t        *ev_fd_active_tail;
    mln_fheap_t             *ev_fd_timeout_heap;
    mln_fheap_t             *ev_timer_heap;
    mln_rbtree_t            *ev_signal_tree;
    mln_u32_t                is_break:1;
    mln_u32_t                in_main_thread:31;
    int                      rd_fd;
    int                      wr_fd;
#if defined(MLN_EPOLL)
    int                      epollfd;
#elif defined(MLN_KQUEUE)
    int                      kqfd;
#else
    int                      select_fd;
    fd_set                   rd_set;
    fd_set                   wr_set;
    fd_set                   err_set;
#endif
};

extern mln_event_t *
mln_event_init(mln_u32_t is_main);
extern void
mln_event_destroy(mln_event_t *ev) __NONNULL1(1);
extern void
mln_event_dispatch(mln_event_t *event) __NONNULL1(1);
extern int
mln_event_set_fd(mln_event_t *event, \
                 int fd, \
                 mln_u32_t flag, \
                 int timeout_ms, \
                 void *data, \
                 ev_fd_handler fd_handler) __NONNULL1(1);
extern int
mln_event_set_timer(mln_event_t *event, \
                    mln_u32_t msec, \
                    void *data, \
                    ev_tm_handler tm_handler) __NONNULL1(1);
extern int
mln_event_set_signal(mln_event_t *event, \
                     mln_u32_t flag, \
                     int signo, \
                     void *data, \
                     ev_sig_handler sg_handler) __NONNULL1(1);
extern void
mln_event_set_fd_timeout_handler(mln_event_t *event, \
                                 int fd, \
                                 void *data, \
                                 ev_fd_handler timeout_handler) __NONNULL1(1);
extern void mln_event_set_break(mln_event_t *ev) __NONNULL1(1);
extern void mln_event_set_callback(mln_event_t *ev, \
                                   dispatch_callback dc, \
                                   void *dc_data) __NONNULL1(1);
#endif

