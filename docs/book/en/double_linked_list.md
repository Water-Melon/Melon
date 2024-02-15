## Doubly linked list

There are two doubly linked list implementations in Melon. We introduce them respectively.



## The first one implementation

### Header file

```c
#include "mln_utils.h"
```



### Module

`utils`



### Functions/Macros



#### MLN_CHAIN_FUNC_DECLARE

```c
 MLN_CHAIN_FUNC_DECLARE(scope,prefix,type,func_attr);
```

Description: This macro is used to declare the insert operation and remove operation functions of the doubly linked list, among which:

- `scope`: The scope keyword for functions.
- `prefix`: The prefix of two function names, this is to allow function declarations for multiple doubly linked lists within a source file.
- `type`: the type of the linked list node
- `func_attr`: Constraints on function parameters (only in Linux), if not, leave it blank



#### MLN_CHAIN_FUNC_DEFINE

```c
MLN_CHAIN_FUNC_DEFINE(scope,prefix,type,prev_ptr,next_ptr);

scope void prefix##_chain_add(type **head, type **tail, type *node);
scope void prefix##_chain_del(type **head, type **tail, type *node);
```

Description: This macro is used to define the insert and remove operation functions of the doubly linked list, where:

- `scope`: The scope keyword for functions.
- `prefix`: The prefix of two function names, this is to allow function declarations for multiple doubly linked lists within a source file.
- `type`: the type of the linked list node
- `prev_ptr`: the name of the pointer to the previous node in the linked list node
- `next_ptr`: the name of the pointer to the next node in the linked list node

`chain_add` and `chain_del` are functions for adding and deleting nodes, respectively. The parameters of the two functions are:

- `head`: secondary pointer, used to automatically modify the head pointer within the operation function
- `tail`: secondary pointer, used to automatically modify the tail pointer within the operation function
- `node`: the added node pointer, the pointers before and after it may be modified



### Example

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



## The second one

### Header file

```c
#include "mln_list.h"
```

Besides functions and macros, we only need to know that if we want to use the doubly linked list, we have to add a `mln_list_t` member (not a pointer) to the custom structure.



### Module

`list`



### Functions/Macros



#### mln_list_add

```c
void mln_list_add(mln_list_t *sentinel, mln_list_t *node);
```

Description: Add the node `node` to the doubly linked list `sentinel`.

Return value: None



#### mln_list_remove

```c
void mln_list_remove(mln_list_t *sentinel, mln_list_t *node);
```

Description: Remove the node `node` from the doubly linked list `sentinel`.

Return value: None



### mln_list_head

```c
mln_list_head(sentinel)
```

Description: Get the first node pointer of the doubly linked list.

Return value: the first node pointer, or `NULL` if there is no



#### mln_list_tail

```c
 mln_list_tail(sentinel)
```

Description: Get the tail node pointer of the doubly linked list.

Return value: the first node pointer, or `NULL` if there is no



#### mln_list_next

```c
mln_list_next(node)
```

Description: Get the next node pointer of the current node `node`.

Return value: pointer to the next node, or `NULL` if none



#### mln_list_prev

```c
mln_list_prev(node)
```

Description: Get the previous node pointer of the current node `node`.

Return value: pointer to the previous node, or `NULL` if none



#### mln_list_null

```c
mln_list_null()
```

Description: Used to initialize the queue.

Return value: None



### Example

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

Here, `mln_container_of` defined in `mln_utils.h` is used to obtain the pointer of the custom structure `test_t` to which the link point belongs.
