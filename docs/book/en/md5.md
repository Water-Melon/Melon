## MD5



### Header file

```c
#include "mln_md5.h"
```



### Module

`md5`



### Functions



#### mln_md5_init

```c
void mln_md5_init(mln_md5_t *m);
```

Description: Initialize the `mln_md5_t` structure `m`.

Return value: none



#### mln_md5_new

```c
mln_md5_t *mln_md5_new(void);
```

Description: Create and initialize the `mln_md5_t` structure allocated by `malloc`.

Return value: return `mln_md5_t` structure pointer if successful, otherwise return `NULL`



#### mln_md5_pool_new

```c
mln_md5_t *mln_md5_pool_new(mln_alloc_t *pool);
```

Description: Create and initialize the `mln_md5_t` structure allocated from the memory pool specified by `pool`.

Return value: return `mln_md5_t` structure pointer if successful, otherwise return `NULL`



#### mln_md5_free

```c
void mln_md5_free(mln_md5_t *m);
```

Description: Free the memory of the `mln_md5_t` structure `m`, which should be created by `mln_md5_new`.

Return value: none



#### mln_md5_pool_free

```c
void mln_md5_pool_free(mln_md5_t *m);
```

Description: Free the memory of the `mln_md5_t` structure `m`, which should be created by `mln_md5_pool_new`.

Return value: none



#### mln_md5_calc

```c
void mln_md5_calc(mln_md5_t *m, mln_u8ptr_t input, mln_uauto_t len, mln_u32_t is_last);
```

Description: Calculates the md5 value. This function can be called multiple times to generate an MD5 value. in:

- `m` is initialized from the above function
- `input` data to be MD5 calculated
- `len` length of `input`
- Whether `is_last` was the last call. Since an MD5 value can be calculated multiple times for a large data, this parameter is used to indicate whether it is the last batch of data.

The calculated MD5 is stored in `m`.

Return value: none



#### mln_md5_tobytes

```c
void mln_md5_tobytes(mln_md5_t *m, mln_u8ptr_t buf, mln_u32_t len);
```

Description: Write the calculated MD5 value in binary form to the memory area specified by `buf` and `len`.

Return value: none



#### mln_md5_tostring

```c
void mln_md5_tostring(mln_md5_t *m, mln_s8ptr_t buf, mln_u32_t len);
```

Description: Write the calculated MD5 value as a string to the memory area specified by `buf` and `len`.

Return value: none



#### mln_md5_dump

```c
void mln_md5_dump(mln_md5_t *m);
```

Description: Print MD5 related information to standard output.

Return value: none



### Example

```c
#include <stdio.h>
#include "mln_md5.h"

int main(int argc, char *argv[])
{
    mln_md5_t m;
    char text[] = "Hello";
    char output[33] = {0};//The MD5 calculation result is a total of 16 bytes, and the string output is twice as binary, so it is 32 bytes, and one more byte is used for \0

    mln_md5_init(&m);
    mln_md5_calc(&m, (mln_u8ptr_t)text, sizeof(text)-1, 1);
    mln_md5_tostring(&m, output, sizeof(output));
    printf("%s\n", output);

    return 0;
}
```

