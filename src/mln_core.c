
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
#include "mln_tools.h"
#include "mln_log.h"
#include "mln_string.h"
#include "mln_conf.h"
#include "mln_fork.h"
#include "mln_thread.h"
#include "mln_core.h"

static int mln_getFrameworkStatus(void);
static void mln_get_root(void);
static void mln_master_routine(void);
static void mln_worker_routine(struct mln_core_attr *attr);
static void mln_sig_conf_reload(mln_event_t *ev, int signo, void *data);
static int mln_conf_reload_scan_handler(mln_event_t *ev, mln_fork_t *f, void *data);

int mln_core_init(struct mln_core_attr *attr)
{
    /*Init configurations*/
    printf("Init configurations\n");
    if (mln_conf_load() < 0) {
        return -1;
    }
    if (mln_conf_set_hook(mln_log_reload, NULL) == NULL) {
        return -1;
    }

    /*Init Melon resources*/
    printf("Init Melon Global Vars\n");
    if (attr->global_init != NULL && attr->global_init() < 0)
        return -1;

    if (mln_getFrameworkStatus()) {
        mln_get_root();
        if (mln_boot_params(attr->argc, attr->argv) < 0)
            return -1;

        /*Modify system limitations*/
        printf("Modify system limitations\n");
        if (mln_sys_limit_modify() < 0) {
            return -1;
        }

        /*daemon*/
        if (mln_daemon() < 0) {
            fprintf(stderr, "Melon boot up failed.\n");
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
            mln_worker_routine(attr);
        }
    } else {
        mln_conf_t *cf;
        mln_conf_domain_t *cd;
        mln_conf_cmd_t *cc;
        mln_conf_item_t *ci;
        if ((cf = mln_get_conf()) == NULL) {
            fprintf(stderr, "configuration messed up.\n");
            return -1;
        }
        if ((cd = cf->search(cf, "main")) == NULL) {
            fprintf(stderr, "No such domain named 'main'.\n");
            return -1;
        }
        if ((cc = cd->search(cd, "daemon")) == NULL) {
            fprintf(stderr, "No such command named 'daemon'.\n");
            return -1;
        }
        if (mln_conf_get_argNum(cc) != 1) {
            fprintf(stderr, "Invalid command 'daemon'.\n");
            return -1;
        }
        if ((ci = cc->search(cc, 1)) == NULL || ci->type != CONF_BOOL) {
            fprintf(stderr, "Invalid command 'daemon'.\n");
            return -1;
        }
        if (mln_log_init(ci->val.b) < 0) return -1;
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
    mln_event_dispatch(ev);
    mln_event_destroy(ev);
}

static void mln_worker_routine(struct mln_core_attr *attr)
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
        mln_event_dispatch(ev);
    } else {
        if (attr->worker_process != NULL) attr->worker_process(ev);
        mln_event_dispatch(ev);
    }
}

static void mln_get_root(void)
{
    if (getuid() == 0) return;
    uid_t euid = geteuid();
    if (euid != 0) {
        fprintf(stderr, "Not running as 'root'.\n");
        exit(1);
    }
    if (setuid(euid) < 0) {
        fprintf(stderr, "Set uid failed. %s\n", strerror(errno));
        exit(1);
    }
}

static void mln_sig_conf_reload(mln_event_t *ev, int signo, void *data)
{
    if (mln_fork_scan_all(ev, mln_conf_reload_scan_handler, NULL) < 0) {
        mln_log(error, "mln_fork_scan() failed.\n");
        return;
    }
    if (mln_conf_reload() < 0) {
        mln_log(error, "mln_conf_reload() failed.\n");
        exit(1);
    }
}

static int mln_conf_reload_scan_handler(mln_event_t *ev, mln_fork_t *f, void *data)
{
    char msg[] = "conf_reload";

    return mln_ipc_master_send_prepare(ev, M_IPC_CONF_RELOAD, msg, sizeof(msg)-1, f);
}

static int mln_getFrameworkStatus(void)
{
    char framework[] = "framework";
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
    mln_conf_cmd_t *cc = cd->search(cd, framework);
    if (cc != NULL) {
        mln_conf_item_t *ci = cc->search(cc, 1);
        if (ci == NULL || ci->type != CONF_BOOL) {
            mln_log(error, "Invalid item of command '%s'.\n", framework);
            exit(1);
        }
        if (ci->val.b) return 1;
    }
    return 0;
}

