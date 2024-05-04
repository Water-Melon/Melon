
/*
 * Copyright (C) Niklaus F.Schen.
 */

#include "mln_event.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include "mln_utils.h"
#include "mln_func.h"
#if !defined(MSVC)
#include <sys/socket.h>
#endif

/*declarations*/
MLN_CHAIN_FUNC_DECLARE(static inline, \
                       ev_fd_wait, \
                       mln_event_desc_t, );
MLN_CHAIN_FUNC_DECLARE(static inline, \
                       ev_fd_active, \
                       mln_event_desc_t, );
static inline void
mln_event_desc_free(void *data);
static int
mln_event_rbtree_fd_cmp(const void *k1, const void *k2) __NONNULL2(1,2);
static inline int
mln_event_fd_timeout_cmp(const void *k1, const void *k2);
static inline void
mln_event_fd_timeout_copy(void *k1, void *k2);
static inline int
mln_event_fheap_timer_cmp(const void *k1, const void *k2) __NONNULL2(1,2);
static inline void
mln_event_fheap_timer_copy(void *k1, void *k2) __NONNULL2(1,2);
static inline void
mln_event_fd_nonblock_set(int fd);
static inline void
mln_event_fd_block_set(int fd);
static inline void
mln_event_fd_clr_set(mln_event_t *event, int fd) __NONNULL1(1);
static inline void
mln_event_active_fd_process(mln_event_t *event) __NONNULL1(1);
static inline void mln_event_fd_timeout_process(mln_event_t *event);
static inline void mln_event_timer_process(mln_event_t *event);
static inline int
mln_event_fd_normal_set(mln_event_t *event, \
                        mln_event_desc_t *ed, \
                        int fd, \
                        mln_u32_t flag, \
                        int timeout_ms, \
                        void *data, \
                        ev_fd_handler fd_handler, \
                        int other_mark);
static inline int
mln_event_fd_append_set(mln_event_t *event, \
                        mln_event_desc_t *ed, \
                        int fd, \
                        mln_u32_t flag, \
                        int timeout_ms, \
                        void *data, \
                        ev_fd_handler fd_handler, \
                        int other_mark);
static int
mln_event_fd_timeout_set(mln_event_t *ev, mln_event_desc_t *ed, int timeout_ms);

/*varliables*/
mln_event_desc_t fheap_min = {
    NULL, NULL, NULL, NULL,
    M_EV_TM, 0,
#if defined(MSVC)
    {0},
#else
    {(mln_event_tm_t){NULL, NULL, 0}},
#endif
};

mln_event_t *mln_event_new(void)
{
    int rc;
#if defined(MSVC)
    struct mln_rbtree_attr rbattr;
#endif
    mln_event_t *ev;
    ev = (mln_event_t *)malloc(sizeof(mln_event_t));
    if (ev == NULL) {
        return NULL;
    }
    ev->callback = NULL;
    ev->callback_data = NULL;
#if defined(MSVC)
    rbattr.pool = NULL;
    rbattr.pool_alloc = NULL;
    rbattr.pool_free = NULL;
    rbattr.cmp = mln_event_rbtree_fd_cmp;
    rbattr.data_free = NULL;
    ev->ev_fd_tree = mln_rbtree_new(&rbattr);
#else
    ev->ev_fd_tree = mln_rbtree_new(NULL);
#endif
    if (ev->ev_fd_tree == NULL) {
        goto err1;
    }
    ev->ev_fd_wait_head = NULL;
    ev->ev_fd_wait_tail = NULL;
    ev->ev_fd_active_head = NULL;
    ev->ev_fd_active_tail = NULL;

    ev->ev_fd_timeout_heap = mln_fheap_new(&fheap_min, NULL);
    if (ev->ev_fd_timeout_heap == NULL) {
        goto err2;
    }
    /*timer heap*/
    ev->ev_timer_heap = mln_fheap_new_fast(&fheap_min, \
                                           mln_event_fheap_timer_cmp, \
                                           mln_event_fheap_timer_copy, \
                                           mln_event_desc_free);
    if (ev->ev_timer_heap == NULL) {
        goto err3;
    }
    ev->is_break = 0;
#if defined(MLN_EPOLL)
    ev->epollfd = epoll_create(M_EV_EPOLL_SIZE);
    if (ev->epollfd < 0) {
        goto err4;
    }
    ev->unusedfd = epoll_create(M_EV_EPOLL_SIZE);
    if (ev->unusedfd < 0) {
        close(ev->epollfd);
        goto err4;
    }
#elif defined(MLN_KQUEUE)
    ev->kqfd = kqueue();
    if (ev->kqfd < 0) {
        goto err4;
    }
    ev->unusedfd = kqueue();
    if (ev->unusedfd < 0) {
        close(ev->kqfd);
        goto err4;
    }
#else
    ev->select_fd = 3;
    FD_ZERO(&(ev->rd_set));
    FD_ZERO(&(ev->wr_set));
    FD_ZERO(&(ev->err_set));
#endif

#if !defined(MSVC)
    rc = pthread_mutex_init(&ev->fd_lock, NULL);
    if (pthread_mutex_init(&ev->timer_lock, NULL) != 0)
        rc = -1;
    if (pthread_mutex_init(&ev->cb_lock, NULL) != 0)
        rc = -1;
#else
    rc = 0;
#endif
    if (rc) {
#if !defined(MSVC)
        pthread_mutex_destroy(&ev->fd_lock);
        pthread_mutex_destroy(&ev->timer_lock);
        pthread_mutex_destroy(&ev->cb_lock);
#endif
#if defined(MLN_EPOLL)
        close(ev->epollfd);
#elif defined(MLN_KQUEUE)
        close(ev->kqfd);
#endif
        goto err4;
    }

    return ev;

err4:
    mln_fheap_inline_free(ev->ev_timer_heap, mln_event_fheap_timer_cmp, mln_event_desc_free);
err3:
    mln_fheap_inline_free(ev->ev_fd_timeout_heap, mln_event_fd_timeout_cmp, NULL);
err2:
    mln_rbtree_free(ev->ev_fd_tree);
err1:
    free(ev);
    return NULL;
}

void mln_event_free(mln_event_t *ev)
{
    if (ev == NULL) return;
    mln_event_desc_t *ed;
    mln_fheap_inline_free(ev->ev_fd_timeout_heap, mln_event_fd_timeout_cmp, NULL);
    mln_rbtree_free(ev->ev_fd_tree);
    while ((ed = ev->ev_fd_wait_head) != NULL) {
        ev_fd_wait_chain_del(&(ev->ev_fd_wait_head), \
                             &(ev->ev_fd_wait_tail), \
                             ed);
        mln_event_desc_free(ed);
    }
    mln_fheap_inline_free(ev->ev_timer_heap, mln_event_fheap_timer_cmp, mln_event_desc_free);
#if defined(MLN_EPOLL)
    close(ev->epollfd);
    close(ev->unusedfd);
#elif defined(MLN_KQUEUE)
    close(ev->kqfd);
    close(ev->unusedfd);
#else
    /*select do nothing.*/
#endif
#if !defined(MSVC)
    pthread_mutex_destroy(&ev->fd_lock);
    pthread_mutex_destroy(&ev->timer_lock);
    pthread_mutex_destroy(&ev->cb_lock);
#endif
    free(ev);
}

MLN_FUNC_VOID(static inline, void, mln_event_desc_free, (void *data), (data), {
    if (data == NULL) return;
    free(data);
})

/*
 * ev_timer
 */
mln_event_timer_t *mln_event_timer_set(mln_event_t *event, mln_u32_t msec, void *data, ev_tm_handler tm_handler)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    mln_uauto_t end = tv.tv_sec*1000000 + tv.tv_usec + msec*1000;
    mln_event_desc_t *ed;
    ed = (mln_event_desc_t *)malloc(sizeof(mln_event_desc_t));
    if (ed == NULL) {
        return NULL;
    }
    ed->type = M_EV_TM;
    ed->flag = 0;
    ed->data.tm.data = data;
    ed->data.tm.handler = tm_handler;
    ed->data.tm.end_tm = end;
    ed->prev = NULL;
    ed->next = NULL;
    ed->act_prev = NULL;
    ed->act_next = NULL;
    mln_fheap_node_t *fn = mln_fheap_node_new(event->ev_timer_heap, ed);
    if (fn == NULL) {
        free(ed);
        return NULL;
    }
#if !defined(MSVC)
    pthread_mutex_lock(&event->timer_lock);
#endif
    mln_fheap_inline_insert(event->ev_timer_heap, fn, mln_event_fheap_timer_cmp);
#if !defined(MSVC)
    pthread_mutex_unlock(&event->timer_lock);
#endif
    return fn;
}

void mln_event_timer_cancel(mln_event_t *event, mln_event_timer_t *timer)
{
#if !defined(MSVC)
    pthread_mutex_lock(&event->timer_lock);
#endif
    mln_fheap_inline_delete(event->ev_timer_heap, timer, mln_event_fheap_timer_copy, mln_event_fheap_timer_cmp);
    mln_fheap_inline_node_free(event->ev_timer_heap, timer, mln_event_desc_free);
#if !defined(MSVC)
    pthread_mutex_unlock(&event->timer_lock);
#endif
}

static inline void mln_event_timer_process(mln_event_t *event)
{
    mln_uauto_t now;
    struct timeval tv;
    gettimeofday(&tv, NULL);
    now = tv.tv_sec * 1000000 + tv.tv_usec;
    mln_event_desc_t *ed;
    mln_fheap_node_t *fn;

lp:
#if !defined(MSVC)
    if (pthread_mutex_trylock(&event->timer_lock))
        return;
#endif

    fn = mln_fheap_minimum(event->ev_timer_heap);
    if (fn == NULL) {
#if !defined(MSVC)
        pthread_mutex_unlock(&event->timer_lock);
#endif
        return;
    }

    ed = (mln_event_desc_t *)mln_fheap_node_key(fn);
    if (ed->data.tm.end_tm > now) {
#if !defined(MSVC)
        pthread_mutex_unlock(&event->timer_lock);
#endif
        return;
    }

#if defined(MSVC)
    fn = mln_fheap_extract_min(event->ev_timer_heap);
#else
    fn = mln_fheap_inline_extract_min(event->ev_timer_heap, mln_event_fheap_timer_cmp);
#endif

#if !defined(MSVC)
    pthread_mutex_unlock(&event->timer_lock);
#endif

    if (ed->data.tm.handler != NULL)
        ed->data.tm.handler(event, ed->data.tm.data);

    mln_fheap_inline_node_free(event->ev_timer_heap, fn, mln_event_desc_free);

    if (!event->is_break)
        goto lp;
}

/*
 * ev_fd
 */
void mln_event_fd_timeout_handler_set(mln_event_t *event, int fd, void *data, ev_fd_handler timeout_handler)
{
#if !defined(MSVC)
    pthread_mutex_lock(&event->fd_lock);
#endif
    mln_event_desc_t tmp;
    memset(&tmp, 0, sizeof(tmp));
    tmp.type = M_EV_FD;
    tmp.data.fd.fd = fd;
#if defined(MSVC)
    mln_rbtree_node_t *rn = mln_rbtree_search(event->ev_fd_tree, &tmp);
#else
    mln_rbtree_node_t *rn = mln_rbtree_inline_search(event->ev_fd_tree, &tmp, mln_event_rbtree_fd_cmp);
#endif
    ASSERT(!mln_rbtree_null(rn, event->ev_fd_tree));
    mln_event_desc_t *ed = (mln_event_desc_t *)mln_rbtree_node_data_get(rn);
    ed->data.fd.timeout_data = data;
    ed->data.fd.timeout_handler = timeout_handler;
#if !defined(MSVC)
    pthread_mutex_unlock(&event->fd_lock);
#endif
}

int mln_event_fd_set(mln_event_t *event, int fd, mln_u32_t flag, int timeout_ms, void *data, ev_fd_handler fd_handler)
{
    ASSERT(fd >= 0 && !(flag & ~M_EV_FD_MASK) && flag <= M_EV_CLR && !((flag & M_EV_NONBLOCK) && (flag & M_EV_BLOCK)));

#if !defined(MSVC)
    pthread_mutex_lock(&event->fd_lock);
#endif
    if (flag == M_EV_CLR) {
        mln_event_fd_clr_set(event, fd);
#if !defined(MSVC)
        pthread_mutex_unlock(&event->fd_lock);
#endif
        return 0;
    }
    mln_event_desc_t tmp;
    memset(&tmp, 0, sizeof(tmp));
    tmp.type = M_EV_FD;
    tmp.data.fd.fd = fd;
    mln_rbtree_node_t *rn;
#if defined(MSVC)
    rn = mln_rbtree_search(event->ev_fd_tree, &tmp);
#else
    rn = mln_rbtree_inline_search(event->ev_fd_tree, &tmp, mln_event_rbtree_fd_cmp);
#endif
    if (!mln_rbtree_null(rn, event->ev_fd_tree)) {
        if (flag & M_EV_APPEND) {
            if (flag & M_EV_NONBLOCK) mln_event_fd_nonblock_set(fd);
            if (flag & M_EV_BLOCK) mln_event_fd_block_set(fd);

            ASSERT(!(((mln_event_desc_t *)mln_rbtree_node_data_get(rn))->data.fd.is_clear));

            if (mln_event_fd_append_set(event, \
                                        (mln_event_desc_t *)mln_rbtree_node_data_get(rn), \
                                        fd, \
                                        flag, \
                                        timeout_ms, \
                                        data, \
                                        fd_handler, \
                                        1) < 0)
            {
#if !defined(MSVC)
                pthread_mutex_unlock(&event->fd_lock);
#endif
                return -1;
            }
        } else {
            if (flag & M_EV_NONBLOCK) {
                mln_event_fd_nonblock_set(fd);
            } else {
                mln_event_fd_block_set(fd);
            }
            if (mln_event_fd_normal_set(event, \
                                        (mln_event_desc_t *)(rn->data), \
                                        fd, \
                                        flag, \
                                        timeout_ms, \
                                        data, \
                                        fd_handler, \
                                        ((mln_event_desc_t *)(rn->data))->data.fd.is_clear?0:1) < 0)
            {
#if !defined(MSVC)
                pthread_mutex_unlock(&event->fd_lock);
#endif
                return -1;
            }
        }
#if !defined(MSVC)
        pthread_mutex_unlock(&event->fd_lock);
#endif
        return 0;
    }
    if (flag & M_EV_NONBLOCK) {
        mln_event_fd_nonblock_set(fd);
    } else {
        mln_event_fd_block_set(fd);
    }
    if (mln_event_fd_normal_set(event, NULL, fd, flag, timeout_ms, data, fd_handler, 0) < 0) {
#if !defined(MSVC)
        pthread_mutex_unlock(&event->fd_lock);
#endif
        return -1;
    }
#if !defined(MSVC)
    pthread_mutex_unlock(&event->fd_lock);
#endif
    return 0;
}

MLN_FUNC(static inline, int, mln_event_fd_normal_set, \
         (mln_event_t *event, mln_event_desc_t *ed, int fd, mln_u32_t flag, \
          int timeout_ms, void *data, ev_fd_handler fd_handler, int other_mark), \
         (event, ed, fd, flag, timeout_ms, data, fd_handler, other_mark), \
{
    if (ed == NULL) {
        ed = (mln_event_desc_t *)malloc(sizeof(mln_event_desc_t));
        if (ed == NULL) {
            return -1;
        }
        ed->type = M_EV_FD;
        ed->flag = 0;
        memset(&(ed->data.fd), 0, sizeof(mln_event_fd_t));
        ed->data.fd.fd = fd;
        ed->next = NULL;
        ed->prev = NULL;
        ed->act_next = NULL;
        ed->act_prev = NULL;
        mln_rbtree_node_t *rn;
        rn = mln_rbtree_node_new(event->ev_fd_tree, ed);
        if (rn == NULL) {
            free(ed);
            return -1;
        }
        mln_rbtree_inline_insert(event->ev_fd_tree, rn, mln_event_rbtree_fd_cmp);
        ev_fd_wait_chain_add(&(event->ev_fd_wait_head), \
                             &(event->ev_fd_wait_tail), \
                             ed);
        if (mln_event_fd_timeout_set(event, ed, timeout_ms) < 0) {
            free(ed);
            return -1;
        }
    } else {
        if (ed->data.fd.is_clear) {
            mln_u32_t in_process = ed->data.fd.in_process;
            memset(&(ed->data.fd), 0, sizeof(mln_event_fd_t));
            ed->data.fd.in_process = in_process;
            ed->data.fd.fd = fd;
            ed->flag = 0;
        } else {
            ed->flag = 0;
            ed->data.fd.rd_oneshot = 0;
            ed->data.fd.wr_oneshot = 0;
            ed->data.fd.err_oneshot = 0;
            ed->data.fd.fd = fd;
        }
        if (mln_event_fd_timeout_set(event, ed, timeout_ms) < 0) {
            return -1;
        }
    }

    return mln_event_fd_append_set(event, \
                                   ed, \
                                   fd, \
                                   flag, \
                                   M_EV_UNMODIFIED, \
                                   data, \
                                   fd_handler, \
                                   other_mark);
})

static inline int mln_event_fd_append_set(mln_event_t *event, \
                                          mln_event_desc_t *ed, \
                                          int fd, mln_u32_t flag, \
                                          int timeout_ms, \
                                          void *data, \
                                          ev_fd_handler fd_handler, \
                                          int other_mark)
{
    if (mln_event_fd_timeout_set(event, ed, timeout_ms) < 0)
        return -1;
#if defined(MLN_EPOLL)
#define CASE_MACRO(flg); \
    if (other_mark) {\
        if (oneshot) {\
            ev.events = (flg)|EPOLLONESHOT;\
            ev.data.ptr = ed;\
            epoll_ctl(event->epollfd, EPOLL_CTL_MOD, fd, &ev);\
        } else {\
            ev.events = (flg);\
            ev.data.ptr = ed;\
            epoll_ctl(event->epollfd, EPOLL_CTL_MOD, fd, &ev);\
        }\
    } else {\
        if (oneshot) {\
            ev.events = (flg)|EPOLLONESHOT;\
            ev.data.ptr = ed;\
            epoll_ctl(event->epollfd, EPOLL_CTL_ADD, fd, &ev);\
        } else {\
            ev.events = (flg);\
            ev.data.ptr = ed;\
            epoll_ctl(event->epollfd, EPOLL_CTL_ADD, fd, &ev);\
        }\
    }

    int oneshot = (flag & M_EV_ONESHOT)? 1: 0;
    int mask = 0;
    if (ed->flag & M_EV_RECV) mask |= 0x1;
    if (ed->flag & M_EV_SEND) mask |= 0x2;
    if (ed->flag & M_EV_ERROR) mask |= 0x4;
    if (flag & M_EV_RECV) {
        ed->flag |= M_EV_RECV;
        ed->data.fd.rcv_data = data;
        ed->data.fd.rcv_handler = fd_handler;
        if (oneshot) ed->data.fd.rd_oneshot = 1;
        mask |= 0x1;
    }
    if (flag & M_EV_SEND) {
        ed->flag |= M_EV_SEND;
        ed->data.fd.snd_data = data;
        ed->data.fd.snd_handler = fd_handler;
        if (oneshot) ed->data.fd.wr_oneshot = 1;
        mask |= 0x2;
    }
    if (flag & M_EV_ERROR) {
        ed->flag |= M_EV_ERROR;
        ed->data.fd.err_data = data;
        ed->data.fd.err_handler = fd_handler;
        if (oneshot) ed->data.fd.err_oneshot = 1;
        mask |= 0x4;
    }
    struct epoll_event ev;
    memset(&ev, 0, sizeof(ev));
    switch (mask) {
        case 1:
            CASE_MACRO(EPOLLIN);
            break;
        case 2:
            CASE_MACRO(EPOLLOUT);
            break;
        case 3:
            CASE_MACRO(EPOLLIN|EPOLLOUT);
            break;
        case 4:
            CASE_MACRO(EPOLLERR);
            break;
        case 5:
            CASE_MACRO(EPOLLIN|EPOLLERR);
            break;
        case 6:
            CASE_MACRO(EPOLLOUT|EPOLLERR);
            break;
        case 7:
            CASE_MACRO(EPOLLIN|EPOLLOUT|EPOLLERR);
            break;
        default: return 0;
    }
#elif defined(MLN_KQUEUE)
    struct kevent ev;
    int oneshot = (flag & M_EV_ONESHOT)? 1: 0;
    if (!other_mark) {
        EV_SET(&ev, fd, EVFILT_READ, EV_ADD|EV_ERROR|EV_DISABLE, 0, 0, ed);
        if (kevent(event->kqfd, &ev, 1, NULL, 0, NULL) < 0) {
            ASSERT(0);
        }
        EV_SET(&ev, fd, EVFILT_WRITE, EV_ADD|EV_ERROR|EV_DISABLE, 0, 0, ed);
        if (kevent(event->kqfd, &ev, 1, NULL, 0, NULL) < 0) {
            ASSERT(0);
        }
    }
    if (flag & M_EV_RECV) {
        ed->flag |= M_EV_RECV;
        if (oneshot) ed->data.fd.rd_oneshot = 1;
        EV_SET(&ev, fd, EVFILT_READ, EV_ENABLE, 0, 0, ed);
        if (kevent(event->kqfd, &ev, 1, NULL, 0, NULL) < 0) {
            ASSERT(0);
        }
        ed->data.fd.rcv_data = data;
        ed->data.fd.rcv_handler = fd_handler;
    }
    if (flag & M_EV_SEND) {
        ed->flag |= M_EV_SEND;
        if (oneshot) ed->data.fd.wr_oneshot = 1;
        EV_SET(&ev, fd, EVFILT_WRITE, EV_ENABLE, 0, 0, ed);
        if (kevent(event->kqfd, &ev, 1, NULL, 0, NULL) < 0) {
            ASSERT(0);
        }
        ed->data.fd.snd_data = data;
        ed->data.fd.snd_handler = fd_handler;
    }
    if (flag & M_EV_ERROR) {
        ed->flag |= M_EV_ERROR;
        if (oneshot) ed->data.fd.err_oneshot = 1;
        ed->data.fd.err_data = data;
        ed->data.fd.err_handler = fd_handler;
    }
#else
    /*other_mark useless*/
    if (flag & M_EV_RECV) {
        FD_SET(fd, &(event->rd_set));
        ed->flag |= M_EV_RECV;
        if (flag & M_EV_ONESHOT) {
            ed->data.fd.rd_oneshot = 1;
        }
        ed->data.fd.rcv_data = data;
        ed->data.fd.rcv_handler = fd_handler;
    }
    if (flag & M_EV_SEND) {
        FD_SET(fd, &(event->wr_set));
        ed->flag |= M_EV_SEND;
        if (flag & M_EV_ONESHOT) {
            ed->data.fd.wr_oneshot = 1;
        }
        ed->data.fd.snd_data = data;
        ed->data.fd.snd_handler = fd_handler;
    }
    if (flag & M_EV_ERROR) {
        FD_SET(fd, &(event->err_set));
        ed->flag |= M_EV_ERROR;
        if (flag & M_EV_ONESHOT) {
            ed->data.fd.err_oneshot = 1;
        }
        ed->data.fd.err_data = data;
        ed->data.fd.err_handler = fd_handler;
    }
#endif
    return 0;
}

MLN_FUNC(static, int, mln_event_fd_timeout_set, \
         (mln_event_t *ev, mln_event_desc_t *ed, int timeout_ms), \
         (ev, ed, timeout_ms), \
{
    if (timeout_ms == M_EV_UNMODIFIED) return 0;
    mln_event_fd_t *ef = &(ed->data.fd);
    if (timeout_ms == M_EV_UNLIMITED) {
        if (ef->timeout_node != NULL) {
            mln_fheap_inline_delete(ev->ev_fd_timeout_heap, ef->timeout_node, mln_event_fd_timeout_copy, mln_event_fd_timeout_cmp);
            mln_fheap_inline_node_free(ev->ev_fd_timeout_heap, ef->timeout_node, NULL);
            ef->timeout_node = NULL;
            ef->end_us = 0;
        }
        return 0;
    }
    mln_fheap_node_t *fn;
    struct timeval tv;
    memset(&tv, 0, sizeof(tv));
    gettimeofday(&tv, NULL);
    if (ef->timeout_node == NULL) {
        ef->end_us = tv.tv_sec*1000000+tv.tv_usec+timeout_ms*1000;
        fn = mln_fheap_node_new(ev->ev_fd_timeout_heap, ed);
        if (fn == NULL) {
            return -1;
        }
        ef->timeout_node = fn;
        mln_fheap_inline_insert(ev->ev_fd_timeout_heap, fn, mln_event_fd_timeout_cmp);
    } else {
        fn = ef->timeout_node;
        mln_fheap_inline_delete(ev->ev_fd_timeout_heap, fn, mln_event_fd_timeout_copy, mln_event_fd_timeout_cmp);
        ef->end_us = tv.tv_sec*1000000+tv.tv_usec+timeout_ms*1000;
        mln_fheap_inline_insert(ev->ev_fd_timeout_heap, fn, mln_event_fd_timeout_cmp);
    }
    return 0;
})

static inline void mln_event_fd_clr_set(mln_event_t *event, int fd)
{
    mln_event_desc_t tmp, *ed;
    memset(&tmp, 0, sizeof(tmp));
    tmp.type = M_EV_FD;
    tmp.data.fd.fd = fd;
    mln_rbtree_node_t *rn;
#if defined(MSVC)
    rn = mln_rbtree_search(event->ev_fd_tree, &tmp);
#else
    rn = mln_rbtree_inline_search(event->ev_fd_tree, &tmp, mln_event_rbtree_fd_cmp);
#endif
    if (mln_rbtree_null(rn, event->ev_fd_tree)) {
        return;
    }
    ed = (mln_event_desc_t *)mln_rbtree_node_data_get(rn);
    if (ed->data.fd.timeout_node != NULL) {
        mln_fheap_inline_delete(event->ev_fd_timeout_heap, ed->data.fd.timeout_node, mln_event_fd_timeout_copy, mln_event_fd_timeout_cmp);
        mln_fheap_inline_node_free(event->ev_fd_timeout_heap, ed->data.fd.timeout_node, NULL);
        ed->data.fd.timeout_node = NULL;
        ed->data.fd.end_us = 0;
    }
#if defined(MLN_EPOLL)
    struct epoll_event ev;
    memset(&ev, 0, sizeof(ev));
    ev.data.ptr = ed;
    epoll_ctl(event->epollfd, EPOLL_CTL_DEL, fd, &ev);
#elif defined(MLN_KQUEUE)
    struct kevent ev;
    EV_SET(&ev, fd, EVFILT_READ, EV_DELETE, 0, 0, ed);
    kevent(event->kqfd, &ev, 1, NULL, 0, NULL);
    EV_SET(&ev, fd, EVFILT_WRITE, EV_DELETE, 0, 0, ed);
    kevent(event->kqfd, &ev, 1, NULL, 0, NULL);
#else
    if (ed->flag & M_EV_RECV)
        FD_CLR(fd, &(event->rd_set));
    if (ed->flag & M_EV_SEND)
        FD_CLR(fd, &(event->wr_set));
    if (ed->flag & M_EV_ERROR)
        FD_CLR(fd, &(event->err_set));
#endif
    if (ed->data.fd.in_process) {
        ed->data.fd.is_clear = 1;
        return;
    }
    mln_rbtree_delete(event->ev_fd_tree, rn);
    mln_rbtree_node_free(event->ev_fd_tree, rn);
    if (ed->data.fd.in_active) {
        ev_fd_active_chain_del(&(event->ev_fd_active_head), \
                               &(event->ev_fd_active_tail), \
                               ed);
        ed->data.fd.active_flag = 0;
        ed->data.fd.in_active = 0;
    }
    ev_fd_wait_chain_del(&(event->ev_fd_wait_head), \
                         &(event->ev_fd_wait_tail), \
                         ed);
    mln_event_desc_free(ed);
}

/*
 * set dispatch callback
 */
void mln_event_callback_set(mln_event_t *ev, dispatch_callback dc, void *dc_data)
{
#if !defined(MSVC)
    pthread_mutex_lock(&ev->cb_lock);
#endif
    ev->callback = dc;
    ev->callback_data = dc_data;
#if !defined(MSVC)
    pthread_mutex_unlock(&ev->cb_lock);
#endif
}

/*
 * tools
 */
static inline void mln_event_fd_nonblock_set(int fd)
{
#if defined(MSVC)
    u_long opt = 1;
    ioctlsocket(fd, FIONBIO, &opt);
#else
    int flg;
    flg = fcntl(fd, F_GETFL, NULL);
    ASSERT(flg >= 0);
    flg = fcntl(fd, F_SETFL, flg | O_NONBLOCK);
    ASSERT(flg >= 0);
#endif
}

static inline void mln_event_fd_block_set(int fd)
{
#if defined(MSVC)
    u_long opt = 0;
    ioctlsocket(fd, FIONBIO, &opt);
#else
    int flg;
    flg = fcntl(fd, F_GETFL, NULL);
    ASSERT(flg >= 0);
    flg = fcntl(fd, F_SETFL, flg & ~(O_NONBLOCK));
    ASSERT(flg >= 0);
#endif
}

/*
 * dispatch
 */
#define BREAK_OUT(); \
    if (event->is_break) {\
        return;\
    }
#if defined(MLN_EPOLL)
MLN_FUNC_VOID(, void, mln_event_dispatch, (mln_event_t *event), (event), {
    __uint32_t mod_event;
    int nfds, n, oneshot, other_oneshot;
    mln_event_desc_t *ed;
    struct epoll_event events[M_EV_EPOLL_SIZE], *ev, mod_ev;

    while (1) {
        if (!pthread_mutex_trylock(&event->cb_lock)) {
            dispatch_callback cb = event->callback;
            void *data = event->callback_data;
            if (cb != NULL) {
                pthread_mutex_unlock(&event->cb_lock);
                cb(event, data);
            } else {
                pthread_mutex_unlock(&event->cb_lock);
            }
        }
        BREAK_OUT();
        mln_event_timer_process(event);
        BREAK_OUT();
        mln_event_active_fd_process(event);
        BREAK_OUT();
        mln_event_fd_timeout_process(event);
        BREAK_OUT();
        mln_event_timer_process(event);
        BREAK_OUT();

        if (pthread_mutex_trylock(&event->fd_lock)) {
            epoll_wait(event->unusedfd, events, M_EV_EPOLL_SIZE, M_EV_NOLOCK_TIMEOUT_MS);
        } else {
            nfds = epoll_wait(event->epollfd, events, M_EV_EPOLL_SIZE, M_EV_TIMEOUT_MS);
            if (nfds < 0) {
                if (errno == EINTR) {
                    pthread_mutex_unlock(&event->fd_lock);
                    continue;
                } else {
                    ASSERT(0);
                }
            } else if (nfds == 0) {
                pthread_mutex_unlock(&event->fd_lock);
                epoll_wait(event->unusedfd, events, M_EV_EPOLL_SIZE, M_EV_NOLOCK_TIMEOUT_MS);
                continue;
            }
            for (n = 0; n < nfds; ++n) {
                mod_event = 0;
                oneshot = 0;
                other_oneshot = 0;
                ev = &events[n];
                ed = (mln_event_desc_t *)(ev->data.ptr);
                if (ed->data.fd.is_clear)
                    continue;

                if (ev->events & EPOLLIN) {
                    if (ed->data.fd.rd_oneshot) {
                        if (ed->data.fd.in_active || ed->data.fd.in_process) {
                            if (!(ed->data.fd.active_flag & M_EV_RECV))
                                mln_event_fd_append_set(event, \
                                                        ed, \
                                                        ed->data.fd.fd, \
                                                        M_EV_RECV|M_EV_ONESHOT, \
                                                        M_EV_UNMODIFIED, \
                                                        ed->data.fd.rcv_data, \
                                                        ed->data.fd.rcv_handler, \
                                                        1);
                        } else {
                            ed->data.fd.rd_oneshot = 0;
                            oneshot = 1;
                            ed->flag &= (~M_EV_RECV);
                        }
                    }
                    if (!ed->data.fd.in_active && !ed->data.fd.in_process)
                        ed->data.fd.active_flag |= M_EV_RECV;
                }
                if (ev->events & EPOLLOUT) {
                    if (ed->data.fd.wr_oneshot) {
                        if (ed->data.fd.in_active || ed->data.fd.in_process) {
                            if (!(ed->data.fd.active_flag & M_EV_SEND))
                                mln_event_fd_append_set(event, \
                                                        ed, \
                                                        ed->data.fd.fd, \
                                                        M_EV_SEND|M_EV_ONESHOT, \
                                                        M_EV_UNMODIFIED, \
                                                        ed->data.fd.snd_data, \
                                                        ed->data.fd.snd_handler, \
                                                        1);
                        } else {
                            ed->data.fd.wr_oneshot = 0;
                            oneshot = 1;
                            ed->flag &= (~M_EV_SEND);
                        }
                    }
                    if (!ed->data.fd.in_active && !ed->data.fd.in_process)
                        ed->data.fd.active_flag |= M_EV_SEND;
                }
                if (ev->events & EPOLLERR) {
                    if (ed->data.fd.err_oneshot) {
                        if (ed->data.fd.in_active || ed->data.fd.in_process) {
                            if (!(ed->data.fd.active_flag & M_EV_ERROR))
                                mln_event_fd_append_set(event, \
                                                        ed, \
                                                        ed->data.fd.fd, \
                                                        M_EV_ERROR|M_EV_ONESHOT, \
                                                        M_EV_UNMODIFIED, \
                                                        ed->data.fd.err_data, \
                                                        ed->data.fd.err_handler, \
                                                        1);
                        } else {
                            ed->data.fd.err_oneshot = 0;
                            oneshot = 1;
                            ed->flag &= (~M_EV_ERROR);
                        }
                    }
                    if (!ed->data.fd.in_active && !ed->data.fd.in_process)
                        ed->data.fd.active_flag |= M_EV_ERROR;
                }

                if (ed->data.fd.in_active || ed->data.fd.in_process) continue;

                ev_fd_active_chain_add(&(event->ev_fd_active_head), \
                                       &(event->ev_fd_active_tail), \
                                       ed);
                ed->data.fd.in_active = 1;
                if (oneshot) {
                    if (ed->flag & M_EV_RECV) mod_event |= EPOLLIN;
                    if (ed->flag & M_EV_SEND) mod_event |= EPOLLOUT;
                    if (ed->flag & M_EV_ERROR) mod_event |= EPOLLERR;
                    if (ed->data.fd.rd_oneshot || \
                        ed->data.fd.wr_oneshot || \
                        ed->data.fd.err_oneshot)
                    {
                        other_oneshot = 1;
                    }
                    memset(&mod_ev, 0, sizeof(mod_ev));
                    if (other_oneshot) {
                        mod_ev.events = mod_event|EPOLLONESHOT;\
                        mod_ev.data.ptr = ed;\
                        epoll_ctl(event->epollfd, EPOLL_CTL_MOD, ed->data.fd.fd, &mod_ev);\
                    } else {
                        mod_ev.events = mod_event;\
                        mod_ev.data.ptr = ed;\
                        epoll_ctl(event->epollfd, EPOLL_CTL_MOD, ed->data.fd.fd, &mod_ev);\
                    }
                }
            }
            pthread_mutex_unlock(&event->fd_lock);
        }
    }
})
#elif defined(MLN_KQUEUE)
MLN_FUNC_VOID(, void, mln_event_dispatch, (mln_event_t *event), (event), {
    int nfds, n;
    mln_event_desc_t *ed;
    struct kevent events[M_EV_EPOLL_SIZE], *ev, mod;
    struct timespec ts;

    while (1) {
        if (!pthread_mutex_trylock(&event->cb_lock)) {
            dispatch_callback cb = event->callback;
            void *data = event->callback_data;
            if (cb != NULL) {
                pthread_mutex_unlock(&event->cb_lock);
                cb(event, data);
            } else {
                pthread_mutex_unlock(&event->cb_lock);
            }
        }
        BREAK_OUT();
        mln_event_timer_process(event);
        BREAK_OUT();
        mln_event_active_fd_process(event);
        BREAK_OUT();
        mln_event_fd_timeout_process(event);
        BREAK_OUT();
        mln_event_timer_process(event);
        BREAK_OUT();

        if (pthread_mutex_trylock(&event->fd_lock)) {
            ts.tv_sec = 0;
            ts.tv_nsec = M_EV_NOLOCK_TIMEOUT_NS;
            kevent(event->unusedfd, NULL, 0, events, M_EV_EPOLL_SIZE, &ts);
        } else {
            ts.tv_sec = 0;
            ts.tv_nsec = M_EV_TIMEOUT_NS;
            nfds = kevent(event->kqfd, NULL, 0, events, M_EV_EPOLL_SIZE, &ts);
            if (nfds < 0) {
                if (errno == EINTR) {
                    pthread_mutex_unlock(&event->fd_lock);
                    continue;
                } else {
                    ASSERT(0);
                }
            } else if (nfds == 0) {
                pthread_mutex_unlock(&event->fd_lock);
                ts.tv_sec = 0;
                ts.tv_nsec = M_EV_NOLOCK_TIMEOUT_NS;
                kevent(event->unusedfd, NULL, 0, events, M_EV_EPOLL_SIZE, &ts);
                continue;
            }
            for (n = 0; n < nfds; ++n) {
                ev = &events[n];
                ed = (mln_event_desc_t *)(ev->udata);
                if (ed->data.fd.is_clear)
                    continue;

                if (ev->filter == EVFILT_READ) {
                    if (ed->data.fd.rd_oneshot) {
                        if (ed->data.fd.in_active || ed->data.fd.in_process) {
                            if (!(ed->data.fd.active_flag & M_EV_RECV))
                                mln_event_fd_append_set(event, \
                                                        ed, \
                                                        ed->data.fd.fd, \
                                                        M_EV_RECV|M_EV_ONESHOT, \
                                                        M_EV_UNMODIFIED, \
                                                        ed->data.fd.rcv_data, \
                                                        ed->data.fd.rcv_handler, \
                                                        1);
                        } else {
                            ed->data.fd.rd_oneshot = 0;
                            EV_SET(&mod, ed->data.fd.fd, EVFILT_READ, EV_DISABLE, 0, 0, ed);
                            if (kevent(event->kqfd, &mod, 1, NULL, 0, NULL) < 0) {
                                ASSERT(0);
                            }
                            ed->flag &= (~M_EV_RECV);
                        }
                    }
                    if (!ed->data.fd.in_active && !ed->data.fd.in_process)
                        ed->data.fd.active_flag |= M_EV_RECV;
                }
                if (ev->filter == EVFILT_WRITE) {
                    if (ed->data.fd.wr_oneshot) {
                        if (ed->data.fd.in_active || ed->data.fd.in_process) {
                            if (!(ed->data.fd.active_flag & M_EV_SEND))
                                mln_event_fd_append_set(event, \
                                                        ed, \
                                                        ed->data.fd.fd, \
                                                        M_EV_SEND|M_EV_ONESHOT, \
                                                        M_EV_UNMODIFIED, \
                                                        ed->data.fd.snd_data, \
                                                        ed->data.fd.snd_handler, \
                                                        1);
                        } else {
                            ed->data.fd.wr_oneshot = 0;
                            EV_SET(&mod, ed->data.fd.fd, EVFILT_WRITE, EV_DISABLE, 0, 0, ed);
                            if (kevent(event->kqfd, &mod, 1, NULL, 0, NULL) < 0) {
                                ASSERT(0);
                            }
                            ed->flag &= (~M_EV_SEND);
                        }
                    }
                    if (!ed->data.fd.in_active && !ed->data.fd.in_process)
                        ed->data.fd.active_flag |= M_EV_SEND;
                }
                if ((ev->flags & EV_ERROR) && (ed->flag & M_EV_ERROR)) {
                    if (ed->data.fd.err_oneshot) {
                        if (ed->data.fd.in_active || ed->data.fd.in_process) {
                            if (!(ed->data.fd.active_flag & M_EV_ERROR))
                                mln_event_fd_append_set(event, \
                                                        ed, \
                                                        ed->data.fd.fd, \
                                                        M_EV_ERROR|M_EV_ONESHOT, \
                                                        M_EV_UNMODIFIED, \
                                                        ed->data.fd.err_data, \
                                                        ed->data.fd.err_handler, \
                                                        1);
                        } else {
                            ed->data.fd.err_oneshot = 0;
                            ed->flag &= ~(M_EV_ERROR);
                        }
                    }
                    if (!ed->data.fd.in_active && !ed->data.fd.in_process)
                        ed->data.fd.active_flag |= M_EV_ERROR;
                }

                if (ed->data.fd.in_active || ed->data.fd.in_process) continue;

                ev_fd_active_chain_add(&(event->ev_fd_active_head), \
                                       &(event->ev_fd_active_tail), \
                                       ed);
                ed->data.fd.in_active = 1;
            }
            pthread_mutex_unlock(&event->fd_lock);
        }
    }
})
#else
void mln_event_dispatch(mln_event_t *event)
{
    int nfds, fd;
    mln_event_desc_t *ed;
    fd_set *rd_set = &(event->rd_set);
    fd_set *wr_set = &(event->wr_set);
    fd_set *err_set = &(event->err_set);
    struct timeval tm;
    mln_u32_t move;

    while (1) {
#if !defined(MSVC)
        if (!pthread_mutex_trylock(&event->cb_lock)) {
            dispatch_callback cb = event->callback;
            void *data = event->callback_data;
            if (cb != NULL) {
                pthread_mutex_unlock(&event->cb_lock);
                cb(event, data);
            } else {
                pthread_mutex_unlock(&event->cb_lock);
            }
        }
#else
        if (event->callback != NULL) event->callback(event, event->callback_data);
#endif
        BREAK_OUT();
        mln_event_timer_process(event);
        BREAK_OUT();
        mln_event_active_fd_process(event);
        BREAK_OUT();
        mln_event_fd_timeout_process(event);
        BREAK_OUT();
        mln_event_timer_process(event);
        BREAK_OUT();
        event->select_fd = 1;
        FD_ZERO(rd_set);
        FD_ZERO(wr_set);
        FD_ZERO(err_set);

#if !defined(MSVC)
        if (pthread_mutex_trylock(&event->fd_lock)) {
            tm.tv_sec = 0;
            tm.tv_usec = M_EV_NOLOCK_TIMEOUT_US;
            select(event->select_fd, rd_set, wr_set, err_set, &tm);
        } else {
#endif
            for (ed = event->ev_fd_wait_head; \
                 ed != NULL; \
                 ed = ed->next)
            {
                fd = ed->data.fd.fd;
                if (ed->flag & M_EV_RECV) FD_SET(fd, rd_set);
                if (ed->flag & M_EV_SEND) FD_SET(fd, wr_set);
                if (ed->flag & M_EV_ERROR) FD_SET(fd, err_set);
                if (fd >= event->select_fd)
                    event->select_fd = fd + 1;
            }
            tm.tv_sec = 0;
            tm.tv_usec = M_EV_TIMEOUT_US;
            nfds = select(event->select_fd, rd_set, wr_set, err_set, &tm);
            if (nfds < 0) {
#if !defined(MSVC)
                if (errno == EINTR || errno == ENOMEM) {
                    pthread_mutex_unlock(&event->fd_lock);
                    continue;
                } else {
                    ASSERT(0);
                }
#else
                continue;
#endif
            } else if (nfds == 0) {
#if !defined(MSVC)
                pthread_mutex_unlock(&event->fd_lock);
#endif
                tm.tv_sec = 0;
                tm.tv_usec = M_EV_NOLOCK_TIMEOUT_US;
                select(event->select_fd, rd_set, wr_set, err_set, &tm);
                continue;
            }
            ed = event->ev_fd_wait_head;
            for (; nfds > 0 && ed != NULL; ed = ed->next) {
                if (ed->data.fd.in_active || ed->data.fd.in_process || ed->data.fd.is_clear)
                    continue;

                move = 0;
                fd = ed->data.fd.fd;
                if (FD_ISSET(fd, rd_set)) {
                    ed->data.fd.active_flag |= M_EV_RECV;
                    if (ed->data.fd.rd_oneshot == 1) {
                        ed->flag &= (~M_EV_RECV);
                        ed->data.fd.rd_oneshot = 0;
                    }
                    --nfds;
                    move = 1;
                }
                if (FD_ISSET(fd, wr_set)) {
                    ed->data.fd.active_flag |= M_EV_SEND;
                    if (ed->data.fd.wr_oneshot == 1) {
                        ed->flag &= (~M_EV_SEND);
                        ed->data.fd.wr_oneshot = 0;
                    }
                    --nfds;
                    move = 1;
                }
                if (FD_ISSET(fd, err_set)) {
                    ed->data.fd.active_flag |= M_EV_ERROR;
                    if (ed->data.fd.err_oneshot == 1) {
                        ed->flag &= (~M_EV_ERROR);
                        ed->data.fd.err_oneshot = 0;
                    }
                    --nfds;
                    move = 1;
                }
                if (move) {
                    ev_fd_active_chain_add(&(event->ev_fd_active_head), \
                                           &(event->ev_fd_active_tail), \
                                           ed);
                    ed->data.fd.in_active = 1;
                }
            }
#if !defined(MSVC)
            pthread_mutex_unlock(&event->fd_lock);
        }
#endif
    }
}
#endif

static inline void mln_event_active_fd_process(mln_event_t *event)
{
    mln_event_desc_t *ed;
    mln_event_fd_t *ef;
    ev_fd_handler h;
    void *data;
    int fd;

lp:
#if !defined(MSVC)
    if (pthread_mutex_trylock(&event->fd_lock))
        return;
#endif

    ed = event->ev_fd_active_head;
    if (ed != NULL) {
        ev_fd_active_chain_del(&(event->ev_fd_active_head), \
                               &(event->ev_fd_active_tail), \
                               ed);
        ef = &(ed->data.fd);
        if (ef->timeout_node != NULL) {
            mln_fheap_inline_delete(event->ev_fd_timeout_heap, ef->timeout_node, mln_event_fd_timeout_copy, mln_event_fd_timeout_cmp);
            mln_fheap_inline_node_free(event->ev_fd_timeout_heap, ef->timeout_node, NULL);
            ef->timeout_node = NULL;
            ef->end_us = 0;
        }

        ef->in_active = 0;
        ef->in_process = 1;
        if (ef->is_clear || event->is_break) ef->active_flag = 0;

        if (ef->active_flag & M_EV_RECV) {
            if (ef->rcv_handler != NULL) {
                h = ef->rcv_handler;
                data = ef->rcv_data;
                fd = ef->fd;
#if !defined(MSVC)
                pthread_mutex_unlock(&event->fd_lock);
#endif
                h(event, fd, data);
#if !defined(MSVC)
                pthread_mutex_lock(&event->fd_lock);
#endif
            }
            ef->active_flag &= (~M_EV_RECV);
        }
        if (ef->is_clear || event->is_break) ef->active_flag = 0;
        if (ef->active_flag & M_EV_SEND) {
            if (ef->snd_handler != NULL) {
                h = ef->snd_handler;
                data = ef->snd_data;
                fd = ef->fd;
#if !defined(MSVC)
                pthread_mutex_unlock(&event->fd_lock);
#endif
                h(event, fd, data);
#if !defined(MSVC)
                pthread_mutex_lock(&event->fd_lock);
#endif
            }
            ef->active_flag &= (~M_EV_SEND);
        }
        if (ef->is_clear || event->is_break) ef->active_flag = 0;
        if (ef->active_flag & M_EV_ERROR) {
            if (ef->err_handler != NULL) {
                h = ef->err_handler;
                data = ef->err_data;
                fd = ef->fd;
#if !defined(MSVC)
                pthread_mutex_unlock(&event->fd_lock);
#endif
                h(event, fd, data);
#if !defined(MSVC)
                pthread_mutex_lock(&event->fd_lock);
#endif
            }
            ef->active_flag &= (~M_EV_ERROR);
        }
        ef->in_process = 0;

        if (ef->is_clear) mln_event_fd_clr_set(event, ef->fd);

#if !defined(MSVC)
        pthread_mutex_unlock(&event->fd_lock);
#endif

        if (event->is_break) return;
        goto lp;
    } else {
#if !defined(MSVC)
        pthread_mutex_unlock(&event->fd_lock);
#endif
    }
}

static inline void mln_event_fd_timeout_process(mln_event_t *event)
{
    mln_u64_t now;
    struct timeval tv;
    gettimeofday(&tv, NULL);
    now = tv.tv_sec * 1000000 + tv.tv_usec;
    mln_event_desc_t *ed;
    mln_fheap_node_t *fn;
    mln_event_fd_t *ef;
    ev_fd_handler h;
    void *data;
    int fd;

lp:
#if !defined(MSVC)
    if (pthread_mutex_trylock(&event->fd_lock))
        return;
#endif

    fn = mln_fheap_minimum(event->ev_fd_timeout_heap);
    if (fn == NULL) {
#if !defined(MSVC)
        pthread_mutex_unlock(&event->fd_lock);
#endif
        return;
    }
    ed = (mln_event_desc_t *)mln_fheap_node_key(fn);
    ef = &(ed->data.fd);
    if (ef->in_active) {
        ev_fd_active_chain_del(&(event->ev_fd_active_head), \
                               &(event->ev_fd_active_tail), \
                               ed);
        ef->in_active = 0;
    }
    if (ef->end_us > now) {
#if !defined(MSVC)
        pthread_mutex_unlock(&event->fd_lock);
#endif
        return;
    }
    ef->in_process = 1;
    mln_fheap_inline_delete(event->ev_fd_timeout_heap, fn, mln_event_fd_timeout_copy, mln_event_fd_timeout_cmp);
    mln_fheap_inline_node_free(event->ev_fd_timeout_heap, fn, NULL);
    ed->data.fd.timeout_node = NULL;

    if (ed->data.fd.timeout_handler != NULL) {
        h = ed->data.fd.timeout_handler;
        fd = ed->data.fd.fd;
        data = ed->data.fd.timeout_data;
#if !defined(MSVC)
        pthread_mutex_unlock(&event->fd_lock);
#endif
        h(event, fd, data);
#if !defined(MSVC)
        pthread_mutex_lock(&event->fd_lock);
#endif
    }

    ef->in_process = 0;

    if (ef->is_clear) mln_event_fd_clr_set(event, ef->fd);

#if !defined(MSVC)
    pthread_mutex_unlock(&event->fd_lock);
#endif

    if (event->is_break) return;
    goto lp;
}

/*
 * rbtree functions
 */
MLN_FUNC(static, int, mln_event_rbtree_fd_cmp, (const void *k1, const void *k2), (k1, k2), {
    mln_event_desc_t *ed1 = (mln_event_desc_t *)k1;
    mln_event_desc_t *ed2 = (mln_event_desc_t *)k2;
    return ed1->data.fd.fd - ed2->data.fd.fd;
})

/*
 * fheap functions
 */
MLN_FUNC(static inline, int, mln_event_fd_timeout_cmp, (const void *k1, const void *k2), (k1, k2), {
    mln_event_desc_t *ed1 = (mln_event_desc_t *)k1;
    mln_event_desc_t *ed2 = (mln_event_desc_t *)k2;
    if (ed1->data.fd.end_us < ed2->data.fd.end_us) return 0;
    return 1;
})

MLN_FUNC_VOID(static inline, void, mln_event_fd_timeout_copy, (void *k1, void *k2), (k1, k2), {
    mln_event_desc_t *ed1 = (mln_event_desc_t *)k1;
    mln_event_desc_t *ed2 = (mln_event_desc_t *)k2;
    ed1->data.fd.end_us = ed2->data.fd.end_us;
})

MLN_FUNC(static inline, int, mln_event_fheap_timer_cmp, (const void *k1, const void *k2), (k1, k2), {
    mln_event_desc_t *ed1 = (mln_event_desc_t *)k1;
    mln_event_desc_t *ed2 = (mln_event_desc_t *)k2;
    if (ed1->data.tm.end_tm < ed2->data.tm.end_tm) return 0;
    return 1;
})

MLN_FUNC_VOID(static inline, void, mln_event_fheap_timer_copy, (void *k1, void *k2), (k1, k2), {
    mln_event_desc_t *ed1 = (mln_event_desc_t *)k1;
    mln_event_desc_t *ed2 = (mln_event_desc_t *)k2;
    ed1->data.tm.end_tm = ed2->data.tm.end_tm;
})

/*
 * chains
 */
MLN_CHAIN_FUNC_DEFINE(static inline, \
                      ev_fd_wait, \
                      mln_event_desc_t, \
                      prev, \
                      next);
MLN_CHAIN_FUNC_DEFINE(static inline, \
                      ev_fd_active, \
                      mln_event_desc_t, \
                      act_prev, \
                      act_next);
