## Array



### Header File

```c
#include "mln_array.h"
```



### Functions/Macros

#### mln_array_init

```c
int mln_array_init(mln_array_t *arr, struct mln_array_attr *attr);
```

Description: For a given array `arr`, initialize it according to the initialization attribute `attr`. The `attr` structure is as follows:

```c
struct mln_array_attr {
    void                     *pool;
    array_pool_alloc_handler  pool_alloc;
    array_pool_free_handler   pool_free;
    mln_size_t                size;
    mln_size_t                nalloc;
};
typedef void *(*array_pool_alloc_handler)(void *, mln_size_t);
typedef void (*array_pool_free_handler)(void *);
```

- `pool` is a custom memory pool structure pointer
- `pool_alloc` is a custom memory pool allocation function pointer
- `pool_free` is a custom memory pool release function pointer
- `size` is the size in bytes of a single array element
- `nalloc` is the initial array length, and the subsequent array expansion is to expand twice the number of elements allocated in the current array

Return value: returns `0` on success, otherwise returns `-1`



#### mln_array_new

```c
mln_array_t *mln_array_new(struct mln_array_attr *attr);
```

Description: Create and initialize an array according to the given initialization attribute `attr`.

Return value: return array pointer if successful, otherwise return `NULL`



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



### Example

```c
#include <stdio.h>
#include "mln_array.h"

typedef struct {
    int i1;
    int i2;
} test_t;

int main(void)
{
    test_t *t;
    mln_size_t i, n;
    mln_array_t arr;
    struct mln_array_attr attr;

    attr.pool = NULL;
    attr.pool_alloc = NULL;
    attr.pool_free = NULL;
    attr.size = sizeof(test_t);
    attr.nalloc = 1;
    mln_array_init(&arr, &attr);

    t = mln_array_push(&arr);
    if (t == NULL)
        return -1;
    t->i1 = 0;

    t = mln_array_pushn(&arr, 9);
    for (i = 0; i < 9; ++i) {
        t[i].i1 = i + 1;
    }

    for (t = mln_array_elts(&arr), i = 0; i < mln_array_nelts(&arr); ++i) {
        printf("%d\n", t[i].i1);
    }

    mln_array_destroy(&arr);

    return 0;
}
```

