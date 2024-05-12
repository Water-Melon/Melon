## 配置




### 说明
这里仅给出开发者所需要的结构定义和函数声明。

在Melon中，配置被划分为两层，每一层为一个`域`。最外层为`main`域，在`main`中允许出现以`名称{...}`扩住的子域，然而子域中不允许再出现子域。但在程序中，`main`与子域其实为同级关系，而非包含关系。

每个域内是若干`配置项`，配置项由`配置指令名`与`配置参数`组成。

Melon的配置文件`melon.conf`会被安装在安装路径下的`conf`子目录中。

在配置文件中，允许出现无效配置项，即：按照上述要求书写配置项，但在程序中不存在对应解析和使用的配置项。因为在Melon中，配置文件中的配置项在初始化时，仅仅是被加载到对应数据结构中，但未对其做任何验证。例如：`framework`配置项就是在配置加载阶段放入数据结构中，然后在随后的初始化阶段对其验证和使用的。因此，对于用户自定义配置，也是一样的。用户可以在配置中写入自己设计的配置内容，然后在代码中调用配置模块函数来操作这些配置（增、删、改、查）。

目前，现有已被Melon库验证和使用的配置项有：

| 配置项           | 说明                                                         |
| ---------------- | ------------------------------------------------------------ |
| `log_level`      | 日志级别。这个配置项有如下取值：`"none"`, `"report"`, `"debug"`, `"warn"`, `"error"`，配置级别也是逐级递增的。 |
| `user`           | 设置Melon宿主进程及其子进程的用户ID，这个配置项需要`root`权限。 |
| `daemon`         | 设置是否引导程序成为后台进程。取值有两个：`on`和`off`。      |
| `core_file_size` | 设置core文件大小。若其参数为数字，则代表core文件字节大小；若为字符串，则其取值只能是`"unlimited"`，表示无限制。 |
| `max_nofile`     | 设置进程最大文件描述符数量。其取值为整数。若需要开启超过1024个文件描述符数量时，需要有`root`权限。 |
| `worker_proc`    | 设置工作进程数量。取值为整数。这个值仅在启用Melon多进程框架时被使用。 |
| `framework`      | 设置Melon的框架功能。取值有：`"multiprocess"`-多进程框架，`"multithread"`-多线程框架，`off`-不启用框架。 |
| `log_path`       | 日志文件路径。参数为字符串类型。                             |
| `trace_mode`     | 设置是否启用动态跟踪模式。参数值有两种：字符串类型则是动态跟踪的处理脚本路径；`off`为不启用。 |
| `proc_exec`      | 这是一个`domain`，用于管理所有使用Melon拉起的其他进程（使用`fork`+`exec`启动的）。 |
| `keepalive`      | 这个配置项仅在`proc_exec`域内有效，用于告知主进程要对该进程的存货状态进行监控，若进程退出则需要重新拉起。 |
| `thread_exec`    | 这是一个`domain`，用于管理所有使用Melon拉起的子线程。        |
| `restart`        | 这个配置项仅在thread_exec中使用，用于告知主线程，若该子线程退出则需要重新拉起。 |
| `default`        | 这个配置项仅在`proc_exec`和`thread_exec`中使用，用于告知主进程或主线程，当进程或线程退出后，不做其他处理。 |



### 关于配置重载

配置模块支持配置重载，重载功能已经集成进Melon库的初始化流程中了。

当使用Melon库的`mln_framework_init`进行初始化后，就可以向库的宿主进程使用`SIGUSR2`信号触发配置重载。

重载配置后，会调用开发者设置的回调函数（使用`mln_conf_hook_set`设置）来做一些处理，例如更新内存中的某些变量值。



### 头文件

```c
#include "mln_conf.h"
```



### 模块名

`conf`



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



#### mln_conf_load

```c
int mln_conf_load(void);
```

描述：加载Melon配置文件中的配置。如果需要加载配置，则这个函数应该在程序尚未创建任何线程前被调用。

返回值：成功则返回`0`，否则返回`-1`。



#### mln_conf

```c
mln_conf_t *mln_conf(void);
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

```c
mln_conf_domain_t *d = conf->search(conf, "main"); //查找名为main的配置域
mln_conf_cmd_t *cc = d->search(d, "framework"); //查找名为framework的配置命令
mln_conf_item_t *ci = cc->search(cc, 1); //查找framework命令的第一个参数项
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

```c
mln_conf_domain_t *d = conf->insert(conf, "domain1"); //向配置中插入一个名为domain1的配置域
mln_conf_cmd_t *c = d->insert(d, "test_cmd"); //向domain1这个配置域里面插入一个名为test_cmd的配置命令
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

```c
d->remove(d, "test_cmd"); //从配置域d中删除名为test_cmd的配置命令
conf->remove(conf, "domain1"); //从配置中删除名为domain1的配置域
```

返回值：

无。尽管函数指针定义存在返回值，但实际处理函数的返回值部分为`void`



#### update

```c
typedef int (*mln_conf_item_update_cb_t) (mln_conf_cmd_t *, mln_conf_item_t *, mln_u32_t);
```

描述：

在Melon配置中，允许开发者更新`mln_conf_cmd_t`中的参数列表。第一个参数为`mln_conf_cmd_t`指针，第二个参数为`mln_conf_item_t`类型数组，第三个参数为数组元素个数。参数二可以为栈上内存，因为在该函数中会自行复制一份参数二的数据进行保存和维护。

```c
mln_conf_item_t items[] = {
  {
    CONF_BOOL,
    0
  },
};
cmd->update(cmd, items, 1);
```

返回值：

成功返回`0`，否则返回`-1`



#### mln_conf_hook_set

```c
mln_conf_hook_t *mln_conf_hook_set(reload_handler reload, void *data);

typedef int (*reload_handler)(void *);
```

描述：

Melon配置支持重载，重载的方式是设置重载回调函数，且允许设置多个。当执行重载时，新配置加载后，将调用这些回调函数。

回调函数的参数即为`mln_conf_hook_set`的第二个参数`data`。

返回值：

- `mln_conf_hook_set`：成功返回`mln_conf_hook_t`回调句柄，否则返回`NULL`。
- 回调函数：成功返回`0`，否则返回`-1`。



#### mln_conf_hook_unset

```c
void mln_conf_hook_unset(mln_conf_hook_t *hook);
```

描述：删除已设置的配置重载回调函数。

返回值：无



#### mln_conf_cmd_num

```c
mln_u32_t mln_conf_cmd_num(mln_conf_t *cf, char *domain);
```

描述：获取某个域下配置项的个数。

返回值：配置项个数



#### mln_conf_cmds

```c
void mln_conf_cmds(mln_conf_t *cf, char *domain, mln_conf_cmd_t **vector);
```

描述：获取某个域内的全部配置项，这些配置项将被存放在`vector`中，`vector`需要在调用前分配好，配置项个数可以通过`mln_conf_cmd_num`预先获取到。

返回值：无



#### mln_conf_arg_num

```c
mln_u32_t mln_conf_arg_num(mln_conf_cmd_t *cc);
```

描述：获取某个配置项的参数个数。

返回值：指令项参数个数



#### mln_conf_is_empty

```c
mln_conf_is_empty(cf);
```

描述：检查配置是否为空。

返回值：

- `0` - 非空
- `非0` - 空



### 示例

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

在使用日志输出时，请确保Melon配置及文件中日志文件的父目录路径是否存在。
