## Array



### Header File

```c
#include "mln_array.h"
```



### Module

`array`



### Functions/Macros

#### mln_array_init

```c
int mln_array_init(mln_array_t *arr, array_free free, mln_size_t size, mln_size_t nalloc);

typedef void (*array_free)(void *);
```

Description: Initialize a given array `arr`. The parameter meanings are as follows:

- `arr` array object.
- `free` is a function pointer used to release resources of array elements.
- `size` is the size in bytes of a single array element.
- `nalloc` is the initial array length, and subsequent array expansion is expanded by twice the number of elements allocated in the current array.

Return value: Returns `0` on success, otherwise returns `-1`



#### mln_array_pool_init

```c
int mln_array_pool_init(mln_array_t *arr, array_free free, mln_size_t size, mln_size_t nalloc,
                        void *pool, array_pool_alloc_handler pool_alloc, array_pool_free_handler pool_free);

typedef void *(*array_pool_alloc_handler)(void *, mln_size_t);
typedef void (*array_pool_free_handler)(void *);
typedef void (*array_free)(void *);
```

Description: Initialize a given array `arr` using memory pool memory. The parameter meanings are as follows:

- `arr` array object.
- `free` is a function pointer used to release resources of array elements.
- `size` is the size in bytes of a single array element.
- `nalloc` is the initial array length, and subsequent array expansion is expanded by twice the number of elements allocated in the current array.
- `pool` is a custom memory pool structure pointer.
- `pool_alloc` is a custom memory pool allocation function pointer.
- `pool_free` is a custom memory pool release function pointer.

Return value: Returns `0` on success, otherwise returns `-1`



#### mln_array_new

```c
mln_array_t *mln_array_new(array_free free, mln_size_t size, mln_size_t nalloc);
```

Description: Create and initialize an array based on the given parameters.

Return value: Returns array pointer if successful, otherwise returns `NULL`



#### mln_array_pool_new

```c
mln_array_t *mln_array_pool_new(array_free free, mln_size_t size, mln_size_t nalloc, void *pool,
                                array_pool_alloc_handler pool_alloc, array_pool_free_handler pool_free);
```

Description: Creates and initializes an array using memory pool memory according to the given parameters.

Return value: Returns array pointer if successful, otherwise returns `NULL`



#### mln_array_destroy

```c
void mln_array_destroy(mln_array_t *arr);
```

Description: Free the memory of all elements of the specified array `arr`.

Return value: None



#### mln_array_free

```c
void mln_array_free(mln_array_t *arr);
```

Description: Release the memory of all elements of the specified array `arr`, and release the memory of the array itself.

Return value: None



#### mln_array_reset

```c
void mln_array_reset(mln_array_t *arr);
```

Description: Release the elements in `arr`, and set the length of the array to 0, but do not release the array structure.

Return value: None



#### mln_array_push

```c
void *mln_array_push(mln_array_t *arr);
```

Description: Append an element to the array and return the memory address of the element.

Return value: return element address if successful, otherwise return `NULL`



#### mln_array_pushn

```c
void *mln_array_pushn(mln_array_t *arr, mln_size_t n);
```

Description: Append `n` elements to the array, and return the first memory address of these elements.

Return value: return element address if successful, otherwise return `NULL`



#### mln_array_pop

```c
void mln_array_pop(mln_array_t *arr);
```

Description: Remove and release the last element of the array. If the array is empty or `arr` is `NULL`, this is a no-op.

Return value: None



#### mln_array_grow

```c
int mln_array_grow(mln_array_t *arr, mln_size_t n);
```

Description: Ensure the array has enough capacity for at least `n` additional elements beyond the current count. The array doubles its allocated size until the required capacity is met. For non-pool arrays, `realloc` is used for efficient in-place growth.

Return value: Returns `0` on success, otherwise returns `-1`



#### mln_array_elts

```c
mln_array_elts(arr);
```

Description: Get the starting address of all elements of the array.

Return value: element starting address



#### mln_array_nelts

```c
mln_array_nelts(arr);
```

Description: Get the number of elements in the array.

Return value: number of elements



#### MLN_ARRAY_PUSH

```c
MLN_ARRAY_PUSH(arr, ret_ptr);
```

Description: Inline macro version of `mln_array_push`. Pushes one element onto the array and assigns its address to `ret_ptr`. On the fast path (no reallocation needed) this avoids function call overhead entirely. If allocation fails, `ret_ptr` is set to `NULL`.

- `arr` pointer to `mln_array_t`.
- `ret_ptr` a `void *` (or typed pointer) lvalue that receives the element address.

Return value: None (result is stored in `ret_ptr`)



#### MLN_ARRAY_PUSHN

```c
MLN_ARRAY_PUSHN(arr, n, ret_ptr);
```

Description: Inline macro version of `mln_array_pushn`. Pushes `n` elements onto the array and assigns the first element's address to `ret_ptr`. If allocation fails, `ret_ptr` is set to `NULL`.

- `arr` pointer to `mln_array_t`.
- `n` number of elements to push.
- `ret_ptr` a `void *` (or typed pointer) lvalue that receives the starting address.

Return value: None (result is stored in `ret_ptr`)



#### MLN_ARRAY_POP

```c
MLN_ARRAY_POP(arr);
```

Description: Inline macro version of `mln_array_pop`. Removes the last element and invokes the free callback if set. If the array is empty or `NULL`, this is a no-op.

- `arr` pointer to `mln_array_t`.

Return value: None



### Example

```c
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "mln_array.h"

static int free_count = 0;

typedef struct {
    int i1;
    int i2;
} test_t;

static void test_free(void *data)
{
    (void)data;
    ++free_count;
}

static void *pool_alloc(void *pool, mln_size_t size)
{
    (void)pool;
    return malloc(size);
}

static void pool_dealloc(void *ptr)
{
    free(ptr);
}

int main(void)
{
    test_t *t;
    mln_size_t i;
    mln_array_t arr;

    /* --- init / push / pushn / elts / nelts / destroy --- */
    assert(mln_array_init(&arr, NULL, sizeof(test_t), 1) == 0);

    t = (test_t *)mln_array_push(&arr);
    assert(t != NULL);
    t->i1 = 0;

    t = (test_t *)mln_array_pushn(&arr, 9);
    assert(t != NULL);
    for (i = 0; i < 9; ++i)
        t[i].i1 = i + 1;

    for (t = (test_t *)mln_array_elts(&arr), i = 0; i < mln_array_nelts(&arr); ++i)
        printf("%d\n", t[i].i1);

    mln_array_destroy(&arr);

    /* --- new / free with free callback --- */
    free_count = 0;
    mln_array_t *a = mln_array_new(test_free, sizeof(test_t), 4);
    assert(a != NULL);
    for (i = 0; i < 3; ++i) {
        t = (test_t *)mln_array_push(a);
        assert(t != NULL);
        t->i1 = (int)i;
    }
    mln_array_free(a);
    assert(free_count == 3);

    /* --- pop --- */
    free_count = 0;
    assert(mln_array_init(&arr, test_free, sizeof(test_t), 4) == 0);
    t = (test_t *)mln_array_push(&arr);
    t->i1 = 1;
    t = (test_t *)mln_array_push(&arr);
    t->i1 = 2;
    mln_array_pop(&arr);
    assert(mln_array_nelts(&arr) == 1 && free_count == 1);
    mln_array_destroy(&arr);

    /* --- reset --- */
    free_count = 0;
    assert(mln_array_init(&arr, test_free, sizeof(test_t), 4) == 0);
    for (i = 0; i < 5; ++i) {
        t = (test_t *)mln_array_push(&arr);
        t->i1 = (int)i;
    }
    mln_array_reset(&arr);
    assert(mln_array_nelts(&arr) == 0 && free_count == 5);
    mln_array_destroy(&arr);

    /* --- grow --- */
    assert(mln_array_init(&arr, NULL, sizeof(test_t), 0) == 0);
    assert(mln_array_grow(&arr, 8) == 0);
    t = (test_t *)mln_array_push(&arr);
    assert(t != NULL);
    mln_array_destroy(&arr);

    /* --- inline macros: MLN_ARRAY_PUSH / MLN_ARRAY_PUSHN / MLN_ARRAY_POP --- */
    assert(mln_array_init(&arr, NULL, sizeof(test_t), 2) == 0);
    for (i = 0; i < 10; ++i) {
        MLN_ARRAY_PUSH(&arr, t);
        assert(t != NULL);
        t->i1 = (int)i;
    }
    MLN_ARRAY_PUSHN(&arr, 5, t);
    assert(t != NULL);
    assert(mln_array_nelts(&arr) == 15);
    MLN_ARRAY_POP(&arr);
    assert(mln_array_nelts(&arr) == 14);
    mln_array_destroy(&arr);

    /* --- pool_init / pool_new --- */
    int dummy_pool = 0;
    assert(mln_array_pool_init(&arr, NULL, sizeof(test_t), 4,
                               &dummy_pool, pool_alloc, pool_dealloc) == 0);
    t = (test_t *)mln_array_push(&arr);
    assert(t != NULL);
    t->i1 = 77;
    mln_array_destroy(&arr);

    mln_array_t *pa = mln_array_pool_new(NULL, sizeof(test_t), 4,
                                         &dummy_pool, pool_alloc, pool_dealloc);
    assert(pa != NULL);
    t = (test_t *)mln_array_push(pa);
    assert(t != NULL);
    mln_array_free(pa);

    printf("ALL PASSED\n");
    return 0;
}
```

