
/*
 * Copyright (C) Niklaus F.Schen.
 */
#ifndef __MLN_RC_H
#define __MLN_RC_H

#include "mln_types.h"

extern void mln_rc4_init(mln_u8ptr_t s, mln_u8ptr_t key, mln_uauto_t len) __NONNULL2(1,2);
extern void mln_rc4_calc(mln_u8ptr_t s, mln_u8ptr_t data, mln_uauto_t len) __NONNULL2(1,2);

#endif

