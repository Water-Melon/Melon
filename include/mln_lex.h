
/*
 * Copyright (C) Niklaus F.Schen.
 */
 
#ifndef __MLN_LEX_H
#define __MLN_LEX_H

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "mln_string.h"

#define MLN_EOF (-1)
#define MLN_ERR (-2)
#define MLN_LEX_ERRMSG_LEN 1024
#define MLN_DEFAULT_BUFLEN 4095
#define MLN_EOF_TEXT "##EOF##"

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
    enum {
        mln_lex_file,
        mln_lex_buf
    } input_type;
    union {
        mln_s8ptr_t   filename;
        /*
         * file_buf must be ended by '\0'.
         */
        mln_s8ptr_t   file_buf;
    } input;
    /*
     * keywords must be ended by NULL
     */
    char              **keywords;
    mln_lex_hooks_t   *hooks;
};

struct mln_lex_s {
    mln_string_t        *filename          __cacheline_aligned;
    char                **keywords;
    mln_lex_hooks_t     hooks;
    mln_s8ptr_t         file_buf           __cacheline_aligned;
    mln_s8ptr_t         file_cur_ptr;
    mln_s8ptr_t         result_buf;
    mln_s8ptr_t         result_cur_ptr;
    mln_s8ptr_t         err_msg;
    mln_s32_t           file_buflen;
    mln_s32_t           result_buflen;
    mln_s32_t           error;
    mln_u32_t           line;
    int                 fd;
};


typedef struct {
    char                sc;
    lex_hook            handler;
    void                *data;
} mln_spechar_t;

extern mln_lex_t *
mln_lex_init(struct mln_lex_attr *attr) __NONNULL1(1);
extern void
mln_lex_destroy(mln_lex_t *lex);
extern char *
mln_lex_strerror(mln_lex_t *lex) __NONNULL1(1);
extern char
mln_geta_char(mln_lex_t *lex) __NONNULL1(1);
extern int
mln_puta_char(mln_lex_t *lex, char c) __NONNULL1(1);
extern void
mln_step_back(mln_lex_t *lex) __NONNULL1(1);
extern int
mln_isletter(char c);
extern int
mln_isoctal(char c);
extern int
mln_ishex(char c);


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
    mln_string_t               *text;\
    mln_u32_t                  line; \
    enum PREFIX_NAME##_enum    type; \
} PREFIX_NAME##_struct_t;            \
\
typedef struct {\
    enum PREFIX_NAME##_enum    type;\
    char                       *type_str;\
} PREFIX_NAME##_type_t;\
SCOPE PREFIX_NAME##_struct_t *PREFIX_NAME##_new(mln_lex_t *lex, enum PREFIX_NAME##_enum type);\
SCOPE void PREFIX_NAME##_free(PREFIX_NAME##_struct_t *ptr);                                   \
SCOPE PREFIX_NAME##_struct_t *PREFIX_NAME##_token(mln_lex_t *lex);\
SCOPE void *PREFIX_NAME##_lex_dup(void *ptr);


#define MLN_DEFINE_TOKEN(PREFIX_NAME,TK_PREFIX,...); \
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
    static PREFIX_NAME##_struct_t *PREFIX_NAME##_excl_default_handler(mln_lex_t *lex, void *data)\
    {\
        return PREFIX_NAME##_new(lex, TK_PREFIX##_TK_EXCL);\
    }\
    static PREFIX_NAME##_struct_t *PREFIX_NAME##_dblq_default_handler(mln_lex_t *lex, void *data)\
    {\
        return PREFIX_NAME##_new(lex, TK_PREFIX##_TK_DBLQ);\
    }\
    static PREFIX_NAME##_struct_t *PREFIX_NAME##_nums_default_handler(mln_lex_t *lex, void *data)\
    {\
        return PREFIX_NAME##_new(lex, TK_PREFIX##_TK_NUMS);\
    }\
    static PREFIX_NAME##_struct_t *PREFIX_NAME##_doll_default_handler(mln_lex_t *lex, void *data)\
    {\
        return PREFIX_NAME##_new(lex, TK_PREFIX##_TK_DOLL);\
    }\
    static PREFIX_NAME##_struct_t *PREFIX_NAME##_perc_default_handler(mln_lex_t *lex, void *data)\
    {\
        return PREFIX_NAME##_new(lex, TK_PREFIX##_TK_PERC);\
    }\
    static PREFIX_NAME##_struct_t *PREFIX_NAME##_amp_default_handler(mln_lex_t *lex, void *data)\
    {\
        return PREFIX_NAME##_new(lex, TK_PREFIX##_TK_AMP);\
    }\
    static PREFIX_NAME##_struct_t *PREFIX_NAME##_sglq_default_handler(mln_lex_t *lex, void *data)\
    {\
        return PREFIX_NAME##_new(lex, TK_PREFIX##_TK_SGLQ);\
    }\
    static PREFIX_NAME##_struct_t *PREFIX_NAME##_lpar_default_handler(mln_lex_t *lex, void *data)\
    {\
        return PREFIX_NAME##_new(lex, TK_PREFIX##_TK_LPAR);\
    }\
    static PREFIX_NAME##_struct_t *PREFIX_NAME##_rpar_default_handler(mln_lex_t *lex, void *data)\
    {\
        return PREFIX_NAME##_new(lex, TK_PREFIX##_TK_RPAR);\
    }\
    static PREFIX_NAME##_struct_t *PREFIX_NAME##_ast_default_handler(mln_lex_t *lex, void *data)\
    {\
        return PREFIX_NAME##_new(lex, TK_PREFIX##_TK_AST);\
    }\
    static PREFIX_NAME##_struct_t *PREFIX_NAME##_plus_default_handler(mln_lex_t *lex, void *data)\
    {\
        return PREFIX_NAME##_new(lex, TK_PREFIX##_TK_PLUS);\
    }\
    static PREFIX_NAME##_struct_t *PREFIX_NAME##_comma_default_handler(mln_lex_t *lex, void *data)\
    {\
        return PREFIX_NAME##_new(lex, TK_PREFIX##_TK_COMMA);\
    }\
    static PREFIX_NAME##_struct_t *PREFIX_NAME##_sub_default_handler(mln_lex_t *lex, void *data)\
    {\
        return PREFIX_NAME##_new(lex, TK_PREFIX##_TK_SUB);\
    }\
    static PREFIX_NAME##_struct_t *PREFIX_NAME##_period_default_handler(mln_lex_t *lex, void *data)\
    {\
        return PREFIX_NAME##_new(lex, TK_PREFIX##_TK_PERIOD);\
    }\
    static PREFIX_NAME##_struct_t *PREFIX_NAME##_slash_default_handler(mln_lex_t *lex, void *data)\
    {\
        return PREFIX_NAME##_new(lex, TK_PREFIX##_TK_SLASH);\
    }\
    static PREFIX_NAME##_struct_t *PREFIX_NAME##_colon_default_handler(mln_lex_t *lex, void *data)\
    {\
        return PREFIX_NAME##_new(lex, TK_PREFIX##_TK_COLON);\
    }\
    static PREFIX_NAME##_struct_t *PREFIX_NAME##_semic_default_handler(mln_lex_t *lex, void *data)\
    {\
        return PREFIX_NAME##_new(lex, TK_PREFIX##_TK_SEMIC);\
    }\
    static PREFIX_NAME##_struct_t *PREFIX_NAME##_lagl_default_handler(mln_lex_t *lex, void *data)\
    {\
        return PREFIX_NAME##_new(lex, TK_PREFIX##_TK_LAGL);\
    }\
    static PREFIX_NAME##_struct_t *PREFIX_NAME##_equal_default_handler(mln_lex_t *lex, void *data)\
    {\
        return PREFIX_NAME##_new(lex, TK_PREFIX##_TK_EQUAL);\
    }\
    static PREFIX_NAME##_struct_t *PREFIX_NAME##_ragl_default_handler(mln_lex_t *lex, void *data)\
    {\
        return PREFIX_NAME##_new(lex, TK_PREFIX##_TK_RAGL);\
    }\
    static PREFIX_NAME##_struct_t *PREFIX_NAME##_ques_default_handler(mln_lex_t *lex, void *data)\
    {\
        return PREFIX_NAME##_new(lex, TK_PREFIX##_TK_QUES);\
    }\
    static PREFIX_NAME##_struct_t *PREFIX_NAME##_at_default_handler(mln_lex_t *lex, void *data)\
    {\
        return PREFIX_NAME##_new(lex, TK_PREFIX##_TK_AT);\
    }\
    static PREFIX_NAME##_struct_t *PREFIX_NAME##_lsquar_default_handler(mln_lex_t *lex, void *data)\
    {\
        return PREFIX_NAME##_new(lex, TK_PREFIX##_TK_LSQUAR);\
    }\
    static PREFIX_NAME##_struct_t *PREFIX_NAME##_bslash_default_handler(mln_lex_t *lex, void *data)\
    {\
        return PREFIX_NAME##_new(lex, TK_PREFIX##_TK_BSLASH);\
    }\
    static PREFIX_NAME##_struct_t *PREFIX_NAME##_rsquar_default_handler(mln_lex_t *lex, void *data)\
    {\
        return PREFIX_NAME##_new(lex, TK_PREFIX##_TK_RSQUAR);\
    }\
    static PREFIX_NAME##_struct_t *PREFIX_NAME##_xor_default_handler(mln_lex_t *lex, void *data)\
    {\
        return PREFIX_NAME##_new(lex, TK_PREFIX##_TK_XOR);\
    }\
    static PREFIX_NAME##_struct_t *PREFIX_NAME##_under_default_handler(mln_lex_t *lex, void *data)\
    {\
        return PREFIX_NAME##_new(lex, TK_PREFIX##_TK_UNDER);\
    }\
    static PREFIX_NAME##_struct_t *PREFIX_NAME##_fulstp_default_handler(mln_lex_t *lex, void *data)\
    {\
        return PREFIX_NAME##_new(lex, TK_PREFIX##_TK_FULSTP);\
    }\
    static PREFIX_NAME##_struct_t *PREFIX_NAME##_lbrace_default_handler(mln_lex_t *lex, void *data)\
    {\
        return PREFIX_NAME##_new(lex, TK_PREFIX##_TK_LBRACE);\
    }\
    static PREFIX_NAME##_struct_t *PREFIX_NAME##_vertl_default_handler(mln_lex_t *lex, void *data)\
    {\
        return PREFIX_NAME##_new(lex, TK_PREFIX##_TK_VERTL);\
    }\
    static PREFIX_NAME##_struct_t *PREFIX_NAME##_rbrace_default_handler(mln_lex_t *lex, void *data)\
    {\
        return PREFIX_NAME##_new(lex, TK_PREFIX##_TK_RBRACE);\
    }\
    static PREFIX_NAME##_struct_t *PREFIX_NAME##_dash_default_handler(mln_lex_t *lex, void *data)\
    {\
        return PREFIX_NAME##_new(lex, TK_PREFIX##_TK_DASH);\
    }\
    \
    mln_spechar_t PREFIX_NAME##_handlers[] = {         \
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
    static void PREFIX_NAME##_set_hooks(mln_lex_t *lex)\
    {\
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
    }\
    \
    PREFIX_NAME##_struct_t *PREFIX_NAME##_new(mln_lex_t *lex, enum PREFIX_NAME##_enum type)\
    {\
        PREFIX_NAME##_struct_t *ptr;\
        ptr = (PREFIX_NAME##_struct_t *)malloc(sizeof(PREFIX_NAME##_struct_t));\
        if (ptr == NULL) {\
            lex->error = MLN_LEX_ENMEM;\
            return NULL;\
        }\
        if (type != TK_PREFIX##_TK_EOF) \
            ptr->text = mln_new_string(lex->result_buf);\
        else \
            ptr->text = mln_new_string(MLN_EOF_TEXT);\
        if (ptr->text == NULL) {\
            free(ptr);\
            lex->error = MLN_LEX_ENMEM;\
            return NULL;\
        }\
        ptr->line = lex->line;\
        ptr->type = type;\
        lex->error = MLN_LEX_SUCCEED;\
        lex->result_cur_ptr = lex->result_buf;\
        if (lex->result_cur_ptr != NULL) \
            *(lex->result_cur_ptr) = 0;\
        return ptr;\
    }\
    \
    void PREFIX_NAME##_free(PREFIX_NAME##_struct_t *ptr)\
    {\
        if (ptr == NULL) return ;\
        if (ptr->text != NULL) mln_free_string(ptr->text);\
        free(ptr);\
    }\
    \
    static inline PREFIX_NAME##_struct_t *PREFIX_NAME##_process_keywords(mln_lex_t *lex)\
    {\
        if (lex->keywords == NULL) return PREFIX_NAME##_new(lex, TK_PREFIX##_TK_ID);\
        mln_s32_t i;\
        for (i = 0; lex->keywords[i] != NULL; i++) {\
            if (!strcmp(lex->result_buf, lex->keywords[i])) {\
                return PREFIX_NAME##_new(lex, TK_PREFIX##_TK_KEYWORD_BEGIN+i+1);\
            }\
        }\
        return PREFIX_NAME##_new(lex, TK_PREFIX##_TK_ID);\
    }\
    \
    static inline PREFIX_NAME##_struct_t *PREFIX_NAME##_process_spec_char(mln_lex_t *lex, char c)\
    {\
        mln_s32_t i;\
        mln_s32_t end = sizeof(PREFIX_NAME##_handlers)/sizeof(mln_spechar_t);\
        for (i = 0; i<end; i++) {\
            if (c == PREFIX_NAME##_handlers[i].sc) {\
                return (PREFIX_NAME##_struct_t *)PREFIX_NAME##_handlers[i].handler(lex, PREFIX_NAME##_handlers[i].data);\
            }\
        }\
        lex->error = MLN_LEX_EINVCHAR;\
        return NULL;\
    }\
    \
    PREFIX_NAME##_struct_t *PREFIX_NAME##_token(mln_lex_t *lex) \
    {\
        char c = mln_geta_char(lex);\
        if (c == MLN_ERR) return NULL;\
lp:\
        switch (c) {\
            case MLN_EOF:\
                 return PREFIX_NAME##_new(lex, TK_PREFIX##_TK_EOF);\
            case '\n':\
                 {\
                     while (c == '\n') {\
                         lex->line++;\
                         c = mln_geta_char(lex);\
                         if (c == MLN_ERR) return NULL;\
                     };\
                     goto lp;\
                 }\
            case '\t':\
            case ' ':\
                 {\
                     while (c == ' ' || c == '\t') {\
                         c = mln_geta_char(lex);\
                         if (c == MLN_ERR) return NULL;\
                     }\
                     goto lp;\
                 }\
            default:\
                {\
                    if (mln_isletter(c)) {\
                        while (mln_isletter(c) || isdigit(c)) {\
                            if (mln_puta_char(lex, c) == MLN_ERR) return NULL;\
                            c = mln_geta_char(lex);\
                            if (c == MLN_ERR) return NULL;\
                        }\
                        mln_step_back(lex);\
                        if (lex->result_buf[0] == '_' && lex->result_buf[1] == 0) {\
                            return PREFIX_NAME##_process_spec_char(lex, '_');\
                        }\
                        return PREFIX_NAME##_process_keywords(lex);\
                    } else if (isdigit(c)) {\
                        while ( 1 ) {\
                            if (mln_puta_char(lex, c) == MLN_ERR) return NULL;\
                            c = mln_geta_char(lex);\
                            if (c == MLN_ERR) return NULL;\
                            if (!isdigit(c) && !isalpha(c) && c != '.') {\
                                mln_step_back(lex);\
                                break;\
                            }\
                        }\
                        /*check number*/\
                        mln_s8ptr_t chk = lex->result_buf;\
                        if (*chk == '0') {\
                            if (*(chk+1) == 'x') {\
                                for (chk += 2; chk != lex->result_cur_ptr; chk++) {\
                                    if (!mln_ishex(*chk)) {\
                                        lex->error = MLN_LEX_EINVHEX;\
                                        return NULL;\
                                    }\
                                }\
                                return PREFIX_NAME##_new(lex, TK_PREFIX##_TK_HEX);\
                            } else if (*(chk+1) == 0){\
                                return PREFIX_NAME##_new(lex, TK_PREFIX##_TK_DEC);\
                            } else {\
                                for (chk++; chk != lex->result_cur_ptr; chk++) {\
                                    if (!mln_isoctal(*chk)) {\
                                        mln_s32_t dot_cnt = 0;\
                                        for (; chk != lex->result_cur_ptr; chk++) {\
                                            if (*chk == '.') dot_cnt++;\
                                            if (!isdigit(*chk) && *chk != '.') {\
                                                lex->error = MLN_LEX_EINVOCT;\
                                                return NULL;\
                                            }\
                                        }\
                                        if (dot_cnt > 1) lex->error = MLN_LEX_EINVREAL;\
                                        else if (dot_cnt == 0) lex->error = MLN_LEX_EINVOCT;\
                                        else return PREFIX_NAME##_new(lex, TK_PREFIX##_TK_REAL);\
                                        return NULL;\
                                    }\
                                }\
                                return PREFIX_NAME##_new(lex, TK_PREFIX##_TK_OCT);\
                            }\
                        } else {\
                            for (; chk != lex->result_cur_ptr; chk++) {\
                                if (isdigit(*chk)) continue;\
                                if (*chk == '.') {\
                                    for (chk++; chk != lex->result_cur_ptr; chk++) {\
                                        if (!isdigit(*chk)) {\
                                            lex->error = MLN_LEX_EINVREAL;\
                                            return NULL;\
                                        }\
                                    }\
                                    return PREFIX_NAME##_new(lex, TK_PREFIX##_TK_REAL);\
                                }\
                                lex->error = MLN_LEX_EINVDEC;\
                                return NULL;\
                            }\
                            return PREFIX_NAME##_new(lex, TK_PREFIX##_TK_DEC);\
                        }\
                    } else {\
                        if (mln_puta_char(lex, c) == MLN_ERR) return NULL;\
                        PREFIX_NAME##_struct_t *tmp = PREFIX_NAME##_process_spec_char(lex, c);\
                        if (tmp == NULL) break; \
                        return tmp;\
                    }\
                }\
        }\
        return NULL;\
    }\
    \
    void *PREFIX_NAME##_lex_dup(void *ptr)\
    {\
        if (ptr == NULL) return NULL;\
        PREFIX_NAME##_struct_t *src = (PREFIX_NAME##_struct_t *)ptr;\
        PREFIX_NAME##_struct_t *dest = (PREFIX_NAME##_struct_t *)malloc(sizeof(PREFIX_NAME##_struct_t));\
        if (dest == NULL) return NULL;\
        dest->text = mln_dup_string(src->text);\
        if (dest->text == NULL) {\
            free(dest);\
            return NULL;\
        }\
        dest->line = src->line;\
        dest->type = src->type;\
        return (void *)dest;\
    }

#define MLN_LEX_INIT_WITH_HOOKS(PREFIX_NAME,lex_ptr,attr_ptr) \
    (lex_ptr) = mln_lex_init((attr_ptr));\
    if ((lex_ptr) != NULL && (attr_ptr)->hooks != NULL) PREFIX_NAME##_set_hooks((lex_ptr));

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
 *     MLN_LEX_INIT_WITH_HOOKS(mln_test_lex, lex, &attr);
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

