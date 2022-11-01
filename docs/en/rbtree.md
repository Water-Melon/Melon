## Red-black Tree



### Header file

```c
#include "mln_rbtree.h"
```



### Structures

```c
typedef struct rbtree_s {
    void                      *pool; //The memory pool structure, or NULL if the memory pool is not used
    rbtree_pool_alloc_handler  pool_alloc; //memory pool allocation function
    rbtree_pool_free_handler   pool_free; //memory pool release function
    mln_rbtree_node_t          nil; //empty node
    mln_rbtree_node_t         *root; //root node
    mln_rbtree_node_t         *min; //min-value node
    mln_rbtree_node_t         *head;
    mln_rbtree_node_t         *tail;
    mln_rbtree_node_t         *free_head;
    mln_rbtree_node_t         *free_tail;
    mln_rbtree_node_t         *iter;
    rbtree_cmp                 cmp; //Node Comparison Function
    rbtree_free_data           data_free; //In-node data release function
    mln_uauto_t                nr_node; //total number of nodes
    mln_u32_t                  del:1;
    mln_u32_t                  cache:1; //Whether to cache all unused node
} mln_rbtree_t;

struct mln_rbtree_node_s {
    void                      *data;//user data
    struct mln_rbtree_node_s  *prev;
    struct mln_rbtree_node_s  *next;
    struct mln_rbtree_node_s  *parent;
    struct mln_rbtree_node_s  *left;
    struct mln_rbtree_node_s  *right;
    enum rbtree_color          color;
};
```

The first structure is a red-black tree structure. In this structure, we only need to pay attention to `root` and `nr_node`. And all the variables here do not need to be modified externally, they will be modified by themselves during the red-black tree operation.

The second structure is a red-black tree node structure. In this structure, we only focus on the `data` field, which holds the data stored in the red-black tree.



### Functions/Macros



#### mln_rbtree_null

```c
mln_rbtree_null(mln_rbtree_node_t *ptr, mln_rbtree_t *ptree)
```

Description: Used to detect whether the red-black tree node `ptr` is empty.

Return value: This check is mainly used for the `mln_rbtree_search` operation. When no node is found, it returns `not 0`, otherwise it returns `0`.



#### mln_rbtree_init

```c
mln_rbtree_t *mln_rbtree_init(struct mln_rbtree_attr *attr);

struct mln_rbtree_attr {
    mln_alloc_t              *pool;//The memory pool, if not needed, set NULL, at this time the red-black tree nodes will be allocated by malloc
    rbtree_cmp                cmp;//Red-black tree node comparison function
    rbtree_free_data          data_free;//User data release function of red-black tree node
    mln_u32_t                 cache:1;//Whether to cache all unused red-black tree nodes
};

typedef int (*rbtree_cmp)(const void *, const void *);
typedef void (*rbtree_free_data)(void *);
```

Description:

Initialize the red-black tree.

`pool` is optional, if you do not need to use the memory pool to allocate memory, set `NULL`.

`cmp` is a node comparison function, which is almost always called in various operations of the red-black tree. The two parameters of this function are user data with the same data structure as `data` in `mln_rbtree_node_t`, and the return value of the function is:

- `-1` the first argument is less than the second argument
- `0` both parameters are the same
- `1` The first argument is greater than the second argument

`data_free` is used to free the user resources pointed to by `data` in the `mln_rbtree_node_t` structure. Set `NULL` if user resources do not need to be released. This function has only one parameter which is the same type of structure as `data` in `mln_rbtree_node_t`.

`cache` is used to indicate whether the red-black tree caches the red-black tree node structure that has been released user resources. **Note**, all node structures are cached.

Return value: return `mln_rbtree_t` pointer successfully, otherwise return `NULL`



#### mln_rbtree_destroy

```c
void mln_rbtree_destroy(mln_rbtree_t *t);
```

Description: Destroy the red-black tree and release the resources stored in each node on it.

Return value: none



#### mln_rbtree_insert

```c
void mln_rbtree_insert(mln_rbtree_t *t, mln_rbtree_node_t *n);
```

Description: Insert a red-black tree node into a red-black tree.

Return value: none



#### mln_rbtree_delete

```c
void mln_rbtree_delete(mln_rbtree_t *t, mln_rbtree_node_t *n);
```

Description: Remove the red-black tree node from the red-black tree. **Note**, this operation will not release the user data resources stored in the node.

Return value: none



#### mln_rbtree_successor

```c
mln_rbtree_node_t *mln_rbtree_successor(mln_rbtree_t *t, mln_rbtree_node_t *n);
```

Description: Get the successor node of the specified node `n`, that is, the next node with larger data than the node `n`.

Return value: red-black tree node pointer, the macro `mln_rbtree_null` needs to be used to confirm whether there is a successor node



#### mln_rbtree_search

```c
mln_rbtree_node_t *mln_rbtree_search(mln_rbtree_t *t, mln_rbtree_node_t *root, const void *key);
```

Description: Start from the `root` node of the red-black tree `t` to find if there is a node with the same `key`. **Note**, `key` should belong to the same data structure as user data.

Return value: red-black tree node pointer, you need to use the macro `mln_rbtree_null` to confirm whether the node is found



#### mln_rbtree_min

```c
mln_rbtree_node_t *mln_rbtree_min(mln_rbtree_t *t);
```

Description: Get the node with the smallest user data in the red-black tree `t`.

Return value: red-black tree node pointer, the macro `mln_rbtree_null` needs to be used to confirm whether the node exists



#### mln_rbtree_node_new

```c
mln_rbtree_node_t *mln_rbtree_node_new(mln_rbtree_t *t, void *data);
```

Description: Create a red-black tree node. The memory required by the node is determined by `pool` and `cache` in `t`. `data` is the user data associated with this node.

Return value: `mln_rbtree_node_t` pointer if successful, `NULL` otherwise



#### mln_rbtree_node_free

```c
void mln_rbtree_node_free(mln_rbtree_t *t, mln_rbtree_node_t *n);
```

Description:

Free the red-black tree node `n`.

When freeing, the node memory will be reclaimed according to the `cache` and `pool` in the red-black tree `t`, and the user data will be released by the `data_free` callback function.

Return value: none



#### mln_rbtree_iterate

```c
int mln_rbtree_iterate(mln_rbtree_t *t, rbtree_iterate_handler handler, void *udata);

typedef int (*rbtree_iterate_handler)(mln_rbtree_node_t *node, void *rn_data, void *udata);
```

Description:

Traverse every node in the red-black tree `t`. And supports deleting tree nodes while traversing.

`handler` is an access function to traverse each node. The meanings of the three parameters of this function are as follows:

- `node` tree node structure currently visited
- `rn_data` `data` within the current tree node
- `udata` is the third parameter of `mln_rbtree_iterate`, which is a parameter passed in by the user. If not needed, it can be set to `NULL`

The reason why the node node is additionally given is because there may be a requirement: to replace the data in the node during the traversal (not recommended because it will violate the existing order of the red-black tree node), but it needs to be used with caution .

return value:

- `mln_rbtree_iterate`: return `0` after all traversal, otherwise return `-1`
- `rbtree_iterate_handler`: return `-1` if the traversal is expected to be interrupted, otherwise the return value should be `greater than or equal to 0`



#### mln_rbtree_reset

```c
void mln_rbtree_reset(mln_rbtree_t *t);
```

Description:

Reset red-black tree `t`, all nodes in it will be freed.

Return value: none



### Example

```c
#include <stdio.h>
#include <stdlib.h>
#include "mln_core.h"
#include "mln_log.h"
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

    rbattr.pool = NULL;
    rbattr.pool_alloc = NULL;
    rbattr.pool_free = NULL;
    rbattr.cmp = cmp_handler;
    rbattr.data_free = NULL;
    rbattr.cache = 0;

    if ((t = mln_rbtree_init(&rbattr)) == NULL) {
        mln_log(error, "rbtree init failed.\n");
        return -1;
    }

    rn = mln_rbtree_node_new(t, &n);
    if (rn == NULL) {
        mln_log(error, "rbtree node init failed.\n");
        return -1;
    }
    mln_rbtree_insert(t, rn);

    rn = mln_rbtree_search(t, t->root, &n);
    if (mln_rbtree_null(rn, t)) {
        mln_log(error, "node not found\n");
        return -1;
    }
    mln_log(debug, "%d\n", *((int *)(rn->data)));

    mln_rbtree_delete(t, rn);
    mln_rbtree_node_free(t, rn);

    mln_rbtree_destroy(t);

    return 0;
}
```

