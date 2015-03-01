
/*
 * Copyright (C) Niklaus F.Schen.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include "mln_log.h"
#include "mln_types.h"
#include "mln_path.h"
#include "mln_thread.h"
#include "mln_thread_module.h"

mln_thread_module_t module_array[2] = {
{"haha", haha_main},
{"hello", hello_main}};

int haha_main(int argc, char **argv)
{
    int fd = atoi(argv[argc-1]);
    mln_thread_msg_t msg;
    int nfds;
    fd_set rdset;
  for (;;) {
    FD_ZERO(&rdset);
    FD_SET(fd, &rdset);
    nfds = select(fd+1, &rdset, NULL, NULL, NULL);
    if (nfds < 0) {
        if (errno == EINTR) continue;
        mln_log(error, "select error. %s\n", strerror(errno));
        return -1;
    }
    memset(&msg, 0, sizeof(msg));
    int n = read(fd, &msg, sizeof(msg));
    if (n != sizeof(msg)) {
        mln_log(debug, "write error. n=%d. %s\n", n, strerror(errno));
        return -1;
    }
    mln_log(debug, "!!!src:%s auto:%l char:%c\n", msg.src->str, msg.sauto, msg.c);
    mln_thread_clear_msg(&msg);
  }
    return 0;
}


#include <assert.h>

static void hello_cleanup(void *data)
{
    mln_log(debug, "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n");
}

int hello_main(int argc, char **argv)
{
  mln_set_cleanup(hello_cleanup, NULL);
  int i;
  for (i = 0; i < 100; i++)  {
    int fd = atoi(argv[argc-1]);
    mln_thread_msg_t msg;
    memset(&msg, 0, sizeof(msg));
    msg.dest = mln_new_string("haha");
    assert(msg.dest);
    msg.sauto = 9736;
    msg.c = 'N';
    msg.type = ITC_REQUEST;
    msg.need_clear = 0;
    int n = write(fd, &msg, sizeof(msg));
    if (n != sizeof(msg)) {
        mln_log(debug, "write error. n=%d. %s\n", n, strerror(errno));
        mln_free_string(msg.dest);
        return -1;
    }
  }
    return 0;
}


void *mln_get_module_entrance(char *alias)
{
    mln_thread_module_t *tm = NULL;
    for (tm = module_array; tm < module_array + 2; tm++) {
        if (!strcmp(alias, tm->alias)) return tm->thread_main;
    }
    return NULL;
}
