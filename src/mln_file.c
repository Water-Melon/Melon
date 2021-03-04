
/*
 * Copyright (C) Niklaus F.Schen.
 */

#include "mln_file.h"
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <sys/time.h>

char mln_file_tmp_dir[256] = "tmp";

MLN_CHAIN_FUNC_DECLARE(reg_file, \
                       mln_file_t, \
                       static inline void,);
static int mln_file_set_cmp(const void *data1, const void *data2);
static void mln_file_free(void *pfile);

mln_fileset_t *mln_fileset_init(mln_size_t max_file)
{
    struct mln_rbtree_attr attr;
    mln_fileset_t *fs;

    fs = (mln_fileset_t *)malloc(sizeof(mln_fileset_t));
    if (fs == NULL) {
        return NULL;
    }

    fs->pool = mln_alloc_init();
    if (fs->pool == NULL) {
        free(fs);
        return NULL;
    }

    attr.pool = NULL;
    attr.cmp = mln_file_set_cmp;
    attr.data_free = mln_file_free;
    attr.cache = 0;
    if ((fs->reg_file_tree = mln_rbtree_init(&attr)) == NULL) {
        mln_alloc_destroy(fs->pool);
        free(fs);
        return NULL;
    }

    fs->reg_free_head = fs->reg_free_tail = NULL;
    fs->max_file = max_file;

    return fs;
}

static int mln_file_set_cmp(const void *data1, const void *data2)
{
    mln_file_t *f1 = (mln_file_t *)data1;
    mln_file_t *f2 = (mln_file_t *)data2;
    return mln_string_strcmp(f1->file_path, f2->file_path);
}

void mln_fileset_destroy(mln_fileset_t *fs)
{
    if (fs == NULL) return;

    if (fs->reg_file_tree != NULL)
        mln_rbtree_destroy(fs->reg_file_tree);
    if (fs->pool != NULL)
        mln_alloc_destroy(fs->pool);
    free(fs);
}


mln_file_t *mln_file_open(mln_fileset_t *fs, const char *filepath)
{
    mln_rbtree_node_t *rn;
    mln_file_t *f, tmpf;
    mln_string_t path;

    mln_string_set(&path, filepath);
    tmpf.file_path = &path;
    rn = mln_rbtree_search(fs->reg_file_tree, fs->reg_file_tree->root, &tmpf);
    if (mln_rbtree_null(rn, fs->reg_file_tree)) {
        struct stat st;

        if ((f = (mln_file_t *)mln_alloc_m(fs->pool, sizeof(mln_file_t))) == NULL) {
            return NULL;
        }
        if ((f->file_path = mln_string_pool_new(fs->pool, filepath)) == NULL) {
            mln_alloc_free(f);
            return NULL;
        }
        if ((f->fd = open(filepath, O_RDONLY)) < 0) {
            mln_string_pool_free(f->file_path);
            mln_alloc_free(f);
            return NULL;
        }
        f->is_tmp = 0;
        if (fstat(f->fd, &st) < 0) {
            close(f->fd);
            mln_string_pool_free(f->file_path);
            mln_alloc_free(f);
            return NULL;
        }
        f->mtime = st.st_mtime;
        f->ctime = st.st_ctime;
        f->atime = st.st_atime;
        f->size = st.st_size;
        f->refer_cnt = 0;
        reg_file_chain_add(&(fs->reg_free_head), &(fs->reg_free_tail), f);
        f->fset = fs;
        if ((rn = mln_rbtree_node_new(fs->reg_file_tree, f)) == NULL) {
            close(f->fd);
            mln_string_pool_free(f->file_path);
            mln_alloc_free(f);
            return NULL;
        }
        mln_rbtree_insert(fs->reg_file_tree, rn);
        f->node = rn;
    } else {
        f = (mln_file_t *)(rn->data);
    }

    if (f->refer_cnt++ == 0) {
        reg_file_chain_del(&(fs->reg_free_head), &(fs->reg_free_tail), f);
    }

    return f;
}

void mln_file_close(void *pfile)
{
    if (pfile == NULL) return;

    mln_file_t *f = (mln_file_t *)pfile;
    mln_fileset_t *fs;

    if (f->is_tmp) {
        if (f->fd >= 0)
            close(f->fd);
        mln_alloc_free(f);
        return;
    }

    if (--(f->refer_cnt) > 0) {
        return;
    }

    fs = f->fset;
    reg_file_chain_add(&(fs->reg_free_head), &(fs->reg_free_tail), f);

    if (fs->reg_file_tree->nr_node > fs->max_file) {
        f = fs->reg_free_head;
        reg_file_chain_del(&(fs->reg_free_head), &(fs->reg_free_tail), f);
        mln_rbtree_delete(fs->reg_file_tree, f->node);
        mln_rbtree_node_free(fs->reg_file_tree, f->node);
    }
}

static void mln_file_free(void *pfile)
{
    if (pfile == NULL) return;

    mln_file_t *f = (mln_file_t *)pfile;
    if (f->file_path != NULL)
        mln_string_pool_free(f->file_path);
    if (f->fd >= 0) close(f->fd);
    mln_alloc_free(f);
}

mln_file_t *mln_file_open_tmp(mln_alloc_t *pool)
{
    char dir_path[512] = {0};
    char tmp_path[1024] = {0};
    struct timeval now;
    unsigned long suffix;
    mln_file_t *f;

    snprintf(dir_path, sizeof(dir_path)-1, "%s/%s", mln_path(), mln_file_tmp_dir);
    if (mkdir(dir_path, S_IRWXU) < 0) {
        if (errno != EEXIST) {
            return NULL;
        }
    }

    f = (mln_file_t *)mln_alloc_m(pool, sizeof(mln_file_t));
    if (f == NULL) {
        return NULL;
    }
    f->file_path = NULL;
    f->fd = -1;
    f->is_tmp = 1;
    f->mtime = f->ctime = f->atime = 0;
    f->size = 0;
    f->refer_cnt = 0;
    f->prev = f->next = NULL;
    f->fset = NULL;
    f->node = NULL;
lp:
    gettimeofday(&now, NULL);
    suffix = now.tv_sec * 1000000 + now.tv_usec;
    snprintf(tmp_path, sizeof(tmp_path)-1, "%s/mln_tmp_%lu.tmp", dir_path, suffix);
    f->fd = open(tmp_path, O_RDWR|O_CREAT|O_EXCL, S_IRWXU);
    if (f->fd < 0) {
        if (errno == EEXIST) {
            memset(tmp_path, 0, sizeof(tmp_path));
            goto lp;
        }
        mln_alloc_free(f);
        return NULL;
    }
    unlink(tmp_path);
    f->mtime = f->ctime = f->atime = now.tv_sec;

    return f;
}


MLN_CHAIN_FUNC_DEFINE(reg_file, \
                      mln_file_t, \
                      static inline void, \
                      prev, \
                      next);

