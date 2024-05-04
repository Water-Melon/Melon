## 词法分析器

词法分析器可以以一定规则将文本中的词素进行提取，这是做编译器以及一些配置文件解析的必要工具。




### 头文件

```c
#include "mln_lex.h"
```



### 模块名

`lex`



### 相关结构

```c
typedef struct {
    mln_string_t              *text; //词素字符串
    mln_u32_t                  line; //词素所在文本中的行号
    enum PREFIX_NAME##_enum    type; //词素类型
    mln_string_t              *file; //词素所属文件名，如果不是从文件中读取的词素，该值为NULL
} PREFIX_NAME##_struct_t;

enum PREFIX_NAME##_enum { //词素类型枚举结构定义，对应于上一结构的type字段，后面的注释表明前面的类型与之对应。
    TK_PREFIX##_TK_EOF = 0,
    TK_PREFIX##_TK_OCT,
    TK_PREFIX##_TK_DEC,
    TK_PREFIX##_TK_HEX,
    TK_PREFIX##_TK_REAL,
    TK_PREFIX##_TK_ID,
    TK_PREFIX##_TK_SPACE,
    TK_PREFIX##_TK_EXCL    /*!*/,
    TK_PREFIX##_TK_DBLQ    /*"*/,
    TK_PREFIX##_TK_NUMS    /*#*/,
    TK_PREFIX##_TK_DOLL    /*$*/,
    TK_PREFIX##_TK_PERC    /*%*/,
    TK_PREFIX##_TK_AMP     /*&*/,
    TK_PREFIX##_TK_SGLQ    /*'*/,
    TK_PREFIX##_TK_LPAR    /*(*/,
    TK_PREFIX##_TK_RPAR    /*)*/,
    TK_PREFIX##_TK_AST     /***/,
    TK_PREFIX##_TK_PLUS    /*+*/,
    TK_PREFIX##_TK_COMMA   /*,*/,
    TK_PREFIX##_TK_SUB     /*-*/,
    TK_PREFIX##_TK_PERIOD  /*.*/,
    TK_PREFIX##_TK_SLASH   /*'/'*/,
    TK_PREFIX##_TK_COLON   /*:*/,
    TK_PREFIX##_TK_SEMIC   /*;*/,
    TK_PREFIX##_TK_LAGL    /*<*/,
    TK_PREFIX##_TK_EQUAL   /*=*/,
    TK_PREFIX##_TK_RAGL    /*>*/,
    TK_PREFIX##_TK_QUES    /*?*/,
    TK_PREFIX##_TK_AT      /*@*/,
    TK_PREFIX##_TK_LSQUAR  /*[*/,
    TK_PREFIX##_TK_BSLASH  /*\*/,
    TK_PREFIX##_TK_RSQUAR  /*]*/,
    TK_PREFIX##_TK_XOR     /*^*/,
    TK_PREFIX##_TK_UNDER   /*_*/,
    TK_PREFIX##_TK_FULSTP  /*`*/,
    TK_PREFIX##_TK_LBRACE  /*{*/,
    TK_PREFIX##_TK_VERTL   /*|*/,
    TK_PREFIX##_TK_RBRACE  /*}*/,
    TK_PREFIX##_TK_DASH    /*~*/,
    TK_PREFIX##_TK_KEYWORD_BEGIN,
    ## __VA_ARGS__
};
```

可以看到，这两个结构其实都是宏定义，尤其是类型部分，可以看到是允许使用者自己进行扩展的，扩展的部分我们后面将会说到。由于使用了TK_PREFIX进行拼接，因此结构和枚举定义的名称在代码文件中都是不完全的，需要根据使用来确定。



### 函数/宏



#### mln_lex_init

```c
mln_lex_t *mln_lex_init(struct mln_lex_attr *attr);

struct mln_lex_attr {
    mln_alloc_t        *pool;
    mln_string_t       *keywords;
    mln_lex_hooks_t    *hooks;
    mln_u32_t           preprocess:1;
    mln_u32_t           padding:31;
    mln_u32_t           type;
    mln_string_t       *data;
    mln_string_t       *env;
};

typedef struct {
    lex_hook    excl_handler;    /*!*/
    void        *excl_data;
    lex_hook    dblq_handler;    /*"*/
    void        *dblq_data;
    lex_hook    nums_handler;    /*#*/
    void        *nums_data;
    lex_hook    doll_handler;    /*$*/
    void        *doll_data;
    lex_hook    perc_handler;    /*%*/
    void        *perc_data;
    lex_hook    amp_handler;     /*&*/
    void        *amp_data;
    lex_hook    sglq_handler;    /*'*/
    void        *slgq_data;
    lex_hook    lpar_handler;    /*(*/
    void        *lpar_data;
    lex_hook    rpar_handler;    /*)*/
    void        *rpar_data;
    lex_hook    ast_handler;     /***/
    void        *ast_data;
    lex_hook    plus_handler;    /*+*/
    void        *plus_data;
    lex_hook    comma_handler;   /*,*/
    void        *comma_data;
    lex_hook    sub_handler;     /*-*/
    void        *sub_data;
    lex_hook    period_handler;  /*.*/
    void        *period_data;
    lex_hook    slash_handler;   /*'/'*/
    void        *slash_data;
    lex_hook    colon_handler;   /*:*/
    void        *colon_data;
    lex_hook    semic_handler;   /*;*/
    void        *semic_data;
    lex_hook    lagl_handler;    /*<*/
    void        *lagl_data;
    lex_hook    equal_handler;   /*=*/
    void        *equal_data;
    lex_hook    ragl_handler;    /*>*/
    void        *ragl_data;
    lex_hook    ques_handler;    /*?*/
    void        *ques_data;
    lex_hook    at_handler;      /*@*/
    void        *at_data;
    lex_hook    lsquar_handler;  /*[*/
    void        *lsquar_data;
    lex_hook    bslash_handler;  /*\*/
    void        *bslash_data;
    lex_hook    rsquar_handler;  /*]*/
    void        *rsquar_data;
    lex_hook    xor_handler;     /*^*/
    void        *xor_data;
    lex_hook    under_handler;   /*_*/
    void        *under_data;
    lex_hook    fulstp_handler;  /*`*/
    void        *fulstp_data;
    lex_hook    lbrace_handler;  /*{*/
    void        *lbrace_data;
    lex_hook    vertl_handler;   /*|*/
    void        *vertl_data;
    lex_hook    rbrace_handler;  /*}*/
    void        *rbrace_data;
    lex_hook    dash_handler;    /*~*/
    void        *dash_data;
} mln_lex_hooks_t;

typedef void *(*lex_hook)(mln_lex_t *, void *);
```

描述：创建并初始化词法分析器，参数`attr`的成员如下：

- `pool` 为给词法分析器使用的内存池，该参数必须非空。
- `keywords` 为关键词字符串数组，数组的最后一个元素的`data`成员为`NULL`。
- `hooks`为各个特殊字符的回调函数数组，用于自定义特殊符号处理。每一个处理函数可以搭配一个用户自定义数据。回调函数第一个参数为词法分析器指针，第二个参数为对应回调函数的用户自定义数据。后面示例中将会给出。
- `preprocess`是否启用预编译功能，该功能包含了引入其他文件、宏定义、宏判断等功能。
- `padding`无用填充
- `type`用于指示`data`是文件路径还是代码字符串：`M_INPUT_T_BUF`为代码字符串，`M_INPUT_T_FILE`为代码文件路径。
- `data`代码文件路径或代码字符串，取决于`type`的值。
- `env`是用于从环境变量设置的目录中找到指定相对路径的文件。

返回值：成功则返回`mln_lex_t`结构指针，否则返回`NULL`



#### mln_lex_destroy

```c
void mln_lex_destroy(mln_lex_t *lex);
```

描述：销毁并释放词法分析器资源。

返回值：无



#### mln_lex_strerror

```c
char *mln_lex_strerror(mln_lex_t *lex);
```

描述：获取当前词法分析器遇到的错误字符串。

返回值：错误字符串



#### mln_lex_push_input_file_stream

```c
int mln_lex_push_input_file_stream(mln_lex_t *lex, mln_string_t *path);
```

描述：该函数用于将代码文件路径`path`压入词法分析器的输入流的最前面。本函数是用于实现词法分析器引入其他文件代码的。

返回值：成功则返回`0`，否则返回`-1`



#### mln_lex_push_input_buf_stream

```c
int mln_lex_push_input_buf_stream(mln_lex_t *lex, mln_string_t *buf);
```

描述：该函数用于将代码字符串`buf`压入词法分析器的输入流的最前面。本函数是用于实现词法分析器引入其他文件代码的。

返回值：成功则返回`0`，否则返回`-1`



#### mln_lex_check_file_loop

```c
int mln_lex_check_file_loop(mln_lex_t *lex, mln_string_t *path);
```

描述：用于检查`mln_lex_push_input_file_stream`引入的文件是否存在循环引用的情况。

返回值：无则返回`0`，否则返回`-1`



#### mln_lex_macro_new

```c
mln_lex_macro_t *mln_lex_macro_new(mln_alloc_t *pool, mln_string_t *key, mln_string_t *val);
```

描述：创建宏，`pool`为用于创建宏结构的内存池结构，一般而言，该结构就是`mln_lex_t`中的`pool`。`key`为宏名称，`val`为宏指代的内容。

返回值：成功则返回宏结构指针，否则返回`NULL`



#### mln_lex_macro_free

```c
void mln_lex_macro_free(void *data);
```

描述：释放宏结构`data`，`data`的类型必须为`mln_lex_macro_t`。

返回值：无



#### mln_lex_stepback

```c
void mln_lex_stepback(mln_lex_t *lex, char c);
```

描述：将字符`c`重新压入词法分析器`lex`输入流的最前面，以便下一次读取。

返回值：无



#### mln_lex_putchar

```c
int mln_lex_putchar(mln_lex_t *lex, char c);
```

描述：将字符`c`追加到到词法分析器输出流的末尾，即最终词素的字符串中的字符。

返回值：成功则返回`0`，否则返回`-1`



#### mln_lex_getchar

```c
char mln_lex_getchar(mln_lex_t *lex);
```

描述：从词法分析器`lex`的输入流中读取一个字符。

返回值：字符



#### mln_lex_is_letter

```c
int mln_lex_is_letter(char c);
```

描述：判断字符是否是下划线或字母。

返回值：是则返回`1`，否则返回`0`



#### mln_lex_is_oct

```c
int mln_lex_is_oct(char c);
```

描述：判断字符`c`是否是八进制数。

返回值：是则返回`1`，否则返回`0`



#### mln_lex_is_hex

```c
int mln_lex_is_hex(char c);
```

描述：判断字符`c`是否是十六进制数。

返回值：是则返回`1`，否则返回`0`



#### PREFIX_NAME##_new

```c
PREFIX_NAME##_struct_t *PREFIX_NAME##_new(mln_lex_t *lex, enum PREFIX_NAME##_enum type);
```

描述：创建新词素，其中`type`的类型参见*相关类型*小节，创建新词素会清空词法分析器`lex`的输出流。

返回值：成功则返回词素结构指针，否则返回`NULL`



#### PREFIX_NAME##_free

```c
void PREFIX_NAME##_free(PREFIX_NAME##_struct_t *ptr);
```

描述：释放词素结构内存。

返回值：无



#### PREFIX_NAME##_token

```c
PREFIX_NAME##_struct_t *PREFIX_NAME##_token(mln_lex_t *lex);
```

描述：从词法分析器`lex`中读取每一个词素。

返回值：成功则返回词素结构指针，否则返回`NULL`



#### MLN_DEFINE_TOKEN_TYPE_AND_STRUCT

```c
MLN_DEFINE_TOKEN_TYPE_AND_STRUCT(SCOPE,PREFIX_NAME,TK_PREFIX,...);
```

描述：该宏用于定义词素、词素类型、函数声明等内容，其中：

- `SCOPE` 为函数声明的范围关键字，例如：static、extern等
- `PREFIX_NAME`为词素结构、类型结构、函数的前缀。
- `TK_PREFIX`为词素类型的前缀（枚举中的值的前缀）。

返回值：无



#### MLN_DEFINE_TOKEN

```c
MLN_DEFINE_TOKEN(SCOPE, PREFIX_NAME,TK_PREFIX,...);
```

描述：该宏用于定义处理函数、特殊符号默认处理函数、词素类型与词素类型字符串数组等内容。其中：

- `SCOPE`为本宏定义的一些变量和函数的作用域。
- `PREFIX_NAME`为词素结构、类型结构、函数的前缀。
- `TK_PREFIX`为词素类型的前缀（枚举中的值的前缀）。

返回值：



#### mln_lex_init_with_hooks

```c
mln_lex_init_with_hooks(PREFIX_NAME,lex_ptr,attr_ptr)
```

描述：该宏为对`mln_lex_init`函数的封装，避免了手工编写代码完成自定义预处理、回调函数等内容的处理过程。

返回值：本身无返回值，但需要在使用后判断`lex_ptr`是否为`NULL`，`NULL`表示失败，否则成功。



#### mln_lex_snapshot_record

```c
mln_lex_off_t mln_lex_snapshot_record(mln_lex_t *lex);
```

描述：记录当前输入流的位置信息。

返回值：文件内偏移或内存地址



#### mln_lex_snapshot_apply

```c
void mln_lex_snapshot_apply(mln_lex_t *lex, mln_lex_off_t off);
```

描述：恢复当前输入流的读取偏移到快照的位置。注意，这个函数应用时应确保当前输入流是`mln_lex_snapshot_record`调用时的输入流，函数中会进行一定的检查，但并不能确保万无一失。

返回值：无



### 示例

```c
#include <stdio.h>
#include "mln_lex.h"

mln_string_t keywords[] = {
    mln_string("on"),
    mln_string("off"),
    mln_string(NULL)
};

MLN_DEFINE_TOKEN_TYPE_AND_STRUCT(static, mln_test, TEST, TEST_TK_ON, TEST_TK_OFF, TEST_TK_STRING);
MLN_DEFINE_TOKEN(static, mln_test, TEST, {TEST_TK_ON, "TEST_TK_ON"}, {TEST_TK_OFF, "TEST_TK_OFF"}, {TEST_TK_STRING, "TEST_TK_STRING"});

static inline int
mln_get_char(mln_lex_t *lex, char c)
{
    if (c == '\\') {
        char n;
        if ((n = mln_lex_getchar(lex)) == MLN_ERR) return -1;
        switch ( n ) {
            case '\"':
                if (mln_lex_putchar(lex, n) == MLN_ERR) return -1;
                break;
            case '\'':
                if (mln_lex_putchar(lex, n) == MLN_ERR) return -1;
                break;
            case 'n':
                if (mln_lex_putchar(lex, '\n') == MLN_ERR) return -1;
                break;
            case 't':
                if (mln_lex_putchar(lex, '\t') == MLN_ERR) return -1;
                break;
            case 'b':
                if (mln_lex_putchar(lex, '\b') == MLN_ERR) return -1;
                break;
            case 'a':
                if (mln_lex_putchar(lex, '\a') == MLN_ERR) return -1;
                break;
            case 'f':
                if (mln_lex_putchar(lex, '\f') == MLN_ERR) return -1;
                break;
            case 'r':
                if (mln_lex_putchar(lex, '\r') == MLN_ERR) return -1;
                break;
            case 'v':
                if (mln_lex_putchar(lex, '\v') == MLN_ERR) return -1;
                break;
            case '\\':
                if (mln_lex_putchar(lex, '\\') == MLN_ERR) return -1;
                break;
            default:
                mln_lex_error_set(lex, MLN_LEX_EINVCHAR);
                return -1;
        }
    } else {
        if (mln_lex_putchar(lex, c) == MLN_ERR) return -1;
    }
    return 0;
}

static mln_test_struct_t *
mln_test_dblq_handler(mln_lex_t *lex, void *data)
{
    mln_lex_result_clean(lex);
    char c;
    while ( 1 ) {
        c = mln_lex_getchar(lex);
        if (c == MLN_ERR) return NULL;
        if (c == MLN_EOF) {
            mln_lex_error_set(lex, MLN_LEX_EINVEOF);
            return NULL;
        }
        if (c == '\"') break;
        if (mln_get_char(lex, c) < 0) return NULL;
    }
    return mln_test_new(lex, TEST_TK_STRING);
}

int main(int argc, char *argv[])
{
    if (argc != 2) {
        fprintf(stderr, "Usage: %s file_path\n", argv[0]);
        return -1;
    }

    mln_string_t path;
    mln_lex_t *lex = NULL;
    struct mln_lex_attr lattr;
    mln_test_struct_t *ts;
    mln_lex_hooks_t hooks;

    memset(&hooks, 0, sizeof(hooks));
    hooks.dblq_handler = (lex_hook)mln_test_dblq_handler;

    mln_string_nset(&path, argv[1], strlen(argv[1]));

    lattr.pool = mln_alloc_init(NULL);
    if (lattr.pool == NULL) {
        fprintf(stderr, "init pool failed\n");
        return -1;
    }
    lattr.keywords = keywords;
    lattr.hooks = &hooks;
    lattr.preprocess = 1;//支持预处理
    lattr.padding = 0;
    lattr.type = M_INPUT_T_FILE;
    lattr.data = &path;
    lattr.env = NULL;

    mln_lex_init_with_hooks(mln_test, lex, &lattr);
    if (lex == NULL) {
        fprintf(stderr, "lexer init failed\n");
        return -1;
    }

    while (1) {
        ts = mln_test_token(lex);
        if (ts == NULL || ts->type == TEST_TK_EOF)
            break;
        write(STDOUT_FILENO, ts->text->data, ts->text->len);
        printf(" line:%u type:%d\n", ts->line, ts->type);
    }

    return 0;
}
```

使用本代码生成可执行程序，然后对如下文本进行解析：

```ini
//a.txt
#include "b.txt" //注意，这里必须是双引号"
test_mode = on
log_level = 'debug'
proc_num = 10
```

```ini
//b.txt
conf_name = "b.txt"
```

得到的输出效果如下：

```
conf_name line:1 type:5
= line:1 type:25
b.txt line:1 type:42
test_mode line:2 type:5
= line:2 type:25
on line:2 type:40
log_level line:3 type:5
= line:3 type:25
' line:3 type:13
debug line:3 type:5
' line:3 type:13
proc_num line:4 type:5
= line:4 type:25
10 line:4 type:2
```

