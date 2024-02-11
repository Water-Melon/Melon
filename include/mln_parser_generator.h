
/*
 * Copyright (C) Niklaus F.Schen.
 */

#ifndef __MLN_PARSERT_GENERATOR_H
#define __MLN_PARSERT_GENERATOR_H

#include "mln_lex.h"
#include "mln_types.h"
#include "mln_string.h"
#include "mln_log.h"
#include "mln_hash.h"
#include "mln_rbtree.h"
#include "mln_queue.h"
#include "mln_stack.h"
#include "mln_alloc.h"
#include "mln_func.h"

#define M_PG_DFL_HASHLEN 31
#define M_PG_ERROR 0
#define M_PG_SHIFT 1
#define M_PG_REDUCE 2
#define M_PG_ACCEPT 3
/*for parser*/
#define M_P_QLEN 16
#define M_P_CUR_STACK 0
#define M_P_OLD_STACK 1
#define M_P_ERR_STACK 2
#define M_P_ERR_DEL 0
#define M_P_ERR_MOD 1

typedef void (*nonterm_free)(void *);
typedef struct mln_pg_rule_s mln_pg_rule_t;
typedef struct mln_pg_token_s mln_pg_token_t;
typedef struct mln_factor_s mln_factor_t;
typedef int (*semantic_func)(mln_factor_t *left, mln_factor_t **right, void *data);

typedef struct {
    char                     *production;
    semantic_func             func;
} mln_production_t;

struct mln_pg_rule_s {
    semantic_func             func;
    mln_pg_token_t           *left;
    mln_pg_token_t          **rights;
    mln_u32_t                 nr_right;
};

struct mln_pg_token_s {
    mln_string_t             *token;
    mln_rbtree_t             *first_set;
    mln_rbtree_t             *follow_set;
    mln_u32_t                *right_rule_index;
    mln_u32_t                *left_rule_index;
    int                       type;
    mln_u32_t                 is_nonterminal:1;
    mln_u32_t                 is_nullable:1;
};

typedef struct mln_pg_item_s {
    struct mln_pg_item_s     *prev;
    struct mln_pg_item_s     *next;
    mln_rbtree_t             *lookahead_set;
    mln_pg_token_t           *read;
    mln_sauto_t               goto_id;
    mln_pg_rule_t            *rule;
    mln_u32_t                 pos;
} mln_pg_item_t;

typedef struct mln_pg_state_s {
    mln_sauto_t               id;
    mln_pg_token_t           *input;
    mln_pg_item_t            *head;
    mln_pg_item_t            *tail;
    struct mln_pg_state_s    *prev;
    struct mln_pg_state_s    *next;
    struct mln_pg_state_s    *q_prev;
    struct mln_pg_state_s    *q_next;
    mln_u64_t                 nr_item;
} mln_pg_state_t;

typedef struct {
    mln_u64_t                 index;
    mln_u32_t                 type;
    mln_u32_t                 rule_index;
    mln_u32_t                 nr_args;
    mln_s32_t                 left_type;
} mln_shift_t;

typedef struct {
    mln_shift_t             **tbl;
    mln_sauto_t               nr_state;
    int                       type_val;
} mln_pg_shift_tbl_t;

struct mln_pg_calc_info_s {
    mln_rbtree_t             *tree;
    mln_pg_state_t           *head;
    mln_pg_state_t           *tail;
    mln_sauto_t               id_counter;
    mln_pg_token_t           *first_input;
    mln_pg_rule_t            *rule;
    mln_u32_t                 nr_rule;
};

enum factor_data_type {
    M_P_TERM,
    M_P_NONTERM
};

struct mln_factor_s {
    void                     *data;
    enum factor_data_type     data_type;
    nonterm_free              nonterm_free_handler;
    mln_sauto_t               cur_state;
    int                       token_type;
    mln_u32_t                 line;
    mln_string_t             *file;
};

typedef struct {
    mln_stack_t              *cur_stack;
    mln_factor_t             *cur_la; /*token type pointer*/
    mln_sauto_t               cur_state;
    mln_sauto_t               cur_reduce;
    mln_stack_t              *old_stack;
    mln_factor_t             *old_la;/*token type pointer*/
    mln_sauto_t               old_state;
    mln_sauto_t               old_reduce;
    mln_stack_t              *err_stack;
    mln_factor_t             *err_la;/*token type pointer*/
    mln_sauto_t               err_state;
    mln_sauto_t               err_reduce;
    mln_queue_t              *cur_queue;
    mln_queue_t              *err_queue;
} mln_parser_t;

struct mln_parse_attr {
    mln_alloc_t              *pool;
    mln_production_t         *prod_tbl;
    mln_lex_t                *lex;
    void                     *pg_data;
    void                     *udata;
};

struct mln_sys_parse_attr {
    mln_alloc_t              *pool;
    mln_parser_t             *p;
    mln_lex_t                *lex;
    mln_pg_shift_tbl_t       *tbl;
    mln_production_t         *prod_tbl;
    void                     *udata;
    int                       type;
    int                       done;
};

struct mln_err_queue_s {
    mln_alloc_t              *pool;
    mln_queue_t              *q;
    mln_uauto_t               index;
    mln_uauto_t               pos;
    int                       opr;
    int                       ctype;
};

extern void mln_pg_output_token(mln_rbtree_t *tree, mln_pg_rule_t *r, mln_u32_t nr_rule);
extern mln_pg_token_t *mln_pg_token_new(mln_string_t *token, mln_u32_t nr_rule);
extern void mln_pg_token_free(void *token);
extern mln_pg_item_t *mln_pg_item_new(void);
extern void mln_pg_item_free(mln_pg_item_t *item);
extern mln_pg_state_t *mln_pg_state_new(void);
extern void mln_pg_state_free(mln_pg_state_t *s);
extern int mln_pg_token_rbtree_cmp(const void *data1, const void *data2);
extern mln_u64_t mln_pg_map_hash_calc(mln_hash_t *h, void *key);
extern int mln_pg_map_hash_cmp(mln_hash_t *h, void *key1, void *key2);
extern void mln_pg_map_hash_free(void *data);
extern int
mln_pg_calc_info_init(struct mln_pg_calc_info_s *pci, \
                      mln_pg_token_t *first_input, \
                      mln_pg_rule_t *rule, \
                      mln_u32_t nr_rule);
extern void mln_pg_calc_info_destroy(struct mln_pg_calc_info_s *pci);
extern int mln_pg_calc_nullable(volatile int *map, mln_pg_rule_t *r, mln_u32_t nr_rule);
extern int mln_pg_calc_first(volatile int *map, mln_pg_rule_t *r, mln_u32_t nr_rule);
extern int mln_pg_calc_follow(volatile int *map, mln_pg_rule_t *r, mln_u32_t nr_rule);
extern int mln_pg_closure(mln_pg_state_t *s, mln_pg_rule_t *r, mln_u32_t nr_rule);
extern int mln_pg_goto(struct mln_pg_calc_info_s *pci);
extern void mln_pg_output_state(mln_pg_state_t *s);


#define MLN_DECLARE_PARSER_GENERATOR(SCOPE,PREFIX_NAME,TK_PREFIX,...); \
MLN_DEFINE_TOKEN_TYPE_AND_STRUCT(SCOPE,PREFIX_NAME,TK_PREFIX,## __VA_ARGS__);\
struct PREFIX_NAME##_preprocess_attr {\
    mln_hash_t               *map_tbl;/*output*/\
    mln_rbtree_t             *token_tree;/*output*/\
    mln_production_t         *prod_tbl;/*input*/\
    PREFIX_NAME##_type_t     *type_array;/*input*/\
    mln_pg_rule_t            *rule_tbl;/*output but allocated in caller*/\
    mln_u32_t                 nr_prod;/*input*/\
    mln_u32_t                 nr_type;/*input*/\
    int                       type_val;\
    int                       terminal_type_val;\
    mln_string_t             *env;\
};\
struct PREFIX_NAME##_reduce_info {\
    mln_shift_t              *sh;\
    mln_pg_item_t            *item;\
    mln_pg_rule_t            *rule;\
    mln_pg_state_t           *state;\
    int                      *failed;\
};\
\
SCOPE int PREFIX_NAME##_reduce_iterate_handler(mln_rbtree_node_t *node, void *udata);\
SCOPE mln_pg_shift_tbl_t *PREFIX_NAME##_build_shift_tbl(struct mln_pg_calc_info_s *pci, \
                                                        struct PREFIX_NAME##_preprocess_attr *attr);\
SCOPE void PREFIX_NAME##_pg_data_free(void *pg_data);\
SCOPE mln_pg_token_t * \
PREFIX_NAME##_pg_create_token(struct PREFIX_NAME##_preprocess_attr *attr, PREFIX_NAME##_struct_t *pgs, int index);\
SCOPE int \
PREFIX_NAME##_pg_process_right(struct PREFIX_NAME##_preprocess_attr *attr, mln_lex_t *lex, int index, int cnt);\
SCOPE inline int \
PREFIX_NAME##_pg_process_token(struct PREFIX_NAME##_preprocess_attr *attr, mln_lex_t *lex, mln_production_t *prod);\
SCOPE int PREFIX_NAME##_preprocess(struct PREFIX_NAME##_preprocess_attr *attr);\
SCOPE void PREFIX_NAME##_preprocess_attr_free(struct PREFIX_NAME##_preprocess_attr *attr);\
SCOPE void *PREFIX_NAME##_parser_generate(mln_production_t *prod_tbl, mln_u32_t nr_prod, mln_string_t *env);\
SCOPE mln_factor_t *PREFIX_NAME##_factor_init(void *data, \
                                              enum factor_data_type data_type, \
                                              int token_type, \
                                              mln_sauto_t cur_state, \
                                              mln_u32_t line, \
                                              mln_string_t *file);\
SCOPE void PREFIX_NAME##_factor_destroy(void *ptr);\
SCOPE void *PREFIX_NAME##_factor_copy(void *ptr, void *data);\
SCOPE mln_parser_t *PREFIX_NAME##_parser_init(void);\
SCOPE void PREFIX_NAME##_parser_destroy(mln_parser_t *p);\
SCOPE void *PREFIX_NAME##_parse(struct mln_parse_attr *pattr);\
SCOPE int PREFIX_NAME##_sys_parse(struct mln_sys_parse_attr *spattr);\
SCOPE int PREFIX_NAME##_reduce_launcher(mln_stack_t *st, \
                                        mln_sauto_t *state, \
                                        mln_production_t *prod_tbl, \
                                        mln_shift_t *sh, \
                                        void *udata, \
                                        int type);\
SCOPE int PREFIX_NAME##_shift(struct mln_sys_parse_attr *spattr, \
                              mln_stack_t **stack, \
                              mln_factor_t **la, \
                              mln_sauto_t *state, \
                              mln_sauto_t *is_reduce, \
                              mln_shift_t *sh);\
SCOPE int PREFIX_NAME##_err_process(struct mln_sys_parse_attr *spattr, int opr);\
SCOPE int PREFIX_NAME##_err_dup(struct mln_sys_parse_attr *spattr, mln_uauto_t pos, int ctype, int opr);\
SCOPE int PREFIX_NAME##_err_dup_iterate_handler(void *q_node, void *udata);\
SCOPE int PREFIX_NAME##_err_recover(struct mln_sys_parse_attr *spattr, mln_uauto_t pos, int ctype, int opr);



#define MLN_DEFINE_PARSER_GENERATOR(SCOPE,PREFIX_NAME,TK_PREFIX,...); \
MLN_DEFINE_TOKEN(SCOPE, PREFIX_NAME,TK_PREFIX,## __VA_ARGS__);\
\
MLN_FUNC(SCOPE, int, PREFIX_NAME##_reduce_iterate_handler, \
         (mln_rbtree_node_t *node, void *udata), (node, udata), \
{\
    mln_pg_token_t *tk = (mln_pg_token_t *)mln_rbtree_node_data_get(node);\
    struct PREFIX_NAME##_reduce_info *info = (struct PREFIX_NAME##_reduce_info *)udata;\
    mln_shift_t *sh = info->sh;\
    int index = tk->type;\
    if (sh[index].type != M_PG_ERROR && \
        (sh[index].type != M_PG_REDUCE || sh[index].index != info->item->rule - info->rule))\
    {\
        if (sh[index].type == M_PG_ACCEPT || sh[index].type == M_PG_REDUCE) {\
            mln_log(error, "State:%d token[%S] Reduce-Reduce conflict.\n", \
                    info->state->id, tk->token);\
        } else {\
            mln_log(error, "State:%d token[%S] Shift-Reduce conflict.\n", \
                    info->state->id, tk->token);\
        }\
        *(info->failed) = 1;\
    }\
    sh[index].index = info->item->rule - info->rule;\
    sh[index].type = M_PG_REDUCE;\
    sh[index].rule_index = info->item->rule - info->rule;\
    sh[index].nr_args = info->item->rule->nr_right;\
    sh[index].left_type = info->item->rule->left->type;\
    return 0;\
})\
\
MLN_FUNC(SCOPE, mln_pg_shift_tbl_t *, PREFIX_NAME##_build_shift_tbl, \
         (struct mln_pg_calc_info_s *pci, struct PREFIX_NAME##_preprocess_attr *attr), \
         (pci, attr), \
{\
    mln_pg_shift_tbl_t *stbl = (mln_pg_shift_tbl_t *)malloc(sizeof(mln_pg_shift_tbl_t));\
    if (stbl == NULL) {\
        mln_log(error, "No memory.\n");\
        return NULL;\
    }\
    stbl->nr_state = pci->id_counter;\
    stbl->type_val = attr->terminal_type_val;\
\
    stbl->tbl = (mln_shift_t **)calloc(stbl->nr_state, sizeof(mln_shift_t *));\
    if (stbl->tbl == NULL) {\
        mln_log(error, "No memory.\n");\
        PREFIX_NAME##_pg_data_free((void *)stbl);\
        return NULL;\
    }\
\
    mln_pg_state_t *s;\
    mln_shift_t *sh;\
    mln_pg_item_t *it;\
    int index, type, failed = 0;\
    struct PREFIX_NAME##_reduce_info info;\
    for (s = pci->head; s != NULL; s = s->next) {\
        stbl->tbl[s->id] = (mln_shift_t *)calloc(attr->type_val+1, sizeof(mln_shift_t));\
        if (stbl->tbl[s->id] == NULL) {\
            mln_log(error, "No memory.\n");\
            PREFIX_NAME##_pg_data_free((void *)stbl);\
            return NULL;\
        }\
        sh = stbl->tbl[s->id];\
        for (it = s->head; it != NULL; it = it->next) {\
            if (it->pos == it->rule->nr_right) {\
                info.sh = sh;\
                info.item = it;\
                info.rule = attr->rule_tbl;\
                info.state = s;\
                info.failed = &failed;\
                if (mln_rbtree_iterate(it->lookahead_set, PREFIX_NAME##_reduce_iterate_handler, &info) < 0) {\
                    PREFIX_NAME##_pg_data_free((void *)stbl);\
                    return NULL;\
                }\
            } else {\
                index = (it->rule->rights[it->pos])->type;\
                if (index == TK_PREFIX##_TK_EOF)\
                    type = M_PG_ACCEPT;\
                else\
                    type = M_PG_SHIFT;\
                if (sh[index].type != M_PG_ERROR && (sh[index].type != type || sh[index].index != it->goto_id)) {\
                    if (sh[index].type == M_PG_ACCEPT || sh[index].type == M_PG_REDUCE) {\
                        mln_log(error, "State:%l token:[%S] Shift-Reduce conflict.\n", \
                                s->id, (it->rule->rights[it->pos])->token);\
                    } else {\
                        mln_log(error, "State:%l token:[%S] Shift-Shift conflict.\n", \
                                s->id, (it->rule->rights[it->pos])->token);\
                    }\
                    failed = 1;\
                }\
                sh[index].index = it->goto_id;\
                sh[index].type = type;\
                sh[index].rule_index = it->rule - attr->rule_tbl;\
                sh[index].nr_args = it->rule->nr_right;\
                sh[index].left_type = it->rule->left->type;\
            }\
        }\
    }\
    if (failed) {\
        PREFIX_NAME##_pg_data_free((void *)stbl);\
        return NULL;\
    }\
\
    return stbl;\
})\
\
MLN_FUNC_VOID(SCOPE, void, PREFIX_NAME##_pg_data_free, (void *pg_data), (pg_data), {\
    mln_pg_shift_tbl_t *tbl = (mln_pg_shift_tbl_t *)pg_data;\
    if (tbl == NULL) return ;\
    if (tbl->tbl != NULL) {\
        mln_sauto_t i;\
        for (i = 0; i < tbl->nr_state; ++i) {\
            if (tbl->tbl[i] != NULL)\
                free(tbl->tbl[i]);\
        }\
        free(tbl->tbl);\
    }\
    free(tbl);\
})\
\
MLN_FUNC(SCOPE, mln_pg_token_t *, PREFIX_NAME##_pg_create_token, \
         (struct PREFIX_NAME##_preprocess_attr *attr, PREFIX_NAME##_struct_t *pgs, int index), \
         (attr, pgs, index), \
{\
    int *type_val;\
    if ((type_val = (int *)mln_hash_search(attr->map_tbl, pgs->text->data)) == NULL) {\
        mln_s8ptr_t new_str = (mln_s8ptr_t)malloc(pgs->text->len+1);\
        if (new_str == NULL) {\
            mln_log(error, "No memory.\n");\
            return NULL;\
        }\
        memcpy(new_str, pgs->text->data, pgs->text->len);\
        new_str[pgs->text->len] = 0;\
        type_val = (int *)malloc(sizeof(int));\
        if (type_val == NULL) {\
            mln_log(error, "No memory.\n");\
            free(new_str);\
            return NULL;\
        }\
        *type_val = ++(attr->type_val);\
        if (mln_hash_insert(attr->map_tbl, new_str, type_val) < 0) {\
            mln_log(error, "No memory.\n");\
            free(type_val);\
            free(new_str);\
            return NULL;\
        }\
    }\
\
    mln_pg_token_t *tk, token;\
    token.type = *type_val;\
    mln_rbtree_node_t *rn = mln_rbtree_search(attr->token_tree, &token);\
    if (mln_rbtree_null(rn, attr->token_tree)) {\
        tk = mln_pg_token_new(pgs->text, attr->nr_prod);\
        if (tk == NULL) {\
            mln_log(error, "No memory.\n");\
            return NULL;\
        }\
        tk->type = *type_val;\
        if (index >= 0) {\
            tk->right_rule_index[index] = 1;\
        } else {\
            tk->is_nonterminal = 1;\
        }\
        rn = mln_rbtree_node_new(attr->token_tree, tk);\
        if (rn == NULL) {\
            mln_log(error, "No memory.\n");\
            mln_pg_token_free(tk);\
            return NULL;\
        }\
        mln_rbtree_insert(attr->token_tree, rn);\
    } else {\
        tk = (mln_pg_token_t *)mln_rbtree_node_data_get(rn);\
        if (index >= 0) {\
            tk->right_rule_index[index] = 1;\
        } else {\
            tk->is_nonterminal = 1;\
        }\
    }\
    return tk;\
})\
\
MLN_FUNC(SCOPE, int, PREFIX_NAME##_pg_process_right, \
         (struct PREFIX_NAME##_preprocess_attr *attr, mln_lex_t *lex, int index, int cnt), \
         (attr, lex, index, cnt), \
{\
    PREFIX_NAME##_struct_t *pgs;\
    if ((pgs = PREFIX_NAME##_token(lex)) == NULL) {\
        mln_log(error, "Get token failed. %s\n", mln_lex_strerror(lex));\
        return -1;\
    }\
    if (pgs->type == TK_PREFIX##_TK_EOF) {\
        PREFIX_NAME##_free(pgs);\
        mln_pg_rule_t *r = attr->rule_tbl + index;\
        if (cnt) {\
            r->rights = (mln_pg_token_t **)malloc(cnt*sizeof(mln_pg_token_t *));\
            if (r->rights == NULL) return -1;\
        }\
        r->nr_right = cnt;\
        return 0;\
    }\
    if (PREFIX_NAME##_pg_process_right(attr, lex, index, cnt+1) < 0) {\
        PREFIX_NAME##_free(pgs);\
        return -1;\
    }\
    mln_pg_rule_t *r = attr->rule_tbl + index;\
    mln_pg_token_t *tk = PREFIX_NAME##_pg_create_token(attr, pgs, index);\
    PREFIX_NAME##_free(pgs);\
    if (tk == NULL) {\
        free(r->rights);\
        r->rights = NULL;\
        return -1;\
    }\
    r->rights[cnt] = tk;\
    return 0;\
})\
\
MLN_FUNC(SCOPE inline, int, PREFIX_NAME##_pg_process_token, \
         (struct PREFIX_NAME##_preprocess_attr *attr, mln_lex_t *lex, mln_production_t *prod), \
         (attr, lex, prod), \
{\
    int index = prod - attr->prod_tbl;\
    PREFIX_NAME##_struct_t *pgs;\
    if ((pgs = PREFIX_NAME##_token(lex)) == NULL) {/*get left*/\
        mln_log(error, "Get token failed. %s\n", mln_lex_strerror(lex));\
        return -1;\
    }\
    if (pgs->type != TK_PREFIX##_TK_ID) {\
        mln_log(error, "Production '%u' error.\n", index);\
        PREFIX_NAME##_free(pgs);\
        return -1;\
    }\
\
    mln_pg_token_t *tk = PREFIX_NAME##_pg_create_token(attr, pgs, -1);\
    PREFIX_NAME##_free(pgs);\
    if (tk == NULL) return -1;\
    tk->left_rule_index[index] = 1;\
\
    mln_pg_rule_t *r = attr->rule_tbl + index;\
    r->func = prod->func;\
    r->left = tk;\
\
    if ((pgs = PREFIX_NAME##_token(lex)) == NULL) {/*get ':'*/\
        mln_log(error, "Get token failed. %s\n", mln_lex_strerror(lex));\
        return -1;\
    }\
    if (pgs->type != TK_PREFIX##_TK_COLON) {\
        PREFIX_NAME##_free(pgs);\
        mln_log(error, "Production '%u' error.\n", index);\
        return -1;\
    }\
    PREFIX_NAME##_free(pgs);\
    if (PREFIX_NAME##_pg_process_right(attr, lex, index, 0) < 0) {\
        return -1;\
    }\
    return 0;\
})\
\
MLN_FUNC(SCOPE, int, PREFIX_NAME##_preprocess, (struct PREFIX_NAME##_preprocess_attr *attr), (attr), {\
    /*Init hash table*/\
    attr->map_tbl = mln_hash_new_fast(mln_pg_map_hash_calc, \
                                      mln_pg_map_hash_cmp, \
                                      mln_pg_map_hash_free, \
                                      mln_pg_map_hash_free, \
                                      M_PG_DFL_HASHLEN, \
                                      1, \
                                      0);\
    if (attr->map_tbl == NULL) {\
        mln_log(error, "No memory.\n");\
        return -1;\
    }\
\
    mln_size_t str_len;\
    mln_s8ptr_t new_str;\
    int *new_val;\
    PREFIX_NAME##_type_t *tp = attr->type_array;\
    PREFIX_NAME##_type_t *tpend = attr->type_array + attr->nr_type;\
    for (; tp < tpend; ++tp) {\
        attr->type_val = tp->type;\
        attr->terminal_type_val = tp->type;\
        if (mln_hash_search(attr->map_tbl, tp->type_str) != NULL)\
            continue;\
        str_len = strlen(tp->type_str);\
        new_str = (mln_s8ptr_t)malloc(str_len+1);\
        if (new_str == NULL) {\
            mln_log(error, "No memory.\n");\
            goto err1;\
        }\
        memcpy(new_str, tp->type_str, str_len);\
        new_str[str_len] = 0;\
        new_val = (int *)malloc(sizeof(int));\
        if (new_val == NULL) {\
            mln_log(error, "No memory.\n");\
            free(new_str);\
            goto err1;\
        }\
        *new_val = tp->type;\
        if (mln_hash_insert(attr->map_tbl, new_str, new_val) < 0) {\
            mln_log(error, "No memory.\n");\
            free(new_val);\
            free(new_str);\
            goto err1;\
        }\
    }\
\
    /*Init token rbtree*/\
    struct mln_rbtree_attr rbattr;\
    rbattr.pool = NULL;\
    rbattr.pool_alloc = NULL;\
    rbattr.pool_free = NULL;\
    rbattr.cmp = mln_pg_token_rbtree_cmp;\
    rbattr.data_free = mln_pg_token_free;\
    attr->token_tree = mln_rbtree_new(&rbattr);\
    if (attr->token_tree == NULL) {\
        mln_log(error, "No memory.\n");\
        goto err1;\
    }\
\
    int ret;\
    mln_lex_t *lex = NULL;\
    mln_alloc_t *pool;\
    struct mln_lex_attr lattr;\
    mln_string_t tmp;\
    mln_production_t *prod = attr->prod_tbl;\
    mln_production_t *prodend = attr->prod_tbl + attr->nr_prod;\
    if ((pool = mln_alloc_init(NULL)) == NULL) {\
        mln_log(error, "No memory.\n");\
        goto err2;\
    }\
    for (; prod < prodend; ++prod) {\
        mln_string_nset(&tmp, prod->production, strlen(prod->production));\
        lattr.pool = pool;\
        lattr.keywords = NULL;\
        lattr.hooks = NULL;\
        lattr.preprocess = 0;\
        lattr.type = M_INPUT_T_BUF;\
        lattr.data = &tmp;\
        lattr.env = attr->env;\
        mln_lex_init_with_hooks(PREFIX_NAME, lex, &lattr);\
        if (lex == NULL) {\
            mln_log(error, "No memory.\n");\
            goto err3;\
        }\
        ret = PREFIX_NAME##_pg_process_token(attr, lex, prod);\
        mln_lex_destroy(lex);\
        if (ret < 0) goto err2;\
    }\
    mln_alloc_destroy(pool);\
\
    return 0;\
\
err3:\
    mln_alloc_destroy(pool);\
\
err2:\
    mln_rbtree_free(attr->token_tree);\
err1:\
    mln_hash_free(attr->map_tbl, M_HASH_F_KV);\
    return -1;\
})\
\
MLN_FUNC_VOID(SCOPE, void, PREFIX_NAME##_preprocess_attr_free, \
              (struct PREFIX_NAME##_preprocess_attr *attr), (attr), \
{\
    if (attr->map_tbl != NULL) \
        mln_hash_free(attr->map_tbl, M_HASH_F_KV);\
    if (attr->token_tree != NULL) \
        mln_rbtree_free(attr->token_tree);\
    if (attr->rule_tbl != NULL) {\
        mln_u32_t i;\
        for (i = 0; i < attr->nr_prod; ++i) {\
            if ((attr->rule_tbl)[i].rights != NULL) \
                free((attr->rule_tbl)[i].rights);\
        }\
        free(attr->rule_tbl);\
    }\
})\
\
MLN_FUNC(SCOPE, void *, PREFIX_NAME##_parser_generate, \
         (mln_production_t *prod_tbl, mln_u32_t nr_prod, mln_string_t *env), \
         (prod_tbl, nr_prod, env), \
{\
    struct PREFIX_NAME##_preprocess_attr pattr;\
    pattr.map_tbl = NULL;\
    pattr.token_tree = NULL;\
    pattr.prod_tbl = prod_tbl;\
    pattr.type_array = PREFIX_NAME##_token_type_array;\
    pattr.rule_tbl = (mln_pg_rule_t *)calloc(nr_prod, sizeof(mln_pg_rule_t));\
    if (pattr.rule_tbl == NULL) {\
        mln_log(error, "No memory.\n");\
        return NULL;\
    }\
    pattr.nr_prod = nr_prod;\
    pattr.nr_type = sizeof(PREFIX_NAME##_token_type_array)/sizeof(PREFIX_NAME##_type_t);\
    pattr.type_val = -1;\
    pattr.terminal_type_val = -1;\
    pattr.env = env;\
    if (PREFIX_NAME##_preprocess(&pattr) < 0) {\
        PREFIX_NAME##_preprocess_attr_free(&pattr);\
        return NULL;\
    }\
\
    volatile int *map = (int *)malloc(nr_prod * sizeof(int));\
    if (map == NULL) {\
        mln_log(error, "No memory.\n");\
        PREFIX_NAME##_preprocess_attr_free(&pattr);\
        return NULL;\
    }\
    if (mln_pg_calc_nullable(map, pattr.rule_tbl, nr_prod) < 0) {\
        free((void *)map);\
        PREFIX_NAME##_preprocess_attr_free(&pattr);\
        return NULL;\
    }\
    if (mln_pg_calc_first(map, pattr.rule_tbl, nr_prod) < 0) {\
        free((void *)map);\
        PREFIX_NAME##_preprocess_attr_free(&pattr);\
        return NULL;\
    }\
    if (mln_pg_calc_follow(map, pattr.rule_tbl, nr_prod) < 0) {\
        free((void *)map);\
        PREFIX_NAME##_preprocess_attr_free(&pattr);\
        return NULL;\
    }\
    free((void *)map);\
\
    mln_pg_token_t first_input = {NULL, NULL, NULL, NULL, NULL, TK_PREFIX##_TK_EOF, 0, 1};\
    struct mln_pg_calc_info_s pci;\
    if (mln_pg_calc_info_init(&pci, &first_input, pattr.rule_tbl, nr_prod) < 0) {\
        mln_log(error, "No memory.\n");\
        PREFIX_NAME##_preprocess_attr_free(&pattr);\
        return NULL;\
    }\
    if (mln_pg_goto(&pci) < 0) {\
        mln_pg_calc_info_destroy(&pci);\
        PREFIX_NAME##_preprocess_attr_free(&pattr);\
        return NULL;\
    }\
    mln_pg_shift_tbl_t *shift_tbl = PREFIX_NAME##_build_shift_tbl(&pci, &pattr);\
    mln_pg_calc_info_destroy(&pci);\
    PREFIX_NAME##_preprocess_attr_free(&pattr);\
    return (void *)shift_tbl;\
})\
\
MLN_FUNC(SCOPE, mln_factor_t *, PREFIX_NAME##_factor_init, \
         (void *data, enum factor_data_type data_type, int token_type, \
          mln_sauto_t cur_state, mln_u32_t line, mln_string_t *file), \
         (data, data_type, token_type, cur_state, line, file), \
{\
    mln_factor_t *f = (mln_factor_t *)malloc(sizeof(mln_factor_t));\
    if (f == NULL) return NULL;\
    f->data = data;\
    f->data_type = data_type;\
    f->nonterm_free_handler = NULL;\
    f->cur_state = cur_state;\
    f->token_type = token_type;\
    f->line = line;\
    if (file == NULL) {\
        f->file = NULL;\
    } else {\
        f->file = mln_string_ref(file);\
    }\
    return f;\
})\
\
MLN_FUNC_VOID(SCOPE, void, PREFIX_NAME##_factor_destroy, (void *ptr), (ptr), {\
    if (ptr == NULL) return;\
    mln_factor_t *f = (mln_factor_t *)ptr;\
    if (f->data_type == M_P_TERM) {\
        if (f->data != NULL) \
            PREFIX_NAME##_free((PREFIX_NAME##_struct_t *)(f->data));\
    } else {\
        if (f->data != NULL && f->nonterm_free_handler != NULL) \
            f->nonterm_free_handler(f->data);\
    }\
    if (f->file != NULL) mln_string_free(f->file);\
    free(f);\
})\
\
MLN_FUNC(SCOPE, void *, PREFIX_NAME##_factor_copy, (void *ptr, void *data), (ptr, data), {\
    if (ptr == NULL || data == NULL) return NULL;\
    mln_alloc_t *pool = (mln_alloc_t *)data;\
    mln_factor_t *src = (mln_factor_t *)ptr;\
    mln_factor_t *dest = (mln_factor_t *)malloc(sizeof(mln_factor_t));\
    if (dest == NULL) return NULL;\
    dest->data = NULL;\
    if (src->data != NULL) {\
        if (src->data_type == M_P_TERM) {\
            dest->data = (void *)PREFIX_NAME##_lex_dup(pool, src->data);\
            if (dest->data == NULL) {\
                free(dest);\
                return NULL;\
            }\
        }\
    }\
    dest->data_type = src->data_type;\
    dest->nonterm_free_handler = src->nonterm_free_handler;\
    dest->token_type = src->token_type;\
    dest->cur_state = src->cur_state;\
    dest->line = src->line;\
    if (src->file == NULL) {\
        dest->file = NULL;\
    } else {\
        dest->file = mln_string_ref(src->file);\
    }\
    return (void *)dest;\
})\
\
MLN_FUNC(SCOPE, mln_parser_t *, PREFIX_NAME##_parser_init, (void), (), {\
    mln_parser_t *p = (mln_parser_t *)malloc(sizeof(mln_parser_t));\
    if (p == NULL) return NULL;\
    p->cur_stack = mln_stack_init(PREFIX_NAME##_factor_destroy, PREFIX_NAME##_factor_copy);\
    if (p->cur_stack == NULL) {\
        goto err1;\
    }\
    p->cur_la = NULL;\
    p->cur_state = 0;\
    p->cur_reduce = 0;\
    p->old_stack = mln_stack_init(PREFIX_NAME##_factor_destroy, PREFIX_NAME##_factor_copy);\
    if (p->old_stack == NULL) {\
        goto err2;\
    }\
    p->old_la = NULL;\
    p->old_state = 0;\
    p->old_reduce = 0;\
\
    p->err_stack = NULL;\
    p->err_la = NULL;\
    p->err_state = 0;\
    p->err_reduce = 0;\
    p->err_queue = NULL;\
    p->cur_queue = mln_queue_init(M_P_QLEN, PREFIX_NAME##_factor_destroy);\
    if (p->cur_queue == NULL) {\
        goto err3;\
    }\
    return p;\
\
err3:\
    mln_stack_destroy(p->old_stack);\
err2:\
    mln_stack_destroy(p->cur_stack);\
err1:\
    free(p);\
    return NULL;\
})\
\
MLN_FUNC_VOID(SCOPE, void, PREFIX_NAME##_parser_destroy, (mln_parser_t *p), (p), {\
    if (p == NULL) return;\
    if (p->cur_stack != NULL) \
        mln_stack_destroy(p->cur_stack);\
    if (p->cur_la != NULL) \
        PREFIX_NAME##_factor_destroy(p->cur_la);\
    if (p->old_stack != NULL) \
        mln_stack_destroy(p->old_stack);\
    if (p->old_la != NULL) \
        PREFIX_NAME##_factor_destroy(p->old_la);\
    if (p->err_stack != NULL) \
        mln_stack_destroy(p->err_stack);\
    if (p->err_la != NULL) \
        PREFIX_NAME##_factor_destroy(p->err_la);\
    if (p->cur_queue != NULL) \
        mln_queue_destroy(p->cur_queue);\
    if (p->err_queue != NULL) \
        mln_queue_destroy(p->err_queue);\
    free(p);\
})\
\
/*====================parse=======================*/\
\
MLN_FUNC(SCOPE, void *, PREFIX_NAME##_parse, (struct mln_parse_attr *pattr), (pattr), {\
    mln_pg_shift_tbl_t *tbl = (mln_pg_shift_tbl_t *)(pattr->pg_data);\
    mln_parser_t *p = PREFIX_NAME##_parser_init();\
    if (p == NULL) {\
        mln_log(error, "No memory.\n");\
        return NULL;\
    }\
    struct mln_sys_parse_attr spattr;\
    spattr.pool = pattr->pool;\
    spattr.p = p;\
    spattr.lex = pattr->lex;\
    spattr.tbl = tbl;\
    spattr.prod_tbl = pattr->prod_tbl;\
    spattr.udata = pattr->udata;\
    spattr.type = M_P_CUR_STACK;\
    spattr.done = 0;\
    mln_u8ptr_t ret = NULL;\
    int rc = PREFIX_NAME##_sys_parse(&spattr);\
    if (rc >= 0) {\
        mln_factor_t *top = (mln_factor_t *)mln_stack_top(spattr.p->old_stack);\
        if (top != NULL) {\
            ret = (mln_u8ptr_t)(top->data);\
            top->data = NULL;\
        }\
    }\
    PREFIX_NAME##_parser_destroy(p);\
    return ret;\
})\
\
MLN_FUNC(SCOPE, int, PREFIX_NAME##_sys_parse, (struct mln_sys_parse_attr *spattr), (spattr), {\
    mln_stack_t **stack;\
    mln_factor_t **la;\
    mln_sauto_t *state, *is_reduce;\
    mln_parser_t *p = spattr->p;\
    int type = spattr->type;\
    if (type == M_P_CUR_STACK) {\
        stack = &(p->cur_stack);\
        la = &(p->cur_la);\
        state = &(p->cur_state);\
        is_reduce = &(p->cur_reduce);\
    } else if (type == M_P_OLD_STACK) {\
        stack = &(p->old_stack);\
        la = &(p->old_la);\
        state = &(p->old_state);\
        is_reduce = &(p->old_reduce);\
    } else {\
        stack = &(p->err_stack);\
        la = &(p->err_la);\
        state = &(p->err_state);\
        is_reduce = &(p->err_reduce);\
    }\
    if (mln_stack_empty(*stack)) {\
        if (type == M_P_CUR_STACK) {\
            PREFIX_NAME##_struct_t *token = PREFIX_NAME##_token(spattr->lex);\
            if (token == NULL) {\
                mln_log(error, "Get token error. %s\n", mln_lex_strerror(spattr->lex));\
                return -1;\
            }\
            *la = PREFIX_NAME##_factor_init(token, M_P_TERM, token->type, *state, token->line, token->file);\
            if (*la == NULL) {\
                mln_log(error, "No memory.\n");\
                PREFIX_NAME##_free(token);\
                return -1;\
            }\
        } else if (type == M_P_OLD_STACK) {\
            *la = mln_queue_get(p->cur_queue);\
            mln_queue_remove(p->cur_queue);\
        } else {\
            *la = mln_queue_get(p->err_queue);\
            mln_queue_remove(p->err_queue);\
        }\
    }\
    mln_shift_t *sh;\
    mln_u64_t state_type, col_index;\
    int failed = 0, ret, is_recovered = 0;\
    mln_sauto_t failed_type = -1, failedline = -1;\
    mln_factor_t *top = NULL;\
    while (1) {\
        if (*is_reduce) {\
            top = (mln_factor_t *)mln_stack_top(*stack);\
            col_index = top->token_type;\
        } else {\
            col_index = (*la)->token_type;\
        }\
        sh = &(spattr->tbl->tbl)[*state][col_index];\
        state_type = sh->type;\
        if (state_type == M_PG_SHIFT) {\
            is_recovered = 0;\
            if ((ret = PREFIX_NAME##_shift(spattr, stack, la, state, is_reduce, sh)) > 0) { \
                break;\
            } else if (ret < 0) {\
                return -1;\
            }\
        } else if (state_type == M_PG_REDUCE) {\
            is_recovered = 0;\
            if (PREFIX_NAME##_reduce_launcher(*stack, \
                                              state, \
                                              spattr->prod_tbl, \
                                              sh, \
                                              spattr->udata, \
                                              spattr->type) < 0)\
            {\
                return -1;\
            }\
            *is_reduce = 1;\
        } else if (state_type == M_PG_ERROR) {\
            if (is_recovered) return -1;\
            if (type == M_P_CUR_STACK) {\
                if (failed && failed_type == (*la)->token_type && failedline == (*la)->line) return -1;\
                failed = 1;\
                failed_type = (*la)->token_type;\
                failedline = (*la)->line;\
                mln_s8ptr_t name = NULL;\
                if ((*la)->token_type != TK_PREFIX##_TK_EOF && (*la)->data != NULL) \
                    name = (mln_s8ptr_t)(((PREFIX_NAME##_struct_t *)((*la)->data))->text->data);\
                if ((*la)->file == NULL) {\
                    mln_log(none, "line %d: Parse Error: Illegal token%s%s%s\n", \
                            *is_reduce?top->line:(*la)->line, \
                            name==NULL? ".": " nearby '", \
                            name==NULL? " ": name, \
                            name==NULL? " ": "'.");\
                } else {\
                    mln_log(none, "%s:%d: Parse Error: Illegal token%s%s%s\n", \
                            (char *)((*la)->file->data), \
                            *is_reduce?top->line:(*la)->line, \
                            name==NULL? ".": " nearby '", \
                            name==NULL? " ": name, \
                            name==NULL? " ": "'.");\
                }\
                if ((*la)->token_type == TK_PREFIX##_TK_EOF) break;\
                ret = PREFIX_NAME##_err_process(spattr, M_P_ERR_MOD);\
                if (ret >= 0) {\
                    is_recovered = 1;\
                    continue;\
                }\
                ret = PREFIX_NAME##_err_process(spattr, M_P_ERR_DEL);\
                if (ret < 0) break;\
                is_recovered = 1;\
            } else {\
                return -1;\
            }\
        } else {/*M_PG_ACCEPT*/\
            is_recovered = 0;\
            if (type == M_P_CUR_STACK) {\
                (*la)->cur_state = *state;\
                if (mln_queue_append(p->cur_queue, (void *)(*la)) < 0) {\
                    mln_log(error, "Queue is full.\n");\
                    return -1;\
                }\
                *la = NULL;\
                struct mln_sys_parse_attr old_spattr;\
                memcpy(&old_spattr, spattr, sizeof(old_spattr));\
                old_spattr.type = M_P_OLD_STACK;\
                old_spattr.done = 1;\
                PREFIX_NAME##_sys_parse(&old_spattr);\
            }\
            break;\
        }\
    }\
    return failed == 0? 0: -1;\
})\
\
MLN_FUNC(SCOPE, int, PREFIX_NAME##_err_process, \
         (struct mln_sys_parse_attr *spattr, int opr), (spattr, opr), \
{\
    int ctype, max = spattr->tbl->type_val+1;\
    mln_factor_t *f;\
    mln_sauto_t pos;\
    mln_uauto_t nr_element = mln_queue_element(spattr->p->cur_queue);\
    if (!nr_element) return -1;\
    for (pos = nr_element-1; pos >= 0; --pos) {\
        for (ctype = 0; ctype < max; ++ctype) {\
            f = mln_queue_search(spattr->p->cur_queue, pos);\
            if (f == NULL) {\
                mln_log(error, "Queue messed up.\n");\
                abort();\
            }\
            if (ctype == f->token_type) continue;\
            if (PREFIX_NAME##_err_dup(spattr, pos, ctype, opr) < 0) {\
                return -1;\
            }\
            struct mln_sys_parse_attr err_spattr;\
            memcpy(&err_spattr, spattr, sizeof(err_spattr));\
            err_spattr.type = M_P_ERR_STACK;\
            err_spattr.done = 0;\
            if (PREFIX_NAME##_sys_parse(&err_spattr) >= 0) {\
                if (PREFIX_NAME##_err_recover(spattr, pos, ctype, opr) < 0) {\
                    return -1;\
                }\
                return 0;\
            }\
            if (opr == M_P_ERR_DEL) break;\
        }\
    }\
    return -1;\
})\
\
MLN_FUNC(SCOPE, int, PREFIX_NAME##_err_recover, \
         (struct mln_sys_parse_attr *spattr, mln_uauto_t pos, int ctype, int opr), \
         (spattr, pos, ctype, opr), \
{\
    mln_parser_t *p = spattr->p;\
    mln_stack_destroy(p->cur_stack);\
    p->cur_stack = mln_stack_dup(p->err_stack, spattr->pool);\
    if (p->cur_stack == NULL) {\
        mln_log(error, "No memory.\n");\
        return -1;\
    }\
    p->cur_stack->free_handler = PREFIX_NAME##_factor_destroy;\
    p->cur_stack->copy_handler = PREFIX_NAME##_factor_copy;\
    if (p->cur_la == NULL) {\
        PREFIX_NAME##_struct_t *token = PREFIX_NAME##_token(spattr->lex);\
        if (token == NULL) {\
            mln_log(error, "Get token error. %s\n", mln_lex_strerror(spattr->lex));\
            return -1;\
        }\
        p->cur_la = PREFIX_NAME##_factor_init(token, M_P_TERM, token->type, p->cur_state, token->line, token->file);\
        if (p->cur_la == NULL) {\
            mln_log(error, "No memory.\n");\
            PREFIX_NAME##_free(token);\
            return -1;\
        }\
    }\
    p->cur_state = p->err_state;\
    p->cur_reduce = p->err_reduce;\
\
    if (opr == M_P_ERR_MOD) {\
        mln_factor_t *f = mln_queue_search(p->cur_queue, pos);\
        if (f == NULL) {\
            mln_log(error, "Current stack messed up.\n");\
            abort();\
        }\
        f->token_type = ctype;\
    } else {/*M_P_ERR_DEL*/\
        mln_queue_free_index(p->cur_queue, pos);\
    }\
    return 0;\
})\
\
MLN_FUNC(SCOPE, int, PREFIX_NAME##_err_dup, \
         (struct mln_sys_parse_attr *spattr, mln_uauto_t pos, int ctype, int opr), \
         (spattr, pos, ctype, opr), \
{\
    mln_parser_t *p = spattr->p;\
\
    if (p->err_stack != NULL) {\
        mln_stack_destroy(p->err_stack);\
        p->err_stack = NULL;\
    }\
    if (p->err_la != NULL) {\
        PREFIX_NAME##_factor_destroy((void *)(p->err_la));\
        p->err_la = NULL;\
    }\
    if (p->err_queue != NULL) {\
        mln_queue_destroy(p->err_queue);\
        p->err_queue = NULL;\
    }\
\
    p->err_stack = mln_stack_dup(p->old_stack, spattr->pool);\
    if (p->err_stack == NULL) {\
        mln_log(error, "No memory.\n");\
        return -1;\
    }\
    p->err_la = PREFIX_NAME##_factor_copy(p->old_la, spattr->pool);\
    if (p->err_la == NULL && p->old_la != NULL) {\
        mln_log(error, "No memory.\n");\
        mln_stack_destroy(p->err_stack);\
        p->err_stack = NULL;\
        return -1;\
    }\
    p->err_state = p->old_state;\
    p->err_reduce = p->old_reduce;\
    p->err_queue = mln_queue_init(M_P_QLEN, PREFIX_NAME##_factor_destroy);\
    if (p->err_queue == NULL) {\
        mln_log(error, "No memory.\n");\
        mln_stack_destroy(p->err_stack);\
        p->err_stack = NULL;\
        PREFIX_NAME##_factor_destroy((void *)(p->err_la));\
        p->err_la = NULL;\
        return -1;\
    }\
    struct mln_err_queue_s eq;\
    eq.pool = spattr->pool;\
    eq.q = p->err_queue;\
    eq.index = 0;\
    eq.pos = pos;\
    eq.opr = opr;\
    eq.ctype = ctype;\
    if (mln_queue_iterate(p->cur_queue, PREFIX_NAME##_err_dup_iterate_handler, (void *)&eq) < 0) {\
        mln_stack_destroy(p->err_stack);\
        p->err_stack = NULL;\
        PREFIX_NAME##_factor_destroy((void *)(p->err_la));\
        p->err_la = NULL;\
        mln_queue_destroy(p->err_queue);\
        p->err_queue = NULL;\
        return -1;\
    }\
    return 0;\
})\
\
MLN_FUNC(SCOPE, int, PREFIX_NAME##_err_dup_iterate_handler, \
         (void *q_node, void *udata), (q_node, udata), \
{\
    struct mln_err_queue_s *eq = (struct mln_err_queue_s *)udata;\
    if (eq->opr == M_P_ERR_DEL && eq->index == eq->pos) {\
        ++(eq->index);\
        return 0;\
    }\
    mln_factor_t *f = PREFIX_NAME##_factor_copy((mln_factor_t *)q_node, eq->pool);\
    if (f == NULL) {\
        mln_log(error, "No memory.\n");\
        return -1;\
    }\
    if (eq->opr == M_P_ERR_MOD && eq->index == eq->pos) {\
        f->token_type = eq->ctype;\
    }\
    if (mln_queue_append(eq->q, (void *)f) < 0) {\
        mln_log(error, "Queue is full.\n");\
        PREFIX_NAME##_factor_destroy((void *)f);\
        return -1;\
    }\
    ++(eq->index);\
    return 0;\
})\
\
MLN_FUNC(SCOPE, int, PREFIX_NAME##_reduce_launcher, \
         (mln_stack_t *st, mln_sauto_t *state, mln_production_t *prod_tbl, \
          mln_shift_t *sh, void *udata, int type), \
         (st, state, prod_tbl, sh, udata, type), \
{\
    mln_factor_t **rights;\
    rights = (mln_factor_t **)calloc(sh->nr_args, sizeof(mln_factor_t *));\
    if (rights == NULL && sh->nr_args) {\
        mln_log(error, "No memory.\n");\
        return -1;\
    }\
    mln_factor_t *left = PREFIX_NAME##_factor_init(NULL, M_P_NONTERM, sh->left_type, *state, 0, NULL);\
    if (left == NULL) {\
        mln_log(error, "No memory.\n");\
        free(rights);\
        return -1;\
    }\
\
    mln_u32_t i, line = 0;\
    mln_string_t *file = NULL;\
    mln_factor_t *right;\
    for (i = 0; i < sh->nr_args; ++i) {\
        right = (mln_factor_t *)mln_stack_pop(st);\
        if (right == NULL) {\
            mln_log(error, "Fatal error. State shift table messed up.\n");\
            abort();\
        }\
        rights[sh->nr_args-1-i] = right;\
        if (right->line > line) {\
            line = right->line;\
            file = right->file;\
        }\
        *state = right->cur_state;\
    }\
    left->line = line;\
    left->cur_state = *state;\
    if (file != NULL) {\
        left->file = mln_string_ref(file);\
    }\
\
    mln_production_t *pp = &prod_tbl[sh->rule_index];\
    int ret = 0;\
    if (type == M_P_OLD_STACK && pp->func != NULL) {\
        ret = pp->func(left, rights, udata);\
    }\
    for (i = 0; i < sh->nr_args; ++i) {\
        PREFIX_NAME##_factor_destroy((void *)rights[i]);\
    }\
    free(rights);\
    if (ret < 0 || mln_stack_push(st, (void *)left) < 0) {\
        mln_log(error, "No memory.\n");\
        PREFIX_NAME##_factor_destroy((void *)left);\
        return -1;\
    }\
    return 0;\
})\
\
MLN_FUNC(SCOPE, int, PREFIX_NAME##_shift, \
         (struct mln_sys_parse_attr *spattr, mln_stack_t **stack, mln_factor_t **la, \
          mln_sauto_t *state, mln_sauto_t *is_reduce, mln_shift_t *sh), \
         (spattr, stack, la, state, is_reduce, sh), \
{\
    if (*is_reduce == 0) {\
        (*la)->cur_state = *state;\
        if (mln_stack_push(*stack, (*la)) < 0) {\
            mln_log(error, "No memory.\n");\
            return -1;\
        }\
        if (spattr->type == M_P_CUR_STACK) {\
            mln_factor_t *new_f = PREFIX_NAME##_factor_copy(*la, spattr->pool);\
            *la = NULL;\
            if (new_f == NULL) {\
                mln_log(error, "No memory.\n");\
                return -1;\
            }\
            if (mln_queue_append(spattr->p->cur_queue, (void *)new_f) < 0) {\
                mln_log(error, "Queue is full.\n");\
                PREFIX_NAME##_factor_destroy((void *)new_f);\
                return -1;\
            }\
            if (mln_queue_full(spattr->p->cur_queue)) {\
                struct mln_sys_parse_attr old_spattr;\
                memcpy(&old_spattr, spattr, sizeof(old_spattr));\
                old_spattr.type = M_P_OLD_STACK;\
                old_spattr.done = 0;\
                PREFIX_NAME##_sys_parse(&old_spattr);\
            }\
            PREFIX_NAME##_struct_t *token = PREFIX_NAME##_token(spattr->lex);\
            if (token == NULL) {\
                mln_log(error, "Get token error. %s\n", mln_lex_strerror(spattr->lex));\
                return -1;\
            }\
            *la = PREFIX_NAME##_factor_init(token, M_P_TERM, token->type, *state, token->line, token->file);\
            if (*la == NULL) {\
                mln_log(error, "No memory.\n");\
                PREFIX_NAME##_free(token);\
                return -1;\
            }\
        } else if (spattr->type == M_P_OLD_STACK) {\
            *la = NULL;\
            if (mln_queue_empty(spattr->p->cur_queue)) \
                return 1;\
            *la = (mln_factor_t *)mln_queue_get(spattr->p->cur_queue);\
            mln_queue_remove(spattr->p->cur_queue);\
        } else {\
            *la = NULL;\
            if (mln_queue_empty(spattr->p->err_queue)) \
                return 1;\
            *la = (mln_factor_t *)mln_queue_get(spattr->p->err_queue);\
            mln_queue_remove(spattr->p->err_queue);\
        }\
    }\
    *state = sh->index;\
    if (spattr->type == M_P_OLD_STACK && *is_reduce == 0 && spattr->done == 0) \
        return 1;\
    *is_reduce = 0;\
    return 0;\
})

#endif

