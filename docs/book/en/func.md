## Function Template



### Header File

```c
#include "mln_func.h"
```

The MLN_FUNC macro will generate special function code only when the macro MLN_FUNC_FLAG is defined at compile time; otherwise, it will generate a regular C function.



### Module

`func`



### Functions/Macros



#### mln_func_entry_callback_set

```c
void mln_func_entry_callback_set(mln_func_entry_cb_t cb);

typedef int (*mln_func_entry_cb_t)(void *fptr, const char *file, const char *func, int line, ...);
```

Description: Set the callback function for the function entry. Description of the parameters and return value of `mln_func_entry_cb_t`:

- `fptr` is a function pointer, which points to the function corresponding to `func`.
- `file`: The file where the function is located.
- `func`: The name of the function.
- `line`: The line number in the file.
- `...`: The variable arguments here are the function arguments of the caller function for this callback function. This parameter only works under C99.
- Return value: `<0` indicates that the function call is stopped and returns immediately, meaning the actual function logic will not be executed. Otherwise, the actual function logic will be executed.

Return value: None



#### mln_func_entry_callback_get

```c
 mln_func_entry_cb_t mln_func_entry_callback_get(void);
```

Description: Get the entry callback function.

Return value: Pointer to the entry function



#### mln_func_exit_callback_set

```c
void mln_func_exit_callback_set(mln_func_exit_cb_t cb);

typedef void (*mln_func_exit_cb_t)(void *fptr, const char *file, const char *func, int line, void *ret, ...);
```

Description: Set the callback function for the function exit. Description of the parameters of `mln_func_exit_cb_t`:

- `fptr` is a function pointer, which points to the function corresponding to `func`.
- `file`: The file where the function is located.
- `func`: The name of the function.
- `line`: The line number in the file.
- `ret`: A pointer to the return value of the function related to the callback. Refer to the example below for usage.
- `...`: The variable arguments here are the function arguments of the caller function for this callback function. This parameter only works under C99.

Return value: None



#### mln_func_exit_callback_get

```c
mln_func_exit_cb_t mln_func_exit_callback_get(void);
```

Description: Get the exit callback function.

Return value: Pointer to the exit function



#### MLN_FUNC

```c
MLN_FUNC(scope, ret_type, name, params, args, ...);
```

Description: Define a function that returns a non-void type. The implementation is to define two functions, one with the name specified by `name`, and the other with a name that starts with `__mln_func_` followed by the name specified by `name`. The first function is just a wrapper, while the second function is the actual function corresponding to the function body represented by `...`. This allows you to call the function in the wrapper and add logic for callback function calls before and after the function call. If the entry callback function returns a value `<0`, the `__mln_func_xxx` function will not be called, and in this case, the return value of the function `name` will be `MLN_FUNC_ERROR`.

The parameters of this macro are:

- `scope`: The scope keyword of the function.
- `ret_type`: The return type of the function.
- `name`: The name of the function.
- `params`: The parameter list of the function, including parameter names and types, enclosed in `()`.
- `args`: The argument list of the function, excluding parameter types, enclosed in `()`. These arguments are the arguments passed to the real function by the wrapper when calling the real function. The names and order of these arguments should be identical with those in `params`.
- `...`: The function body, enclosed in `{}`.

Return value: None




#### MLN_FUNC_VOID

```c
MLN_FUNC_VOID(scope, ret_type, name, params, args, func_body);
```

Description: Define a function that returns void. The implementation is to define two functions, one with the name specified by `name`, and the other with a name that starts with `__mln_func_` followed by the name specified by `name`. The first function is just a wrapper, while the second function is the actual function corresponding to the function body represented by `...`. This allows you to call the function in the wrapper and add logic for callback function calls before and after the function call. If the entry callback function returns a value `<0`, the `__mln_func_xxx` function will not be called, and in this case, the return value of the function `name` will be `MLN_FUNC_ERROR`.

The parameters of this macro are:

- `scope`: The scope keyword of the function.
- `ret_type`: The return type of the function.
- `name`: The name of the function.
- `params`: The parameter list of the function, including parameter names and types, enclosed in `()`.
- `args`: The argument list of the function, excluding parameter types, enclosed in `()`. These arguments are the arguments passed to the real function by the wrapper when calling the real function. The names and order of these arguments should be identical with those in `params`.
- `...`: The function body, enclosed in `{}`.

Return value: None




#### MLN_FUNC_CUSTOM

```c
MLN_FUNC_CUSTOM(entry, exit, scope, ret_type, name, params, args, ...);
```

Description: Define a function that returns a non-void type. The implementation is to define two functions, one with the name specified by `name`, and the other with a name that starts with `__mln_func_` followed by the name specified by `name`. The first function is just a wrapper, while the second function is the actual function corresponding to the function body represented by `...`. This allows you to call the function in the wrapper and add logic for callback function calls before and after the function call. If the entry callback function returns a value `<0`, the `__mln_func_xxx` function will not be called, and in this case, the return value of the function `name` will be `MLN_FUNC_ERROR`.

The parameters of this macro are:

- `entry`: The callback function for the function entry.
- `exit`: The callback function for the function exit.
- `scope`: The scope keyword of the function.
- `ret_type`: The return type of the function.
- `name`: The name of the function.
- `params`: The parameter list of the function, including parameter names and types, enclosed in `()`.
- `args`: The argument list of the function, excluding parameter types, enclosed in `()`. These arguments are the arguments passed to the real function by the wrapper when calling the real function. The names and order of these arguments should be identical with those in `params`.
- `...`: The function body, enclosed in `{}`.

Return value: None



#### MLN_FUNC_VOID_CUSTOM

```c
MLN_FUNC_VOID_CUSTOM(entry, exit, scope, ret_type, name, params, args, ...);
```

Description: Define a function that returns void. The implementation is to define two functions, one with the name specified by `name`, and the other with a name that starts with `__mln_func_` followed by the name specified by `name`. The first function is just a wrapper, while the second function is the actual function corresponding to the function body represented by `...`. This allows you to call the function in the wrapper and add logic for callback function calls before and after the function call. If the entry callback function returns a value `<0`, the `__mln_func_xxx` function will not be called, and in this case, the return value of the function `name` will be `MLN_FUNC_ERROR`.

The parameters of this macro are:

- `entry`: The callback function for the function entry.
- `exit`: The callback function for the function exit.
- `scope`: The scope keyword of the function.
- `ret_type`: The return type of the function.
- `name`: The name of the function.
- `params`: The parameter list of the function, including parameter names and types, enclosed in `()`.
- `args`: The argument list of the function, excluding parameter types, enclosed in `()`. These arguments are the arguments passed to the real function by the wrapper when calling the real function. The names and order of these arguments should be identical with those in `params`.
- `...`: The function body, enclosed in `{}`.

Return value: None




### Example

```c
//a.c

#include "mln_func.h"
#include <stdio.h>
#include <string.h>
#if defined(MLN_C99)
#include <stdarg.h>
#endif

MLN_FUNC_VOID(static, void, foo, (int *a, int b), (a, b), {
    printf("in %s: %d\n", __FUNCTION__, *a);
    *a += b;
})

MLN_FUNC(static, int, car, (int *a, int b), (a, b), {
    printf("%s\n", __FUNCTION__);
    return 100;
})

MLN_FUNC(static, int, bar, (void), (), {
    printf("%s\n", __FUNCTION__);
    return 0;
})

static int my_entry(void *fptr, const char *file, const char *func, int line, ...)
{
    if (!strcmp(func, "bar")) {
        printf("%s won't be executed\n", func);
        return -1;
    }

#if defined(MLN_C99)
    va_list args;
    va_start(args, line);
    int *a = (int *)va_arg(args, int *);
    va_end(args);

    printf("entry %s %s %d %d\n", file, func, line, *a);
    ++(*a);
#else
    printf("entry %s %s %d\n", file, func, line);
#endif

    return 0;
}

static void my_exit(void *fptr, const char *file, const char *func, int line, void *ret, ...)
{
    if (!strcmp(func, "bar"))
        return;

#if defined(MLN_C99)
    va_list args;
    va_start(args, ret);
    int *a = (int *)va_arg(args, int *);
    va_end(args);

    printf("exit %s %s %d %d\n", file, func, line, *a);
#else
    printf("exit %s %s %d\n", file, func, line);
#endif
    if (ret != NULL)
        printf("return value is %d\n", *((int *)ret));
}

int main(void)
{
    int a = 1;

    mln_func_entry_callback_set(my_entry);
    mln_func_exit_callback_set(my_exit);

    foo(&a, 2);
    car(&a, 3);
    return bar();
}
```

In this example, the `MLN_FUNC` is used to define three functions, `foo`, `bar`, and `car`.

Let's first look at the execution under C99, compile at first:

```bash
cc -o a a.c -I /usr/local/melon/include/ -L /usr/local/melon/lib/ -lmelon -std=c99 -DMLN_C99 -DMLN_FUNC_FLAG
```

You will see the following output after running this program:

```
entry a.c foo 8 1
in __mln_func_foo: 2
exit a.c foo 8 4
entry a.c car 13 4
__mln_func_car
exit a.c car 13 5
return value is 100
bar won't be executed
```


As explained in the `MLN_FUNC` macro, when using this macro to define functions, the actual function will be prefixed with `__`, while the original name given will be a wrapper.

Next, let's look at the execution under C89, compile at first:

```bash
cc -o a a.c -I /usr/local/melon/include/ -L /usr/local/melon/lib/ -lmelon -DMLN_FUNC_FLAG
```

You will see the following output after running this program:

```
entry a.c foo 8
in __mln_func_foo: 1
exit a.c foo 8
entry a.c car 13
__mln_func_car
exit a.c car 13
return value is 100
bar won't be executed
```

Finally, let's see the effect of not defining the macro `MLN_FUNC_FLAG`. Compile it at first:

```bash
cc -o a a.c -I /usr/local/melon/include/ -L /usr/local/melon/lib/ -lmelon
```

You will see the following output after running this program:

```
in foo: 1
car
bar
```

