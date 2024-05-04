
/*
 * Copyright (C) Niklaus F.Schen.
 */

#ifndef __MLN_CHAIN_H
#define __MLN_CHAIN_H

#include "mln_types.h"
#include "mln_string.h"
#include "mln_alloc.h"
#include "mln_file.h"

typedef struct mln_buf_s {
    mln_u8ptr_t         left_pos;
    mln_u8ptr_t         pos;
    mln_u8ptr_t         last;
    mln_u8ptr_t         start;
    mln_u8ptr_t         end;
    struct mln_buf_s   *shadow;
    mln_off_t           file_left_pos;
    mln_off_t           file_pos;
    mln_off_t           file_last;
    mln_file_t         *file;
    mln_u32_t           temporary:1;
#if !defined(MSVC)
    mln_u32_t           mmap:1;
#endif
    mln_u32_t           in_memory:1;
    mln_u32_t           in_file:1;
    mln_u32_t           flush:1;
    mln_u32_t           sync:1;
    mln_u32_t           last_buf:1;
    mln_u32_t           last_in_chain:1;
} mln_buf_t;

typedef struct mln_chain_s {
    mln_buf_t          *buf;
    struct mln_chain_s *next;
} mln_chain_t;

#define mln_buf_size(pbuf) \
    ((pbuf) == NULL? 0: \
        ((pbuf)->in_file? (pbuf)->file_last - (pbuf)->file_pos: (pbuf)->last - (pbuf)->pos))


#define mln_buf_left_size(pbuf) \
    ((pbuf) == NULL? 0: \
        ((pbuf)->in_file? (pbuf)->file_last - (pbuf)->file_left_pos: (pbuf)->last - (pbuf)->left_pos))

#define mln_chain_add(pphead,pptail,c) \
{\
    if (*(pphead) == NULL) {\
        *(pphead) = *(pptail) = (c);\
    } else {\
        (*(pptail))->next = (c);\
        *(pptail) = (c);\
    }\
}

extern mln_buf_t *mln_buf_new(mln_alloc_t *pool);
extern mln_chain_t *mln_chain_new(mln_alloc_t *pool);
extern void mln_buf_pool_release(mln_buf_t *b);
extern void mln_chain_pool_release(mln_chain_t *c);
extern void mln_chain_pool_release_all(mln_chain_t *c);


#endif
