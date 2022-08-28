## Memory Pool



In Melon, memory pools are divided into two categories:

- heap memory
- Shared memory

Among them, the shared memory memory pool only allows data to be shared between the main and child processes (also shared between sibling processes). That is, when used, the shared memory memory pool is created by the main process, and then the child process is created.



### Header file

```c
#include "mln_alloc.h"
```



### Functions



#### mln_alloc_init

```c
mln_alloc_t *mln_alloc_init(mln_alloc_t *parent);
```

Description: Create a heap memory memory pool. The parameter `parent` is a memory pool instance. When the parameter is `NULL`, the memory pool created by this function will allocate memory from the heap. If it is not `NULL`, it will allocate memory from the pool where `parent` is located. That is, the pool structure can be cascaded.

Return value: If successful, return the memory pool structure pointer, otherwise return `NULL`



#### mln_alloc_shm_init

```c
mln_alloc_t *mln_alloc_shm_init(struct mln_alloc_shm_attr_s *attr);
```

Description: Create a shared memory memory pool.

Parameter structure definition:

```c
struct mln_alloc_shm_attr_s {
    mln_size_t                size;
    void                     *locker;
    mln_alloc_shm_lock_cb_t   lock;
    mln_alloc_shm_lock_cb_t   unlock;
};
typedef int (*mln_alloc_shm_lock_cb_t)(void *);
```

The pool size `size` (in bytes) needs to be given when the pool is created. Once created, it cannot be expanded later.

`locker` is a pointer to a lock resource structure.

`lock` is a callback function used to lock the lock resource. The function parameter is the lock resource pointer. Returns a `non-0` value if the lock fails.

`unlock` is the callback function used to unlock the lock resource, the function parameter is the lock resource pointer. Returns a `non-0` value if the unlock fails.

`lock` and `unlock` are only used for function calls in subpools. Therefore, if you directly operate on the shared memory pool, you need to add and unlock it externally, and the allocation and release functions acting on the shared memory pool will not call this callback.

Return value: If successful, return the memory pool structure pointer, otherwise return `NULL`



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
#include "mln_core.h"
#include "mln_log.h"
#include "mln_alloc.h"

int main(int argc, char *argv[])
{
    char *p;
    mln_alloc_t *pool;
    struct mln_core_attr cattr;

    cattr.argc = argc;
    cattr.argv = argv;
    cattr.global_init = NULL;
    cattr.master_process = NULL;
    cattr.worker_process = NULL;
    if (mln_core_init(&cattr) < 0) {
        fprintf(stderr, "init failed\n");
        return -1;
    }

    pool = mln_alloc_init(NULL);
    if (pool == NULL) {
        mln_log(error, "pool init failed\n");
        return -1;
    }

    p = (char *)mln_alloc_m(pool, 6);
    if (p == NULL) {
        mln_log(error, "alloc failed\n");
        return -1;
    }

    memcpy(p, "hello", 5);
    p[5] = 0;
    mln_log(debug, "%s\n", p);

    mln_alloc_free(p);

    return 0;
}
```

