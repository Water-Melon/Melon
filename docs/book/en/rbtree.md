## Red-black tree

This document explains how to use the red-black tree component.

In the red-black tree component, there are two usage modes:

- Basic Usage
- Inline Usage (not supported in `MSVC`)
- Container Usage



### Header File

```c
#include "mln_rbtree.h"
```



### Module

`rbtree`



### Structures

In the red-black tree component, users do not need to learn the tree and node structure, but only need to use the functions or macros provided by this component to obtain the corresponding information.



## Basic Usage

In basic usage, the following functions and macros are included:

- mln_rbtree_new
- mln_rbtree_free
- mln_rbtree_insert
- mln_rbtree_search
- mln_rbtree_delete
- mln_rbtree_successor
- mln_rbtree_min
- mln_rbtree_reset
- mln_rbtree_node_new
- mln_rbtree_node_free
- mln_rbtree_iterate
- mln_rbtree_node_num
- mln_rbtree_null
- mln_rbtree_root
- mln_rbtree_node_data_get
- mln_rbtree_node_data_set



### Functions/Macros



#### mln_rbtree_new

```c
mln_rbtree_t *mln_rbtree_new(struct mln_rbtree_attr *attr);

struct mln_rbtree_attr {
    void                      *pool; //Memory pool structure, NULL if no memory pool is used
    rbtree_pool_alloc_handler  pool_alloc; //memory pool allocation function
    rbtree_pool_free_handler   pool_free; //Memory pool release function
    rbtree_cmp                 cmp;//Red-black tree node comparison function
    rbtree_free_data           data_free;//Release function of user data in red-black tree node
};

typedef int (*rbtree_cmp)(const void *, const void *);
typedef void (*rbtree_free_data)(void *);
```

Description:

Initialize the red-black tree.

`attr` is the initialization attribute parameter, if not provided, it defaults to `NULL` for all attributes.

`pool`, `pool_alloc` and `pool_free` in `attr` are used to support user-defined memory pool implementation. That is, the red-black tree structure and tree node structure can be allocated from the memory pool specified by the user.

The `cmp` in `attr` is a node comparison function, which is almost always called in various operations of the red-black tree. The two parameters of this function are user-defined data structure pointers (specified by the second parameter of the `mln_rbtree_node_new` function), and the return value of the function is:

- `-1` the first argument is less than the second argument
- `0` both arguments are the same
- `1` the first argument is greater than the second argument

`data_free` is used to release user-defined data structures in red-black tree nodes. This callback function will be called in `mln_rbtree_node_free` and `mln_rbtree_free` functions. That is, it is called when the tree structure or tree node structure is released, and the user-defined data in the node is also cleaned up. Set `NULL` if you do not need to release user-defined data.

Return value:  `mln_rbtree_t` pointer on success, otherwise `NULL` returned



#### mln_rbtree_free

```c
void mln_rbtree_free(mln_rbtree_t *t);
```

Description: Destroy the red-black tree and release the user-defined data stored in each node on it.

Return value: None



#### mln_rbtree_insert

```c
void mln_rbtree_insert(mln_rbtree_t *t, mln_rbtree_node_t *n);
```

Description: Insert a red-black tree node into a red-black tree. The second parameter can be created by `mln_rbtree_node_new` function.

Return value: None



#### mln_rbtree_delete

```c
void mln_rbtree_delete(mln_rbtree_t *t, mln_rbtree_node_t *n);
```

Description: Remove the red-black tree node from the red-black tree. **Note**, this operation will **not** release the tree nodes and user-defined data stored in the nodes.

Return value: None



#### mln_rbtree_successor

```c
mln_rbtree_node_t *mln_rbtree_successor(mln_rbtree_t *t, mln_rbtree_node_t *n);
```

Description: Get the successor node of the specified node `n`, that is, the next node larger than the data in the node `n`.

Return value: Red-black tree node pointer, need to use the macro `mln_rbtree_null` to confirm whether there is a successor node



#### mln_rbtree_search

```c
mln_rbtree_node_t *mln_rbtree_search(mln_rbtree_t *t, const void *key);
```

Description: Start from the root node of the red-black tree `t` to find out whether there is a node identical to `key`. **Note**, `key` should belong to the same data type as the user-defined data.

Return value: Red-black tree node pointer, you need to use the macro `mln_rbtree_null` to confirm whether the node is found



#### mln_rbtree_min

```c
mln_rbtree_node_t *mln_rbtree_min(mln_rbtree_t *t);
```

Description: Get the node with the smallest user data in the red-black tree `t`.

Return value: Red-black tree node pointer, need to use macro `mln_rbtree_null` to confirm whether the node exists



#### mln_rbtree_node_new

```c
mln_rbtree_node_t *mln_rbtree_node_new(mln_rbtree_t *t, void *data);
```

Description: Create a red-black tree node. `data` is the user-defined data associated with this node.

Return value:  `mln_rbtree_node_t` pointer on success, otherwise `NULL` returned



#### mln_rbtree_node_free

```c
void mln_rbtree_node_free(mln_rbtree_t *t, mln_rbtree_node_t *n);
```

Description: Release the resources of the red-black tree node `n`, and possibly release its associated user-defined data (determined by the attributes given in `mln_rbtree_new`).

Return value: None



#### mln_rbtree_iterate

```c
int mln_rbtree_iterate(mln_rbtree_t *t, rbtree_iterate_handler handler, void *udata);

typedef int (*rbtree_iterate_handler)(mln_rbtree_node_t *node, void *udata);
```

Description:

Traverse each node in the red-black tree `t`. And it supports deleting tree nodes during traversal.

`handler` is an access function for traversing each node. The meanings of the two parameters of this function are as follows:

- `node` currently visited tree node structure
- `udata` is the third parameter of `mln_rbtree_iterate`, which is a parameter passed in by the user, if not needed, it can be set to `NULL`

The reason why the node node is given additionally is because there may be requirements: to replace the data in the node during traversal (this is not recommended, because it will violate the existing order of the red-black tree nodes), or to replace the tree nodes Set `NULL` for user-defined data to temporarily prevent the data from being released. **Careful** use is required.

return value:

- `mln_rbtree_iterate`: return `0` after all traversal, otherwise return `-1`
- `rbtree_iterate_handler`: returns `-1` if it is expected to interrupt the traversal, otherwise the return value should be `greater than or equal to 0`



#### mln_rbtree_reset

```c
void mln_rbtree_reset(mln_rbtree_t *t);
```

Description: Reset the entire red-black tree `t` and release all nodes in it.

Return value: None



#### mln_rbtree_node_num

```c
mln_rbtree_node_num(ptree)
```

Description: Get the number of nodes in the current tree.

Return value: the number of integer nodes



#### mln_rbtree_null

```c
mln_rbtree_null(mln_rbtree_node_t *ptr, mln_rbtree_t *ptree)
```

Description: Used to detect whether the red-black tree node `ptr` is empty.

Return value: This test is mainly used for `mln_rbtree_search` operation, when no node is found, it returns `non-0`, otherwise it returns `0`.



#### mln_rbtree_root

```c
mln_rbtree_root(ptree)
```

Description: Get the root node pointer of the tree structure.

Return value: root node pointer



#### mln_rbtree_node_data_get

```c
mln_rbtree_node_data_get(node)
```

Description: Get the user-defined data associated with the tree node `node`.

Return value: user-defined data



#### mln_rbtree_node_data_set

```c
mln_rbtree_node_data_set(node, ud)
```

Description: Set the user-defined data associated with the tree node `node` to `ud`.

Return value: None



### Example

```c
#include <stdio.h>
#include <stdlib.h>
#include "mln_rbtree.h"

static int cmp_handler(const void *data1, const void *data2)
{
    return *(int *)data1 - *(int *)data2;
}

int main(int argc, char *argv[])
{
    int n = 10;
    mln_rbtree_t *t;
    mln_rbtree_node_t *rn;
    struct mln_rbtree_attr rbattr;

    rbattr.pool = NULL;
    rbattr.pool_alloc = NULL;
    rbattr.pool_free = NULL;
    rbattr.cmp = cmp_handler;
    rbattr.data_free = NULL;

    if ((t = mln_rbtree_new(&rbattr)) == NULL) {
        fprintf(stderr, "rbtree init failed.\n");
        return -1;
    }

    rn = mln_rbtree_node_new(t, &n);
    if (rn == NULL) {
        fprintf(stderr, "rbtree node init failed.\n");
        return -1;
    }
    mln_rbtree_insert(t, rn);

    rn = mln_rbtree_search(t, &n);
    if (mln_rbtree_null(rn, t)) {
        fprintf(stderr, "node not found\n");
        return -1;
    }
    printf("%d\n", *((int *)mln_rbtree_node_data_get(rn)));

    mln_rbtree_delete(t, rn);
    mln_rbtree_node_free(t, rn);

    mln_rbtree_free(t);

    return 0;
}
```



## Inline Usage

The purpose of inline usage is to improve the execution efficiency of each operation of the red-black tree. The core difference between it and the basic usage is that the existence of callback functions (eg, `cmp`, `data_free`) is avoided. That is, the callback function is turned into an inline function, and the function call is eliminated by using compiler optimization.

Although this method of use is efficient, it also has some usage scenario limitations.

> Scenario assumptions:
>
> We first created a red-black tree, which may be inserted into a lot of nodes in the future. And each tree node is a red-black tree (we temporarily call it a subtree). The related operations of this subtree are independently maintained by its associated code files, and are not related to other subtrees.
>
> Our requirement is: when the red-black tree (non-subtree) executes `mln_rbtree_free`, all subtrees will be released and destroyed together.

In the case of basic usage, we only need to set the `data_free` attribute of the red-black tree to `mln_rbtree_free`, and each subtree can be automatically released, regardless of the data type associated with each subtree node. and how to release these data types. Because these data types will be correctly released by the callback function set by the `data_free` attribute of the subtree.

However, in inline usage, the comparison and release operations of node data need to be clearly specified which function is to be handled. Combined with our scenario assumptions above, it means that the red-black tree must know the data type of each subtree, and then explicitly call the release function corresponding to the specified type to release resources. However, when resources are continuously nested between multiple data structures and code modules as in the above scenario, it is difficult for us to know the data types of other modules in a certain code module, and it is impossible to release these data type resources correctly. This is the scenario where inline usage doesn't quite work out.

The inline usage additionally provides a set of macro statement expressions to replace some functions in the basic usage:

- mln_rbtree_inline_insert
- mln_rbtree_inline_root_search
- mln_rbtree_inline_search
- mln_rbtree_inline_node_free
- mln_rbtree_inline_free
- mln_rbtree_inline_reset



### Functions/Macros



#### mln_rbtree_inline_insert

```c
mln_rbtree_inline_insert(t, n, compare)
```

Description: Same as `mln_rbtree_insert`. `compare` is the `cmp` function in `struct mln_rbtree_attr` in the basic usage, but at this time the function can be declared as `inline`, and it will be inlined after compiler optimization is turned on.

Return value: same as `mln_rbtree_insert`



#### mln_rbtree_inline_search

```c
mln_rbtree_inline_search(t, key, compare)
```

Description: Same as `mln_rbtree_search`. `compare` is the `cmp` function in `struct mln_rbtree_attr` in the basic usage, but at this time the function can be declared as `inline`, and it will be inlined after compiler optimization is turned on.

Return value: same as `mln_rbtree_search`



#### mln_rbtree_inline_root_search

```c
mln_rbtree_inline_root_search(t, root, key, compare)
```

Description: Similar to the function of `mln_rbtree_inline_search`, the only difference is that this operation is to search from the specified tree node `root`, not just the root node of the entire tree.

Return value: same as `mln_rbtree_inline_search`



#### mln_rbtree_inline_node_free

```c
mln_rbtree_inline_node_free(t, n, freer)
```

Description: Same as `mln_rbtree_node_free`. `freer` is the `data_free` function in `struct mln_rbtree_attr` in the basic usage, but at this time the function can be declared as `inline` and be inlined after compiler optimization is turned on.

Return value: same as `mln_rbtree_node_free`



#### mln_rbtree_inline_free

```c
mln_rbtree_inline_free(t, freer)
```

Description: Same as `mln_rbtree_free`. `freer` is the `data_free` function in `struct mln_rbtree_attr` in the basic usage, but at this time the function can be declared as `inline` and be inlined after compiler optimization is turned on.

Return value: same as `mln_rbtree_free`



#### mln_rbtree_inline_reset

```
mln_rbtree_inline_reset(t, freer)
```

Description: Same as `mln_rbtree_reset`. `freer` is the `data_free` function in `struct mln_rbtree_attr` in the basic usage, but at this time the function can be declared as `inline` and be inlined after compiler optimization is turned on.

Return value: same as `mln_rbtree_reset`



It can be seen that these new APIs have added some parameters, and these parameters happen to be the callback function in `struct mln_rbtree_attr`. Therefore, in the case of inline usage, `mln_rbtree_new` can set the parameter to `NULL` when creating a red-black tree (if there is no custom memory pool), because these callback functions are all in the above-mentioned red-black tree operation interface given.



### Example

Let's modify the example in the basic usage:

```c
#include <stdio.h>
#include <stdlib.h>
#include "mln_rbtree.h"

//inline function
static inline int cmp_handler(const void *data1, const void *data2)
{
    return *(int *)data1 - *(int *)data2;
}

int main(int argc, char *argv[])
{
    int n = 10;
    mln_rbtree_t *t;
    mln_rbtree_node_t *rn;

    if ((t = mln_rbtree_new(NULL)) == NULL) { //attr can be NULL
        fprintf(stderr, "rbtree init failed.\n");
        return -1;
    }

    rn = mln_rbtree_node_new(t, &n);
    if (rn == NULL) {
        fprintf(stderr, "rbtree node init failed.\n");
        return -1;
    }
    mln_rbtree_inline_insert(t, rn, cmp_handler); //inline insert

    rn = mln_rbtree_inline_search(t, &n, cmp_handler); //inline search
    if (mln_rbtree_null(rn, t)) {
        fprintf(stderr, "node not found\n");
        return -1;
    }
    printf("%d\n", *((int *)mln_rbtree_node_data_get(rn)));

    mln_rbtree_delete(t, rn);
    mln_rbtree_node_free(t, rn); //cannot use mln_rbtree_inline_node_free cause the last argument can not be NULL

    mln_rbtree_free(t); //mln_rbtree_inline_free cannot be used cause the last argument can not be NULL

    return 0;
}
```





## Container Usage

The container usage does not conflict with the above two usages, on the contrary, the container usage needs to use the functions/macros of the above two usages to operate the red-black tree. The meaning of the container is to use the red-black tree node structure as an attribute in the user-defined data structure. For example:

```c
struct user_defined_s {
    int int_val;
    ...
    mln_rbtree_node_t node; //Note that this is not a pointer
    ...
};
```

In this structure definition, `node` is a red-black tree node type, which is a member variable in the structure `user_defined_s`.



### Functions/Macros

In container usage, we use `mln_rbtree_node_init` to replace `mln_rbtree_node_new` to initialize red-black tree nodes. The difference between these two interfaces is that `mln_rbtree_node_init` does not dynamically allocate the node structure (because it has been defined in other custom structures and is allocated together when the custom structure is instantiated).



#### mln_rbtree_node_init

```c
mln_rbtree_node_init(n, ud)
```

Description: Initialize the tree node pointer `n`, and associate the user-defined data `ud` with the node structure `n`.

Return value: tree node structure pointer `n`



### Example

```c
#include <stdio.h>
#include "mln_rbtree.h"

typedef struct user_defined_s {
    int val;
    mln_rbtree_node_t node; //node as member
} ud_t; //user-defined structure

static int cmp_handler(const void *data1, const void *data2)
{
    return ((ud_t *)data1)->val - ((ud_t *)data2)->val;
}

int main(int argc, char *argv[])
{
    mln_rbtree_t *t;
    ud_t data1, data2; //at this point, node has been allocated
    mln_rbtree_node_t *rn;
    struct mln_rbtree_attr rbattr;

    rbattr.pool = NULL;
    rbattr.pool_alloc = NULL;
    rbattr.pool_free = NULL;
    rbattr.cmp = cmp_handler;
    rbattr.data_free = NULL;

    if ((t = mln_rbtree_new(&rbattr)) == NULL) {
        fprintf(stderr, "rbtree init failed.\n");
        return -1;
    }

    //Initialize and assign custom data
    data1.val = 1;
    data2.val = 2;
    mln_rbtree_node_init(&data1.node, &data1); //Initialize the node node in the custom structure, and use the custom structure as node associated data
    mln_rbtree_node_init(&data2.node, &data2);

    mln_rbtree_insert(t, &data1.node);
    mln_rbtree_insert(t, &data2.node);

    rn = mln_rbtree_search(t, &data1);
    if (mln_rbtree_null(rn, t)) {
        fprintf(stderr, "node not found\n");
        return -1;
    }

    //Here are two ways to get a pointer to a custom data structure
    printf("%d\n", ((ud_t *)mln_rbtree_node_data_get(rn))->val);
    printf("%d\n", mln_container_of(rn, ud_t, node)->val);

    mln_rbtree_delete(t, &data1.node);
    mln_rbtree_node_free(t, &data1.node);

    mln_rbtree_free(t);

    return 0;
}
```

