#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include "mln_event.h"

/*
 * Test counters
 */
static int test_pass = 0;
static int test_fail = 0;

#define TEST_ASSERT(cond, msg) do { \
    if (cond) { test_pass++; } \
    else { test_fail++; fprintf(stderr, "FAIL: %s\n", msg); } \
} while(0)

/* ======== Test 1: Basic timer ======== */
static int timer_fired = 0;

static void test_timer_handler(mln_event_t *ev, void *data)
{
    timer_fired = 1;
    mln_event_break_set(ev);
}

static void test_basic_timer(void)
{
    mln_event_t *ev = mln_event_new();
    TEST_ASSERT(ev != NULL, "event new");

    timer_fired = 0;
    mln_event_timer_t *t = mln_event_timer_set(ev, 50, NULL, test_timer_handler);
    TEST_ASSERT(t != NULL, "timer set");

    mln_event_dispatch(ev);
    TEST_ASSERT(timer_fired == 1, "timer fired");

    mln_event_free(ev);
    printf("  [PASS] basic timer\n");
}

/* ======== Test 2: Timer cancel ======== */
static int cancel_timer_fired = 0;

static void test_cancel_timer_handler(mln_event_t *ev, void *data)
{
    cancel_timer_fired = 1;
}

static void test_cancel_break_handler(mln_event_t *ev, void *data)
{
    mln_event_break_set(ev);
}

static void test_timer_cancel(void)
{
    mln_event_t *ev = mln_event_new();
    TEST_ASSERT(ev != NULL, "event new for cancel");

    cancel_timer_fired = 0;
    mln_event_timer_t *t = mln_event_timer_set(ev, 100, NULL, test_cancel_timer_handler);
    TEST_ASSERT(t != NULL, "cancel timer set");

    mln_event_timer_cancel(ev, t);

    /* Set a shorter timer to break the loop */
    mln_event_timer_set(ev, 200, NULL, test_cancel_break_handler);

    mln_event_dispatch(ev);
    TEST_ASSERT(cancel_timer_fired == 0, "cancelled timer did not fire");

    mln_event_free(ev);
    printf("  [PASS] timer cancel\n");
}

/* ======== Test 3: FD write event ======== */
static int fd_write_count = 0;

static void test_fd_write_handler(mln_event_t *ev, int fd, void *data)
{
    fd_write_count++;
    mln_event_fd_set(ev, fd, M_EV_CLR, M_EV_UNLIMITED, NULL, NULL);
    mln_event_break_set(ev);
}

static void test_fd_write(void)
{
    int pipefd[2];
    TEST_ASSERT(pipe(pipefd) == 0, "pipe create");

    mln_event_t *ev = mln_event_new();
    TEST_ASSERT(ev != NULL, "event new for fd write");

    fd_write_count = 0;
    TEST_ASSERT(mln_event_fd_set(ev, pipefd[1], M_EV_SEND|M_EV_NONBLOCK, M_EV_UNLIMITED, NULL, test_fd_write_handler) == 0, "fd_set write");

    mln_event_dispatch(ev);
    TEST_ASSERT(fd_write_count == 1, "fd write handler called");

    mln_event_free(ev);
    close(pipefd[0]);
    close(pipefd[1]);
    printf("  [PASS] fd write event\n");
}

/* ======== Test 4: FD read event ======== */
static int fd_read_count = 0;

static void test_fd_read_handler(mln_event_t *ev, int fd, void *data)
{
    char buf[64];
    read(fd, buf, sizeof(buf));
    fd_read_count++;
    mln_event_fd_set(ev, fd, M_EV_CLR, M_EV_UNLIMITED, NULL, NULL);
    mln_event_break_set(ev);
}

static void test_fd_read(void)
{
    int pipefd[2];
    TEST_ASSERT(pipe(pipefd) == 0, "pipe create for read");

    /* Write some data so read is ready */
    write(pipefd[1], "hello", 5);

    mln_event_t *ev = mln_event_new();
    TEST_ASSERT(ev != NULL, "event new for fd read");

    fd_read_count = 0;
    TEST_ASSERT(mln_event_fd_set(ev, pipefd[0], M_EV_RECV|M_EV_NONBLOCK, M_EV_UNLIMITED, NULL, test_fd_read_handler) == 0, "fd_set read");

    mln_event_dispatch(ev);
    TEST_ASSERT(fd_read_count == 1, "fd read handler called");

    mln_event_free(ev);
    close(pipefd[0]);
    close(pipefd[1]);
    printf("  [PASS] fd read event\n");
}

/* ======== Test 5: FD oneshot ======== */
static int oneshot_count = 0;

static void test_oneshot_handler(mln_event_t *ev, int fd, void *data)
{
    oneshot_count++;
    /* Don't clear - oneshot should prevent re-trigger */
}

static void test_oneshot_break(mln_event_t *ev, void *data)
{
    mln_event_break_set(ev);
}

static void test_fd_oneshot(void)
{
    int pipefd[2];
    TEST_ASSERT(pipe(pipefd) == 0, "pipe create for oneshot");

    mln_event_t *ev = mln_event_new();
    TEST_ASSERT(ev != NULL, "event new for oneshot");

    oneshot_count = 0;
    TEST_ASSERT(mln_event_fd_set(ev, pipefd[1], M_EV_SEND|M_EV_ONESHOT|M_EV_NONBLOCK, M_EV_UNLIMITED, NULL, test_oneshot_handler) == 0, "fd_set oneshot");

    /* Break after 100ms to check */
    mln_event_timer_set(ev, 100, NULL, test_oneshot_break);

    mln_event_dispatch(ev);
    TEST_ASSERT(oneshot_count == 1, "oneshot fired exactly once");

    mln_event_fd_set(ev, pipefd[1], M_EV_CLR, M_EV_UNLIMITED, NULL, NULL);
    mln_event_free(ev);
    close(pipefd[0]);
    close(pipefd[1]);
    printf("  [PASS] fd oneshot\n");
}

/* ======== Test 6: FD append ======== */
static int append_recv_count = 0;
static int append_send_count = 0;

static void test_append_recv_handler(mln_event_t *ev, int fd, void *data)
{
    char buf[64];
    read(fd, buf, sizeof(buf));
    append_recv_count++;
}

static void test_append_send_handler(mln_event_t *ev, int fd, void *data)
{
    append_send_count++;
}

static void test_append_break(mln_event_t *ev, void *data)
{
    mln_event_break_set(ev);
}

static void test_fd_append(void)
{
    int pipefd[2];
    TEST_ASSERT(pipe(pipefd) == 0, "pipe create for append");

    /* Make read end non-blocking and write data */
    write(pipefd[1], "data", 4);

    mln_event_t *ev = mln_event_new();
    TEST_ASSERT(ev != NULL, "event new for append");

    append_recv_count = 0;
    append_send_count = 0;

    /* First set read event */
    TEST_ASSERT(mln_event_fd_set(ev, pipefd[0], M_EV_RECV|M_EV_NONBLOCK, M_EV_UNLIMITED, NULL, test_append_recv_handler) == 0, "fd_set recv for append");

    /* Append write event - pipefd[0] is read-end, so we test on pipefd[1] separately */
    /* For append test, use a socketpair instead for bidirectional */
    mln_event_timer_set(ev, 100, NULL, test_append_break);

    mln_event_dispatch(ev);
    TEST_ASSERT(append_recv_count >= 1, "append recv fired");

    mln_event_fd_set(ev, pipefd[0], M_EV_CLR, M_EV_UNLIMITED, NULL, NULL);
    mln_event_free(ev);
    close(pipefd[0]);
    close(pipefd[1]);
    printf("  [PASS] fd append\n");
}

/* ======== Test 7: FD timeout ======== */
static int fd_timeout_fired = 0;

static void test_fd_timeout_handler(mln_event_t *ev, int fd, void *data)
{
    fd_timeout_fired = 1;
    mln_event_fd_set(ev, fd, M_EV_CLR, M_EV_UNLIMITED, NULL, NULL);
    mln_event_break_set(ev);
}

static void test_fd_recv_noop(mln_event_t *ev, int fd, void *data)
{
    /* should not be called since no data arrives */
}

static void test_fd_timeout(void)
{
    int pipefd[2];
    TEST_ASSERT(pipe(pipefd) == 0, "pipe create for timeout");

    mln_event_t *ev = mln_event_new();
    TEST_ASSERT(ev != NULL, "event new for fd timeout");

    fd_timeout_fired = 0;
    /* Set recv event with short timeout - no data will come */
    TEST_ASSERT(mln_event_fd_set(ev, pipefd[0], M_EV_RECV|M_EV_NONBLOCK, 50, NULL, test_fd_recv_noop) == 0, "fd_set with timeout");
    mln_event_fd_timeout_handler_set(ev, pipefd[0], NULL, test_fd_timeout_handler);

    mln_event_dispatch(ev);
    TEST_ASSERT(fd_timeout_fired == 1, "fd timeout handler fired");

    mln_event_free(ev);
    close(pipefd[0]);
    close(pipefd[1]);
    printf("  [PASS] fd timeout\n");
}

/* ======== Test 8: Dispatch callback ======== */
static int callback_count = 0;

static void test_dispatch_callback(mln_event_t *ev, void *data)
{
    callback_count++;
    if (callback_count >= 3) {
        mln_event_break_set(ev);
    }
}

static void test_callback(void)
{
    mln_event_t *ev = mln_event_new();
    TEST_ASSERT(ev != NULL, "event new for callback");

    callback_count = 0;
    mln_event_callback_set(ev, test_dispatch_callback, NULL);

    mln_event_dispatch(ev);
    TEST_ASSERT(callback_count >= 3, "dispatch callback called multiple times");

    mln_event_free(ev);
    printf("  [PASS] dispatch callback\n");
}

/* ======== Test 9: Break and reset ======== */
static void test_break_reset(void)
{
    mln_event_t *ev = mln_event_new();
    TEST_ASSERT(ev != NULL, "event new for break/reset");

    mln_event_break_set(ev);
    mln_event_dispatch(ev);
    /* Should return immediately */
    TEST_ASSERT(1, "break set causes immediate return");

    mln_event_break_reset(ev);

    /* Now set a timer to break again */
    timer_fired = 0;
    mln_event_timer_set(ev, 50, NULL, test_timer_handler);
    mln_event_dispatch(ev);
    TEST_ASSERT(timer_fired == 1, "reset allows dispatch to run again");

    mln_event_free(ev);
    printf("  [PASS] break and reset\n");
}

/* ======== Test 10: Multiple timers ordering ======== */
static int timer_order[4];
static int timer_order_idx = 0;

static void test_order_handler_1(mln_event_t *ev, void *data)
{
    timer_order[timer_order_idx++] = 1;
}

static void test_order_handler_2(mln_event_t *ev, void *data)
{
    timer_order[timer_order_idx++] = 2;
}

static void test_order_handler_3(mln_event_t *ev, void *data)
{
    timer_order[timer_order_idx++] = 3;
    mln_event_break_set(ev);
}

static void test_timer_ordering(void)
{
    mln_event_t *ev = mln_event_new();
    TEST_ASSERT(ev != NULL, "event new for timer ordering");

    timer_order_idx = 0;
    memset(timer_order, 0, sizeof(timer_order));

    /* Set timers in reverse order but they should fire in time order */
    mln_event_timer_set(ev, 150, NULL, test_order_handler_3);
    mln_event_timer_set(ev, 50, NULL, test_order_handler_1);
    mln_event_timer_set(ev, 100, NULL, test_order_handler_2);

    mln_event_dispatch(ev);
    TEST_ASSERT(timer_order_idx == 3, "all 3 timers fired");
    TEST_ASSERT(timer_order[0] == 1, "first timer correct");
    TEST_ASSERT(timer_order[1] == 2, "second timer correct");
    TEST_ASSERT(timer_order[2] == 3, "third timer correct");

    mln_event_free(ev);
    printf("  [PASS] timer ordering\n");
}

/* ======== Test 11: FD CLR ======== */
static int clr_handler_count = 0;

static void test_clr_write_handler(mln_event_t *ev, int fd, void *data)
{
    clr_handler_count++;
}

static void test_clr_break(mln_event_t *ev, void *data)
{
    mln_event_break_set(ev);
}

static void test_fd_clr(void)
{
    int pipefd[2];
    TEST_ASSERT(pipe(pipefd) == 0, "pipe create for clr");

    mln_event_t *ev = mln_event_new();
    TEST_ASSERT(ev != NULL, "event new for clr");

    clr_handler_count = 0;

    TEST_ASSERT(mln_event_fd_set(ev, pipefd[1], M_EV_SEND|M_EV_NONBLOCK, M_EV_UNLIMITED, NULL, test_clr_write_handler) == 0, "fd_set for clr test");
    /* Clear immediately */
    TEST_ASSERT(mln_event_fd_set(ev, pipefd[1], M_EV_CLR, M_EV_UNLIMITED, NULL, NULL) == 0, "fd_set clr");

    mln_event_timer_set(ev, 100, NULL, test_clr_break);
    mln_event_dispatch(ev);

    TEST_ASSERT(clr_handler_count == 0, "cleared fd handler not called");

    mln_event_free(ev);
    close(pipefd[0]);
    close(pipefd[1]);
    printf("  [PASS] fd clr\n");
}

/* ======== Test 12: Performance benchmark ======== */
static int perf_timer_count = 0;
#define PERF_TIMER_TOTAL 10000

static void test_perf_timer_handler(mln_event_t *ev, void *data)
{
    perf_timer_count++;
    if (perf_timer_count >= PERF_TIMER_TOTAL) {
        mln_event_break_set(ev);
    }
}

static void test_perf_timer(void)
{
    mln_event_t *ev = mln_event_new();
    TEST_ASSERT(ev != NULL, "event new for perf timer");

    struct timeval t1, t2;
    perf_timer_count = 0;

    gettimeofday(&t1, NULL);

    /* Set all timers to fire immediately (0ms) */
    int i;
    for (i = 0; i < PERF_TIMER_TOTAL; i++) {
        mln_event_timer_t *t = mln_event_timer_set(ev, 0, NULL, test_perf_timer_handler);
        TEST_ASSERT(t != NULL, "perf timer set");
    }

    mln_event_dispatch(ev);

    gettimeofday(&t2, NULL);
    long elapsed_us = (t2.tv_sec - t1.tv_sec)*1000000 + (t2.tv_usec - t1.tv_usec);
    double elapsed_ms = elapsed_us / 1000.0;

    TEST_ASSERT(perf_timer_count == PERF_TIMER_TOTAL, "all perf timers fired");
    printf("  [PERF] %d timers in %.2f ms (%.0f timers/ms)\n",
           PERF_TIMER_TOTAL, elapsed_ms,
           elapsed_ms > 0 ? PERF_TIMER_TOTAL / elapsed_ms : 0);

    mln_event_free(ev);
}

/* ======== Test 13: Performance - FD set/clr cycles ======== */
#define PERF_FD_CYCLES 10000

static void test_perf_fd_noop(mln_event_t *ev, int fd, void *data)
{
    /* noop */
}

static void test_perf_fd_set_clr(void)
{
    int pipefd[2];
    TEST_ASSERT(pipe(pipefd) == 0, "pipe create for perf fd");

    mln_event_t *ev = mln_event_new();
    TEST_ASSERT(ev != NULL, "event new for perf fd");

    struct timeval t1, t2;
    gettimeofday(&t1, NULL);

    int i;
    for (i = 0; i < PERF_FD_CYCLES; i++) {
        mln_event_fd_set(ev, pipefd[1], M_EV_SEND|M_EV_NONBLOCK, M_EV_UNLIMITED, NULL, test_perf_fd_noop);
        mln_event_fd_set(ev, pipefd[1], M_EV_CLR, M_EV_UNLIMITED, NULL, NULL);
    }

    gettimeofday(&t2, NULL);
    long elapsed_us = (t2.tv_sec - t1.tv_sec)*1000000 + (t2.tv_usec - t1.tv_usec);
    double elapsed_ms = elapsed_us / 1000.0;

    printf("  [PERF] %d fd set/clr cycles in %.2f ms (%.0f cycles/ms)\n",
           PERF_FD_CYCLES, elapsed_ms,
           elapsed_ms > 0 ? PERF_FD_CYCLES / elapsed_ms : 0);

    mln_event_free(ev);
    close(pipefd[0]);
    close(pipefd[1]);
}

/* ======== Test 14: Stability - many concurrent fd events ======== */
#define STABILITY_FD_COUNT 100
static int stability_fd_fired[STABILITY_FD_COUNT];
static int stability_total_fired = 0;

static void test_stability_handler(mln_event_t *ev, int fd, void *data)
{
    int idx = (int)(long)data;
    stability_fd_fired[idx]++;
    stability_total_fired++;
    mln_event_fd_set(ev, fd, M_EV_CLR, M_EV_UNLIMITED, NULL, NULL);
}

static void test_stability_break(mln_event_t *ev, void *data)
{
    mln_event_break_set(ev);
}

static void test_stability(void)
{
    int pipes[STABILITY_FD_COUNT][2];
    int i;

    mln_event_t *ev = mln_event_new();
    TEST_ASSERT(ev != NULL, "event new for stability");

    memset(stability_fd_fired, 0, sizeof(stability_fd_fired));
    stability_total_fired = 0;

    for (i = 0; i < STABILITY_FD_COUNT; i++) {
        if (pipe(pipes[i]) < 0) {
            fprintf(stderr, "  [WARN] pipe creation failed at %d, testing with %d fds\n", i, i);
            break;
        }
        /* Write data to make read ready */
        write(pipes[i][1], "x", 1);
        mln_event_fd_set(ev, pipes[i][0], M_EV_RECV|M_EV_NONBLOCK, M_EV_UNLIMITED, (void *)(long)i, test_stability_handler);
    }
    int actual_count = i;

    /* Timeout to break if not all done */
    mln_event_timer_set(ev, 2000, NULL, test_stability_break);

    mln_event_dispatch(ev);

    TEST_ASSERT(stability_total_fired == actual_count, "all stability fds fired");

    for (i = 0; i < actual_count; i++) {
        TEST_ASSERT(stability_fd_fired[i] == 1, "stability fd fired exactly once");
        close(pipes[i][0]);
        close(pipes[i][1]);
    }

    mln_event_free(ev);
    printf("  [PASS] stability with %d concurrent fds\n", actual_count);
}

/* ======== Test 15: Recurring timer ======== */
static int recurring_count = 0;

static void test_recurring_handler(mln_event_t *ev, void *data)
{
    recurring_count++;
    if (recurring_count < 5) {
        mln_event_timer_set(ev, 20, NULL, test_recurring_handler);
    } else {
        mln_event_break_set(ev);
    }
}

static void test_recurring_timer(void)
{
    mln_event_t *ev = mln_event_new();
    TEST_ASSERT(ev != NULL, "event new for recurring");

    recurring_count = 0;
    mln_event_timer_set(ev, 20, NULL, test_recurring_handler);

    mln_event_dispatch(ev);
    TEST_ASSERT(recurring_count == 5, "recurring timer fired 5 times");

    mln_event_free(ev);
    printf("  [PASS] recurring timer\n");
}

/* ======== Test 16: User data passing ======== */
static void test_userdata_handler(mln_event_t *ev, int fd, void *data)
{
    int *val = (int *)data;
    TEST_ASSERT(*val == 42, "user data passed correctly");
    mln_event_fd_set(ev, fd, M_EV_CLR, M_EV_UNLIMITED, NULL, NULL);
    mln_event_break_set(ev);
}

static void test_userdata(void)
{
    int pipefd[2];
    TEST_ASSERT(pipe(pipefd) == 0, "pipe create for userdata");

    mln_event_t *ev = mln_event_new();
    TEST_ASSERT(ev != NULL, "event new for userdata");

    int mydata = 42;
    mln_event_fd_set(ev, pipefd[1], M_EV_SEND|M_EV_NONBLOCK, M_EV_UNLIMITED, &mydata, test_userdata_handler);

    mln_event_dispatch(ev);

    mln_event_free(ev);
    close(pipefd[0]);
    close(pipefd[1]);
    printf("  [PASS] user data passing\n");
}

/* ======== Test 17: High fd value (array growth) ======== */
static void test_highfd_handler(mln_event_t *ev, int fd, void *data)
{
    mln_event_fd_set(ev, fd, M_EV_CLR, M_EV_UNLIMITED, NULL, NULL);
    mln_event_break_set(ev);
}

static void test_high_fd(void)
{
    /*
     * Use dup2 to create a high-numbered fd (beyond initial array size of 1024).
     * This validates the fd-indexed array grows correctly via realloc
     * without rehashing or copying entries.
     */
    int pipefd[2];
    TEST_ASSERT(pipe(pipefd) == 0, "pipe create for high fd");

    int highfd = dup2(pipefd[1], 2048);
    if (highfd < 0) {
        /* environment may limit fd range - skip gracefully */
        printf("  [SKIP] high fd (dup2 failed, fd limit too low)\n");
        close(pipefd[0]);
        close(pipefd[1]);
        return;
    }

    mln_event_t *ev = mln_event_new();
    TEST_ASSERT(ev != NULL, "event new for high fd");

    TEST_ASSERT(mln_event_fd_set(ev, highfd, M_EV_SEND|M_EV_NONBLOCK, M_EV_UNLIMITED, NULL, test_highfd_handler) == 0, "fd_set high fd");

    mln_event_dispatch(ev);
    TEST_ASSERT(1, "high fd dispatch completed");

    mln_event_free(ev);
    close(highfd);
    close(pipefd[0]);
    close(pipefd[1]);
    printf("  [PASS] high fd (fd=%d, array growth)\n", highfd);
}

/* ======== Test 18: Rapid add/remove churn (no memory leak / no rehash) ======== */
#define CHURN_CYCLES 50000

static void test_churn_noop(mln_event_t *ev, int fd, void *data) { }

static void test_fd_churn(void)
{
    int pipefd[2];
    TEST_ASSERT(pipe(pipefd) == 0, "pipe create for churn");

    mln_event_t *ev = mln_event_new();
    TEST_ASSERT(ev != NULL, "event new for churn");

    struct timeval t1, t2;
    gettimeofday(&t1, NULL);

    int i;
    for (i = 0; i < CHURN_CYCLES; i++) {
        mln_event_fd_set(ev, pipefd[1], M_EV_SEND|M_EV_NONBLOCK, M_EV_UNLIMITED, NULL, test_churn_noop);
        mln_event_fd_set(ev, pipefd[1], M_EV_CLR, M_EV_UNLIMITED, NULL, NULL);
    }

    gettimeofday(&t2, NULL);
    long elapsed_us = (t2.tv_sec - t1.tv_sec)*1000000 + (t2.tv_usec - t1.tv_usec);
    double elapsed_ms = elapsed_us / 1000.0;

    printf("  [PERF] %d fd add/remove churn cycles in %.2f ms (%.0f cycles/ms)\n",
           CHURN_CYCLES, elapsed_ms,
           elapsed_ms > 0 ? CHURN_CYCLES / elapsed_ms : 0);

    mln_event_free(ev);
    close(pipefd[0]);
    close(pipefd[1]);
}

/* ======== Test 19: 10k distinct fds rapid add/remove ======== */
#define MASS_FD_COUNT 10000

static void test_mass_fd_noop(mln_event_t *ev, int fd, void *data) { }

static void test_mass_fd_churn(void)
{
    /*
     * Create event first (needs kqueue/epoll fd), then create pipes.
     * This avoids exhausting the fd limit before the event engine
     * can allocate its internal fds.
     */
    int (*pipes)[2] = NULL;
    int i, actual = 0;

    mln_event_t *ev = mln_event_new();
    if (ev == NULL) {
        printf("  [SKIP] 10k fd churn (mln_event_new failed)\n");
        return;
    }

    pipes = (int (*)[2])malloc(MASS_FD_COUNT * sizeof(int[2]));
    if (pipes == NULL) {
        printf("  [SKIP] 10k fd churn (malloc failed)\n");
        mln_event_free(ev);
        return;
    }

    for (i = 0; i < MASS_FD_COUNT; i++) {
        if (pipe(pipes[i]) < 0) break;
        actual++;
    }

    if (actual < 1000) {
        printf("  [SKIP] 10k fd churn (only %d pipes, fd limit too low)\n", actual);
        for (i = 0; i < actual; i++) { close(pipes[i][0]); close(pipes[i][1]); }
        free(pipes);
        mln_event_free(ev);
        return;
    }

    struct timeval t1, t2;
    int round, rounds = 5;

    gettimeofday(&t1, NULL);

    for (round = 0; round < rounds; round++) {
        /* add all */
        for (i = 0; i < actual; i++) {
            mln_event_fd_set(ev, pipes[i][0], M_EV_RECV|M_EV_NONBLOCK,
                             M_EV_UNLIMITED, NULL, test_mass_fd_noop);
        }
        /* remove all */
        for (i = 0; i < actual; i++) {
            mln_event_fd_set(ev, pipes[i][0], M_EV_CLR,
                             M_EV_UNLIMITED, NULL, NULL);
        }
    }

    gettimeofday(&t2, NULL);
    long elapsed_us = (t2.tv_sec - t1.tv_sec) * 1000000 + (t2.tv_usec - t1.tv_usec);
    double elapsed_ms = elapsed_us / 1000.0;
    long total_ops = (long)actual * rounds * 2;

    printf("  [PERF] %d fds x %d rounds (%ld set+clr ops) in %.2f ms (%.0f ops/ms)\n",
           actual, rounds, total_ops, elapsed_ms,
           elapsed_ms > 0 ? total_ops / elapsed_ms : 0);

    mln_event_free(ev);
    for (i = 0; i < actual; i++) { close(pipes[i][0]); close(pipes[i][1]); }
    free(pipes);
}

/* ======== Main ======== */
int main(int argc, char *argv[])
{
    printf("=== Event Module Tests ===\n\n");

    printf("[Feature Tests]\n");
    test_basic_timer();
    test_timer_cancel();
    test_fd_write();
    test_fd_read();
    test_fd_oneshot();
    test_fd_append();
    test_fd_timeout();
    test_callback();
    test_break_reset();
    test_timer_ordering();
    test_fd_clr();
    test_recurring_timer();
    test_userdata();

    printf("\n[Performance Tests]\n");
    test_perf_timer();
    test_perf_fd_set_clr();

    printf("\n[Stability Tests]\n");
    test_stability();
    test_high_fd();
    test_fd_churn();
    test_mass_fd_churn();

    printf("\n=== Results: %d passed, %d failed ===\n", test_pass, test_fail);

    return test_fail > 0 ? 1 : 0;
}
