
/*
 * Copyright (C) Niklaus F.Schen.
 */
#ifndef __MLN_BASE64_H
#define __MLN_BASE64_H

#include "mln_types.h"
#include "mln_alloc.h"

extern int
mln_base64_encode(mln_u8ptr_t in, mln_uauto_t inlen, mln_u8ptr_t *out, mln_uauto_t *outlen) __NONNULL3(1,3,4);
extern int
mln_base64_pool_encode(mln_alloc_t *pool, mln_u8ptr_t in, mln_uauto_t inlen, mln_u8ptr_t *out, mln_uauto_t *outlen) __NONNULL4(1,2,4,5);
extern int
mln_base64_decode(mln_u8ptr_t in, mln_uauto_t inlen, mln_u8ptr_t *out, mln_uauto_t *outlen) __NONNULL3(1,3,4);
extern int
mln_base64_pool_decode(mln_alloc_t *pool, mln_u8ptr_t in, mln_uauto_t inlen, mln_u8ptr_t *out, mln_uauto_t *outlen) __NONNULL4(1,2,4,5);
extern void mln_base64_free(mln_u8ptr_t data);
extern void mln_base64_pool_free(mln_u8ptr_t data);

#endif

