
/*
 * Copyright (C) Niklaus F.Schen.
 */
#ifndef __MLN_SPAN_H
#define __MLN_SPAN_H

#include <sys/time.h>
#include "mln_stack.h"
#include "mln_array.h"
#include "mln_func.h"
#include "mln_utils.h"
#if defined(WIN32)
#include <windows.h>
#else
#include <pthread.h>
#endif

typedef struct mln_span_s {
    struct timeval     begin;
    struct timeval     end;
    const char        *file;
    const char        *func;
    int                line;
    mln_array_t        subspans;
    struct mln_span_s *parent;
} mln_span_t;

extern mln_stack_t *mln_span_callstack;
extern mln_span_t *mln_span_root;
#if defined(WIN32)
extern DWORD mln_span_registered_thread;
#else
extern pthread_t mln_span_registered_thread;
#endif

extern mln_span_t *mln_span_new(mln_span_t *parent, const char *file, const char *func, int line);
extern void mln_span_free(mln_span_t *s);
extern void mln_span_entry(const char *file, const char *func, int line);
extern void mln_span_exit(const char *file, const char *func, int line);

#if defined(WIN32)
#define mln_span_start() ({\
    int r;\
    struct mln_stack_attr sattr;\
    mln_func_entry_callback_set(mln_span_entry);\
    mln_func_exit_callback_set(mln_span_exit);\
    sattr.free_handler = NULL;\
    sattr.copy_handler = NULL;\
    mln_span_registered_thread = GetCurrentThreadId();\
    if ((mln_span_callstack = mln_stack_init(&sattr)) == NULL) {\
        r = -1;\
    } else {\
        r = 0;\
        mln_span_entry(__FILE__, __FUNCTION__, __LINE__);\
    }\
    r;\
})
#else
#define mln_span_start() ({\
    int r;\
    struct mln_stack_attr sattr;\
    mln_func_entry_callback_set(mln_span_entry);\
    mln_func_exit_callback_set(mln_span_exit);\
    sattr.free_handler = NULL;\
    sattr.copy_handler = NULL;\
    mln_span_registered_thread = pthread_self();\
    if ((mln_span_callstack = mln_stack_init(&sattr)) == NULL) {\
        r = -1;\
    } else {\
        r = 0;\
        mln_span_entry(__FILE__, __FUNCTION__, __LINE__);\
    }\
    r;\
})
#endif

#define mln_span_stop() ({\
    mln_span_exit(__FILE__, __FUNCTION__, __LINE__);\
    mln_func_entry_callback_set(NULL);\
    mln_func_exit_callback_set(NULL);\
    mln_stack_destroy(mln_span_callstack);\
})

#define mln_span_release() ({\
    mln_span_free(mln_span_root);\
    mln_span_root = NULL;\
})

#define mln_span_move() ({\
    mln_span_t *span = mln_span_root;\
    mln_span_root = NULL;\
    span;\
})
extern void mln_span_dump(void);

#endif

