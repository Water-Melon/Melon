add_rules("mode.debug", "mode.release")

option("install_path")
    set_default("/usr/local/melon")
option("conf_path")
    set_default("/usr/local/melon/conf/melon.conf")
option("tmpfile_path")
    set_default("/usr/local/melon/tmp")
option("pid_path")
    set_default("/usr/local/melon/logs/melon.pid")
option("log_path")
    set_default("/usr/local/melon/logs/melon.log")
option("null_path")
    set_default("/dev/null")
option("melang_lib_path")
    set_default("/usr/local/lib/melang")
option("melang_dylib_path")
    set_default("/usr/local/lib/melang_dynamic")

target("melon")
    set_kind("$(kind)")
    add_files(path.join("src", "*.c"))
    add_includedirs("include", {public = true})
    add_headerfiles(path.join("include", "*.h"))
    before_build(function (target)
        import("core.project.config")
        io.writefile(path.join("include", "mln_path.h"), [[
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
]])
        io.writefile(path.join("src", "mln_path.c"), [[

/*
 * Copyright (C) Niklaus F.Schen.
 */
#include <stdio.h>
#include "mln_path.h"

static char install_path[] = "/usr/local/melon";

static char conf_path[] = "/usr/local/melon/conf/melon.conf";

static char tmpfile_path[] = "/usr/local/melon/tmp";

static char pid_path[] = "/usr/local/melon/logs/melon.pid";

static char log_path[] = "/usr/local/melon/logs/melon.log";

static char null_path[] = "/dev/null";

static char melang_lib_path[] = "/usr/local/lib/melang";

static char melang_dylib_path[] = "/usr/local/lib/melang_dynamic";

static mln_path_hook_t _install_path = NULL;

static mln_path_hook_t _null_path = NULL;

static mln_path_hook_t _melang_lib_path = NULL;

static mln_path_hook_t _melang_dylib_path = NULL;

static mln_path_hook_t _conf_path = NULL;

static mln_path_hook_t _tmpfile_path = NULL;

static mln_path_hook_t _pid_path = NULL;

static mln_path_hook_t _log_path = NULL;

void mln_path_hook_set(mln_path_type_t type, mln_path_hook_t hook)
{
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
}

char *mln_path(void)
{
    return _install_path == NULL? install_path: _install_path();
}

char *mln_path_conf(void)
{
    return _conf_path == NULL? conf_path: _conf_path();
}

char *mln_path_tmpfile(void)
{
    return _tmpfile_path == NULL? tmpfile_path: _tmpfile_path();
}

char *mln_path_pid(void)
{
    return _pid_path == NULL? pid_path: _pid_path();
}

char *mln_path_log(void)
{
    return _log_path == NULL? log_path: _log_path();
}

char *mln_path_null(void)
{
    return _null_path == NULL? null_path: _null_path();
}

char *mln_path_melang_lib(void)
{
    return _melang_lib_path == NULL? melang_lib_path: _melang_lib_path();
}

char *mln_path_melang_dylib(void)
{
    return _melang_dylib_path == NULL? melang_dylib_path: _melang_dylib_path();
}
]])
        io.replace(path.join("src", "mln_path.c"), 'static char install_path[] = "/usr/local/melon";', 'static char install_path[] = "' .. config.get("install_path") .. '";', {plain = true})
        io.replace(path.join("src", "mln_path.c"), 'static char conf_path[] = "/usr/local/melon/conf/melon.conf";', 'static char conf_path[] = "' .. config.get("conf_path") .. '";', {plain = true})
        io.replace(path.join("src", "mln_path.c"), 'static char tmpfile_path[] = "/usr/local/melon/tmp";', 'static char tmpfile_path[] = "' .. config.get("tmpfile_path") .. '";', {plain = true})
        io.replace(path.join("src", "mln_path.c"), 'static char pid_path[] = "/usr/local/melon/logs/melon.pid";', 'static char pid_path[] = "' .. config.get("pid_path") .. '";', {plain = true})
        io.replace(path.join("src", "mln_path.c"), 'static char log_path[] = "/usr/local/melon/logs/melon.log";', 'static char log_path[] = "' .. config.get("log_path") .. '";', {plain = true})
        io.replace(path.join("src", "mln_path.c"), 'static char null_path[] = "/dev/null";', 'static char null_path[] = "' .. config.get("null_path") .. '";', {plain = true})
        io.replace(path.join("src", "mln_path.c"), 'static char melang_lib_path[] = "/usr/local/lib/melang";', 'static char melang_lib_path[] = "' .. config.get("melang_lib_path") .. '";', {plain = true})
        io.replace(path.join("src", "mln_path.c"), 'static char melang_dylib_path[] = "/usr/local/lib/melang_dynamic";', 'static char melang_dylib_path[] = "' .. config.get("melang_dylib_path") .. '";', {plain = true})
    end)
	
