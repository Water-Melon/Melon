## 自旋锁

Melon中的自旋锁会根据不同编译器和不同CPU架构选用不同的实现。

本模块在MSVC环境中暂不支持。


### 头文件

```c
#include "mln_utils.h"
#include "mln_types.h"
```



### 模块名

`utils`



### 函数/宏

如下函数中的`lock_ptr`均为`mln_spin_t`类型，该类型定义在`mln_types.h`中。



#### mln_spin_init

```c
mln_spin_init(lock_ptr)
```

描述：初始化锁`lock_ptr`。

返回值：成功返回`0`，否则返回`非0`



#### mln_spin_destroy

```c
mln_spin_destroy(lock_ptr)
```

描述：销毁锁`lock_ptr`。

返回值：成功返回`0`，否则返回`非0`



#### mln_spin_trylock

```c
 mln_spin_trylock(lock_ptr)
```

描述：尝试锁定自旋锁。若锁资源被占用则会立即返回。

返回值：成功锁住则返回`0`，否则返回`非0`



#### mln_spin_lock

```c
mln_spin_lock(lock_ptr)
```

描述：锁定锁资源。若锁资源被占用，则等待其可用并将其锁定。

返回值：无



#### mln_spin_unlock

```c
mln_spin_unlock(lock_ptr)
```

描述：释放锁。

返回值：无



### 示例

这里仅展示使用，暂不考虑示例单线程合理性。

```c
#include "mln_utils.h"
#include "mln_types.h"

int main(int argc, char *argv[])
{
    mln_spin_t lock;

    mln_spin_init(&lock);
    mln_spin_lock(&lock);
    mln_spin_unlock(&lock);
    mln_spin_destroy(&lock);
    return 0;
}
```

