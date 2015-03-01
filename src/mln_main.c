
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


static void mln_get_root(void);
static int mln_cmdline(int argc, char *argv[]);
static void output_version(void);
static void output_help(void);
static void mln_master_routine(void);
static void mln_worker_routine(void);

int main(int argc, char *argv[])
{
    mln_global_init();
    mln_get_root();
    if (mln_cmdline(argc, argv) < 0)
         return -1;

    /*Init system resources*/
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
    if (mln_conf_stack_init() < 0) {
        return -1;
    }
    if (mln_load_conf() < 0) {
        mln_conf_stack_destroy();
        return -1;
    }

    mln_log(none, "Change log output level\n");
    if (mln_set_log_level() < 0) {
        return -1;
    }

    /*Init user and group*/
    mln_log(none, "Change user to 'melon'\n");
    if (mln_set_id() < 0) {
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
    mln_dispatch(ev);
}

static void mln_worker_routine(void)
{
    mln_event_t *ev = mln_event_init(1);
    if (ev == NULL) exit(1);
    mln_fork_worker_set_events(ev);
    if (mln_load_thread(ev) < 0)
        exit(1);
    mln_dispatch(ev);
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
    mln_init_sys_log(daemon);
    mln_init_pid_log();
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
    printf("Version 0.8.1.\n");
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

