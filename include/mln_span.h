
/*
 * Copyright (C) Niklaus F.Schen.
 */
#ifndef __MLN_SPAN_H
#define __MLN_SPAN_H

#if defined(MSVC)
#include "mln_utils.h"
#include <windows.h>
#else
#include <sys/time.h>
#include <pthread.h>
#endif
#include "mln_func.h"

typedef struct mln_span_s {
    struct timeval                begin;
    struct timeval                end;
    const char                   *file;
    const char                   *func;
    int                           line;
    struct mln_span_s            *subspans_head;
    struct mln_span_s            *subspans_tail;
    struct mln_span_s            *parent;
    struct mln_span_s            *prev;
    struct mln_span_s            *next;
} mln_span_t;

typedef struct mln_span_stack_node_s {
    mln_span_t                   *span;
    struct mln_span_stack_node_s *next;
} mln_span_stack_node_t;

typedef void (*mln_span_dump_cb_t)(mln_span_t *s, int level, void *data);


extern mln_span_stack_node_t *__mln_span_stack_top;
extern mln_span_stack_node_t *__mln_span_stack_bottom;
extern mln_span_t *mln_span_root;
#if defined(MSVC)
extern DWORD mln_span_registered_thread;
#else
extern pthread_t mln_span_registered_thread;
#endif

extern void mln_span_stack_free(void);
extern mln_span_t *mln_span_new(mln_span_t *parent, const char *file, const char *func, int line);
extern void mln_span_free(mln_span_t *s);
extern int mln_span_entry(void *fptr, const char *file, const char *func, int line, ...);
extern void mln_span_exit(void *fptr, const char *file, const char *func, int line, void *ret, ...);

#if defined(MSVC)
#define mln_span_start() do {\
    mln_func_entry_callback_set(mln_span_entry);\
    mln_func_exit_callback_set(mln_span_exit);\
    mln_span_registered_thread = GetCurrentThreadId();\
    mln_span_root = NULL;\
    __mln_span_stack_top = __mln_span_stack_bottom = NULL;\
    mln_span_entry(__FILE__, __FUNCTION__, __LINE__);\
} while (0)
#else
#define mln_span_start() ({\
    mln_func_entry_callback_set(mln_span_entry);\
    mln_func_exit_callback_set(mln_span_exit);\
    mln_span_registered_thread = pthread_self();\
    __mln_span_stack_top = __mln_span_stack_bottom = NULL;\
    mln_span_root = NULL;\
    mln_span_entry(__FILE__, __FUNCTION__, __LINE__);\
})
#endif

#if !defined(MSVC)
#define mln_span_stop() ({\
    mln_span_exit(__FILE__, __FUNCTION__, __LINE__, NULL);\
    mln_func_entry_callback_set(NULL);\
    mln_func_exit_callback_set(NULL);\
    mln_span_stack_free();\
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

#define mln_span_time_cost(s) ({\
    mln_span_t *_s = (s);\
    mln_u64_t r = (_s->end.tv_sec * 1000000 + _s->end.tv_usec) - (_s->begin.tv_sec * 1000000 + _s->begin.tv_usec);\
    r;\
})
#else
#define mln_span_stop() do {\
    mln_span_exit(__FILE__, __FUNCTION__, __LINE__, NULL);\
    mln_func_entry_callback_set(NULL);\
    mln_func_exit_callback_set(NULL);\
    mln_span_stack_free();\
} while (0)

extern void mln_span_release(void);
extern mln_span_t *mln_span_move(void);
extern mln_u64_t mln_span_time_cost(mln_span_t *s);
#endif
#define mln_span_file(s)      ((s)->file)
#define mln_span_func(s)      ((s)->func)
#define mln_span_line(s)      ((s)->line)

extern void mln_span_dump(mln_span_dump_cb_t cb, void *data);
#endif

