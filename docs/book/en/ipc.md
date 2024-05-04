## IPC module development

This module is not supported in the MSVC.


### Header file

```c
#include "mln_ipc.h"
```



### Module

`ipc`



**Note**: This function is temporarily not supported under Windows.

Melon supports a multi-process framework, so naturally there will be communication issues between the main and sub-processes, and this part is handled by IPC.

> Since version 2.3.0, the IPC part will not be integrated by adding code files to the ipc_handlers subdirectory, but will provide a registration function to complete the registration and loading of IPC, so as to integrate the user-defined type handler function with Melon Decoupling from the code directory level.

Developers can call before the framework is initialized

```c
mln_ipc_cb_t *mln_ipc_handler_register(mln_u32_t type, ipc_handler master_handler, ipc_handler worker_handler, void *master_data, void *worker_data);
```

To register your own IPC message processing function, which:

- `type` is the type of the message. As a convention, `0`~`1024` will be used as the message type value used internally by Melon. Developers try not to overlap with it. If the type field overlaps, the new processing function will Override old handler functions.

- master_handler` is the handler function of the main process, and its function prototype is:

  ```c
  void (*ipc_handler)(mln_event_t *ev, void *f_ptr, void *buf, mln_u32_t len, void **udata_ptr);
  ```

  - `ev` is the event structure associated with this message.
  - `f_ptr` is a `mln_fork_t` type pointer (yes, you read it right, I wrote it right), we can get some information related to the child process through this pointer, such as the link structure of communication.
  - `buf` is the content of the message received by the main/child process this time.
  - `len` is the length of the message
  - `udata_ptr` is a customizable user data. We can assign it in this call, and in the next call it will be passed back to continue to use, the framework will not initialize and release it.

- `worker_handler` is the handler function of the child process, and its function prototype is the same as `master_handler`.

- `master_data` is the user-defined data matching the main process processing function. If not needed, `NULL` can be passed in.

- `worker_data` is the user-defined data matching the subprocess processing function. If not needed, `NULL` can be passed in.

- Return value: A `mln_ipc_cb_t *` pointer, which records information such as the IPC message processing function and message type.


We can also call

```c
void mln_ipc_handler_unregister(mln_ipc_cb_t *cb);
```

to unregister the IPC callback handlers.


Regarding the `mln_fork_t` type:

```c
struct mln_fork_s {
    struct mln_fork_s       *prev;
    struct mln_fork_s       *next;
    mln_s8ptr_t             *args; //args of subprocess
    mln_tcp_conn_t           conn; //Communication link structure, socketpair is used as tcp
    pid_t                    pid; //child process id
    mln_u32_t                n_args;//number of argments
    mln_u32_t                state;//child process state
    mln_u32_t                msg_len;//message length
    mln_u32_t                msg_type;//message type
    mln_size_t               error_bytes;//bytes of error message
    void                    *msg_content;//message content
    enum proc_exec_type      etype;//Whether the child process needs to be replaced by the exec image (exec) or not
    enum proc_state_type     stype;//Whether the child process needs to be restarted after exiting
};
```



Since Melon is a master-multiple-slave model, a method is needed to traverse the list of subprocesses and send messages to them, so a function is provided to traverse all subprocess structures:

```c
int mln_fork_iterate(mln_event_t *ev, fork_iterate_handler handler, void *data);

typedef int (*fork_iterate_handler)(mln_event_t *, mln_fork_t *, void *);
```

- `ev` is an event handling structure related to messages from the main process.
- `handler` is the processing function of each child process node. The function has three parameters: the message-related event processing structure, the `mln_fork_t` structure of the child process, and the user-defined data (that is, the third parameter of `mln_fork_iterate` ).
- `data` user-defined data.



Other functions are also available in Melon:

- mln_fork_master_connection_get

  ```c
  mln_tcp_conn_t *mln_fork_master_connection_get(void);
  ```

  The TCP link structure used in the child process to communicate with the main process (socketpair is treated as TCP).

- mln_ipc_master_send_prepare

  ```c
  int mln_ipc_master_send_prepare(mln_event_t *ev, mln_u32_t type, void *buf, mln_size_t len, mln_fork_t *f_child);
  ```

  Used by the main process to send a message `buf` of length `len` class behavior `type` to the child process specified by `f_child`.

- mln_ipc_worker_send_prepare

  ```c
  int mln_ipc_worker_send_prepare(mln_event_t *ev, mln_u32_t type, void *msg, mln_size_t len);
  ```

  Used by the child process to send a message `buf` of length `len` class behavior `type` to the main process.

- mln_fork_master_ipc_handler_set

  ```c
  int mln_fork_master_ipc_handler_set(mln_u32_t type, ipc_handler handler, void *data);
  ```

  This function is used to set the IPC handler of the main process. In fact, this function is called inside `mln_ipc_handler_register` for setting, but the difference is that this function can be called either before or after the framework is initialized.

  But you need to make sure that this function needs to be called in the main process, and a segmentation fault caused by accessing `NULL` memory will occur in the child process.

- mln_fork_worker_ipc_handler_set

  ```c
  int mln_fork_worker_ipc_handler_set(mln_u32_t type, ipc_handler handler, void *data);
  ```

  This function is used to set the IPC handler for the child process. In fact, this function is called inside `mln_ipc_handler_register` for setting, but the difference is that this function can be called either before or after the framework is initialized.

  But you need to make sure that this function needs to be called in the child process, and the settings in the main process will not be used.
