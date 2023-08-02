
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
#define mln_asn1_is_single_id(id,idmacro) ((id) == (idmacro))

typedef struct mln_asn1_deresult_s mln_asn1_deresult_t;

struct mln_asn1_deresult_s {
    mln_alloc_t                *pool;
    mln_u8ptr_t                 code_buf;
    mln_u64_t                   code_len;
    mln_asn1_deresult_t       **contents;
    mln_u32_t                   max;
    mln_u32_t                   pos;
    mln_u32_t                   _class:2;
    mln_u32_t                   is_struct:1;
    mln_u32_t                   ident:28;
    mln_u32_t                   free:1;
};

#define mln_asn1_deresult_pool_get(pres)         ((pres)->pool)
#define mln_asn1_deresult_class_get(pres)        ((pres)->_class)
#define mln_asn1_deresult_ident_get(pres)        ((pres)->ident)
#define mln_asn1_deresult_ident_set(pres,id)     ((pres)->ident = (id))
#define mln_asn1_deresult_isstruct_get(pres)     ((pres)->is_struct)
#define mln_asn1_deresult_code_get(pres)         ((pres)->code_buf)
#define mln_asn1_deresult_code_length_get(pres)  ((pres)->code_len)
#define mln_asn1_deresult_content_num(pres)      ((pres)->pos)

extern mln_asn1_deresult_t *mln_asn1_decode_chain(mln_chain_t *in, int *err, mln_alloc_t *pool) __NONNULL3(1,2,3);
extern mln_asn1_deresult_t *mln_asn1_decode(void *data, mln_u64_t len, int *err, mln_alloc_t *pool) __NONNULL3(1,3,4);
extern mln_asn1_deresult_t *mln_asn1_decode_ref(void *data, mln_u64_t len, int *err, mln_alloc_t *pool) __NONNULL3(1,3,4);
extern void mln_asn1_deresult_free(mln_asn1_deresult_t *res);
extern mln_asn1_deresult_t *mln_asn1_deresult_content_get(mln_asn1_deresult_t *res, mln_u32_t index) __NONNULL1(1);
extern void mln_asn1_deresult_dump(mln_asn1_deresult_t *res);


typedef struct {
    mln_alloc_t             *pool;
    mln_u64_t                size;
    mln_string_t            *contents;
    mln_u32_t                max;
    mln_u32_t                pos;
    mln_u32_t                _class:2;
    mln_u32_t                is_struct:1;
    mln_u32_t                ident:29;
} mln_asn1_enresult_t;

#define mln_asn1_enresult_pool_set(pres,p)             ((pres)->pool = (p))
#define mln_asn1_enresult_pool_get(pres)               ((pres)->pool)
#define mln_asn1_enresult_class_set(pres,c)            ((pres)->_class = (c))
#define mln_asn1_enresult_class_get(pres)              ((pres)->_class)
#define mln_asn1_enresult_isstruct_set(pres)           ((pres)->is_struct = 1)
#define mln_asn1_enresult_isstruct_reset(pres)         ((pres)->is_struct = 0)
#define mln_asn1_enresult_isstruct_get(pres)           ((pres)->is_struct)
#define mln_asn1_enresult_ident_set(pres,id)           ((pres)->ident = (id))
#define mln_asn1_enresult_ident_get(pres)              ((pres)->ident)

extern int mln_asn1_enresult_init(mln_asn1_enresult_t *res, mln_alloc_t *pool) __NONNULL2(1,2);
extern void mln_asn1_enresult_destroy(mln_asn1_enresult_t *res);
extern mln_asn1_enresult_t *mln_asn1_enresult_new(mln_alloc_t *pool) __NONNULL1(1);
extern void mln_asn1_enresult_free(mln_asn1_enresult_t *res);
extern int mln_asn1_encode_boolean(mln_asn1_enresult_t *res, mln_u8_t val) __NONNULL1(1);
extern int mln_asn1_encode_integer(mln_asn1_enresult_t *res, mln_u8ptr_t ints, mln_u64_t nints) __NONNULL2(1,2);
extern int mln_asn1_encode_bitstring(mln_asn1_enresult_t *res, mln_u8ptr_t bits, mln_u64_t nbits) __NONNULL2(1,2);
extern int mln_asn1_encode_octetstring(mln_asn1_enresult_t *res, mln_u8ptr_t octets, mln_u64_t n) __NONNULL2(1,2);
extern int mln_asn1_encode_null(mln_asn1_enresult_t *res) __NONNULL1(1);
extern int mln_asn1_encode_object_identifier(mln_asn1_enresult_t *res, mln_u8ptr_t oid, mln_u64_t n) __NONNULL2(1,2);
extern int mln_asn1_encode_utf8string(mln_asn1_enresult_t *res, mln_u8ptr_t s, mln_u64_t slen) __NONNULL2(1,2);
extern int mln_asn1_encode_printablestring(mln_asn1_enresult_t *res, mln_s8ptr_t s, mln_u64_t slen) __NONNULL2(1,2);
extern int mln_asn1_encode_t61string(mln_asn1_enresult_t *res, mln_u8ptr_t s, mln_u64_t slen) __NONNULL2(1,2);
extern int mln_asn1_encode_ia5string(mln_asn1_enresult_t *res, mln_u8ptr_t s, mln_u64_t slen) __NONNULL2(1,2);
extern int mln_asn1_encode_utctime(mln_asn1_enresult_t *res, time_t time) __NONNULL1(1);
extern int mln_asn1_encode_generalized_time(mln_asn1_enresult_t *res, time_t time) __NONNULL1(1);
extern int mln_asn1_encode_sequence(mln_asn1_enresult_t *res) __NONNULL1(1);
#define mln_asn1_encode_sequenceof(res) mln_asn1_encode_sequence(res)
extern int mln_asn1_encode_set(mln_asn1_enresult_t *res) __NONNULL1(1);
extern int mln_asn1_encode_setof(mln_asn1_enresult_t *res) __NONNULL1(1);
extern int mln_asn1_encode_merge(mln_asn1_enresult_t *dest, mln_asn1_enresult_t *src) __NONNULL2(1,2);
extern int mln_asn1_encode_trans_chain_once(mln_asn1_enresult_t *res, mln_chain_t **head, mln_chain_t **tail) __NONNULL3(1,2,3);
extern int mln_asn1_enresult_get_content(mln_asn1_enresult_t *res, mln_u32_t index, mln_u8ptr_t *buf, mln_u64_t *len) __NONNULL3(1,3,4);
extern int mln_asn1_encode_implicit(mln_asn1_enresult_t *res, mln_u32_t ident, mln_u32_t index) __NONNULL1(1);
#endif

