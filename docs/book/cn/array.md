## 数组



### 头文件

```c
#include "mln_array.h"
```



### 模块名

`array`



### 函数/宏

#### mln_array_init

```c
int mln_array_init(mln_array_t *arr, array_free free, mln_size_t size, mln_size_t nalloc);

typedef void (*array_free)(void *);
```

描述：对一个给定的数组`arr`进行初始化，参数含义如下：

- `arr` 数组对象
- `free`是用于对数组元素做资源释放的函数指针
- `size`是单个数组元素的字节大小
- `nalloc`是初始数组长度，后续数组扩张则是针对当前数组分配元素的两倍进行扩张

返回值：成功则返回`0`，否则返回`-1`



#### mln_array_pool_init

```c
int mln_array_pool_init(mln_array_t *arr, array_free free, mln_size_t size, mln_size_t nalloc,
                        void *pool, array_pool_alloc_handler pool_alloc, array_pool_free_handler pool_free);

typedef void *(*array_pool_alloc_handler)(void *, mln_size_t);
typedef void (*array_pool_free_handler)(void *);
typedef void (*array_free)(void *);
```

描述：对一个给定的数组`arr`使用内存池内存来初始化，参数含义如下：

- `arr` 数组对象
- `free`是用于对数组元素做资源释放的函数指针
- `size`是单个数组元素的字节大小
- `nalloc`是初始数组长度，后续数组扩张则是针对当前数组分配元素的两倍进行扩张
- `pool`是自定义内存池结构指针
- `pool_alloc`是自定义内存池分配函数指针
- `pool_free`是自定义内存池释放函数指针

返回值：成功则返回`0`，否则返回`-1`



#### mln_array_new

```c
mln_array_t *mln_array_new(array_free free, mln_size_t size, mln_size_t nalloc);
```

描述：根据给定参数创建和初始化数组。

返回值：成功则返回数组指针，否则返回`NULL`



#### mln_array_pool_new

```c
mln_array_t *mln_array_pool_new(array_free free, mln_size_t size, mln_size_t nalloc, void *pool,
                                array_pool_alloc_handler pool_alloc, array_pool_free_handler pool_free);
```

描述：根据给定参数使用内存池内存来创建和初始化数组。

返回值：成功则返回数组指针，否则返回`NULL`



#### mln_array_destroy

```c
void mln_array_destroy(mln_array_t *arr);
```

描述：释放指定数组`arr`的所有元素内存。

返回值：无



#### mln_array_free

```c
void mln_array_free(mln_array_t *arr);
```

描述：释放指定数组`arr`的所有元素内存，并释放数组本身内存。

返回值：无



#### mln_array_reset

```c
void mln_array_reset(mln_array_t *arr);
```

描述：将`arr`中的元素进行释放，并将数组长度归0，但不释放数组结构。

返回值：无



#### mln_array_push

```c
void *mln_array_push(mln_array_t *arr);
```

描述：向数组中追加一个元素，并将元素内存地址返回。

返回值：成功则返回元素地址，否则返回`NULL`



#### mln_array_pushn

```c
void *mln_array_pushn(mln_array_t *arr, mln_size_t n);
```

描述：向数组中追加`n`个元素，并将这些元素的内存首地址返回。

返回值：成功则返回元素地址，否则返回`NULL`



#### mln_array_pop

```c
void *mln_array_pop(mln_array_t *arr);
```

描述：将数组的最后一个元素移除并释放。

返回值：无



#### mln_array_elts

```c
mln_array_elts(arr);
```

描述：获取数组所有元素的起始地址。

返回值：元素起始地址



#### mln_array_nelts

```c
mln_array_nelts(arr);
```

描述：获取数组的元素个数。

返回值：元素个数



### 示例

```c
#include <stdio.h>
#include "mln_array.h"

typedef struct {
    int i1;
    int i2;
} test_t;

int main(void)
{
    test_t *t;
    mln_size_t i, n;
    mln_array_t arr;

    mln_array_init(&arr, NULL, sizeof(test_t), 1);

    t = mln_array_push(&arr);
    if (t == NULL)
        return -1;
    t->i1 = 0;

    t = mln_array_pushn(&arr, 9);
    for (i = 0; i < 9; ++i) {
        t[i].i1 = i + 1;
    }

    for (t = mln_array_elts(&arr), i = 0; i < mln_array_nelts(&arr); ++i) {
        printf("%d\n", t[i].i1);
    }

    mln_array_destroy(&arr);

    return 0;
}
```

