## AES



### 头文件

```c
#include "mln_aes.h"
```



### 模块名

`aes`



### 函数



#### mln_aes_init

```c
int mln_aes_init(mln_aes_t *a, mln_u8ptr_t key, mln_u32_t bits);
```

描述：初始化`mln_aes_t`结构，该结构由调用方定义并传入。`key`为密钥，密钥长度与`bit`有关，`bit`分为:

- `M_AES_128`128位（16字节）密钥
- `M_AES_192`192位（24字节）密钥
- `M_AES_256`256位（32字节）密钥

`key`长度必须严格与`bit`要求位数相等。

返回值：成功返回`0`，否则返回`-1`



#### mln_aes_new

```c
mln_aes_t *mln_aes_new(mln_u8ptr_t key, mln_u32_t bits);
```

描述：新建并初始化`mln_aes_t`结构，该结构有`malloc`进行分配。`key`为密钥，密钥长度与`bit`有关，`bit`分为:

- `M_AES_128`128位（16字节）密钥
- `M_AES_192`192位（24字节）密钥
- `M_AES_256`256位（32字节）密钥

`key`长度必须严格与`bit`要求位数相等。

返回值：成功则返回`mln_aes_t`结构，否则返回`NULL`



#### mln_aes_pool_new

```c
mln_aes_t *mln_aes_pool_new(mln_alloc_t *pool, mln_u8ptr_t key, mln_u32_t bits);
```

描述：新建并初始化`mln_aes_t`结构，该结构有`pool`指向的内存池进行分配。`key`为密钥，密钥长度与`bit`有关，`bit`分为:

- `M_AES_128`128位（16字节）密钥
- `M_AES_192`192位（24字节）密钥
- `M_AES_256`256位（32字节）密钥

`key`长度必须严格与`bit`要求位数相等。

返回值：成功则返回`mln_aes_t`结构，否则返回`NULL`



#### mln_aes_free

```c
void mln_aes_free(mln_aes_t *a);
```

描述：释放`mln_aes_t`结构`a`，`a`应由`mln_aes_new`创建而来。

返回值：无



#### mln_aes_pool_free

```c
void mln_aes_pool_free(mln_aes_t *a);
```

描述：释放`mln_aes_t`结构`a`，`a`应由`mln_aes_pool_new`创建而来。

返回值：无



#### mln_aes_encrypt

```c
int mln_aes_encrypt(mln_aes_t *a, mln_u8ptr_t text);
```

描述：对`text`文本进行AES加密。**注意**：`text`长度必须为128位（16字节）。密文会直接写入`text`内。

**注意**：由于这里会将密文写回参数中，因此需要确保参数内存可写性，对于只读内存会出现段错误。

返回值：成功则返回`0`，否则返回`-1`



#### mln_aes_decrypt

```c
int mln_aes_decrypt(mln_aes_t *a, mln_u8ptr_t cipher);
```

描述：对`cipher`文本进行AES解密。**注意**：`cipher`长度必须为128位（16字节）。明文会直接写入`cipher`内。

**注意**：由于这里会将密文写回参数中，因此需要确保参数内存可写性，对于只读内存会出现段错误。

返回值：成功则返回`0`，否则返回`-1`



### 示例

```c
#include <stdio.h>
#include <stdlib.h>
#include "mln_string.h"
#include "mln_aes.h"

int main(int argc, char *argv[])
{
    mln_aes_t a;
    char p[] = "1234567890123456";//128-bit 这里如果将char p[] 改为 char *p，则字符串内存区为只读，会导致段错误
    mln_string_t s;

    if (mln_aes_init(&a, (mln_u8ptr_t)"abcdefghijklmnop", M_AES_128) < 0) {
        fprintf(stderr, "aes init failed\n");
        return -1;
    }

    mln_string_set(&s, p);
    if (mln_aes_encrypt(&a, s.data) < 0) {
        fprintf(stderr, "aes encrypt failed\n");
        return -1;
    }
    write(STDOUT_FILENO, s.data, s.len);
    write(STDOUT_FILENO, "\n", 1);

    if (mln_aes_decrypt(&a, s.data) < 0) {
        fprintf(stderr, "aes decrypt failed\n");
        return -1;
    }
    write(STDOUT_FILENO, s.data, s.len);
    write(STDOUT_FILENO, "\n", 1);

    return 0;
}
```

