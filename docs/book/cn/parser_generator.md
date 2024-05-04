## 语法解析器生成器

利用语法解析器生成器，我们可以生成一个符合LALR(1)文法的自定义规则的语法解析器，并利用解析器解析文本，亦可生成自定义的抽象语法树结构。

后续将给出一个简单的使用例子，较为完整的应用可参考`mln_lang_ast.c/h`中melang的实现。或者加入官方群咨询了解。



### 头文件

```c
#include "mln_parser_generator.h"
```


### 模块名

`parser_generator`



### 函数/宏

#### MLN_DECLARE_PARSER_GENERATOR

```c
#define MLN_DECLARE_PARSER_GENERATOR(SCOPE,PREFIX_NAME,TK_PREFIX,...);
```

描述：用于声明语法解析器生成器相关的函数、结构等内容。其中：

- `SCOPE`是声明的作用域关键字，如`static`。
- `PREFIX_NAME`是声明中的函数、结构命名的前缀，完成的名称由自定义前缀+固定后缀组成。
- `TK_PREFIX`是产生式(production)中使用的词素前缀，词素完整名由词素前缀+固定后缀组成（已有词素可参考`mln_lex.h`中定义的）。
- `...`，可变参部分为自定义关键字或运算符的词素名称，注意，词素名称先后顺序很重要，具体参考`mln_lex.h`中宏内容。

返回值：无

#### MLN_DEFINE_PASER_GENERATOR

```c
#define MLN_DEFINE_PARSER_GENERATOR(SCOPE,PREFIX_NAME,TK_PREFIX,...);
```

描述：用于定义语法解析器生成器相关的函数、结构等内容。其中：

- `SCOPE`是声明的作用域关键字，如`static`。
- `PREFIX_NAME`是定义中的函数、结构命名的前缀，完成的名称由自定义前缀+固定后缀组成。
- `TK_PREFIX`是产生式(production)中使用的词素前缀，词素完整名由词素前缀+固定后缀组成（已有词素可参考`mln_lex.h`中定义的）。
- `...`，可变参部分为自定义关键字或运算符的词素名称及其名称字符串，注意，词素名称先后顺序很重要，具体参考`mln_lex.h`中宏内容。

返回值：无

#### xxxx_parser_generate

```c
void *PREFIX_NAME##_parser_generate(mln_production_t *prod_tbl, mln_u32_t nr_prod);
```

描述：根据`prod_tbl`和`nr_prod`指定的产生式（production），生成LALR(1)状态转换表。状态转换表将被用于parse函数，用于对自定义语言语法进行检查。关于产生式结构可参考下面的简单示例。

返回值：返回状态转换表结构，若为`NULL`则表示出错。

#### xxxx_parse

```c
void *test1_parse(struct mln_parse_attr *pattr);
```

描述：根据参数`pattr`对指定自定义语言文本进行语法解析。其中，`pattr`的结构定义如下：

```c
struct mln_parse_attr {
    mln_alloc_t              *pool; //仅用于语法解析之用，语法解析后可直接进行销毁。
    mln_production_t         *prod_tbl; //产生式数组，与parser_generate保持一致
    mln_lex_t                *lex; //词法分析器指针
    void                     *pg_data; //由parser_generate函数创建的状态转换表
    void                     *udata; //用户自定义数据
};
```

返回值：返回自定义抽象语法树结构指针。

### 示例

本例演示如何创建一个简单语法的语言解析器。语法支持如下写法：

```
变量 + 变量;
变量 - 变量;
整数 + 整数;
整数 - 整数;
变量 + 整数;
变量 - 整数;
整数 + 变量;
整数 - 变量;
```

例如：

```
a + 1;
1 + 1;
_b + a;
```

代码如下：

```c
//test.c
#include <stdio.h>
#include <string.h>
#include "mln_log.h"
#include "mln_lex.h"
#include "mln_alloc.h"
#include "mln_parser_generator.h"

//声明语法解析器生成器，函数作用域为static，函数与结构体名称前缀为test，词素前缀为TEST。本例没有自定义词素。
MLN_DECLARE_PARSER_GENERATOR(static, test, TEST);
//定义语法解析器生成器，函数作用域为static，函数与结构体名称前缀为test，词素前缀为TEST。本例没有自定义词素。
MLN_DEFINE_PARSER_GENERATOR(static, test, TEST);

//产生式表，产生式的原理与代数非常相似，就是将同名的部分代入展开。start为所有语法的启始，xxx_TK_EOF表示语言读取完毕处
//本例中stm表示语句（statement），exp表示表达式（expression），addsub表示加减法表达式。
//TEST_TK_SEMIC为分号（;），TEST_TK_ID为变量名（以下划线字母起始，后续自符有字母数字下划线组成），TEST_TK_DEC为整数。
//这里使用了词法分析器的默认词素切分规则生成的词素，开发者可根据自己需求对关键字、特殊运算符进行扩展。
static mln_production_t prod_tbl[] = {
{"start: stm TEST_TK_EOF", NULL},
{"stm: exp TEST_TK_SEMIC stm", NULL},
{"stm: ", NULL},
{"exp: TEST_TK_ID addsub", NULL},
{"exp: TEST_TK_DEC addsub", NULL},
{"addsub: TEST_TK_PLUS exp", NULL},
{"addsub: TEST_TK_SUB exp", NULL},
{"addsub: ", NULL},
};

int main(int argc, char *argv[])
{
    mln_lex_t *lex = NULL;
    struct mln_lex_attr lattr;
    mln_alloc_t *pool;
    mln_string_t path;
    struct mln_parse_attr pattr;
    mln_u8ptr_t ptr, ast;
    mln_lex_hooks_t hooks;

    //设置自定义语言文本文件路径
    mln_string_set(&path, argv[1]);

    //创建内存池，用于语法分析过程中使用，使用后可进行释放。这里需要注意，生成的抽象语法树结构尽量不要使用该内存池，
    //开发者可能会习惯性按本示例一样在解析后进行释放。则在后续处理抽象语法树时就会发生越界访问。
    if ((pool = mln_alloc_init(NULL)) == NULL) {
        mln_log(error, "init memory pool failed.\n");
        return -1;
    }

    //设置词法分析器内存池
    lattr.pool = pool;
    //本例没有自定义关键字
    lattr.keywords = NULL;
    //本例没有对运算符进行扩展
    memset(&hooks, 0, sizeof(hooks));
    lattr.hooks = &hooks;
    //启用预编译机制，启用后，自定义语言中可使用#include、#def、#endif等预编译宏
    lattr.preprocess = 1;
    //设置为文件路径名类型。待解析内容可以直接给出字符串内容，也可以是文本路径，可参考词法分析器中的定义。
    lattr.type = M_INPUT_T_FILE;
    //若type为M_INPUT_T_FILE，则data为文件路径，否则为自定义语言字符串。
    lattr.data = &path;
    //设置词法分析器include时，查找文件位置的环境变量
    lattr.env = NULL;
    //初始化词法分析器
    mln_lex_init_with_hooks(test, lex, &lattr);
    if (lex == NULL) {
        mln_log(error, "init lexer failed.\n");
        return -1;
    }

    //生成状态转换表
    ptr = test_parser_generate(prod_tbl, sizeof(prod_tbl)/sizeof(mln_production_t), NULL);
    if (ptr == NULL) {
        mln_log(error, "generate state shift table failed.\n");
        return -1;
    }

    //设置语法解析器内存池
    pattr.pool = pool;
    //设置产生式
    pattr.prod_tbl = prod_tbl;
    //设置词法解析器，待解析的语言由词法分析器拆解后交由本解析器处理
    pattr.lex = lex;
    //设置状态转换表
    pattr.pg_data = ptr;
    //本例没有自定义数据
    pattr.udata = NULL;
    //执行解析
    ast = test_parse(&pattr);

    //销毁词法分析器
    mln_lex_destroy(lex);
    //释放内存池
    mln_alloc_destroy(pool);

    return 0;
}
```

编译生成可执行程序：

```shell
$ cc -o test test.c -I /path/to/melon/include -L /path/to/melon/lib -lmelon -lpthread
```

编辑一个符合新语法规范的文本`a.test`：

```
a + 1;
b + a;
c - b;
```

执行：

```shell
$ ./test a.test
```

如果正确则不会输出任何内容。

如果我们对`a.test`修改成如下内容：

```
a * 1;
b + a;
c - b;
```

执行后则可看到输出如下：

```
a.test:1: Parse Error: Illegal token nearby '*'.
```

因为我们的语言语法不支持乘法运算。
