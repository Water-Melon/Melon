
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
#include "mln_tools.h"

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
static int mln_log_set_level(mln_log_t *log, int is_init);
static ssize_t mln_log_write(mln_log_t *log, void *buf, mln_size_t size);
static void mln_log_atfork_lock(void);
static void mln_log_atfork_unlock(void);
static int mln_log_get_log(mln_log_t *log, int is_init);

/*
 * global variables
 */
char log_err_level[] = "Log level permission deny.";
char log_err_fmt[] = "Log message format error.";
char log_path_cmd[] = "log_path";
mln_log_t gLog = {{0},{0},{0},STDERR_FILENO,0,none,(mln_lock_t)0};

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
 * gLog
 */
int mln_log_init(int in_daemon)
{
    mln_log_t *log = &gLog;
    char *ab_path, path[M_LOG_PATH_LEN];

    ab_path = mln_path();

    memset(path, 0, M_LOG_PATH_LEN);
    snprintf(path, M_LOG_PATH_LEN-1, "%s/logs", ab_path);
    if (mkdir(path, S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH) < 0) {
        if (errno != EEXIST) {
            fprintf(stderr, "mkdir '%s' failed. %s\n", path, strerror(errno));
            return -1;
        }
    }

    memset(log->pid_path, 0, M_LOG_PATH_LEN);
    snprintf(log->pid_path, M_LOG_PATH_LEN-1, "%s/logs/melon.pid", ab_path);
    int fd = open(log->pid_path, O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
    if (fd < 0) {
        fprintf(stderr, "%s(): open pid file failed. %s\n", __FUNCTION__, strerror(errno));
        return -1;
    }
    char pid_str[64] = {0};
    snprintf(pid_str, sizeof(pid_str)-1, "%lu", (unsigned long)getpid());
    mln_file_lock(fd);
    write(fd, pid_str, strlen(pid_str));
    mln_file_unlock(fd);
    close(fd);

    memset(path, 0, M_LOG_PATH_LEN);
    snprintf(path, M_LOG_PATH_LEN-1, "%s/logs/melon.log", ab_path);
    fd = open(path, O_WRONLY|O_CREAT|O_APPEND, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
    if (fd < 0) {
        fprintf(stderr, "%s(): open log file failed. %s\n", __FUNCTION__, strerror(errno));
        return -1;
    }
    close(fd);

    log->in_daemon = in_daemon;
    log->level = none;
    int ret = 0;
    if ((ret = MLN_LOCK_INIT(&(log->thread_lock))) != 0) {
        fprintf(stderr, "%s(): Init log's thread_lock failed. %s\n", __FUNCTION__, strerror(ret));
        return -1;
    }
    if ((ret = pthread_atfork(mln_log_atfork_lock, \
                              mln_log_atfork_unlock, \
                              mln_log_atfork_unlock)) != 0)
    {
        fprintf(stderr, "%s(): pthread_atfork failed. %s\n", __FUNCTION__, strerror(ret));
        MLN_LOCK_DESTROY(&(log->thread_lock));
        return -1;
    }

    if (mln_log_get_log(log, 1) < 0) {
        fprintf(stderr, "%s(): Get log file failed.\n", __FUNCTION__);
        mln_log_destroy();
        return -1;
    }

    if (mln_log_set_level(log, 1) < 0) {
        fprintf(stderr, "%s(): Set log level failed.\n", __FUNCTION__);
        mln_log_destroy();
        return -1;
    }
    return 0;
}

static int
mln_log_get_log(mln_log_t *log, int is_init)
{
    mln_conf_t *cf;
    mln_conf_domain_t *cd;
    mln_conf_cmd_t *cc;
    mln_conf_item_t *ci;
    char *path_str = NULL;
    mln_u32_t path_len = 0;
    char buf[M_LOG_PATH_LEN] = {0}, *p;
    char default_dir[] = "logs", default_file[] = "melon.log";
    int fd;

    cf = mln_get_conf();
    cd = cf->search(cf, "main");
    cc = cd->search(cd, log_path_cmd);
    if (cc == NULL) {
        path_len = snprintf(buf, sizeof(buf)-1, "%s/%s/%s", \
                            mln_path(), default_dir, default_file);
        path_str = buf;
    } else {
        if (mln_conf_get_argNum(cc) != 1) {
            fprintf(stderr, "%s(): Invalid command '%s' in domain 'main'.\n", \
                    __FUNCTION__, log_path_cmd);
            return -1;
        }
        ci = cc->search(cc, 1);
        if (ci->type != CONF_STR) {
            fprintf(stderr, "%s(): Invalid command '%s' in domain 'main'.\n", \
                    __FUNCTION__, log_path_cmd);
            return -1;
        }
        if ((ci->val.s->data)[0] != '/') {
            path_len = snprintf(buf, sizeof(buf)-1, "%s/%s", \
                                mln_path(), (char *)(ci->val.s->data));
            path_str = buf;
        } else {
            path_len = ci->val.s->len > M_LOG_PATH_LEN-1? M_LOG_PATH_LEN-1: ci->val.s->len;
            path_str = (char *)(ci->val.s->data);
        }
    }

    fd = open(path_str, O_WRONLY|O_CREAT|O_APPEND, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
    if (fd < 0) {
        fprintf(stderr, "%s(): open '%s' failed. %s\n", __FUNCTION__, path_str, strerror(errno));
        return -1;
    }

    if (!is_init && \
        log->fd > 0 && \
        log->fd != STDIN_FILENO && \
        log->fd != STDOUT_FILENO && \
        log->fd != STDERR_FILENO)
    {
        close(log->fd);
    }

    log->fd = fd;
    memcpy(log->log_path, path_str, path_len);
    log->log_path[path_len] = 0;
    for (p = &(path_str[path_len - 1]); p >= path_str && *p != '/'; --p)
        ;
    memcpy(log->dir_path, path_str, p - path_str);
    log->dir_path[p - path_str] = 0;

    return 0;
}

static void mln_log_atfork_lock(void)
{
    MLN_LOCK(&(gLog.thread_lock));
}

static void mln_log_atfork_unlock(void)
{
    MLN_UNLOCK(&(gLog.thread_lock));
}

void mln_log_destroy(void)
{
    mln_log_t *log = &gLog;
    if (log->fd > 0 && \
        log->fd != STDIN_FILENO && \
        log->fd != STDOUT_FILENO && \
        log->fd != STDERR_FILENO)
    {
        close(log->fd);
    }
    MLN_LOCK_DESTROY(&(log->thread_lock));
}

/*
 * mln_log_set_level
 */
static int mln_log_set_level(mln_log_t *log, int is_init)
{
    mln_conf_t *cf = mln_get_conf();
    if (cf == NULL) return 0;
    mln_conf_domain_t *cd = cf->search(cf, "main");
    if (cd == NULL) {
        if (is_init)
            fprintf(stderr, "No 'main' domain.\n");
        else
            mln_log(error, "No 'main' domain.\n");
        abort();
    }
    mln_conf_cmd_t *cc = cd->search(cd, "log_level");
    if (cc == NULL) return 0;
    mln_conf_item_t *ci = cc->search(cc, 1);
    if (ci == NULL) {
        if (is_init)
            fprintf(stderr, "Command 'log_level' need a parameter.\n");
        else
            mln_log(error, "Command 'log_level' need a parameter.\n");
        return -1;
    }
    if (ci->type != CONF_STR) {
        if (is_init)
            fprintf(stderr, "Parameter type of command 'log_level' error.\n");
        else
            mln_log(error, "Parameter type of command 'log_level' error.\n");
        return -1;
    }
    if (!mln_string_constStrcmp(ci->val.s, "none")) {
        log->level = none;
    } else if (!mln_string_constStrcmp(ci->val.s, "report")) {
        log->level = report;
    } else if (!mln_string_constStrcmp(ci->val.s, "debug")) {
        log->level = debug;
    } else if (!mln_string_constStrcmp(ci->val.s, "warn")) {
        log->level = warn;
    } else if (!mln_string_constStrcmp(ci->val.s, "error")) {
        log->level = error;
    } else {
        if (is_init)
            fprintf(stderr, "Parameter value of command [log_level] error.\n");
        else
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
    mln_log_get_log(&gLog, 0);
    mln_file_lock(gLog.fd);
    int ret = mln_log_set_level(&gLog, 0);
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
    struct UTCTime_s uc;
    gettimeofday(&tv, NULL);
    mln_UTCTime(tv.tv_sec, &uc);
    char line_str[256] = {0};
    if (level > none) {
        n = snprintf(line_str, sizeof(line_str)-1, \
                         "%02ld/%02ld/%ld %02ld:%02ld:%02ld GMT ", \
                         uc.month, uc.day, uc.year, \
                         uc.hour, uc.minute, uc.second);
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
        case warn:
            mln_log_write(log, (void *)"WARN: ", 6);
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
            ++cnt;
            ++msg;
            continue;
        }
        mln_log_write(log, (void *)p, cnt);
        cnt = 0;
        ++msg;
        p = msg + 1;
        switch (*msg) {
            case 's':
            {
                char *s = va_arg(arg, char *);
                mln_log_write(log, (void *)s, strlen(s));
                break;
            }
            case 'S':
            {
                mln_string_t *s = va_arg(arg, mln_string_t *);
                mln_log_write(log, s->data, s->len);
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
        ++msg;
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

char *mln_log_getDirPath(void)
{
    return gLog.dir_path;
}

char *mln_log_getLogPath(void)
{
    return gLog.log_path;
}

char *mln_log_getPidPath(void)
{
    return gLog.pid_path;
}

