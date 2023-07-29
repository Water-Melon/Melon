## 正则表达式



### 头文件

```c
#include "mln_regexp.h"
```



### 相关结构

```c
typedef struct mln_reg_match_s {
    mln_string_t            data;//匹配的字符串
    struct mln_reg_match_s *prev;//上一个匹配的内容
    struct mln_reg_match_s *next;//下一个匹配的内容
} mln_reg_match_t;
```



### 函数



#### mln_reg_match

```c
int mln_reg_match(mln_string_t *exp, mln_string_t *text, mln_reg_match_t **head, mln_reg_match_t **tail);
```

描述：在`text`中使用正则表达式`exp`进行匹配，并将匹配的字符串结果存放在`head`和`tail`指带的双向链表中。

**注意**：匹配结果`mln_reg_match_t`中`data`成员指向的内存是直接引用的`text`的内存，因此在使用完匹配结果前，不应释放`text`的内存。

返回值：

- `0` 完全匹配
- `>0` 匹配的结果数量
- `<0` 执行出错失败，可能为内存不足



#### mln_reg_equal

```c
int mln_reg_equal(mln_string_t *exp, mln_string_t *text);
```

描述：判断`text`是否完全匹配正则表达式`exp`。

返回值：完全匹配则返回`非0`，否则返回`0`



#### mln_reg_match_result_free

```c
void mln_reg_match_result_free(mln_reg_match_t *results);
```

描述：释放`mln_reg_match`返回的匹配结果结构内存。

返回值：无



### 示例

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

