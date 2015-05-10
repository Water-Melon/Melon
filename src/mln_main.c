
/*
 * Copyright (C) Niklaus F.Schen.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include "mln_types.h"
#include "mln_global.h"
#include "mln_resource.h"
#include "mln_tools.h"
#include "mln_log.h"
#include "mln_string.h"
#include "mln_conf.h"
#include "mln_fork.h"
#include "mln_process.h"
#include "mln_thread.h"


static void mln_get_root(void);
static int mln_cmdline(int argc, char *argv[]);
static void output_version(void);
static void output_help(void);
static void mln_master_routine(void);
static void mln_worker_routine(void);
static void mln_sig_conf_reload(mln_event_t *ev, int signo, void *data);
static void mln_conf_reload_send(mln_event_t *ev, int fd, void *data);

int main(int argc, char *argv[])
{
    mln_global_init();
    mln_get_root();
    if (mln_cmdline(argc, argv) < 0)
         return -1;

    /*Modify system limitations*/
    mln_log(none, "Init system resources\n");
    if (mln_unlimit_memory() < 0) {
        return -1;
    }
    if (mln_cancel_core() < 0) {
        return -1;
    }
    if (mln_unlimit_fd() < 0) {
        return -1;
    }

    /*Init configurations*/
    mln_log(none, "Init configurations\n");
    if (mln_conf_load() < 0) {
        return -1;
    }
    if (mln_conf_set_hook(mln_log_reload, NULL) < 0) {
        return -1;
    }

    mln_log(none, "Change log output level\n");
    if (mln_log_set_level() < 0) {
        return -1;
    }

    /*Init user and group*/
    mln_log(none, "Change user to 'melon'\n");
    if (mln_set_id() < 0) {
        return -1;
    }

    /*Init Melon resources*/
    mln_log(none, "Init Melon resources\n");
    if (mln_init_all_resource() < 0) {
        return -1;
    }

    /*fork*/
    if (mln_pre_fork() < 0) {
        return -1;
    }
    int is_master = do_fork();
    if (is_master < 0) {
        return -1;
    }

    if (is_master) {
        mln_master_routine();
        goto chl;
    } else {
chl:
        mln_worker_routine();
    }
    return 0;
}

static void mln_master_routine(void)
{
    mln_event_t *ev = mln_event_init(1);
    if (ev == NULL) exit(1);
    mln_fork_master_set_events(ev);
    if (mln_event_set_signal(ev, M_EV_SET, SIGUSR2, NULL, mln_sig_conf_reload) < 0) {
        mln_log(error, "mln_event_set_signal() failed.\n");
        exit(1);
    }
    mln_dispatch(ev);
    mln_event_destroy(ev);
}

static void mln_worker_routine(void)
{
    mln_event_t *ev = mln_event_init(1);
    if (ev == NULL) exit(1);
    mln_fork_worker_set_events(ev);

    /* mln_process or mln_thread*/
    char thread_mode[] = "thread_mode";
    int i_thread_mode = 0;
    mln_conf_t *cf = mln_get_conf();
    if (cf == NULL) {
        mln_log(error, "Configuration crashed.\n");
        abort();
    }
    mln_conf_domain_t *cd = cf->search(cf, "main");
    if (cd == NULL) {
        mln_log(error, "Domain 'main' NOT existed.\n");
        abort();
    }
    mln_conf_cmd_t *cc = cd->search(cd, thread_mode);
    if (cc != NULL) {
        mln_conf_item_t *ci = cc->search(cc, 1);
        if (ci == NULL || ci->type != CONF_BOOL) {
            mln_log(error, "Invalid item of command '%s'.\n", thread_mode);
            exit(1);
        }
        i_thread_mode = ci->val.b;
    }

    if (i_thread_mode) {
        if (mln_load_thread(ev) < 0)
            exit(1);
        mln_dispatch(ev);
    } else {
        mln_worker_process(ev);
    }
}

static void mln_get_root(void)
{
    if (getuid() == 0) return;
    uid_t euid = geteuid();
    if (euid != 0) {
        mln_log(error, "Not running as 'root'.\n");
        exit(1);
    }
    if (setuid(euid) < 0) {
        mln_log(error, "Set uid failed. %s\n", strerror(errno));
        exit(1);
    }
}

static int mln_cmdline(int argc, char *argv[])
{
    int daemon = 0, version = 0;
    char **p = &argv[1];
    for (; *p != NULL; p++) {
        if (!strncasecmp(*p, "daemon", 6)) {
            daemon = 1;
            continue;
        }
        if (!strncasecmp(*p, "version", 7)) {
            version = 1;
            continue;
        }
        fprintf(stderr, "Invalid No.%d parameter \"%s\".\n", \
                (int)(p - argv), *p);
        output_help();
        return -1;
    }
    if (version) {
        output_version();
    }
    if (mln_log_init(daemon) < 0) {
        fprintf(stderr, "mln_log_init failed.\n");
        return -1;
    }
    if (daemon) {
        mln_daemon();
        mln_close_terminal();
        return 1;
    }
    return 0;
}

static void output_version(void)
{
    printf("Melon Platform.\n");
    printf("Version 1.2.3.\n");
    printf("Copyright (C) Niklaus F.Schen (Chinese name: Shen Fanchen).\n");
    printf("\n");
}

static void output_help(void)
{
    output_version();
    printf("Boot parameters:\n");
    printf("\tdaemon\t\t\trun as daemon\n");
    printf("\tversion\t\t\tshow version\n");
}

static void mln_sig_conf_reload(mln_event_t *ev, int signo, void *data)
{
    if (mln_fork_scan_all(ev, \
                          M_EV_SEND|M_EV_APPEND|M_EV_ONESHOT, \
                          M_EV_UNLIMITED, \
                          mln_conf_reload_send) < 0)
    {
        mln_log(error, "mln_fork_scan() failed.\n");
        return;
    }
    if (mln_conf_reload() < 0) {
        mln_log(error, "mln_conf_reload() failed.\n");
        exit(1);
    }
}

static void mln_conf_reload_send(mln_event_t *ev, int fd, void *data)
{
    char msg[] = "conf_reload";
    char buf[128] = {0};
    mln_u32_t type = M_IPC_CONF_RELOAD;
    mln_u32_t len = sizeof(type) + sizeof(msg) - 1;
    memcpy(buf, &len, sizeof(len));
    memcpy(buf+sizeof(len), &type, sizeof(type));
    memcpy(buf+sizeof(len)*2, msg, sizeof(msg)-1);
    mln_fork_t *f = (mln_fork_t *)data;
    mln_tcp_connection_t *c = &(f->conn);
    mln_tcp_connection_set_buf(c, buf, len+sizeof(len), M_C_SEND, 1);
    int ret = mln_tcp_connection_send(c);
    if (ret == M_C_FINISH) {
        mln_tcp_connection_clr_buf(c, M_C_SEND);
    } else if (ret == M_C_NOTYET) {
        if (mln_event_set_fd(ev, \
                             fd, \
                             M_EV_SEND|M_EV_APPEND|M_EV_ONESHOT, \
                             M_EV_UNLIMITED, \
                             c, \
                             mln_conf_reload_send) < 0)
        {
            mln_log(error, "mln_event_set_fd() failed.\n");
            abort();/*shouldn't be here.*/
        }
    } else if (ret == M_C_ERROR) {
        mln_log(error, "mln_tcp_connection_send() failed. %s\n", strerror(errno));
    } else {
        mln_log(error, "Shouldn't be here.\n");
        abort();
    }
}

