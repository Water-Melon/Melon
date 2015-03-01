
/*
 * Copyright (C) Niklaus F.Schen.
 */

#ifndef __MLN_LOG_H
#define __MLN_LOG_H


enum sys_log {
    none,
    report,
    debug,
    error,
    nolog
};

extern int mln_set_log_level(void);
extern void mln_set_sys_log_fd(int fd);
extern void mln_init_sys_log(int is_daemon);
extern void mln_init_pid_log(void);
void _mln_sys_log(enum sys_log level, \
                  const char *file, \
                  const char *func, \
                  int line, \
                  char *msg, \
                  ...);
extern int mln_get_log_fd(void);
#define mln_log(err_lv,msg,...) \
    _mln_sys_log(err_lv, __FILE__, __FUNCTION__, __LINE__, msg, ## __VA_ARGS__)
extern char *mln_get_log_dir_path(void);
extern char *mln_get_log_file_path(void);
extern char *mln_get_pid_file_path(void);
#endif

