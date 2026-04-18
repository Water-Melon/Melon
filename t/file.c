#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h>
#include <assert.h>
#include "mln_file.h"
#include "mln_path.h"

#define TEST_DIR     "/tmp/mln_file_test"
#define TEST_FILE    TEST_DIR "/testfile"
#define PERF_COUNT   100000

static void create_test_file(const char *path, const char *content)
{
    int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    assert(fd >= 0);
    if (content != NULL) {
        ssize_t n = write(fd, content, strlen(content));
        assert(n == (ssize_t)strlen(content));
    }
    close(fd);
}

static double time_diff_us(struct timeval *start, struct timeval *end)
{
    return (double)(end->tv_sec - start->tv_sec) * 1e6 + (double)(end->tv_usec - start->tv_usec);
}

static void test_basic_open_close(void)
{
    mln_fileset_t *fset;
    mln_file_t *file;

    printf("  test_basic_open_close ... ");

    fset = mln_fileset_init(100);
    assert(fset != NULL);

    create_test_file(TEST_FILE, "Hello World");

    file = mln_file_open(fset, TEST_FILE);
    assert(file != NULL);
    assert(file->fd >= 0);
    assert(file->file_path != NULL);
    assert(strcmp((char *)file->file_path->data, TEST_FILE) == 0);
    assert(file->size == 11);
    assert(file->is_tmp == 0);
    assert(file->refer_cnt == 1);

    mln_file_close(file);
    mln_fileset_destroy(fset);

    printf("OK\n");
}

static void test_reopen_cache_hit(void)
{
    mln_fileset_t *fset;
    mln_file_t *f1, *f2;

    printf("  test_reopen_cache_hit ... ");

    fset = mln_fileset_init(100);
    assert(fset != NULL);

    create_test_file(TEST_FILE, "cache test");

    f1 = mln_file_open(fset, TEST_FILE);
    assert(f1 != NULL);
    assert(f1->refer_cnt == 1);

    /* opening same file returns same pointer (cache hit) */
    f2 = mln_file_open(fset, TEST_FILE);
    assert(f2 == f1);
    assert(f1->refer_cnt == 2);

    mln_file_close(f1);
    assert(f2->refer_cnt == 1);

    mln_file_close(f2);
    mln_fileset_destroy(fset);

    printf("OK\n");
}

static void test_reference_counting(void)
{
    mln_fileset_t *fset;
    mln_file_t *f1, *f2, *f3;

    printf("  test_reference_counting ... ");

    fset = mln_fileset_init(100);
    assert(fset != NULL);

    create_test_file(TEST_FILE, "refcount");

    f1 = mln_file_open(fset, TEST_FILE);
    assert(f1 != NULL);
    assert(f1->refer_cnt == 1);

    f2 = mln_file_open(fset, TEST_FILE);
    assert(f2 == f1);
    assert(f1->refer_cnt == 2);

    f3 = mln_file_open(fset, TEST_FILE);
    assert(f3 == f1);
    assert(f1->refer_cnt == 3);

    mln_file_close(f3);
    assert(f1->refer_cnt == 2);
    mln_file_close(f2);
    assert(f1->refer_cnt == 1);
    mln_file_close(f1);

    /* After all refs closed, file is in free list but still cached */
    f1 = mln_file_open(fset, TEST_FILE);
    assert(f1 != NULL);
    assert(f1->refer_cnt == 1);

    mln_file_close(f1);
    mln_fileset_destroy(fset);

    printf("OK\n");
}

static void test_multiple_files(void)
{
    mln_fileset_t *fset;
    mln_file_t *files[10];
    char path[256];
    int i;

    printf("  test_multiple_files ... ");

    fset = mln_fileset_init(100);
    assert(fset != NULL);

    for (i = 0; i < 10; i++) {
        snprintf(path, sizeof(path), TEST_DIR "/multi_%d", i);
        create_test_file(path, "data");
        files[i] = mln_file_open(fset, path);
        assert(files[i] != NULL);
        assert(files[i]->refer_cnt == 1);
    }

    /* Verify all files are distinct */
    for (i = 0; i < 10; i++) {
        int j;
        for (j = i + 1; j < 10; j++) {
            assert(files[i] != files[j]);
        }
    }

    for (i = 0; i < 10; i++) {
        mln_file_close(files[i]);
    }

    mln_fileset_destroy(fset);

    printf("OK\n");
}

static void test_max_file_eviction(void)
{
    mln_fileset_t *fset;
    mln_file_t *file;
    char path[256];
    int i;

    printf("  test_max_file_eviction ... ");

    /* Set max_file to 3 so eviction triggers quickly */
    fset = mln_fileset_init(3);
    assert(fset != NULL);

    /* Open and close 5 files; after max_file exceeded, oldest free ones are evicted */
    for (i = 0; i < 5; i++) {
        snprintf(path, sizeof(path), TEST_DIR "/evict_%d", i);
        create_test_file(path, "evict");
        file = mln_file_open(fset, path);
        assert(file != NULL);
        mln_file_close(file);
    }

    /* Eviction should have removed some. Opening a new file should still work. */
    snprintf(path, sizeof(path), TEST_DIR "/evict_new");
    create_test_file(path, "new");
    file = mln_file_open(fset, path);
    assert(file != NULL);
    mln_file_close(file);

    mln_fileset_destroy(fset);

    printf("OK\n");
}

static void test_eviction_skips_referenced(void)
{
    mln_fileset_t *fset;
    mln_file_t *held, *f;
    char path[256];
    int i;

    printf("  test_eviction_skips_referenced ... ");

    fset = mln_fileset_init(2);
    assert(fset != NULL);

    /* Hold a reference to file 0 */
    snprintf(path, sizeof(path), TEST_DIR "/held_0");
    create_test_file(path, "held");
    held = mln_file_open(fset, path);
    assert(held != NULL);

    /* Open and close more files to trigger eviction */
    for (i = 1; i <= 4; i++) {
        snprintf(path, sizeof(path), TEST_DIR "/held_%d", i);
        create_test_file(path, "other");
        f = mln_file_open(fset, path);
        assert(f != NULL);
        mln_file_close(f);
    }

    /* The held file should still be valid */
    assert(held->refer_cnt == 1);
    assert(held->fd >= 0);

    mln_file_close(held);
    mln_fileset_destroy(fset);

    printf("OK\n");
}

static void test_file_metadata(void)
{
    mln_fileset_t *fset;
    mln_file_t *file;
    struct stat st;

    printf("  test_file_metadata ... ");

    fset = mln_fileset_init(100);
    assert(fset != NULL);

    create_test_file(TEST_FILE, "metadata test content");

    stat(TEST_FILE, &st);

    file = mln_file_open(fset, TEST_FILE);
    assert(file != NULL);
    assert(file->size == (size_t)st.st_size);
    assert(file->mtime == st.st_mtime);
    assert(file->ctime == st.st_ctime);
    assert(file->atime == st.st_atime);

    mln_file_close(file);
    mln_fileset_destroy(fset);

    printf("OK\n");
}

static void test_file_fd_macro(void)
{
    mln_fileset_t *fset;
    mln_file_t *file;

    printf("  test_file_fd_macro ... ");

    fset = mln_fileset_init(100);
    assert(fset != NULL);

    create_test_file(TEST_FILE, "fd macro");

    file = mln_file_open(fset, TEST_FILE);
    assert(file != NULL);
    assert(mln_file_fd(file) == file->fd);
    assert(mln_file_fd(file) >= 0);

    mln_file_close(file);
    mln_fileset_destroy(fset);

    printf("OK\n");
}

static void test_tmp_file(void)
{
    mln_alloc_t *pool;
    mln_file_t *tmpf;
    char buf[64] = {0};
    ssize_t n;

    printf("  test_tmp_file ... ");

    pool = mln_alloc_init(NULL, 0);
    assert(pool != NULL);

    tmpf = mln_file_tmp_open(pool);
    assert(tmpf != NULL);
    assert(tmpf->is_tmp == 1);
    assert(tmpf->fd >= 0);
    assert(tmpf->file_path == NULL);

    /* Write and read back from tmp file */
    n = write(tmpf->fd, "temporary", 9);
    assert(n == 9);
    lseek(tmpf->fd, 0, SEEK_SET);
    n = read(tmpf->fd, buf, sizeof(buf));
    assert(n == 9);
    assert(memcmp(buf, "temporary", 9) == 0);

    mln_file_close(tmpf);
    mln_alloc_destroy(pool);

    printf("OK\n");
}

static void test_open_nonexistent(void)
{
    mln_fileset_t *fset;
    mln_file_t *file;

    printf("  test_open_nonexistent ... ");

    fset = mln_fileset_init(100);
    assert(fset != NULL);

    file = mln_file_open(fset, "/tmp/mln_file_test_nonexistent_12345");
    assert(file == NULL);

    mln_fileset_destroy(fset);

    printf("OK\n");
}

static void test_close_null(void)
{
    printf("  test_close_null ... ");
    mln_file_close(NULL); /* should not crash */
    printf("OK\n");
}

static void test_destroy_null(void)
{
    printf("  test_destroy_null ... ");
    mln_fileset_destroy(NULL); /* should not crash */
    printf("OK\n");
}

static void test_perf_open_cached(void)
{
    mln_fileset_t *fset;
    mln_file_t *file;
    struct timeval start, end;
    double elapsed;
    int i;

    printf("  test_perf_open_cached (%d iterations) ... ", PERF_COUNT);

    fset = mln_fileset_init(1000);
    assert(fset != NULL);

    create_test_file(TEST_FILE, "perf");

    /* First open to populate cache */
    file = mln_file_open(fset, TEST_FILE);
    assert(file != NULL);
    mln_file_close(file);

    gettimeofday(&start, NULL);
    for (i = 0; i < PERF_COUNT; i++) {
        file = mln_file_open(fset, TEST_FILE);
        mln_file_close(file);
    }
    gettimeofday(&end, NULL);

    elapsed = time_diff_us(&start, &end);
    printf("%.2f us total, %.4f us/op\n", elapsed, elapsed / PERF_COUNT);

    mln_fileset_destroy(fset);
}

static void test_perf_open_many_files(void)
{
    mln_fileset_t *fset;
    mln_file_t *files[1000];
    char path[256];
    struct timeval start, end;
    double elapsed;
    int i;
    int count = 1000;

    printf("  test_perf_open_many_files (%d files) ... ", count);

    fset = mln_fileset_init(2000);
    assert(fset != NULL);

    for (i = 0; i < count; i++) {
        snprintf(path, sizeof(path), TEST_DIR "/perf_%d", i);
        create_test_file(path, "p");
    }

    gettimeofday(&start, NULL);
    for (i = 0; i < count; i++) {
        snprintf(path, sizeof(path), TEST_DIR "/perf_%d", i);
        files[i] = mln_file_open(fset, path);
        assert(files[i] != NULL);
    }
    gettimeofday(&end, NULL);

    elapsed = time_diff_us(&start, &end);
    printf("open %.2f us total, %.4f us/op; ", elapsed, elapsed / count);

    /* Now close all */
    gettimeofday(&start, NULL);
    for (i = 0; i < count; i++) {
        mln_file_close(files[i]);
    }
    gettimeofday(&end, NULL);

    elapsed = time_diff_us(&start, &end);
    printf("close %.2f us total, %.4f us/op\n", elapsed, elapsed / count);

    mln_fileset_destroy(fset);
}

static void test_perf_cached_lookup(void)
{
    mln_fileset_t *fset;
    mln_file_t *files[100];
    char paths[100][256];
    struct timeval start, end;
    double elapsed;
    int i, j;
    int file_count = 100;
    int lookup_rounds = 1000;

    printf("  test_perf_cached_lookup (%d files x %d rounds) ... ", file_count, lookup_rounds);

    fset = mln_fileset_init(200);
    assert(fset != NULL);

    for (i = 0; i < file_count; i++) {
        snprintf(paths[i], sizeof(paths[i]), TEST_DIR "/lookup_%d", i);
        create_test_file(paths[i], "lk");
        files[i] = mln_file_open(fset, paths[i]);
        assert(files[i] != NULL);
    }

    /* Benchmark repeated lookups of already-open files (cache hits) */
    gettimeofday(&start, NULL);
    for (j = 0; j < lookup_rounds; j++) {
        for (i = 0; i < file_count; i++) {
            mln_file_t *f = mln_file_open(fset, paths[i]);
            assert(f == files[i]);
            mln_file_close(f);
        }
    }
    gettimeofday(&end, NULL);

    elapsed = time_diff_us(&start, &end);
    printf("%.2f us total, %.4f us/op\n", elapsed, elapsed / (file_count * lookup_rounds));

    for (i = 0; i < file_count; i++) {
        mln_file_close(files[i]);
    }

    mln_fileset_destroy(fset);
}

static void test_stability_repeated_init_destroy(void)
{
    int i;

    printf("  test_stability_repeated_init_destroy (1000 cycles) ... ");

    for (i = 0; i < 1000; i++) {
        mln_fileset_t *fset = mln_fileset_init(10);
        assert(fset != NULL);

        create_test_file(TEST_FILE, "stability");
        mln_file_t *f = mln_file_open(fset, TEST_FILE);
        assert(f != NULL);
        mln_file_close(f);

        mln_fileset_destroy(fset);
    }

    printf("OK\n");
}

static void test_stability_open_close_cycle(void)
{
    mln_fileset_t *fset;
    mln_file_t *file;
    char path[256];
    int i;
    int cycles = 5000;

    printf("  test_stability_open_close_cycle (%d cycles) ... ", cycles);

    fset = mln_fileset_init(10);
    assert(fset != NULL);

    for (i = 0; i < cycles; i++) {
        snprintf(path, sizeof(path), TEST_DIR "/cycle_%d", i % 50);
        create_test_file(path, "cycle");
        file = mln_file_open(fset, path);
        assert(file != NULL);
        mln_file_close(file);
    }

    mln_fileset_destroy(fset);

    printf("OK\n");
}

static void test_stability_tmp_files(void)
{
    mln_alloc_t *pool;
    mln_file_t *tmpf;
    int i;
    int count = 200;

    printf("  test_stability_tmp_files (%d files) ... ", count);

    pool = mln_alloc_init(NULL, 0);
    assert(pool != NULL);

    for (i = 0; i < count; i++) {
        tmpf = mln_file_tmp_open(pool);
        assert(tmpf != NULL);
        assert(tmpf->fd >= 0);
        mln_file_close(tmpf);
    }

    mln_alloc_destroy(pool);

    printf("OK\n");
}

static char *test_tmpfile_path(void)
{
    return TEST_DIR;
}

static void setup_test_dir(void)
{
    mkdir(TEST_DIR, S_IRWXU);
    mln_path_hook_set(m_p_tmpfile, test_tmpfile_path);
}

static void cleanup_test_dir(void)
{
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "rm -rf %s", TEST_DIR);
    (void)!system(cmd);
}

int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    setup_test_dir();
    setvbuf(stdout, NULL, _IONBF, 0);

    printf("=== File Module Tests ===\n");

    printf("[Functional Tests]\n");
    test_basic_open_close();
    test_reopen_cache_hit();
    test_reference_counting();
    test_multiple_files();
    test_max_file_eviction();
    test_eviction_skips_referenced();
    test_file_metadata();
    test_file_fd_macro();
    test_tmp_file();
    test_open_nonexistent();
    test_close_null();
    test_destroy_null();

    printf("[Performance Tests]\n");
    test_perf_open_cached();
    test_perf_open_many_files();
    test_perf_cached_lookup();

    printf("[Stability Tests]\n");
    test_stability_repeated_init_destroy();
    test_stability_open_close_cycle();
    test_stability_tmp_files();

    cleanup_test_dir();

    printf("=== All tests passed ===\n");

    return 0;
}
