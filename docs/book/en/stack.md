## Stack



### Header file

```c
#include "mln_stack.h"
```



### Module

`stack`



### Functions/Macros



#### 	mln_stack_init

```c
mln_stack_t *mln_stack_init(stack_free free_handler, stack_copy copy_handler);

typedef void (*stack_free)(void *);
typedef void *(*stack_copy)(void *, void *);
```

Description:

Initialize the stack structure.

`free_handler`: It is the release function of the data on the stack. Since the data on the stack may be a custom data structure, if you need to release it, you can set it, otherwise set it to `NULL`.

`copy_handler`: Copy stack node data.

The parameter of `stack_free` is the data structure pointer of user-defined data.

The parameters of `stack_copy` are: the data structure pointer of the copied stack node data and the second parameter of the `mln_stack_dup` function (ie user-defined data), this callback function is only called in the `mln_stack_dup` function.

Return value: return stack pointer on success, otherwise `NULL`



#### mln_stack_destroy

```c
void mln_stack_destroy(mln_stack_t *st);
```

Description: Destroy the stack structure and release the data resources in the stack node.

Return value: none



#### mln_stack_push

```c
int mln_stack_push(mln_stack_t *st, void *data);
```

Description: Push data `data` onto stack `st`.

Return value: return `0` on success, otherwise return `-1`



#### mln_stack_pop

```c
void *mln_stack_pop(mln_stack_t *st);
```

Description: Pop the top element data of stack `st`.

Return value: `NULL` if there is no element in the stack, otherwise it is the data pointer in the stack node



#### mln_stack_empty

```c
mln_stack_empty(s)
```

Description: Check if the stack is empty.

Return value: null is `non-0`, otherwise `0`



#### mln_stack_top

```c
mln_stack_top(st)
```

Description: Get the top element data of the stack.

Return value: Returns `NULL` if stack `st` is empty, otherwise it is the data pointer in the top node of the stack



#### mln_stack_dup

```c
mln_stack_t *mln_stack_dup(mln_stack_t *st, void *udata);
```

Description: Completely duplicate stack `st`. `udata` provides additional data for the user.

Return value: if successful, return the new stack pointer, otherwise return `NULL`



#### mln_stack_iterate

```c
int mln_stack_iterate(mln_stack_t *st, stack_iterate_handler handler, void *data);

typedef int (*stack_iterate_handler)(void *, void *);
```

Description:

Traverse the data of each element in the stack `st` from the top of the stack to the bottom of the stack. `handler` is the data access function, `data` is additional user data when traversing.

`stack_iterate_handler` has two parameters: the data pointer in the stack node and the `data` parameter.

return value:

- `mln_stack_iterate`: return `0` after all traversal, otherwise return `-1`
- `stack_iterate_handler`: If you want to interrupt the traversal, return the value of `less than 0`, otherwise the return value of `greater than or equal to 0`


### Example

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

