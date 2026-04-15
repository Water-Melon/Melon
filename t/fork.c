#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/time.h>
#include <unistd.h>
#include "mln_fork.h"
#include "mln_ipc.h"
#include "mln_conf.h"
#include "mln_log.h"
#include "mln_framework.h"

/*
 * Test IPC handler hash table performance.
 * Register many handlers then do lookups via message exchange.
 */

mln_string_t me_framework_mode = mln_string("multiprocess");
mln_conf_item_t me_framework_conf = {CONF_STR, .val.s=&me_framework_mode};
mln_conf_item_t me_workerproc_conf = {CONF_INT, .val.i=1};

static int master_recv_count = 0;
#define TEST_MSG_TYPE 1025
#define TEST_MSG_TYPE2 1026
#define PERF_MSG_COUNT 100

/*
 * Handler for TEST_MSG_TYPE
 */
static void master_handler_type1(mln_event_t *ev, void *f_ptr, void *buf, mln_u32_t len, void **pdata)
{
    ++master_recv_count;
    mln_log(debug, "master handler_type1 recv count=%d: [%s]\n", master_recv_count, (char *)buf);
    if (master_recv_count >= PERF_MSG_COUNT) {
        mln_log(debug, "All %d messages received by master.\n", PERF_MSG_COUNT);
        exit(0);
    }
}

static void worker_handler_type1(mln_event_t *ev, void *f_ptr, void *buf, mln_u32_t len, void **pdata)
{
    mln_log(debug, "worker handler_type1 received: [%s]\n", (char *)buf);
    /* Send back PERF_MSG_COUNT responses */
    int i;
    for (i = 0; i < PERF_MSG_COUNT; ++i) {
        char reply[32];
        snprintf(reply, sizeof(reply), "reply_%d", i);
        assert(mln_ipc_worker_send_prepare(ev, TEST_MSG_TYPE, reply, strlen(reply) + 1) == 0);
    }
}

/*
 * Handler for TEST_MSG_TYPE2
 */
static void master_handler_type2(mln_event_t *ev, void *f_ptr, void *buf, mln_u32_t len, void **pdata)
{
    mln_log(debug, "master handler_type2 received: [%s]\n", (char *)buf);
}

static void worker_handler_type2(mln_event_t *ev, void *f_ptr, void *buf, mln_u32_t len, void **pdata)
{
    mln_log(debug, "worker handler_type2 received: [%s]\n", (char *)buf);
}

/*
 * Handler registration performance test
 */
static void test_handler_registration_perf(void)
{
    struct timeval start, end;
    mln_ipc_cb_t *cbs[256];
    int i;

    gettimeofday(&start, NULL);
    for (i = 0; i < 256; ++i) {
        cbs[i] = mln_ipc_handler_register(2000 + i,
            master_handler_type1, worker_handler_type1, NULL, NULL);
        assert(cbs[i] != NULL);
    }
    gettimeofday(&end, NULL);
    long reg_us = (end.tv_sec - start.tv_sec) * 1000000L + (end.tv_usec - start.tv_usec);

    gettimeofday(&start, NULL);
    for (i = 0; i < 256; ++i) {
        mln_ipc_handler_unregister(cbs[i]);
    }
    gettimeofday(&end, NULL);
    long unreg_us = (end.tv_sec - start.tv_sec) * 1000000L + (end.tv_usec - start.tv_usec);

    printf("Fork handler perf: 256 register=%ld us, 256 unregister=%ld us\n",
           reg_us, unreg_us);
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

    /* Register IPC handlers for two types */
    assert(mln_ipc_handler_register(TEST_MSG_TYPE,
        master_handler_type1, worker_handler_type1, NULL, NULL) != NULL);
    assert(mln_ipc_handler_register(TEST_MSG_TYPE2,
        master_handler_type2, worker_handler_type2, NULL, NULL) != NULL);

    /* Performance test */
    test_handler_registration_perf();

    return 0;
}

static int child_iterator(mln_event_t *ev, mln_fork_t *child, void *data)
{
    /* Send trigger message to worker */
    return mln_ipc_master_send_prepare(ev, TEST_MSG_TYPE, "start", 6, child);
}

static void master_process_handler(mln_event_t *ev)
{
    /* Test: iterate all workers and send messages */
    assert(mln_fork_iterate(ev, child_iterator, NULL) == 0);

    /* Test: master connection accessor */
    mln_tcp_conn_t *conn = mln_fork_master_connection_get();
    assert(conn != NULL);
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
    cattr.master_process = master_process_handler;
    cattr.worker_process = NULL;
    return mln_framework_init(&cattr);
}
