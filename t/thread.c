#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include "mln_framework.h"
#include "mln_log.h"
#include "mln_thread.h"

mln_string_t me_framework_mode = mln_string("multithread");
mln_conf_item_t me_framework_conf = {CONF_STR, .val.s=&me_framework_mode};
mln_conf_item_t me_workerproc_conf = {CONF_INT, .val.i=1};
mln_string_t me_thread_hello_str = mln_string("hello");
mln_conf_item_t me_thread_hello = {CONF_STR, .val.s=&me_thread_hello_str};
mln_string_t me_thread_haha_str = mln_string("haha");
mln_conf_item_t me_thread_haha = {CONF_STR, .val.s=&me_thread_haha_str};

static int global_init(void)
{
    mln_conf_t *cf;
    mln_conf_domain_t *cd;
    mln_conf_cmd_t *cc;

    cf = mln_conf();
    cd = cf->search(cf, "main");
    cc = cd->search(cd, "framework");
    if (cc == NULL) {
        assert((cc = cd->insert(cd, "framework")) != NULL);
    }
    assert(cc->update(cc, &me_framework_conf, 1) == 0);

    cc = cd->search(cd, "worker_proc");
    if (cc == NULL) {
        assert((cc = cd->insert(cd, "worker_proc")) != NULL);
    }
    assert(cc->update(cc, &me_workerproc_conf, 1) == 0);

    cd = cf->search(cf, "thread_exec");
    if (cd == NULL) {
        cd = cf->insert(cf, "thread_exec");
    }
    assert(cd != NULL);

    assert((cc = cd->insert(cd, "default")) != NULL);
    assert(cc->update(cc, &me_thread_hello, 1) == 0);

    assert((cc = cd->insert(cd, "default")) != NULL);
    assert(cc->update(cc, &me_thread_haha, 1) == 0);

    return 0;
}

static int haha(int argc, char **argv)
{
    int fd = atoi(argv[argc-1]);
    mln_thread_msg_t msg;
    int nfds;
    fd_set rdset;

    FD_ZERO(&rdset);
    FD_SET(fd, &rdset);
    nfds = select(fd+1, &rdset, NULL, NULL, NULL);
    if (nfds < 0) {
        mln_log(error, "select error. %s\n", strerror(errno));
        return -1;
    }
    memset(&msg, 0, sizeof(msg));
    int n = recv(fd, &msg, sizeof(msg), 0);
    if (n != sizeof(msg)) {
        mln_log(debug, "recv error. n=%d. %s\n", n, strerror(errno));
        return -1;
    }
    mln_log(debug, "!!!src:%S auto:%l char:%c\n", msg.src, msg.sauto, msg.c);
    mln_thread_clear_msg(&msg);

    return 0;
}

static void hello_cleanup(void *data)
{
    mln_log(debug, "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n");
}

static int hello(int argc, char **argv)
{
    mln_thread_cleanup_set(hello_cleanup, NULL);
    int i;
    for (i = 0; i < 1; ++i)  {
        int fd = atoi(argv[argc-1]);
        mln_thread_msg_t msg;
        memset(&msg, 0, sizeof(msg));
        msg.dest = mln_string_new("haha");
        assert(msg.dest);
        msg.sauto = 9736;
        msg.c = 'N';
        msg.type = ITC_REQUEST;
        msg.need_clear = 1;
        int n = send(fd, &msg, sizeof(msg), 0);
        if (n != sizeof(msg)) {
            mln_log(debug, "send error. n=%d. %s\n", n, strerror(errno));
            mln_string_free(msg.dest);
            return -1;
        }
    }
    usleep(1000000);
    exit(0);
    return 0;
}

static void master_handler(mln_event_t *ev)
{
    usleep(100000);
    exit(0);
}

int main(int argc, char *argv[])
{
    printf("NOTE: This test has memory leak because we don't release memory of log, conf, multiprocess-related and multithread-related stuffs.\n");
    printf("In fact, `mln_framework_init` should be the last function called in `main`, so we don't need to release anything.\n");

    mln_thread_module_t modules[] = {
      {"haha", haha},
      {"hello", hello},
    };

    struct mln_framework_attr cattr;

    cattr.argc = argc;
    cattr.argv = argv;
    cattr.global_init = global_init;
    cattr.main_thread = NULL;
    cattr.worker_process = NULL;
    cattr.master_process = master_handler;

    mln_thread_module_set(modules, 2);

    if (mln_framework_init(&cattr) < 0) {
       fprintf(stderr, "Melon init failed.\n");
       return -1;
    }

    return 0;
}

