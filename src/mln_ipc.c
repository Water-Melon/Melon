
/*
 * Copyright (C) Niklaus F.Schen.
 */
#include "mln_lex.h"
#include "mln_fork.h"
#include "mln_ipc.h"
#include "mln_log.h"
#include "mln_connection.h"
#include "mln_event.h"
#include "mln_fheap.h"
#include "mln_global.h"
#include "mln_hash.h"
#include "mln_prime_generator.h"
#include "mln_rbtree.h"
#include "mln_string.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>

/*
 * IPC only act on A child process and the parent process.
 * If there are some threads in a child process,
 * IPC only act on the control thread (main thread) and the parent process.
 * If you need to send something to the peer,
 * you can call mln_tcp_connection_set_buf() to set connection send buffer
 * and call mln_event_set_fd() to set send event, which means you can
 * customize the send routine.
 */

void conf_reload_master(mln_event_t *ev, void *f_ptr, void *buf, mln_u32_t len, void **udata_ptr)
{
    /*
     * do nothing.
     */
}

void conf_reload_worker(mln_event_t *ev, void *f_ptr, void *buf, mln_u32_t len, void **udata_ptr)
{
    if (mln_conf_reload() < 0) {
        mln_log(error, "mln_conf_reload() failed.\n");
        exit(1);
    }
}


#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

static void distri_init_cmd_check(mln_conf_cmd_t *cc, int is_srv);
static int distri_init_isalive(mln_conf_cmd_t *cc);
static int distri_init_copy(mln_conf_cmd_t *cc, void *buf, int is_srv);
static void distri_init_master_send(mln_event_t *ev, int fd, void *data);

char distribute_domain[] = "distribute";
char distribute_init_internal[] = "internal_addr";
char distribute_init_external[] = "external_addr";
char distribute_init_server[] = "dis_srv";
/*
 * [nr_addr(int)|internal-addr|external-addr|rest-addrs...]
 * if nr_addr == 0, there is no configuration about distribute module.
 * if nr_addr < 0, memory no enough.
 */

void distri_init_master(mln_event_t *ev, void *f_ptr, void *buf, mln_u32_t len, void **udata_ptr)
{
    int nr_addr = 0;
    len = sizeof(int);
    buf = NULL;
    mln_conf_cmd_t **vc = NULL;
    mln_conf_t *cf = mln_get_conf();
    if (cf == NULL) {
        mln_log(error, "Configuration messed up.\n");
        abort();
    }
    int nr_cmd;
    if ((nr_cmd = mln_get_cmd_num(cf, distribute_domain)) == 0) {
        goto out;
    }
    vc = (mln_conf_cmd_t **)calloc(nr_cmd, sizeof(mln_conf_t *));
    if (vc == NULL) {
        nr_addr = -1;
        goto out;
    }
    mln_get_all_cmds(cf, distribute_domain, vc);

    len += (nr_cmd * sizeof(struct sockaddr));
    buf = (mln_u8ptr_t)malloc(len);
    if (buf == NULL) {
        free(vc);
        nr_addr = -1;
        goto out;
    }

    nr_addr = nr_cmd;
    int i;
    int inter_times = 0, exter_times = 0;
    mln_conf_cmd_t *cc;
    mln_s8ptr_t p = buf + sizeof(int);
    for (i = 0; i < nr_cmd; i++) {
        cc = vc[i];
        if (!mln_const_strcmp(cc->cmd_name, distribute_init_internal)) {
            inter_times++;
            distri_init_cmd_check(cc, 0);
            if (distri_init_copy(cc, p, 0) < 0) {
                mln_log(error, "Initialize internal address failed.\n");
                exit(1);
            }
            p += sizeof(struct sockaddr);
            continue;
        }
        if (!mln_const_strcmp(cc->cmd_name, distribute_init_external)) {
            exter_times++;
            distri_init_cmd_check(cc, 0);
            if (distri_init_copy(cc, p, 0) < 0) {
                mln_log(error, "Initialize external address failed.\n");
                exit(1);
            }
            p += sizeof(struct sockaddr);
            continue;
        }
        if (!mln_const_strcmp(cc->cmd_name, distribute_init_server)) {
            distri_init_cmd_check(cc, 1);
            if (distri_init_isalive(cc)) {
                if (distri_init_copy(cc, p, 1) < 0) {
                    nr_addr--;
                    len -= sizeof(struct sockaddr);
                } else {
                    p += sizeof(struct sockaddr);
                }
            } else {
                nr_addr--;
                len -= sizeof(struct sockaddr);
            }
            continue;
        }
        mln_log(error, "No such command in domain '%s'.\n", cc->cmd_name->str);
        exit(1);
    }
    if (inter_times != 1 || exter_times != 1) {
        mln_log(error, "Repeat command in domain '%s'.\n", distribute_domain);
        exit(1);
    }
out:
    if (nr_addr <= 0) buf = (mln_u8ptr_t)(&nr_addr);
    else memcpy(buf, &nr_addr, sizeof(nr_addr));

    mln_fork_t *f = (mln_fork_t *)f_ptr;
    mln_tcp_connection_t *conn = &(f->conn);
    if (mln_tcp_connection_set_buf(conn, buf, len, M_C_SEND, 0) < 0) {
        mln_log(report, "No memory.\n");
        if (nr_addr > 0) free(buf);
        if (vc != NULL) free(vc);
        return;
    }
    if (nr_addr > 0) free(buf);
    if (mln_event_set_fd(ev, \
                         conn->fd, \
                         M_EV_SEND|M_EV_APPEND|M_EV_ONESHOT, \
                         M_EV_UNLIMITED, \
                         f, \
                         distri_init_master_send) < 0)
    {
        mln_log(report, "No memroy.\n");
        mln_tcp_connection_clr_buf(conn, M_C_SEND);
    }
    if (vc != NULL) free(vc);
}

static void distri_init_cmd_check(mln_conf_cmd_t *cc, int is_srv)
{
    int i = 1;
    mln_u32_t nr_item = mln_get_cmd_args_num(cc);
    if (nr_item < 2 || nr_item > 3) {
        mln_log(error, "Item in command '%s' in domain '%s' error.\n", \
                cc->cmd_name->str, distribute_domain);
        exit(1);
    }
    mln_conf_item_t *ci;
    if (is_srv) {
        ci = cc->search(cc, i++);
        if (ci->type != CONF_BOOL) {
            mln_log(error, "Command need a server state in domain '%s'.\n", \
                    distribute_domain);
            exit(1);
        }
    }
    ci = cc->search(cc, i++);
    if (ci->type != CONF_STR) {
        mln_log(error, "Invalid item type in domain '%s'.\n", \
                distribute_domain);
        exit(1);
    }
    ci = cc->search(cc, i);
    if (ci == NULL) {
        mln_log(error, "Lack of an item in command '%s' in domain '%s'.\n", \
                cc->cmd_name->str, distribute_domain);
        exit(1);
    }
    if (ci->type != CONF_STR) {
        mln_log(error, "Invalid item type in domain '%s'.\n", \
                distribute_domain);
        exit(1);
    }
}

static int distri_init_isalive(mln_conf_cmd_t *cc)
{
    mln_conf_item_t *ci = cc->search(cc, 1);
    if (ci->val.b) return 1;
    return 0;
}

static int distri_init_copy(mln_conf_cmd_t *cc, void *buf, int is_srv)
{
    int err, sockfd;
    mln_conf_item_t *host, *serv;
    struct addrinfo hints, *res, *ressave;
    memset(&hints, 0, sizeof(hints));
    if (is_srv) {
        host = cc->search(cc, 2);
        serv = cc->search(cc, 3);
        hints.ai_flags = AI_CANONNAME;
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        err = getaddrinfo(host->val.s->str, serv->val.s->str, &hints, &res);
        if (err != 0) {
            mln_log(error, "Get address '%s':'%s' structure failed. %s\n", \
            host->val.s->str, serv->val.s->str, gai_strerror(err));
            return -1;
        }
        ressave = res;
        do {
            sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
            if (sockfd < 0) continue;
            if (connect(sockfd, res->ai_addr, res->ai_addrlen) == 0) {
                memcpy(buf, res->ai_addr, sizeof(struct sockaddr));
                close(sockfd);
                break;
            }
            close(sockfd);
        } while ((res = res->ai_next) != NULL);
        if (res == NULL) {
            mln_log(error, "Server address '%s':'%s' error or server not start up.\n", \
                    host->val.s->str, serv->val.s->str);
            freeaddrinfo(ressave);
            return -1;
        }
    } else {
        int on = 1;
        host = cc->search(cc, 1);
        serv = cc->search(cc, 2);
        hints.ai_flags = AI_PASSIVE;
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        err = getaddrinfo(host->val.s->str, serv->val.s->str, &hints, &res);
        if (err != 0) {
            mln_log(error, "Get address '%s':'%s' structure failed. %s\n", \
            host->val.s->str, serv->val.s->str, gai_strerror(err));
            return -1;
        }
        ressave = res;
        do {
            sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
            if (sockfd < 0) continue;
            if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) == -1) {
                close(sockfd);
                continue;
            }
            if (bind(sockfd, res->ai_addr, res->ai_addrlen) == 0) {
                memcpy(buf, res->ai_addr, sizeof(struct sockaddr));
                close(sockfd);
                break;
            }
            close(sockfd);
        } while ((res = res->ai_next) != NULL);
        if (res == NULL) {
            mln_log(error, "Initialize server address '%s':'%s' failed.\n", \
                    host->val.s->str, serv->val.s->str);
            freeaddrinfo(ressave);
            return -1;
        }
    }
    freeaddrinfo(ressave);
    return 0;
}

static void distri_init_master_send(mln_event_t *ev, int fd, void *data)
{
    int ret;
    mln_fork_t *f = (mln_fork_t *)data;
    mln_tcp_connection_t *c = &(f->conn);
    if ((ret = mln_tcp_connection_send(c)) == M_C_FINISH) {
        mln_tcp_connection_clr_buf(c, M_C_SEND);
    } else if (ret == M_C_NOTYET) {
        if (mln_event_set_fd(ev, \
                             fd, \
                             M_EV_SEND|M_EV_APPEND|M_EV_ONESHOT, \
                             M_EV_UNLIMITED, \
                             c, \
                             distri_init_master_send) < 0)
        {
            mln_log(error, "Shouldn't occur an error. No memory need to be allocated.\n");
            abort();
        }
    } else if (ret == M_C_ERROR) {
        mln_log(error, "mln_tcp_connection_send() error. %s\n", strerror(errno));
        mln_socketpair_close_handler(ev, f, fd);
    } else {
        mln_log(error, "Shouldn't be here.\n");
        abort();
    }
}

void distri_init_worker(mln_event_t *ev, void *f_ptr, void *buf, mln_u32_t len, void **udata_ptr)
{
  /*
   * do nothing.
   * we don't receive any message here.
   */
}



char send_buffer[] = "Hello world!";

void master_send_handler(mln_event_t *ev, int fd, void *data)
{
    int ret;
    mln_fork_t *f = (mln_fork_t *)data;
    mln_tcp_connection_t *c = &(f->conn);
    if ((ret = mln_tcp_connection_send(c)) == M_C_FINISH) {
        mln_tcp_connection_clr_buf(c, M_C_SEND);
        mln_log(report, "Send succeed!\n");
    } else if (ret == M_C_NOTYET) {
        if (mln_event_set_fd(ev, \
                             fd, \
                             M_EV_SEND|M_EV_APPEND|M_EV_ONESHOT, \
                             M_EV_UNLIMITED, \
                             c, \
                             master_send_handler) < 0)
        {
            mln_log(error, "mln_event_set_fd() failed.\n");
            abort();
        }
    } else if (ret == M_C_ERROR) {
        mln_log(error, "mln_tcp_connection_send error. %s\n", strerror(errno));
    } else {
        mln_log(error, "Shouldn't be here.\n");
        abort();
    }
}

void worker_send_handler(mln_event_t *ev, int fd, void *data)
{
    int ret;
    mln_tcp_connection_t *c = (mln_tcp_connection_t *)data;
    if ((ret = mln_tcp_connection_send(c)) == M_C_FINISH) {
        mln_tcp_connection_clr_buf(c, M_C_SEND);
        mln_log(report, "Send succeed!\n");
    } else if (ret == M_C_NOTYET) {
        if (mln_event_set_fd(ev, \
                             fd, \
                             M_EV_SEND|M_EV_APPEND|M_EV_ONESHOT, \
                             M_EV_UNLIMITED, \
                             c, \
                             worker_send_handler) < 0)
        {
            mln_log(error, "mln_event_set_fd() failed.\n");
            abort();
        }
    } else if (ret == M_C_ERROR) {
        mln_log(error, "mln_tcp_connection_send error. %s\n", strerror(errno));
    } else {
        mln_log(error, "Shouldn't be here.\n");
        abort();
    }
}

void test_master(mln_event_t *ev, void *f_ptr, void *buf, mln_u32_t len, void **udata_ptr)
{
    mln_fork_t *f = (mln_fork_t *)f_ptr;
    mln_tcp_connection_t *conn = &(f->conn);
    if (strncmp(send_buffer, (char *)buf, len)) {
        mln_log(error, "Content incorrect. [%s]\n", (char *)buf);
        abort();
    }
    mln_log(report, "Parent ipc received!\n");
    char b[32] = {0};
    mln_u32_t le = sizeof(mln_u32_t) + len;
    memcpy(b, &le, sizeof(mln_u32_t));
    mln_u32_t type = M_IPC_1;
    memcpy(b+sizeof(mln_u32_t), &type, sizeof(mln_u32_t));
    memcpy(b+2*sizeof(mln_u32_t), buf, len);
    mln_tcp_connection_set_buf(conn, b, len+2*sizeof(mln_u32_t), M_C_SEND, 0);
    if (mln_event_set_fd(ev, \
                         conn->fd, \
                         M_EV_SEND|M_EV_APPEND|M_EV_ONESHOT, \
                         M_EV_UNLIMITED, \
                         f, \
                         master_send_handler) < 0)
    {
        mln_log(error, "mln_event_set_fd() failed.\n");
        abort();
    }
}

void test_worker(mln_event_t *ev, void *c, void *buf, mln_u32_t len, void **udata_ptr)
{
    mln_tcp_connection_t *conn = (mln_tcp_connection_t *)c;
    if (strncmp(send_buffer, (char *)buf, len)) {
        mln_log(error, "Content incorrect. [%s]\n", (char *)buf);
        abort();
    }
    mln_log(report, "Child ipc received!\n");
    char b[32] = {0};
    mln_u32_t le = sizeof(mln_u32_t) + len;
    memcpy(b, &le, sizeof(mln_u32_t));
    mln_u32_t type = M_IPC_1;
    memcpy(b+sizeof(mln_u32_t), &type, sizeof(mln_u32_t));
    memcpy(b+2*sizeof(mln_u32_t), buf, len);
    mln_tcp_connection_set_buf(conn, b, len+2*sizeof(mln_u32_t), M_C_SEND, 0);
    if (mln_event_set_fd(ev, \
                         conn->fd, \
                         M_EV_SEND|M_EV_APPEND|M_EV_ONESHOT, \
                         M_EV_UNLIMITED, \
                         conn, \
                         worker_send_handler) < 0)
    {
        mln_log(error, "mln_event_set_fd() failed.\n");
        abort();
    }
}


mln_ipc_handler_t ipc_master_handlers[] = {
{conf_reload_master, NULL, M_IPC_CONF_RELOAD},
{distri_init_master, NULL, M_IPC_DISTRIBUTE_INIT},
{test_master, NULL, M_IPC_1}
};
mln_ipc_handler_t ipc_worker_handlers[] = {
{conf_reload_worker, NULL, M_IPC_CONF_RELOAD},
{distri_init_worker, NULL, M_IPC_DISTRIBUTE_INIT},
{test_worker, NULL, M_IPC_1}
};
void mln_set_ipc_handlers(void)
{
    int i;
    for (i = 0; i < sizeof(ipc_master_handlers)/sizeof(mln_ipc_handler_t); i++) {
        mln_set_master_ipc_handler(&ipc_master_handlers[i]);
    }
    for (i = 0; i < sizeof(ipc_worker_handlers)/sizeof(mln_ipc_handler_t); i++) {
        mln_set_worker_ipc_handler(&ipc_worker_handlers[i]);
    }
}
