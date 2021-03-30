
/*
 * Copyright (C) Niklaus F.Schen.
 */

#ifndef __MLN_TOOLS_H
#define __MLN_TOOLS_H

#include "mln_string.h"
#include "mln_types.h"

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
#if !defined(WINNT)
extern int mln_daemon(void);
#endif
extern int mln_boot_params(int argc, char *argv[]);
extern void mln_utctime(time_t tm, struct UTCTime_s *uc) __NONNULL1(2);
extern int mln_s2time(time_t *tm, mln_string_t *s, int type) __NONNULL2(1,2);
#endif

