
/*
 * Copyright (C) Niklaus F.Schen.
 */

#include "mln_file.h"
#include "mln_path.h"
#include "mln_func.h"
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#if defined(MSVC)
#include "mln_utils.h"
#else
#include <sys/time.h>
#endif

MLN_CHAIN_FUNC_DECLARE(static inline, \
                       reg_file, \
                       mln_file_t, );
static mln_u64_t mln_file_hash_calc(mln_hash_t *h, void *key);
static int mln_file_hash_cmp(mln_hash_t *h, void *key1, void *key2);
static void mln_file_free(void *pfile);

MLN_FUNC(static, mln_u64_t, mln_file_hash_calc, (mln_hash_t *h, void *key), (h, key), {
    char *s = (char *)key;
    mln_u64_t hash = 14695981039346656037ULL;
    while (*s) {
        hash ^= (unsigned char)*s++;
        hash *= 1099511628211ULL;
    }
    return hash % h->len;
})

MLN_FUNC(static, int, mln_file_hash_cmp, (mln_hash_t *h, void *key1, void *key2), (h, key1, key2), {
    (void)h;
    return !strcmp((char *)key1, (char *)key2);
})

MLN_FUNC(, mln_fileset_t *, mln_fileset_init, (mln_size_t max_file), (max_file), {
    mln_fileset_t *fs;

    fs = (mln_fileset_t *)malloc(sizeof(mln_fileset_t));
    if (fs == NULL) {
        return NULL;
    }

    fs->pool = mln_alloc_init(NULL, 0);
    if (fs->pool == NULL) {
        free(fs);
        return NULL;
    }

    fs->reg_file_hash = mln_hash_new_fast(mln_file_hash_calc, mln_file_hash_cmp, NULL, mln_file_free, max_file < 32 ? 32 : max_file, 1, 1);
    if (fs->reg_file_hash == NULL) {
        mln_alloc_destroy(fs->pool);
        free(fs);
        return NULL;
    }

    fs->reg_free_head = fs->reg_free_tail = NULL;
    fs->max_file = max_file;

    return fs;
})

MLN_FUNC_VOID(, void, mln_fileset_destroy, (mln_fileset_t *fs), (fs), {
    if (fs == NULL) return;

    if (fs->reg_file_hash != NULL)
        mln_hash_free(fs->reg_file_hash, M_HASH_F_VAL);
    if (fs->pool != NULL)
        mln_alloc_destroy(fs->pool);
    free(fs);
})


MLN_FUNC(, mln_file_t *, mln_file_open, (mln_fileset_t *fs, char *filepath), (fs, filepath), {
    mln_file_t *f;

    f = (mln_file_t *)mln_hash_search(fs->reg_file_hash, filepath);
    if (f == NULL) {
        struct stat st;

        if ((f = (mln_file_t *)mln_alloc_m(fs->pool, sizeof(mln_file_t))) == NULL) {
            return NULL;
        }
        if ((f->file_path = mln_string_pool_new(fs->pool, filepath)) == NULL) {
            mln_alloc_free(f);
            return NULL;
        }
        if ((f->fd = open(filepath, O_RDONLY)) < 0) {
            mln_string_free(f->file_path);
            mln_alloc_free(f);
            return NULL;
        }
        f->is_tmp = 0;
        if (fstat(f->fd, &st) < 0) {
            close(f->fd);
            mln_string_free(f->file_path);
            mln_alloc_free(f);
            return NULL;
        }
        f->mtime = st.st_mtime;
        f->ctime = st.st_ctime;
        f->atime = st.st_atime;
        f->size = st.st_size;
        f->refer_cnt = 0;
        f->prev = f->next = NULL;
        reg_file_chain_add(&(fs->reg_free_head), &(fs->reg_free_tail), f);
        f->fset = fs;
        if (mln_hash_insert(fs->reg_file_hash, f->file_path->data, f) < 0) {
            close(f->fd);
            mln_string_free(f->file_path);
            mln_alloc_free(f);
            return NULL;
        }
    }

    if (f->refer_cnt++ == 0) {
        reg_file_chain_del(&(fs->reg_free_head), &(fs->reg_free_tail), f);
    }

    return f;
})

MLN_FUNC_VOID(, void, mln_file_close, (mln_file_t *pfile), (pfile), {
    if (pfile == NULL) return;

    mln_fileset_t *fs;

    if (pfile->is_tmp) {
        if (pfile->fd >= 0)
            close(pfile->fd);
        mln_alloc_free(pfile);
        return;
    }

    if (--(pfile->refer_cnt) > 0) {
        return;
    }

    fs = pfile->fset;
    reg_file_chain_add(&(fs->reg_free_head), &(fs->reg_free_tail), pfile);

    if (fs->reg_file_hash->nr_nodes > fs->max_file) {
        pfile = fs->reg_free_head;
        reg_file_chain_del(&(fs->reg_free_head), &(fs->reg_free_tail), pfile);
        mln_hash_remove(fs->reg_file_hash, pfile->file_path->data, M_HASH_F_VAL);
    }
})

MLN_FUNC_VOID(static, void, mln_file_free, (void *pfile), (pfile), {
    if (pfile == NULL) return;

    mln_file_t *f = (mln_file_t *)pfile;
    if (f->file_path != NULL)
        mln_string_free(f->file_path);
    if (f->fd >= 0) close(f->fd);
    mln_alloc_free(f);
})

MLN_FUNC(, mln_file_t *, mln_file_tmp_open, (mln_alloc_t *pool), (pool), {
    char dir_path[512] = {0};
    char tmp_path[1024] = {0};
    struct timeval now;
    unsigned long suffix;
    mln_file_t *f;

    snprintf(dir_path, sizeof(dir_path)-1, "%s", mln_path_tmpfile());
#if defined(MSVC)
    if (mkdir(dir_path) < 0) {
#else
    if (mkdir(dir_path, S_IRWXU) < 0) {
#endif
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
#if defined(MSVC)
    f->fd = open(tmp_path, O_RDWR|O_CREAT|O_EXCL, _S_IREAD|_S_IWRITE|_S_IEXEC);
#else
    f->fd = open(tmp_path, O_RDWR|O_CREAT|O_EXCL, S_IRWXU);
#endif
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
})


MLN_CHAIN_FUNC_DEFINE(static inline, \
                      reg_file, \
                      mln_file_t, \
                      prev, \
                      next);
