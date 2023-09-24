## MD5



### 头文件

```c
#include "mln_md5.h"
```



### 模块名

`md5`



### 函数



#### mln_md5_init

```c
void mln_md5_init(mln_md5_t *m);
```

描述：初始化`mln_md5_t`结构`m`。

返回值：无



#### mln_md5_new

```c
mln_md5_t *mln_md5_new(void);
```

描述：创建并初始化`mln_md5_t`结构，该结构由`malloc`分配而来。

返回值：成功则返回`mln_md5_t`结构指针，否则返回`NULL`



#### mln_md5_pool_new

```c
mln_md5_t *mln_md5_pool_new(mln_alloc_t *pool);
```

描述：创建并初始化`mln_md5_t`结构，该结构由`pool`指定的内存池分配而来。

返回值：成功则返回`mln_md5_t`结构指针，否则返回`NULL`



#### mln_md5_free

```c
void mln_md5_free(mln_md5_t *m);
```

描述：释放`mln_md5_t`结构`m`的内存，`m`应由`mln_md5_new`创建而来。

返回值：无



#### mln_md5_pool_free

```c
void mln_md5_pool_free(mln_md5_t *m);
```

描述：释放`mln_md5_t`结构`m`的内存，`m`应由`mln_md5_pool_new`创建而来。

返回值：无



#### mln_md5_calc

```c
void mln_md5_calc(mln_md5_t *m, mln_u8ptr_t input, mln_uauto_t len, mln_u32_t is_last);
```

描述：计算md5值。该函数可以被多次调用用来生成一个MD5值。其中：

- `m` 由上述函数初始化而来
- `input`要被计算MD5的数据
- `len` `input`的长度
- `is_last` 是否是最后一次调用。由于可以对一个较大数据分多次进行计算出一个MD5值，因此用该参数表示是否是最后一批数据。

被计算的MD5被存放在`m`中。

返回值：无



#### mln_md5_tobytes

```c
void mln_md5_tobytes(mln_md5_t *m, mln_u8ptr_t buf, mln_u32_t len);
```

描述：将计算好的MD5值以二进制形式写入`buf`和`len`指定的内存区中。

返回值：无



#### mln_md5_tostring

```c
void mln_md5_tostring(mln_md5_t *m, mln_s8ptr_t buf, mln_u32_t len);
```

描述：将计算好的MD5值以字符串形式写入`buf`和`len`指定的内存区中。

返回值：无



#### mln_md5_dump

```c
void mln_md5_dump(mln_md5_t *m);
```

描述：将MD5相关信息输出到标准输出上。

返回值：无



### 示例

```c
#include <stdio.h>
#include "mln_md5.h"

int main(int argc, char *argv[])
{
    mln_md5_t m;
    char text[] = "Hello";
    char output[33] = {0};//MD5计算结果一共16字节，改为字符串输出则是二进制的两倍，因此是32字节，多一字节用于\0

    mln_md5_init(&m);
    mln_md5_calc(&m, (mln_u8ptr_t)text, sizeof(text)-1, 1);
    mln_md5_tostring(&m, output, sizeof(output));
    printf("%s\n", output);

    return 0;
}
```

