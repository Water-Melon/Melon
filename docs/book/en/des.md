## DES



### Header file

```c
#include "mln_des.h"
```



### Module

`des`



### Functions



#### mln_des_init

```c
void mln_des_init(mln_des_t *d, mln_u64_t key);
```

Description: Initialize the `mln_des_t` structure. `key` is an 8-byte key.

Return value: none



#### mln_des_new

```c
mln_des_t *mln_des_new(mln_u64_t key);
```

Description: Create and initialize the `mln_des_t` structure allocated by `malloc`. `key` is an 8-byte key.

Return value: return `mln_des_t` pointer if successful, otherwise return `NULL`



#### mln_des_pool_new

```c
mln_des_t *mln_des_pool_new(mln_alloc_t *pool, mln_u64_t key);
```

Description: Create and initialize the `mln_des_t` structure allocated from the memory pool pointed to by `pool`. `key` is an 8-byte key.

Return value: return `mln_des_t` pointer if successful, otherwise return `NULL`



#### mln_des_free

```c
void mln_des_free(mln_des_t *d);
```

Description: Free the memory of the `mln_des_t` structure, which should have been allocated by `mln_des_new`.

Return value: none



#### mln_des_pool_free

```c
void mln_des_pool_free(mln_des_t *d);
```

Description: Free the memory of the `mln_des_t` structure, which should have been allocated by `mln_des_pool_new`.

Return value: none



#### mln_des

```c
 mln_u64_t mln_des(mln_des_t *d, mln_u64_t msg, mln_u32_t is_encrypt);
```

Description: Encrypt or decrypt `msg`, `msg` is an 8-byte data. `is_encrypt` is used to tell the function whether to perform an encryption operation (`non-0`) or a decryption operation (`0`).

Return value: 8 bytes ciphertext/plaintext



#### mln_des_buf

```c
void mln_des_buf(mln_des_t *d, mln_u8ptr_t in, mln_uauto_t inlen, mln_u8ptr_t out, mln_uauto_t outlen, mln_u8_t fill, mln_u32_t is_encrypt);
```

Description: It is used to encrypt and decrypt the plaintext indicated by `in` and `inlen`, and output the result to the memory area (allocated by the caller) indicated by `out` and `outlen`. **Note**: The input data specified by `in` and `inlen` must be an integer multiple of 8 bytes. If the input data length is less than 8 bytes, padding bytes are required. The filled byte content is `fill`. `is_encrypt` is used to tell the function whether to perform an encryption operation (`non-0`) or a decryption operation (`0`).

**Note**: The input data specified by `in` and `inlen` must be an integer multiple of 8 bytes.

Return value: none



#### mln_3des_init

```c
void mln_3des_init(mln_3des_t *tdes, mln_u64_t key1, mln_u64_t key2);
```

Description: Initialize the `mln_3des_t` structure. Both `key1` and `key2` are 8-byte keys.

Return value: none



#### mln_3des_new

```c
mln_3des_t *mln_3des_new(mln_u64_t key1, mln_u64_t key2);
```

Description: Create and initialize the `mln_3des_t` structure allocated by `malloc`. Both `key1` and `key2` are 8-byte keys.

Return value: return `mln_3des_t` pointer if successful, otherwise return `NULL`



#### mln_3des_pool_new

```c
mln_3des_t *mln_3des_pool_new(mln_alloc_t *pool, mln_u64_t key1, mln_u64_t key2);
```

Description: Create and initialize the `mln_3des_t` structure allocated from the memory pool specified by `pool`. Both `key1` and `key2` are 8-byte keys.

Return value: return `mln_3des_t` pointer if successful, otherwise return `NULL`



#### mln_3des_free

```c
void mln_3des_free(mln_3des_t *tdes);
```

Description: Free the memory of the `mln_3des_t` structure, which should have been allocated by `mln_des_new`.

Return value: none



#### mln_3des_pool_free

```c
void mln_3des_pool_free(mln_3des_t *tdes);
```

Description: Free the memory of the `mln_des_t` structure, which should have been allocated by `mln_des_pool_new`.

Return value: none



#### mln_3des

```c
mln_u64_t mln_3des(mln_3des_t *tdes, mln_u64_t msg, mln_u32_t is_encrypt);
```

Description: Encrypt or decrypt `msg`, `msg` is an 8-byte data. `is_encrypt` is used to tell the function whether to perform an encryption operation (`non-0`) or a decryption operation (`0`).

Return value: 8 bytes ciphertext/plaintext



#### mln_3des_buf

```c
void mln_3des_buf(mln_3des_t *tdes, mln_u8ptr_t in, mln_uauto_t inlen, mln_u8ptr_t out, mln_uauto_t outlen, mln_u8_t fill, mln_u32_t is_encrypt);
```

Description: It is used to encrypt and decrypt the plaintext indicated by `in` and `inlen`, and output the result to the memory area (allocated by the caller) indicated by `out` and `outlen`. **Note**: The input data specified by `in` and `inlen` must be an integer multiple of 8 bytes. If the input data length is less than 8 bytes, padding bytes are required. The filled byte content is `fill`. `is_encrypt` is used to tell the function whether to perform an encryption operation (`non-0`) or a decryption operation (`0`).

Return value: none



### Example

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

