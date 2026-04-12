## ASN.1



### Header file

```c
#include "mln_asn1.h"
```



### Module

`asn1`



### Overview

This module provides DER (Distinguished Encoding Rules) encoding and decoding of ASN.1 data structures as defined in ITU-T X.690. It supports all common Universal-class primitive types as well as SEQUENCE, SET, SET OF, and IMPLICIT tag rewriting.

Key properties:
- Strictly DER-compliant: indefinite-length encodings are rejected.
- High-tag-number (multi-byte) tags per X.690 §8.1.2 are supported for decoding.
- Memory is managed through Melon memory pools (`mln_alloc_t`).



### Structures

```c
/* Decode result node */
typedef struct mln_asn1_deresult_s mln_asn1_deresult_t;

struct mln_asn1_deresult_s {
    mln_alloc_t            *pool;
    mln_u8ptr_t             code_buf;   /* raw DER bytes of this element's value */
    mln_u64_t               code_len;   /* byte length of code_buf */
    mln_asn1_deresult_t   **contents;   /* child nodes (for constructed types) */
    mln_u32_t               max;        /* allocated slots in contents[] */
    mln_u32_t               pos;        /* number of children stored */
    mln_u32_t               _class:2;   /* ASN.1 class (Universal/Application/…) */
    mln_u32_t               is_struct:1;/* 1 = constructed, 0 = primitive */
    mln_u32_t               ident:28;   /* tag number */
    mln_u32_t               free:1;
};

/* Encode result node */
typedef struct {
    mln_alloc_t    *pool;
    mln_u64_t       size;       /* total encoded byte length */
    mln_string_t   *contents;   /* array of encoded field strings */
    mln_u32_t       max;
    mln_u32_t       pos;
    mln_u32_t       _class:2;
    mln_u32_t       is_struct:1;
    mln_u32_t       ident:29;   /* tag number */
} mln_asn1_enresult_t;
```

**Tag / class constants:**

```c
/* Class values */
#define M_ASN1_CLASS_UNIVERSAL        0
#define M_ASN1_CLASS_APPLICATION      1
#define M_ASN1_CLASS_CONTEXT_SPECIFIC 2
#define M_ASN1_CLASS_PRIVATE          3

/* Universal tag numbers (ident field) */
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

/* Decode return codes */
#define M_ASN1_RET_OK           0
#define M_ASN1_RET_INCOMPLETE   1
#define M_ASN1_RET_ERROR        2
```



### Functions/Macros



#### mln_asn1_decode

```c
mln_asn1_deresult_t *mln_asn1_decode(void *data, mln_u64_t len, int *err, mln_alloc_t *pool);
```

Description: Decode the DER-encoded byte array `data` of length `len`. Memory for the resulting tree is allocated from `pool`. On return, `*err` is set to one of `M_ASN1_RET_OK`, `M_ASN1_RET_INCOMPLETE`, or `M_ASN1_RET_ERROR`. The returned `mln_asn1_deresult_t` owns internal copies of the data; the caller may safely discard `data` after this call.

Return value: pointer to the decode-result tree on success, or `NULL` on error



#### mln_asn1_decode_ref

```c
mln_asn1_deresult_t *mln_asn1_decode_ref(void *data, mln_u64_t len, int *err, mln_alloc_t *pool);
```

Description: Same as `mln_asn1_decode`, but the value buffers inside each `mln_asn1_deresult_t` node point directly into `data` (zero-copy). The caller must keep `data` alive for as long as the result tree is in use.

Return value: pointer to the decode-result tree on success, or `NULL` on error



#### mln_asn1_decode_chain

```c
mln_asn1_deresult_t *mln_asn1_decode_chain(mln_chain_t *in, int *err, mln_alloc_t *pool);
```

Description: Decode DER data from the chain of buffers `in`. Useful when the encoded data spans multiple `mln_chain_t` nodes. Memory is allocated from `pool`. `*err` is set as in `mln_asn1_decode`.

Return value: pointer to the decode-result tree on success, or `NULL` on error



#### mln_asn1_deresult_free

```c
void mln_asn1_deresult_free(mln_asn1_deresult_t *res);
```

Description: Recursively free all memory used by the decode-result tree rooted at `res`. If `res` was allocated without a pool (i.e., internal allocations used `malloc`), use this function. Otherwise the pool's cleanup handles it automatically.

Return value: none



#### mln_asn1_deresult_content_get

```c
mln_asn1_deresult_t *mln_asn1_deresult_content_get(mln_asn1_deresult_t *res, mln_u32_t index);
```

Description: Return the child node at position `index` within the constructed type `res`. `index` is zero-based. Returns `NULL` if `index` is out of range.

Return value: child node pointer, or `NULL`



#### mln_asn1_deresult_dump

```c
void mln_asn1_deresult_dump(mln_asn1_deresult_t *res);
```

Description: Print a human-readable representation of the decode-result tree rooted at `res` to standard error.

Return value: none



#### Decode accessor macros

```c
mln_asn1_deresult_pool_get(pres)         /* pool used for this node */
mln_asn1_deresult_class_get(pres)        /* class (M_ASN1_CLASS_*) */
mln_asn1_deresult_ident_get(pres)        /* tag number (M_ASN1_ID_*) */
mln_asn1_deresult_ident_set(pres, id)    /* set the tag number */
mln_asn1_deresult_isstruct_get(pres)     /* 1 = constructed */
mln_asn1_deresult_code_get(pres)         /* raw value bytes */
mln_asn1_deresult_code_length_get(pres)  /* length of raw value bytes */
mln_asn1_deresult_content_num(pres)      /* number of children */
```



#### mln_asn1_enresult_new

```c
mln_asn1_enresult_t *mln_asn1_enresult_new(mln_alloc_t *pool);
```

Description: Allocate and initialize a new encode-result node using `pool`.

Return value: pointer to the new node, or `NULL` on allocation failure



#### mln_asn1_enresult_init

```c
int mln_asn1_enresult_init(mln_asn1_enresult_t *res, mln_alloc_t *pool);
```

Description: Initialize an encode-result node that was allocated by the caller (e.g., on the stack). `pool` provides the backing allocator for internal buffers.

Return value: `0` on success, `-1` on failure



#### mln_asn1_enresult_free

```c
void mln_asn1_enresult_free(mln_asn1_enresult_t *res);
```

Description: Free a node created by `mln_asn1_enresult_new`.

Return value: none



#### mln_asn1_enresult_destroy

```c
void mln_asn1_enresult_destroy(mln_asn1_enresult_t *res);
```

Description: Release internal resources of a node that was initialized with `mln_asn1_enresult_init` (i.e., not `new`-allocated).

Return value: none



#### Encode primitive types

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

Description: Append a DER-encoded primitive field to `res`. For each function:

- `mln_asn1_encode_boolean` — encode a BOOLEAN; `val` is 0 (FALSE) or non-0 (TRUE).
- `mln_asn1_encode_integer` / `mln_asn1_encode_enumerated` — encode an INTEGER or ENUMERATED from a big-endian byte array `ints` of `nints` bytes.
- `mln_asn1_encode_bitstring` — encode a BIT STRING from `bits` (big-endian), `nbits` is the number of bits.
- `mln_asn1_encode_octetstring` — encode an OCTET STRING from `octets` of `n` bytes.
- `mln_asn1_encode_null` — encode a NULL (two-byte TLV).
- `mln_asn1_encode_object_identifier` — encode an OID from the pre-encoded BER OID bytes `oid` of `n` bytes.
- String functions — encode the respective ASN.1 string type; `s` is the raw bytes and `slen` is the byte count.
- `mln_asn1_encode_utctime` / `mln_asn1_encode_generalized_time` — encode a UTC or Generalized time from `time_t`.

Return value: `0` on success, `-1` on failure



#### mln_asn1_encode_sequence

```c
int mln_asn1_encode_sequence(mln_asn1_enresult_t *res);
```

Description: Wrap all fields currently in `res` into a DER SEQUENCE (tag 0x30). The individual fields are merged into a single encoded SEQUENCE TLV and `res` is updated to contain only that one encoded item.

Return value: `0` on success, `-1` on failure



#### mln_asn1_encode_set

```c
int mln_asn1_encode_set(mln_asn1_enresult_t *res);
```

Description: Wrap all fields in `res` into a DER SET (tag 0x31), without sorting.

Return value: `0` on success, `-1` on failure



#### mln_asn1_encode_setof

```c
int mln_asn1_encode_setof(mln_asn1_enresult_t *res);
```

Description: Wrap all fields in `res` into a DER SET OF (tag 0x31), sorting the encoded components in ascending lexicographic order as required by DER.

Return value: `0` on success, `-1` on failure



#### mln_asn1_encode_merge

```c
int mln_asn1_encode_merge(mln_asn1_enresult_t *dest, mln_asn1_enresult_t *src);
```

Description: Move all encoded fields from `src` into `dest`. After the call `src` is empty. Useful for building nested structures before wrapping them with `mln_asn1_encode_sequence`.

Return value: `0` on success, `-1` on failure



#### mln_asn1_encode_implicit

```c
int mln_asn1_encode_implicit(mln_asn1_enresult_t *res, mln_u32_t ident, mln_u32_t index);
```

Description: Rewrite the tag of the encoded field at position `index` in `res` to `ident` using an IMPLICIT tag (context-specific class). The primitive/constructed bit is preserved.

Return value: `0` on success, `-1` on failure



#### mln_asn1_encode_trans_chain_once

```c
int mln_asn1_encode_trans_chain_once(mln_asn1_enresult_t *res,
                                     mln_chain_t **head,
                                     mln_chain_t **tail);
```

Description: Serialize the first encoded field in `res` into a newly allocated `mln_chain_t` node appended to the chain `[*head, *tail]`. The serialized field is consumed from `res`. Call repeatedly to drain all fields.

Return value: `0` on success, `-1` on failure



#### mln_asn1_enresult_get_content

```c
int mln_asn1_enresult_get_content(mln_asn1_enresult_t *res, mln_u32_t index,
                                  mln_u8ptr_t *buf, mln_u64_t *len);
```

Description: Retrieve a pointer to the raw DER bytes and byte count of the encoded field at position `index` in `res`. `*buf` and `*len` are set on success.

Return value: `0` on success, `-1` if `index` is out of range



#### Encode accessor/mutator macros

```c
mln_asn1_enresult_pool_set(pres, p)    /* set the pool */
mln_asn1_enresult_pool_get(pres)       /* get the pool */
mln_asn1_enresult_class_set(pres, c)   /* set ASN.1 class */
mln_asn1_enresult_class_get(pres)      /* get ASN.1 class */
mln_asn1_enresult_isstruct_set(pres)   /* mark as constructed */
mln_asn1_enresult_isstruct_reset(pres) /* mark as primitive */
mln_asn1_enresult_isstruct_get(pres)   /* query constructed flag */
mln_asn1_enresult_ident_set(pres, id)  /* set tag number */
mln_asn1_enresult_ident_get(pres)      /* get tag number */
```



### Example

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

    /* --- Encoding --- */
    mln_asn1_enresult_t enc;
    assert(mln_asn1_enresult_init(&enc, pool) == 0);

    /* Add a BOOLEAN TRUE */
    assert(mln_asn1_encode_boolean(&enc, 1) == 0);

    /* Add an INTEGER 42 */
    mln_u8_t intval[] = { 42 };
    assert(mln_asn1_encode_integer(&enc, intval, sizeof(intval)) == 0);

    /* Add an IA5String "hello" */
    assert(mln_asn1_encode_ia5string(&enc, (mln_u8ptr_t)"hello", 5) == 0);

    /* Wrap in a SEQUENCE */
    assert(mln_asn1_encode_sequence(&enc) == 0);

    /* Retrieve the encoded bytes */
    mln_u8ptr_t buf;
    mln_u64_t   len;
    assert(mln_asn1_enresult_get_content(&enc, 0, &buf, &len) == 0);

    /* --- Decoding --- */
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

