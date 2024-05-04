
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
static inline mln_iothread_msg_t *mln_iothread_msg_new(mln_u32_t type, void *data, int feedback);
static inline void mln_iothread_msg_free(mln_iothread_msg_t *msg);
MLN_CHAIN_FUNC_DECLARE(static inline, mln_iothread_msg, mln_iothread_msg_t,);
MLN_CHAIN_FUNC_DEFINE(static inline, mln_iothread_msg, mln_iothread_msg_t, prev, next);

MLN_FUNC(, int, mln_iothread_init, \
         (mln_iothread_t *t, mln_u32_t nthread, mln_iothread_entry_t entry, void *args, mln_iothread_msg_process_t handler), \
         (t, nthread, entry, args, handler), \
{
    mln_u32_t i;
    int fds[2];

    if (!nthread || entry == NULL) {
        return -1;
    }

    if (socketpair(AF_UNIX, SOCK_STREAM, 0, fds) < 0) {
        return -1;
    }
    t->io_fd = fds[0];
    t->user_fd = fds[1];
    mln_iothread_fd_nonblock_set(t->io_fd);
    mln_iothread_fd_nonblock_set(t->user_fd);
    t->entry = entry;
    t->args = args;
    t->handler = handler;
    pthread_mutex_init(&(t->io_lock), NULL);
    pthread_mutex_init(&(t->user_lock), NULL);
    t->io_head = t->io_tail = NULL;
    t->user_head = t->user_tail = NULL;
    t->nthread = nthread;

    if ((t->tids = (pthread_t *)calloc(t->nthread, sizeof(pthread_t))) == NULL) {
        mln_socket_close(fds[0]);
        mln_socket_close(fds[1]);
        return -1;
    }
    for (i = 0; i < t->nthread; ++i) {
        if (pthread_create(t->tids + i, NULL, t->entry, t->args) != 0) {
            mln_iothread_destroy(t);
            return -1;
        }
        if (pthread_detach(t->tids[i]) != 0) {
            mln_iothread_destroy(t);
            return -1;
        }
    }

    return 0;
})

MLN_FUNC_VOID(, void, mln_iothread_destroy, (mln_iothread_t *t), (t), {
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

    if ((msg = mln_iothread_msg_new(type, data, feedback)) == NULL)
        return -1;

    if (feedback)
        pthread_mutex_lock(&(msg->mutex));

    pthread_mutex_lock(plock);

    mln_iothread_msg_chain_add(head, tail, msg);

    if (*head == *tail && *head == msg && send(fd, " ", 1, 0) != 1) {
        mln_iothread_msg_chain_del(head, tail, msg);
        if (feedback) pthread_mutex_unlock(&(msg->mutex));
        pthread_mutex_unlock(plock);
        mln_iothread_msg_free(msg);
        return 1;
    }

    pthread_mutex_unlock(plock);

    if (feedback) {
        pthread_mutex_lock(&(msg->mutex));
        pthread_mutex_unlock(&(msg->mutex));
        mln_iothread_msg_free(msg);
    }

    return 0;
})

MLN_FUNC(, int, mln_iothread_recv, (mln_iothread_t *t, mln_iothread_ep_type_t from), (t, from), {
    int fd, n = 0;
    mln_s8_t c;
    pthread_mutex_t *plock;
    mln_iothread_msg_t *msg, *pos;
    mln_iothread_msg_t **head, **tail;

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

    pos = *head;
    while ((msg = *head) != NULL) {
        mln_iothread_msg_chain_del(head, tail, msg);
        if (t->handler != NULL)
            t->handler(t, from, msg);
        if (msg->feedback) {
            if (!msg->hold)
                pthread_mutex_unlock(&(msg->mutex));
        } else {
            mln_iothread_msg_free(msg);
        }
        ++n;
    }

    if (pos != *head)
        (void)recv(fd, &c, 1, 0);

    pthread_mutex_unlock(plock);

    return n;
})


MLN_FUNC(static inline, mln_iothread_msg_t *, mln_iothread_msg_new, \
         (mln_u32_t type, void *data, int feedback), (type, data, feedback), \
{
    mln_iothread_msg_t *msg = (mln_iothread_msg_t *)malloc(sizeof(mln_iothread_msg_t));
    if (msg == NULL)
        return NULL;

    msg->feedback = feedback;
    msg->hold = 0;
    msg->type = type;
    msg->data = data;
    msg->prev = msg->next = NULL;

    if (feedback && pthread_mutex_init(&(msg->mutex), NULL) != 0) {
        free(msg);
        return NULL;
    }
    return msg;
})

MLN_FUNC_VOID(static inline, void, mln_iothread_msg_free, (mln_iothread_msg_t *msg), (msg), {
    if (msg == NULL)
        return;

    if (msg->feedback)
        pthread_mutex_destroy(&(msg->mutex));

    free(msg);
})

MLN_FUNC_VOID(static inline, void, mln_iothread_fd_nonblock_set, (int fd), (fd), {
    int flg = fcntl(fd, F_GETFL, NULL);
    fcntl(fd, F_SETFL, flg | O_NONBLOCK);
})

#endif

