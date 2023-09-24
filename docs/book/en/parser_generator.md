## Parser Generator

Using the parser generator, we can generate a parser that conforms to the custom rules of the LALR(1) syntax, and use the parser to parse the text, and also generate a custom abstract syntax tree structure.

A simple usage example will be given later. For a more complete application, please refer to the implementation of melang in `mln_lang_ast.c/h`. Or join the official group for consultation.



### Header file

```c
#include "mln_parser_generator.h"
```



### Module

`parser_generator`



### Functions/Macros



#### MLN_DECLARE_PARSER_GENERATOR

```c
#define MLN_DECLARE_PARSER_GENERATOR(SCOPE,PREFIX_NAME,TK_PREFIX,...);
```

Description: Used to declare functions, structures, etc. related to the parser generator. in:

- `SCOPE` is a declared scope keyword like `static`.
- `PREFIX_NAME` is the prefix for the function and structure naming in the declaration, and the completed name consists of a custom prefix + a fixed suffix.
- `TK_PREFIX` is the token prefix used in production. The complete token name is composed of token prefix + fixed suffix (for existing token, please refer to the definition in `mln_lex.h`).
- `...`, the variable parameter part is the token name of the custom keyword or operator. Note that the order of token names is very important. For details, refer to the macro content in `mln_lex.h`.

Return value: none



#### MLN_DEFINE_PASER_GENERATOR

```c
#define MLN_DEFINE_PARSER_GENERATOR(SCOPE,PREFIX_NAME,TK_PREFIX,...);
```

Description: Used to define functions and structures related to the parser generator. in:

- `SCOPE` is a declared scope keyword like `static`.
- `PREFIX_NAME` is the prefix for the function and structure naming in the definition, and the completed name consists of a custom prefix + a fixed suffix.
- `TK_PREFIX` is the token prefix used in production. The complete token name is composed of token prefix + fixed suffix (for existing tokens, please refer to the definition in `mln_lex.h`).
- `...`, the variable parameter part is the token name of the custom keyword or operator and its name string. Note that the order of token names is very important. For details, refer to the macro content in `mln_lex.h`.

Return value: none



#### xxxx_parser_generate

```c
void *PREFIX_NAME##_parser_generate(mln_production_t *prod_tbl, mln_u32_t nr_prod);
```

Description: Generate the LALR(1) state transition table according to the productions specified by `prod_tbl` and `nr_prod`. The state transition table will be used in the parse function to check the custom language syntax. For the production structure, please refer to the following simple example.

Return value: Return the state transition table structure, if it is `NULL`, it means an error.



#### xxxx_parse

```c
void *test1_parse(struct mln_parse_attr *pattr);
```

Description: Parse the specified custom language text according to the parameter `pattr`. Among them, the structure of `pattr` is defined as follows:

```c
struct mln_parse_attr {
    mln_alloc_t              *pool; //It is only used for parsing, and can be destroyed directly after parsing.
    mln_production_t         *prod_tbl; //Array of productions, consistent with parser_generate
    mln_lex_t                *lex; //lexer pointer
    void                     *pg_data; //State transition table created by the parser_generate function
    void                     *udata; //user-defined data
};
```

Return value: Returns a pointer to a custom abstract syntax tree structure.



### Example

This example demonstrates how to create a language parser for a simple syntax. The syntax supports the following notation:

```
variable + variable;
variable - variable;
integer + integer;
integer - integer;
variable + integer;
variable - integer;
integer + variable;
integer - variable;
```

for example：

```
a + 1;
1 + 1;
_b + a;
```

Code：

```c
//test.c
#include <stdio.h>
#include <string.h>
#include "mln_log.h"
#include "mln_lex.h"
#include "mln_alloc.h"
#include "mln_parser_generator.h"

//Declare a parser generator, function scope is static, function and structure names are prefixed with test, and token are prefixed with TEST. There are no custom token in this example.
MLN_DECLARE_PARSER_GENERATOR(static, test, TEST);
//Define a parser generator, the function scope is static, the function and structure name prefix is test, and the lexeme prefix is TEST. There are no custom token in this example.
MLN_DEFINE_PARSER_GENERATOR(static, test, TEST);

//Production table, the principle of production is very similar to algebra, that is, the part of the same name is substituted into the expansion. start is the start of all syntaxs, xxx_TK_EOF indicates the end of the language reading
//In this example, stm represents a statement, exp represents an expression, and addsub represents an addition and subtraction expression.
//TEST_TK_SEMIC is a semicolon (;), TEST_TK_ID is a variable name (starting with an underscore letter, followed by an alphanumeric underscore), and TEST_TK_DEC is an integer.
//The morphemes generated by the default morpheme segmentation rules of the lexical analyzer are used here, and developers can expand keywords and special operators according to their own needs.
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

    //Set custom language text file path
    mln_string_set(&path, argv[1]);

    //Create a memory pool for use during parsing and release after use. It should be noted here that the generated abstract syntax tree structure should not use this memory pool as much as possible.
    //Developers may habitually release after parsing as in this example. Then an out-of-bounds access will occur in subsequent processing of the abstract syntax tree.
    if ((pool = mln_alloc_init(NULL)) == NULL) {
        mln_log(error, "init memory pool failed.\n");
        return -1;
    }

    //Set the lexer memory pool
    lattr.pool = pool;
    //There are no custom keywords in this example
    lattr.keywords = NULL;
    //This example does not expand the operator
    memset(&hooks, 0, sizeof(hooks));
    lattr.hooks = &hooks;
    //Enable the pre-compilation mechanism. After enabling, pre-compiled macros such as #include, #def, #endif can be used in custom languages
    lattr.preprocess = 1;
    //Set to file pathname type. The content to be parsed can be directly given the string content, or it can be a text path, please refer to the definition in the lexical analyzer.
    lattr.type = M_INPUT_T_FILE;
    //If type is M_INPUT_T_FILE, data is the file path, otherwise it is a custom language string.
    lattr.data = &path;
    //Set the environment variable to find the location of the included file
    lattr.env = NULL;
    //Initialize the lexer
    mln_lex_init_with_hooks(test, lex, &lattr);
    if (lex == NULL) {
        mln_log(error, "init lexer failed.\n");
        return -1;
    }

    //Generate state transition table
    ptr = test_parser_generate(prod_tbl, sizeof(prod_tbl)/sizeof(mln_production_t), NULL);
    if (ptr == NULL) {
        mln_log(error, "generate state shift table failed.\n");
        return -1;
    }

    //Set the parser memory pool
    pattr.pool = pool;
    //set production
    pattr.prod_tbl = prod_tbl;
    //Set the lexical analyzer, the language to be parsed is disassembled by the lexical analyzer and then handed over to the parser for processing
    pattr.lex = lex;
    //Set state transition table
    pattr.pg_data = ptr;
    //No custom data in this example
    pattr.udata = NULL;
    //parsing
    ast = test_parse(&pattr);

    //destroy the lexer
    mln_lex_destroy(lex);
    //free memory pool
    mln_alloc_destroy(pool);

    return 0;
}
```

Compile to generate executable program:

```shell
$ cc -o test test.c -I /path/to/melon/include -L /path/to/melon/lib -lmelon -lpthread
```

Edit a text `a.test` that conforms to the new syntax specification:

```
a + 1;
b + a;
c - b;
```

Execute：

```shell
$ ./test a.test
```

If it is correct nothing will be output.

If we modify `a.test` to the following:

```
a * 1;
b + a;
c - b;
```

After execution, you can see the output as follows:

```
a.test:1: Parse Error: Illegal token nearby '*'.
```

Because our language syntax doesn't support multiplication.
