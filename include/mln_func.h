
/*
 * Copyright (C) Niklaus F.Schen.
 */
#ifndef __MLN_FUNC_H
#define __MLN_FUNC_H

#include <stdio.h>

typedef int (*mln_func_entry_cb_t)(void *fptr, const char *file, const char *func, int line, ...);
typedef void (*mln_func_exit_cb_t)(void *fptr, const char *file, const char *func, int line, void *ret, ...);

#define MLN_FUNC_ERROR (~((long)0))

#if defined(MLN_FUNC_FLAG)
#if defined(MLN_C99)
#define MLN_FUNC_STRIP(...) __VA_OPT__(,) __VA_ARGS__
#define MLN_FUNC(scope, ret_type, name, params, args, ...) \
    scope ret_type name params;\
    scope ret_type __mln_func_##name params __VA_ARGS__\
    scope ret_type name params {\
        ret_type _r;\
        if (mln_func_entry != NULL) {\
            if (mln_func_entry(name, __FILE__, __FUNCTION__, __LINE__ MLN_FUNC_STRIP args) < 0) {\
                _r = (ret_type)MLN_FUNC_ERROR;\
                goto out;\
            }\
        }\
        _r = __mln_func_##name args;\
out:\
        if (mln_func_exit != NULL) mln_func_exit(name, __FILE__, __FUNCTION__, __LINE__, &_r MLN_FUNC_STRIP args);\
        return _r;\
    }
#define MLN_FUNC_VOID(scope, ret_type, name, params, args, ...) \
    scope ret_type name params;\
    scope ret_type __mln_func_##name params __VA_ARGS__\
    scope ret_type name params {\
        int _r = 0;\
        if (mln_func_entry != NULL) {\
            if (mln_func_entry(name, __FILE__, __FUNCTION__, __LINE__ MLN_FUNC_STRIP args) < 0) {\
                _r = (int)MLN_FUNC_ERROR;\
                goto out;\
            }\
        }\
        __mln_func_##name args;\
out:\
        if (mln_func_exit != NULL) mln_func_exit(name, __FILE__, __FUNCTION__, __LINE__, _r? &_r: NULL MLN_FUNC_STRIP args);\
    }
#define MLN_FUNC_CUSTOM(entry, exit, scope, ret_type, name, params, args, ...) \
    scope ret_type name params;\
    scope ret_type __mln_func_##name params __VA_ARGS__\
    scope ret_type name params {\
        ret_type _r;\
        if (entry(name, __FILE__, __FUNCTION__, __LINE__ MLN_FUNC_STRIP args) < 0) {\
            _r = (ret_type)MLN_FUNC_ERROR;\
            goto out;\
        }\
        _r = __mln_func_##name args;\
out:\
        exit(name, __FILE__, __FUNCTION__, __LINE__, &_r MLN_FUNC_STRIP args);\
        return _r;\
    }
#define MLN_FUNC_VOID_CUSTOM(entry, exit, scope, ret_type, name, params, args, ...) \
    scope ret_type name params;\
    scope ret_type __mln_func_##name params __VA_ARGS__\
    scope ret_type name params {\
        int _r = 0;\
        if (entry(name, __FILE__, __FUNCTION__, __LINE__ MLN_FUNC_STRIP args) < 0) {\
            _r = (int)MLN_FUNC_ERROR;\
            goto out;\
        }\
        __mln_func_##name args;\
out:\
        exit(name, __FILE__, __FUNCTION__, __LINE__, _r? &_r: NULL MLN_FUNC_STRIP args);\
    }

#else
#define MLN_FUNC(scope, ret_type, name, params, args, ...) \
    scope ret_type name params;\
    scope ret_type __mln_func_##name params __VA_ARGS__\
    scope ret_type name params {\
        ret_type _r;\
        if (mln_func_entry != NULL) {\
            if (mln_func_entry(name, __FILE__, __FUNCTION__, __LINE__) < 0) {\
                _r = (ret_type)MLN_FUNC_ERROR;\
                goto out;\
            }\
        }\
        _r = __mln_func_##name args;\
out:\
        if (mln_func_exit != NULL) mln_func_exit(name, __FILE__, __FUNCTION__, __LINE__, &_r);\
        return _r;\
    }
#define MLN_FUNC_VOID(scope, ret_type, name, params, args, ...) \
    scope ret_type name params;\
    scope ret_type __mln_func_##name params __VA_ARGS__\
    scope ret_type name params {\
        int _r = 0;\
        if (mln_func_entry != NULL) {\
            if (mln_func_entry(name, __FILE__, __FUNCTION__, __LINE__) < 0) {\
                _r = (int)MLN_FUNC_ERROR;\
                goto out;\
            }\
        }\
        __mln_func_##name args;\
out:\
        if (mln_func_exit != NULL) mln_func_exit(name, __FILE__, __FUNCTION__, __LINE__, _r? &_r: NULL);\
    }
#define MLN_FUNC_CUSTOM(entry, exit, scope, ret_type, name, params, args, ...) \
    scope ret_type name params;\
    scope ret_type __mln_func_##name params __VA_ARGS__\
    scope ret_type name params {\
        ret_type _r;\
        if (entry(name, __FILE__, __FUNCTION__, __LINE__) < 0) {\
            _r = (ret_type)MLN_FUNC_ERROR;\
            goto out;\
        }\
        _r = __mln_func_##name args;\
out:\
        exit(name, __FILE__, __FUNCTION__, __LINE__, &_r);\
        return _r;\
    }
#define MLN_FUNC_VOID_CUSTOM(entry, exit, scope, ret_type, name, params, args, ...) \
    scope ret_type name params;\
    scope ret_type __mln_func_##name params __VA_ARGS__\
    scope ret_type name params {\
        int _r = 0;\
        if (entry(name, __FILE__, __FUNCTION__, __LINE__) < 0) {\
            _r = (int)MLN_FUNC_ERROR;\
            goto out;\
        }\
        __mln_func_##name args;\
out:\
        exit(name, __FILE__, __FUNCTION__, __LINE__, _r? &_r: NULL);\
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

extern mln_func_entry_cb_t mln_func_entry;
extern mln_func_exit_cb_t  mln_func_exit;

extern void mln_func_entry_callback_set(mln_func_entry_cb_t cb);
extern mln_func_entry_cb_t mln_func_entry_callback_get(void);
extern void mln_func_exit_callback_set(mln_func_exit_cb_t cb);
extern mln_func_exit_cb_t mln_func_exit_callback_get(void);

#endif

