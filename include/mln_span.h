
/*
 * Copyright (C) Niklaus F.Schen.
 */
#ifndef __MLN_SPAN_H
#define __MLN_SPAN_H

#include <sys/time.h>
#include "mln_func.h"
#if defined(WIN32)
#include <windows.h>
#else
#include <pthread.h>
#endif

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
#if defined(WIN32)
extern DWORD mln_span_registered_thread;
#else
extern pthread_t mln_span_registered_thread;
#endif

extern void mln_span_stack_free(void);
extern mln_span_t *mln_span_new(mln_span_t *parent, const char *file, const char *func, int line);
extern void mln_span_free(mln_span_t *s);
extern void mln_span_entry(const char *file, const char *func, int line);
extern void mln_span_exit(const char *file, const char *func, int line);

#if defined(WIN32)
#define mln_span_start() ({\
    int r = 0;\
    mln_func_entry_callback_set(mln_span_entry);\
    mln_func_exit_callback_set(mln_span_exit);\
    mln_span_registered_thread = GetCurrentThreadId();\
    mln_span_root = NULL;\
    __mln_span_stack_top = __mln_span_stack_bottom = NULL;\
    mln_span_entry(__FILE__, __FUNCTION__, __LINE__);\
    r;\
})
#else
#define mln_span_start() ({\
    int r = 0;\
    mln_func_entry_callback_set(mln_span_entry);\
    mln_func_exit_callback_set(mln_span_exit);\
    mln_span_registered_thread = pthread_self();\
    __mln_span_stack_top = __mln_span_stack_bottom = NULL;\
    mln_span_root = NULL;\
    mln_span_entry(__FILE__, __FUNCTION__, __LINE__);\
    r;\
})
#endif

#define mln_span_stop() ({\
    mln_span_exit(__FILE__, __FUNCTION__, __LINE__);\
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

#define mln_span_file(s)      ((s)->file)
#define mln_span_func(s)      ((s)->func)
#define mln_span_line(s)      ((s)->line)
#define mln_span_time_cost(s) ({\
    mln_span_t *_s = (s);\
    mln_u64_t r = (_s->end.tv_sec * 1000000 + _s->end.tv_usec) - (_s->begin.tv_sec * 1000000 + _s->begin.tv_usec);\
    r;\
})

extern void mln_span_dump(mln_span_dump_cb_t cb, void *data);
#endif

