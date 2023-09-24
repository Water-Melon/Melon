## 大数计算



### 头文件

```c
#include "mln_bignum.h"
```



### 模块名

`bignum`



### 相关结构

```c
typedef struct {
    mln_u32_t tag;//正负数标记 
    mln_u32_t length;//当前大数数值所使用到的data的元素个数
    mln_u64_t data[M_BIGNUM_SIZE];//大数数值
} mln_bignum_t;

#define M_BIGNUM_POSITIVE 0 //正数
#define M_BIGNUM_NEGATIVE 1 //负数

#define M_BIGNUM_SIZE     257
```

Melon中大数的实现是定长的，即大数是有上限的，目前支持到最大2048位。



### 函数/宏



#### mln_bignum_new

```c
mln_bignum_t *mln_bignum_new(void);
```

描述：创建并初始化大数结构`mln_bignum_t`，该结构由`malloc`分配而来。

返回值：成功则返回大数结构指针，否则返回`NULL`



#### mln_bignum_pool_new

```c
mln_bignum_t *mln_bignum_pool_new(mln_alloc_t *pool);
```

描述：创建并初始化大数结构`mln_bignum_t`，该结构由`pool`指定的内存池分配而来。

返回值：成功则返回大数结构指针，否则返回`NULL`



#### mln_bignum_free

```c
void mln_bignum_free(mln_bignum_t *bn);
```

描述：释放大数结构`bn`，该结构应由`mln_bignum_new`分配而来。

返回值：无



#### mln_bignum_pool_free

```c
void mln_bignum_pool_free(mln_bignum_t *bn);
```

描述：释放大数结构`bn`，该结构应由`mln_bignum_pool_new`分配而来。

返回值：无



#### mln_bignum_dup

```c
mln_bignum_t *mln_bignum_dup(mln_bignum_t *bn);
```

描述：完全复制一份大数结构`bn`，复制品由`malloc`分配内存。

返回值：成功则返回大数结构指针，否则返回`NULL`



#### mln_bignum_pool_dup

```c
mln_bignum_t *mln_bignum_pool_dup(mln_alloc_t *pool, mln_bignum_t *bn);
```

描述：完全复制一份大数结构`bn`，复制品由`pool`指定的内存池上分配内存。

返回值：成功则返回大数结构指针，否则返回`NULL`



#### mln_bignum_assign

```c
int mln_bignum_assign(mln_bignum_t *bn, mln_s8ptr_t sval, mln_u32_t len);
```

描述：将`sval`和`len`表示的字符串形式的大数赋值给大数结构`bn`。

返回值：成功则返回`0`，否则返回`-1`



#### mln_bignum_add

```c
void mln_bignum_add(mln_bignum_t *dest, mln_bignum_t *src);
```

描述：大数加法，计算结果会放入`dest`中。

返回值：无



#### mln_bignum_sub

```c
void mln_bignum_sub(mln_bignum_t *dest, mln_bignum_t *src);
```

描述：大数减法，被减数`dest`，减数`src`，计算结果会放入`dest`中。

返回值：无



#### mln_bignum_mul

```c
void mln_bignum_mul(mln_bignum_t *dest, mln_bignum_t *src);
```

描述：大数乘法，计算结果会放入`dest`中。

返回值：无



#### mln_bignum_div

```c
int mln_bignum_div(mln_bignum_t *dest, mln_bignum_t *src, mln_bignum_t *quotient);
```

描述：大数除法，被除数`dest`, 除数`src`，余数会放入`dest`中，若`quotient`不为空，则商将放入其中。

返回值：成功则返回`0`，否则返回`-1`



#### mln_bignum_pwr

```c
int mln_bignum_pwr(mln_bignum_t *dest, mln_bignum_t *exponent, mln_bignum_t *mod);
```

描述：大数幂运算，计算`dest`的`exponent`次方。若`mod`不为空，则结果会对`mod`取模。最终计算结果会放入`dest`中。

返回值：成功则返回`0`，否则返回`-1`



#### mln_bignum_compare

```c
int mln_bignum_compare(mln_bignum_t *bn1, mln_bignum_t *bn2);
```

描述：带符号的比较大数大小值。

返回值：

-  `1` - `bn1` > `bn2`
- `-1` -` bn1` < `bn2`
-  `0` - `bn1` = `bn2`



#### mln_bignum_abs_compare

```c
int mln_bignum_abs_compare(mln_bignum_t *bn1, mln_bignum_t *bn2);
```

描述：绝对值比大小。

返回值：

-  `1` - `bn1` > `bn2`
- `-1` -` bn1` < `bn2`
-  `0` - `bn1` = `bn2`



#### mln_bignum_bit_test

```c
int mln_bignum_bit_test(mln_bignum_t *bn, mln_u32_t index);
```

描述：检测`index`指定的大数`bn`中该比特是否为1。

返回值：为1则返回`1`，否则返回`0`



#### mln_bignum_left_shift

```c
void mln_bignum_left_shift(mln_bignum_t *bn, mln_u32_t n);
```

描述：将大数`bn`左移`n`位。

返回值：无



#### mln_bignum_right_shift

```c
void mln_bignum_right_shift(mln_bignum_t *bn, mln_u32_t n);
```

描述：将大数`bn`右移`n`位。

返回值：无



#### mln_bignum_prime

```c
int mln_bignum_prime(mln_bignum_t *res, mln_u32_t bitwidth);
```

描述：计算一个`bitwidth`位的大素数，并将结果写入`res`中。

返回值：成功则返回`0`，否则返回`-1`



#### mln_bignum_extend_eulid

```c
int mln_bignum_extend_eulid(mln_bignum_t *a, mln_bignum_t *b, mln_bignum_t *x, mln_bignum_t *y);
```

描述：大数版本的扩展欧几里得算法。

返回值：成功则返回`0`，否则返回`-1`



#### mln_bignum_i2osp

```c
int mln_bignum_i2osp(mln_bignum_t *n, mln_u8ptr_t buf, mln_size_t len);
```

描述：将`n`的二进制值写入`buf`与`len`指代的内存中。

返回值：成功则返回`0`，否则返回`-1`



#### mln_bignum_os2ip

```c
int mln_bignum_os2ip(mln_bignum_t *n, mln_u8ptr_t buf, mln_size_t len);
```

描述：将`buf`与`len`指代的内存中的二进制值赋予`n`。与`mln_bignum_i2osp`是相反操作。

返回值：成功则返回`0`，否则返回`-1`



#### mln_bignum_tostring

```c
mln_string_t *mln_bignum_tostring(mln_bignum_t *n);
```

描述：将大数`n`转换成十进制字符串。本函数因其性能较低，故应仅用于调试。不要忘记使用后将返回值使用`mln_string_free`进行释放。

返回值：成功则返回十进制数字符串，否则返回`NULL`



#### mln_bignum_positive

```c
mln_bignum_positive(pbn)
```

描述：将`mln_bignum_t *`的大数`pbn`设置为正数。

返回值：无



#### mln_bignum_negative

```c
mln_bignum_negative(pbn)
```

描述：将`mln_bignum_t *`的大数`pbn`设置为负数。

返回值：无



#### mln_bignum_is_positive

```c
mln_bignum_is_positive(pbn)
```

描述：判断`mln_bignum_t *`的大数`pbn`是否为为正数。

返回值：为正则返回`非0`，否则返回`0`



#### mln_bignum_is_negative

```c
mln_bignum_is_negative(pbn)
```

描述：判断`mln_bignum_t *`的大数`pbn`是否为为负数。

返回值：为负则返回`非0`，否则返回`0`



#### mln_bignum_get_length

```c
mln_bignum_get_length(pbn)
```

描述：获取`mln_bignum_t *`的大数`pbn`的值占`data`多少个数组元素。

返回值：元素个数



#### mln_bignum_zero

```s
mln_bignum_zero()
```

描述：返回一个值为`0`的大数。

返回值：大数值为`0`的常量



### 示例

```c
#include "mln_bignum.h"

int main(int argc, char *argv[])
{
    mln_string_t *s;
    mln_bignum_t n1, n2 = mln_bignum_zero();

    mln_bignum_init(n1); //same as mln_bignum_zero

    mln_bignum_assign(&n1, "10", 2);
    mln_bignum_assign(&n2, "30", 2);

    mln_bignum_pwr(&n1, &n2, NULL);
    s = mln_bignum_tostring(&n1);
    write(STDOUT_FILENO, s->data, s->len);
    write(STDOUT_FILENO, "\n", 1);
    mln_string_free(s);

    return 0;
}
```

