## RSA



### 头文件

```c
#include "mln_rsa.h"
```



### 模块名

`rsa`



### 函数/宏



#### mln_rsa_key_new

```c
mln_rsa_key_t *mln_rsa_key_new(void);
```

描述：创建mln_rsa_key_t`结构，该结构由`malloc`分配而来。

返回值：成功则返回`mln_rsa_key_t`指针，否则返回`NULL`



#### mln_rsa_key_pool_new

```c
mln_rsa_key_t *mln_rsa_key_pool_new(mln_alloc_t *pool);
```

描述：创建mln_rsa_key_t`结构，该结构由`pool`指向的内存池分配而来。

返回值：成功则返回`mln_rsa_key_t`指针，否则返回`NULL`



#### mln_rsa_key_free

```c
void mln_rsa_key_free(mln_rsa_key_t *key);
```

描述：释放`mln_rsa_key_t`结构`key`的资源，该结构应由`mln_rsa_key_new`分配而来。

返回值：无



#### mln_rsa_key_pool_free

```c
void mln_rsa_key_pool_free(mln_rsa_key_t *key);
```

描述：释放`mln_rsa_key_t`结构`key`的资源，该结构应由`mln_rsa_key_pool_new`分配而来。

返回值：无



#### mln_rsa_key_generate

```c
int mln_rsa_key_generate(mln_rsa_key_t *pub, mln_rsa_key_t *pri, mln_u32_t bits);
```

描述：创建公私钥。`pub`为公钥，`pri`为私钥，`bits`为密钥位长度。创建后的密钥会被写入参数指针所指向的内存中。

返回值：成功则返回`0`，否则返回`-1`



#### mln_RSAESPKCS1V15_public_encrypt

```c
mln_string_t *mln_RSAESPKCS1V15_public_encrypt(mln_rsa_key_t *pub, mln_string_t *text);
```

描述：使用公钥`pub`对`text`进行加密。

返回值：`mln_string_t`类型加密内容，失败则返回`NULL`



#### mln_RSAESPKCS1V15_public_decrypt

```c
mln_string_t *mln_RSAESPKCS1V15_public_decrypt(mln_rsa_key_t *pub, mln_string_t *cipher);
```

描述：使用公钥`pub`对`cipher`进行解密。

返回值：`mln_string_t`类型解密内容，失败则返回`NULL`



#### mln_RSAESPKCS1V15_private_encrypt

```c
mln_string_t *mln_RSAESPKCS1V15_private_encrypt(mln_rsa_key_t *pri, mln_string_t *text);
```

描述：使用私钥`pri`对`text`进行加密。

返回值：`mln_string_t`类型加密内容，失败则返回`NULL`



#### mln_RSAESPKCS1V15_private_decrypt

```c
 mln_string_t *mln_RSAESPKCS1V15_private_decrypt(mln_rsa_key_t *pri, mln_string_t *cipher);
```

描述：使用私钥`pri`对`cipher`进行解密。

返回值：`mln_string_t`类型解密内容，失败则返回`NULL`



#### mln_RSAESPKCS1V15_free

```c
void mln_RSAESPKCS1V15_free(mln_string_t *s);
```

描述：对加解密和签名结果进行释放。

返回值：无



#### mln_RSASSAPKCS1V15_sign

```c
mln_string_t *mln_RSASSAPKCS1V15_sign(mln_alloc_t *pool, mln_rsa_key_t *pri, mln_string_t *m, mln_u32_t hash_type);
```

描述：使用私钥`pri`对`m`进行签名，签名结果从`pool`指向的内存池中分配内存。其中哈希算法由参数`hash_type`决定：

- `M_EMSAPKCS1V15_HASH_MD5` MD5算法
- `M_EMSAPKCS1V15_HASH_SHA1` SHA1算法
- `M_EMSAPKCS1V15_HASH_SHA256` SHA256算法

返回值：`mln_string_t`类型签名内容，失败则返回`NULL`



#### mln_RSASSAPKCS1V15_verify

```c
int mln_RSASSAPKCS1V15_verify(mln_alloc_t *pool, mln_rsa_key_t *pub, mln_string_t *m, mln_string_t *s);
```

描述：使用公钥`pub`对`m`的签名`s`进行核验。

返回值：成功则返回`0`，否则返回`-1`



#### mln_RSA_public_pwr_set

```c
mln_RSA_public_pwr_set(pkey)
```

描述：设置pwr位，该位控制着密钥生成方法。默认不启用。

返回值：无



#### mln_RSA_public_pwr_reset

```c
mln_RSA_public_pwr_reset(pkey)
```

描述：复位pwr位，该位控制着密钥生成方法。默认不启用。

返回值：无



#### mln_RSA_public_value_ref_get

```c
mln_RSA_public_value_ref_get(pkey,pmodulus,pexponent);
```

描述：获取公钥`pkey`的取模值与指数的地址。

返回值：无



#### mln_RSA_public_value_get

```c
mln_RSA_public_value_get(pkey,modulus,exponent);
```

描述：获取公钥`pkey`的取模值与指数。

返回值：无



#### mln_RSA_public_value_set

```c
mln_RSA_public_value_set(pkey,modulus,exponent);
```

描述：设置公钥`pkey`的取模值与指数。

返回值：无



### 示例

```c
#include <stdio.h>
#include <stdlib.h>
#include "mln_rsa.h"

int main(int argc, char *argv[])
{
    char s[] = "Hello";
    mln_string_t tmp, *cipher = NULL, *text = NULL;
    mln_rsa_key_t *pub = NULL, *pri = NULL;

    pub = mln_rsa_key_new();
    pri = mln_rsa_key_new();
    if (pri == NULL || pub == NULL) {
        fprintf(stderr, "new pub/pri key failed\n");
        goto failed;
    }

    if (mln_rsa_key_generate(pub, pri, 1024) < 0) {
        fprintf(stderr, "key generate failed\n");
        goto failed;
    }

    mln_string_set(&tmp, s);
    cipher = mln_RSAESPKCS1V15_public_encrypt(pub, &tmp);
    if (cipher == NULL) {
        fprintf(stderr, "pub key encrypt failed\n");
        goto failed;
    }
    write(STDOUT_FILENO, cipher->data, cipher->len);
    write(STDOUT_FILENO, "\n", 1);

    text = mln_RSAESPKCS1V15_private_decrypt(pri, cipher);
    if (text == NULL) {
        fprintf(stderr, "pri key decrypt failed\n");
        goto failed;
    }
    write(STDOUT_FILENO, text->data, text->len);
    write(STDOUT_FILENO, "\n", 1);

failed:
    if (pub != NULL) mln_rsa_key_free(pub);
    if (pri != NULL) mln_rsa_key_free(pri);
    if (cipher != NULL) mln_RSAESPKCS1V15_free(cipher);
    if (text != NULL) mln_RSAESPKCS1V15_free(text);

    return 0;
}
```

