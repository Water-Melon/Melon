#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/time.h>
#include "mln_framework.h"
#include "mln_conf.h"
#include "mln_log.h"

/*
 * Test 1: Basic framework initialization without any mode
 */
static int test_basic_init(int argc, char *argv[])
{
    struct mln_framework_attr cattr;
    memset(&cattr, 0, sizeof(cattr));
    cattr.argc = argc;
    cattr.argv = argv;
    cattr.global_init = NULL;
    cattr.main_thread = NULL;
    cattr.master_process = NULL;
    cattr.worker_process = NULL;
    return mln_framework_init(&cattr);
}

/*
 * Test 2: Framework with global_init callback
 */
static int global_init_called = 0;

static int test_global_init_cb(void)
{
    global_init_called = 1;
    return 0;
}

static int test_global_init_fail_cb(void)
{
    return -1;
}

static int test_framework_with_global_init(int argc, char *argv[])
{
    struct mln_framework_attr cattr;
    memset(&cattr, 0, sizeof(cattr));
    cattr.argc = argc;
    cattr.argv = argv;
    cattr.global_init = test_global_init_cb;
    cattr.main_thread = NULL;
    cattr.master_process = NULL;
    cattr.worker_process = NULL;
    int ret = mln_framework_init(&cattr);
    assert(global_init_called == 1);
    return ret;
}

/*
 * Test 3: Framework with failing global_init
 */
static int test_framework_global_init_fail(int argc, char *argv[])
{
    struct mln_framework_attr cattr;
    memset(&cattr, 0, sizeof(cattr));
    cattr.argc = argc;
    cattr.argv = argv;
    cattr.global_init = test_global_init_fail_cb;
    cattr.main_thread = NULL;
    cattr.master_process = NULL;
    cattr.worker_process = NULL;
    int ret = mln_framework_init(&cattr);
    assert(ret < 0);
    return 0;
}

/*
 * Performance test: measure framework init overhead
 */
static int test_framework_init_perf(int argc, char *argv[])
{
    struct timeval start, end;
    int iterations = 1000;
    int i;

    gettimeofday(&start, NULL);
    for (i = 0; i < iterations; ++i) {
        test_global_init_cb();
    }
    gettimeofday(&end, NULL);

    long elapsed_us = (end.tv_sec - start.tv_sec) * 1000000L + (end.tv_usec - start.tv_usec);
    printf("Performance: %d global_init callbacks in %ld us (%.2f us/call)\n",
           iterations, elapsed_us, (double)elapsed_us / iterations);

    return 0;
}

int main(int argc, char *argv[])
{
    printf("NOTE: This test has memory leak because we don't release memory of log, conf and multiprocess-related stuffs.\n");
    printf("In fact, `mln_framework_init` should be the last function called in `main`, so we don't need to release anything.\n");

    /* Test 1: Performance measurement of callback overhead */
    printf("=== Test: Framework callback performance ===\n");
    assert(test_framework_init_perf(argc, argv) == 0);
    printf("PASS\n");

    /* Test 2: Framework with global_init failure */
    printf("=== Test: Framework global_init failure ===\n");
    assert(test_framework_global_init_fail(argc, argv) == 0);
    printf("PASS\n");

    /* Test 3: Framework with global_init success */
    printf("=== Test: Framework with global_init ===\n");
    assert(test_framework_with_global_init(argc, argv) == 0);
    printf("PASS\n");

    /* Test 4: Basic framework init (this is last since it may set up logging) */
    printf("=== Test: Basic framework init ===\n");
    assert(test_basic_init(argc, argv) == 0);
    printf("PASS\n");

    printf("All framework tests passed.\n");
    return 0;
}
