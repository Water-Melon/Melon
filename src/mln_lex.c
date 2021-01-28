
/*
 * Copyright (C) Niklaus F.Schen.
 */
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <ctype.h>
#include "mln_log.h"
#include "mln_string.h"
#include "mln_lex.h"

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

mln_lex_preprocessData_t *mln_lex_preprocessData_new(mln_alloc_t *pool)
{
    mln_lex_preprocessData_t *lpd = (mln_lex_preprocessData_t *)mln_alloc_m(pool, sizeof(mln_lex_preprocessData_t));
    if (lpd == NULL) return NULL;
    lpd->if_level = 0;
    lpd->if_matched = 0;
    return lpd;
}

void mln_lex_preprocessData_free(mln_lex_preprocessData_t *lpd)
{
    if (lpd == NULL) return;
    mln_alloc_free(lpd);
}

mln_lex_input_t *
mln_lex_input_new(mln_alloc_t *pool, mln_u32_t type, mln_string_t *data, int *err, mln_u64_t line)
{
    mln_lex_input_t *li;
    if ((li = (mln_lex_input_t *)mln_alloc_m(pool, sizeof(mln_lex_input_t))) == NULL) {
        *err = MLN_LEX_ENMEM;
        return NULL;
    }
    li->type = type;
    li->line = line;
    if ((li->data = mln_string_pool_dup(pool, data)) == NULL) {
        mln_alloc_free(li);
        *err = MLN_LEX_ENMEM;
        return NULL;
    }
    if (type == M_INPUT_T_BUF) {
        li->fd = -1;
        li->pos = li->buf = li->data->data;
        li->buf_len = data->len;
    } else if (type == M_INPUT_T_FILE) {
        int n;
        char path[1024] = {0};
        char *melang_path = NULL;
        mln_u32_t len = data->len >= 1024? 1023: data->len;
        memcpy(path, data->data, len);
        path[len] = 0;
        if (path[0] == '/') {
            li->fd = open(path, O_RDONLY);
        } else {
            if (!access(path, F_OK)) {
                li->fd = open(path, O_RDONLY);
            } else if ((melang_path = getenv("MELANG_PATH")) != NULL) {
                char *end = strchr(melang_path, ';');
                char tmp_path[1024];
                int found = 0;
                while (end != NULL) {
                    *end = 0;
                    n = snprintf(tmp_path, sizeof(tmp_path)-1, "%s/%s", melang_path, path);
                    tmp_path[n] = 0;
                    if (!access(tmp_path, F_OK)) {
                        li->fd = open(tmp_path, O_RDONLY);
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
                    } else {
                        li->fd = -1;
                    }
                }
            } else {
                char tmp_path[1024];
                n = snprintf(tmp_path, sizeof(tmp_path)-1, "/usr/lib/melang/%s", path);
                tmp_path[n] = 0;
                li->fd = open(tmp_path, O_RDONLY);
            }
        }
        if (li->fd < 0) {
            mln_alloc_free(li->data);
            mln_alloc_free(li);
            *err = MLN_LEX_EREAD;
            return NULL;
        }
        li->buf = li->pos = NULL;
        li->buf_len = MLN_DEFAULT_BUFLEN;
    } else {
        mln_alloc_free(li->data);
        mln_alloc_free(li);
        *err = MLN_LEX_EINPUTTYPE;
        return NULL;
    }

    return li;
}

void mln_lex_input_free(void *in)
{
    if (in == NULL) return;
    mln_lex_input_t *input = (mln_lex_input_t *)in;
    if (input->fd >= 0) close(input->fd);
    if (input->data != NULL) mln_string_pool_free(input->data);
    if (input->buf != NULL && input->type == M_INPUT_T_FILE) mln_alloc_free(input->buf);
    mln_alloc_free(input);
}

mln_lex_macro_t *
mln_lex_macro_new(mln_alloc_t *pool, mln_string_t *key, mln_string_t *val)
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
        mln_string_pool_free(lm->key);
        mln_alloc_free(lm);
        return NULL;
    }
    if (val == NULL) lm->val = NULL;
    return lm;
}

void mln_lex_macro_free(void *data)
{
    if (data == NULL) return;
    mln_lex_macro_t *lm = (mln_lex_macro_t *)data;
    if (lm->key != NULL) mln_string_pool_free(lm->key);
    if (lm->val != NULL) mln_string_pool_free(lm->val);
    mln_alloc_free(lm);
}

static int mln_lex_macro_cmp(const void *data1, const void *data2)
{
    mln_lex_macro_t *lm1 = (mln_lex_macro_t *)data1;
    mln_lex_macro_t *lm2 = (mln_lex_macro_t *)data2;
    return mln_string_strcmp(lm1->key, lm2->key);
}

static inline mln_lex_keyword_t *
mln_lex_keyword_new(mln_string_t *keyword, mln_uauto_t val)
{
    mln_lex_keyword_t *lk = (mln_lex_keyword_t *)malloc(sizeof(mln_lex_keyword_t));
    if (lk == NULL) return NULL;
    if ((lk->keyword = mln_string_dup(keyword)) == NULL) {
        free(lk);
        return NULL;
    }
    lk->val = val;
    return lk;
}

static void mln_lex_keyword_free(void *data)
{
    if (data == NULL) return;
    mln_lex_keyword_t *lk = (mln_lex_keyword_t *)data;
    if (lk->keyword != NULL) mln_string_free(lk->keyword);
    free(lk);
}

static int mln_lex_keywords_cmp(const void *data1, const void *data2)
{
    mln_lex_keyword_t *lk1 = (mln_lex_keyword_t *)data1;
    mln_lex_keyword_t *lk2 = (mln_lex_keyword_t *)data2;
    return mln_string_strcmp(lk1->keyword, lk2->keyword);
}

mln_lex_t *mln_lex_init(struct mln_lex_attr *attr)
{
    mln_lex_macro_t *lm;
    struct mln_rbtree_attr rbattr;
    mln_rbtree_node_t *rn;
    struct mln_stack_attr sattr;
    mln_string_t k1 = mln_string("1");
    mln_string_t k2 = mln_string("true");
    mln_string_t *scan;
    mln_lex_keyword_t *newkw;
    sattr.free_handler = mln_lex_input_free;
    sattr.copy_handler = NULL;
    mln_lex_t *lex;
    if ((lex = (mln_lex_t *)mln_alloc_m(attr->pool, sizeof(mln_lex_t))) == NULL) {
        return NULL;
    }
    lex->pool = attr->pool;

    rbattr.cmp = mln_lex_macro_cmp;
    rbattr.data_free = mln_lex_macro_free;
    if ((lex->macros = mln_rbtree_init(&rbattr)) == NULL) {
        mln_alloc_free(lex);
        return NULL;
    }
    if ((lm = mln_lex_macro_new(lex->pool, &k1, NULL)) == NULL) {
err:
        mln_rbtree_destroy(lex->macros);
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
    if ((lex->stack = mln_stack_init(&sattr)) == NULL) {
        goto err;
    }
    if (attr->hooks != NULL)
        memcpy(&(lex->hooks), attr->hooks, sizeof(mln_lex_hooks_t));
    else
        memset(&(lex->hooks), 0, sizeof(mln_lex_hooks_t));

    if (attr->keywords != NULL) {
        rbattr.cmp = mln_lex_keywords_cmp;
        rbattr.data_free = mln_lex_keyword_free;
        if ((lex->keywords = mln_rbtree_init(&rbattr)) == NULL) {
            mln_stack_destroy(lex->stack);
            goto err;
        }
        for (scan = attr->keywords; scan->data != NULL; ++scan) {
            if ((newkw = mln_lex_keyword_new(scan, scan - attr->keywords)) == NULL) {
                 mln_rbtree_destroy(lex->keywords);
                 mln_stack_destroy(lex->stack);
                 goto err;
            }
            if ((rn = mln_rbtree_node_new(lex->keywords, newkw)) == NULL) {
                mln_lex_keyword_free(newkw);
                mln_rbtree_destroy(lex->keywords);
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

    if (attr->data != NULL) {
        if (attr->type == M_INPUT_T_BUF) {
            if (mln_lex_push_InputBufStream(lex, attr->data) < 0) {
                mln_lex_destroy(lex);
                return NULL;
            }
        } else if (attr->type == M_INPUT_T_FILE) {
            if (mln_lex_push_InputFileStream(lex, attr->data) < 0) {
                mln_lex_destroy(lex);
                return NULL;
            }
        } else {
            mln_lex_destroy(lex);
            return NULL;
        }
    }

    return lex;
}

void mln_lex_destroy(mln_lex_t *lex)
{
    if (lex == NULL) return;
    if (lex->preprocess) {
        mln_lex_preprocessData_free((mln_lex_preprocessData_t *)(lex->hooks.at_data));
        lex->hooks.at_data = NULL;
    }
    if (lex->macros != NULL) mln_rbtree_destroy(lex->macros);
    if (lex->cur != NULL) mln_lex_input_free(lex->cur);
    if (lex->stack != NULL) mln_stack_destroy(lex->stack);
    if (lex->err_msg != NULL) mln_alloc_free(lex->err_msg);
    if (lex->result_buf != NULL) mln_alloc_free(lex->result_buf);
    if (lex->keywords != NULL) mln_rbtree_destroy(lex->keywords);
    mln_alloc_free(lex);
}

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
#if defined(i386) || defined(__arm__)
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

int mln_lex_push_InputFileStream(mln_lex_t *lex, mln_string_t *path)
{
    int err = MLN_LEX_SUCCEED;
    mln_lex_input_t *in = mln_lex_input_new(lex->pool, M_INPUT_T_FILE, path, &err, lex->line);
    if (in == NULL) {
        lex->error = err;
        return -1;
    }
    if (lex->cur != NULL) {
        if (mln_stack_push(lex->stack, lex->cur) < 0) {
            lex->error = MLN_LEX_ENMEM;
            return -1;
        }
        lex->cur = NULL;
    }
    if (mln_stack_push(lex->stack, in) < 0) {
        lex->error = MLN_LEX_ENMEM;
        return -1;
    }
    lex->line = 1;
    return 0;
}

int mln_lex_push_InputBufStream(mln_lex_t *lex, mln_string_t *buf)
{
    int err = MLN_LEX_SUCCEED;
    mln_lex_input_t *in = mln_lex_input_new(lex->pool, M_INPUT_T_BUF, buf, &err, lex->line);
    if (in == NULL) {
        lex->error = err;
        return -1;
    }
    if (lex->cur != NULL) {
        if (mln_stack_push(lex->stack, lex->cur) < 0) {
            lex->error = MLN_LEX_ENMEM;
            return -1;
        }
        lex->cur = NULL;
    }
    if (mln_stack_push(lex->stack, in) < 0) {
        lex->error = MLN_LEX_ENMEM;
        return -1;
    }
    lex->line = 1;
    return 0;
}

static int mln_lex_checkFileLoop_scanner(void *st_data, void *data)
{
    mln_string_t *path = (mln_string_t *)data;
    mln_lex_input_t *in = (mln_lex_input_t *)st_data;
    if (in->type != M_INPUT_T_FILE) return 0;
    if (!mln_string_strcmp(path, in->data)) return -1;
    return 0;
}

int mln_lex_checkFileLoop(mln_lex_t *lex, mln_string_t *path)
{
    if (lex->cur != NULL && !mln_string_strcmp(path, lex->cur->data)) {
        mln_lex_setError(lex, MLN_LEX_EINCLUDELOOP);
        return -1;
    }
    if (mln_stack_scan_all(lex->stack, mln_lex_checkFileLoop_scanner, path) < 0) {
        mln_lex_setError(lex, MLN_LEX_EINCLUDELOOP);
        return -1;
    }
    return 0;
}

int mln_lex_conditionTest(mln_lex_t *lex)
{
    mln_lex_cleanResult(lex);

    char c;
    int reverse = 0;
    mln_rbtree_node_t *rn;
    mln_lex_macro_t lm;
    mln_string_t tmp;

    while (1) {
        if ((c = mln_lex_getAChar(lex)) == MLN_ERR) {
            return -1;
        }
        if (c == ' ' || c == '\t') continue;
        if (c == '\n') {
            mln_lex_setError(lex, MLN_LEX_EINVEOL);
            return -1;
        }
        if (c == MLN_EOF) {
            mln_lex_setError(lex, MLN_LEX_EINVEOF);
            return -1;
        }
        mln_lex_stepBack(lex, c);
        break;
    }

    if ((c = mln_lex_getAChar(lex)) == MLN_ERR) {
        return -1;
    }
    if (c == '!') reverse = 1;
    else mln_lex_stepBack(lex, c);

    while (1) {
        if ((c = mln_lex_getAChar(lex)) == MLN_ERR) {
            return -1;
        }
        if (c == ' ' || c == '\t' || c == '\n' || c == MLN_EOF) {
            break;
        }
        if (!mln_lex_isLetter(c) && !isdigit(c)) {
            mln_lex_setError(lex, MLN_LEX_EINVCHAR);
            return -1;
        }
        if (mln_lex_putAChar(lex, c) == MLN_ERR) {
            return -1;
        }
    }

    mln_lex_getResult(lex, &tmp);
    lm.key = &tmp;
    lm.val = NULL;
    rn = mln_rbtree_search(lex->macros, lex->macros->root, &lm);
    mln_lex_cleanResult(lex);
    if (mln_rbtree_null(rn, lex->macros)) {
        return reverse? 1: 0;
    }
    return reverse? 0: 1;
}

