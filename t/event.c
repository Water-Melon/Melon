#include <stdio.h>
#include <stdlib.h>
#include "mln_event.h"

static void timer_handler(mln_event_t *ev, void *data)
{
    printf("timer\n");
    mln_event_break_set(ev);
}

static void mln_fd_write(mln_event_t *ev, int fd, void *data)
{
    printf("write handler\n");
    write(fd, "hello\n", 6);
    mln_event_fd_set(ev, fd, M_EV_CLR, M_EV_UNLIMITED, NULL, NULL);
}

int main(int argc, char *argv[])
{
    mln_event_t *ev;

    ev = mln_event_new();
    if (ev == NULL) {
        fprintf(stderr, "event init failed.\n");
        return -1;
    }

    if (mln_event_timer_set(ev, 1000, NULL, timer_handler) < 0) {
        fprintf(stderr, "timer set failed.\n");
        return -1;
    }

    if (mln_event_fd_set(ev, STDOUT_FILENO, M_EV_SEND, M_EV_UNLIMITED, NULL, mln_fd_write) < 0) {
        fprintf(stderr, "fd handler set failed.\n");
        return -1;
    }

    mln_event_dispatch(ev);

    mln_event_free(ev);

    return 0;
}
