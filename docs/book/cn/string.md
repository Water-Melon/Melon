## 字符串



### 头文件

```
mln_string.h
```



### 模块名

`string`



### 主要数据结构

```c
typedef struct {
    mln_u8ptr_t  data; //数据存放的内存起始地址
    mln_u64_t    len; //数据字节长度
    mln_uauto_t  data_ref:1; //data是否是引用
    mln_uauto_t  pool:1; //本结构是否是由内存池分配
    mln_uauto_t  ref:30; //本结构所被引用的次数
} mln_string_t;
```



### 函数/宏列表



#### mln_string

```c
mln_string(str)
```

描述：利用字符串常量`str`创建一个`mln_string_t`对象。用于定义`mln_string_t`变量的同时对其进行初始化。

返回值：`mln_string_t`类型结构体

举例：

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

描述：用于将`s`这个字符串赋值给`pstr`这个`mln_string_t`指针所指向的结构。此时，`data_ref`成员会被置1。

返回值：无

举例：

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

描述：与`mln_string_set`功能一样，只是`pstr`所指向的`mln_string_t`仅记录了`s`的前`n`个字节。

返回值：无

举例：

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

描述：将`pstr`所指向的`mln_string_t`结构的`ref`成员累加1，用于直接引用`pstr`这个内存结构。在释放内存时，引用计数大于1时是不会实际释放内存的。

返回值：`mln_string_t`类型指针

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

描述：释放`ptrs`所指向的`mln_string_t`结构内存，若`ref`大于1则仅递减引用计数，若`data_ref`为1，则不释放`data`成员指向的内存，否则释放`data`成员内存，随后释放`pstr`内存。释放时，会根据`pool`成员判断是释放回内存池，还是返还malloc库。

返回值：无



#### mln_string_new

```c
mln_string_t *mln_string_new(const char *s);
```

描述：根据字符串常量`s`创建字符串结构，此时新字符串结构及其数据部分内存均由malloc库进行分配，并将`s`的内容拷贝进`data`成员中。

返回值：成功则返回`mln_string_t`指针，否则返回`NULL`。



#### mln_string_pool_new

```c
mln_string_t *mln_string_pool_new(mln_alloc_t *pool, const char *s);
```

描述：与`mln_string_new`功能一致，仅内存是由`pool`所指向的内存池中分配而来。

返回值：成功则返回`mln_string_t`指针，否则返回`NULL`。



#### mln_string_buf_new

```c
mln_string_t *mln_string_buf_new(mln_u8ptr_t buf, mln_u64_t len);
```

描述：利用`buf`和`len`作为字符串内容，创建一个字符串结构。**注意**：`buf`应是通过malloc族函数进行分配而来的，且不可在外部进行释放。

返回值：成功则返回`mln_string_t`指针，否则返回`NULL`。



#### mln_string_buf_pool_new

```c
mln_string_t *mln_string_buf_pool_new(mln_alloc_t *pool, mln_u8ptr_t buf, mln_u64_t len);
```

描述：利用`buf`和`len`作为字符串内容，创建一个字符串结构。**注意**：`buf`应是从`pool`中分配而来的，且不可在外部进行释放。

返回值：成功则返回`mln_string_t`指针，否则返回`NULL`。



#### mln_string_dup

```c
mln_string_t *mln_string_dup(mln_string_t *str);
```

描述：完全复制一份`str`，其内存均由malloc进行分配。

返回值：成功则返回`mln_string_t`指针，否则返回`NULL`。



#### mln_string_pool_dup

```c
mln_string_t *mln_string_pool_dup(mln_alloc_t *pool, mln_string_t *str);
```

描述：与`mln_string_dup`功能一致，仅内存是从`pool`所指向的内存池中分配而来。

返回值：成功则返回`mln_string_t`指针，否则返回`NULL`。



#### mln_string_alloc

```c
mln_string_t *mln_string_alloc(mln_s32_t size);
```

描述：创建一个新字符串对象，并预分配size字节的缓冲区。

返回值：成功则返回`mln_string_t`指针，否则返回`NULL`。



#### mln_string_pool_alloc

```c
mln_string_t *mln_string_pool_alloc(mln_alloc_t *pool, mln_s32_t size);
```

描述：创建一个新字符串对象，并预分配size字节的缓冲区。与`mln_string_alloc`的差异在于所有内存均从内存池`pool`中分配。

返回值：成功则返回`mln_string_t`指针，否则返回`NULL`。



#### mln_string_const_ndup

```c
mln_string_t *mln_string_const_ndup(char *str, mln_s32_t size);
```

描述：创建一个新字符串对象，并仅复制`str`中前`size`个字节数据。

返回值：成功则返回`mln_string_t`指针，否则返回`NULL`。



#### mln_string_ref_dup

```c
mln_string_t *mln_string_ref_dup(mln_string_t *str);
```

描述：创建一个新的字符串结构，但结构中的`data`成员指向`str`中`data`成员所指向的地址，且新结构中`data_ref`会被置位。

返回值：成功则返回`mln_string_t`指针，否则返回`NULL`。



#### mln_string_const_ref_dup

```c
mln_string_t *mln_string_const_ref_dup(char *s);
```

描述：创建一个新的字符串结构，但结构中的`data`成员指向`s`，且新结构中`data_ref`会被置位。

返回值：成功则返回`mln_string_t`指针，否则返回`NULL`。



#### mln_string_concat

```c
mln_string_t *mln_string_concat(mln_string_t *s1, mln_string_t *s2, mln_string_t *sep);
```

描述：将字符串`s1`和`s2`拼接起来，并在中加增加`sep`作为分隔。

返回值：成功则返回`mln_string_t`指针，否则返回`NULL`。



#### mln_string_pool_concat

```c
mln_string_t *mln_string_pool_concat(mln_alloc_t *pool, mln_string_t *s1, mln_string_t *s2, mln_string_t *sep);
```

描述：将字符串`s1`和`s2`拼接起来，并在中加增加`sep`作为分隔。返回值指针及其数据所使用的内存均使用`pool`指定的内存池分配。

返回值：成功则返回`mln_string_t`指针，否则返回`NULL`。



#### mln_string_strseqcmp

```c
int mln_string_strseqcmp(mln_string_t *s1, mln_string_t *s2);
```

描述：比较`s1`与`s2`的数据，如果短的一方刚好与长的一方的前面完全匹配，则长的一方大于短的一方。

返回值：

- -1 - `s1`比`s2`小
-  1 - `s1`比`s2`大
-  0 -  二者相同

举例：

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

描述：比较`s1`与`s2`中数据的大小。

返回值：

- -1 - `s1`比`s2`小
-  1 - `s1`比`s2`大
-  0 -  二者相同



#### mln_string_const_strcmp

```c
int mln_string_const_strcmp(mln_string_t *s1, char *s2);
```

描述：比较`s1`所记录的数据与`s2`的大小。

返回值：

- -1 - `s1`比`s2`小
-  1 - `s1`比`s2`大
-  0 -  二者相同



#### mln_string_strncmp

```c
int mln_string_strncmp(mln_string_t *s1, mln_string_t *s2, mln_u32_t n);
```

描述：比较`s1`与`s2`的前`n`个字节的大小。

返回值：

- -1 - `s1`比`s2`小
-  1 - `s1`比`s2`大
-  0 -  二者相同



#### mln_string_const_strncmp

```c
int mln_string_const_strncmp(mln_string_t *s1, char *s2, mln_u32_t n);
```

描述：比较`s1`所记录的数据与`s2`的前`n`个字节的大小。

返回值：

- -1 - `s1`比`s2`小
-  1 - `s1`比`s2`大
-  0 -  二者相同



#### mln_string_strcasecmp

```c
int mln_string_strcasecmp(mln_string_t *s1, mln_string_t *s2);
```

描述：比较`s1`与`s2`数据的大小，且忽略大小写。

返回值：

- -1 - `s1`比`s2`小
-  1 - `s1`比`s2`大
-  0 -  二者相同



#### mln_string_const_strcasecmp

```c
int mln_string_const_strcasecmp(mln_string_t *s1, char *s2);
```

描述：比较`s1`所记录的数据与`s2`的大小，且忽略大小写。

返回值：

- -1 - `s1`比`s2`小
-  1 - `s1`比`s2`大
-  0 -  二者相同



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

描述：比较`s1`与`s2`所记录数据的前`n`个字节的大小，且忽略大小写。

返回值：

- -1 - `s1`比`s2`小
-  1 - `s1`比`s2`大
-  0 -  二者相同



#### mln_string_strstr

```c
char *mln_string_strstr(mln_string_t *text, mln_string_t *pattern);
```

描述：匹配`text`所记录的数据中与`pattern`中数据一样的起始地址。

返回值：若匹配成功，则返回`text`的`data`成员所指向地址中的对应地址；否则返回`NULL`。



#### mln_string_const_strstr

```c
char *mln_string_const_strstr(mln_string_t *text, char *pattern);
```

描述：匹配`text`所记录的数据中与`pattern`一样的起始地址。

返回值：若匹配成功，则返回`text`的`data`成员所指向地址中的对应地址；否则返回`NULL`。



#### mln_string_new_strstr

```c
mln_string_t *mln_string_new_strstr(mln_string_t *text, mln_string_t *pattern);
```

描述：与`mln_string_strstr`功能一致，但返回的是由`mln_string_t`结构包装后的字符串。

返回值：成功则返回`mln_string_t`指针，否则返回`NULL`。



#### mln_string_new_const_strstr

```c
mln_string_t *mln_string_new_const_strstr(mln_string_t *text, char *pattern);
```

描述：与`mln_string_const_strstr`功能一致，但返回的是由`mln_string_t`结构包装后的字符串。

返回值：成功则返回`mln_string_t`指针，否则返回`NULL`。



#### mln_string_kmp

```c
char *mln_string_kmp(mln_string_t *text, mln_string_t *pattern);
```

描述：与`mln_string_strstr`功能一致，但是是由KMP算法实现的。KMP算法适用场景是，`text`中有较多与`pattern`前缀相同的字符串的情况。例如: `text`中包含`aaaaaaaaaabc`，`pattern`中包含`ab`，此时，KMP算法性能将高于朴素算法。

返回值：若匹配成功，则返回`text`的`data`成员所指向地址中的对应地址；否则返回`NULL`。



#### mln_string_const_kmp

```c
char *mln_string_const_kmp(mln_string_t *text, char *pattern);
```

描述：与`mln_string_kmp`功能一致，但`pattern`为字符指针类型。

返回值：若匹配成功，则返回`text`的`data`成员所指向地址中的对应地址；否则返回`NULL`。



#### mln_string_new_kmp

```c
mln_string_t *mln_string_new_kmp(mln_string_t *text, mln_string_t *pattern);
```

描述：与`mln_string_kmp`功能一致，但返回的是由`mln_string_t`结构包装后的数据。

返回值：成功则返回`mln_string_t`指针，失败则返回`NULL`。



#### mln_string_new_const_kmp

```c
mln_string_t *mln_string_new_const_kmp(mln_string_t *text, char *pattern);
```

描述：与`mln_string_const_kmp`功能一致，但返回的是由`mln_string_t`结构包装后的数据。

返回值：成功则返回`mln_string_t`指针，失败则返回`NULL`。



#### mln_string_slice

```c
mln_string_t *mln_string_slice(mln_string_t *s, const char *sep_array/*ended by \0*/);
```

描述：`seq_array`是一个字符数组且以0结尾，该数组的每一个字符都是一个分隔标志。函数会扫描`s`的数据部分，当数据中遇到`seq_array`中的任意一个字符时都会被进行分割，连续遇到多个时仅分割一次，且分割后，分隔符不会出现在被分割后的字符串中。

返回值：成功则返回`mln_string_t`数组，否则返回`NULL`。数组的最后一个元素的`len`为`0`。

举例：

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

描述：释放由`mln_string_slice`函数创建的`mln_string_t`数组。

返回值：无



#### mln_string_strcat

```c
mln_string_t *mln_string_strcat(mln_string_t *s1, mln_string_t *s2);
```

描述：创建一个新的`mln_string_t`结构，其数据为`s1`和`s2`依此顺序拼接后的结果。

返回值：成功则返回`mln_string_t`指针，否则返回`NULL`。



#### mln_string_pool_strcat

```c
mln_string_t *mln_string_pool_strcat(mln_alloc_t *pool, mln_string_t *s1, mln_string_t *s2);
```

描述：与`mln_string_strcat`功能一致，仅新的结构所使用内存由`pool`指向的内存池分配。

返回值：成功则返回`mln_string_t`指针，否则返回`NULL`。



#### mln_string_trim

```c
mln_string_t *mln_string_trim(mln_string_t *s, mln_string_t *mask);
```

描述：去除字符串首尾处的由`mask`指定的空白字符（或者其他字符）。

返回值：由堆上分配的去除后的字符串`mln_string_t`指针。



#### mln_string_pool_trim

```c
mln_string_t *mln_string_pool_trim(mln_alloc_t *pool, mln_string_t *s, mln_string_t *mask);
```

描述：与`mln_string_trim`功能相同。

返回值：由`pool`指定的内存池上分配的去除后的字符串`mln_string_t`指针。



#### mln_string_upper

```c
void mln_string_upper(mln_string_t *s);
```

描述：将字符串`s`中的所有英文字母转换为大写。

返回值：无



#### mln_string_lower

```c
void mln_string_lower(mln_string_t *s);
```

描述：将字符串`s`中的所有英文字母转换为小写。

返回值：无
