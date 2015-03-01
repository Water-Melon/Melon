
/*
 * Copyright (C) Niklaus F.Schen.
 */

#ifndef __MLN_CONF_H
#define __MLN_CONF_H

#include "mln_lex.h"
#include "mln_types.h"
#include "mln_hash.h"
#include "mln_string.h"

#define MLN_CONF_HASH_LEN 10

typedef struct mln_conf_item_s    mln_conf_item_t;
typedef struct mln_conf_domain_s  mln_conf_domain_t;
typedef struct mln_conf_cmd_s     mln_conf_cmd_t;
typedef struct mln_conf_s         mln_conf_t;

/*
 * the second argument is an index locating command's item.
 * This index start from 1, not 0.
 */
typedef mln_conf_item_t   *(*search_item)  (mln_conf_cmd_t *, mln_u32_t);
typedef mln_conf_cmd_t    *(*search_cmd)   (mln_conf_domain_t *, char *);
typedef mln_conf_domain_t *(*search_domain)(mln_conf_t *, char *);

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
    search_item                    search;
    mln_conf_item_t               *arg_tbl;
    mln_u32_t                      n_args;
};

struct mln_conf_domain_s {
    search_cmd                     search;
    mln_string_t                  *domain_name;
    mln_hash_t                    *cmd_hash_tbl;
};

struct mln_conf_s {
    mln_lex_t                     *lex;
    mln_hash_t                    *domain_hash_tbl;
    search_domain                  search;
    struct mln_conf_s             *prev;
    struct mln_conf_s             *next;
    mln_u32_t                      refer_cnt;
};

typedef struct {
    mln_conf_t                    *top_new;
    mln_conf_t                    *bottom_old;
} mln_conf_stack_t;


extern int
mln_conf_stack_init(void);
extern void
mln_conf_stack_destroy(void);
extern int mln_load_conf(void);
extern void
mln_free_conf(mln_conf_t *cf) __NONNULL1(1);
extern mln_conf_t *mln_get_conf(void);
extern void mln_dump_conf(void);

extern mln_u32_t
mln_get_cmd_num(mln_conf_t *cf, char *domain) __NONNULL2(1,2);
extern void
mln_get_all_cmds(mln_conf_t *cf, char *domain, mln_conf_cmd_t **vector) __NONNULL3(1,2,3);
extern mln_u32_t
mln_get_cmd_args_num(mln_conf_cmd_t *cc) __NONNULL1(1);

#endif

