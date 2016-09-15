
/*
 * Copyright (C) Niklaus F.Schen.
 */

#ifndef __MLN_LOG_H
#define __MLN_LOG_H

#include "mln_types.h"

#define M_LOG_PATH_LEN 1024

enum log_level {
    none,
    report,
    debug,
    warn,
    error
};

typedef struct {
    char           dir_path[M_LOG_PATH_LEN];
    char           pid_path[M_LOG_PATH_LEN];
    char           log_path[M_LOG_PATH_LEN];
    int            fd;
    int            in_daemon;
    enum log_level level;
    mln_lock_t     thread_lock;
} mln_log_t;
    

extern int mln_log_reload(void *data);
extern int mln_log_init(int in_daemon);
extern void mln_log_destroy(void);
extern void _mln_sys_log(enum log_level level, \
                         const char *file, \
                         const char *func, \
                         int line, \
                         char *msg, \
                         ...);
#define mln_log(err_lv,msg,...) \
    _mln_sys_log(err_lv, __FILE__, __FUNCTION__, __LINE__, msg, ## __VA_ARGS__)
extern int mln_log_get_fd(void);
extern char *mln_log_getDirPath(void);
extern char *mln_log_getLogPath(void);
extern char *mln_log_getPidPath(void);
#endif

