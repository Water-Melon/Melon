## JSON



### 头文件

```c
#include "mln_json.h"
```



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
int mln_json_decode(mln_string_t *jstr, mln_json_t *out);
```

描述：将JSON字符串`jstr`解析成数据结构，结果会被放入参数`out`中。

返回值：

- `0` - 成功
- `-1` - 失败



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
M_JSON_IS_OBJECT(json)
M_JSON_IS_ARRAY(json)
M_JSON_IS_STRING(json)
M_JSON_IS_NUMBER(json)
M_JSON_IS_TRUE(json)
M_JSON_IS_FALSE(json)
M_JSON_IS_NULL(json)
M_JSON_IS_NONE(json)
```

描述：判断`mln_json_t`结构的`json`类型，依次分别为：对象、数组、字符串、数字、布尔真、布尔假、NULL、无类型。

返回值：满足条件返回`非0`，否则返回`0`



#### set_type

```c
M_JSON_SET_TYPE_NONE(json)
M_JSON_SET_TYPE_OBJECT(json)
M_JSON_SET_TYPE_ARRAY(json)
M_JSON_SET_TYPE_STRING(json)
M_JSON_SET_TYPE_NUMBER(json)
M_JSON_SET_TYPE_TRUE(json)
M_JSON_SET_TYPE_FALSE(json)
M_JSON_SET_TYPE_NULL(json)
```

描述：给`mln_json_t`类型的`json`节点设置类型，依次分别为：无类型、对象、数组、字符串、数字、布尔真、布尔假、NULL。

返回值：无



#### get_data

```c
M_JSON_GET_DATA_OBJECT(json)
M_JSON_GET_DATA_ARRAY(json)
M_JSON_GET_DATA_STRING(json)
M_JSON_GET_DATA_NUMBER(json)
M_JSON_GET_DATA_TRUE(json)
M_JSON_GET_DATA_FALSE(json)
M_JSON_GET_DATA_NULL(json)
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



### 示例

```c
#include <stdio.h>
#include "mln_string.h"
#include "mln_json.h"

int main(int argc, char *argv[])
{
    mln_json_t j;
    mln_string_t *res;
    mln_string_t tmp = mln_string("{\"paths\":[\"/mock\"],\"methods\":null,\"sources\":null,\"destinations\":null,\"name\":\"example_route\",\"headers\":null,\"hosts\":null,\"preserve_host\":false,\"regex_priority\":0,\"snis\":null,\"https_redirect_status_code\":426,\"tags\":null,\"protocols\":[\"http\",\"https\"],\"path_handling\":\"v0\",\"id\":\"52d58293-ae25-4c69-acc8-6dd729718a61\",\"updated_at\":1661345592,\"service\":{\"id\":\"c1e98b2b-6e77-476c-82ca-a5f1fb877e07\"},\"response_buffering\":true,\"strip_path\":true,\"request_buffering\":true,\"created_at\":1661345592}");

    if (mln_json_decode(&tmp, &j) < 0) {
        fprintf(stderr, "decode error\n");
        return -1;
    }
    mln_json_dump(&j, 0, NULL);

    res = mln_json_encode(&j);
    mln_json_destroy(&j);
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

