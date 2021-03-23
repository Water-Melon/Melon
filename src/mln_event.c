
/*
 * Copyright (C) Niklaus F.Schen.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include "mln_event.h"
#include "mln_log.h"
#include "mln_global.h"

/*declarations*/
MLN_CHAIN_FUNC_DECLARE(ev_fd_wait, \
                       mln_event_desc_t, \
                       static inline void,);
MLN_CHAIN_FUNC_DECLARE(ev_fd_active, \
                       mln_event_desc_t, \
                       static inline void,);
MLN_CHAIN_FUNC_DECLARE(event, \
                       mln_event_t, \
                       static inline void,);
MLN_CHAIN_FUNC_DECLARE(ev_sig, \
                       mln_event_desc_t, \
                       static inline void,);
#if !defined(WINNT)
static void mln_event_atfork_lock(void);
static void mln_event_atfork_unlock(void);
#endif
static inline void
mln_event_desc_free(void *data);
static int
mln_event_rbtree_fd_cmp(const void *k1, const void *k2) __NONNULL2(1,2);
static int
mln_event_rbtree_sig_cmp(const void *k1, const void *k2) __NONNULL2(1,2);
static void
mln_event_rbtree_sig_chain_free(void *data);
static inline void
mln_event_sig_decrease_refer(int signo);
static int
mln_event_rbtree_refer_cmp(const void *k1, const void *k2) __NONNULL2(1,2);
static void
mln_event_rbtree_refer_free(void *data);
static int
mln_event_rbtree_refer_init(void);
static int
mln_event_fd_timeout_cmp(const void *k1, const void *k2);
static void
mln_event_fd_timeout_copy(void *k1, void *k2);
static int
mln_event_fheap_timer_cmp(const void *k1, const void *k2) __NONNULL2(1,2);
static void
mln_event_fheap_timer_copy(void *k1, void *k2) __NONNULL2(1,2);
static inline void
mln_event_set_fd_nonblock(int fd);
static inline void
mln_event_set_fd_block(int fd);
static inline void
mln_event_set_fd_clr(mln_event_t *event, int fd) __NONNULL1(1);
static void
mln_event_signal_handler(int signo);
static inline void
mln_event_deal_active_fd(mln_event_t *event) __NONNULL1(1);
static inline void
mln_event_deal_fd_timeout(mln_event_t *event, int *ret) __NONNULL2(1,2);
static inline void
mln_event_deal_timer(mln_event_t *event, int *ret) __NONNULL2(1,2);
static inline int
mln_event_set_fd_normal(mln_event_t *event, \
                        mln_event_desc_t *ed, \
                        int fd, \
                        mln_u32_t flag, \
                        int timeout_ms, \
                        void *data, \
                        ev_fd_handler fd_handler, \
                        int other_mark);
static inline int
mln_event_set_fd_append(mln_event_t *event, \
                        mln_event_desc_t *ed, \
                        int fd, \
                        mln_u32_t flag, \
                        int timeout_ms, \
                        void *data, \
                        ev_fd_handler fd_handler, \
                        int other_mark);
static int
mln_event_set_fd_timeout(mln_event_t *ev, mln_event_desc_t *ed, int timeout_ms);
static inline int
mln_event_set_signal_set(mln_event_t *event, \
                         int signo, \
                         void *data, \
                         ev_sig_handler sg_handler);
static inline int
mln_event_set_signal_unset(mln_event_t *event, \
                           int signo, \
                           void *data, \
                           ev_sig_handler sg_handler);
static void
mln_event_signal_fd_handler(mln_event_t *ev, int fd, void *data) __NONNULL1(1);

/*varliables*/
mln_event_desc_t fheap_min = {
    M_EV_TM, 0,
    {{NULL, NULL, 0}},
    NULL, NULL, NULL, NULL
};
mln_rbtree_t *ev_signal_refer_tree = NULL;
mln_event_t  *main_thread_event = NULL;
mln_event_t  *event_chain_head = NULL;
mln_event_t  *event_chain_tail = NULL;
mln_lock_t    event_lock;
mln_u32_t     is_lock_init = 0;
mln_u32_t     main_thread_freeing = 0;


mln_event_t *mln_event_init(mln_u32_t is_main)
{
    mln_event_t *ev;
    ev = (mln_event_t *)malloc(sizeof(mln_event_t));
    if (ev == NULL) {
        mln_log(error, "No memory.\n");
        return NULL;
    }
    ev->next = NULL;
    ev->prev = NULL;
    ev->callback = NULL;
    ev->callback_data = NULL;
    struct mln_rbtree_attr rbattr;
    rbattr.pool = NULL;
    rbattr.cmp = mln_event_rbtree_fd_cmp;
    rbattr.data_free = NULL;
    rbattr.cache = 0;
    ev->ev_fd_tree = mln_rbtree_init(&rbattr);
    if (ev->ev_fd_tree == NULL) {
        mln_log(error, "No memory.\n");
        goto err1;
    }
    ev->ev_fd_wait_head = NULL;
    ev->ev_fd_wait_tail = NULL;
    ev->ev_fd_active_head = NULL;
    ev->ev_fd_active_tail = NULL;
    if (!is_lock_init) {
        MLN_LOCK_INIT(&event_lock);
        is_lock_init = 1;
        if (pthread_atfork(mln_event_atfork_lock, \
                           mln_event_atfork_unlock, \
                           mln_event_atfork_unlock) != 0)
        {
            mln_log(error, "No memory.\n");
            goto err2;
        }
    }
    struct mln_fheap_attr fattr;
    fattr.cmp = mln_event_fd_timeout_cmp;
    fattr.copy = mln_event_fd_timeout_copy;
    fattr.key_free = NULL;
    fattr.min_val = &fheap_min;
    fattr.min_val_size = sizeof(mln_event_desc_t);
    ev->ev_fd_timeout_heap = mln_fheap_init(&fattr);
    if (ev->ev_fd_timeout_heap == NULL) {
        mln_log(error, "No memory.\n");
        goto err2;
    }
    /*timer heap*/
    fattr.cmp = mln_event_fheap_timer_cmp;
    fattr.copy = mln_event_fheap_timer_copy;
    fattr.key_free = mln_event_desc_free;
    fattr.min_val = &fheap_min;
    fattr.min_val_size = sizeof(mln_event_desc_t);
    ev->ev_timer_heap = mln_fheap_init(&fattr);
    if (ev->ev_timer_heap == NULL) {
        mln_log(error, "No memory.\n");
        goto err3;
    }
    /*signal rbtree*/
    rbattr.pool = NULL;
    rbattr.cmp = mln_event_rbtree_sig_cmp;
    rbattr.data_free = mln_event_rbtree_sig_chain_free;
    rbattr.cache = 0;
    ev->ev_signal_tree = mln_rbtree_init(&rbattr);
    if (ev->ev_signal_tree == NULL) {
        mln_log(error, "No memory.\n");
        goto err4;
    }
    ev->is_break = 0;
    ev->in_main_thread = is_main;
    int fds[2];
#if defined(WINNT)
    if (_pipe(fds, 256, O_BINARY) < 0) {
#else
    if (pipe(fds) < 0) {
#endif
        mln_log(error, "pipe error. %s\n", strerror(errno));
        goto err5;
    }
    ev->rd_fd = fds[0];
    ev->wr_fd = fds[1];
#if defined(MLN_EPOLL)
    ev->epollfd = epoll_create(M_EV_EPOLL_SIZE);
    if (ev->epollfd < 0) {
        mln_log(error, "epoll_create error. %s\n", strerror(errno));
        goto err6;
    }
#elif defined(MLN_KQUEUE)
    ev->kqfd = kqueue();
    if (ev->kqfd < 0) {
        mln_log(error, "kqueue error. %s\n", strerror(errno));
        goto err6;
        return NULL;
    }
#else
    ev->select_fd = 3;
    FD_ZERO(&(ev->rd_set));
    FD_ZERO(&(ev->wr_set));
    FD_ZERO(&(ev->err_set));
#endif
    if (mln_event_set_fd(ev, \
                         ev->rd_fd, \
                         M_EV_RECV, \
                         M_EV_UNLIMITED, \
                         NULL, \
                         mln_event_signal_fd_handler) < 0)
        goto err7;
    if (is_main) {
        if (main_thread_event == NULL) main_thread_event = ev;
        else {
            mln_log(error, "Main thread already existed.\n");
            abort();/*there is only one event run in main thread.*/
        }
    } else {
        MLN_LOCK(&event_lock);
        event_chain_add(&event_chain_head, &event_chain_tail, ev);
        MLN_UNLOCK(&event_lock);
    }
    return ev;

err7:
#if defined(MLN_EPOLL)
    close(ev->epollfd);
err6:
#elif defined(MLN_KQUEUE)
    close(ev->kqfd);
err6:
#else
    /*do nothing*/
#endif    
    mln_socket_close(ev->rd_fd);
    mln_socket_close(ev->wr_fd);
err5:
    mln_rbtree_destroy(ev->ev_signal_tree);
err4:
    mln_fheap_destroy(ev->ev_timer_heap);
err3:
    mln_fheap_destroy(ev->ev_fd_timeout_heap);
err2:
    mln_rbtree_destroy(ev->ev_fd_tree);
err1:
    free(ev);
    return NULL;
}

#if !defined(WINNT)
static void mln_event_atfork_lock(void)
{
    MLN_LOCK(&event_lock);
}

static void mln_event_atfork_unlock(void)
{
    MLN_UNLOCK(&event_lock);
}
#endif

static void
mln_event_signal_fd_handler(mln_event_t *ev, int fd, void *data)
{
    int signo = 0, n;
lp:
    n = read(fd, &signo, sizeof(signo));
    if (n < 0) {
        if (errno == EINTR) goto lp;
        mln_log(error, "pipe read error, fd:%d. %s\n", fd, strerror(errno));
        abort();
    } else if (n == 0) {
        mln_log(error, "pipe closed, fd:%d.\n", fd);
        abort();
    }
    mln_rbtree_node_t *rn;
    mln_event_sig_chain_t esc, *escp;
    esc.signo = signo;
    rn = mln_rbtree_search(ev->ev_signal_tree, \
                           ev->ev_signal_tree->root, \
                           &esc);
    if (mln_rbtree_null(rn, ev->ev_signal_tree)) return;
    escp = (mln_event_sig_chain_t *)(rn->data);
    mln_event_desc_t *ed;
    for (ed = escp->sig_head; ed != NULL; ed = ed->next) {
        if (ed->data.sig.handler != NULL)
            ed->data.sig.handler(ev, signo, ed->data.sig.data);
    }
}

void mln_event_destroy(mln_event_t *ev)
{
    if (ev == NULL) return;
    mln_event_desc_t *ed;
    if (ev->in_main_thread) 
        main_thread_freeing = 1;
    MLN_LOCK(&event_lock);
    mln_fheap_destroy(ev->ev_fd_timeout_heap);
    if (ev->next != NULL || \
        ev->prev != NULL || \
        event_chain_head == ev || \
        (ev == main_thread_event && ev->in_main_thread))
    {
        if (ev->in_main_thread) main_thread_event = NULL;
        else event_chain_del(&event_chain_head, &event_chain_tail, ev);
    }
    mln_rbtree_destroy(ev->ev_fd_tree);
    while ((ed = ev->ev_fd_wait_head) != NULL) {
        ev_fd_wait_chain_del(&(ev->ev_fd_wait_head), \
                             &(ev->ev_fd_wait_tail), \
                             ed);
        mln_event_desc_free(ed);
    }
    mln_fheap_destroy(ev->ev_timer_heap);
    mln_rbtree_destroy(ev->ev_signal_tree);
    mln_socket_close(ev->rd_fd);
    mln_socket_close(ev->wr_fd);
#if defined(MLN_EPOLL)
    close(ev->epollfd);
#elif defined(MLN_KQUEUE)
    close(ev->kqfd);
#else
    /*select do nothing.*/
#endif
    if (ev->in_main_thread)
        main_thread_freeing = 0;
    free(ev);
    if (event_chain_head == NULL) {
        mln_rbtree_destroy(ev_signal_refer_tree);
        ev_signal_refer_tree = NULL;
    }
    MLN_UNLOCK(&event_lock);
}

static inline void
mln_event_desc_free(void *data)
{
    if (data == NULL) return;
    free(data);
}

/*
 * ev_signal
 */
int mln_event_set_signal(mln_event_t *event, \
                         mln_u32_t flag, \
                         int signo, \
                         void *data, \
                         ev_sig_handler sg_handler)
{
    int ret;
    switch (flag) {
        case M_EV_SET:
            if (ev_signal_refer_tree == NULL) {
                if (mln_event_rbtree_refer_init() < 0)
                    return -1;
            }
            MLN_LOCK(&event_lock);
            ret = mln_event_set_signal_set(event, signo, data, sg_handler);
            MLN_UNLOCK(&event_lock);
            if (ret < 0) return -1;
            break;
        case M_EV_UNSET:
            if (ev_signal_refer_tree == NULL) {
                mln_log(error, "No signal set.\n");
                abort();
            }
            MLN_LOCK(&event_lock);
            ret = mln_event_set_signal_unset(event, signo, data, sg_handler);
            MLN_UNLOCK(&event_lock);
            if (ret < 0) return -1;
            break;
        default:
            mln_log(error, "flag error. %x\n", flag);
            abort();
    }
    return 0;
}

static inline int
mln_event_set_signal_set(mln_event_t *event, \
                         int signo, \
                         void *data, \
                         ev_sig_handler sg_handler)
{
    mln_event_desc_t *ed;
    ed = (mln_event_desc_t *)malloc(sizeof(mln_event_desc_t));
    if (ed == NULL) {
        mln_log(error, "No memory.\n");
        return -1;
    }
    ed->type = M_EV_SIG;
    ed->flag = 0;
    ed->data.sig.data = data;
    ed->data.sig.handler = sg_handler;
    ed->data.sig.signo = signo;
    ed->prev = NULL;
    ed->next = NULL;
    ed->act_prev = NULL;
    ed->act_next = NULL;
    mln_rbtree_node_t *rn;
    mln_event_sig_chain_t esc, *escp;
    esc.signo = signo;
    rn = mln_rbtree_search(event->ev_signal_tree, \
                           event->ev_signal_tree->root, \
                           &esc);
    if (mln_rbtree_null(rn, event->ev_signal_tree)) {
        escp = (mln_event_sig_chain_t *)malloc(sizeof(mln_event_sig_chain_t));
        if (escp == NULL) {
            mln_log(error, "No memory.\n");
            free(ed);
            return -1;
        }
        escp->signo = signo;
        escp->sig_head = NULL;
        escp->sig_tail = NULL;
        rn = mln_rbtree_node_new(event->ev_signal_tree, escp);
        if (rn == NULL) {
            mln_log(error, "No memory.\n");
            free(escp);
            free(ed);
            return -1;
        }
        mln_rbtree_insert(event->ev_signal_tree, rn);
    }
    escp = (mln_event_sig_chain_t *)(rn->data);
    ev_sig_chain_add(&(escp->sig_head), &(escp->sig_tail), ed);

    mln_event_sig_refer_t esr, *esrp;
    esr.signo = signo;
    rn = mln_rbtree_search(ev_signal_refer_tree, \
                           ev_signal_refer_tree->root, \
                           &esr);
    if (mln_rbtree_null(rn, ev_signal_refer_tree)) {
        esrp = (mln_event_sig_refer_t *)malloc(sizeof(mln_event_sig_refer_t));
        if (esrp == NULL) {
            mln_log(error, "No memory.\n");
            ev_sig_chain_del(&(escp->sig_head), &(escp->sig_tail), ed);
            free(ed);
            return -1;
        }
        esrp->signo = signo;
        esrp->refer_cnt = 0;
        rn = mln_rbtree_node_new(ev_signal_refer_tree, esrp);
        if (rn == NULL) {
            mln_log(error, "No memory.\n");
            free(esrp);
            ev_sig_chain_del(&(escp->sig_head), &(escp->sig_tail), ed);
            free(ed);
            return -1;
        }
        mln_rbtree_insert(ev_signal_refer_tree, rn);
    }
    esrp = (mln_event_sig_refer_t *)(rn->data);
    if (esrp->refer_cnt++ == 0) {
#if defined(WINNT)
        signal(SIGABRT, mln_event_signal_handler);
        signal(SIGFPE, mln_event_signal_handler);
        signal(SIGILL, mln_event_signal_handler);
        signal(SIGINT, mln_event_signal_handler);
        signal(SIGSEGV, mln_event_signal_handler);
        signal(SIGTERM, mln_event_signal_handler);
#else
        struct sigaction act;
        memset(&act, 0, sizeof(act));
        act.sa_handler = mln_event_signal_handler;
        sigemptyset(&(act.sa_mask));
        sigfillset(&(act.sa_mask));
        if (sigaction(signo, &act, NULL) < 0) {
            mln_log(error, "sigaction error, signo:%d. %s\n", \
                    signo, strerror(errno));
            abort();
        }
#endif
    }
    return 0;
}

static void
mln_event_signal_handler(int signo)
{
    int n;
    if (!main_thread_freeing) {
        MLN_LOCK(&event_lock);
lp1:
        n = write(main_thread_event->wr_fd, &signo, sizeof(signo));
        if (n < 0) {
            if (errno == EINTR) goto lp1;
            mln_log(error, "write error. %s\n", strerror(errno));
            abort();
        }
    }
    mln_event_t *ev;
    for (ev = event_chain_head; ev != NULL; ev = ev->next) {
lp2:
        n = write(ev->wr_fd, &signo, sizeof(signo));
        if (n < 0) {
            if (errno == EINTR) goto lp2;
            mln_log(error, "write error. %s\n", strerror(errno));
            abort();
        }
    }
    if (!main_thread_freeing)
        MLN_UNLOCK(&event_lock);
}

static inline int
mln_event_set_signal_unset(mln_event_t *event, \
                           int signo, \
                           void *data, \
                           ev_sig_handler sg_handler)
{
    mln_rbtree_node_t *rn;
    mln_event_sig_chain_t esc, *escp;
    esc.signo = signo;
    mln_event_desc_t *ed;
    rn = mln_rbtree_search(event->ev_signal_tree, \
                           event->ev_signal_tree->root, \
                           &esc);
    if (mln_rbtree_null(rn, event->ev_signal_tree)) {
        mln_log(error, "signal %d not registed.\n", signo);
        abort();
    }
    escp = (mln_event_sig_chain_t *)(rn->data);
    for (ed = escp->sig_head; ed != NULL; ed = ed->next) {
        if (ed->data.sig.data == data && \
            ed->data.sig.handler == sg_handler)
        break;
    }
    if (ed == NULL) {
        mln_log(error, "signal %d handler not registered.\n", signo);
        abort();
    }
    ev_sig_chain_del(&(escp->sig_head), &(escp->sig_tail), ed);
    mln_event_desc_free(ed);
    if (escp->sig_head == NULL) {
        mln_rbtree_delete(event->ev_signal_tree, rn);
        mln_rbtree_node_free(event->ev_signal_tree, rn);
    }
    mln_event_sig_decrease_refer(signo);
    return 0;
}

/*
 * ev_timer
 */
int mln_event_set_timer(mln_event_t *event, \
                        mln_u32_t msec, \
                        void *data, \
                        ev_tm_handler tm_handler)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    mln_uauto_t end = tv.tv_sec*1000000 + tv.tv_usec + msec*1000;
    mln_event_desc_t *ed;
    ed = (mln_event_desc_t *)malloc(sizeof(mln_event_desc_t));
    if (ed == NULL) {
        mln_log(error, "No memory.\n");
        return -1;
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
    mln_fheap_node_t *fn = mln_fheap_node_init(event->ev_timer_heap, ed);
    if (fn == NULL) {
        mln_log(error, "No memory.\n");
        free(ed);
        return -1;
    }
    mln_fheap_insert(event->ev_timer_heap, fn);
    return 0;
}

static inline void
mln_event_deal_timer(mln_event_t *event, int *ret)
{
    mln_uauto_t now;
    struct timeval tv;
    gettimeofday(&tv, NULL);
    now = tv.tv_sec * 1000000 + tv.tv_usec;
    mln_event_desc_t *ed;
    mln_fheap_node_t *fn;
    while ( 1 ) {
        fn = mln_fheap_minimum(event->ev_timer_heap);
        if (fn == NULL) {
            break;
        }
        *ret = -1;
        ed = (mln_event_desc_t *)(fn->key);
        if (ed->data.tm.end_tm > now) break;
        fn = mln_fheap_extract_min(event->ev_timer_heap);
        if (ed->data.tm.handler != NULL)
            ed->data.tm.handler(event, ed->data.tm.data);
        mln_fheap_node_destroy(event->ev_timer_heap, fn);
        if (event->is_break) return;
    }
}

/*
 * ev_fd
 */
void mln_event_set_fd_timeout_handler(mln_event_t *event, \
                                      int fd, \
                                      void *data, \
                                      ev_fd_handler timeout_handler)
{
    mln_event_desc_t tmp;
    memset(&tmp, 0, sizeof(tmp));
    tmp.type = M_EV_FD;
    tmp.data.fd.fd = fd;
    mln_rbtree_node_t *rn = mln_rbtree_search(event->ev_fd_tree, \
                                              event->ev_fd_tree->root, \
                                              &tmp);
    if (mln_rbtree_null(rn, event->ev_fd_tree)) {
        mln_log(error, "No such file descriptor in RB-Tree.\n");
        abort();
    }
    mln_event_desc_t *ed = (mln_event_desc_t *)(rn->data);
    ed->data.fd.timeout_data = data;
    ed->data.fd.timeout_handler = timeout_handler;
}

int mln_event_set_fd(mln_event_t *event, \
                     int fd, \
                     mln_u32_t flag, \
                     int timeout_ms, \
                     void *data, \
                     ev_fd_handler fd_handler)
{
    if (fd < 0 || \
        (flag & ~M_EV_FD_MASK) || \
        flag > M_EV_CLR || \
        ((flag & M_EV_NONBLOCK) && (flag & M_EV_BLOCK)))
    {
        mln_log(error, "fd or flag error.\n");
        abort();
    }
    if (flag == M_EV_CLR) {
        mln_event_set_fd_clr(event, fd);
        return 0;
    }
    mln_event_desc_t tmp;
    memset(&tmp, 0, sizeof(tmp));
    tmp.type = M_EV_FD;
    tmp.data.fd.fd = fd;
    mln_rbtree_node_t *rn;
    rn = mln_rbtree_search(event->ev_fd_tree, \
                           event->ev_fd_tree->root, \
                           &tmp);
    if (!mln_rbtree_null(rn, event->ev_fd_tree)) {
        if (flag & M_EV_APPEND) {
            if (flag & M_EV_NONBLOCK) mln_event_set_fd_nonblock(fd);
            if (flag & M_EV_BLOCK) mln_event_set_fd_block(fd);
            if (((mln_event_desc_t *)(rn->data))->data.fd.is_clear) {
                mln_log(error, "Append fd already clear.\n");
                abort();
            }
            if (mln_event_set_fd_append(event, \
                                        (mln_event_desc_t *)(rn->data), \
                                        fd, \
                                        flag, \
                                        timeout_ms, \
                                        data, \
                                        fd_handler, \
                                        1) < 0)
            return -1;
        } else {
            if (flag & M_EV_NONBLOCK) {
                mln_event_set_fd_nonblock(fd);
            } else {
                mln_event_set_fd_block(fd);
            }
            if (mln_event_set_fd_normal(event, \
                                        (mln_event_desc_t *)(rn->data), \
                                        fd, \
                                        flag, \
                                        timeout_ms, \
                                        data, \
                                        fd_handler, \
                                        ((mln_event_desc_t *)(rn->data))->data.fd.is_clear?0:1) < 0)
            return -1;
        }
        return 0;
    }
    if (flag & M_EV_NONBLOCK) {
        mln_event_set_fd_nonblock(fd);
    } else {
        mln_event_set_fd_block(fd);
    }
    if (mln_event_set_fd_normal(event, NULL, fd, flag, timeout_ms, data, fd_handler, 0) < 0)
        return -1;
    return 0;
}

static inline int
mln_event_set_fd_normal(mln_event_t *event, \
                        mln_event_desc_t *ed, \
                        int fd, \
                        mln_u32_t flag, \
                        int timeout_ms, \
                        void *data, \
                        ev_fd_handler fd_handler, \
                        int other_mark)
{
    if (ed == NULL) {
        ed = (mln_event_desc_t *)malloc(sizeof(mln_event_desc_t));
        if (ed == NULL) {
            mln_log(error, "No memory.\n");
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
            mln_log(error, "No memory.\n");
            free(ed);
            return -1;
        }
        mln_rbtree_insert(event->ev_fd_tree, rn);
        ev_fd_wait_chain_add(&(event->ev_fd_wait_head), \
                             &(event->ev_fd_wait_tail), \
                             ed);
        if (mln_event_set_fd_timeout(event, ed, timeout_ms) < 0) {
            free(ed);
            return -1;
        }
    } else {
        if (ed->data.fd.is_clear) {
            memset(&(ed->data.fd), 0, sizeof(mln_event_fd_t));
            ed->data.fd.fd = fd;
            ed->flag = 0;
        } else {
            ed->flag = 0;
            ed->data.fd.rd_oneshot = 0;
            ed->data.fd.wr_oneshot = 0;
            ed->data.fd.err_oneshot = 0;
            ed->data.fd.fd = fd;
        }
        if (mln_event_set_fd_timeout(event, ed, timeout_ms) < 0) {
            return -1;
        }
    }

    return mln_event_set_fd_append(event, \
                                   ed, \
                                   fd, \
                                   flag, \
                                   M_EV_UNMODIFIED, \
                                   data, \
                                   fd_handler, \
                                   other_mark);
}

static inline int
mln_event_set_fd_append(mln_event_t *event, \
                        mln_event_desc_t *ed, \
                        int fd, \
                        mln_u32_t flag, \
                        int timeout_ms, \
                        void *data, \
                        ev_fd_handler fd_handler, \
                        int other_mark)
{
    if (mln_event_set_fd_timeout(event, ed, timeout_ms) < 0)
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
            mln_log(error, "kevent error. %s\n", strerror(errno));
            abort();
        }
        EV_SET(&ev, fd, EVFILT_WRITE, EV_ADD|EV_ERROR|EV_DISABLE, 0, 0, ed);
        if (kevent(event->kqfd, &ev, 1, NULL, 0, NULL) < 0) {
            mln_log(error, "kevent error. %s\n", strerror(errno));
            abort();
        }
    }
    if (flag & M_EV_RECV) {
        ed->flag |= M_EV_RECV;
        if (oneshot) ed->data.fd.rd_oneshot = 1;
        EV_SET(&ev, fd, EVFILT_READ, EV_ENABLE, 0, 0, ed);
        if (kevent(event->kqfd, &ev, 1, NULL, 0, NULL) < 0) {
            mln_log(error, "kevent error. %s\n", strerror(errno));
            abort();
        }
        ed->data.fd.rcv_data = data;
        ed->data.fd.rcv_handler = fd_handler;
    }
    if (flag & M_EV_SEND) {
        ed->flag |= M_EV_SEND;
        if (oneshot) ed->data.fd.wr_oneshot = 1;
        EV_SET(&ev, fd, EVFILT_WRITE, EV_ENABLE, 0, 0, ed);
        if (kevent(event->kqfd, &ev, 1, NULL, 0, NULL) < 0) {
            mln_log(error, "kevent error. %s\n", strerror(errno));
            abort();
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

static int
mln_event_set_fd_timeout(mln_event_t *ev, mln_event_desc_t *ed, int timeout_ms)
{
    if (timeout_ms == M_EV_UNMODIFIED) return 0;
    mln_event_fd_t *ef = &(ed->data.fd);
    if (timeout_ms == M_EV_UNLIMITED) {
        if (ef->timeout_node != NULL) {
            mln_fheap_delete(ev->ev_fd_timeout_heap, ef->timeout_node);
            mln_fheap_node_destroy(ev->ev_fd_timeout_heap, ef->timeout_node);
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
        fn = mln_fheap_node_init(ev->ev_fd_timeout_heap, ed);
        if (fn == NULL) {
            mln_log(error, "No memory.\n");
            return -1;
        }
        ef->timeout_node = fn;
        mln_fheap_insert(ev->ev_fd_timeout_heap, fn);
    } else {
        fn = ef->timeout_node;
        mln_fheap_delete(ev->ev_fd_timeout_heap, fn);
        ef->end_us = tv.tv_sec*1000000+tv.tv_usec+timeout_ms*1000;
        mln_fheap_insert(ev->ev_fd_timeout_heap, fn);
    }
    return 0;
}

static inline void
mln_event_set_fd_clr(mln_event_t *event, int fd)
{
    mln_event_desc_t tmp, *ed;
    memset(&tmp, 0, sizeof(tmp));
    tmp.type = M_EV_FD;
    tmp.data.fd.fd = fd;
    mln_rbtree_node_t *rn;
    rn = mln_rbtree_search(event->ev_fd_tree, \
                           event->ev_fd_tree->root, \
                           &tmp);
    if (mln_rbtree_null(rn, event->ev_fd_tree)) {
        return;
    }
    ed = (mln_event_desc_t *)(rn->data);
    if (ed->data.fd.timeout_node != NULL) {
        mln_fheap_delete(event->ev_fd_timeout_heap, ed->data.fd.timeout_node);
        mln_fheap_node_destroy(event->ev_fd_timeout_heap, ed->data.fd.timeout_node);
        ed->data.fd.timeout_node = NULL;
        ed->data.fd.end_us = 0;
    }
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
#if defined(MLN_EPOLL)
    struct epoll_event ev;
    memset(&ev, 0, sizeof(ev));
    ev.data.ptr = ed;
    epoll_ctl(event->epollfd, EPOLL_CTL_DEL, fd, &ev);
#elif defined(MLN_KQUEUE)
    struct kevent ev;
    EV_SET(&ev, fd, EVFILT_READ, EV_DELETE, 0, 0, ed);
    kevent(event->kqfd, &ev, 1, NULL, 0, NULL);
#else
    if (ed->flag & M_EV_RECV)
        FD_CLR(fd, &(event->rd_set));
    if (ed->flag & M_EV_SEND)
        FD_CLR(fd, &(event->wr_set));
    if (ed->flag & M_EV_ERROR)
        FD_CLR(fd, &(event->err_set));
#endif
    mln_event_desc_free(ed);
}

/*
 * set_break
 */
void mln_event_set_break(mln_event_t *ev)
{
    ev->is_break = 1;
}

/*
 * set dispatch callback
 */
void mln_event_set_callback(mln_event_t *ev, \
                            dispatch_callback dc, \
                            void *dc_data)
{
    ev->callback = dc;
    ev->callback_data = dc_data;
}

/*
 * tools
 */
static inline void
mln_event_set_fd_nonblock(int fd)
{
#if defined(WINNT)
    u_long opt = 1;
    ioctlsocket(fd, FIONBIO, &opt);
#else
    int flg;
    flg = fcntl(fd, F_GETFL, NULL);
    if (flg < 0) {
        mln_log(error, "fcntl F_GETFL failed. %s\n", strerror(errno));
        abort();
    }
    if (fcntl(fd, F_SETFL, flg | O_NONBLOCK) < 0) {
        mln_log(error, "fcntl F_SETFL failed. %s\n", strerror(errno));
        abort();
    }
#endif
}

static inline void
mln_event_set_fd_block(int fd)
{
#if defined(WINNT)
    u_long opt = 0;
    ioctlsocket(fd, FIONBIO, &opt);
#else
    int flg;
    flg = fcntl(fd, F_GETFL, NULL);
    if (flg < 0) {
        mln_log(error, "fcntl F_GETFL failed. %s\n", strerror(errno));
        abort();
    }
    if (fcntl(fd, F_SETFL, flg & ~(O_NONBLOCK)) < 0) {
        mln_log(error, "fcntl F_SETFL failed. %s\n", strerror(errno));
        abort();
    }
#endif
}

/*
 * dispatch
 */
#define BREAK_OUT(); \
    if (event->is_break) {\
        event->is_break = 0;\
        return;\
    }
#if defined(MLN_EPOLL)
void mln_event_dispatch(mln_event_t *event)
{
    __uint32_t mod_event;
    int nfds, n, oneshot, other_oneshot, ret;
    mln_event_desc_t *ed;
    struct epoll_event events[M_EV_EPOLL_SIZE], *ev, mod_ev;
    while (1) {
        ret = 0;
        if (event->callback) {
            event->callback(event, event->callback_data);
            BREAK_OUT();
        }
        mln_event_deal_timer(event, &ret);
        BREAK_OUT();
        mln_event_deal_active_fd(event);
        BREAK_OUT();
        mln_event_deal_fd_timeout(event, &ret);
        BREAK_OUT();
        mln_event_deal_timer(event, &ret);
        BREAK_OUT();
        if (ret < 0)
            nfds = epoll_wait(event->epollfd, events, M_EV_EPOLL_SIZE, M_EV_TIMEOUT_MS);
        else
            nfds = epoll_wait(event->epollfd, events, M_EV_EPOLL_SIZE, -1);
        if (nfds < 0) {
            if (errno == EINTR) continue;
            else {
                mln_log(error, "epoll_wait error. %s\n", strerror(errno));
                abort();
            }
        } else if (nfds == 0) {
            continue;
        }
        for (n = 0; n < nfds; ++n) {
            mod_event = 0;
            oneshot = 0;
            other_oneshot = 0;
            ev = &events[n];
            ed = (mln_event_desc_t *)(ev->data.ptr);
            if (ev->events & EPOLLIN) {
                if (ed->data.fd.rd_oneshot) {
                    ed->data.fd.rd_oneshot = 0;
                    oneshot = 1;
                    ed->flag &= (~M_EV_RECV);
                }
                ed->data.fd.active_flag |= M_EV_RECV;
            }
            if (ev->events & EPOLLOUT) {
                if (ed->data.fd.wr_oneshot) {
                    ed->data.fd.wr_oneshot = 0;
                    oneshot = 1;
                    ed->flag &= (~M_EV_SEND);
                }
                ed->data.fd.active_flag |= M_EV_SEND;
            }
            if (ev->events & EPOLLERR) {
                if (ed->data.fd.err_oneshot) {
                    ed->data.fd.err_oneshot = 0;
                    oneshot = 1;
                    ed->flag &= (~M_EV_ERROR);
                }
                ed->data.fd.active_flag |= M_EV_ERROR;
            }
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
    }
}
#elif defined(MLN_KQUEUE)
void mln_event_dispatch(mln_event_t *event)
{
    int nfds, n, ret;
    mln_event_desc_t *ed;
    struct kevent events[M_EV_EPOLL_SIZE], *ev, mod;
    struct timespec ts;
    while (1) {
        ret = 0;
        if (event->callback) {
            event->callback(event, event->callback_data);
            BREAK_OUT();
        }
        mln_event_deal_timer(event, &ret);
        BREAK_OUT();
        mln_event_deal_active_fd(event);
        BREAK_OUT();
        mln_event_deal_fd_timeout(event, &ret);
        BREAK_OUT();
        mln_event_deal_timer(event, &ret);
        BREAK_OUT();
        if (ret < 0) {
            ts.tv_sec = 0;
            ts.tv_nsec = M_EV_TIMEOUT_NS;
            nfds = kevent(event->kqfd, NULL, 0, events, M_EV_EPOLL_SIZE, &ts);
        } else {
            nfds = kevent(event->kqfd, NULL, 0, events, M_EV_EPOLL_SIZE, NULL);
        }
        if (nfds < 0) {
            if (errno == EINTR) continue;
            else {
                mln_log(error, "kevent error. %s\n", strerror(errno));
                abort();
            }
        } else if (nfds == 0) {
            continue;
        }
        for (n = 0; n < nfds; ++n) {
            ev = &events[n];
            ed = (mln_event_desc_t *)(ev->udata);
            if (ev->filter == EVFILT_READ) {
                ed->data.fd.active_flag |= M_EV_RECV;
                if (ed->data.fd.rd_oneshot) {
                    ed->data.fd.rd_oneshot = 0;
                    EV_SET(&mod, ed->data.fd.fd, EVFILT_READ, EV_DISABLE, 0, 0, ed);
                    if (kevent(event->kqfd, &mod, 1, NULL, 0, NULL) < 0) {
                        mln_log(error, "kevent error. %s\n", strerror(errno));
                        abort();
                    }
                    ed->flag &= (~M_EV_RECV);
                }
            }
            if (ev->filter == EVFILT_WRITE) {
                if (ed->data.fd.wr_oneshot) {
                    ed->data.fd.wr_oneshot = 0;
                    EV_SET(&mod, ed->data.fd.fd, EVFILT_WRITE, EV_DISABLE, 0, 0, ed);
                    if (kevent(event->kqfd, &mod, 1, NULL, 0, NULL) < 0) {
                        mln_log(error, "kevent error. %s\n", strerror(errno));
                        abort();
                    }
                    ed->flag &= (~M_EV_SEND);
                }
                ed->data.fd.active_flag |= M_EV_SEND;
            }
            if ((ev->flags & EV_ERROR) && (ed->flag & M_EV_ERROR)) {
                if (ed->data.fd.err_oneshot) {
                    ed->data.fd.err_oneshot = 0;
                    ed->flag &= ~(M_EV_ERROR);
                }
                ed->data.fd.active_flag |= M_EV_ERROR;
            }
            ev_fd_active_chain_add(&(event->ev_fd_active_head), \
                                   &(event->ev_fd_active_tail), \
                                   ed);
            ed->data.fd.in_active = 1;
        }
    }
}
#else
void mln_event_dispatch(mln_event_t *event)
{
    int nfds, fd, ret;
    mln_event_desc_t *ed;
    fd_set *rd_set = &(event->rd_set);
    fd_set *wr_set = &(event->wr_set);
    fd_set *err_set = &(event->err_set);
    struct timeval tm;
    mln_u32_t move;
    while (1) {
        ret = 0;
        if (event->callback) {
            event->callback(event, event->callback_data);
            BREAK_OUT();
        }
        mln_event_deal_timer(event, &ret);
        BREAK_OUT();
        mln_event_deal_active_fd(event);
        BREAK_OUT();
        mln_event_deal_fd_timeout(event, &ret);
        BREAK_OUT();
        mln_event_deal_timer(event, &ret);
        BREAK_OUT();
        event->select_fd = 1;
        FD_ZERO(rd_set);
        FD_ZERO(wr_set);
        FD_ZERO(err_set);
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
        if (ret < 0) {
            tm.tv_sec = 0;
            tm.tv_usec = M_EV_TIMEOUT_US;
            nfds = select(event->select_fd, rd_set, wr_set, err_set, &tm);
        } else {
            nfds = select(event->select_fd, rd_set, wr_set, err_set, NULL);
        }
        if (nfds < 0) {
            if (errno == EINTR || errno == ENOMEM) continue;
            else {
                mln_log(error, "select error. %s\n", strerror(errno));
                abort();
            }
        } else if (nfds == 0) {
            continue;
        }
        ed = event->ev_fd_wait_head;
        for (; nfds > 0 && ed != NULL; ed = ed->next) {
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
    }
}
#endif

static inline void
mln_event_deal_active_fd(mln_event_t *event)
{
    mln_event_desc_t *ed;
    mln_event_fd_t *ef;
    while ((ed = event->ev_fd_active_head) != NULL) {
        ev_fd_active_chain_del(&(event->ev_fd_active_head), \
                               &(event->ev_fd_active_tail), \
                               ed);
        ef = &(ed->data.fd);
        if (ef->timeout_node != NULL) {
            mln_fheap_delete(event->ev_fd_timeout_heap, ef->timeout_node);
            mln_fheap_node_destroy(event->ev_fd_timeout_heap, ef->timeout_node);
            ef->timeout_node = NULL;
            ef->end_us = 0;
        }
        ef->in_active = 0;
        ef->in_process = 1;
        if (ef->is_clear || event->is_break) ef->active_flag = 0;
        if (ef->active_flag & M_EV_RECV) {
            if (ef->rcv_handler != NULL)
                ef->rcv_handler(event, ef->fd, ef->rcv_data);
            ef->active_flag &= (~M_EV_RECV);
        }
        if (ef->is_clear || event->is_break) ef->active_flag = 0;
        if (ef->active_flag & M_EV_SEND) {
            if (ef->snd_handler != NULL)
                ef->snd_handler(event, ef->fd, ef->snd_data);
            ef->active_flag &= (~M_EV_SEND);
        }
        if (ef->is_clear || event->is_break) ef->active_flag = 0;
        if (ef->active_flag & M_EV_ERROR) {
            if (ef->err_handler != NULL)
                ef->err_handler(event, ef->fd, ef->err_data);
            ef->active_flag &= (~M_EV_ERROR);
        }
        ef->in_process = 0;
        if (ef->is_clear)
            mln_event_set_fd_clr(event, ef->fd);
        if (event->is_break) return;
    }
}

static inline void
mln_event_deal_fd_timeout(mln_event_t *event, int *ret)
{
    mln_u64_t now;
    struct timeval tv;
    gettimeofday(&tv, NULL);
    now = tv.tv_sec * 1000000 + tv.tv_usec;
    mln_event_desc_t *ed;
    mln_fheap_node_t *fn;
    mln_event_fd_t *ef;
    while ( 1 ) {
        fn = mln_fheap_minimum(event->ev_fd_timeout_heap);
        if (fn == NULL) {
            break;
        }
        *ret = -1;
        ed = (mln_event_desc_t *)(fn->key);
        ef = &(ed->data.fd);
        if (ef->in_active) {
            ev_fd_active_chain_del(&(event->ev_fd_active_head), \
                                   &(event->ev_fd_active_tail), \
                                   ed);
            ef->in_active = 0;
        }
        ef->in_process = 1;
        if (ef->end_us > now) break;
        mln_fheap_delete(event->ev_fd_timeout_heap, fn);
        mln_fheap_node_destroy(event->ev_fd_timeout_heap, fn);
        ed->data.fd.timeout_node = NULL;
        if (ed->data.fd.timeout_handler != NULL)
            ed->data.fd.timeout_handler(event, ed->data.fd.fd, ed->data.fd.timeout_data);
        ef->in_process = 0;
        if (ef->is_clear)
            mln_event_set_fd_clr(event, ef->fd);
        if (event->is_break) return;
    }
}

/*
 * rbtree functions
 */
static int
mln_event_rbtree_fd_cmp(const void *k1, const void *k2)
{
    mln_event_desc_t *ed1 = (mln_event_desc_t *)k1;
    mln_event_desc_t *ed2 = (mln_event_desc_t *)k2;
    return ed1->data.fd.fd - ed2->data.fd.fd;
}

static int
mln_event_rbtree_sig_cmp(const void *k1, const void *k2)
{
    mln_event_sig_chain_t *esc1 = (mln_event_sig_chain_t *)k1;
    mln_event_sig_chain_t *esc2 = (mln_event_sig_chain_t *)k2;
    return esc1->signo - esc2->signo;
}

static void
mln_event_rbtree_sig_chain_free(void *data)
{
    if (data == NULL) return;
    mln_event_sig_chain_t *esc = (mln_event_sig_chain_t *)data;
    mln_event_desc_t *ed;
    while ((ed = esc->sig_head) != NULL) {
        ev_sig_chain_del(&(esc->sig_head), &(esc->sig_tail), ed);
        mln_event_desc_free(ed);
        mln_event_sig_decrease_refer(esc->signo);
    }
    free(esc);
}

static inline void
mln_event_sig_decrease_refer(int signo)
{
    mln_rbtree_node_t *rn;
    mln_event_sig_refer_t esr, *ptr;
    esr.signo = signo;
    rn  = mln_rbtree_search(ev_signal_refer_tree, \
                            ev_signal_refer_tree->root, \
                            &esr);
    if (mln_rbtree_null(rn, ev_signal_refer_tree)) return;
    ptr = (mln_event_sig_refer_t *)(rn->data);
    if (ptr->refer_cnt == 0) {
        mln_log(error, "signal referrence shouldn't be 0.\n");
        abort();
    }
    if ((--(ptr->refer_cnt)) == 0) {
        mln_rbtree_delete(ev_signal_refer_tree, rn);
        mln_rbtree_node_free(ev_signal_refer_tree, rn);
    }
}

static int
mln_event_rbtree_refer_cmp(const void *k1, const void *k2)
{
    mln_event_sig_refer_t *esr1 = (mln_event_sig_refer_t *)k1;
    mln_event_sig_refer_t *esr2 = (mln_event_sig_refer_t *)k2;
    return esr1->signo - esr2->signo;
}

static void
mln_event_rbtree_refer_free(void *data)
{
    if (data == NULL) return ;
    mln_event_sig_refer_t *esr = (mln_event_sig_refer_t *)data;
#if defined(WINNT)
    signal(esr->signo, SIG_DFL);
#else
    struct sigaction act;
    memset(&act, 0, sizeof(act));
    act.sa_handler = SIG_DFL;
    if (sigaction(esr->signo, &act, NULL) != 0) {
        mln_log(error, "sigaction error. %s\n", strerror(errno));
        abort();
    }
#endif
    free(data);
}

static int
mln_event_rbtree_refer_init(void)
{
    struct mln_rbtree_attr rbattr;
    rbattr.pool = NULL;
    rbattr.cmp = mln_event_rbtree_refer_cmp;
    rbattr.data_free = mln_event_rbtree_refer_free;
    rbattr.cache = 0;
    ev_signal_refer_tree = mln_rbtree_init(&rbattr);
    if (ev_signal_refer_tree == NULL) {
        mln_log(error, "No memory.\n");
        return -1;
    }
    return 0;
}

/*
 * fheap functions
 */
static int
mln_event_fd_timeout_cmp(const void *k1, const void *k2)
{
    mln_event_desc_t *ed1 = (mln_event_desc_t *)k1;
    mln_event_desc_t *ed2 = (mln_event_desc_t *)k2;
    if (ed1->data.fd.end_us < ed2->data.fd.end_us) return 0;
    return 1;
}

static void
mln_event_fd_timeout_copy(void *k1, void *k2)
{
    mln_event_desc_t *ed1 = (mln_event_desc_t *)k1;
    mln_event_desc_t *ed2 = (mln_event_desc_t *)k2;
    ed1->data.fd.end_us = ed2->data.fd.end_us;
}

static int
mln_event_fheap_timer_cmp(const void *k1, const void *k2)
{
    mln_event_desc_t *ed1 = (mln_event_desc_t *)k1;
    mln_event_desc_t *ed2 = (mln_event_desc_t *)k2;
    if (ed1->data.tm.end_tm < ed2->data.tm.end_tm) return 0;
    return 1;
}

static void
mln_event_fheap_timer_copy(void *k1, void *k2)
{
    mln_event_desc_t *ed1 = (mln_event_desc_t *)k1;
    mln_event_desc_t *ed2 = (mln_event_desc_t *)k2;
    ed1->data.tm.end_tm = ed2->data.tm.end_tm;
}

/*
 * chains
 */
MLN_CHAIN_FUNC_DEFINE(ev_fd_wait, \
                      mln_event_desc_t, \
                      static inline void, \
                      prev, \
                      next);
MLN_CHAIN_FUNC_DEFINE(ev_fd_active, \
                      mln_event_desc_t, \
                      static inline void, \
                      act_prev, \
                      act_next);
MLN_CHAIN_FUNC_DEFINE(event, \
                      mln_event_t, \
                      static inline void, \
                      prev, \
                      next);
MLN_CHAIN_FUNC_DEFINE(ev_sig, \
                      mln_event_desc_t, \
                      static inline void, \
                      prev, \
                      next);
