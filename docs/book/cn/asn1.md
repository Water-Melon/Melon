## ASN.1



### 头文件

```c
#include "mln_asn1.h"
```



### 模块名

`asn1`



### 概述

本模块提供符合 ITU-T X.690 标准的 ASN.1 数据结构的 DER（可辨别编码规则）编解码功能，支持所有常用的 Universal 类原始类型，以及 SEQUENCE、SET、SET OF 和 IMPLICIT 标签改写。

主要特性：
- 严格遵循 DER 规范：拒绝不定长编码。
- 支持解码 X.690 §8.1.2 定义的高标签号（多字节标签）。
- 内存通过 Melon 内存池（`mln_alloc_t`）管理。



### 相关结构

```c
/* 解码结果节点 */
typedef struct mln_asn1_deresult_s mln_asn1_deresult_t;

struct mln_asn1_deresult_s {
    mln_alloc_t            *pool;
    mln_u8ptr_t             code_buf;   /* 当前元素值的原始 DER 字节 */
    mln_u64_t               code_len;   /* code_buf 的字节长度 */
    mln_asn1_deresult_t   **contents;   /* 子节点（用于构造类型） */
    mln_u32_t               max;        /* contents[] 中已分配的槽数 */
    mln_u32_t               pos;        /* 已存入的子节点数 */
    mln_u32_t               _class:2;   /* ASN.1 类（通用/应用/…） */
    mln_u32_t               is_struct:1;/* 1 = 构造类型，0 = 原始类型 */
    mln_u32_t               ident:28;   /* 标签号 */
    mln_u32_t               free:1;
};

/* 编码结果节点 */
typedef struct {
    mln_alloc_t    *pool;
    mln_u64_t       size;       /* 编码后的总字节数 */
    mln_string_t   *contents;   /* 已编码字段的字符串数组 */
    mln_u32_t       max;
    mln_u32_t       pos;
    mln_u32_t       _class:2;
    mln_u32_t       is_struct:1;
    mln_u32_t       ident:29;   /* 标签号 */
} mln_asn1_enresult_t;
```

**标签 / 类常量：**

```c
/* 类值 */
#define M_ASN1_CLASS_UNIVERSAL        0
#define M_ASN1_CLASS_APPLICATION      1
#define M_ASN1_CLASS_CONTEXT_SPECIFIC 2
#define M_ASN1_CLASS_PRIVATE          3

/* Universal 标签号（ident 字段） */
#define M_ASN1_ID_BOOLEAN           1
#define M_ASN1_ID_INTEGER           2
#define M_ASN1_ID_BIT_STRING        3
#define M_ASN1_ID_OCTET_STRING      4
#define M_ASN1_ID_NULL              5
#define M_ASN1_ID_OBJECT_IDENTIFIER 6
#define M_ASN1_ID_ENUMERATED        10
#define M_ASN1_ID_UTF8STRING        12
#define M_ASN1_ID_RELATIVE_OID      13
#define M_ASN1_ID_SEQUENCE          16
#define M_ASN1_ID_SEQUENCEOF        M_ASN1_ID_SEQUENCE
#define M_ASN1_ID_SET               17
#define M_ASN1_ID_SETOF             M_ASN1_ID_SET
#define M_ASN1_ID_NUMERICSTRING     18
#define M_ASN1_ID_PRINTABLESTRING   19
#define M_ASN1_ID_T61STRING         20
#define M_ASN1_ID_IA5STRING         22
#define M_ASN1_ID_UTCTIME           23
#define M_ASN1_ID_GENERALIZEDTIME   24
#define M_ASN1_ID_BMPSTRING         30

/* 解码返回码 */
#define M_ASN1_RET_OK           0
#define M_ASN1_RET_INCOMPLETE   1
#define M_ASN1_RET_ERROR        2
```



### 函数/宏



#### mln_asn1_decode

```c
mln_asn1_deresult_t *mln_asn1_decode(void *data, mln_u64_t len, int *err, mln_alloc_t *pool);
```

描述：对长度为 `len` 字节的 DER 编码数组 `data` 进行解码。结果树的内存从 `pool` 中分配。返回时，`*err` 被置为 `M_ASN1_RET_OK`、`M_ASN1_RET_INCOMPLETE` 或 `M_ASN1_RET_ERROR` 之一。返回的 `mln_asn1_deresult_t` 持有数据的内部副本，调用方在此函数返回后可以释放 `data`。

返回值：成功则返回解码结果树的根指针，否则返回 `NULL`



#### mln_asn1_decode_ref

```c
mln_asn1_deresult_t *mln_asn1_decode_ref(void *data, mln_u64_t len, int *err, mln_alloc_t *pool);
```

描述：与 `mln_asn1_decode` 相同，但每个 `mln_asn1_deresult_t` 节点内的值缓冲区直接指向 `data`（零拷贝）。在使用结果树期间，调用方必须确保 `data` 保持有效。

返回值：成功则返回解码结果树的根指针，否则返回 `NULL`



#### mln_asn1_decode_chain

```c
mln_asn1_deresult_t *mln_asn1_decode_chain(mln_chain_t *in, int *err, mln_alloc_t *pool);
```

描述：从缓冲链 `in` 中解码 DER 数据，适用于编码数据跨越多个 `mln_chain_t` 节点的场景。内存从 `pool` 中分配，`*err` 的含义与 `mln_asn1_decode` 相同。

返回值：成功则返回解码结果树的根指针，否则返回 `NULL`



#### mln_asn1_deresult_free

```c
void mln_asn1_deresult_free(mln_asn1_deresult_t *res);
```

描述：递归释放以 `res` 为根的解码结果树所占用的全部内存。若分配时未使用内存池（即内部使用 `malloc`），则调用此函数；否则内存池清理时会自动处理。

返回值：无



#### mln_asn1_deresult_content_get

```c
mln_asn1_deresult_t *mln_asn1_deresult_content_get(mln_asn1_deresult_t *res, mln_u32_t index);
```

描述：返回构造类型 `res` 中位置 `index`（从 0 开始）处的子节点。若 `index` 越界则返回 `NULL`。

返回值：子节点指针，或 `NULL`



#### mln_asn1_deresult_dump

```c
void mln_asn1_deresult_dump(mln_asn1_deresult_t *res);
```

描述：将以 `res` 为根的解码结果树以人类可读格式打印到标准错误输出。

返回值：无



#### 解码访问器宏

```c
mln_asn1_deresult_pool_get(pres)         /* 该节点使用的内存池 */
mln_asn1_deresult_class_get(pres)        /* 类（M_ASN1_CLASS_*） */
mln_asn1_deresult_ident_get(pres)        /* 标签号（M_ASN1_ID_*） */
mln_asn1_deresult_ident_set(pres, id)    /* 设置标签号 */
mln_asn1_deresult_isstruct_get(pres)     /* 1 = 构造类型 */
mln_asn1_deresult_code_get(pres)         /* 原始值字节 */
mln_asn1_deresult_code_length_get(pres)  /* 原始值字节数 */
mln_asn1_deresult_content_num(pres)      /* 子节点数量 */
```



#### mln_asn1_enresult_new

```c
mln_asn1_enresult_t *mln_asn1_enresult_new(mln_alloc_t *pool);
```

描述：使用 `pool` 分配并初始化一个新的编码结果节点。

返回值：成功则返回新节点指针，否则返回 `NULL`



#### mln_asn1_enresult_init

```c
int mln_asn1_enresult_init(mln_asn1_enresult_t *res, mln_alloc_t *pool);
```

描述：初始化由调用方分配（如在栈上）的编码结果节点，`pool` 为内部缓冲区提供内存。

返回值：成功则返回 `0`，否则返回 `-1`



#### mln_asn1_enresult_free

```c
void mln_asn1_enresult_free(mln_asn1_enresult_t *res);
```

描述：释放由 `mln_asn1_enresult_new` 创建的节点。

返回值：无



#### mln_asn1_enresult_destroy

```c
void mln_asn1_enresult_destroy(mln_asn1_enresult_t *res);
```

描述：释放由 `mln_asn1_enresult_init` 初始化的节点（即非 `new` 分配的节点）的内部资源。

返回值：无



#### 编码原始类型

```c
int mln_asn1_encode_boolean(mln_asn1_enresult_t *res, mln_u8_t val);
int mln_asn1_encode_integer(mln_asn1_enresult_t *res, mln_u8ptr_t ints, mln_u64_t nints);
int mln_asn1_encode_enumerated(mln_asn1_enresult_t *res, mln_u8ptr_t ints, mln_u64_t nints);
int mln_asn1_encode_bitstring(mln_asn1_enresult_t *res, mln_u8ptr_t bits, mln_u64_t nbits);
int mln_asn1_encode_octetstring(mln_asn1_enresult_t *res, mln_u8ptr_t octets, mln_u64_t n);
int mln_asn1_encode_null(mln_asn1_enresult_t *res);
int mln_asn1_encode_object_identifier(mln_asn1_enresult_t *res, mln_u8ptr_t oid, mln_u64_t n);
int mln_asn1_encode_utf8string(mln_asn1_enresult_t *res, mln_u8ptr_t s, mln_u64_t slen);
int mln_asn1_encode_numericstring(mln_asn1_enresult_t *res, mln_s8ptr_t s, mln_u64_t slen);
int mln_asn1_encode_printablestring(mln_asn1_enresult_t *res, mln_s8ptr_t s, mln_u64_t slen);
int mln_asn1_encode_t61string(mln_asn1_enresult_t *res, mln_u8ptr_t s, mln_u64_t slen);
int mln_asn1_encode_ia5string(mln_asn1_enresult_t *res, mln_u8ptr_t s, mln_u64_t slen);
int mln_asn1_encode_bmpstring(mln_asn1_enresult_t *res, mln_u8ptr_t s, mln_u64_t slen);
int mln_asn1_encode_utctime(mln_asn1_enresult_t *res, time_t time);
int mln_asn1_encode_generalized_time(mln_asn1_enresult_t *res, time_t time);
```

描述：向 `res` 追加一个 DER 编码的原始字段。各函数说明：

- `mln_asn1_encode_boolean` — 编码 BOOLEAN；`val` 为 0（FALSE）或非 0（TRUE）。
- `mln_asn1_encode_integer` / `mln_asn1_encode_enumerated` — 从大端字节数组 `ints`（`nints` 字节）编码 INTEGER 或 ENUMERATED。
- `mln_asn1_encode_bitstring` — 从大端字节数组 `bits` 编码 BIT STRING，`nbits` 为比特数。
- `mln_asn1_encode_octetstring` — 从 `octets`（`n` 字节）编码 OCTET STRING。
- `mln_asn1_encode_null` — 编码 NULL（2 字节 TLV）。
- `mln_asn1_encode_object_identifier` — 从预编码的 BER OID 字节数组 `oid`（`n` 字节）编码 OID。
- 字符串函数 — 编码对应的 ASN.1 字符串类型；`s` 为原始字节，`slen` 为字节数。
- `mln_asn1_encode_utctime` / `mln_asn1_encode_generalized_time` — 从 `time_t` 编码 UTC 时间或广义时间。

返回值：成功则返回 `0`，否则返回 `-1`



#### mln_asn1_encode_sequence

```c
int mln_asn1_encode_sequence(mln_asn1_enresult_t *res);
```

描述：将 `res` 中当前所有字段打包为一个 DER SEQUENCE（标签 0x30）。各字段被合并为单个编码 SEQUENCE TLV，并更新 `res` 使其只包含该一个编码项。

返回值：成功则返回 `0`，否则返回 `-1`



#### mln_asn1_encode_set

```c
int mln_asn1_encode_set(mln_asn1_enresult_t *res);
```

描述：将 `res` 中所有字段打包为一个 DER SET（标签 0x31），不进行排序。

返回值：成功则返回 `0`，否则返回 `-1`



#### mln_asn1_encode_setof

```c
int mln_asn1_encode_setof(mln_asn1_enresult_t *res);
```

描述：将 `res` 中所有字段打包为一个 DER SET OF（标签 0x31），按 DER 要求对编码组件进行字典序升序排列。

返回值：成功则返回 `0`，否则返回 `-1`



#### mln_asn1_encode_merge

```c
int mln_asn1_encode_merge(mln_asn1_enresult_t *dest, mln_asn1_enresult_t *src);
```

描述：将 `src` 中所有编码字段移入 `dest`，调用完成后 `src` 为空。适用于在用 `mln_asn1_encode_sequence` 包装前构建嵌套结构。

返回值：成功则返回 `0`，否则返回 `-1`



#### mln_asn1_encode_implicit

```c
int mln_asn1_encode_implicit(mln_asn1_enresult_t *res, mln_u32_t ident, mln_u32_t index);
```

描述：使用 IMPLICIT 标签（上下文特定类）将 `res` 中位置 `index` 处编码字段的标签改写为 `ident`，原始类型/构造类型位保持不变。

返回值：成功则返回 `0`，否则返回 `-1`



#### mln_asn1_encode_trans_chain_once

```c
int mln_asn1_encode_trans_chain_once(mln_asn1_enresult_t *res,
                                     mln_chain_t **head,
                                     mln_chain_t **tail);
```

描述：将 `res` 中第一个编码字段序列化到新分配的 `mln_chain_t` 节点中，并追加到链 `[*head, *tail]` 的末尾。序列化后该字段从 `res` 中消耗掉。可反复调用以逐个取出所有字段。

返回值：成功则返回 `0`，否则返回 `-1`



#### mln_asn1_enresult_get_content

```c
int mln_asn1_enresult_get_content(mln_asn1_enresult_t *res, mln_u32_t index,
                                  mln_u8ptr_t *buf, mln_u64_t *len);
```

描述：获取 `res` 中位置 `index` 处编码字段的原始 DER 字节指针及字节数。成功时设置 `*buf` 和 `*len`。

返回值：成功则返回 `0`，`index` 越界则返回 `-1`



#### 编码访问器/修改器宏

```c
mln_asn1_enresult_pool_set(pres, p)    /* 设置内存池 */
mln_asn1_enresult_pool_get(pres)       /* 获取内存池 */
mln_asn1_enresult_class_set(pres, c)   /* 设置 ASN.1 类 */
mln_asn1_enresult_class_get(pres)      /* 获取 ASN.1 类 */
mln_asn1_enresult_isstruct_set(pres)   /* 标记为构造类型 */
mln_asn1_enresult_isstruct_reset(pres) /* 标记为原始类型 */
mln_asn1_enresult_isstruct_get(pres)   /* 查询构造类型标志 */
mln_asn1_enresult_ident_set(pres, id)  /* 设置标签号 */
mln_asn1_enresult_ident_get(pres)      /* 获取标签号 */
```



### 示例

```c
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "mln_asn1.h"
#include "mln_alloc.h"

int main(void)
{
    mln_alloc_t *pool = mln_alloc_init(NULL, 0);
    assert(pool != NULL);

    /* --- 编码 --- */
    mln_asn1_enresult_t enc;
    assert(mln_asn1_enresult_init(&enc, pool) == 0);

    /* 添加 BOOLEAN TRUE */
    assert(mln_asn1_encode_boolean(&enc, 1) == 0);

    /* 添加 INTEGER 42 */
    mln_u8_t intval[] = { 42 };
    assert(mln_asn1_encode_integer(&enc, intval, sizeof(intval)) == 0);

    /* 添加 IA5String "hello" */
    assert(mln_asn1_encode_ia5string(&enc, (mln_u8ptr_t)"hello", 5) == 0);

    /* 打包为 SEQUENCE */
    assert(mln_asn1_encode_sequence(&enc) == 0);

    /* 获取编码后的字节 */
    mln_u8ptr_t buf;
    mln_u64_t   len;
    assert(mln_asn1_enresult_get_content(&enc, 0, &buf, &len) == 0);

    /* --- 解码 --- */
    int err = M_ASN1_RET_OK;
    mln_asn1_deresult_t *dec = mln_asn1_decode(buf, len, &err, pool);
    assert(dec != NULL && err == M_ASN1_RET_OK);
    assert(mln_asn1_deresult_ident_get(dec) == M_ASN1_ID_SEQUENCE);
    assert(mln_asn1_deresult_content_num(dec) == 3);

    mln_asn1_deresult_t *child = mln_asn1_deresult_content_get(dec, 1);
    assert(child != NULL);
    assert(mln_asn1_deresult_ident_get(child) == M_ASN1_ID_INTEGER);
    assert(mln_asn1_deresult_code_get(child)[0] == 42);

    mln_asn1_deresult_free(dec);
    mln_asn1_enresult_destroy(&enc);
    mln_alloc_destroy(pool);
    return 0;
}
```

