## Configuration



Since the loading of the configuration file is automatically loaded in the initialization function of Melon, most data structures and functions do not need to be concerned by developers. So only the structure definitions and function declarations required by developers are given here.

In Melon, configuration is divided into two layers, each layer is a `domain`. The outermost layer is the `main` domain. Subdomains extended with `name{...}` are allowed in `main`, but subdomains are not allowed in subdomains. But in the program, `main` and subdomains are actually siblings, not containment.

Each domain contains several `configuration items`, which consist of `configuration command name` and `configuration parameters`.

The Melon configuration file `melon.conf` will be installed in the `conf` subdirectory of the installation path.

In the configuration file, unused configuration directives are allowed, that is, configuration directives are written according to the above rules, but there is no corresponding configuration directive for parsing and use in the program. Because in Melon, the configuration directives in the configuration file are only loaded into the corresponding data structure during initialization, but no verification is performed on it. For example: the configuration directive `framework` is put into the data structure during the configuration loading phase, and then it's verified and used in the subsequent initialization phase. Therefore, it is the same to user-defined configurations. Users can write their own configuration content in the configuration file, and then call the functions of configuration module in the code to operate these configurations (create, delete, update, read).

Currently, the existing configuration items that have been verified and used by the Melon library are:

| Configuration domains and directives | Description |
| ---------------- | -------------------------------- ---------------------------- |
| `log_level` | The log level. This configuration item has the following values: `"none"`, `"report"`, `"debug"`, `"warn"`, `"error"`. |
| `user` | Set the user ID of the Melon host process and its child processes. This configuration item requires `root` permission. |
| `daemon` | Sets whether the bootstrap should be a daemon process. There are two values: `on` and `off`. |
| `core_file_size` | Set the core file size. If the parameter is a integer, it represents the byte size of the core file; if it is a string, its value can only be `"unlimited"`, which means unlimited. |
| `max_nofile` | Set the maximum number of file descriptors for a process. Its value is an integer. If you need to open more than 1024 file descriptors, you need `root` permission. |
| `worker_proc` | Set the number of worker processes. The value is an integer. This value is only used when the Melon multi-processing framework is enabled. |
| `framework` | Sets the framework capabilities of Melon. Values are: `"multiprocess"` - multi-process framework, `"multithread"` - multi-thread framework, `off` - disable the framework. |
| `log_path` | Log file path. The parameter is of type string. |
| `trace_mode` | Set whether to enable dynamic trace mode. There are two parameter values: the string type is the processing script path of dynamic tracing; `off` means not enabled. |
| `proc_exec` | This is a `domain` that manages all other processes started by Melon (started using `fork`+`exec`). |
| `keepalive` | This configuration directive is only valid in the `proc_exec` domain, and is used to inform the main process to monitor the process status. If the process exits, it needs to be restarted. |
| `thread_exec` | This is a `domain` used to manage all child threads pulled up by Melon. |
| `restart` | This configuration directive is only used in thread_exec to inform the main thread that if the child thread exits, it needs to be restarted. |
| `default` | This configuration directive is only used in `proc_exec` and `thread_exec`, and is used to inform the main process or main thread that do nothing when the process or thread exits. |



### About configuration overloading

The configuration module supports configuration overloading, and the overloading function has been integrated into the initialization process of the Melon library.

After initialization using the `mln_framework_init` of the Melon library, the `SIGUSR2` signal can be used to trigger configuration reloading to the host process of the library.

After the configuration is reloaded, the callback function set by the developer (using `mln_conf_hook_set`) will be called to do some processing, such as updating some variable values in memory.



### Header file

```c
#include "mln_conf.h"
```



### Module

`conf`



### Structure

```c
typedef struct mln_conf_item_s    mln_conf_item_t;
struct mln_conf_item_s {
    enum {
        CONF_NONE = 0,
        CONF_STR,
        CONF_CHAR,
        CONF_BOOL,
        CONF_INT,
        CONF_FLOAT
    } type; //Configuration item parameter type
    union {
        mln_string_t *s;
        mln_s8_t c;
        mln_u8_t b;
        mln_sauto_t i;
        float f;
    } val; //Configuration item parameter data
};
```

- In Melon, `configuration parameters` are divided into 5 types (ignoring NONE), which are:

  - string: character set extended with `""`
  - Characters: Characters enclosed by `''`
  - boolean switch: `on` or `off`
  - Integer: Decimal integer
  - Float: Real numbers in decimal form



### Functions



#### mln_conf_load

```c
int mln_conf_load(void);
```

Description: Load Melon configuration from config file. If you need to load configuration, please make sure this function to be called before any `pthread_create`.

Return value: `0` on success, otherwise `-1` returned.



#### mln_conf

```c
mln_conf_t *mln_conf(void);
```

Description: Get the global configuration structure.

Return value: `mln_conf_t` pointer, if it is `NULL`, it means that Melon is not initialized.



#### search

```c
typedef mln_conf_domain_t *(*mln_conf_domain_cb_t) (mln_conf_t *, char *);
typedef mln_conf_cmd_t    *(*mln_conf_cmd_cb_t)    (mln_conf_domain_t *, char *);
typedef mln_conf_item_t   *(*mln_conf_item_cb_t)   (mln_conf_cmd_t *, mln_u32_t);
```

Description:

In Melon, all configuration is loaded into the `mln_conf_t` structure, and subsequent retrieval is done through these three search functions. The three search function pointers are respectively `mln_conf_t`, `mln_conf_domain_t` and the `search` member in `mln_conf_cmd_t`. Therefore, when using it, it looks like this:

```c
mln_conf_domain_t *d = conf->search(conf, "main"); //Look for a configuration domain named main
mln_conf_cmd_t *cc = d->search(d, "framework"); //Look for a configuration command named framework
mln_conf_item_t *ci = cc->search(cc, 1); //Find the first parameter item of the framework command
```

The three search functions are:

- Get a domain structure `mln_conf_domain_t` from the global `mln_conf_t`
- Get the corresponding configuration command item `mln_conf_cmd_t` from a domain (`mln_conf_domain_t`)
- Get a parameter `mln_conf_item_t` from a command (`mln_conf_cmd_t`)

Among them, the second parameter of the last search is the parameter subscript, and the subscript starts from `1` instead of `0`.

A usage example is given at the end of this article.

return value:

In usual, as long as the required configuration is in the configuration file and the configuration is initialized normally, the return value must not be `NULL`.



#### insert

```c
typedef mln_conf_domain_t *(*mln_conf_domain_cb_t) (mln_conf_t *, char *);
typedef mln_conf_cmd_t    *(*mln_conf_cmd_cb_t)    (mln_conf_domain_t *, char *);
```

Description:

In Melon configuration, developers are allowed to insert configuration items into `mln_conf_t` and `mln_conf_domain_t` areas. You can directly call the `insert` function pointer in these two structures to implement the insertion of configuration items. E.g:

```c
mln_conf_domain_t *d = conf->insert(conf, "domain1"); //Insert a configuration domain called domain1 into the configuration
mln_conf_cmd_t *c = d->insert(d, "test_cmd"); //Insert a configuration command named test_cmd into the domain1 configuration domain
```

return value:

Returns the pointer to the inserted configuration item structure, or `NULL` on failure



#### remove

```c
typedef mln_conf_domain_t *(*mln_conf_domain_cb_t) (mln_conf_t *, char *);
typedef mln_conf_cmd_t    *(*mln_conf_cmd_cb_t)    (mln_conf_domain_t *, char *);
```

Description:

In Melon configuration, developers are allowed to delete configuration items from `mln_conf_t` and `mln_conf_domain_t` areas. You can directly call the `remove` function pointer in these two structures to delete configuration items. E.g

```c
d->remove(d, "test_cmd"); //Delete the configuration command named test_cmd from configuration domain d
conf->remove(conf, "domain1"); //Delete the configuration domain named domain1 from the configuration
```

return value:

None. Although the function pointer definition has a return value, the return value part of the actual processing function is `void`



#### update

```c
typedef int (*mln_conf_item_update_cb_t) (mln_conf_cmd_t *, mln_conf_item_t *, mln_u32_t);
```

Description:

In Melon configuration, allows developers to update the argument list in `mln_conf_cmd_t`. The first argument is `mln_conf_cmd_t` pointer, the second argument is `mln_conf_item_t` type array, and the third argument is the number of array elements. Argument 2 can be the memory on the stack, because in this function, it will be duplicated.

```c
mln_conf_item_t items[] = {
  {
    CONF_BOOL,
    0
  },
};
cmd->update(cmd, items, 1);
```

Return value:

`0` on sucess, otherwise `-1` returned



#### mln_conf_hook_set

```c
mln_conf_hook_t *mln_conf_hook_set(reload_handler reload, void *data);

typedef int (*reload_handler)(void *);
```

Description:

Melon configuration supports overloading. The overloading method is to set overloaded callback functions, and multiple settings are allowed. When the reload is performed, these callback functions are called after the new configuration is loaded.

The parameter of the callback function is the second parameter `data` of `mln_conf_hook_set`.

return value:

- `mln_conf_hook_set`: Returns the `mln_conf_hook_t` callback handle on success, otherwise returns `NULL`.
- Callback function: return `0` on success, otherwise return `-1`.



#### mln_conf_hook_unset

```c
void mln_conf_hook_unset(mln_conf_hook_t *hook);
```

Description: Remove the set configuration overload callback function.

Return value: none



#### mln_conf_cmd_num

```c
mln_u32_t mln_conf_cmd_num(mln_conf_t *cf, char *domain);
```

Description: Get the number of configuration items in a domain.

Return value: the number of configuration items



#### mln_conf_cmds

```c
void mln_conf_cmds(mln_conf_t *cf, char *domain, mln_conf_cmd_t **vector);
```

Description: Get all configuration items in a domain. These configuration items will be stored in `vector`. The `vector` needs to be allocated before calling. The number of configuration items can be obtained in advance through `mln_conf_cmd_num`.

Return value: none



#### mln_conf_arg_num

```c
mln_u32_t mln_conf_arg_num(mln_conf_cmd_t *cc);
```

Description: Get the number of parameters of a configuration item.

Return value: the number of command item parameters



#### mln_conf_is_empty

```c
mln_conf_is_empty(cf);
```

Description: Check if the configuration is empty.

Return values:

- `0` - Not empty
- `Non-zero` - Empty



### Example

```c
#include <stdio.h>
#include "mln_conf.h"

int main(int argc, char *argv[])
{
    mln_conf_t *cf;
    mln_conf_domain_t *cd;
    mln_conf_cmd_t *cc;
    mln_conf_item_t *ci;

    if (mln_conf_load() < 0) {
        fprintf(stderr, "Load configuration failed.\n");
        return -1;
    }

    cf = mln_conf();
    cd = cf->search(cf, "main");
    cc = cd->search(cd, "framework");
    if (cc == NULL) {
        fprintf(stderr, "framework not found.\n");
        return -1;
    }
    ci = cc->search(cc, 1);
    if (ci->type == CONF_BOOL && !ci->val.b) {
        printf("framework off\n");
    } else if (ci->type == CONF_STR) {
        printf("framework %s\n", ci->val.s->data);
    } else {
        fprintf(stderr, "Invalid framework value.\n");
    }
    return 0;
}
```

When using log output, make sure that the parent directory path of the log file in the Melon configuration exists.
