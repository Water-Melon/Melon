## Configuration



Since the loading of the configuration file is automatically loaded in the initialization function of Melon, most data structures and functions do not need to be concerned by developers, and only the structure definitions and function declarations required by developers are given here.

In Melon, configuration is divided into two layers, each layer is a `domain`. The outermost layer is the `main` domain. Subdomains extended with `name{...}` are allowed in `main`, but subdomains are not allowed in subdomains. But in the program, `main` and subdomains are actually siblings, not containment.

Each domain contains several `configuration items`, which consist of `configuration command name` and `configuration parameters`.

The Melon configuration file `melon.conf` will be installed in the `conf` subdirectory of the installation path.



### Header file

```c
#include "mln_conf.h"
```



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



#### mln_get_conf

```c
mln_conf_t *mln_get_conf(void);
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

```
mln_conf_domain_t *d = conf->search(conf, "main");
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

```
mln_conf_domain_t *d = conf->insert(conf, "domain1");
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

```
conf->remove(conf, "domain1");
```

return value:

None. Although the function pointer definition has a return value, the return value part of the actual processing function is `void`



#### mln_conf_set_hook

```c
mln_conf_hook_t *mln_conf_set_hook(reload_handler reload, void *data);

typedef int (*reload_handler)(void *);
```

Description:

Melon configuration supports overloading. The overloading method is to set overloaded callback functions, and multiple settings are allowed. When the reload is performed, these callback functions are called after the new configuration is loaded.

The parameter of the callback function is the second parameter `data` of `mln_conf_set_hook`.

return value:

- `mln_conf_set_hook`: Returns the `mln_conf_hook_t` callback handle on success, otherwise returns `NULL`.
- Callback function: return `0` on success, otherwise return `-1`.



#### mln_conf_unset_hook

```c
void mln_conf_unset_hook(mln_conf_hook_t *hook);
```

Description: Remove the set configuration overload callback function.

Return value: none



#### mln_conf_get_ncmd

```c
mln_u32_t mln_conf_get_cmdNum(mln_conf_t *cf, char *domain);
```

Description: Get the number of configuration items in a domain.

Return value: the number of configuration items



#### mln_conf_get_cmds

```c
void mln_conf_get_cmds(mln_conf_t *cf, char *domain, mln_conf_cmd_t **vector);
```

Description: Get all configuration items in a domain. These configuration items will be stored in `vector`. The `vector` needs to be allocated before calling. The number of configuration items can be obtained in advance through `mln_conf_get_ncmd`.

Return value: none



#### mln_conf_get_narg

```c
mln_u32_t mln_conf_get_argNum(mln_conf_cmd_t *cc);
```

Description: Get the number of parameters of a configuration item.

Return value: the number of command item parameters



### Example

```c
#include <stdio.h>
#include "mln_core.h"

static int global_init(void)
{
  mln_conf_t *cf;
  mln_conf_domain_t *d;
  mln_conf_cmd_t *c;
  mln_conf_item_t *i;
  
  cf = mln_get_conf();
  d = cf->search(cf, "main"); //If the main does not exist, it means that there is a serious problem with the configuration initialization.
  c = d->search(cf, "daemon"); //Here we get the configuration item daemon
  if（c == NULL) {
    mln_log(error, "daemon not exist.\n");
    return -1;//return error
  }
  i = c->search(c, 1); //The daemon has only one parameter in the configuration file, and the subscript of the configuration parameter starts from 1
  if (i == NULL) {
    mln_log(error, "Invalid daemon argument.\n");
    return -1;
  }
  if (i->type != CONF_BOOL) { //daemon The parameter should be boolean
    mln_log(error, "Invalid type of daemon argument.\n");
    return -1;
  }
  mln_log(debug, "%u\n", i->val.b); //output boolean value

  return 0;
}

int main(int argc, char *argv[])
{
    struct mln_core_attr cattr;
    cattr.argc = argc;
    cattr.argv = argv;
    cattr.global_init = global_init;
    cattr.master_process = NULL;
    cattr.worker_process = NULL;
    return mln_core_init(&cattr);
}
```

When using log output, make sure that the parent directory path of the log file in the Melon configuration exists.
