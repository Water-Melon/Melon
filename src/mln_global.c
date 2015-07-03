
/*
 * Copyright (C) Niklaus F.Schen.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "mln_log.h"
#define __MLN_DEFINE
#include "mln_global.h"

void mln_global_init(void)
{
    cr0 = cr1 = cr2 = cr3 = 0;
}

