
/*
 * Copyright (C) Niklaus F.Schen.
 */

#include "mln_event.h"
#include "mln_conf.h"
#include "mln_log.h"
#include "mln_func.h"
#include "mln_path.h"
#include <stdlib.h>
#include <string.h>

#define CONF_ERR(lex,TK,MSG); \
{\
    mln_string_t *path = mln_lex_get_cur_filename(lex);\
    fprintf(stderr, "Configuration error. ");\
    if (path != NULL) {\
        fprintf(stderr, "%s:", (char *)(path->data));\
    }\
    fprintf(stderr, "%d: \"%s\" %s.\n", (TK)->line, (char *)((TK)->text->data), MSG);\
}

#define CONF_HASH_LEN 31
#define CONF_ITEM_STACK_SIZE 16

static mln_conf_hook_t *g_conf_hook_head = NULL, *g_conf_hook_tail = NULL;
static mln_conf_t *g_conf = NULL;
static char default_domain[] = "main";
static mln_string_t conf_keywords[] = {
    mln_string("on"),
    mln_string("off"),
    mln_string(NULL)
};
static mln_string_t mln_conf_env = mln_string("MELON_CONF_PATH");


MLN_DEFINE_TOKEN_TYPE_AND_STRUCT(static, mln_conf_lex, CONF, \
                                 CONF_TK_ON, \
                                 CONF_TK_OFF, \
                                 CONF_TK_COMMENT, \
                                 CONF_TK_CHAR, \
                                 CONF_TK_STRING);
MLN_DEFINE_TOKEN(static, mln_conf_lex, CONF, \
                 {CONF_TK_ON, "CONF_TK_ON"}, \
                 {CONF_TK_OFF, "CONF_TK_OFF"}, \
                 {CONF_TK_COMMENT, "CONF_TK_COMMENT"}, \
                 {CONF_TK_CHAR, "CONF_TK_CHAR"}, \
                 {CONF_TK_STRING, "CONF_TK_STRING"});

MLN_CHAIN_FUNC_DECLARE(static inline, \
                       conf_hook, \
                       mln_conf_hook_t, );

/*
 * declarations
 */
/*hash functions*/
static mln_u64_t mln_conf_hash_calc(mln_hash_t *h, void *key);
static int mln_conf_hash_cmp(mln_hash_t *h, void *key1, void *key2);
static void mln_conf_domain_val_free(void *data);
static void mln_conf_cmd_val_free(void *data);
/*for lex*/
static mln_conf_lex_struct_t *
mln_conf_token(mln_lex_t *lex) __NONNULL1(1);
static inline int
mln_get_char(mln_lex_t *lex, char c) __NONNULL1(1);
/*for mln_conf_t*/
static mln_conf_t *mln_conf_init(void);
static void
mln_conf_destroy(mln_conf_t *cf);
static inline void
mln_conf_destroy_lex(mln_conf_t *cf);
/*for mln_conf_domain_t*/
static mln_conf_domain_t *
mln_conf_domain_insert(mln_conf_t *cf, char *domain_name);
static void
mln_conf_domain_remove(mln_conf_t *cf, char *domain_name);
static mln_conf_domain_t *
mln_conf_domain_init(mln_conf_t *cf, mln_string_t *domain_name) __NONNULL2(1,2);
static mln_conf_domain_t *
mln_conf_domain_search(mln_conf_t *cf, char *domain_name) __NONNULL2(1,2);
/*for mln_conf_cmd_t*/
static mln_conf_cmd_t *
mln_conf_cmd_init(mln_string_t *cmd_name) __NONNULL1(1);
static mln_conf_cmd_t *
mln_conf_cmd_search(mln_conf_domain_t *cd, char *cmd_name) __NONNULL2(1,2);
static mln_conf_cmd_t *
mln_conf_cmd_insert(mln_conf_domain_t *cd, char *cmd_name);
static void
mln_conf_cmd_remove(mln_conf_domain_t *cd, char *cmd_name);
/*for mln_conf_item_t*/
static int mln_isvalid_item(mln_conf_lex_struct_t *cls) __NONNULL1(1);
static int
mln_conf_item_init(mln_conf_t *cf, mln_conf_lex_struct_t *cls, mln_conf_item_t *ci) __NONNULL2(1,2);
static int _mln_conf_load(mln_conf_t *cf, mln_conf_domain_t *current) __NONNULL2(1,2);
static mln_conf_item_t *
mln_conf_item_search(mln_conf_cmd_t *cmd, mln_u32_t index) __NONNULL1(1);
static int
mln_conf_item_update(mln_conf_cmd_t *cmd, mln_conf_item_t *items, mln_u32_t nitems);
static int mln_conf_item_collect(mln_conf_t *cf, mln_conf_cmd_t *cc, mln_conf_lex_struct_t *first);
static int mln_conf_cmds_iterate_handler(mln_hash_t *h, void *key, void *val, void *udata);
static int mln_conf_dump_conf_iterate_handler(mln_hash_t *h, void *key, void *val, void *udata);
static int mln_conf_dump_domain_iterate_handler(mln_hash_t *h, void *key, void *val, void *udata);
/*for hook*/
static mln_conf_hook_t *mln_conf_hook_init(void);
static void mln_conf_hook_destroy(mln_conf_hook_t *ch);
#if !defined(MSVC)
static void mln_conf_reload_master_handler(mln_event_t *ev, void *f_ptr, void *buf, mln_u32_t len, void **udata_ptr);
static void mln_conf_reload_worker_handler(mln_event_t *ev, void *f_ptr, void *buf, mln_u32_t len, void **udata_ptr);
#endif


/*
 * tool functions for lexer
 */
MLN_FUNC(static, mln_conf_lex_struct_t *, mln_conf_lex_sglq_handler, (mln_lex_t *lex, void *data), (lex, data), {
    mln_lex_result_clean(lex);
    char c = mln_lex_getchar(lex);
    if (c == MLN_ERR) return NULL;
    if (!mln_isascii(c) || c == '\'') {
        mln_lex_error_set(lex, MLN_LEX_EINVCHAR);
        return NULL;
    }
    if (mln_get_char(lex, c) < 0) return NULL;
    if ((c = mln_lex_getchar(lex)) == MLN_ERR) return NULL;
    if (c != '\'') {
        mln_lex_error_set(lex, MLN_LEX_EINVCHAR);
        return NULL;
    }
    return mln_conf_lex_new(lex, CONF_TK_CHAR);
})

static inline int mln_get_char(mln_lex_t *lex, char c)
{
    if (c == '\\') {
        char n;
        if ((n = mln_lex_getchar(lex)) == MLN_ERR) return -1;
        switch ( n ) {
            case '\"':
                if (mln_lex_putchar(lex, n) == MLN_ERR) return -1;
                break;
            case '\'':
                if (mln_lex_putchar(lex, n) == MLN_ERR) return -1;
                break;
            case 'n':
                if (mln_lex_putchar(lex, '\n') == MLN_ERR) return -1;
                break;
            case 't':
                if (mln_lex_putchar(lex, '\t') == MLN_ERR) return -1;
                break;
            case 'b':
                if (mln_lex_putchar(lex, '\b') == MLN_ERR) return -1;
                break;
            case 'a':
                if (mln_lex_putchar(lex, '\a') == MLN_ERR) return -1;
                break;
            case 'f':
                if (mln_lex_putchar(lex, '\f') == MLN_ERR) return -1;
                break;
            case 'r':
                if (mln_lex_putchar(lex, '\r') == MLN_ERR) return -1;
                break;
#if !defined(MSVC)
            case 'e':
                if (mln_lex_putchar(lex, '\e') == MLN_ERR) return -1;
                break;
#endif
            case 'v':
                if (mln_lex_putchar(lex, '\v') == MLN_ERR) return -1;
                break;
            case '\\':
                if (mln_lex_putchar(lex, '\\') == MLN_ERR) return -1;
                break;
            default:
                mln_lex_error_set(lex, MLN_LEX_EINVCHAR);
                return -1;
        }
    } else {
        if (mln_lex_putchar(lex, c) == MLN_ERR) return -1;
    }
    return 0;
}

MLN_FUNC(static, mln_conf_lex_struct_t *, mln_conf_lex_dblq_handler, (mln_lex_t *lex, void *data), (lex, data), {
    mln_lex_result_clean(lex);
    char c;
    while ( 1 ) {
        c = mln_lex_getchar(lex);
        if (c == MLN_ERR) return NULL;
        if (c == MLN_EOF) {
            mln_lex_error_set(lex, MLN_LEX_EINVEOF);
            return NULL;
        }
        if (c == '\"') break;
        if (mln_get_char(lex, c) < 0) return NULL;
    }
    return mln_conf_lex_new(lex, CONF_TK_STRING);
})

MLN_FUNC(static, mln_conf_lex_struct_t *, mln_conf_lex_slash_handler, (mln_lex_t *lex, void *data), (lex, data), {
    char c = mln_lex_getchar(lex);
    if (c == MLN_ERR) return NULL;
    if (c == '*') {
        if (mln_lex_putchar(lex, c) == MLN_ERR) return NULL;
        while ( 1 ) {
            c = mln_lex_getchar(lex);
            if (c == MLN_ERR) return NULL;
            if (c == MLN_EOF) {
                mln_lex_stepback(lex, c);
                break;
            }
            if (c == '\n') ++(lex->line);
            if (c == '*') {
                if (mln_lex_putchar(lex, c) == MLN_ERR) return NULL;
                c = mln_lex_getchar(lex);
                if (c == MLN_ERR) return NULL;
                if (c == MLN_EOF) {
                    mln_lex_stepback(lex, c);
                    break;
                }
                if (c == '\n') ++(lex->line);
                if (c == '/') {
                    if (mln_lex_putchar(lex, c) == MLN_ERR) return NULL;
                    break;
                }
            }
            if (mln_lex_putchar(lex, c) == MLN_ERR) return NULL;
        }
    } else if (c == '/') {
        if (mln_lex_putchar(lex, c) == MLN_ERR) return NULL;
        while ( 1 ) {
            c = mln_lex_getchar(lex);
            if (c == MLN_ERR) return NULL;
            if (c == MLN_EOF) {
                mln_lex_stepback(lex, c);
                break;
            }
            if (c == '\n') {
                mln_lex_stepback(lex, c);
                break;
            }
            if (mln_lex_putchar(lex, c) == MLN_ERR) return NULL;
        }
    } else {
        mln_lex_stepback(lex, c);
        return mln_conf_lex_new(lex, CONF_TK_SLASH);
    }
    return mln_conf_lex_new(lex, CONF_TK_COMMENT);
})

MLN_FUNC(static, mln_conf_lex_struct_t *, mln_conf_token, (mln_lex_t *lex), (lex), {
    mln_conf_lex_struct_t *clst;
    int sub_mark = 0;
    while ( 1 ) {
        clst = mln_conf_lex_token(lex);
        if (clst == NULL) {
            if (sub_mark) {
                mln_lex_error_set(lex, MLN_LEX_EINVCHAR);
                return NULL;
            }
            break;
        }
        if (clst->type == CONF_TK_COMMENT) {
            mln_conf_lex_free(clst);
            if (sub_mark) {
                mln_lex_error_set(lex, MLN_LEX_EINVCHAR);
                return NULL;
            }
            continue;
        }
        if (clst->type == CONF_TK_SUB) {
            mln_conf_lex_free(clst);
            if (sub_mark) {
                mln_lex_error_set(lex, MLN_LEX_EINVCHAR);
                return NULL;
            }
            sub_mark = 1;
            continue;
        }
        if (clst->type == CONF_TK_DEC || clst->type == CONF_TK_REAL) {
            if (!sub_mark) break;
            mln_u64_t len = clst->text->len + 2;
            mln_u8ptr_t s = (mln_u8ptr_t)mln_alloc_m(lex->pool, len);
            if (s == NULL) {
                mln_lex_error_set(lex, MLN_LEX_ENMEM);
                mln_conf_lex_free(clst);
                return NULL;
            }
            s[0] = '-';
            memcpy(s+1, clst->text->data, clst->text->len);
            s[len-1] = 0;
            mln_alloc_free(clst->text->data);
            clst->text->data = s;
            break;
        } else {
            if (!sub_mark) break;
            mln_lex_error_set(lex, MLN_LEX_EINVCHAR);
            mln_conf_lex_free(clst);
            return NULL;
        }
    }
    return clst;
})


/*
 * hash functions
 */
MLN_FUNC(static, mln_u64_t, mln_conf_hash_calc, (mln_hash_t *h, void *key), (h, key), {
    char *s = (char *)key;
    mln_u64_t hash = 5381;
    while (*s)
        hash = ((hash << 5) + hash) + (unsigned char)*s++;
    return hash % h->len;
})

MLN_FUNC(static, int, mln_conf_hash_cmp, (mln_hash_t *h, void *key1, void *key2), (h, key1, key2), {
    (void)h;
    return !strcmp((char *)key1, (char *)key2);
})


MLN_FUNC_VOID(static, void, mln_conf_domain_val_free, (void *data), (data), {
    if (data == NULL) return;
    mln_conf_domain_t *cd = (mln_conf_domain_t *)data;
    if (cd->domain_name != NULL) {
        mln_string_free(cd->domain_name);
        cd->domain_name = NULL;
    }
    if (cd->cmd != NULL) {
        mln_hash_free(cd->cmd, M_HASH_F_VAL);
        cd->cmd = NULL;
    }
    free(cd);
})

MLN_FUNC_VOID(static, void, mln_conf_cmd_val_free, (void *data), (data), {
    if (data == NULL) return;
    mln_conf_cmd_t *cc = (mln_conf_cmd_t *)data;
    if (cc->cmd_name != NULL) {
        mln_string_free(cc->cmd_name);
        cc->cmd_name = NULL;
    }
    if (cc->arg_tbl != NULL) {
        mln_u32_t i;
        mln_conf_item_t *ci;
        for (i = 0; i < cc->n_args; ++i) {
            ci = &(cc->arg_tbl[i]);
            if (ci->type == CONF_NONE) continue;
            if (ci->type == CONF_STR) {
                mln_string_free(ci->val.s);
                ci->val.s = NULL;
            }
        }
        free(cc->arg_tbl);
        cc->arg_tbl = NULL;
        cc->n_args = 0;
    }
    free(cc);
})

/*mln_conf_t*/
static inline mln_conf_t *mln_conf_init(void)
{
    mln_conf_t *cf;
    cf = (mln_conf_t *)malloc(sizeof(mln_conf_t));
    if (cf == NULL) {
        fprintf(stderr, "%s:%d: No memory.\n", __FUNCTION__, __LINE__);
        return NULL;
    }
    cf->search = mln_conf_domain_search;
    cf->insert = mln_conf_domain_insert;
    cf->remove = (mln_conf_domain_cb_t)mln_conf_domain_remove;

    cf->domain = mln_hash_new_fast(mln_conf_hash_calc, mln_conf_hash_cmp, NULL, mln_conf_domain_val_free, CONF_HASH_LEN, 0, 0);
    if (cf->domain == NULL) {
        fprintf(stderr, "%s:%d: No memory.\n", __FUNCTION__, __LINE__);
        free(cf);
        return NULL;
    }

    mln_alloc_t *pool;
    mln_string_t path;
    char *conf_file_path;
    mln_size_t path_len = strlen(mln_path_conf());
    mln_lex_hooks_t hooks;
    struct mln_lex_attr lattr;

    if ((pool = mln_alloc_init(NULL, 0)) == NULL) {
        fprintf(stderr, "%s:%d: No memory.\n", __FUNCTION__, __LINE__);
        mln_hash_free(cf->domain, M_HASH_F_VAL);
        free(cf);
        return NULL;
    }
    if ((conf_file_path = (char *)mln_alloc_m(pool, path_len + 1)) == NULL) {
        fprintf(stderr, "%s:%d: No memory.\n", __FUNCTION__, __LINE__);
        mln_alloc_destroy(pool);
        mln_hash_free(cf->domain, M_HASH_F_VAL);
        free(cf);
        return NULL;
    }
    memcpy(conf_file_path, mln_path_conf(), path_len);
    conf_file_path[path_len] = '\0';
    mln_string_nset(&path, conf_file_path, path_len);

#if defined(MSVC)
    if (!_access(conf_file_path, 0)) {
#else
    if (!access(conf_file_path, F_OK)) {
#endif
        lattr.pool = pool;
        lattr.keywords = conf_keywords;
        memset(&hooks, 0, sizeof(hooks));
        hooks.slash_handler = (lex_hook)mln_conf_lex_slash_handler;
        hooks.sglq_handler = (lex_hook)mln_conf_lex_sglq_handler;
        hooks.dblq_handler = (lex_hook)mln_conf_lex_dblq_handler;
        lattr.hooks = &hooks;
        lattr.preprocess = 1;
        lattr.type = M_INPUT_T_FILE;
        lattr.data = &path;
        lattr.env = &mln_conf_env;
        mln_lex_init_with_hooks(mln_conf_lex, cf->lex, &lattr);
        mln_alloc_free(conf_file_path);
        if (cf->lex == NULL) {
            fprintf(stderr, "%s:%d: No memory.\n", __FUNCTION__, __LINE__);
            mln_alloc_destroy(pool);
            mln_hash_free(cf->domain, M_HASH_F_VAL);
            free(cf);
            return NULL;
        }
    } else {
        fprintf(stderr, "[Warn] Configuration file [%s] not found, default configuration will be used.\n", conf_file_path);
        cf->lex = NULL;
    }

    if (cf->insert(cf, default_domain) == NULL) {
        fprintf(stderr, "%s:%d: No memory.\n", __FUNCTION__, __LINE__);
        mln_conf_destroy(cf);
        return NULL;
    }
#if !defined(MSVC)
    cf->cb = NULL;
#endif
    return cf;
}

static void mln_conf_destroy(mln_conf_t *cf)
{
    if (cf == NULL) return;
    mln_conf_destroy_lex(cf);
    if (cf->domain != NULL) {
        mln_hash_free(cf->domain, M_HASH_F_VAL);
        cf->domain = NULL;
    }
#if !defined(MSVC)
    if (cf->cb != NULL) mln_ipc_handler_unregister(cf->cb);
#endif
    free(cf);
}

MLN_FUNC_VOID(static inline, void, mln_conf_destroy_lex, (mln_conf_t *cf), (cf), {
    if (cf == NULL) return;
    if (cf->lex != NULL) {
        mln_alloc_t *pool = mln_lex_get_pool(cf->lex);
        mln_lex_destroy(cf->lex);
        mln_alloc_destroy(pool);
        cf->lex = NULL;
    }
})

/*mln_conf_domain_t*/
MLN_FUNC(static, mln_conf_domain_t *, mln_conf_domain_init, \
         (mln_conf_t *cf, mln_string_t *domain_name), \
         (cf, domain_name), \
{
    (void)cf;
    mln_conf_domain_t *cd;
    cd = (mln_conf_domain_t *)malloc(sizeof(mln_conf_domain_t));
    if (cd == NULL) return NULL;
    cd->search = mln_conf_cmd_search;
    cd->insert = mln_conf_cmd_insert;
    cd->remove = (mln_conf_cmd_cb_t)mln_conf_cmd_remove;
    cd->domain_name = mln_string_dup(domain_name);
    if (cd->domain_name == NULL) {
        free(cd);
        return NULL;
    }
    cd->cmd = mln_hash_new_fast(mln_conf_hash_calc, mln_conf_hash_cmp, NULL, mln_conf_cmd_val_free, CONF_HASH_LEN, 0, 0);
    if (cd->cmd == NULL) {
        mln_string_free(cd->domain_name);
        free(cd);
        return NULL;
    }
    return cd;
})

MLN_FUNC(static, mln_conf_domain_t *, mln_conf_domain_search, (mln_conf_t *cf, char *domain_name), (cf, domain_name), {
    return (mln_conf_domain_t *)mln_hash_search(cf->domain, domain_name);
})

MLN_FUNC(static, mln_conf_domain_t *, mln_conf_domain_insert, (mln_conf_t *cf, char *domain_name), (cf, domain_name), {
    mln_string_t name;

    mln_string_set(&name, domain_name);
    mln_conf_domain_t *cd = mln_conf_domain_init(cf, &name);
    if (cd == NULL) {
        return NULL;
    }
    if (mln_hash_insert(cf->domain, cd->domain_name->data, cd) < 0) {
        mln_conf_domain_val_free((void *)cd);
        return NULL;
    }

    return cd;
})

MLN_FUNC_VOID(static, void, mln_conf_domain_remove, (mln_conf_t *cf, char *domain_name), (cf, domain_name), {
    mln_hash_remove(cf->domain, domain_name, M_HASH_F_VAL);
})

/*mln_conf_cmd_t*/
MLN_FUNC(static, mln_conf_cmd_t *, mln_conf_cmd_init, (mln_string_t *cmd_name), (cmd_name), {
    mln_conf_cmd_t *cc;
    cc = (mln_conf_cmd_t *)malloc(sizeof(mln_conf_cmd_t));
    if (cc == NULL) return NULL;
    cc->cmd_name = mln_string_dup(cmd_name);
    if (cc->cmd_name == NULL) {
        free(cc);
        return NULL;
    }
    cc->search = mln_conf_item_search;
    cc->update = mln_conf_item_update;
    cc->arg_tbl = NULL;
    cc->n_args = 0;
    return cc;
})

MLN_FUNC(static, mln_conf_cmd_t *, mln_conf_cmd_search, (mln_conf_domain_t *cd, char *cmd_name), (cd, cmd_name), {
    return (mln_conf_cmd_t *)mln_hash_search(cd->cmd, cmd_name);
})

MLN_FUNC(static, mln_conf_cmd_t *, mln_conf_cmd_insert, \
         (mln_conf_domain_t *cd, char *cmd_name), (cd, cmd_name), \
{
    mln_conf_cmd_t *cmd;
    mln_string_t cname;

    mln_string_set(&cname, cmd_name);

    cmd = mln_conf_cmd_init(&cname);
    if (cmd == NULL) {
        return NULL;
    }
    if (mln_hash_insert(cd->cmd, cmd->cmd_name->data, cmd) < 0) {
        mln_conf_cmd_val_free(cmd);
        return NULL;
    }

    return cmd;
})

MLN_FUNC_VOID(static, void, mln_conf_cmd_remove, (mln_conf_domain_t *cd, char *cmd_name), (cd, cmd_name), {
    mln_hash_remove(cd->cmd, cmd_name, M_HASH_F_VAL);
})

/*mln_conf_item_t*/
MLN_FUNC(static, int, mln_isvalid_item, (mln_conf_lex_struct_t *cls), (cls), {
    switch( cls->type ) {
        case CONF_TK_DEC:
        case CONF_TK_REAL:
        case CONF_TK_STRING:
        case CONF_TK_ON:
        case CONF_TK_OFF:
        case CONF_TK_CHAR:
            break;
        default: return 0;
    }
    return 1;
})

MLN_FUNC(static, int, mln_conf_item_init, \
         (mln_conf_t *cf, mln_conf_lex_struct_t *cls, mln_conf_item_t *ci), \
         (cf, cls, ci), \
{
    if (!mln_isvalid_item(cls)) {
        CONF_ERR(cf->lex, cls, "Invalid type of item");
        return -1;
    }
    switch (cls->type) {
        case CONF_TK_DEC:
            ci->type = CONF_INT;
            ci->val.i = atol((char *)(cls->text->data));
            break;
        case CONF_TK_REAL:
            ci->type = CONF_FLOAT;
            ci->val.f = atof((char *)(cls->text->data));
            break;
        case CONF_TK_STRING:
            ci->type = CONF_STR;
            ci->val.s = mln_string_dup(cls->text);
            if (ci->val.s == NULL) {
                fprintf(stderr, "%s:%d: No memory.\n", __FUNCTION__, __LINE__);
                return -1;
            }
            break;
        case CONF_TK_ON:
            ci->type = CONF_BOOL;
            ci->val.b = 1;
            break;
        case CONF_TK_OFF:
            ci->type = CONF_BOOL;
            ci->val.b = 0;
            break;
        case CONF_TK_CHAR:
            ci->type = CONF_CHAR;
            ci->val.c = (mln_s8_t)(cls->text->data[0]);
            break;
        default:
            fprintf(stderr, "%s:%d: No such token type.\n", __FUNCTION__, __LINE__);
            abort();
    }
    return 0;
})

MLN_FUNC(static, mln_conf_item_t *, mln_conf_item_search, (mln_conf_cmd_t *cmd, mln_u32_t index), (cmd, index), {
    if (!index || index > cmd->n_args) return NULL;
    return &(cmd->arg_tbl[index-1]);
})

MLN_FUNC(static, int, mln_conf_item_update, \
         (mln_conf_cmd_t *cmd, mln_conf_item_t *items, mln_u32_t nitems), \
         (cmd, items, nitems), \
{
    mln_u32_t i, j;
    mln_conf_item_t *args;
    mln_conf_item_t *ci;

    if ((args = (mln_conf_item_t *)malloc(sizeof(mln_conf_item_t) * nitems)) == NULL) {
        return -1;
    }
    for (i = 0; i < nitems; ++i) {
        args[i].type = items[i].type;
        if (items[i].type == CONF_STR) {
            if ((args[i].val.s = mln_string_dup(items[i].val.s)) == NULL) {
                for (j = 0; j < i; ++j) {
                    if (args[j].type == CONF_STR && args[j].val.s != NULL)
                        mln_string_free(args[j].val.s);
                }
                free(args);
                return -1;
            }
        } else {
            args[i].val = items[i].val;
        }
    }

    if (cmd->arg_tbl != NULL) {
        for (i = 0; i < cmd->n_args; ++i) {
            ci = &(cmd->arg_tbl[i]);
            if (ci->type == CONF_NONE) continue;
            if (ci->type == CONF_STR) {
                mln_string_free(ci->val.s);
                ci->val.s = NULL;
            }
        }
        free(cmd->arg_tbl);
    }
    cmd->arg_tbl = args;
    cmd->n_args = nitems;
    return 0;
})

/*
 * load and free configurations
 */
MLN_FUNC(static, int, mln_conf_item_collect, \
         (mln_conf_t *cf, mln_conf_cmd_t *cc, mln_conf_lex_struct_t *first), \
         (cf, cc, first), \
{
    mln_conf_lex_struct_t *stack_buf[CONF_ITEM_STACK_SIZE];
    mln_conf_lex_struct_t **tokens = stack_buf;
    mln_u32_t cnt = 0, cap = CONF_ITEM_STACK_SIZE;
    mln_conf_lex_struct_t *cls = first;
    mln_u32_t i;

    while (1) {
        if (cls == NULL) {
            fprintf(stderr, "%s:%d: Get token error. %s\n", __FUNCTION__, __LINE__, mln_lex_strerror(cf->lex));
            goto err;
        }
        if (cls->type == CONF_TK_EOF) {
            CONF_ERR(cf->lex, cls, "Invalid end of file");
            mln_conf_lex_free(cls);
            goto err;
        }
        if (cls->type == CONF_TK_SEMIC) {
            mln_conf_lex_free(cls);
            break;
        }
        if (cnt >= cap) {
            mln_u32_t new_cap = cap << 1;
            mln_conf_lex_struct_t **new_tokens;
            if (tokens == stack_buf) {
                new_tokens = (mln_conf_lex_struct_t **)malloc(new_cap * sizeof(mln_conf_lex_struct_t *));
                if (new_tokens == NULL) { mln_conf_lex_free(cls); goto err; }
                memcpy(new_tokens, stack_buf, cnt * sizeof(mln_conf_lex_struct_t *));
            } else {
                new_tokens = (mln_conf_lex_struct_t **)realloc(tokens, new_cap * sizeof(mln_conf_lex_struct_t *));
                if (new_tokens == NULL) { mln_conf_lex_free(cls); goto err; }
            }
            tokens = new_tokens;
            cap = new_cap;
        }
        tokens[cnt++] = cls;
        cls = mln_conf_token(cf->lex);
    }

    if (cnt == 0) {
        if (tokens != stack_buf) free(tokens);
        return 0;
    }

    cc->arg_tbl = (mln_conf_item_t *)calloc(cnt, sizeof(mln_conf_item_t));
    if (cc->arg_tbl == NULL) {
        fprintf(stderr, "%s:%d: No memory.\n", __FUNCTION__, __LINE__);
        goto err;
    }
    cc->n_args = cnt;

    for (i = 0; i < cnt; ++i) {
        if (mln_conf_item_init(cf, tokens[i], &cc->arg_tbl[i]) < 0) {
            mln_u32_t j;
            for (j = i; j < cnt; ++j)
                mln_conf_lex_free(tokens[j]);
            if (tokens != stack_buf) free(tokens);
            return -1;
        }
        mln_conf_lex_free(tokens[i]);
    }

    if (tokens != stack_buf) free(tokens);
    return 0;

err:
    for (i = 0; i < cnt; ++i)
        mln_conf_lex_free(tokens[i]);
    if (tokens != stack_buf) free(tokens);
    return -1;
})

MLN_FUNC(static, int, _mln_conf_load, (mln_conf_t *cf, mln_conf_domain_t *current), (cf, current), {
    mln_conf_lex_struct_t *fir, *next;
    mln_conf_cmd_t *cmd;
    mln_conf_domain_t *cd;
    mln_string_t dname;

    mln_string_set(&dname, default_domain);

    while ( 1 ) {
        fir = mln_conf_token(cf->lex);
        if (fir == NULL) {
            fprintf(stderr, "%s:%d: Get token error. %s\n", __FUNCTION__, __LINE__, mln_lex_strerror(cf->lex));
            return -1;
        } else if (fir->type == CONF_TK_EOF) {
            mln_conf_lex_free(fir);
            break;
        } else if (fir->type == CONF_TK_RBRACE) {
            if (mln_string_strcmp(current->domain_name, &dname)) {
                mln_conf_lex_free(fir);
                return 0;
            }
            CONF_ERR(cf->lex, fir, "Invalid right brace");
            mln_conf_lex_free(fir);
            return -1;
        } else if (fir->type != CONF_TK_ID) {
            CONF_ERR(cf->lex, fir, "Unexpected token");
            mln_conf_lex_free(fir);
            return -1;
        }
        next = mln_conf_token(cf->lex);
        if (next == NULL) {
            mln_conf_lex_free(fir);
            fprintf(stderr, "%s:%d: Get token error. %s\n", __FUNCTION__, __LINE__, mln_lex_strerror(cf->lex));
            return -1;
        }
        else if (next->type == CONF_TK_EOF) {
            CONF_ERR(cf->lex, next, "Invalid end of file");
            mln_conf_lex_free(fir);
            mln_conf_lex_free(next);
            return -1;
        }
        /*as domain*/
        if (next->type == CONF_TK_LBRACE) {
            mln_conf_lex_free(next);
            if (mln_string_strcmp(current->domain_name, &dname)) {
                CONF_ERR(cf->lex, fir, "Illegal domain");
                mln_conf_lex_free(fir);
                return -1;
            }

            cd = cf->insert(cf, (char *)(fir->text->data));
            mln_conf_lex_free(fir);
            if (cd == NULL) {
                fprintf(stderr, "%s:%d: No memory.\n", __FUNCTION__, __LINE__);
                return -1;
            }
            if (_mln_conf_load(cf, cd) < 0) return -1;
            continue;
        }
        /*as command*/
        cmd = current->insert(current, (char *)(fir->text->data));
        mln_conf_lex_free(fir);
        if (cmd == NULL) {
            fprintf(stderr, "%s:%d: No memory.\n", __FUNCTION__, __LINE__);
            mln_conf_lex_free(next);
            return -1;
        }

        if (mln_conf_item_collect(cf, cmd, next) < 0) return -1;
        /*
         * we don't need to free the pointer 'next' here,
         * because it has already freed in mln_conf_item_collect().
         */
    }
    return 0;
})

int mln_conf_load(void)
{
    mln_conf_domain_t *cd;

    if (g_conf != NULL) return 0;

    g_conf = mln_conf_init();

    if (g_conf == NULL) return -1;

    cd = (mln_conf_domain_t *)mln_hash_search(g_conf->domain, default_domain);
    if (g_conf->lex != NULL) {
        mln_s32_t ret = _mln_conf_load(g_conf, cd);
        mln_conf_destroy_lex(g_conf);
        if (ret < 0) {
            mln_conf_destroy(g_conf);
            g_conf = NULL;
            return -1;
        }
    }
#if !defined(MSVC)
    if ((g_conf->cb = mln_ipc_handler_register(M_IPC_TYPE_CONF, mln_conf_reload_master_handler, mln_conf_reload_worker_handler, NULL, NULL)) == NULL) {
        mln_conf_destroy(g_conf);
        g_conf = NULL;
        return -1;
    }
#endif
    return 0;
}

MLN_FUNC_VOID(, void, mln_conf_free, (void), (), {
    if (g_conf == NULL) return;
    mln_conf_destroy(g_conf);
    g_conf = NULL;
})

/*
 * hook
 */
MLN_FUNC(static, mln_conf_hook_t *, mln_conf_hook_init, (void), (), {
    mln_conf_hook_t *ch = (mln_conf_hook_t *)malloc(sizeof(mln_conf_hook_t));
    if (ch == NULL) return NULL;
    ch->reload = NULL;
    ch->data = NULL;
    ch->prev = NULL;
    ch->next = NULL;
    return ch;
})

MLN_FUNC_VOID(static, void, mln_conf_hook_destroy, (mln_conf_hook_t *ch), (ch), {
    if (ch == NULL) return;
    free(ch);
})

MLN_FUNC(, mln_conf_hook_t *, mln_conf_hook_set, (reload_handler reload, void *data), (reload, data), {
    mln_conf_hook_t *ch = mln_conf_hook_init();
    if (ch == NULL) return NULL;
    ch->reload = reload;
    ch->data = data;
    conf_hook_chain_add(&g_conf_hook_head, &g_conf_hook_tail, ch);
    return ch;
})

MLN_FUNC_VOID(, void, mln_conf_hook_unset, (mln_conf_hook_t *hook), (hook), {
    if (hook == NULL) return;
    conf_hook_chain_del(&g_conf_hook_head, &g_conf_hook_tail, hook);
    mln_conf_hook_destroy(hook);
})

MLN_FUNC_VOID(, void, mln_conf_hook_free, (void), (), {
    mln_conf_hook_t *ch;
    while ((ch = g_conf_hook_head) != NULL) {
        conf_hook_chain_del(&g_conf_hook_head, &g_conf_hook_tail, ch);
        mln_conf_hook_destroy(ch);
    }
})

/*
 * reload
 */
MLN_FUNC(, int, mln_conf_reload, (void), (), {
    mln_conf_free();
    mln_conf_load();
    mln_conf_hook_t *ch;
    for (ch = g_conf_hook_head; ch != NULL; ch = ch->next) {
        if (ch->reload != NULL && ch->reload(ch->data) < 0)
            return -1;
    }
    return 0;
})

/*
 * misc
 */
MLN_FUNC(, mln_conf_t *, mln_conf, (void), (), {
    return g_conf;
})

MLN_FUNC(, mln_u32_t, mln_conf_cmd_num, (mln_conf_t *cf, char *domain), (cf, domain), {
    mln_conf_domain_t *cd = cf->search(cf, domain);
    if (cd == NULL) return 0;
    return cd->cmd->nr_nodes;
})

struct conf_cmds_scan_s {
    mln_conf_cmd_t **cc;
    mln_u32_t        pos;
};

MLN_FUNC_VOID(, void, mln_conf_cmds, (mln_conf_t *cf, char *domain, mln_conf_cmd_t **v), (cf, domain, v), {
    mln_conf_domain_t *cd = cf->search(cf, domain);
    if (cd == NULL) return;
    struct conf_cmds_scan_s ccs;
    ccs.cc = v;
    ccs.pos = 0;
    if (mln_hash_iterate(cd->cmd, mln_conf_cmds_iterate_handler, (void *)&ccs) < 0) {
        mln_log(error, "Shouldn't be here.\n");
        abort();
    }
})

MLN_FUNC(static, int, mln_conf_cmds_iterate_handler, (mln_hash_t *h, void *key, void *val, void *udata), (h, key, val, udata), {
    (void)h;
    (void)key;
    struct conf_cmds_scan_s *ccs = (struct conf_cmds_scan_s *)udata;
    ccs->cc[(ccs->pos)++] = (mln_conf_cmd_t *)val;
    return 0;
})

MLN_FUNC(, mln_u32_t, mln_conf_arg_num, (mln_conf_cmd_t *cc), (cc), {
    return cc->n_args;
})

/*
 * dump
 */
MLN_FUNC_VOID(, void, mln_conf_dump, (void), (), {
    printf("CONFIGURATIONS:\n");
    mln_hash_iterate(g_conf->domain, mln_conf_dump_conf_iterate_handler, NULL);
})

MLN_FUNC(static, int, mln_conf_dump_conf_iterate_handler, (mln_hash_t *h, void *key, void *val, void *udata), (h, key, val, udata), {
    (void)h;
    (void)key;
    (void)udata;
    mln_conf_domain_t *cd = (mln_conf_domain_t *)val;
    printf("\tDOMAIN [%s]:\n", (char *)(cd->domain_name->data));
    mln_hash_iterate(cd->cmd, mln_conf_dump_domain_iterate_handler, NULL);
    return 0;
})

MLN_FUNC(static, int, mln_conf_dump_domain_iterate_handler, (mln_hash_t *h, void *key, void *val, void *udata), (h, key, val, udata), {
    (void)h;
    (void)key;
    (void)udata;
    mln_conf_cmd_t *cc = (mln_conf_cmd_t *)val;
    printf("\t\tCOMMAND [%s]:\n", (char *)(cc->cmd_name->data));
    mln_s32_t i;
    mln_conf_item_t *ci;
    for (i = 0; i < cc->n_args; ++i) {
        ci = &(cc->arg_tbl[i]);
        printf("\t\t\t");
        switch (ci->type) {
            case CONF_STR:
                printf("STRING [%s]\n", (char *)(ci->val.s->data));
                break;
            case CONF_CHAR:
                printf("CHAR [%c]\n", ci->val.c);
                break;
            case CONF_BOOL:
                printf("BOOL [%u]\n", ci->val.b);
                break;
            case CONF_INT:
                printf("INT [%ld]\n", ci->val.i);
                break;
            case CONF_FLOAT:
                printf("FLOAT [%f]\n", ci->val.f);
                break;
            default: break;
        }
    }
    return 0;
})

/*
 * chain
 */
MLN_CHAIN_FUNC_DEFINE(static inline, \
                      conf_hook, \
                      mln_conf_hook_t, \
                      prev, \
                      next);

/*
 * ipc handlers
 */
#if !defined(MSVC)
MLN_FUNC_VOID(static, void, mln_conf_reload_master_handler, \
              (mln_event_t *ev, void *f_ptr, void *buf, mln_u32_t len, void **udata_ptr), \
              (ev, f_ptr, buf, len, udata_ptr), \
{
    /*
     * do nothing.
     */
})

MLN_FUNC_VOID(static, void, mln_conf_reload_worker_handler, \
              (mln_event_t *ev, void *f_ptr, void *buf, mln_u32_t len, void **udata_ptr), \
              (ev, f_ptr, buf, len, udata_ptr), \
{
    if (mln_conf_reload() < 0) {
        mln_log(error, "mln_conf_reload() failed.\n");
        exit(1);
    }
})
#endif

