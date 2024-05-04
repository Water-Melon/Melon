## 斐波那契堆

Melon中实现的是**最小堆**。

与红黑树组件类似，斐波那契堆组件有三种用法：

- 基本用法
- 内联用法 (`msvc`中不支持)
- 容器用法



### 头文件

```c
#include "mln_fheap.h"
```



### 模块名

`fheap`



### 相关结构

斐波那契堆及其堆结点结构为组件内部使用结构，开发者无需关心这两个数据结构的组成，使用本文所提供的函数/宏对堆和结点进行操作即可。



## 基本用法



### 函数/宏

#### mln_fheap_new

```c
mln_fheap_t *mln_fheap_new(void *min_val, struct mln_fheap_attr *attr);

struct mln_fheap_attr {
    void                     *pool;//内存池
    fheap_pool_alloc_handler  pool_alloc;//内存池分配内存函数指针
    fheap_pool_free_handler   pool_free;//内存池释放内存函数指针
    fheap_cmp                 cmp;//比较函数
    fheap_copy                copy;//复制函数
    fheap_key_free            key_free;//key释放函数
};
typedef int (*fheap_cmp)(const void *, const void *);
typedef void (*fheap_copy)(void *, void *);
typedef void (*fheap_key_free)(void *);
typedef void *(*fheap_pool_alloc_handler)(void *, mln_size_t);
typedef void (*fheap_pool_free_handler)(void *);
```

描述：创建斐波那契堆。

其中：

- `min_val`是指堆结点中用户自定义数据类型的最小值，这个指针的类型与堆结点内用户自定义数据的类型一致。注意，这个指针指向的内存不应被更改，其生命周期应至少贯穿该斐波那契堆的生命周期。

- `attr`是斐波那契堆的初始化属性结构，其中包含：

  - `pool`是用户定义的内存池结构，如果该项不为`NULL`，则斐波那契堆结构将从该内存池中分配，否则从malloc库中分配。
  - `pool_alloc`是内存池的分配内存函数指针，`pool`不为空时会使用该回调。
  - `pool_free`是内存池的释放内存函数指针，`pool`不为空时会使用该回调。
  - `cmp`是用于key比较大小的函数，该函数有两个参数，皆为用户自定义的key结构指针。如果参数1小于参数2，则返回`0`，否则返回`非0`。
  - `copy`是用来拷贝key结构的，这个回调函数会在`decrease key`时被调用，用于将原有key值改为新的key值。这个函数有两个参数，依次分别为：原有key值指针和新key值指针。
  - `key_free`为key值结构释放函数，如果不需要释放，则可以置`NULL`。

  这里额外提一句，如果使用内联模式，且未使用`pool`字段，则可以直接将`mln_fheap_new`的attr参数置`NULL`。

返回值：成功则返回`mln_fheap_t`类型指针，否则返回`NULL`



#### mln_fheap_new_fast

```c
mln_fheap_t *mln_fheap_new_fast(void *min_val, fheap_cmp cmp, fheap_copy copy, fheap_key_free key_free);

typedef int (*fheap_cmp)(const void *, const void *);
typedef void (*fheap_copy)(void *, void *);
typedef void (*fheap_key_free)(void *);
```

描述：创建斐波那契堆。与`mln_fheap_new_fast`的差异在于不支持内存池且将属性作为函数参数传递。

其中：

- `min_val`是指堆结点中用户自定义数据类型的最小值，这个指针的类型与堆结点内用户自定义数据的类型一致。注意，这个指针指向的内存不应被更改，其生命周期应至少贯穿该斐波那契堆的生命周期。
- `cmp`是用于key比较大小的函数，该函数有两个参数，皆为用户自定义的key结构指针。如果参数1小于参数2，则返回`0`，否则返回`非0`。
- `copy`是用来拷贝key结构的，这个回调函数会在`decrease key`时被调用，用于将原有key值改为新的key值。这个函数有两个参数，依次分别为：原有key值指针和新key值指针。
- `key_free`为key值结构释放函数，如果不需要释放，则可以置`NULL`。

返回值：成功则返回`mln_fheap_t`类型指针，否则返回`NULL`



#### mln_fheap_free

```c
void mln_fheap_free(mln_fheap_t *fh);
```

描述：销毁斐波那契堆，并根据`key_free`对堆内key进行释放。

返回值：无



#### mln_fheap_node_new

```c
mln_fheap_node_t *mln_fheap_node_new(mln_fheap_t *fh, void *key);
```

描述：创建一个斐波那契堆节点结构，`key`为用户自定义结构，`fh`为`mln_fheap_new`创建的堆。

返回值：成功则返回节点结构指针，否则返回`NULL`



#### mln_fheap_node_free

```c
void mln_fheap_node_free(mln_fheap_t *fh, mln_fheap_node_t *fn);
```

描述：释放斐波那契堆`fh`的节点`fn`的资源，并根据`key_free`对节点数据进行释放。

返回值：无



#### mln_fheap_insert

```c
void mln_fheap_insert(mln_fheap_t *fh, mln_fheap_node_t *fn);
```

描述：将堆节点插入堆中。

返回值：无



#### mln_fheap_minimum

```c
mln_fheap_node_t *mln_fheap_minimum(mln_fheap_t *fh);
```

描述：获取堆`fh`内当前key值最小的堆节点。

返回值：成功返回节点结构，否则返回`NULL`



#### mln_fheap_extract_min

```c
mln_fheap_node_t *mln_fheap_extract_min(mln_fheap_t *fh);
```

描述：将堆`fh`中当前key值最小的节点从堆中取出并返回。

返回值：成功则返回节点结构，否则返回`NULL`



#### mln_fheap_decrease_key

```c
int mln_fheap_decrease_key(mln_fheap_t *fh, mln_fheap_node_t *node, void *key);
```

描述：将堆`fh`中的节点`node`的key值降为`key`所给出的值。

**注意**：如果`key`大于原有key值则会执行失败返回。

返回值：成功返回`0`，否则返回`-1`



#### mln_fheap_delete

```c
void mln_fheap_delete(mln_fheap_t *fh, mln_fheap_node_t *node);
```

描述：将节点`node`从堆`fh`中删除，但不会释放`node`结点及其关联的资源。

返回值：无



### 示例

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



## 内联用法

内联用法目的是为了提升斐波那契堆各个操作的执行效率。其与基本用法的核心区别在于，避免了回调函数的存在（例如，`cmp`，`ke y_free`，`copy`）。即将回调函数变为内联函数，利用编译器优化消除函数调用。

这种使用方法虽然高效，但也存在一些使用场景限制。

> 场景假设：
>
> 我们首先创建了一斐波那契堆，这个堆在未来可能要被插入非常多的结点。而每一个结点，又都是一个斐波那契堆（我们暂时称之为子堆），这个子堆的相关操作被其关联的代码文件独立维护，与其他子堆都无关联。
>
> 我们的需求是：斐波那契堆（非子堆）执行`mln_fheap_free`时，将所有子堆一同释放销毁。



在基本用法的情况下，我们只需要将斐波那契堆的`key_free`属性设置为`mln_fheap_free`，就可以自动释放每一个子堆，而不需要考虑每个子堆结点关联的数据类型是什么，以及如何释放这些数据类型。因为这些数据类型都会被子堆的`key_free`属性所设置的回调函数正确的释放。

但在内联用法下，结点数据的比较和释放操作都是需要明确指定是由哪个函数处理的。结合我们上面的场景假设，也就意味着，斐波那契堆必须知道每个子堆的数据类型，然后显示调用指定类型对应的释放函数来释放资源。但当资源如上述场景一般，不断在多个数据结构间和代码模块间嵌套时，我们很难在某一个代码模块内得知其他模块的数据类型，也就无法正确释放这些数据类型资源了。这便是内联用法无法完美解决的场景。

内联用法额外提供了一组宏语句表达式来取代基本用法中的部分函数：

- mln_fheap_inline_insert
- mln_fheap_inline_extract_min
- mln_fheap_inline_decrease_key
- mln_fheap_inline_delete
- mln_fheap_inline_node_free
- mln_fheap_inline_free



### 函数/宏

#### mln_fheap_inline_insert

```c
mln_fheap_inline_insert(fh, fn, compare)
```

描述：与`mln_fheap_insert`功能一致，将结点`fn`插入斐波那契堆`fh`中。`compare`是基本用法中`struct mln_fheap_attr`中的`cmp`函数，但此时该函数可以被声明为`inline`，并在开启编译优化后被内联。如果`compare`为`NULL`，则会试图获取斐波那契堆`fh`的`cmp`回调函数。

返回值：与`mln_fheap_insert`一样



#### mln_fheap_inline_extract_min

```c
mln_fheap_inline_extract_min(fh, compare)
```

描述：与`mln_fheap_extract_min`功能一致，将值最小的堆结点从堆中移除并返回。`compare`是基本用法中`struct mln_fheap_attr`中的`cmp`函数，但此时该函数可以被声明为`inline`，并在开启编译优化后被内联。如果`compare`为`NULL`，则会试图获取斐波那契堆`fh`的`cmp`回调函数。

返回值：与`mln_fheap_extract_min`一样



#### mln_fheap_inline_decrease_key

```c
mln_fheap_inline_decrease_key(fh, node, k, cpy, compare)
```

描述：与`mln_fheap_decrease_key`功能一致，将结点`node`的key值降低为`k`。`compare`是基本用法中`struct mln_fheap_attr`中的`cmp`函数，`cpy`是`struct mln_fheap_attr`中的`copy`函数，但此时这两个函数可以被声明为`inline`，并在开启编译优化后被内联。如果`compare`为`NULL`，则会试图获取斐波那契堆`fh`的`cmp`回调函数。如果`cpy`为`NULL`，则会试图获取斐波那契堆`fh`的`copy`回调函数。

返回值：与`mln_fheap_decrease_key`一样



#### mln_fheap_inline_delete

```c
mln_fheap_inline_delete(fh, node, cpy, compare)
```

描述：与`mln_fheap_delete`功能一致，将结点`node`从堆`fh`中移除。`compare`是基本用法中`struct mln_fheap_attr`中的`cmp`函数，`cpy`是`struct mln_fheap_attr`中的`copy`函数，但此时这两个函数可以被声明为`inline`，并在开启编译优化后被内联。如果`compare`为`NULL`，则会试图获取斐波那契堆`fh`的`cmp`回调函数。如果`cpy`为`NULL`，则会试图获取斐波那契堆`fh`的`copy`回调函数。

返回值：与`mln_fheap_delete`一样



#### mln_fheap_inline_node_free

```c
mln_fheap_inline_node_free(fh, fn, freer)
```

描述：与`mln_fheap_node_free`功能一致，将结点`fn`释放。`freer`是基本用法中`struct mln_fheap_attr`中的`key_free`函数，但此时该函数可以被声明为`inline`，并在开启编译优化后被内联。如果`freer`为`NULL`，则会试图获取斐波那契堆`fh`的`key_free`回调函数。

返回值：与`mln_fheap_node_free`一样



#### mln_fheap_inline_free

```c
mln_fheap_inline_free(fh, compare, freer)
```

描述：与`mln_fheap_free`功能一致，将结点`fn`释放。`freer`是基本用法中`struct mln_fheap_attr`中的`key_free`函数，但此时该函数可以被声明为`inline`，并在开启编译优化后被内联。如果`freer`为`NULL`，则会试图获取斐波那契堆`fh`的`key_free`回调函数。

返回值：与`mln_fheap_free`一样



### 示例

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





## 容器用法

容器用法与上述两种用法并不冲突，相反容器用法需要使用上述两种用法的函数/宏来操作斐波那契堆。容器的意思是指，将斐波那契堆结点结构作为用户自定义数据结构中的一个属性。例如：

```c
struct user_defined_s {
    int int_val;
    ...
    mln_fheap_node_t node; //注意，这里不是指针
    ...
};
```

这段结构定义中，`node`是斐波那契堆结点类型，它是结构体`user_defined_s`中的一个成员变量。



### 函数/宏

容器用法中，我们使用`mln_fheap_node_init`来取代`mln_fheap_node_new`对堆结点的初始化工作。这两个接口的差异是，`mln_fheap_node_init`并不会动态创建结点结构（因为已经在其他自定义结构体内定义，并在自定义结构体实例化时被一同创建出来了）。

#### mln_fheap_node_init

```c
mln_fheap_node_init(fn, k)
```

描述：对堆结点指针`fn`进行初始化，将用户自定义key`k`与结点结构`fn`进行关联。

返回值：堆结点结构指针`fn`



### 示例

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
