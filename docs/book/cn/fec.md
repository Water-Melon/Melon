## FEC

FEC是由RFC5109定义的，用于对RTP报文进行前向纠错的。

本篇暂不给出示例，使用也较为简单，基本流程就是：初始化fec结构->生成/修复FEC报文->自定义处理->释放生成/修复的FEC报文->释放fec结构。



### 头文件

```c
#include "mln_fec.h"
```



### 模块名

`fec`



### 函数/宏



#### mln_fec_new

```c
mln_fec_t *mln_fec_new(void);
```

描述：创建`mln_fec_t`结构。

返回值：成功返回结构指针，否则返回`NULL`



#### mln_fec_free

```c
void mln_fec_free(mln_fec_t *fec);
```

描述：释放`mln_fec_t`结构。

返回值：无



#### mln_fec_encode

```c
mln_fec_result_t *mln_fec_encode(mln_fec_t *fec, uint8_t *packets[], uint16_t packlen[], size_t n, uint16_t group_size);
```

描述：对给定的RTP报文生成相应的FEC报文，其中：

- `packets` RTP报文数组，每个元素是一个RTP报文。**注意**：这里是完整的RTP报文（含头和体）。
- `packlen` RTP报文长度数组，每个元素与`packets`中元素一一对应。
- `n`为RTP报文个数。
- `group_size` 指出每`group_size`个RTP报文生成一个FEC报文，即本函数可能一次生成多个FEC报文。

返回值：成功则返回`mln_fec_result_t`结构指针，否则返回`NULL`



#### mln_fec_result_free

```c
void mln_fec_result_free(mln_fec_result_t *fr);
```

描述：释放`mln_fec_result_t`结构内存。

返回值：无



#### mln_fec_decode

```c
mln_fec_result_t *mln_fec_decode(mln_fec_t *fec, uint8_t *packets[], uint16_t *packLen, size_t n);
```

描述：修复RTP报文。FEC采用异或进行修复，因此只能接受仅丢失一个RTP报文的情况，若丢失超过一个则无法修复。

- `packets` 含FEC报文和RTP报文的数组，每个元素是一个RTP或FEC报文。**注意**：这里是完整的RTP报文（含头和体）。
- `packlen` 报文长度数组，每个元素与`packets`中元素一一对应。
- `n`为报文个数。

返回值：成功则返回`mln_fec_result_t`结构指针，否则返回`NULL`



#### mln_fec_set_pt

```c
mln_fec_set_pt(fec,_pt)
```

描述：设置RTP报文头中的PT字段。

返回值：无



#### mln_fec_get_result

```c
mln_fec_get_result(_result,index,_len)
```

描述：获取类行为`mln_fec_result_t`的`_result`中第`index`个报文，`index`从0开始。`_len`为获取到的报文长度。

返回值：获取到的报文内容内存地址



#### mln_fec_get_result_num

```c
mln_fec_get_result_num(_result)
```

描述：获取类行为`mln_fec_result_t`的`_result`中结果的个数。

返回值：结果个数

