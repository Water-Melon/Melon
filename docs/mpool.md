内存池

Melon中，内存池分为两类：

- 堆内存
- 共享内存

其中，共享内存内存池只允许主子进程之间共享数据（兄弟进程之间也共享）。即使用时，由主进程创建共享内存内存池，然后创建子进程。



头文件

```c
#include "mln_alloc.h"
```



函数



mln_alloc_init

```c
mln_alloc_t *mln_alloc_init(void);
```

描述：创建堆内存内存池。

返回值：成功则返回内存池结构指针，否则返回`NULL`



mln_alloc_shm_init

```c
mln_alloc_t *mln_alloc_shm_init(mln_size_t size);
```

描述：创建共享内存内存池。本池建立时需要给出池大小`size`（单位字节），一旦创建完毕后则后续无法再扩大。

返回值：成功则返回内存池结构指针，否则返回`NULL`



mln_alloc_destroy

```c
void mln_alloc_destroy(mln_alloc_t *pool);
```

描述：销毁内存池。销毁操作会将内存池中管理的所有内存进行统一释放。

返回值：无



mln_alloc_m

```c
void *mln_alloc_m(mln_alloc_t *pool, mln_size_t size);
```

描述：从内存池`pool`中分配一个`size`大小的内存。如果内存池是共享内存内存池，则会从共享内存中进行分配，否则从堆内存中进行分配。

返回值：成功则返回内存起始地址，否则返回`NULL`



mln_alloc_c

```c
void *mln_alloc_c(mln_alloc_t *pool, mln_size_t size);
```

描述：从内存池`pool`中分配一个`size`大小的内存，且该内存会被清零。

返回值：成功则返回内存起始地址，否则返回`NULL`



mln_alloc_re

```c
void *mln_alloc_re(mln_alloc_t *pool, void *ptr, mln_size_t size);
```

描述：从内存池`pool`中分配一个`size`大小的内存，并将`ptr`指向的内存中的数据拷贝到新的内存中。

`ptr`必须为内存池分配的内存起始地址。若`size`为`0`，`ptr`指向的内存会被释放。

返回值：成功则返回内存起始地址，否则返回`NULL`



mln_alloc_free

```c
void mln_alloc_free(void *ptr);
```

描述：释放`ptr`指向的内存。**注意**：`ptr`必须为分配函数返回的地址，而不可以是分配的内存中某一个位置。

返回值：无



mln_alloc_shm_rdlock

```c
int mln_alloc_shm_rdlock(mln_alloc_t *pool);
```

描述：读锁定。本函数会等待直到锁资源可用，并将之锁定。

本函数及后续锁相关函数均用于共享内存内存池。

出于对读多写少的场景考虑，给共享内存配备的是读写锁，而非互斥锁。

返回值：成功返回`0`，否则返回`非0`



mln_alloc_shm_tryrdlock

```c
int mln_alloc_shm_tryrdlock(mln_alloc_t *pool);
```

描述：尝试读锁定。本函数不会挂起等待锁资源可用。

返回值：成功返回`0`，否则返回`非0`



mln_alloc_shm_wrlock

```c
int mln_alloc_shm_wrlock(mln_alloc_t *pool);
```

描述：写锁定。本函数会等待直到锁资源可用，并将之锁定。

返回值：成功返回`0`，否则返回`非0`



mln_alloc_shm_trywrlock

```c
int mln_alloc_shm_trywrlock(mln_alloc_t *pool);
```

描述：尝试写锁定。本函数不会挂起等待锁资源可用。

返回值：成功返回`0`，否则返回`非0`



mln_alloc_shm_unlock

```c
int mln_alloc_shm_unlock(mln_alloc_t *pool);
```

描述：解除锁定。

返回值：成功返回`0`，否则返回`非0`



示例

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
    cattr.worker_process = NULL;
    if (mln_core_init(&cattr) < 0) {
        fprintf(stderr, "init failed\n");
        return -1;
    }

    pool = mln_alloc_init();
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

