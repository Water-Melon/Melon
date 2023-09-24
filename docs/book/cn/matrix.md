## 矩阵运算



### 头文件

```c
#include "mln_matrix.h"
```



### 模块名

`matrix`



### 相关结构

```c
typedef struct {                 
    mln_size_t  row;//矩阵的行数
    mln_size_t  col;//矩阵的列数
    double     *data;//一个一维数组，包含了矩阵内所有元素，按行一次排列
    mln_u32_t   is_ref:1;//标识data是否为外部引用，该标记用于释放矩阵结构时忽略对data的释放
} mln_matrix_t;
```



### 函数



#### mln_matrix_new

```c
mln_matrix_t *mln_matrix_new(mln_size_t row, mln_size_t col, double *data, mln_u32_t is_ref);
```

描述：创建一个`row`行`col`列，数据为`data`的矩阵。若`is_ref`为`0`则表示矩阵结构会完全复制一个`data`在其中，否则直接引用`data`。

返回值：成功则返回矩阵结构指针，否则返回`NULL`



#### mln_matrix_free

```c
void mln_matrix_free(mln_matrix_t *matrix);
```

描述：释放矩阵结构内存。

返回值：无



#### mln_matrix_mul

```c
mln_matrix_t *mln_matrix_mul(mln_matrix_t *m1, mln_matrix_t *m2);
```

描述：矩阵乘法。

返回值：成功则返回结果矩阵指针，否则返回`NULL`



#### mln_matrix_inverse

```c
mln_matrix_t *mln_matrix_inverse(mln_matrix_t *matrix);
```

描述：矩阵求逆。**注意**：矩阵求逆要求是该矩阵为方阵。

返回值：成功则返回结果矩阵指针，否则返回`NULL`



#### mln_matrix_dump

```c
void mln_matrix_dump(mln_matrix_t *matrix);
```

描述：将矩阵的信息输出到标准输出中。仅用于调试。

返回值：无



### 示例

```c
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include "mln_matrix.h"

int main(int argc, char *argv[])
{
    mln_matrix_t *a, *b;
    double data[] = {1, 1, 1, 1, 2, 4, 2, 8, 64};

    a = mln_matrix_new(3, 3, data, 1);
    if (a == NULL) {
        fprintf(stderr, "init matrix failed\n");
        return -1;
    }
    mln_matrix_dump(a);

    b = mln_matrix_inverse(a);
    mln_matrix_free(a);
    if (b == NULL) {
        fprintf(stderr, "inverse failed: %s\n", strerror(errno));
        return -1;
    }
    mln_matrix_dump(b);
    mln_matrix_free(b);

    return 0;
}
```

