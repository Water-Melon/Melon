
/*
 * Copyright (C) Niklaus F.Schen.
 */

#ifndef __MLN_FILE_H
#define __MLN_FILE_H

#include "mln_types.h"
#include "mln_string.h"
#include "mln_rbtree.h"
#include "mln_alloc.h"

typedef struct mln_fileset_s mln_fileset_t;

typedef struct mln_file_s {
    struct mln_file_s *prev;
    struct mln_file_s *next;
    int                fd;
    mln_u32_t          is_tmp:1;
    mln_string_t      *file_path;
    mln_fileset_t     *fset;
    mln_rbtree_node_t *node;
    size_t             refer_cnt;
    size_t             size;
    time_t             mtime;
    time_t             ctime;
    time_t             atime;
} mln_file_t;

struct mln_fileset_s {
    mln_alloc_t       *pool;
    mln_rbtree_t      *reg_file_tree;
    mln_file_t        *reg_free_head;
    mln_file_t        *reg_free_tail;
    mln_size_t         max_file;
};

#define mln_file_fd(pfile) ((pfile)->fd)
extern mln_fileset_t *mln_fileset_init(mln_size_t max_file);
extern void mln_fileset_destroy(mln_fileset_t *fs);
extern mln_file_t *mln_file_open(mln_fileset_t *fs, char *filepath) __NONNULL2(1,2);
extern void mln_file_close(mln_file_t *pfile);
extern mln_file_t *mln_file_tmp_open(mln_alloc_t *pool) __NONNULL1(1);

#endif

