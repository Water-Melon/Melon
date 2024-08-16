
/*
 * Copyright (C) Niklaus F.Schen.
 */
#ifndef __MLN_EXPR_H
#define __MLN_EXPR_H

#include "mln_string.h"
#include "mln_array.h"

#define MLN_EXPR_RET_ERR     -1
#define MLN_EXPR_RET_OK       0
#define MLN_EXPR_RET_COMMA    1
#define MLN_EXPR_RET_RPAR     2

#define MLN_EXPR_DEFAULT_ARGS 8

typedef void (*mln_expr_udata_free)(void *);

typedef enum {
    mln_expr_type_null = 0,
    mln_expr_type_bool,
    mln_expr_type_int,
    mln_expr_type_real,
    mln_expr_type_string,
    mln_expr_type_udata,
} mln_expr_typ_t;

typedef struct {
    mln_expr_typ_t       type;
    union {
        mln_u8_t         b;
        mln_s64_t        i;
        double           r;
        mln_string_t    *s;
        void            *u;
    } data;
    mln_expr_udata_free  free;
} mln_expr_val_t;

typedef mln_expr_val_t *(*mln_expr_cb_t)(mln_string_t *namespace, \
                                         mln_string_t *name, \
                                         int is_func, \
                                         mln_array_t *args, \
                                         void *data);

extern mln_expr_val_t *mln_expr_run(mln_string_t *exp, mln_expr_cb_t cb, void *data);
extern mln_expr_val_t *mln_expr_run_file(mln_string_t *path, mln_expr_cb_t cb, void *data);
extern mln_expr_val_t *mln_expr_val_new(mln_expr_typ_t type, void *data, mln_expr_udata_free free);
extern void mln_expr_val_free(mln_expr_val_t *ev);
extern mln_expr_val_t *mln_expr_val_dup(mln_expr_val_t *val);
extern void mln_expr_val_copy(mln_expr_val_t *dest, mln_expr_val_t *src);
#endif
