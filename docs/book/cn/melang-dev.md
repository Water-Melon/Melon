## 脚本开发

本篇将介绍如何开发脚本语言动态扩展库，同时也会给出开发所需要的各类函数。



### 头文件

```c
#include "mln_lang.h"
```



### 模块名

`lang`



### 开发流程

动态扩展库开发流程如下：

1. 构造一个含有名为`init`函数的动态库，其中`init`的函数原型为：

   ```c
   mln_lang_var_t *init(mln_lang_ctx_t *);
   ```

   `mln_lang_var_t`是脚本任务中函数返回值的类型，创建的方式可以参考`mln_lang.h`以及melang的各种内置函数实现。

   开发者可在`init`函数中加载各种脚本函数、集合、变量等内容。如果函数返回值为`nil`，则这与内置库的步骤4是类似的。

   `init`中的`mln_lang_var_t`返回值是在函数`import`调用结束时返回的。

2. 使用时，在脚本中调用`import`函数引入此动态库，程序会调用`init`函数来将库内要加载的资源加载到脚本当前的作用域内。


### 函数/宏

开发者不仅可以使用这里给出的函数，也可以使用上一篇介绍的函数。例如Melang中`eval`函数的实现。



### 相关结构

```c
struct mln_lang_var_s {
    mln_lang_ctx_t                  *ctx;//脚本任务结构
    mln_lang_var_type_t              type;//是否为引用类型
    mln_string_t                    *name;//变量名，匿名变量会置NULL
    mln_lang_val_t                  *val;//具体值
    mln_lang_set_detail_t           *in_set;//是否属于某个Set结构（即类定义）
    mln_lang_var_t                  *prev;
    mln_lang_var_t                  *next;
    mln_lang_var_t                  *prev;
    mln_lang_var_t                  *next;
    mln_uauto_t                      ref;//被引用的次数
};

struct mln_lang_val_s {
    mln_lang_ctx_t                  *ctx;//脚本任务结构
    struct mln_lang_val_s           *prev;
    struct mln_lang_val_s           *next;
    union {
        mln_s64_t                i;
        mln_u8_t                 b;
        double                   f;
        mln_string_t            *s;
        mln_lang_object_t       *obj;
        mln_lang_func_detail_t  *func;
        mln_lang_array_t        *array;
        mln_lang_funccall_val_t *call;
    } data; //具体数据
    mln_s32_t                        type;//数据类型
    mln_u32_t                        ref;//值结构被引用次数
    mln_lang_val_t                  *udata;//用于响应式编程
    mln_lang_func_detail_t          *func;//用于响应式编程
    mln_u32_t                        not_modify:1;//本值结构是否允许被修改
};

struct mln_lang_symbol_node_s {
    mln_string_t                    *symbol;//符号字符串
    mln_lang_ctx_t                  *ctx;//脚本任务结构
    mln_lang_symbol_type_t           type;//符号类型 类定义（M_LANG_SYMBOL_SET）还是变量（M_LANG_SYMBOL_VAR）
    union {
        mln_lang_var_t          *var;
        mln_lang_set_detail_t   *set;
    } data;
    mln_uauto_t                      layer;//所属作用域层数
    mln_lang_hash_bucket_t          *bucket;//所属符号表桶结构
    struct mln_lang_symbol_node_s   *prev;
    struct mln_lang_symbol_node_s   *next;
    struct mln_lang_symbol_node_s   *scope_prev;
    struct mln_lang_symbol_node_s   *scope_next;
};
```

在脚本中，这前两个结构是脚本中最主要的结构。可以看到`mln_lang_var_t`是`mln_lang_val_t`的一层包装。`mln_lang_val_t`用于存放具体数值、字符串等内容。而`mln_lang_var_t`一般会作为函数参数、返回值、数组元素值的封装结构。

之所以封装一层是为了两方面：

1. 函数参数的引用功能
2. 将变量名与变量值进行映射

我们可以将前者称为变量结构，后者称为值结构。

值结构的类型包含如下：

- `M_LANG_VAL_TYPE_NIL` 空值
- `M_LANG_VAL_TYPE_INT` 整数
- `M_LANG_VAL_TYPE_BOOL` 布尔值
- `M_LANG_VAL_TYPE_REAL` 实数
- `M_LANG_VAL_TYPE_STRING` 字符串
- `M_LANG_VAL_TYPE_OBJECT` 对象
- `M_LANG_VAL_TYPE_FUNC` 函数定义
- `M_LANG_VAL_TYPE_ARRAY` 数组
- `M_LANG_VAL_TYPE_CALL` 函数调用



#### mln_lang_var_new

```c
mln_lang_var_t *mln_lang_var_new(mln_lang_ctx_t *ctx, mln_string_t *name, mln_lang_var_type_t type, mln_lang_val_t *val, mln_lang_set_detail_t *in_set);
```

描述：创建变量结构，参数含义依次为：

- `ctx`脚本结构
- `name`变量名，若无则置`NULL`
- `type` 变量类型，引用（`M_LANG_VAR_REFER`）还是复制（`M_LANG_VAR_NORMAL`）。
- `val`对应的值结构。
- `in_set`是否属于某个Set（类定义），若无则置`NULL`。

返回值：成功则返回结构指针，否则返回`NULL`



#### mln_lang_var_free

```c
void mln_lang_var_free(void *data);
```

描述：释放`mln_lang_var_t`类型指针`data`的资源，包括其内部的值结构。

返回值：无



#### mln_lang_var_ref

```c
mln_lang_var_ref(var)
```

描述：创建变量结构的引用。当释放时，若引用大于1，则不会真正释放变量结构。

返回值：`mln_lang_var_t`类型指针



#### mln_lang_var_val_get

```c
mln_lang_var_val_get(var)
```

描述：获取`mln_lang_var_t`类型指针`var`的值结构。

返回值：`mln_lang_val_t`类型指针



#### mln_lang_val_not_modify_set

```c
mln_lang_val_not_modify_set(val)
```

描述：设置`mln_lang_val_t`类型指针`val`为不可更改。

返回值：无



#### mln_lang_val_not_modify_isset

```c
mln_lang_val_not_modify_isset(val)
```

描述：判断`mln_lang_val_t`类型指针`val`是否为不可更改。

返回值：不可更改返回`非0`，否则返回`0`



#### mln_lang_val_not_modify_reset

```c
mln_lang_val_not_modify_reset(val)
```

描述：设置`mln_lang_val_t`类型指针`val`为可更改。

返回值：无



#### mln_lang_var_dup

```c
mln_lang_var_t *mln_lang_var_dup(mln_lang_ctx_t *ctx, mln_lang_var_t *var);
```

描述：复制`var`。对于数组和对象类型的变量，只引用不复制。复制品使用ctx中的内存池结构进行内存分配。

返回值：变量结构指针。



#### mln_lang_var_value_set

```c
int mln_lang_var_value_set(mln_lang_ctx_t *ctx, mln_lang_var_t *dest, mln_lang_var_t *src);
```

描述：将`src`中的值赋给`dest`。若`dest`中原本有值，则会将该值释放。对于数组和对象，仅引用，不复制。对于字符串，本函数完全复制一份。

返回值：成功则返回`0`，否则返回`-1`



#### mln_lang_var_value_set_string_ref

```c
int mln_lang_var_value_set_string_ref(mln_lang_ctx_t *ctx, mln_lang_var_t *dest, mln_lang_var_t *src);
```

描述：将`src`中的值赋给`dest`。若`dest`中原本有值，则会将该值释放。对于数组和对象，仅引用，不复制。对于字符串，本函数仅引用不复制。

返回值：成功则返回`0`，否则返回`-1`



#### mln_lang_var_val_type_get

```c
mln_s32_t mln_lang_var_val_type_get(mln_lang_var_t *var);
```

描述：获取变量的值的类型，类型定义参见相关结构部分。

返回值：类型值



#### mln_lang_ctx_global_var_add

```c
int mln_lang_ctx_global_var_add(mln_lang_ctx_t *ctx, mln_string_t *name, void *val, mln_u32_t type);
```

描述：给脚本任务`ctx`增加一个最外层（也算是全局）变量。变量名为`name`，变量值为`val`，值的类型为`type`。`type`满足相关结构小节处定义的类型。若为字符串类型，则新增变量会使用`mln_string_ref`进行引用，因此要注意`val`为栈中分配时可能造成的问题。

返回值：成功则返回`0`，否则返回`-1`



#### mln_lang_var_create_call

```c
mln_lang_var_t *mln_lang_var_create_call(mln_lang_ctx_t *ctx, mln_lang_funccall_val_t *call);
```

描述：创建一个函数调用类型的变量。此变量仅在脚本核心中被使用，这里仅顺带提一下。

返回值：成功则返回变量类型指针，否则返回`NULL`



#### mln_lang_var_create_nil

```c
mln_lang_var_t *mln_lang_var_create_nil(mln_lang_ctx_t *ctx, mln_string_t *name);
```

描述：创建一个空类型变量。若该变量不需要变量名，则`name`置`NULL`。注意，这里name会被`mln_string_ref`进行引用，因此需要注意`name`为栈中分配的内存时所带来的影响。

返回值：成功则返回变量类型指针，否则返回`NULL`



#### mln_lang_var_create_obj

```c
mln_lang_var_t *mln_lang_var_create_obj(mln_lang_ctx_t *ctx, mln_lang_set_detail_t *in_set, mln_string_t *name);
```

描述：创建一个对象类型变量。若该变量不需要变量名，则`name`置`NULL`。注意，这里name会被`mln_string_ref`进行引用，因此需要注意`name`为栈中分配的内存时所带来的影响。`in_set`为对象所属类（Set），本函数仅对`in_set`做引用，而不会完全复制。

返回值：成功则返回变量类型指针，否则返回`NULL`



#### mln_lang_var_create_true

```c
mln_lang_var_t *mln_lang_var_create_true(mln_lang_ctx_t *ctx, mln_string_t *name);
```

描述：创建一个布尔真变量。若该变量不需要变量名，则`name`置`NULL`。注意，这里name会被`mln_string_ref`进行引用，因此需要注意`name`为栈中分配的内存时所带来的影响。

返回值：成功则返回变量类型指针，否则返回`NULL`



#### mln_lang_var_create_false

```c
mln_lang_var_t *mln_lang_var_create_false(mln_lang_ctx_t *ctx, mln_string_t *name);
```

描述：创建一个布尔假变量。若该变量不需要变量名，则`name`置`NULL`。注意，这里name会被`mln_string_ref`进行引用，因此需要注意`name`为栈中分配的内存时所带来的影响。

返回值：成功则返回变量类型指针，否则返回`NULL`



#### mln_lang_var_create_int

```c
mln_lang_var_t *mln_lang_var_create_int(mln_lang_ctx_t *ctx, mln_s64_t off, mln_string_t *name);
```

描述：创建一个整数类型变量。若该变量不需要变量名，则`name`置`NULL`。注意，这里name会被`mln_string_ref`进行引用，因此需要注意`name`为栈中分配的内存时所带来的影响。

返回值：成功则返回变量类型指针，否则返回`NULL`



#### mln_lang_var_create_real

```c
mln_lang_var_t *mln_lang_var_create_real(mln_lang_ctx_t *ctx, double f, mln_string_t *name);
```

描述：创建一个实数类型变量。若该变量不需要变量名，则`name`置`NULL`。注意，这里name会被`mln_string_ref`进行引用，因此需要注意`name`为栈中分配的内存时所带来的影响。

返回值：成功则返回变量类型指针，否则返回`NULL`



#### mln_lang_var_create_bool

```c
mln_lang_var_t *mln_lang_var_create_bool(mln_lang_ctx_t *ctx, mln_u8_t b, mln_string_t *name);
```

描述：创建一个布尔类型变量。若该变量不需要变量名，则`name`置`NULL`。注意，这里name会被`mln_string_ref`进行引用，因此需要注意`name`为栈中分配的内存时所带来的影响。`b`为`0`时为布尔假，否则为布尔真。

返回值：成功则返回变量类型指针，否则返回`NULL`



#### mln_lang_var_create_string

```c
mln_lang_var_t *mln_lang_var_create_string(mln_lang_ctx_t *ctx, mln_string_t *s, mln_string_t *name);
```

描述：创建一个空类型变量。若该变量不需要变量名，则`name`置`NULL`。注意，这里name会被`mln_string_ref`进行引用，因此需要注意`name`为栈中分配的内存时所带来的影响。本函数会完全复制一份`s`。

返回值：成功则返回变量类型指针，否则返回`NULL`



#### mln_lang_var_create_ref_string

```c
mln_lang_var_t *mln_lang_var_create_ref_string(mln_lang_ctx_t *ctx, mln_string_t *s, mln_string_t *name);
```

描述：创建一个空类型变量。若该变量不需要变量名，则`name`置`NULL`。注意，这里name会被`mln_string_ref`进行引用，因此需要注意`name`为栈中分配的内存时所带来的影响。本函数仅会用`mln_string_ref`引用`s`，因此注意`s`为栈中分配的内存时所带来的影响。

返回值：成功则返回变量类型指针，否则返回`NULL`



#### mln_lang_var_create_array

```c
mln_lang_var_t *mln_lang_var_create_array(mln_lang_ctx_t *ctx, mln_string_t *name);
```

描述：创建一个数组类型变量。若该变量不需要变量名，则`name`置`NULL`。注意，这里name会被`mln_string_ref`进行引用，因此需要注意`name`为栈中分配的内存时所带来的影响。创建完毕后，为空数组。

返回值：成功则返回变量类型指针，否则返回`NULL`



#### mln_lang_condition_is_true

```
int mln_lang_condition_is_true(mln_lang_var_t *var);
```

描述：判断`var`变量的值是否为逻辑真。

返回值：真则返回`1`，否则返回`0`



#### mln_lang_ctx_set_ret_var

```c
void mln_lang_ctx_set_ret_var(mln_lang_ctx_t *ctx, mln_lang_var_t *var);
```

描述：设置当前脚本任务`ctx`的返回值为`var`。此处对`var`为引用而非复制。

返回值：无



#### mln_lang_val_new

```c
mln_lang_val_t *mln_lang_val_new(mln_lang_ctx_t *ctx, mln_s32_t type, void *data);
```

描述：创建值结构。`type`类型参见相关结构小节。`data`根据`type`不同，则其类型也不同，具体类型参见相关结构中结构体中共同体部分的类型。但需要注意，本参数是指针类型，即整型就是`mln_s64_t *`而非`mln_s64_t`类型强制转换。

返回值：成功则返回值结构指针，否则返回`NULL`



#### mln_lang_val_free

```c
void mln_lang_val_free(mln_lang_val_t *val);
```

描述：释放值结构。

返回值：无



#### mln_lang_symbol_node_search

```c
mln_lang_symbol_node_t *mln_lang_symbol_node_search(mln_lang_ctx_t *ctx, mln_string_t *name, int local);
```

描述：在脚本任务`ctx`的符号表中查询名字为`name`的符号。若仅是在当前函数作用域内进行查询，则`local`为`非0`，否则为`0`。

返回值：若存在则返回符号结构指针，否则返回`NULL`



#### mln_lang_symbol_node_join

```c
int mln_lang_symbol_node_join(mln_lang_ctx_t *ctx, mln_lang_symbol_type_t type, void *data);
```

描述：将类型为`type`的`data`加入符号表。`type`值为：

- `M_LANG_SYMBOL_VAR` - 变量
- `M_LANG_SYMBOL_SET` - 类

根据`type`的不同，`data`的类型也不同：

- `mln_lang_var_t *`
- `mln_lang_set_detail_t *`

本函数处理时，`data`为直接引用而非复制。

**注意**：变量必须有变量名。

返回值：成功则返回`0`，否则返回`-1`



#### mln_lang_symbol_node_upper_join

```c
int mln_lang_symbol_node_upper_join(mln_lang_ctx_t *ctx, mln_lang_symbol_type_t type, void *data);
```

描述：将类型为`type`的`data`加入上一层作用域的符号表，若已是最外层，则加入最外层的符号表。`type`值为：

- `M_LANG_SYMBOL_VAR` - 变量
- `M_LANG_SYMBOL_SET` - 类

根据`type`的不同，`data`的类型也不同：

- `mln_lang_var_t *`
- `mln_lang_set_detail_t *`

本函数处理时，`data`为直接引用而非复制。

**注意**：变量必须有变量名。

返回值：成功则返回`0`，否则返回`-1`



#### mln_lang_func_detail_new

```c
mln_lang_func_detail_t *mln_lang_func_detail_new(mln_lang_ctx_t *ctx, mln_lang_func_type_t type, void *data, mln_lang_exp_t *exp, mln_lang_exp_t *closure);
```

描述：创建函数定义。参数含义：

- `ctx`脚本结构
- `type`函数类型：内部函数（`M_FUNC_INTERNAL`） 和 脚本中定义的函数（`M_FUNC_EXTERNAL`）
- `data`函数的具体处理部分。`type`为`M_FUNC_INTERNAL`时，`data`类型为`mln_lang_internal`。否则类型为`mln_lang_stm_t *`，该类型为抽象语法树结构中的语句结构。
- `exp`为参数列表表达式结构。对于使用C自行构造内部函数时，该参数置`NULL`，然后自行构造参数链表。

返回值：成功则返回函数定义结构指针，否则返回`NULL`



#### mln_lang_func_detail_free

```c
void mln_lang_func_detail_free(mln_lang_func_detail_t *lfd);
```

描述：释放函数定义结构。

返回值：无



#### mln_lang_set_detail_new

```c
mln_lang_set_detail_t *mln_lang_set_detail_new(mln_alloc_t *pool, mln_string_t *name);
```

描述：创建类（Set）定义结构。`name`不可为`NULL`，且本函数会对其完全拷贝一份。

返回值：成功则返回类定义结构指针，否则返回`NULL`



#### mln_lang_set_detail_free

```c
void mln_lang_set_detail_free(mln_lang_set_detail_t *c);
```

描述：释放类定义结构。

返回值：无



#### mln_lang_set_detail_self_free

```c
void mln_lang_set_detail_self_free(mln_lang_set_detail_t *c);
```

描述：释放类定义结构。与`mln_lang_set_detail_free`的差别在于，本函数会先释放类定义中的成员红黑树，而`mln_lang_set_detail_free`则是判断引用计数，若引用大于1则不会释放红黑树。

返回值：无



#### mln_lang_set_member_search

```c
mln_lang_var_t *mln_lang_set_member_search(mln_rbtree_t *members, mln_string_t *name);
```

描述：查找成员红黑树中是否存在名为`name`的成员。本函数既可以用于类也可以用于对象的成员查询，因为两个结构都含有成员红黑树结构。

返回值：存在则返回成员的变量结构指针，则否返回`NULL`



#### mln_lang_set_member_add

```c
int mln_lang_set_member_add(mln_alloc_t *pool, mln_rbtree_t *members, mln_lang_var_t *var);
```

描述：向类成员红黑树中增加一个成员`var`。

返回值：成功则返回`0`，否则返回`-1`



#### mln_lang_object_add_member

```c
int mln_lang_object_add_member(mln_lang_ctx_t *ctx, mln_lang_object_t *obj, mln_lang_var_t *var);
```

描述：向对象中增加一个成员`var`。

返回值：成功则返回`0`，否则返回`-1`



#### mln_lang_array_new

```c
 mln_lang_array_t *mln_lang_array_new(mln_lang_ctx_t *ctx);
```

描述：新建一个数组结构。

返回值：成功则返回数组结构指针，否则返回`NULL`



#### mln_lang_array_free

```c
void mln_lang_array_free(mln_lang_array_t *array);
```

描述：释放数组结构，若数组引用计数大于1，则仅递减引用计数。

返回值：无



#### mln_lang_array_get

```c
mln_lang_var_t *mln_lang_array_get(mln_lang_ctx_t *ctx, mln_lang_array_t *array, mln_lang_var_t *key);
```

描述：在数组`array`中获取下表为`key`的元素。若`key`不存在，则新建数组元素。若`key`为`NULL`，则自行追加数组下标。`key`为变量结构，即`key`既可以是整数，也可以是字符串，也可以是对象等等。

返回值：成功则返回新建元素或已存在元素的变量结构指针，否则返回`NULL`



#### mln_lang_array_elem_exist

```c
int mln_lang_array_elem_exist(mln_lang_array_t *array, mln_lang_var_t *key);
```

描述：判断数组`array`中是否存在下标为`key`的元素。

返回值：存在则返回`1`，否则返回`0`



#### mln_lang_ctx_resource_register

```c
int mln_lang_ctx_resource_register(mln_lang_ctx_t *ctx, char *name, void *data, mln_lang_resource_free free_handler);

typedef void (*mln_lang_resource_free)(void *data);
```

描述：向脚本任务`ctx`中注册脚本任务级别的资源及其清理函数。例如消息队列的消息需要在任务级别进行管理，例如当当前任务主动或者被动退出时，需要对该任务所使用到的资源进行统一清理。可参考`Melon/melang/msgqueue`中的使用。`name`为资源名称，`data`为资源指针，`free_handler`为释放函数，释放函数的参数即资源指针`data`。

返回值：成功则返回`0`，否则返回`-1`



#### mln_lang_ctx_resource_fetch

```c
void *mln_lang_ctx_resource_fetch(mln_lang_ctx_t *ctx, const char *name);
```

描述：获取脚本任务`ctx`中注册的名为`name`的资源。

返回值：若存在则返回资源指针，否则返回`NULL`



#### mln_lang_resource_register

```c
int mln_lang_resource_register(mln_lang_t *lang, char *name, void *data, mln_lang_resource_free free_handler);

typedef void (*mln_lang_resource_free)(void *data);
```

描述：向脚本管理结构`lang`中注册名为`name`的资源`data`及其清理函数`free_handler`。这个注册的含义是，该资源将跨脚本任务被使用，例如网络通信时允许将套接字交由一个新协程单独处理。

返回值：成功则返回`0`，否则返回`-1`



#### mln_lang_resource_cancel

```c
void mln_lang_resource_cancel(mln_lang_t *lang, const char *name);
```

描述：将名为`name`的资源从脚本管理结构中注销，并释放资源。

返回值：无



#### mln_lang_resource_fetch

```c
void *mln_lang_resource_fetch(mln_lang_t *lang, const char *name);
```

描述：从脚本管理结构`lang`中获取名为`name`的资源结构。

返回值：若存在则返回资源指针，否则返回`NULL`



#### mln_lang_ctx_suspend

```c
void mln_lang_ctx_suspend(mln_lang_ctx_t *ctx);
```

描述：将脚本任务`ctx`挂起等待。调用该函数前，必须要调用`mln_lang_mutex_lock`该宏上锁。

返回值：无



#### mln_lang_ctx_continue

```c
void mln_lang_ctx_continue(mln_lang_ctx_t *ctx);
```

描述：将脚本任务`ctx`放回执行队列继续执行。调用该函数前，必须要调用`mln_lang_mutex_lock`该宏上锁。

返回值：无



#### mln_lang_func_detail_arg_append

```c
void mln_lang_func_detail_arg_append(mln_lang_func_detail_t *func, mln_lang_var_t *var);
```

描述：将参数`var`挂入到函数定义`func`的参数列表最后方。

返回值：无



### 示例

函数的实现可以参考`Melang/lib_src/aes`中的内容，极为简洁。

类实现的部分可以参考`Melang/lib_src/file`中的内容。
