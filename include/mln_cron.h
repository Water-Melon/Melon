
/*
 * Copyright (C) Niklaus F.Schen.
 */
#ifndef __MLN_CRON_H
#define __MLN_CRON_H

#include <time.h>
#include "mln_string.h"

extern time_t mln_cron_parse(mln_string_t *exp, time_t base) __NONNULL1(1);

#endif
