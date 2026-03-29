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

描述：初始化JSON类型结点`j`，将结点类型设为`NONE`。

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

描述：将JSON字符串`jstr`解析成数据结构，结果会被放入参数`out`中。`policy`为安全策略结构，由`mln_json_policy_init`进行初始化。若解码失败，`out`会被自动清理，无需手动调用`mln_json_destroy`。

返回值：

- `0` - 成功
- `-1` - 失败。如果`policy`不为`NULL`，可使用`mln_json_policy_error`检查具体违反了哪个安全限制条件。



#### mln_json_destroy

```c
void mln_json_destroy(mln_json_t *j);
```

描述：释放`mln_json_t`类型的`j`节点所持有的内存。

返回值：无



#### mln_json_dump

```c
void mln_json_dump(mln_json_t *j, int n_space, char *prefix);
```

描述：将JSON节点`j`的详细信息输出到标准输出。`n_space`表示当前缩进空格数，`prefix`为输出内容的前缀。

返回值：无



#### mln_json_encode

```c
mln_string_t *mln_json_encode(mln_json_t *j, mln_u32_t flags);
```

描述：由`mln_json_t`节点结构生成JSON文本字符串。返回值使用后需要调用`mln_string_free`进行释放。

标志位：
- `0` - 输出为标准JSON文本格式（默认）
- `M_JSON_ENCODE_UNICODE` (0x1) - 将非ASCII字符（如中文）转义为`\uXXXX`形式

返回值：成功返回`mln_string_t`字符串指针，否则返回`NULL`



#### mln_json_obj_search

```c
mln_json_t *mln_json_obj_search(mln_json_t *j, mln_string_t *key);
```

描述：从对象类型结点`j`中搜索key为`key`的value。

返回值：成功则返回`mln_json_t`类型的value，否则返回`NULL`



#### mln_json_array_search

```c
mln_json_t *mln_json_array_search(mln_json_t *j, mln_uauto_t index);
```

描述：从数组类型结点`j`中搜索下标为`index`的元素。

返回值：成功则返回`mln_json_t`类型的元素节点，否则返回`NULL`



#### mln_json_array_length

```c
mln_uauto_t mln_json_array_length(mln_json_t *j);
```

描述：获取数组的长度。`j`必须为数组类型。

返回值：数组长度



#### mln_json_obj_update

```c
int mln_json_obj_update(mln_json_t *j, mln_json_t *key, mln_json_t *val);
```

描述：将`key`与`val`对添加到对象类型的JSON节点`j`中。若`key`已经存在，则原有的key和value将被新参数替换。函数内部会拷贝`key`和`val`的内容，调用方无需为其分配堆内存。

返回值：成功则返回`0`，否则返回`-1`



#### mln_json_obj_element_num

```c
mln_size_t mln_json_obj_element_num(mln_json_t *j);
```

描述：获取对象类型JSON节点`j`中键值对的数量。

返回值：键值对数量



#### mln_json_array_append

```c
int mln_json_array_append(mln_json_t *j, mln_json_t *value);
```

描述：将`value`追加到数组类型的JSON结构`j`末尾。函数内部会拷贝`value`的内容。

返回值：成功则返回`0`，否则返回`-1`



#### mln_json_array_update

```c
int mln_json_array_update(mln_json_t *j, mln_json_t *value, mln_uauto_t index);
```

描述：将`value`更新到数组类型JSON结构`j`中下标为`index`的位置。若下标不存在，则会失败。函数内部会拷贝`value`的内容。

返回值：成功则返回`0`，否则返回`-1`



#### mln_json_reset

```c
void mln_json_reset(mln_json_t *j);
```

描述：重置JSON节点`j`，释放其所持有的数据，并将类型恢复为`NONE`。

返回值：无



#### mln_json_obj_remove

```c
void mln_json_obj_remove(mln_json_t *j, mln_string_t *key);
```

描述：将key为`key`的键值对从对象类型的JSON结构`j`中删除并释放。

返回值：无



#### mln_json_array_remove

```c
void mln_json_array_remove(mln_json_t *j, mln_uauto_t index);
```

描述：删除并释放数组类型JSON节点`j`中的最后一个元素。参数`index`必须等于最后一个有效下标（即`length - 1`），仅在调试构建中用于断言检查。

返回值：无



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

描述：判断`mln_json_t`结构的`json`类型，依次为：对象、数组、字符串、数字、布尔真、布尔假、NULL、无类型。

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

描述：给`mln_json_t`类型的`json`节点设置类型，依次为：无类型、对象、数组、字符串、数字、布尔真、布尔假、NULL。

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

描述：获取`mln_json_t`类型的`json`节点中对应类型的数据部分。

返回值：

- 对象类型为`mln_json_obj_t`类型指针
- 数组类型为`mln_array_t`类型指针
- 字符串类型为`mln_string_t`类型指针
- 数字类型为`double`类型值
- 布尔真为`mln_u8_t`类型值
- 布尔假为`mln_u8_t`类型值
- NULL类型为`mln_u8ptr_t`类型的NULL值



#### mln_json_policy_init

```c
mln_json_policy_init(policy, _depth, _keylen, _strlen, _elemnum, _kvnum);
```

描述：对`mln_json_policy_t`类型的`policy`进行初始化，其余参数含义如下：

- `_depth` JSON的最大嵌套层数。解析时，遇到数组或对象会递增嵌套层数，解析完成后递减。值为`0`时表示不限制。

- `_keylen` 对象中key的最大长度。值为`0`时表示不限制。

- `_strlen` 字符串值的最大长度。值为`0`时表示不限制。

- `_elemnum` 最大数组元素个数。值为`0`时表示不限制。

- `_kvnum` 最大对象键值对个数。值为`0`时表示不限制。

返回值：无



#### mln_json_policy_error

```c
mln_json_policy_error(policy)
```

描述：从`mln_json_policy_t`类型的`policy`获取错误号。一般用于`mln_json_decode`返回`-1`时检查是否违反了安全策略。

返回值：`int`型错误号，取值如下：

- `M_JSON_OK` 没有违反安全策略。

- `M_JSON_DEPTH` 嵌套层数过多。

- `M_JSON_KEYLEN` 对象Key的长度超过限制。

- `M_JSON_STRLEN` 字符串值的长度超过限制。

- `M_JSON_ARRELEM` 数组元素个数超过限制。

- `M_JSON_OBJKV` 对象键值对个数超过限制。



#### mln_json_fetch

```c
int mln_json_fetch(mln_json_t *j, mln_string_t *exp, mln_json_iterator_t iterator, void *data)

typedef int (*mln_json_iterator_t)(mln_json_t *, void *);
```

描述：

根据表达式`exp`从JSON节点`j`中查找匹配的子结点。如果找到匹配的子结点且`iterator`不为`NULL`，则调用`iterator`回调函数进行处理。`data`为用户自定义数据，会在调用`iterator`时传入。

假设有如下JSON：

```
{
  "a": [1, {
    "c": 3
  }],
  "b": 2
}
```

`exp`的写法示例：

| 写法    | 含义                                                         |
| ------- | ------------------------------------------------------------ |
| `a`     | 获取key为`a`的值（每个值都保存在`mln_json_t`结构中）         |
| `b`     | 获取key为`b`的值                                             |
| `a.0`   | 获取key为`a`的数组中下标为`0`的元素                          |
| `a.1.c` | 获取key为`a`的数组中下标为`1`的对象，再获取其key为`c`的值    |
| `a.2.c` | 下标`2`的元素不存在，`mln_json_fetch`返回`-1`                |

返回值：

- `0` - 成功
- `-1` - 失败



#### mln_json_generate

```c
int mln_json_generate(mln_json_t *j, char *fmt, ...);
```

描述：

根据`fmt`给出的格式，利用后续可变参数填充至`j`中。

`fmt`支持如下格式符：

- `{}` - 表示大括号内是一个对象类型，key与value之间使用`:`分隔，每组键值对之间以`,`分隔
- `[]` - 表示中括号内是一个数组类型，每个数组元素之间以`,`分隔
- `j` - 对应的可变参数是`mln_json_t`指针
- `d` - 对应的可变参数是32位有符号值，例如`int`或`mln_s32_t`
- `D` - 对应的可变参数是64位有符号值，例如`long long`或`mln_s64_t`
- `u` - 对应的可变参数是32位无符号值，例如`unsigned int`或`mln_u32_t`
- `U` - 对应的可变参数是64位无符号值，例如`unsigned long long`或`mln_u64_t`
- `F` - 对应的可变参数是`double`类型的值
- `t` - 此位置填入一个`true`，不需要对应的可变参数
- `f` - 此位置填入一个`false`，不需要对应的可变参数
- `n` - 此位置填入一个`null`，不需要对应的可变参数
- `s` - 对应的可变参数是`char *`类型字符串
- `S` - 对应的可变参数是`mln_string_t *`类型字符串
- `c` - 对应的可变参数是`struct mln_json_call_attr *`类型结构，包含`callback`和`data`两个成员，表示由`callback`函数配合`data`生成该位置的值

注意事项：

1. `fmt`中仅可出现上述格式符号，不可包含空格、制表符或换行符等其他字符。
2. 成功执行本函数后，格式符`j`对应的可变参数所指向的JSON节点将被`j`接管，调用方不可再对其调用`mln_json_destroy`或`mln_json_reset`。

例如：

```c
mln_json_init(&j);
mln_json_generate(&j, "[{s:d,s:d,s:{s:d}},d]", "a", 1, "b", 3, "c", "d", 4, 5);
mln_string_t *res = mln_json_encode(&j, 0);

// 得到res中的内容为： [{"a":1,"b":3,"c":{"d":4}},5]
```

返回值：

- `0` - 成功
- `-1` - 失败



#### mln_json_object_iterate

```c
int mln_json_object_iterate(mln_json_t *j, mln_json_object_iterator_t it, void *data);

typedef int (*mln_json_object_iterator_t)(mln_json_t * /*key*/, mln_json_t * /*val*/, void *);
```

描述：遍历对象`j`中的每一对键值对，并使用`it`进行处理。`data`是用户自定义数据，会在调用`it`时一并传入。

返回值：

- `0` - 成功
- `-1` - 失败



#### mln_json_array_iterate

```c
mln_json_array_iterate(j, it, data);

typedef int (*mln_json_array_iterator_t)(mln_json_t *, void *);
```

描述：遍历数组`j`中的每一个元素，并使用`it`进行处理。`data`是用户自定义数据，会在调用`it`时一并传入。

返回值：

- `0` - 成功
- `非0` - 失败或其他含义。`it`返回任何`非0`值都将中断遍历






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

    /* 解码字符串，生成mln_json_t的结点 */
    if (mln_json_decode(&tmp, &j, NULL) < 0) {
        fprintf(stderr, "decode error\n");
        return -1;
    }

    /* 获取key为protocols的数组的第0个元素，并交给handler处理 */
    mln_json_fetch(&j, &exp, handler, NULL);

    /* 填充用户自定义数据解析结构 */
    ca.callback = callback;
    ca.data = &i;

    /* mln_json_generate前一定要对未初始化的json变量进行初始化 */
    mln_json_init(&k);
    if (mln_json_generate(&k, "[{s:d,s:d,s:{s:d}},d,[],j,c]", "a", 1, "b", 3, "c", "d", 4, 5, &j, &ca) < 0) {
        fprintf(stderr, "generate failed\n");
        return -1;
    }
    /* 这两个元素会被追加到k数组中 */
    mln_json_generate(&k, "[s,d]", "g", 99);
    /* 对生成的结构编码为JSON字符串 */
    res = mln_json_encode(&k, 0);

    /* 注意：不要释放j，因为j的内容已被k接管 */
    mln_json_destroy(&k);

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
[{"a":1,"b":3,"c":{"d":4}},5,[],{"paths":["/mock"],"methods":null,"sources":null,"destinations":null,"name":"example_route","headers":null,"hosts":null,"preserve_host":false,"regex_priority":0,"snis":null,"https_redirect_status_code":426,"tags":null,"protocols":["http","https"],"path_handling":"v0","id":"52d58293-ae25-4c69-acc8-6dd729718a61","updated_at":1661345592,"service":{"id":"c1e98b2b-6e77-476c-82ca-a5f1fb877e07"},"response_buffering":true,"strip_path":true,"request_buffering":true,"created_at":1661345592},1024,"g",99]
```

第一行为`handler`中的`mln_json_dump`输出，第二行为`mln_json_encode`生成的字符串。


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

    mln_json_fetch(&j, &exp, handler, NULL);

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
