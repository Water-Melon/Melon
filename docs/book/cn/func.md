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
void mln_func_entry_callback_set(mln_func_cb_t cb);

typedef void (*mln_func_cb_t)(const char *file, const char *func, int line);
```

描述：设置函数入口的回调函数。`mln_func_cb_t`的参数含义：

- `file` 函数所在文件
- `func` 函数名
- `line` 所在文件行数

返回值：无



#### mln_func_entry_callback_get

```c
 mln_func_cb_t mln_func_entry_callback_get(void);
```

描述：获取入口回调函数。

返回值：入口函数指针



#### mln_func_exit_callback_set

```c
void mln_func_exit_callback_set(mln_func_cb_t cb);
```

描述：设置函数出口的回调函数。`mln_func_cb_t`的参数含义：

- `file` 函数所在文件
- `func` 函数名
- `line` 所在文件行数

返回值：无



#### mln_func_exit_callback_get

```c
mln_func_cb_t mln_func_exit_callback_get(void);
```

描述：获取出口回调函数。

返回值：出口函数指针



### MLN_FUNC

```c
MLN_FUNC(ret_type, name, params, args, func_body);
```

描述：定义一个函数。其实现原理是，定义两个函数，一个函数的名字是`name`指定的名字，另一个函数的名字是以`__`开头，后接`name`指定的名字。第一个函数只是一个包装器，而第二个函数是`func_body`的函数。这样就可以在包装器中调用函数并在函数调用前后增加回调函数调用的逻辑了。

这个宏的参数：

- `ret_type` 是函数的返回值类型，且包含了函数作用域关键字在内。
- `name` 是函数的名字。
- `params` 是函数的形参列表，包含了参数名和参数类型，并且以`()`将参数列表扩起。
- `args` 是函数的实参列表，不包含参数的类型，并以`()`将参数列表扩起。这个实参是指包装器在调用真实函数时，传递给真实函数的参数。这个列表中的参数名和顺序，应该与`params`中的一致。
- `func_body` 函数体，使用`{}`扩住。

返回值：无



### 示例

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

本例中使用`MLN_FUNC`宏定义了两个函数分别为`abc`和`bcd`。在`bcd`中会调用`abc`。

运行这个程序，会看到如下输出：

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

正如`MLN_FUNC`中说明的，使用这个宏定义函数时，实际的函数会被添加`__`前缀，而原本给出的名字则是一个包装器。
