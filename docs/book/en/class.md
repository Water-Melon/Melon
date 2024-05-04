## Class Template

Class template is used to provide a programming experience similar to object-oriented programming. The module provides several macros to implement object-oriented programming style.

This module is not supported in the MSVC.


### Header file

```c
#include "mln_class.h"
```



### Module

`class`



### Macros

#### class

```c
class(type, constructor, destructor, ...);
```

Description: Defines a class with the following macro parameters:

- `type`: The type name of the class, i.e., the class name.
- `constructor`: The constructor of the class. The first parameter must be a `void *`, which is actually a pointer to the object of this class. However, due to compiler processing order and issues with function and struct definitions, the type specified by `type` cannot be directly used here. The remaining parameters and return value type are user-defined. The return value of the constructor will be ignored.
- `destructor`: The destructor of the class. Similar to `constructor`, the first parameter must be a `void *`, and the remaining parameters are user-defined, with the return value being ignored.
- `...`: The member definitions of the class, enclosed in `{}`.

Return value: None.



#### new

```c
new(type, ...);
```

Description: Instantiate an object of a class. `type` is the name of the class, and `...` are the arguments corresponding to the parameters in the constructor, excluding the first parameter.

Return value: Returns a pointer to the object if successful, otherwise returns `NULL`. Note that a failure in the constructor does not result in a `NULL` return value, as the return value of the constructor is ignored.



#### delete

```c
delete(o, ...);
```

Description: Free an object. `o` is a pointer to the object, and `...` are the remaining arguments in the destructor, excluding the first parameter.

Return value: None



### Example

```c
#include "mln_class.h"
#include <stdio.h>

typedef void (*func_t)(void *);
static void _constructor(void *o, int a, func_t func);
static void _destructor(void *o);

class(F, _constructor, _destructor, {
    int a;
    func_t f;
});

static void fcall(void *o)
{
    printf("in function call\n");
}

static void _constructor(void *o, int a, func_t func)
{
    printf("constructor\n");
    F *f = (F *)o;
    f->a = a;
    f->f = func;
}

static void _destructor(void *o)
{
    printf("destructor\n");
}

int main(void)
{
     F *f = new(F, 1, fcall);
     printf("%d\n", f->a);
     f->f(f);
     delete(f);
     return 0;
}
```

Note that function typedef, and constructor and destructor declarations must be given before the `class` macro. All function definitions must come after the `class` macro.
