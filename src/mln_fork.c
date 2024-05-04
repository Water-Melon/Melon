
/*
 * Copyright (C) Niklaus F.Schen.
 */
#if !defined(MSVC)
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include "mln_utils.h"
#include "mln_fork.h"
#include "mln_rbtree.h"
#include "mln_log.h"
#include "mln_conf.h"
#include "mln_ipc.h"
#include "mln_alloc.h"
#include "mln_func.h"
#include <sys/ioctl.h>

mln_tcp_conn_t master_conn;
mln_size_t child_error_bytes;
mln_u32_t child_state;
mln_u32_t cur_msg_len;
mln_u8ptr_t child_msg_content;
mln_u32_t cur_msg_type;
mln_fork_t *worker_list_head = NULL;
mln_fork_t *worker_list_tail = NULL;
mln_rbtree_t *master_ipc_tree = NULL;
mln_rbtree_t *worker_ipc_tree = NULL;
clr_handler rs_clr_handler = NULL;
void *rs_clr_data = NULL;

MLN_CHAIN_FUNC_DECLARE(static inline, \
                       worker_list, \
                       mln_fork_t, );
static int
mln_fork_rbtree_cmp(const void *k1, const void *k2) __NONNULL2(1,2);
static int
do_fork_worker_process(mln_sauto_t n_worker_proc);
static int
do_fork_core(enum proc_exec_type etype, \
             enum proc_state_type stype, \
             mln_s8ptr_t *args, \
             mln_u32_t n_args, \
             mln_event_t *master_ev);
static mln_fork_t *
mln_fork_init(struct mln_fork_attr *attr) __NONNULL1(1);
static void
mln_fork_destroy(mln_fork_t *f, int free_args);
static void
mln_fork_destroy_all(void);
static void
mln_ipc_fd_handler_master_process(mln_event_t *ev, mln_fork_t *f);
static inline int
mln_ipc_get_buf_with_len(mln_tcp_conn_t *tc, void *buf, mln_size_t len);
static inline mln_size_t
mln_ipc_discard_bytes(mln_tcp_conn_t *tc, mln_size_t size);
static void
mln_ipc_fd_handler_worker_process(mln_event_t *ev, mln_tcp_conn_t *tc);
static void
mln_ipc_fd_handler_master_send(mln_event_t *ev, int fd, void *data);
static void
mln_ipc_fd_handler_worker_send(mln_event_t *ev, int fd, void *data);
static inline mln_ipc_handler_t *mln_ipc_handler_new(mln_u32_t type, ipc_handler handler, void *data);
static void mln_ipc_handler_free(mln_ipc_handler_t *ih);

/*pre-fork*/
MLN_FUNC(, int, mln_fork_prepare, (void), (), {
    if (signal(SIGCHLD, SIG_IGN) == SIG_ERR) {
        mln_log(error, "signal() to ignore SIGCHLD failed, %s\n", strerror(errno));
        return -1;
    }
    if (signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
        mln_log(error, "signal() to ignore SIGPIPE failed, %s\n", strerror(errno));
        return -1;
    }
    if (mln_tcp_conn_init(&master_conn, -1) < 0) {
        mln_log(error, "No memory.\n");
        return -1;
    }
    child_state = STATE_IDLE;
    child_error_bytes = 0;
    cur_msg_len = 0;
    child_msg_content = NULL;
    cur_msg_type = 0;
    struct mln_rbtree_attr rbattr;
    rbattr.pool = NULL;
    rbattr.pool_alloc = NULL;
    rbattr.pool_free = NULL;
    rbattr.cmp = mln_fork_rbtree_cmp;
    rbattr.data_free = (rbtree_free_data)mln_ipc_handler_free;
    if ((master_ipc_tree = mln_rbtree_new(&rbattr)) == NULL) {
        mln_log(error, "No memory.\n");
        if (mln_tcp_conn_fd_get(&master_conn) >= 0)
            mln_socket_close(mln_tcp_conn_fd_get(&master_conn));
        mln_tcp_conn_destroy(&master_conn);
        return -1;
    }
    if ((worker_ipc_tree = mln_rbtree_new(&rbattr)) == NULL) {
        mln_log(error, "No memory.\n");
        mln_rbtree_free(master_ipc_tree);
        master_ipc_tree = NULL;
        if (mln_tcp_conn_fd_get(&master_conn) >= 0)
            mln_socket_close(mln_tcp_conn_fd_get(&master_conn));
        mln_tcp_conn_destroy(&master_conn);
        return -1;
    }
    if (mln_ipc_set_process_handlers() < 0) {
        mln_log(error, "No memory.\n");
        mln_rbtree_free(worker_ipc_tree);
        worker_ipc_tree = NULL;
        mln_rbtree_free(master_ipc_tree);
        master_ipc_tree = NULL;
        if (mln_tcp_conn_fd_get(&master_conn) >= 0)
            mln_socket_close(mln_tcp_conn_fd_get(&master_conn));
        mln_tcp_conn_destroy(&master_conn);
        return -1;
    }
    return 0;
})

MLN_FUNC(static, int, mln_fork_rbtree_cmp, (const void *k1, const void *k2), (k1, k2), {
    mln_ipc_handler_t *ih1 = (mln_ipc_handler_t *)k1;
    mln_ipc_handler_t *ih2 = (mln_ipc_handler_t *)k2;
    if (ih1->type > ih2->type) return 1;
    else if (ih1->type == ih2->type) return 0;
    return -1;
})

/*mln_fork_t*/
MLN_FUNC(static, mln_fork_t *, mln_fork_init, (struct mln_fork_attr *attr), (attr), {
    mln_fork_t *f = (mln_fork_t *)malloc(sizeof(mln_fork_t));
    if (f == NULL) return NULL;
    f->prev = NULL;
    f->next = NULL;
    f->args = attr->args;
    if (mln_tcp_conn_init(&(f->conn), attr->fd) < 0) {
        free(f);
        return NULL;
    }
    f->pid = attr->pid;
    f->n_args = attr->n_args;
    f->state = STATE_IDLE;
    f->msg_len = 0;
    f->msg_type = 0;
    f->error_bytes = 0;
    f->msg_content = NULL;
    f->etype = attr->etype;
    f->stype = attr->stype;
    worker_list_chain_add(&worker_list_head, &worker_list_tail, f);
    return f;
})

MLN_FUNC_VOID(static, void, mln_fork_destroy, (mln_fork_t *f, int free_args), (f, free_args), {
    if (f == NULL) return;
    if (f->args != NULL && free_args) {
        free(f->args);
        f->args = NULL;
    }
    if (f->msg_content != NULL) {
        free(f->msg_content);
    }
    if (mln_tcp_conn_fd_get(&(f->conn)) >= 0)
        mln_socket_close(mln_tcp_conn_fd_get(&(f->conn)));
    mln_tcp_conn_destroy(&(f->conn));
    worker_list_chain_del(&worker_list_head, &worker_list_tail, f);
    free(f);
})

MLN_FUNC_VOID(static, void, mln_fork_destroy_all, (void), (), {
    mln_fork_t *f;
    while ((f = worker_list_head) != NULL) {
        mln_fork_destroy(f, 1);
    }
})

/*
 * fork processes
 */
MLN_FUNC(, int, mln_fork, (void), (), {
    mln_conf_t *cf = mln_conf();
    if (cf == NULL) {
        mln_log(error, "configuration crashed.\n");
        abort();
    }
    mln_conf_domain_t *cd = cf->search(cf, "main");
    if (cd == NULL) {
        mln_log(error, "Domain 'main' NOT existed.\n");
        abort();
    }
    mln_sauto_t n_worker_proc = 0;
    mln_conf_cmd_t *cmd = cd->search(cd, "worker_proc");
    if (cmd != NULL) {
        if (mln_conf_arg_num(cmd) > 1) {
            mln_log(error, "Too many arguments follow 'worker_proc'.\n");
            exit(1);
        }
        mln_conf_item_t *ci = cmd->search(cmd, 1);
        if (ci == NULL) {
            mln_log(error, "'worker_proc' need an integer argument.\n");
            exit(1);
        }
        if (ci->type != CONF_INT) {
            mln_log(error, "'worker_proc' need an integer argument.\n");
            exit(1);
        }
        n_worker_proc = ci->val.i;
        if (n_worker_proc < 0) {
            mln_log(error, "Invalid value to 'worker_process'.\n");
            exit(1);
        }
    }
    if (!do_fork_worker_process(n_worker_proc)) return 0;

    mln_conf_cmd_t **v, **cc;
    mln_u32_t i, n_args;
    mln_conf_item_t *arg_ci;
    mln_s8ptr_t *v_args;
    mln_u32_t n = mln_conf_cmd_num(cf, "proc_exec");
    if (n == 0) return 1;

    v = (mln_conf_cmd_t **)calloc(n+1, sizeof(mln_conf_cmd_t *));
    if (v == NULL) {
        mln_log(error, "No memory.\n");
        return -1;
    }
    mln_conf_cmds(cf, "proc_exec", v);
    for (cc = v; *cc != NULL; ++cc) {
        n_args = mln_conf_arg_num(*cc);
        if (n_args == 0) {
            mln_log(error, "Demand arguments in 'proc_exec'.\n");
            exit(1);
        }
        v_args = (mln_s8ptr_t *)calloc(n_args+2, sizeof(mln_s8ptr_t));
        if (v_args == NULL) {
            mln_log(error, "No memory.\n");
            continue;
        }
        for (i = 0; i < n_args; ++i) {
            arg_ci = (*cc)->search(*cc, i+1);
            if (arg_ci->type != CONF_STR) {
                mln_log(error, "Demand string arguments in 'proc_exec'.\n");
                exit(1);
            }
            v_args[i] = (char *)(arg_ci->val.s->data);
        }
        if (!mln_string_const_strcmp((*cc)->cmd_name, "keepalive")) {
            mln_fork_spawn(M_PST_SUP, v_args, n_args, NULL);
        } else if (!mln_string_const_strcmp((*cc)->cmd_name, "default")) {
            mln_fork_spawn(M_PST_DFL, v_args, n_args, NULL);
        } else {
            mln_log(error, "Invalid command '%S' in 'proc_exec'.\n", (*cc)->cmd_name);
            exit(1);
        }
    }
    free(v);
    return 1;
})

MLN_FUNC(, int, mln_fork_spawn, \
         (enum proc_state_type stype, mln_s8ptr_t *args, mln_u32_t n_args, mln_event_t *master_ev), \
         (stype, args, n_args, master_ev), \
{
    mln_log(none, "Start up process '%s'\n", args[0]);
    int ret = do_fork_core(M_PET_EXE, \
                           stype, \
                           args, \
                           n_args, \
                           master_ev);
    if (ret < 0) {
        return -1;
    } else if (ret == 0) {
        char fd_str[256] = {0};
        snprintf(fd_str, sizeof(fd_str)-1, "%d", \
                 mln_tcp_conn_fd_get(&master_conn));
        args[n_args] = fd_str;
        if (master_ev != NULL) mln_event_free(master_ev);
        mln_log_destroy();
        if (execv(args[0], args) < 0) {
            mln_log(error, "execv error, %s\n", strerror(errno));
            exit(1);
        }
    }
    return 0;
})

MLN_FUNC(, int, mln_fork_restart, (mln_event_t *master_ev), (master_ev), {
    return do_fork_core(M_PET_DFL, \
                        M_PST_SUP, \
                        NULL, \
                        0, \
                        master_ev);
})

MLN_FUNC(static, int, do_fork_worker_process, (mln_sauto_t n_worker_proc), (n_worker_proc), {
    mln_sauto_t i;
    int ret;
    for (i = 0; i < n_worker_proc; ++i) {
        mln_log(none, "Start up worker process No.%l\n", i+1);
        if ((ret = mln_fork_restart(NULL)) < 0) {
            continue;
        } else if (ret == 0) {
            return 0;
        }
    }
    return 1;
})

MLN_FUNC_VOID(, void, mln_fork_resource_clear_handler_set, (clr_handler handler, void *data), (handler, data), {
    rs_clr_handler = handler;
    rs_clr_data = data;
})

MLN_FUNC(static, int, do_fork_core, \
         (enum proc_exec_type etype, enum proc_state_type stype, \
          mln_s8ptr_t *args, mln_u32_t n_args, mln_event_t *master_ev), \
         (etype, stype, args, n_args, master_ev), \
{
    int fds[2];
    mln_u8_t c;

    if (socketpair(AF_UNIX, SOCK_STREAM, 0, fds) < 0) {
        mln_log(error, "socketpair() error. %s\n", strerror(errno));
        return -1;
    }

    pid_t pid = fork();
    if (pid > 0) {
        mln_socket_close(fds[1]);
        /*
         * In linux 2.6.32-279, there is a loophole in process restart.
         * If you use select() or kqueue(), you wouldn't get this problem.
         * If I don't add a signal to make parent process wait for child,
         * the event which combined with the fd that
         * created by socketpair() and used to receive child process messages
         * would be tiggered by epoll_wait() with an empty receive buffer.
         * If fd is blocking model, the parent routine would be blocked
         * in read(). Because there is a moment that child process not be built yet,
         * but parent process have already added the fd into the epoll and
         * jump into the mln_event_dispatch().
         * And then, child process built, that would make the event
         * which is triggered in parent process to be a lie.
         */
        while (read(fds[0], &c, 1) <= 0)
            ;

        struct mln_fork_attr fattr;
        fattr.args = args;
        fattr.n_args = n_args;
        fattr.fd = fds[0];
        fattr.pid = pid;
        fattr.etype = etype;
        fattr.stype = stype;
        mln_fork_t *f = mln_fork_init(&fattr);
        if (f == NULL) {
            mln_log(error, "No memory.\n");
            abort();
        }
        if (master_ev != NULL) {
            if (mln_event_fd_set(master_ev, \
                                 mln_tcp_conn_fd_get(&(f->conn)), \
                                 M_EV_RECV, \
                                 M_EV_UNLIMITED, \
                                 f, \
                                 mln_ipc_fd_handler_master) < 0)
            {
                mln_log(error, "mln_event_fd_set() failed.\n");
                abort();
            }
        }
        return 1;
    } else if (pid == 0) {
        mln_socket_close(fds[0]);
        mln_fork_destroy_all();
        mln_rbtree_free(master_ipc_tree);
        if (rs_clr_handler != NULL)
            rs_clr_handler(rs_clr_data);
        master_ipc_tree = NULL;
        mln_tcp_conn_fd_set(&master_conn, fds[1]);
        signal(SIGCHLD, SIG_DFL);
        if (write(fds[1], " ", 1) < 0)
            exit(1);
        return 0;
    }
    mln_log(error, "fork() error. %s\n", strerror(errno));
    return -1;
})

/*mln_set_master_ipc_handler*/
MLN_FUNC(, int, mln_fork_master_ipc_handler_set, \
         (mln_u32_t type, ipc_handler handler, void *data), \
         (type, handler, data), \
{
    mln_ipc_handler_t *ih = mln_ipc_handler_new(type, handler, data);
    if (ih == NULL) return -1;

    mln_rbtree_node_t *rn = mln_rbtree_search(master_ipc_tree, ih);
    if (mln_rbtree_node_data_get(rn) != NULL) {
        mln_rbtree_delete(master_ipc_tree, rn);
        mln_rbtree_node_free(master_ipc_tree, rn);
    }
    rn = mln_rbtree_node_new(master_ipc_tree, ih);
    if (rn == NULL) {
        mln_ipc_handler_free(ih);
        return -1;
    }
    mln_rbtree_insert(master_ipc_tree, rn);
    return 0;
})
/*mln_set_worker_ipc_handler*/
MLN_FUNC(, int, mln_fork_worker_ipc_handler_set, \
         (mln_u32_t type, ipc_handler handler, void *data), \
         (type, handler, data), \
{
    mln_ipc_handler_t *ih = mln_ipc_handler_new(type, handler, data);
    if (ih == NULL) return -1;

    mln_rbtree_node_t *rn = mln_rbtree_search(worker_ipc_tree, ih);
    if (mln_rbtree_node_data_get(rn) != NULL) {
        mln_rbtree_delete(worker_ipc_tree, rn);
        mln_rbtree_node_free(worker_ipc_tree, rn);
    }
    rn = mln_rbtree_node_new(worker_ipc_tree, ih);
    if (rn == NULL) {
        mln_ipc_handler_free(ih);
        return -1;
    }
    mln_rbtree_insert(worker_ipc_tree, rn);
    return 0;
})

MLN_FUNC(static inline, mln_ipc_handler_t *, mln_ipc_handler_new, \
         (mln_u32_t type, ipc_handler handler, void *data), (type, handler, data), \
{
    mln_ipc_handler_t *ih;
    if ((ih = (mln_ipc_handler_t *)malloc(sizeof(mln_ipc_handler_t))) == NULL) {
        return NULL;
    }
    ih->handler = handler;
    ih->data = data;
    ih->type = type;
    return ih;
})

MLN_FUNC_VOID(static, void, mln_ipc_handler_free, (mln_ipc_handler_t *ih), (ih), {
    if (ih == NULL) return;
    free(ih);
})

/*
 * events
 */
MLN_FUNC_VOID(, void, mln_fork_master_events_set, (mln_event_t *ev), (ev), {
    mln_fork_t *f;
    for (f = worker_list_head; f != NULL; f = f->next) {
        if (mln_event_fd_set(ev, \
                             mln_tcp_conn_fd_get(&(f->conn)), \
                             M_EV_RECV|M_EV_NONBLOCK, \
                             M_EV_UNLIMITED, \
                             f, \
                             mln_ipc_fd_handler_master) < 0)
        {
            mln_log(error, "mln_event_fd_set() failed.\n");
            abort();
        }
    }
})

MLN_FUNC_VOID(, void, mln_fork_worker_events_set, (mln_event_t *ev), (ev), {
    if (mln_event_fd_set(ev, \
                         mln_tcp_conn_fd_get(&master_conn), \
                         M_EV_RECV|M_EV_NONBLOCK, \
                         M_EV_UNLIMITED, \
                         NULL, \
                         mln_ipc_fd_handler_worker) < 0)
    {
        mln_log(error, "mln_event_fd_set() failed.\n");
        abort();
    }
})

MLN_FUNC(, int, mln_fork_iterate, \
         (mln_event_t *ev, fork_iterate_handler handler, void *data), \
         (ev, handler, data), \
{
    mln_fork_t *f;
    for (f = worker_list_head; f != NULL; f = f->next) {
        if (handler != NULL) {
            if (handler(ev, f, data) < 0) return -1;
        }
    }
    return 0;
})

MLN_FUNC(, mln_tcp_conn_t *, mln_fork_master_connection_get, (void), (), {
    return &master_conn;
})

/*
 * There is a case that I press Ctrl+c to terminate master process,
 * a worker process dead firstly and the master will receive a
 * message of socketpair closed. It's OK, because I don't set
 * any signal handler to deal with that signal. So if you encounter
 * that problem, just IGNORE it.
 */
/*
 * Message Format:
 *     [Length(include type length) 4bytes|type 4bytes|content Nbytes]
 */
MLN_FUNC_VOID(, void, mln_ipc_fd_handler_master, (mln_event_t *ev, int fd, void *data), (ev, fd, data), {
    int ret;
    mln_fork_t *f = (mln_fork_t *)data;
    mln_tcp_conn_t *conn = &(f->conn);

    while (1) {
        ret = mln_tcp_conn_recv(conn, M_C_TYPE_MEMORY);
        if (ret == M_C_FINISH) {
            continue;
        } else if (ret == M_C_NOTYET) {
            break;
        } else if (ret == M_C_CLOSED) {
            mln_log(report, "Child process dead!\n");
            mln_fork_socketpair_close_handler(ev, f, fd);
            return ;
        } else {
            mln_log(error, "recv msg error. %s\n", strerror(errno));
            mln_fork_socketpair_close_handler(ev, f, fd);
            return ;
        }
    }

    mln_ipc_fd_handler_master_process(ev, f);
})

MLN_FUNC(static inline, int, mln_ipc_get_buf_with_len, \
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

MLN_FUNC(static inline, mln_size_t, mln_ipc_discard_bytes, \
         (mln_tcp_conn_t *tc, mln_size_t size), (tc, size), \
{
    mln_chain_t *c;
    mln_buf_t *b;
    mln_size_t left_size;

    while ((c = mln_tcp_conn_head(tc, M_C_RECV)) != NULL) {
        b = c->buf;
        if (b == NULL || b->pos == NULL) {
            mln_chain_pool_release(mln_tcp_conn_pop(tc, M_C_RECV));
            continue;
        }
        left_size = mln_buf_left_size(b);
        if (left_size > size) {
            b->left_pos += size;
            size = 0;
            break;
        }

        b->left_pos += left_size;
        size -= left_size;
        mln_chain_pool_release(mln_tcp_conn_pop(tc, M_C_RECV));
        if (size == 0) return 0;
    }

    return size;
})

MLN_FUNC_VOID(static, void, mln_ipc_fd_handler_master_process, (mln_event_t *ev, mln_fork_t *f), (ev, f), {
    mln_tcp_conn_t *tc = &(f->conn);

    while (1) {
        while (f->error_bytes) {
            f->error_bytes = mln_ipc_discard_bytes(tc, f->error_bytes);
        }
        switch (f->state) {
            case STATE_IDLE:
            {
                if (mln_ipc_get_buf_with_len(tc, &(f->msg_len), M_F_LENLEN) < 0) {
                    return;
                }
                f->state = STATE_LENGTH;
            }
            case STATE_LENGTH:
            {
                if (f->msg_content == NULL) {
                    f->msg_content = malloc(f->msg_len);
                    if (f->msg_content == NULL) {
                        f->error_bytes = f->msg_len;
                        f->state = STATE_IDLE;
                        break;
                    }
                }
                if (mln_ipc_get_buf_with_len(tc, f->msg_content, f->msg_len) < 0) {
                    return;
                }
                memcpy(&(f->msg_type), f->msg_content, M_F_TYPELEN);
                f->state = STATE_IDLE;
                mln_ipc_handler_t ih;
                ih.type = f->msg_type;
                mln_rbtree_node_t *rn = mln_rbtree_search(master_ipc_tree, &ih);
                if (!mln_rbtree_null(rn, master_ipc_tree)) {
                    mln_ipc_handler_t *ihp = (mln_ipc_handler_t *)mln_rbtree_node_data_get(rn);
                    if (ihp->handler != NULL)
                        ihp->handler(ev, \
                                     f, \
                                     f->msg_content+M_F_TYPELEN, \
                                     f->msg_len-M_F_TYPELEN, \
                                     &(ihp->data));
                }
                free(f->msg_content);
                f->msg_content = NULL;
                break;
            }
            default:
                mln_log(error, "No such state!\n");
                abort();
        }
    }
})

MLN_FUNC_VOID(, void, mln_fork_socketpair_close_handler, \
              (mln_event_t *ev, mln_fork_t *f, int fd), \
              (ev, f, fd), \
{
    mln_event_fd_set(ev, fd, M_EV_CLR, M_EV_UNLIMITED, NULL, NULL);
    enum proc_exec_type etype = f->etype;
    enum proc_state_type stype = f->stype;
    mln_s8ptr_t *args = f->args;
    mln_u32_t n_args = f->n_args;
    if (stype == M_PST_SUP) {
        mln_fork_destroy(f, 0);
        if (etype == M_PET_DFL) {
            int rv = mln_fork_restart(ev);
            if (rv < 0) {
                mln_log(error, "mln_fork_restart() error.\n");
                abort();
            } else if (rv == 0) {
                mln_event_break_set(ev);
            } else {
                mln_log(report, "Child process restart.\n");
            }
        } else {/*M_PET_EXE*/
            if (mln_fork_spawn(stype, args, n_args, ev) < 0) {
                mln_log(error, "mln_fork_spawn() error.\n");
                abort();
            }
            mln_log(report, "Process %s restart.\n", args[0]);
        }
    } else {/*M_PST_DFL*/
        mln_fork_destroy(f, 1);
    }
})

MLN_FUNC_VOID(, void, mln_ipc_fd_handler_worker, (mln_event_t *ev, int fd, void *data), (ev, fd, data), {
    int ret;
    mln_tcp_conn_t *conn = &master_conn;

    while (1) {
        ret = mln_tcp_conn_recv(conn, M_C_TYPE_MEMORY);
        if (ret == M_C_FINISH) {
            continue;
        } else if (ret == M_C_NOTYET) {
            break;
        } else if (ret == M_C_CLOSED) {
            mln_log(report, "Master process dead!\n");
            exit(127);
        } else {
            mln_log(error, "recv msg error. %s\n", strerror(errno));
            exit(127);
        }
    }

    mln_ipc_fd_handler_worker_process(ev, conn);
})

MLN_FUNC_VOID(static, void, mln_ipc_fd_handler_worker_process, \
              (mln_event_t *ev, mln_tcp_conn_t *tc), (ev, tc), \
{
    while (1) {
        while (child_error_bytes) {
            child_error_bytes = mln_ipc_discard_bytes(tc, child_error_bytes);
        }
        switch (child_state) {
            case STATE_IDLE:
            {
                if (mln_ipc_get_buf_with_len(tc, &(cur_msg_len), M_F_LENLEN) < 0) {
                    return;
                }
                child_state = STATE_LENGTH;
            }
            case STATE_LENGTH:
            {
                if (child_msg_content == NULL) {
                    child_msg_content = (mln_u8ptr_t)malloc(cur_msg_len);
                    if (child_msg_content == NULL) {
                        child_error_bytes = cur_msg_len;
                        child_state = STATE_IDLE;
                        break;
                    }
                }
                if (mln_ipc_get_buf_with_len(tc, child_msg_content, cur_msg_len) < 0) {
                    return;
                }
                memcpy(&cur_msg_type, child_msg_content, M_F_TYPELEN);
                child_state = STATE_IDLE;
                mln_ipc_handler_t ih;
                ih.type = cur_msg_type;
                mln_rbtree_node_t *rn = mln_rbtree_search(worker_ipc_tree, &ih);
                if (!mln_rbtree_null(rn, worker_ipc_tree)) {
                    mln_ipc_handler_t *ihp = (mln_ipc_handler_t *)mln_rbtree_node_data_get(rn);
                    if (ihp->handler != NULL)
                        ihp->handler(ev, \
                                     tc, \
                                     child_msg_content+M_F_TYPELEN, \
                                     cur_msg_len-M_F_TYPELEN, \
                                     &(ihp->data));
                }
                free(child_msg_content);
                child_msg_content = NULL;
                break;
            }
            default:
                mln_log(error, "No such state!\n");
                abort();
        }
    }
})

MLN_FUNC(, int, mln_ipc_master_send_prepare, \
         (mln_event_t *ev, mln_u32_t type, void *msg, mln_size_t len, mln_fork_t *f_child), \
         (ev, type, msg, len, f_child), \
{
    mln_u32_t length = sizeof(type) + len;
    mln_u8ptr_t buf;
    mln_chain_t *c;
    mln_buf_t *b;
    mln_size_t buflen;
    mln_tcp_conn_t *conn = &(f_child->conn);
    mln_alloc_t *pool = mln_tcp_conn_pool_get(conn);

    buflen = length + sizeof(length);

    c = mln_chain_new(pool);
    if (c == NULL) return -1;

    b = mln_buf_new(pool);
    if (b == NULL) {
        mln_chain_pool_release(c);
        return -1;
    }
    c->buf = b;

    buf = (mln_u8ptr_t)mln_alloc_m(pool, buflen);
    if (buf == NULL) {
        mln_chain_pool_release(c);
        return -1;
    }

    b->left_pos = b->pos = b->start = buf;
    b->last = b->end = buf + buflen;
    b->in_memory = 1;
    b->last_buf = 1;
    b->last_in_chain = 1;

    memcpy(buf, &length, sizeof(length));
    memcpy(buf+sizeof(length), &type, sizeof(type));
    memcpy(buf+sizeof(length)+sizeof(type), msg, len);

    mln_tcp_conn_append(conn, c, M_C_SEND);

    mln_event_fd_set(ev, \
                     mln_tcp_conn_fd_get(conn), \
                     M_EV_SEND|M_EV_APPEND|M_EV_NONBLOCK|M_EV_ONESHOT, \
                     M_EV_UNLIMITED, \
                     f_child, \
                     mln_ipc_fd_handler_master_send);

    return 0;
})

MLN_FUNC_VOID(static, void, mln_ipc_fd_handler_master_send, \
              (mln_event_t *ev, int fd, void *data), (ev, fd, data), \
{
    mln_fork_t *f = (mln_fork_t *)data;
    mln_tcp_conn_t *conn = &(f->conn);
    mln_chain_t *c;
    int ret;

    while ((c = mln_tcp_conn_head(conn, M_C_SEND)) != NULL) {
        ret = mln_tcp_conn_send(conn);
        if (ret == M_C_FINISH) {
            continue;
        } else if (ret == M_C_NOTYET) {
            mln_chain_pool_release_all(mln_tcp_conn_remove(conn, M_C_SENT));
            mln_event_fd_set(ev, \
                             mln_tcp_conn_fd_get(conn), \
                             M_EV_SEND|M_EV_APPEND|M_EV_NONBLOCK|M_EV_ONESHOT, \
                             M_EV_UNLIMITED, \
                             f, \
                             mln_ipc_fd_handler_master_send);
            return;
        } else if (ret == M_C_ERROR) {
            mln_log(report, "Child process dead!\n");
            mln_fork_socketpair_close_handler(ev, f, fd);
            return ;
        } else {
            mln_log(error, "Shouldn't be here.\n");
            abort();
        }
    }

    mln_chain_pool_release_all(mln_tcp_conn_remove(conn, M_C_SENT));
})

MLN_FUNC(, int, mln_ipc_worker_send_prepare, \
         (mln_event_t *ev, mln_u32_t type, void *msg, mln_size_t len), \
         (ev, type, msg, len), \
{
    mln_u32_t length = sizeof(type) + len;
    mln_u8ptr_t buf;
    mln_chain_t *c;
    mln_buf_t *b;
    mln_size_t buflen;
    mln_tcp_conn_t *conn = &master_conn;
    mln_alloc_t *pool = mln_tcp_conn_pool_get(conn);

    buflen = length + sizeof(length);

    c = mln_chain_new(pool);
    if (c == NULL) return -1;

    b = mln_buf_new(pool);
    if (b == NULL) {
        mln_chain_pool_release(c);
        return -1;
    }
    c->buf = b;

    buf = (mln_u8ptr_t)mln_alloc_m(pool, buflen);
    if (buf == NULL) {
        mln_chain_pool_release(c);
        return -1;
    }

    b->left_pos = b->pos = b->start = buf;
    b->last = b->end = buf + buflen;
    b->in_memory = 1;
    b->last_buf = 1;
    b->last_in_chain = 1;

    memcpy(buf, &length, sizeof(length));
    memcpy(buf+sizeof(length), &type, sizeof(type));
    memcpy(buf+sizeof(length)+sizeof(type), msg, len);

    mln_tcp_conn_append(conn, c, M_C_SEND);

    mln_event_fd_set(ev, \
                     mln_tcp_conn_fd_get(conn), \
                     M_EV_SEND|M_EV_APPEND|M_EV_NONBLOCK|M_EV_ONESHOT, \
                     M_EV_UNLIMITED, \
                     NULL, \
                     mln_ipc_fd_handler_worker_send);

    return 0;
})

MLN_FUNC_VOID(static, void, mln_ipc_fd_handler_worker_send, \
              (mln_event_t *ev, int fd, void *data), (ev, fd, data), \
{
    mln_tcp_conn_t *conn = &master_conn;
    mln_chain_t *c;
    int ret;

    while ((c = mln_tcp_conn_head(conn, M_C_SEND)) != NULL) {
        ret = mln_tcp_conn_send(conn);
        if (ret == M_C_FINISH) {
            continue;
        } else if (ret == M_C_NOTYET) {
            mln_chain_pool_release_all(mln_tcp_conn_remove(conn, M_C_SENT));
            mln_event_fd_set(ev, \
                             mln_tcp_conn_fd_get(conn), \
                             M_EV_SEND|M_EV_APPEND|M_EV_NONBLOCK|M_EV_ONESHOT, \
                             M_EV_UNLIMITED, \
                             NULL, \
                             mln_ipc_fd_handler_worker_send);
            return;
        } else if (ret == M_C_ERROR) {
            mln_log(report, "master process dead!\n");
            exit(127);
        } else {
            mln_log(error, "Shouldn't be here.\n");
            abort();
        }
    }

    mln_chain_pool_release_all(mln_tcp_conn_remove(conn, M_C_SENT));
})


/*chain*/
MLN_CHAIN_FUNC_DEFINE(static inline, \
                      worker_list, \
                      mln_fork_t, \
                      prev, \
                      next);
#endif

