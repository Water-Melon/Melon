## RSA



### Header file

```c
#include "mln_rsa.h"
```



### Module

`rsa`



### Functions/Macros



#### mln_rsa_key_new

```c
mln_rsa_key_t *mln_rsa_key_new(void);
```

Description: Create the mln_rsa_key_t` structure, which is allocated by `malloc`.

Return value: return `mln_rsa_key_t` pointer if successful, otherwise return `NULL`



#### mln_rsa_key_pool_new

```c
mln_rsa_key_t *mln_rsa_key_pool_new(mln_alloc_t *pool);
```

Description: Create the mln_rsa_key_t` structure allocated from the memory pool pointed to by `pool`.

Return value: return `mln_rsa_key_t` pointer if successful, otherwise return `NULL`



#### mln_rsa_key_free

```c
void mln_rsa_key_free(mln_rsa_key_t *key);
```

Description: Free the resources of the `mln_rsa_key_t` structure `key`, which should have been allocated by `mln_rsa_key_new`.

Return value: none



#### mln_rsa_key_pool_free

```c
void mln_rsa_key_pool_free(mln_rsa_key_t *key);
```

Description: Free the resources of the `mln_rsa_key_t` structure `key`, which should have been allocated by `mln_rsa_key_pool_new`.

Return value: none



#### mln_rsa_key_generate

```c
int mln_rsa_key_generate(mln_rsa_key_t *pub, mln_rsa_key_t *pri, mln_u32_t bits);
```

Description: Create public and private keys. `pub` is the public key, `pri` is the private key, and `bits` is the key bit length. The created key is written to the memory pointed to by the parameter pointer.

Return value: return `0` if successful, otherwise return `-1`



#### mln_RSAESPKCS1V15_public_encrypt

```c
mln_string_t *mln_RSAESPKCS1V15_public_encrypt(mln_rsa_key_t *pub, mln_string_t *text);
```

Description: Encrypt `text` using the public key `pub`.

Return value: `mln_string_t` type encrypted content, if it fails, return `NULL`



#### mln_RSAESPKCS1V15_public_decrypt

```c
mln_string_t *mln_RSAESPKCS1V15_public_decrypt(mln_rsa_key_t *pub, mln_string_t *cipher);
```

Description: Decrypt `cipher` using public key `pub`.

Return value: `mln_string_t` to decrypt the content, or `NULL` if it fails



#### mln_RSAESPKCS1V15_private_encrypt

```c
mln_string_t *mln_RSAESPKCS1V15_private_encrypt(mln_rsa_key_t *pri, mln_string_t *text);
```

Description: Encrypt `text` using the private key `pri`.

Return value: `mln_string_t` type encrypted content, if it fails, return `NULL`



#### mln_RSAESPKCS1V15_private_decrypt

```c
 mln_string_t *mln_RSAESPKCS1V15_private_decrypt(mln_rsa_key_t *pri, mln_string_t *cipher);
```

Description: Decrypt `cipher` using private key `pri`.

Return value: `mln_string_t` to decrypt the content, or `NULL` if it fails



#### mln_RSAESPKCS1V15_free

```c
void mln_RSAESPKCS1V15_free(mln_string_t *s);
```

Description: Release the encryption, decryption and signature results.

Return value: none



#### mln_RSASSAPKCS1V15_sign

```c
mln_string_t *mln_RSASSAPKCS1V15_sign(mln_alloc_t *pool, mln_rsa_key_t *pri, mln_string_t *m, mln_u32_t hash_type);
```

Description: Sign `m` with the private key `pri`, and allocate memory from the memory pool pointed to by `pool`. The hash algorithm is determined by the parameter `hash_type`:

- `M_EMSAPKCS1V15_HASH_MD5` MD5 algorithm
- `M_EMSAPKCS1V15_HASH_SHA1` SHA1 algorithm
- `M_EMSAPKCS1V15_HASH_SHA256` SHA256 algorithm

Return value: `mln_string_t` type signature content, or `NULL` on failure



#### mln_RSASSAPKCS1V15_verify

```c
int mln_RSASSAPKCS1V15_verify(mln_alloc_t *pool, mln_rsa_key_t *pub, mln_string_t *m, mln_string_t *s);
```

Description: Verifies the signature `s` of `m` using the public key `pub`.

Return value: return `0` if successful, otherwise return `-1`



#### mln_RSA_public_pwr_set

```c
mln_RSA_public_pwr_set(pkey)
```

Description: Set the pwr bit, which controls the key generation method. Not enabled by default.

Return value: none



#### mln_RSA_public_pwr_reset

```c
mln_RSA_public_pwr_reset(pkey)
```

Description: Resets the pwr bit, which controls the key generation method. Not enabled by default.

Return value: none



#### mln_RSA_public_value_ref_get

```c
mln_RSA_public_value_ref_get(pkey,pmodulus,pexponent);
```

Description: Get the address of the modulo value and exponent of the public key `pkey`.

Return value: none



#### mln_RSA_public_value_get

```c
mln_RSA_public_value_get(pkey,modulus,exponent);
```

Description: Get the modulo value and exponent of the public key `pkey`.

Return value: none



#### mln_RSA_public_value_set

```c
mln_RSA_public_value_set(pkey,modulus,exponent);
```

Description: Set the modulo value and exponent of the public key `pkey`.

Return value: none



### Example

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

