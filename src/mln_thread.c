
/*
 * Copyright (C) Niklaus F.Schen.
 */

#if !defined(MSVC)

#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <dlfcn.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include "mln_thread.h"
#include "mln_rbtree.h"
#include "mln_conf.h"
#include "mln_log.h"
#include "mln_func.h"

/*
 * global variables
 */
__thread void (*thread_cleanup)(void *) = NULL;
__thread void *thread_data = NULL;
__thread mln_thread_t *m_thread = NULL;
mln_rbtree_t *thread_tree = NULL;
char thread_domain[] = "thread_exec";
char thread_s_restart[] = "restart";
char thread_s_default[] = "default";
char thread_start_func[] = "thread_main";
/*
 * config format:
 * [restart|default] "alias" "path" "arg1", ... ;
 */
static mln_thread_module_t *module_array = NULL;
static mln_size_t module_array_num = 0;

/*
 * declarations
 */
MLN_CHAIN_FUNC_DECLARE(static inline, \
                       msg_dest, \
                       mln_thread_msgq_t, );
MLN_CHAIN_FUNC_DECLARE(static inline, \
                       msg_local, \
                       mln_thread_msgq_t, );
static mln_thread_msgq_t *
mln_thread_msgq_init(mln_thread_t *sender, mln_thread_msg_t *msg);
static void
mln_thread_msgq_destroy(mln_thread_msgq_t *tmq);
static mln_thread_t *
mln_thread_init(struct mln_thread_attr *attr) __NONNULL1(1);
static void
mln_thread_destroy(mln_thread_t *t);
static void
mln_thread_clear_msg_queue(mln_event_t *ev, mln_thread_t *t);
static int
mln_thread_rbtree_init(void);
static int
mln_thread_rbtree_cmp(const void *data1, const void *data2);
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
static inline int
mln_itc_get_buf_with_len(mln_tcp_conn_t *tc, void *buf, mln_size_t len);
static void
mln_main_thread_itc_recv_handler_process(mln_event_t *ev, mln_thread_t *t);
static inline void
mln_thread_itc_chain_release_msg(mln_chain_t *c);
static inline void *
mln_get_module_entrance(char *alias);
static inline int
__mln_thread_create(mln_thread_t *t);


/*
 * mln_thread_t
 */
MLN_FUNC(static, mln_thread_t *, mln_thread_init, (struct mln_thread_attr *attr), (attr), {
    mln_thread_t *t = (mln_thread_t *)malloc(sizeof(mln_thread_t));
    if (t == NULL) return NULL;
    t->ev = attr->ev;
    t->thread_main = attr->thread_main;
    t->alias = mln_string_new(attr->alias);
    if (t->alias == NULL) {
        free(t);
        return NULL;
    }
    t->argv = attr->argv;
    t->argc = attr->argc;
    t->peerfd = attr->peerfd;
    t->is_created = 0;
    t->stype = attr->stype;
    if (mln_tcp_conn_init(&(t->conn), attr->sockfd) < 0) {
        mln_string_free(t->alias);
        free(t);
        return NULL;
    }
    t->local_head = NULL;
    t->local_tail = NULL;
    t->dest_head = NULL;
    t->dest_tail = NULL;
    return t;
})

MLN_FUNC_VOID(static, void, mln_thread_destroy, (mln_thread_t *t), (t), {
    mln_chain_t *c;

    if (t == NULL) return;
    if (t->alias != NULL)
        mln_string_free(t->alias);
    if (t->argv != NULL) {
        if (t->argv[t->argc-1] != NULL)
            free(t->argv[t->argc-1]);
        free(t->argv);
    }
    if (t->peerfd >= 0) mln_socket_close(t->peerfd);
    if (t->is_created) {
        void *tret = NULL;
        int err = pthread_join(t->tid, &tret);
        if (err != 0) {
            mln_log(error, "pthread_join error. %s\n", strerror(errno));
            abort();
        }
        mln_log(report, "child thread pthread_join's exit code: %l\n", (intptr_t)tret);
    }
    if (mln_tcp_conn_fd_get(&(t->conn)) >= 0)
        mln_socket_close(mln_tcp_conn_fd_get(&(t->conn)));
    c = mln_tcp_conn_head(&(t->conn), M_C_SEND);
    mln_thread_itc_chain_release_msg(c);
    c = mln_tcp_conn_head(&(t->conn), M_C_RECV);
    mln_thread_itc_chain_release_msg(c);
    mln_tcp_conn_destroy(&(t->conn));
    mln_thread_clear_msg_queue(t->ev, t);
    free(t);
})

MLN_FUNC_VOID(static, void, mln_thread_clear_msg_queue, \
              (mln_event_t *ev, mln_thread_t *t), (ev, t), \
{
    mln_thread_msgq_t *tmq;
    while ((tmq = t->local_head) != NULL) {
        msg_local_chain_del(&(t->local_head), &(t->local_tail), tmq);
        tmq->sender = NULL;
    }
    mln_thread_t *sender;
    while ((tmq = t->dest_head) != NULL) {
        sender = tmq->sender;
        if (sender != NULL) {
            msg_local_chain_del(&(sender->local_head), &(sender->local_tail), tmq);
        }
        msg_dest_chain_del(&(t->dest_head), &(t->dest_tail), tmq);
        mln_thread_clear_msg(&(tmq->msg));
        mln_thread_msgq_destroy(tmq);
    }
})

/*
 * msg
 */
MLN_FUNC_VOID(, void, mln_thread_clear_msg, (mln_thread_msg_t *msg), (msg), {
    if (msg == NULL) return;
    if (msg->dest != NULL) {
        mln_string_free(msg->dest);
        msg->dest = NULL;
    }
    if (msg->src != NULL) {
        mln_string_free(msg->src);
        msg->src = NULL;
    }
    msg->pfunc = NULL;
    if (msg->need_clear && msg->pdata != NULL)
        free(msg->pdata);
    else
        msg->pdata = NULL;
})

/*
 * load
 */
MLN_FUNC(, int, mln_load_thread, (mln_event_t *ev), (ev), {
    if (mln_thread_rbtree_init() < 0) {
        mln_log(error, "No memory.\n");
        return -1;
    }
    mln_conf_t *cf = mln_conf();
    if (cf == NULL) {
        mln_log(error, "configuration messed up!\n");
        abort();
    }

    mln_u32_t nr_cmds = mln_conf_cmd_num(cf, thread_domain);
    if (nr_cmds == 0) return 0;

    mln_conf_cmd_t **v = (mln_conf_cmd_t **)calloc(nr_cmds, sizeof(mln_conf_cmd_t *));
    if (v == NULL) {
        mln_log(error, "No memory.\n");
        mln_rbtree_free(thread_tree);
        thread_tree = NULL;
        return -1;
    }
    mln_conf_cmds(cf, thread_domain, v);

    mln_u32_t i;
    for (i = 0; i < nr_cmds; ++i) {
        mln_loada_thread(ev, v[i]);
    }
    free(v);
    return 0;
})

MLN_FUNC_VOID(static, void, mln_loada_thread, (mln_event_t *ev, mln_conf_cmd_t *cc), (ev, cc), {
    mln_thread_t *t;
    mln_u32_t i, nr_args = 0;
    mln_conf_item_t *ci;
    int fds[2];
    struct mln_thread_attr thattr;
    if (!mln_string_const_strcmp(cc->cmd_name, thread_s_restart)) {
        thattr.stype = THREAD_RESTART;
    } else if (!mln_string_const_strcmp(cc->cmd_name, thread_s_default)) {
        thattr.stype = THREAD_DEFAULT;
    } else {
        mln_log(error, "No such command '%s' in domain '%s'.\n", \
                cc->cmd_name, thread_domain);
        return;
    }
    nr_args = mln_conf_arg_num(cc);
    if (nr_args < 1) {
        mln_log(error, "Invalid arguments in domain '%s'.\n", thread_domain);
        return;
    }

    thattr.ev = ev;
    thattr.argv = (char **)calloc(nr_args+2, sizeof(char *));
    if (thattr.argv == NULL) return;
    thattr.argc = nr_args+1;
    for (i = 1; i <= nr_args; ++i) {
        ci = cc->search(cc, i);
        if (ci->type != CONF_STR) {
            mln_log(error, "Invalid argument type in domain '%s'.\n", thread_domain);
            free(thattr.argv);
            return;
        }
        thattr.argv[i-1] = (char *)(ci->val.s->data);
    }

    thattr.alias = thattr.argv[0];
    thattr.thread_main = (mln_thread_entrance_t)mln_get_module_entrance(thattr.alias);
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
        mln_socket_close(fds[0]); mln_socket_close(fds[1]);
        free(thattr.argv);
        return;
    }
    mln_log(none, "Start thread '%s'\n", t->argv[0]);
    if (__mln_thread_create(t) < 0) {
        mln_thread_destroy(t);
    }
})

MLN_FUNC(, int, mln_thread_create, \
         (mln_event_t *ev, char *alias, mln_thread_stype_t stype, \
          mln_thread_entrance_t entrance, int argc, char *argv[]), \
         (ev, alias, stype, entrance, argc, argv), \
{
    mln_thread_t *t;
    int fds[2];
    struct mln_thread_attr thattr;

    if (socketpair(AF_UNIX, SOCK_STREAM, 0, fds) < 0) {
        return -1;
    }

    thattr.ev = ev;
    thattr.stype = stype;
    thattr.argv = argv;
    thattr.argc = argc + 1;
    thattr.alias = alias;
    thattr.thread_main = entrance;
    thattr.sockfd = fds[0];
    thattr.peerfd = fds[1];
    t = mln_thread_init(&thattr);
    if (t == NULL) {
        mln_socket_close(fds[0]);
        mln_socket_close(fds[1]);
        return -1;
    }
    if (__mln_thread_create(t) < 0) {
        mln_thread_destroy(t);
        return -1;
    }

    return 0;
})

MLN_FUNC(static, void *, mln_get_module_entrance, (char *alias), (alias), {
    mln_thread_module_t *tm = module_array, *end = module_array + module_array_num;
    for (; tm < end; ++tm) {
        if (!strcmp(alias, tm->alias)) return tm->thread_main;
    }
    return NULL;
})

/*
 * create_thread
 */
MLN_FUNC(static inline, int, __mln_thread_create, (mln_thread_t *t), (t), {
    mln_rbtree_node_t *rn;
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
    if ((rn = mln_rbtree_node_new(thread_tree, t)) == NULL) {
        mln_log(error, "No memory.\n");
        return -1;
    }
    mln_rbtree_insert(thread_tree, rn);
    if (mln_event_fd_set(t->ev, \
                         mln_tcp_conn_fd_get(&(t->conn)), \
                         M_EV_RECV|M_EV_NONBLOCK, \
                         M_EV_UNLIMITED, \
                         t, \
                         mln_main_thread_itc_recv_handler) < 0)
    {
        mln_log(error, "mln_event_fd_set failed.\n");
        mln_rbtree_delete(thread_tree, rn);
        mln_rbtree_node_free(thread_tree, rn);
        t->node = NULL;
        return -1;
    }
    int err;
    if ((err = pthread_create(&(t->tid), NULL, mln_thread_launcher, t)) != 0) {
        mln_log(error, "pthread_create error. %s\n", strerror(err));
        mln_event_fd_set(t->ev, \
                         mln_tcp_conn_fd_get(&(t->conn)), \
                         M_EV_CLR, \
                         M_EV_UNLIMITED, \
                         NULL, \
                         NULL);
        mln_rbtree_delete(thread_tree, rn);
        mln_rbtree_node_free(thread_tree, rn);
        t->node = NULL;
        return -1;
    }
    t->is_created = 1;
    t->node = rn;
    return 0;
})

MLN_FUNC(static inline, int, mln_itc_get_buf_with_len, \
         (mln_tcp_conn_t *tc, void *buf, mln_size_t len), (tc, buf, len), \
{
    mln_size_t size = 0;
    mln_chain_t *c;
    mln_buf_t *b;
    mln_u8ptr_t pos;

    c = mln_tcp_conn_head(tc, M_C_RECV);
    for (; c != NULL; c = c->next) {
        if (c->buf == NULL || c->buf->pos == NULL) continue;
        size += mln_buf_left_size(c->buf);
        if (size >= len) break;
    }
    if (c == NULL) return -1;

    pos = buf;
    while ((c = mln_tcp_conn_head(tc, M_C_RECV)) != NULL) {
        b = c->buf;
        if (b == NULL || b->pos == NULL) {
            mln_chain_pool_release(mln_tcp_conn_pop(tc, M_C_RECV));
            continue;
        }
        size = mln_buf_left_size(b);
        if (size > len) {
            memcpy(pos, b->left_pos, len);
            b->left_pos += len;
            break;
        }
        memcpy(pos, b->left_pos, size);
        pos += size;
        len -= size;
        b->left_pos += size;
        mln_chain_pool_release(mln_tcp_conn_pop(tc, M_C_RECV));
        if (len == 0) break;
    }

    return 0;
})

MLN_FUNC_VOID(static inline, void, mln_thread_itc_chain_release_msg, (mln_chain_t *c), (c), {
    mln_buf_t *b;

    for (; c != NULL; c = c->next) {
        if ((b = c->buf)== NULL) continue;
        mln_thread_clear_msg((mln_thread_msg_t *)(b->pos));
    }
})

/*
 * main thread itc_handler
 */
MLN_FUNC_VOID(static, void, mln_main_thread_itc_recv_handler, \
              (mln_event_t *ev, int fd, void *data), (ev, fd, data), \
{
    mln_thread_t *t = (mln_thread_t *)data;
    mln_tcp_conn_t *conn = &(t->conn);
    int ret;

    while (1) {
        ret = mln_tcp_conn_recv(conn, M_C_TYPE_MEMORY);
        if (ret == M_C_FINISH) {
            continue;
        } else if (ret == M_C_NOTYET) {
            break;
        } else if (ret == M_C_CLOSED) {
            mln_log(report, "Child thread '%s' exit.\n", t->argv[0]);
            mln_thread_deal_child_exit(ev, t);
            return;
        } else {
            mln_log(error, "mln_tcp_conn_recv() error. %s\n", strerror(errno));
            mln_thread_deal_child_exit(ev, t);
            return;
        }
    }

    mln_main_thread_itc_recv_handler_process(ev, t);
})

MLN_FUNC_VOID(static, void, mln_main_thread_itc_recv_handler_process, \
              (mln_event_t *ev, mln_thread_t *t), (ev, t), \
{
    mln_tcp_conn_t *conn = &(t->conn);
    mln_thread_msg_t msg, *m;
    mln_thread_msgq_t *tmq;
    mln_thread_t *target, tmp;
    mln_rbtree_node_t *rn;

    while (1) {
        if (mln_itc_get_buf_with_len(conn, &msg, sizeof(msg)) < 0) {
            return;
        }

        tmq = mln_thread_msgq_init(t, &msg);
        if (tmq == NULL) {
            mln_log(error, "No memory.\n");
            continue;
        }

        m = &(tmq->msg);
        m->src = mln_string_dup(t->alias);
        if (m->src == NULL) {
            mln_log(error, "No memory.\n");
            mln_thread_clear_msg(&(tmq->msg));
            mln_thread_msgq_destroy(tmq);
            continue;
        }

        tmp.alias = m->dest;
        rn = mln_rbtree_search(thread_tree, &tmp);
        if (mln_rbtree_null(rn, thread_tree)) {
            mln_log(report, "No such thread named '%s'.\n", (char *)(m->dest->data));
            mln_thread_clear_msg(&(tmq->msg));
            mln_thread_msgq_destroy(tmq);
            continue;
        }

        target = (mln_thread_t *)mln_rbtree_node_data_get(rn);
        if (target->dest_head == NULL) {
            mln_event_fd_set(ev, \
                             mln_tcp_conn_fd_get(&(target->conn)), \
                             M_EV_SEND|M_EV_ONESHOT|M_EV_NONBLOCK|M_EV_APPEND, \
                             M_EV_UNLIMITED, \
                             target, \
                             mln_main_thread_itc_send_handler);
        }

        msg_local_chain_add(&(t->local_head), &(t->local_tail), tmq);
        msg_dest_chain_add(&(target->dest_head), &(target->dest_tail), tmq);
    }
})

MLN_FUNC_VOID(static, void, mln_main_thread_itc_send_handler, \
              (mln_event_t *ev, int fd, void *data), (ev, fd, data), \
{
    mln_thread_t *t = (mln_thread_t *)data;
    mln_thread_msgq_t *tmq;
    mln_tcp_conn_t *conn = &(t->conn);
    mln_alloc_t *pool = mln_tcp_conn_pool_get(conn);
    mln_chain_t *c;
    mln_buf_t *b;
    mln_u8ptr_t buf;
    int ret;

again:
    while ((c = mln_tcp_conn_head(conn, M_C_SEND)) != NULL) {
        ret = mln_tcp_conn_send(conn);
        if (ret == M_C_FINISH) {
            continue;
        } else if (ret == M_C_NOTYET) {
            mln_chain_pool_release_all(mln_tcp_conn_remove(conn, M_C_SENT));
            mln_event_fd_set(ev, \
                             mln_tcp_conn_fd_get(&(t->conn)), \
                             M_EV_SEND|M_EV_ONESHOT|M_EV_NONBLOCK|M_EV_APPEND, \
                             M_EV_UNLIMITED, \
                             t, \
                             mln_main_thread_itc_send_handler);
            return;
        } else if (ret == M_C_ERROR) {
            mln_log(error, "mln_tcp_conn_send() error. %s\n", strerror(errno));
            mln_thread_deal_child_exit(ev, t);
            return;
        } else {
            mln_log(error, "Shouldn't be here.\n");
            abort();
        }
    }

    mln_chain_pool_release_all(mln_tcp_conn_remove(conn, M_C_SENT));

    while ((tmq = t->dest_head) != NULL) {
        msg_dest_chain_del(&(t->dest_head), &(t->dest_tail), tmq);
        if (tmq->sender != NULL)
            msg_local_chain_del(&(tmq->sender->local_head), &(tmq->sender->local_tail), tmq);

        c = mln_chain_new(pool);
        if (c == NULL) {
            mln_log(error, "No memory.\n");
            mln_thread_clear_msg(&(tmq->msg));
            mln_thread_msgq_destroy(tmq);
            continue;
        }
        b = mln_buf_new(pool);
        if (b == NULL) {
            mln_log(error, "No memory.\n");
            mln_thread_clear_msg(&(tmq->msg));
            mln_thread_msgq_destroy(tmq);
            mln_chain_pool_release(c);
            continue;
        }
        c->buf = b;
        buf = (mln_u8ptr_t)mln_alloc_m(pool, sizeof(mln_thread_msg_t));
        if (buf == NULL) {
            mln_log(error, "No memory.\n");
            mln_thread_clear_msg(&(tmq->msg));
            mln_thread_msgq_destroy(tmq);
            mln_chain_pool_release(c);
            continue;
        }

        memcpy(buf, &(tmq->msg), sizeof(mln_thread_msg_t));
        mln_thread_msgq_destroy(tmq);
        b->left_pos = b->pos = b->start = buf;
        b->last = b->end = buf + sizeof(mln_thread_msg_t);
        b->in_memory = 1;
        b->last_buf = 1;
        b->last_in_chain = 1;

        mln_tcp_conn_append(conn, c, M_C_SEND);

        goto again;
    }
})

MLN_FUNC(static, int, mln_thread_deal_child_exit, \
         (mln_event_t *ev, mln_thread_t *t), (ev, t), \
{
    mln_chain_t *c;

    mln_rbtree_delete(thread_tree, t->node);
    mln_rbtree_node_free(thread_tree, t->node);
    t->node = NULL;
    mln_event_fd_set(ev, \
                     mln_tcp_conn_fd_get(&(t->conn)), \
                     M_EV_CLR, \
                     M_EV_UNLIMITED, \
                     NULL, \
                     NULL);

    if (t->stype == THREAD_DEFAULT) {
        mln_thread_destroy(t);
        return 0;
    }

    void *tret = NULL;
    int err = pthread_join(t->tid, &tret);
    if (err != 0) {
        mln_log(error, "pthread_join error. %s\n", strerror(errno));
        abort();
    }
    mln_log(report, "child thread pthread_join's exit code: %l\n", (intptr_t)tret);

    t->is_created = 0;

    if (t->argv != NULL && t->argv[t->argc-1] != NULL) {
        free(t->argv[t->argc-1]);
        t->argv[t->argc-1] = NULL;
    }

    mln_socket_close(mln_tcp_conn_fd_get(&(t->conn)));
    c = mln_tcp_conn_remove(&(t->conn), M_C_SEND);
    mln_thread_itc_chain_release_msg(c);
    mln_chain_pool_release_all(c);
    c = mln_tcp_conn_remove(&(t->conn), M_C_RECV);
    mln_thread_itc_chain_release_msg(c);
    mln_chain_pool_release_all(c);
    mln_chain_pool_release_all(mln_tcp_conn_remove(&(t->conn), M_C_SENT));

    mln_thread_clear_msg_queue(ev, t);

    int fds[2];
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, fds) < 0) {
        mln_log(error, "socketpair error. %s\n", strerror(errno));
        mln_thread_destroy(t);
        return -1;
    }
    mln_tcp_conn_fd_set(&(t->conn), fds[0]);
    t->peerfd = fds[1];
    if (__mln_thread_create(t) < 0) {
        mln_thread_destroy(t);
        return -1;
    }
    return 0;
})

/*
 * thread launcher
 */
MLN_FUNC(static, void *, mln_thread_launcher, (void *args), (args), {
    mln_thread_t *t = (mln_thread_t *)args;
    m_thread = t;
    int ret = t->thread_main(t->argc, t->argv);
    if (thread_cleanup != NULL)
        thread_cleanup(thread_data);
    mln_log(report, "Thread '%s' return %d.\n", t->argv[0], ret);
    mln_socket_close(t->peerfd);
    t->peerfd = -1;
    return NULL;
})

 /*
  * hash
  */
MLN_FUNC(static, int, mln_thread_rbtree_init, (void), (), {
    struct mln_rbtree_attr rbattr;
    rbattr.pool = NULL;
    rbattr.pool_alloc = NULL;
    rbattr.pool_free = NULL;
    rbattr.cmp = mln_thread_rbtree_cmp;
    rbattr.data_free = NULL;
    if ((thread_tree = mln_rbtree_new(&rbattr)) == NULL) {
        return -1;
    }
    return 0;
})

MLN_FUNC(static, int, mln_thread_rbtree_cmp, \
         (const void *data1, const void *data2), (data1, data2), \
{
    mln_thread_t *t1 = (mln_thread_t *)data1;
    mln_thread_t *t2 = (mln_thread_t *)data2;
    return mln_string_strcmp(t1->alias, t2->alias);
})

/*
 * other apis
 */
MLN_FUNC_VOID(, void, mln_thread_module_set, \
              (mln_thread_module_t *modules, mln_size_t num), (modules, num), \
{
    module_array = modules;
    module_array_num = num;
})

MLN_FUNC_VOID(, void, mln_thread_exit, (int exit_code), (exit_code), {
    mln_socket_close(m_thread->peerfd);
    m_thread->peerfd = -1;
    intptr_t ec = exit_code;
    pthread_exit((void *)ec);
})

MLN_FUNC_VOID(, void, mln_thread_kill, (mln_string_t *alias), (alias), {
    mln_thread_t *t, tmp;
    mln_rbtree_node_t *rn;
    tmp.alias = alias;
    rn = mln_rbtree_search(thread_tree, &tmp);
    if (mln_rbtree_null(rn, thread_tree)) return;
    t = (mln_thread_t *)mln_rbtree_node_data_get(rn);
    mln_socket_close(t->peerfd);
    t->peerfd = -1;
    pthread_cancel(t->tid);
})

MLN_FUNC_VOID(, void, mln_thread_cleanup_set, (void (*tcleanup)(void *), void *data), (tcleanup, data), {
    thread_cleanup = tcleanup;
    thread_data = data;
})

/*
 * chain
 */
MLN_CHAIN_FUNC_DEFINE(static inline, \
                      msg_dest, \
                      mln_thread_msgq_t, \
                      dest_prev, \
                      dest_next);
MLN_CHAIN_FUNC_DEFINE(static inline, \
                      msg_local, \
                      mln_thread_msgq_t, \
                      local_prev, \
                      local_next);

MLN_FUNC(static, mln_thread_msgq_t *, mln_thread_msgq_init, \
         (mln_thread_t *sender, mln_thread_msg_t *msg), (sender, msg), \
{
    mln_thread_msgq_t *tmq = (mln_thread_msgq_t *)malloc(sizeof(mln_thread_msgq_t));
    if (tmq == NULL) return NULL;
    tmq->sender = sender;
    memcpy(&(tmq->msg), msg, sizeof(mln_thread_msg_t));
    return tmq;
})

MLN_FUNC_VOID(static, void, mln_thread_msgq_destroy, (mln_thread_msgq_t *tmq), (tmq), {
    free(tmq);
})

#endif

