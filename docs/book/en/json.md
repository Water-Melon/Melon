## JSON



### Header file

```c
#include "mln_json.h"
```



### Module

`json`



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

- `0` - on success
- `-1` - on failure



#### mln_json_array_init

```c
int mln_json_array_init(mln_json_t *j);
```

Description: Initialize JSON type node `j` to array type.

Return value:

- `0` - on success
- `-1` - on failure



#### mln_json_decode

```c
int mln_json_decode(mln_string_t *jstr, mln_json_t *out, mln_json_policy_t *policy);
```

Description: Parse the JSON string `jstr` into a data structure, and the result will be put into the parameter `out`. The policy is a security policy structure initialized by mln_json_policy_init.

Return value:

- `0` - on success
- `-1` - on failure. If policy is not NULL, you need to use mln_json_policy_error to check which specific security constraint has been violated.



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
mln_json_is_object(json)
mln_json_is_array(json)
mln_json_is_string(json)
mln_json_is_number(json)
mln_json_is_true(json)
mln_json_is_false(json)
mln_json_is_null(json)
mln_json_is_none(json)
```

Description: Determine the `json` type of the `mln_json_t` structure, which are: object, array, string, number, Boolean true, Boolean false, NULL, and no type.

Return value: Returns `non-0` if the conditions are met, otherwise returns `0`



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

Description: Set the type for the `json` node of type `mln_json_t`, in order: no type, object, array, string, number, Boolean true, Boolean false, NULL.

Return value: None



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

Description: Get the data part of the corresponding type in the `json` node of type `mln_json_t`. The types are: object, array, string, number, Boolean true, Boolean false, and NULL.

Return value:

- The object type is `mln_hash_t` type pointer
- The array type is `mln_rbtree_t` type pointer
- The string type is `mln_string_t` type pointer
- The number type is a `double` type value
- Boolean true for `mln_u8_t` type value
- Boolean false for `mln_u8_t` type value
- NULL type is a NULL value of type `mln_u8ptr_t`



#### mln_json_policy_init

```c
mln_json_policy_init(policy, _depth, _keylen, _strlen, _elemnum, _kvnum);
```

Description: Initialize the `policy` of type `mln_json_policy_t`. The meanings of the remaining parameters are as follows:

- `_depth` The maximum nesting depth of the JSON. During parsing, the nesting depth increases when encountering an array or object, and decreases when the array or object is fully parsed.

- `_keylen` The maximum length of keys in objects.

- `_strlen` The maximum length of string values.

- `_elemnum` The maximum number of elements in an array.

- `_kvnum` The maximum number of key-value pairs in an object.

Return Value: None



#### mln_json_policy_error

```c
mln_json_policy_error(policy)
```

Description: Get the error code from the `policy` of type `mln_json_policy_t`. This macro is typically used to check for security policy violations when `mln_json_decode` returns `-1`.

Return Value: An int type error code with the following values:

- `M_JSON_OK` No security policy violations.

- `M_JSON_DEPTH` The nesting depth is too deep.

- `M_JSON_KEYLEN` The length of the object key exceeds the limit.

- `M_JSON_STRLEN` The length of the string value exceeds the limit.

- `M_JSON_ARRELEM` The number of elements in the array exceeds the limit.

- `M_JSON_OBJKV` The number of key-value pairs in the object exceeds the limit.



#### mln_json_parse

```c
int mln_json_parse(mln_json_t *j, mln_string_t *exp, mln_json_iterator_t iterator, void *data)

typedef int (*mln_json_iterator_t)(mln_json_t *, void *);
```

Description:

From the JSON managed by the `j` node, obtain the matching `mln_json_t` child node according to `exp`. If there is a matching child node, the `iterator` callback function will be called for processing. `data` is User-defined data is passed in when `iterator` is called.

Suppose we have the following JSON：

```
{
  "a": [1, {
    "c": 3
  }],
  "b": 2
}
```

The content of `exp` is as follows:

| Content | Meaning                                                      |
| ------- | ------------------------------------------------------------ |
| `a`     | Get the value with key a (each value is saved in the `mln_json_t` structure) |
| `b`     | Get the value with key b                                     |
| `a.0`   | Get the key a, and its value should be an array, and then get the 0th subscript element of the array |
| `a.2.c` | Get the array corresponding to the value with key `a`, then get the array element with subscript `2`, which should be an object, and finally get the value with key `c` of the element object. This expression will cause `mln_json_parse` in this case returns `-1` because the array element with index `2` does not exist |

Return value：

- `0` - on success
- `-1` - on failure



#### mln_json_generate

```c
int mln_json_generate(mln_json_t *j, char *fmt, ...);
```

Description:

According to the format given by `fmt`, fill the subsequent arguments into `j`.

`fmt` supports the following format characters:

- `{}` - Indicates that what is inside the curly brackets is an object type. The key and value of the object are separated by `:`, and each group of key-value is separated by `,`.
- `[]` - Indicates that what is in the square brackets is an array type, and each array element is separated by `,`.
- `j` - Indicates that the variable parameter part is passed in a `mln_json_t` pointer.
- `d` - Indicates that the variable parameter part is passed in a 32-bit signed value, such as `int` or `mln_s32_t`.
- `D` - Indicates that the variable parameter part is passed in a 64-bit signed value, such as `long long` or `mln_s64_t`.
- `u` - Indicates that the variable parameter part is passed in a 32-bit unsigned value, such as `unsigned int` or `mln_u32_t`.
- `U` - Indicates that the variable parameter part is passed in a 64-bit unsigned value, such as `unsigned long long` or `mln_u64_t`.
- `F` - Indicates that the variable parameter part is passed in a value of type `double`.
- `t` - Indicates that a `true` is filled in at this position. This placeholder does not need to pass in the corresponding variable parameters.
- `f` - Indicates that a `false` is filled in at this position. This placeholder does not need to pass in the corresponding variable parameters.
- `n` - Indicates that a `null` is filled in at this position. This placeholder does not need to pass in the corresponding variable parameters.
- `s` - Indicates that the variable parameter part is passed in a `char *` type string, such as `"Hello"`.
- `S` - Indicates that the variable parameter part is passed in a `mln_string_t *` type string.
- 'c' - Indicates that the variable parameter part is passed in a `struct mln_json_call_attr *` type structure. This structure contains two members `callback` and `data`. The meaning is: the value at this position is parsed by the function `callback` with user-defined `data`.

Notes:

1. Only the above format symbols can appear in `fmt`, and there cannot be any other types of symbols such as spaces, tabs, or enters.
2. After successfully executing this function, the variable parameter corresponding to the format symbol `j` cannot be released, i.e. `mln_json_destroy` or `mln_json_reset`.

For example:

```c
mln_json_generate(&j, "[{s:d,s:d,s:{s:d}},d]", "a", 1, "b", 3, "c", "d", 4, 5);
mln_string_t *res = mln_json_encode(&j);

// The result is: [{"b":3,"c":{"d":4},"a":1},5]
```

Return value:

- `0` - on success
- `-1` - on failure



#### mln_json_object_iterate

```c
int mln_json_object_iterate(mln_json_t *j, mln_json_object_iterator_t it, void *data);

typedef int (*mln_json_object_iterator_t)(mln_json_t * /*key*/, mln_json_t * /*val*/, void *);
```

Description: Traverse each `key`-`value` pair in object `j`, and use `it` to process the key-value pair. `data` is user-defined data, which will be processed when `it` is called. and passed in.

Return value

- `0` - on success
- `-1` - on failure


#### mln_json_array_iterate

```c
mln_json_array_iterate(j, it, data);

typedef int (*mln_json_array_iterator_t)(mln_json_t *, void *);
```

Description: Traverse each element in the array `j`, and use `it` to process the array elements. `data` is user-defined data, which will be passed in when `it` is called.

Return value

- `0` - Success
- `non-zero` - Failed or other meaning. Any `non-zero` value returned by `it` will interrupt the traversal of the array



### Example

#### Example 1

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
    mln_json_t j, k;
    struct mln_json_call_attr ca;
    mln_string_t *res, exp = mln_string("protocols.0");
    mln_string_t tmp = mln_string("{\"paths\":[\"/mock\"],\"methods\":null,\"sources\":null,\"destinations\":null,\"name\":\"example_route\",\"headers\":null,\"hosts\":null,\"preserve_host\":false,\"regex_priority\":0,\"snis\":null,\"https_redirect_status_code\":426,\"tags\":null,\"protocols\":[\"http\",\"https\"],\"path_handling\":\"v0\",\"id\":\"52d58293-ae25-4c69-acc8-6dd729718a61\",\"updated_at\":1661345592,\"service\":{\"id\":\"c1e98b2b-6e77-476c-82ca-a5f1fb877e07\"},\"response_buffering\":true,\"strip_path\":true,\"request_buffering\":true,\"created_at\":1661345592}");

    if (mln_json_decode(&tmp, &j, NULL) < 0) { //Decode the string and generate the node of mln_json_t
        fprintf(stderr, "decode error\n");
        return -1;
    }

    mln_json_parse(&j, &exp, handler, NULL); //Get the first element of the array whose key is protocols in the JSON and hand it to the handler for processing

    //Fill in user-defined data parsing structure
    ca.callback = callback;
    ca.data = &i;
    //Generate JSON structure using format characters
    mln_json_init(&k);//Uninitialized json variables must be initialized before mln_json_generate
    if (mln_json_generate(&k, "[{s:d,s:d,s:{s:d}},d,[],j,c]", "a", 1, "b", 3, "c", "d", 4, 5, &j, &ca) < 0) {
        fprintf(stderr, "generate failed\n");
        return -1;
    }
    mln_json_generate(&k, "[s,d]", "g", 99);//These two elements will be added to the k array
    res = mln_json_encode(&k); //Generate corresponding JSON string for the generated structure

    mln_json_destroy(&k); //Note, do not release j here, because k will release the memory in j

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

The output of this example:

```
 type:string val:[http]
[{"b":3,"c":{"d":4},"a":1},5,[],{"preserve_host":false,"name":"example_route","destinations":null,"methods":null,"tags":null,"hosts":null,"response_buffering":true,"snis":null,"https_redirect_status_code":426,"headers":null,"request_buffering":true,"sources":null,"strip_path":true,"protocols":["http","https"],"path_handling":"v0","created_at":1661345592,"id":"52d58293-ae25-4c69-acc8-6dd729718a61","updated_at":1661345592,"paths":["/mock"],"regex_priority":0,"service":{"id":"c1e98b2b-6e77-476c-82ca-a5f1fb877e07"}},1024,"g",99]
```

The first line is the output of `mln_dump` called in `handler`. The second line is the JSON string encoded by `mln_json_encode`.


#### Example 2

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

The running result is:

```
width: 1280
height: 720
```
