
/*
 * Copyright (C) Niklaus F.Schen.
 */
#ifndef __MLN_ASN1_H
#define __MLN_ASN1_H

#include "mln_types.h"
#include "mln_chain.h"
#include "mln_alloc.h"

#define M_ASN1_BUFSIZE        32

#define M_ASN1_RET_OK         0
#define M_ASN1_RET_INCOMPLETE 1
#define M_ASN1_RET_ERROR      2

/*
 * class
 */
#define M_ASN1_CLASS_UNIVERSAL        0
#define M_ASN1_CLASS_APPLICATION      1
#define M_ASN1_CLASS_CONTEXT_SPECIFIC 2
#define M_ASN1_CLASS_PRIVATE          3

/*
 * single octet
 */
#define M_ASN1_ID_NONE              0xfffffff
#define M_ASN1_ID_BOOLEAN           1
#define M_ASN1_ID_INTEGER           2
#define M_ASN1_ID_BIT_STRING        3
#define M_ASN1_ID_OCTET_STRING      4
#define M_ASN1_ID_NULL              5
#define M_ASN1_ID_OBJECT_IDENTIFIER 6
#define M_ASN1_ID_UTF8STRING        12
#define M_ASN1_ID_SEQUENCE          16
#define M_ASN1_ID_SEQUENCEOF        M_ASN1_ID_SEQUENCE
#define M_ASN1_ID_SET               17
#define M_ASN1_ID_SETOF             M_ASN1_ID_SET
#define M_ASN1_ID_PRINTABLESTRING   19
#define M_ASN1_ID_T61STRING         20
#define M_ASN1_ID_IA5STRING         22
#define M_ASN1_ID_UTCTIME           23
#define M_ASN1_ID_GENERALIZEDTIME   24
#define mln_asn1_isSingleID(id,idmacro) ((id) == (idmacro))

typedef struct mln_asn1_deResult_s mln_asn1_deResult_t;

struct mln_asn1_deResult_s {
    mln_alloc_t                *pool;
    mln_u8ptr_t                 codeBuf;
    mln_u64_t                   codeLen;
    mln_asn1_deResult_t       **contents;
    mln_u32_t                   max;
    mln_u32_t                   pos;
    mln_u32_t                   _class:2;
    mln_u32_t                   isStruct:1;
    mln_u32_t                   ident:28;
    mln_u32_t                   free:1;
};

#define mln_asn1_deResult_getPool(pres)         ((pres)->pool)
#define mln_asn1_deResult_getClass(pres)        ((pres)->_class)
#define mln_asn1_deResult_getIdent(pres)        ((pres)->ident)
#define mln_asn1_deResult_setIdent(pres,id)     ((pres)->ident = (id))
#define mln_asn1_deResult_isStructure(pres)     ((pres)->isStruct)
#define mln_asn1_deResult_getCode(pres)         ((pres)->codeBuf)
#define mln_asn1_deResult_getCodeLength(pres)   ((pres)->codeLen)
#define mln_asn1_deResult_getNContent(pres)     ((pres)->pos)

extern mln_asn1_deResult_t *mln_asn1_decodeChain(mln_chain_t *in, int *err, mln_alloc_t *pool) __NONNULL3(1,2,3);
extern mln_asn1_deResult_t *mln_asn1_decode(void *data, mln_u64_t len, int *err, mln_alloc_t *pool) __NONNULL3(1,3,4);
extern mln_asn1_deResult_t *mln_asn1_decodeRef(void *data, mln_u64_t len, int *err, mln_alloc_t *pool) __NONNULL3(1,3,4);
extern void mln_asn1_deResult_free(mln_asn1_deResult_t *res);
extern mln_asn1_deResult_t *mln_asn1_deResult_getContent(mln_asn1_deResult_t *res, mln_u32_t index) __NONNULL1(1);
extern void mln_asn1_deResult_dump(mln_asn1_deResult_t *res);


typedef struct {
    mln_alloc_t             *pool;
    mln_u64_t                size;
    mln_string_t            *contents;
    mln_u32_t                max;
    mln_u32_t                pos;
    mln_u32_t                _class:2;
    mln_u32_t                isStruct:1;
    mln_u32_t                ident:29;
} mln_asn1_enResult_t;

#define mln_asn1_enResult_setPool(pres,p)             ((pres)->pool = (p))
#define mln_asn1_enResult_getPool(pres)               ((pres)->pool)
#define mln_asn1_enResult_setClass(pres,c)            ((pres)->_class = (c))
#define mln_asn1_enResult_getClass(pres)              ((pres)->_class)
#define mln_asn1_enResult_setIsStruct(pres)           ((pres)->isStruct = 1)
#define mln_asn1_enResult_resetIsStruct(pres)         ((pres)->isStruct = 0)
#define mln_asn1_enResult_getIsStruct(pres)           ((pres)->isStruct)
#define mln_asn1_enResult_setIdent(pres,id)           ((pres)->ident = (id))
#define mln_asn1_enResult_getIdent(pres)              ((pres)->ident)

extern int mln_asn1_enResult_init(mln_asn1_enResult_t *res, mln_alloc_t *pool) __NONNULL2(1,2);
extern void mln_asn1_enResult_destroy(mln_asn1_enResult_t *res);
extern mln_asn1_enResult_t *mln_asn1_enResult_new(mln_alloc_t *pool) __NONNULL1(1);
extern void mln_asn1_enResult_free(mln_asn1_enResult_t *res);
extern int mln_asn1_encode_boolean(mln_asn1_enResult_t *res, mln_u8_t val) __NONNULL1(1);
extern int mln_asn1_encode_integer(mln_asn1_enResult_t *res, mln_u8ptr_t ints, mln_u64_t nints) __NONNULL2(1,2);
extern int mln_asn1_encode_bitString(mln_asn1_enResult_t *res, mln_u8ptr_t bits, mln_u64_t nbits) __NONNULL2(1,2);
extern int mln_asn1_encode_octetString(mln_asn1_enResult_t *res, mln_u8ptr_t octets, mln_u64_t n) __NONNULL2(1,2);
extern int mln_asn1_encode_null(mln_asn1_enResult_t *res) __NONNULL1(1);
extern int mln_asn1_encode_objectIdentifier(mln_asn1_enResult_t *res, mln_u8ptr_t oid, mln_u64_t n) __NONNULL2(1,2);
extern int mln_asn1_encode_utf8String(mln_asn1_enResult_t *res, mln_u8ptr_t s, mln_u64_t slen) __NONNULL2(1,2);
extern int mln_asn1_encode_printableString(mln_asn1_enResult_t *res, mln_s8ptr_t s, mln_u64_t slen) __NONNULL2(1,2);
extern int mln_asn1_encode_t61String(mln_asn1_enResult_t *res, mln_u8ptr_t s, mln_u64_t slen) __NONNULL2(1,2);
extern int mln_asn1_encode_ia5String(mln_asn1_enResult_t *res, mln_u8ptr_t s, mln_u64_t slen) __NONNULL2(1,2);
extern int mln_asn1_encode_utctime(mln_asn1_enResult_t *res, time_t time) __NONNULL1(1);
extern int mln_asn1_encode_generalizedTime(mln_asn1_enResult_t *res, time_t time) __NONNULL1(1);
extern int mln_asn1_encode_sequence(mln_asn1_enResult_t *res) __NONNULL1(1);
#define mln_asn1_encode_sequenceOf(res) mln_asn1_encode_sequence(res)
extern int mln_asn1_encode_set(mln_asn1_enResult_t *res) __NONNULL1(1);
extern int mln_asn1_encode_setOf(mln_asn1_enResult_t *res) __NONNULL1(1);
extern int mln_asn1_encode_merge(mln_asn1_enResult_t *dest, mln_asn1_enResult_t *src) __NONNULL2(1,2);
extern int mln_asn1_encode_transChainOnce(mln_asn1_enResult_t *res, mln_chain_t **head, mln_chain_t **tail) __NONNULL3(1,2,3);
extern int mln_asn1_enResult_getContent(mln_asn1_enResult_t *res, mln_u32_t index, mln_u8ptr_t *buf, mln_u64_t *len) __NONNULL3(1,3,4);
extern int mln_asn1_encode_implicit(mln_asn1_enResult_t *res, mln_u32_t ident, mln_u32_t index) __NONNULL1(1);
#endif

