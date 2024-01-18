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



### MLN_FUNC

```c
MLN_FUNC(ret_type, name, params, args, func_body);
```

Description: Define a function. The implementation principle is to define two functions. The name of one function is specified by `name`, and the name of the other function starts with `__`, followed by the name specified by `name`. The first function is just a wrapper, while the second function is the function with `func_body`. In this way, we can call the function in the wrapper and add the logic of callback function calling before and after the function call.

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
