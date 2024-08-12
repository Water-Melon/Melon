
/*
 * Copyright (C) Niklaus F.Schen.
 */
#ifndef __MLN_DEFS_H
#define __MLN_DEFS_H

#if !defined(MSVC)
#include <pthread.h>
#else
#include "mln_types.h"
#endif
#include "mln_func.h"

#ifdef __DEBUG__
#include <assert.h>
#define ASSERT(x) assert(x)
#else
#define ASSERT(x);
#endif

/*
 * container_of and offsetof
 */
#define mln_offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
#if defined(MSVC)
#define mln_container_of(ptr, type, member, ret) do {\
    type *__rptr = NULL;\
    if ((ptr) != NULL) {\
        __rptr = (type *)((char *)((const typeof(((type *)0)->member) *)(ptr)) - mln_offsetof(type, member));\
    }\
    (ret) = __rptr;\
} while (0)
#else
#define mln_container_of(ptr, type, member) ({\
    type *__rptr = NULL;\
    if ((ptr) != NULL) {\
        __rptr = (type *)((char *)((const typeof(((type *)0)->member) *)(ptr)) - mln_offsetof(type, member));\
    }\
    __rptr;\
})
#endif

/*
 * nonnull attribute
 */
#if defined(__APPLE__) || defined(MSVC) || defined(__wasm__) || defined(__FreeBSD__)
#define __NONNULL1(x)
#define __NONNULL2(x,y)
#define __NONNULL3(x,y,z)
#define __NONNULL4(w,x,y,z)
#define __NONNULL5(v,w,x,y,z)
#else
#define __NONNULL1(x)     __nonnull((x))
#define __NONNULL2(x,y)   __nonnull((x)) __nonnull((y))
#define __NONNULL3(x,y,z) __nonnull((x)) __nonnull((y)) __nonnull((z))
#define __NONNULL4(w,x,y,z) __nonnull((w)) __nonnull((x)) __nonnull((y)) __nonnull((z))
#define __NONNULL5(v,w,x,y,z) __nonnull((v)) __nonnull((w)) __nonnull((x)) __nonnull((y)) __nonnull((z))
#endif

/*
 * lock
 */
#if defined(__GNUC__) && (__GNUC__ >= 4 && __GNUC_MINOR__ > 1)

extern void spin_lock(void *lock);
extern void spin_unlock(void *lock);
extern int spin_trylock(void *lock);
#define mln_spin_lock              spin_lock
#define mln_spin_unlock            spin_unlock
#define mln_spin_trylock           spin_trylock
#define mln_spin_init(lock_ptr) (*(lock_ptr) = 0)
#define mln_spin_destroy(lock_ptr) (*(lock_ptr) = 0)

#elif defined(i386) || defined(__x86_64)

extern void spin_lock(void *lock);
extern void spin_unlock(void *lock);
extern int spin_trylock(void *lock);
#define mln_spin_lock              spin_lock
#define mln_spin_unlock            spin_unlock
#define mln_spin_trylock           spin_trylock
#define mln_spin_init(lock_ptr)    (*(lock_ptr) = 0)
#define mln_spin_destroy(lock_ptr) (*(lock_ptr) = 0)

#else

#if !defined(MSVC)
/*
 * In FreeBSD, pthread_spin_init() will call calloc(),
 * so we cannot use these interfaces in FreeBSD to
 * implement malloc().
 */
#define mln_spin_trylock pthread_spin_trylock
#define mln_spin_lock    pthread_spin_lock
#define mln_spin_unlock  pthread_spin_unlock
#define mln_spin_init(lock_ptr) \
    pthread_spin_init((lock_ptr), PTHREAD_PROCESS_PRIVATE)
#define mln_spin_destroy(lock_ptr) \
    pthread_spin_destroy((lock_ptr))

#endif
#endif

/*
 * Chain
 */
#define MLN_CHAIN_FUNC_DECLARE(scope,prefix,type,func_attr); \
    scope void prefix##_chain_add(type **head, type **tail, type *node) func_attr;\
    scope void prefix##_chain_del(type **head, type **tail, type *node) func_attr;

#define MLN_CHAIN_FUNC_DEFINE(scope,prefix,type,prev_ptr,next_ptr); \
    MLN_FUNC_VOID(scope, void, prefix##_chain_add, (type **head, type **tail, type *node), (head, tail, node), { \
        node->next_ptr = NULL;\
        if (*head == NULL) {\
            *head = node;\
        } else {\
            (*tail)->next_ptr = node;\
        }\
        node->prev_ptr = (*tail);\
        *tail = node;\
    })\
    \
    MLN_FUNC_VOID(scope, void, prefix##_chain_del, (type **head, type **tail, type *node), (head, tail, node), { \
        if (*head == node) {\
            if (*tail == node) {\
                *head = *tail = NULL;\
            } else {\
                *head = node->next_ptr;\
                (*head)->prev_ptr = NULL;\
            }\
        } else {\
            if (*tail == node) {\
                *tail = node->prev_ptr;\
                (*tail)->next_ptr = NULL;\
            } else {\
                node->prev_ptr->next_ptr = node->next_ptr;\
                node->next_ptr->prev_ptr = node->prev_ptr;\
            }\
        }\
        node->prev_ptr = node->next_ptr = NULL;\
    })

#define mln_bigendian_encode(num,buf,Blen); \
{\
    size_t i;\
    for (i = 0; i < Blen; ++i) {\
        *(buf)++ = ((mln_u64_t)(num) >> ((Blen-i-1) << 3)) & 0xff;\
    }\
}

#define mln_bigendian_decode(num,buf,Blen); \
{\
    size_t i;\
    num = 0;\
    for (i = 0; i < Blen; ++i, ++buf) {\
        num |= ((((mln_u64_t)(*(buf))) & 0xff) << (((Blen)-i-1) << 3));\
    }\
}

#if defined(MSVC)
extern int pipe(int fds[2]);
extern int socketpair(int domain, int type, int protocol, int sv[2]);
#define mln_socket_close closesocket
#else
#define mln_socket_close close
#endif

#define mln_isalpha(x)      (((x) >= 'a' && (x) <= 'z') || ((x) >= 'A' && (x) <= 'Z'))
#define mln_isdigit(x)      ((x) >= '0' && (x) <= '9')
#define mln_isascii(x)      ((x) >= 0 && (x) <= 127)
#define mln_isprint(x)      ((x) >= 32 && (x) <= 126)
#define mln_iswhitespace(x) ((x) == ' ' || (x) == '\t' || (x) == '\n' || (x) == '\f' || (x) == '\r' || (x) == '\v')

#if defined(MLN_C99) && defined(__linux__)
extern void usleep(unsigned long usec);
#endif

#if defined(MSVC)
#include <windows.h>
#include <string.h>
#define MAX_PATH_LEN 260

struct dirent {
    char d_name[MAX_PATH_LEN];
};

typedef struct {
    HANDLE hFind;
    WIN32_FIND_DATAA findFileData;
    char* path;
    CRITICAL_SECTION lock;
} DIR;

extern DIR* opendir(const char* path);
extern struct dirent* readdir(DIR* dirp);
extern int closedir(DIR* dirp);
extern int gettimeofday(struct timeval *tv, void *tz);
#endif

#define MLN_AUTHOR "Niklaus F.Schen"

#endif

