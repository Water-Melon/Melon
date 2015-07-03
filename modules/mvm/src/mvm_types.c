
/*
 * Copyright (C) Niklaus F.Schen.
 */
#include "mvm_types.h"

char *mvm_errors[] = {
    "Succeed.", 
    "System error."
};

char *mvm_strerror(mvm_stat_t stat)
{
    if (stat == MVM_OK)
        return mvm_errors[0];
    return mvm_errors[-stat];
}

