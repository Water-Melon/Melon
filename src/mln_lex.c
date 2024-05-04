
/*
 * Copyright (C) Niklaus F.Schen.
 */
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#if defined(MLN_C99)
#define _GNU_SOURCE
#endif
#include <errno.h>
#if !defined(MSVC)
#include <dirent.h>
#include <unistd.h>
#else
#include "mln_utils.h"
#endif
#include "mln_string.h"
#include "mln_lex.h"
#include "mln_path.h"

/*
 * error information
 */
char mln_lex_errmsg[][MLN_LEX_ERRMSG_LEN] = {
    "Succeed.",
    "No memory.",
    "Invalid character.",
    "Invalid hexadecimal.",
    "Invalid decimal.",
    "Invalid octal number.",
    "Invalid floating number.",
    "Read file error.",
    "Invalid end of file.",
    "Invalid end of line.",
    "Invalid input stream type.",
    "Invalid file path.",
    "File infinite loop.",
    "Unknown macro.",
    "Too many macro definitions.",\
    "Invalid macro."
};
char error_msg_err[] = "No memory to store error message.";

MLN_FUNC(, mln_lex_preprocess_data_t *, mln_lex_preprocess_data_new, (mln_alloc_t *pool), (pool), {
    mln_lex_preprocess_data_t *lpd = (mln_lex_preprocess_data_t *)mln_alloc_m(pool, sizeof(mln_lex_preprocess_data_t));
    if (lpd == NULL) return NULL;
    lpd->if_level = 0;
    lpd->if_matched = 0;
    return lpd;
})

MLN_FUNC_VOID(, void, mln_lex_preprocess_data_free, (mln_lex_preprocess_data_t *lpd), (lpd), {
    if (lpd == NULL) return;
    mln_alloc_free(lpd);
})

MLN_FUNC(static inline, int, mln_lex_base_dir, \
         (mln_lex_t *lex, mln_lex_input_t *input, char *path, int *err), \
         (lex, input, path, err), \
{
    char *p = strrchr(path, '/');
    char tmp[1024] = {0};
    int n;
    mln_string_t dir;

    if (p == NULL) {
        n = snprintf(tmp, sizeof(tmp) - 1, ".");
        mln_string_nset(&dir, tmp, n);
    } else {
        mln_string_nset(&dir, path, p - path);
    }
    if ((input->dir = mln_string_pool_dup(lex->pool, &dir)) == NULL) {
        *err = MLN_LEX_ENMEM;
        return -1;
    }
    return 0;
})

mln_lex_input_t *mln_lex_input_new(mln_lex_t *lex, mln_u32_t type, mln_string_t *data, int *err, mln_u64_t line)
{
    int r;
    mln_lex_input_t *li;
    if ((li = (mln_lex_input_t *)mln_alloc_m(lex->pool, sizeof(mln_lex_input_t))) == NULL) {
        *err = MLN_LEX_ENMEM;
        return NULL;
    }
    li->type = type;
    li->line = line;
    if ((li->data = mln_string_pool_dup(lex->pool, data)) == NULL) {
        mln_alloc_free(li);
        *err = MLN_LEX_ENMEM;
        return NULL;
    }
    li->dir = NULL;
    if (type == M_INPUT_T_BUF) {
        li->fd = -1;
        li->pos = li->buf = li->data->data;
        li->buf_len = data->len;
    } else if (type == M_INPUT_T_FILE) {
        int n;
        mln_u32_t len;
        char path[1024] = {0};
        char tmp_path[1024];
        char *melang_path = NULL;

        if (data->len && data->data[0] == (mln_u8_t)'@') {
            if (lex->cur == NULL || lex->cur->dir == NULL)
                n = snprintf(path, sizeof(path) - 1, "./");
            else
                n = snprintf(path, sizeof(path) - 1, "%s/", lex->cur->dir->data);
            len = (data->len - 1) >= 1024? 1023: (data->len - 1);
            memcpy(path + n, data->data + 1, len);
        } else {
            len = data->len >= 1024? 1023: data->len;
            memcpy(path, data->data, len);
        }
#if defined(MSVC)
        if (len > 1 && path[1] == ':') {
#else
        if (path[0] == '/') {
#endif
            li->fd = open(path, O_RDONLY);
            r = mln_lex_base_dir(lex, li, path, err);
        } else {
#if defined(MSVC)
            if (!_access(path, 0)) {
#else
            if (!access(path, F_OK)) {
#endif
                li->fd = open(path, O_RDONLY);
                r = mln_lex_base_dir(lex, li, path, err);
            } else if (lex->env != NULL && (melang_path = getenv((char *)(lex->env->data))) != NULL) {
                char *end = strchr(melang_path, ';');
                int found = 0;
                while (end != NULL) {
                    *end = 0;
                    n = snprintf(tmp_path, sizeof(tmp_path)-1, "%s/%s", melang_path, path);
                    tmp_path[n] = 0;
#if defined(MSVC)
                    if (!_access(tmp_path, 0)) {
#else
                    if (!access(tmp_path, F_OK)) {
#endif
                        li->fd = open(tmp_path, O_RDONLY);
                        r = mln_lex_base_dir(lex, li, tmp_path, err);
                        found = 1;
                        break;
                    }
                    melang_path = end + 1;
                    end = strchr(melang_path, ';');
                }
                if (!found) {
                    if (*melang_path) {
                        n = snprintf(tmp_path, sizeof(tmp_path)-1, "%s/%s", melang_path, path);
                        tmp_path[n] = 0;
                        li->fd = open(tmp_path, O_RDONLY);
                        r = mln_lex_base_dir(lex, li, tmp_path, err);
                    } else {
                        goto goon;
                    }
                }
            } else {
goon:
                n = snprintf(tmp_path, sizeof(tmp_path)-1, "%s/%s", mln_path_melang_lib(), path);
                tmp_path[n] = 0;
                li->fd = open(tmp_path, O_RDONLY);
                r = mln_lex_base_dir(lex, li, tmp_path, err);
            }
        }
        li->buf = li->pos = NULL;
        li->buf_len = MLN_DEFAULT_BUFLEN;
        if (r < 0 || li->fd < 0) {
            mln_lex_input_free(li);
            *err = MLN_LEX_EREAD;
            return NULL;
        }
    } else {
        mln_alloc_free(li->data);
        mln_alloc_free(li);
        *err = MLN_LEX_EINPUTTYPE;
        return NULL;
    }

    return li;
}

MLN_FUNC_VOID(, void, mln_lex_input_free, (void *in), (in), {
    if (in == NULL) return;
    mln_lex_input_t *input = (mln_lex_input_t *)in;
    if (input->fd >= 0) close(input->fd);
    if (input->data != NULL) mln_string_free(input->data);
    if (input->dir != NULL) mln_string_free(input->dir);
    if (input->buf != NULL && input->type == M_INPUT_T_FILE) mln_alloc_free(input->buf);
    mln_alloc_free(input);
})

MLN_FUNC(, mln_lex_macro_t *, mln_lex_macro_new, \
         (mln_alloc_t *pool, mln_string_t *key, mln_string_t *val), \
         (pool, key, val), \
{
    mln_lex_macro_t *lm;
    if ((lm = (mln_lex_macro_t *)mln_alloc_m(pool, sizeof(mln_lex_macro_t))) == NULL) {
        return NULL;
    }
    if ((lm->key = mln_string_pool_dup(pool, key)) == NULL) {
        mln_alloc_free(lm);
        return NULL;
    }
    if (val != NULL && (lm->val = mln_string_pool_dup(pool, val)) == NULL) {
        mln_string_free(lm->key);
        mln_alloc_free(lm);
        return NULL;
    }
    if (val == NULL) lm->val = NULL;
    return lm;
})

MLN_FUNC_VOID(, void, mln_lex_macro_free, (void *data), (data), {
    if (data == NULL) return;
    mln_lex_macro_t *lm = (mln_lex_macro_t *)data;
    if (lm->key != NULL) mln_string_free(lm->key);
    if (lm->val != NULL) mln_string_free(lm->val);
    mln_alloc_free(lm);
})

MLN_FUNC(static, int, mln_lex_macro_cmp, \
         (const void *data1, const void *data2), (data1, data2), \
{
    mln_lex_macro_t *lm1 = (mln_lex_macro_t *)data1;
    mln_lex_macro_t *lm2 = (mln_lex_macro_t *)data2;
    return mln_string_strcmp(lm1->key, lm2->key);
})

MLN_FUNC(static inline, mln_lex_keyword_t *, mln_lex_keyword_new, \
         (mln_string_t *keyword, mln_uauto_t val), (keyword, val), \
{
    mln_lex_keyword_t *lk = (mln_lex_keyword_t *)malloc(sizeof(mln_lex_keyword_t));
    if (lk == NULL) return NULL;
    if ((lk->keyword = mln_string_dup(keyword)) == NULL) {
        free(lk);
        return NULL;
    }
    lk->val = val;
    return lk;
})

MLN_FUNC_VOID(static, void, mln_lex_keyword_free, (void *data), (data), {
    if (data == NULL) return;
    mln_lex_keyword_t *lk = (mln_lex_keyword_t *)data;
    if (lk->keyword != NULL) mln_string_free(lk->keyword);
    free(lk);
})

MLN_FUNC(static, int, mln_lex_keywords_cmp, \
         (const void *data1, const void *data2), (data1, data2), \
{
    mln_lex_keyword_t *lk1 = (mln_lex_keyword_t *)data1;
    mln_lex_keyword_t *lk2 = (mln_lex_keyword_t *)data2;
    return mln_string_strcmp(lk1->keyword, lk2->keyword);
})

MLN_FUNC(, mln_lex_t *, mln_lex_init, (struct mln_lex_attr *attr), (attr), {
    mln_lex_macro_t *lm;
    struct mln_rbtree_attr rbattr;
    mln_rbtree_node_t *rn;
    mln_string_t k1 = mln_string("1");
    mln_string_t k2 = mln_string("true");
    mln_string_t *scan;
    mln_lex_keyword_t *newkw;
    mln_lex_t *lex;
    if ((lex = (mln_lex_t *)mln_alloc_m(attr->pool, sizeof(mln_lex_t))) == NULL) {
        return NULL;
    }
    lex->pool = attr->pool;
    lex->preprocess_data = NULL;

    rbattr.pool = lex->pool;
    rbattr.pool_alloc = (rbtree_pool_alloc_handler)mln_alloc_m;
    rbattr.pool_free = (rbtree_pool_free_handler)mln_alloc_free;
    rbattr.cmp = mln_lex_macro_cmp;
    rbattr.data_free = mln_lex_macro_free;
    if ((lex->macros = mln_rbtree_new(&rbattr)) == NULL) {
        mln_alloc_free(lex);
        return NULL;
    }
    if ((lm = mln_lex_macro_new(lex->pool, &k1, NULL)) == NULL) {
err:
        mln_rbtree_free(lex->macros);
        mln_alloc_free(lex);
        return NULL;
    }
    if ((rn = mln_rbtree_node_new(lex->macros, lm)) == NULL) {
        mln_lex_macro_free(lm);
        goto err;
    }
    mln_rbtree_insert(lex->macros, rn);
    if ((lm = mln_lex_macro_new(lex->pool, &k2, NULL)) == NULL) {
        goto err;
    }
    if ((rn = mln_rbtree_node_new(lex->macros, lm)) == NULL) {
        mln_lex_macro_free(lm);
        goto err;
    }
    mln_rbtree_insert(lex->macros, rn);

    lex->cur = NULL;
    if ((lex->stack = mln_stack_init(mln_lex_input_free, NULL)) == NULL) {
        goto err;
    }
    if (attr->hooks != NULL)
        memcpy(&(lex->hooks), attr->hooks, sizeof(mln_lex_hooks_t));
    else
        memset(&(lex->hooks), 0, sizeof(mln_lex_hooks_t));

    if (attr->keywords != NULL) {
        rbattr.pool = lex->pool;
        rbattr.pool_alloc = (rbtree_pool_alloc_handler)mln_alloc_m;
        rbattr.pool_free = (rbtree_pool_free_handler)mln_alloc_free;
        rbattr.cmp = mln_lex_keywords_cmp;
        rbattr.data_free = mln_lex_keyword_free;
        if ((lex->keywords = mln_rbtree_new(&rbattr)) == NULL) {
            mln_stack_destroy(lex->stack);
            goto err;
        }
        for (scan = attr->keywords; scan->data != NULL; ++scan) {
            if ((newkw = mln_lex_keyword_new(scan, scan - attr->keywords)) == NULL) {
                 mln_rbtree_free(lex->keywords);
                 mln_stack_destroy(lex->stack);
                 goto err;
            }
            if ((rn = mln_rbtree_node_new(lex->keywords, newkw)) == NULL) {
                mln_lex_keyword_free(newkw);
                mln_rbtree_free(lex->keywords);
                mln_stack_destroy(lex->stack);
                goto err;
            }
            mln_rbtree_insert(lex->keywords, rn);
        }
    } else {
        lex->keywords = NULL;
    }
    lex->err_msg = NULL;
    lex->result_buf = lex->result_pos = NULL;
    lex->result_buf_len = MLN_DEFAULT_BUFLEN;
    lex->line = 1;
    lex->error = MLN_LEX_SUCCEED;
    lex->preprocess = attr->preprocess;
    lex->ignore = 0;
    if (attr->env != NULL) {
        if ((lex->env = mln_string_pool_dup(lex->pool, attr->env)) == NULL) {
            mln_lex_destroy(lex);
            return NULL;
        }
    } else {
        lex->env = NULL;
    }

    if (attr->data != NULL) {
        if (attr->type == M_INPUT_T_BUF) {
            if (mln_lex_push_input_buf_stream(lex, attr->data) < 0) {
                mln_lex_destroy(lex);
                return NULL;
            }
        } else if (attr->type == M_INPUT_T_FILE) {
            if (mln_lex_push_input_file_stream(lex, attr->data) < 0) {
                mln_lex_destroy(lex);
                return NULL;
            }
        } else {
            mln_lex_destroy(lex);
            return NULL;
        }
    }

    return lex;
})

MLN_FUNC_VOID(, void, mln_lex_destroy, (mln_lex_t *lex), (lex), {
    if (lex == NULL) return;
    if (lex->preprocess) {
        mln_lex_preprocess_data_free((mln_lex_preprocess_data_t *)(lex->hooks.at_data));
        lex->hooks.at_data = NULL;
    }
    if (lex->macros != NULL) mln_rbtree_free(lex->macros);
    if (lex->cur != NULL) mln_lex_input_free(lex->cur);
    if (lex->stack != NULL) mln_stack_destroy(lex->stack);
    if (lex->err_msg != NULL) mln_alloc_free(lex->err_msg);
    if (lex->result_buf != NULL) mln_alloc_free(lex->result_buf);
    if (lex->keywords != NULL) mln_rbtree_free(lex->keywords);
    if (lex->env != NULL) mln_string_free(lex->env);
    if (lex->preprocess_data != NULL) mln_lex_preprocess_data_free(lex->preprocess_data);
    mln_alloc_free(lex);
})

char *mln_lex_strerror(mln_lex_t *lex)
{
    if (lex->err_msg != NULL) mln_alloc_free(lex->err_msg);
    int len = 0;
    if (lex->cur != NULL) {
        len += (lex->cur->type != M_INPUT_T_FILE? 0: lex->cur->data->len);
    }
    len += strlen(mln_lex_errmsg[lex->error]);
    len += ((lex->error == MLN_LEX_EREAD)?strlen(strerror(errno)):0);
    len += (lex->result_pos - lex->result_buf + 1);
    len += 256;
    if ((lex->err_msg = (mln_s8ptr_t)mln_alloc_c(lex->pool, len+1)) == NULL) {
        return error_msg_err;
    }

    int n = 0;
    if (lex->cur != NULL && lex->cur->fd >= 0)
        n += snprintf(lex->err_msg + n, len - n, "%s:", (char *)(lex->cur->data->data));
#if defined(MSVC) || defined(i386) || defined(__arm__) || defined(__wasm__)
    n += snprintf(lex->err_msg + n, len - n, "%llu: %s", lex->line, mln_lex_errmsg[lex->error]);
#else
    n += snprintf(lex->err_msg + n, len - n, "%lu: %s", lex->line, mln_lex_errmsg[lex->error]);
#endif
    if (lex->result_pos > lex->result_buf) {
        *(lex->err_msg+(n++)) = ' ';
        *(lex->err_msg+(n++)) = '"';
        mln_u32_t diff = lex->result_pos - lex->result_buf;
        memcpy(lex->err_msg+n, lex->result_buf, diff);
        n += diff;
        *(lex->err_msg+(n++)) = '"';
    }
    if (lex->error == MLN_LEX_EREAD)
        n += snprintf(lex->err_msg + n, len - n, ". %s", strerror(errno));
    lex->err_msg[n] = 0;
    return lex->err_msg;
}

int mln_lex_push_input_file_stream(mln_lex_t *lex, mln_string_t *path)
{
    int err = MLN_LEX_SUCCEED;
    mln_lex_input_t *in;
    char p[1024];
    struct stat path_stat;
    int n;

    if (path->len && path->data[0] == (mln_u8_t)'@') {
        if (lex->cur == NULL || lex->cur->dir == NULL)
            n = snprintf(p, sizeof(p) - 1, "./");
        else
            n = snprintf(p, sizeof(p) - 1, "%s/", lex->cur->dir->data);
        if (path->len - 1 >= sizeof(p) - n) {
            memcpy(p + n, path->data + 1, sizeof(p) - n -1);
            n += (sizeof(p) - n - 1);
        } else {
            memcpy(p + n, path->data + 1, path->len - 1);
            n += (path->len - 1);
        }
    } else {
        n = path->len > sizeof(p)-1? sizeof(p)-1: path->len;
        memcpy(p, path->data, n);
    }
    p[n] = 0;
#if defined(MSVC)
    if (_access(p, 0)) {
#else
    if (access(p, F_OK)) {
#endif
        mln_lex_error_set(lex, MLN_LEX_EFPATH);
        return -1;
    }
    if (stat(p, &path_stat) < 0) {
        mln_lex_error_set(lex, MLN_LEX_EFPATH);
        return -1;
    }

    if (S_ISDIR(path_stat.st_mode)) {
        int m;
        mln_string_t tmp;
        struct dirent *entry;
        DIR *directory;
#if defined(MSVC)
        WIN32_FIND_DATA fileData;
#endif
        if ((directory = opendir(p)) == NULL) {
            mln_lex_error_set(lex, MLN_LEX_EFPATH);
            return -1;
        }
        p[n++] = '/';
        while ((entry = readdir(directory)) != NULL) {
            m = strlen(entry->d_name);
            if (sizeof(p)-1-n < m)
                m = sizeof(p) - 1 - n;
            memcpy(p + n, entry->d_name, m);
            p[n + m] = 0;
            mln_string_nset(&tmp, p, n + m);
            path = &tmp;
#if defined(MSVC)
            FindClose(FindFirstFile(p, &fileData));
            if (fileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY || entry->d_name[0] == '.') {
#else
            if (entry->d_name[0] == '.') continue;
#if defined(MLN_C99)
            struct stat st;
            if (stat(entry->d_name, &st) < 0) {
                lex->error = MLN_LEX_EFPATH;
                closedir(directory);
                return -1;
            }
            if (!S_ISREG(st.st_mode)) {
#else
            if (entry->d_type != DT_REG) {
#endif
#endif
                continue;
            }

            if ((in = mln_lex_input_new(lex, M_INPUT_T_FILE, path, &err, lex->line)) == NULL) {
                lex->error = err;
                return -1;
            }
            if (lex->cur != NULL) {
                if (mln_stack_push(lex->stack, lex->cur) < 0) {
                    mln_lex_input_free(in);
                    lex->error = MLN_LEX_ENMEM;
                    closedir(directory);
                    return -1;
                }
                lex->cur = NULL;
            }
            if (mln_stack_push(lex->stack, in) < 0) {
                mln_lex_input_free(in);
                lex->error = MLN_LEX_ENMEM;
                closedir(directory);
                return -1;
            }
            lex->line = 1;
        }
        closedir(directory);
    } else {
        if ((in = mln_lex_input_new(lex, M_INPUT_T_FILE, path, &err, lex->line)) == NULL) {
            lex->error = err;
            return -1;
        }
        if (lex->cur != NULL) {
            if (mln_stack_push(lex->stack, lex->cur) < 0) {
                mln_lex_input_free(in);
                lex->error = MLN_LEX_ENMEM;
                return -1;
            }
            lex->cur = NULL;
        }
        if (mln_stack_push(lex->stack, in) < 0) {
            mln_lex_input_free(in);
            lex->error = MLN_LEX_ENMEM;
            return -1;
        }
        lex->line = 1;
    }
    return 0;
}

MLN_FUNC(, int, mln_lex_push_input_buf_stream, \
         (mln_lex_t *lex, mln_string_t *buf), (lex, buf), \
{
    int err = MLN_LEX_SUCCEED;
    mln_lex_input_t *in = mln_lex_input_new(lex, M_INPUT_T_BUF, buf, &err, lex->line);
    if (in == NULL) {
        lex->error = err;
        return -1;
    }
    if (lex->cur != NULL) {
        if (mln_stack_push(lex->stack, lex->cur) < 0) {
            mln_lex_input_free(in);
            lex->error = MLN_LEX_ENMEM;
            return -1;
        }
        lex->cur = NULL;
    }
    if (mln_stack_push(lex->stack, in) < 0) {
        mln_lex_input_free(in);
        lex->error = MLN_LEX_ENMEM;
        return -1;
    }
    lex->line = 1;
    return 0;
})

MLN_FUNC(static, int, mln_lex_check_file_loop_iterate_handler, \
         (void *st_data, void *data), (st_data, data), \
{
    mln_string_t *path = (mln_string_t *)data;
    mln_lex_input_t *in = (mln_lex_input_t *)st_data;
    if (in->type != M_INPUT_T_FILE) return 0;
    if (!mln_string_strcmp(path, in->data)) return -1;
    return 0;
})

int mln_lex_check_file_loop(mln_lex_t *lex, mln_string_t *path)
{
    char p[1024];
    struct stat path_stat;
    int n;

    if (path->len && path->data[0] == (mln_u8_t)'@') {
        if (lex->cur == NULL || lex->cur->dir == NULL)
            n = snprintf(p, sizeof(p) - 1, "./");
        else
            n = snprintf(p, sizeof(p) - 1, "%s/", lex->cur->dir->data);
        if (path->len - 1 >= sizeof(p) - n) {
            memcpy(p + n, path->data + 1, sizeof(p) - n -1);
            n += (sizeof(p) - n - 1);
        } else {
            memcpy(p + n, path->data + 1, path->len - 1);
            n += (path->len - 1);
        }
    } else {
        n = path->len > sizeof(p)-1? sizeof(p)-1: path->len;
        memcpy(p, path->data, n);
    }
    p[n] = 0;

#if defined(MSVC)
    if (_access(p, 0)) {
#else
    if (access(p, F_OK)) {
#endif
        mln_lex_error_set(lex, MLN_LEX_EFPATH);
        return -1;
    }
    if (stat(p, &path_stat) < 0) {
        mln_lex_error_set(lex, MLN_LEX_EFPATH);
        return -1;
    }
    if (S_ISDIR(path_stat.st_mode)) {
        int m;
        DIR *directory;
        mln_string_t tmp;
        struct dirent *entry;
#if defined(MSVC)
        WIN32_FIND_DATA fileData;
#endif

        if ((directory = opendir(p)) == NULL) {
            mln_lex_error_set(lex, MLN_LEX_EFPATH);
            return -1;
        }

        p[n++] = '/';
        while ((entry = readdir(directory)) != NULL) {
            m = strlen(entry->d_name);
            if (sizeof(p)-1-n < m)
                m = sizeof(p) - 1 - n;
            memcpy(p + n, entry->d_name, m);
            p[n + m] = 0;
            mln_string_nset(&tmp, p, n + m);
            path = &tmp;
#if defined(MSVC)
            FindClose(FindFirstFile(p, &fileData));
            if (fileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY || entry->d_name[0] == '.') {
#else
            if (entry->d_name[0] == '.') continue;
#if defined(MLN_C99)
            struct stat st;
            if (stat(entry->d_name, &st) < 0) {
                mln_lex_error_set(lex, MLN_LEX_EFPATH);
                closedir(directory);
                return -1;
            }
            if (!S_ISREG(st.st_mode)) {
#else
            if (entry->d_type != DT_REG) {
#endif
#endif
                continue;
            }

            if (lex->cur != NULL && !mln_string_strcmp(path, lex->cur->data)) {
                mln_lex_error_set(lex, MLN_LEX_EINCLUDELOOP);
                closedir(directory);
                return -1;
            }
            if (mln_stack_iterate(lex->stack, mln_lex_check_file_loop_iterate_handler, path) < 0) {
                mln_lex_error_set(lex, MLN_LEX_EINCLUDELOOP);
                closedir(directory);
                return -1;
            }
        }
        closedir(directory);
    } else {
        if (lex->cur != NULL && !mln_string_strcmp(path, lex->cur->data)) {
            mln_lex_error_set(lex, MLN_LEX_EINCLUDELOOP);
            return -1;
        }
        if (mln_stack_iterate(lex->stack, mln_lex_check_file_loop_iterate_handler, path) < 0) {
            mln_lex_error_set(lex, MLN_LEX_EINCLUDELOOP);
            return -1;
        }
    }
    return 0;
}

MLN_FUNC(, int, mln_lex_condition_test, (mln_lex_t *lex), (lex), {
    mln_lex_result_clean(lex);

    char c;
    int reverse = 0;
    mln_rbtree_node_t *rn;
    mln_lex_macro_t lm;
    mln_string_t tmp;

    while (1) {
        if ((c = mln_lex_getchar(lex)) == MLN_ERR) {
            return -1;
        }
        if (c == ' ' || c == '\t') continue;
        if (c == '\n') {
            mln_lex_error_set(lex, MLN_LEX_EINVEOL);
            return -1;
        }
        if (c == MLN_EOF) {
            mln_lex_error_set(lex, MLN_LEX_EINVEOF);
            return -1;
        }
        mln_lex_stepback(lex, c);
        break;
    }

    if ((c = mln_lex_getchar(lex)) == MLN_ERR) {
        return -1;
    }
    if (c == '!') reverse = 1;
    else mln_lex_stepback(lex, c);

    while (1) {
        if ((c = mln_lex_getchar(lex)) == MLN_ERR) {
            return -1;
        }
        if (c == ' ' || c == '\t' || c == '\n' || c == MLN_EOF) {
            break;
        }
        if (!mln_lex_is_letter(c) && !mln_isdigit(c)) {
            mln_lex_error_set(lex, MLN_LEX_EINVCHAR);
            return -1;
        }
        if (mln_lex_putchar(lex, c) == MLN_ERR) {
            return -1;
        }
    }

    mln_lex_result_get(lex, &tmp);
    lm.key = &tmp;
    lm.val = NULL;
    rn = mln_rbtree_search(lex->macros, &lm);
    mln_lex_result_clean(lex);
    if (mln_rbtree_null(rn, lex->macros)) {
        return reverse? 1: 0;
    }
    return reverse? 0: 1;
})

MLN_FUNC(, mln_lex_off_t, mln_lex_snapshot_record, (mln_lex_t *lex), (lex), {
    mln_lex_off_t ret;
    mln_lex_input_t *input = lex->cur;

    if (input == NULL && (input = mln_stack_top(lex->stack)) == NULL) {
        ret.soff = NULL;
        return ret;
    }

    if (input->type == M_INPUT_T_BUF) {
        ret.soff = input->pos;
    } else {
        ret.foff = lseek(input->fd, 0, SEEK_CUR) - (input->buf_len - (input->pos - input->buf));
    }

    return ret;
})

MLN_FUNC_VOID(, void, mln_lex_snapshot_apply, (mln_lex_t *lex, mln_lex_off_t off), (lex, off), {
    mln_lex_input_t *input = lex->cur;

    if (input == NULL && (input = mln_stack_top(lex->stack)) == NULL) {
        return;
    }
    if (input->type == M_INPUT_T_BUF) {
        if (off.soff < input->buf || off.soff >= input->buf + input->buf_len) return;

        input->pos = off.soff;
    } else {
        off_t foff = lseek(input->fd, 0, SEEK_CUR);
        if (off.foff >= foff || off.foff < foff - input->buf_len) return;
        lseek(input->fd, off.foff, SEEK_SET);
        input->pos = input->buf + input->buf_len;
    }
})

