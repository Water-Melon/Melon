
/*
 * Copyright (C) Niklaus F.Schen.
 */
#include <fcntl.h>
#include "mln_lang_ast.h"
#include "mln_parser_generator.h"
#include "mln_path.h"
#include "mln_func.h"

MLN_DECLARE_PARSER_GENERATOR(static, \
                             mln_lang, \
                             LANG, \
                             LANG_TK_IF, \
                             LANG_TK_ELSE, \
                             LANG_TK_WHILE, \
                             LANG_TK_FOR, \
                             LANG_TK_CONTINUE, \
                             LANG_TK_BREAK, \
                             LANG_TK_RETURN, \
                             LANG_TK_GOTO, \
                             LANG_TK_NIL, \
                             LANG_TK_TRUE, \
                             LANG_TK_FALSE, \
                             LANG_TK_SWITCH, \
                             LANG_TK_CASE, \
                             LANG_TK_DEFAULT, \
                             LANG_TK_FI, \
                             LANG_TK_STRING, \
                             LANG_TK_PLUSEQ, \
                             LANG_TK_SUBEQ, \
                             LANG_TK_LMOVEQ, \
                             LANG_TK_RMOVEQ, \
                             LANG_TK_MULEQ, \
                             LANG_TK_DIVEQ, \
                             LANG_TK_OREQ, \
                             LANG_TK_ANDEQ, \
                             LANG_TK_ANTIEQ, \
                             LANG_TK_XOREQ, \
                             LANG_TK_MODEQ, \
                             LANG_TK_LOWOR, \
                             LANG_TK_LOWAND, \
                             LANG_TK_DEQUAL, \
                             LANG_TK_NONEQUAL, \
                             LANG_TK_LESSEQ, \
                             LANG_TK_GREATEREQ, \
                             LANG_TK_INC, \
                             LANG_TK_DECR, \
                             LANG_TK_LMOVE, \
                             LANG_TK_RMOVE);
MLN_DEFINE_PARSER_GENERATOR(static, \
                            mln_lang, \
                            LANG, \
                            {LANG_TK_IF, "LANG_TK_IF"}, \
                            {LANG_TK_ELSE, "LANG_TK_ELSE"}, \
                            {LANG_TK_WHILE, "LANG_TK_WHILE"}, \
                            {LANG_TK_FOR, "LANG_TK_FOR"}, \
                            {LANG_TK_CONTINUE, "LANG_TK_CONTINUE"}, \
                            {LANG_TK_BREAK, "LANG_TK_BREAK"}, \
                            {LANG_TK_RETURN, "LANG_TK_RETURN"}, \
                            {LANG_TK_GOTO, "LANG_TK_GOTO"}, \
                            {LANG_TK_NIL, "LANG_TK_NIL"}, \
                            {LANG_TK_TRUE, "LANG_TK_TRUE"}, \
                            {LANG_TK_FALSE, "LANG_TK_FALSE"}, \
                            {LANG_TK_SWITCH, "LANG_TK_SWITCH"}, \
                            {LANG_TK_CASE, "LANG_TK_CASE"}, \
                            {LANG_TK_DEFAULT, "LANG_TK_DEFAULT"}, \
                            {LANG_TK_FI, "LANG_TK_FI"}, \
                            {LANG_TK_STRING, "LANG_TK_STRING"}, \
                            {LANG_TK_PLUSEQ, "LANG_TK_PLUSEQ"}, \
                            {LANG_TK_SUBEQ, "LANG_TK_SUBEQ"}, \
                            {LANG_TK_LMOVEQ, "LANG_TK_LMOVEQ"}, \
                            {LANG_TK_RMOVEQ, "LANG_TK_RMOVEQ"}, \
                            {LANG_TK_MULEQ, "LANG_TK_MULEQ"}, \
                            {LANG_TK_DIVEQ, "LANG_TK_DIVEQ"}, \
                            {LANG_TK_OREQ, "LANG_TK_OREQ"}, \
                            {LANG_TK_ANDEQ, "LANG_TK_ANDEQ"}, \
                            {LANG_TK_ANTIEQ, "LANG_TK_ANTIEQ"}, \
                            {LANG_TK_XOREQ, "LANG_TK_XOREQ"}, \
                            {LANG_TK_MODEQ, "LANG_TK_MODEQ"}, \
                            {LANG_TK_LOWOR, "LANG_TK_LOWOR"}, \
                            {LANG_TK_LOWAND, "LANG_TK_LOWAND"}, \
                            {LANG_TK_DEQUAL, "LANG_TK_DEQUAL"}, \
                            {LANG_TK_NONEQUAL, "LANG_TK_NONEQUAL"}, \
                            {LANG_TK_LESSEQ, "LANG_TK_LESSEQ"}, \
                            {LANG_TK_GREATEREQ, "LANG_TK_GREATEREQ"}, \
                            {LANG_TK_INC, "LANG_TK_INC"}, \
                            {LANG_TK_DECR, "LANG_TK_DECR"}, \
                            {LANG_TK_LMOVE, "LANG_TK_LMOVE"}, \
                            {LANG_TK_RMOVE, "LANG_TK_RMOVE"});

static inline mln_lang_stm_t *
mln_lang_stm_new(mln_alloc_t *pool, \
                 void *data, \
                 mln_lang_stm_type_t type, \
                 mln_lang_stm_t *next, \
                 mln_u64_t line, \
                 mln_string_t *file);
static void mln_lang_stm_free(void *data);
static inline mln_lang_funcdef_t *
mln_lang_funcdef_new(mln_alloc_t *pool, \
                     mln_string_t *name, \
                     mln_lang_exp_t *exp, \
                     mln_lang_exp_t *closure, \
                     mln_lang_stm_t *stm, \
                     mln_u64_t line, \
                     mln_string_t *file);
static void mln_lang_funcdef_free(void *data);
static inline mln_lang_set_t *
mln_lang_set_new(mln_alloc_t *pool, \
                 mln_string_t *name, \
                 mln_lang_setstm_t *stm, \
                 mln_u64_t line, \
                 mln_string_t *file);
static void mln_lang_set_free(void *data);
static inline mln_lang_setstm_t *
mln_lang_setstm_new(mln_alloc_t *pool, \
                      void *data, \
                      mln_lang_setstm_type_t type, \
                      mln_lang_setstm_t *next, \
                      mln_u64_t line, \
                      mln_string_t *file);
static void mln_lang_setstm_free(void *data);
static inline mln_lang_block_t *
mln_lang_block_new(mln_alloc_t *pool, void *data, mln_lang_block_type_t type, mln_u64_t line, mln_string_t *file);
static void mln_lang_block_free(void *data);
static inline mln_lang_switch_t *
mln_lang_switch_new(mln_alloc_t *pool, mln_lang_exp_t *exp, mln_lang_switchstm_t *switchstm, mln_u64_t line, mln_string_t *file);
static void mln_lang_switch_free(void *data);
static inline mln_lang_switchstm_t *
mln_lang_switchstm_new(mln_alloc_t *pool, \
                       mln_lang_factor_t *factor, \
                       mln_lang_stm_t *stm, \
                       mln_lang_switchstm_t *next, \
                       mln_u64_t line, \
                       mln_string_t *file);
static void mln_lang_switchstm_free(void *data);
static inline mln_lang_while_t *
mln_lang_while_new(mln_alloc_t *pool, mln_lang_exp_t *exp, mln_lang_block_t *blockstm, mln_u64_t line, mln_string_t *file);
static void mln_lang_while_free(void *data);
static inline mln_lang_for_t *
mln_lang_for_new(mln_alloc_t *pool, \
                 mln_lang_exp_t *init_exp, \
                 mln_lang_exp_t *condition, \
                 mln_lang_exp_t *mod_exp, \
                 mln_lang_block_t *blockstm, \
                 mln_u64_t line, \
                 mln_string_t *file);
static void mln_lang_for_free(void *data);
static inline mln_lang_if_t *
mln_lang_if_new(mln_alloc_t *pool, \
                mln_lang_exp_t *condition, \
                mln_lang_block_t *truestm, \
                mln_lang_block_t *falsestm, \
                mln_u64_t line, \
                mln_string_t *file);
static void mln_lang_if_free(void *data);
static inline mln_lang_exp_t *
mln_lang_exp_new(mln_alloc_t *pool, mln_lang_assign_t *assign, mln_lang_exp_t *next, mln_u64_t line, mln_string_t *file);
static void mln_lang_exp_free(void *data);
static inline mln_lang_assign_t *
mln_lang_assign_new(mln_alloc_t *pool, \
                    mln_lang_logiclow_t *left, \
                    mln_lang_assign_op_t op, \
                    mln_lang_assign_t *right, \
                    mln_u64_t line, \
                    mln_string_t *file);
static void mln_lang_assign_free(void *data);
static inline mln_lang_assign_tmp_t *
mln_lang_assign_tmp_new(mln_alloc_t *pool, mln_lang_assign_op_t op, mln_lang_assign_t *assign);
static void mln_lang_assign_tmp_free(void *data);
static inline mln_lang_logiclow_t *
mln_lang_logiclow_new(mln_alloc_t *pool, \
                      mln_lang_logichigh_t *left, \
                      mln_lang_logiclow_op_t op, \
                      mln_lang_logiclow_t *right, \
                      mln_u64_t line, \
                      mln_string_t *file);
static void mln_lang_logiclow_free(void *data);
static inline mln_lang_logiclow_tmp_t *
mln_lang_logiclow_tmp_new(mln_alloc_t *pool, \
                          mln_lang_logiclow_op_t op, \
                          mln_lang_logiclow_t *logiclow);
static void mln_lang_logiclow_tmp_free(void *data);
static inline mln_lang_logichigh_t *
mln_lang_logichigh_new(mln_alloc_t *pool, \
                       mln_lang_relativelow_t *left, \
                       mln_lang_logichigh_op_t op, \
                       mln_lang_logichigh_t *right, \
                       mln_u64_t line, \
                       mln_string_t *file);
static void mln_lang_logichigh_free(void *data);
static inline mln_lang_logichigh_tmp_t *
mln_lang_logichigh_tmp_new(mln_alloc_t *pool, \
                           mln_lang_logichigh_op_t op, \
                           mln_lang_logichigh_t *logichigh);
static void mln_lang_logichigh_tmp_free(void *data);
static inline mln_lang_relativelow_t *
mln_lang_relativelow_new(mln_alloc_t *pool, \
                         mln_lang_relativehigh_t *left, \
                         mln_lang_relativelow_op_t op, \
                         mln_lang_relativelow_t *right, \
                         mln_u64_t line, \
                         mln_string_t *file);
static void mln_lang_relativelow_free(void *data);
static inline mln_lang_relativelow_tmp_t *
mln_lang_relativelow_tmp_new(mln_alloc_t *pool, \
                             mln_lang_relativelow_op_t op, \
                             mln_lang_relativelow_t *relativelow);
static void mln_lang_relativelow_tmp_free(void *data);
static inline mln_lang_relativehigh_t *
mln_lang_relativehigh_new(mln_alloc_t *pool, \
                          mln_lang_move_t *left, \
                          mln_lang_relativehigh_op_t op, \
                          mln_lang_relativehigh_t *right, \
                          mln_u64_t line, \
                          mln_string_t *file);
static void mln_lang_relativehigh_free(void *data);
static inline mln_lang_relativehigh_tmp_t *
mln_lang_relativehigh_tmp_new(mln_alloc_t *pool, \
                              mln_lang_relativehigh_op_t op, \
                              mln_lang_relativehigh_t *relativehigh);
static void mln_lang_relativehigh_tmp_free(void *data);
static inline mln_lang_move_t *
mln_lang_move_new(mln_alloc_t *pool, \
                  mln_lang_addsub_t *left, \
                  mln_lang_move_op_t op, \
                  mln_lang_move_t *right, \
                  mln_u64_t line, \
                  mln_string_t *file);
static void mln_lang_move_free(void *data);
static inline mln_lang_move_tmp_t *
mln_lang_move_tmp_new(mln_alloc_t *pool, \
                      mln_lang_move_op_t op, \
                      mln_lang_move_t *move);
static void mln_lang_move_tmp_free(void *data);
static inline mln_lang_addsub_t *
mln_lang_addsub_new(mln_alloc_t *pool, \
                    mln_lang_muldiv_t *left, \
                    mln_lang_addsub_op_t op, \
                    mln_lang_addsub_t *right, \
                    mln_u64_t line, \
                    mln_string_t *file);
static void mln_lang_addsub_free(void *data);
static inline mln_lang_addsub_tmp_t *
mln_lang_addsub_tmp_new(mln_alloc_t *pool, \
                        mln_lang_addsub_op_t op, \
                        mln_lang_addsub_t *addsub);
static void mln_lang_addsub_tmp_free(void *data);
static inline mln_lang_muldiv_t *
mln_lang_muldiv_new(mln_alloc_t *pool, \
                    mln_lang_not_t *left, \
                    mln_lang_muldiv_op_t op, \
                    mln_lang_muldiv_t *right, \
                    mln_u64_t line, \
                    mln_string_t *file);
static void mln_lang_muldiv_free(void *data);
static inline mln_lang_muldiv_tmp_t *
mln_lang_muldiv_tmp_new(mln_alloc_t *pool, \
                        mln_lang_muldiv_op_t op, \
                        mln_lang_muldiv_t *muldiv);
static void mln_lang_muldiv_tmp_free(void *data);
static inline mln_lang_not_t *
mln_lang_not_new(mln_alloc_t *pool, \
                  mln_lang_not_op_t op, \
                  void *data, \
                  mln_u64_t line, \
                  mln_string_t *file);
static void mln_lang_not_free(void *data);
static inline mln_lang_suffix_t *
mln_lang_suffix_new(mln_alloc_t *pool, mln_lang_locate_t *left, mln_lang_suffix_op_t op, mln_u64_t line, mln_string_t *file);
static void mln_lang_suffix_free(void *data);
static inline mln_lang_suffix_tmp_t *
mln_lang_suffix_tmp_new(mln_alloc_t *pool, mln_lang_suffix_op_t op);
static void mln_lang_suffix_tmp_free(void *data);
static inline mln_lang_locate_t *
mln_lang_locate_new(mln_alloc_t *pool, \
                    mln_lang_spec_t *left, \
                    mln_lang_locate_op_t op, \
                    void *right, \
                    mln_lang_locate_tmp_t *next, \
                    mln_u64_t line, \
                    mln_string_t *file);
static void mln_lang_locate_free(void *data);
static inline mln_lang_locate_tmp_t *
mln_lang_locate_tmp_new(mln_alloc_t *pool, \
                        mln_lang_locate_op_t op, \
                        void *data, \
                        mln_lang_locate_tmp_t *next);
static void mln_lang_locate_tmp_free(void *data);
static inline mln_lang_spec_t *
mln_lang_spec_new(mln_alloc_t *pool, \
                  mln_lang_spec_op_t op, \
                  void *data, \
                  mln_u64_t line, \
                  mln_string_t *file);
static void mln_lang_spec_free(void *data);
static inline mln_lang_factor_t *
mln_lang_factor_new(mln_alloc_t *pool, mln_lang_factor_type_t type, void *data, mln_u64_t line, mln_string_t *file);
static void mln_lang_factor_free(void *data);
static inline mln_lang_elemlist_t *
mln_lang_elemlist_new(mln_alloc_t *pool, \
                      mln_lang_assign_t *key, \
                      mln_lang_assign_t *val, \
                      mln_lang_elemlist_t *next, \
                      mln_u64_t line, \
                      mln_string_t *file);
static void mln_lang_elemlist_free(void *data);

static int mln_lang_semantic_start(mln_factor_t *left, mln_factor_t **right, void *data);
static int mln_lang_semantic_stm_block(mln_factor_t *left, mln_factor_t **right, void *data);
static int mln_lang_semantic_stmfunc(mln_factor_t *left, mln_factor_t **right, void *data);
static int mln_lang_semantic_stmset(mln_factor_t *left, mln_factor_t **right, void *data);
static int mln_lang_semantic_setstm_var(mln_factor_t *left, mln_factor_t **right, void *data);
static int mln_lang_semantic_setstm_func(mln_factor_t *left, mln_factor_t **right, void *data);
static int mln_lang_semantic_blockstmexp(mln_factor_t *left, mln_factor_t **right, void *data);
static int mln_lang_semantic_blockstmstm(mln_factor_t *left, mln_factor_t **right, void *data);
static int mln_lang_semantic_labelstm(mln_factor_t *left, mln_factor_t **right, void *data);
static int mln_lang_semantic_whilestm(mln_factor_t *left, mln_factor_t **right, void *data);
static int mln_lang_semantic_forstm(mln_factor_t *left, mln_factor_t **right, void *data);
static int mln_lang_semantic_ifstm(mln_factor_t *left, mln_factor_t **right, void *data);
static int mln_lang_semantic_switchstm(mln_factor_t *left, mln_factor_t **right, void *data);
static int mln_lang_semantic_funcdef(mln_factor_t *left, mln_factor_t **right, void *data);
static int mln_lang_semantic_funcdef_closure(mln_factor_t *left, mln_factor_t **right, void *data);
static int mln_lang_semantic_switchstm__(mln_factor_t *left, mln_factor_t **right, void *data);
static int mln_lang_semantic_switchprefix(mln_factor_t *left, mln_factor_t **right, void *data);
static int mln_lang_semantic_elsestm(mln_factor_t *left, mln_factor_t **right, void *data);
static int mln_lang_semantic_continue(mln_factor_t *left, mln_factor_t **right, void *data);
static int mln_lang_semantic_break(mln_factor_t *left, mln_factor_t **right, void *data);
static int mln_lang_semantic_return(mln_factor_t *left, mln_factor_t **right, void *data);
static int mln_lang_semantic_goto(mln_factor_t *left, mln_factor_t **right, void *data);
static int mln_lang_semantic_exp(mln_factor_t *left, mln_factor_t **right, void *data);
static int mln_lang_semantic_explist(mln_factor_t *left, mln_factor_t **right, void *data);
static int mln_lang_semantic_assignexp(mln_factor_t *left, mln_factor_t **right, void *data);
static int mln_lang_semantic_assignexpeq(mln_factor_t *left, mln_factor_t **right, void *data);
static int mln_lang_semantic_assignexppluseq(mln_factor_t *left, mln_factor_t **right, void *data);
static int mln_lang_semantic_assignexpsubeq(mln_factor_t *left, mln_factor_t **right, void *data);
static int mln_lang_semantic_assignexplmoveq(mln_factor_t *left, mln_factor_t **right, void *data);
static int mln_lang_semantic_assignexprmoveq(mln_factor_t *left, mln_factor_t **right, void *data);
static int mln_lang_semantic_assignexpmuleq(mln_factor_t *left, mln_factor_t **right, void *data);
static int mln_lang_semantic_assignexpdiveq(mln_factor_t *left, mln_factor_t **right, void *data);
static int mln_lang_semantic_assignexporeq(mln_factor_t *left, mln_factor_t **right, void *data);
static int mln_lang_semantic_assignexpandeq(mln_factor_t *left, mln_factor_t **right, void *data);
static int mln_lang_semantic_assignexpxoreq(mln_factor_t *left, mln_factor_t **right, void *data);
static int mln_lang_semantic_assignexpmodeq(mln_factor_t *left, mln_factor_t **right, void *data);
static int mln_lang_semantic_logiclowexp(mln_factor_t *left, mln_factor_t **right, void *data);
static int mln_lang_semantic_logiclowexpor(mln_factor_t *left, mln_factor_t **right, void *data);
static int mln_lang_semantic_logiclowexpand(mln_factor_t *left, mln_factor_t **right, void *data);
static int mln_lang_semantic_logichighexp(mln_factor_t *left, mln_factor_t **right, void *data);
static int mln_lang_semantic_logichighor(mln_factor_t *left, mln_factor_t **right, void *data);
static int mln_lang_semantic_logichighand(mln_factor_t *left, mln_factor_t **right, void *data);
static int mln_lang_semantic_logichighxor(mln_factor_t *left, mln_factor_t **right, void *data);
static int mln_lang_semantic_relativelowexp(mln_factor_t *left, mln_factor_t **right, void *data);
static int mln_lang_semantic_relativeloweq(mln_factor_t *left, mln_factor_t **right, void *data);
static int mln_lang_semantic_relativelownoneq(mln_factor_t *left, mln_factor_t **right, void *data);
static int mln_lang_semantic_relativehighexp(mln_factor_t *left, mln_factor_t **right, void *data);
static int mln_lang_semantic_relativehighless(mln_factor_t *left, mln_factor_t **right, void *data);
static int mln_lang_semantic_relativehighlesseq(mln_factor_t *left, mln_factor_t **right, void *data);
static int mln_lang_semantic_relativehighgreater(mln_factor_t *left, mln_factor_t **right, void *data);
static int mln_lang_semantic_relativehighgreatereq(mln_factor_t *left, mln_factor_t **right, void *data);
static int mln_lang_semantic_moveexp(mln_factor_t *left, mln_factor_t **right, void *data);
static int mln_lang_semantic_moveleft(mln_factor_t *left, mln_factor_t **right, void *data);
static int mln_lang_semantic_moveright(mln_factor_t *left, mln_factor_t **right, void *data);
static int mln_lang_semantic_addsubexp(mln_factor_t *left, mln_factor_t **right, void *data);
static int mln_lang_semantic_addsubplus(mln_factor_t *left, mln_factor_t **right, void *data);
static int mln_lang_semantic_addsubsub(mln_factor_t *left, mln_factor_t **right, void *data);
static int mln_lang_semantic_muldivexp(mln_factor_t *left, mln_factor_t **right, void *data);
static int mln_lang_semantic_muldivmul(mln_factor_t *left, mln_factor_t **right, void *data);
static int mln_lang_semantic_muldivdiv(mln_factor_t *left, mln_factor_t **right, void *data);
static int mln_lang_semantic_muldivmod(mln_factor_t *left, mln_factor_t **right, void *data);
static int mln_lang_semantic_notnot(mln_factor_t *left, mln_factor_t **right, void *data);
static int mln_lang_semantic_notsuffix(mln_factor_t *left, mln_factor_t **right, void *data);
static int mln_lang_semantic_suffixexp(mln_factor_t *left, mln_factor_t **right, void *data);
static int mln_lang_semantic_suffixinc(mln_factor_t *left, mln_factor_t **right, void *data);
static int mln_lang_semantic_suffixdec(mln_factor_t *left, mln_factor_t **right, void *data);
static int mln_lang_semantic_locateexp(mln_factor_t *left, mln_factor_t **right, void *data);
static int mln_lang_semantic_locateindex(mln_factor_t *left, mln_factor_t **right, void *data);
static int mln_lang_semantic_locateproperty(mln_factor_t *left, mln_factor_t **right, void *data);
static int mln_lang_semantic_locatefunc(mln_factor_t *left, mln_factor_t **right, void *data);
static int mln_lang_semantic_specnegative(mln_factor_t *left, mln_factor_t **right, void *data);
static int mln_lang_semantic_specreverse(mln_factor_t *left, mln_factor_t **right, void *data);
static int mln_lang_semantic_specrefer(mln_factor_t *left, mln_factor_t **right, void *data);
static int mln_lang_semantic_specinc(mln_factor_t *left, mln_factor_t **right, void *data);
static int mln_lang_semantic_specdec(mln_factor_t *left, mln_factor_t **right, void *data);
static int mln_lang_semantic_specnew(mln_factor_t *left, mln_factor_t **right, void *data);
static int mln_lang_semantic_specparenth(mln_factor_t *left, mln_factor_t **right, void *data);
static int mln_lang_semantic_specfactor(mln_factor_t *left, mln_factor_t **right, void *data);
static int mln_lang_semantic_factortrue(mln_factor_t *left, mln_factor_t **right, void *data);
static int mln_lang_semantic_factorfalse(mln_factor_t *left, mln_factor_t **right, void *data);
static int mln_lang_semantic_factornil(mln_factor_t *left, mln_factor_t **right, void *data);
static int mln_lang_semantic_factorid(mln_factor_t *left, mln_factor_t **right, void *data);
static int mln_lang_semantic_factorint(mln_factor_t *left, mln_factor_t **right, void *data);
static int mln_lang_semantic_factorreal(mln_factor_t *left, mln_factor_t **right, void *data);
static int mln_lang_semantic_factorstring(mln_factor_t *left, mln_factor_t **right, void *data);
static int mln_lang_semantic_factorarray(mln_factor_t *left, mln_factor_t **right, void *data);
static int mln_lang_semantic_elemlist(mln_factor_t *left, mln_factor_t **right, void *data);
static int mln_lang_semantic_elemval(mln_factor_t *left, mln_factor_t **right, void *data);
static int mln_lang_semantic_elemnext(mln_factor_t *left, mln_factor_t **right, void *data);

static mln_string_t keywords[] = {
mln_string("if"),
mln_string("else"),
mln_string("while"),
mln_string("for"),
mln_string("continue"),
mln_string("break"),
mln_string("return"),
mln_string("goto"),
mln_string("nil"),
mln_string("true"),
mln_string("false"),
mln_string("switch"),
mln_string("case"),
mln_string("default"),
mln_string("fi"),
mln_string(NULL),
};

static mln_production_t prod_tbl[] = {
{"start: stm LANG_TK_EOF", mln_lang_semantic_start},
{"stm: blockstm stm", mln_lang_semantic_stm_block},
{"stm: funcdef stm", mln_lang_semantic_stmfunc},
{"stm: LANG_TK_ID LANG_TK_LBRACE setstm LANG_TK_RBRACE stm", mln_lang_semantic_stmset},
{"stm: LANG_TK_ID LANG_TK_COLON stm", mln_lang_semantic_labelstm},
{"stm: LANG_TK_WHILE LANG_TK_LPAR exp LANG_TK_RPAR blockstm stm", mln_lang_semantic_whilestm},
{"stm: LANG_TK_FOR LANG_TK_LPAR exp LANG_TK_SEMIC exp LANG_TK_SEMIC exp LANG_TK_RPAR blockstm stm", mln_lang_semantic_forstm},
{"stm: LANG_TK_SWITCH LANG_TK_LPAR exp LANG_TK_RPAR LANG_TK_LBRACE switchstm LANG_TK_RBRACE stm", mln_lang_semantic_switchstm},
{"stm: ", NULL},
{"setstm: LANG_TK_ID LANG_TK_SEMIC setstm", mln_lang_semantic_setstm_var},
{"setstm: funcdef setstm", mln_lang_semantic_setstm_func},
{"setstm: ", NULL},
{"funcdef: LANG_TK_AT LANG_TK_ID LANG_TK_LPAR exp LANG_TK_RPAR use LANG_TK_LBRACE stm LANG_TK_RBRACE", mln_lang_semantic_funcdef},
{"use: LANG_TK_DOLL LANG_TK_LPAR exp LANG_TK_RPAR", mln_lang_semantic_funcdef_closure},
{"use: ", NULL},
{"switchstm: switchprefix LANG_TK_COLON stm switchstm", mln_lang_semantic_switchstm__},
{"switchstm: ", NULL},
{"switchprefix: LANG_TK_DEFAULT", NULL},
{"switchprefix: LANG_TK_CASE factor", mln_lang_semantic_switchprefix},
{"blockstm: exp LANG_TK_SEMIC", mln_lang_semantic_blockstmexp},
{"blockstm: LANG_TK_LBRACE stm LANG_TK_RBRACE", mln_lang_semantic_blockstmstm},
{"blockstm: LANG_TK_CONTINUE LANG_TK_SEMIC", mln_lang_semantic_continue},
{"blockstm: LANG_TK_BREAK LANG_TK_SEMIC", mln_lang_semantic_break},
{"blockstm: LANG_TK_RETURN exp LANG_TK_SEMIC", mln_lang_semantic_return},
{"blockstm: LANG_TK_GOTO LANG_TK_ID LANG_TK_SEMIC", mln_lang_semantic_goto},
{"blockstm: LANG_TK_IF LANG_TK_LPAR exp LANG_TK_RPAR blockstm else_exp", mln_lang_semantic_ifstm},
{"else_exp: LANG_TK_ELSE blockstm", mln_lang_semantic_elsestm},
{"else_exp: LANG_TK_FI", NULL},
{"exp: assign_exp explist", mln_lang_semantic_exp},
{"exp: ", NULL},
{"explist: LANG_TK_COMMA exp", mln_lang_semantic_explist},
{"explist: ", NULL},
{"assign_exp: logiclow_exp __assign_exp", mln_lang_semantic_assignexp},
{"__assign_exp: LANG_TK_EQUAL assign_exp", mln_lang_semantic_assignexpeq},
{"__assign_exp: LANG_TK_PLUSEQ assign_exp", mln_lang_semantic_assignexppluseq},
{"__assign_exp: LANG_TK_SUBEQ assign_exp", mln_lang_semantic_assignexpsubeq},
{"__assign_exp: LANG_TK_LMOVEQ assign_exp", mln_lang_semantic_assignexplmoveq},
{"__assign_exp: LANG_TK_RMOVEQ assign_exp", mln_lang_semantic_assignexprmoveq},
{"__assign_exp: LANG_TK_MULEQ assign_exp", mln_lang_semantic_assignexpmuleq},
{"__assign_exp: LANG_TK_DIVEQ assign_exp", mln_lang_semantic_assignexpdiveq},
{"__assign_exp: LANG_TK_OREQ assign_exp", mln_lang_semantic_assignexporeq},
{"__assign_exp: LANG_TK_ANDEQ assign_exp", mln_lang_semantic_assignexpandeq},
{"__assign_exp: LANG_TK_XOREQ assign_exp", mln_lang_semantic_assignexpxoreq},
{"__assign_exp: LANG_TK_MODEQ assign_exp", mln_lang_semantic_assignexpmodeq},
{"__assign_exp: ", NULL},
{"logiclow_exp: logichigh_exp __logiclow_exp", mln_lang_semantic_logiclowexp},
{"__logiclow_exp: LANG_TK_LOWOR logiclow_exp", mln_lang_semantic_logiclowexpor},
{"__logiclow_exp: LANG_TK_LOWAND logiclow_exp", mln_lang_semantic_logiclowexpand},
{"__logiclow_exp: ", NULL},
{"logichigh_exp: relativelow_exp __logichigh_exp", mln_lang_semantic_logichighexp},
{"__logichigh_exp: LANG_TK_VERTL logichigh_exp", mln_lang_semantic_logichighor},
{"__logichigh_exp: LANG_TK_AMP logichigh_exp", mln_lang_semantic_logichighand},
{"__logichigh_exp: LANG_TK_XOR logichigh_exp", mln_lang_semantic_logichighxor},
{"__logichigh_exp: ", NULL},
{"relativelow_exp: relativehigh_exp __relativelow_exp", mln_lang_semantic_relativelowexp},
{"__relativelow_exp: LANG_TK_DEQUAL relativelow_exp", mln_lang_semantic_relativeloweq},
{"__relativelow_exp: LANG_TK_NONEQUAL relativelow_exp", mln_lang_semantic_relativelownoneq},
{"__relativelow_exp: ", NULL},
{"relativehigh_exp: move_exp __relativehigh_exp", mln_lang_semantic_relativehighexp},
{"__relativehigh_exp: LANG_TK_LAGL relativehigh_exp", mln_lang_semantic_relativehighless},
{"__relativehigh_exp: LANG_TK_LESSEQ relativehigh_exp", mln_lang_semantic_relativehighlesseq},
{"__relativehigh_exp: LANG_TK_RAGL relativehigh_exp", mln_lang_semantic_relativehighgreater},
{"__relativehigh_exp: LANG_TK_GREATEREQ relativehigh_exp", mln_lang_semantic_relativehighgreatereq},
{"__relativehigh_exp: ", NULL},
{"move_exp: addsub_exp __move_exp", mln_lang_semantic_moveexp},
{"__move_exp: LANG_TK_LMOVE move_exp", mln_lang_semantic_moveleft},
{"__move_exp: LANG_TK_RMOVE move_exp", mln_lang_semantic_moveright},
{"__move_exp: ", NULL},
{"addsub_exp: muldiv_exp __addsub_exp", mln_lang_semantic_addsubexp},
{"__addsub_exp: LANG_TK_PLUS addsub_exp", mln_lang_semantic_addsubplus},
{"__addsub_exp: LANG_TK_SUB addsub_exp", mln_lang_semantic_addsubsub},
{"__addsub_exp: ", NULL},
{"muldiv_exp: not_exp __muldiv_exp", mln_lang_semantic_muldivexp},
{"__muldiv_exp: LANG_TK_AST muldiv_exp", mln_lang_semantic_muldivmul},
{"__muldiv_exp: LANG_TK_SLASH muldiv_exp", mln_lang_semantic_muldivdiv},
{"__muldiv_exp: LANG_TK_PERC muldiv_exp", mln_lang_semantic_muldivmod},
{"__muldiv_exp: ", NULL},
{"not_exp: LANG_TK_EXCL not_exp", mln_lang_semantic_notnot},
{"not_exp: suffix_exp", mln_lang_semantic_notsuffix},
{"suffix_exp: locate_exp __suffix_exp", mln_lang_semantic_suffixexp},
{"__suffix_exp: LANG_TK_INC", mln_lang_semantic_suffixinc},
{"__suffix_exp: LANG_TK_DECR", mln_lang_semantic_suffixdec},
{"__suffix_exp: ", NULL},
{"locate_exp: spec_exp __locate_exp", mln_lang_semantic_locateexp},
{"__locate_exp: LANG_TK_LSQUAR exp LANG_TK_RSQUAR __locate_exp", mln_lang_semantic_locateindex},
{"__locate_exp: LANG_TK_PERIOD LANG_TK_ID __locate_exp", mln_lang_semantic_locateproperty},
{"__locate_exp: LANG_TK_LPAR exp LANG_TK_RPAR __locate_exp", mln_lang_semantic_locatefunc},
{"__locate_exp: ", NULL},
{"spec_exp: LANG_TK_SUB spec_exp", mln_lang_semantic_specnegative},
{"spec_exp: LANG_TK_DASH spec_exp", mln_lang_semantic_specreverse},
{"spec_exp: LANG_TK_AMP spec_exp", mln_lang_semantic_specrefer},
{"spec_exp: LANG_TK_INC spec_exp", mln_lang_semantic_specinc},
{"spec_exp: LANG_TK_DECR spec_exp", mln_lang_semantic_specdec},
{"spec_exp: LANG_TK_DOLL LANG_TK_ID", mln_lang_semantic_specnew},
{"spec_exp: LANG_TK_LPAR exp LANG_TK_RPAR", mln_lang_semantic_specparenth},
{"spec_exp: factor", mln_lang_semantic_specfactor},
{"factor: LANG_TK_TRUE", mln_lang_semantic_factortrue},
{"factor: LANG_TK_FALSE", mln_lang_semantic_factorfalse},
{"factor: LANG_TK_NIL", mln_lang_semantic_factornil},
{"factor: LANG_TK_ID", mln_lang_semantic_factorid},
{"factor: LANG_TK_OCT", mln_lang_semantic_factorint},
{"factor: LANG_TK_DEC", mln_lang_semantic_factorint},
{"factor: LANG_TK_HEX", mln_lang_semantic_factorint},
{"factor: LANG_TK_REAL", mln_lang_semantic_factorreal},
{"factor: LANG_TK_STRING", mln_lang_semantic_factorstring},
{"factor: LANG_TK_LSQUAR elemlist LANG_TK_RSQUAR", mln_lang_semantic_factorarray},
{"elemlist: assign_exp elemval elemnext", mln_lang_semantic_elemlist},
{"elemlist: ", NULL},
{"elemval: LANG_TK_COLON assign_exp", mln_lang_semantic_elemval},
{"elemval: ", NULL},
{"elemnext: LANG_TK_COMMA elemlist", mln_lang_semantic_elemnext},
{"elemnext: ", NULL}
};

static mln_string_t mln_lang_env = mln_string("MELANG_PATH");

static inline int
mln_get_char(mln_lex_t *lex, char c);

MLN_FUNC(static, mln_lang_struct_t *, mln_lang_sub_handler, \
         (mln_lex_t *lex, void *data), (lex, data), \
{
    char c = mln_lex_getchar(lex);
    if (c == '-') {
        if (mln_lex_putchar(lex, c) == MLN_ERR) return NULL;
        return mln_lang_new(lex, LANG_TK_DECR);
    }
    if (c == '=') {
        if (mln_lex_putchar(lex, c) == MLN_ERR) return NULL;
        return mln_lang_new(lex, LANG_TK_SUBEQ);
    }
    mln_lex_stepback(lex, c);
    return mln_lang_new(lex, LANG_TK_SUB);
})

MLN_FUNC(static, mln_lang_struct_t *, mln_lang_plus_handler, \
         (mln_lex_t *lex, void *data), (lex, data), \
{
    char c = mln_lex_getchar(lex);
    if (c == '+') {
        if (mln_lex_putchar(lex, c) == MLN_ERR) return NULL;
        return mln_lang_new(lex, LANG_TK_INC);
    }
    if (c == '=') {
        if (mln_lex_putchar(lex, c) == MLN_ERR) return NULL;
        return mln_lang_new(lex, LANG_TK_PLUSEQ);
    }
    mln_lex_stepback(lex, c);
    return mln_lang_new(lex, LANG_TK_PLUS);
})

MLN_FUNC(static, mln_lang_struct_t *, mln_lang_ast_handler, \
         (mln_lex_t *lex, void *data), (lex, data), \
{
    char c = mln_lex_getchar(lex);
    if (c == '=') {
        if (mln_lex_putchar(lex, c) == MLN_ERR) return NULL;
        return mln_lang_new(lex, LANG_TK_MULEQ);
    }
    mln_lex_stepback(lex, c);
    return mln_lang_new(lex, LANG_TK_AST);
})

MLN_FUNC(static, mln_lang_struct_t *, mln_lang_lagl_handler, \
         (mln_lex_t *lex, void *data), (lex, data), \
{
    char c = mln_lex_getchar(lex);
    if (c == '<') {
        if (mln_lex_putchar(lex, c) == MLN_ERR) return NULL;
        if ((c = mln_lex_getchar(lex)) != '=') {
            mln_lex_stepback(lex, c);
            return mln_lang_new(lex, LANG_TK_LMOVE);
        }
        if (mln_lex_putchar(lex, c) == MLN_ERR) return NULL;
        return mln_lang_new(lex, LANG_TK_LMOVEQ);
    }
    if (c == '=') {
        if (mln_lex_putchar(lex, c) == MLN_ERR) return NULL;
        return mln_lang_new(lex, LANG_TK_LESSEQ);
    }
    mln_lex_stepback(lex, c);
    return mln_lang_new(lex, LANG_TK_LAGL);
})

MLN_FUNC(static, mln_lang_struct_t *, mln_lang_ragl_handler, \
         (mln_lex_t *lex, void *data), (lex, data), \
{
    char c = mln_lex_getchar(lex);
    if (c == '>') {
        if (mln_lex_putchar(lex, c) == MLN_ERR) return NULL;
        if ((c = mln_lex_getchar(lex)) != '=') {
            mln_lex_stepback(lex, c);
            return mln_lang_new(lex, LANG_TK_RMOVE);
        }
        if (mln_lex_putchar(lex, c) == MLN_ERR) return NULL;
        return mln_lang_new(lex, LANG_TK_RMOVEQ);
    }
    if (c == '=') {
        if (mln_lex_putchar(lex, c) == MLN_ERR) return NULL;
        return mln_lang_new(lex, LANG_TK_GREATEREQ);
    }
    mln_lex_stepback(lex, c);
    return mln_lang_new(lex, LANG_TK_RAGL);
})

MLN_FUNC(static, mln_lang_struct_t *, mln_lang_vertl_handler, \
         (mln_lex_t *lex, void *data), (lex, data), \
{
    char c = mln_lex_getchar(lex);
    if (c == '=') {
        if (mln_lex_putchar(lex, c) == MLN_ERR) return NULL;
        return mln_lang_new(lex, LANG_TK_OREQ);
    }
    if (c == '|') {
        if (mln_lex_putchar(lex, c) == MLN_ERR) return NULL;
        return mln_lang_new(lex, LANG_TK_LOWOR);
    }
    mln_lex_stepback(lex, c);
    return mln_lang_new(lex, LANG_TK_VERTL);
})

MLN_FUNC(static, mln_lang_struct_t *, mln_lang_amp_handler, \
         (mln_lex_t *lex, void *data), (lex, data), \
{
    char c = mln_lex_getchar(lex);
    if (c == '=') {
        if (mln_lex_putchar(lex, c) == MLN_ERR) return NULL;
        return mln_lang_new(lex, LANG_TK_ANDEQ);
    }
    if (c == '&') {
        if (mln_lex_putchar(lex, c) == MLN_ERR) return NULL;
        return mln_lang_new(lex, LANG_TK_LOWAND);
    }
    mln_lex_stepback(lex, c);
    return mln_lang_new(lex, LANG_TK_AMP);
})

MLN_FUNC(static, mln_lang_struct_t *, mln_lang_dash_handler, \
         (mln_lex_t *lex, void *data), (lex, data), \
{
    char c = mln_lex_getchar(lex);
    if (c == '=') {
        if (mln_lex_putchar(lex, c) == MLN_ERR) return NULL;
        return mln_lang_new(lex, LANG_TK_ANTIEQ);
    }
    mln_lex_stepback(lex, c);
    return mln_lang_new(lex, LANG_TK_DASH);
})

MLN_FUNC(static, mln_lang_struct_t *, mln_lang_xor_handler, \
         (mln_lex_t *lex, void *data), (lex, data), \
{
    char c = mln_lex_getchar(lex);
    if (c == '=') {
        if (mln_lex_putchar(lex, c) == MLN_ERR) return NULL;
        return mln_lang_new(lex, LANG_TK_XOREQ);
    }
    mln_lex_stepback(lex, c);
    return mln_lang_new(lex, LANG_TK_XOR);
})

MLN_FUNC(static, mln_lang_struct_t *, mln_lang_perc_handler, \
         (mln_lex_t *lex, void *data), (lex, data), \
{
    char c = mln_lex_getchar(lex);
    if (c == '=') {
        if (mln_lex_putchar(lex, c) == MLN_ERR) return NULL;
        return mln_lang_new(lex, LANG_TK_MODEQ);
    }
    mln_lex_stepback(lex, c);
    return mln_lang_new(lex, LANG_TK_PERC);
})

MLN_FUNC(static, mln_lang_struct_t *, mln_lang_equal_handler, \
         (mln_lex_t *lex, void *data), (lex, data), \
{
    char c = mln_lex_getchar(lex);
    if (c == '=') {
        if (mln_lex_putchar(lex, c) == MLN_ERR) return NULL;
        return mln_lang_new(lex, LANG_TK_DEQUAL);
    }
    mln_lex_stepback(lex, c);
    return mln_lang_new(lex, LANG_TK_EQUAL);
})

MLN_FUNC(static, mln_lang_struct_t *, mln_lang_excl_handler, \
         (mln_lex_t *lex, void *data), (lex, data), \
{
    char c = mln_lex_getchar(lex);
    if (c == '=') {
        if (mln_lex_putchar(lex, c) == MLN_ERR) return NULL;
        return mln_lang_new(lex, LANG_TK_NONEQUAL);
    }
    mln_lex_stepback(lex, c);
    return mln_lang_new(lex, LANG_TK_EXCL);
})

MLN_FUNC(static, mln_lang_struct_t *, mln_lang_sglq_handler, \
         (mln_lex_t *lex, void *data), (lex, data), \
{
    mln_lex_result_clean(lex);
    char c;
    while ( 1 ) {
        if ((c = mln_lex_getchar(lex)) == MLN_ERR) return NULL;
        if (c == MLN_EOF) {
            mln_lex_error_set(lex, MLN_LEX_EINVEOF);
            return NULL;
        }
        if (c == '\'') break;
        if (c == '\n') ++lex->line;
        if (mln_lex_putchar(lex, c) == MLN_ERR) return NULL;
    }
    return mln_lang_new(lex, LANG_TK_STRING);
})

static inline int mln_get_char(mln_lex_t *lex, char c)
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
#if !defined(MSVC)
            case 'e':
                if (mln_lex_putchar(lex, '\e') == MLN_ERR) return -1;
                break;
#endif
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

MLN_FUNC(static, mln_lang_struct_t *, mln_lang_dblq_handler, \
         (mln_lex_t *lex, void *data), (lex, data), \
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
        if (c == '\n') ++lex->line;
        if (mln_get_char(lex, c) < 0) return NULL;
    }
    return mln_lang_new(lex, LANG_TK_STRING);
})

MLN_FUNC(static, mln_lang_struct_t *, mln_lang_slash_handler, \
         (mln_lex_t *lex, void *data), (lex, data), \
{
    char c = mln_lex_getchar(lex);
    if (c == MLN_ERR) return NULL;
    if (c == '*') {
        if (mln_lex_putchar(lex, c) == MLN_ERR) return NULL;
        while ( 1 ) {
            c = mln_lex_getchar(lex);
            if (c == MLN_ERR) return NULL;
            if (c == MLN_EOF) {
                mln_lex_stepback(lex, c);
                break;
            }
            if (c == '\n') ++(lex->line);
            if (c == '*') {
                if (mln_lex_putchar(lex, c) == MLN_ERR) return NULL;
                c = mln_lex_getchar(lex);
                if (c == MLN_ERR) return NULL;
                if (c == MLN_EOF) {
                    mln_lex_stepback(lex, c);
                    break;
                }
                if (c == '\n') ++(lex->line);
                if (c == '/') {
                    if (mln_lex_putchar(lex, c) == MLN_ERR) return NULL;
                    break;
                }
            }
            if (mln_lex_putchar(lex, c) == MLN_ERR) return NULL;
        }
    } else if (c == '/') {
        if (mln_lex_putchar(lex, c) == MLN_ERR) return NULL;
        while ( 1 ) {
            c = mln_lex_getchar(lex);
            if (c == MLN_ERR) return NULL;
            if (c == MLN_EOF) {
                mln_lex_stepback(lex, c);
                break;
            }
            if (c == '\n') {
                mln_lex_stepback(lex, c);
                break;
            }
            if (mln_lex_putchar(lex, c) == MLN_ERR) return NULL;
        }
    } else if (c == '=') {
        if (mln_lex_putchar(lex, c) == MLN_ERR) return NULL;
        return mln_lang_new(lex, LANG_TK_DIVEQ);
    } else {
        mln_lex_stepback(lex, c);
        return mln_lang_new(lex, LANG_TK_SLASH);
    }
    mln_lex_result_clean(lex);
    return mln_lang_token(lex);
})

/*
 * AST structs
 */
MLN_FUNC(static inline, mln_lang_stm_t *, mln_lang_stm_new, \
         (mln_alloc_t *pool, void *data, mln_lang_stm_type_t type, \
          mln_lang_stm_t *next, mln_u64_t line, mln_string_t *file), \
         (pool, data, type, next, line, file), \
{
    mln_lang_stm_t *ls;
    if ((ls = (mln_lang_stm_t *)mln_alloc_m(pool, sizeof(mln_lang_stm_t))) == NULL) {
        return NULL;
    }
    ls->file = NULL;
    if (file != NULL && (ls->file = mln_string_pool_dup(pool, file)) == NULL) {
        mln_alloc_free(ls);
        return NULL;
    }
    ls->line = line;
    ls->type = type;
    switch (type) {
        case M_STM_BLOCK:
            ls->data.block = (mln_lang_block_t *)data;
            break;
        case M_STM_FUNC:
            ls->data.func = (mln_lang_funcdef_t *)data;
            break;
        case M_STM_SET:
            ls->data.setdef = (mln_lang_set_t *)data;
            break;
        case M_STM_LABEL:
            ls->data.pos = (mln_string_t *)data;
            break;
        case M_STM_SWITCH:
            ls->data.sw = (mln_lang_switch_t *)data;
            break;
        case M_STM_WHILE:
            ls->data.w = (mln_lang_while_t *)data;
            break;
        default:
            ls->data.f = (mln_lang_for_t *)data;
            break;
    }
    ls->next = next;
    ls->jump = NULL;
    ls->jump_type = 0;
    return ls;
})

MLN_FUNC_VOID(static, void, mln_lang_stm_free, (void *data), (data), {
    mln_lang_stm_t *stm, *next = (mln_lang_stm_t *)data;
    mln_lang_stm_type_t type;
    while (next != NULL) {
        stm = next;
        type = stm->type;
        switch (type) {
            case M_STM_BLOCK:
                if (stm->data.block != NULL) mln_lang_block_free(stm->data.block);
                break;
            case M_STM_FUNC:
                if (stm->data.func != NULL) mln_lang_funcdef_free(stm->data.func);
                break;
            case M_STM_SET:
                if (stm->data.setdef != NULL) mln_lang_set_free(stm->data.setdef);
                break;
            case M_STM_LABEL:
                if (stm->data.pos != NULL) mln_string_free(stm->data.pos);
                break;
            case M_STM_SWITCH:
                if (stm->data.sw != NULL) mln_lang_switch_free(stm->data.sw);
                break;
            case M_STM_WHILE:
                if (stm->data.w != NULL) mln_lang_while_free(stm->data.w);
                break;
            default:
                if (stm->data.f != NULL) mln_lang_for_free(stm->data.f);
                break;
        }
        next = stm->next;
        if (stm->file != NULL)
            mln_string_free(stm->file);
        mln_alloc_free(stm);
    }
})


MLN_FUNC(static inline, mln_lang_funcdef_t *, mln_lang_funcdef_new, \
         (mln_alloc_t *pool, mln_string_t *name, mln_lang_exp_t *exp, mln_lang_exp_t *closure, \
          mln_lang_stm_t *stm, mln_u64_t line, mln_string_t *file), \
         (pool, name, exp, closure, stm, line, file), \
{
    mln_lang_funcdef_t *lf;
    if ((lf = (mln_lang_funcdef_t *)mln_alloc_m(pool, sizeof(mln_lang_funcdef_t))) == NULL) {
        return NULL;
    }
    lf->file = NULL;
    if (file != NULL && (lf->file = mln_string_pool_dup(pool, file)) == NULL) {
        mln_alloc_free(lf);
        return NULL;
    }
    lf->line = line;
    lf->name = name;
    lf->args = exp;
    lf->stm = stm;
    lf->closure = closure;
    return lf;
})

MLN_FUNC_VOID(static, void, mln_lang_funcdef_free, (void *data), (data), {
    if (data == NULL) return;
    mln_lang_funcdef_t *lf = (mln_lang_funcdef_t *)data;
    if (lf->name != NULL) mln_string_free(lf->name);
    if (lf->args != NULL) mln_lang_exp_free(lf->args);
    if (lf->stm != NULL) mln_lang_stm_free(lf->stm);
    if (lf->closure != NULL) mln_lang_exp_free(lf->closure);
    if (lf->file != NULL) mln_string_free(lf->file);
    mln_alloc_free(lf);
})


MLN_FUNC(static inline, mln_lang_set_t *, mln_lang_set_new, \
         (mln_alloc_t *pool, mln_string_t *name, mln_lang_setstm_t *stm, \
          mln_u64_t line, mln_string_t *file), \
         (pool, name, stm, line, file), \
{
    mln_lang_set_t *lc;
    if ((lc = (mln_lang_set_t *)mln_alloc_m(pool, sizeof(mln_lang_set_t))) == NULL) {
        return NULL;
    }
    lc->file = NULL;
    if (file != NULL && (lc->file = mln_string_pool_dup(pool, file)) == NULL) {
        mln_alloc_free(lc);
        return NULL;
    }
    lc->line = line;
    lc->name = name;
    lc->stm = stm;
    return lc;
})

MLN_FUNC_VOID(static, void, mln_lang_set_free, (void *data), (data), {
    if (data == NULL) return;
    mln_lang_set_t *lc = (mln_lang_set_t *)data;
    if (lc->name != NULL) mln_string_free(lc->name);
    if (lc->stm != NULL) mln_lang_setstm_free(lc->stm);
    if (lc->file != NULL) mln_string_free(lc->file);
    mln_alloc_free(lc);
})


MLN_FUNC(static inline, mln_lang_setstm_t *, mln_lang_setstm_new, \
         (mln_alloc_t *pool, void *data, mln_lang_setstm_type_t type, \
          mln_lang_setstm_t *next, mln_u64_t line, mln_string_t *file), \
         (pool, data, type, next, line, file), \
{
    mln_lang_setstm_t *lc;
    if ((lc = (mln_lang_setstm_t *)mln_alloc_m(pool, sizeof(mln_lang_setstm_t))) == NULL) {
        return NULL;
    }
    lc->file = NULL;
    if (file != NULL && (lc->file = mln_string_pool_dup(pool, file)) == NULL) {
        mln_alloc_free(lc);
        return NULL;
    }
    lc->line = line;
    lc->type = type;
    switch (type) {
        case M_SETSTM_VAR:
            lc->data.var = (mln_string_t *)data;
            break;
        default:
            lc->data.func = (mln_lang_funcdef_t *)data;
            break;
    }
    lc->next = next;
    return lc;
})

MLN_FUNC_VOID(static, void, mln_lang_setstm_free, (void *data), (data), {
    mln_lang_setstm_t *lc, *next = (mln_lang_setstm_t *)data;
    while (next != NULL) {
        lc = next;
        switch (lc->type) {
            case M_SETSTM_VAR:
                if (lc->data.var != NULL) mln_string_free(lc->data.var);
                break;
            default:
                if (lc->data.func != NULL) mln_lang_funcdef_free(lc->data.func);
                break;
        }
        next = lc->next;
        if (lc->file != NULL) mln_string_free(lc->file);
        mln_alloc_free(lc);
    }
})


MLN_FUNC(static inline, mln_lang_block_t *, mln_lang_block_new, \
         (mln_alloc_t *pool, void *data, mln_lang_block_type_t type, mln_u64_t line, mln_string_t *file), \
         (pool, data, type, line, file), \
{
    mln_lang_block_t *lb;
    if ((lb = (mln_lang_block_t *)mln_alloc_m(pool, sizeof(mln_lang_block_t))) == NULL) {
        return NULL;
    }
    lb->file = NULL;
    if (file != NULL && (lb->file = mln_string_pool_dup(pool, file)) == NULL) {
        mln_alloc_free(lb);
        return NULL;
    }
    lb->line = line;
    lb->type = type;
    switch (type) {
        case M_BLOCK_EXP:
        case M_BLOCK_RETURN:
            lb->data.exp = (mln_lang_exp_t *)data;
            break;
        case M_BLOCK_STM:
            lb->data.stm = (mln_lang_stm_t *)data;
            break;
        case M_BLOCK_GOTO:
            lb->data.pos = (mln_string_t *)data;
            break;
        case M_BLOCK_IF:
            lb->data.i = (mln_lang_if_t *)data;
            break;
        default:
            lb->data.pos = NULL;
            break;
    }
    lb->jump = NULL;
    lb->jump_type = 0;
    return lb;
})

MLN_FUNC_VOID(static, void, mln_lang_block_free, (void *data), (data), {
    if (data == NULL) return;
    mln_lang_block_t *lb = (mln_lang_block_t *)data;
    mln_lang_block_type_t type = lb->type;
    switch (type) {
        case M_BLOCK_EXP:
        case M_BLOCK_RETURN:
            if (lb->data.exp != NULL) mln_lang_exp_free(lb->data.exp);
            break;
        case M_BLOCK_STM:
            if (lb->data.stm != NULL) mln_lang_stm_free(lb->data.stm);
            break;
        case M_BLOCK_GOTO:
            if (lb->data.pos != NULL) mln_string_free(lb->data.pos);
            break;
        case M_BLOCK_IF:
            if (lb->data.i != NULL) mln_lang_if_free(lb->data.i);
            break;
        default:
            break;
    }
    if (lb->file != NULL) mln_string_free(lb->file);
    mln_alloc_free(lb);
})


MLN_FUNC(static inline, mln_lang_switch_t *, mln_lang_switch_new, \
         (mln_alloc_t *pool, mln_lang_exp_t *exp, mln_lang_switchstm_t *switchstm, mln_u64_t line, mln_string_t *file), \
         (pool, exp, switchstm, line, file), \
{
    mln_lang_switch_t *ls;
    if ((ls = (mln_lang_switch_t *)mln_alloc_m(pool, sizeof(mln_lang_switch_t))) == NULL) {
        return NULL;
    }
    ls->file = NULL;
    if (file != NULL && (ls->file = mln_string_pool_dup(pool, file)) == NULL) {
        mln_alloc_free(ls);
        return NULL;
    }
    ls->line = line;
    ls->condition = exp;
    ls->switchstm = switchstm;
    return ls;
})

MLN_FUNC_VOID(static, void, mln_lang_switch_free, (void *data), (data), {
    if (data == NULL) return;
    mln_lang_switch_t *ls = (mln_lang_switch_t *)data;
    if (ls->condition != NULL) mln_lang_exp_free(ls->condition);
    if (ls->switchstm != NULL) mln_lang_switchstm_free(ls->switchstm);
    if (ls->file != NULL) mln_string_free(ls->file);
    mln_alloc_free(ls);
})


MLN_FUNC(static inline, mln_lang_switchstm_t *, mln_lang_switchstm_new, \
         (mln_alloc_t *pool, mln_lang_factor_t *factor, mln_lang_stm_t *stm, \
          mln_lang_switchstm_t *next, mln_u64_t line, mln_string_t *file), \
         (pool, factor, stm, next, line, file), \
{
    mln_lang_switchstm_t *ls;
    if ((ls = (mln_lang_switchstm_t *)mln_alloc_m(pool, sizeof(mln_lang_switchstm_t))) == NULL) {
        return NULL;
    }
    ls->file = NULL;
    if (file != NULL && (ls->file = mln_string_pool_dup(pool, file)) == NULL) {
        mln_alloc_free(ls);
        return NULL;
    }
    ls->line = line;
    ls->factor = factor;
    ls->stm = stm;
    ls->next = next;
    return ls;
})

MLN_FUNC_VOID(static, void, mln_lang_switchstm_free, (void *data), (data), {
    mln_lang_switchstm_t *ls, *next = (mln_lang_switchstm_t *)data;
    while (next != NULL) {
        ls = next;
        if (ls->factor != NULL) mln_lang_factor_free(ls->factor);
        if (ls->stm != NULL) mln_lang_stm_free(ls->stm);
        next = ls->next;
        if (ls->file != NULL) mln_string_free(ls->file);
        mln_alloc_free(ls);
    }
})


MLN_FUNC(static inline, mln_lang_while_t *, mln_lang_while_new, \
         (mln_alloc_t *pool, mln_lang_exp_t *exp, mln_lang_block_t *blockstm, mln_u64_t line, mln_string_t *file), \
         (pool, exp, blockstm, line, file), \
{
    mln_lang_while_t *lw;
    if ((lw = (mln_lang_while_t *)mln_alloc_m(pool, sizeof(mln_lang_while_t))) == NULL) {
        return NULL;
    }
    lw->file = NULL;
    if (file != NULL && (lw->file = mln_string_pool_dup(pool, file)) == NULL) {
        mln_alloc_free(lw);
        return NULL;
    }
    lw->line = line;
    lw->condition = exp;
    lw->blockstm = blockstm;
    return lw;
})

MLN_FUNC_VOID(static, void, mln_lang_while_free, (void *data), (data), {
    if (data == NULL) return;
    mln_lang_while_t *lw = (mln_lang_while_t *)data;
    if (lw->condition != NULL) mln_lang_exp_free(lw->condition);
    if (lw->blockstm != NULL) mln_lang_block_free(lw->blockstm);
    if (lw->file != NULL) mln_string_free(lw->file);
    mln_alloc_free(lw);
})


MLN_FUNC(static inline, mln_lang_for_t *, mln_lang_for_new, \
         (mln_alloc_t *pool, mln_lang_exp_t *init_exp, mln_lang_exp_t *condition, mln_lang_exp_t *mod_exp, \
          mln_lang_block_t *blockstm, mln_u64_t line, mln_string_t *file), \
         (pool, init_exp, condition, mod_exp, blockstm, line, file), \
{
    mln_lang_for_t *lf;
    if ((lf = (mln_lang_for_t *)mln_alloc_m(pool, sizeof(mln_lang_for_t))) == NULL) {
        return NULL;
    }
    lf->file = NULL;
    if (file != NULL && (lf->file = mln_string_pool_dup(pool, file)) == NULL) {
        mln_alloc_free(lf);
        return NULL;
    }
    lf->line = line;
    lf->init_exp = init_exp;
    lf->condition = condition;
    lf->mod_exp = mod_exp;
    lf->blockstm = blockstm;
    return lf;
})

MLN_FUNC_VOID(static, void, mln_lang_for_free, (void *data), (data), {
    if (data == NULL) return;
    mln_lang_for_t *lf = (mln_lang_for_t *)data;
    if (lf->init_exp != NULL) mln_lang_exp_free(lf->init_exp);
    if (lf->condition != NULL) mln_lang_exp_free(lf->condition);
    if (lf->mod_exp != NULL) mln_lang_exp_free(lf->mod_exp);
    if (lf->blockstm != NULL) mln_lang_block_free(lf->blockstm);
    if (lf->file != NULL) mln_string_free(lf->file);
    mln_alloc_free(lf);
})


MLN_FUNC(static inline, mln_lang_if_t *, mln_lang_if_new, \
         (mln_alloc_t *pool, mln_lang_exp_t *condition, mln_lang_block_t *truestm, \
          mln_lang_block_t *falsestm, mln_u64_t line, mln_string_t *file), \
         (pool, condition, truestm, falsestm, line, file), \
{
    mln_lang_if_t *li;
    if ((li = (mln_lang_if_t *)mln_alloc_m(pool, sizeof(mln_lang_if_t))) == NULL) {
        return NULL;
    }
    li->file = NULL;
    if (file != NULL && (li->file = mln_string_pool_dup(pool, file)) == NULL) {
        mln_alloc_free(li);
        return NULL;
    }
    li->line = line;
    li->condition = condition;
    li->blockstm = truestm;
    li->elsestm = falsestm;
    return li;
})

MLN_FUNC_VOID(static, void, mln_lang_if_free, (void *data), (data), {
    if (data == NULL) return;
    mln_lang_if_t *li = (mln_lang_if_t *)data;
    if (li->condition != NULL) mln_lang_exp_free(li->condition);
    if (li->blockstm != NULL) mln_lang_block_free(li->blockstm);
    if (li->elsestm != NULL) mln_lang_block_free(li->elsestm);
    if (li->file != NULL) mln_string_free(li->file);
    mln_alloc_free(li);
})


MLN_FUNC(static inline, mln_lang_exp_t *, mln_lang_exp_new, \
         (mln_alloc_t *pool, mln_lang_assign_t *assign, mln_lang_exp_t *next, mln_u64_t line, mln_string_t *file), \
         (pool, assign, next, line, file), \
{
    mln_lang_exp_t *le;
    if ((le = (mln_lang_exp_t *)mln_alloc_m(pool, sizeof(mln_lang_exp_t))) == NULL) {
        return NULL;
    }
    le->file = NULL;
    if (file != NULL && (le->file = mln_string_pool_dup(pool, file)) == NULL) {
        mln_alloc_free(le);
        return NULL;
    }
    le->line = line;
    le->assign = assign;
    le->next = next;
    le->jump = NULL;
    le->type = 0;
    return le;
})

MLN_FUNC_VOID(static, void, mln_lang_exp_free, (void *data), (data), {
    mln_lang_exp_t *le, *next = (mln_lang_exp_t *)data;
    while (next != NULL) {
        le = next;
        if (le->assign != NULL) mln_lang_assign_free(le->assign);
        next = le->next;
        if (le->file != NULL) mln_string_free(le->file);
        mln_alloc_free(le);
    }
})


MLN_FUNC(static inline, mln_lang_assign_t *, mln_lang_assign_new, \
         (mln_alloc_t *pool, mln_lang_logiclow_t *left, mln_lang_assign_op_t op, \
          mln_lang_assign_t *right, mln_u64_t line, mln_string_t *file), \
         (pool, left, op, right, line, file), \
{
    mln_lang_assign_t *la;
    if ((la = (mln_lang_assign_t *)mln_alloc_m(pool, sizeof(mln_lang_assign_t))) == NULL) {
        return NULL;
    }
    la->file = NULL;
    if (file != NULL && (la->file = mln_string_pool_dup(pool, file)) == NULL) {
        mln_alloc_free(la);
        return NULL;
    }
    la->line = line;
    la->left = left;
    la->op = op;
    la->right = right;
    la->jump = NULL;
    la->type = 0;
    return la;
})

MLN_FUNC_VOID(static, void, mln_lang_assign_free, (void *data), (data), {
    mln_lang_assign_t *la, *right = (mln_lang_assign_t *)data;
    while (right != NULL) {
        la = right;
        if (la->left != NULL) mln_lang_logiclow_free(la->left);
        right = la->right;
        if (la->file != NULL) mln_string_free(la->file);
        mln_alloc_free(la);
    }
})


MLN_FUNC(static inline, mln_lang_assign_tmp_t *, mln_lang_assign_tmp_new, \
         (mln_alloc_t *pool, mln_lang_assign_op_t op, mln_lang_assign_t *assign), \
         (pool, op, assign), \
{
    mln_lang_assign_tmp_t *lat;
    if ((lat = (mln_lang_assign_tmp_t *)mln_alloc_m(pool, sizeof(mln_lang_assign_tmp_t))) == NULL) {
        return NULL;
    }
    lat->op = op;
    lat->assign = assign;
    return lat;
})

MLN_FUNC_VOID(static, void, mln_lang_assign_tmp_free, (void *data), (data), {
    if (data == NULL) return;
    mln_lang_assign_tmp_t *lat = (mln_lang_assign_tmp_t *)data;
    if (lat->assign != NULL) mln_lang_assign_free(lat->assign);
    mln_alloc_free(lat);
})


MLN_FUNC(static inline, mln_lang_logiclow_t *, mln_lang_logiclow_new, \
         (mln_alloc_t *pool, mln_lang_logichigh_t *left, mln_lang_logiclow_op_t op, \
          mln_lang_logiclow_t *right, mln_u64_t line, mln_string_t *file), \
         (pool, left, op, right, line, file), \
{
    mln_lang_logiclow_t *ll;
    if ((ll = (mln_lang_logiclow_t *)mln_alloc_m(pool, sizeof(mln_lang_logiclow_t))) == NULL) {
        return NULL;
    }
    ll->file = NULL;
    if (file != NULL && (ll->file = mln_string_pool_dup(pool, file)) == NULL) {
        mln_alloc_free(ll);
        return NULL;
    }
    ll->line = line;
    ll->left = left;
    ll->op = op;
    ll->right = right;
    ll->jump = NULL;
    ll->type = 0;
    return ll;
})

MLN_FUNC_VOID(static, void, mln_lang_logiclow_free, (void *data), (data), {
    mln_lang_logiclow_t *ll, *right = (mln_lang_logiclow_t *)data;
    while (right != NULL) {
        ll = right;
        if (ll->left != NULL) mln_lang_logichigh_free(ll->left);
        right = ll->right;
        if (ll->file != NULL) mln_string_free(ll->file);
        mln_alloc_free(ll);
    }
})


MLN_FUNC(static inline, mln_lang_logiclow_tmp_t *, mln_lang_logiclow_tmp_new, \
         (mln_alloc_t *pool, mln_lang_logiclow_op_t op, mln_lang_logiclow_t *logiclow), \
         (pool, op, logiclow), \
{
    mln_lang_logiclow_tmp_t *llt;
    if ((llt = (mln_lang_logiclow_tmp_t *)mln_alloc_m(pool, sizeof(mln_lang_logiclow_tmp_t))) == NULL) {
        return NULL;
    }
    llt->op = op;
    llt->logiclow = logiclow;
    return llt;
})

MLN_FUNC_VOID(static, void, mln_lang_logiclow_tmp_free, (void *data), (data), {
    if (data == NULL) return;
    mln_lang_logiclow_tmp_t *llt = (mln_lang_logiclow_tmp_t *)data;
    if (llt->logiclow != NULL) mln_lang_logiclow_free(llt->logiclow);
    mln_alloc_free(llt);
})


MLN_FUNC(static inline, mln_lang_logichigh_t *, mln_lang_logichigh_new, \
         (mln_alloc_t *pool, mln_lang_relativelow_t *left, mln_lang_logichigh_op_t op, \
          mln_lang_logichigh_t *right, mln_u64_t line, mln_string_t *file), \
         (pool, left, op, right, line, file), \
{
    mln_lang_logichigh_t *ll;
    if ((ll = (mln_lang_logichigh_t *)mln_alloc_m(pool, sizeof(mln_lang_logichigh_t))) == NULL) {
        return NULL;
    }
    ll->file = NULL;
    if (file != NULL && (ll->file = mln_string_pool_dup(pool, file)) == NULL) {
        mln_alloc_free(ll);
        return NULL;
    }
    ll->line = line;
    ll->left = left;
    ll->op = op;
    ll->right = right;
    ll->jump = NULL;
    ll->type = 0;
    return ll;
})

MLN_FUNC_VOID(static, void, mln_lang_logichigh_free, (void *data), (data), {
    mln_lang_logichigh_t *ll, *right = (mln_lang_logichigh_t *)data;
    while (right != NULL) {
        ll = right;
        if (ll->left != NULL) mln_lang_relativelow_free(ll->left);
        right = ll->right;
        if (ll->file != NULL) mln_string_free(ll->file);
        mln_alloc_free(ll);
    }
})


MLN_FUNC(static inline, mln_lang_logichigh_tmp_t *, mln_lang_logichigh_tmp_new, \
         (mln_alloc_t *pool, mln_lang_logichigh_op_t op, mln_lang_logichigh_t *logichigh), \
         (pool, op, logichigh), \
{
    mln_lang_logichigh_tmp_t *llt;
    if ((llt = (mln_lang_logichigh_tmp_t *)mln_alloc_m(pool, sizeof(mln_lang_logichigh_tmp_t))) == NULL) {
        return NULL;
    }
    llt->op = op;
    llt->logichigh = logichigh;
    return llt;
})

MLN_FUNC_VOID(static, void, mln_lang_logichigh_tmp_free, (void *data), (data), {
    if (data == NULL) return;
    mln_lang_logichigh_tmp_t *llt = (mln_lang_logichigh_tmp_t *)data;
    if (llt->logichigh != NULL) mln_lang_logichigh_free(llt->logichigh);
    mln_alloc_free(llt);
})


MLN_FUNC(static inline, mln_lang_relativelow_t *, mln_lang_relativelow_new, \
         (mln_alloc_t *pool, mln_lang_relativehigh_t *left, mln_lang_relativelow_op_t op, \
          mln_lang_relativelow_t *right, mln_u64_t line, mln_string_t *file), \
         (pool, left, op, right, line, file), \
{
    mln_lang_relativelow_t *lr;
    if ((lr = (mln_lang_relativelow_t *)mln_alloc_m(pool, sizeof(mln_lang_relativelow_t))) == NULL) {
        return NULL;
    }
    lr->file = NULL;
    if (file != NULL && (lr->file = mln_string_pool_dup(pool, file)) == NULL) {
        mln_alloc_free(lr);
        return NULL;
    }
    lr->line = line;
    lr->left = left;
    lr->op = op;
    lr->right = right;
    lr->jump = NULL;
    lr->type = 0;
    return lr;
})

MLN_FUNC_VOID(static, void, mln_lang_relativelow_free, (void *data), (data), {
    mln_lang_relativelow_t *lr, *right = (mln_lang_relativelow_t *)data;
    while (right != NULL) {
        lr = right;
        if (lr->left != NULL) mln_lang_relativehigh_free(lr->left);
        right = lr->right;
        if (lr->file != NULL) mln_string_free(lr->file);
        mln_alloc_free(lr);
    }
})


MLN_FUNC(static inline, mln_lang_relativelow_tmp_t *, mln_lang_relativelow_tmp_new, \
         (mln_alloc_t *pool, mln_lang_relativelow_op_t op, mln_lang_relativelow_t *relativelow), \
         (pool, op, relativelow), \
{
    mln_lang_relativelow_tmp_t *lrt;
    if ((lrt = (mln_lang_relativelow_tmp_t *)mln_alloc_m(pool, sizeof(mln_lang_relativelow_tmp_t))) == NULL) {
        return NULL;
    }
    lrt->op = op;
    lrt->relativelow = relativelow;
    return lrt;
})

MLN_FUNC_VOID(static, void, mln_lang_relativelow_tmp_free, (void *data), (data), {
    if (data == NULL) return;
    mln_lang_relativelow_tmp_t *lrt = (mln_lang_relativelow_tmp_t *)data;
    if (lrt->relativelow != NULL) mln_lang_relativelow_free(lrt->relativelow);
    mln_alloc_free(lrt);
})


MLN_FUNC(static inline, mln_lang_relativehigh_t *, mln_lang_relativehigh_new, \
         (mln_alloc_t *pool, mln_lang_move_t *left, mln_lang_relativehigh_op_t op, \
          mln_lang_relativehigh_t *right, mln_u64_t line, mln_string_t *file), \
         (pool, left, op, right, line, file), \
{
    mln_lang_relativehigh_t *lr;
    if ((lr = (mln_lang_relativehigh_t *)mln_alloc_m(pool, sizeof(mln_lang_relativehigh_t))) == NULL) {
        return NULL;
    }
    lr->file = NULL;
    if (file != NULL && (lr->file = mln_string_pool_dup(pool, file)) == NULL) {
        mln_alloc_free(lr);
        return NULL;
    }
    lr->line = line;
    lr->left = left;
    lr->op = op;
    lr->right = right;
    lr->jump = NULL;
    lr->type = 0;
    return lr;
})

MLN_FUNC_VOID(static, void, mln_lang_relativehigh_free, (void *data), (data), {
    mln_lang_relativehigh_t *lr, *right = (mln_lang_relativehigh_t *)data;
    while (right !=  NULL) {
        lr = right;
        if (lr->left != NULL) mln_lang_move_free(lr->left);
        right = lr->right;
        if (lr->file != NULL) mln_string_free(lr->file);
        mln_alloc_free(lr);
    }
})


MLN_FUNC(static inline, mln_lang_relativehigh_tmp_t *, mln_lang_relativehigh_tmp_new, \
         (mln_alloc_t *pool, mln_lang_relativehigh_op_t op, mln_lang_relativehigh_t *relativehigh), \
         (pool, op, relativehigh), \
{
    mln_lang_relativehigh_tmp_t *lrt;
    if ((lrt = (mln_lang_relativehigh_tmp_t *)mln_alloc_m(pool, sizeof(mln_lang_relativehigh_tmp_t))) == NULL) {
        return NULL;
    }
    lrt->op = op;
    lrt->relativehigh = relativehigh;
    return lrt;
})

MLN_FUNC_VOID(static, void, mln_lang_relativehigh_tmp_free, (void *data), (data), {
    if (data == NULL) return;
    mln_lang_relativehigh_tmp_t *lrt = (mln_lang_relativehigh_tmp_t *)data;
    if (lrt->relativehigh != NULL) mln_lang_relativehigh_free(lrt->relativehigh);
    mln_alloc_free(lrt);
})


MLN_FUNC(static inline, mln_lang_move_t *, mln_lang_move_new, \
         (mln_alloc_t *pool, mln_lang_addsub_t *left, mln_lang_move_op_t op, \
          mln_lang_move_t *right, mln_u64_t line, mln_string_t *file), \
         (pool, left, op, right, line, file), \
{
    mln_lang_move_t *lm;
    if ((lm = (mln_lang_move_t *)mln_alloc_m(pool, sizeof(mln_lang_move_t))) == NULL) {
        return NULL;
    }
    lm->file = NULL;
    if (file != NULL && (lm->file = mln_string_pool_dup(pool, file)) == NULL) {
        mln_alloc_free(lm);
        return NULL;
    }
    lm->line = line;
    lm->left = left;
    lm->op = op;
    lm->right = right;
    lm->jump = NULL;
    lm->type = 0;
    return lm;
})

MLN_FUNC_VOID(static, void, mln_lang_move_free, (void *data), (data), {
    mln_lang_move_t *lm, *right = (mln_lang_move_t *)data;
    while (right != NULL) {
        lm = right;
        if (lm->left != NULL) mln_lang_addsub_free(lm->left);
        right = lm->right;
        if (lm->file != NULL) mln_string_free(lm->file);
        mln_alloc_free(lm);
    }
})


MLN_FUNC(static inline, mln_lang_move_tmp_t *, mln_lang_move_tmp_new, \
         (mln_alloc_t *pool, mln_lang_move_op_t op, mln_lang_move_t *move), \
         (pool, op, move), \
{
    mln_lang_move_tmp_t *lmt;
    if ((lmt = (mln_lang_move_tmp_t *)mln_alloc_m(pool, sizeof(mln_lang_move_tmp_t))) == NULL) {
        return NULL;
    }
    lmt->op = op;
    lmt->move = move;
    return lmt;
})

MLN_FUNC_VOID(static, void, mln_lang_move_tmp_free, (void *data), (data), {
    if (data == NULL) return;
    mln_lang_move_tmp_t *lmt = (mln_lang_move_tmp_t *)data;
    if (lmt->move != NULL) mln_lang_move_free(lmt->move);
    mln_alloc_free(lmt);
})


MLN_FUNC(static inline, mln_lang_addsub_t *, mln_lang_addsub_new, \
         (mln_alloc_t *pool, mln_lang_muldiv_t *left, mln_lang_addsub_op_t op, \
          mln_lang_addsub_t *right, mln_u64_t line, mln_string_t *file), \
         (pool, left, op, right, line, file), \
{
    mln_lang_addsub_t *la;
    if ((la = (mln_lang_addsub_t *)mln_alloc_m(pool, sizeof(mln_lang_addsub_t))) == NULL) {
        return NULL;
    }
    la->file = NULL;
    if (file != NULL && (la->file = mln_string_pool_dup(pool, file)) == NULL) {
        mln_alloc_free(la);
        return NULL;
    }
    la->line = line;
    la->left = left;
    la->op = op;
    la->right = right;
    la->jump = NULL;
    la->type = 0;
    return la;
})

MLN_FUNC_VOID(static, void, mln_lang_addsub_free, (void *data), (data), {
    mln_lang_addsub_t *la, *right = (mln_lang_addsub_t *)data;
    while (right != NULL) {
        la = right;
        if (la->left != NULL) mln_lang_muldiv_free(la->left);
        right = la->right;
        if (la->file != NULL) mln_string_free(la->file);
        mln_alloc_free(la);
    }
})


MLN_FUNC(static inline, mln_lang_addsub_tmp_t *, mln_lang_addsub_tmp_new, \
         (mln_alloc_t *pool, mln_lang_addsub_op_t op, mln_lang_addsub_t *addsub), \
         (pool, op, addsub), \
{
    mln_lang_addsub_tmp_t *lat;
    if ((lat = (mln_lang_addsub_tmp_t *)mln_alloc_m(pool, sizeof(mln_lang_addsub_tmp_t))) == NULL) {
        return NULL;
    }
    lat->op = op;
    lat->addsub = addsub;
    return lat;
})

MLN_FUNC_VOID(static, void, mln_lang_addsub_tmp_free, (void *data), (data), {
    if (data == NULL) return;
    mln_lang_addsub_tmp_t *lat = (mln_lang_addsub_tmp_t *)data;
    if (lat->addsub != NULL) mln_lang_addsub_free(lat->addsub);
    mln_alloc_free(lat);
})


MLN_FUNC(static inline, mln_lang_muldiv_t *, mln_lang_muldiv_new, \
         (mln_alloc_t *pool, mln_lang_not_t *left, mln_lang_muldiv_op_t op, \
          mln_lang_muldiv_t *right, mln_u64_t line, mln_string_t *file), \
         (pool, left, op, right, line, file), \
{
    mln_lang_muldiv_t *lm;
    if ((lm = (mln_lang_muldiv_t *)mln_alloc_m(pool, sizeof(mln_lang_muldiv_t))) == NULL) {
        return NULL;
    }
    lm->file = NULL;
    if (file != NULL && (lm->file = mln_string_pool_dup(pool, file)) == NULL) {
        mln_alloc_free(lm);
        return NULL;
    }
    lm->line = line;
    lm->left = left;
    lm->op = op;
    lm->right = right;
    lm->jump = NULL;
    lm->type = 0;
    return lm;
})

MLN_FUNC_VOID(static, void, mln_lang_muldiv_free, (void *data), (data), {
    mln_lang_muldiv_t *lm, *right = (mln_lang_muldiv_t *)data;
    while (right != NULL) {
        lm = right;
        if (lm->left != NULL) mln_lang_not_free(lm->left);
        right = lm->right;
        if (lm->file != NULL) mln_string_free(lm->file);
        mln_alloc_free(lm);
    }
})


MLN_FUNC(static inline, mln_lang_muldiv_tmp_t *, mln_lang_muldiv_tmp_new, \
         (mln_alloc_t *pool, mln_lang_muldiv_op_t op, mln_lang_muldiv_t *muldiv), \
         (pool, op, muldiv), \
{
    mln_lang_muldiv_tmp_t *lmt;
    if ((lmt = (mln_lang_muldiv_tmp_t *)mln_alloc_m(pool, sizeof(mln_lang_muldiv_tmp_t))) == NULL) {
        return NULL;
    }
    lmt->op = op;
    lmt->muldiv = muldiv;
    return lmt;
})

MLN_FUNC_VOID(static, void, mln_lang_muldiv_tmp_free, (void *data), (data), {
    if (data == NULL) return;
    mln_lang_muldiv_tmp_t *lmt = (mln_lang_muldiv_tmp_t *)data;
    if (lmt->muldiv != NULL) mln_lang_muldiv_free(lmt->muldiv);
    mln_alloc_free(lmt);
})


MLN_FUNC(static inline, mln_lang_not_t *, mln_lang_not_new, \
         (mln_alloc_t *pool, mln_lang_not_op_t op, void *data, mln_u64_t line, mln_string_t *file), \
         (pool, op, data, line, file), \
{
    mln_lang_not_t *ln;
    if ((ln = (mln_lang_not_t *)mln_alloc_m(pool, sizeof(mln_lang_not_t))) == NULL) {
        return NULL;
    }
    ln->file = NULL;
    if (file != NULL && (ln->file = mln_string_pool_dup(pool, file)) == NULL) {
        mln_alloc_free(ln);
        return NULL;
    }
    ln->line = line;
    ln->op = op;
    switch (op) {
        case M_NOT_NOT:
            ln->right.not = (mln_lang_not_t *)data;
            break;
        default: /* M_NOT_NONE */
            ln->right.suffix = (mln_lang_suffix_t *)data;
            break;
    }
    ln->jump = NULL;
    ln->type = 0;
    return ln;
})

MLN_FUNC_VOID(static, void, mln_lang_not_free, (void *data), (data), {
    if (data == NULL) return;
    mln_lang_not_t *ln = (mln_lang_not_t *)data, *fr;

again:
    ASSERT(ln != NULL);
    fr = ln;
    if (ln->op == M_NOT_NOT) {
        ln = ln->right.not;
        if (fr->file != NULL) mln_string_free(fr->file);
        mln_alloc_free(fr);
        goto again;
    } else {
        mln_lang_suffix_free(fr->right.suffix);
        if (fr->file != NULL) mln_string_free(fr->file);
        mln_alloc_free(fr);
    }
})


MLN_FUNC(static inline, mln_lang_suffix_t *, mln_lang_suffix_new, \
         (mln_alloc_t *pool, mln_lang_locate_t *left, mln_lang_suffix_op_t op, mln_u64_t line, mln_string_t *file), \
         (pool, left, op, line, file), \
{
    mln_lang_suffix_t *ls;
    if ((ls = (mln_lang_suffix_t *)mln_alloc_m(pool, sizeof(mln_lang_suffix_t))) == NULL) {
        return NULL;
    }
    ls->file = NULL;
    if (file != NULL && (ls->file = mln_string_pool_dup(pool, file)) == NULL) {
        mln_alloc_free(ls);
        return NULL;
    }
    ls->line = line;
    ls->left = left;
    ls->op = op;
    ls->jump = NULL;
    ls->type = 0;
    return ls;
})

MLN_FUNC_VOID(static, void, mln_lang_suffix_free, (void *data), (data), {
    if (data == NULL) return;
    mln_lang_suffix_t *ls = (mln_lang_suffix_t *)data;
    if (ls->left != NULL) mln_lang_locate_free(ls->left);
    if (ls->file != NULL) mln_string_free(ls->file);
    mln_alloc_free(ls);
})


MLN_FUNC(static inline, mln_lang_suffix_tmp_t *, mln_lang_suffix_tmp_new, \
         (mln_alloc_t *pool, mln_lang_suffix_op_t op), (pool, op), \
{
    mln_lang_suffix_tmp_t *lst;
    if ((lst = (mln_lang_suffix_tmp_t *)mln_alloc_m(pool, sizeof(mln_lang_suffix_tmp_t))) == NULL) {
        return NULL;
    }
    lst->op = op;
    return lst;
})

MLN_FUNC_VOID(static, void, mln_lang_suffix_tmp_free, (void *data), (data), {
    if (data == NULL) return;
    mln_alloc_free(data);
})


MLN_FUNC(static inline, mln_lang_locate_t *, mln_lang_locate_new, \
         (mln_alloc_t *pool, mln_lang_spec_t *left, mln_lang_locate_op_t op, void *right, \
          mln_lang_locate_tmp_t *next, mln_u64_t line, mln_string_t *file), \
         (pool, left, op, right, next, line, file), \
{
    mln_lang_locate_t *ll, *n;
    if ((ll = (mln_lang_locate_t *)mln_alloc_m(pool, sizeof(mln_lang_locate_t))) == NULL) {
        return NULL;
    }
    ll->file = NULL;
    if (file != NULL && (ll->file = mln_string_pool_dup(pool, file)) == NULL) {
        mln_alloc_free(ll);
        return NULL;
    }
    ll->line = line;
    ll->left = left;
    ll->op = op;
    switch (op) {
        case M_LOCATE_INDEX:
        case M_LOCATE_FUNC:
            ll->right.exp = (mln_lang_exp_t *)right;
            break;
        case M_LOCATE_PROPERTY:
            ll->right.id = (mln_string_t *)right;
            break;
        default:
            ll->right.exp = NULL;
            break;
    }
    ll->jump = NULL;
    ll->type = 0;
    ll->next = NULL;
    if (next != NULL) {
        mln_lang_locate_t *tmp = ll;
        while (next != NULL) {
            if ((n = (mln_lang_locate_t *)mln_alloc_m(pool, sizeof(mln_lang_locate_t))) == NULL) {
                mln_lang_locate_free(ll);
                return NULL;
            }
            n->file = NULL;
            if (file != NULL && (n->file = mln_string_pool_dup(pool, file)) == NULL) {
                mln_alloc_free(n);
                mln_lang_locate_free(ll);
                return NULL;
            }
            n->line = line;
            n->left = NULL;
            n->op = next->op;
            n->next = NULL;
            switch (n->op) {
                case M_LOCATE_INDEX:
                case M_LOCATE_FUNC:
                    n->right.exp = next->locate.exp;
                    next->locate.exp = NULL;
                    break;
                case M_LOCATE_PROPERTY:
                    n->right.id = next->locate.id;
                    next->locate.id = NULL;
                    break;
                default:
                    n->right.exp = NULL;
                    break;
            }
            tmp->next = n;
            tmp = n;
            next = next->next;
        }
    }
    return ll;
})

MLN_FUNC_VOID(static, void, mln_lang_locate_free, (void *data), (data), {
    if (data == NULL) return;
    mln_lang_locate_t *ll = (mln_lang_locate_t *)data, *next;
    mln_lang_locate_op_t op;

lp:
    op = ll->op;
    if (ll->left != NULL) mln_lang_spec_free(ll->left);
    switch (op) {
        case M_LOCATE_INDEX:
        case M_LOCATE_FUNC:
            if (ll->right.exp != NULL) mln_lang_exp_free(ll->right.exp);
            break;
        case M_LOCATE_PROPERTY:
            if (ll->right.id != NULL) mln_string_free(ll->right.id);
            break;
        default:
            break;
    }
    next = ll->next;
    if (ll->file != NULL) mln_string_free(ll->file);
    mln_alloc_free(ll);
    if (next != NULL) {
        ll = next;
        goto lp;
    }
})


MLN_FUNC(static inline, mln_lang_locate_tmp_t *, mln_lang_locate_tmp_new, \
         (mln_alloc_t *pool, mln_lang_locate_op_t op, void *data, mln_lang_locate_tmp_t *next), \
         (pool, op, data, next), \
{
    mln_lang_locate_tmp_t *llt;
    if ((llt = (mln_lang_locate_tmp_t *)mln_alloc_m(pool, sizeof(mln_lang_locate_tmp_t))) == NULL) {
        return NULL;
    }
    llt->op = op;
    switch (op) {
        case M_LOCATE_INDEX:
        case M_LOCATE_FUNC:
            llt->locate.exp = (mln_lang_exp_t *)data;
            break;
        case M_LOCATE_PROPERTY:
            llt->locate.id = (mln_string_t *)data;
            break;
        default:
            llt->locate.exp = NULL;
            break;
    }
    llt->next = next;
    return llt;
})

MLN_FUNC_VOID(static, void, mln_lang_locate_tmp_free, (void *data), (data), {
    if (data == NULL) return;
    mln_lang_locate_tmp_t *llt = (mln_lang_locate_tmp_t *)data;
    mln_lang_locate_op_t op = llt->op;
    switch (op) {
        case M_LOCATE_INDEX:
        case M_LOCATE_FUNC:
            if (llt->locate.exp != NULL) mln_lang_assign_free(llt->locate.exp);
            break;
        case M_LOCATE_PROPERTY:
            if (llt->locate.id != NULL) mln_string_free(llt->locate.id);
            break;
        default:
            break;
    }
    if (llt->next != NULL) mln_lang_locate_tmp_free(llt->next);
    mln_alloc_free(llt);
})

MLN_FUNC(static inline, mln_lang_spec_t *, mln_lang_spec_new, \
         (mln_alloc_t *pool, mln_lang_spec_op_t op, void *data, mln_u64_t line, mln_string_t *file), \
         (pool, op, data, line, file), \
{
    mln_lang_spec_t *ls;
    if ((ls = (mln_lang_spec_t *)mln_alloc_m(pool, sizeof(mln_lang_spec_t))) == NULL) {
        return NULL;
    }
    ls->file = NULL;
    if (file != NULL && (ls->file = mln_string_pool_dup(pool, file)) == NULL) {
        mln_alloc_free(ls);
        return NULL;
    }
    ls->line = line;
    ls->op = op;
    switch (op) {
        case M_SPEC_NEGATIVE:
        case M_SPEC_REVERSE:
        case M_SPEC_REFER:
        case M_SPEC_INC:
        case M_SPEC_DEC:
            ls->data.spec = (mln_lang_spec_t *)data;
            break;
        case M_SPEC_NEW:
            ls->data.set_name = (mln_string_t *)data;
            break;
        case M_SPEC_PARENTH:
            ls->data.exp = (mln_lang_exp_t *)data;
            break;
        default: /* M_SPEC_FACTOR */
            ls->data.factor = (mln_lang_factor_t *)data;
            break;
    }
    return ls;
})

MLN_FUNC_VOID(static, void, mln_lang_spec_free, (void *data), (data), {
    if (data == NULL) return;
    mln_lang_spec_t *ls, *right = (mln_lang_spec_t *)data;
    mln_lang_spec_op_t op;
    while (right != NULL) {
        ls = right;
        op = ls->op;
        switch (op) {
            case M_SPEC_NEGATIVE:
            case M_SPEC_REVERSE:
            case M_SPEC_REFER:
            case M_SPEC_INC:
            case M_SPEC_DEC:
                right = ls->data.spec;
                break;
            case M_SPEC_NEW:
                if (ls->data.set_name != NULL) mln_string_free(ls->data.set_name);
                right = NULL;
                break;
            case M_SPEC_PARENTH:
                if (ls->data.exp != NULL) mln_lang_exp_free(ls->data.exp);
                right = NULL;
                break;
            default: /* M_SPEC_FACTOR */
                if (ls->data.factor != NULL) mln_lang_factor_free(ls->data.factor);
                right = NULL;
                break;
        }
        if (ls->file != NULL) mln_string_free(ls->file);
        mln_alloc_free(ls);
    }
})


MLN_FUNC(static inline, mln_lang_factor_t *, mln_lang_factor_new, \
         (mln_alloc_t *pool, mln_lang_factor_type_t type, void *data, mln_u64_t line, mln_string_t *file), \
         (pool, type, data, line, file), \
{
    mln_lang_factor_t *lf;
    if ((lf = (mln_lang_factor_t *)mln_alloc_m(pool, sizeof(mln_lang_factor_t))) == NULL) {
        return NULL;
    }
    lf->file = NULL;
    if (file != NULL && (lf->file = mln_string_pool_dup(pool, file)) == NULL) {
        mln_alloc_free(lf);
        return NULL;
    }
    lf->line = line;
    lf->type = type;
    switch (type) {
        case M_FACTOR_BOOL:
            lf->data.b = *(mln_u8ptr_t)data;
            break;
        case M_FACTOR_STRING:
        case M_FACTOR_ID:
            lf->data.s_id = (mln_string_t *)data;
            break;
        case M_FACTOR_INT:
            lf->data.i = *(mln_s64_t *)data;
            break;
        case M_FACTOR_REAL:
            lf->data.f = *(double *)data;
            break;
        case M_FACTOR_ARRAY:
            lf->data.array = (mln_lang_elemlist_t *)data;
            break;
        default:
            lf->data.ptr = NULL;
            break;
    }
    return lf;
})

MLN_FUNC_VOID(static, void, mln_lang_factor_free, (void *data), (data), {
    if (data == NULL) return;
    mln_lang_factor_t *lf = (mln_lang_factor_t *)data;
    mln_lang_factor_type_t type = lf->type;
    switch (type) {
        case M_FACTOR_STRING:
        case M_FACTOR_ID:
            if (lf->data.s_id != NULL) mln_string_free(lf->data.s_id);
            break;
        case M_FACTOR_ARRAY:
            if (lf->data.array != NULL) mln_lang_elemlist_free(lf->data.array);
            break;
        case M_FACTOR_BOOL:
        case M_FACTOR_INT:
        case M_FACTOR_REAL:
        default:
            break;
    }
    if (lf->file != NULL) mln_string_free(lf->file);
    mln_alloc_free(lf);
})


MLN_FUNC(static inline, mln_lang_elemlist_t *, mln_lang_elemlist_new, \
         (mln_alloc_t *pool, mln_lang_assign_t *key, mln_lang_assign_t *val, \
          mln_lang_elemlist_t *next, mln_u64_t line, mln_string_t *file), \
         (pool, key, val, next, line, file), \
{
    mln_lang_elemlist_t *le;
    if ((le = (mln_lang_elemlist_t *)mln_alloc_m(pool, sizeof(mln_lang_elemlist_t))) == NULL) {
        return NULL;
    }
    le->file = NULL;
    if (file != NULL && (le->file = mln_string_pool_dup(pool, file)) == NULL) {
        mln_alloc_free(le);
        return NULL;
    }
    le->line = line;
    le->key = key;
    le->val = val;
    le->next = next;
    return le;
})

MLN_FUNC_VOID(static, void, mln_lang_elemlist_free, (void *data), (data), {
    mln_lang_elemlist_t *le, *next = (mln_lang_elemlist_t *)data;
    while (next != NULL) {
        le = next;
        if (le->key != NULL) mln_lang_assign_free(le->key);
        if (le->val != NULL) mln_lang_assign_free(le->val);
        next = le->next;
        if (le->file != NULL) mln_string_free(le->file);
        mln_alloc_free(le);
    }
})

/*
 * semantic
 */
MLN_FUNC(static, int, mln_lang_semantic_start, \
         (mln_factor_t *left, mln_factor_t **right, void *data), \
         (left, right, data), \
{
    left->data = right[0]->data;
    left->nonterm_free_handler = right[0]->nonterm_free_handler;
    right[0]->data = NULL;
    return 0;
})

MLN_FUNC(static, int, mln_lang_semantic_stm_block, \
         (mln_factor_t *left, mln_factor_t **right, void *data), \
         (left, right, data), \
{
    mln_alloc_t *pool = (mln_alloc_t *)data;
    mln_lang_stm_t *stm;
    if ((stm = mln_lang_stm_new(pool, right[0]->data, M_STM_BLOCK, (mln_lang_stm_t *)(right[1]->data), left->line, left->file)) == NULL) {
        return -1;
    }
    left->data = stm;
    left->nonterm_free_handler = mln_lang_stm_free;
    right[0]->data = NULL;
    right[1]->data = NULL;
    return 0;
})

MLN_FUNC(static, int, mln_lang_semantic_stmfunc, \
         (mln_factor_t *left, mln_factor_t **right, void *data), \
         (left, right, data), \
{
    mln_alloc_t *pool = (mln_alloc_t *)data;
    mln_lang_stm_t *stm;
    if ((stm = mln_lang_stm_new(pool, right[0]->data, M_STM_FUNC, (mln_lang_stm_t *)(right[1]->data), left->line, left->file)) == NULL) {
        return -1;
    }
    right[0]->data = NULL;
    right[1]->data = NULL;
    left->data = stm;
    left->nonterm_free_handler = mln_lang_stm_free;
    return 0;
})

MLN_FUNC(static, int, mln_lang_semantic_stmset, \
         (mln_factor_t *left, mln_factor_t **right, void *data), \
         (left, right, data), \
{
    mln_alloc_t *pool = (mln_alloc_t *)data;
    mln_lang_stm_t *stm;
    mln_lang_set_t *clas;
    mln_lang_struct_t *ls = (mln_lang_struct_t *)(right[0]->data);
    mln_string_t *name = mln_string_pool_dup(pool, ls->text);
    if (name == NULL) return -1;
    if ((clas = mln_lang_set_new(pool, \
                                 name, \
                                 (mln_lang_setstm_t *)(right[2]->data), \
                                 left->line, \
                                 left->file)) == NULL)
    {
        mln_string_free(name);
        return -1;
    }
    if ((stm =  mln_lang_stm_new(pool, clas, M_STM_SET, (mln_lang_stm_t *)(right[4]->data), left->line, left->file)) == NULL) {
        mln_lang_set_free(clas);
        return -1;
    }
    right[2]->data = NULL;
    right[4]->data = NULL;
    left->data = stm;
    left->nonterm_free_handler = mln_lang_stm_free;
    return 0;
})

MLN_FUNC(static, int, mln_lang_semantic_setstm_var, \
         (mln_factor_t *left, mln_factor_t **right, void *data), \
         (left, right, data), \
{
    mln_alloc_t *pool = (mln_alloc_t *)data;
    mln_lang_struct_t *ls = (mln_lang_struct_t *)(right[0]->data);
    mln_string_t *var = mln_string_pool_dup(pool, ls->text);
    if (var == NULL) return -1;
    mln_lang_setstm_t *lc = mln_lang_setstm_new(pool, \
                                                var, \
                                                M_SETSTM_VAR, \
                                                (mln_lang_setstm_t *)(right[2]->data), \
                                                left->line, \
                                                left->file);
    if (lc == NULL) {
        mln_string_free(var);
        return -1;
    }
    left->data = lc;
    left->nonterm_free_handler = mln_lang_setstm_free;
    right[2]->data = NULL;
    return 0;
})

MLN_FUNC(static, int, mln_lang_semantic_setstm_func, \
         (mln_factor_t *left, mln_factor_t **right, void *data), \
         (left, right, data), \
{
    mln_alloc_t *pool = (mln_alloc_t *)data;
    mln_lang_setstm_t *lc = mln_lang_setstm_new(pool, \
                                                right[0]->data, \
                                                M_SETSTM_FUNC, \
                                                (mln_lang_setstm_t *)(right[1]->data), \
                                                left->line, \
                                                left->file);
    if (lc == NULL) return -1;
    left->data = lc;
    left->nonterm_free_handler = mln_lang_setstm_free;
    right[0]->data = NULL;
    right[1]->data = NULL;
    return 0;
})

MLN_FUNC(static, int, mln_lang_semantic_blockstmexp, \
         (mln_factor_t *left, mln_factor_t **right, void *data), \
         (left, right, data), \
{
    mln_alloc_t *pool = (mln_alloc_t *)data;
    mln_lang_block_t *lb = mln_lang_block_new(pool, right[0]->data, M_BLOCK_EXP, left->line, left->file);
    if (lb == NULL) return -1;
    left->data = lb;
    left->nonterm_free_handler = mln_lang_block_free;
    right[0]->data = NULL;
    return 0;
})

MLN_FUNC(static, int, mln_lang_semantic_blockstmstm, \
         (mln_factor_t *left, mln_factor_t **right, void *data), \
         (left, right, data), \
{
    mln_alloc_t *pool = (mln_alloc_t *)data;
    mln_lang_block_t *lb = mln_lang_block_new(pool, right[1]->data, M_BLOCK_STM, left->line, left->file);
    if (lb == NULL) return -1;
    left->data = lb;
    left->nonterm_free_handler = mln_lang_block_free;
    right[1]->data = NULL;
    return 0;
})

MLN_FUNC(static, int, mln_lang_semantic_labelstm, \
         (mln_factor_t *left, mln_factor_t **right, void *data), \
         (left, right, data), \
{
    mln_alloc_t *pool = (mln_alloc_t *)data;
    mln_lang_struct_t *ls = (mln_lang_struct_t *)(right[0]->data);
    mln_string_t *label = mln_string_pool_dup(pool, ls->text);
    if (label == NULL) return -1;
    mln_lang_stm_t *stm = mln_lang_stm_new(pool, label, M_STM_LABEL, (mln_lang_stm_t *)(right[2]->data), left->line, left->file);
    if (stm == NULL) {
        mln_string_free(label);
        return -1;
    }
    left->data = stm;
    left->nonterm_free_handler = mln_lang_stm_free;
    right[2]->data = NULL;
    return 0;
})

MLN_FUNC(static, int, mln_lang_semantic_whilestm, \
         (mln_factor_t *left, mln_factor_t **right, void *data), \
         (left, right, data), \
{
    mln_alloc_t *pool = (mln_alloc_t *)data;
    mln_lang_while_t *w = mln_lang_while_new(pool, \
                                             (mln_lang_exp_t *)(right[2]->data), \
                                             (mln_lang_block_t *)(right[4]->data), \
                                             left->line, \
                                             left->file);
    if (w == NULL) return -1;
    mln_lang_stm_t *stm = mln_lang_stm_new(pool, w, M_STM_WHILE, (mln_lang_stm_t *)(right[5]->data), left->line, left->file);
    if (stm == NULL) {
        mln_lang_while_free(w);
        return -1;
    }
    left->data = stm;
    left->nonterm_free_handler = mln_lang_stm_free;
    right[2]->data = NULL;
    right[4]->data = NULL;
    right[5]->data = NULL;
    return 0;
})


MLN_FUNC(static, int, mln_lang_semantic_forstm, \
         (mln_factor_t *left, mln_factor_t **right, void *data), \
         (left, right, data), \
{
    mln_alloc_t *pool = (mln_alloc_t *)data;
    mln_lang_for_t *f = mln_lang_for_new(pool, \
                                         (mln_lang_exp_t *)(right[2]->data), \
                                         (mln_lang_exp_t *)(right[4]->data), \
                                         (mln_lang_exp_t *)(right[6]->data), \
                                         (mln_lang_block_t *)(right[8]->data), \
                                         left->line, \
                                         left->file);
    if (f == NULL) return -1;
    mln_lang_stm_t *stm = mln_lang_stm_new(pool, f, M_STM_FOR, (mln_lang_stm_t *)(right[9]->data), left->line, left->file);
    if (stm == NULL) {
        mln_lang_for_free(f);
        return -1;
    }
    left->data = stm;
    left->nonterm_free_handler = mln_lang_stm_free;
    right[2]->data = NULL;
    right[4]->data = NULL;
    right[6]->data = NULL;
    right[8]->data = NULL;
    right[9]->data = NULL;
    return 0;
})

MLN_FUNC(static, int, mln_lang_semantic_ifstm, \
         (mln_factor_t *left, mln_factor_t **right, void *data), \
         (left, right, data), \
{
    mln_alloc_t *pool = (mln_alloc_t *)data;
    mln_lang_if_t *i = mln_lang_if_new(pool, \
                                       (mln_lang_exp_t *)(right[2]->data), \
                                       (mln_lang_block_t *)(right[4]->data), \
                                       (mln_lang_block_t *)(right[5]->data), \
                                       left->line, \
                                       left->file);
    if (i == NULL) return -1;
    mln_lang_block_t *lb = mln_lang_block_new(pool, i, M_BLOCK_IF, left->line, left->file);
    if (lb == NULL) {
        mln_lang_if_free(i);
        return -1;
    }
    left->data = lb;
    left->nonterm_free_handler = mln_lang_block_free;
    right[2]->data = NULL;
    right[4]->data = NULL;
    right[5]->data = NULL;
    return 0;
})

MLN_FUNC(static, int, mln_lang_semantic_switchstm, \
         (mln_factor_t *left, mln_factor_t **right, void *data), \
         (left, right, data), \
{
    mln_alloc_t *pool = (mln_alloc_t *)data;
    mln_lang_switch_t *sw = mln_lang_switch_new(pool, \
                                                (mln_lang_exp_t *)(right[2]->data), \
                                                (mln_lang_switchstm_t *)(right[5]->data), \
                                                left->line, \
                                                left->file);
    if (sw == NULL) return -1;
    mln_lang_stm_t *stm = mln_lang_stm_new(pool, sw, M_STM_SWITCH, (mln_lang_stm_t *)(right[7]->data), left->line, left->file);
    if (stm == NULL) {
        mln_lang_switch_free(sw);
        return -1;
    }
    left->data = stm;
    left->nonterm_free_handler = mln_lang_stm_free;
    right[2]->data = NULL;
    right[5]->data = NULL;
    right[7]->data = NULL;
    return 0;
})

MLN_FUNC(static, int, mln_lang_semantic_funcdef, \
         (mln_factor_t *left, mln_factor_t **right, void *data), \
         (left, right, data), \
{
    mln_alloc_t *pool = (mln_alloc_t *)data;
    mln_lang_struct_t *ls = (mln_lang_struct_t *)(right[1]->data);
    mln_string_t *name = mln_string_pool_dup(pool, ls->text);
    if (name == NULL) return -1;
    mln_lang_funcdef_t *lf = mln_lang_funcdef_new(pool, \
                                                  name, \
                                                  (mln_lang_exp_t *)(right[3]->data), \
                                                  (mln_lang_exp_t *)(right[5]->data), \
                                                  (mln_lang_stm_t *)(right[7]->data), \
                                                  left->line, \
                                                  left->file);
    if (lf == NULL) {
        mln_string_free(name);
        return -1;
    }
    left->data = lf;
    left->nonterm_free_handler = mln_lang_funcdef_free;
    right[3]->data = NULL;
    right[5]->data = NULL;
    right[7]->data = NULL;
    return 0;
})

MLN_FUNC(static, int, mln_lang_semantic_funcdef_closure, \
         (mln_factor_t *left, mln_factor_t **right, void *data), \
         (left, right, data), \
{
    left->data = right[2]->data;
    left->nonterm_free_handler = right[2]->nonterm_free_handler;
    right[2]->data = NULL;
    return 0;
})

MLN_FUNC(static, int, mln_lang_semantic_switchstm__, \
         (mln_factor_t *left, mln_factor_t **right, void *data), \
         (left, right, data), \
{
    mln_alloc_t *pool = (mln_alloc_t *)data;
    mln_lang_switchstm_t *ls = mln_lang_switchstm_new(pool, \
                                                      (mln_lang_factor_t *)(right[0]->data), \
                                                      (mln_lang_stm_t *)(right[2]->data), \
                                                      (mln_lang_switchstm_t *)(right[3]->data), \
                                                      left->line, \
                                                      left->file);
    if (ls == NULL) return -1;
    left->data = ls;
    left->nonterm_free_handler = mln_lang_switchstm_free;
    right[0]->data = NULL;
    right[2]->data = NULL;
    right[3]->data = NULL;
    return 0;
})

MLN_FUNC(static, int, mln_lang_semantic_switchprefix, \
         (mln_factor_t *left, mln_factor_t **right, void *data), \
         (left, right, data), \
{
    left->data = right[1]->data;
    left->nonterm_free_handler = right[1]->nonterm_free_handler;
    right[1]->data = NULL;
    return 0;
})

MLN_FUNC(static, int, mln_lang_semantic_elsestm, \
         (mln_factor_t *left, mln_factor_t **right, void *data), \
         (left, right, data), \
{
    left->data = right[1]->data;
    left->nonterm_free_handler = right[1]->nonterm_free_handler;
    right[1]->data = NULL;
    return 0;
})

MLN_FUNC(static, int, mln_lang_semantic_continue, \
         (mln_factor_t *left, mln_factor_t **right, void *data), \
         (left, right, data), \
{
    mln_alloc_t *pool = (mln_alloc_t *)data;
    mln_lang_block_t *block = mln_lang_block_new(pool, NULL, M_BLOCK_CONTINUE, left->line, left->file);
    if (block == NULL) return -1;
    left->data = block;
    left->nonterm_free_handler = mln_lang_block_free;
    return 0;
})

MLN_FUNC(static, int, mln_lang_semantic_break, \
         (mln_factor_t *left, mln_factor_t **right, void *data), \
         (left, right, data), \
{
    mln_alloc_t *pool = (mln_alloc_t *)data;
    mln_lang_block_t *block = mln_lang_block_new(pool, NULL, M_BLOCK_BREAK, left->line, left->file);
    if (block == NULL) return -1;
    left->data = block;
    left->nonterm_free_handler = mln_lang_block_free;
    return 0;
})

MLN_FUNC(static, int, mln_lang_semantic_return, \
         (mln_factor_t *left, mln_factor_t **right, void *data), \
         (left, right, data), \
{
    mln_alloc_t *pool = (mln_alloc_t *)data;
    mln_lang_block_t *block = mln_lang_block_new(pool, right[1]->data, M_BLOCK_RETURN, left->line, left->file);
    if (block == NULL) return -1;
    left->data = block;
    left->nonterm_free_handler = mln_lang_block_free;
    right[1]->data = NULL;
    return 0;
})

MLN_FUNC(static, int, mln_lang_semantic_goto, \
         (mln_factor_t *left, mln_factor_t **right, void *data), \
         (left, right, data), \
{
    mln_alloc_t *pool = (mln_alloc_t *)data;
    mln_lang_struct_t *ls = (mln_lang_struct_t *)(right[1]->data);
    mln_string_t *pos = mln_string_pool_dup(pool, ls->text);
    if (pos == NULL) return -1;
    mln_lang_block_t *block = mln_lang_block_new(pool, pos, M_BLOCK_GOTO, left->line, left->file);
    if (block == NULL) {
        mln_string_free(pos);
        return -1;
    }
    left->data = block;
    left->nonterm_free_handler = mln_lang_block_free;
    return 0;
})

MLN_FUNC(static, int, mln_lang_semantic_exp, \
         (mln_factor_t *left, mln_factor_t **right, void *data), \
         (left, right, data), \
{
    mln_alloc_t *pool = (mln_alloc_t *)data;
    mln_lang_exp_t *exp = mln_lang_exp_new(pool, \
                                           (mln_lang_assign_t *)(right[0]->data), \
                                           (mln_lang_exp_t *)(right[1]->data), \
                                           left->line, \
                                           left->file);
    if (exp == NULL) return -1;
    left->data = exp;
    left->nonterm_free_handler = mln_lang_exp_free;
    right[0]->data = NULL;
    right[1]->data = NULL;
    return 0;
})

MLN_FUNC(static, int, mln_lang_semantic_explist, \
         (mln_factor_t *left, mln_factor_t **right, void *data), \
         (left, right, data), \
{
    left->data = right[1]->data;
    left->nonterm_free_handler = right[1]->nonterm_free_handler;
    right[1]->data = NULL;
    return 0;
})

MLN_FUNC(static, int, mln_lang_semantic_assignexp, \
         (mln_factor_t *left, mln_factor_t **right, void *data), \
         (left, right, data), \
{
    mln_alloc_t *pool = (mln_alloc_t *)data;
    mln_lang_assign_t *assign, *r = NULL;
    mln_lang_assign_op_t op = M_ASSIGN_NONE;
    mln_lang_assign_tmp_t *tmp = (mln_lang_assign_tmp_t *)(right[1]->data);
    if (tmp != NULL) {
        op = tmp->op;
        r = tmp->assign;
        tmp->assign = NULL;
    }
    if ((assign = mln_lang_assign_new(pool, (mln_lang_logiclow_t *)(right[0]->data), op, r, left->line, left->file)) == NULL) {
        return -1;
    }
    left->data = assign;
    left->nonterm_free_handler = mln_lang_assign_free;
    right[0]->data = NULL;
    return 0;
})

MLN_FUNC(static, int, mln_lang_semantic_assignexpeq, \
         (mln_factor_t *left, mln_factor_t **right, void *data), \
         (left, right, data), \
{
    mln_alloc_t *pool = (mln_alloc_t *)data;
    mln_lang_assign_tmp_t *tmp = mln_lang_assign_tmp_new(pool, M_ASSIGN_EQUAL, (mln_lang_assign_t *)(right[1]->data));
    if (tmp == NULL) return -1;
    left->data = tmp;
    left->nonterm_free_handler = mln_lang_assign_tmp_free;
    right[1]->data = NULL;
    return 0;
})

MLN_FUNC(static, int, mln_lang_semantic_assignexppluseq, \
         (mln_factor_t *left, mln_factor_t **right, void *data), \
         (left, right, data), \
{
    mln_alloc_t *pool = (mln_alloc_t *)data;
    mln_lang_assign_tmp_t *tmp = mln_lang_assign_tmp_new(pool, M_ASSIGN_PLUSEQ, (mln_lang_assign_t *)(right[1]->data));
    if (tmp == NULL) return -1;
    left->data = tmp;
    left->nonterm_free_handler = mln_lang_assign_tmp_free;
    right[1]->data = NULL;
    return 0;
})

MLN_FUNC(static, int, mln_lang_semantic_assignexpsubeq, \
         (mln_factor_t *left, mln_factor_t **right, void *data), \
         (left, right, data), \
{
    mln_alloc_t *pool = (mln_alloc_t *)data;
    mln_lang_assign_tmp_t *tmp = mln_lang_assign_tmp_new(pool, M_ASSIGN_SUBEQ, (mln_lang_assign_t *)(right[1]->data));
    if (tmp == NULL) return -1;
    left->data = tmp;
    left->nonterm_free_handler = mln_lang_assign_tmp_free;
    right[1]->data = NULL;
    return 0;
})

MLN_FUNC(static, int, mln_lang_semantic_assignexplmoveq, \
         (mln_factor_t *left, mln_factor_t **right, void *data), \
         (left, right, data), \
{
    mln_alloc_t *pool = (mln_alloc_t *)data;
    mln_lang_assign_tmp_t *tmp = mln_lang_assign_tmp_new(pool, M_ASSIGN_LMOVEQ, (mln_lang_assign_t *)(right[1]->data));
    if (tmp == NULL) return -1;
    left->data = tmp;
    left->nonterm_free_handler = mln_lang_assign_tmp_free;
    right[1]->data = NULL;
    return 0;
})

MLN_FUNC(static, int, mln_lang_semantic_assignexprmoveq, \
         (mln_factor_t *left, mln_factor_t **right, void *data), \
         (left, right, data), \
{
    mln_alloc_t *pool = (mln_alloc_t *)data;
    mln_lang_assign_tmp_t *tmp = mln_lang_assign_tmp_new(pool, M_ASSIGN_RMOVEQ, (mln_lang_assign_t *)(right[1]->data));
    if (tmp == NULL) return -1;
    left->data = tmp;
    left->nonterm_free_handler = mln_lang_assign_tmp_free;
    right[1]->data = NULL;
    return 0;
})

MLN_FUNC(static, int, mln_lang_semantic_assignexpmuleq, \
         (mln_factor_t *left, mln_factor_t **right, void *data), \
         (left, right, data), \
{
    mln_alloc_t *pool = (mln_alloc_t *)data;
    mln_lang_assign_tmp_t *tmp = mln_lang_assign_tmp_new(pool, M_ASSIGN_MULEQ, (mln_lang_assign_t *)(right[1]->data));
    if (tmp == NULL) return -1;
    left->data = tmp;
    left->nonterm_free_handler = mln_lang_assign_tmp_free;
    right[1]->data = NULL;
    return 0;
})

MLN_FUNC(static, int, mln_lang_semantic_assignexpdiveq, \
         (mln_factor_t *left, mln_factor_t **right, void *data), \
         (left, right, data), \
{
    mln_alloc_t *pool = (mln_alloc_t *)data;
    mln_lang_assign_tmp_t *tmp = mln_lang_assign_tmp_new(pool, M_ASSIGN_DIVEQ, (mln_lang_assign_t *)(right[1]->data));
    if (tmp == NULL) return -1;
    left->data = tmp;
    left->nonterm_free_handler = mln_lang_assign_tmp_free;
    right[1]->data = NULL;
    return 0;
})

MLN_FUNC(static, int, mln_lang_semantic_assignexporeq, \
         (mln_factor_t *left, mln_factor_t **right, void *data), \
         (left, right, data), \
{
    mln_alloc_t *pool = (mln_alloc_t *)data;
    mln_lang_assign_tmp_t *tmp = mln_lang_assign_tmp_new(pool, M_ASSIGN_OREQ, (mln_lang_assign_t *)(right[1]->data));
    if (tmp == NULL) return -1;
    left->data = tmp;
    left->nonterm_free_handler = mln_lang_assign_tmp_free;
    right[1]->data = NULL;
    return 0;
})

MLN_FUNC(static, int, mln_lang_semantic_assignexpandeq, \
         (mln_factor_t *left, mln_factor_t **right, void *data), \
         (left, right, data), \
{
    mln_alloc_t *pool = (mln_alloc_t *)data;
    mln_lang_assign_tmp_t *tmp = mln_lang_assign_tmp_new(pool, M_ASSIGN_ANDEQ, (mln_lang_assign_t *)(right[1]->data));
    if (tmp == NULL) return -1;
    left->data = tmp;
    left->nonterm_free_handler = mln_lang_assign_tmp_free;
    right[1]->data = NULL;
    return 0;
})

MLN_FUNC(static, int, mln_lang_semantic_assignexpxoreq, \
         (mln_factor_t *left, mln_factor_t **right, void *data), \
         (left, right, data), \
{
    mln_alloc_t *pool = (mln_alloc_t *)data;
    mln_lang_assign_tmp_t *tmp = mln_lang_assign_tmp_new(pool, M_ASSIGN_XOREQ, (mln_lang_assign_t *)(right[1]->data));
    if (tmp == NULL) return -1;
    left->data = tmp;
    left->nonterm_free_handler = mln_lang_assign_tmp_free;
    right[1]->data = NULL;
    return 0;
})

MLN_FUNC(static, int, mln_lang_semantic_assignexpmodeq, \
         (mln_factor_t *left, mln_factor_t **right, void *data), \
         (left, right, data), \
{
    mln_alloc_t *pool = (mln_alloc_t *)data;
    mln_lang_assign_tmp_t *tmp = mln_lang_assign_tmp_new(pool, M_ASSIGN_MODEQ, (mln_lang_assign_t *)(right[1]->data));
    if (tmp == NULL) return -1;
    left->data = tmp;
    left->nonterm_free_handler = mln_lang_assign_tmp_free;
    right[1]->data = NULL;
    return 0;
})

MLN_FUNC(static, int, mln_lang_semantic_logiclowexp, \
         (mln_factor_t *left, mln_factor_t **right, void *data), \
         (left, right, data), \
{
    mln_alloc_t *pool = (mln_alloc_t *)data;
    mln_lang_logiclow_t *ll, *r = NULL;
    mln_lang_logiclow_op_t op = M_LOGICLOW_NONE;
    mln_lang_logiclow_tmp_t *tmp = (mln_lang_logiclow_tmp_t *)(right[1]->data);
    if (tmp != NULL) {
        op = tmp->op;
        r = tmp->logiclow;
        tmp->logiclow = NULL;
    }
    if ((ll = mln_lang_logiclow_new(pool, (mln_lang_logichigh_t *)(right[0]->data), op, r, left->line, left->file)) == NULL) {
        return -1;
    }
    left->data = ll;
    left->nonterm_free_handler = mln_lang_logiclow_free;
    right[0]->data = NULL;
    return 0;
})

MLN_FUNC(static, int, mln_lang_semantic_logiclowexpor, \
         (mln_factor_t *left, mln_factor_t **right, void *data), \
         (left, right, data), \
{
    mln_alloc_t *pool = (mln_alloc_t *)data;
    mln_lang_logiclow_tmp_t *tmp;
    if ((tmp = mln_lang_logiclow_tmp_new(pool, M_LOGICLOW_OR, (mln_lang_logiclow_t *)(right[1]->data))) == NULL)
        return -1;
    left->data = tmp;
    left->nonterm_free_handler = mln_lang_logiclow_tmp_free;
    right[1]->data = NULL;
    return 0;
})

MLN_FUNC(static, int, mln_lang_semantic_logiclowexpand, \
         (mln_factor_t *left, mln_factor_t **right, void *data), \
         (left, right, data), \
{
    mln_alloc_t *pool = (mln_alloc_t *)data;
    mln_lang_logiclow_tmp_t *tmp;
    if ((tmp = mln_lang_logiclow_tmp_new(pool, M_LOGICLOW_AND, (mln_lang_logiclow_t *)(right[1]->data))) == NULL)
        return -1;
    left->data = tmp;
    left->nonterm_free_handler = mln_lang_logiclow_tmp_free;
    right[1]->data = NULL;
    return 0;
})

MLN_FUNC(static, int, mln_lang_semantic_logichighexp, \
         (mln_factor_t *left, mln_factor_t **right, void *data), \
         (left, right, data), \
{
    mln_alloc_t *pool = (mln_alloc_t *)data;
    mln_lang_logichigh_t *ll, *r = NULL;
    mln_lang_logichigh_op_t op = M_LOGICHIGH_NONE;
    mln_lang_logichigh_tmp_t *tmp = (mln_lang_logichigh_tmp_t *)(right[1]->data);
    if (tmp != NULL) {
        op = tmp->op;
        r = tmp->logichigh;
        tmp->logichigh = NULL;
    }
    if ((ll = mln_lang_logichigh_new(pool, (mln_lang_relativelow_t *)(right[0]->data), op, r, left->line, left->file)) == NULL) {
        return -1;
    }
    left->data = ll;
    left->nonterm_free_handler = mln_lang_logichigh_free;
    right[0]->data = NULL;
    return 0;
})

MLN_FUNC(static, int, mln_lang_semantic_logichighor, \
         (mln_factor_t *left, mln_factor_t **right, void *data), \
         (left, right, data), \
{
    mln_alloc_t *pool = (mln_alloc_t *)data;
    mln_lang_logichigh_tmp_t *tmp;
    if ((tmp = mln_lang_logichigh_tmp_new(pool, M_LOGICHIGH_OR, (mln_lang_logichigh_t *)(right[1]->data))) == NULL)
        return -1;
    left->data = tmp;
    left->nonterm_free_handler = mln_lang_logichigh_tmp_free;
    right[1]->data = NULL;
    return 0;
})

MLN_FUNC(static, int, mln_lang_semantic_logichighand, \
         (mln_factor_t *left, mln_factor_t **right, void *data), \
         (left, right, data), \
{
    mln_alloc_t *pool = (mln_alloc_t *)data;
    mln_lang_logichigh_tmp_t *tmp;
    if ((tmp = mln_lang_logichigh_tmp_new(pool, M_LOGICHIGH_AND, (mln_lang_logichigh_t *)(right[1]->data))) == NULL)
        return -1;
    left->data = tmp;
    left->nonterm_free_handler = mln_lang_logichigh_tmp_free;
    right[1]->data = NULL;
    return 0;
})

MLN_FUNC(static, int, mln_lang_semantic_logichighxor, \
         (mln_factor_t *left, mln_factor_t **right, void *data), \
         (left, right, data), \
{
    mln_alloc_t *pool = (mln_alloc_t *)data;
    mln_lang_logichigh_tmp_t *tmp;
    if ((tmp = mln_lang_logichigh_tmp_new(pool, M_LOGICHIGH_XOR, (mln_lang_logichigh_t *)(right[1]->data))) == NULL)
        return -1;
    left->data = tmp;
    left->nonterm_free_handler = mln_lang_logichigh_tmp_free;
    right[1]->data = NULL;
    return 0;
})

MLN_FUNC(static, int, mln_lang_semantic_relativelowexp, \
         (mln_factor_t *left, mln_factor_t **right, void *data), \
         (left, right, data), \
{
    mln_alloc_t *pool = (mln_alloc_t *)data;
    mln_lang_relativelow_t *lr, *r = NULL; 
    mln_lang_relativelow_op_t op = M_RELATIVELOW_NONE;
    mln_lang_relativelow_tmp_t *tmp = (mln_lang_relativelow_tmp_t *)(right[1]->data);
    if (tmp != NULL) {
        op = tmp->op;
        r = tmp->relativelow;
        tmp->relativelow = NULL;
    }
    if ((lr = mln_lang_relativelow_new(pool, (mln_lang_relativehigh_t *)(right[0]->data), op, r, left->line, left->file)) == NULL) {
        return -1;
    }
    left->data = lr;
    left->nonterm_free_handler = mln_lang_relativelow_free;
    right[0]->data = NULL;
    return 0;
})

MLN_FUNC(static, int, mln_lang_semantic_relativeloweq, \
         (mln_factor_t *left, mln_factor_t **right, void *data), \
         (left, right, data), \
{
    mln_alloc_t *pool = (mln_alloc_t *)data;
    mln_lang_relativelow_tmp_t *tmp;
    if ((tmp = mln_lang_relativelow_tmp_new(pool, M_RELATIVELOW_EQUAL, (mln_lang_relativelow_t *)(right[1]->data))) == NULL)
        return -1;
    left->data = tmp;
    left->nonterm_free_handler = mln_lang_relativelow_tmp_free;
    right[1]->data = NULL;
    return 0;
})

MLN_FUNC(static, int, mln_lang_semantic_relativelownoneq, \
         (mln_factor_t *left, mln_factor_t **right, void *data), \
         (left, right, data), \
{
    mln_alloc_t *pool = (mln_alloc_t *)data;
    mln_lang_relativelow_tmp_t *tmp;
    if ((tmp = mln_lang_relativelow_tmp_new(pool, M_RELATIVELOW_NEQUAL, (mln_lang_relativelow_t *)(right[1]->data))) == NULL)
        return -1;
    left->data = tmp;
    left->nonterm_free_handler = mln_lang_relativelow_tmp_free;
    right[1]->data = NULL;
    return 0;
})

MLN_FUNC(static, int, mln_lang_semantic_relativehighexp, \
         (mln_factor_t *left, mln_factor_t **right, void *data), \
         (left, right, data), \
{
    mln_alloc_t *pool = (mln_alloc_t *)data;
    mln_lang_relativehigh_t *lr, *r = NULL;
    mln_lang_relativehigh_op_t op = M_RELATIVEHIGH_NONE;
    mln_lang_relativehigh_tmp_t *tmp = (mln_lang_relativehigh_tmp_t *)(right[1]->data);
    if (tmp != NULL) {
        op = tmp->op;
        r = tmp->relativehigh;
        tmp->relativehigh = NULL;
    }
    if ((lr = mln_lang_relativehigh_new(pool, (mln_lang_move_t *)(right[0]->data), op, r, left->line, left->file)) == NULL) {
        return -1;
    }
    left->data = lr;
    left->nonterm_free_handler = mln_lang_relativehigh_free;
    right[0]->data = NULL;
    return 0;
})

MLN_FUNC(static, int, mln_lang_semantic_relativehighless, \
         (mln_factor_t *left, mln_factor_t **right, void *data), \
         (left, right, data), \
{
    mln_alloc_t *pool = (mln_alloc_t *)data;
    mln_lang_relativehigh_tmp_t *tmp;
    if ((tmp = mln_lang_relativehigh_tmp_new(pool, M_RELATIVEHIGH_LESS, (mln_lang_relativehigh_t *)(right[1]->data))) == NULL)
        return -1;
    left->data = tmp;
    left->nonterm_free_handler = mln_lang_relativehigh_tmp_free;
    right[1]->data = NULL;
    return 0;
})

MLN_FUNC(static, int, mln_lang_semantic_relativehighlesseq, \
         (mln_factor_t *left, mln_factor_t **right, void *data), \
         (left, right, data), \
{
    mln_alloc_t *pool = (mln_alloc_t *)data;
    mln_lang_relativehigh_tmp_t *tmp;
    if ((tmp = mln_lang_relativehigh_tmp_new(pool, M_RELATIVEHIGH_LESSEQ, (mln_lang_relativehigh_t *)(right[1]->data))) == NULL)
        return -1;
    left->data = tmp;
    left->nonterm_free_handler = mln_lang_relativehigh_tmp_free;
    right[1]->data = NULL;
    return 0;
})

MLN_FUNC(static, int, mln_lang_semantic_relativehighgreater, \
         (mln_factor_t *left, mln_factor_t **right, void *data), \
         (left, right, data), \
{
    mln_alloc_t *pool = (mln_alloc_t *)data;
    mln_lang_relativehigh_tmp_t *tmp;
    if ((tmp = mln_lang_relativehigh_tmp_new(pool, M_RELATIVEHIGH_GREATER, (mln_lang_relativehigh_t *)(right[1]->data))) == NULL)
        return -1;
    left->data = tmp;
    left->nonterm_free_handler = mln_lang_relativehigh_tmp_free;
    right[1]->data = NULL;
    return 0;
})

MLN_FUNC(static, int, mln_lang_semantic_relativehighgreatereq, \
         (mln_factor_t *left, mln_factor_t **right, void *data), \
         (left, right, data), \
{
    mln_alloc_t *pool = (mln_alloc_t *)data;
    mln_lang_relativehigh_tmp_t *tmp;
    if ((tmp = mln_lang_relativehigh_tmp_new(pool, M_RELATIVEHIGH_GREATEREQ, (mln_lang_relativehigh_t *)(right[1]->data))) == NULL)
        return -1;
    left->data = tmp;
    left->nonterm_free_handler = mln_lang_relativehigh_tmp_free;
    right[1]->data = NULL;
    return 0;
})

MLN_FUNC(static, int, mln_lang_semantic_moveexp, \
         (mln_factor_t *left, mln_factor_t **right, void *data), \
         (left, right, data), \
{
    mln_alloc_t *pool = (mln_alloc_t *)data;
    mln_lang_move_t *move, *r = NULL;
    mln_lang_move_op_t op = M_MOVE_NONE;
    mln_lang_move_tmp_t *tmp = (mln_lang_move_tmp_t *)(right[1]->data);
    if (tmp != NULL) {
        op = tmp->op;
        r = tmp->move;
        tmp->move = NULL;
    }
    if ((move = mln_lang_move_new(pool, (mln_lang_addsub_t *)(right[0]->data), op, r, left->line, left->file)) == NULL) {
        return -1;
    }
    left->data = move;
    left->nonterm_free_handler = mln_lang_move_free;
    right[0]->data = NULL;
    return 0;
})

MLN_FUNC(static, int, mln_lang_semantic_moveleft, \
         (mln_factor_t *left, mln_factor_t **right, void *data), \
         (left, right, data), \
{
    mln_alloc_t *pool = (mln_alloc_t *)data;
    mln_lang_move_tmp_t *tmp;
    if ((tmp = mln_lang_move_tmp_new(pool, M_MOVE_LMOVE, (mln_lang_move_t *)(right[1]->data))) == NULL)
        return -1;
    left->data = tmp;
    left->nonterm_free_handler = mln_lang_move_tmp_free;
    right[1]->data = NULL;
    return 0;
})

MLN_FUNC(static, int, mln_lang_semantic_moveright, \
         (mln_factor_t *left, mln_factor_t **right, void *data), \
         (left, right, data), \
{
    mln_alloc_t *pool = (mln_alloc_t *)data;
    mln_lang_move_tmp_t *tmp;
    if ((tmp = mln_lang_move_tmp_new(pool, M_MOVE_RMOVE, (mln_lang_move_t *)(right[1]->data))) == NULL)
        return -1;
    left->data = tmp;
    left->nonterm_free_handler = mln_lang_move_tmp_free;
    right[1]->data = NULL;
    return 0;
})

MLN_FUNC(static, int, mln_lang_semantic_addsubexp, \
         (mln_factor_t *left, mln_factor_t **right, void *data), \
         (left, right, data), \
{
    mln_alloc_t *pool = (mln_alloc_t *)data;
    mln_lang_addsub_t *la, *r = NULL;
    mln_lang_addsub_op_t op = M_ADDSUB_NONE;
    mln_lang_addsub_tmp_t *tmp = (mln_lang_addsub_tmp_t *)(right[1]->data);
    if (tmp != NULL) {
        op = tmp->op;
        r = tmp->addsub;
        tmp->addsub = NULL;
    }
    if ((la = mln_lang_addsub_new(pool, (mln_lang_muldiv_t *)(right[0]->data), op, r, left->line, left->file)) == NULL) {
        return -1;
    }
    left->data = la;
    left->nonterm_free_handler = mln_lang_addsub_free;
    right[0]->data = NULL;
    return 0;
})

MLN_FUNC(static, int, mln_lang_semantic_addsubplus, \
         (mln_factor_t *left, mln_factor_t **right, void *data), \
         (left, right, data), \
{
    mln_alloc_t *pool = (mln_alloc_t *)data;
    mln_lang_addsub_tmp_t *tmp;
    if ((tmp = mln_lang_addsub_tmp_new(pool, M_ADDSUB_PLUS, (mln_lang_addsub_t *)(right[1]->data))) == NULL)
        return -1;
    left->data = tmp;
    left->nonterm_free_handler = mln_lang_addsub_tmp_free;
    right[1]->data = NULL;
    return 0;
})

MLN_FUNC(static, int, mln_lang_semantic_addsubsub, \
         (mln_factor_t *left, mln_factor_t **right, void *data), \
         (left, right, data), \
{
    mln_alloc_t *pool = (mln_alloc_t *)data;
    mln_lang_addsub_tmp_t *tmp;
    if ((tmp = mln_lang_addsub_tmp_new(pool, M_ADDSUB_SUB, (mln_lang_addsub_t *)(right[1]->data))) == NULL)
        return -1;
    left->data = tmp;
    left->nonterm_free_handler = mln_lang_addsub_tmp_free;
    right[1]->data = NULL;
    return 0;
})

MLN_FUNC(static, int, mln_lang_semantic_muldivexp, \
         (mln_factor_t *left, mln_factor_t **right, void *data), \
         (left, right, data), \
{
    mln_alloc_t *pool = (mln_alloc_t *)data;
    mln_lang_muldiv_t *lm, *r = NULL;
    mln_lang_muldiv_op_t op = M_MULDIV_NONE;
    mln_lang_muldiv_tmp_t *tmp = (mln_lang_muldiv_tmp_t *)(right[1]->data);
    if (tmp != NULL) {
        op = tmp->op;
        r = tmp->muldiv;
        tmp->muldiv = NULL;
    }
    if ((lm = mln_lang_muldiv_new(pool, (mln_lang_not_t *)(right[0]->data), op, r, left->line, left->file)) == NULL) {
        return -1;
    }
    left->data = lm;
    left->nonterm_free_handler = mln_lang_muldiv_free;
    right[0]->data = NULL;
    return 0;
})

MLN_FUNC(static, int, mln_lang_semantic_muldivmul, \
         (mln_factor_t *left, mln_factor_t **right, void *data), \
         (left, right, data), \
{
    mln_alloc_t *pool = (mln_alloc_t *)data;
    mln_lang_muldiv_tmp_t *tmp;
    if ((tmp = mln_lang_muldiv_tmp_new(pool, M_MULDIV_MUL, (mln_lang_muldiv_t *)(right[1]->data))) == NULL)
        return -1;
    left->data = tmp;
    left->nonterm_free_handler = mln_lang_muldiv_tmp_free;
    right[1]->data = NULL;
    return 0;
})

MLN_FUNC(static, int, mln_lang_semantic_muldivdiv, \
         (mln_factor_t *left, mln_factor_t **right, void *data), \
         (left, right, data), \
{
    mln_alloc_t *pool = (mln_alloc_t *)data;
    mln_lang_muldiv_tmp_t *tmp;
    if ((tmp = mln_lang_muldiv_tmp_new(pool, M_MULDIV_DIV, (mln_lang_muldiv_t *)(right[1]->data))) == NULL)
        return -1;
    left->data = tmp;
    left->nonterm_free_handler = mln_lang_muldiv_tmp_free;
    right[1]->data = NULL;
    return 0;
})

MLN_FUNC(static, int, mln_lang_semantic_muldivmod, \
         (mln_factor_t *left, mln_factor_t **right, void *data), \
         (left, right, data), \
{
    mln_alloc_t *pool = (mln_alloc_t *)data;
    mln_lang_muldiv_tmp_t *tmp;
    if ((tmp = mln_lang_muldiv_tmp_new(pool, M_MULDIV_MOD, (mln_lang_muldiv_t *)(right[1]->data))) == NULL)
        return -1;
    left->data = tmp;
    left->nonterm_free_handler = mln_lang_muldiv_tmp_free;
    right[1]->data = NULL;
    return 0;
})

MLN_FUNC(static, int, mln_lang_semantic_notnot, \
         (mln_factor_t *left, mln_factor_t **right, void *data), \
         (left, right, data), \
{
    mln_alloc_t *pool = (mln_alloc_t *)data;
    mln_lang_not_t *ln;
    if ((ln = mln_lang_not_new(pool, M_NOT_NOT, right[1]->data, left->line, left->file)) == NULL) {
        return -1;
    }
    left->data = ln;
    left->nonterm_free_handler = mln_lang_not_free;
    right[1]->data = NULL;
    return 0;
})

MLN_FUNC(static, int, mln_lang_semantic_notsuffix, \
         (mln_factor_t *left, mln_factor_t **right, void *data), \
         (left, right, data), \
{
    mln_alloc_t *pool = (mln_alloc_t *)data;
    mln_lang_not_t *ln;
    if ((ln = mln_lang_not_new(pool, M_NOT_NONE, right[0]->data, left->line, left->file)) == NULL) {
        return -1;
    }
    left->data = ln;
    left->nonterm_free_handler = mln_lang_not_free;
    right[0]->data = NULL;
    return 0;
})

MLN_FUNC(static, int, mln_lang_semantic_suffixexp, \
         (mln_factor_t *left, mln_factor_t **right, void *data), \
         (left, right, data), \
{
    mln_alloc_t *pool = (mln_alloc_t *)data;
    mln_lang_suffix_t *ls;
    mln_lang_suffix_op_t op = M_SUFFIX_NONE;
    mln_lang_suffix_tmp_t *tmp = (mln_lang_suffix_tmp_t *)(right[1]->data);
    if (tmp != NULL) {
        op = tmp->op;
    }
    if ((ls = mln_lang_suffix_new(pool, (mln_lang_locate_t *)(right[0]->data), op, left->line, left->file)) == NULL) {
        return -1;
    }
    left->data = ls;
    left->nonterm_free_handler = mln_lang_suffix_free;
    right[0]->data = NULL;
    return 0;
})

MLN_FUNC(static, int, mln_lang_semantic_suffixinc, \
         (mln_factor_t *left, mln_factor_t **right, void *data), \
         (left, right, data), \
{
    mln_alloc_t *pool = (mln_alloc_t *)data;
    mln_lang_suffix_tmp_t *tmp;
    if ((tmp = mln_lang_suffix_tmp_new(pool, M_SUFFIX_INC)) == NULL) {
        return -1;
    }
    left->data = tmp;
    left->nonterm_free_handler = mln_lang_suffix_tmp_free;
    return 0;
})

MLN_FUNC(static, int, mln_lang_semantic_suffixdec, \
         (mln_factor_t *left, mln_factor_t **right, void *data), \
         (left, right, data), \
{
    mln_alloc_t *pool = (mln_alloc_t *)data;
    mln_lang_suffix_tmp_t *tmp;
    if ((tmp = mln_lang_suffix_tmp_new(pool, M_SUFFIX_DEC)) == NULL) {
        return -1;
    }
    left->data = tmp;
    left->nonterm_free_handler = mln_lang_suffix_tmp_free;
    return 0;
})

MLN_FUNC(static, int, mln_lang_semantic_locateexp, \
         (mln_factor_t *left, mln_factor_t **right, void *data), \
         (left, right, data), \
{
    mln_alloc_t *pool = (mln_alloc_t *)data;
    mln_lang_locate_t *ll = NULL;
    mln_lang_locate_op_t op = M_LOCATE_NONE;
    mln_lang_locate_tmp_t *tmp = (mln_lang_locate_tmp_t *)(right[1]->data);
    if (tmp != NULL) op = tmp->op;
    switch (op) {
        case M_LOCATE_INDEX:
        case M_LOCATE_FUNC:
            if ((ll = mln_lang_locate_new(pool, \
                                          (mln_lang_spec_t *)(right[0]->data), \
                                          op, \
                                          tmp->locate.exp, \
                                          tmp->next, \
                                          left->line, \
                                          left->file)) == NULL)
            {
                return -1;
            }
            tmp->locate.exp = NULL;
            break;
        case M_LOCATE_PROPERTY:
            if ((ll = mln_lang_locate_new(pool, \
                                          (mln_lang_spec_t *)(right[0]->data), \
                                          op, \
                                          tmp->locate.id, \
                                          tmp->next, \
                                          left->line, \
                                          left->file)) == NULL)
            {
                return -1;
            }
            tmp->locate.id = NULL;
            break;
        default:
            if ((ll = mln_lang_locate_new(pool, (mln_lang_spec_t *)(right[0]->data), op, NULL, NULL, left->line, left->file)) == NULL)
            {
                return -1;
            }
            break;
    }
    left->data = ll;
    left->nonterm_free_handler = mln_lang_locate_free;
    right[0]->data = NULL;
    return 0;
})

MLN_FUNC(static, int, mln_lang_semantic_locateindex, \
         (mln_factor_t *left, mln_factor_t **right, void *data), \
         (left, right, data), \
{
    mln_alloc_t *pool = (mln_alloc_t *)data;
    mln_lang_locate_tmp_t *tmp;
    if ((tmp = mln_lang_locate_tmp_new(pool, \
                                       M_LOCATE_INDEX, \
                                       right[1]->data, \
                                       (mln_lang_locate_tmp_t *)(right[3]->data))) == NULL)
    {
        return -1;
    }
    left->data = tmp;
    left->nonterm_free_handler = mln_lang_locate_tmp_free;
    right[1]->data = NULL;
    right[3]->data = NULL;
    return 0;
})

MLN_FUNC(static, int, mln_lang_semantic_locateproperty, \
         (mln_factor_t *left, mln_factor_t **right, void *data), \
         (left, right, data), \
{
    mln_alloc_t *pool = (mln_alloc_t *)data;
    mln_lang_locate_tmp_t *tmp;
    mln_lang_struct_t *ls = (mln_lang_struct_t *)(right[1]->data);
    mln_string_t *id = mln_string_pool_dup(pool, ls->text);
    if (id == NULL) return -1;
    if ((tmp = mln_lang_locate_tmp_new(pool, \
                                       M_LOCATE_PROPERTY, \
                                       id, \
                                       (mln_lang_locate_tmp_t *)(right[2]->data))) == NULL)
    {
        mln_string_free(id);
        return -1;
    }
    left->data = tmp;
    left->nonterm_free_handler = mln_lang_locate_tmp_free;
    right[2]->data = NULL;
    return 0;
})

MLN_FUNC(static, int, mln_lang_semantic_locatefunc, \
         (mln_factor_t *left, mln_factor_t **right, void *data), \
         (left, right, data), \
{
    mln_alloc_t *pool = (mln_alloc_t *)data;
    mln_lang_locate_tmp_t *tmp;
    if ((tmp = mln_lang_locate_tmp_new(pool, \
                                       M_LOCATE_FUNC, \
                                       right[1]->data, \
                                       (mln_lang_locate_tmp_t *)(right[3]->data))) == NULL)
    {
        return -1;
    }
    left->data = tmp;
    left->nonterm_free_handler = mln_lang_locate_tmp_free;
    right[1]->data = NULL;
    right[3]->data = NULL;
    return 0;
})

MLN_FUNC(static, int, mln_lang_semantic_specnegative, \
         (mln_factor_t *left, mln_factor_t **right, void *data), \
         (left, right, data), \
{
    mln_alloc_t *pool = (mln_alloc_t *)data;
    mln_lang_spec_t *ls;
    if ((ls = mln_lang_spec_new(pool, M_SPEC_NEGATIVE, right[1]->data, left->line, left->file)) == NULL) {
        return -1;
    }
    left->data = ls;
    left->nonterm_free_handler = mln_lang_spec_free;
    right[1]->data = NULL;
    return 0;
})

MLN_FUNC(static, int, mln_lang_semantic_specreverse, \
         (mln_factor_t *left, mln_factor_t **right, void *data), \
         (left, right, data), \
{
    mln_alloc_t *pool = (mln_alloc_t *)data;
    mln_lang_spec_t *ls;
    if ((ls = mln_lang_spec_new(pool, M_SPEC_REVERSE, right[1]->data, left->line, left->file)) == NULL) {
        return -1;
    }
    left->data = ls;
    left->nonterm_free_handler = mln_lang_spec_free;
    right[1]->data = NULL;
    return 0;
})

MLN_FUNC(static, int, mln_lang_semantic_specrefer, \
         (mln_factor_t *left, mln_factor_t **right, void *data), \
         (left, right, data), \
{
    mln_alloc_t *pool = (mln_alloc_t *)data;
    mln_lang_spec_t *ls;
    if ((ls = mln_lang_spec_new(pool, M_SPEC_REFER, right[1]->data, left->line, left->file)) == NULL) {
        return -1;
    }
    left->data = ls;
    left->nonterm_free_handler = mln_lang_spec_free;
    right[1]->data = NULL;
    return 0;
})

MLN_FUNC(static, int, mln_lang_semantic_specinc, \
         (mln_factor_t *left, mln_factor_t **right, void *data), \
         (left, right, data), \
{
    mln_alloc_t *pool = (mln_alloc_t *)data;
    mln_lang_spec_t *ls;
    if ((ls = mln_lang_spec_new(pool, M_SPEC_INC, right[1]->data, left->line, left->file)) == NULL) {
        return -1;
    }
    left->data = ls;
    left->nonterm_free_handler = mln_lang_spec_free;
    right[1]->data = NULL;
    return 0;
})

MLN_FUNC(static, int, mln_lang_semantic_specdec, \
         (mln_factor_t *left, mln_factor_t **right, void *data), \
         (left, right, data), \
{
    mln_alloc_t *pool = (mln_alloc_t *)data;
    mln_lang_spec_t *ls;
    if ((ls = mln_lang_spec_new(pool, M_SPEC_DEC, right[1]->data, left->line, left->file)) == NULL) {
        return -1;
    }
    left->data = ls;
    left->nonterm_free_handler = mln_lang_spec_free;
    right[1]->data = NULL;
    return 0;
})

MLN_FUNC(static, int, mln_lang_semantic_specnew, \
         (mln_factor_t *left, mln_factor_t **right, void *data), \
         (left, right, data), \
{
    mln_alloc_t *pool = (mln_alloc_t *)data;
    mln_lang_spec_t *spec;
    mln_lang_struct_t *ls = (mln_lang_struct_t *)(right[1]->data);
    mln_string_t *name = mln_string_pool_dup(pool, ls->text);
    if (name == NULL) return -1;
    if ((spec = mln_lang_spec_new(pool, M_SPEC_NEW, name, left->line, left->file)) == NULL) {
        mln_string_free(name);
        return -1;
    }
    left->data = spec;
    left->nonterm_free_handler = mln_lang_spec_free;
    return 0;
})

MLN_FUNC(static, int, mln_lang_semantic_specparenth, \
         (mln_factor_t *left, mln_factor_t **right, void *data), \
         (left, right, data), \
{
    mln_alloc_t *pool = (mln_alloc_t *)data;
    mln_lang_spec_t *ls;
    if ((ls = mln_lang_spec_new(pool, \
                                M_SPEC_PARENTH, \
                                right[1]->data, \
                                left->line, \
                                left->file)) == NULL)
    {
        return -1;
    }
    left->data = ls;
    left->nonterm_free_handler = mln_lang_spec_free;
    right[1]->data = NULL;
    return 0;
})

MLN_FUNC(static, int, mln_lang_semantic_specfactor, \
         (mln_factor_t *left, mln_factor_t **right, void *data), \
         (left, right, data), \
{
    mln_alloc_t *pool = (mln_alloc_t *)data;
    mln_lang_spec_t *ls;
    if ((ls = mln_lang_spec_new(pool, M_SPEC_FACTOR, right[0]->data, left->line, left->file)) == NULL) {
        return -1;
    }
    left->data = ls;
    left->nonterm_free_handler = mln_lang_spec_free;
    right[0]->data = NULL;
    return 0;
})

MLN_FUNC(static, int, mln_lang_semantic_factortrue, \
         (mln_factor_t *left, mln_factor_t **right, void *data), \
         (left, right, data), \
{
    mln_alloc_t *pool = (mln_alloc_t *)data;
    mln_lang_factor_t *lf;
    mln_u8_t t = 1;
    if ((lf = mln_lang_factor_new(pool, M_FACTOR_BOOL, &t, left->line, left->file)) == NULL) {
        return -1;
    }
    left->data = lf;
    left->nonterm_free_handler = mln_lang_factor_free;
    return 0;
})

MLN_FUNC(static, int, mln_lang_semantic_factorfalse, \
         (mln_factor_t *left, mln_factor_t **right, void *data), \
         (left, right, data), \
{
    mln_alloc_t *pool = (mln_alloc_t *)data;
    mln_lang_factor_t *lf;
    mln_u8_t t = 0;
    if ((lf = mln_lang_factor_new(pool, M_FACTOR_BOOL, &t, left->line, left->file)) == NULL) {
        return -1;
    }
    left->data = lf;
    left->nonterm_free_handler = mln_lang_factor_free;
    return 0;
})

MLN_FUNC(static, int, mln_lang_semantic_factornil, \
         (mln_factor_t *left, mln_factor_t **right, void *data), \
         (left, right, data), \
{
    mln_alloc_t *pool = (mln_alloc_t *)data;
    mln_lang_factor_t *lf;
    if ((lf = mln_lang_factor_new(pool, M_FACTOR_NIL, NULL, left->line, left->file)) == NULL) {
        return -1;
    }
    left->data = lf;
    left->nonterm_free_handler = mln_lang_factor_free;
    return 0;
})

MLN_FUNC(static, int, mln_lang_semantic_factorid, \
         (mln_factor_t *left, mln_factor_t **right, void *data), \
         (left, right, data), \
{
    mln_alloc_t *pool = (mln_alloc_t *)data;
    mln_lang_struct_t *ls = (mln_lang_struct_t *)(right[0]->data);
    mln_string_t *id = mln_string_pool_dup(pool, ls->text);
    if (id == NULL) return -1;
    mln_lang_factor_t *lf;
    if ((lf = mln_lang_factor_new(pool, M_FACTOR_ID, id, left->line, left->file)) == NULL) {
        mln_string_free(id);
        return -1;
    }
    left->data = lf;
    left->nonterm_free_handler = mln_lang_factor_free;
    return 0;
})

static int mln_lang_semantic_factorint(mln_factor_t *left, mln_factor_t **right, void *data)
{
    mln_alloc_t *pool = (mln_alloc_t *)data;
    mln_lang_struct_t *ls = (mln_lang_struct_t *)(right[0]->data);
    char num[2048] = {0};
    if (ls->text->len > sizeof(num)-1) {
        mln_log(error, "Integer too long. %S\n", ls->text);
        return -1;
    }
    memcpy(num, ls->text->data, ls->text->len);
    num[ls->text->len] = 0;
    mln_s64_t i;
    if (ls->text->len > 1 && num[0] == '0') {
        if (num[1] == 'x' || num[1] == 'X') {
#if defined(MSVC) || defined(i386) || defined(__arm__) || defined(__wasm__)
            sscanf(num, "%llx", &i);
#else
            sscanf(num, "%lx", &i);
#endif
        } else {
#if defined(MSVC) || defined(i386) || defined(__arm__) || defined(__wasm__)
            sscanf(num, "%llo", &i);
#else
            sscanf(num, "%lo", &i);
#endif
        }
    } else {
#if defined(MSVC) || defined(i386) || defined(__arm__) || defined(__wasm__)
        sscanf(num, "%lld", &i);
#else
        sscanf(num, "%ld", &i);
#endif
    }
    mln_lang_factor_t *lf;
    if ((lf = mln_lang_factor_new(pool, M_FACTOR_INT, &i, left->line, left->file)) == NULL) {
        return -1;
    }
    left->data = lf;
    left->nonterm_free_handler = mln_lang_factor_free;
    return 0;
}

MLN_FUNC(static, int, mln_lang_semantic_factorreal, \
         (mln_factor_t *left, mln_factor_t **right, void *data), \
         (left, right, data), \
{
    mln_alloc_t *pool = (mln_alloc_t *)data;
    mln_lang_struct_t *ls = (mln_lang_struct_t *)(right[0]->data);
    char num[2048] = {0};
    if (ls->text->len > sizeof(num)-1) {
        mln_log(error, "Real number too long. %S\n", ls->text);
        return -1;
    }
    memcpy(num, ls->text->data, ls->text->len);
    num[ls->text->len] = 0;
    double f = atof(num);
    mln_lang_factor_t *lf;
    if ((lf = mln_lang_factor_new(pool, M_FACTOR_REAL, &f, left->line, left->file)) == NULL) {
        return -1;
    }
    left->data = lf;
    left->nonterm_free_handler = mln_lang_factor_free;
    return 0;
})

MLN_FUNC(static, int, mln_lang_semantic_factorstring, \
         (mln_factor_t *left, mln_factor_t **right, void *data), \
         (left, right, data), \
{
    mln_alloc_t *pool = (mln_alloc_t *)data;
    mln_lang_struct_t *ls = (mln_lang_struct_t *)(right[0]->data);
    mln_string_t *s = mln_string_pool_dup(pool, ls->text);
    if (s == NULL) return -1;
    mln_lang_factor_t *lf;
    if ((lf = mln_lang_factor_new(pool, M_FACTOR_STRING, s, left->line, left->file)) == NULL) {
        mln_string_free(s);
        return -1;
    }
    left->data = lf;
    left->nonterm_free_handler = mln_lang_factor_free;
    return 0;
})

MLN_FUNC(static, int, mln_lang_semantic_factorarray, \
         (mln_factor_t *left, mln_factor_t **right, void *data), \
         (left, right, data), \
{
    mln_alloc_t *pool = (mln_alloc_t *)data;
    mln_lang_factor_t *lf;
    if ((lf = mln_lang_factor_new(pool, M_FACTOR_ARRAY, right[1]->data, left->line, left->file)) == NULL) {
        return -1;
    }
    left->data = lf;
    left->nonterm_free_handler = mln_lang_factor_free;
    right[1]->data = NULL;
    return 0;
})

MLN_FUNC(static, int, mln_lang_semantic_elemlist, \
         (mln_factor_t *left, mln_factor_t **right, void *data), \
         (left, right, data), \
{
    mln_alloc_t *pool = (mln_alloc_t *)data;
    mln_lang_elemlist_t *le;
    mln_lang_assign_t *val = (mln_lang_assign_t *)(right[1]->data);
    mln_lang_assign_t *key = NULL;
    if (val == NULL) {
        val = (mln_lang_assign_t *)(right[0]->data);
    } else {
        key = (mln_lang_assign_t *)(right[0]->data);
    }
    if ((le = mln_lang_elemlist_new(pool, \
                                    key, \
                                    val, \
                                    (mln_lang_elemlist_t *)(right[2]->data), \
                                    left->line, \
                                    left->file)) == NULL)
    {
        return -1;
    }
    left->data = le;
    left->nonterm_free_handler = mln_lang_elemlist_free;
    right[0]->data = NULL;
    right[1]->data = NULL;
    right[2]->data = NULL;
    return 0;
})

MLN_FUNC(static, int, mln_lang_semantic_elemval, \
         (mln_factor_t *left, mln_factor_t **right, void *data), \
         (left, right, data), \
{
    left->data = right[1]->data;
    left->nonterm_free_handler = right[1]->nonterm_free_handler;
    right[1]->data = NULL;
    return 0;
})

MLN_FUNC(static, int, mln_lang_semantic_elemnext, \
         (mln_factor_t *left, mln_factor_t **right, void *data), \
         (left, right, data), \
{
    left->data = right[1]->data;
    left->nonterm_free_handler = right[1]->nonterm_free_handler;
    right[1]->data = NULL;
    return 0;
})

/*
 * APIs
 */
int mln_lang_ast_file_open(mln_string_t *file_path)
{
    int fd, n;
    size_t len = file_path->len >= 1024? 1023: file_path->len;
    char path[1024], *melang_path = NULL, tmp_path[1024];
    memcpy(path, file_path->data, len);
    path[len] = 0;

#if defined(MSVC)
    if (len > 1 && path[1] == ':') {
#else
    if (path[0] == '/') {
#endif
        fd = open(path, O_RDONLY);
    } else {
#if defined(MSVC)
        if (!_access(path, 0)) {
#else
        if (!access(path, F_OK)) {
#endif
            fd = open(path, O_RDONLY);
        } else if ((melang_path = getenv((char *)(mln_lang_env.data))) != NULL) {
            char *end = strchr(melang_path, ';');
            int found = 0;
            while (end != NULL) {
                *end = 0;
                n = snprintf(tmp_path, sizeof(tmp_path)-1, "%s/%s", melang_path, path);
                tmp_path[n] = 0;
#if defined(MSVC)
                if (!_access(tmp_path, 0)) {
#else
                if (!access(tmp_path, F_OK)) {
#endif
                    fd = open(tmp_path, O_RDONLY);
                    found = 1;
                    break;
                }
                melang_path = end + 1;
                end = strchr(melang_path, ';');
            }
            if (!found) {
                if (*melang_path) {
                    n = snprintf(tmp_path, sizeof(tmp_path)-1, "%s/%s", melang_path, path);
                    tmp_path[n] = 0;
                    fd = open(tmp_path, O_RDONLY);
                } else {
                    goto goon;
                }
            }
        } else {
goon:
            n = snprintf(tmp_path, sizeof(tmp_path)-1, "%s/%s", mln_path_melang_lib(), path);
            tmp_path[n] = 0;
            fd = open(tmp_path, O_RDONLY);
        }
    }

    return fd;
}

MLN_FUNC(, void *, mln_lang_ast_parser_generate, (void), (), {
    return mln_lang_parser_generate(prod_tbl, sizeof(prod_tbl)/sizeof(mln_production_t), &mln_lang_env);
})

MLN_FUNC_VOID(, void, mln_lang_ast_parser_destroy, (void *data), (data), {
    if (data != NULL) mln_lang_pg_data_free(data);
})

MLN_FUNC(, void *, mln_lang_ast_generate, \
         (mln_alloc_t *pool, void *state_tbl, mln_string_t *data, mln_u32_t data_type), \
         (pool, state_tbl, data, data_type), \
{
    mln_lex_hooks_t hooks;
    struct mln_lex_attr lattr;
    mln_lex_t *lex;
    struct mln_parse_attr pattr;
    mln_alloc_t *internal_pool;
    mln_u8ptr_t ret;

    if ((internal_pool = mln_alloc_init(NULL)) == NULL) {
        return NULL;
    }
    lattr.pool = internal_pool;
    lattr.keywords = keywords;
    memset(&hooks, 0, sizeof(hooks));
    hooks.sub_handler = (lex_hook)mln_lang_sub_handler;
    hooks.plus_handler = (lex_hook)mln_lang_plus_handler;
    hooks.ast_handler = (lex_hook)mln_lang_ast_handler;
    hooks.lagl_handler = (lex_hook)mln_lang_lagl_handler;
    hooks.ragl_handler = (lex_hook)mln_lang_ragl_handler;
    hooks.vertl_handler = (lex_hook)mln_lang_vertl_handler;
    hooks.amp_handler = (lex_hook)mln_lang_amp_handler;
    hooks.dash_handler = (lex_hook)mln_lang_dash_handler;
    hooks.xor_handler = (lex_hook)mln_lang_xor_handler;
    hooks.perc_handler = (lex_hook)mln_lang_perc_handler;
    hooks.equal_handler = (lex_hook)mln_lang_equal_handler;
    hooks.excl_handler = (lex_hook)mln_lang_excl_handler;
    hooks.sglq_handler = (lex_hook)mln_lang_sglq_handler;
    hooks.dblq_handler = (lex_hook)mln_lang_dblq_handler;
    hooks.slash_handler = (lex_hook)mln_lang_slash_handler;
    lattr.hooks = &hooks;
    lattr.preprocess = 1;
    lattr.type = data_type;
    lattr.data = data;
    lattr.env = &mln_lang_env;
    mln_lex_init_with_hooks(mln_lang, lex, &lattr);
    if (lex == NULL) {
        mln_alloc_destroy(internal_pool);
        return NULL;
    }

    pattr.pool = internal_pool;
    pattr.prod_tbl = prod_tbl;
    pattr.lex = lex;
    pattr.pg_data = state_tbl;
    pattr.udata = pool;
    if ((ret = mln_lang_parse(&pattr)) == NULL) {
        mln_lex_destroy(lex);
        mln_alloc_destroy(internal_pool);
        return NULL;
    }
    mln_lex_destroy(lex);
    mln_alloc_destroy(internal_pool);
    return ret;
})

MLN_FUNC_VOID(, void, mln_lang_ast_free, (void *ast), (ast), {
    if (ast == NULL) return;
    mln_lang_stm_free(ast);
})

