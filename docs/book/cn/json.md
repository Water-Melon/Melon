## JSON




### 头文件

```c
#include "mln_json.h"
```



### 模块名

`json`



### 函数/宏



#### mln_json_init

```c
mln_json_init(j)
```

描述：初始化JSON类型结点`j`，该结点类型为`NONE`.

返回值：无



#### mln_json_string_init

```c
mln_json_string_init(j, s)
```

描述：将JSON类型结点`j`初始化为字符串类型，并赋予`mln_string_t *`类型的值`s`。`s`会被JSON结构接管，调用方需要保证`s`的内存生命周期以及不可修改`s`的内容。

返回值：无



#### mln_json_number_init

```c
mln_json_number_init(j, n)
```

描述：将JSON类型结点`j`初始化为数字类型，并赋予`double`类型的值`n`。

返回值：无



#### mln_json_true_init

```c
mln_json_true_init(j)
```

描述：将JSON类型结点`j`初始化为`true`类型。

返回值：无



#### mln_json_false_init

```c
mln_json_false_init(j)
```

描述：将JSON类型结点`j`初始化为`false`类型。

返回值：无



#### mln_json_null_init

```c
mln_json_null_init(j)
```

描述：将JSON类型结点`j`初始化为`null`类型。

返回值：无



#### mln_json_obj_init

```c
int mln_json_obj_init(mln_json_t *j);
```

描述：将JSON类型结点`j`初始化为对象类型。

返回值：

- `0` - 成功
- `-1` - 失败



#### mln_json_array_init

```c
int mln_json_array_init(mln_json_t *j);
```

描述：将JSON类型结点`j`初始化为数组类型。

返回值：

- `0` - 成功
- `-1` - 失败



#### mln_json_decode

```c
int mln_json_decode(mln_string_t *jstr, mln_json_t *out, mln_json_policy_t *policy);
```

描述：将JSON字符串`jstr`解析成数据结构，结果会被放入参数`out`中。`policy`为安全策略结构，由`mln_json_policy_init`进行初始化。

返回值：

- `0` - 成功
- `-1` - 失败, 如果`policy`不为`NULL`，则需要使用`mln_json_policy_error`检查具体违反了哪个安全限制条件。



#### mln_json_destroy

```c
void mln_json_destroy(mln_json_t *j;
```

描述：释放`mln_json_t`类型的`j`节点内存。

返回值：无



#### mln_json_dump

```c
void mln_json_dump(mln_json_t *j, int n_space, char *prefix);
```

描述：将json节点`j`的详细信息输出到标准输出。`n_space`表示当前缩进空格数，`prefix`为输出内容的前缀。

返回值：无



#### mln_json_encode

```c
mln_string_t *mln_json_encode(mln_json_t *j);
```

描述：由`mln_json_t`节点结构生成JSON字符串。返回值使用后需要调用`mln_string_free`进行释放。

返回值：成功返回`mln_string_t`字符串指针，否则返回`NULL`



#### mln_json_obj_search

```c
mln_json_t *mln_json_obj_search(mln_json_t *j, mln_string_t *key);
```

描述：从对象类型结点`j`中搜索key为`key`的value内容。

返回值：成功则返回`mln_json_t`类型的value，否则返回`NULL`



#### mln_json_array_search

```c
mln_json_t *mln_json_array_search(mln_json_t *j, mln_uauto_t index);
```

描述：从数组类型结点`j`中搜索下标为`index`的元素内容。

返回值：成功则返回`mln_json_t`类型的元素节点，否则返回`NULL`



#### mln_json_array_length

```c
mln_uauto_t mln_json_array_length(mln_json_t *j);
```

描述：获取数组的长度。此时`j`必须为数组类型。

返回值：数组长度



#### mln_json_obj_update

```c
int mln_json_obj_update(mln_json_t *j, mln_json_t *key, mln_json_t *val);
```

描述：将`key`与`val`对添加到`j` JSON节点中。此时，`j`需为对象类型。若`key`已经存在，则将原本`key`和`value`将会被新参数替换。因此参数`key`和`val`在调用后将被`j`接管。

返回值：成功则返回`0`，否则返回`-1`



#### mln_json_array_append

```c
int mln_json_array_append(mln_json_t *j, mln_json_t *value);
```

描述：将`value`加入到数组类型的JSON结构`j`中。

返回值：成功则返回`0`，否则返回`-1`



#### mln_json_array_update

```c
int mln_json_array_update(mln_json_t *j, mln_json_t *value, mln_uauto_t index);
```

描述：将`value`更新到数组类型JSON结构`j`的下标为`index`的位置上。若下标不存在，则会失败。

返回值：成功则返回`0`，否则返回`-1`



#### mln_json_reset

```c
void mln_json_reset(mln_json_t *j);
```

描述：重置JSON节点`j`数据结构，将其原有数据释放。

返回值：无



#### mln_json_obj_remove

```c
void mln_json_obj_remove(mln_json_t *j, mln_string_t *key);
```

描述：将key值为`key`的键值对从对象类型的JSON结构`j`中删除并释放。

返回值：存在则返回对应value部分的JSON节点，否则返回`NULL`



#### mln_json_array_remove

```c
void mln_json_array_remove(mln_json_t *j, mln_uauto_t index);
```

描述：将下标为`index`的元素从数组类型JSON节点上删除并释放。

返回值：存在则返回元素指针，否则返回`NULL`



#### is_type

```c
mln_json_is_object(json)
mln_json_is_array(json)
mln_json_is_string(json)
mln_json_is_number(json)
mln_json_is_true(json)
mln_json_is_false(json)
mln_json_is_null(json)
mln_json_is_none(json)
```

描述：判断`mln_json_t`结构的`json`类型，依次分别为：对象、数组、字符串、数字、布尔真、布尔假、NULL、无类型。

返回值：满足条件返回`非0`，否则返回`0`



#### set_type

```c
mln_json_none_type_set(json)
mln_json_object_type_set(json)
mln_json_array_type_set(json)
mln_json_string_type_set(json)
mln_json_number_type_set(json)
mln_json_true_type_set(json)
mln_json_false_type_set(json)
mln_json_null_type_set(json)
```

描述：给`mln_json_t`类型的`json`节点设置类型，依次分别为：无类型、对象、数组、字符串、数字、布尔真、布尔假、NULL。

返回值：无



#### get_data

```c
mln_json_object_data_get(json)
mln_json_array_data_get(json)
mln_json_string_data_get(json)
mln_json_number_data_get(json)
mln_json_true_data_get(json)
mln_json_false_data_get(json)
mln_json_null_data_get(json)
```

描述：获取`mln_json_t`类型的`json`节点中对应类型的数据部分。类型依次为：对象、数组、字符串、数字、布尔真、布尔假、NULL。

返回值：

- 对象类型为`mln_hash_t`类型指针
- 数组类型为`mln_rbtree_t`类型指针
- 字符串类型为`mln_string_t`类型指针
- 数字类型为`double`类型值
- 布尔真为`mln_u8_t`类型值
- 布尔假为`mln_u8_t`类型值
- NULL类型为`mln_u8ptr_t`类型的NULL值



#### mln_json_policy_init

```c
mln_json_policy_init(policy, _depth, _keylen, _strlen, _elemnum, _kvnum);
```

描述：对`mln_json_policy_t`类型的`policy`进行初始化，剩余参数含义如下：

- `_depth` JSON的最大嵌套层数。解析时，当遇到数组或对象时，嵌套层数会递增。数组或对象解析完成时，嵌套层数会递减。

- `_keylen` 对象中key的最大长度。

- `_strlen` 字符串值的最大长度。

- `_elemnum` 最大数组元素个数。

- `_kvnum` 最大对象key-value对个数。

返回值：无



#### mln_json_policy_error

```c
mln_json_policy_error(policy)
```

描述：从`mln_json_policy_t`类型的`policy`获取错误号。这个宏一般用于`mln_json_decode`返回`-1`时检查是否有违反安全策略的情况。

返回值：`int`型错误号，错误号有如下值：

- `M_JSON_OK` 没有违反安全策略。

- `M_JSON_DEPTH` 嵌套层数过多。

- `M_JSON_KEYLEN` 对象Key的长度超过限制。

- `M_JSON_STRLEN` 字符串值的长度超过限制。

- `M_JSON_ARRELEM` 数组元素个数超过限制。

- `M_JSON_OBJKV` 对象key-value对个数超过限制。



#### mln_json_parse

```c
int mln_json_parse(mln_json_t *j, mln_string_t *exp, mln_json_iterator_t iterator, void *data)

typedef int (*mln_json_iterator_t)(mln_json_t *, void *);
```

描述：

从`j`结点管理的JSON中，根据`exp`获取其中的匹配的`mln_json_t`子结点，如果存在匹配的子结点，则会调用`iterator`回调函数进行处理，`data`为用户自定义数据，在`iterator`调用时被传入。

如果我们有如下JSON

```
{
  "a": [1, {
    "c": 3
  }],
  "b": 2
}
```

`exp`的写法形如：

| 写法    | 含义                                                         |
| ------- | ------------------------------------------------------------ |
| `a`     | 获取key为`a`的值（每个值都保存在`mln_json_t`结构中）         |
| `b`     | 获取key为`b`的值                                             |
| `a.0`   | 获取key为`a`，且其值应为数组，再获取数组的第`0`个下标的元素  |
| `a.2.c` | 获取key为`a`的值对应的数组，再获取其下标为`2`的数组元素，该元素应为一个对象，最后获取该元素对象的key为`c`的值，这个表达式在本例中会导致`mln_json_parse`返回-1，因为下标为`2`的数组元素不存在 |

返回值：

- `0` - 成功
- `-1` - 失败



#### mln_json_generate

```c
int mln_json_generate(mln_json_t *j, char *fmt, ...);
```

描述：

根据`fmt`给出的格式，利用后续参数填充至`j`中。

其中，`fmt`支持如下格式符：

- `{}` - 表示大括号内的是一个对象类型，对象的key与value之间使用`:`分隔，每组key-value之间以`,`分隔
- `[]` - 表示中括号内的是一个数组类型，每个数组元素之间以`,`分隔
- `j` - 表示可变参部分传入的是一个`mln_json_t`指针
- `d` - 表示可变参部分传入的是一个32位有符号值，例如`int`或`mln_s32_t`
- `D` - 表示可变参部分传入的是一个64位有符号值，例如`long long`或`mln_s64_t`
- `u` - 表示可变参部分传入的是一个32位无符号值，例如`unsigned int`或`mln_u32_t`
- `U` - 表示可变参部分传入的是一个64位无符号值，例如`unsigned long long`或`mln_u64_t`
- `F` - 表示可变参部分传入的是一个`double`类型的值
- `t` - 表示这个位置填入一个`true`，这个占位符无需传入对应的可变参数
- `f` - 表示这个位置填入一个`false`，这个占位符无需传入对应的可变参数
- `n` - 表示这个位置填入一个`null`，这个占位符无需传入对应的可变参数
- `s` - 表示可变参部分传入的是一个`char *`类型字符串，例如`"Hello"`
- `S` - 表示可变参部分传入的是一个`mln_string_t *`类型字符串
- 'c' - 表示可变参部分传入的是一个`struct mln_json_call_attr *`类型结构，这个结构包含两个成员`callback`和`data`，含义是：这个位置的值是由函数`callback`对`data`解析而来的

这里有如下注意事项：

1. `fmt`中仅可出现上述格式符号，不可存在任何的空格、制表符或者换行符等其他类型符号
2. 在成功执行本函数后，不可对格式符`j`对应的可变参数进行释放（即`mln_json_destroy`或`mln_json_reset`）

例如：

```c
mln_json_generate(&j, "[{s:d,s:d,s:{s:d}},d]", "a", 1, "b", 3, "c", "d", 4, 5);
mln_string_t *res = mln_json_encode(&j);

// 得到res中的内容为： [{"b":3,"c":{"d":4},"a":1},5]
```

返回值：

- `0` - 成功
- `-1` - 失败



#### mln_json_object_iterate

```c
int mln_json_object_iterate(mln_json_t *j, mln_json_object_iterator_t it, void *data);

typedef int (*mln_json_object_iterator_t)(mln_json_t * /*key*/, mln_json_t * /*val*/, void *);
```

描述：遍历对象`j`中的每一对`key`-`value`对，并使用`it`对键值对进行处理，`data`是用户自定义数据，会在`it`调用时一并传入。

返回值：

- `0` - 成功
- `-1` - 失败



#### mln_json_array_iterate

```c
mln_json_array_iterate(j, it, data);

typedef int (*mln_json_array_iterator_t)(mln_json_t *, void *);
```

描述：遍历数组`j`中的每一个元素，并使用`it`对数组元素进行处理，`data`是用户自定义数据，会在`it`调用时一并传入。

返回值：

- `0` - 成功
- `非0` - 失败或者其他含义，`it`返回任何`非0`值都将中断对数组的遍历



### 示例

#### 示例1

```c
#include <stdio.h>
#include "mln_string.h"
#include "mln_json.h"

static int callback(mln_json_t *j, void *data)
{
    mln_json_number_init(j, *(int *)data);
    return 0;
}

static int handler(mln_json_t *j, void *data)
{
    mln_json_dump(j, 0, "");
    return 0;
}

int main(int argc, char *argv[])
{
    int i = 1024;
    mln_json_t j, k;
    struct mln_json_call_attr ca;
    mln_string_t *res, exp = mln_string("protocols.0");
    mln_string_t tmp = mln_string("{\"paths\":[\"/mock\"],\"methods\":null,\"sources\":null,\"destinations\":null,\"name\":\"example_route\",\"headers\":null,\"hosts\":null,\"preserve_host\":false,\"regex_priority\":0,\"snis\":null,\"https_redirect_status_code\":426,\"tags\":null,\"protocols\":[\"http\",\"https\"],\"path_handling\":\"v0\",\"id\":\"52d58293-ae25-4c69-acc8-6dd729718a61\",\"updated_at\":1661345592,\"service\":{\"id\":\"c1e98b2b-6e77-476c-82ca-a5f1fb877e07\"},\"response_buffering\":true,\"strip_path\":true,\"request_buffering\":true,\"created_at\":1661345592}");

    if (mln_json_decode(&tmp, &j, NULL) < 0) { //解码字符串，生成mln_json_t的结点
        fprintf(stderr, "decode error\n");
        return -1;
    }

    mln_json_parse(&j, &exp, handler, NULL); //获取该JSON中key为protocols的数组的第1个元素，并交给handler处理

    //填充用户自定义数据解析结构
    ca.callback = callback;
    ca.data = &i;
    //利用格式符生成JSON结构
    mln_json_init(&k);//mln_json_generate前一定要对未初始化的json变量进行初始化
    if (mln_json_generate(&k, "[{s:d,s:d,s:{s:d}},d,[],j,c]", "a", 1, "b", 3, "c", "d", 4, 5, &j, &ca) < 0) {
        fprintf(stderr, "generate failed\n");
        return -1;
    }
    mln_json_generate(&k, "[s,d]", "g", 99);//这两个元素会被加入到k数组中
    res = mln_json_encode(&k); //对生成的结构生成相应的JSON字符串

    mln_json_destroy(&k); //注意，这里不要释放j，因为k会释放j中的内存

    if (res == NULL) {
        fprintf(stderr, "encode failed\n");
        return -1;
    }
    write(STDOUT_FILENO, res->data, res->len);
    write(STDOUT_FILENO, "\n", 1);
    mln_string_free(res);

    return 0;
}
```

本例的执行结果如下：

```
 type:string val:[http]
[{"b":3,"c":{"d":4},"a":1},5,[],{"preserve_host":false,"name":"example_route","destinations":null,"methods":null,"tags":null,"hosts":null,"response_buffering":true,"snis":null,"https_redirect_status_code":426,"headers":null,"request_buffering":true,"sources":null,"strip_path":true,"protocols":["http","https"],"path_handling":"v0","created_at":1661345592,"id":"52d58293-ae25-4c69-acc8-6dd729718a61","updated_at":1661345592,"paths":["/mock"],"regex_priority":0,"service":{"id":"c1e98b2b-6e77-476c-82ca-a5f1fb877e07"}},1024,"g",99]
```

第一行为`handler`中的`mln_dump`输出，第二行为`mln_json_encode`生成的字符串。


#### 示例2

```c
#include "mln_json.h"
#include "mln_log.h"

static int obj_iterator(mln_json_t *k, mln_json_t *v, void *data)
{
    mln_string_t *s = mln_json_string_data_get(k);
    int i = (int)mln_json_number_data_get(v);
    mln_log(none, "%S: %d\n", s, i);
    return 0;
}

static int array_iterator(mln_json_t *j, void *data)
{
    return mln_json_object_iterate(j, obj_iterator, data);
}

static int handler(mln_json_t *j, void *data)
{
    return mln_json_array_iterate(j, array_iterator, data);
}

static void parse(mln_string_t *p)
{
    mln_json_t j;
    mln_json_policy_t policy;
    mln_string_t exp = mln_string("resolutions");

    mln_json_policy_init(policy, 3, 11, 10, 1, 2);

    mln_json_decode(p, &j, &policy);

    mln_json_parse(&j, &exp, handler, NULL);

    mln_json_destroy(&j);
}

int main(void)
{
    mln_string_t p = mln_string("{\"name\":\"Awesome 4K\",\"resolutions\":[{\"width\":1280,\"height\":720}]}");
    parse(&p);
    return 0;
}
```

运行结果如下：

```
width: 1280
height: 720
```
