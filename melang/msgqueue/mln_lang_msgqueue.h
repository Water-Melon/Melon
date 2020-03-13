
/*
 * Copyright (C) Niklaus F.Schen.
 */
#ifndef __MLN_LANG_MSGQUEUE_H
#define __MLN_LANG_MSGQUEUE_H

#include "mln_fheap.h"
#include "mln_lang.h"

typedef struct mln_lang_mq_s mln_lang_mq_t;

typedef struct mln_lang_mq_msg_s {
    struct mln_lang_mq_msg_s      *prev;
    struct mln_lang_mq_msg_s      *next;
    mln_lang_t                    *lang;
    union {
        mln_s64_t      i;
        mln_u8_t       b;
        double         f;
        mln_string_t  *s;
    }                              data;
    int                            type;
} mln_lang_mq_msg_t;

typedef struct mln_lang_mq_wait_s {
    mln_lang_ctx_t                *ctx;
    mln_lang_mq_t                 *mq;
    mln_fheap_node_t              *fnode;
    mln_u64_t                      timestamp;
    mln_u64_t                      in_heap:1;
    mln_u64_t                      padding:63;
    struct mln_lang_mq_wait_s     *prev;
    struct mln_lang_mq_wait_s     *next;
} mln_lang_mq_wait_t;

struct mln_lang_mq_s {
    mln_string_t                  *name;
    mln_lang_mq_msg_t             *msg_head;
    mln_lang_mq_msg_t             *msg_tail;
    mln_lang_mq_wait_t            *wait_head;
    mln_lang_mq_wait_t            *wait_tail;
};

typedef struct mln_lang_ctx_mq_s {
    mln_lang_mq_wait_t            *mq_wait;
    mln_rbtree_t                  *topics;
} mln_lang_ctx_mq_t;

typedef struct mln_lang_ctx_mq_topic_s {
    mln_string_t                  *topic_name;
    mln_lang_ctx_t                *ctx;
    mln_lang_mq_msg_t             *msg_head;
    mln_lang_mq_msg_t             *msg_tail;
} mln_lang_ctx_mq_topic_t;

extern int mln_lang_msgqueue(mln_lang_ctx_t *ctx);

#endif
