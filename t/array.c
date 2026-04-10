#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "mln_array.h"
#include "mln_func.h"

static int free_count = 0;

typedef struct {
    int i1;
    int i2;
} test_t;

MLN_FUNC_VOID(static, void, test_free, (void *data), (data), {
    (void)data;
    ++free_count;
})

/*
 * Simple pool: wraps malloc/free so we can exercise pool_init / pool_new paths.
 */
MLN_FUNC(static, void *, pool_alloc, (void *pool, mln_size_t size), (pool, size), {
    (void)pool;
    return malloc(size);
})

MLN_FUNC_VOID(static, void, pool_dealloc, (void *ptr), (ptr), {
    free(ptr);
})

/* ---- test helpers ---- */

MLN_FUNC_VOID(static, void, test_init_destroy, (void), (), {
    test_t *t;
    mln_size_t i;
    mln_array_t arr;

    assert(mln_array_init(&arr, NULL, sizeof(test_t), 1) == 0);

    t = (test_t *)mln_array_push(&arr);
    assert(t != NULL);
    t->i1 = 0;

    t = (test_t *)mln_array_pushn(&arr, 9);
    assert(t != NULL);
    for (i = 0; i < 9; ++i)
        t[i].i1 = i + 1;

    assert(mln_array_nelts(&arr) == 10);

    t = (test_t *)mln_array_elts(&arr);
    for (i = 0; i < mln_array_nelts(&arr); ++i)
        assert(t[i].i1 == (int)i);

    mln_array_destroy(&arr);
    printf("PASS test_init_destroy\n");
})

MLN_FUNC_VOID(static, void, test_new_free, (void), (), {
    test_t *t;
    mln_array_t *arr;

    arr = mln_array_new(NULL, sizeof(test_t), 4);
    assert(arr != NULL);

    t = (test_t *)mln_array_push(arr);
    assert(t != NULL);
    t->i1 = 42;

    assert(mln_array_nelts(arr) == 1);
    t = (test_t *)mln_array_elts(arr);
    assert(t[0].i1 == 42);

    mln_array_free(arr);
    printf("PASS test_new_free\n");
})

MLN_FUNC_VOID(static, void, test_pop, (void), (), {
    test_t *t;
    mln_array_t arr;

    free_count = 0;
    assert(mln_array_init(&arr, test_free, sizeof(test_t), 4) == 0);

    t = (test_t *)mln_array_push(&arr);
    assert(t != NULL);
    t->i1 = 1;

    t = (test_t *)mln_array_push(&arr);
    assert(t != NULL);
    t->i1 = 2;

    assert(mln_array_nelts(&arr) == 2);

    mln_array_pop(&arr);
    assert(mln_array_nelts(&arr) == 1);
    assert(free_count == 1);

    mln_array_pop(&arr);
    assert(mln_array_nelts(&arr) == 0);
    assert(free_count == 2);

    /* popping an empty array should be a no-op */
    mln_array_pop(&arr);
    assert(mln_array_nelts(&arr) == 0);
    assert(free_count == 2);

    mln_array_destroy(&arr);
    printf("PASS test_pop\n");
})

MLN_FUNC_VOID(static, void, test_reset, (void), (), {
    test_t *t;
    mln_size_t i;
    mln_array_t arr;

    free_count = 0;
    assert(mln_array_init(&arr, test_free, sizeof(test_t), 4) == 0);

    for (i = 0; i < 5; ++i) {
        t = (test_t *)mln_array_push(&arr);
        assert(t != NULL);
        t->i1 = (int)i;
    }
    assert(mln_array_nelts(&arr) == 5);

    mln_array_reset(&arr);
    assert(mln_array_nelts(&arr) == 0);
    assert(free_count == 5);

    /* array is still usable after reset */
    t = (test_t *)mln_array_push(&arr);
    assert(t != NULL);
    t->i1 = 99;
    assert(mln_array_nelts(&arr) == 1);

    mln_array_destroy(&arr);
    printf("PASS test_reset\n");
})

MLN_FUNC_VOID(static, void, test_grow, (void), (), {
    test_t *t;
    mln_array_t arr;

    assert(mln_array_init(&arr, NULL, sizeof(test_t), 0) == 0);
    assert(mln_array_nelts(&arr) == 0);

    /* manually grow the array before any push */
    assert(mln_array_grow(&arr, 8) == 0);

    t = (test_t *)mln_array_push(&arr);
    assert(t != NULL);
    t->i1 = 100;
    assert(mln_array_nelts(&arr) == 1);

    mln_array_destroy(&arr);
    printf("PASS test_grow\n");
})

MLN_FUNC_VOID(static, void, test_macro_push_pop, (void), (), {
    test_t *t;
    mln_size_t i;
    mln_array_t arr;

    free_count = 0;
    assert(mln_array_init(&arr, test_free, sizeof(test_t), 2) == 0);

    for (i = 0; i < 10; ++i) {
        MLN_ARRAY_PUSH(&arr, t);
        assert(t != NULL);
        t->i1 = (int)i;
    }
    assert(mln_array_nelts(&arr) == 10);

    t = (test_t *)mln_array_elts(&arr);
    for (i = 0; i < 10; ++i)
        assert(t[i].i1 == (int)i);

    /* pop 5 elements using macro */
    for (i = 0; i < 5; ++i)
        MLN_ARRAY_POP(&arr);

    assert(mln_array_nelts(&arr) == 5);
    assert(free_count == 5);

    mln_array_destroy(&arr);
    printf("PASS test_macro_push_pop\n");
})

MLN_FUNC_VOID(static, void, test_macro_pushn, (void), (), {
    test_t *t;
    mln_size_t i;
    mln_array_t arr;

    assert(mln_array_init(&arr, NULL, sizeof(test_t), 2) == 0);

    MLN_ARRAY_PUSHN(&arr, 8, t);
    assert(t != NULL);
    for (i = 0; i < 8; ++i)
        t[i].i1 = (int)(i * 10);

    assert(mln_array_nelts(&arr) == 8);
    t = (test_t *)mln_array_elts(&arr);
    for (i = 0; i < 8; ++i)
        assert(t[i].i1 == (int)(i * 10));

    mln_array_destroy(&arr);
    printf("PASS test_macro_pushn\n");
})

MLN_FUNC_VOID(static, void, test_pool_init_destroy, (void), (), {
    int dummy_pool = 0; /* dummy pool object */
    test_t *t;
    mln_size_t i;
    mln_array_t arr;

    free_count = 0;
    assert(mln_array_pool_init(&arr, test_free, sizeof(test_t), 4,
                               &dummy_pool, pool_alloc, pool_dealloc) == 0);

    for (i = 0; i < 6; ++i) {
        t = (test_t *)mln_array_push(&arr);
        assert(t != NULL);
        t->i1 = (int)i;
    }
    assert(mln_array_nelts(&arr) == 6);

    t = (test_t *)mln_array_elts(&arr);
    for (i = 0; i < 6; ++i)
        assert(t[i].i1 == (int)i);

    mln_array_destroy(&arr);
    assert(free_count == 6);
    printf("PASS test_pool_init_destroy\n");
})

MLN_FUNC_VOID(static, void, test_pool_new_free, (void), (), {
    int dummy_pool = 0;
    test_t *t;
    mln_array_t *arr;

    arr = mln_array_pool_new(NULL, sizeof(test_t), 4,
                             &dummy_pool, pool_alloc, pool_dealloc);
    assert(arr != NULL);

    t = (test_t *)mln_array_push(arr);
    assert(t != NULL);
    t->i1 = 7;
    assert(mln_array_nelts(arr) == 1);

    mln_array_free(arr);
    printf("PASS test_pool_new_free\n");
})

MLN_FUNC_VOID(static, void, test_free_callback, (void), (), {
    test_t *t;
    mln_array_t *arr;

    free_count = 0;
    arr = mln_array_new(test_free, sizeof(test_t), 4);
    assert(arr != NULL);

    t = (test_t *)mln_array_push(arr);
    assert(t != NULL);
    t->i1 = 1;

    t = (test_t *)mln_array_push(arr);
    assert(t != NULL);
    t->i1 = 2;

    t = (test_t *)mln_array_push(arr);
    assert(t != NULL);
    t->i1 = 3;

    /* free should invoke the callback for every live element, then free arr */
    mln_array_free(arr);
    assert(free_count == 3);
    printf("PASS test_free_callback\n");
})

int main(void)
{
    test_init_destroy();
    test_new_free();
    test_pop();
    test_reset();
    test_grow();
    test_macro_push_pop();
    test_macro_pushn();
    test_pool_init_destroy();
    test_pool_new_free();
    test_free_callback();

    printf("ALL PASSED\n");
    return 0;
}

