#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include "mln_chain.h"

static long elapsed_us(struct timespec *start, struct timespec *end)
{
    long sec_us = (end->tv_sec - start->tv_sec) * 1000000L;
    long nsec_us = (end->tv_nsec - start->tv_nsec) / 1000L;
    return sec_us + nsec_us;
}

static void test_basic(void)
{
    printf("Testing basic allocation...\n");
    mln_alloc_t *pool = mln_alloc_init(NULL, 0);
    assert(pool != NULL);

    mln_buf_t *b = mln_buf_new(pool);
    assert(b != NULL);
    assert(b->left_pos == NULL);
    assert(b->pos == NULL);
    assert(b->last == NULL);
    assert(b->shadow == NULL);
    assert(b->in_memory == 0);
    assert(b->in_file == 0);
    assert(b->temporary == 0);
    assert(b->flush == 0);
    assert(b->sync == 0);
    assert(b->last_buf == 0);
    assert(b->last_in_chain == 0);

    mln_chain_t *c = mln_chain_new(pool);
    assert(c != NULL);
    assert(c->buf == NULL);
    assert(c->next == NULL);

    mln_chain_pool_release(c);
    mln_buf_pool_release(b);
    mln_alloc_destroy(pool);
    printf("  PASS: basic allocation\n");
}

static void test_chain_new_with_buf(void)
{
    printf("Testing chain_new_with_buf...\n");
    mln_alloc_t *pool = mln_alloc_init(NULL, 0);
    assert(pool != NULL);

    mln_chain_t *c = mln_chain_new_with_buf(pool);
    assert(c != NULL);
    assert(c->buf != NULL);
    assert(c->next == NULL);
    assert(c->buf->in_memory == 0);
    assert(c->buf->in_file == 0);

    mln_chain_pool_release(c);
    mln_alloc_destroy(pool);
    printf("  PASS: chain_new_with_buf\n");
}

static void test_buf_types(void)
{
    printf("Testing buffer types...\n");
    mln_alloc_t *pool = mln_alloc_init(NULL, 0);
    assert(pool != NULL);

    /* in_memory buffer */
    mln_buf_t *b1 = mln_buf_new(pool);
    assert(b1 != NULL);
    mln_u8ptr_t mem = (mln_u8ptr_t)mln_alloc_m(pool, 1024);
    assert(mem != NULL);
    b1->left_pos = b1->pos = b1->start = mem;
    b1->last = b1->end = mem + 1024;
    b1->in_memory = 1;
    mln_buf_pool_release(b1);

    /* temporary buffer */
    mln_buf_t *b2 = mln_buf_new(pool);
    assert(b2 != NULL);
    b2->temporary = 1;
    mln_buf_pool_release(b2);

    /* shadow buffer */
    mln_buf_t *b3 = mln_buf_new(pool);
    assert(b3 != NULL);
    mln_buf_t *bs = mln_buf_new(pool);
    assert(bs != NULL);
    b3->shadow = bs;
    mln_buf_pool_release(b3);
    mln_buf_pool_release(bs);

    /* bare buffer (no flags) */
    mln_buf_t *b4 = mln_buf_new(pool);
    assert(b4 != NULL);
    mln_buf_pool_release(b4);

    mln_alloc_destroy(pool);
    printf("  PASS: buffer types\n");
}

static void test_chain_macros(void)
{
    printf("Testing chain macros...\n");
    mln_alloc_t *pool = mln_alloc_init(NULL, 0);
    assert(pool != NULL);

    mln_chain_t *head = NULL, *tail = NULL;

    for (int i = 0; i < 5; i++) {
        mln_chain_t *c = mln_chain_new(pool);
        assert(c != NULL);
        mln_buf_t *b = mln_buf_new(pool);
        assert(b != NULL);
        c->buf = b;

        mln_u8ptr_t mem = (mln_u8ptr_t)mln_alloc_m(pool, 256);
        assert(mem != NULL);
        b->left_pos = b->start = mem;
        b->pos = mem + 10;
        b->last = b->end = mem + 256;
        b->in_memory = 1;

        mln_chain_add(&head, &tail, c);
    }

    assert(head != NULL);
    assert(tail != NULL);

    int count = 0;
    mln_chain_t *iter = head;
    while (iter != NULL) {
        assert(iter->buf != NULL);
        assert(mln_buf_size(iter->buf) == 246);
        assert(mln_buf_left_size(iter->buf) > 0);
        iter = iter->next;
        count++;
    }
    assert(count == 5);

    /* mln_buf_size / mln_buf_left_size with valid buf */
    {
        mln_buf_t *nb = mln_buf_new(pool);
        assert(nb != NULL);
        assert(mln_buf_size(nb) == 0);
        assert(mln_buf_left_size(nb) == 0);
        mln_buf_pool_release(nb);
    }

    mln_chain_pool_release_all(head);
    mln_alloc_destroy(pool);
    printf("  PASS: chain macros\n");
}

static void test_release_all(void)
{
    printf("Testing release_all...\n");
    mln_alloc_t *pool = mln_alloc_init(NULL, 0);
    assert(pool != NULL);

    mln_chain_t *head = NULL, *tail = NULL;
    for (int i = 0; i < 10; i++) {
        mln_chain_t *c = mln_chain_new_with_buf(pool);
        assert(c != NULL);
        mln_chain_add(&head, &tail, c);
    }

    mln_chain_pool_release_all(head);

    /* Releasing NULL should be safe */
    mln_chain_pool_release_all(NULL);
    mln_chain_pool_release(NULL);
    mln_buf_pool_release(NULL);

    mln_alloc_destroy(pool);
    printf("  PASS: release_all\n");
}

static void test_performance(void)
{
    printf("Testing performance (1000000 iterations)...\n");

    mln_alloc_t *pool = mln_alloc_init(NULL, 0);
    assert(pool != NULL);

    struct timespec start, end;
    const int N = 1000000;

    /* Benchmark: chain_new + buf_new + release */
    clock_gettime(CLOCK_MONOTONIC, &start);
    for (int i = 0; i < N; i++) {
        mln_chain_t *c = mln_chain_new(pool);
        mln_buf_t *b = mln_buf_new(pool);
        if (c != NULL && b != NULL) {
            c->buf = b;
            mln_chain_pool_release(c);
        }
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    long us_separate = elapsed_us(&start, &end);

    /* Benchmark: chain_new_with_buf + release */
    clock_gettime(CLOCK_MONOTONIC, &start);
    for (int i = 0; i < N; i++) {
        mln_chain_t *c = mln_chain_new_with_buf(pool);
        if (c != NULL) {
            mln_chain_pool_release(c);
        }
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    long us_combined = elapsed_us(&start, &end);

    printf("  Separate alloc: %ld us (%.0f ops/sec)\n",
           us_separate, (N * 1000000.0) / us_separate);
    printf("  Combined alloc: %ld us (%.0f ops/sec)\n",
           us_combined, (N * 1000000.0) / us_combined);
    if (us_combined > 0)
        printf("  Combined speedup: %.2fx\n", (double)us_separate / us_combined);

    mln_alloc_destroy(pool);
    printf("  PASS: performance\n");
}

static void test_stability(void)
{
    printf("Testing stability with heavy usage...\n");

    mln_alloc_t *pool = mln_alloc_init(NULL, 0);
    assert(pool != NULL);

    for (int round = 0; round < 100; round++) {
        mln_chain_t *head = NULL, *tail = NULL;
        int len = (round % 50) + 1;

        for (int i = 0; i < len; i++) {
            mln_chain_t *c = mln_chain_new_with_buf(pool);
            assert(c != NULL);

            mln_u8ptr_t mem = (mln_u8ptr_t)mln_alloc_m(pool, 128);
            if (mem != NULL) {
                memset(mem, 'A' + (i % 26), 128);
                c->buf->left_pos = c->buf->pos = c->buf->start = mem;
                c->buf->last = c->buf->end = mem + 128;
                c->buf->in_memory = 1;
            }
            mln_chain_add(&head, &tail, c);
        }

        /* Verify chain integrity */
        int count = 0;
        mln_chain_t *iter = head;
        while (iter != NULL) {
            count++;
            iter = iter->next;
        }
        assert(count == len);

        mln_chain_pool_release_all(head);
    }

    mln_alloc_destroy(pool);
    printf("  PASS: stability\n");
}

int main(void)
{
    printf("=== Chain Module Tests ===\n\n");

    test_basic();
    test_chain_new_with_buf();
    test_buf_types();
    test_chain_macros();
    test_release_all();
    test_performance();
    test_stability();

    printf("\n=== All chain tests passed! ===\n");
    return 0;
}
