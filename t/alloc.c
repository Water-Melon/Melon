/*
 * Copyright (C) Niklaus F.Schen.
 *
 * Comprehensive tests for mln_alloc.
 *
 * Covers:
 *   - pool lifecycle (init / destroy, with & without capacity limits)
 *   - mln_alloc_m / mln_alloc_c / mln_alloc_re / mln_alloc_free
 *   - small / medium / large (>mgr table) allocation paths
 *   - mln_alloc_available_capacity bookkeeping
 *   - cascaded (parent) pools
 *   - complex multi-round alloc/free churn for stability
 *   - micro-benchmark to demonstrate hot-path throughput
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "mln_alloc.h"

static int nr_ok = 0;
static int nr_fail = 0;

#define CHECK(cond, msg) do { \
    if (cond) { \
        ++nr_ok; \
    } else { \
        ++nr_fail; \
        fprintf(stderr, "FAIL: %s (line %d)\n", msg, __LINE__); \
    } \
} while (0)

static double now_sec(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

/* --- basic behaviour ----------------------------------------------------- */

static void test_basic(void)
{
    mln_alloc_t *pool = mln_alloc_init(NULL, 0);
    CHECK(pool != NULL, "pool init");

    char *p = (char *)mln_alloc_m(pool, 6);
    CHECK(p != NULL, "alloc 6 bytes");
    memcpy(p, "hello", 5);
    p[5] = 0;
    CHECK(strcmp(p, "hello") == 0, "write/read small buf");
    printf("%s\n", p);
    mln_alloc_free(p);

    /* calloc path: memory is zeroed */
    unsigned char *z = (unsigned char *)mln_alloc_c(pool, 128);
    CHECK(z != NULL, "calloc 128 bytes");
    int all_zero = 1;
    for (int i = 0; i < 128; ++i) if (z[i] != 0) { all_zero = 0; break; }
    CHECK(all_zero, "calloc zero-fill");
    mln_alloc_free(z);

    /* realloc: grow preserves content */
    char *r = (char *)mln_alloc_m(pool, 16);
    CHECK(r != NULL, "alloc 16");
    memcpy(r, "abcdefghijklmno", 16);
    r = (char *)mln_alloc_re(pool, r, 64);
    CHECK(r != NULL, "realloc grow");
    CHECK(memcmp(r, "abcdefghijklmno", 15) == 0, "realloc preserves content");
    /* realloc shrink within same block should return same ptr */
    void *r2 = mln_alloc_re(pool, r, 8);
    CHECK(r2 == r, "realloc shrink is in-place");
    /* realloc(ptr, 0) frees */
    void *r3 = mln_alloc_re(pool, r, 0);
    CHECK(r3 == NULL, "realloc to zero returns NULL");

    /* free(NULL) must be a no-op */
    mln_alloc_free(NULL);

    mln_alloc_destroy(pool);
}

/* --- size coverage: every mgr class and the "large" fallback ------------- */

static void test_size_classes(void)
{
    mln_alloc_t *pool = mln_alloc_init(NULL, 0);
    CHECK(pool != NULL, "pool for size classes");

    /* small sizes exercise the mgr classifier; the last one overflows
     * M_ALLOC_MGR_LEN and should go through the large/chunk path. */
    static const mln_size_t sizes[] = {
        1, 2, 4, 8, 15, 16, 17, 31, 32, 63, 64, 127, 128, 255, 256,
        511, 512, 1023, 1024, 2047, 2048, 4095, 4096,
        8192, 16384, 32768, 65536, 131072, 262144, 524288,
        1024*1024, 2*1024*1024 /* large: outside mgr_tbl */
    };
    int n = (int)(sizeof(sizes) / sizeof(sizes[0]));
    void **ptrs = (void **)calloc(n, sizeof(void *));

    for (int i = 0; i < n; ++i) {
        ptrs[i] = mln_alloc_m(pool, sizes[i]);
        CHECK(ptrs[i] != NULL, "alloc across size classes");
        memset(ptrs[i], 0xA5, sizes[i]);
    }
    /* verify writes didn't clobber neighbors */
    for (int i = 0; i < n; ++i) {
        unsigned char *q = (unsigned char *)ptrs[i];
        int ok = 1;
        for (mln_size_t k = 0; k < sizes[i]; ++k)
            if (q[k] != 0xA5) { ok = 0; break; }
        CHECK(ok, "content integrity across size classes");
    }
    for (int i = 0; i < n; ++i) mln_alloc_free(ptrs[i]);
    free(ptrs);

    mln_alloc_destroy(pool);
}

/* --- capacity enforcement ------------------------------------------------ */

static void test_capacity(void)
{
    /* very tight cap: first small alloc succeeds, follow-ups eventually
     * fail once the pool exhausts the limit. */
    mln_alloc_t *pool = mln_alloc_init(NULL, 64 * 1024);
    CHECK(pool != NULL, "capped pool init");

    mln_size_t before = mln_alloc_available_capacity(pool);
    CHECK(before <= 64 * 1024, "capacity starts <= limit");

    void *p = mln_alloc_m(pool, 32);
    CHECK(p != NULL, "small alloc in capped pool");
    mln_size_t after = mln_alloc_available_capacity(pool);
    CHECK(after < before, "capacity shrinks after alloc");

    /* keep allocating until it fails, then free everything */
    void *list[4096];
    int count = 0;
    while (count < 4096) {
        void *q = mln_alloc_m(pool, 256);
        if (q == NULL) break;
        list[count++] = q;
    }
    CHECK(count > 0, "capped pool services allocations");
    for (int i = 0; i < count; ++i) mln_alloc_free(list[i]);
    mln_alloc_free(p);

    /* unlimited pool: available_capacity returns sentinel */
    mln_alloc_t *inf = mln_alloc_init(NULL, 0);
    CHECK(mln_alloc_available_capacity(inf) == M_ALLOC_INFINITE_SIZE, "unlimited sentinel");
    mln_alloc_destroy(inf);

    mln_alloc_destroy(pool);
}

/* --- cascaded pools (child drawing from parent) -------------------------- */

static void test_parent_child(void)
{
    mln_alloc_t *parent = mln_alloc_init(NULL, 0);
    CHECK(parent != NULL, "parent pool init");
    mln_alloc_t *child = mln_alloc_init(parent, 0);
    CHECK(child != NULL, "child pool init");

    void *p = mln_alloc_m(child, 123);
    CHECK(p != NULL, "child alloc");
    memset(p, 0x5A, 123);
    mln_alloc_free(p);

    /* destroying child must not touch parent */
    mln_alloc_destroy(child);

    void *q = mln_alloc_m(parent, 333);
    CHECK(q != NULL, "parent still usable after child destroy");
    mln_alloc_free(q);

    mln_alloc_destroy(parent);
}

/* --- complex multi-round churn (stress + stability) --------------------- */

#define CHURN_SLOTS 2048
#define CHURN_ROUNDS 32

static void test_churn(void)
{
    mln_alloc_t *pool = mln_alloc_init(NULL, 0);
    CHECK(pool != NULL, "churn pool init");

    void *slots[CHURN_SLOTS];
    mln_size_t sizes[CHURN_SLOTS];
    memset(slots, 0, sizeof(slots));

    /* mix of sizes straddling small/medium/large paths */
    static const mln_size_t shape[] = {
        8, 24, 48, 96, 160, 320, 640, 1280, 2560, 5120, 10240, 40960
    };
    const int nshape = (int)(sizeof(shape) / sizeof(shape[0]));

    unsigned rng = 0xC0FFEEu;
    for (int round = 0; round < CHURN_ROUNDS; ++round) {
        /* phase 1: fill every slot */
        for (int i = 0; i < CHURN_SLOTS; ++i) {
            rng = rng * 1103515245u + 12345u;
            sizes[i] = shape[(rng >> 8) % nshape];
            slots[i] = mln_alloc_m(pool, sizes[i]);
            if (slots[i] == NULL) { ++nr_fail; fprintf(stderr, "alloc failed round %d slot %d\n", round, i); break; }
            /* write a per-slot sentinel */
            memset(slots[i], (int)(i & 0xff), sizes[i]);
        }
        /* phase 2: randomly free half */
        for (int i = 0; i < CHURN_SLOTS; ++i) {
            rng = rng * 1103515245u + 12345u;
            if ((rng & 1) && slots[i]) {
                /* verify sentinel before freeing */
                unsigned char *q = (unsigned char *)slots[i];
                if (q[0] != (unsigned char)(i & 0xff) || q[sizes[i]-1] != (unsigned char)(i & 0xff)) {
                    ++nr_fail;
                    fprintf(stderr, "sentinel corruption round %d slot %d\n", round, i);
                }
                mln_alloc_free(slots[i]);
                slots[i] = NULL;
            }
        }
        /* phase 3: realloc another quarter to a new random size */
        for (int i = 0; i < CHURN_SLOTS; ++i) {
            rng = rng * 1103515245u + 12345u;
            if ((rng & 3) == 0 && slots[i]) {
                mln_size_t nsize = shape[(rng >> 8) % nshape];
                void *np = mln_alloc_re(pool, slots[i], nsize);
                if (np == NULL) { ++nr_fail; fprintf(stderr, "realloc failed round %d slot %d\n", round, i); continue; }
                slots[i] = np;
                sizes[i] = nsize;
                memset(slots[i], (int)(i & 0xff), sizes[i]);
            }
        }
        /* phase 4: refill freed slots */
        for (int i = 0; i < CHURN_SLOTS; ++i) {
            if (slots[i] == NULL) {
                rng = rng * 1103515245u + 12345u;
                sizes[i] = shape[(rng >> 8) % nshape];
                slots[i] = mln_alloc_m(pool, sizes[i]);
                if (slots[i] == NULL) { ++nr_fail; fprintf(stderr, "refill failed round %d slot %d\n", round, i); continue; }
                memset(slots[i], (int)(i & 0xff), sizes[i]);
            }
        }
    }
    /* drain everything */
    for (int i = 0; i < CHURN_SLOTS; ++i) {
        if (slots[i]) mln_alloc_free(slots[i]);
    }
    ++nr_ok;

    mln_alloc_destroy(pool);
}

/* --- micro-benchmark: demonstrate hot-path throughput -------------------- */

#define BENCH_N   200000
#define BENCH_SZ  64
#define BENCH_PING 2000000

static void test_benchmark(void)
{
    mln_alloc_t *pool = mln_alloc_init(NULL, 0);
    CHECK(pool != NULL, "bench pool init");

    /* fresh alloc/free pairs */
    double t0 = now_sec();
    for (int i = 0; i < BENCH_N; ++i) {
        void *p = mln_alloc_m(pool, BENCH_SZ);
        mln_alloc_free(p);
    }
    double t1 = now_sec();
    printf("  bench fresh alloc/free x%d : %.3f ms  (%.0f ops/s)\n",
           BENCH_N, (t1 - t0) * 1000.0, BENCH_N / (t1 - t0));

    /* batched fill-and-drain (exercises chunk refill path) */
    enum { BATCH = 1024 };
    void *batch[BATCH];
    t0 = now_sec();
    for (int round = 0; round < BENCH_N / BATCH; ++round) {
        for (int i = 0; i < BATCH; ++i) batch[i] = mln_alloc_m(pool, BENCH_SZ);
        for (int i = 0; i < BATCH; ++i) mln_alloc_free(batch[i]);
    }
    t1 = now_sec();
    printf("  bench fill/drain %d (batch %d) : %.3f ms\n",
           BENCH_N, BATCH, (t1 - t0) * 1000.0);

    /* same-slot ping-pong (hottest hot path) */
    t0 = now_sec();
    for (int i = 0; i < BENCH_PING; ++i) {
        void *p = mln_alloc_m(pool, BENCH_SZ);
        mln_alloc_free(p);
    }
    t1 = now_sec();
    printf("  bench ping-pong x%d      : %.3f ms  (%.0f ops/s)\n",
           BENCH_PING, (t1 - t0) * 1000.0, BENCH_PING / (t1 - t0));

    mln_alloc_destroy(pool);
}

int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    test_basic();
    test_size_classes();
    test_capacity();
    test_parent_child();
    test_churn();
    test_benchmark();

    printf("\nalloc tests: %d passed, %d failed\n", nr_ok, nr_fail);
    return nr_fail == 0 ? 0 : -1;
}
