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
void mln_array_pop(mln_array_t *arr);
```

描述：将数组的最后一个元素移除并释放。若数组为空或`arr`为`NULL`，则不做任何操作。

返回值：无



#### mln_array_grow

```c
int mln_array_grow(mln_array_t *arr, mln_size_t n);
```

描述：确保数组在当前元素数量基础上至少有容纳`n`个额外元素的空间。数组的分配大小会以两倍增长直到满足需求。对于非内存池数组，使用`realloc`进行高效的原地扩容。

返回值：成功则返回`0`，否则返回`-1`



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



#### MLN_ARRAY_PUSH

```c
MLN_ARRAY_PUSH(arr, ret_ptr);
```

描述：`mln_array_push`的内联宏版本。向数组中追加一个元素，并将其地址赋给`ret_ptr`。在快速路径（无需重新分配）上完全避免函数调用开销。若分配失败，`ret_ptr`将被设为`NULL`。

- `arr` 指向`mln_array_t`的指针。
- `ret_ptr` 一个`void *`（或类型化指针）左值，接收元素地址。

返回值：无（结果存储在`ret_ptr`中）



#### MLN_ARRAY_PUSHN

```c
MLN_ARRAY_PUSHN(arr, n, ret_ptr);
```

描述：`mln_array_pushn`的内联宏版本。向数组中追加`n`个元素，并将首个元素的地址赋给`ret_ptr`。若分配失败，`ret_ptr`将被设为`NULL`。

- `arr` 指向`mln_array_t`的指针。
- `n` 要追加的元素数量。
- `ret_ptr` 一个`void *`（或类型化指针）左值，接收起始地址。

返回值：无（结果存储在`ret_ptr`中）



#### MLN_ARRAY_POP

```c
MLN_ARRAY_POP(arr);
```

描述：`mln_array_pop`的内联宏版本。移除最后一个元素并调用释放回调（若已设置）。若数组为空或为`NULL`，则不做任何操作。

- `arr` 指向`mln_array_t`的指针。

返回值：无



### 示例

```c
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "mln_array.h"

static int free_count = 0;

typedef struct {
    int i1;
    int i2;
} test_t;

static void test_free(void *data)
{
    (void)data;
    ++free_count;
}

static void *pool_alloc(void *pool, mln_size_t size)
{
    (void)pool;
    return malloc(size);
}

static void pool_dealloc(void *ptr)
{
    free(ptr);
}

int main(void)
{
    test_t *t;
    mln_size_t i;
    mln_array_t arr;

    /* --- init / push / pushn / elts / nelts / destroy --- */
    assert(mln_array_init(&arr, NULL, sizeof(test_t), 1) == 0);

    t = (test_t *)mln_array_push(&arr);
    assert(t != NULL);
    t->i1 = 0;

    t = (test_t *)mln_array_pushn(&arr, 9);
    assert(t != NULL);
    for (i = 0; i < 9; ++i)
        t[i].i1 = i + 1;

    for (t = (test_t *)mln_array_elts(&arr), i = 0; i < mln_array_nelts(&arr); ++i)
        printf("%d\n", t[i].i1);

    mln_array_destroy(&arr);

    /* --- new / free 及释放回调 --- */
    free_count = 0;
    mln_array_t *a = mln_array_new(test_free, sizeof(test_t), 4);
    assert(a != NULL);
    for (i = 0; i < 3; ++i) {
        t = (test_t *)mln_array_push(a);
        assert(t != NULL);
        t->i1 = (int)i;
    }
    mln_array_free(a);
    assert(free_count == 3);

    /* --- pop --- */
    free_count = 0;
    assert(mln_array_init(&arr, test_free, sizeof(test_t), 4) == 0);
    t = (test_t *)mln_array_push(&arr);
    t->i1 = 1;
    t = (test_t *)mln_array_push(&arr);
    t->i1 = 2;
    mln_array_pop(&arr);
    assert(mln_array_nelts(&arr) == 1 && free_count == 1);
    mln_array_destroy(&arr);

    /* --- reset --- */
    free_count = 0;
    assert(mln_array_init(&arr, test_free, sizeof(test_t), 4) == 0);
    for (i = 0; i < 5; ++i) {
        t = (test_t *)mln_array_push(&arr);
        t->i1 = (int)i;
    }
    mln_array_reset(&arr);
    assert(mln_array_nelts(&arr) == 0 && free_count == 5);
    mln_array_destroy(&arr);

    /* --- grow --- */
    assert(mln_array_init(&arr, NULL, sizeof(test_t), 0) == 0);
    assert(mln_array_grow(&arr, 8) == 0);
    t = (test_t *)mln_array_push(&arr);
    assert(t != NULL);
    mln_array_destroy(&arr);

    /* --- 内联宏：MLN_ARRAY_PUSH / MLN_ARRAY_PUSHN / MLN_ARRAY_POP --- */
    assert(mln_array_init(&arr, NULL, sizeof(test_t), 2) == 0);
    for (i = 0; i < 10; ++i) {
        MLN_ARRAY_PUSH(&arr, t);
        assert(t != NULL);
        t->i1 = (int)i;
    }
    MLN_ARRAY_PUSHN(&arr, 5, t);
    assert(t != NULL);
    assert(mln_array_nelts(&arr) == 15);
    MLN_ARRAY_POP(&arr);
    assert(mln_array_nelts(&arr) == 14);
    mln_array_destroy(&arr);

    /* --- pool_init / pool_new --- */
    int dummy_pool = 0;
    assert(mln_array_pool_init(&arr, NULL, sizeof(test_t), 4,
                               &dummy_pool, pool_alloc, pool_dealloc) == 0);
    t = (test_t *)mln_array_push(&arr);
    assert(t != NULL);
    t->i1 = 77;
    mln_array_destroy(&arr);

    mln_array_t *pa = mln_array_pool_new(NULL, sizeof(test_t), 4,
                                         &dummy_pool, pool_alloc, pool_dealloc);
    assert(pa != NULL);
    t = (test_t *)mln_array_push(pa);
    assert(t != NULL);
    mln_array_free(pa);

    printf("ALL PASSED\n");
    return 0;
}
```

