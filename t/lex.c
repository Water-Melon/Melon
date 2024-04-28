#include <stdio.h>
#include "mln_lex.h"
#include <assert.h>
#include "mln_log.h"

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

int main(void)
{
    mln_string_t ini = mln_string("a = b; c = d");
    mln_lex_t *lex = NULL;
    struct mln_lex_attr lattr;
    mln_test_struct_t *ts;
    mln_lex_hooks_t hooks;

    memset(&hooks, 0, sizeof(hooks));
    hooks.dblq_handler = (lex_hook)mln_test_dblq_handler;

    if ((lattr.pool = mln_alloc_init(NULL)) != NULL);
    lattr.keywords = keywords;
    lattr.hooks = &hooks;
    lattr.preprocess = 1;
    lattr.padding = 0;
    lattr.type = M_INPUT_T_BUF;
    lattr.data = &ini;
    lattr.env = NULL;

    mln_lex_init_with_hooks(mln_test, lex, &lattr);
    assert(lex != NULL);

    while (1) {
        ts = mln_test_token(lex);
        if (ts == NULL || ts->type == TEST_TK_EOF) {
            mln_test_free(ts);
            break;
        }
        mln_log(debug, "line:%u text:[%S] type:%d\n", ts->line, ts->text, ts->type);
        mln_test_free(ts);
    }

    mln_lex_destroy(lex);
    mln_alloc_destroy(lattr.pool);

    return 0;
}
