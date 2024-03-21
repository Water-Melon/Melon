
/*
 * Copyright (C) Niklaus F.Schen.
 */
 
#ifndef __MLN_LEX_H
#define __MLN_LEX_H

#include <stdio.h>
#include <string.h>
#include "mln_stack.h"
#include "mln_string.h"
#include "mln_alloc.h"
#include "mln_rbtree.h"
#include "mln_func.h"
#include "mln_utils.h"
#include <stdlib.h>
#include <errno.h>

#define MLN_EOF (char)(-1)
#define MLN_ERR (char)(-2)
#define MLN_LEX_ERRMSG_LEN 1024
#define MLN_DEFAULT_BUFLEN 4095
#define MLN_EOF_TEXT "##EOF##"

#define M_INPUT_T_BUF  0
#define M_INPUT_T_FILE 1

/*
 * error number
 */
#define MLN_LEX_SUCCEED 0
#define MLN_LEX_ENMEM 1
#define MLN_LEX_EINVCHAR 2
#define MLN_LEX_EINVHEX 3
#define MLN_LEX_EINVDEC 4
#define MLN_LEX_EINVOCT 5
#define MLN_LEX_EINVREAL 6
#define MLN_LEX_EREAD 7
#define MLN_LEX_EINVEOF 8
#define MLN_LEX_EINVEOL 9
#define MLN_LEX_EINPUTTYPE 10
#define MLN_LEX_EFPATH 11
#define MLN_LEX_EINCLUDELOOP 12
#define MLN_LEX_EUNKNOWNMACRO 13
#define MLN_LEX_EMANYMACRODEF 14
#define MLN_LEX_EINVMACRO 15

typedef struct mln_lex_s mln_lex_t;
/*
 * Actually lex_hooks should be defined as 
 * typedef PREFIX_NAME##_struct_t *(*lex_hook)(mln_lex_t *, void *);
 * But if this structure defined as a macro,
 * the structure mln_lex_attr and mln_lex_t will
 * also be defined as a macro, that is a lot work to do.
 * So we define the function type to be void *,
 * and we define the hook-functions type to be a specified type.
 */
typedef void *(*lex_hook)(mln_lex_t *, void *);

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

struct mln_lex_attr {
    mln_alloc_t        *pool;
    mln_string_t       *keywords;
    mln_lex_hooks_t    *hooks;
    mln_u32_t           preprocess:1;
    mln_u32_t           padding:31;
    mln_u32_t           type;
    mln_string_t       *env;
    mln_string_t       *data;
};

typedef union {
    mln_u8ptr_t         soff;
    off_t               foff;
} mln_lex_off_t;

typedef struct {
    mln_u32_t           type;
    int                 fd;
    mln_string_t       *data;
    mln_string_t       *dir;
    mln_u8ptr_t         buf;
    mln_u8ptr_t         pos;
    mln_u64_t           buf_len;
    mln_u64_t           line;
} mln_lex_input_t;

typedef struct {
    mln_string_t       *keyword;
    mln_uauto_t         val;
} mln_lex_keyword_t;

typedef struct {
    mln_u64_t           if_level;
    mln_u64_t           if_matched;
} mln_lex_preprocess_data_t;

struct mln_lex_s {
    mln_alloc_t               *pool;
    mln_rbtree_t              *macros;
    mln_lex_input_t           *cur;
    mln_stack_t               *stack;
    mln_lex_hooks_t            hooks;
    mln_rbtree_t              *keywords;
    mln_s8ptr_t                err_msg;
    mln_u8ptr_t                result_buf;
    mln_u8ptr_t                result_pos;
    mln_u64_t                  result_buf_len;
    mln_u64_t                  line;
    mln_s32_t                  error;
    mln_u32_t                  preprocess:1;
    mln_u32_t                  ignore:1;
    mln_string_t              *env;
    mln_lex_preprocess_data_t *preprocess_data;
};

typedef struct {
    char                sc;
    lex_hook            handler;
    void               *data;
} mln_spechar_t;

typedef struct {
    mln_string_t        command;
    lex_hook            handler;
} mln_preprocess_handler_t;

typedef struct {
    mln_string_t       *key;
    mln_string_t       *val;
} mln_lex_macro_t;

extern mln_lex_t *mln_lex_init(struct mln_lex_attr *attr) __NONNULL1(1);
extern void mln_lex_destroy(mln_lex_t *lex);
extern char *mln_lex_strerror(mln_lex_t *lex) __NONNULL1(1);
extern int mln_lex_push_input_file_stream(mln_lex_t *lex, mln_string_t *path) __NONNULL2(1,2);
extern int mln_lex_push_input_buf_stream(mln_lex_t *lex, mln_string_t *buf) __NONNULL2(1,2);
extern int mln_lex_check_file_loop(mln_lex_t *lex, mln_string_t *path) __NONNULL2(1,2);
extern mln_lex_macro_t *
mln_lex_macro_new(mln_alloc_t *pool, mln_string_t *key, mln_string_t *val) __NONNULL2(1,2);
extern void mln_lex_macro_free(void *data);
extern mln_lex_preprocess_data_t *mln_lex_preprocess_data_new(mln_alloc_t *pool) __NONNULL1(1);
extern void mln_lex_preprocess_data_free(mln_lex_preprocess_data_t *lpd);
extern int mln_lex_condition_test(mln_lex_t *lex) __NONNULL1(1);
extern mln_lex_input_t *
mln_lex_input_new(mln_lex_t *lex, mln_u32_t type, mln_string_t *data, int *err, mln_u64_t line) __NONNULL3(1,3,4);
extern void mln_lex_input_free(void *in);
extern mln_lex_off_t mln_lex_snapshot_record(mln_lex_t *lex);
extern void mln_lex_snapshot_apply(mln_lex_t *lex, mln_lex_off_t off);
#define mln_lex_get_pool(lex) ((lex)->pool)
#define mln_lex_result_clean(lex) ((lex)->result_pos = (lex)->result_buf)
#define mln_lex_get_cur_filename(lex) \
    ((lex)->cur==NULL? NULL: ((lex)->cur->type==M_INPUT_T_BUF?NULL: ((lex)->cur->data)))
#define mln_lex_result_get(lex,res_pstr) mln_string_nset((res_pstr), lex->result_buf, lex->result_pos-lex->result_buf)
#define mln_lex_error_set(lex,err) ((lex)->error = (err))
MLN_FUNC_VOID(static inline, void, mln_lex_stepback, \
              (mln_lex_t *lex, char c), (lex, c), \
{
    if (c == MLN_EOF) return;
    if (lex->cur == NULL || lex->cur->pos <= lex->cur->buf) {
        /* shouldn't be here, lexer crashed */
        abort();
    }
    --(lex->cur->pos);
})
MLN_FUNC(static inline, int, mln_lex_putchar, \
         (mln_lex_t *lex, char c), (lex, c), \
{
    if (lex->result_buf == NULL) {
        if ((lex->result_buf = (mln_u8ptr_t)mln_alloc_m(lex->pool, lex->result_buf_len)) == NULL) {
            lex->error = MLN_LEX_ENMEM;
            return MLN_ERR;
        }
        lex->result_pos = lex->result_buf;
    }
    if (lex->result_pos >= lex->result_buf+lex->result_buf_len) {
        mln_u64_t len = lex->result_buf_len + 1;
        lex->result_buf_len += (len>>1);
        mln_u8ptr_t tmp = lex->result_buf;
        if ((lex->result_buf = (mln_u8ptr_t)mln_alloc_re(lex->pool, tmp, lex->result_buf_len)) == NULL) {
            lex->result_buf = tmp;
            lex->result_buf_len = len - 1;
            lex->error = MLN_LEX_ENMEM;
            return MLN_ERR;
        }
        lex->result_pos = lex->result_buf + len - 1;
    }
    *(lex->result_pos)++ = (mln_u8_t)c;
    return 0;
})

MLN_FUNC(static inline, char, mln_lex_getchar, (mln_lex_t *lex), (lex), {
    int n;
    mln_lex_input_t *in;
lp:
    if (lex->cur == NULL && (lex->cur = (mln_lex_input_t *)mln_stack_pop(lex->stack)) == NULL) {
        return MLN_EOF;
    }
    in = lex->cur;
    if (in->type == M_INPUT_T_BUF) {
        if (in->buf == NULL) {
            in->pos = in->buf = in->data->data;
        }
        if (in->pos >= in->buf+in->buf_len) {
            lex->line = in->line;
            mln_lex_input_free(in);
            lex->cur = NULL;
            goto lp;
        }
    } else {
        if (in->buf == NULL) {
            if ((in->buf = (mln_u8ptr_t)mln_alloc_m(lex->pool, MLN_DEFAULT_BUFLEN)) == NULL) {
                lex->error = MLN_LEX_ENMEM;
                return MLN_ERR;
            }
            in->pos = in->buf + in->buf_len;
        }
        if (in->pos >= in->buf+in->buf_len) {
again:
            if ((n = read(in->fd, in->buf, MLN_DEFAULT_BUFLEN)) < 0) {
                if (errno == EINTR) goto again;
                lex->error = MLN_LEX_EREAD;
                return MLN_ERR;
            } else if (n == 0) {
                lex->line = in->line;
                mln_lex_input_free(in);
                lex->cur = NULL;
                goto lp;
            }
            in->pos = in->buf;
            in->buf_len = n;
        }
    }
    return (char)(*(in->pos)++);
})

MLN_FUNC(static inline, int, mln_lex_is_letter, (char c), (c), {
    if (c == '_' || mln_isalpha(c))
        return 1;
    return 0;
})

MLN_FUNC(static inline, int, mln_lex_is_oct, (char c), (c), {
    if (c >= '0' && c < '8')
        return 1;
    return 0;
})

MLN_FUNC(static inline, int, mln_lex_is_hex, (char c), (c), {
    if (mln_isdigit(c) || \
        (c >= 'a' && c <= 'f') || \
        (c >= 'A' && c <= 'F'))
        return 1;
    return 0;
})




/*
 * We must make sure the sequence
 * and number of keywords in array
 * are the same as them in macro MLN_DEFINE_TOKEN_TYPE_AND_STRUCT
 */
#define MLN_DEFINE_TOKEN_TYPE_AND_STRUCT(SCOPE,PREFIX_NAME,TK_PREFIX,...); \
enum PREFIX_NAME##_enum { \
    TK_PREFIX##_TK_EOF = 0,        \
    TK_PREFIX##_TK_OCT,            \
    TK_PREFIX##_TK_DEC,            \
    TK_PREFIX##_TK_HEX,            \
    TK_PREFIX##_TK_REAL,           \
    TK_PREFIX##_TK_ID,             \
    TK_PREFIX##_TK_SPACE,          \
    TK_PREFIX##_TK_EXCL    /*!*/,  \
    TK_PREFIX##_TK_DBLQ    /*"*/,  \
    TK_PREFIX##_TK_NUMS    /*#*/,  \
    TK_PREFIX##_TK_DOLL    /*$*/,  \
    TK_PREFIX##_TK_PERC    /*%*/,  \
    TK_PREFIX##_TK_AMP     /*&*/,  \
    TK_PREFIX##_TK_SGLQ    /*'*/,  \
    TK_PREFIX##_TK_LPAR    /*(*/,  \
    TK_PREFIX##_TK_RPAR    /*)*/,  \
    TK_PREFIX##_TK_AST     /***/,  \
    TK_PREFIX##_TK_PLUS    /*+*/,  \
    TK_PREFIX##_TK_COMMA   /*,*/,  \
    TK_PREFIX##_TK_SUB     /*-*/,  \
    TK_PREFIX##_TK_PERIOD  /*.*/,  \
    TK_PREFIX##_TK_SLASH   /*'/'*/,\
    TK_PREFIX##_TK_COLON   /*:*/,  \
    TK_PREFIX##_TK_SEMIC   /*;*/,  \
    TK_PREFIX##_TK_LAGL    /*<*/,  \
    TK_PREFIX##_TK_EQUAL   /*=*/,  \
    TK_PREFIX##_TK_RAGL    /*>*/,  \
    TK_PREFIX##_TK_QUES    /*?*/,  \
    TK_PREFIX##_TK_AT      /*@*/,  \
    TK_PREFIX##_TK_LSQUAR  /*[*/,  \
    TK_PREFIX##_TK_BSLASH  /*\*/,  \
    TK_PREFIX##_TK_RSQUAR  /*]*/,  \
    TK_PREFIX##_TK_XOR     /*^*/,  \
    TK_PREFIX##_TK_UNDER   /*_*/,  \
    TK_PREFIX##_TK_FULSTP  /*`*/,  \
    TK_PREFIX##_TK_LBRACE  /*{*/,  \
    TK_PREFIX##_TK_VERTL   /*|*/,  \
    TK_PREFIX##_TK_RBRACE  /*}*/,  \
    TK_PREFIX##_TK_DASH    /*~*/,  \
    TK_PREFIX##_TK_KEYWORD_BEGIN,  \
    ## __VA_ARGS__        \
};\
\
typedef struct {\
    mln_string_t              *text; \
    mln_u32_t                  line; \
    enum PREFIX_NAME##_enum    type; \
    mln_string_t              *file; \
} PREFIX_NAME##_struct_t;            \
\
typedef struct {\
    enum PREFIX_NAME##_enum    type;\
    char                      *type_str;\
} PREFIX_NAME##_type_t;\
SCOPE PREFIX_NAME##_struct_t *PREFIX_NAME##_new(mln_lex_t *lex, enum PREFIX_NAME##_enum type);\
SCOPE void PREFIX_NAME##_free(PREFIX_NAME##_struct_t *ptr);                                   \
SCOPE PREFIX_NAME##_struct_t *PREFIX_NAME##_token(mln_lex_t *lex);\
void *PREFIX_NAME##_lex_dup(mln_alloc_t *pool, void *ptr);\
SCOPE PREFIX_NAME##_struct_t *PREFIX_NAME##_nums_handler(mln_lex_t *lex, void *data);


#define MLN_DEFINE_TOKEN(SCOPE, PREFIX_NAME,TK_PREFIX,...); \
PREFIX_NAME##_type_t PREFIX_NAME##_token_type_array[] = {           \
    {TK_PREFIX##_TK_EOF,          #TK_PREFIX"_TK_EOF"},             \
    {TK_PREFIX##_TK_OCT,          #TK_PREFIX"_TK_OCT"},             \
    {TK_PREFIX##_TK_DEC,          #TK_PREFIX"_TK_DEC"},             \
    {TK_PREFIX##_TK_HEX,          #TK_PREFIX"_TK_HEX"},             \
    {TK_PREFIX##_TK_REAL,         #TK_PREFIX"_TK_REAL"},            \
    {TK_PREFIX##_TK_ID,           #TK_PREFIX"_TK_ID"},              \
    {TK_PREFIX##_TK_SPACE,        #TK_PREFIX"_TK_SPACE"},           \
    {TK_PREFIX##_TK_EXCL,         #TK_PREFIX"_TK_EXCL"}     /*!*/,  \
    {TK_PREFIX##_TK_DBLQ,         #TK_PREFIX"_TK_DBLQ"}     /*"*/,  \
    {TK_PREFIX##_TK_NUMS,         #TK_PREFIX"_TK_NUMS"}     /*#*/,  \
    {TK_PREFIX##_TK_DOLL,         #TK_PREFIX"_TK_DOLL"}     /*$*/,  \
    {TK_PREFIX##_TK_PERC,         #TK_PREFIX"_TK_PERC"}     /*%*/,  \
    {TK_PREFIX##_TK_AMP,          #TK_PREFIX"_TK_AMP"}      /*&*/,  \
    {TK_PREFIX##_TK_SGLQ,         #TK_PREFIX"_TK_SGLQ"}     /*'*/,  \
    {TK_PREFIX##_TK_LPAR,         #TK_PREFIX"_TK_LPAR"}     /*(*/,  \
    {TK_PREFIX##_TK_RPAR,         #TK_PREFIX"_TK_RPAR"}     /*)*/,  \
    {TK_PREFIX##_TK_AST,          #TK_PREFIX"_TK_AST"}      /***/,  \
    {TK_PREFIX##_TK_PLUS,         #TK_PREFIX"_TK_PLUS"}     /*+*/,  \
    {TK_PREFIX##_TK_COMMA,        #TK_PREFIX"_TK_COMMA"}    /*,*/,  \
    {TK_PREFIX##_TK_SUB,          #TK_PREFIX"_TK_SUB"}      /*-*/,  \
    {TK_PREFIX##_TK_PERIOD,       #TK_PREFIX"_TK_PERIOD"}   /*.*/,  \
    {TK_PREFIX##_TK_SLASH,        #TK_PREFIX"_TK_SLASH"}    /*'/'*/,\
    {TK_PREFIX##_TK_COLON,        #TK_PREFIX"_TK_COLON"}    /*:*/,  \
    {TK_PREFIX##_TK_SEMIC,        #TK_PREFIX"_TK_SEMIC"}    /*;*/,  \
    {TK_PREFIX##_TK_LAGL,         #TK_PREFIX"_TK_LAGL"}     /*<*/,  \
    {TK_PREFIX##_TK_EQUAL,        #TK_PREFIX"_TK_EQUAL"}    /*=*/,  \
    {TK_PREFIX##_TK_RAGL,         #TK_PREFIX"_TK_RAGL"}     /*>*/,  \
    {TK_PREFIX##_TK_QUES,         #TK_PREFIX"_TK_QUES"}     /*?*/,  \
    {TK_PREFIX##_TK_AT,           #TK_PREFIX"_TK_AT"}       /*@*/,  \
    {TK_PREFIX##_TK_LSQUAR,       #TK_PREFIX"_TK_LSQUAR"}   /*[*/,  \
    {TK_PREFIX##_TK_BSLASH,       #TK_PREFIX"_TK_BSLASH"}   /*\*/,  \
    {TK_PREFIX##_TK_RSQUAR,       #TK_PREFIX"_TK_RSQUAR"}   /*]*/,  \
    {TK_PREFIX##_TK_XOR,          #TK_PREFIX"_TK_XOR"}      /*^*/,  \
    {TK_PREFIX##_TK_UNDER,        #TK_PREFIX"_TK_UNDER"}    /*_*/,  \
    {TK_PREFIX##_TK_FULSTP,       #TK_PREFIX"_TK_FULSTP"}   /*`*/,  \
    {TK_PREFIX##_TK_LBRACE,       #TK_PREFIX"_TK_LBRACE"}   /*{*/,  \
    {TK_PREFIX##_TK_VERTL,        #TK_PREFIX"_TK_VERTL"}    /*|*/,  \
    {TK_PREFIX##_TK_RBRACE,       #TK_PREFIX"_TK_RBRACE"}   /*}*/,  \
    {TK_PREFIX##_TK_DASH,         #TK_PREFIX"_TK_DASH"}     /*~*/,  \
    {TK_PREFIX##_TK_KEYWORD_BEGIN,#TK_PREFIX"_TK_KEYWORD_BEGIN"},   \
    ## __VA_ARGS__                                                  \
};\
    MLN_FUNC(SCOPE, PREFIX_NAME##_struct_t *, PREFIX_NAME##_excl_default_handler, (mln_lex_t *lex, void *data), (lex, data), {\
        return PREFIX_NAME##_new(lex, TK_PREFIX##_TK_EXCL);\
    })\
    MLN_FUNC(SCOPE, PREFIX_NAME##_struct_t *, PREFIX_NAME##_dblq_default_handler, (mln_lex_t *lex, void *data), (lex, data), {\
        return PREFIX_NAME##_new(lex, TK_PREFIX##_TK_DBLQ);\
    })\
    MLN_FUNC(SCOPE, PREFIX_NAME##_struct_t *, PREFIX_NAME##_nums_default_handler, (mln_lex_t *lex, void *data), (lex, data), {\
        return PREFIX_NAME##_new(lex, TK_PREFIX##_TK_NUMS);\
    })\
    MLN_FUNC(SCOPE, PREFIX_NAME##_struct_t *, PREFIX_NAME##_doll_default_handler, (mln_lex_t *lex, void *data), (lex, data), {\
        return PREFIX_NAME##_new(lex, TK_PREFIX##_TK_DOLL);\
    })\
    MLN_FUNC(SCOPE, PREFIX_NAME##_struct_t *, PREFIX_NAME##_perc_default_handler, (mln_lex_t *lex, void *data), (lex, data), {\
        return PREFIX_NAME##_new(lex, TK_PREFIX##_TK_PERC);\
    })\
    MLN_FUNC(SCOPE, PREFIX_NAME##_struct_t *, PREFIX_NAME##_amp_default_handler, (mln_lex_t *lex, void *data), (lex, data), {\
        return PREFIX_NAME##_new(lex, TK_PREFIX##_TK_AMP);\
    })\
    MLN_FUNC(SCOPE, PREFIX_NAME##_struct_t *, PREFIX_NAME##_sglq_default_handler, (mln_lex_t *lex, void *data), (lex, data), {\
        return PREFIX_NAME##_new(lex, TK_PREFIX##_TK_SGLQ);\
    })\
    MLN_FUNC(SCOPE, PREFIX_NAME##_struct_t *, PREFIX_NAME##_lpar_default_handler, (mln_lex_t *lex, void *data), (lex, data), {\
        return PREFIX_NAME##_new(lex, TK_PREFIX##_TK_LPAR);\
    })\
    MLN_FUNC(SCOPE, PREFIX_NAME##_struct_t *, PREFIX_NAME##_rpar_default_handler, (mln_lex_t *lex, void *data), (lex, data), {\
        return PREFIX_NAME##_new(lex, TK_PREFIX##_TK_RPAR);\
    })\
    MLN_FUNC(SCOPE, PREFIX_NAME##_struct_t *, PREFIX_NAME##_ast_default_handler, (mln_lex_t *lex, void *data), (lex, data), {\
        return PREFIX_NAME##_new(lex, TK_PREFIX##_TK_AST);\
    })\
    MLN_FUNC(SCOPE, PREFIX_NAME##_struct_t *, PREFIX_NAME##_plus_default_handler, (mln_lex_t *lex, void *data), (lex, data), {\
        return PREFIX_NAME##_new(lex, TK_PREFIX##_TK_PLUS);\
    })\
    MLN_FUNC(SCOPE, PREFIX_NAME##_struct_t *, PREFIX_NAME##_comma_default_handler, (mln_lex_t *lex, void *data), (lex, data), {\
        return PREFIX_NAME##_new(lex, TK_PREFIX##_TK_COMMA);\
    })\
    MLN_FUNC(SCOPE, PREFIX_NAME##_struct_t *, PREFIX_NAME##_sub_default_handler, (mln_lex_t *lex, void *data), (lex, data), {\
        return PREFIX_NAME##_new(lex, TK_PREFIX##_TK_SUB);\
    })\
    MLN_FUNC(SCOPE, PREFIX_NAME##_struct_t *, PREFIX_NAME##_period_default_handler, (mln_lex_t *lex, void *data), (lex, data), {\
        return PREFIX_NAME##_new(lex, TK_PREFIX##_TK_PERIOD);\
    })\
    MLN_FUNC(SCOPE, PREFIX_NAME##_struct_t *, PREFIX_NAME##_slash_default_handler, (mln_lex_t *lex, void *data), (lex, data), {\
        return PREFIX_NAME##_new(lex, TK_PREFIX##_TK_SLASH);\
    })\
    MLN_FUNC(SCOPE, PREFIX_NAME##_struct_t *, PREFIX_NAME##_colon_default_handler, (mln_lex_t *lex, void *data), (lex, data), {\
        return PREFIX_NAME##_new(lex, TK_PREFIX##_TK_COLON);\
    })\
    MLN_FUNC(SCOPE, PREFIX_NAME##_struct_t *, PREFIX_NAME##_semic_default_handler, (mln_lex_t *lex, void *data), (lex, data), {\
        return PREFIX_NAME##_new(lex, TK_PREFIX##_TK_SEMIC);\
    })\
    MLN_FUNC(SCOPE, PREFIX_NAME##_struct_t *, PREFIX_NAME##_lagl_default_handler, (mln_lex_t *lex, void *data), (lex, data), {\
        return PREFIX_NAME##_new(lex, TK_PREFIX##_TK_LAGL);\
    })\
    MLN_FUNC(SCOPE, PREFIX_NAME##_struct_t *, PREFIX_NAME##_equal_default_handler, (mln_lex_t *lex, void *data), (lex, data), {\
        return PREFIX_NAME##_new(lex, TK_PREFIX##_TK_EQUAL);\
    })\
    MLN_FUNC(SCOPE, PREFIX_NAME##_struct_t *, PREFIX_NAME##_ragl_default_handler, (mln_lex_t *lex, void *data), (lex, data), {\
        return PREFIX_NAME##_new(lex, TK_PREFIX##_TK_RAGL);\
    })\
    MLN_FUNC(SCOPE, PREFIX_NAME##_struct_t *, PREFIX_NAME##_ques_default_handler, (mln_lex_t *lex, void *data), (lex, data), {\
        return PREFIX_NAME##_new(lex, TK_PREFIX##_TK_QUES);\
    })\
    MLN_FUNC(SCOPE, PREFIX_NAME##_struct_t *, PREFIX_NAME##_at_default_handler, (mln_lex_t *lex, void *data), (lex, data), {\
        return PREFIX_NAME##_new(lex, TK_PREFIX##_TK_AT);\
    })\
    MLN_FUNC(SCOPE, PREFIX_NAME##_struct_t *, PREFIX_NAME##_lsquar_default_handler, (mln_lex_t *lex, void *data), (lex, data), {\
        return PREFIX_NAME##_new(lex, TK_PREFIX##_TK_LSQUAR);\
    })\
    MLN_FUNC(SCOPE, PREFIX_NAME##_struct_t *, PREFIX_NAME##_bslash_default_handler, (mln_lex_t *lex, void *data), (lex, data), {\
        return PREFIX_NAME##_new(lex, TK_PREFIX##_TK_BSLASH);\
    })\
    MLN_FUNC(SCOPE, PREFIX_NAME##_struct_t *, PREFIX_NAME##_rsquar_default_handler, (mln_lex_t *lex, void *data), (lex, data), {\
        return PREFIX_NAME##_new(lex, TK_PREFIX##_TK_RSQUAR);\
    })\
    MLN_FUNC(SCOPE, PREFIX_NAME##_struct_t *, PREFIX_NAME##_xor_default_handler, (mln_lex_t *lex, void *data), (lex, data), {\
        return PREFIX_NAME##_new(lex, TK_PREFIX##_TK_XOR);\
    })\
    MLN_FUNC(SCOPE, PREFIX_NAME##_struct_t *, PREFIX_NAME##_under_default_handler, (mln_lex_t *lex, void *data), (lex, data), {\
        return PREFIX_NAME##_new(lex, TK_PREFIX##_TK_UNDER);\
    })\
    MLN_FUNC(SCOPE, PREFIX_NAME##_struct_t *, PREFIX_NAME##_fulstp_default_handler, (mln_lex_t *lex, void *data), (lex, data), {\
        return PREFIX_NAME##_new(lex, TK_PREFIX##_TK_FULSTP);\
    })\
    MLN_FUNC(SCOPE, PREFIX_NAME##_struct_t *, PREFIX_NAME##_lbrace_default_handler, (mln_lex_t *lex, void *data), (lex, data), {\
        return PREFIX_NAME##_new(lex, TK_PREFIX##_TK_LBRACE);\
    })\
    MLN_FUNC(SCOPE, PREFIX_NAME##_struct_t *, PREFIX_NAME##_vertl_default_handler, (mln_lex_t *lex, void *data), (lex, data),  {\
        return PREFIX_NAME##_new(lex, TK_PREFIX##_TK_VERTL);\
    })\
    MLN_FUNC(SCOPE, PREFIX_NAME##_struct_t *, PREFIX_NAME##_rbrace_default_handler, (mln_lex_t *lex, void *data), (lex, data), {\
        return PREFIX_NAME##_new(lex, TK_PREFIX##_TK_RBRACE);\
    })\
    MLN_FUNC(SCOPE, PREFIX_NAME##_struct_t *, PREFIX_NAME##_dash_default_handler, (mln_lex_t *lex, void *data), (lex, data), {\
        return PREFIX_NAME##_new(lex, TK_PREFIX##_TK_DASH);\
    })\
    \
    SCOPE mln_spechar_t PREFIX_NAME##_handlers[] = {         \
        {'!',  (lex_hook)PREFIX_NAME##_excl_default_handler,   NULL},\
        {'"',  (lex_hook)PREFIX_NAME##_dblq_default_handler,   NULL},\
        {'#',  (lex_hook)PREFIX_NAME##_nums_default_handler,   NULL},\
        {'$',  (lex_hook)PREFIX_NAME##_doll_default_handler,   NULL},\
        {'%',  (lex_hook)PREFIX_NAME##_perc_default_handler,   NULL},\
        {'&',  (lex_hook)PREFIX_NAME##_amp_default_handler,    NULL},\
        {'\'', (lex_hook)PREFIX_NAME##_sglq_default_handler,   NULL},\
        {'(',  (lex_hook)PREFIX_NAME##_lpar_default_handler,   NULL},\
        {')',  (lex_hook)PREFIX_NAME##_rpar_default_handler,   NULL},\
        {'*',  (lex_hook)PREFIX_NAME##_ast_default_handler,    NULL},\
        {'+',  (lex_hook)PREFIX_NAME##_plus_default_handler,   NULL},\
        {',',  (lex_hook)PREFIX_NAME##_comma_default_handler,  NULL},\
        {'-',  (lex_hook)PREFIX_NAME##_sub_default_handler,    NULL},\
        {'.',  (lex_hook)PREFIX_NAME##_period_default_handler, NULL},\
        {'/',  (lex_hook)PREFIX_NAME##_slash_default_handler,  NULL},\
        {':',  (lex_hook)PREFIX_NAME##_colon_default_handler,  NULL},\
        {';',  (lex_hook)PREFIX_NAME##_semic_default_handler,  NULL},\
        {'<',  (lex_hook)PREFIX_NAME##_lagl_default_handler,   NULL},\
        {'=',  (lex_hook)PREFIX_NAME##_equal_default_handler,  NULL},\
        {'>',  (lex_hook)PREFIX_NAME##_ragl_default_handler,   NULL},\
        {'?',  (lex_hook)PREFIX_NAME##_ques_default_handler,   NULL},\
        {'@',  (lex_hook)PREFIX_NAME##_at_default_handler,     NULL},\
        {'[',  (lex_hook)PREFIX_NAME##_lsquar_default_handler, NULL},\
        {'\\', (lex_hook)PREFIX_NAME##_bslash_default_handler, NULL},\
        {']',  (lex_hook)PREFIX_NAME##_rsquar_default_handler, NULL},\
        {'^',  (lex_hook)PREFIX_NAME##_xor_default_handler,    NULL},\
        {'_',  (lex_hook)PREFIX_NAME##_under_default_handler,  NULL},\
        {'`',  (lex_hook)PREFIX_NAME##_fulstp_default_handler, NULL},\
        {'{',  (lex_hook)PREFIX_NAME##_lbrace_default_handler, NULL},\
        {'|',  (lex_hook)PREFIX_NAME##_vertl_default_handler,  NULL},\
        {'}',  (lex_hook)PREFIX_NAME##_rbrace_default_handler, NULL},\
        {'~',  (lex_hook)PREFIX_NAME##_dash_default_handler,   NULL} \
    };\
    \
    MLN_FUNC_VOID(SCOPE, void, PREFIX_NAME##_set_hooks, (mln_lex_t *lex), (lex), {\
        mln_lex_hooks_t *hooks = &(lex->hooks);\
        if (hooks->excl_handler != NULL)   {PREFIX_NAME##_handlers[0].handler  = hooks->excl_handler;  PREFIX_NAME##_handlers[0].data = hooks->excl_data;   }\
        if (hooks->dblq_handler != NULL)   {PREFIX_NAME##_handlers[1].handler  = hooks->dblq_handler;  PREFIX_NAME##_handlers[1].data = hooks->dblq_data;   }\
        if (hooks->nums_handler != NULL)   {PREFIX_NAME##_handlers[2].handler  = hooks->nums_handler;  PREFIX_NAME##_handlers[2].data = hooks->nums_data;   }\
        if (hooks->doll_handler != NULL)   {PREFIX_NAME##_handlers[3].handler  = hooks->doll_handler;  PREFIX_NAME##_handlers[3].data = hooks->doll_data;   }\
        if (hooks->perc_handler != NULL)   {PREFIX_NAME##_handlers[4].handler  = hooks->perc_handler;  PREFIX_NAME##_handlers[4].data = hooks->perc_data;   }\
        if (hooks->amp_handler != NULL)    {PREFIX_NAME##_handlers[5].handler  = hooks->amp_handler;   PREFIX_NAME##_handlers[5].data = hooks->amp_data;    }\
        if (hooks->sglq_handler != NULL)   {PREFIX_NAME##_handlers[6].handler  = hooks->sglq_handler;  PREFIX_NAME##_handlers[6].data = hooks->slgq_data;   }\
        if (hooks->lpar_handler != NULL)   {PREFIX_NAME##_handlers[7].handler  = hooks->lpar_handler;  PREFIX_NAME##_handlers[7].data = hooks->lpar_data;   }\
        if (hooks->rpar_handler != NULL)   {PREFIX_NAME##_handlers[8].handler  = hooks->rpar_handler;  PREFIX_NAME##_handlers[8].data = hooks->rpar_data;   }\
        if (hooks->ast_handler != NULL)    {PREFIX_NAME##_handlers[9].handler  = hooks->ast_handler;   PREFIX_NAME##_handlers[9].data = hooks->ast_data;    }\
        if (hooks->plus_handler != NULL)   {PREFIX_NAME##_handlers[10].handler = hooks->plus_handler;  PREFIX_NAME##_handlers[10].data = hooks->plus_data;  }\
        if (hooks->comma_handler != NULL)  {PREFIX_NAME##_handlers[11].handler = hooks->comma_handler; PREFIX_NAME##_handlers[11].data = hooks->comma_data; }\
        if (hooks->sub_handler != NULL)    {PREFIX_NAME##_handlers[12].handler = hooks->sub_handler;   PREFIX_NAME##_handlers[12].data = hooks->sub_data;   }\
        if (hooks->period_handler != NULL) {PREFIX_NAME##_handlers[13].handler = hooks->period_handler;PREFIX_NAME##_handlers[13].data = hooks->period_data;}\
        if (hooks->slash_handler != NULL)  {PREFIX_NAME##_handlers[14].handler = hooks->slash_handler; PREFIX_NAME##_handlers[14].data = hooks->slash_data; }\
        if (hooks->colon_handler != NULL)  {PREFIX_NAME##_handlers[15].handler = hooks->colon_handler; PREFIX_NAME##_handlers[15].data = hooks->colon_data; }\
        if (hooks->semic_handler != NULL)  {PREFIX_NAME##_handlers[16].handler = hooks->semic_handler; PREFIX_NAME##_handlers[16].data = hooks->semic_data; }\
        if (hooks->lagl_handler != NULL)   {PREFIX_NAME##_handlers[17].handler = hooks->lagl_handler;  PREFIX_NAME##_handlers[17].data = hooks->lagl_data;  }\
        if (hooks->equal_handler != NULL)  {PREFIX_NAME##_handlers[18].handler = hooks->equal_handler; PREFIX_NAME##_handlers[18].data = hooks->equal_data; }\
        if (hooks->ragl_handler != NULL)   {PREFIX_NAME##_handlers[19].handler = hooks->ragl_handler;  PREFIX_NAME##_handlers[19].data = hooks->ragl_data;  }\
        if (hooks->ques_handler != NULL)   {PREFIX_NAME##_handlers[20].handler = hooks->ques_handler;  PREFIX_NAME##_handlers[20].data = hooks->ques_data;  }\
        if (hooks->at_handler != NULL)     {PREFIX_NAME##_handlers[21].handler = hooks->at_handler;    PREFIX_NAME##_handlers[21].data = hooks->at_data;    }\
        if (hooks->lsquar_handler != NULL) {PREFIX_NAME##_handlers[22].handler = hooks->lsquar_handler;PREFIX_NAME##_handlers[22].data = hooks->lsquar_data;}\
        if (hooks->bslash_handler != NULL) {PREFIX_NAME##_handlers[23].handler = hooks->bslash_handler;PREFIX_NAME##_handlers[23].data = hooks->bslash_data;}\
        if (hooks->rsquar_handler != NULL) {PREFIX_NAME##_handlers[24].handler = hooks->rsquar_handler;PREFIX_NAME##_handlers[24].data = hooks->rsquar_data;}\
        if (hooks->xor_handler != NULL)    {PREFIX_NAME##_handlers[25].handler = hooks->xor_handler;   PREFIX_NAME##_handlers[25].data = hooks->xor_data;   }\
        if (hooks->under_handler != NULL)  {PREFIX_NAME##_handlers[26].handler = hooks->under_handler; PREFIX_NAME##_handlers[26].data = hooks->under_data; }\
        if (hooks->fulstp_handler != NULL) {PREFIX_NAME##_handlers[27].handler = hooks->fulstp_handler;PREFIX_NAME##_handlers[27].data = hooks->fulstp_data;}\
        if (hooks->lbrace_handler != NULL) {PREFIX_NAME##_handlers[28].handler = hooks->lbrace_handler;PREFIX_NAME##_handlers[28].data = hooks->lbrace_data;}\
        if (hooks->vertl_handler != NULL)  {PREFIX_NAME##_handlers[29].handler = hooks->vertl_handler; PREFIX_NAME##_handlers[29].data = hooks->vertl_data; }\
        if (hooks->rbrace_handler != NULL) {PREFIX_NAME##_handlers[30].handler = hooks->rbrace_handler;PREFIX_NAME##_handlers[30].data = hooks->rbrace_data;}\
        if (hooks->dash_handler != NULL)   {PREFIX_NAME##_handlers[31].handler = hooks->dash_handler;  PREFIX_NAME##_handlers[31].data = hooks->dash_data;  }\
    })\
    \
    MLN_FUNC(SCOPE, PREFIX_NAME##_struct_t *, PREFIX_NAME##_new, (mln_lex_t *lex, enum PREFIX_NAME##_enum type), (lex, type), {\
        mln_string_t tmp;\
        PREFIX_NAME##_struct_t *ptr;\
        if ((ptr = (PREFIX_NAME##_struct_t *)mln_alloc_m(lex->pool, sizeof(PREFIX_NAME##_struct_t))) == NULL) {\
            mln_lex_error_set(lex, MLN_LEX_ENMEM);\
            return NULL;\
        }\
        if (type != TK_PREFIX##_TK_EOF) {\
            mln_string_nset(&tmp, lex->result_buf, lex->result_pos - lex->result_buf);\
        } else {\
            mln_string_nset(&tmp, MLN_EOF_TEXT, sizeof(MLN_EOF_TEXT)-1);\
        }\
        if ((ptr->text = mln_string_pool_dup(lex->pool, &tmp)) == NULL) {\
            mln_alloc_free(ptr);\
            mln_lex_error_set(lex, MLN_LEX_ENMEM);\
            return NULL;\
        }\
        ptr->line = lex->line;\
        ptr->type = type;\
        ptr->file = NULL;\
        if (lex->cur != NULL && lex->cur->type == M_INPUT_T_FILE) {\
            ptr->file = mln_string_ref(lex->cur->data);\
        }\
        mln_lex_error_set(lex, MLN_LEX_SUCCEED);\
        lex->result_pos = lex->result_buf;\
        return ptr;\
    })\
    \
    MLN_FUNC_VOID(SCOPE, void, PREFIX_NAME##_free, (PREFIX_NAME##_struct_t *ptr), (ptr), {\
        if (ptr == NULL) return ;\
        if (ptr->text != NULL) mln_string_free(ptr->text);\
        if (ptr->file != NULL) mln_string_free(ptr->file);\
        mln_alloc_free(ptr);\
    })\
    \
    MLN_FUNC(static inline, PREFIX_NAME##_struct_t *, PREFIX_NAME##_process_keywords, (mln_lex_t *lex), (lex), {\
        if (lex->keywords == NULL) return PREFIX_NAME##_new(lex, TK_PREFIX##_TK_ID);\
        mln_string_t tmp;\
        mln_u32_t diff = lex->result_pos - lex->result_buf;\
        mln_u8ptr_t p = lex->result_buf;\
        mln_lex_keyword_t lk, *plk;\
        mln_rbtree_node_t *rn;\
        mln_string_nset(&tmp, p, diff);\
        lk.keyword = &tmp;\
        rn = mln_rbtree_search(lex->keywords, &lk);\
        if (!mln_rbtree_null(rn, lex->keywords)) {\
            plk = (mln_lex_keyword_t *)mln_rbtree_node_data_get(rn);\
            return PREFIX_NAME##_new(lex, TK_PREFIX##_TK_KEYWORD_BEGIN+plk->val+1);\
        }\
        return PREFIX_NAME##_new(lex, TK_PREFIX##_TK_ID);\
    })\
    \
    MLN_FUNC(static inline, PREFIX_NAME##_struct_t *, PREFIX_NAME##_process_spec_char, (mln_lex_t *lex, char c), (lex, c), {\
        mln_s32_t i;\
        mln_s32_t end = sizeof(PREFIX_NAME##_handlers)/sizeof(mln_spechar_t);\
        for (i = 0; i<end; ++i) {\
            if (c == PREFIX_NAME##_handlers[i].sc) {\
                return (PREFIX_NAME##_struct_t *)PREFIX_NAME##_handlers[i].handler(lex, PREFIX_NAME##_handlers[i].data);\
            }\
        }\
        mln_lex_error_set(lex, MLN_LEX_EINVCHAR);\
        return NULL;\
    })\
    \
    MLN_FUNC(SCOPE, PREFIX_NAME##_struct_t *, PREFIX_NAME##_token, (mln_lex_t *lex), (lex), {\
        char c;\
        PREFIX_NAME##_struct_t *sret;\
beg:\
        if ((c = mln_lex_getchar(lex)) == MLN_ERR) return NULL;\
lp:\
        switch (c) {\
            case (char)MLN_EOF:\
                 return PREFIX_NAME##_new(lex, TK_PREFIX##_TK_EOF);\
            case '\n':\
                 {\
                     while (c == '\n') {\
                         ++(lex->line);\
                         c = mln_lex_getchar(lex);\
                         if (c == MLN_ERR) return NULL;\
                     };\
                     goto lp;\
                 }\
            case '\t':\
            case ' ':\
                 {\
                     while (c == ' ' || c == '\t') {\
                         c = mln_lex_getchar(lex);\
                         if (c == MLN_ERR) return NULL;\
                     }\
                     goto lp;\
                 }\
            default:\
                {\
                    if (mln_lex_is_letter(c)) {\
                        while (mln_lex_is_letter(c) || mln_isdigit(c)) {\
                            if (mln_lex_putchar(lex, c) == MLN_ERR) return NULL;\
                            c = mln_lex_getchar(lex);\
                            if (c == MLN_ERR) return NULL;\
                        }\
                        mln_lex_stepback(lex, c);\
                        if (lex->result_buf[0] == (mln_u8_t)'_' && lex->result_pos == lex->result_buf+1) {\
                            sret = PREFIX_NAME##_process_spec_char(lex, '_');\
                            if (sret == NULL || !lex->ignore) return sret;\
                            goto beg;\
                        }\
                        sret = PREFIX_NAME##_process_keywords(lex);\
                        if (sret == NULL || !lex->ignore) return sret;\
                        goto beg;\
                    } else if (mln_isdigit(c)) {\
                        while ( 1 ) {\
                            if (mln_lex_putchar(lex, c) == MLN_ERR) return NULL;\
                            c = mln_lex_getchar(lex);\
                            if (c == MLN_ERR) return NULL;\
                            if (!mln_isdigit(c) && !mln_isalpha(c) && c != '.') {\
                                mln_lex_stepback(lex, c);\
                                break;\
                            }\
                        }\
                        /*check number*/\
                        mln_u8ptr_t chk = lex->result_buf;\
                        if (*chk == (mln_u8_t)'0' && lex->result_pos-lex->result_buf > 1) {\
                            if (*(chk+1) == (mln_u8_t)'x') {\
                                if (lex->result_pos-lex->result_buf == 2) {\
                                    mln_lex_error_set(lex, MLN_LEX_EINVHEX);\
                                    return NULL;\
                                }\
                                for (chk += 2; chk < lex->result_pos; ++chk) {\
                                    if (!mln_lex_is_hex((char)(*chk))) {\
                                        mln_lex_error_set(lex, MLN_LEX_EINVHEX);\
                                        return NULL;\
                                    }\
                                }\
                                sret = PREFIX_NAME##_new(lex, TK_PREFIX##_TK_HEX);\
                                if (sret == NULL || !lex->ignore) return sret;\
                                goto beg;\
                            } else {\
                                for (++chk; chk < lex->result_pos; ++chk) {\
                                    if (!mln_lex_is_oct((char)(*chk))) {\
                                        mln_s32_t dot_cnt = 0;\
                                        for (; chk < lex->result_pos; ++chk) {\
                                            if (*chk == (mln_u8_t)'.') ++dot_cnt;\
                                            if (!mln_isdigit((char)(*chk)) && *chk != (mln_u8_t)'.') {\
                                                mln_lex_error_set(lex, MLN_LEX_EINVOCT);\
                                                return NULL;\
                                            }\
                                        }\
                                        if (dot_cnt > 1) {\
                                            mln_lex_error_set(lex, MLN_LEX_EINVREAL);\
                                        } else if (dot_cnt == 0) {\
                                            mln_lex_error_set(lex, MLN_LEX_EINVOCT);\
                                        } else {\
                                            sret = PREFIX_NAME##_new(lex, TK_PREFIX##_TK_REAL);\
                                            if (sret == NULL || !lex->ignore) return sret;\
                                            goto beg;\
                                        }\
                                        return NULL;\
                                    }\
                                }\
                                sret = PREFIX_NAME##_new(lex, TK_PREFIX##_TK_OCT);\
                                if (sret == NULL || !lex->ignore) return sret;\
                                goto beg;\
                            }\
                        } else {\
                            for (; chk < lex->result_pos; ++chk) {\
                                if (mln_isdigit((char)(*chk))) continue;\
                                if (*chk == (mln_u8_t)'.') {\
                                    for (++chk; chk < lex->result_pos; ++chk) {\
                                        if (!mln_isdigit((char)(*chk))) {\
                                            mln_lex_error_set(lex, MLN_LEX_EINVREAL);\
                                            return NULL;\
                                        }\
                                    }\
                                    sret = PREFIX_NAME##_new(lex, TK_PREFIX##_TK_REAL);\
                                    if (sret == NULL || !lex->ignore) return sret;\
                                    goto beg;\
                                }\
                                mln_lex_error_set(lex, MLN_LEX_EINVDEC);\
                                return NULL;\
                            }\
                            sret = PREFIX_NAME##_new(lex, TK_PREFIX##_TK_DEC);\
                            if (sret == NULL || !lex->ignore) return sret;\
                            goto beg;\
                        }\
                    } else {\
                        if (mln_lex_putchar(lex, c) == MLN_ERR) return NULL;\
                        sret = PREFIX_NAME##_process_spec_char(lex, c);\
                        if (sret == NULL) break; \
                        if (!lex->ignore) return sret;\
                        goto beg;\
                    }\
                }\
        }\
        return NULL;\
    })\
    \
    MLN_FUNC(, void *, PREFIX_NAME##_lex_dup, (mln_alloc_t *pool, void *ptr), (pool, ptr), {\
        if (ptr == NULL) return NULL;\
        PREFIX_NAME##_struct_t *src = (PREFIX_NAME##_struct_t *)ptr;\
        PREFIX_NAME##_struct_t *dest = (PREFIX_NAME##_struct_t *)mln_alloc_m(pool, sizeof(PREFIX_NAME##_struct_t));\
        if (dest == NULL) return NULL;\
        dest->text = mln_string_pool_dup(pool, src->text);\
        if (dest->text == NULL) {\
            mln_alloc_free(dest);\
            return NULL;\
        }\
        dest->line = src->line;\
        dest->type = src->type;\
        if (src->file == NULL) {\
            dest->file = NULL;\
        } else {\
            dest->file = mln_string_ref(src->file);\
        }\
        return (void *)dest;\
    })\
    \
    MLN_FUNC(SCOPE, PREFIX_NAME##_struct_t *, PREFIX_NAME##_lex_preprocess_include, (mln_lex_t *lex, void *data), (lex, data), {\
        char c;\
        mln_string_t path;\
        while (1) {\
            if ((c = mln_lex_getchar(lex)) == MLN_ERR) {\
                return NULL;\
            }\
            if (c == ' ' || c == '\t') continue;\
            if (c == MLN_EOF) {\
                mln_lex_error_set(lex, MLN_LEX_EINVEOF);\
                return NULL;\
            }\
            if (c == '\n') {\
                mln_lex_error_set(lex, MLN_LEX_EINVEOL);\
                return NULL;\
            }\
            if (c != '\"') {\
                mln_lex_error_set(lex, MLN_LEX_EINVCHAR);\
                return NULL;\
            }\
            break;\
        }\
        mln_lex_result_clean(lex);\
        while (1) {\
            if ((c = mln_lex_getchar(lex)) == MLN_ERR) {\
                return NULL;\
            }\
            if (c == MLN_EOF) {\
                mln_lex_error_set(lex, MLN_LEX_EINVEOF);\
                return NULL;\
            }\
            if (c == '\"') break;\
            if (mln_lex_putchar(lex, c) == MLN_ERR) return NULL;\
        }\
        mln_lex_result_get(lex, &path);\
        if (path.len == 0) {\
            mln_lex_error_set(lex, MLN_LEX_EFPATH);\
            return NULL;\
        }\
        if (mln_lex_check_file_loop(lex, &path) < 0) {\
            return NULL;\
        }\
        if (mln_lex_push_input_file_stream(lex, &path) < 0) {\
            return NULL;\
        }\
        mln_lex_result_clean(lex);\
        return PREFIX_NAME##_token(lex);\
    })\
    MLN_FUNC(SCOPE, PREFIX_NAME##_struct_t *, PREFIX_NAME##_lex_preprocess_define, (mln_lex_t *lex, void *data), (lex, data), {\
        char c;\
        mln_string_t str, *k = NULL, *v = NULL;\
        mln_lex_macro_t *lm, tmp;\
        mln_rbtree_node_t *rn;\
        while (1) {\
            if ((c = mln_lex_getchar(lex)) == MLN_ERR) {\
                return NULL;\
            }\
            if (c == ' ' || c == '\t') continue;\
            if (c == MLN_EOF) {\
                mln_lex_error_set(lex, MLN_LEX_EINVEOF);\
                return NULL;\
            }\
            if (c == '\n') {\
                mln_lex_error_set(lex, MLN_LEX_EINVEOL);\
                return NULL;\
            }\
            if (!mln_lex_is_letter(c) && !mln_isdigit(c)) {\
                mln_lex_error_set(lex, MLN_LEX_EINVCHAR);\
                return NULL;\
            }\
            mln_lex_stepback(lex, c);\
            break;\
        }\
        mln_lex_result_clean(lex);\
        while (1) {\
            if ((c = mln_lex_getchar(lex)) == MLN_ERR) {\
                return NULL;\
            }\
            if (c == ' ' || c == '\t' || c == '\n') break;\
            if (mln_lex_putchar(lex, c) == MLN_ERR) return NULL;\
        }\
        mln_lex_result_get(lex, &str);\
        if (!lex->ignore) {\
            if ((k = mln_string_pool_dup(lex->pool, &str)) == NULL) {\
                mln_lex_error_set(lex, MLN_LEX_ENMEM);\
                return NULL;\
            }\
            tmp.key = k;\
            tmp.val = NULL;\
            rn = mln_rbtree_search(lex->macros, &tmp);\
            if (!mln_rbtree_null(rn, lex->macros)) {\
                mln_string_free(k);\
                return NULL;\
            }\
            if (c != '\n') {\
                mln_lex_result_clean(lex);\
                while (1) {\
                    if ((c = mln_lex_getchar(lex)) == MLN_ERR) {\
                        return NULL;\
                    }\
                    if (c == ' ' || c == '\t') continue;\
                    if (c == MLN_EOF || c == '\n') {\
                        goto goon;\
                    }\
                    mln_lex_stepback(lex, c);\
                    break;\
                }\
again:\
                while (1) {\
                    if ((c = mln_lex_getchar(lex)) == MLN_ERR) {\
                        return NULL;\
                    }\
                    if (c == MLN_EOF || c == '\n') break;\
                    if (mln_lex_putchar(lex, c) == MLN_ERR) return NULL;\
                }\
                if (lex->result_pos > lex->result_buf) {\
                    if (lex->result_pos != NULL && *(lex->result_pos-1) == (mln_u8_t)'\\') {\
                        --(lex->result_pos);\
                        goto again;\
                    }\
                    mln_lex_result_get(lex, &str);\
                    v = &str;\
                }\
            }\
goon:\
            if ((lm = mln_lex_macro_new(lex->pool, k, v)) == NULL) {\
                mln_string_free(k);\
                mln_lex_error_set(lex, MLN_LEX_ENMEM);\
                return NULL;\
            }\
            mln_string_free(k);\
            if ((rn = mln_rbtree_node_new(lex->macros, lm)) == NULL) {\
                mln_lex_error_set(lex, MLN_LEX_ENMEM);\
                return NULL;\
            }\
            mln_rbtree_insert(lex->macros, rn);\
        }\
        mln_lex_result_clean(lex);\
        return PREFIX_NAME##_token(lex);\
    })\
    MLN_FUNC(SCOPE, PREFIX_NAME##_struct_t *, PREFIX_NAME##_lex_preprocess_if, (mln_lex_t *lex, void *data), (lex, data), {\
        int ret;\
        mln_lex_preprocess_data_t *lpd = (mln_lex_preprocess_data_t *)data;\
        mln_lex_result_clean(lex);\
        ++(lpd->if_level);\
        if (lex->ignore) {\
            return PREFIX_NAME##_token(lex);\
        }\
        if ((ret = mln_lex_condition_test(lex)) < 0) return NULL;\
        if (ret) {\
            ++(lpd->if_matched);\
            lex->ignore = 0;\
        } else {\
            lex->ignore = 1;\
        }\
        mln_lex_result_clean(lex);\
        return PREFIX_NAME##_token(lex);\
    })\
    MLN_FUNC(SCOPE, PREFIX_NAME##_struct_t *, PREFIX_NAME##_lex_preprocess_else, (mln_lex_t *lex, void *data), (lex, data), {\
        mln_lex_preprocess_data_t *lpd = (mln_lex_preprocess_data_t *)data;\
        if (lpd->if_level == 0) {\
            mln_lex_error_set(lex, MLN_LEX_EINVMACRO);\
            return NULL;\
        }\
        mln_lex_result_clean(lex);\
        if (lpd->if_level <= lpd->if_matched) {\
            lex->ignore = 1;\
            --(lpd->if_matched);\
        } else if (lpd->if_matched+1 == lpd->if_level) {\
            ++(lpd->if_matched);\
            lex->ignore = 0;\
        }\
        return PREFIX_NAME##_token(lex);\
    })\
    MLN_FUNC(SCOPE, PREFIX_NAME##_struct_t *, PREFIX_NAME##_lex_preprocess_endif, (mln_lex_t *lex, void *data), (lex, data), {\
        mln_lex_preprocess_data_t *lpd = (mln_lex_preprocess_data_t *)data;\
        if (lpd->if_level == 0) {\
            mln_lex_error_set(lex, MLN_LEX_EINVMACRO);\
            return NULL;\
        }\
        if (lpd->if_matched == lpd->if_level--) {\
            --(lpd->if_matched);\
        }\
        lex->ignore = !(lpd->if_matched == lpd->if_level);\
        mln_lex_result_clean(lex);\
        return PREFIX_NAME##_token(lex);\
    })\
    MLN_FUNC(SCOPE, PREFIX_NAME##_struct_t *, PREFIX_NAME##_lex_preprocess_undef, (mln_lex_t *lex, void *data), (lex, data), {\
        char c;\
        mln_string_t str;\
        mln_lex_macro_t tmp;\
        mln_rbtree_node_t *rn;\
        while (1) {\
            if ((c = mln_lex_getchar(lex)) == MLN_ERR) {\
                return NULL;\
            }\
            if (c == ' ' || c == '\t') continue;\
            if (c == MLN_EOF) {\
                mln_lex_error_set(lex, MLN_LEX_EINVEOF);\
                return NULL;\
            }\
            if (c == '\n') {\
                mln_lex_error_set(lex, MLN_LEX_EINVEOL);\
                return NULL;\
            }\
            if (!mln_lex_is_letter(c) && !mln_isdigit(c)) {\
                mln_lex_error_set(lex, MLN_LEX_EINVCHAR);\
                return NULL;\
            }\
            mln_lex_stepback(lex, c);\
            break;\
        }\
        mln_lex_result_clean(lex);\
        while (1) {\
            if ((c = mln_lex_getchar(lex)) == MLN_ERR) {\
                return NULL;\
            }\
            if (c == ' ' || c == '\t' || c == '\n') break;\
            if (mln_lex_putchar(lex, c) == MLN_ERR) return NULL;\
        }\
        mln_lex_result_get(lex, &str);\
        tmp.key = &str;\
        tmp.val = NULL;\
        rn = mln_rbtree_search(lex->macros, &tmp);\
        if (!mln_rbtree_null(rn, lex->macros)) {\
            mln_rbtree_delete(lex->macros, rn);\
            mln_rbtree_node_free(lex->macros, rn);\
        }\
        mln_lex_result_clean(lex);\
        return PREFIX_NAME##_token(lex);\
    })\
    SCOPE mln_preprocess_handler_t PREFIX_NAME##_preprocess_handlers[] = {\
        {mln_string("define"), (lex_hook)PREFIX_NAME##_lex_preprocess_define},\
        {mln_string("include"), (lex_hook)PREFIX_NAME##_lex_preprocess_include},\
        {mln_string("if"), (lex_hook)PREFIX_NAME##_lex_preprocess_if},\
        {mln_string("else"), (lex_hook)PREFIX_NAME##_lex_preprocess_else},\
        {mln_string("endif"), (lex_hook)PREFIX_NAME##_lex_preprocess_endif},\
        {mln_string("undef"), (lex_hook)PREFIX_NAME##_lex_preprocess_undef}\
    };\
    MLN_FUNC(SCOPE, PREFIX_NAME##_struct_t *, PREFIX_NAME##_nums_handler, (mln_lex_t *lex, void *data), (lex, data), {\
        mln_preprocess_handler_t *ph, *phend;\
        mln_string_t tmp;\
        mln_rbtree_node_t *rn;\
        mln_lex_macro_t lm;\
        char c = mln_lex_getchar(lex);\
        if (c == MLN_ERR) return NULL;\
        if (!mln_lex_is_letter(c) && !mln_isdigit(c)) {\
            mln_lex_stepback(lex, c);\
            return PREFIX_NAME##_new(lex, TK_PREFIX##_TK_AT);\
        }\
        mln_lex_result_clean(lex);\
        while (mln_lex_is_letter(c) || mln_isdigit(c)) {\
            if (mln_lex_putchar(lex, c) == MLN_ERR) return NULL;\
            if ((c = mln_lex_getchar(lex)) == MLN_ERR) return NULL;\
        }\
        mln_lex_stepback(lex, c);\
        mln_lex_result_get(lex, &tmp);\
        phend = PREFIX_NAME##_preprocess_handlers + \
                  sizeof(PREFIX_NAME##_preprocess_handlers) / \
                    sizeof(mln_preprocess_handler_t);\
        for (ph = PREFIX_NAME##_preprocess_handlers; ph < phend; ++ph) {\
            if (!mln_string_strcmp(&(ph->command), &tmp)) {\
                mln_lex_result_clean(lex);\
                return (PREFIX_NAME##_struct_t *)ph->handler(lex, data);\
            }\
        }\
        mln_lex_result_get(lex, &tmp);\
        lm.key = &tmp;\
        lm.val = NULL;\
        rn = mln_rbtree_search(lex->macros, &lm);\
        if (!mln_rbtree_null(rn, lex->macros)) {\
            mln_lex_macro_t *ltmp = (mln_lex_macro_t *)mln_rbtree_node_data_get(rn);\
            if (ltmp->val != NULL && mln_lex_push_input_buf_stream(lex, ltmp->val) < 0) {\
                mln_lex_error_set(lex, MLN_LEX_ENMEM);\
                return NULL;\
            }\
            mln_lex_result_clean(lex);\
            return PREFIX_NAME##_token(lex);\
        }\
        mln_lex_error_set(lex, MLN_LEX_EUNKNOWNMACRO);\
        return NULL;\
    })

#define mln_lex_init_with_hooks(PREFIX_NAME,lex_ptr,attr_ptr) \
    if ((attr_ptr)->preprocess) {\
        mln_lex_preprocess_data_t *lpd = mln_lex_preprocess_data_new((attr_ptr)->pool);\
        if (lpd == NULL) {\
            (lex_ptr) = NULL;\
        } else {\
            if ((attr_ptr)->hooks == NULL) {\
                mln_lex_hooks_t __hooks;\
                memset(&__hooks, 0, sizeof(__hooks));\
                __hooks.nums_handler = (lex_hook)PREFIX_NAME##_nums_handler;\
                __hooks.nums_data = lpd;\
                (attr_ptr)->hooks = &__hooks;\
            } else {\
                (attr_ptr)->hooks->nums_handler = (lex_hook)PREFIX_NAME##_nums_handler;\
                (attr_ptr)->hooks->nums_data = lpd;\
            }\
            (lex_ptr) = mln_lex_init((attr_ptr));\
            if ((lex_ptr) != NULL) {\
                if ((attr_ptr)->hooks != NULL) PREFIX_NAME##_set_hooks((lex_ptr));\
                (lex_ptr)->preprocess_data = lpd;\
            } else {\
                mln_lex_preprocess_data_free(lpd);\
            }\
        }\
    } else {\
        (lex_ptr) = mln_lex_init((attr_ptr));\
        if ((lex_ptr) != NULL && (attr_ptr)->hooks != NULL) PREFIX_NAME##_set_hooks((lex_ptr));\
    }

/*
 * Example: if you want to define a lexer, you can follow these steps:
 * Firstly, you need to include header file.
 *     #include "mln_lex.h"
 * this include must be located before any other includes.
 * Secondly, you need only one line to define structure and enumeration, and declare functions:
 *     MLN_DEFINE_TOKEN_TYPE_AND_STRUCT(extern, mln_test_lex, TEST, TEST_TK_FOR, TEST_TK_WHILE, TEST_TK_SWITCH);
 * Thirdly, there is one line for the definitions of functions and arrays.
 *     MLN_DEFINE_TOKEN(mln_test_lex, TEST);
 * Finally, you should initialize lexer in this way,
 *     struct mln_lex_attr attr;
 *     ...//set some hooks or clear hooks with NULL.
 *     mln_lex_t *lex;
 *     mln_lex_init_with_hooks(mln_test_lex, lex, &attr);
 *
 * After these steps, we have already defined some functions which name are:
 *     mln_test_lex_token
 *     mln_test_lex_new
 *     mln_test_lex_free
 * We can customize not only the keywords but also the process of special characters.
 */
 
/*drafts:
-> ++ -- << >> == != && || /= *= %= += -= <<= >>= &= ^= |=
\ \n \t \b \a \f \r \v \\ \" \'  \123 \x1234
'  'a'
"  "abc\t"
. .2
/  //     */    /*
*  */
#endif

