
/*
 * Copyright (C) Niklaus F.Schen.
 */
#ifndef __MLN_LANG_AST_H
#define __MLN_LANG_AST_H
#include "mln_types.h"
#include "mln_alloc.h"
#include "mln_string.h"

typedef struct mln_lang_stm_s              mln_lang_stm_t;
typedef struct mln_lang_funcdef_s          mln_lang_funcdef_t;
typedef struct mln_lang_set_s              mln_lang_set_t;
typedef struct mln_lang_setstm_s           mln_lang_setstm_t;
typedef struct mln_lang_block_s            mln_lang_block_t;
typedef struct mln_lang_while_s            mln_lang_while_t;
typedef struct mln_lang_switch_s           mln_lang_switch_t;
typedef struct mln_lang_switchstm_s        mln_lang_switchstm_t;
typedef struct mln_lang_for_s              mln_lang_for_t;
typedef struct mln_lang_if_s               mln_lang_if_t;
typedef struct mln_lang_exp_s              mln_lang_exp_t;
typedef struct mln_lang_assign_s           mln_lang_assign_t;
typedef struct mln_lang_assign_tmp_s       mln_lang_assign_tmp_t;
typedef struct mln_lang_logiclow_s         mln_lang_logiclow_t;
typedef struct mln_lang_logiclow_tmp_s     mln_lang_logiclow_tmp_t;
typedef struct mln_lang_logichigh_s        mln_lang_logichigh_t;
typedef struct mln_lang_logichigh_tmp_s    mln_lang_logichigh_tmp_t;
typedef struct mln_lang_relativelow_s      mln_lang_relativelow_t;
typedef struct mln_lang_relativelow_tmp_s  mln_lang_relativelow_tmp_t;
typedef struct mln_lang_relativehigh_s     mln_lang_relativehigh_t;
typedef struct mln_lang_relativehigh_tmp_s mln_lang_relativehigh_tmp_t;
typedef struct mln_lang_move_s             mln_lang_move_t;
typedef struct mln_lang_move_tmp_s         mln_lang_move_tmp_t;
typedef struct mln_lang_addsub_s           mln_lang_addsub_t;
typedef struct mln_lang_addsub_tmp_s       mln_lang_addsub_tmp_t;
typedef struct mln_lang_muldiv_s           mln_lang_muldiv_t;
typedef struct mln_lang_muldiv_tmp_s       mln_lang_muldiv_tmp_t;
typedef struct mln_lang_not_s              mln_lang_not_t;
typedef struct mln_lang_suffix_s           mln_lang_suffix_t;
typedef struct mln_lang_suffix_tmp_s       mln_lang_suffix_tmp_t;
typedef struct mln_lang_locate_s           mln_lang_locate_t;
typedef struct mln_lang_locate_tmp_s       mln_lang_locate_tmp_t;
typedef struct mln_lang_spec_s             mln_lang_spec_t;
typedef struct mln_lang_factor_s           mln_lang_factor_t;
typedef struct mln_lang_elemlist_s         mln_lang_elemlist_t;

typedef enum {
    M_STM_BLOCK = 0,
    M_STM_FUNC,
    M_STM_SET,
    M_STM_LABEL,
    M_STM_SWITCH,
    M_STM_WHILE,
    M_STM_FOR
} mln_lang_stm_type_t;

struct mln_lang_stm_s {
    mln_string_t                    *file;
    mln_u64_t                        line;
    mln_lang_stm_type_t              type;
    union {
        mln_lang_block_t        *block;
        mln_lang_funcdef_t      *func;
        mln_lang_set_t          *setdef;
        mln_lang_switch_t       *sw;
        mln_lang_while_t        *w;
        mln_lang_for_t          *f;
        mln_string_t            *pos;
    } data;
    struct mln_lang_stm_s           *next;
    void                            *jump;
    int                              jump_type;
};

struct mln_lang_funcdef_s {
    mln_string_t                    *file;
    mln_u64_t                        line;
    mln_string_t                    *name;
    mln_lang_exp_t                  *args;
    mln_lang_stm_t                  *stm;
    mln_lang_exp_t                  *closure;
};

struct mln_lang_set_s {
    mln_string_t                    *file;
    mln_u64_t                        line;
    mln_string_t                    *name;
    mln_lang_setstm_t               *stm;
};

typedef enum {
    M_SETSTM_VAR = 0,
    M_SETSTM_FUNC
} mln_lang_setstm_type_t;

struct mln_lang_setstm_s {
    mln_string_t                    *file;
    mln_u64_t                        line;
    mln_lang_setstm_type_t           type;
    union {
        mln_string_t            *var;
        mln_lang_funcdef_t      *func;
    } data;
    mln_lang_setstm_t               *next;
};

typedef enum {
    M_BLOCK_EXP = 0,
    M_BLOCK_STM,
    M_BLOCK_CONTINUE,
    M_BLOCK_BREAK,
    M_BLOCK_RETURN,
    M_BLOCK_GOTO,
    M_BLOCK_IF
} mln_lang_block_type_t;

struct mln_lang_block_s {
    mln_string_t                    *file;
    mln_u64_t                        line;
    mln_lang_block_type_t            type;
    union {
        mln_lang_exp_t          *exp;
        mln_lang_stm_t          *stm;
        mln_string_t            *pos;
        mln_lang_if_t           *i;
    } data;
    void                            *jump;
    int                              jump_type;
};

struct mln_lang_switch_s {
    mln_string_t                    *file;
    mln_u64_t                        line;
    mln_lang_exp_t                  *condition;
    mln_lang_switchstm_t            *switchstm;
};

struct mln_lang_switchstm_s {
    mln_string_t                    *file;
    mln_u64_t                        line;
    mln_lang_factor_t               *factor;
    mln_lang_stm_t                  *stm;
    mln_lang_switchstm_t            *next;
};

struct mln_lang_while_s {
    mln_string_t                    *file;
    mln_u64_t                        line;
    mln_lang_exp_t                  *condition;
    mln_lang_block_t                *blockstm;
};

struct mln_lang_for_s {
    mln_string_t                    *file;
    mln_u64_t                        line;
    mln_lang_exp_t                  *init_exp;
    mln_lang_exp_t                  *condition;
    mln_lang_exp_t                  *mod_exp;
    mln_lang_block_t                *blockstm;
};

struct mln_lang_if_s {
    mln_string_t                    *file;
    mln_u64_t                        line;
    mln_lang_exp_t                  *condition;
    mln_lang_block_t                *blockstm;
    mln_lang_block_t                *elsestm;
};

struct mln_lang_exp_s {
    mln_string_t                    *file;
    mln_u64_t                        line;
    mln_lang_assign_t               *assign;
    mln_lang_exp_t                  *next;
    void                            *jump;
    int                              type;
};

typedef enum mln_lang_assign_op_e {
    M_ASSIGN_NONE = 0,
    M_ASSIGN_EQUAL,
    M_ASSIGN_PLUSEQ,
    M_ASSIGN_SUBEQ,
    M_ASSIGN_LMOVEQ,
    M_ASSIGN_RMOVEQ,
    M_ASSIGN_MULEQ,
    M_ASSIGN_DIVEQ,
    M_ASSIGN_OREQ,
    M_ASSIGN_ANDEQ,
    M_ASSIGN_XOREQ,
    M_ASSIGN_MODEQ
} mln_lang_assign_op_t;

struct mln_lang_assign_s {
    mln_string_t                    *file;
    mln_u64_t                        line;
    mln_lang_logiclow_t             *left;
    mln_lang_assign_op_t             op;
    mln_lang_assign_t               *right;
    void                            *jump;
    int                              type;
};

struct mln_lang_assign_tmp_s {
    mln_lang_assign_op_t             op;
    mln_lang_assign_t               *assign;
};

typedef enum mln_lang_logiclow_op_e {
    M_LOGICLOW_NONE = 0,
    M_LOGICLOW_OR,
    M_LOGICLOW_AND
}mln_lang_logiclow_op_t;

struct mln_lang_logiclow_s {
    mln_string_t                    *file;
    mln_u64_t                        line;
    mln_lang_logichigh_t            *left;
    mln_lang_logiclow_op_t           op;
    mln_lang_logiclow_t             *right;
    void                            *jump;
    int                              type;
};

struct mln_lang_logiclow_tmp_s {
    mln_lang_logiclow_op_t           op;
    mln_lang_logiclow_t             *logiclow;
};

typedef enum mln_lang_logichigh_op_e {
    M_LOGICHIGH_NONE = 0,
    M_LOGICHIGH_OR,
    M_LOGICHIGH_AND,
    M_LOGICHIGH_XOR
} mln_lang_logichigh_op_t;

struct mln_lang_logichigh_s {
    mln_string_t                    *file;
    mln_u64_t                        line;
    mln_lang_relativelow_t          *left;
    mln_lang_logichigh_op_t          op;
    mln_lang_logichigh_t            *right;
    void                            *jump;
    int                              type;
};

struct mln_lang_logichigh_tmp_s {
    mln_lang_logichigh_op_t          op;
    mln_lang_logichigh_t            *logichigh;
};

typedef enum mln_lang_relativelow_op_e {
    M_RELATIVELOW_NONE = 0,
    M_RELATIVELOW_EQUAL,
    M_RELATIVELOW_NEQUAL
} mln_lang_relativelow_op_t;

struct mln_lang_relativelow_s {
    mln_string_t                    *file;
    mln_u64_t                        line;
    mln_lang_relativehigh_t         *left;
    mln_lang_relativelow_op_t        op;
    mln_lang_relativelow_t          *right;
    void                            *jump;
    int                              type;
};

struct mln_lang_relativelow_tmp_s {
    mln_lang_relativelow_op_t        op;
    mln_lang_relativelow_t          *relativelow;
};

typedef enum mln_lang_relativehigh_op_e {
    M_RELATIVEHIGH_NONE = 0,
    M_RELATIVEHIGH_LESS,
    M_RELATIVEHIGH_LESSEQ,
    M_RELATIVEHIGH_GREATER,
    M_RELATIVEHIGH_GREATEREQ
} mln_lang_relativehigh_op_t;

struct mln_lang_relativehigh_s {
    mln_string_t                    *file;
    mln_u64_t                        line;
    mln_lang_move_t                 *left;
    mln_lang_relativehigh_op_t       op;
    mln_lang_relativehigh_t         *right;
    void                            *jump;
    int                              type;
};

struct mln_lang_relativehigh_tmp_s {
    mln_lang_relativehigh_op_t       op;
    mln_lang_relativehigh_t         *relativehigh;
};

typedef enum mln_lang_move_op_e {
    M_MOVE_NONE = 0,
    M_MOVE_LMOVE,
    M_MOVE_RMOVE
} mln_lang_move_op_t;

struct mln_lang_move_s {
    mln_string_t                    *file;
    mln_u64_t                        line;
    mln_lang_addsub_t               *left;
    mln_lang_move_op_t               op;
    mln_lang_move_t                 *right;
    void                            *jump;
    int                              type;
};

struct mln_lang_move_tmp_s {
    mln_lang_move_op_t               op;
    mln_lang_move_t                 *move;
};

typedef enum mln_lang_addsub_op_e {
    M_ADDSUB_NONE = 0,
    M_ADDSUB_PLUS,
    M_ADDSUB_SUB
} mln_lang_addsub_op_t;

struct mln_lang_addsub_s {
    mln_string_t                    *file;
    mln_u64_t                        line;
    mln_lang_muldiv_t               *left;
    mln_lang_addsub_op_t             op;
    mln_lang_addsub_t               *right;
    void                            *jump;
    int                              type;
};

struct mln_lang_addsub_tmp_s {
    mln_lang_addsub_op_t             op;
    mln_lang_addsub_t               *addsub;
};

typedef enum mln_lang_muldiv_op_e {
    M_MULDIV_NONE = 0,
    M_MULDIV_MUL,
    M_MULDIV_DIV,
    M_MULDIV_MOD
} mln_lang_muldiv_op_t;

struct mln_lang_muldiv_s {
    mln_string_t                    *file;
    mln_u64_t                        line;
    mln_lang_not_t                  *left;
    mln_lang_muldiv_op_t             op;
    mln_lang_muldiv_t               *right;
    void                            *jump;
    int                              type;
};

struct mln_lang_muldiv_tmp_s {
    mln_lang_muldiv_op_t             op;
    mln_lang_muldiv_t               *muldiv;
};

typedef enum mln_lang_not_op_e {
    M_NOT_NONE = 0,
    M_NOT_NOT
} mln_lang_not_op_t;

struct mln_lang_not_s {
    mln_string_t                    *file;
    mln_u64_t                        line;
    mln_lang_not_op_t                 op;
    union {
        mln_lang_not_t          *not;
        mln_lang_suffix_t       *suffix;
    } right;
    void                            *jump;
    int                              type;
};

typedef enum mln_lang_suffix_op_e {
    M_SUFFIX_NONE = 0,
    M_SUFFIX_INC,
    M_SUFFIX_DEC
} mln_lang_suffix_op_t;

struct mln_lang_suffix_s {
    mln_string_t                    *file;
    mln_u64_t                        line;
    mln_lang_locate_t               *left;
    mln_lang_suffix_op_t             op;
    void                            *jump;
    int                              type;
};

struct mln_lang_suffix_tmp_s {
    mln_lang_suffix_op_t             op;
};

typedef enum mln_lang_locate_op_e {
    M_LOCATE_NONE = 0,
    M_LOCATE_INDEX,
    M_LOCATE_PROPERTY,
    M_LOCATE_FUNC
} mln_lang_locate_op_t;

struct mln_lang_locate_s {
    mln_string_t                    *file;
    mln_u64_t                        line;
    mln_lang_spec_t                 *left;
    mln_lang_locate_op_t             op;
    union {
        mln_lang_exp_t          *exp;
        mln_string_t            *id;
    } right;
    mln_lang_locate_t               *next;
    void                            *jump;
    int                              type;
};

struct mln_lang_locate_tmp_s {
    mln_lang_locate_op_t             op;
    union {
        mln_lang_exp_t          *exp;
        mln_string_t            *id;
    } locate;
    mln_lang_locate_tmp_t           *next;
};

typedef enum mln_lang_spec_op_e {
    M_SPEC_NEGATIVE = 0,
    M_SPEC_REVERSE,
    M_SPEC_REFER,
    M_SPEC_INC,
    M_SPEC_DEC,
    M_SPEC_NEW,
    M_SPEC_PARENTH,
    M_SPEC_FACTOR
} mln_lang_spec_op_t;

struct mln_lang_spec_s {
    mln_string_t                    *file;
    mln_u64_t                        line;
    mln_lang_spec_op_t               op;
    union {
        mln_lang_exp_t          *exp;
        mln_lang_factor_t       *factor;
        mln_lang_spec_t         *spec;
        mln_string_t            *set_name;
    } data;
};

typedef enum {
    M_FACTOR_BOOL = 0,
    M_FACTOR_STRING,
    M_FACTOR_INT,
    M_FACTOR_REAL,
    M_FACTOR_ARRAY,
    M_FACTOR_ID,
    M_FACTOR_NIL
} mln_lang_factor_type_t;

struct mln_lang_factor_s {
    mln_string_t                    *file;
    mln_u64_t                        line;
    mln_lang_factor_type_t           type;
    union {
        mln_u8_t                 b;
        mln_u8ptr_t             *ptr;
        mln_string_t            *s_id;
        mln_s64_t                i;
        double                   f;
        mln_lang_elemlist_t     *array;
    } data;
};

struct mln_lang_elemlist_s {
    mln_string_t                    *file;
    mln_u64_t                        line;
    mln_lang_assign_t               *key;
    mln_lang_assign_t               *val;
    mln_lang_elemlist_t             *next;
};

extern int mln_lang_ast_file_open(mln_string_t *file_path);
extern void *mln_lang_ast_parser_generate(void);
extern void mln_lang_ast_parser_destroy(void *data);
extern void *
mln_lang_ast_generate(mln_alloc_t *pool, void *state_tbl, mln_string_t *data, mln_u32_t data_type) __NONNULL3(1,2,3);
extern void mln_lang_ast_free(void *ast);

#endif
