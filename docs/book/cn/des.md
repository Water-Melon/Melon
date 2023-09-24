## DES



### 头文件

```c
#include "mln_des.h"
```



### 模块名

`des`



### 函数



#### mln_des_init

```c
void mln_des_init(mln_des_t *d, mln_u64_t key);
```

描述：初始化`mln_des_t`结构。`key`为8字节密钥。

返回值：无



#### mln_des_new

```c
mln_des_t *mln_des_new(mln_u64_t key);
```

描述：创建并初始化`mln_des_t`结构，该结构由`malloc`分配而来。`key`为8字节密钥。

返回值：成功则返回`mln_des_t`指针，否则返回`NULL`



#### mln_des_pool_new

```c
mln_des_t *mln_des_pool_new(mln_alloc_t *pool, mln_u64_t key);
```

描述：创建并初始化`mln_des_t`结构，该结构由`pool`指向的内存池分配而来。`key`为8字节密钥。

返回值：成功则返回`mln_des_t`指针，否则返回`NULL`



#### mln_des_free

```c
void mln_des_free(mln_des_t *d);
```

描述：释放`mln_des_t`结构内存，该结构应由`mln_des_new`分配而来。

返回值：无



#### mln_des_pool_free

```c
void mln_des_pool_free(mln_des_t *d);
```

描述：释放`mln_des_t`结构内存，该结构应由`mln_des_pool_new`分配而来。

返回值：无



#### mln_des

```c
 mln_u64_t mln_des(mln_des_t *d, mln_u64_t msg, mln_u32_t is_encrypt);
```

描述：对`msg`进行加密或者解密操作，`msg`是一个8字节大小数据。`is_encrypt`用于告知函数执行加密操作（`非0`）还是解密操作（`0`）。

返回值：8字节密文/明文



#### mln_des_buf

```c
void mln_des_buf(mln_des_t *d, mln_u8ptr_t in, mln_uauto_t inlen, mln_u8ptr_t out, mln_uauto_t outlen, mln_u8_t fill, mln_u32_t is_encrypt);
```

描述：用于对`in`和`inlen`指代的明文进行加解密操作，并将结果输出到`out`和`outlen`指代的内存区（由调用方分配）中。**注意**：`in`和`inlen`指定的输入数据必须是8字节的整数倍。若输入数据长度不足8字节时需要填充字节。填充的字节内容为`fill`。`is_encrypt`用于告知函数执行加密操作（`非0`）还是解密操作（`0`）。

**注意**：`in`和`inlen`指定的输入数据必须是8字节的整数倍。

返回值：无



#### mln_3des_init

```c
void mln_3des_init(mln_3des_t *tdes, mln_u64_t key1, mln_u64_t key2);
```

描述：初始化`mln_3des_t`结构。`key1`和`key2`均为8字节密钥。

返回值：无



#### mln_3des_new

```c
mln_3des_t *mln_3des_new(mln_u64_t key1, mln_u64_t key2);
```

描述：创建并初始化`mln_3des_t`结构，该结构由`malloc`分配而来。`key1`和`key2`均为8字节密钥。

返回值：成功则返回`mln_3des_t`指针，否则返回`NULL`



#### mln_3des_pool_new

```c
mln_3des_t *mln_3des_pool_new(mln_alloc_t *pool, mln_u64_t key1, mln_u64_t key2);
```

描述：创建并初始化`mln_3des_t`结构，该结构由`pool`指定的内存池分配而来。`key1`和`key2`均为8字节密钥。

返回值：成功则返回`mln_3des_t`指针，否则返回`NULL`



#### mln_3des_free

```c
void mln_3des_free(mln_3des_t *tdes);
```

描述：释放`mln_3des_t`结构内存，该结构应由`mln_des_new`分配而来。

返回值：无



#### mln_3des_pool_free

```c
void mln_3des_pool_free(mln_3des_t *tdes);
```

描述：释放`mln_des_t`结构内存，该结构应由`mln_des_pool_new`分配而来。

返回值：无



#### mln_3des

```c
mln_u64_t mln_3des(mln_3des_t *tdes, mln_u64_t msg, mln_u32_t is_encrypt);
```

描述：对`msg`进行加密或者解密操作，`msg`是一个8字节大小数据。`is_encrypt`用于告知函数执行加密操作（`非0`）还是解密操作（`0`）。

返回值：8字节密文/明文



#### mln_3des_buf

```c
void mln_3des_buf(mln_3des_t *tdes, mln_u8ptr_t in, mln_uauto_t inlen, mln_u8ptr_t out, mln_uauto_t outlen, mln_u8_t fill, mln_u32_t is_encrypt);
```

描述：用于对`in`和`inlen`指代的明文进行加解密操作，并将结果输出到`out`和`outlen`指代的内存区（由调用方分配）中。**注意**：`in`和`inlen`指定的输入数据必须是8字节的整数倍。若输入数据长度不足8字节时需要填充字节。填充的字节内容为`fill`。`is_encrypt`用于告知函数执行加密操作（`非0`）还是解密操作（`0`）。

返回值：无



### 示例

```c
#include <stdio.h>
#include "mln_des.h"

int main(int argc, char *argv[])
{
    mln_3des_t d;
    mln_u8_t text[9] = {0};
    mln_u8_t cipher[9] = {0};

    mln_3des_init(&d, 0xffff, 0xff120000);
    mln_3des_buf(&d, (mln_u8ptr_t)"Hi Tom!!", 11, cipher, sizeof(cipher), 0, 1);
    mln_3des_buf(&d, cipher, sizeof(cipher)-1, text, sizeof(text), 0, 0);
    printf("%s\n", text);

    return 0;
}
```

