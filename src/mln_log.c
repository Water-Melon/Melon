
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
#include "mln_log.h"
#include "mln_conf.h"
#include "mln_path.h"

/*
 * declarations
 */
static void
_mln_sys_log_process(mln_log_t *log, \
                     enum log_level level, \
                     const char *file, \
                     const char *func, \
                     int line, \
                     char *msg, \
                     va_list arg);
static void mln_file_lock(int fd);
static void mln_file_unlock(int fd);
static ssize_t mln_log_write(mln_log_t *log, void *buf, mln_size_t size);

/*
 * global variables
 */
long mon_days[2][12] = {
    {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
    {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}
};
char log_err_level[] = "Log level permission deny.";
char log_err_fmt[] = "Log message format error.";
mln_log_t gLog;

/*
 * file lock
 */
static void mln_file_lock(int fd)
{
    struct flock fl;
    memset(&fl, 0, sizeof(fl));
    fl.l_type = F_WRLCK;
    fl.l_start = 0;
    fl.l_whence = SEEK_SET;
    fl.l_len = 0;
    fcntl(fd, F_SETLKW, &fl);
}

static void mln_file_unlock(int fd)
{
    struct flock fl;
    memset(&fl, 0, sizeof(fl));
    fl.l_type = F_UNLCK;
    fl.l_start = 0;
    fl.l_whence = SEEK_SET;
    fl.l_len = 0;
    fcntl(fd, F_SETLKW, &fl); 
}


/*
 * time
 */
static inline int
mln_is_leap(long year)
{
    if (((year%4 == 0) && (year%100 != 0)) || (year%400 == 0))
        return 1;
    return 0;
}

static void
mln_get_localtime(struct timeval *tv, struct localtime_s *lc)
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
    lc->minute = (subsec % 3600) / 60;
    lc->second = (subsec % 3600) % 60;
}

/*
 * gLog
 */
int mln_log_init(int in_daemon)
{
    mln_log_t *log = &gLog;
    char *ab_path = mln_get_path();
    memset(log->dir_path, 0, M_LOG_PATH_LEN);
    snprintf(log->dir_path, M_LOG_PATH_LEN-1, "%s/logs", ab_path);
    memset(log->pid_path, 0, M_LOG_PATH_LEN);
    snprintf(log->pid_path, M_LOG_PATH_LEN-1, "%s/logs/melon.pid", ab_path);
    memset(log->log_path, 0, M_LOG_PATH_LEN);
    snprintf(log->log_path, M_LOG_PATH_LEN-1, "%s/logs/melon.log", ab_path);

    if (mkdir(log->dir_path, S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH) < 0) {
        if (errno != EEXIST) return -1;
    }

    int pid_fd = open(log->pid_path, O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
    if (pid_fd < 0) {
        fprintf(stderr, "%s(): open pid file failed. %s\n", __FUNCTION__, strerror(errno));
        return -1;
    }
    char pid_str[64] = {0};
    snprintf(pid_str, sizeof(pid_str)-1, "%lu", (unsigned long)getpid());
    mln_file_lock(pid_fd);
    write(pid_fd, pid_str, strlen(pid_str));
    mln_file_unlock(pid_fd);
    close(pid_fd);

    log->fd = open(log->log_path, O_WRONLY|O_CREAT|O_APPEND, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
    if (log->fd < 0) {
        fprintf(stderr, "%s(): open log file failed. %s\n", __FUNCTION__, strerror(errno));
        return -1;
    }
    log->in_daemon = in_daemon;
    log->level = none;
    int ret = 0;
    if ((ret = MLN_LOCK_INIT(&(log->thread_lock))) != 0) {
        close(log->fd);
        return -1;
    }
    return 0;
}

void mln_log_destroy(void)
{
    mln_log_t *log = &gLog;
    close(log->fd);
    MLN_LOCK_DESTROY(&(log->thread_lock));
}

/*
 * mln_log_set_level
 */
int mln_log_set_level(void)
{
    mln_log_t *log = &gLog;
    mln_conf_t *cf = mln_get_conf();
    if (cf == NULL) return 0;
    mln_conf_domain_t *cd = cf->search(cf, "main");
    if (cd == NULL) {
        mln_log(error, "No 'main' domain.\n");
        abort();
    }
    mln_conf_cmd_t *cc = cd->search(cd, "log_level");
    if (cc == NULL) return 0;
    mln_conf_item_t *ci = cc->search(cc, 1);
    if (ci == NULL) {
        mln_log(error, "Command 'log_level' need a parameter.\n");
        return -1;
    }
    if (ci->type != CONF_STR) {
        mln_log(error, "Parameter type of command [log_level] error.\n");
        return -1;
    }
    if (!mln_const_strcmp(ci->val.s, "none")) {
        log->level = none;
    } else if (!mln_const_strcmp(ci->val.s, "report")) {
        log->level = report;
    } else if (!mln_const_strcmp(ci->val.s, "debug")) {
        log->level = debug;
    } else if (!mln_const_strcmp(ci->val.s, "error")) {
        log->level = error;
    } else {
        mln_log(error, "Parameter value of command [log_level] error.\n");
        return -1;
    }
    return 0;
}

/*
 * log_reload
 */
int mln_log_reload(void *data)
{
    MLN_LOCK(&(gLog.thread_lock));
    mln_file_lock(gLog.fd);
    int ret = mln_log_set_level();
    mln_file_unlock(gLog.fd);
    MLN_UNLOCK(&(gLog.thread_lock));
    return ret;
}

/*
 * reocrd log
 */
void _mln_sys_log(enum log_level level, \
                  const char *file, \
                  const char *func, \
                  int line, \
                  char *msg, \
                  ...)
{
    MLN_LOCK(&(gLog.thread_lock));
    mln_file_lock(gLog.fd);
    va_list arg;
    va_start(arg, msg);
    _mln_sys_log_process(&gLog, level, file, func, line, msg, arg);
    va_end(arg);
    mln_file_unlock(gLog.fd);
    MLN_UNLOCK(&(gLog.thread_lock));
}

static ssize_t mln_log_write(mln_log_t *log, void *buf, mln_size_t size)
{
    ssize_t ret = write(log->fd, buf, size);
    if (!log->in_daemon)
        write(STDERR_FILENO, buf, size);
    return ret;
}

static void
_mln_sys_log_process(mln_log_t *log, \
                     enum log_level level, \
                     const char *file, \
                     const char *func, \
                     int line, \
                     char *msg, \
                     va_list arg)
{
    if (level < log->level) return;
    int n;
    struct timeval tv;
    struct localtime_s lc;
    gettimeofday(&tv, NULL);
    mln_get_localtime(&tv, &lc);
    char line_str[256] = {0};
    if (level > none) {
        n = snprintf(line_str, sizeof(line_str)-1, \
                         "%02ld/%02ld/%ld %02ld:%02ld:%02ld GMT ", \
                         lc.month, lc.day, lc.year, \
                         lc.hour, lc.minute, lc.second);
        mln_log_write(log, (void *)line_str, n);
    }
    switch (level) {
        case none:
            break;
        case report:
            mln_log_write(log, (void *)"REPORT: ", 8);
            break;
        case debug:
            mln_log_write(log, (void *)"DEBUG: ", 7);
            break;
        case error:
            mln_log_write(log, (void *)"ERROR: ", 7);
            break;
        default: 
            return ;
    }
    if (level >= debug) {
        memset(line_str, 0, sizeof(line_str));
        mln_log_write(log, (void *)file, strlen(file));
        mln_log_write(log, (void *)":", 1);
        mln_log_write(log, (void *)func, strlen(func));
        mln_log_write(log, (void *)":", 1);
        n = snprintf(line_str, sizeof(line_str)-1, "%d", line);
        mln_log_write(log, (void *)line_str, n);
        mln_log_write(log, (void *)": ", 2);
    }
    
    if (level > none) {
        memset(line_str, 0, sizeof(line_str));
        n = snprintf(line_str, sizeof(line_str)-1, "PID:%d ", getpid());
        mln_log_write(log, (void *)line_str, n);
    }

    int cnt = 0;
    char *p = msg;
    while (*msg != 0) {
        if (*msg != '%') {
            cnt++;
            msg++;
            continue;
        }
        mln_log_write(log, (void *)p, cnt);
        cnt = 0;
        msg++;
        p = msg + 1;
        switch (*msg) {
            case 's':
            {
                char *s = va_arg(arg, char *);
                mln_log_write(log, (void *)s, strlen(s));
                break;
            }
            case 'l':
            {
                memset(line_str, 0, sizeof(line_str));
                mln_sauto_t num = va_arg(arg, long);
                int n = snprintf(line_str, sizeof(line_str)-1, "%ld", num);
                mln_log_write(log, (void *)line_str, n);
                break;
            }
            case 'd':
            {
                memset(line_str, 0, sizeof(line_str));
                int num = va_arg(arg, int);
                int n = snprintf(line_str, sizeof(line_str)-1, "%d", num);
                mln_log_write(log, (void *)line_str, n);
                break;
            }
            case 'c':
            {
                int ch = va_arg(arg, int);
                mln_log_write(log, (void *)&ch, 1);
                break;
            }
            case 'f':
            {
                double f = va_arg(arg, double);
                memset(line_str, 0, sizeof(line_str));
                int n = snprintf(line_str, sizeof(line_str)-1, "%f", f);
                mln_log_write(log, (void *)line_str, n);
                break;
            }
            case 'x':
            {
                memset(line_str, 0, sizeof(line_str));
                int num = va_arg(arg, int);
                int n = snprintf(line_str, sizeof(line_str)-1, "%x", num);
                mln_log_write(log, (void *)line_str, n);
                break;
            }
            case 'X':
            {
                memset(line_str, 0, sizeof(line_str));
                long num = va_arg(arg, long);
                int n = snprintf(line_str, sizeof(line_str)-1, "%lx", num);
                mln_log_write(log, (void *)line_str, n);
                break;
            }
            case 'u':
            {
                memset(line_str, 0, sizeof(line_str));
                unsigned int num = va_arg(arg, unsigned int);
                int n = snprintf(line_str, sizeof(line_str)-1, "%u", num);
                mln_log_write(log, (void *)line_str, n);
                break;
            }
            case 'U':
            {
                memset(line_str, 0, sizeof(line_str));
                unsigned long num = va_arg(arg, unsigned long);
                int n = snprintf(line_str, sizeof(line_str)-1, "%lu", num);
                mln_log_write(log, (void *)line_str, n);
                break;
            }
            default:
                mln_log_write(log, (void *)log_err_fmt, sizeof(log_err_fmt)-1);
                mln_log_write(log, (void *)"\n", 1);
                return;
        }
        msg++;
    }
    if (cnt)
        mln_log_write(log, (void *)p, cnt);
}

/*
 * get
 */
int mln_log_get_fd(void)
{
    return gLog.fd;
}

char *mln_log_get_dir_path(void)
{
    return gLog.dir_path;
}

char *mln_log_get_log_path(void)
{
    return gLog.log_path;
}

char *mln_log_get_pid_path(void)
{
    return gLog.pid_path;
}

