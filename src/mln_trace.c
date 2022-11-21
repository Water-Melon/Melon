
/*
 * Copyright (C) Niklaus F.Schen.
 */

#include "mln_conf.h"
#include "mln_log.h"
#include "mln_trace.h"
#include "mln_iothread.h"

static mln_lang_t *trace_lang = NULL;
static mln_lang_ctx_t *trace_ctx = NULL;
static int trace_signal_fds[2];

static inline int mln_trace_enable(mln_string_t **path);
static int mln_trace_lang_signal(mln_lang_t *lang);
static int mln_trace_lang_clear(mln_lang_t *lang);
static void *mln_trace_iothread_entry(void *args);
static void mln_trace_lang_ctx_return_handler(mln_lang_ctx_t *ctx);

int mln_trace_init(void)
{
    int rc;
    mln_event_t *ev;
    mln_iothread_t t;
    mln_string_t *path = NULL;
    struct mln_iothread_attr tattr;

    if ((rc = mln_trace_enable(&path)) <= 0) {
        return rc;
    }

    if (socketpair(AF_UNIX, SOCK_STREAM, 0, trace_signal_fds) < 0) {
        mln_log(error, "event new failed\n");
        goto err1;
    }

    if ((ev = mln_event_new()) == NULL) {
        mln_log(error, "event new failed\n");
        goto err2;
    }

    if ((trace_lang = mln_lang_new(ev, mln_trace_lang_signal, mln_trace_lang_clear)) == NULL) {
        mln_log(error, "trace lang init failed\n");
        goto err3;
    }
    mln_lang_cache_set(trace_lang);

    trace_ctx = mln_lang_job_new(trace_lang, M_INPUT_T_FILE, path, NULL, mln_trace_lang_ctx_return_handler);
    if (trace_ctx == NULL) {
        mln_log(error, "Load trace script failed\n");
        goto err4;
    }

    tattr.nthread = 1;
    tattr.entry = mln_trace_iothread_entry;
    tattr.args = ev;
    tattr.handler = NULL;
    if (mln_iothread_init(&t, &tattr) < 0) {
        mln_log(error, "iothread init failed\n");
        goto err4;
    }

    return 0;

err4:
    mln_lang_free(trace_lang);
err3:
    mln_event_free(ev);
err2:
    close(trace_signal_fds[0]);
    close(trace_signal_fds[1]);
err1:
    return -1;
}

mln_lang_ctx_t *mln_trace_task_get(void)
{
    return trace_ctx;
}

static inline int mln_trace_enable(mln_string_t **path)
{
    mln_conf_t *cf;
    mln_conf_domain_t *cd;
    mln_conf_cmd_t *cc;
    mln_conf_item_t *ci;

    cf = mln_get_conf();
    if (cf == NULL) {
        mln_log(error, "Configuration messed up\n");
        return -1;
    }

    cd = cf->search(cf, "main");
    if (cd == NULL) {
        mln_log(error, "Conf [main] domain not exist\n");
        return -1;
    }

    cc = cd->search(cd, "trace_mode");
    if (cc == NULL) return 0;

    ci = cc->search(cc, 1);
    if (mln_conf_get_narg(cc) != 1 || ci == NULL) {
        mln_log(error, "Invalid argument of configuration [trace_mode]\n");
        return -1;
    }
    if (ci->type == CONF_STR) {
        *path = ci->val.s;
    } else if (ci->type == CONF_BOOL && !ci->val.b) {
        return 0;
    } else {
        mln_log(error, "Invalid argument of configuration [trace_mode]\n");
        return -1;
    }

    return 1;
}

static int mln_trace_lang_signal(mln_lang_t *lang)
{
    return mln_event_set_fd(mln_lang_event_get(lang), trace_signal_fds[0], M_EV_SEND|M_EV_ONESHOT, M_EV_UNLIMITED, lang, mln_lang_launcher_get(lang));
}

static int mln_trace_lang_clear(mln_lang_t *lang)
{
    return mln_event_set_fd(mln_lang_event_get(lang), trace_signal_fds[0], M_EV_CLR, M_EV_UNLIMITED, NULL, NULL);
}

static void *mln_trace_iothread_entry(void *args)
{
    mln_event_dispatch((mln_event_t *)args);
    return NULL;
}

static void mln_trace_lang_ctx_return_handler(mln_lang_ctx_t *ctx)
{
    trace_ctx = NULL;
    if (mln_lang_ctx_is_quit(ctx))
        mln_log(warn, "trace script exit\n");
}

