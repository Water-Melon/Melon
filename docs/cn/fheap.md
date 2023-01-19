## 斐波那契堆

这里的实现的是**最小堆**。



### 头文件

```c
#include "mln_fheap.h"
```



### 相关结构

```c
typedef struct mln_fheap_node_s {
    void                    *key; //斐波那契堆节点中存放的用户数据
    ...
} mln_fheap_node_t;
```



### 函数



#### mln_fheap_new

```c
mln_fheap_t *mln_fheap_new(struct mln_fheap_attr *attr);

struct mln_fheap_attr {
    void                     *pool;//内存池
    fheap_pool_alloc_handler  pool_alloc;//内存池分配内存函数指针
    fheap_pool_free_handler   pool_free;//内存池释放内存函数指针
    fheap_cmp                 cmp;//比较函数
    fheap_copy                copy;//复制函数
    fheap_key_free            key_free;//key释放函数
    void                     *min_val;//最小值结构
    mln_size_t                min_val_size;//最小值结构字节大小
};
typedef int (*fheap_cmp)(const void *, const void *);
typedef void (*fheap_copy)(void *, void *);
typedef void (*fheap_key_free)(void *);
typedef void *(*fheap_pool_alloc_handler)(void *, mln_size_t);
typedef void (*fheap_pool_free_handler)(void *);
```

描述：创建斐波那契堆。

其中：

- `pool`是用户定义的内存池结构，如果该项不为`NULL`，则斐波那契堆结构将从该内存池中分配，否则从malloc库中分配。
- `pool_alloc`是内存池的分配内存函数指针。
- `pool_free`是内存池的释放内存函数指针。
- `cmp`是用于key比较大小的函数，该函数有两个参数，皆为用户自定义的key结构指针。如果参数1小于参数2，则返回`0`，否则返回`非0`。
- `copy`是用来拷贝key结构的，这个回调函数会在`decrease key`时被调用，用于将原有key值改为新的key值。这个函数有两个参数，依次分别为：原有key值指针和新key值指针。
- `key_free`为key值结构释放函数，如果不需要释放，则可以置`NULL`。
- `min_val`定义了最小key值，是一个用户自定义结构指针。原则上，`cmp`函数中遇到该值时都应该判定该值为小的一方。
- `min_val_size`定义了`min_val`的类型大小，以字节为单位。

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

描述：将节点`node`从堆`fh`中删除，并根据`key_free`的设置对节点内的资源进行释放。

返回值：无



### 示例

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

