#include "mln_iothread.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/time.h>

static volatile int msg_count = 0;
static volatile int data_check_ok = 1;
static mln_iothread_msg_t * volatile held_msg_ptr = NULL;
static volatile int hold_phase = 0;

static void basic_handler(mln_iothread_t *t, mln_iothread_ep_type_t from, mln_iothread_msg_t *msg)
{
    (void)t; (void)from;
    __sync_fetch_and_add((int *)&msg_count, 1);
}

static void data_handler(mln_iothread_t *t, mln_iothread_ep_type_t from, mln_iothread_msg_t *msg)
{
    int *val = (int *)mln_iothread_msg_data(msg);
    mln_u32_t type = mln_iothread_msg_type(msg);
    (void)t; (void)from;
    if (val == NULL || *val != (int)type)
        data_check_ok = 0;
    __sync_fetch_and_add((int *)&msg_count, 1);
}

static void hold_handler(mln_iothread_t *t, mln_iothread_ep_type_t from, mln_iothread_msg_t *msg)
{
    mln_u32_t type = mln_iothread_msg_type(msg);
    (void)t; (void)from;
    if (type == 99) {
        mln_iothread_msg_hold(msg);
        held_msg_ptr = msg;
        hold_phase = 1;
    }
    __sync_fetch_and_add((int *)&msg_count, 1);
}

static void *recv_entry(void *args)
{
    mln_iothread_t *t = (mln_iothread_t *)args;
    while (1) {
        mln_iothread_recv(t, user_thread);
        usleep(500);
    }
    return NULL;
}

static void *perf_entry(void *args)
{
    mln_iothread_t *t = (mln_iothread_t *)args;
    while (1) {
        mln_iothread_recv(t, user_thread);
    }
    return NULL;
}

static void *hold_entry(void *args)
{
    mln_iothread_t *t = (mln_iothread_t *)args;
    while (1) {
        mln_iothread_recv(t, user_thread);
        if (hold_phase == 1 && held_msg_ptr != NULL) {
            mln_iothread_msg_t *m = (mln_iothread_msg_t *)held_msg_ptr;
            held_msg_ptr = NULL;
            hold_phase = 2;
            usleep(5000);
            mln_iothread_msg_release(m);
        }
        usleep(500);
    }
    return NULL;
}

static long long time_us(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (long long)tv.tv_sec * 1000000LL + tv.tv_usec;
}

/* Test 1: error cases */
static int test_error_cases(void)
{
    mln_iothread_t t;
    if (mln_iothread_init(&t, 0, recv_entry, &t, basic_handler) == 0) return -1;
    if (mln_iothread_init(&t, 1, NULL, &t, basic_handler) == 0) return -1;
    return 0;
}

/* Test 2: init/destroy */
static int test_init_destroy(void)
{
    mln_iothread_t t;
    if (mln_iothread_init(&t, 2, recv_entry, &t, basic_handler) < 0) return -1;
    usleep(10000);
    mln_iothread_destroy(&t);
    return 0;
}

/* Test 3: non-feedback send */
static int test_send_nofeedback(void)
{
    mln_iothread_t t;
    int i, rc;
    msg_count = 0;

    if (mln_iothread_init(&t, 1, recv_entry, &t, basic_handler) < 0) return -1;

    for (i = 0; i < 500; ++i) {
        rc = mln_iothread_send(&t, (mln_u32_t)i, NULL, io_thread, 0);
        if (rc < 0) { mln_iothread_destroy(&t); return -1; }
    }
    usleep(200000);
    mln_iothread_destroy(&t);
    if (msg_count < 500) {
        fprintf(stderr, "  nofeedback: expected 500, got %d\n", msg_count);
        return -1;
    }
    return 0;
}

/* Test 4: feedback send */
static int test_send_feedback(void)
{
    mln_iothread_t t;
    int i, rc;
    msg_count = 0;

    if (mln_iothread_init(&t, 1, recv_entry, &t, basic_handler) < 0) return -1;

    for (i = 0; i < 200; ++i) {
        rc = mln_iothread_send(&t, (mln_u32_t)i, NULL, io_thread, 1);
        if (rc < 0) { mln_iothread_destroy(&t); return -1; }
        if (rc > 0) { --i; continue; }
    }
    if (msg_count != 200) {
        fprintf(stderr, "  feedback: expected 200, got %d\n", msg_count);
        mln_iothread_destroy(&t);
        return -1;
    }
    mln_iothread_destroy(&t);
    return 0;
}

/* Test 5: data passing */
static int test_data_passing(void)
{
    mln_iothread_t t;
    int i, rc;
    int values[100];
    msg_count = 0;
    data_check_ok = 1;

    if (mln_iothread_init(&t, 1, recv_entry, &t, data_handler) < 0) return -1;

    for (i = 0; i < 100; ++i) {
        values[i] = i;
        rc = mln_iothread_send(&t, (mln_u32_t)i, &values[i], io_thread, 1);
        if (rc < 0) { mln_iothread_destroy(&t); return -1; }
        if (rc > 0) { --i; continue; }
    }
    mln_iothread_destroy(&t);
    if (!data_check_ok) {
        fprintf(stderr, "  data check failed\n");
        return -1;
    }
    return 0;
}

/* Test 6: hold/release */
static int test_hold_release(void)
{
    mln_iothread_t t;
    int rc;
    hold_phase = 0;
    held_msg_ptr = NULL;
    msg_count = 0;

    if (mln_iothread_init(&t, 1, hold_entry, &t, hold_handler) < 0) return -1;

    rc = mln_iothread_send(&t, 99, NULL, io_thread, 1);
    if (rc < 0) { mln_iothread_destroy(&t); return -1; }

    if (hold_phase < 2) {
        fprintf(stderr, "  hold/release: phase=%d\n", hold_phase);
        mln_iothread_destroy(&t);
        return -1;
    }
    mln_iothread_destroy(&t);
    return 0;
}

/* Test 7: multiple threads */
static int test_multi_thread(void)
{
    mln_iothread_t t;
    int i, rc;
    msg_count = 0;

    if (mln_iothread_init(&t, 4, recv_entry, &t, basic_handler) < 0) return -1;

    for (i = 0; i < 1000; ++i) {
        rc = mln_iothread_send(&t, (mln_u32_t)i, NULL, io_thread, 0);
        if (rc < 0) { mln_iothread_destroy(&t); return -1; }
    }
    usleep(300000);
    mln_iothread_destroy(&t);
    if (msg_count < 1000) {
        fprintf(stderr, "  multi: expected 1000, got %d\n", msg_count);
        return -1;
    }
    return 0;
}

/* Test 8: sockfd_get macro */
static int test_sockfd_get(void)
{
    mln_iothread_t t;
    int iofd, userfd;

    if (mln_iothread_init(&t, 1, recv_entry, &t, basic_handler) < 0) return -1;

    iofd = mln_iothread_sockfd_get(&t, io_thread);
    userfd = mln_iothread_sockfd_get(&t, user_thread);

    if (iofd < 0 || userfd < 0 || iofd == userfd) {
        fprintf(stderr, "  sockfd: io=%d user=%d\n", iofd, userfd);
        mln_iothread_destroy(&t);
        return -1;
    }
    mln_iothread_destroy(&t);
    return 0;
}

/* Test 9: performance benchmark */
static int test_performance(void)
{
    mln_iothread_t t;
    int i, rc;
    long long start, end;
    int count = 100000;

    msg_count = 0;
    if (mln_iothread_init(&t, 1, perf_entry, &t, basic_handler) < 0) return -1;

    /* Feedback benchmark */
    start = time_us();
    for (i = 0; i < count; ++i) {
        rc = mln_iothread_send(&t, (mln_u32_t)i, NULL, io_thread, 1);
        if (rc < 0) { mln_iothread_destroy(&t); return -1; }
        if (rc > 0) { --i; continue; }
    }
    end = time_us();
    printf("\n  feedback:    %d msgs in %lld us (%.0f msgs/sec)\n",
           count, end - start, count * 1000000.0 / (end - start));

    /* Non-feedback benchmark */
    msg_count = 0;
    start = time_us();
    for (i = 0; i < count; ++i) {
        rc = mln_iothread_send(&t, (mln_u32_t)i, NULL, io_thread, 0);
        if (rc < 0) { mln_iothread_destroy(&t); return -1; }
        if (rc > 0) { --i; continue; }
    }
    while (msg_count < count) usleep(100);
    end = time_us();
    printf("  nofeedback:  %d msgs in %lld us (%.0f msgs/sec)\n",
           count, end - start, count * 1000000.0 / (end - start));

    mln_iothread_destroy(&t);
    return 0;
}

/* Test 10: stress test */
static int test_stress(void)
{
    mln_iothread_t t;
    int i, rc;
    int count = 500000;

    msg_count = 0;
    if (mln_iothread_init(&t, 4, perf_entry, &t, basic_handler) < 0) return -1;

    for (i = 0; i < count; ++i) {
        rc = mln_iothread_send(&t, (mln_u32_t)(i & 0xff), NULL, io_thread, (i % 10 == 0) ? 1 : 0);
        if (rc < 0) { mln_iothread_destroy(&t); return -1; }
        if (rc > 0) { --i; continue; }
    }
    while (msg_count < count) usleep(100);

    mln_iothread_destroy(&t);
    if (msg_count != count) {
        fprintf(stderr, "  stress: expected %d, got %d\n", count, msg_count);
        return -1;
    }
    printf("\n  %d messages (mixed feedback/nofeedback) with 4 threads: OK\n", count);
    return 0;
}

int main(void)
{
    struct { const char *name; int (*fn)(void); } tests[] = {
        {"error cases",     test_error_cases},
        {"init/destroy",    test_init_destroy},
        {"send nofeedback", test_send_nofeedback},
        {"send feedback",   test_send_feedback},
        {"data passing",    test_data_passing},
        {"hold/release",    test_hold_release},
        {"multi-thread",    test_multi_thread},
        {"sockfd_get",      test_sockfd_get},
        {"performance",     test_performance},
        {"stress",          test_stress},
    };
    int i, n = sizeof(tests) / sizeof(tests[0]);
    int passed = 0, failed = 0;

    for (i = 0; i < n; ++i) {
        printf("[%2d/%d] %-20s ", i + 1, n, tests[i].name);
        fflush(stdout);
        if (tests[i].fn() == 0) {
            printf("PASS\n");
            ++passed;
        } else {
            printf("FAIL\n");
            ++failed;
        }
    }

    printf("\nResults: %d passed, %d failed, %d total\n", passed, failed, n);
    if (failed) return 1;
    printf("DONE\n");
    return 0;
}
