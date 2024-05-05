## 栈



### 头文件

```c
#include "mln_stack.h"
```



### 模块名

`stack`



### 函数/宏



#### 	mln_stack_init

```c
mln_stack_t *mln_stack_init(stack_free free_handler, stack_copy copy_handler);

typedef void (*stack_free)(void *);
typedef void *(*stack_copy)(void *, void *);
```

描述：

初始化栈结构。

`free_handler`：是入栈数据的释放函数，由于入栈数据可能为自定义数据结构，因此若需释放，可对此进行设置否则置`NULL`。

`copy_handler`：复制栈节点数据。

`stack_free`的参数为用户自定义数据的数据结构指针。

`stack_copy`的参数分别为：被复制的栈节点数据的数据结构指针 和 `mln_stack_dup`函数的第二个参数（即用户自定义数据），这个回调函数仅在`mln_stack_dup`函数中被调用。

返回值：成功则返回栈指针，否则为`NULL`



#### mln_stack_destroy

```c
void mln_stack_destroy(mln_stack_t *st);
```

描述：销毁栈结构，并释放栈节点内数据资源。

返回值：无



#### mln_stack_push

```c
int mln_stack_push(mln_stack_t *st, void *data);
```

描述：将数据`data`压入栈`st`中。

返回值：成功返回`0`，否则返回`-1`



#### mln_stack_pop

```c
void *mln_stack_pop(mln_stack_t *st);
```

描述：将栈`st`的栈顶元素数据弹出。

返回值：若栈内无元素则为`NULL`，否则为栈节点内的数据指针



#### mln_stack_empty

```c
mln_stack_empty(s)
```

描述：判断栈是否为空。

返回值：空为`非0`，否则为`0`



#### mln_stack_top

```c
mln_stack_top(st)
```

描述：获取栈顶元素数据。

返回值：若栈`st`为空则返回`NULL`，否则为栈顶节点内的数据指针



#### mln_stack_dup

```c
mln_stack_t *mln_stack_dup(mln_stack_t *st, void *udata);
```

描述：完全复制栈`st`。`udata`为用户提供的额外数据。

返回值：若成功则返回新栈指针，否则返回`NULL`



#### mln_stack_iterate

```c
int mln_stack_iterate(mln_stack_t *st, stack_iterate_handler handler, void *data);

typedef int (*stack_iterate_handler)(void *, void *);
```

描述：

从栈顶向栈底遍历栈`st`的每一个栈内元素数据。`handler`为数据访问函数，`data`为遍历时的额外用户数据。

`stack_iterate_handler`有两个参数，分别为：栈节点内数据指针 和 `data`参数。

返回值：

- `mln_stack_iterate`：全部遍历完则返回`0`，否则返回`-1`
- `stack_iterate_handler`：若想中断遍历则返回`小于0`的值，否则返回值`大于等于0`


### 示例

```c
#include "mln_stack.h"
#include <stdlib.h>
#include <assert.h>

typedef struct {
    void *data1;
    void *data2;
} data_t;

static void *copy(data_t *d, void *data)
{
    data_t *dup;
    assert((dup = (data_t *)malloc(sizeof(data_t))) != NULL);
    *dup = *d;
    return dup;
}

int main(void)
{
    int i;
    data_t *d;
    mln_stack_t *st1, *st2;

    assert((st1 = mln_stack_init((stack_free)free, (stack_copy)copy)) != NULL);

    for (i = 0; i < 3; ++i) {
        assert((d = (data_t *)malloc(sizeof(data_t))) != NULL);
        assert(mln_stack_push(st1, d) == 0);
    }

    assert((st2 = mln_stack_dup(st1, NULL)) != NULL);

    assert((d = mln_stack_pop(st1)) != NULL);
    free(d);

    mln_stack_destroy(st1);
    mln_stack_destroy(st2);

    return 0;
}
```

