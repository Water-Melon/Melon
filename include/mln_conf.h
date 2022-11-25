
/*
 * Copyright (C) Niklaus F.Schen.
 */

#ifndef __MLN_CONF_H
#define __MLN_CONF_H

#include "mln_lex.h"
#include "mln_types.h"
#include "mln_rbtree.h"
#include "mln_string.h"

#define M_IPC_TYPE_CONF 1

typedef struct mln_conf_item_s    mln_conf_item_t;
typedef struct mln_conf_domain_s  mln_conf_domain_t;
typedef struct mln_conf_cmd_s     mln_conf_cmd_t;
typedef struct mln_conf_s         mln_conf_t;

/*
 * the second argument is an index locating command's item.
 * This index start from 1, not 0.
 */
typedef mln_conf_item_t   *(*mln_conf_item_cb_t)   (mln_conf_cmd_t *, mln_u32_t);
typedef mln_conf_cmd_t    *(*mln_conf_cmd_cb_t)    (mln_conf_domain_t *, char *);
typedef mln_conf_domain_t *(*mln_conf_domain_cb_t) (mln_conf_t *, char *);
typedef int                (*reload_handler)(void *);

struct mln_conf_item_s {
    enum {
        CONF_NONE = 0,
        CONF_STR,
        CONF_CHAR,
        CONF_BOOL,
        CONF_INT,
        CONF_FLOAT
    } type;
    union {
        mln_string_t *s;
        mln_s8_t c;
        mln_u8_t b;
        mln_sauto_t i;
        float f;
    } val;
};

struct mln_conf_cmd_s {
    mln_string_t                  *cmd_name;
    mln_conf_item_cb_t             search;
    mln_conf_item_t               *arg_tbl;
    mln_u32_t                      n_args;
};

struct mln_conf_domain_s {
    mln_conf_cmd_cb_t              search;
    mln_conf_cmd_cb_t              insert;
    mln_conf_cmd_cb_t              remove;
    mln_string_t                  *domain_name;
    mln_rbtree_t                  *cmd;
};

struct mln_conf_s {
    mln_lex_t                     *lex;
    mln_rbtree_t                  *domain;
    mln_conf_domain_cb_t           search;
    mln_conf_domain_cb_t           insert;
    mln_conf_domain_cb_t           remove;
};

typedef struct mln_conf_hook_s {
    reload_handler                 reload;
    void                          *data;
    struct mln_conf_hook_s        *prev;
    struct mln_conf_hook_s        *next;
} mln_conf_hook_t;

extern int mln_conf_reload(void);
extern int mln_conf_load(void);
extern void mln_conf_free(void);
extern mln_conf_t *mln_get_conf(void);
extern void mln_conf_dump(void);
extern mln_conf_hook_t *mln_conf_set_hook(reload_handler reload, void *data);
extern void mln_conf_unset_hook(mln_conf_hook_t *hook);
extern void mln_conf_free_hook(void);

extern mln_u32_t
mln_conf_get_ncmd(mln_conf_t *cf, char *domain) __NONNULL2(1,2);
extern void
mln_conf_get_cmds(mln_conf_t *cf, char *domain, mln_conf_cmd_t **vector) __NONNULL3(1,2,3);
extern mln_u32_t
mln_conf_get_narg(mln_conf_cmd_t *cc) __NONNULL1(1);

#endif

