
/*
 * Copyright (C) Niklaus F.Schen.
 */
#ifndef __MVM_TYPES_H
#define __MVM_TYPES_H

#include "mln_types.h"

#define MVM_OK 0
#define MVM_ERROR -1

typedef int mvm_stat_t;


char *mvm_strerror(mvm_stat_t stat);

#endif

