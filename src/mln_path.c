
/*
 * Copyright (C) Niklaus F.Schen.
 */
#include <stdio.h>
#include "mln_path.h"
#include "mln_func.h"

static char install_path[] = MLN_ROOT;

#if defined(MSVC)
static char conf_path[] = MLN_ROOT"\\conf\\melon.conf";

static char tmpfile_path[] = MLN_ROOT"\\tmp";

static char pid_path[] = MLN_ROOT"\\logs\\melon.pid";

static char log_path[] = MLN_ROOT"\\logs\\melon.log";
#else
static char conf_path[] = MLN_ROOT"/conf/melon.conf";

static char tmpfile_path[] = MLN_ROOT"/tmp";

static char pid_path[] = MLN_ROOT"/logs/melon.pid";

static char log_path[] = MLN_ROOT"/logs/melon.log";
#endif

static char null_path[] = MLN_NULL;

static char melang_lib_path[] = MLN_LANG_LIB;

static char melang_dylib_path[] = MLN_LANG_DYLIB;

static mln_path_hook_t _install_path = NULL;

static mln_path_hook_t _null_path = NULL;

static mln_path_hook_t _melang_lib_path = NULL;

static mln_path_hook_t _melang_dylib_path = NULL;

static mln_path_hook_t _conf_path = NULL;

static mln_path_hook_t _tmpfile_path = NULL;

static mln_path_hook_t _pid_path = NULL;

static mln_path_hook_t _log_path = NULL;

MLN_FUNC_VOID(, void, mln_path_hook_set, (mln_path_type_t type, mln_path_hook_t hook), (type, hook), {

    switch (type) {

        case m_p_install:
            _install_path = hook;
            break;
        case m_p_conf:
            _conf_path = hook;
            break;
        case m_p_tmpfile:
            _tmpfile_path = hook;
            break;
        case m_p_pid:
            _pid_path = hook;
            break;
        case m_p_log:
            _log_path = hook;
            break;
        case m_p_null:
            _null_path = hook;
            break;
        case m_p_melang_lib:
            _melang_lib_path = hook;
            break;
        case m_p_melang_dylib:
            _melang_dylib_path = hook;
            break;
        default:
            break;
    }
})

MLN_FUNC(, char *, mln_path, (void), (), {

    return _install_path == NULL? install_path: _install_path();
})

MLN_FUNC(, char *, mln_path_conf, (void), (), {

    return _conf_path == NULL? conf_path: _conf_path();
})

MLN_FUNC(, char *, mln_path_tmpfile, (void), (), {

    return _tmpfile_path == NULL? tmpfile_path: _tmpfile_path();
})

MLN_FUNC(, char *, mln_path_pid, (void), (), {

    return _pid_path == NULL? pid_path: _pid_path();
})

MLN_FUNC(, char *, mln_path_log, (void), (), {

    return _log_path == NULL? log_path: _log_path();
})

MLN_FUNC(, char *, mln_path_null, (void), (), {

    return _null_path == NULL? null_path: _null_path();
})

MLN_FUNC(, char *, mln_path_melang_lib, (void), (), {

    return _melang_lib_path == NULL? melang_lib_path: _melang_lib_path();
})

MLN_FUNC(, char *, mln_path_melang_dylib, (void), (), {

    return _melang_dylib_path == NULL? melang_dylib_path: _melang_dylib_path();
})
