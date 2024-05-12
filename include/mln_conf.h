
/*
 * Copyright (C) Niklaus F.Schen.
 */

#ifndef __MLN_CONF_H
#define __MLN_CONF_H

#include "mln_lex.h"
#include "mln_types.h"
#include "mln_rbtree.h"
#include "mln_string.h"
#include "mln_ipc.h"

#define M_IPC_TYPE_CONF 1

typedef struct mln_conf_item_s    mln_conf_item_t;
typedef struct mln_conf_domain_s  mln_conf_domain_t;
typedef struct mln_conf_cmd_s     mln_conf_cmd_t;
typedef struct mln_conf_s         mln_conf_t;

/*
 * the second argument is an index locating command's item.
 * This index start from 1, not 0.
 */
typedef mln_conf_item_t   *(*mln_conf_item_cb_t)        (mln_conf_cmd_t *, mln_u32_t);
typedef int                (*mln_conf_item_update_cb_t) (mln_conf_cmd_t *, mln_conf_item_t *, mln_u32_t);
typedef mln_conf_cmd_t    *(*mln_conf_cmd_cb_t)         (mln_conf_domain_t *, char *);
typedef mln_conf_domain_t *(*mln_conf_domain_cb_t)      (mln_conf_t *, char *);
typedef int                (*reload_handler)            (void *);

typedef enum {
    CONF_NONE = 0,
    CONF_STR,
    CONF_CHAR,
    CONF_BOOL,
    CONF_INT,
    CONF_FLOAT
} mln_conf_item_type_t;

struct mln_conf_item_s {
    mln_conf_item_type_t type;
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
    mln_conf_item_update_cb_t      update;
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
#if !defined(MSVC)
    mln_ipc_cb_t                  *cb;
#endif
};

typedef struct mln_conf_hook_s {
    reload_handler                 reload;
    void                          *data;
    struct mln_conf_hook_s        *prev;
    struct mln_conf_hook_s        *next;
} mln_conf_hook_t;

/*
 * mln_conf_load should be called before any pthread_create
 */

#define mln_conf_is_empty(cf) ((cf) == NULL || \
    (mln_rbtree_node_num((cf)->domain) <= 1 && \
    mln_rbtree_node_num(((mln_conf_domain_t *)(mln_rbtree_node_data_get(mln_rbtree_root((cf)->domain))))->cmd) == 0))

extern int mln_conf_load(void);
extern int mln_conf_reload(void);
extern void mln_conf_free(void);
extern mln_conf_t *mln_conf(void);
extern void mln_conf_dump(void);
extern mln_conf_hook_t *mln_conf_hook_set(reload_handler reload, void *data);
extern void mln_conf_hook_unset(mln_conf_hook_t *hook);
extern void mln_conf_hook_free(void);

extern mln_u32_t
mln_conf_cmd_num(mln_conf_t *cf, char *domain) __NONNULL2(1,2);
extern void
mln_conf_cmds(mln_conf_t *cf, char *domain, mln_conf_cmd_t **vector) __NONNULL3(1,2,3);
extern mln_u32_t
mln_conf_arg_num(mln_conf_cmd_t *cc) __NONNULL1(1);

#endif

