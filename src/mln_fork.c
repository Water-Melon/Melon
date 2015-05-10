
/*
 * Copyright (C) Niklaus F.Schen.
 */
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <sys/ioctl.h>
#include "mln_fork.h"
#include "mln_rbtree.h"
#include "mln_log.h"
#include "mln_global.h"

mln_tcp_connection_t master_conn;
mln_u32_t child_state;
mln_u32_t cur_msg_len;
mln_u32_t cur_msg_type;
mln_fork_t *worker_list_head = NULL;
mln_fork_t *worker_list_tail = NULL;
mln_rbtree_t *master_ipc_tree = NULL;
mln_rbtree_t *worker_ipc_tree = NULL;
clr_handler rs_clr_handler = NULL;
void *rs_clr_data = NULL;
volatile mln_u32_t child_startup = 0;
int wait_signo = SIGUSR1;

MLN_CHAIN_FUNC_DECLARE(worker_list, \
                       mln_fork_t, \
                       static inline void, \
                       __NONNULL3(1,2,3));
static int
mln_fork_rbtree_cmp(const void *k1, const void *k2) __NONNULL2(1,2);
static int
do_fork_worker_process(mln_sauto_t n_worker_proc);
static void
do_fork_sig_handler(int signo);
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

/*pre-fork*/
int mln_pre_fork(void)
{
    if (signal(SIGCHLD, SIG_IGN) == SIG_ERR) {
        mln_log(error, "signal() to ignore SIGCHLD failed, %s\n", strerror(errno));
        return -1;
    }
    if (signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
        mln_log(error, "signal() to ignore SIGPIPE failed, %s\n", strerror(errno));
        return -1;
    }
    struct sigaction act;
    memset(&act, 0, sizeof(act));
    act.sa_handler = do_fork_sig_handler;
    sigemptyset(&(act.sa_mask));
    sigfillset(&(act.sa_mask));
    if (sigaction(wait_signo, &act, NULL) < 0) {
        mln_log(error, "sigaction error, signo:%d. %s\n", \
                wait_signo, strerror(errno));
        return -1;
    }
    memset(&master_conn, 0, sizeof(master_conn));
    master_conn.fd = -1;
    child_state = STATE_IDLE;
    cur_msg_len = 0;
    cur_msg_type = 0;
    struct mln_rbtree_attr rbattr;
    rbattr.cmp = mln_fork_rbtree_cmp;
    rbattr.data_free = NULL;
    if ((master_ipc_tree = mln_rbtree_init(&rbattr)) < 0) {
        mln_log(error, "No memory.\n");
        return -1;
    }
    if ((worker_ipc_tree = mln_rbtree_init(&rbattr)) < 0) {
        mln_log(error, "No memory.\n");
        mln_rbtree_destroy(master_ipc_tree);
        master_ipc_tree = NULL;
        return -1;
    }
    mln_set_ipc_handlers();
    return 0;
}

static int
mln_fork_rbtree_cmp(const void *k1, const void *k2)
{
    mln_ipc_handler_t *ih1 = (mln_ipc_handler_t *)k1;
    mln_ipc_handler_t *ih2 = (mln_ipc_handler_t *)k2;
    if (ih1->type > ih2->type) return 1;
    else if (ih1->type == ih2->type) return 0;
    return -1;
}

/*mln_fork_t*/
static mln_fork_t *
mln_fork_init(struct mln_fork_attr *attr)
{
    mln_fork_t *f = (mln_fork_t *)malloc(sizeof(mln_fork_t));
    if (f == NULL) return NULL;
    f->prev = NULL;
    f->next = NULL;
    f->args = attr->args;
    mln_tcp_connection_init(&(f->conn), attr->fd);
    f->pid = attr->pid;
    f->n_args = attr->n_args;
    f->state = STATE_IDLE;
    f->msg_len = 0;
    f->msg_type = 0;
    f->etype = attr->etype;
    f->stype = attr->stype;
    worker_list_chain_add(&worker_list_head, &worker_list_tail, f);
    return f;
}

static void
mln_fork_destroy(mln_fork_t *f, int free_args)
{
    if (f == NULL) return;
    if (f->args != NULL && free_args) {
        free(f->args);
        f->args = NULL;
    }
    mln_tcp_connection_destroy(&(f->conn));
    worker_list_chain_del(&worker_list_head, &worker_list_tail, f);
    free(f);
}

static void
mln_fork_destroy_all(void)
{
    mln_fork_t *f;
    while ((f = worker_list_head) != NULL) {
        mln_fork_destroy(f, 1);
    }
}

/*
 * fork processes
 */
int do_fork(void)
{
    mln_conf_t *cf = mln_get_conf();
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
        if (mln_get_cmd_args_num(cmd) > 1) {
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
    mln_u32_t n = mln_get_cmd_num(cf, "exec_proc");
    if (n == 0) return 1;

    v = (mln_conf_cmd_t **)calloc(n+1, sizeof(mln_conf_cmd_t *));
    if (v == NULL) {
        mln_log(error, "No memory.\n");
        return -1;
    }
    mln_get_all_cmds(cf, "exec_proc", v);
    for (cc = v; *cc != NULL; cc++) {
        n_args = mln_get_cmd_args_num(*cc);
        if (n_args == 0) {
            mln_log(error, "Demand arguments in 'exec_proc'.\n");
            exit(1);
        }
        v_args = (mln_s8ptr_t *)calloc(n_args+2, sizeof(mln_s8ptr_t));
        if (v_args == NULL) {
            mln_log(error, "No memory.\n");
            continue;
        }
        for (i = 0; i < n_args; i++) {
            arg_ci = (*cc)->search(*cc, i+1);
            if (arg_ci->type != CONF_STR) {
                mln_log(error, "Demand string arguments in 'exec_proc'.\n");
                exit(1);
            }
            v_args[i] = arg_ci->val.s->str;
        }
        if (!mln_const_strcmp((*cc)->cmd_name, "keepalive")) {
            mln_fork_spawn(M_PST_SUP, v_args, n_args, NULL);
        } else if (!mln_const_strcmp((*cc)->cmd_name, "default")) {
            mln_fork_spawn(M_PST_DFL, v_args, n_args, NULL);
        } else {
            mln_log(error, "Invalid command '%s' in 'exec_proc'.\n", (*cc)->cmd_name->str);
            exit(1);
        }
    }
    free(v);
    return 1;
}

int mln_fork_spawn(enum proc_state_type stype, \
                   mln_s8ptr_t *args, \
                   mln_u32_t n_args, \
                   mln_event_t *master_ev)
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
        snprintf(fd_str, sizeof(fd_str)-1, "%d", master_conn.fd);
        args[n_args] = fd_str;
        if (master_ev != NULL) mln_event_destroy(master_ev);
        mln_log_destroy();
        if (execv(args[0], args) < 0) {
            mln_log(error, "execv error, %s\n", strerror(errno));
            exit(1);
        }
    }
    return 0;
}

int mln_fork_restart(mln_event_t *master_ev)
{
    return do_fork_core(M_PET_DFL, \
                        M_PST_SUP, \
                        NULL, \
                        0, \
                        master_ev);
}

static int
do_fork_worker_process(mln_sauto_t n_worker_proc)
{
    mln_sauto_t i;
    int ret;
    for (i = 0; i < n_worker_proc; i++) {
        mln_log(none, "Start up worker process No.%l\n", i+1);
        if ((ret = mln_fork_restart(NULL)) < 0) {
            continue;
        } else if (ret == 0) {
            return 0;
        }
    }
    return 1;
}

void mln_set_resource_clear_handler(clr_handler handler, void *data)
{
    rs_clr_handler = handler;
    rs_clr_data = data;
}

static void
do_fork_sig_handler(int signo)
{
    child_startup = 1;
}

static int
do_fork_core(enum proc_exec_type etype, \
             enum proc_state_type stype, \
             mln_s8ptr_t *args, \
             mln_u32_t n_args, \
             mln_event_t *master_ev)
{
    int fds[2];
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, fds) < 0) {
        mln_log(error, "socketpair() error. %s\n", strerror(errno));
        return -1;
    }
    pid_t pid = fork();
    if (pid > 0) {
        close(fds[1]);
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
         * jump into the mln_dispatch().
         * And then, child process built, that would make the event
         * which is triggered in parent process to be a lie.
         */
        while (child_startup == 0)
            ;
        child_startup = 0;
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
            if (mln_event_set_fd(master_ev, \
                                 f->conn.fd, \
                                 M_EV_RECV, \
                                 M_EV_UNLIMITED, \
                                 f, \
                                 mln_ipc_fd_handler_master) < 0)
            {
                mln_log(error, "mln_event_set_fd() failed.\n");
                abort();
            }
        }
        return 1;
    } else if (pid == 0) {
        close(fds[0]);
        mln_fork_destroy_all();
        mln_rbtree_destroy(master_ipc_tree);
        if (rs_clr_handler != NULL)
            rs_clr_handler(rs_clr_data);
        master_ipc_tree = NULL;
        mln_tcp_connection_init(&master_conn, fds[1]);
        signal(SIGCHLD, SIG_DFL);
        signal(wait_signo, SIG_DFL);
        if (kill(getppid(), wait_signo) < 0) {
            mln_log(error, "kill() error. %s\n", strerror(errno));
            abort();
        }
        return 0;
    }
    mln_log(error, "fork() error. %s\n", strerror(errno));
    return -1;
}

/*mln_set_master_ipc_handler*/
void mln_set_master_ipc_handler(mln_ipc_handler_t *ih)
{
    mln_rbtree_node_t *rn = mln_rbtree_search(master_ipc_tree, \
                                              master_ipc_tree->root, \
                                              ih);
    if (rn->data != NULL) {
        mln_log(error, "IPC type '%d' already existed.\n", ih->type);
        abort();
    }
    rn = mln_rbtree_new_node(master_ipc_tree, ih);
    if (rn == NULL) {
        mln_log(error, "No memory.\n");
        abort();
    }
    mln_rbtree_insert(master_ipc_tree, rn);
}
/*mln_set_worker_ipc_handler*/
void mln_set_worker_ipc_handler(mln_ipc_handler_t *ih)
{
    mln_rbtree_node_t *rn = mln_rbtree_search(worker_ipc_tree, \
                                              worker_ipc_tree->root, \
                                              ih);
    if (rn->data != NULL) {
        mln_log(error, "IPC type '%d' already existed.\n", ih->type);
        abort();
    }
    rn = mln_rbtree_new_node(worker_ipc_tree, ih);
    if (rn == NULL) {
        mln_log(error, "No memory.\n");
        abort();
    }
    mln_rbtree_insert(worker_ipc_tree, rn);
}

/*
 * events
 */
void mln_fork_master_set_events(mln_event_t *ev)
{
    mln_fork_t *f;
    for (f = worker_list_head; f != NULL; f = f->next) {
        if (mln_event_set_fd(ev, \
                             f->conn.fd, \
                             M_EV_RECV, \
                             M_EV_UNLIMITED, \
                             f, \
                             mln_ipc_fd_handler_master) < 0)
        {
            mln_log(error, "mln_event_set_fd() failed.\n");
            abort();
        }
    }
}

void mln_fork_worker_set_events(mln_event_t *ev)
{
    if (mln_event_set_fd(ev, \
                         master_conn.fd, \
                         M_EV_RECV, \
                         M_EV_UNLIMITED, \
                         NULL, \
                         mln_ipc_fd_handler_worker) < 0)
    {
        mln_log(error, "mln_event_set_fd() failed.\n");
        abort();
    }
}

int mln_fork_scan_all(mln_event_t *ev, \
                      mln_u32_t flag, \
                      int timeout_ms, \
                      ev_fd_handler fd_handler)
{
    mln_fork_t *f;
    for (f = worker_list_head; f != NULL; f = f->next) {
        if (mln_event_set_fd(ev, \
                             f->conn.fd, \
                             flag, \
                             timeout_ms, \
                             f, \
                             fd_handler) < 0)
        {
            return -1;
        }
    }
    return 0;
}

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
void mln_ipc_fd_handler_master(mln_event_t *ev, int fd, void *data)
{
    int ret;
    mln_fork_t *f = (mln_fork_t *)data;
    switch (f->state) {
        case STATE_IDLE:
        {
            if (M_C_RCV_NULL(&(f->conn))) {
                if (mln_tcp_connection_set_buf(&(f->conn), \
                                               NULL, \
                                               M_F_LENLEN, \
                                               M_C_RECV, \
                                               0) < 0)
                {
                    mln_log(error, "No memory.\n");
                    return;
                }
            }
            if ((ret = mln_tcp_connection_recv(&(f->conn))) == M_C_FINISH) {
                f->msg_len = 0;
                memcpy(&(f->msg_len), \
                       (mln_u32_t *)mln_tcp_connection_get_buf(&(f->conn), \
                                                               M_C_RECV), \
                       M_F_LENLEN);
                mln_tcp_connection_clr_buf(&(f->conn), M_C_RECV);
                f->state = STATE_LENGTH;
                return ;
            } else if (ret == M_C_NOTYET) {
                return ;
            } else if (ret == M_C_ERROR) {
                mln_log(error, "recv msg error. %s\n", strerror(errno));
                mln_socketpair_close_handler(ev, f, fd);
                return ;
            } else { /*M_C_CLOSED*/
                mln_log(report, "Child process dead!\n");
                mln_socketpair_close_handler(ev, f, fd);
                return ;
            }
        }
        case STATE_LENGTH:
        {
            if (M_C_RCV_NULL(&(f->conn))) {
                if (mln_tcp_connection_set_buf(&(f->conn), \
                                               NULL, \
                                               f->msg_len, \
                                               M_C_RECV, \
                                               0) < 0)
                {
                    mln_log(error, "No memory.\n");
                    return;
                }
            }
            if ((ret = mln_tcp_connection_recv(&(f->conn))) == M_C_FINISH) {
                f->msg_type = 0;
                mln_u8ptr_t buf = (mln_u8ptr_t)mln_tcp_connection_get_buf(&(f->conn), M_C_RECV);
                memcpy(&(f->msg_type), (mln_u32_t *)buf, M_F_TYPELEN);
                mln_ipc_handler_t ih;
                ih.type = f->msg_type;
                mln_rbtree_node_t *rn = mln_rbtree_search(master_ipc_tree, \
                                                          master_ipc_tree->root, \
                                                          &ih);
                if (!mln_rbtree_null(rn, master_ipc_tree)) {
                    mln_ipc_handler_t *ihp = (mln_ipc_handler_t *)(rn->data);
                    if (ihp->handler != NULL)
                        ihp->handler(ev, \
                                     f, \
                                     buf+M_F_TYPELEN, \
                                     f->msg_len-M_F_TYPELEN, \
                                     &(ihp->data));
                }
                mln_tcp_connection_clr_buf(&(f->conn), M_C_RECV);
                f->state = STATE_IDLE;
                return ;
            } else if (ret == M_C_NOTYET) {
                return ;
            } else if (ret == M_C_ERROR) {
                mln_log(error, "recv msg error. %s\n", strerror(errno));
                mln_socketpair_close_handler(ev, f, fd);
                return ;
            } else {
                mln_log(report, "Child process dead!\n");
                mln_socketpair_close_handler(ev, f, fd);
                return ;
            }
        }
        default:
            mln_log(error, "No such state!\n");
            abort();
    }
}

void mln_socketpair_close_handler(mln_event_t *ev, mln_fork_t *f, int fd)
{
    mln_event_set_fd(ev, fd, M_EV_CLR, M_EV_UNLIMITED, NULL, NULL);
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
                mln_event_set_break(ev);
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
}

void mln_ipc_fd_handler_worker(mln_event_t *ev, int fd, void *data)
{
    int ret;
    switch (child_state) {
        case STATE_IDLE:
        {
            if (M_C_RCV_NULL(&master_conn)) {
                if (mln_tcp_connection_set_buf(&master_conn, \
                                               NULL, \
                                               M_F_LENLEN, \
                                               M_C_RECV, \
                                               0) < 0)
                {
                    mln_log(error, "No memory.\n");
                    return;
                }
            }
            if ((ret = mln_tcp_connection_recv(&master_conn)) == M_C_FINISH) {
                cur_msg_len = 0;
                memcpy(&cur_msg_len, \
                       (mln_u32_t *)mln_tcp_connection_get_buf(&master_conn, \
                                                               M_C_RECV), \
                       M_F_LENLEN);
                mln_tcp_connection_clr_buf(&master_conn, M_C_RECV);
                child_state = STATE_LENGTH;
                return ;
            } else if (ret == M_C_NOTYET) {
                return ;
            } else if (ret == M_C_ERROR) {
                mln_log(error, "recv msg error. %s\n", strerror(errno));
                abort();
            } else { /*M_C_CLOSED*/
                mln_log(report, "Master process dead!\n");
                exit(127);
            }
        }
        case STATE_LENGTH:
        {
            if (M_C_RCV_NULL(&master_conn)) {
                if (mln_tcp_connection_set_buf(&master_conn, \
                                               NULL, \
                                               cur_msg_len, \
                                               M_C_RECV, \
                                               0) < 0)
                {
                    mln_log(error, "No memory.\n");
                    return;
                }
            }
            if ((ret = mln_tcp_connection_recv(&master_conn)) == M_C_FINISH) {
                cur_msg_type = 0;
                mln_u8ptr_t buf = (mln_u8ptr_t)mln_tcp_connection_get_buf(&master_conn, M_C_RECV);
                memcpy(&cur_msg_type, (mln_u32_t *)buf, M_F_TYPELEN);
                mln_ipc_handler_t ih;
                ih.type = cur_msg_type;
                mln_rbtree_node_t *rn = mln_rbtree_search(worker_ipc_tree, \
                                                          worker_ipc_tree->root, \
                                                          &ih);
                if (!mln_rbtree_null(rn, worker_ipc_tree)) {
                    mln_ipc_handler_t *ihp = (mln_ipc_handler_t *)(rn->data);
                    if (ihp->handler != NULL)
                        ihp->handler(ev, \
                                     &master_conn, \
                                     buf+M_F_TYPELEN, \
                                     cur_msg_len-M_F_TYPELEN, \
                                     &(ihp->data));
                }
                mln_tcp_connection_clr_buf(&master_conn, M_C_RECV);
                child_state = STATE_IDLE;
                return ;
            } else if (ret == M_C_NOTYET) {
                return ;
            } else if (ret == M_C_ERROR) {
                mln_log(error, "recv msg error. %s\n", strerror(errno));
                abort();
            } else { /*M_C_CLOSED*/
                mln_log(report, "Master process dead!\n");
                exit(127);
            }
        }
        default:
            mln_log(error, "No such state!\n");
            abort();
    }
}

/*chain*/
MLN_CHAIN_FUNC_DEFINE(worker_list, \
                      mln_fork_t, \
                      static inline void, \
                      prev, \
                      next);

