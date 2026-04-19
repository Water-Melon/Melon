
/*
 * Copyright (C) Niklaus F.Schen.
 */

#include <string.h>
#include <errno.h>
#include <fcntl.h>
#if !defined(MSVC)
#include <sys/resource.h>
#include <pwd.h>
#include <unistd.h>
#endif
#include <stdlib.h>
#include <sys/types.h>
#include <signal.h>
#include "mln_tools.h"
#include "mln_conf.h"
#include "mln_log.h"
#include "mln_path.h"
#include "mln_tools.h"

static int
mln_boot_help(const char *boot_str, const char *alias);
#if !defined(MSVC)
static int mln_set_id(void);
static int
mln_boot_reload(const char *boot_str, const char *alias);
static int
mln_boot_stop(const char *boot_str, const char *alias);
#endif
static int mln_sys_core_modify(void);
static int mln_sys_nofile_modify(void);

long mon_days[2][12] = {
    {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
    {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}
};
/* prefix sums: mon_days_prefix[leap][m] = sum of days for months 0..m-1 */
static long mon_days_prefix[2][13] = {
    {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365},
    {0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335, 366}
};
mln_boot_t boot_params[] = {
{"--help", "-h", mln_boot_help, 0},
#if !defined(MSVC)
{"--reload", "-r", mln_boot_reload, 0},
{"--stop", "-s", mln_boot_stop, 0}
#endif
};
char mln_core_file_cmd[] = "core_file_size";
char mln_nofile_cmd[] = "max_nofile";
char mln_limit_unlimited[] = "unlimited";

MLN_FUNC(, int, mln_sys_limit_modify, (void), (), {
    if (mln_sys_core_modify() < 0) {
        return -1;
    }
    return mln_sys_nofile_modify();
})

static int mln_sys_core_modify(void)
{
#ifdef RLIMIT_CORE
    rlim_t core_file_size = 0;

    mln_conf_t *cf = mln_conf();
    if (cf == NULL) {
        fprintf(stderr, "Configuration messed up.\n");
        return -1;
    }
    mln_conf_domain_t *cd = cf->search(cf, "main");
    if (cd == NULL) {
        fprintf(stderr, "Configuration messed up.\n");
        return -1;
    }
    mln_conf_cmd_t *cc = cd->search(cd, mln_core_file_cmd);
    if (cc == NULL) return 0;

    mln_conf_item_t *ci = cc->search(cc, 1);
    if (ci->type == CONF_INT) {
        core_file_size = (rlim_t)ci->val.i;
    } else if (ci->type == CONF_STR) {
        if (mln_string_const_strcmp(ci->val.s, mln_limit_unlimited)) {
            fprintf(stderr, "Invalid argument of %s.\n", mln_core_file_cmd);
            return -1;
        }
        core_file_size = RLIM_INFINITY;
    } else {
        fprintf(stderr, "Invalid argument of %s.\n", mln_core_file_cmd);
        return -1;
    }

    struct rlimit rl;
    memset(&rl, 0, sizeof(rl));
    rl.rlim_cur = rl.rlim_max = core_file_size;
    if (setrlimit(RLIMIT_CORE, &rl) != 0) {
        fprintf(stderr, "setrlimit core failed, %s\n", strerror(errno));
        return -1;
    }
#endif
    return 0;
}

static int mln_sys_nofile_modify(void)
{
#ifdef RLIMIT_NOFILE
    rlim_t nofile = 0;

    mln_conf_t *cf = mln_conf();
    if (cf == NULL) {
        fprintf(stderr, "Configuration messed up.\n");
        return -1;
    }
    mln_conf_domain_t *cd = cf->search(cf, "main");
    if (cd == NULL) {
        fprintf(stderr, "Configuration messed up.\n");
        return -1;
    }
    mln_conf_cmd_t *cc = cd->search(cd, mln_nofile_cmd);
    if (cc == NULL) return 0;

    mln_conf_item_t *ci = cc->search(cc, 1);
    if (ci->type == CONF_INT) {
        nofile = (rlim_t)ci->val.i;
    } else if (ci->type == CONF_STR) {
        if (mln_string_const_strcmp(ci->val.s, mln_limit_unlimited)) {
            fprintf(stderr, "Invalid argument of %s.\n", mln_nofile_cmd);
            return -1;
        }
        nofile = RLIM_INFINITY;
    } else {
        fprintf(stderr, "Invalid argument of %s.\n", mln_nofile_cmd);
        return -1;
    }

    struct rlimit rl;
    memset(&rl, 0, sizeof(rl));
    rl.rlim_cur = rl.rlim_max = nofile;
    if (setrlimit(RLIMIT_NOFILE, &rl) != 0) {
        fprintf(stderr, "setrlimit fd failed, %s\n", strerror(errno));
        return -1;
    }
#endif
    return 0;
}

#if !defined(MSVC)
MLN_FUNC(, int, mln_daemon, (void), (), {
    int ret = mln_log_init(NULL);
    if (ret < 0) return ret;
    if (!mln_log_in_daemon()) return mln_set_id();

    pid_t pid;
    if ((pid = fork()) < 0) {
        mln_log(error, "%s\n", strerror(errno));
        exit(1);
    } else if (pid != 0) {
        exit(0);
    }
    setsid();

    int fd0 = STDIN_FILENO;
    int fd1 = STDOUT_FILENO;
    int fd2 = STDERR_FILENO;
    close(fd0);
    close(fd1);
    close(fd2);
    fd0 = open(mln_path_null(), O_RDWR);
    fd1 = dup(fd0);
    fd2 = dup(fd0);
    if (fd0 != STDIN_FILENO || \
        fd1 != STDOUT_FILENO || \
        fd2 != STDERR_FILENO)
    {
        fprintf(stderr, "Unexpected file descriptors %d %d %d\n", fd0, fd1, fd2);
    }
    return mln_set_id();
})

MLN_FUNC(static, int, mln_set_id, (void), (), {
    char name[256] = {0};
    int len;
    uid_t uid;
    gid_t gid;
    struct passwd *pwd;

    /*get user name*/
    mln_conf_t *cf = mln_conf();
    if (cf == NULL) {
        mln_log(error, "No configuration.\n");
        abort();
    }
    mln_conf_domain_t *cd = cf->search(cf, "main");
    if (cd == NULL) {
        mln_log(error, "No 'main' domain.\n");
        abort();
    }
    mln_conf_cmd_t *cc = cd->search(cd, "user");
    if (cc == NULL) {
        uid = getuid();
        gid = getgid();
    } else {
        mln_conf_item_t *ci = cc->search(cc, 1);
        if (ci == NULL) {
            mln_log(error, "Command 'user' need a parameter.\n");
            return -1;
        }
        if (ci->type != CONF_STR) {
            mln_log(error, "Parameter type of command 'user' error.\n");
            return -1;
        }
        len = ci->val.s->len;
        if (len > 255) len = 255;
        memcpy(name, ci->val.s->data, len);
        if ((pwd = getpwnam(name)) == NULL) {
            mln_log(error, "getpwnam failed. %s\n", strerror(errno));
            return -1;
        }
        if (pwd->pw_uid != getuid()) {
            uid = pwd->pw_uid;
            gid = pwd->pw_gid;
        } else {
            uid = getuid();
            gid = getgid();
        }
    }

    /*set log files' uid & gid*/
    int rc = 1;
    char *path = mln_log_dir_path();
    if (!access(path, F_OK))
        rc = chown(path, uid, gid);
    path = mln_log_logfile_path();
    if (!access(path, F_OK))
        rc = chown(path, uid, gid);
    path = mln_log_pid_path();
    if (!access(path, F_OK))
        rc = chown(path, uid, gid);
    if (rc < 0) rc = 1;/*do nothing*/

    /*set uid, gid*/
    if (setgid(gid) < 0) {
        mln_log(error, "Set GID failed. %s\n", strerror(errno));
        return -1;
    }
    if (setuid(uid) < 0) {
        mln_log(error, "Set UID failed. %s\n", strerror(errno));
    }
    if (setegid(gid) < 0) {
        mln_log(error, "Set eGID failed. %s\n", strerror(errno));
    }
    if (seteuid(uid) < 0) {
        mln_log(error, "Set eUID failed. %s\n", strerror(errno));
    }

    return 0;
})
#endif

MLN_FUNC(, int, mln_boot_params, (int argc, char *argv[]), (argc, argv), {
    int i, ret;
    char *p;
    mln_boot_t *b;
    mln_boot_t *bend = boot_params + sizeof(boot_params)/sizeof(mln_boot_t);
    for (i = 1; i < argc; ++i) {
        p = argv[i];
        for (b = boot_params; b < bend; ++b) {
             if (!b->cnt && \
                 (!strcmp(p, b->boot_str) || \
                 !strcmp(p, b->alias)))
             {
                 ++(b->cnt);
                 ret = b->handler(b->boot_str, b->alias);
                 if (ret < 0) {
                     return ret;
                 }
             }
        }
    }
    return 0;
})

MLN_FUNC(static, int, mln_boot_help, \
         (const char *boot_str, const char *alias), (boot_str, alias), \
{
    printf("Boot parameters:\n");
    printf("\t--reload  -r\t\t\treload configuration\n");
    printf("\t--stop    -s\t\t\tstop melon service.\n");
    exit(0);
    return 0;
})

#if !defined(MSVC)
MLN_FUNC(static, int, mln_boot_reload, \
         (const char *boot_str, const char *alias), \
         (boot_str, alias), \
{
    char buf[1024] = {0};
    int fd, n, pid;

    fd = open(mln_log_pid_path(), O_RDONLY);
    if (fd < 0) {
        fprintf(stderr, "'melon.pid' not existent.\n");
        exit(1);
    }
    n = read(fd, buf, sizeof(buf)-1);
    if (n <= 0) {
        fprintf(stderr, "Invalid file 'melon.pid'.\n");
        exit(1);
    }
    buf[n] = 0;

    pid = atoi(buf);
    kill(pid, SIGUSR2);

    exit(0);
})

MLN_FUNC(static, int, mln_boot_stop, \
         (const char *boot_str, const char *alias), (boot_str, alias), \
{
    char buf[1024] = {0};
    int fd, n, pid;

    snprintf(buf, sizeof(buf)-1, "%s", mln_path_pid());
    fd = open(buf, O_RDONLY);
    if (fd < 0) {
        fprintf(stderr, "'melon.pid' not existent.\n");
        exit(1);
    }
    n = read(fd, buf, sizeof(buf)-1);
    if (n <= 0) {
        fprintf(stderr, "Invalid file 'melon.pid'.\n");
        exit(1);
    }
    buf[n] = 0;

    pid = atoi(buf);
    kill(pid, SIGKILL);

    exit(0);
})
#endif

/*
 * time
 */
MLN_FUNC(static inline, int, mln_is_leap, (long year), (year), {
    if (((year%4 == 0) && (year%100 != 0)) || (year%400 == 0))
        return 1;
    return 0;
})

MLN_FUNC_VOID(, void, mln_time2utc, (time_t tm, struct utctime *uc), (tm, uc), {
    /*
     * O(1) civil date algorithm (era-based decomposition).
     * Shift epoch to 0000-03-01 so Feb (leap day) is last month of the year,
     * then decompose using 400/100/4/1-year cycles.
     */
    long days = (long)(tm / 86400);
    long subsec = (long)(tm % 86400);
    long z, era, doe, yoe, y, doy, mp, d, m, month, year;

    /* shift to days since 0000-03-01 */
    z = days + 719468;
    era = (z >= 0 ? z : z - 146096) / 146097;
    doe = z - era * 146097;
    yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    y = yoe + era * 400;
    doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    mp = (5 * doy + 2) / 153;
    d = doy - (153 * mp + 2) / 5 + 1;
    m = mp < 10 ? mp + 3 : mp - 9;
    y += (m <= 2);

    uc->year = y;
    uc->month = m;
    uc->day = d;
    uc->hour = subsec / 3600;
    uc->minute = (subsec % 3600) / 60;
    uc->second = (subsec % 3600) % 60;
    month = uc->month < 3? uc->month + 12: uc->month;
    year = uc->month < 3? uc->year - 1: uc->year;
    uc->week = (uc->day + 1 + 2 * month + 3 * (month + 1) / 5 + year + (year >> 2) - year / 100 + year / 400) % 7;
})

MLN_FUNC(, time_t, mln_utc2time, (struct utctime *uc), (uc), {
    /*
     * O(1) reverse civil date algorithm.
     * Shift March-based year so leap day is at end of cycle,
     * then compute days since epoch directly.
     */
    long y = uc->year;
    long m = uc->month;
    long d = uc->day;
    long era, yoe, doy, doe, days;

    y -= (m <= 2);
    era = (y >= 0 ? y : y - 399) / 400;
    yoe = y - era * 400;
    doy = (153 * (m > 2 ? m - 3 : m + 9) + 2) / 5 + d - 1;
    doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    days = era * 146097 + doe - 719468;

    return (time_t)(days * 86400 + uc->hour * 3600 + uc->minute * 60 + uc->second);
})

MLN_FUNC_VOID(, void, mln_utc_adjust, (struct utctime *uc), (uc), {
    /*
     * O(1) adjust: normalize by converting to epoch seconds
     * and decomposing back via the O(1) civil date routines.
     */
    time_t total = mln_utc2time(uc);
    mln_time2utc(total, uc);
})

MLN_FUNC(, long, mln_month_days, (long year, long month), (year, month), {
    return mon_days[mln_is_leap(year)][month-1];
})

MLN_FUNC(, int, mln_s2time, (time_t *tm, mln_string_t *s, int type), (tm, s, type), {
    mln_u8ptr_t p, end;
    time_t year = 0, month = 0, day = 0;
    time_t hour = 0, minute = 0, second = 0;

    switch (type) {
        case M_TOOLS_TIME_UTC:
            p = s->data;
            end = s->data + s->len - 1;
            if (s->len != 13 || (*end != 'Z' && *end != 'z')) return -1;
            for (; p < end; ++p) if (!mln_isdigit(*p)) return -1;
            p = s->data;
            year = ((*p++) - '0') * 10;
            year += ((*p++) - '0');
            if (year >= 50) year += 1900;
            else year += 2000;
            break;
        case M_TOOLS_TIME_GENERALIZEDTIME:
            p = s->data;
            end = s->data + s->len - 1;
            if (s->len != 15 || (*end != 'Z' && *end != 'z')) return -1;
            for (; p < end; ++p) if (!mln_isdigit(*p)) return -1;
            p = s->data;
            year = ((*p++) - '0') * 1000;
            year += (((*p++) - '0') * 100);
            year += (((*p++) - '0') * 10);
            year += ((*p++) - '0');
            break;
        default:
            return -1;
    }
    month = (*p++ - '0') * 10;
    month += (*p++ - '0');
    day = (*p++ - '0') * 10;
    day += (*p++ - '0');
    hour = (*p++ - '0') * 10;
    hour += (*p++ - '0');
    minute = (*p++ - '0') * 10;
    minute += (*p++ - '0');
    second = (*p++ - '0') * 10;
    second += (*p++ - '0');
    if (year < 1970 || \
        month > 12 || \
        day > mon_days[mln_is_leap(year)][month-1] || \
        hour >= 24 || \
        minute >= 60 || \
        second >= 60)
    {
        return -1;
    }

    /* O(1) year-to-days: count leap years with formula instead of loop */
    {
        long y1 = year - 1, y0 = 1969;
        long leap1 = y1 / 4 - y1 / 100 + y1 / 400;
        long leap0 = y0 / 4 - y0 / 100 + y0 / 400;
        day += 365 * (year - 1970) + (leap1 - leap0);
    }
    /* O(1) month-to-days via prefix sum */
    day += mon_days_prefix[mln_is_leap(year)][month - 1];
    --day;
    *tm = day * 86400;
    if (hour || minute || second) {
        *tm += (hour * 3600 + minute * 60 + second);
    }

    return 0;
})

