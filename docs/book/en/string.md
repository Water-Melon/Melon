## String



### Header file

```
mln_string.h
```



### Module

`string`



### Structure

```c
typedef struct {
    mln_u8ptr_t  data; //string content
    mln_u64_t    len; //string length
    mln_uauto_t  data_ref:1; //whether data is referenced
    mln_uauto_t  pool:1; //is allocated from memory pool
    mln_uauto_t  ref:30; //reference counter
} mln_string_t;
```



### Functions/Macros



#### mln_string

```c
mln_string(str)
```

Description: Create a `mln_string_t` object using the string constant `str`. Used to initialize the `mln_string_t` variable while defining it.

Return value: `mln_string_t` type structure

Example:

```c
void foo()
{
  mln_string_t s = mln_string("Hello");
}
```



#### mln_string_set

```c
mln_string_set(pstr, s)
```

Description: Used to assign the string `s` to the structure pointed to by the `mln_string_t` pointer `pstr`. At this point, the `data_ref` member will be set to 1.

Return value: none

Example:

```c
void foo()
{
  char text[] = "hello";
  mln_string_t s;
  mln_string_set(&s, text);
}
```



#### mln_string_nset

```c
mln_string_nset(pstr, s, n)
```

Description: Same as `mln_string_set`, except that `mln_string_t` pointed to by `pstr` only records the first `n` bytes of `s`.

Return value: none

Example:

```c
void foo()
{
  char text[] = "hello world";
  mln_string_t s;
  mln_string_nset(&s, text, 5); //利用mln_log的%S进行输出时，仅会输出hello
}
```



#### mln_string_ref

```c
mln_string_ref(pstr)
```

Description: Add 1 to the `ref` member of the `mln_string_t` structure pointed to by `pstr` to directly refer to the `pstr` memory structure. When freeing memory, memory is not actually freed when the reference count is greater than 1.

Return value: pointer of type `mln_string_t`

```c
void foo(mln_string_t s)
{
  mln_string_t *ref = mln_string_ref(s); //此时ref与s的内存地址完全相同
  ...
}
```



#### mln_string_free

```c
mln_string_free(pstr)
```

Description: Release the `mln_string_t` structure memory pointed to by `ptrs`. If `ref` is greater than 1, only the reference count will be decremented. If `data_ref` is 1, the memory pointed to by the `data` member will not be released, otherwise `data` will be released. member memory, followed by freeing `pstr` memory. When releasing, it will judge whether to release back to the memory pool or return to the malloc library according to the `pool` member.

Return value: none



#### mln_string_new

```c
mln_string_t *mln_string_new(const char *s);
```

Description: Create a string structure based on the string constant `s`. At this time, the memory of the new string structure and its data part is allocated by the malloc library, and the content of `s` is copied into the `data` member.

Return value: return `mln_string_t` pointer on success, otherwise return `NULL`.



#### mln_string_pool_new

```c
mln_string_t *mln_string_pool_new(mln_alloc_t *pool, const char *s);
```

Description: Consistent with the `mln_string_new` function, only the memory is allocated from the memory pool pointed to by `pool`.

Return value: return `mln_string_t` pointer on success, otherwise return `NULL`.



#### mln_string_buf_new

```c
mln_string_t *mln_string_buf_new(mln_u8ptr_t buf, mln_u64_t len);
```

Description: Create a string structure using `buf` and `len` as string contents. **Note**: `buf` should be allocated through malloc family functions and cannot be released externally.

Return value: return `mln_string_t` pointer on success, otherwise return `NULL`.



#### mln_string_buf_pool_new

```c
mln_string_t *mln_string_buf_pool_new(mln_alloc_t *pool, mln_u8ptr_t buf, mln_u64_t len);
```

Description: Create a string structure using `buf` and `len` as string contents. **Note**: `buf` should be allocated from `pool` and cannot be freed externally.

Return value: return `mln_string_t` pointer on success, otherwise return `NULL`.



#### mln_string_dup

```c
mln_string_t *mln_string_dup(mln_string_t *str);
```

Description: A complete copy of `str` whose memory is allocated by malloc.

Return value: return `mln_string_t` pointer on success, otherwise return `NULL`.



#### mln_string_pool_dup

```c
mln_string_t *mln_string_pool_dup(mln_alloc_t *pool, mln_string_t *str);
```

Description: Consistent with the `mln_string_dup` function, only memory is allocated from the memory pool pointed to by `pool`.

Return value: return `mln_string_t` pointer on success, otherwise return `NULL`.



#### mln_string_alloc

```c
mln_string_t *mln_string_alloc(mln_s32_t size);
```

Description: Create a new string object and pre-allocate a buffer of `size` bytes.

Return value: return `mln_string_t` pointer on success, otherwise return `NULL`.



#### mln_string_pool_alloc

```c
mln_string_t *mln_string_pool_alloc(mln_alloc_t *pool, mln_s32_t size);
```

Description: Create a new string object and pre-allocate a buffer of `size` bytes. The difference with `mln_string_alloc` is that all memory is allocated from the memory pool `pool`.

Return value: return `mln_string_t` pointer on success, otherwise return `NULL`.



#### mln_string_const_ndup

```c
mln_string_t *mln_string_const_ndup(char *str, mln_s32_t size);
```

Description: Creates a new string object and copies only the first `size` bytes of data in `str`.

Return value: return `mln_string_t` pointer on success, otherwise return `NULL`.



#### mln_string_ref_dup

```c
mln_string_t *mln_string_ref_dup(mln_string_t *str);
```

Description: Create a new string structure, but the `data` member in the structure points to the address pointed to by the `data` member in `str`, and the `data_ref` in the new structure will be set.

Return value: return `mln_string_t` pointer on success, otherwise return `NULL`.



#### mln_string_const_ref_dup

```c
mln_string_t *mln_string_const_ref_dup(char *s);
```

Description: Create a new string structure, but the `data` member in the structure points to `s`, and the `data_ref` in the new structure will be set.

Return value: return `mln_string_t` pointer on success, otherwise return `NULL`.



#### mln_string_concat

```c
mln_string_t *mln_string_concat(mln_string_t *s1, mln_string_t *s2, mln_string_t *sep);
```

Description: Concatenate the strings s1 and s2, and add sep as a separator in between.

Return value: return `mln_string_t` pointer on success, otherwise return `NULL`.



#### mln_string_pool_concat

```c
mln_string_t *mln_string_pool_concat(mln_alloc_t *pool, mln_string_t *s1, mln_string_t *s2, mln_string_t *sep);
```

Description: Concatenate the strings s1 and s2, adding sep as a separator in between. The pointer to the return value and the memory used for its data are both allocated from the memory pool specified by pool.

Return value: return `mln_string_t` pointer on success, otherwise return `NULL`.



#### mln_string_strseqcmp

```c
int mln_string_strseqcmp(mln_string_t *s1, mln_string_t *s2);
```

Description: Compare the data of `s1` and `s2`, if the short side exactly matches the front of the long side, then the long side is greater than the short side.

return value:

- -1 - `s1` is smaller than `s2`
- 1 - `s1` is larger than `s2`
- 0 - both are the same

Example:

```c
int main(void)
{
  mln_string_t s1 = mln_string("abcd");
  mln_string_t s2 = mln_string("abcdefg");
  printf("%d", mln_string_strseqcmp(&s1, &s2)); //-1
  return 0;
}
```



#### mln_string_strcmp

```c
int mln_string_strcmp(mln_string_t *s1, mln_string_t *s2);
```

Description: Compare the size of data in `s1` and `s2`.

return value:

- -1 - `s1` is smaller than `s2`
- 1 - `s1` is larger than `s2`
- 0 - both are the same



#### mln_string_const_strcmp

```c
int mln_string_const_strcmp(mln_string_t *s1, char *s2);
```

Description: Compare the data recorded by `s1` with the size of `s2`.

return value:

- -1 - `s1` is smaller than `s2`
- 1 - `s1` is larger than `s2`
- 0 - both are the same



#### mln_string_strncmp

```c
int mln_string_strncmp(mln_string_t *s1, mln_string_t *s2, mln_u32_t n);
```

Description: Compare the size of the first `n` bytes of `s1` and `s2`.

return value:

- -1 - `s1` is smaller than `s2`
- 1 - `s1` is larger than `s2`
- 0 - both are the same



#### mln_string_const_strncmp

```c
int mln_string_const_strncmp(mln_string_t *s1, char *s2, mln_u32_t n);
```

Description: Compare the size of the data recorded by `s1` with the size of the first `n` bytes of `s2`.

return value:

- -1 - `s1` is smaller than `s2`
- 1 - `s1` is larger than `s2`
- 0 - both are the same



#### mln_string_strcasecmp

```c
int mln_string_strcasecmp(mln_string_t *s1, mln_string_t *s2);
```

Description: Compare the size of `s1` and `s2` data, ignoring case.

return value:

- -1 - `s1` is smaller than `s2`
- 1 - `s1` is larger than `s2`
- 0 - both are the same



#### mln_string_const_strcasecmp

```c
int mln_string_const_strcasecmp(mln_string_t *s1, char *s2);
```

Description: Compare the size of the data recorded by `s1` with that of `s2`, ignoring case.

return value:

- -1 - `s1` is smaller than `s2`
- 1 - `s1` is larger than `s2`
- 0 - both are the same



#### mln_string_const_strncasecmp

```c
int mln_string_const_strncasecmp(mln_string_t *s1, char *s2, mln_u32_t n);
```

描述：比较`s1`所记录的数据与`s2`的前`n`个字节的大小，且忽略大小写。

返回值：

- -1 - `s1`比`s2`小
-  1 - `s1`比`s2`大
-  0 -  二者相同



#### mln_string_strncasecmp

```c
int mln_string_strncasecmp(mln_string_t *s1, mln_string_t *s2, mln_u32_t n);
```

Description: Compare the size of the first `n` bytes of data recorded by `s1` and `s2`, ignoring case.

return value:

- -1 - `s1` is smaller than `s2`
- 1 - `s1` is larger than `s2`
- 0 - both are the same



#### mln_string_strstr

```c
char *mln_string_strstr(mln_string_t *text, mln_string_t *pattern);
```

Description: Match the data recorded by `text` with the same starting address as the data in `pattern`.

Return value: If the match is successful, return the corresponding address in the address pointed to by the `data` member of `text`; otherwise, return `NULL`.



#### mln_string_const_strstr

```c
char *mln_string_const_strstr(mln_string_t *text, char *pattern);
```

Description: Matches the same starting address as `pattern` in the data recorded by `text`.

Return value: If the match is successful, return the corresponding address in the address pointed to by the `data` member of `text`; otherwise, return `NULL`.



#### mln_string_new_strstr

```c
mln_string_t *mln_string_new_strstr(mln_string_t *text, mln_string_t *pattern);
```

Description: Same function as `mln_string_strstr`, but returns a string wrapped by `mln_string_t` structure.

Return value: return `mln_string_t` pointer on success, otherwise return `NULL`.



#### mln_string_new_const_strstr

```c
mln_string_t *mln_string_new_const_strstr(mln_string_t *text, char *pattern);
```

Description: Same function as `mln_string_const_strstr`, but returns a string wrapped by `mln_string_t` structure.

Return value: return `mln_string_t` pointer on success, otherwise return `NULL`.



#### mln_string_kmp

```c
char *mln_string_kmp(mln_string_t *text, mln_string_t *pattern);
```

Description: Same function as `mln_string_strstr`, but implemented by KMP algorithm. The applicable scenario of the KMP algorithm is when there are many strings with the same prefix as the `pattern` in `text`. For example: `text` contains `aaaaaaaaaabc`, `pattern` contains `ab`, at this time, the performance of KMP algorithm will be higher than that of naive algorithm.

Return value: If the match is successful, return the corresponding address in the address pointed to by the `data` member of `text`; otherwise, return `NULL`.



#### mln_string_const_kmp

```c
char *mln_string_const_kmp(mln_string_t *text, char *pattern);
```

Description: Same function as `mln_string_kmp`, but `pattern` is a character pointer type.

Return value: If the match is successful, return the corresponding address in the address pointed to by the `data` member of `text`; otherwise, return `NULL`.



#### mln_string_new_kmp

```c
mln_string_t *mln_string_new_kmp(mln_string_t *text, mln_string_t *pattern);
```

Description: Consistent with the `mln_string_kmp` function, but returns the data wrapped by the `mln_string_t` structure.

Return value: return `mln_string_t` pointer on success, `NULL` on failure.



#### mln_string_new_const_kmp

```c
mln_string_t *mln_string_new_const_kmp(mln_string_t *text, char *pattern);
```

Description: Consistent with the `mln_string_const_kmp` function, but returns the data wrapped by the `mln_string_t` structure.

Return value: return `mln_string_t` pointer on success, `NULL` on failure.



#### mln_string_slice

```c
mln_string_t *mln_string_slice(mln_string_t *s, const char *sep_array/*ended by \0*/);
```

Description: `seq_array` is a zero-terminated array of characters, where each character of the array is a delimiter. The function will scan the data `s`. When any character in `seq_array` is encountered in the data, it will be divided. When multiple characters are encountered in a row, it will only be divided once, and after division, the delimiter will not appear in the in the split string.

Return value: return `mln_string_t` array on success, otherwise return `NULL`. The `len` of the last element of the array is `0`.

Example:

```c
int main(void)
{
  mln_string_t s = mln_string("abc-def-=ghi");
  mln_string_t *str, *arr = mln_string_slice(&s, "-=");
  for (str = arr; str->len; ++str) {
    mln_log(debug, "%S", str);
  }
  mln_string_slice_free(arr);
  return 0;
}
```



#### mln_string_slice_free

```c
void mln_string_slice_free(mln_string_t *array);
```

Description: Free the `mln_string_t` array created by the `mln_string_slice` function.

Return value: none



#### mln_string_strcat

```c
mln_string_t *mln_string_strcat(mln_string_t *s1, mln_string_t *s2);
```

Description: Create a new `mln_string_t` structure whose data is the result of concatenating `s1` and `s2` in this order.

Return value: return `mln_string_t` pointer on success, otherwise return `NULL`.



#### mln_string_pool_strcat

```c
mln_string_t *mln_string_pool_strcat(mln_alloc_t *pool, mln_string_t *s1, mln_string_t *s2);
```

Description: Consistent with the `mln_string_strcat` function, only the memory used by the new structure is allocated by the memory pool pointed to by `pool`.

Return value: return `mln_string_t` pointer on success, otherwise return `NULL`.



#### mln_string_trim

```c
mln_string_t *mln_string_trim(mln_string_t *s, mln_string_t *mask);
```

Description: Remove whitespace (or other characters) specified by `mask` at the beginning and end of the string.

Return value: pointer to the stripped string `mln_string_t` allocated by the heap.



#### mln_string_pool_trim

```c
mln_string_t *mln_string_pool_trim(mln_alloc_t *pool, mln_string_t *s, mln_string_t *mask);
```

Description: Same function as `mln_string_trim`.

Return value: The stripped string `mln_string_t` pointer allocated on the memory pool specified by `pool`.



#### mln_string_upper

```c
void mln_string_upper(mln_string_t *s);
```

Description: Convert all English letters in the string `s` to uppercase.

Return value: none



#### mln_string_lower

```c
void mln_string_lower(mln_string_t *s);
```

Description: Convert all English letters in the string `s` to lowercase.

Return value: none
