## 函数模板



### 头文件

```c
#include "mln_func.h"
```

仅在编译时定义了宏`MLN_FUNC_FLAG`时，`MLN_FUNC`宏才会生成特殊的函数代码，否则仅生成常规C函数。



### 模块名

`func`



### 函数/宏



#### mln_func_entry_callback_set

```c
void mln_func_entry_callback_set(mln_func_entry_cb_t cb);

typedef int (*mln_func_entry_cb_t)(void *fptr, const char *file, const char *func, int line, ...);
```

描述：设置函数入口的回调函数。`mln_func_entry_cb_t`的参数和返回值含义：

- `fptr` 函数指针，也就是`func`对应的函数
- `file` 函数所在文件
- `func` 函数名
- `line` 所在文件行数
- `...` 这里的可变参数为本回调函数的调用方函数的函数参数。这个参数只有在c99下才起作用。
- 返回值：`<0`表示函数调用停止，立即返回，即实际函数逻辑不会被执行。否则将执行实际函数逻辑。

返回值：无



#### mln_func_entry_callback_get

```c
 mln_func_entry_cb_t mln_func_entry_callback_get(void);
```

描述：获取入口回调函数。

返回值：入口函数指针



#### mln_func_exit_callback_set

```c
void mln_func_exit_callback_set(mln_func_exit_cb_t cb);

typedef void (*mln_func_exit_cb_t)(void *fptr, const char *file, const char *func, int line, void *ret, ...);
```

描述：设置函数出口的回调函数。`mln_func_exit_cb_t`的参数含义：

- `fptr` 函数指针，也就是`func`对应的函数
- `file` 函数所在文件
- `func` 函数名
- `line` 所在文件行数
- `ret` 功能函数的返回值的指针，如何使用可以参考后面的示例
- `...` 这里的可变参数为本回调函数的调用方函数的函数参数。这个参数只有在c99下才起作用。

返回值：无



#### mln_func_exit_callback_get

```c
mln_func_exit_cb_t mln_func_exit_callback_get(void);
```

描述：获取出口回调函数。

返回值：出口函数指针



#### MLN_FUNC

```c
MLN_FUNC(scope, ret_type, name, params, args, ...);
```

描述：定义一个返回值类型为非void的函数。其实现原理是，定义两个函数，一个函数的名字是`name`指定的名字，另一个函数的名字是以`__mln_func_`开头，后接`name`指定的名字。第一个函数只是一个包装器，而第二个函数是`...`指代的函数体对应的函数。这样就可以在包装器中调用函数并在函数调用前后增加回调函数调用的逻辑了。如果入口回调函数返回`<0`的值，则`__mln_func_xxx`函数将不会被调用直接返回，此时函数`name`的返回值将是`MLN_FUNC_ERROR`。

这个宏的参数：

- `scope` 是函数的作用域关键字。
- `ret_type` 是函数的返回值类型。
- `name` 是函数的名字。
- `params` 是函数的形参列表，包含了参数名和参数类型，并且以`()`将参数列表扩起。
- `args` 是函数的实参列表，不包含参数的类型，并以`()`将参数列表扩起。这个实参是指包装器在调用真实函数时，传递给真实函数的参数。这个列表中的参数名和顺序，应该与`params`中的一致。
- `...` 是函数体，使用`{}`扩住。

返回值：无



#### MLN_FUNC_VOID

```c
MLN_FUNC_VOID(scope, ret_type, name, params, args, func_body);
```

描述：定义一个返回值类型为void的函数。其实现原理是，定义两个函数，一个函数的名字是`name`指定的名字，另一个函数的名字是以`__mln_func_`开头，后接`name`指定的名字。第一个函数只是一个包装器，而第二个函数是`...`指代的的函数体对应的函数。这样就可以在包装器中调用函数并在函数调用前后增加回调函数调用的逻辑了。如果入口回调函数返回`<0`的值，则`__mln_func_xxx`函数将不会被调用直接返回，此时函数`name`的返回值将是`MLN_FUNC_ERROR`。

这个宏的参数：

- `scope` 是函数的作用域关键字。
- `ret_type` 是函数的返回值类型。
- `name` 是函数的名字。
- `params` 是函数的形参列表，包含了参数名和参数类型，并且以`()`将参数列表扩起。
- `args` 是函数的实参列表，不包含参数的类型，并以`()`将参数列表扩起。这个实参是指包装器在调用真实函数时，传递给真实函数的参数。这个列表中的参数名和顺序，应该与`params`中的一致。
- `...` 是函数体，使用`{}`扩住。

返回值：无



#### MLN_FUNC_CUSTOM

```c
MLN_FUNC_CUSTOM(entry, exit, scope, ret_type, name, params, args, ...);
```

描述：定义一个返回值类型为非void的函数。其实现原理是，定义两个函数，一个函数的名字是`name`指定的名字，另一个函数的名字是以`__mln_func_`开头，后接`name`指定的名字。第一个函数只是一个包装器，而第二个函数是`...`指代的函数体对应的函数。这样就可以在包装器中调用函数并在函数调用前后增加回调函数调用的逻辑了。如果入口回调函数返回`<0`的值，则`__mln_func_xxx`函数将不会被调用直接返回，此时函数`name`的返回值将是`MLN_FUNC_ERROR`。

这个宏的参数：

- `entry` 是函数入口的回调函数。
- `exit` 是函数的出口回调函数。
- `scope` 是函数的作用域关键字。
- `ret_type` 是函数的返回值类型。
- `name` 是函数的名字。
- `params` 是函数的形参列表，包含了参数名和参数类型，并且以`()`将参数列表扩起。
- `args` 是函数的实参列表，不包含参数的类型，并以`()`将参数列表扩起。这个实参是指包装器在调用真实函数时，传递给真实函数的参数。这个列表中的参数名和顺序，应该与`params`中的一致。
- `...` 是函数体，使用`{}`扩住。

返回值：无



#### MLN_FUNC_VOID_CUSTOM

```c
MLN_FUNC_VOID_CUSTOM(entry, exit, scope, ret_type, name, params, args, ...);
```

描述：定义一个返回值类型为void的函数。其实现原理是，定义两个函数，一个函数的名字是`name`指定的名字，另一个函数的名字是以`__mln_func_`开头，后接`name`指定的名字。第一个函数只是一个包装器，而第二个函数是`...`指代的的函数体对应的函数。这样就可以在包装器中调用函数并在函数调用前后增加回调函数调用的逻辑了。如果入口回调函数返回`<0`的值，则`__mln_func_xxx`函数将不会被调用直接返回，此时函数`name`的返回值将是`MLN_FUNC_ERROR`。

这个宏的参数：

- `entry` 是函数入口的回调函数。
- `exit` 是函数的出口回调函数。
- `scope` 是函数的作用域关键字。
- `ret_type` 是函数的返回值类型。
- `name` 是函数的名字。
- `params` 是函数的形参列表，包含了参数名和参数类型，并且以`()`将参数列表扩起。
- `args` 是函数的实参列表，不包含参数的类型，并以`()`将参数列表扩起。这个实参是指包装器在调用真实函数时，传递给真实函数的参数。这个列表中的参数名和顺序，应该与`params`中的一致。
- `...` 是函数体，使用`{}`扩住。

返回值：无



### 示例

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

本例中使用`MLN_FUNC`宏定义了三个函数分别为`foo`, `bar`, `car`。

我们首先看C99下的执行，先对其编译：

```bash
cc -o a a.c -I /usr/local/melon/include/ -L /usr/local/melon/lib/ -lmelon -std=c99 -DMLN_C99 -DMLN_FUNC_FLAG
```

运行这个程序，会看到如下输出：

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

正如`MLN_FUNC`中说明的，使用这个宏定义函数时，实际的函数会被添加`__`前缀，而原本给出的名字则是一个包装器。

接着看下C89下的执行，还是先编译：

```bash
cc -o a a.c -I /usr/local/melon/include/ -L /usr/local/melon/lib/ -lmelon -DMLN_FUNC_FLAG
```

执行后的输出如下：

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

最后看下不定义`MLN_FUNC_FLAG`宏的效果，先编译：

```bash
cc -o a a.c -I /usr/local/melon/include/ -L /usr/local/melon/lib/ -lmelon
```

执行后看到如下输出：

```
in foo: 1
car
bar
```

