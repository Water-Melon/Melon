
/*
 * Copyright (C) Niklaus F.Schen.
 */

#ifndef __MLN_FILE_H
#define __MLN_FILE_H

#include "mln_types.h"
#include "mln_string.h"
#include "mln_hash.h"
#include "mln_alloc.h"

typedef struct mln_file_set_s mln_file_set_t;

typedef struct mln_file_s {
    mln_string_t      *file_path;
    int                fd;
    mln_u32_t          is_tmp:1;
    time_t             mtime;
    time_t             ctime;
    time_t             atime;
    size_t             size;
    size_t             refer_cnt;
    struct mln_file_s *prev;
    struct mln_file_s *next;
    mln_file_set_t    *fset;
} mln_file_t;

struct mln_file_set_s {
    mln_alloc_t       *pool;
    mln_hash_t        *reg_file_hash;
    mln_file_t        *reg_free_head;
    mln_file_t        *reg_free_tail;
    mln_size_t         max_file;
};

#define mln_file_fd(pfile) ((pfile)->fd)
extern mln_file_set_t *mln_file_set_init(mln_size_t max_file);
extern void mln_file_set_destroy(mln_file_set_t *fs);
extern mln_file_t *mln_file_open(mln_file_set_t *fs, const char *filepath) __NONNULL2(1,2);
extern void mln_file_close(void *pfile);
extern mln_file_t *mln_file_tmp_open(mln_alloc_t *pool) __NONNULL1(1);

#endif

