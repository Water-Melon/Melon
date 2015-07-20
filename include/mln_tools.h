
/*
 * Copyright (C) Niklaus F.Schen.
 */

#ifndef __MLN_TOOLS_H
#define __MLN_TOOLS_H

#include "mln_types.h"
#include "mln_lex.h"

MLN_DEFINE_TOKEN_TYPE_AND_STRUCT(extern, \
                                 mln_passwd_lex, \
                                 PWD, \
                                 PWD_TK_MELON, PWD_TK_COMMENT);

typedef int (*boot_param)(const char *, const char *);

typedef struct {
    char       *boot_str;
    char       *alias;
    boot_param  handler;
    mln_size_t  cnt;
} mln_boot_t;


extern int mln_unlimit_memory(void);
extern int mln_cancel_core(void);
extern int mln_unlimit_fd(void);
extern int mln_daemon(void);
extern int mln_boot_params(int argc, char *argv[]);
#endif

