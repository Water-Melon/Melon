#include "mln_log.h"
#include "mln_string.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#if !defined(MSVC)
#include <unistd.h>
#include <pthread.h>
#endif

/*
 * test basic log output at all levels
 */
static void test_log_levels(void)
{
    mln_log(none, "none level message\n");
    mln_log(report, "report level message %d\n", 1);
    mln_log(debug, "debug level message %s\n", "hello");
    mln_log(warn, "warn level message %d\n", 2);
    mln_log(error, "error level message %d\n", 3);
    fprintf(stderr, "  [PASS] test_log_levels\n");
}

/*
 * test all format specifiers
 */
static void test_log_format_specifiers(void)
{
    mln_string_t s;
    s.data = (mln_u8ptr_t)"mln_string";
    s.len = 10;
    s.data_ref = 1;
    s.pool = 0;
    s.ref = 1;

    mln_log(debug, "string: %s\n", "hello");
    mln_log(debug, "mln_string: %S\n", &s);
    mln_log(debug, "long: %l\n", (long)123456789);
    mln_log(debug, "int: %d\n", 42);
    mln_log(debug, "char: %c\n", 'A');
    mln_log(debug, "float: %f\n", 3.14);
    mln_log(debug, "hex: %x\n", 0xff);
    mln_log(debug, "long hex: %X\n", (long)0xdeadbeef);
    mln_log(debug, "unsigned: %u\n", (unsigned int)12345);
    mln_log(debug, "unsigned long: %U\n", (unsigned long)99999);
#if defined(MSVC) || defined(i386) || defined(__arm__)
    mln_log(debug, "int64: %i\n", (long long)1234567890LL);
    mln_log(debug, "uint64: %I\n", (unsigned long long)9876543210ULL);
#else
    mln_log(debug, "int64: %i\n", (long)1234567890);
    mln_log(debug, "uint64: %I\n", (unsigned long)9876543210UL);
#endif
    mln_log(debug, "mixed: %s %d %f %x\n", "test", 100, 2.718, 0xab);
    fprintf(stderr, "  [PASS] test_log_format_specifiers\n");
}

/*
 * test accessor functions
 */
static void test_log_accessors(void)
{
    int fd = mln_log_fd();
    assert(fd >= 0);

    char *dir = mln_log_dir_path();
    assert(dir != NULL);

    char *logfile = mln_log_logfile_path();
    assert(logfile != NULL);

    char *pidpath = mln_log_pid_path();
    assert(pidpath != NULL);

    fprintf(stderr, "  [PASS] test_log_accessors (fd=%d, dir=%s, log=%s, pid=%s)\n",
            fd, dir, logfile, pidpath);
}

/*
 * test mln_log_writen
 */
static void test_log_writen(void)
{
    const char msg[] = "direct write test message\n";
    int n = mln_log_writen((void *)msg, sizeof(msg) - 1);
    assert(n > 0);
    fprintf(stderr, "  [PASS] test_log_writen\n");
}

/*
 * test custom logger
 */
static int custom_logger_called = 0;

static void custom_logger(mln_log_t *log, mln_log_level_t level,
                          const char *file, const char *func,
                          int line, char *msg, va_list arg)
{
    (void)log; (void)level; (void)file; (void)func; (void)line; (void)msg; (void)arg;
    custom_logger_called++;
}

static void test_custom_logger(void)
{
    mln_logger_t old = mln_log_logger_get();
    assert(old != NULL);

    mln_log_logger_set(custom_logger);
    assert(mln_log_logger_get() == custom_logger);

    custom_logger_called = 0;
    mln_log(debug, "this goes to custom logger\n");
    assert(custom_logger_called == 1);

    mln_log(error, "another custom log\n");
    assert(custom_logger_called == 2);

    /* restore default logger */
    mln_log_logger_set(old);
    assert(mln_log_logger_get() == old);

    fprintf(stderr, "  [PASS] test_custom_logger\n");
}

/*
 * test log before init (stderr only)
 */
static void test_log_before_init(void)
{
    mln_log(debug, "pre-init log works on stderr %d\n", 42);
    fprintf(stderr, "  [PASS] test_log_before_init\n");
}

/*
 * multi-thread correctness test
 */
#if !defined(MSVC)
#define THREAD_COUNT    4
#define THREAD_ITERS    2

static void *thread_log_worker(void *arg)
{
    int id = *(int *)arg;
    int i;
    for (i = 0; i < THREAD_ITERS; i++) {
        mln_log(debug, "thread %d iter %d\n", id, i);
    }
    return NULL;
}

static void test_thread_safety(void)
{
    pthread_t threads[THREAD_COUNT];
    int ids[THREAD_COUNT];
    int i;

    for (i = 0; i < THREAD_COUNT; i++) {
        ids[i] = i;
        assert(pthread_create(&threads[i], NULL, thread_log_worker, &ids[i]) == 0);
    }
    for (i = 0; i < THREAD_COUNT; i++) {
        pthread_join(threads[i], NULL);
    }

    fprintf(stderr, "  [PASS] test_thread_safety\n");
}
#endif

int main(int argc, char *argv[])
{
    (void)argc; (void)argv;

    fprintf(stderr, "=== Log Module Tests ===\n");

    /* test before init */
    test_log_before_init();

    /* init */
    mln_conf_load();
    assert(mln_log_init(NULL) == 0);

    /* feature tests */
    test_log_levels();
    test_log_format_specifiers();
    test_log_accessors();
    test_log_writen();
    test_custom_logger();

    /* thread safety test */
#if !defined(MSVC)
    test_thread_safety();
#endif

    fprintf(stderr, "=== All Log Tests Passed ===\n");

    mln_log_destroy();
    mln_conf_free();
    return 0;
}
