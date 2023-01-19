## Fibonacci Heap

The implementation here is **min heap**.



### Header file

```c
#include "mln_fheap.h"
```



### Structure

```c
typedef struct mln_fheap_node_s {
    void                    *key; //User data stored in Fibonacci heap nodes
    ...
} mln_fheap_node_t;
```



### Functions



#### mln_fheap_new

```c
mln_fheap_t *mln_fheap_new(struct mln_fheap_attr *attr);

struct mln_fheap_attr {
    void                     *pool;//memory pool
    fheap_pool_alloc_handler  pool_alloc;//allocation callback of memory pool
    fheap_pool_free_handler   pool_free;//free callback of memory pool
    fheap_cmp                 cmp;//comparison function
    fheap_copy                copy;//copy function
    fheap_key_free            key_free;//memory free function of key
    void                     *min_val;//Minimum value
    mln_size_t                min_val_size;//bytes of Minimum value
};
typedef int (*fheap_cmp)(const void *, const void *);
typedef void (*fheap_copy)(void *, void *);
typedef void (*fheap_key_free)(void *);
typedef void *(*fheap_pool_alloc_handler)(void *, mln_size_t);
typedef void (*fheap_pool_free_handler)(void *);
```

Description: Create a Fibonacci heap.

- `pool` is a user-defined memory pool structure, if the item is not `NULL`, the Fibonacci heap structure will be allocated from this memory pool, otherwise it will be allocated from the malloc library.
- `pool_alloc` is the pointer to the allocation memory function of the memory pool.
- `pool_free` is the free memory function pointer of the memory pool.
- `cmp` is a function for comparing the size of keys. This function has two parameters, both of which are user-defined pointers to key structures. Returns `0` if parameter 1 is less than parameter 2, otherwise returns `not 0`.
- `copy` is used to copy the key structure. This callback function will be called when `derease key` is used to change the original key value to the new key value. This function has two parameters, in turn: the original key value pointer and the new key value pointer.
- `key_free` is the key value structure release function, if you don't need to release it, you can set `NULL`.
- `min_val` defines the minimum key value and is a user-defined structure pointer. In principle, when this value is encountered in the `cmp` function, it should be determined that the value is the smaller one.
- `min_val_size` defines the type size of `min_val`, in bytes.

Return value: return `mln_fheap_t` type pointer if successful, otherwise return `NULL`



#### mln_fheap_free

```c
void mln_fheap_free(mln_fheap_t *fh);
```

Description: Destroy the Fibonacci heap and release the keys in the heap according to `key_free`.

Return value: none



#### mln_fheap_node_new

```c
mln_fheap_node_t *mln_fheap_node_new(mln_fheap_t *fh, void *key);
```

Description: Create a Fibonacci heap node structure, `key` is a user-defined structure, `fh` is the heap created by `mln_fheap_new`.

Return value: Returns the node structure pointer if successful, otherwise returns `NULL`



#### mln_fheap_node_free

```c
void mln_fheap_node_free(mln_fheap_t *fh, mln_fheap_node_t *fn);
```

Description: Release the resources of the node `fn` of the Fibonacci heap `fh`, and release the node data according to `key_free`.

Return value: none



#### mln_fheap_insert

```c
void mln_fheap_insert(mln_fheap_t *fh, mln_fheap_node_t *fn);
```

Description: Insert a heap node into the heap.

Return value: none



#### mln_fheap_minimum

```c
mln_fheap_node_t *mln_fheap_minimum(mln_fheap_t *fh);
```

Description: Get the heap node with the smallest key value in the heap `fh`.

Return value: Returns the node structure successfully, otherwise returns `NULL`



#### mln_fheap_extract_min

```c
mln_fheap_node_t *mln_fheap_extract_min(mln_fheap_t *fh);
```

Description: Remove the node with the smallest key value in the heap `fh` from the heap and return it.

Return value: Returns the node structure if successful, otherwise returns `NULL`



#### mln_fheap_decrease_key

```c
int mln_fheap_decrease_key(mln_fheap_t *fh, mln_fheap_node_t *node, void *key);
```

Description: Reduce the key value of node `node` in heap `fh` to the value given by `key`.

**Note**: If `key` is greater than the original key value, it will fail and return.

Return value: return `0` on success, otherwise return `-1`



#### mln_fheap_delete

```c
void mln_fheap_delete(mln_fheap_t *fh, mln_fheap_node_t *node);
```

Description: Remove the node `node` from the heap `fh`, and release the resources in the node according to the setting of `key_free`.

Return value: none



### Example

```c
#include <stdio.h>
#include <stdlib.h>
#include "mln_core.h"
#include "mln_log.h"
#include "mln_fheap.h"

static int cmp_handler(const void *key1, const void *key2)
{
    return *(int *)key1 < *(int *)key2? 0: 1;
}

static void copy_handler(void *old_key, void *new_key)
{
    *(int *)old_key = *(int *)new_key;
}

int main(int argc, char *argv[])
{
    int i = 10, min = 0;
    mln_fheap_t *fh;
    mln_fheap_node_t *fn;
    struct mln_fheap_attr fattr;
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

    fattr.pool = NULL;
    fattr.pool_alloc = NULL;
    fattr.pool_free = NULL;
    fattr.cmp = cmp_handler;
    fattr.copy = copy_handler;
    fattr.key_free = NULL;
    fattr.min_val = &min;
    fattr.min_val_size = sizeof(min);
    fh = mln_fheap_new(&fattr);
    if (fh == NULL) {
        mln_log(error, "fheap init failed.\n");
        return -1;
    }

    fn = mln_fheap_node_new(fh, &i);
    if (fn == NULL) {
        mln_log(error, "fheap node init failed.\n");
        return -1;
    }
    mln_fheap_insert(fh, fn);

    fn = mln_fheap_minimum(fh);
    mln_log(debug, "%d\n", *((int *)mln_fheap_node_key(fn)));

    mln_fheap_free(fh);

    return 0;
}
```

