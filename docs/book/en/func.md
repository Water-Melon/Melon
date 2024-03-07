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

typedef void (*mln_func_cb_t)(const char *file, const char *func, int line, ...);
```

Description: Set the callback function of the function entry. Parameter descriptions of `mln_func_cb_t`:

- `file` The file where the function is located
- `func` function name
- `line` is the number of lines in the file
- `...` The variable arguments are the function arguments of the calling function for this callback function. This parameter only works on c99.

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
MLN_FUNC(scope, ret_type, name, params, args, ...);
```

Description: Define a function with a return value type other than `void`. The implementation principle is to define two functions. The name of one function is the name specified by `name`, and the name of the other function starts with `__mln_func_`, followed by the name specified by `name`. The first function is just a wrapper, while the second function is the function corresponding to the function body pointed to by `...`. This allows you to call functions in the wrapper and add callbacks before and after the function call.
The logic of counting calls is gone.

Parameters of this macro:

- `scope` is the scope keyword of the function.
- `ret_type` is the return value type of the function.
- `name` is the name of the function.
- `params` is the formal parameter list of the function, including the parameter name and parameter type, and the parameter list is expanded with `()`.
- `args` is the actual parameter list of the function, does not include the type of the parameter, and expands the parameter list with `()`. This actual parameter refers to the parameter passed to the real function when the wrapper calls the real function. The parameter names and order in this list should be consistent with those in `params`.
- `...` is the function body, use `{}` to expand it.

Return value: None


#### MLN_FUNC_VOID

```c
MLN_FUNC_VOID(scope, ret_type, name, params, args, func_body);
```

Description: Define a function with a return value type of `void`. The implementation principle is to define two functions. The name of one function is the name specified by `name`, and the name of the other function starts with `__mln_func_`, followed by the name specified by `name`. The first function is just a wrapper, while the second function is the function corresponding to the function body referred to by `...`. This allows you to call functions in the wrapper and add callbacks before and after the function call
The logic of counting calls is gone.

Parameters of this macro:

- `scope` is the scope keyword of the function.
- `ret_type` is the return value type of the function.
- `name` is the name of the function.
- `params` is the formal parameter list of the function, including the parameter name and parameter type, and the parameter list is expanded with `()`.
- `args` is the actual parameter list of the function, does not include the type of the parameter, and expands the parameter list with `()`. This actual parameter refers to the parameter passed to the real function when the wrapper calls the real function. The parameter names and order in this list should be consistent with those in `params`.
- `...` is the function body, use `{}` to expand it.

Return value: None



#### MLN_FUNC_CUSTOM

```c
MLN_FUNC_CUSTOM(entry, exit, scope, ret_type, name, params, args, ...);
```

Description: Define a function with a return value type other than `void`. The implementation principle is to define two functions. The name of one function is the name specified by `name`, and the name of the other function starts with `__mln_func_`, followed by the name specified by `name`. The first function is just a wrapper, while the second function is the function corresponding to the function body pointed to by `...`. This allows you to call functions in the wrapper and add callbacks before and after the function call.
The logic of counting calls is gone.

Parameters of this macro:

- `entry` is the callback function called before the defined function is invoked.
- `exit` is the callback function called after the defined function returns.
- `scope` is the scope keyword of the function.
- `ret_type` is the return value type of the function.
- `name` is the name of the function.
- `params` is the formal parameter list of the function, including the parameter name and parameter type, and the parameter list is expanded with `()`.
- `args` is the actual parameter list of the function, does not include the type of the parameter, and expands the parameter list with `()`. This actual parameter refers to the parameter passed to the real function when the wrapper calls the real function. The parameter names and order in this list should be consistent with those in `params`.
- `...` is the function body, use `{}` to expand it.

Return value: None


#### MLN_FUNC_VOID_CUSTOM

```c
MLN_FUNC_VOID_CUSTOM(entry, exit, scope, ret_type, name, params, args, ...);
```

Description: Define a function with a return value type of `void`. The implementation principle is to define two functions. The name of one function is the name specified by `name`, and the name of the other function starts with `__mln_func_`, followed by the name specified by `name`. The first function is just a wrapper, while the second function is the function corresponding to the function body referred to by `...`. This allows you to call functions in the wrapper and add callbacks before and after the function call
The logic of counting calls is gone.

Parameters of this macro:

- `entry` is the callback function called before the defined function is invoked.
- `exit` is the callback function called after the defined function returns.
- `scope` is the scope keyword of the function.
- `ret_type` is the return value type of the function.
- `name` is the name of the function.
- `params` is the formal parameter list of the function, including the parameter name and parameter type, and the parameter list is expanded with `()`.
- `args` is the actual parameter list of the function, does not include the type of the parameter, and expands the parameter list with `()`. This actual parameter refers to the parameter passed to the real function when the wrapper calls the real function. The parameter names and order in this list should be consistent with those in `params`.
- `...` is the function body, use `{}` to expand it.

Return value: None



### Example

```c
#define MLN_FUNC_FLAG

#include "mln_func.h"

MLN_FUNC(, int, abc, (int a, int b), (a, b), {
    printf("in %s\n", __FUNCTION__);
    return a + b;
})

MLN_FUNC(static, int, bcd, (int a, int b), (a, b), {
    printf("in %s\n", __FUNCTION__);
    return abc(a, b) + abc(a, b);
})

static void my_entry(const char *file, const char *func, int line, ...)
{
    printf("entry %s %s %d\n", file, func, line);
}

static void my_exit(const char *file, const char *func, int line, ...)
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

