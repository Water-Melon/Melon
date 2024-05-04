## 类模板

类模板用于提供一种类似面向对象编程的编程体验。模块提供了若干宏来实现面向对象的编程风格。

本模块在MSVC环境中暂不支持。



### 头文件

```c
#include "mln_class.h"
```



### 模块名

`class`



### 宏

#### class

```c
class(type, constructor, destructor, ...);
```

描述：定义一个类。宏参数描述如下：

- `type` 这是类的类型名，也就是类名称。
- `constructor` 这是类的构造函数。这个构造函数第一个参数必须是一个`void *`参数，这个参数实际上是这个类的对象指针。但由于编译时编译器处理顺序和函数以及结构体定义顺序问题，这里无法直接使用`type`指定的类型。构造函数的剩余参数以及返回值类型都是开发者自定义的。构造函数的返回值会被忽略。
- `destructor` 这是类的析构函数。与`constructor`一样，第一个参数必须是一个void *参数，其余参数都是开发者自定义的，且返回值会被忽略。
- `...` 这是类中的成员定义，也就是以`{}`扩住的结构体成员定义语句。

返回值：无



#### new

```c
new(type, ...);
```

描述：实例化一个类对象。`type`是类的名称，`...`是构造函数中除第一个参数外的其他参数对应的实参。

返回值：成功则返回对象指针，否则返回`NULL`。注意，构造函数处理失败是不会导致返回值为`NULL`的，因为构造函数的返回值会被忽略。



#### delete

```c
delete(o, ...);
```

描述：释放一个对象。`o`是对象指针，`...`是析构函数中除第一个参数外的剩余实参。

返回值：无



### 示例

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

注意，这里函数指针、构造函数和析构函数的声明必须放在宏`class`前。而所有函数定义必须放在`class`之后。
