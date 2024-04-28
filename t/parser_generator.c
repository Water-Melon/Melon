#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "mln_log.h"
#include "mln_lex.h"
#include "mln_alloc.h"
#include "mln_parser_generator.h"

MLN_DECLARE_PARSER_GENERATOR(static, test, TEST);
MLN_DEFINE_PARSER_GENERATOR(static, test, TEST);

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

int main(void)
{
    mln_lex_t *lex = NULL;
    struct mln_lex_attr lattr;
    mln_alloc_t *pool;
    struct mln_parse_attr pattr;
    mln_u8ptr_t ptr, ast;
    mln_lex_hooks_t hooks;
    mln_string_t code = mln_string("a + 1;\nb + a;\nc - b;\n");

    assert((pool = mln_alloc_init(NULL)) != NULL);

    lattr.pool = pool;
    lattr.keywords = NULL;
    memset(&hooks, 0, sizeof(hooks));
    lattr.hooks = &hooks;
    lattr.preprocess = 1;
    lattr.type = M_INPUT_T_BUF;
    lattr.data = &code;
    lattr.env = NULL;
    mln_lex_init_with_hooks(test, lex, &lattr);
    assert(lex != NULL);

    assert((ptr = test_parser_generate(prod_tbl, sizeof(prod_tbl)/sizeof(mln_production_t), NULL)) != NULL);

    pattr.pool = pool;
    pattr.prod_tbl = prod_tbl;
    pattr.lex = lex;
    pattr.pg_data = ptr;
    pattr.udata = NULL;
    assert((ast = test_parse(&pattr)) == NULL); /* because all callbacks in prod_tbl are `NULL` */

    mln_lex_destroy(lex);
    test_pg_data_free(ptr);
    mln_alloc_destroy(pool);

    return 0;
}

