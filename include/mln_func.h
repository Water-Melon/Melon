
/*
 * Copyright (C) Niklaus F.Schen.
 */
#ifndef __MLN_FUNC_H
#define __MLN_FUNC_H

#include <stdio.h>

typedef void (*mln_func_cb_t)(const char *file, const char *func, int line);

#if defined(MLN_FUNC_FLAG)
#define MLN_FUNC(ret_type, name, params, args, ...) \
    ret_type __##name params __VA_ARGS__\
    ret_type name params {\
        if (mln_func_entry != NULL) mln_func_entry(__FILE__, __FUNCTION__, __LINE__);\
        ret_type _r;\
        _r = __##name args;\
        if (mln_func_exit != NULL) mln_func_exit(__FILE__, __FUNCTION__, __LINE__);\
        return _r;\
    }
#define MLN_FUNC_VOID(ret_type, name, params, args, ...) \
    ret_type __##name params __VA_ARGS__\
    ret_type name params {\
        if (mln_func_entry != NULL) mln_func_entry(__FILE__, __FUNCTION__, __LINE__);\
        __##name args;\
        if (mln_func_exit != NULL) mln_func_exit(__FILE__, __FUNCTION__, __LINE__);\
    }
#else
#define MLN_FUNC(ret_type, name, params, args, ...) \
    ret_type name params __VA_ARGS__ 
#define MLN_FUNC_VOID(ret_type, name, params, args, ...) \
    ret_type name params __VA_ARGS__ 
#endif

extern mln_func_cb_t mln_func_entry;
extern mln_func_cb_t mln_func_exit;

extern void mln_func_entry_callback_set(mln_func_cb_t cb);
extern mln_func_cb_t mln_func_entry_callback_get(void);
extern void mln_func_exit_callback_set(mln_func_cb_t cb);
extern mln_func_cb_t mln_func_exit_callback_get(void);

#endif

