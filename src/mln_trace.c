
/*
 * Copyright (C) Niklaus F.Schen.
 */

#include "mln_conf.h"
#include "mln_trace.h"
#include "mln_log.h"

static mln_lang_t *trace_lang = NULL;
static mln_lang_ctx_t *trace_ctx = NULL;
static int trace_signal_fds[2];

static int mln_trace_lang_signal(mln_lang_t *lang);
static int mln_trace_lang_clear(mln_lang_t *lang);
static void mln_trace_lang_ctx_return_handler(mln_lang_ctx_t *ctx);
static void mln_trace_lang_check(mln_event_t *ev, void *data);

mln_string_t *mln_trace_path(void)
{
    mln_conf_t *cf;
    mln_conf_domain_t *cd;
    mln_conf_cmd_t *cc;
    mln_conf_item_t *ci;

    cf = mln_get_conf();
    if (cf == NULL) return NULL;

    cd = cf->search(cf, "main");
    if (cd == NULL) return NULL;

    cc = cd->search(cd, "trace_mode");
    if (cc == NULL) return NULL;

    ci = cc->search(cc, 1);
    if (mln_conf_get_narg(cc) != 1 || ci == NULL) return NULL;

    if (ci->type == CONF_STR)
        return ci->val.s;

    return NULL;
}

int mln_trace_init(mln_event_t *ev, mln_string_t *path)
{
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, trace_signal_fds) < 0) {
        goto err1;
    }

    if ((trace_lang = mln_lang_new(ev, mln_trace_lang_signal, mln_trace_lang_clear)) == NULL) {
        goto err2;
    }
    mln_lang_cache_set(trace_lang);

    trace_ctx = mln_lang_job_new(trace_lang, M_INPUT_T_FILE, path, NULL, mln_trace_lang_ctx_return_handler);
    if (trace_ctx == NULL) {
        goto err3;
    }

    if (mln_event_timer_set(ev, 10, NULL, mln_trace_lang_check) == NULL) {
        goto err3;
    }

    return 0;

err3:
    mln_lang_free(trace_lang);
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

static void mln_trace_lang_check(mln_event_t *ev, void *data)
{
    if (trace_lang != NULL && trace_lang->run_head == NULL && trace_lang->wait_head == NULL) {
        mln_lang_free(trace_lang);
        trace_lang = NULL;
        trace_ctx = NULL;
    } else {
        mln_event_timer_set(ev, 10, NULL, mln_trace_lang_check);
    }
}

static int mln_trace_lang_signal(mln_lang_t *lang)
{
    return mln_event_fd_set(mln_lang_event_get(lang), trace_signal_fds[0], M_EV_SEND|M_EV_ONESHOT, M_EV_UNLIMITED, lang, mln_lang_launcher_get(lang));
}

static int mln_trace_lang_clear(mln_lang_t *lang)
{
    return mln_event_fd_set(mln_lang_event_get(lang), trace_signal_fds[0], M_EV_CLR, M_EV_UNLIMITED, NULL, NULL);
}

static void mln_trace_lang_ctx_return_handler(mln_lang_ctx_t *ctx)
{
    trace_ctx = NULL;
    if (mln_lang_ctx_is_quit(ctx))
        mln_log(warn, "trace script exit\n");
}

