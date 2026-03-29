## JSON



### Header File

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

Description: Initialize JSON type node `j`, setting its type to `NONE`.

Return value: None



#### mln_json_string_init

```c
mln_json_string_init(j, s)
```

Description: Initialize JSON type node `j` to string type and assign value `s` of type `mln_string_t *`. The JSON structure takes ownership of `s`; the caller must ensure `s` remains valid and must not modify its content.

Return value: None



#### mln_json_number_init

```c
mln_json_number_init(j, n)
```

Description: Initialize JSON type node `j` to number type and assign a `double` value `n`.

Return value: None



#### mln_json_true_init

```c
mln_json_true_init(j)
```

Description: Initialize JSON type node `j` to `true` type.

Return value: None



#### mln_json_false_init

```c
mln_json_false_init(j)
```

Description: Initialize JSON type node `j` to `false` type.

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

Description: Parse the JSON string `jstr` into a data structure, storing the result in `out`. The `policy` parameter is a security policy structure initialized by `mln_json_policy_init`. On failure, `out` is automatically cleaned up — do not call `mln_json_destroy` on it.

Return value:

- `0` - on success
- `-1` - on failure. If `policy` is not `NULL`, use `mln_json_policy_error` to check which security constraint was violated.



#### mln_json_destroy

```c
void mln_json_destroy(mln_json_t *j);
```

Description: Free all memory held by the `mln_json_t` node `j`.

Return value: None



#### mln_json_dump

```c
void mln_json_dump(mln_json_t *j, int n_space, char *prefix);
```

Description: Print the details of JSON node `j` to standard output. `n_space` specifies the current indentation level in spaces, and `prefix` is prepended to the output.

Return value: None



#### mln_json_encode

```c
mln_string_t *mln_json_encode(mln_json_t *j, mln_u32_t flags);
```

Description: Serialize the `mln_json_t` node structure into a JSON text string. The caller must free the returned value with `mln_string_free`.

Flags:
- `0` - Output as standard JSON text format (default)
- `M_JSON_ENCODE_UNICODE` (0x1) - Escape non-ASCII characters (e.g. Chinese) to `\uXXXX` form

Return value: An `mln_string_t` pointer on success, or `NULL` on failure



#### mln_json_obj_search

```c
mln_json_t *mln_json_obj_search(mln_json_t *j, mln_string_t *key);
```

Description: Search for the value associated with `key` in the object type node `j`.

Return value: The `mln_json_t` value on success, or `NULL` if not found



#### mln_json_array_search

```c
mln_json_t *mln_json_array_search(mln_json_t *j, mln_uauto_t index);
```

Description: Retrieve the element at index `index` from the array type node `j`.

Return value: The `mln_json_t` element on success, or `NULL` if the index is out of range



#### mln_json_array_length

```c
mln_uauto_t mln_json_array_length(mln_json_t *j);
```

Description: Get the length of the array. `j` must be of array type.

Return value: Array length



#### mln_json_obj_update

```c
int mln_json_obj_update(mln_json_t *j, mln_json_t *key, mln_json_t *val);
```

Description: Add the `key`-`val` pair to the object type JSON node `j`. If `key` already exists, the existing key and value are replaced. The function copies the contents of `key` and `val` internally; the caller does not need to heap-allocate them.

Return value: `0` on success, `-1` on failure



#### mln_json_obj_element_num

```c
mln_size_t mln_json_obj_element_num(mln_json_t *j);
```

Description: Get the number of key-value pairs in the object type JSON node `j`.

Return value: Number of key-value pairs



#### mln_json_array_append

```c
int mln_json_array_append(mln_json_t *j, mln_json_t *value);
```

Description: Append `value` to the end of the array type JSON node `j`. The function copies the contents of `value` internally.

Return value: `0` on success, `-1` on failure



#### mln_json_array_update

```c
int mln_json_array_update(mln_json_t *j, mln_json_t *value, mln_uauto_t index);
```

Description: Replace the element at index `index` in the array type JSON node `j` with `value`. Fails if the index does not exist. The function copies the contents of `value` internally.

Return value: `0` on success, `-1` on failure



#### mln_json_reset

```c
void mln_json_reset(mln_json_t *j);
```

Description: Reset JSON node `j` by releasing its data and restoring its type to `NONE`.

Return value: None



#### mln_json_obj_remove

```c
void mln_json_obj_remove(mln_json_t *j, mln_string_t *key);
```

Description: Remove and free the key-value pair with key `key` from the object type JSON node `j`.

Return value: None



#### mln_json_array_remove

```c
void mln_json_array_remove(mln_json_t *j, mln_uauto_t index);
```

Description: Remove and free the last element from the array type JSON node `j`. The `index` parameter must equal the last valid index (i.e., `length - 1`); it is used only for an assertion check in debug builds.

Return value: None



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

Description: Check the type of the `mln_json_t` node `json`. The types are, in order: object, array, string, number, true, false, null, none.

Return value: Non-zero if the condition is met, `0` otherwise



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

Description: Set the type for the `mln_json_t` node `json`. The types are, in order: none, object, array, string, number, true, false, null.

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

Description: Get the data portion of the corresponding type from the `mln_json_t` node `json`.

Return value:

- Object type: `mln_json_obj_t` pointer
- Array type: `mln_array_t` pointer
- String type: `mln_string_t` pointer
- Number type: `double` value
- True type: `mln_u8_t` value
- False type: `mln_u8_t` value
- Null type: `mln_u8ptr_t` NULL value



#### mln_json_policy_init

```c
mln_json_policy_init(policy, _depth, _keylen, _strlen, _elemnum, _kvnum);
```

Description: Initialize the `mln_json_policy_t` variable `policy`. The parameters are:

- `_depth` - Maximum nesting depth. During parsing, the depth increases when entering an array or object, and decreases when leaving. A value of `0` means no limit.

- `_keylen` - Maximum length of object keys. A value of `0` means no limit.

- `_strlen` - Maximum length of string values. A value of `0` means no limit.

- `_elemnum` - Maximum number of array elements. A value of `0` means no limit.

- `_kvnum` - Maximum number of key-value pairs in an object. A value of `0` means no limit.

Return value: None



#### mln_json_policy_error

```c
mln_json_policy_error(policy)
```

Description: Get the error code from the `mln_json_policy_t` variable `policy`. Typically used to check which security constraint was violated when `mln_json_decode` returns `-1`.

Return value: An `int` error code with the following possible values:

- `M_JSON_OK` - No security policy violation.

- `M_JSON_DEPTH` - Nesting depth exceeded.

- `M_JSON_KEYLEN` - Object key length exceeded.

- `M_JSON_STRLEN` - String value length exceeded.

- `M_JSON_ARRELEM` - Array element count exceeded.

- `M_JSON_OBJKV` - Object key-value pair count exceeded.



#### mln_json_fetch

```c
int mln_json_fetch(mln_json_t *j, mln_string_t *exp, mln_json_iterator_t iterator, void *data)

typedef int (*mln_json_iterator_t)(mln_json_t *, void *);
```

Description:

Navigate the JSON tree rooted at `j` using the dot-separated expression `exp` to locate a matching child node. If a match is found and `iterator` is not `NULL`, the `iterator` callback is invoked with the matched node. `data` is user-defined data passed to `iterator`.

Given the following JSON:

```
{
  "a": [1, {
    "c": 3
  }],
  "b": 2
}
```

Example expressions:

| Expression | Meaning                                                      |
| ---------- | ------------------------------------------------------------ |
| `a`        | Get the value with key `a` (each value is an `mln_json_t` node) |
| `b`        | Get the value with key `b`                                   |
| `a.0`      | Get the element at index `0` of the array under key `a`      |
| `a.1.c`    | Navigate into the array under key `a`, then element at index `1` (an object), then get the value with key `c` |
| `a.2.c`    | Element at index `2` does not exist, so `mln_json_fetch` returns `-1` |

Return value:

- `0` - on success
- `-1` - on failure



#### mln_json_generate

```c
int mln_json_generate(mln_json_t *j, char *fmt, ...);
```

Description:

Populate `j` according to the format string `fmt` using the subsequent variadic arguments.

Supported format specifiers in `fmt`:

- `{}` - Object type. Keys and values are separated by `:`, key-value pairs are separated by `,`.
- `[]` - Array type. Elements are separated by `,`.
- `j` - The variadic argument is a `mln_json_t` pointer.
- `d` - The variadic argument is a 32-bit signed integer (e.g. `int`, `mln_s32_t`).
- `D` - The variadic argument is a 64-bit signed integer (e.g. `long long`, `mln_s64_t`).
- `u` - The variadic argument is a 32-bit unsigned integer (e.g. `unsigned int`, `mln_u32_t`).
- `U` - The variadic argument is a 64-bit unsigned integer (e.g. `unsigned long long`, `mln_u64_t`).
- `F` - The variadic argument is a `double` value.
- `t` - Insert a `true` value. No corresponding variadic argument is needed.
- `f` - Insert a `false` value. No corresponding variadic argument is needed.
- `n` - Insert a `null` value. No corresponding variadic argument is needed.
- `s` - The variadic argument is a `char *` string.
- `S` - The variadic argument is a `mln_string_t *` string.
- `c` - The variadic argument is a `struct mln_json_call_attr *` containing a `callback` function and `data` pointer. The value at this position is produced by invoking `callback` with `data`.

Notes:

1. Only the format specifiers listed above may appear in `fmt`. No spaces, tabs, newlines, or other characters are allowed.
2. After a successful call, the JSON node passed via the `j` format specifier is taken over by the output. The caller must not call `mln_json_destroy` or `mln_json_reset` on it.

Example:

```c
mln_json_init(&j);
mln_json_generate(&j, "[{s:d,s:d,s:{s:d}},d]", "a", 1, "b", 3, "c", "d", 4, 5);
mln_string_t *res = mln_json_encode(&j, 0);

// The result is: [{"a":1,"b":3,"c":{"d":4}},5]
```

Return value:

- `0` - on success
- `-1` - on failure



#### mln_json_object_iterate

```c
int mln_json_object_iterate(mln_json_t *j, mln_json_object_iterator_t it, void *data);

typedef int (*mln_json_object_iterator_t)(mln_json_t * /*key*/, mln_json_t * /*val*/, void *);
```

Description: Iterate over each key-value pair in the object `j`, invoking `it` for each pair. `data` is user-defined data passed to `it`.

Return value:

- `0` - on success
- `-1` - on failure



#### mln_json_array_iterate

```c
mln_json_array_iterate(j, it, data);

typedef int (*mln_json_array_iterator_t)(mln_json_t *, void *);
```

Description: Iterate over each element in the array `j`, invoking `it` for each element. `data` is user-defined data passed to `it`.

Return value:

- `0` - on success
- Non-zero - Failure or early termination. Any non-zero value returned by `it` stops the iteration.






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
    int i = 1024;
    mln_json_t j, k;
    struct mln_json_call_attr ca;
    mln_string_t *res, exp = mln_string("protocols.0");
    mln_string_t tmp = mln_string("{\"paths\":[\"/mock\"],\"methods\":null,\"sources\":null,\"destinations\":null,\"name\":\"example_route\",\"headers\":null,\"hosts\":null,\"preserve_host\":false,\"regex_priority\":0,\"snis\":null,\"https_redirect_status_code\":426,\"tags\":null,\"protocols\":[\"http\",\"https\"],\"path_handling\":\"v0\",\"id\":\"52d58293-ae25-4c69-acc8-6dd729718a61\",\"updated_at\":1661345592,\"service\":{\"id\":\"c1e98b2b-6e77-476c-82ca-a5f1fb877e07\"},\"response_buffering\":true,\"strip_path\":true,\"request_buffering\":true,\"created_at\":1661345592}");

    /* Decode the string into an mln_json_t node */
    if (mln_json_decode(&tmp, &j, NULL) < 0) {
        fprintf(stderr, "decode error\n");
        return -1;
    }

    /* Get the first element of the array under key "protocols" */
    mln_json_fetch(&j, &exp, handler, NULL);

    /* Set up the user-defined callback structure */
    ca.callback = callback;
    ca.data = &i;

    /* Always initialize before calling mln_json_generate */
    mln_json_init(&k);
    if (mln_json_generate(&k, "[{s:d,s:d,s:{s:d}},d,[],j,c]", "a", 1, "b", 3, "c", "d", 4, 5, &j, &ca) < 0) {
        fprintf(stderr, "generate failed\n");
        return -1;
    }
    /* These two elements are appended to the k array */
    mln_json_generate(&k, "[s,d]", "g", 99);
    /* Encode the structure to a JSON string */
    res = mln_json_encode(&k, 0);

    /* Do not destroy j separately — its contents are now owned by k */
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

Output:

```
 type:string val:[http]
[{"a":1,"b":3,"c":{"d":4}},5,[],{"paths":["/mock"],"methods":null,"sources":null,"destinations":null,"name":"example_route","headers":null,"hosts":null,"preserve_host":false,"regex_priority":0,"snis":null,"https_redirect_status_code":426,"tags":null,"protocols":["http","https"],"path_handling":"v0","id":"52d58293-ae25-4c69-acc8-6dd729718a61","updated_at":1661345592,"service":{"id":"c1e98b2b-6e77-476c-82ca-a5f1fb877e07"},"response_buffering":true,"strip_path":true,"request_buffering":true,"created_at":1661345592},1024,"g",99]
```

The first line is the output from `mln_json_dump` in `handler`. The second line is the JSON string produced by `mln_json_encode`.


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

Output:

```
width: 1280
height: 720
```
