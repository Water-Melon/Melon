
/*
 * Copyright (C) Niklaus F.Schen.
 */
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <unistd.h>
#include <dlfcn.h>
#include <pthread.h>
#include "mln_thread.h"
#include "mln_hash.h"
#include "mln_conf.h"
#include "mln_log.h"
#include "mln_thread_module.h"

/*
 * global variables
 */
__thread void (*thread_cleanup)(void *) = NULL;
__thread void *thread_data = NULL;
mln_hash_t *thread_hash = NULL;
char thread_domain[] = "thread_exec";
char thread_s_restart[] = "restart";
char thread_s_default[] = "default";
char thread_start_func[] = "thread_main";
/*
 * config format:
 * [restart|default] "alias" "path" "arg1", ... ;
 */

/*
 * declarations
 */
MLN_CHAIN_FUNC_DECLARE(msg_dest, \
                       mln_thread_msgq_t, \
                       static inline void, \
                       __NONNULL3(1,2,3));
MLN_CHAIN_FUNC_DECLARE(msg_local, \
                       mln_thread_msgq_t, \
                       static inline void, \
                       __NONNULL3(1,2,3));
static mln_thread_msgq_t *
mln_thread_msgq_init(mln_thread_t *sender, mln_thread_msg_t *msg);
static void
mln_thread_msgq_destroy(mln_thread_msgq_t *tmq);
static mln_thread_t *
mln_thread_init(struct mln_thread_attr *attr) __NONNULL1(1);
static void
mln_thread_destroy(mln_event_t *ev, mln_thread_t *t);
static void
mln_thread_clear_msg_queue(mln_event_t *ev, mln_thread_t *t);
static int
mln_thread_hash_init(void);
static int
mln_thread_hash_calc(mln_hash_t *h, void *key);
static int
mln_thread_hash_cmp(mln_hash_t *h, void *k1, void *k2);
static void *
mln_thread_launcher(void *args);
static void
mln_main_thread_itc_recv_handler(mln_event_t *ev, int fd, void *data);
static void
mln_main_thread_itc_send_handler(mln_event_t *ev, int fd, void *data);
static int
mln_thread_deal_child_exit(mln_event_t *ev, mln_thread_t *t);
static void
mln_loada_thread(mln_event_t *ev, mln_conf_cmd_t *cc);


/*
 * mln_thread_t
 */
static mln_thread_t *
mln_thread_init(struct mln_thread_attr *attr)
{
    mln_thread_t *t = (mln_thread_t *)malloc(sizeof(mln_thread_t));
    if (t == NULL) return NULL;
    t->thread_main = attr->thread_main;
    t->alias = mln_new_string(attr->alias);
    if (t->alias == NULL) {
        free(t);
        return NULL;
    }
    t->argv = attr->argv;
    t->argc = attr->argc;
    t->peerfd = attr->peerfd;
    t->stype = attr->stype;
    mln_tcp_connection_init(&(t->conn), attr->sockfd);
    t->local_head = NULL;
    t->local_tail = NULL;
    t->dest_head = NULL;
    t->dest_tail = NULL;
    return t;
}

static void
mln_thread_destroy(mln_event_t *ev, mln_thread_t *t)
{
    if (t == NULL) return;
    if (t->alias != NULL)
        mln_free_string(t->alias);
    if (t->argv != NULL) {
        if (t->argv[t->argc-1] != NULL)
            free(t->argv[t->argc-1]);
        free(t->argv);
    }
    /*
     * peerfd closed before function called.
     * thread was cleaned up before function called too.
     */
    mln_tcp_connection_destroy(&(t->conn));
    mln_thread_clear_msg_queue(ev, t);
    free(t);
}

static void
mln_thread_clear_msg_queue(mln_event_t *ev, mln_thread_t *t)
{
    mln_thread_msgq_t *tmq;
    while ((tmq = t->local_head) != NULL) {
        msg_local_chain_del(&(t->local_head), &(t->local_tail), tmq);
        tmq->sender = NULL;
    }
    mln_thread_t *sender;
    mln_u32_t flag;
    while ((tmq = t->dest_head) != NULL) {
        msg_dest_chain_del(&(t->dest_head), &(t->dest_tail), tmq);
        sender = tmq->sender;
        mln_thread_clear_msg(&(tmq->msg));
        if (sender != NULL) {
            msg_local_chain_del(&(sender->local_head), &(sender->local_tail), tmq);
            flag = M_EV_RECV|M_EV_ONESHOT;
            if (sender->dest_head != NULL)
                flag |= M_EV_APPEND;
            mln_event_set_fd(ev, \
                             sender->conn.fd, \
                             flag, \
                             sender, \
                             mln_main_thread_itc_recv_handler);
        }
        mln_thread_msgq_destroy(tmq);
    }
}

/*
 * msg
 */
void mln_thread_clear_msg(mln_thread_msg_t *msg)
{
    if (msg == NULL) return;
    if (msg->dest != NULL) {
        mln_free_string(msg->dest);
        msg->dest = NULL;
    }
    if (msg->src != NULL) {
        mln_free_string(msg->src);
        msg->src = NULL;
    }
    msg->pfunc = NULL;
    if (msg->need_clear && msg->pdata != NULL)
        free(msg->pdata);
    else
        msg->pdata = NULL;
}

/*
 * load
 */
int mln_load_thread(mln_event_t *ev)
{
    if (mln_thread_hash_init() < 0) {
        mln_log(error, "No memory.\n");
        return -1;
    }
    mln_conf_t *cf = mln_get_conf();
    if (cf == NULL) {
        mln_log(error, "configuration messed up!\n");
        abort();
    }

    mln_u32_t nr_cmds = mln_get_cmd_num(cf, thread_domain);
    if (nr_cmds == 0) return 0;

    mln_conf_cmd_t **v = (mln_conf_cmd_t **)calloc(nr_cmds, sizeof(mln_conf_cmd_t *));;
    if (v == NULL) {
        mln_log(error, "No memory.\n");
        mln_hash_destroy(thread_hash, hash_none);
        return -1;
    }
    mln_get_all_cmds(cf, thread_domain, v);

    mln_u32_t i;
    for (i = 0; i < nr_cmds; i++) {
        mln_loada_thread(ev, v[i]);
    }
    free(v);
    return 0;
}

static void
mln_loada_thread(mln_event_t *ev, mln_conf_cmd_t *cc)
{
    mln_thread_t *t;
    mln_u32_t i, nr_args = 0;
    mln_conf_item_t *ci;
    int fds[2];
    struct mln_thread_attr thattr;
    if (!mln_const_strcmp(cc->cmd_name, thread_s_restart)) {
        thattr.stype = THREAD_RESTART;
    } else if (!mln_const_strcmp(cc->cmd_name, thread_s_default)) {
        thattr.stype = THREAD_DEFAULT;
    } else {
        mln_log(error, "No such command '%s' in domain '%s'.\n", \
                cc->cmd_name, thread_domain);
        return;
    }
    nr_args = mln_get_cmd_args_num(cc);
    if (nr_args < 1) {
        mln_log(error, "Invalid arguments in domain '%s'.\n", thread_domain);
        return;
    }

    thattr.argv = (char **)calloc(nr_args+2, sizeof(char *));
    if (thattr.argv == NULL) return;
    thattr.argc = nr_args+1;
    for (i = 1; i <= nr_args; i++) {
        ci = cc->search(cc, i);
        if (ci->type != CONF_STR) {
            mln_log(error, "Invalid argument type in domain '%s'.\n", thread_domain);
            free(thattr.argv);
            return;
        }
        thattr.argv[i-1] = ci->val.s->str;
    }

    thattr.alias = thattr.argv[0];
    thattr.thread_main = (tentrance)mln_get_module_entrance(thattr.alias);
    if (thattr.thread_main == NULL) {
        mln_log(error, "No such thread module named '%s'.\n", thattr.alias);
        free(thattr.argv);
        return;
    }

    if (socketpair(AF_UNIX, SOCK_STREAM, 0, fds) < 0) {
        mln_log(error, "socketpair error. %s\n", strerror(errno));
        free(thattr.argv);
        return;
    }
    thattr.sockfd = fds[0];
    thattr.peerfd = fds[1];
    t = mln_thread_init(&thattr);
    if (t == NULL) {
        mln_log(error, "No memory.\n");
        close(fds[0]); close(fds[1]);
        free(thattr.argv);
        return;
    }
    mln_log(none, "Start thread '%s'\n", t->argv[0]);
    if (mln_thread_create(t, ev) < 0) {
        close(t->peerfd);
        mln_thread_destroy(ev, t);
    }
}

/*
 * create_thread
 */
int mln_thread_create(mln_thread_t *t, mln_event_t *ev)
{
    if (t->argv[t->argc-1] == NULL) {
        char *int_str = (char *)malloc(THREAD_SOCKFD_LEN);
        if (int_str == NULL) {
            mln_log(error, "No memory.\n");
            return -1;
        }
        t->argv[t->argc-1] = int_str;
    }
    memset(t->argv[t->argc-1], 0, THREAD_SOCKFD_LEN);
    snprintf(t->argv[t->argc-1], THREAD_SOCKFD_LEN-1, "%d", t->peerfd);
    if (mln_hash_insert(thread_hash, t->alias, t) < 0) {
        mln_log(error, "No memory.\n");
        return -1;
    }
    if (mln_event_set_fd(ev, \
                         t->conn.fd, \
                         M_EV_RECV|M_EV_ONESHOT, \
                         t, \
                         mln_main_thread_itc_recv_handler) < 0)
    {
        mln_log(error, "mln_event_set_fd failed.\n");
        mln_hash_remove(thread_hash, t->alias, hash_none);
        return -1;
    }
    int err;
    if ((err = pthread_create(&(t->tid), NULL, mln_thread_launcher, t)) != 0) {
        mln_log(error, "pthread_create error. %s\n", strerror(err));
        mln_event_set_fd(ev, t->conn.fd, M_EV_CLR, NULL, NULL);
        mln_hash_remove(thread_hash, t->alias, hash_none);
        return -1;
    }
    return 0;
}

/*
 * main thread itc_handler
 */
static void
mln_main_thread_itc_recv_handler(mln_event_t *ev, int fd, void *data)
{
    mln_thread_t *t = (mln_thread_t *)data;
    if (M_C_RCV_NULL(&(t->conn))) {
        if (mln_tcp_connection_set_buf(&(t->conn), \
                                       NULL, \
                                       sizeof(mln_thread_msg_t), \
                                       M_C_RECV) < 0)
        {
            mln_log(error, "No memory.\n");
            mln_event_set_fd(ev, \
                             t->conn.fd, \
                             M_EV_RECV|M_EV_ONESHOT|M_EV_APPEND, \
                             t, \
                             mln_main_thread_itc_recv_handler);
            return;
        }
    }
    int ret = mln_tcp_connection_recv(&(t->conn));
    if (ret == M_C_CLOSED) {
        mln_log(report, "Child thread '%s' exit.\n", t->argv[0]);
        mln_thread_deal_child_exit(ev, t);
        return;
    } else if (ret == M_C_ERROR) {
        mln_log(error, "mln_tcp_connection_recv() error. %s\n", strerror(errno));
        mln_thread_deal_child_exit(ev, t);
        return;
    } else if (ret == M_C_NOTYET) {
        mln_event_set_fd(ev, \
                         t->conn.fd, \
                         M_EV_RECV|M_EV_ONESHOT|M_EV_APPEND, \
                         t, \
                         mln_main_thread_itc_recv_handler);
        return;
    }
    mln_thread_msg_t *msg = (mln_thread_msg_t *)mln_tcp_connection_get_buf(&(t->conn), M_C_RECV);
    mln_thread_msgq_t *tmq;
    tmq = mln_thread_msgq_init(t, msg);
    mln_tcp_connection_clr_buf(&(t->conn), M_C_RECV);
    if (tmq == NULL) {
        mln_log(error, "No memory.\n");
        mln_event_set_fd(ev, \
                         t->conn.fd, \
                         M_EV_RECV|M_EV_ONESHOT|M_EV_APPEND, \
                         t, \
                         mln_main_thread_itc_recv_handler);
        return;
    }
    msg = &(tmq->msg);
    msg->src = mln_dup_string(t->alias);
    if (msg->src == NULL) {
        mln_log(error, "No memory.\n");
        mln_thread_clear_msg(&(tmq->msg));
        mln_thread_msgq_destroy(tmq);
        mln_event_set_fd(ev, \
                         t->conn.fd, \
                         M_EV_RECV|M_EV_ONESHOT|M_EV_APPEND, \
                         t, \
                         mln_main_thread_itc_recv_handler);
        return;
    }

    mln_thread_t *target = (mln_thread_t *)mln_hash_search(thread_hash, msg->dest);
    if (target == NULL) {
        mln_log(report, "No such thread named '%s'.\n", msg->dest->str);
        mln_thread_clear_msg(&(tmq->msg));
        mln_thread_msgq_destroy(tmq);
        mln_event_set_fd(ev, \
                         t->conn.fd, \
                         M_EV_RECV|M_EV_ONESHOT|M_EV_APPEND, \
                         t, \
                         mln_main_thread_itc_recv_handler);
        return;
    }

    if (target->dest_head == NULL) {
        mln_event_set_fd(ev, \
                         target->conn.fd, \
                         M_EV_SEND|M_EV_ONESHOT|M_EV_APPEND, \
                         target, \
                         mln_main_thread_itc_send_handler);
    }
    msg_local_chain_add(&(t->local_head), &(t->local_tail), tmq);
    msg_dest_chain_add(&(target->dest_head), &(target->dest_tail), tmq);
}

static void
mln_main_thread_itc_send_handler(mln_event_t *ev, int fd, void *data)
{
    mln_thread_t *t = (mln_thread_t *)data;
    if (M_C_SND_NULL(&(t->conn))) {
        mln_thread_msgq_t *tmq = t->dest_head;
        if (tmq == NULL) {
            mln_log(error, "No message. Message queue messed up.\n");
            abort();
        }
        if (mln_tcp_connection_set_buf(&(t->conn), \
                                       &(tmq->msg), \
                                       sizeof(mln_thread_msg_t), \
                                       M_C_SEND) < 0)
        {
            mln_log(error, "No memory.\n");
            mln_event_set_fd(ev, \
                             t->conn.fd, \
                             M_EV_SEND|M_EV_ONESHOT|M_EV_APPEND, \
                             t, \
                             mln_main_thread_itc_send_handler);
            return;
        }
    }
    int ret = mln_tcp_connection_send(&(t->conn));
    if (ret == M_C_CLOSED) {
        mln_log(error, "Shouldn't be here!\n");
        abort();
    } else if (ret == M_C_ERROR) {
        mln_log(error, "mln_tcp_connection_send() error. %s\n", strerror(errno));
        mln_thread_deal_child_exit(ev, t);
        return;
    } else if (ret == M_C_NOTYET) {
        mln_event_set_fd(ev, \
                         t->conn.fd, \
                         M_EV_SEND|M_EV_ONESHOT|M_EV_APPEND, \
                         t, \
                         mln_main_thread_itc_send_handler);
        return;
    }
    mln_tcp_connection_clr_buf(&(t->conn), M_C_SEND);
    mln_thread_msgq_t *tmq = t->dest_head;
    msg_dest_chain_del(&(t->dest_head), &(t->dest_tail), tmq);
    mln_thread_t *sender = tmq->sender;
    if (sender != NULL) {
        msg_local_chain_del(&(sender->local_head), &(sender->local_tail), tmq);
        mln_event_set_fd(ev, \
                         sender->conn.fd, \
                         M_EV_RECV|M_EV_ONESHOT|M_EV_APPEND, \
                         sender, \
                         mln_main_thread_itc_recv_handler);
    }
    mln_thread_msgq_destroy(tmq);
    if (t->dest_head != NULL) {
        mln_event_set_fd(ev, \
                         t->conn.fd, \
                         M_EV_SEND|M_EV_ONESHOT|M_EV_APPEND, \
                         t, \
                         mln_main_thread_itc_send_handler);
    }
}

static int
mln_thread_deal_child_exit(mln_event_t *ev, mln_thread_t *t)
{
    void *tret = NULL;
    int err = pthread_join(t->tid, &tret);
    if (err != 0) {
        mln_log(error, "pthread_join error. %s\n", strerror(errno));
        abort();
    }
    intptr_t itret = (intptr_t)tret;
    mln_log(report, "child thread pthread_join's exit code: %l\n", itret);
    mln_hash_remove(thread_hash, t->alias, hash_none);
    mln_event_set_fd(ev, t->conn.fd, M_EV_CLR, NULL, NULL);
    if (t->stype == THREAD_DEFAULT) {
        mln_thread_destroy(ev, t);
        return 0;
    }
    int fds[2];
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, fds) < 0) {
        mln_log(error, "socketpair error. %s\n", strerror(errno));
        mln_thread_destroy(ev, t);
        return -1;
    }
    mln_tcp_connection_init(&(t->conn), fds[0]);
    t->peerfd = fds[1];
    mln_thread_clear_msg_queue(ev, t);
    if (mln_thread_create(t, ev) < 0) {
        close(fds[1]);
        mln_thread_destroy(ev, t);
        return -1;
    }
    return 0;
}

/*
 * thread launcher
 */
static void *
mln_thread_launcher(void *args)
{
    mln_thread_t *t = (mln_thread_t *)args;
    int ret = t->thread_main(t->argc, t->argv);
    if (thread_cleanup != NULL)
        thread_cleanup(thread_data);
    mln_log(report, "Thread '%s' return %d.\n", t->argv[0], ret);
    close(t->peerfd);
    return NULL;
}

 /*
  * hash
  */
static int
mln_thread_hash_init(void)
{
    struct mln_hash_attr hattr;
    hattr.hash = mln_thread_hash_calc;
    hattr.cmp = mln_thread_hash_cmp;
    hattr.free_key = NULL;
    hattr.free_val = NULL;
    hattr.len_base = THREAD_HASH_LEN;
    hattr.expandable = 1;
    thread_hash = mln_hash_init(&hattr);
    if (thread_hash == NULL) return -1;
    return 0;
}

static int
mln_thread_hash_calc(mln_hash_t *h, void *key)
{
    mln_string_t *str = (mln_string_t *)key;
    mln_u32_t tmp = 0;
    char *s = str->str;
    char *end = str->str + str->len;
    for(; s < end; s++) {
        tmp += (((mln_u32_t)(*s)) * 65599);
    }
    return tmp % h->len;
}

static int
mln_thread_hash_cmp(mln_hash_t *h, void *k1, void *k2)
{
    mln_string_t *str1 = (mln_string_t *)k1;
    mln_string_t *str2 = (mln_string_t *)k2;
    return !mln_strcmp(str1, str2);
}

/*
 * other apis
 */
void mln_thread_exit(int sockfd, int exit_code)
{
    close(sockfd);
    intptr_t ec = exit_code;
    pthread_exit((void *)ec);
}

void mln_thread_kill(mln_string_t *alias)
{
    mln_thread_t *t = (mln_thread_t *)mln_hash_search(thread_hash, alias);
    if (t == NULL) return;
    close(t->peerfd);
    pthread_cancel(t->tid);
}

void mln_set_cleanup(void (*tcleanup)(void *), void *data)
{
    thread_cleanup = tcleanup;
    thread_data = data;
}

/*
 * chain
 */
MLN_CHAIN_FUNC_DEFINE(msg_dest, \
                      mln_thread_msgq_t, \
                      static inline void, \
                      dest_prev, \
                      dest_next);
MLN_CHAIN_FUNC_DEFINE(msg_local, \
                      mln_thread_msgq_t, \
                      static inline void, \
                      local_prev, \
                      local_next);

static mln_thread_msgq_t *
mln_thread_msgq_init(mln_thread_t *sender, mln_thread_msg_t *msg)
{
    mln_thread_msgq_t *tmq = (mln_thread_msgq_t *)malloc(sizeof(mln_thread_msgq_t));
    if (tmq == NULL) return NULL;
    tmq->sender = sender;
    memcpy(&(tmq->msg), msg, sizeof(mln_thread_msg_t));
    return tmq;
}

static void
mln_thread_msgq_destroy(mln_thread_msgq_t *tmq)
{
    free(tmq);
}

