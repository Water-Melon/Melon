## JSON



### Header

```c
#include "mln_json.h"
```



### Functions/Macros



#### mln_json_new

```c
mln_json_t *mln_json_new(void);
```

Description: Create a new json node for generating json strings.

Return value: return `mln_json_t` pointer if successful, otherwise return `NULL`



#### mln_json_parse

```c
mln_json_t *mln_json_parse(mln_string_t *jstr);
```

Description: Parse the JSON string `jstr` into a data structure.

Return value: return `mln_json_t` pointer if successful, otherwise return `NULL`



#### mln_json_free

```c
void mln_json_free(void *json);
```

Description: Free `json` node memory of type `mln_json_t`.

Return value: none



#### mln_json_dump

```c
void mln_json_dump(mln_json_t *j, int n_space, char *prefix);
```

Description: Print the details of the json node `j` to standard output. `n_space` indicates the current number of indented spaces, and `prefix` is the prefix of the output content.

Return value: none



#### mln_json_generate

```c
mln_string_t *mln_json_generate(mln_json_t *j);
```

Description: Generate JSON string from `mln_json_t` node structure.

Return value: return `mln_string_t` string pointer successfully, otherwise return `NULL`



#### mln_json_value_search

```c
mln_json_t *mln_json_value_search(mln_json_t *j, mln_string_t *key);
```

Description: Search the value content of key `key` from node `j`. In this case, `j` must be of type object (dictionary with key:value pairs).

Return value: return value of type `mln_json_t` if successful, otherwise return `NULL`



#### mln_json_element_search

```c
mln_json_t *mln_json_element_search(mln_json_t *j, mln_uauto_t index);
```

Description: Search for the element content with subscript `index` from node `j`. In this case, `j` must be an array type.

Return value: If successful, return an element node of type `mln_json_t`, otherwise return `NULL`



#### mln_json_array_length

```c
mln_uauto_t mln_json_array_length(mln_json_t *j);
```

Description: Get the length of the array. In this case `j` must be an array type.

Return value: Array length



#### mln_json_obj_update

```c
int mln_json_obj_update(mln_json_t *j, mln_json_t *key, mln_json_t *val);
```

Description: Add `key` and `val` pairs to the `j` JSON node. In this case, `j` needs to be an object type. If `key` already exists, replace the original value with `val`.

Return value: return `0` if successful, otherwise return `-1`



#### mln_json_element_add

```c
int mln_json_element_add(mln_json_t *j, mln_json_t *value);
```

Description: Add `value` to JSON structure `j` of array type.

Return value: return `0` if successful, otherwise return `-1`



#### mln_json_element_update

```c
int mln_json_element_update(mln_json_t *j, mln_json_t *value, mln_uauto_t index);
```

Description: Update `value` to the position indexed `index` of the array type JSON structure `j`.

Return value: return `0` if successful, otherwise return `-1`



#### mln_json_reset

```c
void mln_json_reset(mln_json_t *j);
```

Description: Reset the JSON node `j` data structure, freeing its memory.

Return value: none



#### mln_json_obj_remove

```c
mln_json_t *mln_json_obj_remove(mln_json_t *j, mln_string_t *key);
```

Description: Remove the key-value pair whose key value is `key` from the JSON structure `j` of the object type, and return the corresponding value.

Return value: if exists, return the JSON node corresponding to the value part, otherwise return `NULL`



#### mln_json_element_remove

```c
mln_json_t *mln_json_element_remove(mln_json_t *j, mln_uauto_t index);
```

Description: Remove the element with subscript `index` from the array type JSON node and return it.

Return value: Returns the element pointer if it exists, otherwise returns `NULL`



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

Description: Determine the `json` type of the `mln_json_t` structure, in order: object, array, string, number, boolean true, boolean false, NULL, no type.

Return value: return `not 0` if the condition is met, otherwise return `0`



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

Description: Set the type for the `json` node of type `mln_json_t`, in order: no type, object, array, string, number, boolean true, boolean false, NULL.

Return value: none



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

Description: Get the data part of the corresponding type in the `json` node of type `mln_json_t`. The types are: object, array, string, number, boolean true, boolean false, NULL.

return value:

- the object type is a pointer of type `mln_hash_t`
- The array type is a pointer of type `mln_rbtree_t`
- The string type is a pointer of type `mln_string_t`
- the numeric type is a value of type `double`
- boolean true value of type `mln_u8_t`
- boolean false for a value of type `mln_u8_t`
- NULL type is a NULL value of type `mln_u8ptr_t`



#### set_data

```c
M_JSON_SET_DATA_STRING(json,str)
M_JSON_SET_DATA_NUMBER(json,num)
M_JSON_SET_DATA_TRUE(json)
M_JSON_SET_DATA_FALSE(json)
M_JSON_SET_DATA_NULL(json)
```

Description: Set data values for different types of JSON nodes `json`. Object and array types are manipulated using hash table and red-black tree functions, respectively, and the rest of the types are set with the above macros.

**Note**: The string set here must be the memory allocated from the memory pool or the heap. The memory in the stack will have a segmentation fault, because the assignment will not automatically copy a copy in the macro but use it directly.

Return value: none



#### M_JSON_SET_INDEX

```c
M_JSON_SET_INDEX(json,i)
```

Description: Set the subscript of `mln_json_t` type node `json` to `index`. This macro is used to generate part of an array in a JSON string.

Return value: none



### Example

```c
#include <stdio.h>
#include <stdlib.h>
#include "mln_core.h"
#include "mln_log.h"
#include "mln_string.h"
#include "mln_json.h"

int main(int argc, char *argv[])
{
    mln_json_t *j = NULL, *key = NULL, *val = NULL;
    mln_string_t s1 = mln_string("name");
    mln_string_t s2 = mln_string("Tom");
    mln_string_t *res;
    struct mln_core_attr cattr;

    cattr.argc = argc;
    cattr.argv = argv;
    cattr.global_init = NULL;
    cattr.main_thread = NULL;
    cattr.master_process = NULL;
    cattr.worker_process = NULL;
    if (mln_core_init(&cattr) < 0) {
        fprintf(stderr, "init failed\n");
        return -1;
    }

    key = mln_json_new();
    if (key == NULL) {
        mln_log(error, "init key failed\n");
        goto err;
    }
    M_JSON_SET_TYPE_STRING(key);
    M_JSON_SET_DATA_STRING(key, mln_string_dup(&s1));//Note that the memory must be allocated by itself, and the memory in the stack cannot be used directly

    val = mln_json_new();
    if (val == NULL) {
        mln_log(error, "init val failed\n");
        goto err;
    }
    M_JSON_SET_TYPE_STRING(val);
    M_JSON_SET_DATA_STRING(val, mln_string_dup(&s2));//Note that the memory must be allocated by itself, and the memory in the stack cannot be used directly

    j = mln_json_new();
    if (j == NULL) {
        mln_log(error, "init object failed\n");
        goto err;
    }
    if (mln_json_obj_update(j, key, val) < 0) {
        mln_log(error, "update object failed\n");
        goto err;
    }
    key = val = NULL;

    res = mln_json_generate(j);
    mln_json_free(j);
    if (res == NULL) {
        mln_log(error, "generate failed\n");
        goto err;
    }
    mln_log(debug, "%S\n", res);

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

