
/*
 * Copyright (C) Niklaus F.Schen.
 */
#ifndef __MLN_FUNC_H
#define __MLN_FUNC_H

#include <stdio.h>

typedef void (*mln_func_cb_t)(const char *file, const char *func, int line, ...);

#if defined(MLN_FUNC_FLAG)
#if defined(MLN_C99)
#define MLN_FUNC_STRIP(...) __VA_OPT__(,) __VA_ARGS__
#define MLN_FUNC(scope, ret_type, name, params, args, ...) \
    scope ret_type name params;\
    scope ret_type __mln_func_##name params __VA_ARGS__\
    scope ret_type name params {\
        if (mln_func_entry != NULL) mln_func_entry(__FILE__, __FUNCTION__, __LINE__ MLN_FUNC_STRIP args);\
        ret_type _r;\
        _r = __mln_func_##name args;\
        if (mln_func_exit != NULL) mln_func_exit(__FILE__, __FUNCTION__, __LINE__ MLN_FUNC_STRIP args);\
        return _r;\
    }
#define MLN_FUNC_VOID(scope, ret_type, name, params, args, ...) \
    scope ret_type name params;\
    scope ret_type __mln_func_##name params __VA_ARGS__\
    scope ret_type name params {\
        if (mln_func_entry != NULL) mln_func_entry(__FILE__, __FUNCTION__, __LINE__ MLN_FUNC_STRIP args);\
        __mln_func_##name args;\
        if (mln_func_exit != NULL) mln_func_exit(__FILE__, __FUNCTION__, __LINE__ MLN_FUNC_STRIP args);\
    }
#define MLN_FUNC_CUSTOM(entry, exit, scope, ret_type, name, params, args, ...) \
    scope ret_type name params;\
    scope ret_type __mln_func_##name params __VA_ARGS__\
    scope ret_type name params {\
        entry(__FILE__, __FUNCTION__, __LINE__ MLN_FUNC_STRIP args);\
        ret_type _r;\
        _r = __mln_func_##name args;\
        exit(__FILE__, __FUNCTION__, __LINE__ MLN_FUNC_STRIP args);\
        return _r;\
    }
#define MLN_FUNC_VOID_CUSTOM(entry, exit, scope, ret_type, name, params, args, ...) \
    scope ret_type name params;\
    scope ret_type __mln_func_##name params __VA_ARGS__\
    scope ret_type name params {\
        entry(__FILE__, __FUNCTION__, __LINE__ MLN_FUNC_STRIP args);\
        __mln_func_##name args;\
        exit(__FILE__, __FUNCTION__, __LINE__ MLN_FUNC_STRIP args);\
    }

#else
#define MLN_FUNC(scope, ret_type, name, params, args, ...) \
    scope ret_type name params;\
    scope ret_type __mln_func_##name params __VA_ARGS__\
    scope ret_type name params {\
        if (mln_func_entry != NULL) mln_func_entry(__FILE__, __FUNCTION__, __LINE__);\
        ret_type _r;\
        _r = __mln_func_##name args;\
        if (mln_func_exit != NULL) mln_func_exit(__FILE__, __FUNCTION__, __LINE__);\
        return _r;\
    }
#define MLN_FUNC_VOID(scope, ret_type, name, params, args, ...) \
    scope ret_type name params;\
    scope ret_type __mln_func_##name params __VA_ARGS__\
    scope ret_type name params {\
        if (mln_func_entry != NULL) mln_func_entry(__FILE__, __FUNCTION__, __LINE__);\
        __mln_func_##name args;\
        if (mln_func_exit != NULL) mln_func_exit(__FILE__, __FUNCTION__, __LINE__);\
    }
#define MLN_FUNC_CUSTOM(entry, exit, scope, ret_type, name, params, args, ...) \
    scope ret_type name params;\
    scope ret_type __mln_func_##name params __VA_ARGS__\
    scope ret_type name params {\
        entry(__FILE__, __FUNCTION__, __LINE__);\
        ret_type _r;\
        _r = __mln_func_##name args;\
        exit(__FILE__, __FUNCTION__, __LINE__);\
        return _r;\
    }
#define MLN_FUNC_VOID_CUSTOM(entry, exit, scope, ret_type, name, params, args, ...) \
    scope ret_type name params;\
    scope ret_type __mln_func_##name params __VA_ARGS__\
    scope ret_type name params {\
        entry(__FILE__, __FUNCTION__, __LINE__);\
        __mln_func_##name args;\
        exit(__FILE__, __FUNCTION__, __LINE__);\
    }
#endif
#else
#define MLN_FUNC(scope, ret_type, name, params, args, ...) \
    scope ret_type name params __VA_ARGS__
#define MLN_FUNC_VOID(scope, ret_type, name, params, args, ...) \
    scope ret_type name params __VA_ARGS__
#define MLN_FUNC_CUSTOM(entry, exit, scope, ret_type, name, params, args, ...) \
    scope ret_type name params __VA_ARGS__
#define MLN_FUNC_VOID_CUSTOM(entry, exit, scope, ret_type, name, params, args, ...) \
    scope ret_type name params __VA_ARGS__
#endif

extern mln_func_cb_t mln_func_entry;
extern mln_func_cb_t mln_func_exit;

extern void mln_func_entry_callback_set(mln_func_cb_t cb);
extern mln_func_cb_t mln_func_entry_callback_get(void);
extern void mln_func_exit_callback_set(mln_func_cb_t cb);
extern mln_func_cb_t mln_func_exit_callback_get(void);

#endif

