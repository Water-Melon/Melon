
/*
 * Copyright (C) Niklaus F.Schen.
 */
#ifndef __MLN_LANG_MYSQL_H
#define __MLN_LANG_MYSQL_H

#include "mln_lang.h"
#ifdef MLN_MYSQL
#include <mysql.h>
#include <mysqld_error.h>


typedef struct mln_lang_mysql_s {
    mln_lang_ctx_t          *ctx;
    mln_lang_var_t          *ret_var;
    MYSQL                   *mysql;
    MYSQL_RES               *result;
    MYSQL_ROW                row;
    mln_size_t               nfield;
    mln_string_t            *db;
    mln_string_t            *username;
    mln_string_t            *password;
    mln_string_t            *host;
    mln_string_t            *sql;
    int                      port;
    int                      fd_signal;
    int                      fd_useless;
    struct mln_lang_mysql_s *prev;
    struct mln_lang_mysql_s *next;
} mln_lang_mysql_t;

typedef struct mln_lang_mysql_timeout_s {
    mln_lang_ctx_t          *ctx;
    mln_lang_mysql_t        *head;
    mln_lang_mysql_t        *tail;
} mln_lang_mysql_timeout_t;

#endif

extern int mln_lang_mysql(mln_lang_ctx_t *ctx);

#endif
