
/*
 * Copyright (C) Niklaus F.Schen.
 */

#ifndef __MLN_LOG_H
#define __MLN_LOG_H

#include <stdarg.h>
#include "mln_types.h"

#define M_LOG_PATH_LEN 1024

typedef enum {
    none,
    report,
    debug,
    warn,
    error
} mln_log_level_t;

typedef struct {
    char            dir_path[M_LOG_PATH_LEN/2];
    char            pid_path[M_LOG_PATH_LEN];
    char            log_path[M_LOG_PATH_LEN];
    int             fd;
    int             in_daemon;
    mln_log_level_t level;
    mln_spin_t      thread_lock;
} mln_log_t;
    

typedef void (*mln_logger_t)(mln_log_t *, mln_log_level_t, const char *, const char *, int, char *, va_list);

extern void mln_log_set_logger(mln_logger_t logger);
extern int mln_log_reload(void *data);
extern int mln_log_init(int in_daemon);
extern void mln_log_destroy(void);
extern void _mln_sys_log(mln_log_level_t level, \
                         const char *file, \
                         const char *func, \
                         int line, \
                         char *msg, \
                         ...);
#define mln_log(err_lv,msg,...) \
    _mln_sys_log(err_lv, __FILE__, __FUNCTION__, __LINE__, msg, ## __VA_ARGS__)
extern ssize_t mln_log_writen(void *buf, mln_size_t size);
extern int mln_log_get_fd(void);
extern char *mln_log_get_dir_path(void);
extern char *mln_log_get_log_path(void);
extern char *mln_log_get_pid_path(void);
#endif

