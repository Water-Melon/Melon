## RC4



### 头文件

```c
#include "mln_rc.h"
```



### 模块名

`rc`



### 函数



#### mln_rc4_init

```c
void mln_rc4_init(mln_u8ptr_t s, mln_u8ptr_t key, mln_uauto_t len);
```

描述：初始化RC4所需参数`s`。`key`为密钥内容，`len`为密钥长度。`s`必须为长度为256字节长的内存区，且被初始化为0。

返回值：无



#### mln_rc4_calc

```c
void mln_rc4_calc(mln_u8ptr_t s, mln_u8ptr_t data, mln_uauto_t len);
```

描述：进行RC4加解密。`s`为`mln_rc4_init`初始化而来的参数。`data`为被加密或解密的数据，`len`为`data`的长度。

加解密的结果会直接写回`data`中，因此要注意`data`内存区的可写性。

返回值：无



### 示例

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

