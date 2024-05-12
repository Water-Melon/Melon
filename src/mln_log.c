
/*
 * Copyright (C) Niklaus F.Schen.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#if !defined(MSVC)
#include <unistd.h>
#include <sys/time.h>
#else
#include "mln_utils.h"
#endif
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include "mln_log.h"
#include "mln_path.h"
#include "mln_tools.h"

/*
 * declarations
 */
static void
_mln_sys_log_process(mln_log_t *log, \
                     mln_log_level_t level, \
                     const char *file, \
                     const char *func, \
                     int line, \
                     char *msg, \
                     va_list arg);
static inline void mln_file_lock(int fd);
static inline void mln_file_unlock(int fd);
static int mln_log_set_level(mln_log_t *log, mln_conf_t *cf, int is_init);
static inline int mln_log_write(mln_log_t *log, void *buf, mln_size_t size);
#if !defined(MSVC)
static void mln_log_atfork_lock(void);
static void mln_log_atfork_unlock(void);
#endif
static int mln_log_get_log(mln_log_t *log, mln_conf_t *cf, int is_init);
static mln_logger_t _logger = _mln_sys_log_process;

/*
 * global variables
 */
char log_err_level[] = "Log level permission deny.";
char log_err_fmt[] = "Log message format error.";
char log_path_cmd[] = "log_path";
#if defined(MSVC)
mln_log_t g_log = {2, 0, 0, 0, none, {0},{0},{0}};
#else
mln_log_t g_log = {(mln_spin_t)0, STDERR_FILENO, 0, 0, 0, none, {0},{0},{0}};
#endif

/*
 * file lock
 */
static inline void mln_file_lock(int fd)
{
#if !defined(MSVC)
    struct flock fl;
    memset(&fl, 0, sizeof(fl));
    fl.l_type = F_WRLCK;
    fl.l_start = 0;
    fl.l_whence = SEEK_SET;
    fl.l_len = 0;
    fcntl(fd, F_SETLKW, &fl);
#endif
}

static inline void mln_file_unlock(int fd)
{
#if !defined(MSVC)
    struct flock fl;
    memset(&fl, 0, sizeof(fl));
    fl.l_type = F_UNLCK;
    fl.l_start = 0;
    fl.l_whence = SEEK_SET;
    fl.l_len = 0;
    fcntl(fd, F_SETLKW, &fl); 
#endif
}

/*
 * set/get logger
 */
void mln_log_logger_set(mln_logger_t logger)
{
    _logger = logger;
}

mln_logger_t mln_log_logger_get(void)
{
    return _logger;
}

/*
 * g_log
 */
int mln_log_init(mln_conf_t *cf)
{
    mln_u32_t in_daemon = 0, init = 0;
    mln_conf_domain_t *cd;
    mln_conf_cmd_t *cc;
    mln_conf_item_t *ci;
    mln_log_t *log = &g_log;
    if (log->init) return 0;

    if (cf == NULL) cf = mln_conf();
    if (mln_conf_is_empty(cf))
        fprintf(stdout, "[WARN] Load configuration failed. Logger won't be initialized.\n");
    else {
        if ((cd = cf->search(cf, "main")) == NULL) {
            fprintf(stderr, "No such domain named 'main'\n");
            return -1;
        }
        if ((cc = cd->search(cd, "daemon")) != NULL) {
            if ((ci = cc->search(cc, 1)) == NULL) {
                fprintf(stderr, "Command 'daemon' need a parameter.\n");
                return -1;
            }
            if (ci->type != CONF_BOOL) {
                fprintf(stderr, "Parameter type of command 'daemon' error.\n");
                return -1;
            }
            if (ci->val.b) in_daemon = 1;
        }
        init = 1;
    }

    log->in_daemon = in_daemon;
    log->init = init;
    log->level = none;
    int ret = 0;
#if !defined(MSVC)
    if ((ret = mln_spin_init(&(log->thread_lock))) != 0) {
        fprintf(stderr, "%s(): Init log's thread_lock failed. %s\n", __FUNCTION__, strerror(ret));
        return -1;
    }
    if ((ret = pthread_atfork(mln_log_atfork_lock, \
                              mln_log_atfork_unlock, \
                              mln_log_atfork_unlock)) != 0)
    {
        fprintf(stderr, "%s(): pthread_atfork failed. %s\n", __FUNCTION__, strerror(ret));
        mln_spin_destroy(&(log->thread_lock));
        return -1;
    }
#endif

    if (mln_log_get_log(log, cf, 1) < 0) {
        fprintf(stderr, "%s(): Get log file failed.\n", __FUNCTION__);
        mln_log_destroy();
        return -1;
    }

    if (mln_log_set_level(log, cf, 1) < 0) {
        fprintf(stderr, "%s(): Set log level failed.\n", __FUNCTION__);
        mln_log_destroy();
        return -1;
    }
    return 0;
}

static int
mln_log_get_log(mln_log_t *log, mln_conf_t *cf, int is_init)
{
    if (mln_conf_is_empty(cf)) return 0;

    mln_conf_domain_t *cd;
    mln_conf_cmd_t *cc;
    mln_conf_item_t *ci;
    char *path_str = NULL;
    mln_u32_t path_len = 0;
    char buf[M_LOG_PATH_LEN] = {0}, *p;
    int fd;

    cd = cf->search(cf, "main");
    cc = cd->search(cd, log_path_cmd);
    if (cc == NULL) {
        path_len = snprintf(buf, sizeof(buf)-1, "%s", mln_path_log());
        path_str = buf;
    } else {
        if (mln_conf_arg_num(cc) != 1) {
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
#if defined(MSVC)
        if (ci->val.s->len <= 1 || (ci->val.s->data)[1] != ':') {
#else
        if ((ci->val.s->data)[0] != '/' && (ci->val.s->data)[0] != '.') {
#endif
            path_len = snprintf(buf, sizeof(buf)-1, "%s/%s", mln_path(), (char *)(ci->val.s->data));
            path_str = buf;
        } else {
            path_len = ci->val.s->len > M_LOG_PATH_LEN-1? M_LOG_PATH_LEN-1: ci->val.s->len;
            path_str = (char *)(ci->val.s->data);
        }
    }
    for (p = &(path_str[path_len - 1]); p >= path_str && *p != '/'; --p)
        ;
    memcpy(log->dir_path, path_str, p - path_str);
    log->dir_path[p - path_str] = 0;
#if defined(MSVC)
    if (mkdir(log->dir_path) < 0) {
#else
    if (mkdir(log->dir_path, S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH) < 0) {
#endif
        if (errno != EEXIST) {
            fprintf(stderr, "mkdir '%s' failed. %s\n", log->dir_path, strerror(errno));
            return -1;
        }
    }

#if defined(MSVC)
    fd = open(path_str, O_WRONLY|O_CREAT|O_APPEND, _S_IREAD|_S_IWRITE);
#else
    fd = open(path_str, O_WRONLY|O_CREAT|O_APPEND, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
#endif
    if (fd < 0) {
        fprintf(stderr, "%s(): open '%s' failed. %s\n", __FUNCTION__, path_str, strerror(errno));
        return -1;
    }
#if defined(MSVC)
    if (!is_init && \
        log->fd > 0 && \
        log->fd != _fileno(stdin) && \
        log->fd != _fileno(stdout) && \
        log->fd != _fileno(stderr))
#else
    if (!is_init && \
        log->fd > 0 && \
        log->fd != STDIN_FILENO && \
        log->fd != STDOUT_FILENO && \
        log->fd != STDERR_FILENO)
#endif
    {
        close(log->fd);
    }
    log->fd = fd;
    memcpy(log->log_path, path_str, path_len);
    log->log_path[path_len] = 0;

    if (is_init) {
        memset(log->pid_path, 0, M_LOG_PATH_LEN);
        snprintf(log->pid_path, M_LOG_PATH_LEN-1, "%s/melon.pid", log->dir_path);
#if defined(MSVC)
        fd = open(log->pid_path, O_WRONLY|O_CREAT|O_TRUNC, _S_IREAD|_S_IWRITE);
#else
        fd = open(log->pid_path, O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
#endif
        if (fd < 0) {
            fprintf(stderr, "%s(): open pid file failed. %s\n", __FUNCTION__, strerror(errno));
            return -1;
        }
        char pid_str[64] = {0};
        snprintf(pid_str, sizeof(pid_str)-1, "%lu", (unsigned long)getpid());
        mln_file_lock(fd);
        int rc = write(fd, pid_str, strlen(pid_str));
        if (rc <= 0) rc = 1;/*do nothing*/
        mln_file_unlock(fd);
        close(fd);
    }
    return 0;
}

#if !defined(MSVC)
static void mln_log_atfork_lock(void)
{
    mln_spin_lock(&(g_log.thread_lock));
}

static void mln_log_atfork_unlock(void)
{
    mln_spin_unlock(&(g_log.thread_lock));
}
#endif

void mln_log_destroy(void)
{
    mln_log_t *log = &g_log;
#if defined(MSVC)
    if (log->fd > 0 && \
        log->fd != _fileno(stdin) && \
        log->fd != _fileno(stdout) && \
        log->fd != _fileno(stderr))
#else
    if (log->fd > 0 && \
        log->fd != STDIN_FILENO && \
        log->fd != STDOUT_FILENO && \
        log->fd != STDERR_FILENO)
#endif
    {
        close(log->fd);
    }
#if !defined(MSVC)
    mln_spin_destroy(&(log->thread_lock));
#endif
}

/*
 * mln_log_set_level
 */
static int mln_log_set_level(mln_log_t *log, mln_conf_t *cf, int is_init)
{
    if (mln_conf_is_empty(cf)) return 0;

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
    if (!mln_string_const_strcmp(ci->val.s, "none")) {
        log->level = none;
    } else if (!mln_string_const_strcmp(ci->val.s, "report")) {
        log->level = report;
    } else if (!mln_string_const_strcmp(ci->val.s, "debug")) {
        log->level = debug;
    } else if (!mln_string_const_strcmp(ci->val.s, "warn")) {
        log->level = warn;
    } else if (!mln_string_const_strcmp(ci->val.s, "error")) {
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
#if !defined(MSVC)
    mln_spin_lock(&(g_log.thread_lock));
#endif
    mln_log_get_log(&g_log, mln_conf(), 0);
    mln_file_lock(g_log.fd);
    int ret = mln_log_set_level(&g_log, mln_conf(), 0);
    mln_file_unlock(g_log.fd);
#if !defined(MSVC)
    mln_spin_unlock(&(g_log.thread_lock));
#endif
    return ret;
}

/*
 * reocrd log
 */
void _mln_sys_log(mln_log_level_t level, \
                  const char *file, \
                  const char *func, \
                  int line, \
                  char *msg, \
                  ...)
{
#if !defined(MSVC)
    mln_spin_lock(&(g_log.thread_lock));
#endif
    mln_file_lock(g_log.fd);
    va_list arg;
    va_start(arg, msg);
    if (_logger != NULL)
        _logger(&g_log, level, file, func, line, msg, arg);
    va_end(arg);
    mln_file_unlock(g_log.fd);
#if !defined(MSVC)
    mln_spin_unlock(&(g_log.thread_lock));
#endif
}

static inline int mln_log_write(mln_log_t *log, void *buf, mln_size_t size)
{
    int ret = write(log->fd, buf, size);
    if (log->init && !log->in_daemon) {
#if defined(MSVC)
        ret = write(_fileno(stderr), buf, size);
#else
        ret = write(STDERR_FILENO, buf, size);
#endif
    }
    return ret;
}

static inline int mln_log_level_write(mln_log_t *log, mln_log_level_t level)
{
    int ret = 0;

    switch (level) {
        case report:
            ret = write(log->fd, (void *)"REPORT: ", 8);
            if (log->init && !log->in_daemon)
#if defined(MSVC)
                ret = write(_fileno(stderr), (void *)"REPORT: ", 17);
#else
                ret = write(STDERR_FILENO, (void *)"\e[34mREPORT\e[0m: ", 17);
#endif
            break;
        case debug:
            ret = write(log->fd, (void *)"DEBUG: ", 7);
            if (log->init && !log->in_daemon)
#if defined(MSVC)
                ret = write(_fileno(stderr), (void *)"DEBUG: ", 16);
#else
                ret = write(STDERR_FILENO, (void *)"\e[32mDEBUG\e[0m: ", 16);
#endif
            break;
        case warn:
            ret = write(log->fd, (void *)"WARN: ", 6);
            if (log->init && !log->in_daemon)
#if defined(MSVC)
                ret = write(_fileno(stderr), (void *)"WARN: ", 15);
#else
                ret = write(STDERR_FILENO, (void *)"\e[33mWARN\e[0m: ", 15);
#endif
            break;
        case error:
            ret = write(log->fd, (void *)"ERROR: ", 7);
            if (log->init && !log->in_daemon)
#if defined(MSVC)
                ret = write(_fileno(stderr), (void *)"ERROR: ", 16);
#else
                ret = write(STDERR_FILENO, (void *)"\e[31mERROR\e[0m: ", 16);
#endif
            break;
        default:
            break;
    }
    return ret;
}

int mln_log_writen(void *buf, mln_size_t size)
{
#if !defined(MSVC)
    mln_spin_lock(&(g_log.thread_lock));
#endif
    mln_file_lock(g_log.fd);
    int n = mln_log_write(&g_log, buf, size);
    mln_file_unlock(g_log.fd);
#if !defined(MSVC)
    mln_spin_unlock(&(g_log.thread_lock));
#endif
    return n;
}

static void
_mln_sys_log_process(mln_log_t *log, \
                     mln_log_level_t level, \
                     const char *file, \
                     const char *func, \
                     int line, \
                     char *msg, \
                     va_list arg)
{
    if (level < log->level) return;
    int n;
    struct timeval tv;
    struct utctime uc;
    gettimeofday(&tv, NULL);
    mln_time2utc(tv.tv_sec, &uc);
    char line_str[256] = {0};
    if (level > none) {
        n = snprintf(line_str, sizeof(line_str)-1, \
                         "%02ld/%02ld/%ld %02ld:%02ld:%02ld UTC ", \
                         uc.month, uc.day, uc.year, \
                         uc.hour, uc.minute, uc.second);
        mln_log_write(log, (void *)line_str, n);
    }
    mln_log_level_write(log, level);
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
            case 'i':
            {
                memset(line_str, 0, sizeof(line_str));
#if defined(MSVC) || defined(i386) || defined(__arm__)
                long long num = va_arg(arg, long long);
                int n = snprintf(line_str, sizeof(line_str)-1, "%lld", num);
#else
                long num = va_arg(arg, long);
                int n = snprintf(line_str, sizeof(line_str)-1, "%ld", num);
#endif
                mln_log_write(log, (void *)line_str, n);
                break;
            }
            case 'I':
            {
                memset(line_str, 0, sizeof(line_str));
#if defined(MSVC) || defined(i386) || defined(__arm__)
                unsigned long long num = va_arg(arg, unsigned long long);
                int n = snprintf(line_str, sizeof(line_str)-1, "%llu", num);
#else
                unsigned long num = va_arg(arg, unsigned long);
                int n = snprintf(line_str, sizeof(line_str)-1, "%lu", num);
#endif
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
int mln_log_fd(void)
{
    return g_log.fd;
}

char *mln_log_dir_path(void)
{
    return g_log.dir_path;
}

char *mln_log_logfile_path(void)
{
    return g_log.log_path;
}

char *mln_log_pid_path(void)
{
    return g_log.pid_path;
}

