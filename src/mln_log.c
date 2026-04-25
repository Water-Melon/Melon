
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
#include <sys/uio.h>
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

#define MLN_LOG_BUF_SIZE 4096

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
    int ret = mln_log_set_level(&g_log, mln_conf(), 0);
#if !defined(MSVC)
    mln_spin_unlock(&(g_log.thread_lock));
#endif
    return ret;
}

/*
 * format log message into buffer
 */
static int mln_log_fmt_build(char *buf, int size, int pos, char *msg, va_list arg)
{
    int cnt = 0, n, remain;
    char *q = msg;

    while (*msg != 0 && pos < size - 1) {
        if (*msg != '%') {
            ++cnt;
            ++msg;
            continue;
        }
        if (cnt > 0) {
            remain = size - 1 - pos;
            if (cnt > remain) cnt = remain;
            memcpy(buf + pos, q, cnt);
            pos += cnt;
        }
        cnt = 0;
        ++msg;
        q = msg + 1;
        remain = size - pos;
        switch (*msg) {
            case 's':
            {
                char *s = va_arg(arg, char *);
                n = strlen(s);
                if (n > remain - 1) n = remain - 1;
                memcpy(buf + pos, s, n);
                pos += n;
                break;
            }
            case 'S':
            {
                mln_string_t *s = va_arg(arg, mln_string_t *);
                n = (int)s->len;
                if (n > remain - 1) n = remain - 1;
                memcpy(buf + pos, s->data, n);
                pos += n;
                break;
            }
            case 'l':
            {
                mln_sauto_t num = va_arg(arg, long);
                n = snprintf(buf + pos, remain, "%ld", num);
                if (n > 0 && n < remain) pos += n;
                break;
            }
            case 'd':
            {
                int num = va_arg(arg, int);
                n = snprintf(buf + pos, remain, "%d", num);
                if (n > 0 && n < remain) pos += n;
                break;
            }
            case 'c':
            {
                int ch = va_arg(arg, int);
                if (pos < size - 1) buf[pos++] = (char)ch;
                break;
            }
            case 'f':
            {
                double f = va_arg(arg, double);
                n = snprintf(buf + pos, remain, "%f", f);
                if (n > 0 && n < remain) pos += n;
                break;
            }
            case 'x':
            {
                int num = va_arg(arg, int);
                n = snprintf(buf + pos, remain, "%x", num);
                if (n > 0 && n < remain) pos += n;
                break;
            }
            case 'X':
            {
                long num = va_arg(arg, long);
                n = snprintf(buf + pos, remain, "%lx", num);
                if (n > 0 && n < remain) pos += n;
                break;
            }
            case 'u':
            {
                unsigned int num = va_arg(arg, unsigned int);
                n = snprintf(buf + pos, remain, "%u", num);
                if (n > 0 && n < remain) pos += n;
                break;
            }
            case 'U':
            {
                unsigned long num = va_arg(arg, unsigned long);
                n = snprintf(buf + pos, remain, "%lu", num);
                if (n > 0 && n < remain) pos += n;
                break;
            }
            case 'i':
            {
#if defined(MSVC) || defined(i386) || defined(__arm__)
                long long num = va_arg(arg, long long);
                n = snprintf(buf + pos, remain, "%lld", num);
#else
                long num = va_arg(arg, long);
                n = snprintf(buf + pos, remain, "%ld", num);
#endif
                if (n > 0 && n < remain) pos += n;
                break;
            }
            case 'I':
            {
#if defined(MSVC) || defined(i386) || defined(__arm__)
                unsigned long long num = va_arg(arg, unsigned long long);
                n = snprintf(buf + pos, remain, "%llu", num);
#else
                unsigned long num = va_arg(arg, unsigned long);
                n = snprintf(buf + pos, remain, "%lu", num);
#endif
                if (n > 0 && n < remain) pos += n;
                break;
            }
            default:
                n = snprintf(buf + pos, remain, "%s\n", log_err_fmt);
                if (n > 0 && n < remain) pos += n;
                return pos;
        }
        ++msg;
    }
    if (cnt > 0 && pos < size - 1) {
        int remain = size - 1 - pos;
        if (cnt > remain) cnt = remain;
        memcpy(buf + pos, q, cnt);
        pos += cnt;
    }
    return pos;
}

/*
 * record log
 */
void _mln_sys_log(mln_log_level_t level, \
                  const char *file, \
                  const char *func, \
                  int line, \
                  char *msg, \
                  ...)
{
    if (_logger == NULL) return;
#if !defined(MSVC)
    mln_spin_lock(&(g_log.thread_lock));
#endif
    va_list arg;
    va_start(arg, msg);
    _logger(&g_log, level, file, func, line, msg, arg);
    va_end(arg);
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

int mln_log_writen(void *buf, mln_size_t size)
{
#if !defined(MSVC)
    mln_spin_lock(&(g_log.thread_lock));
#endif
    int n = mln_log_write(&g_log, buf, size);
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

    char buf[MLN_LOG_BUF_SIZE];
    int pos = 0, n;
    int prefix_end = 0, level_end = 0;

    /* timestamp */
    if (level > none) {
        struct timeval tv;
        struct utctime uc;
        gettimeofday(&tv, NULL);
        mln_time2utc(tv.tv_sec, &uc);
        n = snprintf(buf, MLN_LOG_BUF_SIZE - 1, \
                     "%02ld/%02ld/%ld %02ld:%02ld:%02ld UTC ", \
                     uc.month, uc.day, uc.year, \
                     uc.hour, uc.minute, uc.second);
        if (n > 0 && n < MLN_LOG_BUF_SIZE - 1) pos = n;
    }
    prefix_end = pos;

    /* level prefix (plain, for log file) */
    switch (level) {
        case report:
            if (pos + 8 < MLN_LOG_BUF_SIZE) { memcpy(buf + pos, "REPORT: ", 8); pos += 8; }
            break;
        case debug:
            if (pos + 7 < MLN_LOG_BUF_SIZE) { memcpy(buf + pos, "DEBUG: ", 7); pos += 7; }
            break;
        case warn:
            if (pos + 6 < MLN_LOG_BUF_SIZE) { memcpy(buf + pos, "WARN: ", 6); pos += 6; }
            break;
        case error:
            if (pos + 7 < MLN_LOG_BUF_SIZE) { memcpy(buf + pos, "ERROR: ", 7); pos += 7; }
            break;
        default:
            break;
    }
    level_end = pos;

    /* file:func:line */
    if (level >= debug) {
        n = snprintf(buf + pos, MLN_LOG_BUF_SIZE - pos, "%s:%s:%d: ", file, func, line);
        if (n > 0 && n < MLN_LOG_BUF_SIZE - pos) pos += n;
    }

    /* PID */
    if (level > none) {
        n = snprintf(buf + pos, MLN_LOG_BUF_SIZE - pos, "PID:%d ", getpid());
        if (n > 0 && n < MLN_LOG_BUF_SIZE - pos) pos += n;
    }

    /* format message body into buffer */
    pos = mln_log_fmt_build(buf, MLN_LOG_BUF_SIZE, pos, msg, arg);

    /* single write to log file */
    n = write(log->fd, buf, pos);
    if (n < 0) n = 0; /*suppress warning*/

    /* single write to stderr (with color on non-MSVC) */
    if (log->init && !log->in_daemon) {
#if !defined(MSVC)
        if (level > none && level <= error) {
            static const char * const color_levels[] = {
                "",
                "\e[34mREPORT\e[0m: ",
                "\e[32mDEBUG\e[0m: ",
                "\e[33mWARN\e[0m: ",
                "\e[31mERROR\e[0m: ",
            };
            static const int color_lens[] = {0, 17, 16, 15, 16};
            struct iovec iov[3];
            iov[0].iov_base = buf;
            iov[0].iov_len = prefix_end;
            iov[1].iov_base = (void *)color_levels[level];
            iov[1].iov_len = color_lens[level];
            iov[2].iov_base = buf + level_end;
            iov[2].iov_len = pos - level_end;
            n = writev(STDERR_FILENO, iov, 3);
        } else {
            n = write(STDERR_FILENO, buf, pos);
        }
#else
        n = write(_fileno(stderr), buf, pos);
#endif
    }
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
