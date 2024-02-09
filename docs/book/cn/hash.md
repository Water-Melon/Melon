## 哈希表



### 头文件

```c
#include "mln_hash.h"
```



### 模块名

`hash`



### 相关结构

```c
struct mln_hash_s {
    void                    *pool;//内存池，不使用可为NULL
    hash_pool_alloc_handler  pool_alloc;//内存池分配函数
    hash_pool_free_handler   pool_free;//内存池释放函数
    hash_calc_handler        hash;//计算所属桶的钩子函数
    hash_cmp_handler         cmp;//同一桶内链表节点比较函数
    hash_free_handler        key_freer;//key释放函数
    hash_free_handler        val_freer;//value释放函数
    mln_hash_mgr_t          *tbl;//桶
    mln_u64_t                len;//桶长
    mln_u32_t                nr_nodes;//表项数
    mln_u32_t                threshold;//扩张阈值
    mln_u32_t                expandable:1;//是否自动扩张桶
    mln_u32_t                calc_prime:1;//桶长是否自动计算为素数
};
```

这里，我们主要关注`len`，因为在计算所属桶时，通常会对该值取模。



### 函数



#### mln_hash_init

```c
int mln_hash_init(mln_hash_t *h, struct mln_hash_attr *attr);

struct mln_hash_attr {
    void                    *pool; //内存池
    hash_pool_alloc_handler  pool_alloc;//内存池分配函数
    hash_pool_free_handler   pool_free;//内存池释放函数
    hash_calc_handler        hash; //计算所属桶的钩子函数
    hash_cmp_handler         cmp; //同一桶内链表节点比较函数
    hash_free_handler        key_freer; //key释放函数
    hash_free_handler        val_freer; //value释放函数
    mln_u64_t                len_base; //建议桶长
    mln_u32_t                expandable:1; //是否自动扩展桶长
    mln_u32_t                calc_prime:1; //是否计算素数桶长
};

typedef mln_u64_t (*hash_calc_handler)(mln_hash_t *, void *);
typedef int  (*hash_cmp_handler) (mln_hash_t *, void *, void *);
typedef void (*hash_free_handler)(void *);
typedef void *(*hash_pool_alloc_handler)(void *, mln_size_t);
typedef void (*hash_pool_free_handler)(void *);
```

描述：

本函数用于初始化哈希表结构`h`。

参数`attr`的`pool`为可选项，如果希望从内存池分配，则赋值，否则置`NULL`由`malloc`分配。即，池结构由调用方给出。

`pool_alloc`与`pool_free`为池`pool`配套的内存分配与释放的函数指针。

`key_freer`与`val_freer`也为可选参数，如果需要在表项删除时或哈希表销毁时同时释放资源，则进行设置，否则置`NULL`。释放函数参数为`key`或`value`的结构。

`hash`返回值为一64位整型偏移，且该偏移不应大于等于桶长，故应在该函数内进行取模运算。

`cmp`返回值：`0`为不相同，`非0`为相同。

哈希表支持根据元素数量自动扩张桶长，但建议谨慎对待该选项，因为桶长扩张将伴随节点迁移，会产生相应计算和时间开销，因此慎用。

哈希表桶长建议为素数，因为对素数取模会相对均匀的将元素落入不同的桶中，避免部分桶链表过长。

返回值：若成功则返回`0`，否则返回`-1`



#### mln_hash_init_fast

```c
int mln_hash_init_fast(mln_hash_t *h, hash_calc_handler hash, hash_cmp_handler cmp, hash_free_handler key_freer,
                       hash_free_handler val_freer, mln_u64_t base_len, mln_u32_t expandable, mln_u32_t calc_prime);
```

描述：本函数用于初始化哈希表结构`h`。参数含义与`mln_hash_init`的一致，只是放在函数参数中直接传递。

返回值：若成功则返回`0`，否则返回`-1`



#### mln_hash_new

```c
mln_hash_t *mln_hash_new(struct mln_hash_attr *attr);

struct mln_hash_attr {
    void                    *pool; //内存池
    hash_pool_alloc_handler  pool_alloc;//内存池分配函数
    hash_pool_free_handler   pool_free;//内存池释放函数
    hash_calc_handler        hash; //计算所属桶的钩子函数
    hash_cmp_handler         cmp; //同一桶内链表节点比较函数
    hash_free_handler        key_freer; //key释放函数
    hash_free_handler        val_freer; //value释放函数
    mln_u64_t                len_base; //建议桶长
    mln_u32_t                expandable:1; //是否自动扩展桶长
    mln_u32_t                calc_prime:1; //是否计算素数桶长
};

typedef mln_u64_t (*hash_calc_handler)(mln_hash_t *, void *);
typedef int  (*hash_cmp_handler) (mln_hash_t *, void *, void *);
typedef void (*hash_free_handler)(void *);
typedef void *(*hash_pool_alloc_handler)(void *, mln_size_t);
typedef void (*hash_pool_free_handler)(void *);
```

描述：

本函数用于初始化哈希表结构。

参数`attr`的`pool`为可选项，如果希望从内存池分配，则赋值，否则置`NULL`由`malloc`分配。即，池结构由调用方给出。

`pool_alloc`与`pool_free`为池`pool`配套的内存分配与释放的函数指针。

`key_freer`与`val_freer`也为可选参数，如果需要在表项删除时或哈希表销毁时同时释放资源，则进行设置，否则置`NULL`。释放函数参数为`key`或`value`的结构。

`hash`返回值为一64位整型偏移，且该偏移不应大于等于桶长，故应在该函数内进行取模运算。

`cmp`返回值：`0`为不相同，`非0`为相同。

哈希表支持根据元素数量自动扩张桶长，但建议谨慎对待该选项，因为桶长扩张将伴随节点迁移，会产生相应计算和时间开销，因此慎用。

哈希表桶长建议为素数，因为对素数取模会相对均匀的将元素落入不同的桶中，避免部分桶链表过长。

返回值：若成功则返回哈希表结构指针，否则为`NULL`



#### mln_hash_new_fast

```c
mln_hash_t *mln_hash_new_fast(hash_calc_handler hash, hash_cmp_handler cmp, hash_free_handler key_freer, hash_free_handler val_freer,
                              mln_u64_t base_len, mln_u32_t expandable, mln_u32_t calc_prime);
```

描述：本函数用于初始化哈希表结构。参数含义与`mln_hash_new`的一致，只是使用函数参数传递。

返回值：若成功则返回哈希表结构指针，否则为`NULL`



#### mln_hash_destroy

```c
void mln_hash_destroy(mln_hash_t *h, mln_hash_flag_t flg);

typedef enum mln_hash_flag {
    M_HASH_F_NONE,
    M_HASH_F_VAL,
    M_HASH_F_KEY,
    M_HASH_F_KV
} mln_hash_flag_t;
```

描述：

销毁哈希表。

参数`flg`用于指示哈希表释放时是否释放其内`key``value`所占资源，一共分为：

- 不需要释放
- 仅释放value资源
- 仅释放key资源
- 同时释放key与value资源

返回值：无



#### mln_hash_free

```c
void mln_hash_free(mln_hash_t *h, mln_hash_flag_t flg);

typedef enum mln_hash_flag {
    M_HASH_F_NONE,
    M_HASH_F_VAL,
    M_HASH_F_KEY,
    M_HASH_F_KV
} mln_hash_flag_t;
```

描述：

销毁并释放哈希表。

参数`flg`用于指示哈希表释放时是否释放其内`key``value`所占资源，一共分为：

- 不需要释放
- 仅释放value资源
- 仅释放key资源
- 同时释放key与value资源

返回值：无



#### mln_hash_search

```c
void *mln_hash_search(mln_hash_t *h, void *key);
```

描述：根据`key`在哈希表`h`中查找相应的value。

返回值：成功则返回与`key`对应的`value`，否则返回`NULL`



#### mln_hash_search_iterator

```c
void *mln_hash_search_iterator(mln_hash_t *h, void *key, int **ctx);
```

描述：

由于哈希表允许有多个相同`key`的项存在于表内，因此提供了这个查询迭代器，每次只查询一个表项。

`ctx`为一个对指针类型变量取地址的二级指针，用于存放本次查找的位置。

注意，该函数使用有些许限制，在查询时不可删除元素，否则有可能造成内存非法访问。

返回值：与`key`关联的`value`结构



#### mln_hash_update

```c
int mln_hash_update(mln_hash_t *h, void *key, void *val);
```

描述：将`key`与`value`替换哈希表`h`中相同`key`的表项。

需要注意，`key`与`val`为二级指针，故在调用时需要对指针类型的键和值结构再次取地址。

返回值：

成功则返回`0`，否则返回`-1`。且，原键和值会被赋值给`key`和`val`，故此一定注意，这两个参数是二级指针，但声明处给的是`void *`。



#### mln_hash_insert

```c
int mln_hash_insert(mln_hash_t *h, void *key, void *val);
```

描述：将`key`和`val`插入哈希表。

返回值：成功返回`0`，否则返回`-1`



#### mln_hash_remove

```c
void mln_hash_remove(mln_hash_t *h, void *key, mln_hash_flag_t flg);

typedef enum mln_hash_flag {
    M_HASH_F_NONE,
    M_HASH_F_VAL,
    M_HASH_F_KEY,
    M_HASH_F_KV
} mln_hash_flag_t;
```

描述：删除哈希表中与`key`对应的项，`flg`用于指示哈希表释放时是否释放其内`key``value`所占资源，一共分为：

- 不需要释放
- 仅释放value资源
- 仅释放key资源
- 同时释放key与value资源

返回值：无



#### mln_hash_iterate

```c
int mln_hash_iterate(mln_hash_t *h, hash_iterate_handler handler, void *udata);

typedef int (*hash_iterate_handler)(mln_hash_t * /*h*/, void * /*key*/, void * /*val*/, void *);
```

描述：遍历哈希表内所有表项。

`handler`为表项访问函数，其第一个参数为`h`也就是哈希表结构，第二个参数为`key`，第三个参数为`val`，第四个参数为`mln_hash_iterate`的第三个参数`udata`。

返回值：

- `mln_hash_iterate`：成功返回`0`，失败返回`-1`
- `handler`处理函数：正常返回`0`，若想中断遍历则返回`-1`



#### mln_hash_change_value

```c
void *mln_hash_change_value(mln_hash_t *h, void *key, void *new_value);
```

描述：用`new_value`替换原有与`key`对应的value，并将原有value返回。

返回值：若`key`不存在，则返回`NULL`，否则返回对应value。



#### mln_hash_key_exist

```c
int mln_hash_key_exist(mln_hash_t *h, void *key);
```

描述：检测哈希表`h`中是否存在键为`key`的表项。

返回值：存在返回`1`，否则返回`0`



#### mln_hash_reset

```c
void mln_hash_reset(mln_hash_t *h, mln_hash_flag_t flg);

typedef enum mln_hash_flag {
    M_HASH_F_NONE,
    M_HASH_F_VAL,
    M_HASH_F_KEY,
    M_HASH_F_KV
} mln_hash_flag_t;
```

描述：重置哈希表所有表项并释放，释放会根据`flg`进行对应处理。

返回值：无



### 示例

```c
#include <stdio.h>
#include <stdlib.h>
#include "mln_hash.h"

typedef struct mln_hash_test_s {
    int    key;
    int    val;
} mln_hash_test_t;

static mln_u64_t calc_handler(mln_hash_t *h, void *key)
{
    return *((int *)key) % h->len;
}

static int cmp_handler(mln_hash_t *h, void *key1, void *key2)
{
    return !(*((int *)key1) - *((int *)key2));
}

static void free_handler(void *val)
{
    free(val);
}

int main(int argc, char *argv[])
{
    mln_hash_t *h;
    struct mln_hash_attr hattr;
    mln_hash_test_t *item, *ret;

    hattr.pool = NULL;
    hattr.pool_alloc = NULL;
    hattr.pool_free = NULL;
    hattr.hash = calc_handler;
    hattr.cmp = cmp_handler;
    hattr.key_freer = NULL;
    hattr.val_freer = free_handler;
    hattr.len_base = 97;
    hattr.expandable = 0;
    hattr.calc_prime = 0;

    if ((h = mln_hash_new(&hattr)) == NULL) {
        fprintf(stderr, "Hash init failed.\n");
        return -1;
    }

    item = (mln_hash_test_t *)malloc(sizeof(mln_hash_test_t));
    if (item == NULL) {
        fprintf(stderr, "malloc failed.\n");
        return -1;
    }
    item->key = 1;
    item->val = 10;
    if (mln_hash_insert(h, &(item->key), item) < 0) {
        fprintf(stderr, "insert failed.\n");
        return -1;
    }

    ret = mln_hash_search(h, &(item->key));
    printf("%p %p\n", ret, item);

    mln_hash_free(h, M_HASH_F_VAL);

    return 0;
}
```
