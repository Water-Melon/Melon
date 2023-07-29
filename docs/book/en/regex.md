## Regular Expression



### Header file

```c
#include "mln_regexp.h"
```



### Structure

```c
typedef struct mln_reg_match_s {
    mln_string_t            data;//匹配的字符串
    struct mln_reg_match_s *prev;//上一个匹配的内容
    struct mln_reg_match_s *next;//下一个匹配的内容
} mln_reg_match_t;
```



### Functions



#### mln_reg_match

```c
int mln_reg_match(mln_string_t *exp, mln_string_t *text, mln_reg_match_t **head, mln_reg_match_t **tail);
```

Description: Use the regular expression `exp` in `text` to match, and store the matched string results in the doubly linked list of `head` and `tail`.

**Note**: The memory pointed to by the `data` member in the match result `mln_reg_match_t` is the memory of the `text` directly referenced, so the memory of `text` should not be released until the match result is used up.

return value:

- `0` is an exact match
- `>0` the number of results matched
- `<0` failed to execute, possibly due to insufficient memory



#### mln_reg_equal

```c
int mln_reg_equal(mln_string_t *exp, mln_string_t *text);
```

Description: Determine if `text` exactly matches the regular expression `exp`.

Return value: return `not 0` for exact match, otherwise return `0`



#### mln_reg_match_result_free

```c
void mln_reg_match_result_free(mln_reg_match_t *results);
```

Description: Free the memory of the match result structure returned by `mln_reg_match`.

Return value: none



### Example

```c
#include <stdio.h>
#include <stdlib.h>
#include "mln_core.h"
#include "mln_log.h"
#include "mln_regexp.h"

int main(int argc, char *argv[])
{
    mln_reg_match_t *head = NULL, *tail = NULL, *match;
    mln_string_t text = mln_string("Hello world");
    mln_string_t exp = mln_string(".*ello");
    int n;
    struct mln_core_attr cattr;

    cattr.argc = argc;
    cattr.argv = argv;
    cattr.global_init = NULL;
    cattr.main_thread = NULL;
    cattr.master_process = NULL;
    cattr.worker_process = NULL;
    if (mln_core_init(&cattr) < 0) {
        fprintf(stderr, "init failed\n");
        return -1;
    }

    n = mln_reg_match(&exp, &text, &head, &tail);
    mln_log(debug, "matched: %d\n", n);
    for (match = head; match != NULL; match = match->next) {
        mln_log(debug, "[%S]\n", &(match->data));
    }
    mln_reg_match_result_free(head);

    return 0;
}
```

