
/*
 * Copyright (C) Niklaus F.Schen.
 */

#ifndef __MLN_GLOBAL_H
#define __MLN_GLOBAL_H
#include <stdio.h>
#include "mln_types.h"
#include "mln_conf.h"

#ifndef __MLN_DEFINE
#define EXTERN extern
#else
#define EXTERN
#endif


EXTERN mln_u32_t cr0;
EXTERN mln_u32_t cr1;
EXTERN mln_u32_t cr2;
EXTERN mln_u32_t cr3;

/*functions*/
extern void mln_global_init(void);
#endif

