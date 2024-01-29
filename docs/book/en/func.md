## Function Template



### Header File

```c
#include "mln_func.h"
```

仅在编译时定义了宏`MLN_FUNC_FLAG`时，`MLN_FUNC`宏才会生成特殊的函数代码，否则仅生成常规C函数。



### Module

`func`



### Functions/Macros



#### mln_func_entry_callback_set

```c
void mln_func_entry_callback_set(mln_func_cb_t cb);

typedef void (*mln_func_cb_t)(const char *file, const char *func, int line);
```

Description: Set the callback function of the function entry. Parameter descriptions of `mln_func_cb_t`:

- `file` The file where the function is located
- `func` function name
- `line` is the number of lines in the file

Return value: None



#### mln_func_entry_callback_get

```c
 mln_func_cb_t mln_func_entry_callback_get(void);
```

Description: Get the entry callback function.

Return value: Entry function pointer



#### mln_func_exit_callback_set

```c
void mln_func_exit_callback_set(mln_func_cb_t cb);
```

Description: Set the callback function for function exit. Parameter descriptions of `mln_func_cb_t`:

- `file` The file where the function is located
- `func` function name
- `line` is the number of lines in the file

Return value: None



#### mln_func_exit_callback_get

```c
mln_func_cb_t mln_func_exit_callback_get(void);
```

Description: Get the exit callback function.

Return value: Exit function pointer



#### MLN_FUNC

```c
MLN_FUNC(ret_type, name, params, args, func_body);
```

Description: Define a function with a `non-void` return type. The implementation principle is to define two functions. The name of one function is specified by `name`, and the name of the other function starts with `__`, followed by the name specified by `name`. The first function is just a wrapper, while the second function is the function with `func_body`. In this way, we can call the function in the wrapper and add the logic of callback function calling before and after the function call.

Parameters of this macro:

- `ret_type` is the return value type of the function and includes the function scope keyword.
- `name` is the name of the function.
- `params` is the parameter list of the function, including the parameter name and parameter type, and the parameter list is quoted by `()`.
- `args` is the argument list of the function, does not include the data types, and quoted by `()`. This argument list refers to the arguments passed to the function whose name is started with `__` when the wrapper calls it. The arguments names and order in this list should be identical with those in `params`.
- `func_body` function body, use `{}` to wrapped.

Return value: None



#### MLN_FUNC_VOID

```c
MLN_FUNC_VOID(ret_type, name, params, args, func_body);
```

Description: Define a function with a `void` return type. The implementation principle is to define two functions. The name of one function is specified by `name`, and the name of the other function starts with `__`, followed by the name specified by `name`. The first function is just a wrapper, while the second function is the function with `func_body`. In this way, we can call the function in the wrapper and add the logic of callback function calling before and after the function call.

Parameters of this macro:

- `ret_type` is the return value type of the function and includes the function scope keyword.
- `name` is the name of the function.
- `params` is the parameter list of the function, including the parameter name and parameter type, and the parameter list is quoted by `()`.
- `args` is the argument list of the function, does not include the data types, and quoted by `()`. This argument list refers to the arguments passed to the function whose name is started with `__` when the wrapper calls it. The arguments names and order in this list should be identical with those in `params`.
- `func_body` function body, use `{}` to wrapped.

Return value: None



### Example

```c
#define MLN_FUNC_FLAG

#include "mln_func.h"

MLN_FUNC(int, abc, (int a, int b), (a, b), {
    printf("in %s\n", __FUNCTION__);
    return a + b;
})

MLN_FUNC(static int, bcd, (int a, int b), (a, b), {
    printf("in %s\n", __FUNCTION__);
    return abc(a, b) + abc(a, b);
})

static void my_entry(const char *file, const char *func, int line)
{
    printf("entry %s %s %d\n", file, func, line);
}

static void my_exit(const char *file, const char *func, int line)
{
    printf("exit %s %s %d\n", file, func, line);
}


int main(void)
{
    mln_func_entry_callback_set(my_entry);
    mln_func_exit_callback_set(my_exit);
    printf("%d\n", bcd(1, 2));
    return 0;
}
```

In this example, the `MLN_FUNC` macro is used to define two functions, `abc` and `bcd`. `abc` will be called in `bcd`.

When you run this program, you will see the following output:

```
entry a.c bcd 10
in __bcd
entry a.c abc 5
in __abc
exit a.c abc 5
entry a.c abc 5
in __abc
exit a.c abc 5
exit a.c bcd 10
6
```



### Function time-consuming example

First create two files `span.c` and `span.h`.

#### span.h

```c
#include <sys/time.h>
#include "mln_array.h"

typedef struct mln_span_s {
    struct timeval     begin;
    struct timeval     end;
    const char        *file;
    const char        *func;
    int                line;
    mln_array_t        subspans;
    struct mln_span_s *parent;
} mln_span_t;

extern int mln_span_start(void);
extern void mln_span_stop(void);
extern void mln_span_dump(void);
extern void mln_span_release(void);
```

#### span.c

```c
#include <stdlib.h>
#include <string.h>
#include "span.h"
#include "mln_stack.h"
#define MLN_FUNC_FLAG
#include "mln_func.h"

static mln_stack_t *callstack = NULL;
static mln_span_t *root = NULL;

static void mln_span_entry(const char *file, const char *func, int line);
static void mln_span_exit(const char *file, const char *func, int line);
static mln_span_t *mln_span_new(mln_span_t *parent, const char *file, const char *func, int line);
static void mln_span_free(mln_span_t *s);

static mln_span_t *mln_span_new(mln_span_t *parent, const char *file, const char *func, int line)
{
    mln_span_t *s;
    struct mln_array_attr attr;

    if (parent != NULL) {
        s = (mln_span_t *)mln_array_push(&parent->subspans);
    } else {
        s = (mln_span_t *)malloc(sizeof(mln_span_t));
    }
    if (s == NULL) return NULL;

    memset(&s->begin, 0, sizeof(struct timeval));
    memset(&s->end, 0, sizeof(struct timeval));
    s->file = file;
    s->func = func;
    s->line = line;
    attr.pool = NULL;
    attr.pool_alloc = NULL;
    attr.pool_free = NULL;
    attr.free = (array_free)mln_span_free;
    attr.size = sizeof(mln_span_t);
    attr.nalloc = 7;
    if (mln_array_init(&s->subspans, &attr) < 0) {
        if (parent == NULL) free(s);
        return NULL;
    }
    s->parent = parent;
    return s;
}

static void mln_span_free(mln_span_t *s)
{
    if (s == NULL) return;
    mln_array_destroy(&s->subspans);
    if (s->parent == NULL) free(s);
}

int mln_span_start(void)
{
    struct mln_stack_attr sattr;

    mln_func_entry_callback_set(mln_span_entry);
    mln_func_exit_callback_set(mln_span_exit);

    sattr.free_handler = NULL;
    sattr.copy_handler = NULL;
    if ((callstack = mln_stack_init(&sattr)) == NULL)
        return -1;

    return 0;
}

void mln_span_stop(void)
{
    mln_func_entry_callback_set(NULL);
    mln_func_exit_callback_set(NULL);
    mln_stack_destroy(callstack);
}

void mln_span_release(void)
{
    mln_span_free(root);
}

static void mln_span_format_dump(mln_span_t *span, int blanks)
{
    int i;
    mln_span_t *sub;

    for (i = 0; i < blanks; ++i)
        printf(" ");
    printf("| %s at %s:%d takes %lu (us)\n", \
           span->func, span->file, span->line, \
           (span->end.tv_sec * 1000000 + span->end.tv_usec) - (span->begin.tv_sec * 1000000 + span->begin.tv_usec));

    for (i = 0; i < mln_array_nelts(&(span->subspans)); ++i) {
        sub = ((mln_span_t *)mln_array_elts(&(span->subspans))) + i;
        mln_span_format_dump(sub, blanks + 2);
    }
}

void mln_span_dump(void)
{
    if (root != NULL)
        mln_span_format_dump(root, 0);
}

static void mln_span_entry(const char *file, const char *func, int line)
{
    mln_span_t *span;

    if ((span = mln_span_new(mln_stack_top(callstack), file, func, line)) == NULL) {
        fprintf(stderr, "new span failed\n");
        exit(1);
    }
    if (mln_stack_push(callstack, span) < 0) {
        fprintf(stderr, "push span failed\n");
        exit(1);
    }
    if (root == NULL) root = span;
    gettimeofday(&span->begin, NULL);
}

static void mln_span_exit(const char *file, const char *func, int line)
{
    mln_span_t *span = mln_stack_pop(callstack);
    if (span == NULL) {
        fprintf(stderr, "call stack crashed\n");
        exit(1);
    }
    gettimeofday(&span->end, NULL);
}
```

Next, create a developer-defined program `a.c`:

```c
#include "span.h"
#define MLN_FUNC_FLAG
#include "mln_func.h"

MLN_FUNC(int, abc, (int a, int b), (a, b), {
    return a + b;
})

MLN_FUNC(static int, bcd, (int a, int b), (a, b), {
    return abc(a, b) + abc(a, b);
})

int main(void)
{
    mln_span_start();
    bcd(1, 2);
    mln_span_stop();
    mln_span_dump();
    mln_span_release();
    return 0;
}
```

Compile the program:

```bash
cc -o a a.c span.c -I /usr/local/melon/include/ -L /usr/local/melon/lib/ -lmelon -ggdb
```

Execute it and you can see the following output:

```
| bcd at a.c:13 takes 1 (us)
  | abc at a.c:9 takes 0 (us)
  | abc at a.c:9 takes 0 (us)
```

