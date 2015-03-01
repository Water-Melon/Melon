
/*
 * Copyright (C) Niklaus F.Schen.
 */

#include "mln_conf.h"
#include "mln_log.h"
#include <stdlib.h>

#define CONF_ERR(TK,MSG); \
    mln_log(error, "Configuration error. %d: \"%s\" %s.\n", (TK)->line, (TK)->text->str, MSG);\

mln_conf_stack_t *cstack = NULL;
mln_string_t default_domain = {"main", 4, 1};
char conf_filename[] = "conf/melon.conf";
char *conf_keywords[] = {
    "on",
    "off",
    NULL
};


MLN_DEFINE_TOKEN_TYPE_AND_STRUCT(static, mln_conf_lex, CONF, \
                                 CONF_TK_ON, \
                                 CONF_TK_OFF, \
                                 CONF_TK_COMMENT, \
                                 CONF_TK_CHAR, \
                                 CONF_TK_STRING);
MLN_DEFINE_TOKEN(mln_conf_lex, CONF);

/*
 * declarations
 */
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
mln_conf_add_in_queue(mln_conf_stack_t *cs, mln_conf_t *cf) __NONNULL2(1,2);
static inline void
mln_conf_del_from_queue(mln_conf_stack_t *cs, mln_conf_t *cf) __NONNULL2(1,2);
static inline void
mln_conf_destroy_lex(mln_conf_t *cf);
/*for mln_conf_domain_t*/
static mln_conf_domain_t *
mln_conf_domain_init(mln_conf_t *cf, mln_string_t *domain_name) __NONNULL2(1,2);
static void
mln_conf_domain_destroy(void *data) __NONNULL1(1);
static int
mln_conf_domain_calc_hash(mln_hash_t *h, void *key) __NONNULL2(1,2);
static int
mln_conf_domain_cmp(mln_hash_t *h, void *key1, void *key2) __NONNULL3(1,2,3);
static void
mln_conf_key_free(void *key);
static mln_conf_domain_t *
mln_conf_search_domain(mln_conf_t *cf, char *domain_name) __NONNULL2(1,2);
/*for mln_conf_cmd_t*/
static mln_conf_cmd_t *
mln_conf_cmd_init(mln_string_t *cmd_name) __NONNULL1(1);
static void
mln_conf_cmd_destroy(void *data);
static int
mln_conf_cmd_calc_hash(mln_hash_t *h, void *key) __NONNULL2(1,2);
static int
mln_conf_cmd_cmp(mln_hash_t *h, void *key1, void *key2) __NONNULL3(1,2,3);
static mln_conf_cmd_t *
mln_conf_search_cmd(mln_conf_domain_t *cd, char *cmd_name) __NONNULL2(1,2);
/*for mln_conf_item_t*/
static int mln_isvalid_item(mln_conf_lex_struct_t *cls) __NONNULL1(1);
static int
mln_conf_item_init(mln_conf_lex_struct_t *cls, mln_conf_item_t *ci) __NONNULL2(1,2);
static int _mln_load_conf(mln_conf_t *cf, mln_conf_domain_t *current) __NONNULL2(1,2);
static mln_conf_item_t *
mln_conf_search_item(mln_conf_cmd_t *cmd, mln_u32_t index) __NONNULL1(1);


/*
 * tool functions for lexer
 */
static mln_conf_lex_struct_t *
mln_conf_lex_sglq_handler(mln_lex_t *lex, void *data)
{
    lex->result_buf[0] = 0;
    lex->result_cur_ptr = lex->result_buf;
    char c = mln_geta_char(lex);
    if (c == MLN_ERR) return NULL;
    if (!isascii(c) || c == '\'') {
        lex->error = MLN_LEX_EINVCHAR;
        return NULL;
    }
    if (mln_get_char(lex, c) < 0) return NULL;
    if ((c = mln_geta_char(lex)) == MLN_ERR) return NULL;
    if (c != '\'') {
        lex->error = MLN_LEX_EINVCHAR;
        return NULL;
    }
    return mln_conf_lex_new(lex, CONF_TK_CHAR);
}

static inline int
mln_get_char(mln_lex_t *lex, char c)
{
    if (c == '\\') {
        char n;
        if ((n = mln_geta_char(lex)) == MLN_ERR) return -1;
        switch ( n ) {
            case '\"':
                if (mln_puta_char(lex, n) == MLN_ERR) return -1;
                break;
            case '\'':
                if (mln_puta_char(lex, n) == MLN_ERR) return -1;
                break;
            case 'n':
                if (mln_puta_char(lex, '\n') == MLN_ERR) return -1;
                break;
            case 't':
                if (mln_puta_char(lex, '\t') == MLN_ERR) return -1;
                break;
            case 'b':
                if (mln_puta_char(lex, '\b') == MLN_ERR) return -1;
                break;
            case 'a':
                if (mln_puta_char(lex, '\a') == MLN_ERR) return -1;
                break;
            case 'f':
                if (mln_puta_char(lex, '\f') == MLN_ERR) return -1;
                break;
            case 'r':
                if (mln_puta_char(lex, '\r') == MLN_ERR) return -1;
                break;
            case 'v':
                if (mln_puta_char(lex, '\v') == MLN_ERR) return -1;
                break;
            case '\\':
                if (mln_puta_char(lex, '\\') == MLN_ERR) return -1;
                break;
            default:
                lex->error = MLN_LEX_EINVCHAR;
                return -1;
        }
    } else {
        if (mln_puta_char(lex, c) == MLN_ERR) return -1;
    }
    return 0;
}

static mln_conf_lex_struct_t *
mln_conf_lex_dblq_handler(mln_lex_t *lex, void *data)
{
    lex->result_buf[0] = 0;
    lex->result_cur_ptr = lex->result_buf;
    char c;
    while ( 1 ) {
        c = mln_geta_char(lex);
        if (c == MLN_ERR) return NULL;
        if (c == MLN_EOF) {
            lex->error = MLN_LEX_EINVEOF;
            return NULL;
        }
        if (c == '\"') break;
        if (mln_get_char(lex, c) < 0) return NULL;
    }
    return mln_conf_lex_new(lex, CONF_TK_STRING);
}

static mln_conf_lex_struct_t *
mln_conf_lex_slash_handler(mln_lex_t *lex, void *data)
{
    char c = mln_geta_char(lex);
    if (c == MLN_ERR) return NULL;
    if (c == '*') {
        if (mln_puta_char(lex, c) == MLN_ERR) return NULL;
        while ( 1 ) {
            c = mln_geta_char(lex);
            if (c == MLN_ERR) return NULL;
            if (c == MLN_EOF) {
                mln_step_back(lex);
                break;
            }
            if (c == '\n') lex->line++;
            if (c == '*') {
                if (mln_puta_char(lex, c) == MLN_ERR) return NULL;
                c = mln_geta_char(lex);
                if (c == MLN_ERR) return NULL;
                if (c == MLN_EOF) {
                    mln_step_back(lex);
                    break;
                }
                if (c == '\n') lex->line++;
                if (c == '/') {
                    if (mln_puta_char(lex, c) == MLN_ERR) return NULL;
                    break;
                }
            }
            if (mln_puta_char(lex, c) == MLN_ERR) return NULL;
        }
    } else if (c == '/') {
        if (mln_puta_char(lex, c) == MLN_ERR) return NULL;
        while ( 1 ) {
            c = mln_geta_char(lex);
            if (c == MLN_ERR) return NULL;
            if (c == MLN_EOF) {
                mln_step_back(lex);
                break;
            }
            if (c == '\n') {
                mln_step_back(lex);
                break;
            }
            if (mln_puta_char(lex, c) == MLN_ERR) return NULL;
        }
    } else {
        mln_step_back(lex);
        return mln_conf_lex_new(lex, CONF_TK_SLASH);
    }
    return mln_conf_lex_new(lex, CONF_TK_COMMENT);
}

static mln_conf_lex_struct_t *mln_conf_token(mln_lex_t *lex)
{
    mln_conf_lex_struct_t *clst;
    int sub_mark = 0;
    while ( 1 ) {
        clst = mln_conf_lex_token(lex);
        if (clst == NULL) {
            if (sub_mark) {
                lex->error = MLN_LEX_EINVCHAR;
                return NULL;
            }
            break;
        }
        if (clst->type == CONF_TK_COMMENT) {
            mln_conf_lex_free(clst);
            if (sub_mark) {
                lex->error = MLN_LEX_EINVCHAR;
                return NULL;
            }
            continue;
        }
        if (clst->type == CONF_TK_SUB) {
            mln_conf_lex_free(clst);
            if (sub_mark) {
                lex->error = MLN_LEX_EINVCHAR;
                return NULL;
            }
            sub_mark = 1;
            continue;
        }
        if (clst->type == CONF_TK_DEC || clst->type == CONF_TK_REAL) {
            if (!sub_mark) break;
            mln_s32_t len = clst->text->len + 2;
            mln_s8ptr_t s = (mln_s8ptr_t)malloc(len);
            if (s == NULL) {
                lex->error = MLN_LEX_ENMEM;
                mln_conf_lex_free(clst);
                return NULL;
            }
            s[0] = '-';
            memcpy(s+1, clst->text->str, clst->text->len);
            s[len-1] = 0;
            free(clst->text->str);
            clst->text->str = s;
            break;
        } else {
            if (!sub_mark) break;
            lex->error = MLN_LEX_EINVCHAR;
            mln_conf_lex_free(clst);
            return NULL;
        }
    }
    return clst;
}

/*
 * other tool functions
 */

    /*mln_conf_stack_t*/
int mln_conf_stack_init(void)
{
    cstack = (mln_conf_stack_t *)malloc(sizeof(mln_conf_stack_t));
    if (cstack == NULL) return -1;
    cstack->top_new = cstack->bottom_old = NULL;
    return 0;
}

void mln_conf_stack_destroy(void)
{
    if (cstack == NULL) return ;
    mln_conf_t *cf;
    while (cstack->top_new != NULL) {
        cf = cstack->top_new;
        mln_conf_del_from_queue(cstack, cf);
        mln_conf_destroy(cf);
    }
    cstack->top_new = cstack->bottom_old = NULL;
    free(cstack);
    cstack = NULL;
}

    /*mln_conf_t*/
static inline mln_conf_t *mln_conf_init(void)
{
    mln_conf_t *cf;
    cf = (mln_conf_t *)malloc(sizeof(mln_conf_t));
    if (cf == NULL) {
        mln_log(error, "No memory.\n");
        return NULL;
    }
    cf->search = mln_conf_search_domain;
    cf->prev = cf->next = NULL;
    cf->refer_cnt = 0;
    struct mln_hash_attr hattr;
    hattr.hash = mln_conf_domain_calc_hash;
    hattr.cmp = mln_conf_domain_cmp;
    hattr.free_key = mln_conf_key_free;
    hattr.free_val = mln_conf_domain_destroy;
    hattr.len_base = MLN_CONF_HASH_LEN;
    hattr.expandable = 1;
    cf->domain_hash_tbl = mln_hash_init(&hattr);
    if (cf->domain_hash_tbl == NULL) {
        mln_log(error, "No memory.\n");
        free(cf);
        return NULL;
    }
    mln_size_t path_len = strlen(mln_get_path());
    char *conf_file_path = malloc(path_len + sizeof(conf_filename) + 1);
    if (conf_file_path == NULL) {
        mln_log(error, "No memory.\n");
        mln_hash_destroy(cf->domain_hash_tbl, hash_free_key_val);
        free(cf);
        return NULL;
    }
    memcpy(conf_file_path, mln_get_path(), path_len);
    conf_file_path[path_len] = '/';
    memcpy(conf_file_path+path_len+1, conf_filename, sizeof(conf_filename));
    struct mln_lex_attr lattr;
    lattr.input_type = mln_lex_file;
    lattr.input.filename = conf_file_path;
    lattr.keywords = conf_keywords;
    mln_lex_hooks_t hooks;
    memset(&hooks, 0, sizeof(hooks));
    hooks.slash_handler = (lex_hook)mln_conf_lex_slash_handler;
    hooks.sglq_handler = (lex_hook)mln_conf_lex_sglq_handler;
    hooks.dblq_handler = (lex_hook)mln_conf_lex_dblq_handler;
    lattr.hooks = &hooks;
    MLN_LEX_INIT_WITH_HOOKS(mln_conf_lex, cf->lex, &lattr);
    free(conf_file_path);
    if (cf->lex == NULL) {
        mln_hash_destroy(cf->domain_hash_tbl, hash_free_key_val);
        free(cf);
        return NULL;
    }
    mln_conf_domain_t *cd = mln_conf_domain_init(cf, &default_domain);
    if (cd == NULL) {
        mln_log(error, "No memory.\n");
        mln_conf_destroy(cf);
        return NULL;
    }
    mln_string_t *main_domain = mln_dup_string(&default_domain);
    if (main_domain == NULL) {
        mln_log(error, "No memory.\n");
        mln_conf_destroy(cf);
        return NULL;
    }
    if (mln_hash_insert(cf->domain_hash_tbl, main_domain, cd) < 0) {
        mln_log(error, "No memory.\n");
        mln_free_string(main_domain);
        mln_conf_destroy(cf);
        return NULL;
    }
    return cf;
}

static void
mln_conf_destroy(mln_conf_t *cf)
{
    if (cf == NULL) return;
    if (cf->lex != NULL) {
        mln_lex_destroy(cf->lex);
        cf->lex = NULL;
    }
    if (cf->domain_hash_tbl != NULL) {
        mln_hash_destroy(cf->domain_hash_tbl, hash_free_key_val);
        cf->domain_hash_tbl = NULL;
    }
    cf->refer_cnt = 0;
    free(cf);
}

static inline void
mln_conf_destroy_lex(mln_conf_t *cf)
{
    if (cf == NULL) return;
    if (cf->lex != NULL) {
        mln_lex_destroy(cf->lex);
        cf->lex = NULL;
    }
}

static inline void
mln_conf_add_in_queue(mln_conf_stack_t *cs, mln_conf_t *cf)
{
    if (cs->top_new == NULL) {
        cs->top_new = cs->bottom_old = cf;
    } else {
        cs->top_new->prev = cf;
        cf->next = cs->top_new;
        cs->top_new = cf;
    }
}

static inline void
mln_conf_del_from_queue(mln_conf_stack_t *cs, mln_conf_t *cf)
{
    if (cs->top_new == cf) {
        if (cs->bottom_old == cf) {
            cs->top_new = cs->bottom_old = NULL;
        } else {
            cs->top_new = cf->next;
            cf->next->prev = NULL;
        }
    } else {
        if (cs->bottom_old == cf) {
            cs->bottom_old = cf->prev;
            cf->prev->next = NULL;
        } else {
            cf->prev->next = cf->next;
            cf->next->prev = cf->prev;
        }
    }
    cf->prev = cf->next = NULL;
}

    /*mln_conf_domain_t*/
static mln_conf_domain_t *
mln_conf_domain_init(mln_conf_t *cf, mln_string_t *domain_name)
{
    mln_conf_domain_t *cd;
    cd = (mln_conf_domain_t *)malloc(sizeof(mln_conf_domain_t));
    if (cd == NULL) return NULL;
    cd->search = mln_conf_search_cmd;
    cd->domain_name = mln_dup_string(domain_name);
    if (cd->domain_name == NULL) {
        free(cd);
        return NULL;
    }
    struct mln_hash_attr hattr;
    hattr.hash = mln_conf_cmd_calc_hash;
    hattr.cmp = mln_conf_cmd_cmp;
    hattr.free_key = mln_conf_key_free;
    hattr.free_val = mln_conf_cmd_destroy;
    hattr.len_base = MLN_CONF_HASH_LEN;
    hattr.expandable = 1;
    cd->cmd_hash_tbl = mln_hash_init(&hattr);
    if (cd->cmd_hash_tbl == NULL) {
        mln_free_string(cd->domain_name);
        free(cd);
        return NULL;
    }
    return cd;
}

static void
mln_conf_domain_destroy(void *data)
{
    if (data == NULL) return;
    mln_conf_domain_t *cd = (mln_conf_domain_t *)data;
    if (cd->domain_name != NULL) {
        mln_free_string(cd->domain_name);
        cd->domain_name = NULL;
    }
    if (cd->cmd_hash_tbl != NULL) {
        mln_hash_destroy(cd->cmd_hash_tbl, hash_free_key_val);
        cd->cmd_hash_tbl = NULL;
    }
    free(cd);
}

static int
mln_conf_domain_calc_hash(mln_hash_t *h, void *key)
{
    mln_string_t *s = (mln_string_t *)key;
    mln_s32_t i;
    mln_u32_t index = 0;
    char *p = s->str;
    for (i = 0; i<s->len; i++) {
        index += (p[i] * 65599);
    }
    return (int)(index % h->len);
}

static int
mln_conf_domain_cmp(mln_hash_t *h, void *key1, void *key2)
{
    return !mln_strcmp((mln_string_t *)key1, (mln_string_t *)key2);
}

static void
mln_conf_key_free(void *key)
{
    if (key == NULL) return;
    mln_free_string((mln_string_t *)key);
}

static mln_conf_domain_t *
mln_conf_search_domain(mln_conf_t *cf, char *domain_name)
{
    mln_string_t str;
    str.str = domain_name;
    str.len = strlen(domain_name);
    str.is_referred = 1;
    return mln_hash_search(cf->domain_hash_tbl, &str);
}

    /*mln_conf_cmd_t*/
static mln_conf_cmd_t *
mln_conf_cmd_init(mln_string_t *cmd_name)
{
    mln_conf_cmd_t *cc;
    cc = (mln_conf_cmd_t *)malloc(sizeof(mln_conf_cmd_t));
    if (cc == NULL) return NULL;
    cc->cmd_name = mln_dup_string(cmd_name);
    if (cc->cmd_name == NULL) {
        free(cc);
        return NULL;
    }
    cc->search = mln_conf_search_item;
    cc->arg_tbl = NULL;
    cc->n_args = 0;
    return cc;
}

static void
mln_conf_cmd_destroy(void *data)
{
    if (data == NULL) return ;
    mln_conf_cmd_t *cc = (mln_conf_cmd_t *)data;
    if (cc->cmd_name != NULL) {
        mln_free_string(cc->cmd_name);
        cc->cmd_name = NULL;
    }
    if (cc->arg_tbl != NULL) {
        mln_u32_t i;
        mln_conf_item_t *ci;
        for (i = 0; i<cc->n_args; i++) {
            ci = &(cc->arg_tbl[i]);
            if (ci->type == CONF_NONE) continue;
            if (ci->type == CONF_STR) {
                mln_free_string(ci->val.s);
                ci->val.s = NULL;
            }
        }
        free(cc->arg_tbl);
        cc->arg_tbl = NULL;
        cc->n_args = 0;
    }
    free(cc);
}

static int
mln_conf_cmd_calc_hash(mln_hash_t *h, void *key)
{
    mln_string_t *s = (mln_string_t *)key;
    mln_s32_t i;
    mln_u32_t index = 0;
    char *p = s->str;
    for (i = 0; i<s->len; i++) {
        index += (p[i] * 65599);
    }
    return (int)(index % h->len);
}

static int
mln_conf_cmd_cmp(mln_hash_t *h, void *key1, void *key2)
{
    return !mln_strcmp((mln_string_t *)key1, (mln_string_t *)key2);
}

static mln_conf_cmd_t *
mln_conf_search_cmd(mln_conf_domain_t *cd, char *cmd_name)
{
    mln_string_t str;
    str.str = cmd_name;
    str.len = strlen(cmd_name);
    str.is_referred = 1;
    return mln_hash_search(cd->cmd_hash_tbl, &str);
}

    /*mln_conf_item_t*/
static int mln_isvalid_item(mln_conf_lex_struct_t *cls)
{
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
}

static int
mln_conf_item_init(mln_conf_lex_struct_t *cls, mln_conf_item_t *ci)
{
    if (!mln_isvalid_item(cls)) {
        CONF_ERR(cls, "Invalid type of item");
        return -1;
    }
    switch (cls->type) {
        case CONF_TK_DEC:
            ci->type = CONF_INT;
            ci->val.i = atol(cls->text->str);
            break;
        case CONF_TK_REAL:
            ci->type = CONF_FLOAT;
            ci->val.f = atof(cls->text->str);
            break;
        case CONF_TK_STRING:
            ci->type = CONF_STR;
            ci->val.s = mln_dup_string(cls->text);
            if (ci->val.s == NULL) {
                mln_log(error, "No memory.\n");
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
            ci->val.c = (mln_s8_t)(cls->text->str[0]);
            break;
        default:
            mln_log(error, "No such token type.\n");
            abort();
    }
    return 0;
}

static mln_conf_item_t *
mln_conf_search_item(mln_conf_cmd_t *cmd, mln_u32_t index)
{
    if (!index || index > cmd->n_args) return NULL;
    return &(cmd->arg_tbl[index-1]);
}

/*
 * load and free configurations
 */
static int
mln_conf_item_recursive(mln_conf_t *cf, \
                        mln_conf_cmd_t *cc, \
                        mln_conf_lex_struct_t *cls, \
                        mln_u32_t cnt)
{
    if (cls == NULL) {
        mln_log(error, "Get token error. %s\n", mln_lex_strerror(cf->lex));
        return -1;
    }
    if (cls->type == CONF_TK_EOF) {
        CONF_ERR(cls, "Invalid end of file");
        mln_conf_lex_free(cls);
        return -1;
    }
    if (cls->type == CONF_TK_SEMIC) {
        if (!cnt) {
            mln_conf_lex_free(cls);
            return 0;
        }
        cc->arg_tbl = (mln_conf_item_t *)calloc(cnt, sizeof(mln_conf_item_t));
        if (cc->arg_tbl == NULL) {
            mln_log(error, "No memory.\n");
            mln_conf_lex_free(cls);
            return -1;
        }
        cc->n_args = cnt;
        mln_conf_lex_free(cls);
        return 0;
    }
    mln_conf_lex_struct_t *next = mln_conf_token(cf->lex);
    if (mln_conf_item_recursive(cf, cc, next, cnt+1) < 0) {
        mln_conf_lex_free(cls);
        return -1;
    }
    mln_conf_item_t *ci = &cc->arg_tbl[cnt];
    int ret = mln_conf_item_init(cls, ci);
    mln_conf_lex_free(cls);
    if (ret < 0) return -1;
    return 0;
}

static int _mln_load_conf(mln_conf_t *cf, mln_conf_domain_t *current)
{
    mln_conf_lex_struct_t *fir, *next;
    mln_conf_cmd_t *cmd;
    mln_conf_domain_t *cd;
    mln_string_t *tk;
    while ( 1 ) {
        fir = mln_conf_token(cf->lex);
        if (fir == NULL) {
            mln_log(error, "Get token error. %s\n", mln_lex_strerror(cf->lex));
            return -1;
        } else if (fir->type == CONF_TK_EOF) {
            mln_conf_lex_free(fir);
            break;
        } else if (fir->type == CONF_TK_RBRACE) {
            if (mln_strcmp(current->domain_name, &default_domain)) {
                mln_conf_lex_free(fir);
                return 0;
            }
            CONF_ERR(fir, "Invalid right brace");
            mln_conf_lex_free(fir);
            return -1;
        } else if (fir->type != CONF_TK_FACTOR) {
            CONF_ERR(fir, "Unexpected token");
            mln_conf_lex_free(fir);
            return -1;
        }
        next = mln_conf_token(cf->lex);
        if (next == NULL) {
            mln_conf_lex_free(fir);
            mln_log(error, "Get token error. %s\n", mln_lex_strerror(cf->lex));
            return -1;
        }
        else if (next->type == CONF_TK_EOF) {
            CONF_ERR(next, "Invalid end of file");
            mln_conf_lex_free(fir);
            mln_conf_lex_free(next);
            return -1;
        }
        /*as domain*/
        if (next->type == CONF_TK_LBRACE) {
            mln_conf_lex_free(next);
            if (mln_strcmp(current->domain_name, &default_domain)) {
                CONF_ERR(fir, "Illegal domain");
                mln_conf_lex_free(fir);
                return -1;
            }
            tk = mln_dup_string(fir->text);
            mln_conf_lex_free(fir);
            if (tk == NULL) {
                mln_log(error, "No memory.\n");
                return -1;
            }
            cd = mln_conf_domain_init(cf, tk);
            if (cd == NULL) {
                mln_log(error, "No memory.\n");
                mln_free_string(tk);
                return -1;
            }
            if (mln_hash_insert(cf->domain_hash_tbl, tk, cd) < 0) {
                mln_log(error, "No memory.\n");
                mln_free_string(tk);
                mln_conf_domain_destroy(cd);
                return -1;
            }
            if (_mln_load_conf(cf, cd) < 0) return -1;
            continue;
        }
        /*as command*/
        tk = mln_dup_string(fir->text);
        mln_conf_lex_free(fir);
        if (tk == NULL) {
            mln_log(error, "No memory.\n");
            mln_conf_lex_free(next);
            return -1;
        }
        cmd = mln_conf_cmd_init(tk);
        if (cmd == NULL) {
            mln_log(error, "No memory.\n");
            mln_free_string(tk);
            mln_conf_lex_free(next);
            return -1;
        }
        if (mln_hash_insert(current->cmd_hash_tbl, tk, cmd) < 0) {
            mln_log(error, "No memory.\n");
            mln_free_string(tk);
            mln_conf_cmd_destroy(cmd);
            mln_conf_lex_free(next);
            return -1;
        }
        if (mln_conf_item_recursive(cf, cmd, next, 0) < 0) return -1;
        /*
         * we don't need to free the pointer 'next' here,
         * because it has already freed in mln_conf_item_recursive().
         */
    }
    return 0;
}

int mln_load_conf(void)
{
    mln_conf_t *cf = mln_conf_init();
    if (cf == NULL) return -1;
    mln_conf_domain_t *cd = mln_hash_search(cf->domain_hash_tbl, &default_domain);
    mln_s32_t ret = _mln_load_conf(cf, cd);
    mln_conf_destroy_lex(cf);
    if (ret < 0) {
        mln_conf_destroy(cf);
        return -1;
    }
    mln_conf_add_in_queue(cstack, cf);
    return 0;
}

void mln_free_conf(mln_conf_t *cf)
{
    if (cf->refer_cnt) return;
    mln_conf_del_from_queue(cstack, cf);
    mln_conf_destroy(cf);
}

mln_conf_t *mln_get_conf(void)
{
    return cstack->top_new;
}

mln_u32_t mln_get_cmd_num(mln_conf_t *cf, char *domain)
{
    mln_conf_domain_t *cd = cf->search(cf, domain);
    if (cd == NULL) return 0;
    return cd->cmd_hash_tbl->nr_nodes;
}

void mln_get_all_cmds(mln_conf_t *cf, char *domain, mln_conf_cmd_t **v)
{
    mln_conf_domain_t *cd = cf->search(cf, domain);
    if (cd == NULL) return;
    mln_hash_entry_t *he;
    mln_hash_mgr_t *mgr = cd->cmd_hash_tbl->tbl, *end;
    mln_u32_t i = 0;
    for (end = cd->cmd_hash_tbl->tbl+cd->cmd_hash_tbl->len; mgr < end; mgr++) {
        for (he = mgr->head; he != NULL; he = he->next) {
            v[i++] = (mln_conf_cmd_t *)(he->val);
        }
    }
}

mln_u32_t mln_get_cmd_args_num(mln_conf_cmd_t *cc)
{
    return cc->n_args;
}

void mln_dump_conf(void)
{
    mln_conf_t *cf;
    mln_conf_domain_t *cd;
    mln_hash_entry_t *he, *he_cmd;
    mln_conf_cmd_t *cmd;
    mln_conf_item_t *ci;
    mln_s32_t i, j, k;
    for (cf = cstack->top_new; cf != NULL; cf = cf->next) {
        printf("CONFIGURATIONS:\n");
        for (i = 0; i<cf->domain_hash_tbl->len; i++) {
            for (he = cf->domain_hash_tbl->tbl[i].head; he != NULL; he = he->next) {
                cd = (mln_conf_domain_t *)(he->val);
                printf("\tDOMAIN [%s]:\n", cd->domain_name->str);
                for (j = 0; j<cd->cmd_hash_tbl->len; j++) {
                    for (he_cmd = cd->cmd_hash_tbl->tbl[j].head; he_cmd != NULL; he_cmd = he_cmd->next) {
                        cmd = (mln_conf_cmd_t *)(he_cmd->val);
                        printf("\t\tCOMMAND [%s]:\n", cmd->cmd_name->str);
                        for (k = 0; k<cmd->n_args; k++) {
                            ci = &(cmd->arg_tbl[k]);
                            printf("\t\t\t");
                            switch(ci->type) {
                                case CONF_STR:
                                    printf("STRING [%s]\n", ci->val.s->str);
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
                                default:
                                    break;
                            }
                        }
                    }
                }
            }
        }
    }
}

