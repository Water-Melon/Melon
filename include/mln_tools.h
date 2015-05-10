
/*
 * Copyright (C) Niklaus F.Schen.
 */

#ifndef __MLN_TOOLS_H
#define __MLN_TOOLS_H

#include "mln_lex.h"
MLN_DEFINE_TOKEN_TYPE_AND_STRUCT(extern, mln_passwd_lex, PWD, PWD_TK_MELON, PWD_TK_COMMENT);

extern int mln_unlimit_memory(void);
extern int mln_cancel_core(void);
extern int mln_unlimit_fd(void);
extern void mln_daemon(void);
extern void mln_close_terminal(void);
extern int mln_set_id(void);
#endif

