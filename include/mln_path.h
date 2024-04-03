
/*
 * Copyright (C) Niklaus F.Schen.
 */
#ifndef __MLN_PATH_H
#define __MLN_PATH_H

typedef enum {
    m_p_install,
    m_p_conf,
    m_p_tmpfile,
    m_p_pid,
    m_p_log,
    m_p_null,
    m_p_melang_lib,
    m_p_melang_dylib
} mln_path_type_t;

typedef char *(*mln_path_hook_t)(void);

extern char *mln_path(void);

extern char *mln_path_null(void);

extern char *mln_path_melang_lib(void);

extern char *mln_path_melang_dylib(void);

extern char *mln_path_conf(void);

extern char *mln_path_tmpfile(void);

extern char *mln_path_pid(void);

extern char *mln_path_log(void);

extern void mln_path_hook_set(mln_path_type_t type, mln_path_hook_t hook);

#endif

