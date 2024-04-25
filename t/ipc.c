#include <stdio.h>
#include <assert.h>
#include "mln_framework.h"
#include "mln_ipc.h"
#include "mln_conf.h"
#include "mln_log.h"

mln_string_t me_framework_mode = mln_string("multiprocess");
mln_conf_item_t me_framework_conf = {CONF_STR, .val.s=&me_framework_mode};
mln_conf_item_t me_workerproc_conf = {CONF_INT, .val.i=1};
mln_ipc_cb_t *ipc_cb;

static void master_ipc_handler(mln_event_t *ev, void *f_ptr, void *buf, mln_u32_t len, void **pdata)
{
    mln_log(debug, "received: [%s]\n", (char *)buf);
    exit(0);
}

static void worker_ipc_timer_handler(mln_event_t *ev, void *data)
{
    exit(0);
}

static void worker_ipc_handler(mln_event_t *ev, void *f_ptr, void *buf, mln_u32_t len, void **pdata)
{
    mln_log(debug, "received: [%s]\n", (char *)buf);
    assert(mln_ipc_worker_send_prepare(ev, 255, "bcd", 4) == 0);
    assert(mln_event_timer_set(ev, 3000, NULL, worker_ipc_timer_handler) != NULL);
}

static int global_init(void)
{
    mln_conf_t *cf;
    mln_conf_domain_t *cd;
    mln_conf_cmd_t *cc;

    cf = mln_conf();
    cd = cf->search(cf, "main");
    cc = cd->search(cd, "framework");
    if (cc == NULL) {
        assert((cc = cd->insert(cd, "framework")) != NULL);
    }
    assert(cc->update(cc, &me_framework_conf, 1) == 0);

    cc = cd->search(cd, "worker_proc");
    if (cc == NULL) {
        assert((cc = cd->insert(cd, "worker_proc")) != NULL);
    }
    assert(cc->update(cc, &me_workerproc_conf, 1) == 0);

    assert((ipc_cb = mln_ipc_handler_register(255, master_ipc_handler, worker_ipc_handler, NULL, NULL)) != NULL);
    return 0;
}

static int child_iterator(mln_event_t *ev, mln_fork_t *child, void *data)
{
    return mln_ipc_master_send_prepare(ev, 255, "abc", 4, child);
}

static void master_handler(mln_event_t *ev)
{
    assert(mln_fork_iterate(ev, child_iterator, NULL) == 0);
}

int main(int argc, char *argv[])
{
    printf("NOTE: This test has memory leak because we don't release memory of log, conf and multiprocess-related stuffs.\n");
    printf("In fact, `mln_framework_init` should be the last function called in `main`, so we don't need to release anything.\n");

    struct mln_framework_attr cattr;
    cattr.argc = argc;
    cattr.argv = argv;
    cattr.global_init = global_init;
    cattr.main_thread = NULL;
    cattr.master_process = master_handler;
    cattr.worker_process = NULL;
    return mln_framework_init(&cattr);
}
