## Lexer

The lexical analyzer can extract the morphemes in the text according to certain rules, which is a necessary tool for the compiler and some configuration file parsing.



### Header file

```c
#include "mln_lex.h"
```



### Module

`lex`



### Structures

```c
typedef struct {
    mln_string_t              *text; //token string
    mln_u32_t                  line; //lines of token in file
    enum PREFIX_NAME##_enum    type; //token type
    mln_string_t              *file; //filepath or NULL (if text is not comming from file)
} PREFIX_NAME##_struct_t;

enum PREFIX_NAME##_enum { //The token type enumeration definition corresponds to the type field of the previous structure, and the following comment indicates that the previous type corresponds to it.
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

It can be seen that these two structures are actually macro definitions, especially the type part. It can be seen that users are allowed to expand by themselves. We will talk about the expansion part later. Since TK_PREFIX is used for splicing, the names of structure and enumeration definitions are incomplete in the code file and need to be determined according to usage.



### Functions/Macros



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

Description: Create and initialize the lexical analyzer. The members of the parameter `attr` are as follows:

- `pool` is the memory pool used by the lexer, this parameter must be non-null.
- `keywords` is an array of keyword strings, and the `data` member of the last element of the array is `NULL`.
- `hooks` is an array of callback functions for each special character, which is used to customize special character processing. Each handler function can be matched with a user-defined data. The first parameter of the callback function is the lexer pointer, and the second parameter is the user-defined data corresponding to the callback function. will be given in the following example.
- Whether `preprocess` enables the precompilation function, which includes the introduction of other files, macro definitions, macro judgment and other functions.
- `padding` useless padding
- `type` is used to indicate whether `data` is a file path or a code string: `M_INPUT_T_BUF` is a code string, `M_INPUT_T_FILE` is a code file path.
- `data` code file path or code string, depending on the value of `type`.
- `env` is used to find the file with the specified relative path from the directory set by the environment variable.

Return value: return `mln_lex_t` structure pointer if successful, otherwise return `NULL`



#### mln_lex_destroy

```c
void mln_lex_destroy(mln_lex_t *lex);
```

Description: Destroy and release lexer resources.

Return value: none



#### mln_lex_strerror

```c
char *mln_lex_strerror(mln_lex_t *lex);
```

Description: Get the error string encountered by the current lexer.

Return value: error string



#### mln_lex_push_input_file_stream

```c
int mln_lex_push_input_file_stream(mln_lex_t *lex, mln_string_t *path);
```

Description: This function is used to push the code file path `path` to the front of the input stream of the lexer. This function is used to implement the lexical analyzer to introduce other file code.

Return value: return `0` if successful, otherwise return `-1`



#### mln_lex_push_input_buf_stream

```c
int mln_lex_push_input_buf_stream(mln_lex_t *lex, mln_string_t *buf);
```

Description: This function is used to push the code string `buf` to the front of the input stream of the lexer. This function is used to implement the lexical analyzer to introduce other file code.

Return value: return `0` if successful, otherwise return `-1`



#### mln_lex_check_file_loop

```c
int mln_lex_check_file_loop(mln_lex_t *lex, mln_string_t *path);
```

Description: Used to check for circular references in files imported by `mln_lex_push_input_file_stream`.

Return value: `0` if none, otherwise `-1`



#### mln_lex_macro_new

```c
mln_lex_macro_t *mln_lex_macro_new(mln_alloc_t *pool, mln_string_t *key, mln_string_t *val);
```

Description: Create a macro. `pool` is the memory pool structure used to create the macro structure. Generally speaking, this structure is the `pool` in `mln_lex_t`. `key` is the macro name, and `val` is the content that the macro refers to.

Return value: return the macro structure pointer if successful, otherwise return `NULL`



#### mln_lex_macro_free

```c
void mln_lex_macro_free(void *data);
```

Description: Free macro structure `data`, `data` must be of type `mln_lex_macro_t`.

Return value: none



#### mln_lex_stepback

```c
void mln_lex_stepback(mln_lex_t *lex, char c);
```

Description: Re-push the character `c` to the front of the lexer `lex` input stream for the next read.

Return value: none



#### mln_lex_putchar

```c
int mln_lex_putchar(mln_lex_t *lex, char c);
```

Description: Appends the character `c` to the end of the lexer output stream, i.e. the characters in the string of the final token.

Return value: return `0` if successful, otherwise return `-1`



#### mln_lex_getchar

```c
char mln_lex_getchar(mln_lex_t *lex);
```

Description: Read a character from the input stream of the lexer `lex`.

Return value: character



#### mln_lex_is_letter

```c
int mln_lex_is_letter(char c);
```

Description: Determines whether the character is an underscore or a letter.

Return value: `1` if yes, `0` otherwise



#### mln_lex_is_oct

```c
int mln_lex_is_oct(char c);
```

Description: Check if the character `c` is an octal number.

Return value: `1` if yes, `0` otherwise



#### mln_lex_is_hex

```c
int mln_lex_is_hex(char c);
```

Description: Determine if the character `c` is a hexadecimal number.

Return value: `1` if yes, `0` otherwise



#### PREFIX_NAME##_new

```c
PREFIX_NAME##_struct_t *PREFIX_NAME##_new(mln_lex_t *lex, enum PREFIX_NAME##_enum type);
```

Description: Create a new lexeme, where the type of `type` is described in the *related types* section, creating a new lexeme will empty the output stream of the lexer `lex`.

Return value: If successful, return the token structure pointer, otherwise return `NULL`



#### PREFIX_NAME##_free

```c
void PREFIX_NAME##_free(PREFIX_NAME##_struct_t *ptr);
```

Description: Free the token structure memory.

Return value: none



#### PREFIX_NAME##_token

```c
PREFIX_NAME##_struct_t *PREFIX_NAME##_token(mln_lex_t *lex);
```

Description: Read each token from the lexer `lex`.

Return value: If successful, return the token structure pointer, otherwise return `NULL`



#### MLN_DEFINE_TOKEN_TYPE_AND_STRUCT

```c
MLN_DEFINE_TOKEN_TYPE_AND_STRUCT(SCOPE,PREFIX_NAME,TK_PREFIX,...);
```

Description: This macro is used to define token, token types, function declarations, etc., where:

- `SCOPE` is the scope keyword of function declaration, for example: static, extern, etc.
- `PREFIX_NAME` is the prefix of the token structure, type structure, and function.
- `TK_PREFIX` is the prefix of the token type (the prefix of the values in the enumeration).

Return value: none



#### MLN_DEFINE_TOKEN

```c
MLN_DEFINE_TOKEN(SCOPE, PREFIX_NAME,TK_PREFIX,...);
```

Description: This macro is used to define processing functions, default processing functions for special symbols, token types and token type string arrays, etc. in:

- `SCOPE` is the scope keyword of function declaration, for example: static.
- `PREFIX_NAME` is the prefix of the token structure, type structure, and function.
- `TK_PREFIX` is the prefix of the token type (the prefix of the values in the enumeration).

return value:



#### mln_lex_init_with_hooks

```c
mln_lex_init_with_hooks(PREFIX_NAME,lex_ptr,attr_ptr)
```

Description: This macro encapsulates the `mln_lex_init` function, which avoids the process of manually writing code to complete custom preprocessing, callback functions, etc.

Return value: There is no return value, but it should be checked whether `lex_ptr` is `NULL` after called. `NULL` on failure, otherwise on success.



#### mln_lex_snapshot_record

```c
mln_lex_off_t mln_lex_snapshot_record(mln_lex_t *lex);
```

Description: Records the position information of the current input stream.

Return value: File offset or memory address



#### mln_lex_snapshot_apply

```c
void mln_lex_snapshot_apply(mln_lex_t *lex, mln_lex_off_t off);
```

Description: Restores the read offset of the current input stream to the position of the snapshot. Note that this function should ensure that the current input stream is the one when `mln_lex_snapshot_record` was called. The function performs some checks, but does not guarantee complete safety.

Return value: None.



### Example

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
    lattr.preprocess = 1;//Preprocessing is supported
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

Use this code to generate an executable program, and then parse the following text:

```ini
//a.txt
#include "b.txt" // Note: here must use " not '
test_mode = on
log_level = 'debug'
proc_num = 10
```

```ini
//b.txt
conf_name = "b.txt"
```

The output is as follows:

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

