## 双向链表

Melon中有两个双向链表实现，读者可自行选用所需的一种。下面，我们分别进行介绍。



## 第一种实现


### 头文件

```c
#include "mln_utils.h"
```



### 模块名

`utils`



### 函数/宏



#### MLN_CHAIN_FUNC_DECLARE

```c
 MLN_CHAIN_FUNC_DECLARE(scope,prefix,type,func_attr);
```

描述：本宏用于对双向链表的添加操作和删除操作函数进行声明，其中：

- `scope`: 为函数的作用域关键字。
- `prefix`：为两个函数名的前缀，这是为了允许在一个源文件内为多个双向链表进行函数声明。
- `type`：链表节点的类型
- `func_attr`：对函数参数的约束（仅限于Linux中），若无则留空即可



#### MLN_CHAIN_FUNC_DEFINE

```c
MLN_CHAIN_FUNC_DEFINE(scope,prefix,type,prev_ptr,next_ptr);

scope void prefix##_chain_add(type **head, type **tail, type *node);
scope void prefix##_chain_del(type **head, type **tail, type *node);
```

描述：本宏用于定义双向链表的添加和删除操作函数，其中：

- `scope`: 为函数的作用域关键字。
- `prefix`：为两个函数名的前缀，这是为了允许在一个源文件内为多个双向链表进行函数声明。
- `type`：链表节点的类型
- `prev_ptr`：链表节点中指向前一节点的指针名
- `next_ptr`：链表节点中指向后一节点的指针名

`chain_add`和`chain_del`分别为添加和删除节点函数，两个函数的参数为：

- `head`：二级指针，用于在操作函数内对头指针自动修改
- `tail`：二级指针，用于在操作函数内对尾指针自动修改
- `node`：被加入的节点指针，其前后指向的指针可能会被修改



### 示例

```c
#include <stdio.h>
#include <stdlib.h>
#include "mln_utils.h"

typedef struct chain_s {
  int             val;
  struct chain_s *prev;
  struct chain_s *next;
} chain_t;

MLN_CHAIN_FUNC_DECLARE(static inline, test, chain_t, );
MLN_CHAIN_FUNC_DEFINE(static inline, test, chain_t, prev, next);

int main(void)
{
  int i;
  chain_t *head = NULL, *tail = NULL, *c;

  for (i = 0; i < 10; ++i) {
    c = (chain_t *)malloc(sizeof(chain_t));
    if (c == NULL) {
        fprintf(stderr, "malloc failed.\n");
        return -1;
    }
    c->val = i;
    c->prev = c->next = NULL;
    test_chain_add(&head, &tail, c);
  }

  for (c = head; c != NULL; c = c->next) {
    printf("%d\n", c->val);
  }
  return 0;
}
```



## 第二种实现

### 头文件

```c
#include "mln_list.h"
```

除了函数和宏以外，我们只需要知道，如果需要使用该双向链表，则需要在自定义的结构体中加入一个`mln_list_t`的成员（不是指针）。



### 模块名

`list`



### 函数/宏



#### mln_list_add

```c
void mln_list_add(mln_list_t *sentinel, mln_list_t *node);
```

描述：将结点`node`添加到双向链表`sentinel`中。

返回值：无



#### mln_list_remove

```c
void mln_list_remove(mln_list_t *sentinel, mln_list_t *node);
```

描述：将结点`node`从双向链表`sentinel`中移除。

返回值：无



### mln_list_head

```c
mln_list_head(sentinel)
```

描述：获得双向链表的首结点指针。

返回值：首结点指针，若无则为`NULL`



#### mln_list_tail

```c
 mln_list_tail(sentinel)
```

描述：获得双向链表的尾结点指针。

返回值：首结点指针，若无则为`NULL`



#### mln_list_next

```c
mln_list_next(node)
```

描述：获得当前结点`node`的下一个结点指针。

返回值：下一个结点的指针，若无则为`NULL`



#### mln_list_prev

```c
mln_list_prev(node)
```

描述：获得当前结点`node`的前一个结点指针。

返回值：前一个结点的指针，若无则为`NULL`



#### mln_list_null

```c
mln_list_null()
```

描述：用于对队列初始化。

返回值：无



### 示例

```c
#include "mln_list.h"
#include "mln_utils.h"
#include <stdlib.h>

typedef struct {
    int        val;
    mln_list_t node;
} test_t;

int main(void)
{
    int i;
    test_t *t;
    mln_list_t sentinel = mln_list_null();

    for (i = 0; i < 3; ++i) {
        t = (test_t *)calloc(1, sizeof(*t));
        if (t == NULL)
            return -1;
        mln_list_add(&sentinel, &t->node);
        t->val = i;
    }
    for (t = mln_container_of(mln_list_head(&sentinel), test_t, node); \
         t != NULL; \
         t = mln_container_of(mln_list_next(&t->node), test_t, node))
    {
        printf("%d\n", t->val);
    }
    return 0;
}
```

这里使用了`mln_utils.h`中定义的`mln_container_of`来获取链结点所属的自定义结构体`test_t`的指针。

