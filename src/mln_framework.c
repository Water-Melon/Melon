
/*
 * Copyright (C) Niklaus F.Schen.
 */
#include "mln_framework.h"
#include "mln_thread.h"
#include "mln_fork.h"
#include "mln_types.h"
#include "mln_tools.h"
#include "mln_log.h"
#include "mln_string.h"
#include "mln_conf.h"
#include "mln_trace.h"
#include "mln_func.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#if !defined(MSVC)
#include <unistd.h>

static int mln_master_trace_init(mln_lang_ctx_t *ctx);
static void mln_worker_routine(struct mln_framework_attr *attr);
static void mln_master_routine(struct mln_framework_attr *attr);
static mln_string_t *mln_get_framework_status(void);
static void mln_sig_conf_reload(int signo);
static int mln_conf_reload_iterate_handler(mln_event_t *ev, mln_fork_t *f, void *data);

static mln_event_t *_ev = NULL;
#endif


int mln_framework_init(struct mln_framework_attr *attr)
{
    /*Init configurations*/
    if (mln_conf_load() < 0) {
        return -1;
    }
    if (mln_conf_hook_set(mln_log_reload, NULL) == NULL) {
        return -1;
    }

    /*Init Melon resources*/
    if (attr->global_init != NULL && attr->global_init() < 0)
        return -1;

#if !defined(MSVC)
    if (mln_get_framework_status()) {
        if (mln_boot_params(attr->argc, attr->argv) < 0)
            return -1;

        /*Modify system limitations*/
        if (mln_sys_limit_modify() < 0) {
            return -1;
        }

        /*daemon*/
        if (mln_daemon() < 0) {
            fprintf(stderr, "Melon boot up failed.\n");
            return -1;
        }

        /*fork*/
        if (mln_fork_prepare() < 0) {
            return -1;
        }
        int is_master = mln_fork();
        if (is_master < 0) {
            return -1;
        }

        if (is_master) {
            mln_master_routine(attr);
            goto chl;
        } else {
chl:
            mln_worker_routine(attr);
        }
    } else {
#endif
        if (mln_log_init(NULL) < 0) return -1;
#if !defined(MSVC)
    }
#endif
    return 0;
}

#if !defined(MSVC)
MLN_FUNC_VOID(static, void, mln_master_routine, (struct mln_framework_attr *attr), (attr), {
    mln_event_t *ev = mln_event_new();
    if (ev == NULL) exit(1);
    if (_ev == NULL) _ev = ev;

    mln_trace_init_callback_set(mln_master_trace_init);
    mln_trace_init(ev, mln_trace_path());
    mln_fork_master_events_set(ev);
    mln_event_signal_set(SIGUSR2, mln_sig_conf_reload);
    if (attr->master_process != NULL) attr->master_process(ev);
    mln_event_dispatch(ev);
    mln_event_free(ev);
})

MLN_FUNC_VOID(static, void, mln_worker_routine, (struct mln_framework_attr *attr), (attr), {
    int i_thread_mode;
    mln_string_t proc_mode = mln_string("multiprocess");
    mln_string_t *framework_mode = mln_get_framework_status();
    mln_event_t *ev = mln_event_new();
    if (ev == NULL) exit(1);
    if (_ev == NULL) _ev = ev;
    mln_fork_worker_events_set(ev);

    if (!mln_string_strcmp(framework_mode, &proc_mode)) {
        i_thread_mode = 0;
    } else {
        i_thread_mode = 1;
    }

    mln_trace_init(ev, mln_trace_path());
    if (i_thread_mode) {
        if (mln_load_thread(ev) < 0)
            exit(1);
        if (attr->main_thread != NULL) attr->main_thread(ev);
        mln_event_dispatch(ev);
    } else {
        if (attr->worker_process != NULL) attr->worker_process(ev);
        mln_event_dispatch(ev);
    }
})

MLN_FUNC(static, int, mln_master_trace_init, (mln_lang_ctx_t *ctx), (ctx), {
    mln_u8_t master = 1;
    mln_string_t *dup, name = mln_string("MASTER");
    if ((dup = mln_string_pool_dup(ctx->pool, &name)) == NULL)
        return -1;
    if (mln_lang_ctx_global_var_add(ctx, dup, &master, M_LANG_VAL_TYPE_BOOL) < 0) {
        mln_string_free(dup);
        return -1;
    }
    return 0;
})

MLN_FUNC_VOID(static, void, mln_sig_conf_reload, (int signo), (signo), {
    if (_ev == NULL) return;

    if (mln_fork_iterate(_ev, mln_conf_reload_iterate_handler, NULL) < 0) {
        mln_log(error, "mln_fork_scan() failed.\n");
        return;
    }
    if (mln_conf_reload() < 0) {
        mln_log(error, "mln_conf_reload() failed.\n");
        exit(1);
    }
})

MLN_FUNC(static, int, mln_conf_reload_iterate_handler, \
         (mln_event_t *ev, mln_fork_t *f, void *data), (ev, f, data), \
{
    char msg[] = "conf_reload";

    return mln_ipc_master_send_prepare(ev, M_IPC_TYPE_CONF, msg, sizeof(msg)-1, f);
})

MLN_FUNC(static, mln_string_t *, mln_get_framework_status, (void), (), {
    char framework[] = "framework";
    mln_string_t proc_mode = mln_string("multiprocess");
    mln_string_t thread_mode = mln_string("multithread");
    mln_conf_t *cf = mln_conf();
    if (cf == NULL) {
        fprintf(stderr, "Configuration crashed.\n");
        abort();
    }
    mln_conf_domain_t *cd = cf->search(cf, "main");
    if (cd == NULL) {
        fprintf(stderr, "Domain 'main' NOT existed.\n");
        abort();
    }
    mln_conf_cmd_t *cc = cd->search(cd, framework);
    if (cc != NULL) {
        mln_conf_item_t *ci = cc->search(cc, 1);
        if (ci == NULL) {
            fprintf(stderr, "Invalid item of command '%s'.\n", framework);
            exit(1);
        }
        if (ci->type == CONF_STR) {
            if (mln_string_strcmp(ci->val.s, &proc_mode) && mln_string_strcmp(ci->val.s, &thread_mode)) {
                fprintf(stderr, "Invalid framework mode '%s'.\n", (char *)(ci->val.s->data));
                exit(1);
            }
            return ci->val.s;
        }
        if  (ci->type == CONF_BOOL && ci->val.b) {
            fprintf(stderr, "Invalid item of command '%s'.\n", framework);
            exit(1);
        }
        return NULL;
    }
    return NULL;
})
#endif

