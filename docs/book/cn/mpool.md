## 内存池

Melon中，内存池分为两类：

- 堆内存
- 共享内存

其中，共享内存内存池只允许主子进程之间共享数据（兄弟进程之间也共享）。即使用时，由主进程创建共享内存内存池，然后创建子进程。



### 头文件

```c
#include "mln_alloc.h"
```



### 模块名

`alloc`



### 函数



#### mln_alloc_init

```c
mln_alloc_t *mln_alloc_init(mln_alloc_t *parent, mln_size_t capacity);
```

描述：创建堆内存内存池。参数`parent`是一个内存池实例，该参数为`NULL`时，本函数创建的内存池将从堆中分配内存，若不为`NULL`时，则从`parent`所在池中分配内存。即池结构可级联。`capacity`是用于限制内存池容量的，当它的值为`0`时，表示内存池容量不受限，也就是跟随系统内存容量。

返回值：成功则返回内存池结构指针，否则返回`NULL`



#### mln_alloc_shm_init

```c
mln_alloc_t *mln_alloc_shm_init(mln_size_t capacity, void *locker, mln_alloc_shm_lock_cb_t lock, mln_alloc_shm_lock_cb_t unlock);
```

描述：创建共享内存内存池。

参数：

- `capacity` 本池建立时需要给出池大小（单位字节），一旦创建完毕后则后续无法再扩大。
- `locker` 是锁资源结构指针。
- `lock` 是用于对锁资源加锁的回调函数，该函数参数为锁资源指针。若加锁失败则返回`非0`值。
- `unlock` 是用于对锁资源解锁的回调函数，该函数参数为锁资源指针。若解锁失败则返回`非0`值。

```c
typedef int (*mln_alloc_shm_lock_cb_t)(void *);
```

`lock`与`unlock`只在子池中的函数调用时被使用。因此，如果你直接对共享内存池操作的话，需要自行在外部加解锁，作用于共享内存池的分配与释放函数不会调用该回调。

返回值：成功则返回内存池结构指针，否则返回`NULL`



#### mln_alloc_destroy

```c
void mln_alloc_destroy(mln_alloc_t *pool);
```

描述：销毁内存池。销毁操作会将内存池中管理的所有内存进行统一释放。

返回值：无



#### mln_alloc_m

```c
void *mln_alloc_m(mln_alloc_t *pool, mln_size_t size);
```

描述：从内存池`pool`中分配一个`size`大小的内存。如果内存池是共享内存内存池，则会从共享内存中进行分配，否则从堆内存中进行分配。

返回值：成功则返回内存起始地址，否则返回`NULL`



#### mln_alloc_c

```c
void *mln_alloc_c(mln_alloc_t *pool, mln_size_t size);
```

描述：从内存池`pool`中分配一个`size`大小的内存，且该内存会被清零。

返回值：成功则返回内存起始地址，否则返回`NULL`



#### mln_alloc_re

```c
void *mln_alloc_re(mln_alloc_t *pool, void *ptr, mln_size_t size);
```

描述：从内存池`pool`中分配一个`size`大小的内存，并将`ptr`指向的内存中的数据拷贝到新的内存中。

`ptr`必须为内存池分配的内存起始地址。若`size`为`0`，`ptr`指向的内存会被释放。

返回值：成功则返回内存起始地址，否则返回`NULL`



#### mln_alloc_free

```c
void mln_alloc_free(void *ptr);
```

描述：释放`ptr`指向的内存。**注意**：`ptr`必须为分配函数返回的地址，而不可以是分配的内存中某一个位置。

返回值：无



#### mln_alloc_available_capacity

```c
mln_size_t mln_alloc_available_capacity(mln_alloc_t *pool);
```

描述：获取当前内存池剩余可用容量（如果内存池设置了容量限制）。

注意：对于堆内存池，当调用`mln_alloc_m`时会欲分配若干与申请大小相似的内存块。在这种情况下，本函数返回的数值并不包含这些被预分配且未被使用的块的内存大小。

返回值：

- 堆内存：若在`mln_alloc_init`时第二个参数（`capacity`）不为`0`，则返回当前内存池剩余可用内存大小（详情参考上面注意事项），若为`0`，则返回`M_ALLOC_INFINITE_SIZE`。
- 共享内存：返回全部可用内存区的字节大小。



### 示例

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

