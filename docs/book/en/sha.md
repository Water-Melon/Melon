## SHA



### Header file

```c
#include "mln_sha.h"
```



### Module

`sha`



### Functions



#### mln_sha1_init

```c
void mln_sha1_init(mln_sha1_t *s);
```

Description: Initialize `mln_sha1_t` type structure `s`.

Return value: none



#### mln_sha1_new

```c
mln_sha1_t *mln_sha1_new(void);
```

Description: Create and initialize a structure of type `mln_sha1_t` allocated by `malloc`.

Return value: return `mln_sha1_t` type pointer successfully, otherwise return `NULL`



#### mln_sha1_pool_new

```c
mln_sha1_t *mln_sha1_pool_new(mln_alloc_t *pool);
```

Description: Create and initialize a structure of type `mln_sha1_t`, which is allocated from the memory pool specified by `pool`.

Return value: return `mln_sha1_t` type pointer successfully, otherwise return `NULL`



#### mln_sha1_free

```c
void mln_sha1_free(mln_sha1_t *s);
```

Description: Free the memory of `mln_sha1_t` type structure `s`, which should be allocated by `mln_sha1_new`.

Return value: none



#### mln_sha1_pool_free

```c
void mln_sha1_pool_free(mln_sha1_t *s);
```

Description: Free the memory of `mln_sha1_t` type structure `s`, which should be allocated by `mln_sha1_pool_new`.

Return value: none



#### mln_sha1_calc

```c
void mln_sha1_calc(mln_sha1_t *s, mln_u8ptr_t input, mln_uauto_t len, mln_u32_t is_last);
```

Description: Calculates the SHA1 value of the data specified by `input` and `len`. This function, like the MD5 function, supports batch calculation of larger data, and finally obtains a SHA1 value. `is_last` is used to indicate whether it is the last batch of this block of data. The calculated result is stored in `s`.

Return value: none



#### mln_sha1_tobytes

```c
void mln_sha1_tobytes(mln_sha1_t *s, mln_u8ptr_t buf, mln_u32_t len);
```

Description: Write the binary result of the SHA1 calculation to the memory specified by `buf` and `len`.

Return value: none



#### mln_sha1_tostring

```c
void mln_sha1_tostring(mln_sha1_t *s, mln_s8ptr_t buf, mln_u32_t len);
```

Description: Write the string result of SHA1 calculation to the memory specified by `buf` and `len`.

Return value: none



#### mln_sha1_dump

```c
void mln_sha1_dump(mln_sha1_t *s);
```

Description: Print `mln_sha1_t` structure information to stdout, for debugging only.

Return value: none



#### mln_sha256_init

```c
 void mln_sha256_init(mln_sha256_t *s);
```

Description: Initialize `mln_sha256_t` type structure `s`.

Return value: none



#### mln_sha256_new

```c
mln_sha256_t *mln_sha256_new(void);
```

Description: Create and initialize a structure of type `mln_sha256_t` allocated by `malloc`.

Return value: return `mln_sha256_t` type pointer successfully, otherwise return `NULL`



#### mln_sha256_pool_new

```c
mln_sha256_t *mln_sha256_pool_new(mln_alloc_t *pool);
```

Description: Create and initialize a structure of type `mln_sha256_t`, which is allocated from the memory pool specified by `pool`.

Return value: return `mln_sha256_t` type pointer successfully, otherwise return `NULL`



#### mln_sha256_free

```c
void mln_sha256_free(mln_sha256_t *s);
```

Description: Free the memory of `mln_sha256_t` type structure `s`, which should be allocated by `mln_sha256_new`.

Return value: none



#### mln_sha256_pool_free

```c
void mln_sha256_pool_free(mln_sha256_t *s);
```

Description: Free the memory of `mln_sha256_t` type structure `s`, which should be allocated by `mln_sha256_pool_new`.

Return value: none



#### mln_sha256_calc

```c
void mln_sha256_calc(mln_sha256_t *s, mln_u8ptr_t input, mln_uauto_t len, mln_u32_t is_last);
```

Description: Calculates the SHA1 value of the data specified by `input` and `len`. Like the MD5 function, this function supports batch calculation of larger data, and finally obtains a SHA256 value. `is_last` is used to indicate whether it is the last batch of this block of data. The calculated result is stored in `s`.

Return value: none



#### mln_sha256_tobytes

```c
 void mln_sha256_tobytes(mln_sha256_t *s, mln_u8ptr_t buf, mln_u32_t len);
```

Description: Write the binary result of the SHA256 calculation to the memory specified by `buf` and `len`.

Return value: none



#### mln_sha256_tostring

```c
void mln_sha256_tostring(mln_sha256_t *s, mln_s8ptr_t buf, mln_u32_t len);
```

Description: Write the string result of SHA256 calculation to the memory specified by `buf` and `len`.

Return value: none



#### mln_sha256_dump

```c
void mln_sha256_dump(mln_sha256_t *s);
```

Description: Print `mln_sha256_t` structure information to stdout, for debugging only.

Return value: none



### Example

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

