## 自旋锁

Melon中的自旋锁会根据不同编译器和不同CPU架构选用不同的实现。



### 头文件

```c
#include "mln_defs.h"
#include "mln_types.h"
```



### 函数/宏

如下函数中的`lock_ptr`均为`mln_lock_t`类型，该类型定义在`mln_types.h`中。



#### MLN_LOCK_INIT

```c
MLN_LOCK_INIT(lock_ptr)
```

描述：初始化锁`lock_ptr`。

返回值：成功返回`0`，否则返回`非0`



#### MLN_LOCK_DESTROY

```c
MLN_LOCK_DESTROY(lock_ptr)
```

描述：销毁锁`lock_ptr`。

返回值：成功返回`0`，否则返回`非0`



#### MLN_TRYLOCK

```c
 MLN_TRYLOCK(lock_ptr)
```

描述：尝试锁定自旋锁。若锁资源被占用则会立即返回。

返回值：成功锁住则返回`0`，否则返回`非0`



#### MLN_LOCK

```c
MLN_LOCK(lock_ptr)
```

描述：锁定锁资源。若锁资源被占用，则等待其可用并将其锁定。

返回值：无



#### MLN_UNLOCK

```c
MLN_UNLOCK(lock_ptr)
```

描述：释放锁。

返回值：无



### 示例

这里仅展示使用，暂不考虑示例单线程合理性。

```c
#include "mln_defs.h"
#include "mln_types.h"

int main(int argc, char *argv[])
{
    mln_lock_t lock;

    MLN_LOCK_INIT(&lock);
    MLN_LOCK(&lock);
    MLN_UNLOCK(&lock);
    MLN_LOCK_DESTROY(&lock);
    return 0;
}
```

