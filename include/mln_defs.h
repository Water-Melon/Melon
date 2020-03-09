
/*
 * Copyright (C) Niklaus F.Schen.
 */
#ifndef __MLN_DEFS_H
#define __MLN_DEFS_H

#include <pthread.h>

#if defined(__APPLE__)
#define __NONNULL1(x)
#define __NONNULL2(x,y)
#define __NONNULL3(x,y,z)
#define __NONNULL4(w,x,y,z)
#else
#define __NONNULL1(x)     __nonnull((x))
#define __NONNULL2(x,y)   __nonnull((x)) __nonnull((y))
#define __NONNULL3(x,y,z) __nonnull((x)) __nonnull((y)) __nonnull((z))
#define __NONNULL4(w,x,y,z) __nonnull((w)) __nonnull((x)) __nonnull((y)) __nonnull((z))
#endif

#ifdef __GNUC__
#define __cacheline_aligned __attribute__((__aligned__(16)))
#else
#define __cacheline_aligned
#endif

/*
 * lock
 */
#if defined(__GNUC__) && (__GNUC__ >= 4 && __GNUC_MINOR__ > 1)

#define MLN_TRYLOCK(lock_ptr) \
    (!__sync_bool_compare_and_swap((lock_ptr), 0, 1))
#define MLN_LOCK(lock_ptr) \
    while (!(__sync_bool_compare_and_swap((lock_ptr), 0, 1)))
#define MLN_UNLOCK(lock_ptr) \
    __sync_bool_compare_and_swap((lock_ptr), 1, 0)
#define MLN_LOCK_INIT(lock_ptr) *(lock_ptr) = 0
#define MLN_LOCK_DESTROY(lock_ptr) *(lock_ptr) = 0

#elif defined(i386) || defined(__x86_64)

extern void spin_lock(void *lock);
extern void spin_unlock(void *lock);
extern int spin_trylock(void *lock);
#define MLN_LOCK(lock_ptr) spin_lock((lock_ptr))
#define MLN_UNLOCK(lock_ptr) spin_unlock((lock_ptr))
#define MLN_TRYLOCK(lock_ptr) spin_trylock((lock_ptr))
#define MLN_LOCK_INIT(lock_ptr) *(lock_ptr) = 0
#define MLN_LOCK_DESTROY(lock_ptr) *(lock_ptr) = 0

#else

/*
 * In FreeBSD, pthread_spin_init() will call calloc(),
 * so we cannot use these interfaces in FreeBSD to
 * implement malloc().
 */
#define MLN_TRYLOCK(lock_ptr) \
    pthread_spin_trylock((lock_ptr))
#define MLN_LOCK(lock_ptr) \
    pthread_spin_lock((lock_ptr))
#define MLN_UNLOCK(lock_ptr) \
    pthread_spin_unlock((lock_ptr))
#define MLN_LOCK_INIT(lock_ptr) \
    pthread_spin_init((lock_ptr), PTHREAD_PROCESS_PRIVATE)
#define MLN_LOCK_DESTROY(lock_ptr) \
    pthread_spin_destroy((lock_ptr))

#endif

/*
 * Chain
 */
#define MLN_CHAIN_FUNC_DECLARE(prefix,type,ret_attr,func_attr); \
    ret_attr prefix##_chain_add(type **head, type **tail, type *node) func_attr;\
    ret_attr prefix##_chain_del(type **head, type **tail, type *node) func_attr;

#define MLN_CHAIN_FUNC_DEFINE(prefix,type,ret_attr,prev_ptr,next_ptr); \
    ret_attr prefix##_chain_add(type **head, type **tail, type *node) \
    {\
        node->prev_ptr = node->next_ptr = NULL;\
        if (*head == NULL) {\
            *head = *tail = node;\
            return ;\
        }\
        (*tail)->next_ptr = node;\
        node->prev_ptr = (*tail);\
        *tail = node;\
    }\
    \
    ret_attr prefix##_chain_del(type **head, type **tail, type *node) \
    {\
        if (head == NULL && tail == NULL && node == NULL) return;\
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
    }

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

#endif

