#include "mln_iothread.h"
#include <string.h>
#include <stdio.h>
#include <errno.h>

static void msg_handler(mln_iothread_t *t, mln_iothread_ep_type_t from, mln_iothread_msg_t *msg)
{
    mln_u32_t type = mln_iothread_msg_type(msg);
    printf("msg type: %u\n", type);
}

static void *entry(void *args)
{
    int n;
    mln_iothread_t *t = (mln_iothread_t *)args;
    while (1) {
        n = mln_iothread_recv(t, user_thread);
        printf("recv %d message(s)\n", n);
    }

    return NULL;
}

int main(void)
{
    int i, rc;
    mln_iothread_t t;

    if (mln_iothread_init(&t, 1, (mln_iothread_entry_t)entry, &t, (mln_iothread_msg_process_t)msg_handler) < 0) {
        fprintf(stderr, "iothread init failed\n");
        return -1;
    }
    for (i = 0; i < 1000; ++i) {
        if ((rc = mln_iothread_send(&t, i, NULL, io_thread, 1)) < 0) {
            fprintf(stderr, "send failed\n");
            return -1;
        } else if (rc > 0)
            continue;
    }
    sleep(1);
    mln_iothread_destroy(&t);
    sleep(3);
    printf("DONE\n");

    return 0;
}
