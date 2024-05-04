## 红黑树

本文档对红黑树组件如何使用进行说明。

在红黑树组件中，有两种使用模式：

- 基础用法
- 内联用法 (`msvc`不支持)
- 容器用法



### 头文件

```c
#include "mln_rbtree.h"
```



### 模块名

`rbtree`



### 相关结构

在红黑树组件中，使用者不需要了解树和结点结构，只需要利用本组件提供的函数或者宏获取到对应信息。



## 基本用法

基本用法中，包含了如下函数和宏：

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



### 函数/宏



#### mln_rbtree_new

```c
mln_rbtree_t *mln_rbtree_new(struct mln_rbtree_attr *attr);

struct mln_rbtree_attr {
    void                      *pool; //内存池结构，若未使用内存池则为NULL
    rbtree_pool_alloc_handler  pool_alloc; //内存池分配函数
    rbtree_pool_free_handler   pool_free; //内存池释放函数
    rbtree_cmp                 cmp;//红黑树结点比较函数
    rbtree_free_data           data_free;//红黑树结点的用户数据释放函数
};

typedef int (*rbtree_cmp)(const void *, const void *);
typedef void (*rbtree_free_data)(void *);
```

描述：

初始化红黑树。

`attr`为初始化属性参数，若不提供，则默认为所有属性均为`NULL`。

`attr`中的`pool`、`pool_alloc`和`pool_free`是用来支持用户自定义内存池实现的。即可以从用户指定的内存池中分配红黑树结构及树结点结构。

`attr`中的`cmp`为结点比较函数，在红黑树的各类操作中几乎都会被调用。该函数的两个参数均为用户自定义数据结构指针（由`mln_rbtree_node_new`函数第二个参数指定），函数的返回值为：

- `-1` 第一个参数小于第二个参数
-  `0`  两个参数相同
-  `1`  第一个参数大于第二个参数

`data_free`用于释放红黑树结点中的用户自定义数据结构。这个回调函数会在`mln_rbtree_node_free`以及`mln_rbtree_free`函数中被调用。即对树结构或者树结点结构进行释放时被调用，将结点中的用户自定义数据一同清理。若不需要释放用户资源，则置`NULL`。

返回值：成功返回`mln_rbtree_t`指针，否则返回`NULL`



#### mln_rbtree_free

```c
void mln_rbtree_free(mln_rbtree_t *t);
```

描述：销毁红黑树，并释放其上每个结点内存放的资源。

返回值：无



#### mln_rbtree_insert

```c
void mln_rbtree_insert(mln_rbtree_t *t, mln_rbtree_node_t *n);
```

描述：将红黑树结点插入红黑树。第二个参数可由`mln_rbtree_node_new`函数创建。

返回值：无



#### mln_rbtree_delete

```c
void mln_rbtree_delete(mln_rbtree_t *t, mln_rbtree_node_t *n);
```

描述：将红黑树结点从红黑树中移除。**注意**，本操作不会释放树结点以及结点中存放的用户自定义数据。

返回值：无



#### mln_rbtree_successor

```c
mln_rbtree_node_t *mln_rbtree_successor(mln_rbtree_t *t, mln_rbtree_node_t *n);
```

描述：获取指定结点`n`的后继结点，即比结点`n`中数据大的下一结点。

返回值：红黑树结点指针，需要以宏`mln_rbtree_null`来确认是否存在后继结点



#### mln_rbtree_search

```c
mln_rbtree_node_t *mln_rbtree_search(mln_rbtree_t *t, const void *key);
```

描述：从红黑树`t`的根结点开始查找是否存在与`key`相同的结点。**注意**，`key`应当与用户自定义数据属于同一数据类型。

返回值：红黑树结点指针，需要以宏`mln_rbtree_null`来确认是否找到结点



#### mln_rbtree_min

```c
mln_rbtree_node_t *mln_rbtree_min(mln_rbtree_t *t);
```

描述：获取红黑树`t`内用户数据最小的结点。

返回值：红黑树结点指针，需要以宏`mln_rbtree_null`来确认是否存在该结点



#### mln_rbtree_node_new

```c
mln_rbtree_node_t *mln_rbtree_node_new(mln_rbtree_t *t, void *data);
```

描述：创建红黑树结点。`data`为与该结点关联的用户自定义数据。

返回值：成功则返回`mln_rbtree_node_t`指针，否则为`NULL`



#### mln_rbtree_node_free

```c
void mln_rbtree_node_free(mln_rbtree_t *t, mln_rbtree_node_t *n);
```

描述：释放红黑树结点`n`的资源，并可能会释放其关联的用户自定义数据（由`mln_rbtree_new`时给定的属性决定）。

返回值：无



#### mln_rbtree_iterate

```c
int mln_rbtree_iterate(mln_rbtree_t *t, rbtree_iterate_handler handler, void *udata);

typedef int (*rbtree_iterate_handler)(mln_rbtree_node_t *node, void *udata);
```

描述：

遍历红黑树`t`中每一个结点。且支持在遍历时删除树结点。

`handler`为遍历每个结点的访问函数，该函数的两个参数含义依次为：

- `node` 当前访问的树结点结构
- `udata`为`mln_rbtree_iterate`的第三个参数，是由用户传入的参数，若不需要则可以置`NULL`

之所以额外给出node结点，是因为可能存在需求：在遍历中替换结点内的数据（不建议如此做，因为会违反红黑树结点现有有序性），或将树结点的用户自定义数据置`NULL`从而临时避免数据被释放。需要**慎重**使用。

返回值：

- `mln_rbtree_iterate`：全部遍历完返回`0`，否则返回`-1`
- `rbtree_iterate_handler`： 期望中断遍历则返回`-1`，否则返回值应`大于等于0`



#### mln_rbtree_reset

```c
void mln_rbtree_reset(mln_rbtree_t *t);
```

描述：重置整个红黑树`t`，将其中全部结点释放。

返回值：无



#### mln_rbtree_node_num

```c
mln_rbtree_node_num(ptree)
```

描述：获取当前树中结点个数。

返回值：整型结点数



#### mln_rbtree_null

```c
mln_rbtree_null(mln_rbtree_node_t *ptr, mln_rbtree_t *ptree)
```

描述：用于检测红黑树结点`ptr`是否是空。

返回值：这个检测主要用于`mln_rbtree_search`操作，当查找不到结点时，返回`非0`，否则返回`0`。



#### mln_rbtree_root

```c
mln_rbtree_root(ptree)
```

描述：获取树结构的根结点指针。

返回值：根结点指针



#### mln_rbtree_node_data_get

```c
mln_rbtree_node_data_get(node)
```

描述：获取树结点`node`关联的用户自定义数据。

返回值：用户自定义数据



#### mln_rbtree_node_data_set

```c
mln_rbtree_node_data_set(node, ud)
```

描述：设置树结点`node`关联的用户自定义数据为`ud`。

返回值：无



### 示例

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



## 内联用法

内联用法目的是为了提升红黑树各个操作的执行效率。其与基本用法的核心区别在于，避免了回调函数的存在（例如，`cmp`，`data_free`）。即将回调函数变为内联函数，利用编译器优化消除函数调用。

这种使用方法虽然高效，但也存在一些使用场景限制。

> 场景假设：
>
> 我们首先创建了一棵红黑树，这棵树在未来可能要被插入非常多的结点。而每一个树结点，又都是一棵红黑树（我们暂时称之为子树），这棵子树的相关操作被其关联的代码文件独立维护，与其他子树都无关联。
>
> 我们的需求是：红黑树（非子树）执行`mln_rbtree_free`时，将所有子树一同释放销毁。

在基本用法的情况下，我们只需要将红黑树的`data_free`属性设置为`mln_rbtree_free`，就可以自动释放每一棵子树，而不需要考虑每棵子树结点关联的数据类型是什么，以及如何释放这些数据类型。因为这些数据类型都会被子树的`data_free`属性所设置的回调函数正确的释放。

但在内联用法下，结点数据的比较和释放操作都是需要明确指定是由哪个函数处理的。结合我们上面的场景假设，也就意味着，红黑树必须知道每棵子树的数据类型，然后显示调用指定类型对应的释放函数来释放资源。但当资源如上述场景一般，不断在多个数据结构间和代码模块间嵌套时，我们很难在某一个代码模块内得知其他模块的数据类型，也就无法正确释放这些数据类型资源了。这便是内联用法无法完美解决的场景。

内联用法额外提供了一组宏语句表达式来取代基本用法中的部分函数：

- mln_rbtree_inline_insert
- mln_rbtree_inline_root_search
- mln_rbtree_inline_search
- mln_rbtree_inline_node_free
- mln_rbtree_inline_free
- mln_rbtree_inline_reset



### 函数/宏



#### mln_rbtree_inline_insert

```c
mln_rbtree_inline_insert(t, n, compare)
```

描述：与`mln_rbtree_insert`功能一致。`compare`是基本用法中`struct mln_rbtree_attr`中的`cmp`函数，但此时该函数可以被声明为`inline`，并在开启编译优化后被内联。

返回值：与`mln_rbtree_insert`一样



#### mln_rbtree_inline_search

```c
mln_rbtree_inline_search(t, key, compare)
```

描述：与`mln_rbtree_search`功能一致。`compare`是基本用法中`struct mln_rbtree_attr`中的`cmp`函数，但此时该函数可以被声明为`inline`，并在开启编译优化后被内联。

返回值：与`mln_rbtree_search`一样



#### mln_rbtree_inline_root_search

```c
mln_rbtree_inline_root_search(t, root, key, compare)
```

描述：与`mln_rbtree_inline_search`的功能相似，差异仅是本操作是从指定树结点`root`开始进行搜索，而不仅仅只是整棵树的根结点。

返回值：与`mln_rbtree_inline_search`一样



#### mln_rbtree_inline_node_free

```c
mln_rbtree_inline_node_free(t, n, freer)
```

描述：与`mln_rbtree_node_free`功能一致。`freer`是基本用法中`struct mln_rbtree_attr`中的`data_free`函数，但此时该函数可以被声明为`inline`，并在开启编译优化后被内联。

返回值：与`mln_rbtree_node_free`一样



#### mln_rbtree_inline_free

```c
mln_rbtree_inline_free(t, freer)
```

描述：与`mln_rbtree_free`功能一致。`freer`是基本用法中`struct mln_rbtree_attr`中的`data_free`函数，但此时该函数可以被声明为`inline`，并在开启编译优化后被内联。

返回值：与`mln_rbtree_free`一样



#### mln_rbtree_inline_reset

```
mln_rbtree_inline_reset(t, freer)
```

描述：与`mln_rbtree_reset`功能一致。`freer`是基本用法中`struct mln_rbtree_attr`中的`data_free`函数，但此时该函数可以被声明为`inline`，并在开启编译优化后被内联。

返回值：与`mln_rbtree_reset`一样



可以看到，这些新的API都新增了一些参数，而这些参数恰好是`struct mln_rbtree_attr`中的回调函数。因此，在内联用法情况下，`mln_rbtree_new`创建红黑树时可以将参数设置为`NULL`（如果不存在自定义内存池的话），因为这些回调函数都在上述这些红黑树操作接口中给出了。



### 示例

我们将基本用法中的示例进行一下改造：

```c
#include <stdio.h>
#include <stdlib.h>
#include "mln_rbtree.h"

//被声明为inline
static inline int cmp_handler(const void *data1, const void *data2)
{
    return *(int *)data1 - *(int *)data2;
}

int main(int argc, char *argv[])
{
    int n = 10;
    mln_rbtree_t *t;
    mln_rbtree_node_t *rn;

    if ((t = mln_rbtree_new(NULL)) == NULL) { //attr参数为NULL
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
    mln_rbtree_node_free(t, rn); //不能用mln_rbtree_inline_node_free，因为最后一个参数不能为NULL

    mln_rbtree_free(t); //不能用mln_rbtree_inline_free，因为最后一个参数不能为NULL

    return 0;
}
```



## 容器用法

容器用法与上述两种用法并不冲突，相反容器用法需要使用上述两种用法的函数/宏来操作红黑树。容器的意思是指，将红黑树结点结构作为用户自定义数据结构中的一个属性。例如：

```c
struct user_defined_s {
    int int_val;
    ...
    mln_rbtree_node_t node; //注意，这里不是指针
    ...
};
```

这段结构定义中，`node`是红黑树结点类型，它是结构体`user_defined_s`中的一个成员变量。



### 函数/宏

容器用法中，我们使用`mln_rbtree_node_init`来取代`mln_rbtree_node_new`对红黑树结点的初始化工作。这两个接口的差异是，`mln_rbtree_node_init`并不会动态创建结点结构（因为已经在其他自定义结构体内定义，并在自定义结构体实例化时被一同创建出来了）。

#### mln_rbtree_node_init

```c
mln_rbtree_node_init(n, ud)
```

描述：对树结点指针`n`进行初始化，将用户自定义数据`ud`与结点结构`n`进行关联。

返回值：树结点结构指针`n`



### 示例

```c
#include <stdio.h>
#include "mln_rbtree.h"

typedef struct user_defined_s {
    int val;
    mln_rbtree_node_t node; //结点作为成员
} ud_t; //用户自定义结构

static int cmp_handler(const void *data1, const void *data2)
{
    return ((ud_t *)data1)->val - ((ud_t *)data2)->val;
}

int main(int argc, char *argv[])
{
    mln_rbtree_t *t;
    ud_t data1, data2; //此时，node已经被创建
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

    data1.val = 1; //对自定义数据进行初始化赋值
    data2.val = 2;

    mln_rbtree_node_init(&data1.node, &data1); //初始化自定义结构中的node结点，并将自定义结构作为结点关联数据
    mln_rbtree_node_init(&data2.node, &data2);

    mln_rbtree_insert(t, &data1.node);
    mln_rbtree_insert(t, &data2.node);

    rn = mln_rbtree_search(t, &data1);
    if (mln_rbtree_null(rn, t)) {
        fprintf(stderr, "node not found\n");
        return -1;
    }

    //这里给出了两种获取自定义数据结构指针的方法
    printf("%d\n", ((ud_t *)mln_rbtree_node_data_get(rn))->val);
    printf("%d\n", mln_container_of(rn, ud_t, node)->val);

    mln_rbtree_delete(t, &data1.node);
    mln_rbtree_node_free(t, &data1.node);

    mln_rbtree_free(t);

    return 0;
}
```

