## JSON



### 头文件

```c
#include "mln_json.h"
```



### 函数/宏



#### mln_json_new

```c
mln_json_t *mln_json_new(void);
```

描述：新建json节点，用于生成json字符串之用。

返回值：成功则返回`mln_json_t`指针，否则返回`NULL`



#### mln_json_parse

```c
mln_json_t *mln_json_parse(mln_string_t *jstr);
```

描述：将JSON字符串`jstr`解析成数据结构。

返回值：成功则返回`mln_json_t`指针，否则返回`NULL`



#### mln_json_free

```c
void mln_json_free(void *json);
```

描述：释放`mln_json_t`类型的`json`节点内存。

返回值：无



#### mln_json_dump

```c
void mln_json_dump(mln_json_t *j, int n_space, char *prefix);
```

描述：将json节点`j`的详细信息输出到标准输出。`n_space`表示当前缩进空格数，`prefix`为输出内容的前缀。

返回值：无



#### mln_json_generate

```c
mln_string_t *mln_json_generate(mln_json_t *j);
```

描述：由`mln_json_t`节点结构生成JSON字符串。

返回值：成功返回`mln_string_t`字符串指针，否则返回`NULL`



#### mln_json_value_search

```c
mln_json_t *mln_json_value_search(mln_json_t *j, mln_string_t *key);
```

描述：从节点`j`中搜索key为`key`的value内容。此时，`j`必须为对象类型（有key: value对的字典）。

返回值：成功则返回`mln_json_t`类型的value，否则返回`NULL`



#### mln_json_element_search

```c
mln_json_t *mln_json_element_search(mln_json_t *j, mln_uauto_t index);
```

描述：从节点`j`中搜索下标为`index`的元素内容。此时，`j`必须为数组类型。

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

描述：将`key`与`val`对添加到`j` JSON节点中。此时，`j`需为对象类型。若`key`已经存在，则将原本value替换为`val`。

返回值：成功则返回`0`，否则返回`-1`



#### mln_json_element_add

```c
int mln_json_element_add(mln_json_t *j, mln_json_t *value);
```

描述：将`value`加入到数组类型的JSON结构`j`中。

返回值：成功则返回`0`，否则返回`-1`



#### mln_json_element_update

```c
int mln_json_element_update(mln_json_t *j, mln_json_t *value, mln_uauto_t index);
```

描述：将`value`更新到数组类型JSON结构`j`的下标为`index`的位置上。

返回值：成功则返回`0`，否则返回`-1`



#### mln_json_reset

```c
void mln_json_reset(mln_json_t *j);
```

描述：重置JSON节点`j`数据结构，将其内存进行释放。

返回值：无



#### mln_json_obj_remove

```c
mln_json_t *mln_json_obj_remove(mln_json_t *j, mln_string_t *key);
```

描述：将key值为`key`的键值对从对象类型的JSON结构`j`中删除，并将相应value返回。

返回值：存在则返回对应value部分的JSON节点，否则返回`NULL`



#### mln_json_element_remove

```c
mln_json_t *mln_json_element_remove(mln_json_t *j, mln_uauto_t index);
```

描述：将下标为`index`的元素从数组类型JSON节点上删除并返回。

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



#### set_data

```c
M_JSON_SET_DATA_STRING(json,str)
M_JSON_SET_DATA_NUMBER(json,num)
M_JSON_SET_DATA_TRUE(json)
M_JSON_SET_DATA_FALSE(json)
M_JSON_SET_DATA_NULL(json)
```

描述：给不同类型的JSON节点`json`设置数据值。对象和数组类型分别使用哈希表和红黑树函数进行操作，其余类型用上述宏进行设置。

**注意**：这里设置的字符串必须是从内存池或堆中分配的内存，栈中内存会出现段错误，因为赋值时不会在宏内自动复制一份而是直接使用。

返回值：无



#### M_JSON_SET_INDEX

```c
M_JSON_SET_INDEX(json,i)
```

描述：设置`mln_json_t`类型节点`json`的下标为`index`。该宏用于生成JSON字符串中数组的部分。

返回值：无



### 示例

```c
#include <stdio.h>
#include <stdlib.h>
#include "mln_string.h"
#include "mln_json.h"

int main(int argc, char *argv[])
{
    mln_json_t *j = NULL, *key = NULL, *val = NULL;
    mln_string_t s1 = mln_string("name");
    mln_string_t s2 = mln_string("Tom");
    mln_string_t *res;

    key = mln_json_new();
    if (key == NULL) {
        fprintf(stderr, "init key failed\n");
        goto err;
    }
    M_JSON_SET_TYPE_STRING(key);
    M_JSON_SET_DATA_STRING(key, mln_string_dup(&s1));//注意，一定是要自行分配内存，不可直接使用栈中内存

    val = mln_json_new();
    if (val == NULL) {
        fprintf(stderr, "init val failed\n");
        goto err;
    }
    M_JSON_SET_TYPE_STRING(val);
    M_JSON_SET_DATA_STRING(val, mln_string_dup(&s2));//注意，一定是要自行分配内存，不可直接使用栈中内存

    j = mln_json_new();
    if (j == NULL) {
        fprintf(stderr, "init object failed\n");
        goto err;
    }
    if (mln_json_obj_update(j, key, val) < 0) {
        fprintf(stderr, "update object failed\n");
        goto err;
    }
    key = val = NULL;

    res = mln_json_generate(j);
    mln_json_free(j);
    if (res == NULL) {
        fprintf(stderr, "generate failed\n");
        goto err;
    }
    write(STDOUT_FILENO, res->data, res->len);
    write(STDOUT_FILENO, "\n", 1);

    j = mln_json_parse(res);
    mln_string_free(res);
    mln_json_dump(j, 0, NULL);

    mln_json_free(j);

    return 0;

err:
    if (j != NULL) mln_json_free(j);
    if (key != NULL) mln_json_free(key);
    if (val != NULL) mln_json_free(val);
    return -1;
}
```

