
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

#define M_TOOLS_TIME_UTC             0
#define M_TOOLS_TIME_GENERALIZEDTIME 1

struct UTCTime_s {
    long        year;
    long        month;
    long        day;
    long        hour;
    long        minute;
    long        second;
};


extern int mln_sys_limit_modify(void);
extern int mln_daemon(void);
extern int mln_boot_params(int argc, char *argv[]);
extern void mln_UTCTime(time_t tm, struct UTCTime_s *uc) __NONNULL1(2);
extern int mln_s2Time(time_t *tm, mln_string_t *s, int type) __NONNULL2(1,2);
#endif

