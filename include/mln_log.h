
/*
 * Copyright (C) Niklaus F.Schen.
 */

#ifndef __MLN_LOG_H
#define __MLN_LOG_H

#include <stdarg.h>
#include "mln_conf.h"
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
#if !defined(MSVC)
    mln_spin_t      thread_lock;
#endif
    int             fd;
    mln_u32_t       in_daemon:1;
    mln_u32_t       init:1;
    mln_u32_t       padding:30;
    mln_log_level_t level;
    char            dir_path[M_LOG_PATH_LEN/2];
    char            pid_path[M_LOG_PATH_LEN];
    char            log_path[M_LOG_PATH_LEN];
} mln_log_t;


typedef void (*mln_logger_t)(mln_log_t *, mln_log_level_t, const char *, const char *, int, char *, va_list);

extern mln_log_t g_log;

#define mln_log_in_daemon() (g_log.in_daemon)

/*
 * mln_log_init should be called before any pthread_create
 */
extern int mln_log_init(mln_conf_t *cf);
extern void mln_log_logger_set(mln_logger_t logger);
extern mln_logger_t mln_log_logger_get(void);
extern int mln_log_reload(void *data);
extern void mln_log_destroy(void);
extern void _mln_sys_log(mln_log_level_t level, \
                         const char *file, \
                         const char *func, \
                         int line, \
                         char *msg, \
                         ...);
#define mln_log(err_lv,msg,...) \
    _mln_sys_log(err_lv, __FILE__, __FUNCTION__, __LINE__, msg, ## __VA_ARGS__)
extern int mln_log_writen(void *buf, mln_size_t size);
extern int mln_log_fd(void);
extern char *mln_log_dir_path(void);
extern char *mln_log_logfile_path(void);
extern char *mln_log_pid_path(void);
#endif

