## 队列



### 头文件

```c
#include "mln_queue.h"
```



### 模块名

`queue`



### 函数/宏



#### mln_queue_init

```c
mln_queue_t *mln_queue_init(mln_uauto_t qlen, queue_free free_handler);

typedef void (*queue_free)(void *);
```

描述：创建队列。

本队列为固定长度队列，因此`qlen`就是队列的长度。`free_handler`为释放函数，用于释放队列内每个成员中的数据。若不需要释放则置`NULL`即可。

释放函数的参数即为队列每个成员的数据结构指针。

返回值：成功则返回`mln_queue_t`类型的队列指针，失败则返回`NULL`



#### mln_queue_destroy

```c
void mln_queue_destroy(mln_queue_t *q);
```

描述：销毁队列。

队列销毁时，会根据`free_handler`的设置而自动释放队列成员的数据。

返回值：无



#### mln_queue_append

```c]
int mln_queue_append(mln_queue_t *q, void *data);
```

描述：将数据`data`追加进队列`q`的末尾。

返回值：若队列已满则返回`-1`，成功返回`0`



#### mln_queue_get

```c
void *mln_queue_get(mln_queue_t *q);
```

描述：获取队首成员的数据。

返回值：成功则返回数据指针，若队列为空则返回`NULL`



#### mln_queue_remove

```c
void mln_queue_remove(mln_queue_t *q);
```

描述：删除队首元素，但不释放资源。

返回值：无



#### mln_queue_search

```c
void *mln_queue_search(mln_queue_t *q, mln_uauto_t index);
```

描述：查找并返回从队列`q`队首开始的第`index`成员的数据，下标从0开始。

返回值：成功则返回数据指针，否则为`NULL`



#### mln_queue_free_index

```c
void mln_queue_free_index(mln_queue_t *q, mln_uauto_t index);
```

描述：释放队列`q`内指定下标`index`的成员，并根据`free_handler`释放其数据。

返回值：无



#### mln_queue_iterate

```c
int mln_queue_iterate(mln_queue_t *q, queue_iterate_handler handler, void *udata);

typedef int (*queue_iterate_handler)(void *, void *);
```

描述：遍历每一个队列成员。

`udata`为辅助遍历的自定义结构指针，若不需要可置`NULL`。

`handler`的两个参数分别为：`成员数据`，`udata`。

返回值：遍历完成返回`0`，被中断则返回`-1`



#### mln_queue_empty

```c
mln_queue_empty(q)
```

描述：判断队列`q`是否为空队列。

返回值：空则为`非0`，否则为`0`



#### mln_queue_full

```c
mln_queue_full(q)
```

描述：判断队列是否已满。

返回值：满则为`非0`，否则为`0`



#### mln_queue_length

```c
mln_queue_length(q)
```

描述：获取队列`q`的总长度。

返回值：无符号整型长度值



#### mln_queue_element

```c
mln_queue_element(q)
```

描述：获取队列`q`中当前的成员数量。

返回值：无符号整型数量值



### 示例

```c
#include <stdio.h>
#include <stdlib.h>
#include "mln_queue.h"

int main(int argc, char *argv[])
{
    int i = 10;
    mln_queue_t *q;

    q = mln_queue_init(10, NULL);
    if (q == NULL) {
        fprintf(stderr, "queue init failed.\n");
        return -1;
    }
    mln_queue_append(q, &i);
    printf("%d\n", *(int *)mln_queue_get(q));
    mln_queue_destroy(q);

    return 0;
}
```

