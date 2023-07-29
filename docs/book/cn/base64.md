## Base64



### 头文件

```c
#include "mln_base64.h"
```



### 函数



#### mln_base64_encode

```c
int mln_base64_encode(mln_u8ptr_t in, mln_uauto_t inlen, mln_u8ptr_t *out, mln_uauto_t *outlen);
```

描述：将`in`和`inlen`指代的内容进行base64编码，并将结果写入`out`和`outlen`中。其中，`out`中的结果是由函数内调用`malloc`进行分配的，使用后需要使用`mln_base64_free`进行释放。

**注意**：`mln_u8ptr_t`是指针类型，而`mln_u8ptr_t *`则是二级指针。

返回值：成功则返回`0`，否则返回`-1`



#### mln_base64_pool_encode

```c
int mln_base64_pool_encode(mln_alloc_t *pool, mln_u8ptr_t in, mln_uauto_t inlen, mln_u8ptr_t *out, mln_uauto_t *outlen);
```

描述：将`in`和`inlen`指代的内容进行base64编码，并将结果写入`out`和`outlen`中。其中，`out`中的结果是从内存池`pool`中进行分配的，使用后需要使用`mln_base64_pool_free`进行释放。

返回值：成功则返回`0`，否则返回`-1`



#### mln_base64_decode

```c
int mln_base64_decode(mln_u8ptr_t in, mln_uauto_t inlen, mln_u8ptr_t *out, mln_uauto_t *outlen);
```

描述：对`in`和`inlen`指代的内容进行base64解码，并将解码的结果写入`out`和`outlen`中。其中，`out`中的结果是由函数内调用`malloc`进行分配的，使用后需要使用`mln_base64_free`进行释放。

返回值：成功则返回`0`，否则返回`-1`



#### mln_base64_pool_decode

```c
int mln_base64_pool_decode(mln_alloc_t *pool, mln_u8ptr_t in, mln_uauto_t inlen, mln_u8ptr_t *out, mln_uauto_t *outlen);
```

描述：对`in`和`inlen`指代的内容进行base64解码，并将解码的结果写入`out`和`outlen`中。其中，`out`中的结果是由函数内调用`malloc`进行分配的，使用后需要使用`mln_base64_pool_free`进行释放。

返回值：成功则返回`0`，否则返回`-1`



#### mln_base64_free

```c
void mln_base64_free(mln_u8ptr_t data);
```

描述：释放`mln_base64_encode`和`mln_base64_decode`中`out`参数指向的内存空间。

返回值：无



#### mln_base64_pool_free

```c
void mln_base64_pool_free(mln_u8ptr_t data);
```

描述：释放`mln_base64_pool_encode`和`mln_base64_pool_decode`中`out`参数指向的内存空间。

返回值：无



### 示例

```c
#include <stdio.h>
#include <stdlib.h>
#include "mln_core.h"
#include "mln_log.h"
#include "mln_string.h"
#include "mln_base64.h"

int main(int argc, char *argv[])
{
    mln_string_t text = mln_string("Hello");
    mln_string_t tmp;
    mln_u8ptr_t p1, p2;
    mln_uauto_t len1, len2;
    struct mln_core_attr cattr;

    cattr.argc = argc;
    cattr.argv = argv;
    cattr.global_init = NULL;
    cattr.main_thread = NULL;
    cattr.master_process = NULL;
    cattr.worker_process = NULL;
    if (mln_core_init(&cattr) < 0) {
        fprintf(stderr, "init failed\n");
        return -1;
    }

    if (mln_base64_encode(text.data, text.len, &p1, &len1) < 0) {
        mln_log(error, "encode failed\n");
        return -1;
    }
    mln_string_nset(&tmp, p1, len1);
    mln_log(debug, "encode:%S\n", &tmp);

    if (mln_base64_decode(p1, len1, &p2, &len2) < 0) {
        mln_log(error, "decode failed\n");
        return -1;
    }
    mln_string_nset(&tmp, p2, len2);
    mln_log(debug, "decode:%S\n", &tmp);

    mln_base64_free(p1);
    mln_base64_free(p2);

    return 0;
}
```

