## Memory Pool



In Melon, memory pools are divided into two categories:

- heap memory
- Shared memory

Among them, the shared memory memory pool only allows data to be shared between the main and child processes (also shared between sibling processes). That is, when used, the shared memory memory pool is created by the main process, and then the child process is created.



### Header file

```c
#include "mln_alloc.h"
```



### Module

`alloc`



### Functions



#### mln_alloc_init

```c
mln_alloc_t *mln_alloc_init(mln_alloc_t *parent);
```

Description: Create a heap memory memory pool. The parameter `parent` is a memory pool instance. When the parameter is `NULL`, the memory pool created by this function will allocate memory from the heap. If it is not `NULL`, it will allocate memory from the pool where `parent` is located. That is, the pool structure can be cascaded.

Return value: If successful, return the memory pool structure pointer, otherwise return `NULL`



#### mln_alloc_shm_init
    
```c
mln_alloc_t *mln_alloc_shm_init(mln_size_t size, void *locker, mln_alloc_shm_lock_cb_t lock, mln_alloc_shm_lock_cb_t unlock);                  
``` 
    
Description: Create a shared memory memory pool.
                                                                                                                                               
Parameters:
                                                                                                                                               
- `size` The pool size `size` (in bytes) needs to be given when creating this pool. Once created, it cannot be expanded later.
- `locker` is the lock resource pointer.
- `lock` is a callback function used to lock the lock resource. The function parameter is the lock resource pointer. If the lock fails, a `non-0` value is returned.
- `unlock` is a callback function used to unlock the lock resource. The function parameter is the lock resource pointer. If unlocking fails, a `non-0` value is returned.
                                                                                                                                               
```c
typedef int (*mln_alloc_shm_lock_cb_t)(void *);
```
                                                                                                                                               
`lock` and `unlock` are only used when functions in the subpool are called. Therefore, if you directly operate on the shared memory pool, you need to unlock it externally. The allocation and release functions that act on the shared memory pool will not call this callback.
    
Return value: Returns the memory pool structure pointer if successful, otherwise returns `NULL`



#### mln_alloc_destroy

```c
void mln_alloc_destroy(mln_alloc_t *pool);
```

Description: Destroy the memory pool. The destroy operation will release all the memory managed in the memory pool uniformly.

Return value: none



#### mln_alloc_m

```c
void *mln_alloc_m(mln_alloc_t *pool, mln_size_t size);
```

Description: Allocate a memory of size `size` from the memory pool `pool`. If the memory pool is a shared memory memory pool, it will be allocated from shared memory, otherwise it will be allocated from heap memory.

Return value: If successful, return the memory start address, otherwise return `NULL`



#### mln_alloc_c

```c
void *mln_alloc_c(mln_alloc_t *pool, mln_size_t size);
```

Description: Allocate a memory of size `size` from the memory pool `pool`, and the memory will be zeroed.

Return value: If successful, return the memory start address, otherwise return `NULL`



#### mln_alloc_re

```c
void *mln_alloc_re(mln_alloc_t *pool, void *ptr, mln_size_t size);
```

Description: Allocate a size of `size` from the memory pool `pool`, and copy the data in the memory pointed to by `ptr` to the new memory.

`ptr` must be the starting address of memory allocated for the memory pool. If `size` is `0`, the memory pointed to by `ptr` will be freed.

Return value: If successful, return the memory start address, otherwise return `NULL`



#### mln_alloc_free

```c
void mln_alloc_free(void *ptr);
```

Description: Free the memory pointed to by `ptr`. **Note**: `ptr` must be the address returned by the allocation function, not a location in the allocated memory.

Return value: none



### Example

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mln_alloc.h"

int main(int argc, char *argv[])
{
    char *p;
    mln_alloc_t *pool;

    pool = mln_alloc_init(NULL);
    if (pool == NULL) {
        fprintf(stderr, "pool init failed\n");
        return -1;
    }

    p = (char *)mln_alloc_m(pool, 6);
    if (p == NULL) {
        fprintf(stderr, "alloc failed\n");
        return -1;
    }

    memcpy(p, "hello", 5);
    p[5] = 0;
    printf("%s\n", p);

    mln_alloc_free(p);
    mln_alloc_destroy(pool);

    return 0;
}
```

