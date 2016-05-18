
/*
 * Copyright (C) Niklaus F.Schen.
 */
#ifndef __MLN_SHA_H
#define __MLN_SHA_H

#include "mln_types.h"
#include "mln_alloc.h"

#define __M_SHA_BUFLEN 64
#define __M_SHA1_ROTATE_LEFT(x,n) (((x) << (n)) | (((x)&0xffffffff) >> (32-(n))))
#define __M_SHA1_F1(b,c,d) (((b)&(c))|((~(b))&(d)))
#define __M_SHA1_F2(b,c,d) ((b)^(c)^(d))
#define __M_SHA1_F3(b,c,d) (((b)&(c))|((d) & ((b)|(c))))
#define __M_SHA1_F4(b,c,d) ((b)^(c)^(d))
#define __M_SHA1_FF1(a,b,c,d,e,wt,kt); \
{\
    (e) += (__M_SHA1_ROTATE_LEFT((a),5)+__M_SHA1_F1((b),(c),(d))+(kt)+(wt));\
    (b) = __M_SHA1_ROTATE_LEFT((b), 30);\
}
#define __M_SHA1_FF2(a,b,c,d,e,wt,kt); \
{\
    (e) += (__M_SHA1_ROTATE_LEFT((a),5)+__M_SHA1_F2((b),(c),(d))+(kt)+(wt));\
    (b) = __M_SHA1_ROTATE_LEFT((b), 30);\
}
#define __M_SHA1_FF3(a,b,c,d,e,wt,kt); \
{\
    (e) += (__M_SHA1_ROTATE_LEFT((a),5)+__M_SHA1_F3((b),(c),(d))+(kt)+(wt));\
    (b) = __M_SHA1_ROTATE_LEFT((b), 30);\
}
#define __M_SHA1_FF4(a,b,c,d,e,wt,kt); \
{\
    (e) += (__M_SHA1_ROTATE_LEFT((a),5)+__M_SHA1_F4((b),(c),(d))+(kt)+(wt));\
    (b) = __M_SHA1_ROTATE_LEFT((b), 30);\
}
#define __M_SHA1_W(t,wt) \
((wt)[(t)&0xf] = __M_SHA1_ROTATE_LEFT((wt)[((t)-3)&0xf]^(wt)[((t)-8)&0xf]^(wt)[((t)-14)&0xf]^(wt)[(t)&0xf], 1))

typedef struct {
    mln_u32_t H0;
    mln_u32_t H1;
    mln_u32_t H2;
    mln_u32_t H3;
    mln_u32_t H4;
    mln_u64_t length;
    mln_u32_t pos;
    mln_u8_t  buf[__M_SHA_BUFLEN];
} mln_sha1_t;

extern void mln_sha1_init(mln_sha1_t *s) __NONNULL1(1);
extern mln_sha1_t *mln_sha1_new(void);
extern mln_sha1_t *mln_sha1_pool_new(mln_alloc_t *pool) __NONNULL1(1);
extern void mln_sha1_free(mln_sha1_t *s);
extern void mln_sha1_pool_free(mln_sha1_t *s);
extern void mln_sha1_calc(mln_sha1_t *s, mln_u8ptr_t input, mln_uauto_t len, mln_u32_t is_last) __NONNULL1(1);
extern void mln_sha1_toBytes(mln_sha1_t *s, mln_u8ptr_t buf, mln_u32_t len) __NONNULL1(1);
extern void mln_sha1_toString(mln_sha1_t *s, mln_s8ptr_t buf, mln_u32_t len) __NONNULL1(1);
extern void mln_sha1_dump(mln_sha1_t *s) __NONNULL1(1);
#endif

