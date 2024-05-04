## Event

The system calls used by events vary according to different operating system platforms, and now support:

- epoll
- kqueue
- select

This module is not thread-safe in the `MSVC` environment.


### Header file

```c
#include "mln_event.h"
```



### Module

`event`



### Functions



#### mln_event_new

```c
mln_event_t *mln_event_new(void);
```

Description: Create an event structure.

Return value: return event structure pointer if successful, otherwise return `NULL`



#### mln_event_free

```c
void mln_event_free(mln_event_t *ev);
```

Description: Destroy the event structure.

Return value: none



#### mln_event_dispatch

```c
void mln_event_dispatch(mln_event_t *event);
```

Description: Dispatches an event on the event set `event`. When an event is triggered, the corresponding callback function will be called for processing.

**Note**: This function will not return without calling `mln_event_break_set`.

Return value: none



#### mln_event_fd_set

```c
int mln_event_fd_set(mln_event_t *event, int fd, mln_u32_t flag, int timeout_ms, void *data, ev_fd_handler fd_handler);

typedef void (*ev_fd_handler)  (mln_event_t *, int, void *);s
```

Description: Set file descriptor event, where:

- `fd` is the file descriptor that the event is concerned with.

- `flag` is divided into the following categories:

  - `M_EV_RECV` read event
  - `M_EV_SEND` write event
  - `M_EV_ERROR` error event
  - `M_EV_ONESHOT` fires only once
  - `M_EV_NONBLOCK` non-blocking mode
  - `M_EV_BLOCK` blocking mode
  - `M_EV_APPEND` appends an event, that is, an event has been set, such as a read event, and if you want to add another type of event, such as a write event, you can use this flag
  - `M_EV_CLR` clears all events

  These flags can be set simultaneously using the OR operator.

- `timeout_ms` event timeout, in milliseconds, the value of this field is:

  - `M_EV_UNLIMITED` never times out
  - `M_EV_UNMODIFIED` retains the previous timeout setting
  - `milliseconds` timeout period

- `data` is the user data structure related to event processing, which can be defined by yourself.

- `ev_fd_handler` is an event handler function. The function has three parameters: event structure, file descriptor and user-defined user data structure.

Return value: return `0` if successful, otherwise return `-1`



#### mln_event_fd_timeout_handler_set

```c
void mln_event_fd_timeout_handler_set(mln_event_t *event, int fd, void *data, ev_fd_handler timeout_handler);
```

Description: Set the descriptor event timeout handler, where:

- `fd` is the file descriptor.
- `data` is a user data structure related to timeout processing, which can be defined by yourself.
- `timeout_handler` is the same as the callback function type of `mln_event_fd_set` function to handle timeout events.

This function needs to be used in conjunction with the `mln_event_fd_set` function, first use `mln_event_fd_set` to set the event, and then use this function to set the timeout processing function.

The reason for this is that some events may not need special functions for processing after timeout, and if they are all set in the `mln_event_fd_set` function, it will lead to too many parameters and too complicated.

Return value: none



#### mln_event_timer_set

```c
int mln_event_timer_set(mln_event_t *event, mln_u32_t msec, void *data, ev_tm_handler tm_handler);

typedef void (*ev_tm_handler)  (mln_event_t *, void *);
```

Description: Set timer event where:

- `msec` is the timer millisecond value
- `data` user-defined data structure for timed events
- `tm_handler` timing event handler function, its parameters are: event structure and user-defined data

Every time a timed event starts, it will be automatically deleted from the event set. If you need to trigger the timed event all the time, you need to call this function in the handler function to set it.

Return value: If successful, return the timer handle pointer, otherwise return `NULL`



#### mln_event_timer_cancel

```c
void mln_event_timer_cancel(mln_event_t *event, mln_event_timer_t *timer);
```

Description: Cancel the timer that has been set. This function needs to ensure that `timer` is a timeout event handler that has been set but not triggered.

Return value: none



#### mln_event_signal_set

```c
typedef void (*sig_t)(int);

sig_t mln_event_signal_set(int signo, sig_t handle);
```

Description: Set the signal handle. It is the alias of system `signal`.

Return value: The signal handle before setting.



#### mln_event_break_set

```c
mln_event_break_set(ev);
```

Description: Interrupt event processing so that the `mln_event_dispatch` function returns.

Return value: none



#### mln_event_break_reset

```c
mln_event_break_reset(ev);
```

Description: Reset break flag.

Return value: none



#### mln_event_callback_set

```c
void (mln_event_t *ev, dispatch_callback dc, void *dc_data);

typedef void (*dispatch_callback) (mln_event_t *, void *);
```

Description: Set the event processing callback function, which will be called once at the beginning of each time loop. Currently mainly used to handle configuration hot reloading.

`dc_data` is a user-defined data structure.

`dc` is a callback function, and its parameters are: event structure and user-defined data structure.

Return value: none



### Example

```c
#include <stdio.h>
#include <stdlib.h>
#include "mln_event.h"

static void timer_handler(mln_event_t *ev, void *data)
{
    printf("timer\n");
    mln_event_timer_set(ev, 1000, NULL, timer_handler);
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

    return 0;
}
```

