
/*
 * Copyright (C) Niklaus F.Schen.
 */

#include <sys/resource.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <signal.h>
#include "mln_tools.h"
#include "mln_global.h"
#include "mln_log.h"

MLN_DEFINE_TOKEN(mln_passwd_lex, PWD, \
                 {PWD_TK_MELON, "PWD_TK_MELON"}, \
                 {PWD_TK_COMMENT, "PWD_TK_COMMENT"});

static int
mln_boot_help(const char *boot_str, const char *alias);
static int
mln_boot_version(const char *boot_str, const char *alias);
static int
mln_boot_reload(const char *boot_str, const char *alias);
static int
mln_boot_stop(const char *boot_str, const char *alias);
static int mln_set_id(void);
static int mln_sys_core_modify(void);
static int mln_sys_nofile_modify(void);

mln_boot_t boot_params[] = {
{"--help", "-h", mln_boot_help, 0},
{"--version", "-v", mln_boot_version, 0},
{"--reload", "-r", mln_boot_reload, 0},
{"--stop", "-s", mln_boot_stop, 0}
};
char mln_core_file_cmd[] = "core_file_size";
char mln_nofile_cmd[] = "max_nofile";
char mln_limit_unlimited[] = "unlimited";

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

int mln_sys_limit_modify(void)
{
    if (getuid()) {
        fprintf(stderr, "Modify system limitation failed. Permission deny.\n");
        return -1;
    }

    if (mln_sys_core_modify() < 0) {
        return -1;
    }
    return mln_sys_nofile_modify();
}

static int mln_sys_core_modify(void)
{
#ifdef RLIMIT_CORE
    rlim_t core_file_size = 0;

    mln_conf_t *cf = mln_get_conf();
    if (cf == NULL) {
        fprintf(stderr, "Configuration messed up.\n");
        return -1;
    }
    mln_conf_domain_t *cd = cf->search(cf, "main");
    if (cd == NULL) {
        fprintf(stderr, "Configuration messed up.\n");
        return -1;
    }
    mln_conf_cmd_t *cc = cd->search(cd, mln_core_file_cmd);
    if (cc == NULL) return 0;

    mln_conf_item_t *ci = cc->search(cc, 1);
    if (ci->type == CONF_INT) {
        core_file_size = (rlim_t)ci->val.i;
    } else if (ci->type == CONF_STR) {
        if (mln_const_strcmp(ci->val.s, mln_limit_unlimited)) {
            fprintf(stderr, "Invalid argument of %s.\n", mln_core_file_cmd);
            return -1;
        }
        core_file_size = RLIM_INFINITY;
    } else {
        fprintf(stderr, "Invalid argument of %s.\n", mln_core_file_cmd);
        return -1;
    }

    struct rlimit rl;
    memset(&rl, 0, sizeof(rl));
    rl.rlim_cur = rl.rlim_max = core_file_size;
    if (setrlimit(RLIMIT_CORE, &rl) != 0) {
        fprintf(stderr, "setrlimit core failed, %s\n", strerror(errno));
        return -1;
    }
#endif
    return 0;
}

static int mln_sys_nofile_modify(void)
{
#ifdef RLIMIT_NOFILE
    rlim_t nofile = 0;

    mln_conf_t *cf = mln_get_conf();
    if (cf == NULL) {
        fprintf(stderr, "Configuration messed up.\n");
        return -1;
    }
    mln_conf_domain_t *cd = cf->search(cf, "main");
    if (cd == NULL) {
        fprintf(stderr, "Configuration messed up.\n");
        return -1;
    }
    mln_conf_cmd_t *cc = cd->search(cd, mln_nofile_cmd);
    if (cc == NULL) return 0;

    mln_conf_item_t *ci = cc->search(cc, 1);
    if (ci->type == CONF_INT) {
        nofile = (rlim_t)ci->val.i;
    } else if (ci->type == CONF_STR) {
        if (mln_const_strcmp(ci->val.s, mln_limit_unlimited)) {
            fprintf(stderr, "Invalid argument of %s.\n", mln_nofile_cmd);
            return -1;
        }
        nofile = RLIM_INFINITY;
    } else {
        fprintf(stderr, "Invalid argument of %s.\n", mln_nofile_cmd);
        return -1;
    }

    struct rlimit rl;
    memset(&rl, 0, sizeof(rl));
    rl.rlim_cur = rl.rlim_max = nofile;
    if (setrlimit(RLIMIT_NOFILE, &rl) != 0) {
        fprintf(stderr, "setrlimit fd failed, %s\n", strerror(errno));
        return -1;
    }
#endif
    return 0;
}

int mln_daemon(void)
{
    int ret;

    mln_conf_t *cf = mln_get_conf();
    if (cf == NULL) goto out;
    mln_conf_domain_t *cd = cf->search(cf, "main");
    if (cd == NULL) {
        fprintf(stderr, "No such domain named 'main'\n");
        abort();
    }
    mln_conf_cmd_t *cc = cd->search(cd, "daemon");
    if (cc == NULL) goto out;
    mln_conf_item_t *ci = cc->search(cc, 1);
    if (ci == NULL) {
        fprintf(stderr, "Command 'daemon' need a parameter.\n");
        return -1;
    }
    if (ci->type != CONF_BOOL) {
        fprintf(stderr, "Parameter type of command 'daemon' error.\n");
        return -1;
    }
    if (!ci->val.b) {
out:
        ret = mln_log_init(0);
        if (ret < 0) return ret;
        return mln_set_id();
    }

    pid_t pid;
    if ((pid = fork()) < 0) {
        mln_log(error, "%s\n", strerror(errno));
        exit(1);
    } else if (pid != 0) {
        exit(0);
    }
    setsid();

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
        fprintf(stderr, "Unexpected file descriptors %d %d %d\n", fd0, fd1, fd2);
    }
    ret = mln_log_init(1);
    if (ret < 0) return ret;
    return mln_set_id();
}

static int mln_set_id(void)
{
    char filename[] = "/etc/passwd";
    char *keywords[] = {"root", NULL};
    struct mln_lex_attr lattr;

    /*get user name*/
    mln_conf_t *cf = mln_get_conf();
    if (cf == NULL) {
        mln_log(error, "No configuration.\n");
        abort();
    }
    mln_conf_domain_t *cd = cf->search(cf, "main");
    if (cd == NULL) {
        mln_log(error, "No 'main' domain.\n");
        abort();
    }
    mln_conf_cmd_t *cc = cd->search(cd, "user");
    if (cc != NULL) {
        mln_conf_item_t *ci = cc->search(cc, 1);
        if (ci == NULL) {
            mln_log(error, "Command 'user' need a parameter.\n");
            return -1;
        }
        if (ci->type != CONF_STR) {
            mln_log(error, "Parameter type of command 'user' error.\n");
            return -1;
        }
        keywords[0] = ci->val.s->str;
    }

    /*init lexer*/
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

int mln_boot_params(int argc, char *argv[])
{
    int i, ret;
    char *p;
    mln_boot_t *b;
    mln_boot_t *bend = boot_params + sizeof(boot_params)/sizeof(mln_boot_t);
    for (i = 1; i < argc; i++) {
        p = argv[i];
        for (b = boot_params; b < bend; b++) {
             if (!b->cnt && \
                 (!strcmp(p, b->boot_str) || \
                 !strcmp(p, b->alias)))
             {
                 b->cnt++;
                 ret = b->handler(b->boot_str, b->alias);
                 if (ret < 0) {
                     return ret;
                 }
             }
        }
    }
    return 0;
}

static int
mln_boot_help(const char *boot_str, const char *alias)
{
    printf("Boot parameters:\n");
    printf("\t--version -v\t\t\tshow version\n");
    printf("\t--reload  -r\t\t\treload configuration\n");
    printf("\t--stop    -s\t\t\tstop melon service.\n");
    exit(0);
    return 0;
}

static int
mln_boot_version(const char *boot_str, const char *alias)
{
    printf("Melon Platform.\n");
    printf("Version 1.3.6.\n");
    printf("Copyright (C) Niklaus F.Schen (Chinese name: Shen Fanchen).\n");
    exit(0);
    return 0;
}

static int
mln_boot_reload(const char *boot_str, const char *alias)
{
    char buf[1024] = {0};
    int fd, n, pid;

    snprintf(buf, sizeof(buf)-1, "%s/logs/melon.pid", mln_get_path());
    fd = open(buf, O_RDONLY);
    if (fd < 0) {
        fprintf(stderr, "'melon.pid' not existent.\n");
        exit(1);
    }
    n = read(fd, buf, sizeof(buf)-1);
    if (n <= 0) {
        fprintf(stderr, "Invalid file 'melon.pid'.\n");
        exit(1);
    }
    buf[n] = 0;

    pid = atoi(buf);
    kill(pid, SIGUSR2);

    exit(0);
}

static int
mln_boot_stop(const char *boot_str, const char *alias)
{
    char buf[1024] = {0};
    int fd, n, pid;

    snprintf(buf, sizeof(buf)-1, "%s/logs/melon.pid", mln_get_path());
    fd = open(buf, O_RDONLY);
    if (fd < 0) {
        fprintf(stderr, "'melon.pid' not existent.\n");
        exit(1);
    }
    n = read(fd, buf, sizeof(buf)-1);
    if (n <= 0) {
        fprintf(stderr, "Invalid file 'melon.pid'.\n");
        exit(1);
    }
    buf[n] = 0;

    pid = atoi(buf);
    kill(pid, SIGKILL);

    exit(0);
}

