## AES



### Header file

```c
#include "mln_aes.h"
```



### Functions



#### mln_aes_init

```c
int mln_aes_init(mln_aes_t *a, mln_u8ptr_t key, mln_u32_t bits);
```

Description: Initializes the `mln_aes_t` structure, which is defined and passed in by the caller. `key` is the key, the length of the key is related to `bit`, `bit` is divided into:

- `M_AES_128` 128-bit (16-byte) key
- `M_AES_192` 192-bit (24-byte) key
- `M_AES_256` 256-bit (32-byte) key

The length of `key` must be strictly equal to the number of bits required by `bit`.

Return value: return `0` on success, otherwise return `-1`



#### mln_aes_new

```c
mln_aes_t *mln_aes_new(mln_u8ptr_t key, mln_u32_t bits);
```

Description: Create and initialize the `mln_aes_t` structure, which is allocated with `malloc`. `key` is the key, the length of the key is related to `bit`, `bit` is divided into:

- `M_AES_128` 128-bit (16-byte) key
- `M_AES_192` 192-bit (24-byte) key
- `M_AES_256` 256-bit (32-byte) key

The length of `key` must be strictly equal to the number of bits required by `bit`.

Return value: return `mln_aes_t` structure if successful, otherwise return `NULL`



#### mln_aes_pool_new

```c
mln_aes_t *mln_aes_pool_new(mln_alloc_t *pool, mln_u8ptr_t key, mln_u32_t bits);
```

Description: Create and initialize the `mln_aes_t` structure, which is allocated with the memory pool pointed to by `pool`. `key` is the key, the length of the key is related to `bit`, `bit` is divided into:

- `M_AES_128` 128-bit (16-byte) key
- `M_AES_192` 192-bit (24-byte) key
- `M_AES_256` 256-bit (32-byte) key

The length of `key` must be strictly equal to the number of bits required by `bit`.

Return value: return `mln_aes_t` structure if successful, otherwise return `NULL`



#### mln_aes_free

```c
void mln_aes_free(mln_aes_t *a);
```

Description: Free the `mln_aes_t` structure `a`, which should have been created by `mln_aes_new`.

Return value: none



#### mln_aes_pool_free

```c
void mln_aes_pool_free(mln_aes_t *a);
```

Description: Free the `mln_aes_t` structure `a`, which should have been created by `mln_aes_pool_new`.

Return value: none



#### mln_aes_encrypt

```c
int mln_aes_encrypt(mln_aes_t *a, mln_u8ptr_t text);
```

Description: AES encryption of `text` text. **Note**: `text` length must be 128 bits (16 bytes). The ciphertext will be written directly into `text`.

**Note**: Since the ciphertext will be written back to the parameter, it is necessary to ensure the writability of the parameter memory, and a segmentation fault will occur for read-only memory.

Return value: return `0` if successful, otherwise return `-1`



#### mln_aes_decrypt

```c
int mln_aes_decrypt(mln_aes_t *a, mln_u8ptr_t cipher);
```

Description: AES decrypt `cipher` text. **Note**: `cipher` length must be 128 bits (16 bytes). The plaintext will be written directly into `cipher`.

**Note**: Since the ciphertext will be written back to the parameter, it is necessary to ensure the writability of the parameter memory, and a segmentation fault will occur for read-only memory.

Return value: return `0` if successful, otherwise return `-1`



### Example

```c
#include <stdio.h>
#include <stdlib.h>
#include "mln_core.h"
#include "mln_log.h"
#include "mln_string.h"
#include "mln_aes.h"

int main(int argc, char *argv[])
{
    mln_aes_t a;
    char p[] = "1234567890123456";//128-bit. Here, if char p[] is changed to char *p, the string memory area is read-only, which will cause a segmentation fault
    mln_string_t s;
    struct mln_core_attr cattr;

    cattr.argc = argc;
    cattr.argv = argv;
    cattr.global_init = NULL;
    cattr.master_process = NULL;
    cattr.worker_process = NULL;
    if (mln_core_init(&cattr) < 0) {
        fprintf(stderr, "init failed\n");
        return -1;
    }

    if (mln_aes_init(&a, (mln_u8ptr_t)"abcdefghijklmnop", M_AES_128) < 0) {
        mln_log(error, "aes init failed\n");
        return -1;
    }

    mln_string_set(&s, p);
    if (mln_aes_encrypt(&a, s.data) < 0) {
        mln_log(error, "aes encrypt failed\n");
        return -1;
    }
    mln_log(debug, "%S\n", &s);

    if (mln_aes_decrypt(&a, s.data) < 0) {
        mln_log(error, "aes decrypt failed\n");
        return -1;
    }
    mln_log(debug, "%S\n", &s);

    return 0;
}
```

