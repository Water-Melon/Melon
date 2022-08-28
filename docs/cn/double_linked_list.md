## 双向链表



### 头文件

```c
#include "mln_defs.h"
```



### 函数/宏



#### MLN_CHAIN_FUNC_DECLARE

```c
 MLN_CHAIN_FUNC_DECLARE(prefix,type,ret_attr,func_attr);
```

描述：本宏用于对双向链表的添加操作和删除操作函数进行声明，其中：

- `prefix`：为两个函数名的前缀，这是为了允许在一个源文件内为多个双向链表进行函数声明。
- `type`：链表节点的类型
- `ret_attr`：两个函数的操作域类型和返回值类型
- `func_attr`：对函数参数的约束（仅限于Linux中），若无则留空即可



#### MLN_CHAIN_FUNC_DEFINE

```c
MLN_CHAIN_FUNC_DEFINE(prefix,type,ret_attr,prev_ptr,next_ptr);

ret_attr prefix##_chain_add(type **head, type **tail, type *node);
ret_attr prefix##_chain_del(type **head, type **tail, type *node);
```

描述：本宏用于定义双向链表的添加和删除操作函数，其中：

- `prefix`：为两个函数名的前缀，这是为了允许在一个源文件内为多个双向链表进行函数声明。
- `type`：链表节点的类型
- `ret_attr`：两个函数的操作域类型和返回值类型
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
#include "mln_defs.h"

typedef struct chain_s {
  int             val;
  struct chain_s *prev;
  struct chain_s *next;
} chain_t;

MLN_CHAIN_FUNC_DECLARE(test, chain_t, static inline void, );
MLN_CHAIN_FUNC_DEFINE(test, chain_t, static inline void, prev, next);

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

