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
void *mln_array_pop(mln_array_t *arr);
```

Description: Remove and release the last element of the array.

Return value: None



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

    mln_array_init(&arr, NULL, sizeof(test_t), 1);

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

