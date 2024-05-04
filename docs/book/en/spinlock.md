## Spinlock

The spinlock in Melon will use different implementations according to different compilers and different CPU architectures.

This module is not supported in the MSVC.


### Header file

```c
#include "mln_utils.h"
#include "mln_types.h"
```



### Module

`utils`



### Functions/Macros

The `lock_ptr` in the following functions are of type `mln_spin_t`, which is defined in `mln_types.h`.



#### mln_spin_init

```c
mln_spin_init(lock_ptr)
```

Description: Initialize lock `lock_ptr`.

Return value: return `0` on success, otherwise return `not 0`



#### mln_spin_destroy

```c
mln_spin_destroy(lock_ptr)
```

Description: Destroy the lock `lock_ptr`.

Return value: return `0` on success, otherwise return `not 0`



#### mln_spin_trylock

```c
 mln_spin_trylock(lock_ptr)
```

Description: Attempt to lock a spinlock. If the lock resource is occupied, it will return immediately.

Return value: return `0` if the lock is successful, otherwise return `non-0`



#### mln_spin_lock

```c
mln_spin_lock(lock_ptr)
```

Description: Lock the lock resource. If the lock resource is occupied, wait for it to become available and lock it.

Return value: none



#### mln_spin_unlock

```c
mln_spin_unlock(lock_ptr)
```

Description: Release the lock.

Return value: none



### Example

his is only to show the use, and the rationality of the example single thread is not considered for the time being.

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

