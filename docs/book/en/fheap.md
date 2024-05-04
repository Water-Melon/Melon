## Fibonacci Heap

What is implemented in Melon is **minimum heap**.

Similar to the red-black tree component, the Fibonacci heap component has three usages:

- Basic Usage
- Inline Usage (not supported in `MSVC`)
- Container Usage



### Header File

```c
#include "mln_fheap.h"
```



### Module

`fheap`



### Data Structures

The Fibonacci heap and its heap node structure are used internally by the component. Developers don't need to care about the composition of these two data structures. Just use the functions/macros provided in this chapter to operate the heap and nodes.



## Basic Usage



### Functions/Macros

#### mln_fheap_new

```c
mln_fheap_t *mln_fheap_new(void *min_val, struct mln_fheap_attr *attr);

struct mln_fheap_attr {
    void                     *pool;//memory pool
    fheap_pool_alloc_handler  pool_alloc;//Memory pool allocation memory function pointer
    fheap_pool_free_handler   pool_free;//Memory pool release memory function pointer
    fheap_cmp                 cmp;//comparison function
    fheap_copy                copy;//copy function
    fheap_key_free            key_free;//key release function
};
typedef int (*fheap_cmp)(const void *, const void *);
typedef void (*fheap_copy)(void *, void *);
typedef void (*fheap_key_free)(void *);
typedef void *(*fheap_pool_alloc_handler)(void *, mln_size_t);
typedef void (*fheap_pool_free_handler)(void *);
```

Description: Create a Fibonacci heap.

- `min_val` refers to the minimum value of the user-defined data type in the heap node, and the type of this pointer is consistent with the type of user-defined data in the heap node. Note that the memory pointed to by this pointer should not be changed, and its life cycle should at least run through the life cycle of the Fibonacci heap.

- `attr` is the initialization attribute structure of the Fibonacci heap, which contains:

   - `pool` is a user-defined memory pool structure. If this item is not `NULL`, the Fibonacci heap structure will be allocated from this memory pool, otherwise it will be allocated from the malloc library.
   - `pool_alloc` is the allocation memory function pointer of the memory pool, and this callback will be used when `pool` is not empty.
   - `pool_free` is the pointer to the free memory function of the memory pool, and this callback will be used when `pool` is not empty.
   - `cmp` is a function used to compare key. This function has two parameters, both of which are user-defined key structure pointers. Return `0` if argument 1 is less than argument 2, otherwise return `not 0`.
   - `copy` is used to copy the key structure, this callback function will be called when `decrease key` is used to change the original key value to a new key value. This function has two parameters, respectively: the original key value pointer and the new key value pointer.
   - `key_free` is the release function of the key value structure, if it does not need to be released, you can set `NULL`.

   Here is an additional note, if you use the inline mode and do not use the `pool` field, you can directly set the attr parameter of `mln_fheap_new` to `NULL`.

Return value: return `mln_fheap_t` type pointer if successful, otherwise return `NULL`



#### mln_fheap_new_fast

```c
mln_fheap_t *mln_fheap_new_fast(void *min_val, fheap_cmp cmp, fheap_copy copy, fheap_key_free key_free);

typedef int (*fheap_cmp)(const void *, const void *);
typedef void (*fheap_copy)(void *, void *);
typedef void (*fheap_key_free)(void *);
```

Description: Create a Fibonacci heap. The difference from `mln_fheap_new_fast` is that memory pool is not supported and attributes are passed as function parameters.

- `min_val` refers to the minimum value of the user-defined data type in the heap node. The type of this pointer is consistent with the type of user-defined data in the heap node. Note that the memory pointed to by this pointer should not be changed, and its life cycle should last at least throughout the life cycle of the Fibonacci heap.
- `cmp` is a function used for key comparison. This function has two parameters, both of which are user-defined key structure pointers. If parameter 1 is less than parameter 2, return `0`, otherwise return `non-0`.
- `copy` is used to copy the key structure. This callback function will be called when `decrease key` is used to change the original key value to the new key value. This function has two parameters, which are: the original key value pointer and the new key value pointer.
- `key_free` is the key value structure release function. If it does not need to be released, it can be set to `NULL`.

Return value: Returns `mln_fheap_t` type pointer if successful, otherwise returns `NULL`



#### mln_fheap_free

```c
void mln_fheap_free(mln_fheap_t *fh);
```

Description: Destroy the Fibonacci heap, and release the key in the heap according to `key_free`.

Return value: None



#### mln_fheap_node_new

```c
mln_fheap_node_t *mln_fheap_node_new(mln_fheap_t *fh, void *key);
```

Description: Create a Fibonacci heap node structure, `key` is the user-defined structure, `fh` is the heap created by `mln_fheap_new`.

Return value: return node structure pointer if successful, otherwise return `NULL`



#### mln_fheap_node_free

```c
void mln_fheap_node_free(mln_fheap_t *fh, mln_fheap_node_t *fn);
```

Description: Release the resources of the node `fn` of the Fibonacci heap `fh`, and release the node data according to `key_free`.

Return value: None



#### mln_fheap_insert

```c
void mln_fheap_insert(mln_fheap_t *fh, mln_fheap_node_t *fn);
```

Description: Insert a heap node into the heap.

Return value: None



#### mln_fheap_minimum

```c
mln_fheap_node_t *mln_fheap_minimum(mln_fheap_t *fh);
```

Description: Get the heap node with the smallest current key value in the heap `fh`.

Return value: Returns the node structure successfully, otherwise returns `NULL`



#### mln_fheap_extract_min

```c
mln_fheap_node_t *mln_fheap_extract_min(mln_fheap_t *fh);
```

Description: Take the node with the smallest key value in the heap `fh` from the heap and return it.

Return value: return node structure if successful, otherwise return `NULL`



#### mln_fheap_decrease_key

```c
int mln_fheap_decrease_key(mln_fheap_t *fh, mln_fheap_node_t *node, void *key);
```

Description: Downgrade the key value of the node `node` in the heap `fh` to the value given by `key`.

**Note**: If `key` is greater than the original key value, the execution will fail and return.

Return value: returns `0` on success, otherwise returns `-1`



#### mln_fheap_delete

```c
void mln_fheap_delete(mln_fheap_t *fh, mln_fheap_node_t *node);
```

Description: Delete the node `node` from the heap `fh`, but it won't release node structure and associated user data.

Return value: None



### Example

```c
#include <stdio.h>
#include <stdlib.h>
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

    fattr.pool = NULL;
    fattr.pool_alloc = NULL;
    fattr.pool_free = NULL;
    fattr.cmp = cmp_handler;
    fattr.copy = copy_handler;
    fattr.key_free = NULL;

    fh = mln_fheap_new(&min, &fattr);
    if (fh == NULL) {
        fprintf(stderr, "create fheap failed.\n");
        return -1;
    }

    fn = mln_fheap_node_new(fh, &i);
    if (fn == NULL) {
        fprintf(stderr, "create fheap node failed.\n");
        return -1;
    }
    mln_fheap_insert(fh, fn);

    fn = mln_fheap_minimum(fh);
    printf("%d\n", *((int *)mln_fheap_node_key(fn)));

    mln_fheap_free(fh);

    return 0;
}
```



## Inline Usage

The purpose of inline usage is to improve the execution efficiency of each operation of the Fibonacci heap. The core difference between it and the basic usage is that the existence of callback functions (for example, `cmp`, `key_free`, `copy`) is avoided. That is, the callback function is turned into an inline function, and the function call is eliminated by using compiler optimization.

Although this method of use is efficient, it also has some usage scenario limitations.

> Scenario assumptions:
>
> We first create a Fibonacci heap, which may be inserted into a lot of nodes in the future. And each node is a Fibonacci heap (we temporarily call it a sub-heap). The related operations of this sub-heap are independently maintained by its associated code files, and are not related to other sub-heaps.
>
> Our requirement is: when the Fibonacci heap (non-sub-heap) executes `mln_fheap_free`, all sub-heaps will be released and destroyed together.



In the case of basic usage, we only need to set the `key_free` attribute of the Fibonacci heap to `mln_fheap_free`, and each sub-heap can be automatically released, regardless of the data type associated with each sub-heap node. What, and how to free these data types. Because these data types will be correctly released by the callback function set by the `key_free` attribute of the sub-heap.

However, in inline usage, the comparison and release operations of node data need to be clearly specified which function is to be handled. Combined with our scenario assumptions above, it means that the Fibonacci heap must know the data type of each sub-heap, and then explicitly call the release function corresponding to the specified type to release resources. However, when resources are continuously nested between multiple data structures and code modules as in the above scenario, it is difficult for us to know the data types of other modules in a certain code module, and it is impossible to release these data type resources correctly. This is the scenario where inline usage doesn't quite work out.

The inline usage additionally provides a set of macro statement expressions to replace some functions in the basic usage:

- mln_fheap_inline_insert
- mln_fheap_inline_extract_min
- mln_fheap_inline_decrease_key
- mln_fheap_inline_delete
- mln_fheap_inline_node_free
- mln_fheap_inline_free



### Functions/Macros

#### mln_fheap_inline_insert

```c
mln_fheap_inline_insert(fh, fn, compare)
```

Description: Same as the function of `mln_fheap_insert`, insert the node `fn` into the Fibonacci heap `fh`. `compare` is the `cmp` function in `struct mln_fheap_attr` in the basic usage, but at this time the function can be declared as `inline`, and it will be inlined after compiler optimization is turned on. If `compare` is `NULL`, it will try to get the `cmp` callback function of the Fibonacci heap `fh`.

Return value: same as `mln_fheap_insert`



#### mln_fheap_inline_extract_min

```c
mln_fheap_inline_extract_min(fh, compare)
```

Description: Same as the function of `mln_fheap_extract_min`, the heap node with the smallest value is removed from the heap and returned. `compare` is the `cmp` function in `struct mln_fheap_attr` in the basic usage, but at this time the function can be declared as `inline`, and it will be inlined after compiler optimization is turned on. If `compare` is `NULL`, it will try to get the `cmp` callback function of the Fibonacci heap `fh`.

Return value: same as `mln_fheap_extract_min`



#### mln_fheap_inline_decrease_key

```c
mln_fheap_inline_decrease_key(fh, node, k, cpy, compare)
```

Description: Same as the function of `mln_fheap_decrease_key`, reduce the key value of the node `node` to `k`. `compare` is the `cmp` function in `struct mln_fheap_attr` in the basic usage, `cpy` is the `copy` function in `struct mln_fheap_attr`, but at this time these two functions can be declared as `inline`, and in Inlined after compiler optimization is turned on. If `compare` is `NULL`, it will try to get the `cmp` callback function of the Fibonacci heap `fh`. If `cpy` is `NULL`, it will try to get the `copy` callback function of the Fibonacci heap `fh`.

Return value: same as `mln_fheap_decrease_key`



#### mln_fheap_inline_delete

```c
mln_fheap_inline_delete(fh, node, cpy, compare)
```

Description: Same as the function of `mln_fheap_delete`, remove the node `node` from the heap `fh`. `compare` is the `cmp` function in `struct mln_fheap_attr` in the basic usage, `cpy` is the `copy` function in `struct mln_fheap_attr`, but at this time these two functions can be declared as `inline`, and in Inlined after compiler optimization is turned on. If `compare` is `NULL`, it will try to get the `cmp` callback function of the Fibonacci heap `fh`. If `cpy` is `NULL`, it will try to get the `copy` callback function of the Fibonacci heap `fh`.

Return value: same as `mln_fheap_delete`



#### mln_fheap_inline_node_free

```c
mln_fheap_inline_node_free(fh, fn, freer)
```

Description: Same as the function of `mln_fheap_node_free`, release the node `fn`. `freer` is the `key_free` function in `struct mln_fheap_attr` in the basic usage, but this function can be declared as `inline` at this time, and it will be inlined after compiler optimization is turned on. If `freer` is `NULL`, it will try to get the `key_free` callback function of the Fibonacci heap `fh`.

Return value: same as `mln_fheap_node_free`



#### mln_fheap_inline_free

```c
mln_fheap_inline_free(fh, compare, freer)
```

Description: Same as the function of `mln_fheap_free`, release the node `fn`. `freer` is the `key_free` function in `struct mln_fheap_attr` in the basic usage, but this function can be declared as `inline` at this time, and it will be inlined after compiler optimization is turned on. If `freer` is `NULL`, it will try to get the `key_free` callback function of the Fibonacci heap `fh`.

Return value: same as `mln_fheap_free`



### Example

```c
#include <stdio.h>
#include <stdlib.h>
#include "mln_fheap.h"

static inline int cmp_handler(const void *key1, const void *key2) //inline
{
    return *(int *)key1 < *(int *)key2? 0: 1;
}

static inline void copy_handler(void *old_key, void *new_key) //inline
{
    *(int *)old_key = *(int *)new_key;
}

int main(int argc, char *argv[])
{
    int i = 10, min = 0;
    mln_fheap_t *fh;
    mln_fheap_node_t *fn;

    fh = mln_fheap_new(&min, NULL);
    if (fh == NULL) {
        fprintf(stderr, "fheap init failed.\n");
        return -1;
    }

    fn = mln_fheap_node_new(fh, &i);
    if (fn == NULL) {
        fprintf(stderr, "fheap node init failed.\n");
        return -1;
    }
    mln_fheap_inline_insert(fh, fn, cmp_handler); //inline insert

    fn = mln_fheap_minimum(fh);
    printf("%d\n", *((int *)mln_fheap_node_key(fn)));

    mln_fheap_inline_free(fh, cmp_handler, NULL); //inline free

    return 0;
}
```





## Container Usage

The container usage does not conflict with the above two usages, on the contrary the container usage needs to use the functions/macros of the above two usages to operate the Fibonacci heap. The meaning of the container is to use the Fibonacci heap node structure as an attribute in the user-defined data structure. For example:

```c
struct user_defined_s {
    int int_val;
    ...
    mln_fheap_node_t node; //Note that this is not a pointer
    ...
};
```

In this structure definition, `node` is a Fibonacci heap node type, which is a member variable in the structure `user_defined_s`.



### Functions/Macros

In container usage, we use `mln_fheap_node_init` to replace `mln_fheap_node_new` to initialize the heap node. The difference between these two interfaces is that `mln_fheap_node_init` does not dynamically create the node structure (because it has been defined in other custom structures and is created together when the custom structure is instantiated).



#### mln_fheap_node_init

```c
mln_fheap_node_init(fn, k)
```

Description: Initialize the heap node pointer `fn`, and associate the user-defined key `k` with the node structure `fn`.

Return value: heap node structure pointer `fn`



### Example

```c
#include <stdio.h>
#include <stdlib.h>
#include "mln_fheap.h"

typedef struct user_defined_s {
    int val;
    mln_fheap_node_t node; //自定义数据结构的成员
} ud_t;

static inline int cmp_handler(const void *key1, const void *key2)
{
    return ((ud_t *)key1)->val < ((ud_t *)key2)->val? 0: 1;
}

static inline void copy_handler(void *old_key, void *new_key)
{
    ((ud_t *)old_key)->val = ((ud_t *)new_key)->val;
}

int main(int argc, char *argv[])
{
    mln_fheap_t *fh;
    ud_t min = {0, };
    ud_t data1 = {1, };
    ud_t data2 = {2, };
    mln_fheap_node_t *fn;

    fh = mln_fheap_new(&min, NULL);
    if (fh == NULL) {
        fprintf(stderr, "fheap init failed.\n");
        return -1;
    }

    mln_fheap_node_init(&data1.node, &data1); //初始化堆结点
    mln_fheap_node_init(&data2.node, &data2);
    mln_fheap_inline_insert(fh, &data1.node, cmp_handler); //插入堆结点
    mln_fheap_inline_insert(fh, &data2.node, cmp_handler);

    fn = mln_fheap_minimum(fh);
    //两种方式获取自定义数据
    printf("%d\n", ((ud_t *)mln_fheap_node_key(fn))->val);
    printf("%d\n", mln_container_of(fn, ud_t, node)->val);

    mln_fheap_inline_free(fh, cmp_handler, NULL);

    return 0;
}
```
