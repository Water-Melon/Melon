## 配置



由于配置文件的加载是在Melon的初始化函数中被自动加载的，因此多数数据结构及函数是无需开发者关心的，这里仅给出开发者所需要的结构定义和函数声明。

在Melon中，配置被划分为两层，每一层为一个`域`。最外层为`main`域，在`main`中允许出现以`名称{...}`扩住的子域，然而子域中不允许再出现子域。但在程序中，`main`与子域其实为同级关系，而非包含关系。

每个域内是若干`配置项`，配置项由`配置指令名`与`配置参数`组成。

Melon的配置文件`melon.conf`会被安装在安装路径下的`conf`子目录中。



### 头文件

```c
#include "mln_conf.h"
```



### 相关结构

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
    } type; //配置项参数类型
    union {
        mln_string_t *s;
        mln_s8_t c;
        mln_u8_t b;
        mln_sauto_t i;
        float f;
    } val; //配置项参数数据
};
```

在Melon中，`配置参数`分为5种类型（忽略NONE），分别为：

- 字符串：以`""`扩住的字符集
- 字符：以`''`扩住的字符
- 布尔开关：`on`或`off`
- 整型：十进制整数
- 浮点型：十进制形式的实数



### 函数



#### mln_get_conf

```c
mln_conf_t *mln_get_conf(void);
```

描述：获取全局配置结构。

返回值：`mln_conf_t`指针，若为`NULL`，则表明Melon并未进行初始化。



#### search

```c
typedef mln_conf_domain_t *(*mln_conf_domain_cb_t) (mln_conf_t *, char *);
typedef mln_conf_cmd_t    *(*mln_conf_cmd_cb_t)    (mln_conf_domain_t *, char *);
typedef mln_conf_item_t   *(*mln_conf_item_cb_t)   (mln_conf_cmd_t *, mln_u32_t);
```

描述：

在Melon中，所有配置都会被加载进`mln_conf_t`结构中，随后的获取则是通过这三个search函数进行的。这三个search函数指针依次分别为`mln_conf_t`，`mln_conf_domain_t`以及`mln_conf_cmd_t`中的`search`成员。故在使用时，则是形如：

```
mln_conf_domain_t *d = conf->search(conf, "main");
```

这三个search函数分别是：

- 从全局`mln_conf_t`中获取某个域结构`mln_conf_domain_t`
- 从某个域(`mln_conf_domain_t`)中获取对应的配置指令项`mln_conf_cmd_t`
- 从某个指令(`mln_conf_cmd_t`)中获取某个参数`mln_conf_item_t`

其中，最后一个search的第二个参数为参数下标，且下标从`1`开始而非`0`。

在本篇末尾处将给出使用示例。

返回值：

正常情况下，只要所需配置在配置文件中，且配置被正常初始化，那么返回值则必不为`NULL`。



#### insert

```c
typedef mln_conf_domain_t *(*mln_conf_domain_cb_t) (mln_conf_t *, char *);
typedef mln_conf_cmd_t    *(*mln_conf_cmd_cb_t)    (mln_conf_domain_t *, char *);
```

描述：

在Melon配置中，允许开发者向`mln_conf_t`和`mln_conf_domain_t`区域中插入配置项。可以直接调用这两个结构中的`insert`函数指针成员来实现配置项的插入。例如：

```
mln_conf_domain_t *d = conf->insert(conf, "domain1");
```

返回值：

返回插入的配置项结构体指针，失败返回`NULL`



#### remove

```c
typedef mln_conf_domain_t *(*mln_conf_domain_cb_t) (mln_conf_t *, char *);
typedef mln_conf_cmd_t    *(*mln_conf_cmd_cb_t)    (mln_conf_domain_t *, char *);
```

描述：

在Melon配置中，允许开发者从`mln_conf_t`和`mln_conf_domain_t`区域中删除配置项。可以直接调用这两个结构中的`remove`函数指针成员来实现配置项的删除。例如：

```
conf->remove(conf, "domain1");
```

返回值：

无。尽管函数指针定义存在返回值，但实际处理函数的返回值部分为`void`



#### mln_conf_set_hook

```c
mln_conf_hook_t *mln_conf_set_hook(reload_handler reload, void *data);

typedef int (*reload_handler)(void *);
```

描述：

Melon配置支持重载，重载的方式是设置重载回调函数，且允许设置多个。当执行重载时，新配置加载后，将调用这些回调函数。

回调函数的参数即为`mln_conf_set_hook`的第二个参数`data`。

返回值：

- `mln_conf_set_hook`：成功返回`mln_conf_hook_t`回调句柄，否则返回`NULL`。
- 回调函数：成功返回`0`，否则返回`-1`。



#### mln_conf_unset_hook

```c
void mln_conf_unset_hook(mln_conf_hook_t *hook);
```

描述：删除已设置的配置重载回调函数。

返回值：无



#### mln_conf_get_ncmd

```c
mln_u32_t mln_conf_get_cmdNum(mln_conf_t *cf, char *domain);
```

描述：获取某个域下配置项的个数。

返回值：配置项个数



#### mln_conf_get_cmds

```c
void mln_conf_get_cmds(mln_conf_t *cf, char *domain, mln_conf_cmd_t **vector);
```

描述：获取某个域内的全部配置项，这些配置项将被存放在`vector`中，`vector`需要在调用前分配好，配置项个数可以通过`mln_conf_get_ncmd`预先获取到。

返回值：无



#### mln_conf_get_narg

```c
mln_u32_t mln_conf_get_argNum(mln_conf_cmd_t *cc);
```

描述：获取某个配置项的参数个数。

返回值：指令项参数个数



### 示例

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
  d = cf->search(cf, "main"); //如果main都不存在，那说明配追初始化有严重问题
  c = d->search(cf, "daemon"); //这里我们获取daemon配置项
  if（c == NULL) {
    mln_log(error, "daemon not exist.\n");
    return -1;//出错返回
  }
  i = c->search(c, 1); //daemon在配置文件中只有一个参数，配置参数的下标从1开始
  if (i == NULL) {
    mln_log(error, "Invalid daemon argument.\n");
    return -1;
  }
  if (i->type != CONF_BOOL) { //daemon 参数应该为布尔开关量
    mln_log(error, "Invalid type of daemon argument.\n");
    return -1;
  }
  mln_log(debug, "%u\n", i->val.b); //输出布尔量的值

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

在使用日志输出时，请确保Melon配置及文件中日志文件的父目录路径是否存在。
