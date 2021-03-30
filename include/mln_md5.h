
/*
 * Copyright (C) Niklaus F.Schen.
 */
#ifndef __MLN_MD5_H
#define __MLN_MD5_H

#include "mln_types.h"
#include "mln_alloc.h"

#define __M_MD5_BUFLEN 64
#define __M_MD5_F(x,y,z) (((x)&(y))|((~(x))&(z)))
#define __M_MD5_G(x,y,z) (((x)&(z))|((y)&(~(z))))
#define __M_MD5_H(x,y,z) ((x)^(y)^(z))
#define __M_MD5_I(x,y,z) ((y)^((x)|(~(z))))
#define __M_MD5_ROTATE_LEFT(x,n) (((x) << (n)) | ((x) >> (32-(n))))
#define __M_MD5_FF(a,b,c,d,mj,s,ti) (a) = (b)+__M_MD5_ROTATE_LEFT(((a)+__M_MD5_F((b),(c),(d))+(mj)+(ti)), (s))
#define __M_MD5_GG(a,b,c,d,mj,s,ti) (a) = (b)+__M_MD5_ROTATE_LEFT(((a)+__M_MD5_G((b),(c),(d))+(mj)+(ti)), (s))
#define __M_MD5_HH(a,b,c,d,mj,s,ti) (a) = (b)+__M_MD5_ROTATE_LEFT(((a)+__M_MD5_H((b),(c),(d))+(mj)+(ti)), (s))
#define __M_MD5_II(a,b,c,d,mj,s,ti) (a) = (b)+__M_MD5_ROTATE_LEFT(((a)+__M_MD5_I((b),(c),(d))+(mj)+(ti)), (s))


typedef struct {
    mln_u32_t   A;
    mln_u32_t   B;
    mln_u32_t   C;
    mln_u32_t   D;
    mln_u64_t   length;
    mln_u32_t   pos;
    mln_u8_t    buf[__M_MD5_BUFLEN];
} mln_md5_t;

extern void mln_md5_init(mln_md5_t *m) __NONNULL1(1);
extern mln_md5_t *mln_md5_new(void);
extern mln_md5_t *mln_md5_pool_new(mln_alloc_t *pool) __NONNULL1(1);
extern void mln_md5_free(mln_md5_t *m);
extern void mln_md5_pool_free(mln_md5_t *m);
extern void mln_md5_calc(mln_md5_t *m, mln_u8ptr_t input, mln_uauto_t len, mln_u32_t is_last) __NONNULL1(1);
extern void mln_md5_tobytes(mln_md5_t *m, mln_u8ptr_t buf, mln_u32_t len) __NONNULL1(1);
extern void mln_md5_tostring(mln_md5_t *m, mln_s8ptr_t buf, mln_u32_t len) __NONNULL1(1);
extern void mln_md5_dump(mln_md5_t *m) __NONNULL1(1);
#endif

