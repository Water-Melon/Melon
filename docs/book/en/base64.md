## Base64



### Header file

```c
#include "mln_base64.h"
```



### Module

`base64`



### Functions



#### mln_base64_encode

```c
int mln_base64_encode(mln_u8ptr_t in, mln_uauto_t inlen, mln_u8ptr_t *out, mln_uauto_t *outlen);
```

Description: Base64 encode what `in` and `inlen` refer to, and write the result to `out` and `outlen`. Among them, the result in `out` is allocated by calling `malloc` in the function, and it needs to be released with `mln_base64_free` after use.

**Note**: `mln_u8ptr_t` is a pointer type, while `mln_u8ptr_t *` is a secondary pointer.

Return value: return `0` if successful, otherwise return `-1`



#### mln_base64_pool_encode

```c
int mln_base64_pool_encode(mln_alloc_t *pool, mln_u8ptr_t in, mln_uauto_t inlen, mln_u8ptr_t *out, mln_uauto_t *outlen);
```

Description: Base64 encode what `in` and `inlen` refer to, and write the result to `out` and `outlen`. Among them, the result in `out` is allocated from the memory pool `pool`, and needs to be released using `mln_base64_pool_free` after use.

Return value: return `0` if successful, otherwise return `-1`



#### mln_base64_decode

```c
int mln_base64_decode(mln_u8ptr_t in, mln_uauto_t inlen, mln_u8ptr_t *out, mln_uauto_t *outlen);
```

Description: Base64 decode what `in` and `inlen` refer to, and write the decoded result to `out` and `outlen`. Among them, the result in `out` is allocated by calling `malloc` in the function, and it needs to be released with `mln_base64_free` after use.

Return value: return `0` if successful, otherwise return `-1`



#### mln_base64_pool_decode

```c
int mln_base64_pool_decode(mln_alloc_t *pool, mln_u8ptr_t in, mln_uauto_t inlen, mln_u8ptr_t *out, mln_uauto_t *outlen);
```

Description: Base64 decode what `in` and `inlen` refer to, and write the decoded result to `out` and `outlen`. Among them, the result in `out` is allocated by calling `malloc` in the function, and it needs to be released with `mln_base64_pool_free` after use.

Return value: return `0` if successful, otherwise return `-1`



#### mln_base64_free

```c
void mln_base64_free(mln_u8ptr_t data);
```

Description: Free the memory space pointed to by the `out` parameter in `mln_base64_encode` and `mln_base64_decode`.

Return value: none



#### mln_base64_pool_free

```c
void mln_base64_pool_free(mln_u8ptr_t data);
```

Description: Free the memory space pointed to by the `out` parameter in `mln_base64_pool_encode` and `mln_base64_pool_decode`.

Return value: none



### Example

```c
#include <stdio.h>
#include <stdlib.h>
#include "mln_string.h"
#include "mln_base64.h"

int main(int argc, char *argv[])
{
    mln_string_t text = mln_string("Hello");
    mln_string_t tmp;
    mln_u8ptr_t p1, p2;
    mln_uauto_t len1, len2;

    if (mln_base64_encode(text.data, text.len, &p1, &len1) < 0) {
        fprintf(stderr, "encode failed\n");
        return -1;
    }
    mln_string_nset(&tmp, p1, len1);
    write(STDOUT_FILENO, tmp.data, tmp.len);
    write(STDOUT_FILENO, "\n", 1);

    if (mln_base64_decode(p1, len1, &p2, &len2) < 0) {
        fprintf(stderr, "decode failed\n");
        return -1;
    }
    mln_string_nset(&tmp, p2, len2);
    write(STDOUT_FILENO, tmp.data, tmp.len);
    write(STDOUT_FILENO, "\n", 1);

    mln_base64_free(p1);
    mln_base64_free(p2);

    return 0;
}
```

