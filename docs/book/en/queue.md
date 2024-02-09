## Queue



### Header file

```c
#include "mln_queue.h"
```



### Module

`queue`



### Functions/Macros



#### mln_queue_init

```c
mln_queue_t *mln_queue_init(mln_uauto_t qlen, queue_free free_handler);

typedef void (*queue_free)(void *);
```

Description: Create a queue.

This queue is a fixed-length queue, so `qlen` is the length of the queue. `free_handler` is the release function, which is used to release the data in each member of the queue. If you don't need to release, just set `NULL`.

The parameter of the release function is the data structure pointer of each member of the queue.

Return value: return a queue pointer of type `mln_queue_t` on success, `NULL` on failure



#### mln_queue_destroy

```c
void mln_queue_destroy(mln_queue_t *q);
```

Description: Destroy the queue.

When the queue is destroyed, the data of the queue members will be automatically released according to the setting of `free_handler`.

Return value: none



#### mln_queue_append

```c]
int mln_queue_append(mln_queue_t *q, void *data);
```

Description: Append data `data` to the end of queue `q`.

Return value: `-1` if the queue is full, `0` if successful



#### mln_queue_get

```c
void *mln_queue_get(mln_queue_t *q);
```

Description: Get the data of the leader of the team.

Return value: return the data pointer if successful, or `NULL` if the queue is empty



#### mln_queue_remove

```c
void mln_queue_remove(mln_queue_t *q);
```

Description: Deletes the head element of the queue, but does not release resources.

Return value: none



#### mln_queue_search

```c
void *mln_queue_search(mln_queue_t *q, mln_uauto_t index);
```

Description: Find and return the data of the `index` member starting from the head of the queue `q`, with the subscript starting from 0.

Return value: Returns the data pointer on success, otherwise `NULL`



#### mln_queue_free_index

```c
void mln_queue_free_index(mln_queue_t *q, mln_uauto_t index);
```

Description: Release the member of the specified subscript `index` in the queue `q`, and release its data according to `free_handler`.

Return value: none



#### mln_queue_iterate

```c
int mln_queue_iterate(mln_queue_t *q, queue_iterate_handler handler, void *udata);

typedef int (*queue_iterate_handler)(void *, void *);
```

Description: Iterate over each queue member.

`udata` is a custom structure pointer to assist traversal, and can be set to `NULL` if not required.

The two parameters of `handler` are: `member data`, `udata`.

Return value: `0` is returned when the traversal is completed, and `-1` is returned when it is interrupted



#### mln_queue_empty

```c
mln_queue_empty(q)
```

Description: Determine if queue `q` is an empty queue.

Return value: `non-0` if empty, `0` otherwise



#### mln_queue_full

```c
mln_queue_full(q)
```

Description: Determine if the queue is full.

Return value: `non-0` if full, `0` otherwise



#### mln_queue_length

```c
mln_queue_length(q)
```

Description: Get the total length of queue `q`.

Return value: unsigned integer length value



#### mln_queue_element

```c
mln_queue_element(q)
```

Description: Get the current number of members in queue `q`.

Return value: unsigned integer value



### Example

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

