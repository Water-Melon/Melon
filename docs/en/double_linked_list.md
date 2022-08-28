## Doubly linked list



### Header file

```c
#include "mln_defs.h"
```



### Functions/Macros



#### MLN_CHAIN_FUNC_DECLARE

```c
 MLN_CHAIN_FUNC_DECLARE(prefix,type,ret_attr,func_attr);
```

Description: This macro is used to declare the insert operation and remove operation functions of the doubly linked list, among which:

- `prefix`: The prefix of two function names, this is to allow function declarations for multiple doubly linked lists within a source file.
- `type`: the type of the linked list node
- `ret_attr`: the operation field type and return value type of the two functions
- `func_attr`: Constraints on function parameters (only in Linux), if not, leave it blank



#### MLN_CHAIN_FUNC_DEFINE

```c
MLN_CHAIN_FUNC_DEFINE(prefix,type,ret_attr,prev_ptr,next_ptr);

ret_attr prefix##_chain_add(type **head, type **tail, type *node);
ret_attr prefix##_chain_del(type **head, type **tail, type *node);
```

Description: This macro is used to define the insert and remove operation functions of the doubly linked list, where:

- `prefix`: The prefix of two function names, this is to allow function declarations for multiple doubly linked lists within a source file.
- `type`: the type of the linked list node
- `ret_attr`: the operation field type and return value type of the two functions
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

