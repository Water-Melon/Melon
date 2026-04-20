
/*
 * Copyright (C) Niklaus F.Schen.
 */
#if !defined(MSVC)
#include "mln_iothread.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include "mln_func.h"
#include <sys/socket.h>

static inline void mln_iothread_fd_nonblock_set(int fd);
static inline mln_iothread_msg_t *mln_iothread_msg_alloc(mln_iothread_t *t, mln_u32_t type, void *data, mln_u32_t feedback);
static inline void mln_iothread_msg_recycle(mln_iothread_t *t, mln_iothread_msg_t *msg);
MLN_CHAIN_FUNC_DECLARE(static inline, mln_iothread_msg, mln_iothread_msg_t,);
MLN_CHAIN_FUNC_DEFINE(static inline, mln_iothread_msg, mln_iothread_msg_t, prev, next);

MLN_FUNC(, int, mln_iothread_init, \
         (mln_iothread_t *t, mln_u32_t nthread, mln_iothread_entry_t entry, void *args, mln_iothread_msg_process_t handler), \
         (t, nthread, entry, args, handler), \
{
    mln_u32_t i;
    int fds[2];

    if (!nthread || entry == NULL) return -1;

    if (socketpair(AF_UNIX, SOCK_STREAM, 0, fds) < 0) return -1;

    t->io_fd = fds[0];
    t->user_fd = fds[1];
    mln_iothread_fd_nonblock_set(t->io_fd);
    mln_iothread_fd_nonblock_set(t->user_fd);
    t->entry = entry;
    t->args = args;
    t->handler = handler;
    pthread_mutex_init(&(t->io_lock), NULL);
    pthread_mutex_init(&(t->user_lock), NULL);
    pthread_mutex_init(&(t->free_lock), NULL);
    pthread_mutex_init(&(t->feedback_lock), NULL);
    pthread_cond_init(&(t->feedback_cond), NULL);
    t->io_head = t->io_tail = NULL;
    t->user_head = t->user_tail = NULL;
    t->free_head = NULL;
    t->free_count = 0;
    t->nthread = nthread;

    if ((t->tids = (pthread_t *)calloc(t->nthread, sizeof(pthread_t))) == NULL) {
        mln_socket_close(fds[0]);
        mln_socket_close(fds[1]);
        pthread_mutex_destroy(&(t->io_lock));
        pthread_mutex_destroy(&(t->user_lock));
        pthread_mutex_destroy(&(t->free_lock));
        pthread_mutex_destroy(&(t->feedback_lock));
        pthread_cond_destroy(&(t->feedback_cond));
        return -1;
    }
    for (i = 0; i < t->nthread; ++i) {
        if (pthread_create(t->tids + i, NULL, t->entry, t->args) != 0) {
            mln_iothread_destroy(t);
            return -1;
        }
    }

    return 0;
})

MLN_FUNC_VOID(, void, mln_iothread_destroy, (mln_iothread_t *t), (t), {
    mln_iothread_msg_t *msg;

    if (t == NULL) return;

    if (t->tids != NULL) {
        mln_u32_t i;
        for (i = 0; i < t->nthread; ++i) {
            pthread_cancel(t->tids[i]);
            pthread_join(t->tids[i], NULL);
        }
        free(t->tids);
    }
    mln_socket_close(t->io_fd);
    mln_socket_close(t->user_fd);

    while ((msg = t->io_head) != NULL) {
        t->io_head = msg->next;
        free(msg);
    }
    while ((msg = t->user_head) != NULL) {
        t->user_head = msg->next;
        free(msg);
    }
    while ((msg = t->free_head) != NULL) {
        t->free_head = msg->next;
        free(msg);
    }

    pthread_mutex_destroy(&(t->io_lock));
    pthread_mutex_destroy(&(t->user_lock));
    pthread_mutex_destroy(&(t->free_lock));
    pthread_mutex_destroy(&(t->feedback_lock));
    pthread_cond_destroy(&(t->feedback_cond));
})

MLN_FUNC(, int, mln_iothread_send, \
         (mln_iothread_t *t, mln_u32_t type, void *data, mln_iothread_ep_type_t to, mln_u32_t feedback), \
         (t, type, data, to, feedback), \
{
    int fd;
    pthread_mutex_t *plock;
    mln_iothread_msg_t *msg;
    mln_iothread_msg_t **head, **tail;

    if (to == io_thread) {
        fd = t->user_fd;
        plock = &(t->io_lock);
        head = &(t->io_head);
        tail = &(t->io_tail);
    } else {
        fd = t->io_fd;
        plock = &(t->user_lock);
        head = &(t->user_head);
        tail = &(t->user_tail);
    }

    if ((msg = mln_iothread_msg_alloc(t, type, data, feedback)) == NULL)
        return -1;

    pthread_mutex_lock(plock);

    mln_iothread_msg_chain_add(head, tail, msg);

    if (*head == *tail && *head == msg && send(fd, " ", 1, 0) != 1) {
        mln_iothread_msg_chain_del(head, tail, msg);
        pthread_mutex_unlock(plock);
        mln_iothread_msg_recycle(t, msg);
        return 1;
    }

    pthread_mutex_unlock(plock);

    if (feedback) {
        int __spins = 256;
        while (__spins-- > 0 && !msg->done)
            ;
        if (!msg->done) {
            pthread_mutex_lock(&(t->feedback_lock));
            while (!msg->done)
                pthread_cond_wait(&(t->feedback_cond), &(t->feedback_lock));
            pthread_mutex_unlock(&(t->feedback_lock));
        }
        mln_iothread_msg_recycle(t, msg);
    }

    return 0;
})

MLN_FUNC(, int, mln_iothread_recv, (mln_iothread_t *t, mln_iothread_ep_type_t from), (t, from), {
    int fd, n = 0;
    mln_s8_t buf[64];
    pthread_mutex_t *plock;
    mln_iothread_msg_t *msg, *batch_head;
    mln_iothread_msg_t **head, **tail;

    pthread_testcancel();

    if (from == io_thread) {
        fd = t->user_fd;
        plock = &(t->user_lock);
        head = &(t->user_head);
        tail = &(t->user_tail);
    } else {
        fd = t->io_fd;
        plock = &(t->io_lock);
        head = &(t->io_head);
        tail = &(t->io_tail);
    }

    pthread_mutex_lock(plock);

    batch_head = *head;
    if (batch_head != NULL) {
        *head = *tail = NULL;
        (void)recv(fd, buf, sizeof(buf), 0);
    }

    pthread_mutex_unlock(plock);

    while ((msg = batch_head) != NULL) {
        batch_head = msg->next;
        msg->prev = msg->next = NULL;
        if (t->handler != NULL)
            t->handler(t, from, msg);
        if (msg->feedback) {
            if (!msg->hold) {
                pthread_mutex_lock(&(t->feedback_lock));
                msg->done = 1;
                pthread_cond_broadcast(&(t->feedback_cond));
                pthread_mutex_unlock(&(t->feedback_lock));
            }
        } else {
            mln_iothread_msg_recycle(t, msg);
        }
        ++n;
    }

    return n;
})

MLN_FUNC(static inline, mln_iothread_msg_t *, mln_iothread_msg_alloc, \
         (mln_iothread_t *t, mln_u32_t type, void *data, mln_u32_t feedback), \
         (t, type, data, feedback), \
{
    mln_iothread_msg_t *msg = NULL;

    pthread_mutex_lock(&(t->free_lock));
    if (t->free_head != NULL) {
        msg = t->free_head;
        t->free_head = msg->next;
        t->free_count--;
    }
    pthread_mutex_unlock(&(t->free_lock));

    if (msg == NULL) {
        msg = (mln_iothread_msg_t *)malloc(sizeof(mln_iothread_msg_t));
        if (msg == NULL) return NULL;
    }

    msg->owner = t;
    msg->type = type;
    msg->data = data;
    msg->feedback = feedback ? 1 : 0;
    msg->hold = 0;
    msg->done = 0;
    msg->prev = msg->next = NULL;

    return msg;
})

MLN_FUNC_VOID(static inline, void, mln_iothread_msg_recycle, \
              (mln_iothread_t *t, mln_iothread_msg_t *msg), (t, msg), \
{
    if (msg == NULL) return;

    pthread_mutex_lock(&(t->free_lock));
    if (t->free_count < MLN_IOTHREAD_FREE_MAX) {
        msg->next = t->free_head;
        t->free_head = msg;
        t->free_count++;
        pthread_mutex_unlock(&(t->free_lock));
        return;
    }
    pthread_mutex_unlock(&(t->free_lock));
    free(msg);
})

MLN_FUNC_VOID(static inline, void, mln_iothread_fd_nonblock_set, (int fd), (fd), {
    int flg = fcntl(fd, F_GETFL, NULL);
    fcntl(fd, F_SETFL, flg | O_NONBLOCK);
})

#endif
