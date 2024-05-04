
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

struct utctime {
    long        year;
    long        month;
    long        day;
    long        hour;
    long        minute;
    long        second;
    long        week;
};


extern int mln_sys_limit_modify(void);
#if !defined(MSVC)
extern int mln_daemon(void);
#endif
extern int mln_boot_params(int argc, char *argv[]);
extern void mln_time2utc(time_t tm, struct utctime *uc) __NONNULL1(2);
extern time_t mln_utc2time(struct utctime *uc) __NONNULL1(1);
extern void mln_utc_adjust(struct utctime *uc) __NONNULL1(1);
extern long mln_month_days(long year, long month);
extern int mln_s2time(time_t *tm, mln_string_t *s, int type) __NONNULL2(1,2);
#endif

