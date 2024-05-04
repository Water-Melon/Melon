
/*
 * Copyright (C) Niklaus F.Schen.
 */
#ifndef __MLN_THREAD_POOL_H
#define __MLN_THREAD_POOL_H

#if !defined(MSVC)

#ifdef MLN_USE_UNIX98
  #ifndef __USE_UNIX98
  #define __USE_UNIX98
  #endif
#endif
#include <pthread.h>
#include "mln_types.h"
#include "mln_string.h"

typedef struct mln_thread_pool_s mln_thread_pool_t;

typedef int  (*mln_thread_process)(void *);
typedef void (*mln_thread_data_free)(void *);

typedef struct mln_thread_pool_resource_s {
    void                              *data;
    struct mln_thread_pool_resource_s *next;
} mln_thread_pool_resource_t;

typedef struct mln_thread_pool_member_s {
    void                              *data;
    mln_thread_pool_t                 *pool;
    mln_u32_t                          idle:1;
    mln_u32_t                          locked:1;
    mln_u32_t                          forked:1;
    mln_u32_t                          child:1;
    struct mln_thread_pool_member_s   *prev;
    struct mln_thread_pool_member_s   *next;
} mln_thread_pool_member_t;

struct mln_thread_pool_s {
    pthread_mutex_t                    mutex;
    pthread_cond_t                     cond;
    pthread_attr_t                     attr;
    mln_thread_pool_resource_t        *res_chain_head;
    mln_thread_pool_resource_t        *res_chain_tail;
    mln_thread_pool_member_t          *child_head;
    mln_thread_pool_member_t          *child_tail;
    mln_u32_t                          max;
    mln_u32_t                          idle;
    mln_u32_t                          counter;
    mln_u32_t                          quit:1;
    mln_u32_t                          padding:31;
    mln_u64_t                          cond_timeout;/*ms*/
    mln_size_t                         n_res;
    mln_thread_process                 process_handler;
    mln_thread_data_free               free_handler;
};

struct mln_thread_pool_attr {
    void                              *main_data;
    mln_thread_process                 child_process_handler;
    mln_thread_process                 main_process_handler;
    mln_thread_data_free               free_handler;
    mln_u64_t                          cond_timeout; /*ms*/
    mln_u32_t                          max;
    mln_u32_t                          concurrency;
};

struct mln_thread_pool_info {
    mln_u32_t                          max_num;
    mln_u32_t                          idle_num;
    mln_u32_t                          cur_num;
    mln_size_t                         res_num;
};

extern int mln_thread_pool_run(struct mln_thread_pool_attr *tpattr) __NONNULL1(1);
extern int mln_thread_pool_resource_add(void *data) __NONNULL1(1);
extern void mln_thread_quit(void);
extern void mln_thread_resource_info(struct mln_thread_pool_info *info);
#endif

#endif
