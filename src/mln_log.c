
/*
 * Copyright (C) Niklaus F.Schen.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <errno.h>
#include <stdarg.h>
#include "mln_types.h"
#include "mln_log.h"
#include "mln_conf.h"
#include "mln_global.h"

static void
_mln_sys_log_process(enum sys_log level, \
                     const char *file, \
                     const char *func, \
                     int line, \
                     char *msg, \
                     va_list arg);
static void
mln_log_proc_lock(void);
static void
mln_log_proc_unlock(void);

struct localtime_s {
    long year;
    long month;
    long day;
    long hour;
    long minute;
    long second;
};

long mon_days[2][12] = {
    {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
    {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}
};
int log_fd;
int log_in_daemon = 0;
enum sys_log allow_level;
char mln_log_file[] = "log/melon.log";
char mln_pid_file[] = "log/melon.pid";
char mln_log_dir[] = "log/";
char mln_log_filepath[1024] = {0};
char mln_pid_filepath[1024] = {0};
char mln_log_dirpath[1024] = {0};
char log_err_level[] = "Log level permission deny.";
char log_err_fmt[] = "Log message format error.";
mln_lock_t log_lock;

static inline int
mln_is_leap(long year)
{
    if (((year%4 == 0) && (year%100 != 0)) || (year%400 == 0))
        return 1;
    return 0;
}

static void
mln_get_localtime(struct timeval *tv, struct timezone *tz, struct localtime_s *lc)
{
    long days = tv->tv_sec / 86400;
    long subsec = tv->tv_sec % 86400;
    long i = 365;
    lc->year = lc->month = 0;
    while (i < days) {
        lc->year++;
        if (mln_is_leap(1970+lc->year))
            i++;
        i += 365;
    }
    lc->year += 1970;
    int is_leap_year = mln_is_leap(lc->year);
    long subdays = days - (i - 365);
    long cnt = 0;
    while (cnt < subdays) {
        cnt += mon_days[is_leap_year][lc->month];
        lc->month++;
    }
    lc->day = subdays - (cnt - mon_days[is_leap_year][lc->month-1]) + 1;
    lc->hour = subsec / 3600;
#if defined(linux)
    lc->hour -= (tz->tz_minuteswest / 60);
    if (lc->hour < 0) {
        lc->hour = 24 + lc->hour;
        lc->day--;
        if (lc->day <= 0) {
            lc->month--;
            if (lc->month <= 0) {
                lc->month = 12;
                lc->year--;
            }
            lc->day = mon_days[is_leap_year][lc->month-1] + lc->day;
        }
    } else if (lc->hour > 23) {
       lc->hour -= 24;
       lc->day++;
       if (lc->day > mon_days[is_leap_year][lc->month-1]) {
           lc->day -= mon_days[is_leap_year][lc->month-1];
           lc->month++;
           if (lc->month > 12) {
               lc->month = 1;
               lc->year++;
           }
       }
    }
#endif
    lc->minute = (subsec % 3600) / 60;
#if defined(linux)
    lc->minute -= (tz->tz_minuteswest % 60);
    if (lc->minute < 0) {
        lc->minute = 60 + lc->minute;
        lc->hour--;
        if (lc->hour < 0) {
            lc->hour = 24 + lc->hour;
            lc->day--;
            if (lc->day <= 0) {
                lc->month--;
                if (lc->month <= 0) {
                    lc->month = 12;
                    lc->year--;
                }
                lc->day = mon_days[is_leap_year][lc->month-1] + lc->day;
            }
        }
    } else if (lc->minute > 59) {
        lc->minute -= 60;
        lc->hour++;
        if (lc->hour > 23) {
            lc->hour -= 24;
            lc->day++;
            if (lc->day > mon_days[is_leap_year][lc->month-1]) {
                lc->day -= mon_days[is_leap_year][lc->month-1];
                lc->month++;
                if (lc->month > 12) {
                    lc->month = 1;
                    lc->year++;
                }
            }
        }
    }
#endif
    lc->second = (subsec % 3600) % 60;
}

void mln_set_sys_log_fd(int fd)
{
    log_fd = fd;
}

void mln_init_sys_log(int is_daemon)
{
    int fd;
    if (is_daemon) {
        log_in_daemon = 1;
        memset(mln_log_dirpath, 0, sizeof(mln_log_dirpath));
        snprintf(mln_log_dirpath, \
                 sizeof(mln_log_dirpath)-1, \
                 "%s/%s", mln_get_path(), mln_log_dir);
        if (access(mln_log_dirpath, F_OK)) {
            if (mkdir(mln_log_dirpath, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH) < 0) {
                fprintf(stderr, "%s(): mkdir log/ failed. %s\n", \
                        __FUNCTION__, strerror(errno));
                exit(1);
            }
        }
        memset(mln_log_filepath, 0, sizeof(mln_log_filepath));
        snprintf(mln_log_filepath, \
                 sizeof(mln_log_filepath)-1, \
                 "%s/%s", mln_get_path(), mln_log_file);
        if (access(mln_log_filepath, F_OK)) {
            fd = open(mln_log_filepath, O_RDWR|O_CREAT, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
        } else {
            fd = open(mln_log_filepath, O_RDWR|O_APPEND);
        }
        if (fd < 0) {
            fprintf(stderr, "%s(): open %s failed. %s\n", \
                    __FUNCTION__, mln_log_filepath, strerror(errno));
            exit(1);
        }
        mln_set_sys_log_fd(fd);
    }
    allow_level = none;
    MLN_LOCK_INIT(&log_lock);
}

void mln_init_pid_log(void)
{
    int fd;
    memset(mln_log_dirpath, 0, sizeof(mln_log_dirpath));
    snprintf(mln_log_dirpath, \
             sizeof(mln_log_dirpath)-1, \
             "%s/%s", mln_get_path(), mln_log_dir);
    if (access(mln_log_dirpath, F_OK)) {
        if (mkdir(mln_log_dirpath, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH) < 0) {
            mln_log(error, "mkdir log/ failed. %s\n", strerror(errno));
            exit(1);
        }
    }
    memset(mln_pid_filepath, 0, sizeof(mln_pid_filepath));
    snprintf(mln_pid_filepath, \
             sizeof(mln_pid_filepath)-1, \
             "%s/%s", mln_get_path(), mln_pid_file);
    if (access(mln_pid_filepath, F_OK)) {
        fd = open(mln_pid_filepath, O_RDWR|O_CREAT, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
    } else {
        fd = open(mln_pid_filepath, O_RDWR|O_TRUNC);
    }
    if (fd < 0) {
        mln_log(error, "open %s failed. %s\n", mln_pid_filepath, strerror(errno));
        exit(1);
    }
    char buf[1024] = {0};
    int n = snprintf(buf, sizeof(buf)-1, "%d", getpid());
    write(fd, buf, n);
    close(fd);
}

int mln_set_log_level(void)
{
    mln_conf_t *cf = mln_get_conf();
    if (cf == NULL) return 0;
    mln_conf_domain_t *cd = cf->search(cf, "main");
    if (cd == NULL) {
        mln_log(error, "No 'main' domain.\n");
        abort();
    }
    mln_conf_cmd_t *cc = cd->search(cd, "log_output_level");
    if (cc == NULL) return 0;
    mln_conf_item_t *ci = cc->search(cc, 1);
    if (ci == NULL) {
        mln_log(error, "Command 'log_output_level' need a parameter.\n");
        return -1;
    }
    if (ci->type != CONF_STR) {
        mln_log(error, "Type of command [log_output_level]'s parameter error.\n");
        return -1;
    }
    if (!mln_const_strcmp(ci->val.s, "none")) {
        allow_level = none;
    } else if (!mln_const_strcmp(ci->val.s, "report")) {
        allow_level = report;
    } else if (!mln_const_strcmp(ci->val.s, "debug")) {
        allow_level = debug;
    } else if (!mln_const_strcmp(ci->val.s, "error")) {
        allow_level = error;
    } else if (!mln_const_strcmp(ci->val.s, "nolog")) {
        allow_level = nolog;
    } else {
        mln_log(error, "Value of command [log_output_level]'s parameter error.\n");
        return -1;
    }
    return 0;
}

static void
mln_log_proc_lock(void)
{
    struct flock fl;
    memset(&fl, 0, sizeof(fl));
    fl.l_type = F_WRLCK;
    fl.l_start = 0;
    fl.l_whence = SEEK_SET;
    fl.l_len = 0;
    fcntl(log_fd, F_SETLKW, &fl);
}

static void
mln_log_proc_unlock(void)
{
    struct flock fl;
    memset(&fl, 0, sizeof(fl));
    fl.l_type = F_UNLCK;
    fl.l_start = 0;
    fl.l_whence = SEEK_SET;
    fl.l_len = 0;
    fcntl(log_fd, F_SETLKW, &fl); 
}

void _mln_sys_log(enum sys_log level, \
                  const char *file, \
                  const char *func, \
                  int line, \
                  char *msg, \
                  ...)
{
    MLN_LOCK(&log_lock);
    mln_log_proc_lock();
    va_list arg;
    va_start(arg, msg);
    _mln_sys_log_process(level, file, func, line, msg, arg);
    va_end(arg);
    mln_log_proc_unlock();
    MLN_UNLOCK(&log_lock);
}

static void
_mln_sys_log_process(enum sys_log level, \
                     const char *file, \
                     const char *func, \
                     int line, \
                     char *msg, \
                     va_list arg)
{
    if (level < allow_level) return;
    int n;
    struct timeval tv;
    struct timezone tz;
    struct localtime_s lc;
    gettimeofday(&tv, &tz);
    mln_get_localtime(&tv, &tz, &lc);
    char line_str[256] = {0};
    if (level > none) {
        n = snprintf(line_str, sizeof(line_str)-1, \
                         "%ld/%02ld/%02ld %02ld:%02ld:%02ld ", \
                         lc.year, lc.month, lc.day, \
                         lc.hour, lc.minute, lc.second);
        write(log_fd, line_str, n);
#if !defined(linux)
        write(log_fd, "GMT ", 4);
#endif
    }
    switch (level) {
        case none:
            break;
        case report:
            write(log_fd, "REPORT: ", 8);
            break;
        case debug:
            write(log_fd, "DEBUG: ", 7);
            break;
        case error:
            write(log_fd, "ERROR: ", 7);
            break;
        default: 
            write(log_fd, log_err_level, sizeof(log_err_level)-1);
            return ;
    }
    if (level == debug || level == error) {
        memset(line_str, 0, sizeof(line_str));
        write(log_fd, file, strlen(file));
        write(log_fd, ":", 1);
        write(log_fd, func, strlen(func));
        write(log_fd, ":", 1);
        n = snprintf(line_str, sizeof(line_str)-1, "%d", line);
        write(log_fd, line_str, n);
        write(log_fd, ": ", 2);
    }
    
    memset(line_str, 0, sizeof(line_str));
    n = snprintf(line_str, sizeof(line_str)-1, "PID:%d ", getpid());
    write(log_fd, line_str, n);

    int cnt = 0;
    char *p = msg;
    while (*msg != 0) {
        if (*msg != '%') {
            cnt++;
            msg++;
            continue;
        }
        write(log_fd, p, cnt);
        cnt = 0;
        msg++;
        p = msg + 1;
        switch (*msg) {
            case 's':
            {
                char *s = va_arg(arg, char *);
                write(log_fd, s, strlen(s));
                break;
            }
            case 'l':
            {
                memset(line_str, 0, sizeof(line_str));
                mln_sauto_t num = va_arg(arg, long);
                int n = snprintf(line_str, sizeof(line_str)-1, "%ld", num);
                write(log_fd, line_str, n);
                break;
            }
            case 'd':
            {
                memset(line_str, 0, sizeof(line_str));
                int num = va_arg(arg, int);
                int n = snprintf(line_str, sizeof(line_str)-1, "%d", num);
                write(log_fd, line_str, n);
                break;
            }
            case 'c':
            {
                int ch = va_arg(arg, int);
                write(log_fd, &ch, 1);
                break;
            }
            case 'f':
            {
                double f = va_arg(arg, double);
                memset(line_str, 0, sizeof(line_str));
                int n = snprintf(line_str, sizeof(line_str)-1, "%f", f);
                write(log_fd, line_str, n);
                break;
            }
            case 'x':
            {
                memset(line_str, 0, sizeof(line_str));
                int num = va_arg(arg, int);
                int n = snprintf(line_str, sizeof(line_str)-1, "%x", num);
                write(log_fd, line_str, n);
                break;
            }
            case 'X':
            {
                memset(line_str, 0, sizeof(line_str));
                long num = va_arg(arg, long);
                int n = snprintf(line_str, sizeof(line_str)-1, "%lx", num);
                write(log_fd, line_str, n);
                break;
            }
            case 'u':
            {
                memset(line_str, 0, sizeof(line_str));
                unsigned int num = va_arg(arg, unsigned int);
                int n = snprintf(line_str, sizeof(line_str)-1, "%u", num);
                write(log_fd, line_str, n);
                break;
            }
            case 'U':
            {
                memset(line_str, 0, sizeof(line_str));
                unsigned long num = va_arg(arg, unsigned long);
                int n = snprintf(line_str, sizeof(line_str)-1, "%lu", num);
                write(log_fd, line_str, n);
                break;
            }
            default:
                write(log_fd, log_err_fmt, sizeof(log_err_fmt)-1);
                write(log_fd, "\n", 1);
                return;
        }
        msg++;
    }
    if (cnt)
        write(log_fd, p, cnt);
}

int mln_get_log_fd(void)
{
    if (log_in_daemon) return log_fd;
    return -1;
}

char *mln_get_log_dir_path(void)
{
    return mln_log_dirpath;
}

char *mln_get_log_file_path(void)
{
    return mln_log_filepath;
}

char *mln_get_pid_file_path(void)
{
    return mln_pid_filepath;
}

