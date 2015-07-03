
/*
 * Copyright (C) Niklaus F.Schen.
 */
#include <stdlib.h>
#include <ctype.h>
#include "mvm_mm.h"
#include "mln_conf.h"
#include "mln_log.h"

/*
 * global varaiables
 */
char mvm_domain[] = "mvm";
char mvm_memory_cmd[] = "memory_size";
mvm_tlb_t gTlb;
mln_lock_t gMMLock;
mln_u8ptr_t *gC2RMap = NULL; /*C emulate physical memory*/
mln_size_t gC2RMap_num = 0;
mln_size_t gMemSize = 0;
mln_conf_hook_t *gMemConfHook = NULL;


/*
 * static declarations
 */
static mvm_mm_reload_memory(void *data);
static mln_size_t
mvm_mm_string_to_size(mln_string_t *s);


/*
 * init global
 */
mvm_stat_t mvm_mm_init_memory(void)
{
    int err = 0;
    mln_conf_t *cf = mln_get_conf();
    if (cf == NULL) {
        mln_log(error, "Configuration crash.\n");
        abort();
    }
    mln_conf_domain_t *cd = cf->search(cf, mvm_domain);
    if (cd == NULL) {
        mln_log(error, "No such domain named '%s'.\n", mvm_domain);
        abort();
    }

    mln_conf_cmd_t *cc = cd->search(cd, mvm_memory_cmd);
    if (cc == NULL) {
        mln_log(error, "No such command named '%s' in domain '%s'.\n", \
                mvm_memory_cmd, mvm_domain);
        abort();
    }
    mln_conf_item_t *ci = cc->search(cc, 1);
    if (ci == NULL) {
        mln_log(error, "Lack of argument of command '%s' in domain '%s'.\n", \
                mvm_memory_cmd, mvm_domain);
        abort();
    }
    if (mln_get_cmd_args_num(cc) != 1) {
        mln_log(error, "Too many arguments of command '%s' in domain '%s'.\n", \
                mvm_memory_cmd, mvm_domain);
        abort();
    }
    if (ci->type != CONF_STR) {
        mln_log(error, "Invalid type of the argument of command '%s' in domain '%s'.\n", \
                mvm_memory_cmd, mvm_domain);
        abort();
    }
    gMemSize = mvm_mm_string_to_size(ci->val.s);
    if (!gMemSize) {
        mln_log(error, "Invalid memory size in domain '%s'.\n", mvm_domain);
        abort();
    }

    err = MLN_LOCK_INIT(&gMMLock);
    if (err != 0) {
        mln_log(error, "gMMLock init error. %s\n", strerror(err));
        return MVM_ERROR;
    }

    gC2RMap_num = (gMemSize+MVM_PAGE_SIZE-1)/MVM_PAGE_SIZE;
    gC2RMap = (mln_u8ptr_t *)calloc(gC2RMap_num, sizeof(mln_u8ptr_t));
    if (gC2RMap == NULL) {
        mln_log(error, "No memory.\n");
        mvm_mm_destroy_memory();
        return MVM_ERROR;
    }

    gMemConfHook = mln_conf_set_hook(mvm_mm_reload_memory, NULL);
    if (gMemConfHook == NULL) {
        mln_log(error, "No memory.\n");
        mvm_mm_destroy_memory();
        return MVM_ERROR;
    }
    return MVM_OK;
}

void mvm_mm_destroy_memory(void)
{
    MLN_LOCK_DESTROY(&gMMLock);
    gMemSize = 0;
    gC2RMap_num = 0;
    if (gC2RMap != NULL) {
        free(gC2RMap);
        gC2RMap = NULL;
    }
    if (gMemConfHook != NULL) {
        mln_conf_unset_hook(gMemConfHook);
        gMemConfHook = NULL;
    }
}

static int mvm_mm_reload_memory(void *data)
{
    MLN_LOCK(&gMMLock);
    
    mln_conf_t *cf = mln_get_conf();
    if (cf == NULL) {
        mln_log(error, "Configuration crash. Configuration not modified.\n");
        MLN_UNLOCK(&gMMLock);
        return 0;
    }
    mln_conf_domain_t *cd = cf->search(cf, mvm_domain);
    if (cd == NULL) {
        mln_log(error, \
                "No such domain named '%s'. Configuration not modified.\n", \
                mvm_domain);
        abort();
    }

    mln_conf_cmd_t *cc = cd->search(cd, mvm_memory_cmd);
    if (cc == NULL) {
        mln_log(error, \
                "No such command named '%s' in domain '%s'. Configuration not modified.\n", \
                mvm_memory_cmd, mvm_domain);
        abort();
    }
    mln_conf_item_t *ci = cc->search(cc, 1);
    if (ci == NULL) {
        mln_log(error, \
                "Lack of argument of command '%s' in domain '%s'. Configuration not modified.\n", \
                mvm_memory_cmd, mvm_domain);
        abort();
    }
    if (mln_get_cmd_args_num(cc) != 1) {
        mln_log(error, \
                "Too many arguments of command '%s' in domain '%s'. Configuration not modified.\n", \
                mvm_memory_cmd, mvm_domain);
        abort();
    }
    if (ci->type != CONF_STR) {
        mln_log(error, \
                "Invalid type of the argument of command '%s' in domain '%s'. Configuration not modified.\n", \
                mvm_memory_cmd, mvm_domain);
        abort();
    }
    mln_size_t reload_size = mvm_mm_string_to_size(ci->val.s);
    if (!reload_size) {
        mln_log(error, \
                "Invalid memory size in domain '%s'. Configuration not modified.\n", \
                mvm_domain);
        abort();
    }
    if (gMemSize == reload_size) {
        MLN_UNLOCK(&gMMLock);
        return 0;
    }
    if (gMemSize > reload_size) {
        mln_log(error, "'memory_size' can not be less than before. Configuration not modified.\n");
        MLN_UNLOCK(&gMMLock);
        return 0;
    }

    mln_size_t new_map_num = (reload_size+MVM_PAGE_SIZE-1)/MVM_PAGE_SIZE;
    if (new_map_num == gC2RMap_num) {
        gMemSize = reload_size;
        MLN_UNLOCK(&gMMLock);
        return 0;
    }
    mln_u8ptr_t *new_map = (mln_u8ptr_t *)realloc(gC2RMap, new_map_num*sizeof(mln_u8ptr_t));
    if (new_map == NULL) {
        mln_log(error, "No memory. Configuration not modified.\n");
        MLN_UNLOCK(&gMMLock);
        return 0;
    }

    gC2RMap = new_map;
    gMemSize = reload_size;
    gC2RMap_num = new_map_num;
    MLN_UNLOCK(&gMMLock);
    return 0;
}

/*
 * tools
 */
static mln_size_t
mvm_mm_string_to_size(mln_string_t *s)
{
    mln_s8ptr_t str = s->str;
    char unit = str[s->len - 1], c;
    mln_u32_t i, j = 1;
    if (isdigit(unit)) {
        i = s->len - 1;
    } else {
        i = s->len - 2;
        unit = 0;
    }
    mln_size_t size = 0;
    for (; i >= 0; i--, j*=10) {
        c = str[i];
        if (!isdigit(c)) {
            return (mln_size_t)0;
        }
        size += (((mln_size_t)(c - '0')) * j);
    }
    switch (unit) {
        case 'G':
        case 'g':
            size <<= 30;
        case 'M':
        case 'm':
            size <<= 20;
            break;
        case 'K':
        case 'k':
            size <<= 10;
            break;
        case 'B':
        case 'b':
        default:
            break;
    }
    return size;
}

