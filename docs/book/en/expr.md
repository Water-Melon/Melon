## Expression



This component implements the functionality to parse a simple expression. The expression supports only **variables**, **constants**, and **functions**.

The syntax is as follows:

```
abc   -- This is a variable
"abc" -- This is a string constant
'abc' -- This is also a string constant
1     -- Integer
1.2   -- Float
0xa   -- Hexadecimal integer
0311  -- Octal integer

concat(abc, bcd) -- This is a function with two parameters, both are variables
concat(abc, "bcd") -- This is a function with two parameters, one is a variable and the other is a constant
concat(1, "bcd") -- Both parameters are constants
concat("abc", concat(bcd, "efg")) -- This example demonstrates nested function calls
concat("abc", concat(bcd, "efg")) aaa concat("bcd", concat(efg, "hij")) -- This example demonstrates running multiple expressions

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



### Header file

```c
#include "mln_expr.h"
```



### Module

`expr`



### Functions

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

Description: Create an expression value object. `mln_expr_typ_t` is the type of the expression, and `mln_expr_val_t` is the structure of the value object. Depending on the first parameter, the second parameter of the function `mln_expr_val_new` can have the following types:

- `mln_u8_t *`
- `mln_s64_t *`
- `double *`
- `mln_string_t *` Strings will be referenced using the `mln_string_ref` within the function.
- `void *` User-defined data. It can be freed by the free function indicated by the third parameter of this function.

Return values:

- On success: Pointer to `mln_expr_val_t`
- On failure: `NULL`



#### mln_expr_val_free

```c
void mln_expr_val_free(mln_expr_val_t *ev);
```

Description: Free an expression value object. If the value is of type `string` and the `free` callback is also set, then the string will be freed using `free`; otherwise, `mln_string_free` will be used for freeing.

Return value: None



#### mln_expr_val_copy

```c
void mln_expr_val_copy(mln_expr_val_t *dest, mln_expr_val_t *src);
```

Description: duplicate an expression value object. Duplicate the content of `src` to `dest`. If the type is a string, the function `mln_string_ref` will be used to reference the string. If it is of type `udata`, simply copy the data pointer and set `src`'s `free` to `NULL`, ensuring that the user-defined data is not freed when `src` is released.

Return value: None



#### mln_expr_val_dup

```c
void mln_expr_val_dup(mln_expr_val_t *val);
```

Description: Copy an expression value object. Similar to `mln_expr_val_copy`, but allocates a completely new block of memory.

Return values:

- On success: Pointer to `mln_expr_val_t`
- On failure: `NULL`



#### mln_expr_run

```c
mln_expr_val_t *mln_expr_run(mln_string_t *exp, mln_expr_cb_t cb, void *data);

typedef mln_expr_val_t *(*mln_expr_cb_t)(mln_string_t *namespace, mln_string_t *name, int is_func, mln_array_t *args, void *data);
```

Description: Run the expression `exp`. Variables and functions (along with their arguments) in the expression, as well as user-defined data `data`, will be passed to the callback function `cb` for evaluation. Developers can customize the values of these functions and variables as needed.

Return values:

- On success: Pointer to `mln_expr_val_t`, containing the result of the evaluation
- On failure: `NULL`



#### mln_expr_run_file

```c
mln_expr_val_t *mln_expr_run_file(mln_string_t *path, mln_expr_cb_t cb, void *data);

typedef mln_expr_val_t *(*mln_expr_cb_t)(mln_string_t *namespace, mln_string_t *name, int is_func, mln_array_t *args, void *data);
```

Description: Run the expression specified in the file identified by `path`. Variables and functions (along with their arguments) in the expression, as well as user-defined data `data`, will be passed to the callback function `cb` for evaluation. Developers can customize the values of these functions and variables as needed.

Return values:

- On success: Pointer to `mln_expr_val_t`, containing the result of the evaluation
- On failure: `NULL`



### Example

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

Execution result:

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

