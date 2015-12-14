
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
                       static inline void, \
                       __NONNULL3(1,2,3));
static int mln_file_set_hash(mln_hash_t *h, void *key);
static int mln_file_set_cmp(mln_hash_t *h, void *key1, void *key2);
static void mln_file_free(void *pfile);

mln_file_set_t *mln_file_set_init(mln_size_t max_file)
{
    struct mln_hash_attr attr;
    mln_file_set_t *fs;

    fs = (mln_file_set_t *)malloc(sizeof(mln_file_set_t));
    if (fs == NULL) {
        return NULL;
    }

    fs->pool = mln_alloc_init();
    if (fs->pool == NULL) {
        free(fs);
        return NULL;
    }

    attr.hash = mln_file_set_hash;
    attr.cmp = mln_file_set_cmp;
    attr.free_key = NULL;
    attr.free_val = mln_file_free;
    attr.len_base = max_file >> 1;
    attr.expandable = 0;
    attr.calc_prime = 0;
    fs->reg_file_hash = mln_hash_init(&attr);
    if (fs->reg_file_hash == NULL) {
        mln_alloc_destroy(fs->pool);
        free(fs);
        return NULL;
    }

    fs->reg_free_head = fs->reg_free_tail = NULL;
    fs->max_file = max_file;

    return fs;
}

static int mln_file_set_hash(mln_hash_t *h, void *key)
{
    mln_string_t *str = (mln_string_t *)key;
    mln_s8ptr_t s, send = str->str + str->len;
    register int index = 0, steplen = (str->len >> 3) - 1;
    if (steplen <= 0) steplen = 1;

    for (s = str->str; s < send; s+=steplen) {
        index += (*s * 3);
    }
    if (index < 0) index = -index;
    index %= h->len;

    return index;
}

static int mln_file_set_cmp(mln_hash_t *h, void *key1, void *key2)
{
    return !mln_strcmp((mln_string_t *)key1, (mln_string_t *)key2);
}

void mln_file_set_destroy(mln_file_set_t *fs)
{
    if (fs == NULL) return;

    if (fs->reg_file_hash != NULL)
        mln_hash_destroy(fs->reg_file_hash, M_HASH_F_VAL);
    if (fs->pool != NULL)
        mln_alloc_destroy(fs->pool);
    free(fs);
}


mln_file_t *mln_file_open(mln_file_set_t *fs, const char *filepath)
{
    mln_file_t *f;
    mln_string_t path;
    mln_string_set(&path, filepath);

    f = mln_hash_search(fs->reg_file_hash, &path);
    if (f == NULL) {
        struct stat st;

        f = (mln_file_t *)mln_alloc_m(fs->pool, sizeof(mln_file_t));
        if (f == NULL) {
            return NULL;
        }
        f->file_path = mln_new_string_pool(fs->pool, filepath);
        if (f->file_path == NULL) {
            mln_alloc_free(f);
            return NULL;
        }
        f->fd = open(filepath, O_RDONLY);
        if (f->fd < 0) {
            mln_free_string_pool(f->file_path);
            mln_alloc_free(f);
            return NULL;
        }
        f->is_tmp = 0;
        if (fstat(f->fd, &st) < 0) {
            close(f->fd);
            mln_free_string_pool(f->file_path);
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
        if (mln_hash_insert(fs->reg_file_hash, f->file_path, f) < 0) {
            close(f->fd);
            mln_free_string_pool(f->file_path);
            mln_alloc_free(f);
            return NULL;
        }
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
    mln_file_set_t *fs;

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

    if (fs->reg_file_hash->nr_nodes > fs->max_file) {
        f = fs->reg_free_head;
        reg_file_chain_del(&(fs->reg_free_head), &(fs->reg_free_tail), f);
        mln_hash_remove(fs->reg_file_hash, f->file_path, M_HASH_F_VAL);
    }
}

static void mln_file_free(void *pfile)
{
    if (pfile == NULL) return;

    mln_file_t *f = (mln_file_t *)pfile;
    if (f->file_path != NULL)
        mln_free_string_pool(f->file_path);
    if (f->fd >= 0) close(f->fd);
    mln_alloc_free(f);
}

mln_file_t *mln_file_tmp_open(mln_alloc_t *pool)
{
    char dir_path[1024] = {0};
    char tmp_path[1024] = {0};
    struct timeval now;
    unsigned long suffix;
    mln_file_t *f;

    snprintf(dir_path, sizeof(dir_path)-1, "%s/%s", mln_get_path(), mln_file_tmp_dir);
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

