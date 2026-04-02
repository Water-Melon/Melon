## 正则表达式



### 头文件

```c
#include "mln_regexp.h"
```



### 模块名

`regexp`



### 函数/宏

#### mln_reg_match

```c
int mln_reg_match(mln_string_t *exp, mln_string_t *text, mln_reg_match_result_t *matches);
```

描述：在`text`中使用正则表达式`exp`进行匹配，并将匹配的字符串结果存放在`matches`中，`matches`由`mln_reg_match_result_new`函数创建。

**注意**：匹配结果`matches`是数组，其中每个元素都是字符串`mln_string_t`类型。字符串的`data`成员指向的内存是直接引用的`text`的内存，因此在使用完匹配结果前，不应释放`text`的内存。

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



#### mln_reg_match_result_new

```c
mln_reg_match_result_new(prealloc)；
```

描述：创建匹配结果数组。`prealloc`为数组预分配元素个数。

返回值：

- 成功 - `mln_reg_match_result_t`指针
- 失败 - `NULL`



#### mln_reg_match_result_free

```c
mln_reg_match_result_free(res);
```

描述：释放`mln_reg_match`返回的匹配结果。

返回值：无



#### mln_reg_match_result_get

```c
mln_reg_match_result_get(res)
```

描述：获取匹配结果数组的起始地址。

返回值：`mln_string_t`指针



### 支持的语法

正则表达式引擎支持以下语法元素：

| 语法 | 描述 |
|------|------|
| `.` | 匹配任意单个字符 |
| `*` | 匹配前一个元素零次或多次 |
| `+` | 匹配前一个元素一次或多次 |
| `?` | 匹配前一个元素零次或一次 |
| `*?` | 惰性星号：尽可能少地匹配 |
| `+?` | 惰性加号：尽可能少地匹配（至少一次） |
| `??` | 惰性问号：优先匹配零次 |
| `{n,m}` | 匹配前一个元素 n 到 m 次 |
| `^` | 锚定：匹配文本开头 |
| `$` | 锚定：匹配文本结尾 |
| `[abc]` | 字符类：匹配列出的任意字符 |
| `[a-z]` | 字符类中的字符范围 |
| `[^abc]` | 否定字符类 |
| `(...)` | 捕获组 |
| `(?:...)` | 非捕获组 |
| `a\|b` | 交替（仅限单字符原子；`cat\|dog` 等同于 `ca(t\|d)og`） |
| `\d` / `\D` | 数字 / 非数字 |
| `\w` / `\W` | 单词字符（`[a-zA-Z0-9_]`）/ 非单词字符 |
| `\s` / `\S` | 空白字符 / 非空白字符 |
| `\b` / `\B` | 单词边界 / 非单词边界 |
| `\n` | 换行符 |
| `\t` | 制表符 |
| `\xHH` | 十六进制转义（例如 `\x41` 匹配 `A`） |
| `(?i)` | 不区分大小写匹配（放置在模式开头） |
| `\\` | 转义的反斜杠字面量 |

**关于交替的说明**：`|` 运算符仅交替相邻的单个原子，而非完整的子表达式。如需多字符交替，请使用分组：不支持 `(cat)|(dog)` 的形式，请改用 `(cat|dog)` 风格的分组。



### 示例

```c
#include <stdio.h>
#include "mln_regexp.h"

int main(int argc, char *argv[])
{
    mln_reg_match_result_t *res = NULL;
    mln_string_t text = mln_string("dabcde");
    mln_string_t exp = mln_string("a.c.*e");
    mln_string_t *s;
    int i, n;

    if ((res = mln_reg_match_result_new(1)) == NULL) {
        fprintf(stderr, "new match result failed.\n");
        return -1;
    }

    n = mln_reg_match(&exp, &text, res);
    printf("matched: %d\n", n);

    s = mln_reg_match_result_get(res);
    for (i = 0; i < n; ++i) {
        write(STDOUT_FILENO, s[i].data, s[i].len);
        write(STDOUT_FILENO, "\n", 1);
    }

    mln_reg_match_result_free(res);

    n = mln_reg_equal(&exp, &text);
    printf("equal returned %d\n", n);

    return 0;
}
```

