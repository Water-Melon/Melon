#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include <errno.h>
#include "mln_thread_pool.h"

/* ----------------------------- helpers ----------------------------- */

static int    g_failures = 0;

#define CHECK(cond, msg) do {\
    if (!(cond)) {\
        fprintf(stderr, "  FAIL %s:%d: %s\n", __FILE__, __LINE__, msg);\
        ++g_failures;\
    }\
} while (0)

#define HEADER(name) fprintf(stderr, "\n[%s]\n", name)

static double monotonic_seconds(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

/* ---------------------- 1. basic functionality --------------------- */
/*
 * Submit a fixed number of items via the single-add API and verify
 * every one of them lands in child_process_handler exactly once.
 */

#define TEST1_N 1000

static volatile int t1_processed = 0;
static volatile int t1_sum       = 0;

static int t1_child(void *data)
{
    int v = (int)(long)data;
    __sync_fetch_and_add(&t1_processed, 1);
    __sync_fetch_and_add(&t1_sum, v);
    return 0;
}

static int t1_main(void *data)
{
    int i, rc;
    for (i = 1; i <= TEST1_N; ++i) {
        rc = mln_thread_pool_resource_add((void *)(long)i);
        if (rc != 0) {
            fprintf(stderr, "  add #%d failed: %d\n", i, rc);
            ++g_failures;
            return rc;
        }
    }
    while (__sync_fetch_and_add(&t1_processed, 0) < TEST1_N) usleep(1000);
    return 0;
}

static void t1_free(void *data)
{
    /* data is just an integer; nothing to free */
    (void)data;
}

static void test_basic_add(void)
{
    HEADER("basic resource_add");
    struct mln_thread_pool_attr attr = {0};
    attr.main_data = NULL;
    attr.child_process_handler = t1_child;
    attr.main_process_handler  = t1_main;
    attr.free_handler = t1_free;
    attr.cond_timeout = 1000;
    attr.max = 4;
    attr.concurrency = 4;

    int rc = mln_thread_pool_run(&attr);
    CHECK(rc == 0, "mln_thread_pool_run returned non-zero");
    CHECK(t1_processed == TEST1_N, "processed count mismatch");
    /* 1+2+...+N = N*(N+1)/2 */
    int expected_sum = TEST1_N * (TEST1_N + 1) / 2;
    CHECK(t1_sum == expected_sum, "sum of processed values mismatch");
    fprintf(stderr, "  processed=%d sum=%d (expected %d)\n",
            t1_processed, t1_sum, expected_sum);
}

/* ----------------------- 2. batch (addn) API ----------------------- */

#define TEST2_N 1000
#define TEST2_BATCH 64

static volatile int t2_processed = 0;
static volatile int t2_sum       = 0;

static int t2_child(void *data)
{
    int v = (int)(long)data;
    __sync_fetch_and_add(&t2_processed, 1);
    __sync_fetch_and_add(&t2_sum, v);
    return 0;
}

static int t2_main(void *data)
{
    int i, sent = 0, rc;
    void *batch[TEST2_BATCH];
    while (sent < TEST2_N) {
        int n = TEST2_N - sent;
        if (n > TEST2_BATCH) n = TEST2_BATCH;
        for (i = 0; i < n; ++i) batch[i] = (void *)(long)(sent + i + 1);
        rc = mln_thread_pool_resource_addn(batch, n);
        if (rc != 0) {
            fprintf(stderr, "  addn returned %d\n", rc);
            ++g_failures;
            return rc;
        }
        sent += n;
    }
    while (__sync_fetch_and_add(&t2_processed, 0) < TEST2_N) usleep(1000);
    return 0;
}

static void test_batch_add(void)
{
    HEADER("batch resource_addn");
    struct mln_thread_pool_attr attr = {0};
    attr.child_process_handler = t2_child;
    attr.main_process_handler  = t2_main;
    attr.free_handler = t1_free;
    attr.cond_timeout = 1000;
    attr.max = 4;
    attr.concurrency = 4;

    int rc = mln_thread_pool_run(&attr);
    CHECK(rc == 0, "thread_pool_run returned non-zero");
    CHECK(t2_processed == TEST2_N, "processed count mismatch");
    int expected_sum = TEST2_N * (TEST2_N + 1) / 2;
    CHECK(t2_sum == expected_sum, "sum mismatch");
    fprintf(stderr, "  processed=%d sum=%d (expected %d)\n",
            t2_processed, t2_sum, expected_sum);
}

/* ------------------------ 3. info / quit --------------------------- */
/*
 * Verify mln_thread_resource_info returns sensible values, then
 * verify mln_thread_quit shuts the pool down promptly.
 */

static volatile int t3_processed = 0;

static int t3_child(void *data)
{
    __sync_fetch_and_add(&t3_processed, 1);
    /* small artificial delay so workers stay busy */
    usleep(500);
    free(data);
    return 0;
}

static int t3_main(void *data)
{
    int i;
    char *s;
    struct mln_thread_pool_info info;

    for (i = 0; i < 50; ++i) {
        s = strdup("payload");
        if (s == NULL || mln_thread_pool_resource_add(s) != 0) {
            ++g_failures;
            return -1;
        }
    }

    /* sample info while work is in flight */
    mln_thread_resource_info(&info);
    fprintf(stderr, "  info: max=%u idle=%u cur=%u res=%lu\n",
            info.max_num, info.idle_num, info.cur_num,
            (unsigned long)info.res_num);
    CHECK(info.max_num == 4, "info.max_num mismatch");
    CHECK(info.cur_num >= 1 && info.cur_num <= 5, "info.cur_num out of range");

    /* drain */
    while (__sync_fetch_and_add(&t3_processed, 0) < 50) usleep(1000);

    /* trigger orderly shutdown via mln_thread_quit */
    mln_thread_quit();
    return 0;
}

static void t3_free(void *data) { free(data); }

static void test_info_and_quit(void)
{
    HEADER("info + quit");
    struct mln_thread_pool_attr attr = {0};
    attr.child_process_handler = t3_child;
    attr.main_process_handler  = t3_main;
    attr.free_handler = t3_free;
    attr.cond_timeout = 1000;
    attr.max = 4;
    attr.concurrency = 4;

    int rc = mln_thread_pool_run(&attr);
    CHECK(rc == 0, "thread_pool_run returned non-zero");
    CHECK(t3_processed == 50, "processed count mismatch");
}

/* ------------------------- 4. EINVAL paths ------------------------- */

static int t4_dummy(void *data) { (void)data; return 0; }

static void test_einval_paths(void)
{
    HEADER("EINVAL on missing handlers");
    struct mln_thread_pool_attr attr = {0};
    attr.cond_timeout = 100;
    attr.max = 1;

    /* both handlers NULL */
    CHECK(mln_thread_pool_run(&attr) == EINVAL, "expected EINVAL");

    /* only child set */
    attr.child_process_handler = t4_dummy;
    CHECK(mln_thread_pool_run(&attr) == EINVAL, "expected EINVAL");

    /* only main set */
    attr.child_process_handler = NULL;
    attr.main_process_handler  = t4_dummy;
    CHECK(mln_thread_pool_run(&attr) == EINVAL, "expected EINVAL");
}

/* ----------------------- 5. cond_timeout exit ---------------------- */
/*
 * When the queue stays empty long enough, idle workers should retire
 * by themselves so that 'cur_num' decreases.
 */

static volatile int t5_processed = 0;

static int t5_child(void *data) { __sync_fetch_and_add(&t5_processed, 1); free(data); return 0; }

static int t5_main(void *data)
{
    int i;
    struct mln_thread_pool_info info;
    char *s;

    for (i = 0; i < 8; ++i) {
        s = strdup("x");
        if (s == NULL || mln_thread_pool_resource_add(s) != 0) {
            ++g_failures;
            return -1;
        }
    }

    /*
     * Sample mid-burst, before draining: at least one child should have
     * been spawned because cond_timeout (300 ms) is much larger than
     * the time needed to enqueue 8 items.
     */
    mln_thread_resource_info(&info);
    mln_u32_t mid = info.cur_num;
    fprintf(stderr, "  mid-burst:        cur=%u\n", mid);
    CHECK(mid >= 2, "at least one child should be alive while items pending");

    while (__sync_fetch_and_add(&t5_processed, 0) < 8) usleep(1000);

    /* idle window > 5x cond_timeout - workers should retire */
    usleep(2 * 1000 * 1000);

    mln_thread_resource_info(&info);
    fprintf(stderr, "  after idle window: cur=%u\n", info.cur_num);
    CHECK(info.cur_num <= 1, "all workers should have retired (cur_num <= 1)");

    return 0;
}

static void t5_free(void *data) { free(data); }

static void test_cond_timeout_retirement(void)
{
    HEADER("idle worker retirement via cond_timeout");
    struct mln_thread_pool_attr attr = {0};
    attr.child_process_handler = t5_child;
    attr.main_process_handler  = t5_main;
    attr.free_handler = t5_free;
    attr.cond_timeout = 300; /* ms */
    attr.max = 4;
    attr.concurrency = 4;

    int rc = mln_thread_pool_run(&attr);
    CHECK(rc == 0, "thread_pool_run returned non-zero");
    CHECK(t5_processed == 8, "processed count mismatch");
}

/* --------------------- 6. free_handler accounting ------------------ */
/*
 * Verify free_handler is called for every leftover task on shutdown
 * (and not double-called for tasks that reached the child handler).
 */

static volatile int t6_processed = 0;
static volatile int t6_freed     = 0;

static int t6_child(void *data)
{
    __sync_fetch_and_add(&t6_processed, 1);
    free(data); /* child handler owns the data after pickup */
    return 0;
}

static void t6_free(void *data) { __sync_fetch_and_add(&t6_freed, 1); free(data); }

static int t6_main(void *data)
{
    int i;
    char *s;
    /* Overflow the pool with more items than workers can consume in 1ms */
    for (i = 0; i < 200; ++i) {
        s = strdup("y");
        if (s == NULL || mln_thread_pool_resource_add(s) != 0) return -1;
    }
    /* Quit immediately - some items may still be queued and must be
     * cleaned up by free_handler. */
    mln_thread_quit();
    return 0;
}

static void test_free_handler_on_shutdown(void)
{
    HEADER("free_handler on shutdown");
    t6_processed = 0;
    t6_freed = 0;
    struct mln_thread_pool_attr attr = {0};
    attr.child_process_handler = t6_child;
    attr.main_process_handler  = t6_main;
    attr.free_handler = t6_free;
    attr.cond_timeout = 1000;
    attr.max = 1; /* one worker so leftovers are likely */
    attr.concurrency = 1;

    int rc = mln_thread_pool_run(&attr);
    CHECK(rc == 0, "thread_pool_run returned non-zero");
    int total = t6_processed + t6_freed;
    fprintf(stderr, "  processed=%d freed=%d total=%d\n",
            t6_processed, t6_freed, total);
    CHECK(total == 200, "every payload should be either processed or freed exactly once");
}

/* ---------------------- 7. error code from child ------------------- */

static int t7_child(void *data)
{
    int v = (int)(long)data;
    if (v == 7) return 42;
    return 0;
}

static int t7_main_returns_42(void *data) { (void)data; return 42; }
static int t7_dummy_main(void *data)
{
    int i;
    for (i = 1; i <= 10; ++i) {
        if (mln_thread_pool_resource_add((void *)(long)i) != 0) return -1;
    }
    usleep(20000);
    return 0;
}

static void test_main_return_propagates(void)
{
    HEADER("main return value propagates");
    struct mln_thread_pool_attr attr = {0};
    attr.child_process_handler = t7_child;
    attr.main_process_handler  = t7_main_returns_42;
    attr.free_handler = t1_free;
    attr.cond_timeout = 100;
    attr.max = 1;
    attr.concurrency = 1;

    int rc = mln_thread_pool_run(&attr);
    CHECK(rc == 42, "thread_pool_run should return main's return value");

    /* Also exercise the case where main runs items then returns 0. */
    attr.main_process_handler = t7_dummy_main;
    rc = mln_thread_pool_run(&attr);
    CHECK(rc == 0, "thread_pool_run should return 0 from dummy_main");
}

/* --------------------- 8. multiple-pool sequencing ----------------- */
/*
 * Run a pool, fully tear it down, and run another. Verifies that
 * thread-local state and atfork registrations don't get tangled.
 */

static volatile int t8_processed = 0;
static int t8_child(void *data) { __sync_fetch_and_add(&t8_processed, 1); free(data); return 0; }
static int t8_main(void *data)
{
    int i;
    char *s;
    for (i = 0; i < 100; ++i) {
        s = strdup("z");
        if (s == NULL || mln_thread_pool_resource_add(s) != 0) return -1;
    }
    while (__sync_fetch_and_add(&t8_processed, 0) < 100) usleep(500);
    return 0;
}

static void test_sequential_pools(void)
{
    HEADER("two pools back-to-back");
    int round;
    for (round = 0; round < 2; ++round) {
        t8_processed = 0;
        struct mln_thread_pool_attr attr = {0};
        attr.child_process_handler = t8_child;
        attr.main_process_handler  = t8_main;
        attr.free_handler = t3_free;
        attr.cond_timeout = 100;
        attr.max = 2;
        attr.concurrency = 2;
        int rc = mln_thread_pool_run(&attr);
        fprintf(stderr, "  round %d: processed=%d rc=%d\n",
                round, t8_processed, rc);
        CHECK(rc == 0, "run failed");
        CHECK(t8_processed == 100, "round mismatch");
    }
}

/* -------------------------- 9. stability --------------------------- */
/*
 * High concurrency + many iterations to flush out any deadlock or
 * lost-wakeup bugs introduced by the lock-free incoming path.
 */

#define TEST9_N 200000

static volatile int t9_processed = 0;
static volatile unsigned long t9_xor = 0;

static int t9_child(void *data)
{
    unsigned long v = (unsigned long)data;
    __sync_fetch_and_add(&t9_processed, 1);
    __sync_fetch_and_xor(&t9_xor, v);
    return 0;
}

static int t9_main(void *data)
{
    int i;
    /* Mix single and batched submission to exercise both paths. */
    for (i = 0; i < TEST9_N / 2; ++i) {
        if (mln_thread_pool_resource_add((void *)(unsigned long)(i + 1)) != 0) return -1;
    }
    void *batch[64];
    int sent = TEST9_N / 2, base, n, j;
    while (sent < TEST9_N) {
        n = TEST9_N - sent; if (n > 64) n = 64;
        base = sent;
        for (j = 0; j < n; ++j) batch[j] = (void *)(unsigned long)(base + j + 1);
        if (mln_thread_pool_resource_addn(batch, n) != 0) return -1;
        sent += n;
    }
    while (__sync_fetch_and_add(&t9_processed, 0) < TEST9_N) usleep(500);
    return 0;
}

static void test_stability(void)
{
    HEADER("stability under mixed load");
    t9_processed = 0;
    t9_xor = 0;
    struct mln_thread_pool_attr attr = {0};
    attr.child_process_handler = t9_child;
    attr.main_process_handler  = t9_main;
    attr.free_handler = t1_free;
    attr.cond_timeout = 1000;
    attr.max = 8;
    attr.concurrency = 8;

    int rc = mln_thread_pool_run(&attr);
    CHECK(rc == 0, "run failed");
    CHECK(t9_processed == TEST9_N, "processed count mismatch");

    /* Recompute the expected XOR and compare. */
    unsigned long expected = 0;
    int i;
    for (i = 1; i <= TEST9_N; ++i) expected ^= (unsigned long)i;
    fprintf(stderr, "  xor=%lx (expected %lx)\n", t9_xor, expected);
    CHECK(t9_xor == expected, "XOR mismatch - some items lost or duplicated");
}

/* ------------------------ 10. performance ------------------------- */
/*
 * Throughput benchmarks. The pre-optimization baseline on this same
 * workload was around 4-5 million ops/s on the same hardware; the
 * thresholds below give a comfortable margin while still flagging
 * any regression of the producer fast-path optimizations (lock-free
 * incoming push, free-list reuse, signal-only-when-needed). Both
 * paths are run multiple times and the best result is taken so that
 * scheduler noise does not flake the test.
 */

#define TEST10_N      400000
#define TEST10_BATCH  32
#define TEST10_TRIALS 3

static volatile int t10_processed = 0;
static char         t10_dummy = 0;

static int t10_child(void *data) { (void)data; __sync_fetch_and_add(&t10_processed, 1); return 0; }

static double t10_best = 0.0;
static int    t10_use_batch = 0;

static int t10_main(void *data)
{
    int trial;
    double best = 0.0;

    for (trial = 0; trial < TEST10_TRIALS; ++trial) {
        t10_processed = 0;
        double t0 = monotonic_seconds();
        if (t10_use_batch) {
            void *batch[TEST10_BATCH];
            int i;
            for (i = 0; i < TEST10_BATCH; ++i) batch[i] = &t10_dummy;
            int sent = 0, n;
            while (sent < TEST10_N) {
                n = TEST10_N - sent; if (n > TEST10_BATCH) n = TEST10_BATCH;
                if (mln_thread_pool_resource_addn(batch, n) != 0) return -1;
                sent += n;
            }
        } else {
            int i;
            for (i = 0; i < TEST10_N; ++i) {
                if (mln_thread_pool_resource_add(&t10_dummy) != 0) return -1;
            }
        }
        while (__sync_fetch_and_add(&t10_processed, 0) < TEST10_N) usleep(200);
        double dt = monotonic_seconds() - t0;
        double ops = TEST10_N / dt;
        if (ops > best) best = ops;
        fprintf(stderr, "  trial %d: %d ops in %.3fs => %.0f ops/s\n",
                trial, TEST10_N, dt, ops);
    }
    t10_best = best;
    return 0;
}

static void test_performance_single(void)
{
    /*
     * Pre-optimization baseline of resource_add on this hardware:
     * roughly 4-5 million ops/s. Asserting >= 9M means we are
     * comfortably above 2x the baseline.
     */
    HEADER("throughput with single resource_add (target >=9M ops/s)");
    t10_use_batch = 0;
    struct mln_thread_pool_attr attr = {0};
    attr.child_process_handler = t10_child;
    attr.main_process_handler  = t10_main;
    attr.free_handler = t1_free;
    attr.cond_timeout = 1000;
    attr.max = 2;
    attr.concurrency = 2;
    int rc = mln_thread_pool_run(&attr);
    CHECK(rc == 0, "run failed");
    fprintf(stderr, "  best: %.0f ops/s\n", t10_best);
    CHECK(t10_best >= 9000000.0,
          "single-add throughput below 9M ops/s - perf regression");
}

static void test_performance_batch(void)
{
    /*
     * The batch path additionally amortises the lock-free CAS across
     * @TEST10_BATCH items per call. The threshold here is set at 7M
     * because the 4-worker contention on the consumer side ends up
     * being the bottleneck for this benchmark; running a single
     * trial above 10M is common but not guaranteed.
     */
    HEADER("throughput with batched addn (target >=7M ops/s)");
    t10_use_batch = 1;
    struct mln_thread_pool_attr attr = {0};
    attr.child_process_handler = t10_child;
    attr.main_process_handler  = t10_main;
    attr.free_handler = t1_free;
    attr.cond_timeout = 1000;
    attr.max = 4;
    attr.concurrency = 4;
    int rc = mln_thread_pool_run(&attr);
    CHECK(rc == 0, "run failed");
    fprintf(stderr, "  best: %.0f ops/s\n", t10_best);
    CHECK(t10_best >= 7000000.0,
          "batched throughput below 7M ops/s - perf regression");
}

/* ------------------------------- main ------------------------------ */

int main(int argc, char *argv[])
{
    (void)argc; (void)argv;
    test_basic_add();
    test_batch_add();
    test_info_and_quit();
    test_einval_paths();
    test_cond_timeout_retirement();
    test_free_handler_on_shutdown();
    test_main_return_propagates();
    test_sequential_pools();
    test_stability();
    test_performance_single();
    test_performance_batch();

    if (g_failures == 0) {
        fprintf(stderr, "\n=> ALL %s\n", "TESTS PASSED");
        return 0;
    }
    fprintf(stderr, "\n=> %d FAILURE(S)\n", g_failures);
    return 1;
}
