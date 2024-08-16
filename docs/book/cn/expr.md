## 表达式



这个组件实现了对一种简单的表达式进行解析的功能。表达式所支持的仅有：**变量**、**常量**和**函数**。

写法如下：

```
abc   --这是一个变量
"abc" --这是一个字符串常量
'abc' --这也是字符串常量
1     --整数
1.2   --浮点数
0xa   --十六进制整数
0311  --八进制整数

concat(abc, bcd) --这是一个函数，参数有两个，都是变量
concat(abc, "bcd") --这是一个函数，参数有两个，一个是变量，一个是常量
concat(1, "bcd") --两个参数都是常量
concat("abc", concat(bcd, "efg")) --这个例子展示了函数嵌套调用
concat("abc", concat(bcd, "efg")) aaa concat("bcd", concat(efg, "hij")) --这个例子展示运行多个表达式

-- if else
if a then
  var1
else
  func2()
fi

if a then
  func1()
  if b then
  fi
else
  if c then
  else
  fi
fi

-- loop
loop condition do
  aaa
  func(func2())
  if a then
    bbb
  else
    ccc
  fi
end
```



### 头文件

```c
#include "mln_expr.h"
```



### 模块名

`expr`



### 函数

#### mln_expr_val_new

```c
mln_expr_val_t *mln_expr_val_new(mln_expr_typ_t type, void *data, mln_expr_udata_free free);

typedef void (*mln_expr_udata_free)(void *);

typedef enum {
    mln_expr_type_null = 0,
    mln_expr_type_bool,
    mln_expr_type_int,
    mln_expr_type_real,
    mln_expr_type_string,
    mln_expr_type_udata,
} mln_expr_typ_t;

typedef struct {
    mln_expr_typ_t       type;
    union {
        mln_u8_t         b;
        mln_s64_t        i;
        double           r;
        mln_string_t    *s;
        void            *u;
    } data;
    mln_expr_udata_free  free;
} mln_expr_val_t;
```

描述：创建一个表达式值对象。`mln_expr_typ_t`是表达式的类型，`mln_expr_val_t`是值对象的结构。函数`mln_expr_val_new`的第二个参数的类型根据第一个参数不同，会有如下几种：

- `mln_u8_t *`
- `mln_s64_t *`
- `double *`
- `mln_string_t *` 字符串会在函数内使用函数`mln_string_ref`引用该字符串。
- `void *` 用户自定义数据。可以使用第三个参数`free`来释放这个用户数据。

返回值：

- 成功：`mln_expr_val_t`指针
- 失败：`NULL`



#### mln_expr_val_free

```c
void mln_expr_val_free(mln_expr_val_t *ev);
```

描述：释放一个表达式值对象。如果值是`string`类型，同时`free`回调也被设置，那么将使用`free`对字符串进行释放，否则使用`mln_string_free`释放字符串。

返回值：无



#### mln_expr_val_copy

```c
void mln_expr_val_copy(mln_expr_val_t *dest, mln_expr_val_t *src);
```

描述：拷贝一个表达式值对象。将`src`的内容拷贝到`dest`中。如果是字符串类型，则会使用函数`mln_string_ref`引用字符串。如果是`udata`类型，则直接复制数据指针，并将`src`的`free`置`NULL`，保证`src`释放时，用户自定义数据不会被释放。

返回值：无



#### mln_expr_val_dup

```c
mln_expr_val_t *mln_expr_val_dup(mln_expr_val_t *val);
```

描述：拷贝一个表达式值对象。与`mln_expr_val_copy`相似，但会分配一块全新的内存。

返回值：

- 成功：`mln_expr_val_t`指针
- 失败：`NULL`



#### mln_expr_run

```c
mln_expr_val_t *mln_expr_run(mln_string_t *exp, mln_expr_cb_t cb, void *data);

typedef mln_expr_val_t *(*mln_expr_cb_t)(mln_string_t *namespace, mln_string_t *name, int is_func, mln_array_t *args, void *data);
```

描述：运行表达式`exp`。表达式中的变量以及函数（和函数的参数）还有用户自定义数据`data`，都会被传递到回调函数`cb`中进行解析。开发者可以根据需求，定制化这些函数和变量的值。

返回值：

- 成功：`mln_expr_val_t`指针，存放运行结果。
- 失败：`NULL`



#### mln_expr_run_file

```c
mln_expr_val_t *mln_expr_run_file(mln_string_t *path, mln_expr_cb_t cb, void *data);

typedef mln_expr_val_t *(*mln_expr_cb_t)(mln_string_t *namespace, mln_string_t *name, int is_func, mln_array_t *args, void *data);
```

描述：运行由`path`指定的文件中的表达式。表达式中的变量以及函数（和函数的参数）还有用户自定义数据`data`，都会被传递到回调函数`cb`中进行解析。开发者可以根据需求，定制化这些函数和变量的值。

返回值：

- 成功：`mln_expr_val_t`指针，存放运行结果。
- 失败：`NULL`



### 示例

```c
#include "mln_expr.h"
#include "mln_log.h"
#include <stdio.h>

static mln_expr_val_t *var_expr_handler(mln_string_t *namespace, mln_string_t *name, int is_func, mln_array_t *args, void *data)
{
    mln_string_t *s;
    mln_expr_val_t *ret;
    mln_string_t anon = mln_string("anonymous namespace");

    if (is_func)
        mln_log(none, "%S %S %d %U %X\n", namespace? namespace: &anon, name, is_func, args->nelts, data);
    else
        mln_log(none, "%S %S %d %X\n", namespace? namespace: &anon, name, is_func, data);
    if ((s = mln_string_dup(name)) == NULL) return NULL;
    ret = mln_expr_val_new(mln_expr_type_string, s, NULL);
    mln_string_free(s);
    return ret;
}

static mln_expr_val_t *func_expr_handler(mln_string_t *namespace, mln_string_t *name, int is_func, mln_array_t *args, void *data)
{
    mln_expr_val_t *v, *p;
    int i;
    mln_string_t *s1 = NULL, *s2, *s3;
    mln_string_t anon = mln_string("anonymous namespace");

    if (is_func) {
        mln_log(none, "%S %S %d %U %X\n", namespace? namespace: &anon, name, is_func, args->nelts, data);
    } else {
        mln_log(none, "%S %S %d %X\n", namespace? namespace: &anon, name, is_func, data);
        return mln_expr_val_new(mln_expr_type_string, name, NULL);
    }

    for (i = 0, v = p = mln_array_elts(args); i < mln_array_nelts(args); v = p + (++i)) {
        if (s1 == NULL) {
            s1 = mln_string_ref(v->data.s);
            continue;
        }
        s2 = v->data.s;
        s3 = mln_string_strcat(s1, s2);
        mln_string_free(s1);
        s1 = s3;
    }

    v = mln_expr_val_new(mln_expr_type_string, s1, NULL);
    mln_string_free(s1);

    return v;
}

int main(void)
{
    mln_string_t var_exp = mln_string(":aaa (a:b:c:bbb)");
    mln_string_t func_exp = mln_string("abc:def:concat('abc', concat(aaa, 'bbb')) ccc concat('eee', concat(bbb, 'fff'))");

    mln_expr_val_t *v;

    v = mln_expr_run(&var_exp, var_expr_handler, NULL);
    if (v == NULL) {
        mln_log(error, "run failed\n");
        return -1;
    }
    mln_log(debug, "%d %S\n", v->type, v->data.s);
    mln_expr_val_free(v);

    v = mln_expr_run(&func_exp, func_expr_handler, NULL);
    if (v == NULL) {
        mln_log(error, "run failed\n");
        return -1;
    }
    mln_log(debug, "%d %S\n", v->type, v->data.s);
    mln_expr_val_free(v);

    return 0;
}
```

执行结果：

```
a:b:c bbb 0 0
anonymous namespace aaa 1 1 0
08/16/2024 14:13:59 UTC DEBUG: a.c:main:64: PID:792687 4 aaa
anonymous namespace aaa 0 0
anonymous namespace concat 1 2 0
abc:def concat 1 2 0
anonymous namespace ccc 0 0
anonymous namespace bbb 0 0
anonymous namespace concat 1 2 0
anonymous namespace concat 1 2 0
08/16/2024 14:13:59 UTC DEBUG: a.c:main:72: PID:792687 4 eeebbbfff
```

