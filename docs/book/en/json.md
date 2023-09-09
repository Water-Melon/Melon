## JSON



### Header file

```c
#include "mln_json.h"
```



### Functions/Macros



#### mln_json_init

```c
mln_json_init(j)
```

Description: Initialize JSON type node `j`, the node type is `NONE`.

Return value: None



#### mln_json_string_init

```c
mln_json_string_init(j, s)
```

Description: Initialize JSON type node `j` to string type and assign value `s` of type `mln_string_t *`. `s` will be taken over by the JSON structure. The caller needs to ensure the memory life cycle of `s` and cannot modify the content of `s`.

Return value: None



#### mln_json_number_init

```c
mln_json_number_init(j, n)
```

Description: Initialize the JSON type node `j` to a numeric type and assign a value `n` of type `double`.

Return value: None



#### mln_json_true_init

```c
mln_json_true_init(j)
```

Description: Initialize JSON type node `j` to type `true`.

Return value: None



#### mln_json_false_init

```c
mln_json_false_init(j)
```

Description: Initialize the JSON type node `j` to the `false` type.

Return value: None



#### mln_json_null_init

```c
mln_json_null_init(j)
```

Description: Initialize JSON type node `j` to `null` type.

Return value: None



#### mln_json_obj_init

```c
int mln_json_obj_init(mln_json_t *j);
```

Description: Initialize JSON type node `j` to object type.

Return value:

- `0` - Success
- `-1` - failed



#### mln_json_array_init

```c
int mln_json_array_init(mln_json_t *j);
```

Description: Initialize JSON type node `j` to array type.

Return value:

- `0` - Success
- `-1` - failed



#### mln_json_decode

```c
int mln_json_decode(mln_string_t *jstr, mln_json_t *out);
```

Description: Parse the JSON string `jstr` into a data structure, and the result will be put into the parameter `out`.

Return value:

- `0` - Success
- `-1` - failed



#### mln_json_destroy

```c
void mln_json_destroy(mln_json_t *j;
```

Description: Release the `j` node memory of type `mln_json_t`.

Return value: None



#### mln_json_dump

```c
void mln_json_dump(mln_json_t *j, int n_space, char *prefix);
```

Description: Output the details of json node `j` to standard output. `n_space` represents the current number of indented spaces, and `prefix` is the prefix of the output content.

Return value: None



#### mln_json_encode

```c
mln_string_t *mln_json_encode(mln_json_t *j);
```

Description: Generate a JSON string from the `mln_json_t` node structure. The return value needs to be released by calling `mln_string_free` after use.

Return value: `mln_string_t` string pointer is returned successfully, otherwise `NULL` is returned



#### mln_json_obj_search

```c
mln_json_t *mln_json_obj_search(mln_json_t *j, mln_string_t *key);
```

Description: Search the value content with key `key` from object type node `j`.

Return value: If successful, return a value of type `mln_json_t`, otherwise return `NULL`



#### mln_json_array_search

```c
mln_json_t *mln_json_array_search(mln_json_t *j, mln_uauto_t index);
```

Description: Search the element content with subscript `index` from array type node `j`.

Return value: If successful, an element node of type `mln_json_t` is returned, otherwise `NULL` is returned.



#### mln_json_array_length

```c
mln_uauto_t mln_json_array_length(mln_json_t *j);
```

Description: Get the length of the array. At this time `j` must be of array type.

Return value: array length



#### mln_json_obj_update

```c
int mln_json_obj_update(mln_json_t *j, mln_json_t *key, mln_json_t *val);
```

Description: Add the `key` and `val` pairs to the `j` JSON node. At this time, `j` needs to be an object type. If `key` already exists, the original `key` and `value` will be replaced by the new parameters. Therefore the parameters `key` and `val` will be taken over by `j` after the call.

Return value: Returns `0` on success, otherwise returns `-1`



#### mln_json_array_append

```c
int mln_json_array_append(mln_json_t *j, mln_json_t *value);
```

Description: Add `value` to the JSON structure `j` of array type.

Return value: Returns `0` on success, otherwise returns `-1`



#### mln_json_array_update

```c
int mln_json_array_update(mln_json_t *j, mln_json_t *value, mln_uauto_t index);
```

Description: Update `value` to the position where the index is `index` of the array type JSON structure `j`. If the subscript does not exist, it will fail.

Return value: Returns `0` on success, otherwise returns `-1`



#### mln_json_reset

```c
void mln_json_reset(mln_json_t *j);
```

Description: Reset the JSON node `j` data structure and release its original data.

Return value: None



#### mln_json_obj_remove

```c
void mln_json_obj_remove(mln_json_t *j, mln_string_t *key);
```

Description: Delete and release the key-value pair with key value `key` from the JSON structure `j` of the object type.

Return value: If it exists, return the JSON node corresponding to the value part, otherwise return `NULL`



#### mln_json_array_remove

```c
void mln_json_array_remove(mln_json_t *j, mln_uauto_t index);
```

Description: Delete and release the element with index `index` from the array type JSON node.

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

Description: Determine the `json` type of the `mln_json_t` structure, which are: object, array, string, number, Boolean true, Boolean false, NULL, and no type.

Return value: Returns `non-0` if the conditions are met, otherwise returns `0`



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

Description: Set the type for the `json` node of type `mln_json_t`, in order: no type, object, array, string, number, Boolean true, Boolean false, NULL.

Return value: None



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

Description: Get the data part of the corresponding type in the `json` node of type `mln_json_t`. The types are: object, array, string, number, Boolean true, Boolean false, and NULL.

Return value:

- The object type is `mln_hash_t` type pointer
- The array type is `mln_rbtree_t` type pointer
- The string type is `mln_string_t` type pointer
- The number type is a `double` type value
- Boolean true for `mln_u8_t` type value
- Boolean false for `mln_u8_t` type value
- NULL type is a NULL value of type `mln_u8ptr_t`



#### mln_json_parse

```c
int mln_json_parse(mln_json_t *j, mln_string_t *exp, mln_json_iterator_t iterator, void *data)

typedef int (*mln_json_iterator_t)(mln_json_t *, void *);
```

Description:

From the decoded `j` node, obtain the matching `mln_json_t` node based on `exp`. If there is a matching node, the `iterator` callback function will be called for processing. `data` is user-defined data, passed in when `iterator` is called.

If we have the following JSON:

```
{
  "a": [1, {
    "c": 3
  }],
  "b": 2
}
```

`exp` is written in the form:

```
a
b
a.0
a.2.c
```

返回值：

- `0` - succeed
- `-1` - failed



#### mln_json_generate

```c
int mln_json_generate(mln_json_t *j, char *fmt, ...);
```

Description:

According to the format given by `fmt`, fill it into `j` with subsequent arguments.

For example:

```c
mln_json_generate(&j, "[{s:d,s:d,s:{s:d}},d]", "a", 1, "b", 3, "c", "d", 4, 5);
mln_string_t *res = mln_json_encode(&j);

// the content of res is: [{"b":3,"c":{"d":4},"a":1},5]
```

Return value:

- `0` - succeed
- `-1` - failed



### Example

```c
#include <stdio.h>
#include "mln_string.h"
#include "mln_json.h"

static int handler(mln_json_t *j, void *data)
{
    mln_json_dump(j, 0, "");
    return 0;
}

int main(int argc, char *argv[])
{
    mln_json_t j;
    mln_string_t *res, exp = mln_string("protocols.0");
    mln_string_t tmp = mln_string("{\"paths\":[\"/mock\"],\"methods\":null,\"sources\":null,\"destinations\":null,\"name\":\"example_route\",\"headers\":null,\"hosts\":null,\"preserve_host\":false,\"regex_priority\":0,\"snis\":null,\"https_redirect_status_code\":426,\"tags\":null,\"protocols\":[\"http\",\"https\"],\"path_handling\":\"v0\",\"id\":\"52d58293-ae25-4c69-acc8-6dd729718a61\",\"updated_at\":1661345592,\"service\":{\"id\":\"c1e98b2b-6e77-476c-82ca-a5f1fb877e07\"},\"response_buffering\":true,\"strip_path\":true,\"request_buffering\":true,\"created_at\":1661345592}");

    if (mln_json_decode(&tmp, &j) < 0) {
        fprintf(stderr, "decode error\n");
        return -1;
    }

    mln_json_parse(&j, &exp, handler, NULL);
    mln_json_destroy(&j);
    if (mln_json_generate(&j, "[{s:d,s:d,s:{s:d}},d]", "a", 1, "b", 3, "c", "d", 4, 5) < 0) {
        fprintf(stderr, "generate failed\n");
        return -1;
    }
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

