
/*
 * Copyright (C) Niklaus F.Schen.
 */

#ifndef __MLN_EVENT_H
#define __MLN_EVENT_H

#if defined(MLN_EPOLL)
#include <sys/epoll.h>
#elif defined(MLN_KQUEUE)
#include <sys/event.h>
#else
#if defined(MSVC)
#include <winsock2.h>
#include <winsock2.h>
#else
#include <sys/select.h>
#endif
#endif
#include <sys/types.h>
#if !defined(MSVC)
#include <unistd.h>
#include <sys/time.h>
#else
#include "mln_utils.h"
#endif
#include <signal.h>
#include "mln_rbtree.h"
#include "mln_fheap.h"

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
/*for epool, kqueue, select*/
#define M_EV_TIMEOUT_US        7000 /*7ms*/
#define M_EV_TIMEOUT_MS        7
#define M_EV_TIMEOUT_NS        7000000 /*7ms*/
#define M_EV_NOLOCK_TIMEOUT_US 3000 /*3ms*/
#define M_EV_NOLOCK_TIMEOUT_MS 3
#define M_EV_NOLOCK_TIMEOUT_NS 3000000/*3ms*/

typedef struct mln_event_s      mln_event_t;
typedef struct mln_event_desc_s mln_event_desc_t;
typedef mln_fheap_node_t        mln_event_timer_t;

typedef void (*ev_fd_handler)  (mln_event_t *, int, void *);
typedef void (*ev_tm_handler)  (mln_event_t *, void *);
/*
 * return value: 0 - no active, 1 - active
 */
typedef void (*dispatch_callback) (mln_event_t *, void *);

enum mln_event_type {
    M_EV_FD,
    M_EV_TM,
};

typedef struct mln_event_fd_s {
    int                      fd;
    mln_u32_t                active_flag;
    mln_u32_t                in_process:1;
    mln_u32_t                is_clear:1;
    mln_u32_t                in_active:1;
    mln_u32_t                rd_oneshot:1;
    mln_u32_t                wr_oneshot:1;
    mln_u32_t                err_oneshot:1;
    mln_u32_t                padding:26;
    void                    *rcv_data;
    ev_fd_handler            rcv_handler;
    void                    *snd_data;
    ev_fd_handler            snd_handler;
    void                    *err_data;
    ev_fd_handler            err_handler;
    void                    *timeout_data;
    ev_fd_handler            timeout_handler;
    mln_fheap_node_t        *timeout_node;
    mln_u64_t                end_us;
} mln_event_fd_t;

typedef struct mln_event_tm_s {
    void                    *data;
    ev_tm_handler            handler;
    mln_uauto_t              end_tm;/*us*/
} mln_event_tm_t;

struct mln_event_desc_s {
    struct mln_event_desc_s *prev;
    struct mln_event_desc_s *next;
    struct mln_event_desc_s *act_prev;
    struct mln_event_desc_s *act_next;
    enum mln_event_type      type;
    mln_u32_t                flag;
    union {
        mln_event_tm_t       tm;
        mln_event_fd_t       fd;
    } data;
};

struct mln_event_s {
#if !defined(MSVC)
    pthread_mutex_t          fd_lock;
    pthread_mutex_t          timer_lock;
    pthread_mutex_t          cb_lock;
#endif
    dispatch_callback        callback;
    void                    *callback_data;
    mln_u32_t                is_break:1;
    mln_u32_t                padding:31;
#if defined(MLN_EPOLL)
    int                      epollfd;
    int                      unusedfd;
#elif defined(MLN_KQUEUE)
    int                      kqfd;
    int                      unusedfd;
#else
    int                      select_fd;
    fd_set                   rd_set;
    fd_set                   wr_set;
    fd_set                   err_set;
#endif

    mln_rbtree_t            *ev_fd_tree;
    mln_event_desc_t        *ev_fd_wait_head;
    mln_event_desc_t        *ev_fd_wait_tail;
    mln_event_desc_t        *ev_fd_active_head;
    mln_event_desc_t        *ev_fd_active_tail;
    mln_fheap_t             *ev_fd_timeout_heap;
    mln_fheap_t             *ev_timer_heap;
};

#define mln_event_break_set(ev) ((ev)->is_break = 1);
#define mln_event_break_reset(ev) ((ev)->is_break = 0);
#define mln_event_signal_set signal
extern mln_event_t *mln_event_new(void);
extern void mln_event_free(mln_event_t *ev);
extern void mln_event_dispatch(mln_event_t *event) __NONNULL1(1);
/*
 * Note: There is a event named A triggered in thread t1,
 * and another event B triggered in thread t2. If we want to free
 * the data 'da' which is combinded with event A in event B handler,
 * we have to resolve the confliction of 'da' in event handler.
 */
extern int
mln_event_fd_set(mln_event_t *event, \
                 int fd, \
                 mln_u32_t flag, \
                 int timeout_ms, \
                 void *data, \
                 ev_fd_handler fd_handler) __NONNULL1(1);
extern mln_event_timer_t *
mln_event_timer_set(mln_event_t *event, \
                    mln_u32_t msec, \
                    void *data, \
                    ev_tm_handler tm_handler) __NONNULL1(1);
extern void mln_event_timer_cancel(mln_event_t *event, mln_event_timer_t *timer) __NONNULL1(1);
extern void
mln_event_fd_timeout_handler_set(mln_event_t *event, \
                                 int fd, \
                                 void *data, \
                                 ev_fd_handler timeout_handler) __NONNULL1(1);
extern void mln_event_callback_set(mln_event_t *ev, \
                                   dispatch_callback dc, \
                                   void *dc_data) __NONNULL1(1);
#endif

