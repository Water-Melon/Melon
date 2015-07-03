
/*
 * Copyright (C) Niklaus F.Schen.
 */

#ifndef __MVM_MM_H
#define __MVM_MM_H

#include "mvm_types.h"

#define MVM_TBL_LEN 256
#define MVM_PAGE_SIZE 4096

typedef struct {
    mln_size_t   last_atime;/*us*/
    mln_u32_t    atimes;
    mln_u32_t    pde_index;
    mln_u32_t   *pde;
    mln_u32_t   *pte;
    mln_u32_t    pte_index;
} mvm_tlb_t;

extern mvm_stat_t mvm_mm_init_memory(void);
extern void mvm_mm_destroy_memory(void);

#endif

