## 错误码管理

该模块用于生成错误返回值。该模块可以生成一个`int`型错误码，用来定位错误文件、错误行数以及错误类型。

其中，32位 `int` 中，8位用于表示错误码，14位表示行数，9位表示文件名下标。即，该`int`型值的使用限制为：

- 支持255个错误码（0xff为表留值）
- 支持每个文件16383行（0x3ffff为保留值）
- 支持511个文件（0x1ff为保留值）
- 仅针对文件名，而非文件路径名，因此应尽量避免不同目录下出现同名代码文件

超出该限制的情况下，程序并不会发生异常或者报错，而是会报出`Unknown ...`的错误，可以参考本章最后一小节的示例。




### 头文件

```c
#include "mln_error.h"
```



### 模块名

`error`



### 函数/宏



#### mln_error_init

```c
void mln_error_init(mln_string_t *filenames, mln_string_t *errmsgs, mln_size_t nfile, mln_size_t nmsg);
```

描述：为错误码模块进行初始化，需要注意，文件和错误信息的顺序很重要，因为后续生成的返回值都将依赖这一顺序的下标。

- `filenames`是`mln_string_t`类型数组，数组中每个元素都是一个文件名
- `errmsgs`是`mln_string_t`类型数组，数组中每个元素都是一个错误信息
- `nfile`是文件名个数
- `nmsg`是错误信息个数
**注意**：`filenames`和`errmsgs`中的每一个`mln_string_t`元素的`data`指针内容都要以`\0`结束。这意味着你必须使用`mln_string.h`中的函数或者宏来创建该数组。一个最简单且常用的例子就是下面的示例。

返回值：无



#### RET

```c
RET(code)
```

描述：根据给定的错误码`code`生成返回值。如果`code`为0，则表示无错。若`code`小于0则是不合法的错误值。若`code`大于0，则表示一个合法错误码。如果再此之前使用了`mln_error_callback_set`设置了回调函数，那么在返回值拼装完成后，回调函数会被调用。

返回值：0或一个负值，0表示无错误，负值表示出错。`msvc`环境下，返回值将直接赋值给参数`code`，而不是像函数一样返回。



#### CODE

```c
CODE(r)
```

描述：根据返回值`r`，获取其对应的错误码。其中，`r`为`RET`宏生成的返回值。

返回值：大于等于0的错误码



#### mln_error_string

```c
char *mln_error_string(int err, void *buf, mln_size_t len);
```

描述：将返回值`err`翻译成字符串，并放入由`buf`和`len`指定的缓冲区中。

返回值：错误信息，`buf`的首地址。



#### mln_error_callback_set

```c
void mln_error_callback_set(mln_error_cb_t cb, void *udata);
```

描述：设置一个回调函数和用户定义数据到这个模块的全局变量中，这个回调函数会在RET宏中，在返回值拼装完成后被调用。
返回值：无



### 示例

```c
#include "mln_error.h"

#define OK    0 //0是个特殊值，表示一切正常，由0生成的返回值就是0，不会增加额外信息
#define NMEM  1 //由使用者自行定义，但顺序必须与errs数组给出的错误信息顺序一致

int main(void)
{
    char msg[1024];
    mln_string_t files[] = {
        mln_string("a.c"),
    };
    mln_string_t errs[] = {
        mln_string("Success"),
        mln_string("No memory"),
    };
    mln_error_init(files, errs, sizeof(files)/sizeof(mln_string_t), sizeof(errs)/sizeof(mln_string_t));
    printf("%x %d [%s]\n", RET(OK), CODE(RET(OK)), mln_error_string(RET(OK), msg, sizeof(msg)));
    printf("%x %d [%s]\n", RET(NMEM), CODE(RET(NMEM)), mln_error_string(RET(NMEM), msg, sizeof(msg)));
    printf("%x %d [%s]\n", RET(2), CODE(RET(2)), mln_error_string(RET(2), msg, sizeof(msg)));
    printf("%x %d [%s]\n", RET(0xff), CODE(RET(0xff)), mln_error_string(RET(0xff), msg, sizeof(msg)));
    return 0;
}
```

输出

```
0 0 [Success]
ffffedff 1 [a.c:18:No memory]
ffffec01 255 [a.c:19:Unknown Code]
ffffeb01 255 [a.c:20:Unknown Code]
```
