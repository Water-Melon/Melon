#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/time.h>
#include <unistd.h>
#include "mln_trace.h"
#include "mln_conf.h"
#include "mln_log.h"
#include "mln_framework.h"

/*
 * Test trace module: path lookup, init callback, finalize and script interaction.
 */

static int trace_init_cb_called = 0;
static int trace_script_recv_called = 0;
static int trace_script_recv_ok = 0;

static int test_trace_init_cb(mln_lang_ctx_t *ctx)
{
    trace_init_cb_called = 1;
    return 0;
}

static int test_trace_recv_cb(mln_lang_ctx_t *ctx, mln_lang_val_t *val)
{
    trace_script_recv_called = 1;
    if (val != NULL &&
        val->type == M_LANG_VAL_TYPE_STRING &&
        val->data.s != NULL &&
        val->data.s->len == sizeof("trace-ping") - 1 &&
        !memcmp(val->data.s->data, "trace-ping", sizeof("trace-ping") - 1))
    {
        trace_script_recv_ok = 1;
    }
    mln_event_break_set(mln_lang_event_get(ctx->lang));
    return 0;
}

static void test_trace_timeout_handler(mln_event_t *ev, void *data)
{
    (void)data;
    mln_event_break_set(ev);
}

/*
 * Test 1: trace_path returns NULL when no trace_mode configured
 */
static void test_trace_path_no_config(void)
{
    mln_string_t *path = mln_trace_path();
    /* Without trace_mode in config, should be NULL */
    printf("trace_path (no config): %s\n", path ? "non-NULL" : "NULL");
}

/*
 * Test 2: trace_task_get returns NULL before init
 */
static void test_trace_task_get_before_init(void)
{
    mln_lang_ctx_t *ctx = mln_trace_task_get();
    assert(ctx == NULL);
    printf("trace_task_get before init: NULL (correct)\n");
}

/*
 * Test 3: trace_init with NULL path should fail gracefully
 */
static void test_trace_init_null_path(void)
{
    /* Create a temporary event for testing */
    mln_event_t *ev = mln_event_new();
    assert(ev != NULL);

    int ret = mln_trace_init(ev, NULL);
    assert(ret < 0); /* Should fail with NULL path */
    printf("trace_init(NULL path): returned %d (correct)\n", ret);

    mln_event_free(ev);
}

/*
 * Test 4: trace_init_callback_set
 */
static void test_trace_init_callback_set(void)
{
    mln_trace_init_callback_set(test_trace_init_cb);
    printf("trace_init_callback_set: OK\n");

    /* Reset */
    mln_trace_init_callback_set(NULL);
}

/*
 * Test 5: trace_recv_handler_set before init should fail
 */
static void test_trace_recv_handler_before_init(void)
{
    int ret = mln_trace_recv_handler_set(NULL);
    assert(ret < 0);
    printf("trace_recv_handler_set before init: returned %d (correct)\n", ret);
}

/*
 * Test 6: trace_finalize before init is safe
 */
static void test_trace_finalize_safe(void)
{
    mln_trace_finalize();
    printf("trace_finalize (no prior init): OK (no crash)\n");
}

/*
 * Test 7: C <-> Melang script interaction via trace Pipe
 */
static void test_trace_script_interaction(void)
{
    static char script_path[] = "/tmp/melon_trace_test.m";
    static char script_code[] =
        "Pipe('subscribe');\n"
        "Pipe('send', 'trace-ping');\n"
        "Pipe('unsubscribe');\n";
    FILE *fp;
    mln_string_t path = mln_string(script_path);
    mln_event_t *ev;

    trace_script_recv_called = 0;
    trace_script_recv_ok = 0;

    fp = fopen(script_path, "w");
    assert(fp != NULL);
    assert(fwrite(script_code, 1, sizeof(script_code) - 1, fp) == sizeof(script_code) - 1);
    fclose(fp);

    ev = mln_event_new();
    assert(ev != NULL);
    assert(mln_trace_init(ev, &path) == 0);
    assert(mln_trace_recv_handler_set(test_trace_recv_cb) == 0);
    assert(mln_event_timer_set(ev, 1000, NULL, test_trace_timeout_handler) != NULL);

    mln_event_dispatch(ev);

    assert(trace_script_recv_called == 1);
    assert(trace_script_recv_ok == 1);
    printf("trace script interaction: OK\n");

    mln_trace_finalize();
    mln_event_free(ev);
    unlink(script_path);
}

/*
 * Performance test: measure trace_path caching
 */
static void test_trace_path_cache_perf(void)
{
    struct timeval start, end;
    int iterations = 100000;
    int i;

    gettimeofday(&start, NULL);
    for (i = 0; i < iterations; ++i) {
        mln_trace_path();
    }
    gettimeofday(&end, NULL);

    long elapsed_us = (end.tv_sec - start.tv_sec) * 1000000L + (end.tv_usec - start.tv_usec);
    printf("trace_path cache perf: %d calls in %ld us (%.2f us/call)\n",
           iterations, elapsed_us, (double)elapsed_us / iterations);
}

static int global_init(void)
{
    return 0;
}

int main(int argc, char *argv[])
{
    printf("=== Trace Module Tests ===\n");

    /* Initialize conf first (needed for trace_path) */
    struct mln_framework_attr cattr;
    memset(&cattr, 0, sizeof(cattr));
    cattr.argc = argc;
    cattr.argv = argv;
    cattr.global_init = global_init;
    cattr.main_thread = NULL;
    cattr.master_process = NULL;
    cattr.worker_process = NULL;

    if (mln_framework_init(&cattr) < 0) {
        fprintf(stderr, "Framework init failed.\n");
        return -1;
    }

    /* Run tests */
    test_trace_path_no_config();
    test_trace_task_get_before_init();
    test_trace_init_null_path();
    test_trace_init_callback_set();
    test_trace_recv_handler_before_init();
    test_trace_finalize_safe();
    test_trace_script_interaction();
    test_trace_path_cache_perf();

    printf("All trace tests passed.\n");
    return 0;
}
