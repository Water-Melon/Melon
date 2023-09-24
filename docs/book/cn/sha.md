## SHA



### 头文件

```c
#include "mln_sha.h"
```



### 模块名

`sha`



### 函数



#### mln_sha1_init

```c
void mln_sha1_init(mln_sha1_t *s);
```

描述：初始化`mln_sha1_t`类型结构`s`。

返回值：无



#### mln_sha1_new

```c
mln_sha1_t *mln_sha1_new(void);
```

描述：创建并初始化`mln_sha1_t`类型结构，该结构由`malloc`分配而来。

返回值：成功返回`mln_sha1_t`类型指针，否则返回`NULL`



#### mln_sha1_pool_new

```c
mln_sha1_t *mln_sha1_pool_new(mln_alloc_t *pool);
```

描述：创建并初始化`mln_sha1_t`类型结构，该结构由`pool`指定的内存池分配而来。

返回值：成功返回`mln_sha1_t`类型指针，否则返回`NULL`



#### mln_sha1_free

```c
void mln_sha1_free(mln_sha1_t *s);
```

描述：释放`mln_sha1_t`类型结构`s`的内存，该参数应由`mln_sha1_new`分配而来。

返回值：无



#### mln_sha1_pool_free

```c
void mln_sha1_pool_free(mln_sha1_t *s);
```

描述：释放`mln_sha1_t`类型结构`s`的内存，该参数应由`mln_sha1_pool_new`分配而来。

返回值：无



#### mln_sha1_calc

```c
void mln_sha1_calc(mln_sha1_t *s, mln_u8ptr_t input, mln_uauto_t len, mln_u32_t is_last);
```

描述：计算`input`和`len`所指定数据的SHA1值。本函数与MD5函数一样，支持对较大数据分批计算，最终得到一个SHA1值，`is_last`用于表明是否是本块数据的最后一批次。计算后的结果存在`s`中。

返回值：无



#### mln_sha1_tobytes

```c
void mln_sha1_tobytes(mln_sha1_t *s, mln_u8ptr_t buf, mln_u32_t len);
```

描述：将SHA1计算的二进制结果写入到`buf`与`len`指定的内存中。

返回值：无



#### mln_sha1_tostring

```c
void mln_sha1_tostring(mln_sha1_t *s, mln_s8ptr_t buf, mln_u32_t len);
```

描述：将SHA1计算的字符串结果写入到`buf`与`len`指定的内存中。

返回值：无



#### mln_sha1_dump

```c
void mln_sha1_dump(mln_sha1_t *s);
```

描述：将`mln_sha1_t`结构信息输出到标准输出上，仅用于调试。

返回值：无



#### mln_sha256_init

```c
 void mln_sha256_init(mln_sha256_t *s);
```

描述：初始化`mln_sha256_t`类型结构`s`。

返回值：无



#### mln_sha256_new

```c
mln_sha256_t *mln_sha256_new(void);
```

描述：创建并初始化`mln_sha256_t`类型结构，该结构由`malloc`分配而来。

返回值：成功返回`mln_sha256_t`类型指针，否则返回`NULL`



#### mln_sha256_pool_new

```c
mln_sha256_t *mln_sha256_pool_new(mln_alloc_t *pool);
```

描述：创建并初始化`mln_sha256_t`类型结构，该结构由`pool`指定的内存池分配而来。

返回值：成功返回`mln_sha256_t`类型指针，否则返回`NULL`



#### mln_sha256_free

```c
void mln_sha256_free(mln_sha256_t *s);
```

描述：释放`mln_sha256_t`类型结构`s`的内存，该参数应由`mln_sha256_new`分配而来。

返回值：无



#### mln_sha256_pool_free

```c
void mln_sha256_pool_free(mln_sha256_t *s);
```

描述：释放`mln_sha256_t`类型结构`s`的内存，该参数应由`mln_sha256_pool_new`分配而来。

返回值：无



#### mln_sha256_calc

```c
void mln_sha256_calc(mln_sha256_t *s, mln_u8ptr_t input, mln_uauto_t len, mln_u32_t is_last);
```

描述：计算`input`和`len`所指定数据的SHA1值。本函数与MD5函数一样，支持对较大数据分批计算，最终得到一个SHA256值，`is_last`用于表明是否是本块数据的最后一批次。计算后的结果存在`s`中。

返回值：无



#### mln_sha256_tobytes

```c
 void mln_sha256_tobytes(mln_sha256_t *s, mln_u8ptr_t buf, mln_u32_t len);
```

描述：将SHA256计算的二进制结果写入到`buf`与`len`指定的内存中。

返回值：无



#### mln_sha256_tostring

```c
void mln_sha256_tostring(mln_sha256_t *s, mln_s8ptr_t buf, mln_u32_t len);
```

描述：将SHA256计算的字符串结果写入到`buf`与`len`指定的内存中。

返回值：无



#### mln_sha256_dump

```c
void mln_sha256_dump(mln_sha256_t *s);
```

描述：将`mln_sha256_t`结构信息输出到标准输出上，仅用于调试。

返回值：无



### 示例

```c
#include <stdio.h>
#include "mln_sha.h"

int main(int argc, char *argv[])
{
    mln_sha256_t s;
    char text[1024] = {0};

    mln_sha256_init(&s);
    mln_sha256_calc(&s, (mln_u8ptr_t)"Hello", sizeof("Hello")-1, 1);
    mln_sha256_tostring(&s, text, sizeof(text)-1);
    printf("%s\n", text);

    return 0;
}
```

