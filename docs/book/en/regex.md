## Regular Expression



### Header file

```c
#include "mln_regexp.h"
```



### Module

`regexp`



### Functions/Macros

#### mln_reg_match

```c
int mln_reg_match(mln_string_t *exp, mln_string_t *text, mln_reg_match_result_t *matches);
```

Description: match the `text` by expression `exp`, and store the matched string results in `matches`. `matches` is created by the `mln_reg_match_result_new` function.

**Note**: The match result `matches` is an array, where each element is of type string `mln_string_t`. The memory pointed to by the `data` member of the string is the memory of `text` that is directly referenced, so the memory of `text` should not be released before the matching results are used.

Return value:

- `0` an exact match
- `>0` Number of matching results
- `<0` Execution error failed, possibly due to insufficient memory.



#### mln_reg_equal

```c
int mln_reg_equal(mln_string_t *exp, mln_string_t *text);
```

Description: Determine whether `text` completely matches the regular expression `exp`.

Return value: Returns `non-0` if there is a complete match, otherwise returns `0`



#### mln_reg_match_result_new

```c
mln_reg_match_result_new(prealloc)；
```

Description: Create an array of matching results. `prealloc` preallocates the number of elements for the array.

Return value:

- Success - `mln_reg_match_result_t` pointer
- Failure - `NULL`



#### mln_reg_match_result_free

```c
mln_reg_match_result_free(res);
```

Description: Release the match result returned by `mln_reg_match`.

Return value: None



#### mln_reg_match_result_get

```c
mln_reg_match_result_get(res)
```

Description: Get the starting address of the matching result array.

Return value: `mln_string_t` pointer



### Supported Syntax

The regular expression engine supports the following syntax elements:

| Syntax | Description |
|--------|-------------|
| `.` | Match any single character |
| `*` | Match zero or more of the preceding element |
| `+` | Match one or more of the preceding element |
| `?` | Match zero or one of the preceding element |
| `*?` | Lazy star: match as few as possible |
| `+?` | Lazy plus: match as few as possible (at least one) |
| `??` | Lazy question: prefer zero match |
| `{n,m}` | Match between n and m of the preceding element |
| `^` | Anchor: match at start of text |
| `$` | Anchor: match at end of text |
| `[abc]` | Character class: match any of the listed characters |
| `[a-z]` | Character range within a class |
| `[^abc]` | Negated character class |
| `(...)` | Capturing group |
| `(?:...)` | Non-capturing group |
| `a\|b` | Alternation (single-char atoms only; `cat\|dog` means `ca(t\|d)og`) |
| `\d` / `\D` | Digit / non-digit |
| `\w` / `\W` | Word character (`[a-zA-Z0-9_]`) / non-word character |
| `\s` / `\S` | Whitespace / non-whitespace |
| `\b` / `\B` | Word boundary / non-word boundary |
| `\n` | Newline |
| `\t` | Tab |
| `\xHH` | Hex escape (e.g. `\x41` matches `A`) |
| `(?i)` | Case-insensitive matching (placed at start of pattern) |
| `\\` | Escaped literal backslash |

**Note on alternation**: The `|` operator alternates adjacent single atoms, not full sub-expressions. Use groups for multi-character alternation: `(cat)|(dog)` is not supported; use `(cat|dog)` style grouping instead.



### Example

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

