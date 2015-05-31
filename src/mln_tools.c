
/*
 * Copyright (C) Niklaus F.Schen.
 */

#include <sys/resource.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include "mln_tools.h"
#include "mln_global.h"
#include "mln_log.h"


MLN_DEFINE_TOKEN(mln_passwd_lex, PWD, \
                 {PWD_TK_MELON, "PWD_TK_MELON"}, \
                 {PWD_TK_COMMENT, "PWD_TK_COMMENT"});

static mln_passwd_lex_struct_t *
mln_passwd_lex_nums_handler(mln_lex_t *lex, void *data)
{
    char c;
    while ( 1 ) {
        c = mln_geta_char(lex);
        if (c == MLN_ERR) return NULL;
        if (c == MLN_EOF) break;
        if (c == '\n') break;
        if (mln_puta_char(lex, c) == MLN_ERR) return NULL;
    }
    return mln_passwd_lex_new(lex, PWD_TK_COMMENT);
}

int mln_unlimit_memory(void)
{
    if (getuid()) {
        mln_log(error, "RLIMIT_AS permission deny.\n");
        return -1;
    }
#ifdef RLIMIT_AS
    struct rlimit rl;
    memset(&rl, 0, sizeof(rl));
    rl.rlim_cur = RLIM_INFINITY;
    rl.rlim_max = RLIM_INFINITY;
    if (setrlimit(RLIMIT_AS, &rl) != 0) {
        mln_log(error, "%s\n", strerror(errno));
        return -1;
    }
#endif
    return 0;
}

int mln_cancel_core(void)
{
    if (getuid()) {
        mln_log(error, "RLIMIT_CORE permission deny.\n");
        return -1;
    }
#ifdef RLIMIT_CORE
    struct rlimit rl;
    memset(&rl, 0, sizeof(rl));
    rl.rlim_cur = rl.rlim_max = 0;
    if (setrlimit(RLIMIT_CORE, &rl) != 0) {
        mln_log(error, "%s\n", strerror(errno));
        return -1;
    }
#endif
    return 0;
}

int mln_unlimit_fd(void)
{
    if (getuid()) {
        mln_log(error, "RLIMIT_NOFILE permission deny.\n");
        return -1;
    }
#ifdef RLIMIT_NOFILE
    struct rlimit rl;
    memset(&rl, 0, sizeof(rl));
    rl.rlim_cur = rl.rlim_max = RLIM_INFINITY;
    if (setrlimit(RLIMIT_NOFILE, &rl) != 0) {
        rl.rlim_cur = rl.rlim_max = 0x100000;
        if (setrlimit(RLIMIT_NOFILE, &rl) != 0) {
            mln_log(error, "%s\n", strerror(errno));
            return -1;
        }
    }
#endif
    return 0;
}

void mln_daemon(void)
{
    pid_t pid;
    if ((pid = fork()) < 0) {
        mln_log(error, "%s\n", strerror(errno));
        exit(1);
    } else if (pid != 0) {
        exit(0);
    }
    setsid();
}

void mln_close_terminal(void)
{
    int fd0 = STDIN_FILENO;
    int fd1 = STDOUT_FILENO;
    int fd2 = STDERR_FILENO;
    close(fd0);
    close(fd1);
    close(fd2);
    fd0 = open("/dev/null", O_RDWR);
    fd1 = dup(fd0);
    fd2 = dup(fd0);
    if (fd0 != STDIN_FILENO || \
        fd1 != STDOUT_FILENO || \
        fd2 != STDERR_FILENO)
    {
        mln_log(error, "Unexpected file descriptors %d %d %d\n", fd0, fd1, fd2);
    }
}

int mln_set_id(void)
{
    /*init lexer*/
    char filename[] = "/etc/passwd";
    char *keywords[] = {"melon", NULL};
    struct mln_lex_attr lattr;
    mln_lex_hooks_t hooks;
    memset(&hooks, 0, sizeof(hooks));
    hooks.nums_handler = (lex_hook)mln_passwd_lex_nums_handler;
    lattr.input_type = mln_lex_file;
    lattr.input.filename = filename;
    lattr.keywords = keywords;
    lattr.hooks = &hooks;
    mln_lex_t *lex;
    MLN_LEX_INIT_WITH_HOOKS(mln_passwd_lex, lex, &lattr);
    if (lex == NULL)  {
        mln_log(error, "No memory.\n");
        return -1;
    }

    /*jump off useless informations*/
    mln_passwd_lex_struct_t *plst;
    while ((plst = mln_passwd_lex_token(lex)) != NULL) {
        if (plst->type == PWD_TK_MELON)
            break;
        if (plst->type == PWD_TK_EOF)
            break;
        mln_passwd_lex_free(plst);
    }
    if (plst == NULL || plst->type == PWD_TK_EOF) {
        if (plst == NULL) mln_log(error, "%s\n", mln_lex_strerror(lex));
        else mln_log(error, "User 'melon' not existed.\n");
        mln_passwd_lex_free(plst);
        mln_lex_destroy(lex);
        return -1;
    }
    mln_passwd_lex_free(plst);

    /*jump off useless columns*/
    mln_s32_t colon_cnt = 0;
    while ((plst = mln_passwd_lex_token(lex)) != NULL) {
        if (plst->type == PWD_TK_COLON) {
            if (++colon_cnt == 2) break;
        }
        mln_passwd_lex_free(plst);
    }
    if (plst == NULL) {
        mln_log(error, "%s\n", mln_lex_strerror(lex));
        mln_lex_destroy(lex);
        return -1;
    }
    mln_passwd_lex_free(plst);

    /*get uid and gid*/
    plst = mln_passwd_lex_token(lex);
    if (plst == NULL || plst->type != PWD_TK_DEC) {
err:
        if (plst == NULL) mln_log(error, "%s\n", mln_lex_strerror(lex));
        else mln_log(error, "Invalid ID.\n");
        mln_passwd_lex_free(plst);
        mln_lex_destroy(lex);
        return -1;
    }
    int uid = atoi(plst->text->str);
    mln_passwd_lex_free(plst);
    plst = mln_passwd_lex_token(lex);
    if (plst == NULL || plst->type != PWD_TK_COLON) goto err;
    mln_passwd_lex_free(plst);
    plst = mln_passwd_lex_token(lex);
    if (plst == NULL || plst->type != PWD_TK_DEC) goto err;
    int gid = atoi(plst->text->str);
    mln_passwd_lex_free(plst);
    mln_lex_destroy(lex);

    /*set log files' uid & gid*/
    char *path = mln_log_get_dir_path();
    if (!access(path, F_OK))
        chown(path, uid, gid);
    path = mln_log_get_log_path();
    if (!access(path, F_OK))
        chown(path, uid, gid);
    path = mln_log_get_pid_path();
    if (!access(path, F_OK))
        chown(path, uid, gid);

    /*set uid, gid, euid and egid*/
    if (setgid(gid) < 0) {
        mln_log(error, "Set GID failed. %s\n", strerror(errno));
        return -1;
    }
    if (setuid(uid) < 0) {
        mln_log(error, "Set UID failed. %s\n", strerror(errno));
    }
    if (setegid(gid) < 0) {
        mln_log(error, "Set eGID failed. %s\n", strerror(errno));
    }
    if (seteuid(uid) < 0) {
        mln_log(error, "Set eUID failed. %s\n", strerror(errno));
    }

    return 0;
}
