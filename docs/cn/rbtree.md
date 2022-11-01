## 红黑树



### 头文件

```c
#include "mln_rbtree.h"
```



### 相关结构

```c
typedef struct rbtree_s {
    void                      *pool; //内存池结构，若未使用内存池则为NULL
    rbtree_pool_alloc_handler  pool_alloc; //内存池分配函数
    rbtree_pool_free_handler   pool_free; //内存池释放函数
    mln_rbtree_node_t          nil; //空节点
    mln_rbtree_node_t         *root; //根节点
    mln_rbtree_node_t         *min; //最小节点
    mln_rbtree_node_t         *head;
    mln_rbtree_node_t         *tail;
    mln_rbtree_node_t         *free_head;
    mln_rbtree_node_t         *free_tail;
    mln_rbtree_node_t         *iter;
    rbtree_cmp                 cmp; //节点比较函数
    rbtree_free_data           data_free; //节点内数据释放函数
    mln_uauto_t                nr_node; //节点总个数
    mln_u32_t                  del:1;
    mln_u32_t                  cache:1; //是否缓存所有节点结构
} mln_rbtree_t;

struct mln_rbtree_node_s {
    void                      *data;//存放了用户数据
    struct mln_rbtree_node_s  *prev;
    struct mln_rbtree_node_s  *next;
    struct mln_rbtree_node_s  *parent;
    struct mln_rbtree_node_s  *left;
    struct mln_rbtree_node_s  *right;
    enum rbtree_color          color;
};
```

第一个结构为红黑树结构，这个结构中我们只需要关注`root`和`nr_node`即可。并且这里所有的变量无需外部修改，均会在红黑树操作时自行修改。

第二个结构为红黑树节点结构。在这个结构中，我们仅关注`data`字段，该字段存放了红黑树中存放的数据。



### 函数/宏



#### mln_rbtree_null

```c
mln_rbtree_null(mln_rbtree_node_t *ptr, mln_rbtree_t *ptree)
```

描述：用于检测红黑树节点`ptr`是否是空。

返回值：这个检测主要用于`mln_rbtree_search`操作，当查找不到节点时，返回`非0`，否则返回`0`。



#### mln_rbtree_init

```c
mln_rbtree_t *mln_rbtree_init(struct mln_rbtree_attr *attr);

struct mln_rbtree_attr {
    mln_alloc_t              *pool;//内存池，如果不需要则置NULL，此时红黑树节点将由malloc进行分配
    rbtree_cmp                cmp;//红黑树节点比较函数
    rbtree_free_data          data_free;//红黑树节点的用户数据释放函数
    mln_u32_t                 cache:1;//是否缓存所有红黑树节点
};

typedef int (*rbtree_cmp)(const void *, const void *);
typedef void (*rbtree_free_data)(void *);
```

描述：

初始化红黑树。

`pool`为可选项，若不需要使用内存池分配内存，则置`NULL`。

`cmp`为节点比较函数，在红黑树的各类操作中几乎都会被调用。该函数的两个参数均为与`mln_rbtree_node_t`中`data`相同数据结构的用户数据，函数的返回值为：

- `-1` 第一个参数小于第二个参数
-  `0`  两个参数相同
-  `1`  第一个参数大于第二个参数

`data_free`用于释放`mln_rbtree_node_t`结构中`data`所指向的用户资源。若不需要释放用户资源，则置`NULL`。该函数仅有一个参数为`mln_rbtree_node_t`中`data`同类型结构。

`cache`用于指示红黑树是否缓存已被释放用户资源的红黑树节点结构。**注意**，是所有的节点结构都会被缓存。

返回值：成功返回`mln_rbtree_t`指针，否则返回`NULL`



#### mln_rbtree_destroy

```c
void mln_rbtree_destroy(mln_rbtree_t *t);
```

描述：销毁红黑树，并释放其上每个节点内存放的资源。

返回值：无



#### mln_rbtree_insert

```c
void mln_rbtree_insert(mln_rbtree_t *t, mln_rbtree_node_t *n);
```

描述：将红黑树节点插入红黑树。

返回值：无



#### mln_rbtree_delete

```c
void mln_rbtree_delete(mln_rbtree_t *t, mln_rbtree_node_t *n);
```

描述：将红黑树节点从红黑树中移除。**注意**，本操作不会释放节点中存放的用户数据资源。

返回值：无



#### mln_rbtree_successor

```c
mln_rbtree_node_t *mln_rbtree_successor(mln_rbtree_t *t, mln_rbtree_node_t *n);
```

描述：获取指定节点`n`的后继节点，即比节点`n`中数据大的下一节点。

返回值：红黑树节点指针，需要以宏`mln_rbtree_null`来确认是否存在后继节点



#### mln_rbtree_search

```c
mln_rbtree_node_t *mln_rbtree_search(mln_rbtree_t *t, mln_rbtree_node_t *root, const void *key);
```

描述：从红黑树`t`的`root`节点开始查找是否存在与`key`相同的节点。**注意**，`key`应与用户数据属于同一数据结构。

返回值：红黑树节点指针，需要以宏`mln_rbtree_null`来确认是否找到节点



#### mln_rbtree_min

```c
mln_rbtree_node_t *mln_rbtree_min(mln_rbtree_t *t);
```

描述：获取红黑树`t`内用户数据最小的节点。

返回值：红黑树节点指针，需要以宏`mln_rbtree_null`来确认是否存在该节点



#### mln_rbtree_node_new

```c
mln_rbtree_node_t *mln_rbtree_node_new(mln_rbtree_t *t, void *data);
```

描述：创建红黑树节点，节点所需内存由`t`中的`pool`和`cache`决定如何分配。`data`为与该节点关联的用户数据。

返回值：成功则返回`mln_rbtree_node_t`指针，否则为`NULL`



#### mln_rbtree_node_free

```c
void mln_rbtree_node_free(mln_rbtree_t *t, mln_rbtree_node_t *n);
```

描述：

释放红黑树节点`n`。

释放时会根据红黑树`t`中的`cache`及`pool`来决定节点内存如何回收，并且用户数据将由`data_free`回调函数决定如何释放。

返回值：无



#### mln_rbtree_iterate

```c
int mln_rbtree_iterate(mln_rbtree_t *t, rbtree_iterate_handler handler, void *udata);

typedef int (*rbtree_iterate_handler)(mln_rbtree_node_t *node, void *udata);
```

描述：

遍历红黑树`t`中每一个节点。且支持在遍历时删除树节点。

·`handler`为遍历每个节点的访问函数，该函数的三个参数含义依次为：

- `node` 当前访问的树节点结构
- `udata`为`mln_rbtree_iterate`的第三个参数，是由用户传入的参数，若不需要则可以置`NULL`

之所以额外给出node节点，是因为可能存在需求：在遍历中替换节点内的数据（不建议如此做，因为会违反红黑树节点现有有序性），但需要**慎重**使用。

返回值：

- `mln_rbtree_iterate`：全部遍历完返回`0`，否则返回`-1`
- `rbtree_iterate_handler`： 期望中断遍历则返回`-1`，否则返回值应`大于等于0`



#### mln_rbtree_reset

```c
void mln_rbtree_reset(mln_rbtree_t *t);
```

描述：

重制整个红黑树`t`，将其中全部节点释放。

返回值：无



### 示例

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

