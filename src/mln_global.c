
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
    mln_set_sys_log_fd(STDERR_FILENO);
}

