#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "mln_hash.h"

static int test_nr = 0;
static int pass_nr = 0;
static int fail_nr = 0;

#define CHECK(cond, msg) do { \
    test_nr++; \
    if (cond) { pass_nr++; } \
    else { fail_nr++; fprintf(stderr, "FAIL #%d: %s\n", test_nr, msg); } \
} while (0)

/* -- helpers -- */
static mln_u64_t calc_handler(mln_hash_t *h, void *key)
{
    return (mln_u64_t)(*(int *)key) % h->len;
}

static int cmp_handler(mln_hash_t *h, void *key1, void *key2)
{
    return *(int *)key1 == *(int *)key2;
}

static void free_handler(void *val) { free(val); }

static int iterate_count_cb(mln_hash_t *h, void *key, void *val, void *udata)
{
    (*(int *)udata)++;
    return 0;
}

static int iterate_abort_cb(mln_hash_t *h, void *key, void *val, void *udata)
{
    (*(int *)udata)++;
    if (*(int *)udata >= 3) return -1;
    return 0;
}

static int iterate_remove_cb(mln_hash_t *h, void *key, void *val, void *udata)
{
    int k = *(int *)key;
    if (k % 2 == 0) {
        mln_hash_remove(h, key, M_HASH_F_NONE);
    }
    return 0;
}

/* -- Globals for deferred-free verification -- */
static int g_free_count = 0;
static int g_in_handler_flag = 0;
static int g_freed_in_handler = 0;

static void counting_freer(void *p)
{
    g_free_count++;
    if (g_in_handler_flag) g_freed_in_handler = 1;
    free(p);
}

/*
 * Callback: remove current node with M_HASH_F_KV during iteration.
 * The key/val freers must NOT be called while we are still inside the handler
 * (deferred removal). After handler returns, iterate must do the cleanup.
 */
static int iterate_remove_current_kv_cb(mln_hash_t *h, void *key, void *val, void *udata)
{
    int *count = (int *)udata;
    (*count)++;
    if (*(int *)key == 5) {
        g_in_handler_flag = 1;
        g_freed_in_handler = 0;
        mln_hash_remove(h, key, M_HASH_F_KV);
        /* At this point free must NOT have been called (deferred) */
        g_in_handler_flag = 0;
    }
    return 0;
}

/*
 * Callback: remove ALL even-keyed current nodes with M_HASH_F_KV.
 * Tests that multiple deferred removals work across the whole iteration.
 */
static int iterate_remove_even_kv_cb(mln_hash_t *h, void *key, void *val, void *udata)
{
    int *count = (int *)udata;
    (*count)++;
    if (*(int *)key % 2 == 0)
        mln_hash_remove(h, key, M_HASH_F_KV);
    return 0;
}

/*
 * Callback: update current node via mln_hash_update during iteration.
 */
static int g_update_new_val = 999;

static int iterate_update_current_cb(mln_hash_t *h, void *key, void *val, void *udata)
{
    int *count = (int *)udata;
    (*count)++;
    if (*(int *)key == 5) {
        void *pk = key;
        void *pv = &g_update_new_val;
        mln_hash_update(h, &pk, &pv);
        /* pk/pv now hold old key/val pointers (swapped out) */
    }
    return 0;
}

/*
 * Callback: remove a non-current node (the next key in insertion order)
 * during iteration. Tests that mln_hash_iterate correctly follows
 * the updated iter chain rather than a stale next_idx.
 * Keys are inserted in order 0..N-1, so the iter chain is 0->1->2->...
 * When visiting key K, we remove K+1 (the next entry, which is NOT the
 * current iter node and therefore gets removed immediately).
 */
static int iterate_remove_next_cb(mln_hash_t *h, void *key, void *val, void *udata)
{
    int *count = (int *)udata;
    (*count)++;
    int next_key = *(int *)key + 1;
    /* Only remove even-numbered next keys to create a mix */
    if (next_key % 2 == 0 && next_key < 10) {
        mln_hash_remove(h, &next_key, M_HASH_F_NONE);
    }
    return 0;
}

/* -- tests -- */

static void test_new_free(void)
{
    struct mln_hash_attr attr;
    memset(&attr, 0, sizeof(attr));
    attr.hash = calc_handler;
    attr.cmp = cmp_handler;
    attr.len_base = 31;

    mln_hash_t *h = mln_hash_new(&attr);
    CHECK(h != NULL, "mln_hash_new returns non-NULL");
    mln_hash_free(h, M_HASH_F_NONE);

    h = mln_hash_new_fast(calc_handler, cmp_handler, NULL, NULL, 31, 0, 0);
    CHECK(h != NULL, "mln_hash_new_fast returns non-NULL");
    mln_hash_free(h, M_HASH_F_NONE);
}

static void test_init_destroy(void)
{
    mln_hash_t h;
    struct mln_hash_attr attr;
    memset(&attr, 0, sizeof(attr));
    attr.hash = calc_handler;
    attr.cmp = cmp_handler;
    attr.len_base = 31;

    int rc = mln_hash_init(&h, &attr);
    CHECK(rc == 0, "mln_hash_init returns 0");
    mln_hash_destroy(&h, M_HASH_F_NONE);

    rc = mln_hash_init_fast(&h, calc_handler, cmp_handler, NULL, NULL, 31, 0, 0);
    CHECK(rc == 0, "mln_hash_init_fast returns 0");
    mln_hash_destroy(&h, M_HASH_F_NONE);
}

static void test_init_invalid(void)
{
    mln_hash_t h;
    struct mln_hash_attr attr;
    memset(&attr, 0, sizeof(attr));
    attr.hash = calc_handler;
    attr.cmp = cmp_handler;
    attr.len_base = 0;
    int rc = mln_hash_init(&h, &attr);
    CHECK(rc == -1, "init with len_base=0 fails");

    attr.len_base = 31;
    attr.hash = NULL;
    rc = mln_hash_init(&h, &attr);
    CHECK(rc == -1, "init with hash=NULL fails");

    attr.hash = calc_handler;
    attr.cmp = NULL;
    rc = mln_hash_init(&h, &attr);
    CHECK(rc == -1, "init with cmp=NULL fails");
}

static void test_insert_search(void)
{
    mln_hash_t *h = mln_hash_new_fast(calc_handler, cmp_handler, NULL, NULL, 101, 0, 0);
    CHECK(h != NULL, "insert_search: new");
    int keys[] = {1, 2, 3, 42, 100};
    int i;
    for (i = 0; i < 5; i++) {
        CHECK(mln_hash_insert(h, &keys[i], &keys[i]) == 0, "insert_search: insert");
    }
    for (i = 0; i < 5; i++) {
        int *v = (int *)mln_hash_search(h, &keys[i]);
        CHECK(v != NULL && *v == keys[i], "insert_search: search found");
    }
    int missing = 999;
    CHECK(mln_hash_search(h, &missing) == NULL, "insert_search: search missing");
    mln_hash_free(h, M_HASH_F_NONE);
}

static void test_insert_with_malloc_val(void)
{
    mln_hash_t *h = mln_hash_new_fast(calc_handler, cmp_handler, NULL, free_handler, 31, 0, 0);
    CHECK(h != NULL, "insert_malloc: new");
    int key = 7;
    int *val = (int *)malloc(sizeof(int));
    *val = 77;
    CHECK(mln_hash_insert(h, &key, val) == 0, "insert_malloc: insert");
    int *found = (int *)mln_hash_search(h, &key);
    CHECK(found != NULL && *found == 77, "insert_malloc: search");
    mln_hash_free(h, M_HASH_F_VAL);
}

static void test_remove(void)
{
    mln_hash_t *h = mln_hash_new_fast(calc_handler, cmp_handler, NULL, NULL, 101, 0, 0);
    int keys[] = {10, 20, 30, 40, 50};
    int i;
    for (i = 0; i < 5; i++)
        mln_hash_insert(h, &keys[i], &keys[i]);

    mln_hash_remove(h, &keys[2], M_HASH_F_NONE); /* remove 30 */
    CHECK(mln_hash_search(h, &keys[2]) == NULL, "remove: gone");
    CHECK(mln_hash_search(h, &keys[0]) != NULL, "remove: others remain 0");
    CHECK(mln_hash_search(h, &keys[4]) != NULL, "remove: others remain 4");

    /* remove non-existent */
    int missing = 999;
    mln_hash_remove(h, &missing, M_HASH_F_NONE);
    CHECK(1, "remove: non-existent no crash");

    mln_hash_free(h, M_HASH_F_NONE);
}

static void test_remove_with_free(void)
{
    mln_hash_t *h = mln_hash_new_fast(calc_handler, cmp_handler, NULL, free_handler, 31, 0, 0);
    int key = 5;
    int *val = (int *)malloc(sizeof(int));
    *val = 55;
    mln_hash_insert(h, &key, val);
    mln_hash_remove(h, &key, M_HASH_F_VAL);
    CHECK(mln_hash_search(h, &key) == NULL, "remove_with_free: val freed");
    mln_hash_free(h, M_HASH_F_NONE);
}

static void test_key_exist(void)
{
    mln_hash_t *h = mln_hash_new_fast(calc_handler, cmp_handler, NULL, NULL, 101, 0, 0);
    int k1 = 1, k2 = 2, k3 = 999;
    mln_hash_insert(h, &k1, &k1);
    mln_hash_insert(h, &k2, &k2);
    CHECK(mln_hash_key_exist(h, &k1) == 1, "key_exist: present");
    CHECK(mln_hash_key_exist(h, &k3) == 0, "key_exist: absent");
    mln_hash_free(h, M_HASH_F_NONE);
}

static void test_change_value(void)
{
    mln_hash_t *h = mln_hash_new_fast(calc_handler, cmp_handler, NULL, NULL, 31, 0, 0);
    int key = 10, val1 = 100, val2 = 200;
    mln_hash_insert(h, &key, &val1);
    int *old = (int *)mln_hash_change_value(h, &key, &val2);
    CHECK(old == &val1, "change_value: returns old val");
    int *cur = (int *)mln_hash_search(h, &key);
    CHECK(cur == &val2, "change_value: new val set");

    int missing = 999;
    CHECK(mln_hash_change_value(h, &missing, &val2) == NULL, "change_value: missing key returns NULL");
    mln_hash_free(h, M_HASH_F_NONE);
}

static void test_update_existing(void)
{
    mln_hash_t *h = mln_hash_new_fast(calc_handler, cmp_handler, NULL, NULL, 31, 0, 0);
    int key = 10, val = 100;
    mln_hash_insert(h, &key, &val);

    int new_k = 10, new_v = 200;
    void *pk = &new_k, *pv = &new_v;
    int rc = mln_hash_update(h, &pk, &pv);
    CHECK(rc == 0, "update_existing: returns 0");
    CHECK(*(int *)pk == 10, "update_existing: old key returned");
    CHECK(*(int *)pv == 100, "update_existing: old val returned");
    int *found = (int *)mln_hash_search(h, &key);
    CHECK(found != NULL && *found == 200, "update_existing: new val found");
    mln_hash_free(h, M_HASH_F_NONE);
}

static void test_update_new(void)
{
    mln_hash_t *h = mln_hash_new_fast(calc_handler, cmp_handler, NULL, NULL, 101, 0, 0);
    int key = 42, val = 420;
    void *pk = &key, *pv = &val;
    int rc = mln_hash_update(h, &pk, &pv);
    CHECK(rc == 0, "update_new: returns 0");
    CHECK(pk == NULL, "update_new: pk set to NULL");
    CHECK(pv == NULL, "update_new: pv set to NULL");
    int search_key = 42;
    int *found = (int *)mln_hash_search(h, &search_key);
    CHECK(found != NULL && *found == 420, "update_new: inserted");
    mln_hash_free(h, M_HASH_F_NONE);
}

static void test_iterate(void)
{
    mln_hash_t *h = mln_hash_new_fast(calc_handler, cmp_handler, NULL, NULL, 101, 0, 0);
    int keys[20];
    int i;
    for (i = 0; i < 20; i++) {
        keys[i] = i;
        mln_hash_insert(h, &keys[i], &keys[i]);
    }
    int count = 0;
    int rc = mln_hash_iterate(h, iterate_count_cb, &count);
    CHECK(rc == 0, "iterate: returns 0");
    CHECK(count == 20, "iterate: visits all 20");

    count = 0;
    rc = mln_hash_iterate(h, iterate_abort_cb, &count);
    CHECK(rc == -1, "iterate: abort returns -1");
    CHECK(count == 3, "iterate: aborted after 3");
    mln_hash_free(h, M_HASH_F_NONE);
}

static void test_iterate_with_remove(void)
{
    mln_hash_t *h = mln_hash_new_fast(calc_handler, cmp_handler, NULL, NULL, 101, 0, 0);
    int keys[20];
    int i;
    for (i = 0; i < 20; i++) {
        keys[i] = i;
        mln_hash_insert(h, &keys[i], &keys[i]);
    }
    mln_hash_iterate(h, iterate_remove_cb, NULL);
    int even_found = 0, odd_found = 0;
    for (i = 0; i < 20; i++) {
        if (mln_hash_search(h, &keys[i]) != NULL) {
            if (i % 2 == 0) even_found++;
            else odd_found++;
        }
    }
    CHECK(even_found == 0, "iterate_remove: even keys removed");
    CHECK(odd_found == 10, "iterate_remove: odd keys remain");
    mln_hash_free(h, M_HASH_F_NONE);
}

/*
 * Test: remove current node during iterate with actual key/val freers (M_HASH_F_KV).
 * Verifies:
 *   1) The freers are NOT called during the handler (deferred)
 *   2) The freers ARE called after the handler returns
 *   3) All N nodes are still visited
 *   4) The removed key is gone; remaining keys are intact
 */
static void test_iterate_remove_current_deferred_free(void)
{
    mln_hash_t *h = mln_hash_new_fast(calc_handler, cmp_handler,
                                       counting_freer, counting_freer,
                                       101, 0, 0);
    CHECK(h != NULL, "iterate_remove_deferred: new");
    int i;
    for (i = 0; i < 10; i++) {
        int *k = (int *)malloc(sizeof(int));
        int *v = (int *)malloc(sizeof(int));
        *k = i;
        *v = i * 100;
        mln_hash_insert(h, k, v);
    }

    g_free_count = 0;
    g_in_handler_flag = 0;
    g_freed_in_handler = 0;

    int count = 0;
    int rc = mln_hash_iterate(h, iterate_remove_current_kv_cb, &count);
    CHECK(rc == 0, "iterate_remove_deferred: returns 0");
    CHECK(count == 10, "iterate_remove_deferred: all 10 nodes visited");
    CHECK(g_freed_in_handler == 0,
          "iterate_remove_deferred: free NOT called during handler (deferred)");
    CHECK(g_free_count == 2,
          "iterate_remove_deferred: key+val freed after handler returned");

    /* key=5 must be gone */
    int sk = 5;
    CHECK(mln_hash_search(h, &sk) == NULL,
          "iterate_remove_deferred: removed key=5 gone");

    /* remaining 9 keys must still be findable */
    int all_others = 1;
    for (i = 0; i < 10; i++) {
        if (i == 5) continue;
        int ssk = i;
        if (mln_hash_search(h, &ssk) == NULL) { all_others = 0; break; }
    }
    CHECK(all_others, "iterate_remove_deferred: other 9 keys remain");

    mln_hash_free(h, M_HASH_F_KV);
}

/*
 * Test: remove multiple current nodes during iterate with actual free.
 * All even-keyed nodes are removed (with M_HASH_F_KV).
 * Verifies that every node is visited, freed correctly, and odd keys survive.
 */
static void test_iterate_remove_multiple_current(void)
{
    mln_hash_t *h = mln_hash_new_fast(calc_handler, cmp_handler,
                                       counting_freer, counting_freer,
                                       101, 0, 0);
    CHECK(h != NULL, "iterate_remove_multi: new");
    int i;
    for (i = 0; i < 20; i++) {
        int *k = (int *)malloc(sizeof(int));
        int *v = (int *)malloc(sizeof(int));
        *k = i;
        *v = i * 100;
        mln_hash_insert(h, k, v);
    }

    g_free_count = 0;
    int count = 0;
    mln_hash_iterate(h, iterate_remove_even_kv_cb, &count);
    CHECK(count == 20, "iterate_remove_multi: all 20 visited");
    /* 10 even entries × 2 (key + val) = 20 frees */
    CHECK(g_free_count == 20,
          "iterate_remove_multi: 10 keys + 10 vals freed");

    int even_count = 0, odd_count = 0;
    for (i = 0; i < 20; i++) {
        int sk = i;
        if (mln_hash_search(h, &sk) != NULL) {
            if (i % 2 == 0) even_count++;
            else odd_count++;
        }
    }
    CHECK(even_count == 0, "iterate_remove_multi: even keys gone");
    CHECK(odd_count == 10, "iterate_remove_multi: odd keys remain");

    mln_hash_free(h, M_HASH_F_KV);
}

/*
 * Test: update current node via mln_hash_update during iterate.
 * Verifies:
 *   1) The in-place key/val swap works without breaking iteration
 *   2) All nodes are visited
 *   3) The updated node has the new value
 *   4) Other nodes are unmodified
 */
static void test_iterate_update_current(void)
{
    mln_hash_t *h = mln_hash_new_fast(calc_handler, cmp_handler, NULL, NULL, 101, 0, 0);
    CHECK(h != NULL, "iterate_update_current: new");
    int keys[10], vals[10];
    int i;
    for (i = 0; i < 10; i++) {
        keys[i] = i;
        vals[i] = i * 100;
        mln_hash_insert(h, &keys[i], &vals[i]);
    }

    g_update_new_val = 999;
    int count = 0;
    int rc = mln_hash_iterate(h, iterate_update_current_cb, &count);
    CHECK(rc == 0, "iterate_update_current: returns 0");
    CHECK(count == 10, "iterate_update_current: all 10 nodes visited");

    /* key=5 should now map to g_update_new_val (999) */
    int sk = 5;
    int *v = (int *)mln_hash_search(h, &sk);
    CHECK(v != NULL && *v == 999,
          "iterate_update_current: key=5 val updated to 999");

    /* all other keys should retain original values */
    int all_ok = 1;
    for (i = 0; i < 10; i++) {
        if (i == 5) continue;
        v = (int *)mln_hash_search(h, &keys[i]);
        if (v == NULL || *v != i * 100) { all_ok = 0; break; }
    }
    CHECK(all_ok, "iterate_update_current: other vals unchanged");

    mln_hash_free(h, M_HASH_F_NONE);
}

/*
 * Test: remove a non-current node (the next in the iter chain) during iterate.
 * This verifies that mln_hash_iterate reads next_idx AFTER the handler returns,
 * so that immediate removals of non-current nodes are reflected correctly.
 *
 * Setup: insert keys 0-9 in order (iter chain: 0->1->2->...->9).
 * Handler: when visiting key K, removes K+1 if K+1 is even and < 10.
 *   - visiting 1 => removes 2 (immediate, not deferred)
 *   - visiting 3 => removes 4
 *   - visiting 5 => removes 6
 *   - visiting 7 => removes 8
 * Expected visits: 0, 1, 3, 5, 7, 9 = 6 total (even keys 2,4,6,8 removed
 * before being visited).
 */
static void test_iterate_remove_next_node(void)
{
    mln_hash_t *h = mln_hash_new_fast(calc_handler, cmp_handler, NULL, NULL, 101, 0, 0);
    CHECK(h != NULL, "iterate_remove_next: new");
    int keys[10];
    int i;
    for (i = 0; i < 10; i++) {
        keys[i] = i;
        mln_hash_insert(h, &keys[i], &keys[i]);
    }

    int count = 0;
    int rc = mln_hash_iterate(h, iterate_remove_next_cb, &count);
    CHECK(rc == 0, "iterate_remove_next: returns 0");
    CHECK(count == 6, "iterate_remove_next: visited 6 nodes (skipped removed 2,4,6,8)");

    /* Even keys 2,4,6,8 should be gone; 0 is even but was never a "next" target */
    int sk;
    sk = 0; CHECK(mln_hash_search(h, &sk) != NULL, "iterate_remove_next: key 0 remains");
    sk = 2; CHECK(mln_hash_search(h, &sk) == NULL, "iterate_remove_next: key 2 removed");
    sk = 4; CHECK(mln_hash_search(h, &sk) == NULL, "iterate_remove_next: key 4 removed");
    sk = 6; CHECK(mln_hash_search(h, &sk) == NULL, "iterate_remove_next: key 6 removed");
    sk = 8; CHECK(mln_hash_search(h, &sk) == NULL, "iterate_remove_next: key 8 removed");
    /* Odd keys should all remain */
    sk = 1; CHECK(mln_hash_search(h, &sk) != NULL, "iterate_remove_next: key 1 remains");
    sk = 3; CHECK(mln_hash_search(h, &sk) != NULL, "iterate_remove_next: key 3 remains");
    sk = 5; CHECK(mln_hash_search(h, &sk) != NULL, "iterate_remove_next: key 5 remains");
    sk = 7; CHECK(mln_hash_search(h, &sk) != NULL, "iterate_remove_next: key 7 remains");
    sk = 9; CHECK(mln_hash_search(h, &sk) != NULL, "iterate_remove_next: key 9 remains");

    mln_hash_free(h, M_HASH_F_NONE);
}

static void test_search_iterator(void)
{
    mln_hash_t *h = mln_hash_new_fast(calc_handler, cmp_handler, NULL, NULL, 101, 0, 0);
    int k1 = 10, v1 = 1;
    int k2 = 10, v2 = 2;
    int k3 = 10, v3 = 3;
    mln_hash_insert(h, &k1, &v1);
    mln_hash_insert(h, &k2, &v2);
    mln_hash_insert(h, &k3, &v3);

    int *ctx = NULL;
    int search_key = 10;
    int found = 0;
    while (mln_hash_search_iterator(h, &search_key, (int **)&ctx) != NULL)
        found++;
    CHECK(found == 3, "search_iterator: found all 3 duplicates");

    int missing = 999;
    ctx = NULL;
    CHECK(mln_hash_search_iterator(h, &missing, (int **)&ctx) == NULL, "search_iterator: missing returns NULL");
    mln_hash_free(h, M_HASH_F_NONE);
}

static void test_reset(void)
{
    mln_hash_t *h = mln_hash_new_fast(calc_handler, cmp_handler, NULL, NULL, 101, 0, 0);
    int keys[10];
    int i;
    for (i = 0; i < 10; i++) {
        keys[i] = i;
        mln_hash_insert(h, &keys[i], &keys[i]);
    }
    mln_hash_reset(h, M_HASH_F_NONE);
    int count = 0;
    mln_hash_iterate(h, iterate_count_cb, &count);
    CHECK(count == 0, "reset: empty after reset");
    CHECK(mln_hash_search(h, &keys[0]) == NULL, "reset: search returns NULL");

    /* Can insert again after reset */
    CHECK(mln_hash_insert(h, &keys[0], &keys[0]) == 0, "reset: can insert after reset");
    CHECK(mln_hash_search(h, &keys[0]) != NULL, "reset: can search after reset");
    mln_hash_free(h, M_HASH_F_NONE);
}

static void test_expandable(void)
{
    mln_hash_t *h = mln_hash_new_fast(calc_handler, cmp_handler, NULL, NULL, 7, 1, 0);
    CHECK(h != NULL, "expandable: new");
    int keys[500];
    int i;
    for (i = 0; i < 500; i++) {
        keys[i] = i;
        CHECK(mln_hash_insert(h, &keys[i], &keys[i]) == 0, "expandable: insert");
    }
    for (i = 0; i < 500; i++) {
        int *v = (int *)mln_hash_search(h, &keys[i]);
        CHECK(v != NULL && *v == i, "expandable: search after expansion");
    }
    CHECK(h->len > 7, "expandable: table grew");
    mln_hash_free(h, M_HASH_F_NONE);
}

static void test_expandable_with_prime(void)
{
    mln_hash_t *h = mln_hash_new_fast(calc_handler, cmp_handler, NULL, NULL, 7, 1, 1);
    CHECK(h != NULL, "expandable_prime: new");
    int keys[200];
    int i;
    for (i = 0; i < 200; i++) {
        keys[i] = i;
        CHECK(mln_hash_insert(h, &keys[i], &keys[i]) == 0, "expandable_prime: insert");
    }
    for (i = 0; i < 200; i++) {
        int *v = (int *)mln_hash_search(h, &keys[i]);
        CHECK(v != NULL && *v == i, "expandable_prime: search");
    }
    mln_hash_free(h, M_HASH_F_NONE);
}

static void test_non_expandable_full(void)
{
    mln_hash_t *h = mln_hash_new_fast(calc_handler, cmp_handler, NULL, NULL, 5, 0, 0);
    CHECK(h != NULL, "non_expandable_full: new");
    int keys[10];
    int i, inserted = 0;
    for (i = 0; i < 10; i++) {
        keys[i] = i;
        if (mln_hash_insert(h, &keys[i], &keys[i]) == 0)
            inserted++;
    }
    CHECK(inserted < 10, "non_expandable_full: not all inserted into small table");
    CHECK(inserted >= 3, "non_expandable_full: at least some inserted");
    mln_hash_free(h, M_HASH_F_NONE);
}

static void test_tombstone_reuse(void)
{
    mln_hash_t *h = mln_hash_new_fast(calc_handler, cmp_handler, NULL, NULL, 101, 0, 0);
    int keys[50];
    int i;
    for (i = 0; i < 50; i++) {
        keys[i] = i;
        mln_hash_insert(h, &keys[i], &keys[i]);
    }
    /* Remove all */
    for (i = 0; i < 50; i++)
        mln_hash_remove(h, &keys[i], M_HASH_F_NONE);

    /* Re-insert: should reuse tombstone slots */
    for (i = 0; i < 50; i++) {
        keys[i] = i + 1000;
        CHECK(mln_hash_insert(h, &keys[i], &keys[i]) == 0, "tombstone_reuse: re-insert");
    }
    for (i = 0; i < 50; i++) {
        int *v = (int *)mln_hash_search(h, &keys[i]);
        CHECK(v != NULL, "tombstone_reuse: search after re-insert");
    }
    mln_hash_free(h, M_HASH_F_NONE);
}

static void test_hash_flags(void)
{
    /* M_HASH_F_KEY */
    mln_hash_t *h = mln_hash_new_fast(calc_handler, cmp_handler, free_handler, NULL, 31, 0, 0);
    int *key = (int *)malloc(sizeof(int));
    *key = 10;
    int val = 100;
    mln_hash_insert(h, key, &val);
    mln_hash_free(h, M_HASH_F_KEY);
    CHECK(1, "hash_flags: M_HASH_F_KEY free no crash");

    /* M_HASH_F_KV */
    h = mln_hash_new_fast(calc_handler, cmp_handler, free_handler, free_handler, 31, 0, 0);
    key = (int *)malloc(sizeof(int));
    *key = 10;
    int *val2 = (int *)malloc(sizeof(int));
    *val2 = 100;
    mln_hash_insert(h, key, val2);
    mln_hash_free(h, M_HASH_F_KV);
    CHECK(1, "hash_flags: M_HASH_F_KV free no crash");
}

static void test_free_null(void)
{
    mln_hash_free(NULL, M_HASH_F_NONE);
    CHECK(1, "free_null: no crash");
    mln_hash_destroy(NULL, M_HASH_F_NONE);
    CHECK(1, "destroy_null: no crash");
}

static void test_single_element(void)
{
    mln_hash_t *h = mln_hash_new_fast(calc_handler, cmp_handler, NULL, NULL, 31, 0, 0);
    int key = 42, val = 420;
    mln_hash_insert(h, &key, &val);
    CHECK(*(int *)mln_hash_search(h, &key) == 420, "single: search");
    mln_hash_remove(h, &key, M_HASH_F_NONE);
    CHECK(mln_hash_search(h, &key) == NULL, "single: removed");
    mln_hash_free(h, M_HASH_F_NONE);
}

static void test_expand_then_reduce(void)
{
    mln_hash_t *h = mln_hash_new_fast(calc_handler, cmp_handler, NULL, NULL, 7, 1, 0);
    int keys[200];
    int i;
    /* Expand: insert many */
    for (i = 0; i < 200; i++) {
        keys[i] = i;
        mln_hash_insert(h, &keys[i], &keys[i]);
    }
    mln_u64_t big_len = h->len;
    /* Reduce: remove most */
    for (i = 10; i < 200; i++)
        mln_hash_remove(h, &keys[i], M_HASH_F_NONE);
    /* Trigger reduce via insert */
    int extra_key = 9999;
    mln_hash_insert(h, &extra_key, &extra_key);
    CHECK(h->len <= big_len, "expand_reduce: table shrank or stayed");
    /* Verify remaining */
    for (i = 0; i < 10; i++) {
        CHECK(mln_hash_search(h, &keys[i]) != NULL, "expand_reduce: remaining found");
    }
    CHECK(mln_hash_search(h, &extra_key) != NULL, "expand_reduce: extra key found");
    mln_hash_free(h, M_HASH_F_NONE);
}

static void test_collision_heavy(void)
{
    /* All keys hash to same bucket (% 7 == 0) */
    mln_hash_t *h = mln_hash_new_fast(calc_handler, cmp_handler, NULL, NULL, 101, 0, 0);
    int keys[20];
    int i;
    for (i = 0; i < 20; i++) {
        keys[i] = i * 101; /* all hash to 0 */
        mln_hash_insert(h, &keys[i], &keys[i]);
    }
    for (i = 0; i < 20; i++) {
        int *v = (int *)mln_hash_search(h, &keys[i]);
        CHECK(v != NULL && *v == keys[i], "collision_heavy: search");
    }
    /* Remove middle */
    mln_hash_remove(h, &keys[10], M_HASH_F_NONE);
    CHECK(mln_hash_search(h, &keys[10]) == NULL, "collision_heavy: removed");
    /* Others still found after tombstone */
    CHECK(mln_hash_search(h, &keys[15]) != NULL, "collision_heavy: after tombstone search");
    mln_hash_free(h, M_HASH_F_NONE);
}

/* -- Performance benchmark -- */
static void bench_performance(void)
{
    #define BENCH_N 1000000
    struct timespec t0, t1;
    double insert_time, search_time, remove_time;
    int i;
    int *keys = (int *)malloc(BENCH_N * sizeof(int));
    for (i = 0; i < BENCH_N; i++) keys[i] = i;

    mln_hash_t *h = mln_hash_new_fast(calc_handler, cmp_handler, NULL, NULL, 1500007, 0, 0);
    CHECK(h != NULL, "bench: new");

    clock_gettime(CLOCK_MONOTONIC, &t0);
    for (i = 0; i < BENCH_N; i++)
        mln_hash_insert(h, &keys[i], &keys[i]);
    clock_gettime(CLOCK_MONOTONIC, &t1);
    insert_time = (t1.tv_sec - t0.tv_sec) + (t1.tv_nsec - t0.tv_nsec) / 1e9;
    printf("  Insert %d: %.6f s (%.0f ops/s)\n", BENCH_N, insert_time, BENCH_N / insert_time);

    clock_gettime(CLOCK_MONOTONIC, &t0);
    for (i = 0; i < BENCH_N; i++) {
        volatile void *v = mln_hash_search(h, &keys[i]);
        (void)v;
    }
    clock_gettime(CLOCK_MONOTONIC, &t1);
    search_time = (t1.tv_sec - t0.tv_sec) + (t1.tv_nsec - t0.tv_nsec) / 1e9;
    printf("  Search %d: %.6f s (%.0f ops/s)\n", BENCH_N, search_time, BENCH_N / search_time);

    clock_gettime(CLOCK_MONOTONIC, &t0);
    for (i = 0; i < BENCH_N; i++)
        mln_hash_remove(h, &keys[i], M_HASH_F_NONE);
    clock_gettime(CLOCK_MONOTONIC, &t1);
    remove_time = (t1.tv_sec - t0.tv_sec) + (t1.tv_nsec - t0.tv_nsec) / 1e9;
    printf("  Remove %d: %.6f s (%.0f ops/s)\n", BENCH_N, remove_time, BENCH_N / remove_time);

    CHECK(insert_time < 1.0, "bench: insert < 1s");
    CHECK(search_time < 1.0, "bench: search < 1s");
    CHECK(remove_time < 1.0, "bench: remove < 1s");

    mln_hash_free(h, M_HASH_F_NONE);
    free(keys);
}

/* -- Stability test -- */
static void test_stability(void)
{
    #define STAB_N 10000
    #define STAB_ROUNDS 100
    int r, i;
    int keys[STAB_N];
    int ok = 1;

    for (r = 0; r < STAB_ROUNDS && ok; r++) {
        mln_hash_t *h = mln_hash_new_fast(calc_handler, cmp_handler, NULL, NULL, 31, 1, 1);
        if (h == NULL) { ok = 0; break; }
        for (i = 0; i < STAB_N; i++) {
            keys[i] = r * STAB_N + i;
            if (mln_hash_insert(h, &keys[i], &keys[i]) < 0) { ok = 0; break; }
        }
        if (!ok) { mln_hash_free(h, M_HASH_F_NONE); break; }
        for (i = 0; i < STAB_N; i++) {
            if (mln_hash_search(h, &keys[i]) == NULL) { ok = 0; break; }
        }
        if (!ok) { mln_hash_free(h, M_HASH_F_NONE); break; }
        /* Remove half */
        for (i = 0; i < STAB_N / 2; i++)
            mln_hash_remove(h, &keys[i], M_HASH_F_NONE);
        /* Verify remaining half */
        for (i = STAB_N / 2; i < STAB_N; i++) {
            if (mln_hash_search(h, &keys[i]) == NULL) { ok = 0; break; }
        }
        mln_hash_free(h, M_HASH_F_NONE);
        if (!ok) break;
    }
    CHECK(ok, "stability: 100 rounds of 10K insert/search/remove");
}

int main(int argc, char *argv[])
{
    printf("=== Hash Table Tests ===\n");

    test_new_free();
    test_init_destroy();
    test_init_invalid();
    test_insert_search();
    test_insert_with_malloc_val();
    test_remove();
    test_remove_with_free();
    test_key_exist();
    test_change_value();
    test_update_existing();
    test_update_new();
    test_iterate();
    test_iterate_with_remove();
    test_iterate_remove_current_deferred_free();
    test_iterate_remove_multiple_current();
    test_iterate_update_current();
    test_iterate_remove_next_node();
    test_search_iterator();
    test_reset();
    test_expandable();
    test_expandable_with_prime();
    test_non_expandable_full();
    test_tombstone_reuse();
    test_hash_flags();
    test_free_null();
    test_single_element();
    test_expand_then_reduce();
    test_collision_heavy();

    printf("\n=== Performance Benchmark ===\n");
    bench_performance();

    printf("\n=== Stability Test ===\n");
    test_stability();

    printf("\n=== Results: %d/%d passed", pass_nr, test_nr);
    if (fail_nr > 0) printf(", %d FAILED", fail_nr);
    printf(" ===\n");
    return fail_nr > 0 ? 1 : 0;
}
