## 里德所罗门纠错码



### 头文件

```c
#include "mln_rs.h"
```



### 模块名

`rs`



### 函数/宏



#### mln_rs_encode

```c
mln_rs_result_t *mln_rs_encode(uint8_t *data_vector, size_t len, size_t n, size_t k);
```

描述：生成纠错码。参数含义：

- `data_vector`存放若干个报文，由于报文必须等长，因此这里直接传入一维指针提升处理性能。
- `len`每一段数据的长度。
- `n`一共多少段数据。
- `k`一共多少段纠错码需要生成。

生成的结果中会包含原始数据于纠错码，且纠错码会存放在数据之后。

返回值：成功则返回`mln_rs_result_t`结构指针，否则返回`NULL`



#### mln_rs_decode

```c
mln_rs_result_t *mln_rs_decode(uint8_t **data_vector, size_t len, size_t n, size_t k);
```

描述：利用纠错码进行纠错。参数含义：

- `data_vector`包含纠错码在内的报文数组。注意：这里是二维数组。**注意1**：数据区中的报文顺序必须与生成时一致，缺失的报文处给`NULL`，但顺序不可以乱。**注意2**：当丢失数据个数多于`k`个时，无法进行修复。
- `len`每一段数据的长度。
- `n`原始一共多少段数据。
- `k`原始一共多少段纠错码被生成。

返回值：成功则返回`mln_rs_result_t`结构指针，否则返回`NULL`



#### mln_rs_result_free

```c
void mln_rs_result_free(mln_rs_result_t *result);
```

描述：释放上述函数生成的结果结构。

返回值：无



#### mln_rs_result_get_num

```c
mln_rs_result_get_num(_presult)
```

描述：获取类行为`mln_rs_result_t`的`_presult`中处理结果的个数。

返回值：处理结果个数



#### mln_rs_result_get_data_by_index

```c
mln_rs_result_get_data_by_index(_presult,index)
```

描述：获取类行为`mln_rs_result_t`的`_presult`中下标为`index`的数据，`index`从0开始。

返回值：结果数据内存地址



###示例

```c
#include <stdio.h>
#include "mln_string.h"
#include "mln_rs.h"

int main(int argc, char *argv[])
{
    mln_rs_result_t *res, *dres;
    char origin[] = "AAAABBBBCCCCDDDD";
    uint8_t *err[6] = {0};
    mln_string_t tmp;

    res = mln_rs_encode((uint8_t *)origin, 4, 4, 2);
    if (res == NULL) {
        fprintf(stderr, "rs encode failed.\n");
        return -1;
    }

    err[0] = NULL;
    err[1] = NULL;
    err[2] = (uint8_t *)origin+8;
    err[3] = (uint8_t *)origin+12;
    err[4] = mln_rs_result_get_data_by_index(res, 4);
    err[5] = mln_rs_result_get_data_by_index(res, 5);

    dres = mln_rs_decode(err, 4, 4, 2);
    if (dres == NULL) {
        fprintf(stderr, "rs decode failed.\n");
        return -1;
    }

    mln_string_nset(&tmp, mln_rs_result_get_data_by_index(dres, 1), 4);
    write(STDOUT_FILENO, tmp.data, tmp.len);
    write(STDOUT_FILENO, "\n", 1);

    mln_rs_result_free(res);
    mln_rs_result_free(dres);
    return 0;
}
```

