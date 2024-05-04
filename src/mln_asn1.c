
/*
 * Copyright (C) Niklaus F.Schen.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mln_asn1.h"
#include "mln_tools.h"
#include "mln_func.h"

static mln_asn1_deresult_t *
mln_asn1_decode_recursive(mln_u8ptr_t *code, mln_u64_t *code_len, int *err, mln_alloc_t *pool, mln_u32_t free);
static int mln_encode_set_cmp(const void *data1, const void *data2);
static int mln_encode_setof_cmp(const void *data1, const void *data2);
static void mln_asn1_deresult_dump_recursive(mln_asn1_deresult_t *res, mln_u32_t nblank);

/*
 * deresult
 */
MLN_FUNC(static, mln_asn1_deresult_t *, mln_asn1_deresult_new, \
         (mln_alloc_t *pool, mln_u32_t _class, mln_u32_t is_struct, mln_u32_t ident, \
          mln_u8ptr_t code, mln_size_t code_len, mln_u32_t free),
         (pool, _class, is_struct, ident, code, code_len, free),
{
    mln_asn1_deresult_t *res = (mln_asn1_deresult_t *)mln_alloc_m(pool, sizeof(mln_asn1_deresult_t));
    if (res == NULL) return NULL;
    res->pool = pool;
    res->code_buf = code;
    res->code_len = code_len;
    res->contents = (mln_asn1_deresult_t **)mln_alloc_m(pool, M_ASN1_BUFSIZE*sizeof(mln_asn1_deresult_t *));
    if (res->contents == NULL) {
        mln_alloc_free(res);
        return NULL;
    }
    res->max = M_ASN1_BUFSIZE;
    res->pos = 0;
    res->_class = _class;
    res->is_struct = is_struct;
    res->ident = ident;
    res->free = free;
    return res;
})

MLN_FUNC_VOID(, void, mln_asn1_deresult_free, (mln_asn1_deresult_t *res), (res), {
    if (res == NULL) return;
    if (res->contents != NULL) {
        mln_asn1_deresult_t **p, **pend;
        pend = res->contents + res->pos;
        for (p = res->contents; p < pend; ++p) {
            mln_asn1_deresult_free(*p);
        }
        mln_alloc_free(res->contents);
    }
    if (res->free) mln_alloc_free(res->code_buf);
    mln_alloc_free(res);
})

/*
 * decode
 */
MLN_FUNC(, mln_asn1_deresult_t *, mln_asn1_decode_chain, \
         (mln_chain_t *in, int *err, mln_alloc_t *pool), \
         (in, err, pool), \
{
    mln_u8ptr_t code, p;
    mln_u64_t code_len = 0;
    mln_chain_t *c;
    mln_buf_t *b;
    mln_asn1_deresult_t *res;
    int n;

    for (c = in; c != NULL; c = c->next) {
        if (c->buf == NULL) continue;
        code_len += mln_buf_size(c->buf);
    }
    if ((code = (mln_u8ptr_t)mln_alloc_m(pool, code_len)) == NULL) {
        *err = M_ASN1_RET_ERROR;
        return NULL;
    }
    for (p = code, c = in; c != NULL; c = c->next) {
        if ((b = c->buf) == NULL) continue;
        if (b->in_memory) {
            memcpy(p, b->pos, mln_buf_size(b));
        } else {
            if (b->file == NULL) {
                mln_alloc_free(code);
                return NULL;
            }
            lseek(mln_file_fd(b->file), b->file_pos, SEEK_SET);
            if ((n = read(mln_file_fd(b->file), p, mln_buf_size(b))) != mln_buf_size(b)) {
                mln_alloc_free(code);
                return NULL;
            }
        }
        p += mln_buf_size(b);
    }

    p = code;
    if ((res = mln_asn1_decode_recursive(&p, &code_len, err, pool, 1)) == NULL) {
        mln_alloc_free(code);
        return NULL;
    }
    *err = M_ASN1_RET_OK;
    return res;
})

MLN_FUNC(, mln_asn1_deresult_t *, mln_asn1_decode, \
         (void *data, mln_u64_t len, int *err, mln_alloc_t *pool), \
         (data, len, err, pool), \
{
    mln_u8ptr_t buf = (mln_u8ptr_t)data;
    return mln_asn1_decode_recursive(&buf, &len, err, pool, 1);
})

MLN_FUNC(, mln_asn1_deresult_t *, mln_asn1_decode_ref, \
         (void *data, mln_u64_t len, int *err, mln_alloc_t *pool), \
         (data, len, err, pool), \
{
    mln_u8ptr_t buf = (mln_u8ptr_t)data;
    return mln_asn1_decode_recursive(&buf, &len, err, pool, 0);
})

MLN_FUNC(static, mln_asn1_deresult_t *, mln_asn1_decode_recursive, \
         (mln_u8ptr_t *code, mln_u64_t *code_len, int *err, mln_alloc_t *pool, mln_u32_t free), \
         (code, code_len, err, pool, free), \
{
    mln_u8ptr_t p = *code, end = *code + *code_len;
    mln_u32_t _class, struc, ident;
    mln_u8_t ch;
    mln_u64_t length = 0;
    mln_asn1_deresult_t *res, *sub_res;

    if (*code_len < 2) {
inc:
        *err = M_ASN1_RET_INCOMPLETE;
        return NULL;
    }
    /*Tag -- not support high-tag-number-form*/
    ch = *p++;
    _class = (ch >> 6) & 0x3;
    struc = (ch >> 5) & 0x1;
    ident = ch & 0x1f;

    /*length -- Don't support more than 8-byte length.*/
    ch = *p++;
    if (ch & 0x80) {
        if ((ch & 0x7f) > sizeof(mln_u64_t)) {
            *err = M_ASN1_RET_ERROR;
            return NULL;
        }
        mln_u8_t tmp[8] = {0}, *q, *qend;
        if (p+(ch&0x7f) >= end) goto inc;
        memcpy(tmp, p, ch&0x7f);
        p += (ch & 0x7f);
        for (q = tmp, qend = tmp+(ch&0x7f); q < qend; ++q) {
            length |= (*q << ((qend - q - 1) << 3));
        }
    } else {
        length = ch;
    }
    if (end - p < length) goto inc;

    if ((res = mln_asn1_deresult_new(pool, _class, struc, ident, *code, p-(*code)+length, free)) == NULL) {
        *err = M_ASN1_RET_ERROR;
        return NULL;
    }

    if (struc) {
        while (length > 0) {
            if ((sub_res = mln_asn1_decode_recursive(&p, &length, err, pool, 0)) == NULL) {
                res->free = 0;
                mln_asn1_deresult_free(res);
                return NULL;
            }
            if (res->pos >= res->max) {
                res->max += (res->max >> 1);
                mln_asn1_deresult_t **ptr;
                if ((ptr = (mln_asn1_deresult_t **)mln_alloc_re(pool, \
                                                                res->contents, \
                                                                res->max*sizeof(mln_asn1_deresult_t *))) == NULL)
                {
                    res->free = 0;
                    mln_asn1_deresult_free(res);
                    *err = M_ASN1_RET_ERROR;
                    return NULL;
                }
                res->contents = ptr;
            }
            res->contents[res->pos++] = sub_res;
        }
    } else {
        if ((sub_res = mln_asn1_deresult_new(pool, M_ASN1_CLASS_UNIVERSAL, 0, M_ASN1_ID_NONE, p, length, 0)) == NULL) {
            res->free = 0;
            mln_asn1_deresult_free(res);
            *err = M_ASN1_RET_ERROR;
            return NULL;
        }
        res->contents[res->pos++] = sub_res;
        p += length;
    }

    *code = p;
    *code_len = end - p;
    return res;
})

MLN_FUNC(, mln_asn1_deresult_t *, mln_asn1_deresult_content_get, \
         (mln_asn1_deresult_t *res, mln_u32_t index), (res, index), \
{
    if (res->contents == NULL) return NULL;
    if (res->pos <= index) return NULL;
    return res->contents[index];
})

MLN_FUNC_VOID(, void, mln_asn1_deresult_dump, (mln_asn1_deresult_t *res), (res), {
    if (res == NULL) return;
    mln_asn1_deresult_dump_recursive(res, 0);
})

MLN_FUNC_VOID(static, void, mln_asn1_deresult_dump_recursive, \
              (mln_asn1_deresult_t *res, mln_u32_t nblank), (res, nblank), \
{
    mln_asn1_deresult_t **p, **end;
    mln_u32_t i;
    char *classes[] = {"universal", "application", "context specific", "private"};
    char *idents[] = {
        "None", "Boolean", "Integer", "Bit string", "Octet string",
        "NULL", "Object identifier", "UTF8String", "Sequence/Sequence Of",
        "Set/Set Of", "PrintableString", "T61String", "IA5String", "UTCTime",
        "GeneralizedTime"
    };
    char *unknown = "Unknown";
    char *pclass, *pident;

    for (i = 0; i < nblank; ++i) {
        printf(" ");
    }
    switch (res->_class) {
        case M_ASN1_CLASS_UNIVERSAL:
            pclass = classes[0]; break;
        case M_ASN1_CLASS_APPLICATION:
            pclass = classes[1]; break;
        case M_ASN1_CLASS_CONTEXT_SPECIFIC:
            pclass = classes[2]; break;
        case M_ASN1_CLASS_PRIVATE:
            pclass = classes[3]; break;
        default:
            pclass = unknown; break;
    }
    switch (res->ident) {
        case M_ASN1_ID_NONE:
            pident = idents[0]; break;
        case M_ASN1_ID_BOOLEAN:
            pident = idents[1]; break;
        case M_ASN1_ID_INTEGER:
            pident = idents[2]; break;
        case M_ASN1_ID_BIT_STRING:
            pident = idents[3]; break;
        case M_ASN1_ID_OCTET_STRING:
            pident = idents[4]; break;
        case M_ASN1_ID_NULL:
            pident = idents[5]; break;
        case M_ASN1_ID_OBJECT_IDENTIFIER:
            pident = idents[6]; break;
        case M_ASN1_ID_UTF8STRING:
            pident = idents[7]; break;
        case M_ASN1_ID_SEQUENCE:
            pident = idents[8]; break;
        case M_ASN1_ID_SET:
            pident = idents[9]; break;
        case M_ASN1_ID_PRINTABLESTRING:
            pident = idents[10]; break;
        case M_ASN1_ID_T61STRING:
            pident = idents[11]; break;
        case M_ASN1_ID_IA5STRING:
            pident = idents[12]; break;
        case M_ASN1_ID_UTCTIME:
            pident = idents[13]; break;
        case M_ASN1_ID_GENERALIZEDTIME:
            pident = idents[14]; break;
        default:
            pident = unknown; break;
    }
    printf("class:[%s][0x%x] is_struct:[%u] ident:[%s][0x%x] free:[%u] nContent:[%u]\n", \
            pclass, res->_class, res->is_struct, pident, res->ident, res->free, res->pos);

    for (i = 0; i < nblank; ++i) {
        printf(" ");
    }
    printf("[");
    for (i = 0; i < res->code_len; ++i) {
        printf("%02x ", res->code_buf[i]);
    }
    printf("]\n");

    end = res->contents + res->pos;
    for (p = res->contents; p < end; ++p) {
        mln_asn1_deresult_dump_recursive(*p, nblank+2);
    }
})

/*
 * enresult
 */
MLN_FUNC(, int, mln_asn1_enresult_init, (mln_asn1_enresult_t *res, mln_alloc_t *pool), (res, pool), {
    res->pool = pool;
    res->size = 0;
    if ((res->contents = (mln_string_t *)mln_alloc_m(pool, \
                             M_ASN1_BUFSIZE*sizeof(mln_string_t))) == NULL)
        return M_ASN1_RET_ERROR;
    res->max = M_ASN1_BUFSIZE;
    res->pos = 0;
    res->_class = M_ASN1_CLASS_UNIVERSAL;
    res->is_struct = 0;
    res->ident = M_ASN1_ID_NONE;
    return M_ASN1_RET_OK;
})

MLN_FUNC_VOID(, void, mln_asn1_enresult_destroy, (mln_asn1_enresult_t *res), (res), {
    if (res == NULL) return;
    if (res->contents != NULL) {
        mln_string_t *s, *send;
        send = res->contents + res->pos;
        for (s = res->contents; s < send; ++s) {
            mln_alloc_free(s->data);
        }
        mln_alloc_free(res->contents);
    }
})

MLN_FUNC(, mln_asn1_enresult_t *, mln_asn1_enresult_new, (mln_alloc_t *pool), (pool), {
    mln_asn1_enresult_t *res = (mln_asn1_enresult_t *)mln_alloc_m(pool, sizeof(mln_asn1_enresult_t));
    if (res == NULL) return NULL;
    if (mln_asn1_enresult_init(res, pool) != M_ASN1_RET_OK) {
        mln_alloc_free(res);
        return NULL;
    }
    return res;
})

MLN_FUNC_VOID(, void, mln_asn1_enresult_free, (mln_asn1_enresult_t *res), (res), {
    if (res == NULL) return;
    mln_asn1_enresult_destroy(res);
    mln_alloc_free(res);
})

/*
 * encode
 */
MLN_FUNC(static, int, mln_asn1_encode_add_content, \
         (mln_asn1_enresult_t *res, mln_u8ptr_t buf, mln_u64_t len), \
         (res, buf, len), \
{
    if (res->pos == res->max) {
        res->max += ((res->max)>>1);
        mln_string_t *ptr = (mln_string_t *)mln_alloc_re(res->pool, \
                                           res->contents, res->max*sizeof(mln_string_t));
        if (ptr == NULL) return -1;
        res->contents = ptr;
    }
    mln_string_t *s = &res->contents[res->pos++];
    s->data = buf;
    s->len = len;
    s->data_ref = 0;
    s->pool = 1;
    s->ref = 1;
    return 0;
})

#define mln_asn1_encode_length_calc(bytes,len); \
{\
    (len) = (bytes) + 1;\
    if ((mln_u64_t)(bytes) > 127) {\
        if ((mln_u64_t)(bytes) > 0xff) {\
            if ((mln_u64_t)(bytes) > 0xffff) {\
                if ((mln_u64_t)(bytes) > 0xffffff) {\
                    if ((mln_u64_t)(bytes) > 0xffffffff) {\
                        if ((mln_u64_t)(bytes) > 0xffffffffffllu) {\
                            if ((mln_u64_t)(bytes) > 0xffffffffffffllu) {\
                                if ((mln_u64_t)(bytes) > 0xffffffffffffffllu) {\
                                    (len) += 9;\
                                } else {\
                                    (len) += 8;\
                                }\
                            } else {\
                                (len) += 7;\
                            }\
                        } else {\
                            (len) += 6;\
                        }\
                    } else {\
                        (len) += 5;\
                    }\
                } else {\
                    (len) += 4;\
                }\
            } else {\
                (len) += 3;\
            }\
        } else {\
            (len) += 2;\
        }\
    } else {\
        ++(len);\
    }\
}

#define mln_asn1_encode_length_fill(bytes,ptr); \
{\
    if ((mln_u64_t)(bytes) > 127) {\
        if ((mln_u64_t)(bytes) > 0xff) {\
            if ((mln_u64_t)(bytes) > 0xffff) {\
                if ((mln_u64_t)(bytes) > 0xffffff) {\
                    if ((mln_u64_t)(bytes) > 0xffffffff) {\
                        if ((mln_u64_t)(bytes) > 0xffffffffffllu) {\
                            if ((mln_u64_t)(bytes) > 0xffffffffffffllu) {\
                                if ((mln_u64_t)(bytes) > 0xffffffffffffffllu) {\
                                    *(ptr)++ = 0x80|8;\
                                    *(ptr)++ = ((mln_u64_t)(bytes) >> 56) & 0xff;\
                                    *(ptr)++ = ((mln_u64_t)(bytes) >> 48) & 0xff;\
                                    *(ptr)++ = ((mln_u64_t)(bytes) >> 40) & 0xff;\
                                    *(ptr)++ = ((mln_u64_t)(bytes) >> 32) & 0xff;\
                                    *(ptr)++ = ((mln_u64_t)(bytes) >> 24) & 0xff;\
                                    *(ptr++) = ((mln_u64_t)(bytes) >> 16) & 0xff;\
                                    *(ptr)++ = ((mln_u64_t)(bytes) >> 8) & 0xff;\
                                    *(ptr)++ = (mln_u64_t)(bytes) & 0xff;\
                                } else {\
                                    *(ptr)++ = 0x80|7;\
                                    *(ptr)++ = ((mln_u64_t)(bytes) >> 48) & 0xff;\
                                    *(ptr)++ = ((mln_u64_t)(bytes) >> 40) & 0xff;\
                                    *(ptr)++ = ((mln_u64_t)(bytes) >> 32) & 0xff;\
                                    *(ptr)++ = ((mln_u64_t)(bytes) >> 24) & 0xff;\
                                    *(ptr++) = ((mln_u64_t)(bytes) >> 16) & 0xff;\
                                    *(ptr)++ = ((mln_u64_t)(bytes) >> 8) & 0xff;\
                                    *(ptr)++ = (mln_u64_t)(bytes) & 0xff;\
                                }\
                            } else {\
                                *(ptr)++ = 0x80|6;\
                                *(ptr)++ = ((mln_u64_t)(bytes) >> 40) & 0xff;\
                                *(ptr)++ = ((mln_u64_t)(bytes) >> 32) & 0xff;\
                                *(ptr)++ = ((mln_u64_t)(bytes) >> 24) & 0xff;\
                                *(ptr++) = ((mln_u64_t)(bytes) >> 16) & 0xff;\
                                *(ptr)++ = ((mln_u64_t)(bytes) >> 8) & 0xff;\
                                *(ptr)++ = (mln_u64_t)(bytes) & 0xff;\
                            }\
                        } else {\
                            *(ptr)++ = 0x80|5;\
                            *(ptr)++ = ((mln_u64_t)(bytes) >> 32) & 0xff;\
                            *(ptr)++ = ((mln_u64_t)(bytes) >> 24) & 0xff;\
                            *(ptr++) = ((mln_u64_t)(bytes) >> 16) & 0xff;\
                            *(ptr)++ = ((mln_u64_t)(bytes) >> 8) & 0xff;\
                            *(ptr)++ = (mln_u64_t)(bytes) & 0xff;\
                        }\
                    } else {\
                        *(ptr)++ = 0x80|4;\
                        *(ptr)++ = ((mln_u64_t)(bytes) >> 24) & 0xff;\
                        *(ptr++) = ((mln_u64_t)(bytes) >> 16) & 0xff;\
                        *(ptr)++ = ((mln_u64_t)(bytes) >> 8) & 0xff;\
                        *(ptr)++ = (mln_u64_t)(bytes) & 0xff;\
                    }\
                } else {\
                    *(ptr)++ = 0x80|3;\
                    *(ptr)++ = ((mln_u64_t)(bytes) >> 16) & 0xff;\
                    *(ptr)++ = ((mln_u64_t)(bytes) >> 8) & 0xff;\
                    *(ptr)++ = (mln_u64_t)(bytes) & 0xff;\
                }\
            } else {\
                *(ptr)++ = 0x80|2;\
                *(ptr)++ = ((mln_u64_t)(bytes) >> 8) & 0xff;\
                *(ptr)++ = (mln_u64_t)(bytes) & 0xff;\
            }\
        } else {\
            *(ptr)++ = 0x80|1;\
            *(ptr)++ = (bytes);\
        }\
    } else {\
        *(ptr)++ = (bytes);\
    }\
}

MLN_FUNC(, int, mln_asn1_encode_sequence, (mln_asn1_enresult_t *res), (res), {
    mln_u64_t len = 0;
    mln_u8ptr_t buf, p;
    mln_alloc_t *pool;
    mln_string_t *s, *send;

    mln_asn1_encode_length_calc(res->size, len);

    pool = mln_asn1_enresult_pool_get(res);
    if ((buf = (mln_u8ptr_t)mln_alloc_m(pool, len)) == NULL) {
        return M_ASN1_RET_ERROR;
    }

    p = buf + 1;
    buf[0] = (res->_class << 6) | (1 << 5) | ((res->ident != M_ASN1_ID_NONE)? (res->ident & 0x1f): M_ASN1_ID_SEQUENCE);
    mln_asn1_encode_length_fill(res->size, p);
    send = res->contents + res->pos;
    for (s = res->contents; s < send; ++s) {
        memcpy(p, s->data, s->len);
        p += s->len;
        mln_alloc_free(s->data);
    }
    res->pos = 0;
    if (mln_asn1_encode_add_content(res, buf, len) < 0) {
        mln_alloc_free(buf);
        return M_ASN1_RET_ERROR;
    }
    res->size = len;

    return M_ASN1_RET_OK;
})

MLN_FUNC(, int, mln_asn1_encode_boolean, (mln_asn1_enresult_t *res, mln_u8_t val), (res, val), {
    mln_u64_t len = 0;
    mln_alloc_t *pool;
    mln_u8ptr_t buf, p;

    mln_asn1_encode_length_calc(1, len);

    pool = mln_asn1_enresult_pool_get(res);
    if ((buf = (mln_u8ptr_t)mln_alloc_m(pool, len)) == NULL) {
        return M_ASN1_RET_ERROR;
    }

    p = buf + 1;
    buf[0] = (res->_class << 6) | ((res->is_struct)?(1<<5):0) | ((res->ident != M_ASN1_ID_NONE)? (res->ident & 0x1f): M_ASN1_ID_BOOLEAN);
    mln_asn1_encode_length_fill(1, p);
    *p = val;

    if (mln_asn1_encode_add_content(res, buf, len) < 0) {
        mln_alloc_free(buf);
        return M_ASN1_RET_ERROR;
    }
    res->size += len;

    return M_ASN1_RET_OK;
})

MLN_FUNC(, int, mln_asn1_encode_integer, (mln_asn1_enresult_t *res, mln_u8ptr_t ints, mln_u64_t nints), (res, ints, nints), {
    mln_u64_t len = 0;
    mln_alloc_t *pool;
    mln_u8ptr_t buf, p;

    mln_asn1_encode_length_calc(nints, len);

    pool = mln_asn1_enresult_pool_get(res);
    if ((buf = (mln_u8ptr_t)mln_alloc_m(pool, len)) == NULL) {
        return M_ASN1_RET_ERROR;
    }
    
    p = buf + 1;
    buf[0] = (res->_class << 6) | ((res->is_struct)?(1<<5):0) | ((res->ident != M_ASN1_ID_NONE)? (res->ident & 0x1f): M_ASN1_ID_INTEGER);
    mln_asn1_encode_length_fill(nints, p);
    memcpy(p, ints, nints);

    if (mln_asn1_encode_add_content(res, buf, len) < 0) {
        mln_alloc_free(buf);
        return M_ASN1_RET_ERROR;
    }
    res->size += len;

    return M_ASN1_RET_OK;
})

MLN_FUNC(, int, mln_asn1_encode_bitstring, (mln_asn1_enresult_t *res, mln_u8ptr_t bits, mln_u64_t nbits), (res, bits, nbits), {
    mln_u64_t len = 0, bytes = 1 + (nbits % 8? (nbits>>3)+1: (nbits>>3));
    mln_u8ptr_t buf, p;
    mln_alloc_t *pool;

    mln_asn1_encode_length_calc(bytes, len);

    pool = mln_asn1_enresult_pool_get(res);
    if ((buf = (mln_u8ptr_t)mln_alloc_m(pool, len)) == NULL) {
        return M_ASN1_RET_ERROR;
    }

    p = buf + 1;
    buf[0] = (res->_class << 6) | ((res->is_struct)?(1<<5):0) | ((res->ident != M_ASN1_ID_NONE)? (res->ident & 0x1f): M_ASN1_ID_BIT_STRING);
    mln_asn1_encode_length_fill(bytes, p);
    *p++ = (nbits % 8)? 8-(nbits % 8): 0;
    if (bytes-1) memcpy(p, bits, bytes-1);
    p += (bytes-1);
    if (nbits % 8) {
        --p;
        *p = ((*p >> (8 - (nbits % 8))) & 0xff) << (8 - (nbits % 8));
    }

    if (mln_asn1_encode_add_content(res, buf, len) < 0) {
        mln_alloc_free(buf);
        return M_ASN1_RET_ERROR;
    }
    res->size += len;

    return M_ASN1_RET_OK;
})

MLN_FUNC(, int, mln_asn1_encode_octetstring, (mln_asn1_enresult_t *res, mln_u8ptr_t octets, mln_u64_t n), (res, octets, n), {
    mln_u64_t len = 0;
    mln_u8ptr_t buf, p;
    mln_alloc_t *pool;

    mln_asn1_encode_length_calc(n, len);

    pool = mln_asn1_enresult_pool_get(res);
    if ((buf = (mln_u8ptr_t)mln_alloc_m(pool, len)) == NULL) {
        return M_ASN1_RET_ERROR;
    }

    p = buf + 1;
    buf[0] = (res->_class << 6) | ((res->is_struct)?(1<<5):0) | ((res->ident != M_ASN1_ID_NONE)? (res->ident & 0x1f): M_ASN1_ID_OCTET_STRING);
    mln_asn1_encode_length_fill(n, p);
    memcpy(p, octets, n);
    if (mln_asn1_encode_add_content(res, buf, len) < 0) {
        mln_alloc_free(buf);
        return M_ASN1_RET_ERROR;
    }
    res->size += len;

    return M_ASN1_RET_OK;
})

MLN_FUNC(, int, mln_asn1_encode_null, (mln_asn1_enresult_t *res), (res), {
    mln_u8ptr_t buf;
    mln_alloc_t *pool = mln_asn1_enresult_pool_get(res);

    if ((buf = (mln_u8ptr_t)mln_alloc_m(pool, 2)) == NULL) {
        return M_ASN1_RET_ERROR;
    }

    buf[0] = (res->_class << 6) | ((res->is_struct)?(1<<5):0) | ((res->ident != M_ASN1_ID_NONE)? (res->ident & 0x1f): M_ASN1_ID_NULL);
    buf[1] = 0;
    if (mln_asn1_encode_add_content(res, buf, 2) < 0) {
        mln_alloc_free(buf);
        return M_ASN1_RET_ERROR;
    }
    res->size += 2;

    return M_ASN1_RET_OK;
})

MLN_FUNC(, int, mln_asn1_encode_object_identifier, \
         (mln_asn1_enresult_t *res, mln_u8ptr_t oid, mln_u64_t n), (res, oid, n), \
{
    mln_u64_t len = 0;
    mln_u8ptr_t buf, p;
    mln_alloc_t *pool;

    mln_asn1_encode_length_calc(n, len);

    pool = mln_asn1_enresult_pool_get(res);
    if ((buf = (mln_u8ptr_t)mln_alloc_m(pool, len)) == NULL) {
        return M_ASN1_RET_ERROR;
    }

    p = buf + 1;
    buf[0] = (res->_class << 6) | ((res->is_struct)?(1<<5):0) | ((res->ident != M_ASN1_ID_NONE)? (res->ident & 0x1f): M_ASN1_ID_OBJECT_IDENTIFIER);
    mln_asn1_encode_length_fill(n, p);
    memcpy(p, oid, n);
    if (mln_asn1_encode_add_content(res, buf, len) < 0) {
        mln_alloc_free(buf);
        return M_ASN1_RET_ERROR;
    }
    res->size += len;

    return M_ASN1_RET_OK;
})

MLN_FUNC(, int, mln_asn1_encode_utf8string, (mln_asn1_enresult_t *res, mln_u8ptr_t s, mln_u64_t slen), (res, s, slen), {
    mln_u64_t len = 0;
    mln_u8ptr_t buf, p;
    mln_alloc_t *pool;

    mln_asn1_encode_length_calc(slen, len);

    pool = mln_asn1_enresult_pool_get(res);
    if ((buf = (mln_u8ptr_t)mln_alloc_m(pool, len)) == NULL) {
        return M_ASN1_RET_ERROR;
    }

    p = buf + 1;
    buf[0] = (res->_class << 6) | ((res->is_struct)?(1<<5):0) | ((res->ident != M_ASN1_ID_NONE)? (res->ident & 0x1f): M_ASN1_ID_UTF8STRING);
    mln_asn1_encode_length_fill(slen, p);
    memcpy(p, s, slen);
    if (mln_asn1_encode_add_content(res, buf, len) < 0) {
        mln_alloc_free(buf);
        return M_ASN1_RET_ERROR;
    }
    res->size += len;

    return M_ASN1_RET_OK;
})

MLN_FUNC(, int, mln_asn1_encode_printablestring, (mln_asn1_enresult_t *res, mln_s8ptr_t s, mln_u64_t slen), (res, s, slen), {
    mln_s8ptr_t scan, end;
    for (scan = s, end = s+slen; scan < end; ++scan) {
        if (!mln_isprint(*scan)) return M_ASN1_RET_ERROR;
    }

    mln_u64_t len = 0;
    mln_u8ptr_t buf, p;
    mln_alloc_t *pool;

    mln_asn1_encode_length_calc(slen, len);

    pool = mln_asn1_enresult_pool_get(res);
    if ((buf = (mln_u8ptr_t)mln_alloc_m(pool, len)) == NULL) {
        return M_ASN1_RET_ERROR;
    }

    p = buf + 1;
    buf[0] = (res->_class << 6) | ((res->is_struct)?(1<<5):0) | ((res->ident != M_ASN1_ID_NONE)? (res->ident & 0x1f): M_ASN1_ID_PRINTABLESTRING);
    mln_asn1_encode_length_fill(slen, p);
    memcpy(p, s, slen);
    if (mln_asn1_encode_add_content(res, buf, len) < 0) {
        mln_alloc_free(buf);
        return M_ASN1_RET_ERROR;
    }
    res->size += len;

    return M_ASN1_RET_OK;
})

MLN_FUNC(, int, mln_asn1_encode_t61string, (mln_asn1_enresult_t *res, mln_u8ptr_t s, mln_u64_t slen), (res, s, slen), {
    mln_u64_t len = 0;
    mln_u8ptr_t buf, p;
    mln_alloc_t *pool;

    mln_asn1_encode_length_calc(slen, len);

    pool = mln_asn1_enresult_pool_get(res);
    if ((buf = (mln_u8ptr_t)mln_alloc_m(pool, len)) == NULL) {
        return M_ASN1_RET_ERROR;
    }

    p = buf + 1;
    buf[0] = (res->_class << 6) | ((res->is_struct)?(1<<5):0) | ((res->ident != M_ASN1_ID_NONE)? (res->ident & 0x1f): M_ASN1_ID_T61STRING);
    mln_asn1_encode_length_fill(slen, p);
    memcpy(p, s, slen);
    if (mln_asn1_encode_add_content(res, buf, len) < 0) {
        mln_alloc_free(buf);
        return M_ASN1_RET_ERROR;
    }
    res->size += len;

    return M_ASN1_RET_OK;
})

MLN_FUNC(, int, mln_asn1_encode_ia5string, (mln_asn1_enresult_t *res, mln_u8ptr_t s, mln_u64_t slen), (res, s, slen), {
    mln_u64_t len = 0;
    mln_u8ptr_t buf, p;
    mln_alloc_t *pool;

    mln_asn1_encode_length_calc(slen, len);

    pool = mln_asn1_enresult_pool_get(res);
    if ((buf = (mln_u8ptr_t)mln_alloc_m(pool, len)) == NULL) {
        return M_ASN1_RET_ERROR;
    }

    p = buf + 1;
    buf[0] = (res->_class << 6) | ((res->is_struct)?(1<<5):0) | ((res->ident != M_ASN1_ID_NONE)? (res->ident & 0x1f): M_ASN1_ID_IA5STRING);
    mln_asn1_encode_length_fill(slen, p);
    memcpy(p, s, slen);
    if (mln_asn1_encode_add_content(res, buf, len) < 0) {
        mln_alloc_free(buf);
        return M_ASN1_RET_ERROR;
    }
    res->size += len;

    return M_ASN1_RET_OK;
})

MLN_FUNC(, int, mln_asn1_encode_utctime, (mln_asn1_enresult_t *res, time_t time), (res, time), {
    struct utctime uc;
    mln_u64_t len = 0;
    mln_u8ptr_t buf, p;
    mln_alloc_t *pool;

    mln_time2utc(time, &uc);
    if (uc.year > 2000) uc.year -= 2000;
    else uc.year -= 1900;

    mln_asn1_encode_length_calc(13, len);

    pool = mln_asn1_enresult_pool_get(res);
    if ((buf = (mln_u8ptr_t)mln_alloc_m(pool, len)) == NULL) {
        return M_ASN1_RET_ERROR;
    }

    p = buf + 1;
    buf[0] = (res->_class << 6) | ((res->is_struct)?(1<<5):0) | ((res->ident != M_ASN1_ID_NONE)? (res->ident & 0x1f): M_ASN1_ID_UTCTIME);
    mln_asn1_encode_length_fill(13, p);
    *p++ = uc.year/10 + '0';
    *p++ = uc.year%10 + '0';
    *p++ = uc.month/10 + '0';
    *p++ = uc.month%10 + '0';
    *p++ = uc.day/10 + '0';
    *p++ = uc.day%10 + '0';
    *p++ = uc.hour/10 + '0';
    *p++ = uc.hour%10 + '0';
    *p++ = uc.minute/10 + '0';
    *p++ = uc.minute%10 + '0';
    *p++ = uc.second/10 + '0';
    *p++ = uc.second%10 + '0';
    *p = 'Z';
    if (mln_asn1_encode_add_content(res, buf, len) < 0) {
        mln_alloc_free(buf);
        return M_ASN1_RET_ERROR;
    }
    res->size += len;

    return M_ASN1_RET_OK;
})

MLN_FUNC(, int, mln_asn1_encode_generalized_time, (mln_asn1_enresult_t *res, time_t time), (res, time), {
    struct utctime uc;
    mln_u64_t len = 0;
    mln_u8ptr_t buf, p;
    mln_alloc_t *pool;

    mln_time2utc(time, &uc);

    mln_asn1_encode_length_calc(15, len);

    pool = mln_asn1_enresult_pool_get(res);
    if ((buf = (mln_u8ptr_t)mln_alloc_m(pool, len)) == NULL) {
        return M_ASN1_RET_ERROR;
    }

    p = buf + 1;
    buf[0] = (res->_class << 6) | ((res->is_struct)?(1<<5):0) | ((res->ident != M_ASN1_ID_NONE)? (res->ident & 0x1f): M_ASN1_ID_GENERALIZEDTIME);
    mln_asn1_encode_length_fill(15, p);
    *p++ = uc.year/1000 + '0';
    *p++ = (uc.year%1000)/100 + '0';
    *p++ = (uc.year%100)/10 + '0';
    *p++ = uc.year%10 + '0';
    *p++ = uc.month/10 + '0';
    *p++ = uc.month%10 + '0';
    *p++ = uc.day/10 + '0';
    *p++ = uc.day%10 + '0';
    *p++ = uc.hour/10 + '0';
    *p++ = uc.hour%10 + '0';
    *p++ = uc.minute/10 + '0';
    *p++ = uc.minute%10 + '0';
    *p++ = uc.second/10 + '0';
    *p++ = uc.second%10 + '0';
    *p = 'Z';
    if (mln_asn1_encode_add_content(res, buf, len) < 0) {
        mln_alloc_free(buf);
        return M_ASN1_RET_ERROR;
    }
    res->size += len;

    return M_ASN1_RET_OK;
})

MLN_FUNC(, int, mln_asn1_encode_set, (mln_asn1_enresult_t *res), (res), {
    mln_u64_t len = 0;
    mln_u8ptr_t buf = NULL, p;
    mln_alloc_t *pool = mln_asn1_enresult_pool_get(res);
    mln_string_t *s, *send;

    mln_asn1_encode_length_calc(res->size, len);
    if ((buf = (mln_u8ptr_t)mln_alloc_m(pool, len)) == NULL) return M_ASN1_RET_ERROR;
    buf[0] = (res->_class << 6) | (1 << 5) | ((res->ident != M_ASN1_ID_NONE)? (res->ident & 0x1f): M_ASN1_ID_SET);
    p = buf + 1;
    mln_asn1_encode_length_fill(res->size, p);

    qsort(res->contents, res->pos, sizeof(mln_string_t), mln_encode_set_cmp);
    send = res->contents + res->pos;
    for (s = res->contents; s < send; ++s) {
        memcpy(p, s->data, s->len);
        p += s->len;
        mln_alloc_free(s->data);
    }
    res->pos = 0;
    if (mln_asn1_encode_add_content(res, buf, len) < 0) {
        mln_alloc_free(buf);
        return M_ASN1_RET_ERROR;
    }
    res->size = len;
    
    return M_ASN1_RET_OK;
})

MLN_FUNC(static, int, mln_encode_set_cmp, (const void *data1, const void *data2), (data1, data2), {
    mln_string_t *s1 = (mln_string_t *)data1;
    mln_string_t *s2 = (mln_string_t *)data2;
    if (s1->data[0] > s2->data[0]) return 1;
    if (s1->data[0] == s2->data[0]) return 0;
    return -1;
})

MLN_FUNC(, int, mln_asn1_encode_setof, (mln_asn1_enresult_t *res), (res), {
    mln_u64_t len = 0;
    mln_u8ptr_t buf = NULL, p;
    mln_alloc_t *pool = mln_asn1_enresult_pool_get(res);
    mln_string_t *s, *send;

    mln_asn1_encode_length_calc(res->size, len);
    if ((buf = (mln_u8ptr_t)mln_alloc_m(pool, len)) == NULL) return M_ASN1_RET_ERROR;
    buf[0] = (res->_class << 6) | (1 << 5) | ((res->ident != M_ASN1_ID_NONE)? (res->ident & 0x1f): M_ASN1_ID_SET);
    p = buf + 1;
    mln_asn1_encode_length_fill(res->size, p);

    qsort(res->contents, res->pos, sizeof(mln_string_t), mln_encode_setof_cmp);
    send = res->contents + res->pos;
    for (s = res->contents; s < send; ++s) {
        memcpy(p, s->data, s->len);
        p += s->len;
        mln_alloc_free(s->data);
    }
    res->pos = 0;
    if (mln_asn1_encode_add_content(res, buf, len) < 0) {
        mln_alloc_free(buf);
        return M_ASN1_RET_ERROR;
    }
    res->size = len;
    
    return M_ASN1_RET_OK;
})

MLN_FUNC(static, int, mln_encode_setof_cmp, (const void *data1, const void *data2), (data1, data2), {
    mln_string_t *s1 = (mln_string_t *)data1;
    mln_string_t *s2 = (mln_string_t *)data2;
    mln_size_t len = s1->len > s2->len? s2->len: s1->len;
    int ret = memcmp(s1->data, s2->data, len);
    if (ret != 0) return ret;

    if (s1->len > s2->len) return 1;
    if (s1->len < s2->len) return -1;
    return 0;
})

MLN_FUNC(, int, mln_asn1_encode_merge, (mln_asn1_enresult_t *dest, mln_asn1_enresult_t *src), (dest, src), {
    mln_string_t *s, *send;
    mln_alloc_t *pool = mln_asn1_enresult_pool_get(dest);
    mln_u8ptr_t buf;

    send = src->contents + src->pos;
    for (s = src->contents; s < send; ++s) {
        if ((buf = (mln_u8ptr_t)mln_alloc_m(pool, s->len)) == NULL) return M_ASN1_RET_ERROR;
        memcpy(buf, s->data, s->len);
        if (mln_asn1_encode_add_content(dest, buf, s->len) < 0) {
            return M_ASN1_RET_ERROR;
        }
    }
    dest->size += src->size;

    return M_ASN1_RET_OK;
})

MLN_FUNC(, int, mln_asn1_encode_trans_chain_once, \
         (mln_asn1_enresult_t *res, mln_chain_t **head, mln_chain_t **tail), \
         (res, head, tail), \
{
    mln_chain_t *c;
    mln_buf_t *b;
    mln_string_t *s, *send;
    mln_alloc_t *pool = mln_asn1_enresult_pool_get(res);

    send = res->contents + res->pos;
    for (s = res->contents; s < send; ++s) {
        if ((c = mln_chain_new(pool)) == NULL) return M_ASN1_RET_ERROR;
        if ((b = c->buf = mln_buf_new(pool)) == NULL) {
            mln_chain_pool_release(c);
            return M_ASN1_RET_ERROR;
        }
        b->left_pos = b->pos = b->start = s->data;
        b->end = b->last = s->data + s->len;
        b->in_memory = 1;
        b->last_buf = 1;

        if (*head == NULL) {
            *head = *tail = c;
        } else {
            (*tail)->next = c;
            *tail = c;
        }
    }
    res->pos = 0;
    res->size = 0;
    return M_ASN1_RET_OK;
})

MLN_FUNC(, int, mln_asn1_enresult_get_content, \
         (mln_asn1_enresult_t *res, mln_u32_t index, mln_u8ptr_t *buf, mln_u64_t *len), \
         (res, index, buf, len), \
{
    if (index >= res->pos) return M_ASN1_RET_ERROR;

    mln_string_t *s = &(res->contents[index]);
    *buf = s->data;
    *len = s->len;
    return M_ASN1_RET_OK;
})

MLN_FUNC(, int, mln_asn1_encode_implicit, \
         (mln_asn1_enresult_t *res, mln_u32_t ident, mln_u32_t index), \
         (res, ident, index), \
{
    if (index >= res->pos) return M_ASN1_RET_ERROR;
    mln_string_t *s = &(res->contents[index]);
    s->data[0] &= 0x20;
    s->data[0] |= ((M_ASN1_CLASS_CONTEXT_SPECIFIC << 6)| ident);
    return M_ASN1_RET_OK;
})

