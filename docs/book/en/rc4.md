## RC4



### Header file

```c
#include "mln_rc.h"
```



### Module

`rc`



### Functions



#### mln_rc4_init

```c
void mln_rc4_init(mln_u8ptr_t s, mln_u8ptr_t key, mln_uauto_t len);
```

Description: The parameter `s` required to initialize RC4. `key` is the key content and `len` is the key length. `s` must be a 256-byte long memory area initialized to 0.

Return value: none



#### mln_rc4_calc

```c
void mln_rc4_calc(mln_u8ptr_t s, mln_u8ptr_t data, mln_uauto_t len);
```

Description: Perform RC4 encryption and decryption. `s` is the parameter initialized by `mln_rc4_init`. `data` is the encrypted or decrypted data, `len` is the length of `data`.

The result of encryption and decryption will be directly written back to `data`, so pay attention to the writability of `data` memory area.

Return value: none



### Example

```c
#include <stdio.h>
#include <stdlib.h>
#include "mln_rc.h"

int main(int argc, char *argv[])
{
    mln_u8_t s[256] = {0};
    mln_u8_t text[] = "Hello";

    mln_rc4_init(s, (mln_u8ptr_t)"this is a key", sizeof("this is a key")-1);
    mln_rc4_calc(s, text, sizeof(text)-1);
    printf("%s\n", text);
    mln_rc4_calc(s, text, sizeof(text)-1);
    printf("%s\n", text);

    return 0;
}
```

