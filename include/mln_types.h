
/*
 * Copyright (C) Niklaus F.Schen.
 */

#ifndef __MLN_TYPES
#define __MLN_TYPES

#include <sys/types.h>
#include "mln_utils.h"
#if !defined(MSVC)
#include <unistd.h>
#if defined(__GNUC__) && (__GNUC__ >= 4 && __GNUC_MINOR__ > 1)
typedef long                  mln_spin_t;
#elif defined(i386) || defined(__x86_64)
typedef unsigned long         mln_spin_t;
#else
#include <pthread.h>
typedef pthread_spinlock_t    mln_spin_t;
#endif
#endif

typedef unsigned char         mln_u8_t;
typedef char                  mln_s8_t;
typedef unsigned short        mln_u16_t;
typedef short                 mln_s16_t;
typedef unsigned int          mln_u32_t;
typedef int                   mln_s32_t;
#if defined(i386) || defined(__arm__) || defined(MSVC) || defined(__wasm__)
typedef unsigned long long    mln_u64_t;
typedef long long             mln_s64_t;
#else
typedef unsigned long         mln_u64_t;
typedef long                  mln_s64_t;
#endif
typedef char *                mln_s8ptr_t;
typedef unsigned char *       mln_u8ptr_t;
typedef short *               mln_s16ptr_t;
typedef unsigned short *      mln_u16ptr_t;
typedef int *                 mln_s32ptr_t;
typedef unsigned int *        mln_u32ptr_t;
#if defined(i386) || defined(__arm__) || defined(MSVC) || defined(__wasm__)
typedef long long *           mln_s64ptr_t;
typedef unsigned long long *  mln_u64ptr_t;
#else
typedef long *                mln_s64ptr_t;
typedef unsigned long *       mln_u64ptr_t;
#endif
typedef size_t                mln_size_t;
typedef off_t                 mln_off_t;
typedef long                  mln_sptr_t;
typedef unsigned long         mln_uptr_t;
typedef long                  mln_sauto_t;
typedef unsigned long         mln_uauto_t;

#endif

