
/*
 * Copyright (C) Niklaus F.Schen.
 */
#include "mln_lang.h"
#if !defined(MSVC)
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#endif
#include <sys/stat.h>
#include <sys/types.h>
#if defined(MSVC)
#include "mln_utils.h"
#else
#include <sys/time.h>
#endif
#include <fcntl.h>
#include "mln_lex.h"
#include "mln_log.h"
#include "mln_lang_int.h"
#include "mln_lang_nil.h"
#include "mln_lang_bool.h"
#include "mln_lang_func.h"
#include "mln_lang_obj.h"
#include "mln_lang_real.h"
#include "mln_lang_str.h"
#include "mln_lang_array.h"
#include "mln_path.h"
#include "mln_func.h"
#if defined(MSVC)
#include <libloaderapi.h>
#else
#include <dlfcn.h>
#endif


struct mln_lang_scan_s {
    int              *cnt;
    mln_rbtree_t     *tree;
    mln_rbtree_t     *tree2;
    mln_lang_ctx_t   *ctx;
};

struct mln_lang_gc_scan_s {
    mln_rbtree_t     *tree;
    mln_gc_t         *gc;
};

struct mln_lang_gc_setter_s {
    mln_rbtree_t *visited;
    mln_gc_t     *gc;
};

MLN_CHAIN_FUNC_DECLARE(static inline, \
                       mln_lang_sym_scope, \
                       mln_lang_symbol_node_t, );
MLN_CHAIN_FUNC_DEFINE(static inline, \
                      mln_lang_sym_scope, \
                      mln_lang_symbol_node_t, \
                      scope_prev, \
                      scope_next);
MLN_CHAIN_FUNC_DECLARE(static inline, \
                       mln_lang_sym, \
                       mln_lang_symbol_node_t, );
MLN_CHAIN_FUNC_DEFINE(static inline, \
                      mln_lang_sym, \
                      mln_lang_symbol_node_t, \
                      prev, \
                      next);
MLN_CHAIN_FUNC_DECLARE(static inline, \
                       mln_lang_ast_cache, \
                       mln_lang_ast_cache_t, );
MLN_CHAIN_FUNC_DEFINE(static inline, \
                      mln_lang_ast_cache, \
                      mln_lang_ast_cache_t, \
                      prev, \
                      next);
MLN_CHAIN_FUNC_DECLARE(static inline, \
                       mln_lang_ctx, \
                       mln_lang_ctx_t, );
MLN_CHAIN_FUNC_DEFINE(static inline, \
                      mln_lang_ctx, \
                      mln_lang_ctx_t, \
                      prev, \
                      next);
static int mln_lang_ctx_alias_cmp(mln_lang_ctx_t *ctx1, mln_lang_ctx_t *ctx2);
static inline mln_lang_ctx_t *
__mln_lang_job_new(mln_lang_t *lang, \
                   mln_string_t *alias, \
                   mln_u32_t type, \
                   mln_string_t *data, \
                   void *udata, \
                   mln_lang_return_handler handler);
static inline void __mln_lang_job_free(mln_lang_ctx_t *ctx);
static void mln_lang_run_handler(mln_event_t *ev, int fd, void *data);
static inline mln_lang_ast_cache_t *
mln_lang_ast_cache_new(mln_lang_t *lang, mln_lang_stm_t *stm, mln_string_t *code, mln_u64_t timestamp);
static inline void
mln_lang_ast_cache_free(mln_lang_ast_cache_t *cache);
static inline mln_lang_ctx_t *
mln_lang_ctx_new(mln_lang_t *lang, void *data, mln_string_t *filename, mln_u32_t type, mln_string_t *content, mln_string_t *alias);
static inline void mln_lang_ctx_free(mln_lang_ctx_t *ctx);
static inline mln_lang_set_detail_t *
mln_lang_ctx_get_class(mln_lang_ctx_t *ctx);
static inline mln_lang_var_t *__mln_lang_var_create_nil(mln_lang_ctx_t *ctx, mln_string_t *name);
static inline mln_lang_var_t *__mln_lang_var_create_call(mln_lang_ctx_t *ctx, mln_lang_funccall_val_t *call);
static inline mln_lang_var_t *__mln_lang_var_create_obj(mln_lang_ctx_t *ctx, mln_lang_set_detail_t *in_set, mln_string_t *name);
static inline mln_lang_var_t *__mln_lang_var_create_true(mln_lang_ctx_t *ctx, mln_string_t *name);
static inline mln_lang_var_t *__mln_lang_var_create_array(mln_lang_ctx_t *ctx, mln_string_t *name);
static inline mln_lang_var_t *__mln_lang_var_create_real(mln_lang_ctx_t *ctx, double f, mln_string_t *name);
static inline mln_lang_var_t *__mln_lang_var_create_ref_string(mln_lang_ctx_t *ctx, mln_string_t *s, mln_string_t *name);
static inline mln_lang_var_t *__mln_lang_var_create_bool(mln_lang_ctx_t *ctx, mln_u8_t b, mln_string_t *name);
static inline mln_lang_var_t *__mln_lang_var_create_int(mln_lang_ctx_t *ctx, mln_s64_t off, mln_string_t *name);
static inline void
mln_lang_stack_node_get_ctx_ret_var(mln_lang_stack_node_t *node, mln_lang_ctx_t *ctx);
static inline void
mln_lang_stack_node_reset_ret_val(mln_lang_stack_node_t *node);
static inline void
mln_lang_stack_node_set_ret_var(mln_lang_stack_node_t *node, mln_lang_var_t *ret_var);
static inline void mln_lang_ctx_reset_ret_var(mln_lang_ctx_t *ctx);
static inline void __mln_lang_ctx_set_ret_var(mln_lang_ctx_t *ctx, mln_lang_var_t *var);
static inline void mln_lang_ctx_get_node_ret_val(mln_lang_ctx_t *ctx, mln_lang_stack_node_t *node);
static inline mln_lang_symbol_node_t *
mln_lang_symbol_node_new(mln_lang_ctx_t *ctx, mln_string_t *symbol, mln_lang_symbol_type_t type, void *data);
static void mln_lang_symbol_node_free(void *data);
static inline int __mln_lang_symbol_node_join(mln_lang_ctx_t *ctx, mln_lang_symbol_type_t type, void *data);
static inline mln_lang_symbol_node_t *
__mln_lang_symbol_node_search(mln_lang_ctx_t *ctx, mln_string_t *name, int local);
static inline mln_lang_symbol_node_t *
mln_lang_symbol_node_id_search(mln_lang_ctx_t *ctx, mln_string_t *name);
static inline mln_lang_var_t *
__mln_lang_set_member_search(mln_rbtree_t *members, mln_string_t *name);
static int mln_lang_set_member_iterate_handler(mln_rbtree_node_t *node, void *udata);
static inline mln_lang_var_t *
__mln_lang_var_new(mln_lang_ctx_t *ctx, \
                 mln_string_t *name, \
                 mln_lang_var_type_t type, \
                 mln_lang_val_t *val, \
                 mln_lang_set_detail_t *in_set);
static inline mln_lang_var_t *
__mln_lang_var_new_ref_string(mln_lang_ctx_t *ctx, \
                              mln_string_t *name, \
                              mln_lang_var_type_t type, \
                              mln_lang_val_t *val, \
                              mln_lang_set_detail_t *in_set);
static void mln_lang_var_pfree(mln_lang_var_t **v);
static inline void __mln_lang_var_free(void *data);
static inline mln_lang_var_t *
__mln_lang_var_dup(mln_lang_ctx_t *ctx, mln_lang_var_t *var);
static inline mln_lang_var_t *
mln_lang_var_dup_with_val(mln_lang_ctx_t *ctx, mln_lang_var_t *var);
static int mln_lang_var_name_cmp(const void *data1, const void *data2);
static inline mln_lang_var_t *
mln_lang_var_transform(mln_lang_ctx_t *ctx, mln_lang_var_t *realvar, mln_lang_var_t *defvar);
static inline void __mln_lang_var_assign(mln_lang_var_t *var, mln_lang_val_t *val);
static inline int
__mln_lang_var_value_set(mln_lang_ctx_t *ctx, mln_lang_var_t *dest, mln_lang_var_t *src);
static inline int
mln_lang_funcdef_args_get(mln_lang_ctx_t *ctx, mln_lang_exp_t *exp, mln_array_t *arr, int is_closure);
static inline mln_lang_func_detail_t *
__mln_lang_func_detail_new(mln_lang_ctx_t *ctx, \
                           mln_lang_func_type_t type, \
                           void *data, \
                           mln_lang_exp_t *exp, \
                           mln_lang_exp_t *closure);
static inline void __mln_lang_func_detail_free(mln_lang_func_detail_t *lfd);
static inline mln_lang_func_detail_t *
mln_lang_func_detail_dup(mln_lang_ctx_t *ctx, mln_lang_func_detail_t *func);
static inline mln_lang_object_t *
mln_lang_object_new(mln_lang_ctx_t *ctx, mln_lang_set_detail_t *in_set);
static inline void mln_lang_object_free(mln_lang_object_t *obj);
static inline mln_lang_val_t *
__mln_lang_val_new(mln_lang_ctx_t *ctx, mln_s32_t type, void *data);
static inline void __mln_lang_val_free(void *data);
static inline void mln_lang_val_data_free(mln_lang_val_t *val);
static inline mln_lang_val_t *
mln_lang_val_dup(mln_lang_ctx_t *ctx, mln_lang_val_t *val);
static int mln_lang_val_cmp(const void *data1, const void *data2);
static inline mln_lang_array_t *__mln_lang_array_new(mln_lang_ctx_t *ctx);
static inline void __mln_lang_array_free(mln_lang_array_t *array);
static int mln_lang_array_elem_index_cmp(const void *data1, const void *data2);
static int mln_lang_array_elem_key_cmp(const void *data1, const void *data2);
static inline mln_lang_array_elem_t *
mln_lang_array_elem_new(mln_alloc_t *pool, mln_lang_var_t *key, mln_lang_var_t *val, mln_u64_t index);
static inline void mln_lang_array_elem_free(void *data);
static inline mln_lang_var_t *
__mln_lang_array_get(mln_lang_ctx_t *ctx, mln_lang_array_t *array, mln_lang_var_t *key);
static inline mln_lang_var_t *
mln_lang_array_get_int(mln_lang_ctx_t *ctx, mln_lang_array_t *array, mln_lang_var_t *key);
static inline mln_lang_var_t *
mln_lang_array_get_other(mln_lang_ctx_t *ctx, mln_lang_array_t *array, mln_lang_var_t *key);
static inline mln_lang_var_t *
mln_lang_array_get_nil(mln_lang_ctx_t *ctx, mln_lang_array_t *array);
static inline mln_lang_funccall_val_t *__mln_lang_funccall_val_new(mln_alloc_t *pool, mln_string_t *name);
static inline void __mln_lang_funccall_val_free(mln_lang_funccall_val_t *func);
static inline int mln_lang_funccall_val_operator_overload_test(mln_lang_ctx_t *ctx, mln_string_t *name);
static void __mln_lang_errmsg(mln_lang_ctx_t *ctx, char *msg);
static inline void mln_lang_generate_jump_ptr(void *ptr, mln_lang_stack_node_type_t type);
static void mln_lang_stack_handler_stm(mln_lang_ctx_t *ctx);
static void mln_lang_stack_handler_funcdef(mln_lang_ctx_t *ctx);
static void mln_lang_stack_handler_set(mln_lang_ctx_t *ctx);
static void mln_lang_stack_handler_setstm(mln_lang_ctx_t *ctx);
static void mln_lang_stack_handler_block(mln_lang_ctx_t *ctx);
static inline int mln_lang_met_return(mln_lang_ctx_t *ctx);
static inline int mln_lang_stack_handler_block_exp(mln_lang_ctx_t *ctx, mln_lang_block_t *block);
static inline int mln_lang_stack_handler_block_stm(mln_lang_ctx_t *ctx, mln_lang_block_t *block);
static inline int mln_lang_stack_handler_block_continue(mln_lang_ctx_t *ctx, mln_lang_block_t *block);
static inline int mln_lang_stack_handler_block_break(mln_lang_ctx_t *ctx, mln_lang_block_t *block);
static inline int mln_lang_stack_handler_block_return(mln_lang_ctx_t *ctx, mln_lang_block_t *block);
static inline int mln_lang_stack_handler_block_goto(mln_lang_ctx_t *ctx, mln_lang_block_t *block);
static inline int mln_lang_stack_handler_block_if(mln_lang_ctx_t *ctx, mln_lang_block_t *block);
static inline mln_lang_stack_node_t *mln_lang_withdraw_to_loop(mln_lang_ctx_t *ctx);
static inline mln_lang_stack_node_t *mln_lang_withdraw_break_loop(mln_lang_ctx_t *ctx);
static inline int mln_lang_withdraw_until_func(mln_lang_ctx_t *ctx);
static void mln_lang_stack_handler_while(mln_lang_ctx_t *ctx);
static inline int __mln_lang_condition_is_true(mln_lang_var_t *var);
static void mln_lang_stack_handler_switch(mln_lang_ctx_t *ctx);
static void mln_lang_stack_handler_switchstm(mln_lang_ctx_t *ctx);
static void mln_lang_stack_handler_for(mln_lang_ctx_t *ctx);
static void mln_lang_stack_handler_if(mln_lang_ctx_t *ctx);
static void mln_lang_stack_handler_exp(mln_lang_ctx_t *ctx);
static void mln_lang_stack_handler_assign(mln_lang_ctx_t *ctx);
static void mln_lang_stack_handler_logiclow(mln_lang_ctx_t *ctx);
static void mln_lang_stack_handler_logichigh(mln_lang_ctx_t *ctx);
static void mln_lang_stack_handler_relativelow(mln_lang_ctx_t *ctx);
static void mln_lang_stack_handler_relativehigh(mln_lang_ctx_t *ctx);
static void mln_lang_stack_handler_move(mln_lang_ctx_t *ctx);
static void mln_lang_stack_handler_addsub(mln_lang_ctx_t *ctx);
static void mln_lang_stack_handler_muldiv(mln_lang_ctx_t *ctx);
static void mln_lang_stack_handler_not(mln_lang_ctx_t *ctx);
static void mln_lang_stack_handler_suffix(mln_lang_ctx_t *ctx);
static void mln_lang_stack_handler_locate(mln_lang_ctx_t *ctx);
static inline int mln_lang_stack_handler_funccall_run(mln_lang_ctx_t *ctx, \
                                               mln_lang_stack_node_t *node, \
                                               mln_lang_funccall_val_t *funccall);
static inline int mln_lang_funcall_run_add_args(mln_lang_ctx_t *ctx, mln_lang_array_t *array, mln_lang_var_t *arg);
static inline mln_lang_array_t *mln_lang_funccall_run_build_args(mln_lang_ctx_t *ctx);
static void mln_lang_stack_handler_spec(mln_lang_ctx_t *ctx);
static inline int mln_lang_stack_handler_spec_new(mln_lang_ctx_t *ctx, mln_string_t *name);
static void mln_lang_stack_handler_factor(mln_lang_ctx_t *ctx);
static void mln_lang_stack_handler_elemlist(mln_lang_ctx_t *ctx);

static int mln_lang_dump_symbol(mln_lang_ctx_t *ctx, void *key, void *val, void *udata);
static int mln_lang_dump_check_cmp(const void *addr1, const void *addr2);
static void mln_lang_dump_var(mln_lang_var_t *var, int cnt, mln_rbtree_t *check);
static void mln_lang_dump_set(mln_lang_set_detail_t *set);
static void mln_lang_dump_object(mln_lang_object_t *obj, int cnt, mln_rbtree_t *check);
static int mln_lang_dump_var_iterate_handler(mln_rbtree_node_t *node, void *udata);
static void mln_lang_dump_function(mln_lang_func_detail_t *func, int cnt);
static void mln_lang_dump_array(mln_lang_array_t *array, int cnt, mln_rbtree_t *check);
static int mln_lang_dump_array_elem(mln_rbtree_node_t *node, void *udata);

static int mln_lang_func_dump(mln_lang_ctx_t *ctx);
static int mln_lang_func_stack(mln_lang_ctx_t *ctx);
static int mln_lang_func_watch(mln_lang_ctx_t *ctx);
static int mln_lang_func_unwatch(mln_lang_ctx_t *ctx);
static int mln_lang_func_eval(mln_lang_ctx_t *ctx);
static int mln_lang_func_kill(mln_lang_ctx_t *ctx);
static mln_lang_var_t *mln_lang_func_eval_process(mln_lang_ctx_t *ctx);
static inline mln_lang_var_t *mln_lang_func_eval_process_list(mln_lang_ctx_t *ctx);
static mln_lang_var_t *mln_lang_func_kill_process(mln_lang_ctx_t *ctx);
static int mln_lang_internal_func_installer(mln_lang_ctx_t *ctx);
static mln_lang_var_t *mln_lang_func_dump_process(mln_lang_ctx_t *ctx);
static mln_lang_var_t *mln_lang_func_stack_process(mln_lang_ctx_t *ctx);
static mln_lang_var_t *mln_lang_func_watch_process(mln_lang_ctx_t *ctx);
static mln_lang_var_t *mln_lang_func_unwatch_process(mln_lang_ctx_t *ctx);
static int mln_lang_func_resource_register(mln_lang_ctx_t *ctx);
static mln_lang_import_t *mln_lang_func_import_new(mln_lang_ctx_t *ctx, mln_string_t *name, void *handle);
static int mln_lang_func_import_cmp(const mln_lang_import_t *i1, const mln_lang_import_t *i2);
static void mln_lang_func_import_free(mln_lang_import_t *i);
static mln_lang_ctx_import_t *mln_lang_func_ctx_import_new(mln_lang_ctx_t *ctx, mln_lang_import_t *i);
static int mln_lang_func_ctx_import_cmp(mln_lang_ctx_import_t *ci1, mln_lang_ctx_import_t *ci2);
static void mln_lang_func_ctx_import_free(mln_lang_ctx_import_t *ci);
static int mln_lang_func_import_handler(mln_lang_ctx_t *ctx);
static mln_lang_var_t *mln_lang_func_import_process(mln_lang_ctx_t *ctx);
static int
mln_lang_watch_func_build(mln_lang_ctx_t *ctx, mln_lang_stack_node_t *node, mln_lang_func_detail_t *funcdef, mln_lang_val_t *udata, mln_lang_val_t *newval);
static inline int
mln_lang_gc_item_new(mln_alloc_t *pool, mln_gc_t *gc, mln_lang_gc_type_t type, void *data);
static void mln_lang_gc_item_free(mln_lang_gc_item_t *gc_item);
static inline void mln_lang_gc_item_free_immediatly(mln_lang_gc_item_t *gc_item);
static void *mln_lang_gc_item_getter(mln_lang_gc_item_t *gc_item);
static void mln_lang_gc_item_setter(mln_lang_gc_item_t *gc_item, void *gc_data);
static void mln_lang_gc_item_member_setter(mln_gc_t *gc, mln_lang_gc_item_t *gc_item);
static int mln_lang_gc_setter_cmp(const void *data1, const void *data2);
static void mln_lang_gc_item_member_setter_recursive(struct mln_lang_gc_setter_s *lgs, mln_lang_gc_item_t *gc_item);
static int mln_lang_gc_item_member_setter_obj_iterate_handler(mln_rbtree_node_t *node, void *udata);
static int mln_lang_gc_item_member_setter_array_iterate_handler(mln_rbtree_node_t *node, void *udata);
static void mln_lang_gc_item_move_handler(mln_gc_t *dest_gc, mln_lang_gc_item_t *gc_item);
static void mln_lang_gc_item_root_setter(mln_gc_t *gc, mln_lang_ctx_t *ctx);
static void mln_lang_gc_item_clean_searcher(mln_gc_t *gc, mln_lang_gc_item_t *gc_item);
static int mln_lang_gc_item_clean_searcher_obj_iterate_handler(mln_rbtree_node_t *node, void *udata);
static int mln_lang_gc_item_clean_searcher_array_iterate_handler(mln_rbtree_node_t *node, void *udata);
static void mln_lang_gc_item_free_handler(mln_lang_gc_item_t *gc_item);
static void mln_lang_ctx_resource_free_handler(mln_lang_resource_t *lr);
static int mln_lang_resource_cmp(const mln_lang_resource_t *lr1, const mln_lang_resource_t *lr2);
static void mln_lang_resource_free_handler(mln_lang_resource_t *lr);
static inline mln_lang_hash_t *mln_lang_hash_new(mln_lang_ctx_t *ctx);
static inline void mln_lang_hash_free(mln_lang_hash_t *h);
static inline mln_lang_hash_bucket_t *mln_lang_hash_get_bucket(mln_lang_hash_t *h, mln_lang_symbol_node_t *sym);
static mln_lang_ctx_pipe_t *mln_lang_ctx_pipe_new(mln_lang_ctx_t *ctx);
static void mln_lang_ctx_pipe_free(mln_lang_ctx_pipe_t *p);
static int mln_lang_func_pipe(mln_lang_ctx_t *ctx);
static mln_lang_var_t *mln_lang_func_pipe_process(mln_lang_ctx_t *ctx);
static inline mln_lang_var_t *mln_lang_func_pipe_process_array_generate(mln_lang_ctx_t *ctx, mln_lang_ctx_pipe_t *pipe);
static inline int mln_lang_ctx_pipe_do_send(mln_lang_ctx_t *ctx, char *fmt, va_list arg);
static inline int mln_lang_ctx_pipe_elem_init(mln_lang_ctx_pipe_elem_t *pe, int type, void *value);
static void mln_lang_ctx_pipe_elem_destroy(mln_lang_ctx_pipe_elem_t *pe);

#define mln_lang_stack_top(ctx)               ((ctx)->run_stack_top)
#if defined(MSVC)
static inline mln_lang_stack_node_t *
mln_lang_stack_push(mln_lang_ctx_t *ctx, mln_lang_stack_node_type_t _type, void *_data)
{
    mln_lang_stack_node_t *n = NULL;
    if (ctx->run_stack_top == NULL) {
        n = ctx->run_stack_top = ctx->run_stack;
    } else if (ctx->run_stack_top - ctx->run_stack < M_LANG_RUN_STACK_LEN) {
        n = ++(ctx->run_stack_top);
    }
    if (n != NULL) {
        n->type = _type;
        n->pos = NULL;
        n->ret_var = n->ret_var2 = NULL;
        n->step = 0;
        n->call = 0;
        switch (_type) {
            case M_LSNT_STM:
                n->data.stm = (mln_lang_stm_t *)_data;
                break;
            case M_LSNT_FUNCDEF:
                n->data.funcdef = (mln_lang_funcdef_t *)_data;
                break;
            case M_LSNT_SET:
                n->data.set = (mln_lang_set_t *)_data;
                break;
            case M_LSNT_SETSTM:
                n->data.set_stm = (mln_lang_setstm_t *)_data;
                break;
            case M_LSNT_BLOCK:
                n->data.block = (mln_lang_block_t *)_data;
                break;
            case M_LSNT_WHILE:
                n->data.w = (mln_lang_while_t *)_data;
                break;
            case M_LSNT_SWITCH:
                n->data.sw = (mln_lang_switch_t *)_data;
                break;
            case M_LSNT_SWITCHSTM:
                n->data.sw_stm = (mln_lang_switchstm_t *)_data;
                break;
            case M_LSNT_FOR:
                n->data.f = (mln_lang_for_t *)_data;
                break;
            case M_LSNT_IF:
                n->data.i = (mln_lang_if_t *)_data;
                break;
            case M_LSNT_EXP:
                n->data.exp = (mln_lang_exp_t *)_data;
                break;
            case M_LSNT_ASSIGN:
                n->data.assign = (mln_lang_assign_t *)_data;
                break;
            case M_LSNT_LOGICLOW:
                n->data.logiclow = (mln_lang_logiclow_t *)_data;
                break;
            case M_LSNT_LOGICHIGH:
                n->data.logichigh = (mln_lang_logichigh_t *)_data;
                n->pos = n->data.logichigh;
                break;
            case M_LSNT_RELATIVELOW:
                n->data.relativelow = (mln_lang_relativelow_t *)_data;
                n->pos = n->data.relativelow;
                break;
            case M_LSNT_RELATIVEHIGH:
                n->data.relativehigh = (mln_lang_relativehigh_t *)_data;
                n->pos = n->data.relativehigh;
                break;
            case M_LSNT_MOVE:
                n->data.move = (mln_lang_move_t *)_data;
                n->pos = n->data.move;
                break;
            case M_LSNT_ADDSUB:
                n->data.addsub = (mln_lang_addsub_t *)_data;
                n->pos = n->data.addsub;
                break;
            case M_LSNT_MULDIV:
                n->data.muldiv = (mln_lang_muldiv_t *)_data;
                n->pos = n->data.muldiv;
                break;
            case M_LSNT_NOT:
                n->data.not = (mln_lang_not_t *)_data;
                n->pos = n->data.not;
                break;
            case M_LSNT_SUFFIX:
                n->data.suffix = (mln_lang_suffix_t *)_data;
                break;
            case M_LSNT_LOCATE:
                n->data.locate = (mln_lang_locate_t *)_data;
                break;
            case M_LSNT_SPEC:
                n->data.spec = (mln_lang_spec_t *)_data;
                break;
            case M_LSNT_FACTOR:
                n->data.factor = (mln_lang_factor_t *)_data;
                break;
            default: /* M_LSNT_ELEMLIST */
                n->data.elemlist = (mln_lang_elemlist_t *)_data;
                break;
        }
    }
    return n;
}

static inline mln_lang_stack_node_t *mln_lang_stack_pop(mln_lang_ctx_t *ctx)
{
    mln_lang_stack_node_t *n = ctx->run_stack_top;
    if (n != NULL && --ctx->run_stack_top < ctx->run_stack) {
        ctx->run_stack_top = NULL;
    }
    return n;
}

static inline void mln_lang_stack_withdraw(mln_lang_ctx_t *ctx)
{
    if (ctx->run_stack_top == NULL) {
        ctx->run_stack_top = ctx->run_stack;
    } else if (ctx->run_stack_top - ctx->run_stack < M_LANG_RUN_STACK_LEN) {
        ++(ctx->run_stack_top);
    } else {
        ASSERT(0);
    }
}

static inline void mln_lang_stack_node_free(void *data)
{
    mln_lang_stack_node_t *n = data;
    if (n != NULL) {
        if (n->ret_var != NULL) {
            __mln_lang_var_free(n->ret_var);
            n->ret_var = NULL;
        }
        if (n->ret_var2 != NULL) {
            __mln_lang_var_free(n->ret_var2);
            n->ret_var2 = NULL;
        }
    }
}

#else

#define mln_lang_stack_push(ctx,_type,_data)  ({\
    mln_lang_stack_node_t *n = NULL;\
    if ((ctx)->run_stack_top == NULL) {\
        n = (ctx)->run_stack_top = (ctx)->run_stack;\
    } else if ((ctx)->run_stack_top - (ctx)->run_stack < M_LANG_RUN_STACK_LEN) {\
        n = ++((ctx)->run_stack_top);\
    }\
    if (n != NULL) {\
        n->type = (_type);\
        n->pos = NULL;\
        n->ret_var = n->ret_var2 = NULL;\
        n->step = 0;\
        n->call = 0;\
        switch ((_type)) {\
            case M_LSNT_STM:\
                n->data.stm = (mln_lang_stm_t *)(_data);\
                break;\
            case M_LSNT_FUNCDEF:\
                n->data.funcdef = (mln_lang_funcdef_t *)(_data);\
                break;\
            case M_LSNT_SET:\
                n->data.set = (mln_lang_set_t *)(_data);\
                break;\
            case M_LSNT_SETSTM:\
                n->data.set_stm = (mln_lang_setstm_t *)(_data);\
                break;\
            case M_LSNT_BLOCK:\
                n->data.block = (mln_lang_block_t *)(_data);\
                break;\
            case M_LSNT_WHILE:\
                n->data.w = (mln_lang_while_t *)(_data);\
                break;\
            case M_LSNT_SWITCH:\
                n->data.sw = (mln_lang_switch_t *)(_data);\
                break;\
            case M_LSNT_SWITCHSTM:\
                n->data.sw_stm = (mln_lang_switchstm_t *)(_data);\
                break;\
            case M_LSNT_FOR:\
                n->data.f = (mln_lang_for_t *)(_data);\
                break;\
            case M_LSNT_IF:\
                n->data.i = (mln_lang_if_t *)(_data);\
                break;\
            case M_LSNT_EXP:\
                n->data.exp = (mln_lang_exp_t *)(_data);\
                break;\
            case M_LSNT_ASSIGN:\
                n->data.assign = (mln_lang_assign_t *)(_data);\
                break;\
            case M_LSNT_LOGICLOW:\
                n->data.logiclow = (mln_lang_logiclow_t *)(_data);\
                break;\
            case M_LSNT_LOGICHIGH:\
                n->data.logichigh = (mln_lang_logichigh_t *)(_data);\
                n->pos = n->data.logichigh;\
                break;\
            case M_LSNT_RELATIVELOW:\
                n->data.relativelow = (mln_lang_relativelow_t *)(_data);\
                n->pos = n->data.relativelow;\
                break;\
            case M_LSNT_RELATIVEHIGH:\
                n->data.relativehigh = (mln_lang_relativehigh_t *)(_data);\
                n->pos = n->data.relativehigh;\
                break;\
            case M_LSNT_MOVE:\
                n->data.move = (mln_lang_move_t *)(_data);\
                n->pos = n->data.move;\
                break;\
            case M_LSNT_ADDSUB:\
                n->data.addsub = (mln_lang_addsub_t *)(_data);\
                n->pos = n->data.addsub;\
                break;\
            case M_LSNT_MULDIV:\
                n->data.muldiv = (mln_lang_muldiv_t *)(_data);\
                n->pos = n->data.muldiv;\
                break;\
            case M_LSNT_NOT:\
                n->data.not = (mln_lang_not_t *)(_data);\
                n->pos = n->data.not;\
                break;\
            case M_LSNT_SUFFIX:\
                n->data.suffix = (mln_lang_suffix_t *)(_data);\
                break;\
            case M_LSNT_LOCATE:\
                n->data.locate = (mln_lang_locate_t *)(_data);\
                break;\
            case M_LSNT_SPEC:\
                n->data.spec = (mln_lang_spec_t *)(_data);\
                break;\
            case M_LSNT_FACTOR:\
                n->data.factor = (mln_lang_factor_t *)(_data);\
                break;\
            default: /* M_LSNT_ELEMLIST */\
                n->data.elemlist = (mln_lang_elemlist_t *)(_data);\
                break;\
        }\
    }\
    n;\
})
#define mln_lang_stack_pop(ctx)    ({\
    mln_lang_stack_node_t *n = (ctx)->run_stack_top;\
    if (n != NULL && --(ctx)->run_stack_top < (ctx)->run_stack) {\
        (ctx)->run_stack_top = NULL;\
    }\
    n;\
})
#define mln_lang_stack_withdraw(ctx) ({\
    if ((ctx)->run_stack_top == NULL) {\
        (ctx)->run_stack_top = (ctx)->run_stack;\
    } else if ((ctx)->run_stack_top - (ctx)->run_stack < M_LANG_RUN_STACK_LEN) {\
        ++((ctx)->run_stack_top);\
    } else {\
        ASSERT(0);\
    }\
})
#define mln_lang_stack_node_free(_data) ({\
    mln_lang_stack_node_t *n = (_data);\
    if (n != NULL) {\
        if (n->ret_var != NULL) {\
            __mln_lang_var_free(n->ret_var);\
            n->ret_var = NULL;\
        }\
        if (n->ret_var2 != NULL) {\
            __mln_lang_var_free(n->ret_var2);\
            n->ret_var2 = NULL;\
        }\
    }\
})
#endif

#define mln_lang_stack_popuntil(ctx); \
    while (mln_lang_stack_top(ctx) != NULL && mln_lang_stack_top(ctx)->step == M_LANG_STEP_OUT) {\
        mln_lang_stack_node_free(mln_lang_stack_pop(ctx));\
    }

#define mln_lang_scope_top(_ctx) ((_ctx)->scope_top)

#define mln_lang_scope_base(_ctx) ((_ctx)->scopes)

#if defined(MSVC)
static inline int mln_lang_scope_in(mln_lang_ctx_t *ctx, mln_lang_scope_t *s)
{
    if (s != NULL && s - ctx->scopes <= M_LANG_SCOPE_LEN) {
        return 1;
    }
    return 0;
}

static inline mln_lang_scope_t *
mln_lang_scope_push(mln_lang_ctx_t *ctx, \
                    mln_string_t *name, \
                    mln_lang_scope_type_t type, \
                    mln_lang_stack_node_t *cur_stack, \
                    mln_lang_stm_t *entry_stm)
{
    mln_lang_scope_t *s = NULL, *last;
    last = ctx->scope_top;
    if (last == NULL) {
        s = ctx->scope_top = ctx->scopes;
    } else if (last - ctx->scopes < M_LANG_SCOPE_LEN) {
        s = (++ctx->scope_top);
    }
    if (s != NULL) {
        s->ctx = ctx;
        s->type = type;
        if (name != NULL) {
            s->name = mln_string_ref(name);
        } else {
            s->name = NULL;
        }
        s->cur_stack = cur_stack;
        s->entry = entry_stm;
        s->layer = last == NULL? 1: last->layer + 1;
        s->sym_head = s->sym_tail = NULL;
    }
    return s;
}

static inline void mln_lang_scope_pop(mln_lang_ctx_t *ctx)
{
    mln_lang_scope_t *s = ctx->scope_top;
    mln_lang_symbol_node_t *sym;
    if (s != NULL) {
        if (s->name != NULL) {
            mln_string_free(s->name);
        }
        while ((sym = s->sym_head) != NULL) {
            mln_lang_sym_scope_chain_del(&s->sym_head, &s->sym_tail, sym);
            mln_lang_sym_chain_del(&sym->bucket->head, &sym->bucket->tail, sym);
            mln_lang_symbol_node_free(sym);
        }
        if (--ctx->scope_top < ctx->scopes) {
            ctx->scope_top = NULL;
        }
    }
}

#else

#define mln_lang_scope_in(_ctx,_scope) ({\
    int in = 0;\
    mln_lang_scope_t *s = (_scope);\
    if (s != NULL && s - (_ctx)->scopes <= M_LANG_SCOPE_LEN) {\
        in = 1;\
    }\
    in;\
})

#define mln_lang_scope_push(_ctx,_name,_type,_cur_stack,_entry_stm) ({\
    mln_lang_scope_t *s = NULL, *last;\
    last = (_ctx)->scope_top;\
    if (last == NULL) {\
        s = (_ctx)->scope_top = (_ctx)->scopes;\
    } else if (last - (_ctx)->scopes < M_LANG_SCOPE_LEN) {\
        s = (++(_ctx)->scope_top);\
    }\
    if (s != NULL) {\
        s->ctx = (_ctx);\
        s->type = (_type);\
        if ((_name) != NULL) {\
            s->name = mln_string_ref((_name));\
        } else {\
            s->name = NULL;\
        }\
        s->cur_stack = (_cur_stack);\
        s->entry = (_entry_stm);\
        s->layer = last == NULL? 1: last->layer + 1;\
        s->sym_head = s->sym_tail = NULL;\
    }\
    s;\
})

#define mln_lang_scope_pop(_ctx) ({\
    mln_lang_scope_t *s = (_ctx)->scope_top;\
    mln_lang_symbol_node_t *sym;\
    if (s != NULL) {\
        if (s->name != NULL) {\
            mln_string_free(s->name);\
        }\
        while ((sym = s->sym_head) != NULL) {\
            mln_lang_sym_scope_chain_del(&s->sym_head, &s->sym_tail, sym);\
            mln_lang_sym_chain_del(&sym->bucket->head, &sym->bucket->tail, sym);\
            mln_lang_symbol_node_free(sym);\
        }\
        if (--(_ctx)->scope_top < (_ctx)->scopes) {\
            (_ctx)->scope_top = NULL;\
        }\
    }\
})
#endif


mln_lang_method_t *mln_lang_methods[] = {
&mln_lang_nil_oprs,
&mln_lang_int_oprs,
&mln_lang_bool_oprs,
&mln_lang_real_oprs,
&mln_lang_str_oprs,
&mln_lang_obj_oprs,
&mln_lang_func_oprs,
&mln_lang_array_oprs,
NULL
};

mln_lang_stack_handler mln_lang_stack_map[] = {
    mln_lang_stack_handler_stm,
    mln_lang_stack_handler_funcdef,
    mln_lang_stack_handler_set,
    mln_lang_stack_handler_setstm,
    mln_lang_stack_handler_block,
    mln_lang_stack_handler_while,
    mln_lang_stack_handler_switch,
    mln_lang_stack_handler_switchstm,
    mln_lang_stack_handler_for,
    mln_lang_stack_handler_if,
    mln_lang_stack_handler_exp,
    mln_lang_stack_handler_assign,
    mln_lang_stack_handler_logiclow,
    mln_lang_stack_handler_logichigh,
    mln_lang_stack_handler_relativelow,
    mln_lang_stack_handler_relativehigh,
    mln_lang_stack_handler_move,
    mln_lang_stack_handler_addsub,
    mln_lang_stack_handler_muldiv,
    mln_lang_stack_handler_not,
    mln_lang_stack_handler_suffix,
    mln_lang_stack_handler_locate,
    mln_lang_stack_handler_spec,
    mln_lang_stack_handler_factor,
    mln_lang_stack_handler_elemlist
};

mln_lang_t *mln_lang_new(mln_event_t *ev, mln_lang_run_ctl_t signal, mln_lang_run_ctl_t clear)
{
    mln_lang_t *lang;
    struct mln_rbtree_attr rbattr;
    mln_alloc_t *pool;

    if ((pool = mln_alloc_init(NULL)) == NULL)
        return NULL;

    if ((lang = (mln_lang_t *)mln_alloc_m(pool, sizeof(mln_lang_t))) == NULL) {
        mln_alloc_destroy(pool);
        return NULL;
    }
    lang->ev = ev;
    lang->pool = pool;
    lang->run_head = lang->run_tail = NULL;
    lang->wait_head = lang->wait_tail = NULL;
    lang->resource_set = NULL;
    lang->cache_head = NULL;
    lang->cache_tail = NULL;
    lang->shift_table = NULL;
    lang->wait = 0;
    lang->quit = 0;
    lang->cache = 0;
    lang->signal = signal;
    lang->launcher = mln_lang_run_handler;
    lang->clear = clear;
    lang->alias_set = NULL;
#if !defined(MSVC)
    if (pthread_mutex_init(&lang->lock, NULL) != 0) {
        mln_alloc_destroy(pool);
        return NULL;
    }
#endif
    rbattr.pool = pool;
    rbattr.pool_alloc = (rbtree_pool_alloc_handler)mln_alloc_m;
    rbattr.pool_free = (rbtree_pool_free_handler)mln_alloc_free;
    rbattr.cmp = (rbtree_cmp)mln_lang_resource_cmp;
    rbattr.data_free = (rbtree_free_data)mln_lang_resource_free_handler;
    if ((lang->resource_set = mln_rbtree_new(&rbattr)) == NULL) {
        mln_lang_free(lang);
        return NULL;
    }
    if ((lang->shift_table = mln_lang_ast_parser_generate()) == NULL) {
        mln_lang_free(lang);
        return NULL;
    }
    rbattr.cmp = (rbtree_cmp)mln_lang_ctx_alias_cmp;
    rbattr.data_free = NULL;
    if ((lang->alias_set = mln_rbtree_new(&rbattr)) == NULL) {
        mln_lang_free(lang);
        return NULL;
    }
    return lang;
}

void mln_lang_free(mln_lang_t *lang)
{
    if (lang == NULL) return;

    mln_alloc_t *pool = lang->pool;

#if !defined(MSVC)
    pthread_mutex_lock(&lang->lock);
#endif

    if (lang->wait) {
        lang->quit = 1;
#if !defined(MSVC)
        pthread_mutex_unlock(&lang->lock);
#endif
        return;
    }
    mln_lang_ctx_t *ctx;
    mln_lang_ast_cache_t *cache;
    while ((ctx = lang->run_head) != NULL) {
        mln_lang_ctx_chain_del(&(lang->run_head), &(lang->run_tail), ctx);
        mln_lang_ctx_free(ctx);
    }
    while ((ctx = lang->wait_head) != NULL) {
        mln_lang_ctx_chain_del(&(lang->wait_head), &(lang->wait_tail), ctx);
        mln_lang_ctx_free(ctx);
    }
    if (lang->resource_set != NULL) mln_rbtree_free(lang->resource_set);
    if (lang->shift_table != NULL) mln_lang_ast_parser_destroy(lang->shift_table);
    while ((cache = lang->cache_head) != NULL) {
        mln_lang_ast_cache_chain_del(&(lang->cache_head), &(lang->cache_tail), cache);
        mln_lang_ast_cache_free(cache);
    }
    if (lang->alias_set != NULL) mln_rbtree_free(lang->alias_set);

#if !defined(MSVC)
    pthread_mutex_unlock(&lang->lock);
    pthread_mutex_destroy(&lang->lock);
#endif

    mln_alloc_free(lang);
    mln_alloc_destroy(pool);
}

static void mln_lang_run_handler(mln_event_t *ev, int fd, void *data)
{
    int n;
    mln_lang_t *lang = (mln_lang_t *)data;
    mln_lang_ctx_t *ctx;
    mln_lang_stack_node_t *node;

#if !defined(MSVC)
    if (pthread_mutex_trylock(&lang->lock) != 0)
        return;
#endif

#if !defined(MSVC)
    if ((ctx = lang->run_head) != NULL && ctx->owner == 0) {
#else
    if ((ctx = lang->run_head) != NULL) {
#endif
        mln_lang_ctx_chain_del(&lang->run_head, &lang->run_tail, ctx);
        mln_lang_ctx_chain_add(&lang->run_head, &lang->run_tail, ctx);
#if !defined(MSVC)
        ctx->owner = pthread_self();
        pthread_mutex_unlock(&lang->lock);
#endif
        for (n = 0; n < M_LANG_DEFAULT_STEP; ++n) {
            if ((node = mln_lang_stack_top(ctx)) == NULL)
                goto quit;
            mln_lang_stack_map[node->type](ctx);
            if (ctx->ref) break;
            if (ctx->quit) {
quit:
                if (ctx->return_handler != NULL) {
                    ctx->return_handler(ctx);
                }
                mln_lang_job_free(ctx);
#if !defined(MSVC)
                pthread_mutex_lock(&lang->lock);
#endif
                goto out;
            }
        }
        /*
         * ctx->ref == 0 means that this task is not suspended.
         * and the gc should not collect if the next step is
         * assign-statement. Because the right value might not be
         * referred now, so it might be collected and free before
         * assigned to the left variable.
         */
        if (!ctx->ref && (mln_lang_stack_top(ctx) == NULL || mln_lang_stack_top(ctx)->type != M_LSNT_ASSIGN)) {
            mln_gc_collect(ctx->gc, ctx);
        }
#if !defined(MSVC)
        pthread_mutex_lock(&lang->lock);
        ctx->owner = 0;
#endif
out:
        if (lang->run_head != NULL)
            lang->signal(lang);
    } else {
        lang->clear(lang);
    }
#if !defined(MSVC)
    pthread_mutex_unlock(&lang->lock);
#endif
}


MLN_FUNC(static inline, mln_lang_ast_cache_t *, mln_lang_ast_cache_new, \
         (mln_lang_t *lang, mln_lang_stm_t *stm, mln_string_t *code, mln_u64_t timestamp), \
         (lang, stm, code, timestamp), \
{
    mln_lang_ast_cache_t *cache;
    if ((cache = (mln_lang_ast_cache_t *)mln_alloc_m(lang->pool, sizeof(mln_lang_ast_cache_t))) == NULL) {
        return NULL;
    }
    cache->stm = stm;
    cache->code = mln_string_pool_dup(lang->pool, code);
    if (cache->code == NULL) {
        mln_alloc_free(cache);
        return NULL;
    }
    cache->ref = 0;
    cache->expire = 0;
    cache->timestamp = timestamp;
    cache->prev = cache->next = NULL;
    return cache;
})

MLN_FUNC_VOID(static inline, void, mln_lang_ast_cache_free, (mln_lang_ast_cache_t *cache), (cache), {
    if (cache == NULL) return;
    if (cache->stm != NULL)
        mln_lang_ast_free(cache->stm);
    if (cache->code != NULL)
        mln_string_free(cache->code);
    mln_alloc_free(cache);
})

MLN_FUNC(static inline, mln_lang_ast_cache_t *, mln_lang_ast_cache_search, \
         (mln_lang_t *lang, mln_u32_t type, mln_string_t *content), \
         (lang, type, content), \
{
    mln_lang_ast_cache_t *cache;
    mln_string_t data;
    mln_u8ptr_t buf = NULL;
    mln_lang_stm_t *stm;
    int fd;
    struct stat st;
    mln_u64_t now;
    struct timeval tv;

    gettimeofday(&tv, NULL);
    now = tv.tv_sec * 1000000 + tv.tv_usec;

    if (type == M_INPUT_T_FILE) {
        if (content->len >= 1 && (content->data[0] == (mln_u8_t)'/' || content->data[0] == (mln_u8_t)'@')) {
            mln_string_nset(&data, content->data, content->len);
        } else {
            if ((fd = mln_lang_ast_file_open(content)) < 0) {
                return NULL;
            }

            if (fstat(fd, &st) < 0) {
                close(fd);
                return NULL;
            }
            buf = (mln_u8ptr_t)malloc(st.st_size);
            if (buf == NULL) {
                close(fd);
                return NULL;
            }
            if (read(fd, buf, st.st_size) != st.st_size) {
                free(buf);
                close(fd);
                return NULL;
            }
            close(fd);
            mln_string_nset(&data, buf, st.st_size);
        }
    } else {
        data = *content;
    }

    for (cache = lang->cache_head; cache != NULL; cache = cache->next) {
        if (cache->expire) continue;
        if (now - cache->timestamp >= MLN_LANG_STM_CACHE_USEC) {
            cache->expire = 1;
            continue;
        }
        if (cache->code->len == data.len && !memcmp(cache->code->data, data.data, data.len)) {
            if (buf != NULL) free(buf);
            return cache;
        }
    }

    stm = (mln_lang_stm_t *)mln_lang_ast_generate(lang->pool, lang->shift_table, content, type);
    if (stm == NULL) {
        if (buf != NULL) free(buf);
        return NULL;
    }

    cache = mln_lang_ast_cache_new(lang, stm, &data, now);
    if (buf != NULL) free(buf);
    if (cache == NULL) {
        mln_lang_ast_free(stm);
        return NULL;
    }
    mln_lang_ast_cache_chain_add(&(lang->cache_head), &(lang->cache_tail), cache);
    return cache;
})


static inline mln_lang_ctx_t *
mln_lang_ctx_new(mln_lang_t *lang, void *data, mln_string_t *filename, mln_u32_t type, mln_string_t *content, mln_string_t *alias)
{
    mln_lang_ctx_t *ctx;
    struct mln_rbtree_attr rbattr;
    struct mln_gc_attr gcattr;
    mln_lang_scope_t *outer_scope;
    mln_string_t *base_scope, tmp = mln_string("__main__");

    if ((ctx = (mln_lang_ctx_t *)mln_alloc_m(lang->pool, sizeof(mln_lang_ctx_t))) == NULL) {
        return NULL;
    }
    ctx->lang = lang;
    if ((ctx->pool = mln_alloc_init(NULL)) == NULL) {
        mln_alloc_free(ctx);
        return NULL;
    }
    if ((ctx->fset = mln_fileset_init(M_LANG_MAX_OPENFILE)) == NULL) {
        mln_alloc_destroy(ctx->pool);
        mln_alloc_free(ctx);
        return NULL;
    }
    ctx->data = data;
    if (lang->cache) {
        ctx->cache = mln_lang_ast_cache_search(lang, type, content);
        if (ctx->cache == NULL) {
            ctx->stm = NULL;
        } else {
            ctx->stm = ctx->cache->stm;
            ++(ctx->cache->ref);
        }
    } else {
        ctx->cache = NULL;
        ctx->stm = (mln_lang_stm_t *)mln_lang_ast_generate(ctx->pool, lang->shift_table, content, type);
    }
    /* ctx->run_stack do not need to be initialized */
    ctx->run_stack_top = NULL;
    /* ctx->scopes do not need to be initialized */
    ctx->scope_top = NULL;
    ctx->ref = 0;
    ctx->filename = NULL;
    ctx->symbols = NULL;
    ctx->alias = NULL;
    rbattr.pool = ctx->pool;
    rbattr.pool_alloc = (rbtree_pool_alloc_handler)mln_alloc_m;
    rbattr.pool_free = (rbtree_pool_free_handler)mln_alloc_free;
    rbattr.cmp = (rbtree_cmp)mln_lang_resource_cmp;
    rbattr.data_free = (rbtree_free_data)mln_lang_ctx_resource_free_handler;
    if ((ctx->resource_set = mln_rbtree_new(&rbattr)) == NULL) {
        if (ctx->cache != NULL) {
            if (!(--(ctx->cache->ref))) {
                mln_lang_ast_cache_chain_del(&(ctx->lang->cache_head), &(ctx->lang->cache_tail), ctx->cache);
                mln_lang_ast_cache_free(ctx->cache);
            }
        } else {
            mln_lang_ast_free(ctx->stm);
        }
        mln_fileset_destroy(ctx->fset);
        mln_alloc_destroy(ctx->pool);
        mln_alloc_free(ctx);
        return NULL;
    }

    ctx->ret_var = NULL;
    ctx->return_handler = NULL;
    ctx->prev = ctx->next = NULL;
    ctx->sym_head = ctx->sym_tail = NULL;
#if !defined(MSVC)
    ctx->owner = 0;
#endif
    ctx->sym_count = 0;
    ctx->ret_flag = ctx->op_array_flag = ctx->op_bool_flag = ctx->op_func_flag = ctx->op_int_flag = \
    ctx->op_nil_flag = ctx->op_obj_flag = ctx->op_real_flag = ctx->op_str_flag = 0;
    ctx->quit = 0;

    gcattr.pool = ctx->pool;
    gcattr.item_getter = (gc_item_getter)mln_lang_gc_item_getter;
    gcattr.item_setter = (gc_item_setter)mln_lang_gc_item_setter;
    gcattr.item_freer = (gc_item_freer)mln_lang_gc_item_free;
    gcattr.member_setter = (gc_member_setter)mln_lang_gc_item_member_setter;
    gcattr.move_handler = (gc_move_handler)mln_lang_gc_item_move_handler;
    gcattr.root_setter = (gc_root_setter)mln_lang_gc_item_root_setter;
    gcattr.clean_searcher = (gc_clean_searcher)mln_lang_gc_item_clean_searcher;
    gcattr.free_handler = (gc_free_handler)mln_lang_gc_item_free_handler;
    if ((ctx->gc = mln_gc_new(&gcattr)) == NULL) {
        mln_lang_ctx_free(ctx);
        return NULL;
    }

    if ((ctx->symbols = mln_lang_hash_new(ctx)) == NULL) {
        mln_lang_ctx_free(ctx);
        return NULL;
    }

    if (ctx->stm != NULL) {
        mln_lang_stack_node_t *node = mln_lang_stack_push(ctx, M_LSNT_STM, ctx->stm);
        if (node == NULL) {
            mln_lang_ctx_free(ctx);
            return NULL;
        }
    }

    if ((base_scope = mln_string_pool_dup(ctx->pool, &tmp)) == NULL) {
        mln_lang_ctx_free(ctx);
        return NULL;
    }
    if ((outer_scope = mln_lang_scope_push(ctx, base_scope, M_LANG_SCOPE_TYPE_FUNC, NULL, ctx->stm)) == NULL) {
        mln_string_free(base_scope);
        mln_lang_ctx_free(ctx);
        return NULL;
    }
    mln_string_free(base_scope);

    if (filename != NULL) {
        if ((ctx->filename = mln_string_pool_dup(ctx->pool, filename)) == NULL) {
            mln_lang_ctx_free(ctx);
            return NULL;
        }
    }

    if (mln_lang_internal_func_installer(ctx) < 0) {
        mln_lang_ctx_free(ctx);
        return NULL;
    }

    if (alias != NULL) {
        mln_rbtree_node_t *rn;

        if ((ctx->alias = mln_string_pool_dup(ctx->pool, alias)) == NULL) {
            mln_lang_ctx_free(ctx);
            return NULL;
        }

        rn = mln_rbtree_search(ctx->lang->alias_set, ctx);
        if (!mln_rbtree_null(rn, ctx->lang->alias_set)) {
            mln_lang_ctx_free(ctx);
            return NULL;
        }
        if ((rn = mln_rbtree_node_new(ctx->lang->alias_set, ctx)) == NULL) {
            mln_lang_ctx_free(ctx);
            return NULL;
        }
        mln_rbtree_insert(ctx->lang->alias_set, rn);
    }
    return ctx;
}

MLN_FUNC_VOID(static inline, void, mln_lang_ctx_free, (mln_lang_ctx_t *ctx), (ctx), {
    if (ctx == NULL) return;
    mln_lang_symbol_node_t *sym;
    mln_rbtree_node_t *rn;

    if (ctx->alias != NULL) {
        rn = mln_rbtree_search(ctx->lang->alias_set, ctx);
        if (!mln_rbtree_null(rn, ctx->lang->alias_set)) {
            mln_rbtree_delete(ctx->lang->alias_set, rn);
            mln_rbtree_node_free(ctx->lang->alias_set, rn);
        }
        mln_string_free(ctx->alias);
    }
    if (ctx->ret_var != NULL) __mln_lang_var_free(ctx->ret_var);
    if (ctx->filename != NULL) mln_string_free(ctx->filename);
    while (mln_lang_stack_top(ctx) != NULL) {
        mln_lang_stack_node_free(mln_lang_stack_pop(ctx));
    }
    while (mln_lang_scope_top(ctx) != NULL) {
        mln_lang_scope_pop(ctx);
    }
    while ((sym = ctx->sym_head) != NULL) {
        mln_lang_sym_chain_del(&ctx->sym_head, &ctx->sym_tail, sym);
        sym->ctx = NULL;
        mln_lang_symbol_node_free(sym);
    }
    if (ctx->symbols != NULL) {
        mln_lang_hash_free(ctx->symbols);
    }
    if (ctx->gc != NULL) {
        mln_gc_free(ctx->gc);
    }
    if (ctx->resource_set != NULL) mln_rbtree_free(ctx->resource_set);
    if (ctx->stm != NULL) {
        if (ctx->cache) {
            if (!(--(ctx->cache->ref))) {
                mln_lang_ast_cache_chain_del(&(ctx->lang->cache_head), &(ctx->lang->cache_tail), ctx->cache);
                mln_lang_ast_cache_free(ctx->cache);
            }
        } else {
            mln_lang_ast_free(ctx->stm);
        }
    }
    mln_fileset_destroy(ctx->fset);
    mln_alloc_destroy(ctx->pool);
    mln_alloc_free(ctx);
})

MLN_FUNC(static, int, mln_lang_ctx_alias_cmp, \
         (mln_lang_ctx_t *ctx1, mln_lang_ctx_t *ctx2), (ctx1, ctx2), \
{
    return mln_string_strcmp(ctx1->alias, ctx2->alias);
})

MLN_FUNC_VOID(, void, mln_lang_ctx_suspend, (mln_lang_ctx_t *ctx), (ctx), {
    if (ctx->ref) return;
    ++(ctx->ref);
    mln_lang_ctx_chain_del(&(ctx->lang->run_head), &(ctx->lang->run_tail), ctx);
    mln_lang_ctx_chain_add(&(ctx->lang->wait_head), &(ctx->lang->wait_tail), ctx);
})

MLN_FUNC_VOID(, void, mln_lang_ctx_continue, (mln_lang_ctx_t *ctx), (ctx), {
    if (!ctx->ref) return;
    --(ctx->ref);
    mln_lang_ctx_chain_del(&(ctx->lang->wait_head), &(ctx->lang->wait_tail), ctx);
    mln_lang_ctx_chain_add(&(ctx->lang->run_head), &(ctx->lang->run_tail), ctx);
    if (ctx->lang->run_head != NULL) {
        ctx->lang->signal(ctx->lang);
    } else {
        ctx->lang->clear(ctx->lang);
    }
})

MLN_FUNC(static inline, mln_lang_set_detail_t *, mln_lang_ctx_get_class, \
         (mln_lang_ctx_t *ctx), (ctx), {
    mln_lang_hash_bucket_t *hb;
    mln_string_t *s, *name = NULL;
    mln_lang_symbol_node_t tmp, *sym;

    ASSERT(mln_lang_scope_in(ctx, mln_lang_scope_top(ctx)));
    if (mln_lang_scope_top(ctx)->type != M_LANG_SCOPE_TYPE_SET) return NULL;
    name = mln_lang_scope_top(ctx)->name;
    ASSERT(mln_lang_scope_in(ctx, mln_lang_scope_top(ctx)-1));

    tmp.symbol = name;
    tmp.layer = (mln_lang_scope_top(ctx) - 1)->layer;
    hb = mln_lang_hash_get_bucket(ctx->symbols, &tmp);
    for (sym = hb->tail; sym != NULL; sym = sym->prev) {
        s = sym->symbol;
        if (s->len != name->len || sym->layer != tmp.layer) continue;
        if (!memcmp(s->data, name->data, s->len)) {
            ASSERT(sym->type == M_LANG_SYMBOL_SET);
            return sym->data.set;
        }
    }
    return NULL;
})

MLN_FUNC(, int, mln_lang_ctx_global_var_add, \
         (mln_lang_ctx_t *ctx, mln_string_t *name, void *val, mln_u32_t type), \
         (ctx, name, val, type), \
{
    mln_lang_val_t *v;
    mln_lang_var_t *var;
    if ((v = __mln_lang_val_new(ctx, type, val)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if ((var = __mln_lang_var_new_ref_string(ctx, name, M_LANG_VAR_NORMAL, v, NULL)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        __mln_lang_val_free(v);
        return -1;
    }
    if (__mln_lang_symbol_node_join(ctx, M_LANG_SYMBOL_VAR, var) < 0) {
        __mln_lang_errmsg(ctx, "No memory.");
        __mln_lang_var_free(var);
        return -1;
    }
    return 0;
})

mln_lang_ctx_t *mln_lang_job_new(mln_lang_t *lang, mln_string_t *alias, mln_u32_t type, mln_string_t *data, void *udata, mln_lang_return_handler handler)
{
    mln_lang_ctx_t *ctx;
#if !defined(MSVC)
    pthread_mutex_lock(&lang->lock);
#endif
    ctx = __mln_lang_job_new(lang, alias, type, data, udata, handler);
#if !defined(MSVC)
    pthread_mutex_unlock(&lang->lock);
#endif
    return ctx;
}

MLN_FUNC(static inline, mln_lang_ctx_t *, __mln_lang_job_new, \
         (mln_lang_t *lang, mln_string_t *alias, mln_u32_t type, \
          mln_string_t *data, void *udata, mln_lang_return_handler handler), \
         (lang, alias, type, data, udata, handler), \
{
    mln_lang_ctx_t *ctx = mln_lang_ctx_new(lang, udata, type==M_INPUT_T_FILE? data: NULL, type, data, alias);
    if (ctx == NULL) {
        return NULL;
    }
    ctx->return_handler = handler;
    mln_lang_ctx_chain_add(&(lang->run_head), &(lang->run_tail), ctx);
    if (lang->run_head != NULL) {
        if (lang->signal(lang) < 0) {
            mln_lang_ctx_chain_del(&(lang->run_head), &(lang->run_tail), ctx);
            mln_lang_ctx_free(ctx);
            return NULL;
        }
    } else {
        lang->clear(lang);
    }
    return ctx;
})

void mln_lang_job_free(mln_lang_ctx_t *ctx)
{
    if (ctx == NULL) return;
    mln_lang_t *lang = ctx->lang;
#if !defined(MSVC)
    pthread_mutex_lock(&lang->lock);
#endif
    __mln_lang_job_free(ctx);
#if !defined(MSVC)
    pthread_mutex_unlock(&lang->lock);
#endif
}

MLN_FUNC_VOID(static inline, void, __mln_lang_job_free, (mln_lang_ctx_t *ctx), (ctx), {
    if (ctx == NULL) return;
    mln_lang_t *lang = ctx->lang;
    if (ctx->ref)
        mln_lang_ctx_chain_del(&(lang->wait_head), &(lang->wait_tail), ctx);
    else
        mln_lang_ctx_chain_del(&(lang->run_head), &(lang->run_tail), ctx);
    mln_lang_ctx_free(ctx);
    if (lang->run_head != NULL) {
        lang->signal(lang);
    } else {
        lang->clear(lang);
    }
})


MLN_FUNC(, mln_lang_var_t *, mln_lang_var_create_call, \
         (mln_lang_ctx_t *ctx, mln_lang_funccall_val_t *call), \
         (ctx, call), \
{
    return __mln_lang_var_create_call(ctx, call);
})

MLN_FUNC(static inline, mln_lang_var_t *, __mln_lang_var_create_call, \
         (mln_lang_ctx_t *ctx, mln_lang_funccall_val_t *call), (ctx, call), \
{
    mln_lang_val_t *val;
    mln_lang_var_t *var;
    if ((val = __mln_lang_val_new(ctx, M_LANG_VAL_TYPE_CALL, call)) == NULL) {
        return NULL;
    }
    if ((var = __mln_lang_var_new_ref_string(ctx, NULL, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        __mln_lang_val_free(val);
        return NULL;
    }
    return var;
})

MLN_FUNC(, mln_lang_var_t *, mln_lang_var_create_nil, \
         (mln_lang_ctx_t *ctx, mln_string_t *name), (ctx, name), \
{
    return __mln_lang_var_create_nil(ctx, name);
})

MLN_FUNC(static inline, mln_lang_var_t *, __mln_lang_var_create_nil, \
         (mln_lang_ctx_t *ctx, mln_string_t *name), (ctx, name), \
{
    mln_lang_val_t *val;
    mln_lang_var_t *var;
    if ((val = __mln_lang_val_new(ctx, M_LANG_VAL_TYPE_NIL, NULL)) == NULL) {
        return NULL;
    }
    if ((var = __mln_lang_var_new_ref_string(ctx, name, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        __mln_lang_val_free(val);
        return NULL;
    }
    return var;
})

MLN_FUNC(, mln_lang_var_t *, mln_lang_var_create_obj, \
         (mln_lang_ctx_t *ctx, mln_lang_set_detail_t *in_set, mln_string_t *name), \
         (ctx, in_set, name), \
{
    return __mln_lang_var_create_obj(ctx, in_set, name);
})

MLN_FUNC(static inline, mln_lang_var_t *, __mln_lang_var_create_obj, \
         (mln_lang_ctx_t *ctx, mln_lang_set_detail_t *in_set, mln_string_t *name), \
         (ctx, in_set, name), \
{
    mln_lang_val_t *val;
    mln_lang_var_t *var;
    mln_lang_object_t *obj;
    if ((obj = mln_lang_object_new(ctx, in_set)) == NULL) {
        return NULL;
    }
    if ((val = __mln_lang_val_new(ctx, M_LANG_VAL_TYPE_OBJECT, obj)) == NULL) {
        mln_lang_object_free(obj);
        return NULL;
    }
    if ((var = __mln_lang_var_new_ref_string(ctx, name, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        __mln_lang_val_free(val);
        return NULL;
    }
    return var;
})

MLN_FUNC(, mln_lang_var_t *, mln_lang_var_create_true, \
         (mln_lang_ctx_t *ctx, mln_string_t *name), (ctx, name), \
{
    return __mln_lang_var_create_true(ctx, name);
})

MLN_FUNC(static inline, mln_lang_var_t *, __mln_lang_var_create_true, \
         (mln_lang_ctx_t *ctx, mln_string_t *name), (ctx, name), \
{
    mln_lang_val_t *val;
    mln_lang_var_t *var;
    mln_u8_t t = 1;
    if ((val = __mln_lang_val_new(ctx, M_LANG_VAL_TYPE_BOOL, &t)) == NULL) {
        return NULL;
    }
    if ((var = __mln_lang_var_new_ref_string(ctx, name, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        __mln_lang_val_free(val);
        return NULL;
    }
    return var;
})

MLN_FUNC(, mln_lang_var_t *, mln_lang_var_create_false, \
         (mln_lang_ctx_t *ctx, mln_string_t *name), (ctx, name), \
{
    mln_lang_val_t *val;
    mln_lang_var_t *var;
    mln_u8_t t = 0;
    if ((val = __mln_lang_val_new(ctx, M_LANG_VAL_TYPE_BOOL, &t)) == NULL) {
        return NULL;
    }
    if ((var = __mln_lang_var_new_ref_string(ctx, name, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        __mln_lang_val_free(val);
        return NULL;
    }
    return var;
})

MLN_FUNC(, mln_lang_var_t *, mln_lang_var_create_int, \
         (mln_lang_ctx_t *ctx, mln_s64_t off, mln_string_t *name), \
         (ctx, off, name), \
{
    return __mln_lang_var_create_int(ctx, off, name);
})

MLN_FUNC(static inline, mln_lang_var_t *, __mln_lang_var_create_int, \
         (mln_lang_ctx_t *ctx, mln_s64_t off, mln_string_t *name), (ctx, off, name), \
{
    mln_lang_val_t *val;
    mln_lang_var_t *var;
    if ((val = __mln_lang_val_new(ctx, M_LANG_VAL_TYPE_INT, &off)) == NULL) {
        return NULL;
    }
    if ((var = __mln_lang_var_new_ref_string(ctx, name, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        __mln_lang_val_free(val);
        return NULL;
    }
    return var;
})

MLN_FUNC(, mln_lang_var_t *, mln_lang_var_create_real, \
         (mln_lang_ctx_t *ctx, double f, mln_string_t *name), (ctx, f, name), \
{
    return __mln_lang_var_create_real(ctx, f, name);
})

MLN_FUNC(static inline, mln_lang_var_t *, __mln_lang_var_create_real, \
         (mln_lang_ctx_t *ctx, double f, mln_string_t *name), (ctx, f, name), \
{
    mln_lang_val_t *val;
    mln_lang_var_t *var;
    if ((val = __mln_lang_val_new(ctx, M_LANG_VAL_TYPE_REAL, &f)) == NULL) {
        return NULL;
    }
    if ((var = __mln_lang_var_new_ref_string(ctx, name, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        __mln_lang_val_free(val);
        return NULL;
    }
    return var;
})

MLN_FUNC(, mln_lang_var_t *, mln_lang_var_create_bool, \
         (mln_lang_ctx_t *ctx, mln_u8_t b, mln_string_t *name), (ctx, b, name), \
{
    return __mln_lang_var_create_bool(ctx, b, name);
})

MLN_FUNC(static inline, mln_lang_var_t *, __mln_lang_var_create_bool, \
         (mln_lang_ctx_t *ctx, mln_u8_t b, mln_string_t *name), (ctx, b, name), \
{
    mln_lang_val_t *val;
    mln_lang_var_t *var;
    if ((val = __mln_lang_val_new(ctx, M_LANG_VAL_TYPE_BOOL, &b)) == NULL) {
        return NULL;
    }
    if ((var = __mln_lang_var_new_ref_string(ctx, name, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        __mln_lang_val_free(val);
        return NULL;
    }
    return var;
})

MLN_FUNC(, mln_lang_var_t *, mln_lang_var_create_ref_string, \
         (mln_lang_ctx_t *ctx, mln_string_t *s, mln_string_t *name), (ctx, s, name), \
{
    return __mln_lang_var_create_ref_string(ctx, s, name);
})

MLN_FUNC(static inline, mln_lang_var_t *, __mln_lang_var_create_ref_string, \
         (mln_lang_ctx_t *ctx, mln_string_t *s, mln_string_t *name), (ctx, s, name), \
{
    mln_lang_val_t *val;
    mln_lang_var_t *var;
    if ((val = __mln_lang_val_new(ctx, M_LANG_VAL_TYPE_STRING, mln_string_ref(s))) == NULL) {
        mln_string_free(s);
        return NULL;
    }
    if ((var = __mln_lang_var_new_ref_string(ctx, name, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        __mln_lang_val_free(val);
        return NULL;
    }
    return var;
})

MLN_FUNC(, mln_lang_var_t *, mln_lang_var_create_string, \
         (mln_lang_ctx_t *ctx, mln_string_t *s, mln_string_t *name), (ctx, s, name), \
{
    mln_lang_val_t *val;
    mln_lang_var_t *var;
    mln_string_t *dup;
    if ((dup = mln_string_pool_dup(ctx->pool, s)) == NULL) {
        return NULL;
    }
    if ((val = __mln_lang_val_new(ctx, M_LANG_VAL_TYPE_STRING, dup)) == NULL) {
        mln_string_free(dup);
        return NULL;
    }
    if ((var = __mln_lang_var_new_ref_string(ctx, name, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        __mln_lang_val_free(val);
        return NULL;
    }
    return var;
})

MLN_FUNC(, mln_lang_var_t *, mln_lang_var_create_array, \
         (mln_lang_ctx_t *ctx, mln_string_t *name), (ctx, name), \
{
    return __mln_lang_var_create_array(ctx, name);
})

MLN_FUNC(static inline, mln_lang_var_t *, __mln_lang_var_create_array, \
         (mln_lang_ctx_t *ctx, mln_string_t *name), (ctx, name), \
{
    mln_lang_val_t *val;
    mln_lang_var_t *var;
    mln_lang_array_t *array;
    if ((array = __mln_lang_array_new(ctx)) == NULL) {
        return NULL;
    }
    if ((val = __mln_lang_val_new(ctx, M_LANG_VAL_TYPE_ARRAY, array)) == NULL) {
        __mln_lang_array_free(array);
        return NULL;
    }
    if ((var = __mln_lang_var_new_ref_string(ctx, name, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        __mln_lang_val_free(val);
        return NULL;
    }
    return var;
})

MLN_FUNC_VOID(static inline, void, mln_lang_stack_node_get_ctx_ret_var, \
              (mln_lang_stack_node_t *node, mln_lang_ctx_t *ctx), (node, ctx), \
{
    if (node->ret_var != NULL) __mln_lang_var_free(node->ret_var);
    node->ret_var = ctx->ret_var;
    ctx->ret_var = NULL;
})

MLN_FUNC_VOID(static inline, void, mln_lang_stack_node_reset_ret_val, \
              (mln_lang_stack_node_t *node), (node), \
{
    if (node->ret_var == NULL) return;
    __mln_lang_var_free(node->ret_var);
    node->ret_var = NULL;
})

MLN_FUNC_VOID(static inline, void, mln_lang_stack_node_set_ret_var, \
              (mln_lang_stack_node_t *node, mln_lang_var_t *ret_var), (node, ret_var), \
{
    mln_lang_stack_node_reset_ret_val(node);
    node->ret_var = ret_var;
})

MLN_FUNC_VOID(static inline, void, mln_lang_ctx_reset_ret_var, (mln_lang_ctx_t *ctx), (ctx), {
    if (ctx->ret_var == NULL) return;
    __mln_lang_var_free(ctx->ret_var);
    ctx->ret_var = NULL;
})

MLN_FUNC_VOID(, void, mln_lang_ctx_set_ret_var, \
              (mln_lang_ctx_t *ctx, mln_lang_var_t *var), (ctx, var), \
{
    __mln_lang_ctx_set_ret_var(ctx, var);
})

MLN_FUNC_VOID(static inline, void, __mln_lang_ctx_set_ret_var, \
              (mln_lang_ctx_t *ctx, mln_lang_var_t *var), (ctx, var), \
{
    mln_lang_ctx_reset_ret_var(ctx);
    ctx->ret_var = var;
})

MLN_FUNC_VOID(static inline, void, mln_lang_ctx_get_node_ret_val, \
              (mln_lang_ctx_t *ctx, mln_lang_stack_node_t *node), (ctx, node), \
{
    if (ctx->ret_var != NULL) __mln_lang_var_free(ctx->ret_var);
    ctx->ret_var = node->ret_var;
    node->ret_var = NULL;
})

MLN_FUNC(static inline, mln_lang_symbol_node_t *, mln_lang_symbol_node_new, \
         (mln_lang_ctx_t *ctx, mln_string_t *symbol, mln_lang_symbol_type_t type, void *data), \
         (ctx, symbol, type, data), \
{
    mln_lang_symbol_node_t *ls;
    if ((ls = ctx->sym_head) != NULL) {
        mln_lang_sym_chain_del(&(ctx->sym_head), &(ctx->sym_tail), ls);
        --(ctx->sym_count);
    } else {
        if ((ls = (mln_lang_symbol_node_t *)mln_alloc_m(ctx->pool, sizeof(mln_lang_symbol_node_t))) == NULL) {
            return NULL;
        }
        ls->ctx = ctx;
        ls->prev = ls->next = NULL;
    }
    ls->symbol = symbol;
    ls->type = type;
    switch (type) {
        case M_LANG_SYMBOL_VAR:
            ls->data.var = (mln_lang_var_t *)data;
            break;
        default: /*M_LANG_SYMBOL_SET*/
            ls->data.set = (mln_lang_set_detail_t *)data;
            break;
    }
    ls->layer = mln_lang_scope_top(ctx)->layer;
    ls->bucket = NULL;
    ls->scope_prev = ls->scope_next = NULL;
    return ls;
})

MLN_FUNC_VOID(static, void, mln_lang_symbol_node_free, (void *data), (data), {
    if (data == NULL) return;
    mln_lang_symbol_node_t *ls = (mln_lang_symbol_node_t *)data;
    switch (ls->type) {
        case M_LANG_SYMBOL_VAR:
            if (ls->data.var != NULL) {
                __mln_lang_var_free(ls->data.var);
                ls->data.var = NULL;
            }
            break;
        default: /*M_LANG_SYMBOL_SET*/
            if (ls->data.set != NULL) {
                mln_lang_set_detail_self_free(ls->data.set);
                ls->data.set = NULL;
            }
            break;
    }
    if (ls->ctx != NULL && ls->ctx->sym_count < M_LANG_CACHE_COUNT) {
        mln_lang_sym_chain_add(&(ls->ctx->sym_head), &(ls->ctx->sym_tail), ls);
        ++(ls->ctx->sym_count);
    } else {
        mln_alloc_free(ls);
    }
})

MLN_FUNC(, int, mln_lang_symbol_node_join, \
         (mln_lang_ctx_t *ctx, mln_lang_symbol_type_t type, void *data), \
         (ctx, type, data), \
{
    return __mln_lang_symbol_node_join(ctx, type, data);
})

MLN_FUNC(static inline, int, __mln_lang_symbol_node_join, \
         (mln_lang_ctx_t *ctx, mln_lang_symbol_type_t type, void *data), \
         (ctx, type, data), \
{
    mln_lang_symbol_node_t *symbol, *tmp;
    mln_string_t *name = type == M_LANG_SYMBOL_VAR? ((mln_lang_var_t *)data)->name: ((mln_lang_set_detail_t *)data)->name;

    if ((symbol = mln_lang_symbol_node_new(ctx, name, type, data)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    symbol->bucket = mln_lang_hash_get_bucket(ctx->symbols, symbol);
    for (tmp = symbol->bucket->tail; tmp != NULL; tmp = tmp->prev) {
         if (tmp->layer != symbol->layer || tmp->symbol->len != name->len) continue;
         if (!memcmp(tmp->symbol->data, name->data, name->len)) {
             mln_lang_sym_chain_del(&(tmp->bucket->head), &(tmp->bucket->tail), tmp);
             mln_lang_sym_scope_chain_del(&(mln_lang_scope_top(ctx)->sym_head), &(mln_lang_scope_top(ctx)->sym_tail), tmp);
             mln_lang_symbol_node_free(tmp);
             break;
         }
    }
    mln_lang_sym_chain_add(&(symbol->bucket->head), &(symbol->bucket->tail), symbol);
    mln_lang_sym_scope_chain_add(&(mln_lang_scope_top(ctx)->sym_head), &(mln_lang_scope_top(ctx)->sym_tail), symbol);

    return 0;
})

MLN_FUNC(, int, mln_lang_symbol_node_upper_join, \
         (mln_lang_ctx_t *ctx, mln_lang_symbol_type_t type, void *data), \
         (ctx, type, data), \
{
    mln_lang_symbol_node_t *symbol, *tmp;
    mln_string_t *name = type == M_LANG_SYMBOL_VAR? ((mln_lang_var_t *)data)->name: ((mln_lang_set_detail_t *)data)->name;

    if ((symbol = mln_lang_symbol_node_new(ctx, name, type, data)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    symbol->layer = !mln_lang_scope_in(ctx, mln_lang_scope_top(ctx)-1)? (mln_lang_scope_top(ctx)->layer): ((mln_lang_scope_top(ctx)-1)->layer);
    symbol->bucket = mln_lang_hash_get_bucket(ctx->symbols, symbol);
    for (tmp = symbol->bucket->tail; tmp != NULL; tmp = tmp->prev) {
         if (tmp->layer != symbol->layer || tmp->symbol->len != name->len) continue;
         if (!memcmp(tmp->symbol->data, name->data, name->len)) {
             mln_lang_sym_chain_del(&(tmp->bucket->head), &(tmp->bucket->tail), tmp);
             if (!mln_lang_scope_in(ctx, mln_lang_scope_top(ctx)-1))
                 mln_lang_sym_scope_chain_del(&(mln_lang_scope_top(ctx)->sym_head), &(mln_lang_scope_top(ctx)->sym_tail), tmp);
             else
                 mln_lang_sym_scope_chain_del(&((mln_lang_scope_top(ctx)-1)->sym_head), &((mln_lang_scope_top(ctx)-1)->sym_tail), tmp);
             mln_lang_symbol_node_free(tmp);
             break;
         }
    }
    mln_lang_sym_chain_add(&(symbol->bucket->head), &(symbol->bucket->tail), symbol);
    if (!mln_lang_scope_in(ctx, mln_lang_scope_top(ctx)-1))
        mln_lang_sym_scope_chain_add(&(mln_lang_scope_top(ctx)->sym_head), &(mln_lang_scope_top(ctx)->sym_tail), symbol);
    else
        mln_lang_sym_scope_chain_add(&((mln_lang_scope_top(ctx)-1)->sym_head), &((mln_lang_scope_top(ctx)-1)->sym_tail), symbol);

    return 0;
})

MLN_FUNC(, mln_lang_symbol_node_t *, mln_lang_symbol_node_search, \
         (mln_lang_ctx_t *ctx, mln_string_t *name, int local), (ctx, name, local), \
{
    return __mln_lang_symbol_node_search(ctx, name, local);
})

MLN_FUNC(static inline, mln_lang_symbol_node_t *, __mln_lang_symbol_node_search, \
         (mln_lang_ctx_t *ctx, mln_string_t *name, int local), (ctx, name, local), \
{
    mln_lang_symbol_node_t *sym, tmp;
    mln_lang_hash_bucket_t *hb;
    mln_lang_scope_t *scope = mln_lang_scope_top(ctx);

    tmp.symbol = name;

    while (scope != NULL) {
        tmp.layer = scope->layer;
        hb = mln_lang_hash_get_bucket(ctx->symbols, &tmp);

        for (sym = hb->tail; sym != NULL; sym = sym->prev) {
            if (sym->layer != scope->layer || sym->symbol->len != name->len) continue;
            if (!memcmp(sym->symbol->data, name->data, name->len)) return sym;
        }
        if (local || scope == mln_lang_scope_base(ctx)) break;
        scope = mln_lang_scope_base(ctx);
    }
    return NULL;
})

MLN_FUNC(static inline, mln_lang_symbol_node_t *, mln_lang_symbol_node_id_search, \
         (mln_lang_ctx_t *ctx, mln_string_t *name), (ctx, name), \
{
    return __mln_lang_symbol_node_search(ctx, name, (name->len > 0 && name->data[0] > 64 && name->data[0] < 91)? 0: 1);
})


MLN_FUNC(, mln_lang_set_detail_t *, mln_lang_set_detail_new, \
         (mln_alloc_t *pool, mln_string_t *name), (pool, name), \
{
    struct mln_rbtree_attr rbattr;
    mln_lang_set_detail_t *lcd;
    if ((lcd = (mln_lang_set_detail_t *)mln_alloc_m(pool, sizeof(mln_lang_set_detail_t))) == NULL) {
        return NULL;
    }
    if ((lcd->name = mln_string_pool_dup(pool, name)) == NULL) {
        mln_alloc_free(lcd);
        return NULL;
    }
    rbattr.pool = pool;
    rbattr.pool_alloc = (rbtree_pool_alloc_handler)mln_alloc_m;
    rbattr.pool_free = (rbtree_pool_free_handler)mln_alloc_free;
    rbattr.cmp = mln_lang_var_name_cmp;
    rbattr.data_free = mln_lang_var_free;
    if ((lcd->members = mln_rbtree_new(&rbattr)) == NULL) {
        mln_alloc_free(lcd);
        return NULL;
    }
    lcd->ref = 0;
    return lcd;
})

MLN_FUNC_VOID(, void, mln_lang_set_detail_free, (mln_lang_set_detail_t *c), (c), {
    if (c == NULL) return;
    if (c->ref-- > 1) return;
    if (c->name != NULL) mln_string_free(c->name);
    if (c->members != NULL) mln_rbtree_free(c->members);
    mln_alloc_free(c);
})

MLN_FUNC_VOID(, void, mln_lang_set_detail_self_free, (mln_lang_set_detail_t *c), (c), {
    if (c == NULL) return;
    if (c->members != NULL) {
        mln_rbtree_free(c->members);
        c->members = NULL;
    }
    if (c->ref-- > 1) return;
    if (c->name != NULL) mln_string_free(c->name);
    mln_alloc_free(c);
})


MLN_FUNC(, int, mln_lang_set_member_add, \
         (mln_alloc_t *pool, mln_rbtree_t *members, mln_lang_var_t *var), \
         (pool, members, var), \
{
    mln_rbtree_node_t *rn;
    rn = mln_rbtree_search(members, var);
    if (!mln_rbtree_null(rn, members)) {
        mln_lang_var_t *tmp = (mln_lang_var_t *)mln_rbtree_node_data_get(rn);
        __mln_lang_var_assign(tmp, var->val);
        __mln_lang_var_free(var);
        return 0;
    }
    if ((rn = mln_rbtree_node_new(members, var)) == NULL) {
        return -1;
    }
    mln_rbtree_insert(members, rn);
    return 0;
})

MLN_FUNC(, mln_lang_var_t *, mln_lang_set_member_search, \
         (mln_rbtree_t *members, mln_string_t *name), (members, name), \
{
    return __mln_lang_set_member_search(members, name);
})

MLN_FUNC(static inline, mln_lang_var_t *, __mln_lang_set_member_search, \
         (mln_rbtree_t *members, mln_string_t *name), (members, name), \
{
    mln_rbtree_node_t *rn;
    mln_lang_var_t var;
    var.name = name;
    rn = mln_rbtree_search(members, &var);
    if (mln_rbtree_null(rn, members)) return NULL;
    return (mln_lang_var_t *)mln_rbtree_node_data_get(rn);
})

MLN_FUNC(static, int, mln_lang_set_member_iterate_handler, \
         (mln_rbtree_node_t *node, void *udata), (node, udata), \
{
    mln_lang_var_t *var, *lv = (mln_lang_var_t *)mln_rbtree_node_data_get(node);
    struct mln_lang_scan_s *ls = (struct mln_lang_scan_s *)udata;
    mln_rbtree_t *tree = ls->tree;
    mln_rbtree_node_t *rn;

    rn = mln_rbtree_search(tree, lv);
    if (!mln_rbtree_null(rn, tree)) {
        ASSERT(0);/*do nothing*/
    }

    if ((var = __mln_lang_var_dup(ls->ctx, lv)) == NULL) {
        return -1;
    }
    if (mln_lang_set_member_add(ls->ctx->pool, tree, var) < 0) {
        __mln_lang_var_free(var);
        return -1;
    }
    return 0;
})

MLN_FUNC(, int, mln_lang_object_add_member, \
         (mln_lang_ctx_t *ctx, mln_lang_object_t *obj, mln_lang_var_t *var), \
         (ctx, obj, var), \
{
    mln_lang_var_t *dup;
    mln_rbtree_node_t *rn;

    rn = mln_rbtree_search(obj->members, var);
    if (!mln_rbtree_null(rn, obj->members)) {
        ASSERT(0);/*do nothing*/
        return -1;
    }
    if ((dup = __mln_lang_var_dup(ctx, var)) == NULL) {
        return -1;
    }
    if (mln_lang_set_member_add(ctx->pool, obj->members, dup) < 0) {
        __mln_lang_var_free(dup);
        return -1;
    }
    return 0;
})


MLN_FUNC(, mln_lang_var_t *, mln_lang_var_new, \
         (mln_lang_ctx_t *ctx, mln_string_t *name, mln_lang_var_type_t type, \
          mln_lang_val_t *val, mln_lang_set_detail_t *in_set), \
         (ctx, name, type, val, in_set), \
{
    return __mln_lang_var_new(ctx, name, type, val, in_set);
})

MLN_FUNC(static inline, mln_lang_var_t *, __mln_lang_var_new, \
         (mln_lang_ctx_t *ctx, mln_string_t *name, mln_lang_var_type_t type, \
          mln_lang_val_t *val, mln_lang_set_detail_t *in_set), \
         (ctx, name, type, val, in_set), \
{
    mln_lang_var_t *var;
    if ((var = (mln_lang_var_t *)mln_alloc_m(ctx->pool, sizeof(mln_lang_var_t))) == NULL) {
        return NULL;
    }
    var->type = type;
    if (name != NULL) {
        if ((var->name = mln_string_pool_dup(ctx->pool, name)) == NULL) {
            mln_alloc_free(var);
            return NULL;
        }
    } else {
        var->name = NULL;
    }
    var->val = val;
    if (val != NULL) ++(val->ref);
    var->in_set = in_set;
    if (in_set != NULL) ++(in_set->ref);
    var->prev = var->next = NULL;
    var->ref = 0;
    return var;
})

MLN_FUNC(static inline, mln_lang_var_t *, __mln_lang_var_new_ref_string, \
         (mln_lang_ctx_t *ctx, mln_string_t *name, mln_lang_var_type_t type, \
          mln_lang_val_t *val, mln_lang_set_detail_t *in_set), \
         (ctx, name, type, val, in_set), \
{
    mln_lang_var_t *var;
    if ((var = (mln_lang_var_t *)mln_alloc_m(ctx->pool, sizeof(mln_lang_var_t))) == NULL) {
        return NULL;
    }
    var->type = type;
    if (name != NULL) {
        var->name = mln_string_ref(name);
    } else {
        var->name = NULL;
    }
    var->val = val;
    if (val != NULL) ++(val->ref);
    var->in_set = in_set;
    if (in_set != NULL) ++(in_set->ref);
    var->prev = var->next = NULL;
    var->ref = 0;
    return var;
})

MLN_FUNC(static inline, mln_lang_var_t *, mln_lang_var_transform, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t *realvar, mln_lang_var_t *defvar), \
         (ctx, realvar, defvar), \
{
    mln_lang_var_t *var;
    if ((var = (mln_lang_var_t *)mln_alloc_m(ctx->pool, sizeof(mln_lang_var_t))) == NULL) {
        return NULL;
    }
    var->type = defvar->type;
    ASSERT(defvar->name != NULL);
    var->name = mln_string_ref(defvar->name);
    if (var->type == M_LANG_VAR_NORMAL) {
        if ((var->val = mln_lang_val_dup(ctx, realvar->val)) == NULL) {
            mln_string_free(var->name);
            mln_alloc_free(var);
            return NULL;
        }
    } else {
        var->val = realvar->val;
    }
    ASSERT(var->val != NULL);
    ++(var->val->ref);
    var->in_set = defvar->in_set;
    if (var->in_set != NULL) ++(var->in_set->ref);
    var->prev = var->next = NULL;
    var->ref = 0;
    return var;
})

MLN_FUNC_VOID(, void, mln_lang_var_free, (void *data), (data), {
    __mln_lang_var_free(data);
})

MLN_FUNC_VOID(static, void, mln_lang_var_pfree, (mln_lang_var_t **v), (v), {
    __mln_lang_var_free(*v);
})

MLN_FUNC_VOID(static inline, void, __mln_lang_var_free, (void *data), (data), {
    if (data == NULL) return;
    mln_lang_var_t *var = (mln_lang_var_t *)data;
    if (var->ref-- > 0) return;
    if (var->name != NULL) {
        mln_string_free(var->name);
        var->name = NULL;
    }
    if (var->val != NULL) {
        __mln_lang_val_free(var->val);
        var->val = NULL;
    }
    if (var->in_set != NULL) {
        mln_lang_set_detail_free(var->in_set);
        var->in_set = NULL;
    }
    mln_alloc_free(var);
})

MLN_FUNC(, mln_lang_var_t *, mln_lang_var_dup, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t *var), (ctx, var), \
{
    return __mln_lang_var_dup(ctx, var);
})

MLN_FUNC(static inline, mln_lang_var_t *, __mln_lang_var_dup, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t *var), (ctx, var), \
{
    mln_lang_var_t *ret;
    mln_lang_val_t *val = var->val;
    if (var->type == M_LANG_VAR_NORMAL && var->val != NULL) {
        if ((val = mln_lang_val_dup(ctx, var->val)) == NULL) {
            return NULL;
        }
    }
    if ((ret = __mln_lang_var_new_ref_string(ctx, var->name, var->type, val, var->in_set)) == NULL) {
        if (var->type == M_LANG_VAR_NORMAL && var->val != NULL) {
            __mln_lang_val_free(val);
        }
        return NULL;
    }
    return ret;
})

MLN_FUNC(static inline, mln_lang_var_t *, mln_lang_var_dup_with_val, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t *var), (ctx, var), \
{
    mln_lang_var_t *ret;
    mln_lang_val_t *val;
    if ((val = mln_lang_val_dup(ctx, var->val)) == NULL) {
        return NULL;
    }
    if ((ret = __mln_lang_var_new(ctx, var->name, M_LANG_VAR_NORMAL, val, var->in_set)) == NULL) {
        __mln_lang_val_free(val);
        return NULL;
    }
    return ret;
})

MLN_FUNC(, int, mln_lang_var_cmp, (const void *data1, const void *data2), (data1, data2), {
    mln_lang_var_t *v1 = (mln_lang_var_t *)data1;
    mln_lang_var_t *v2 = (mln_lang_var_t *)data2;
    if (v1 == NULL && v2 != NULL) return -1;
    else if (v1 != NULL && v2 == NULL) return 1;
    if (v1->val == NULL && v2->val != NULL) return -1;
    else if (v1->val != NULL && v2->val == NULL) return 1;
    return mln_lang_val_cmp(v1->val, v2->val);
})

MLN_FUNC(static, int, mln_lang_var_name_cmp, \
         (const void *data1, const void *data2), (data1, data2), \
{
    mln_lang_var_t *v1 = (mln_lang_var_t *)data1;
    mln_lang_var_t *v2 = (mln_lang_var_t *)data2;
    return mln_string_strcmp(v1->name, v2->name);
})

MLN_FUNC_VOID(, void, mln_lang_var_assign, \
              (mln_lang_var_t *var, mln_lang_val_t *val), (var, val), \
{
    __mln_lang_var_assign(var, val);
})

MLN_FUNC_VOID(static inline, void, __mln_lang_var_assign, \
              (mln_lang_var_t *var, mln_lang_val_t *val), (var, val), \
{
    ASSERT(val != NULL);
    if (val != NULL) ++(val->ref);
    if (var->val != NULL) __mln_lang_val_free(var->val);
    var->val = val;
})

MLN_FUNC(, int, mln_lang_var_value_set, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t *dest, mln_lang_var_t *src), \
         (ctx, dest, src), \
{
    return __mln_lang_var_value_set(ctx, dest, src);
})

MLN_FUNC(static inline, int, __mln_lang_var_value_set, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t *dest, mln_lang_var_t *src), \
         (ctx, dest, src), \
{
    ASSERT(dest != NULL && dest->val != NULL && src != NULL && src->val != NULL);
    mln_lang_val_t *val1 = dest->val;
    mln_lang_val_t *val2 = src->val;
    mln_lang_val_data_free(val1);
    switch (val2->type) {
        case M_LANG_VAL_TYPE_NIL:
            val1->type = M_LANG_VAL_TYPE_NIL;
            val1->data.s = NULL;
            break;
        case M_LANG_VAL_TYPE_INT:
            val1->type = M_LANG_VAL_TYPE_INT;
            val1->data.i = val2->data.i;
            break;
        case M_LANG_VAL_TYPE_BOOL:
            val1->type = M_LANG_VAL_TYPE_BOOL;
            val1->data.b = val2->data.b;
            break;
        case M_LANG_VAL_TYPE_REAL:
            val1->type = M_LANG_VAL_TYPE_REAL;
            val1->data.f = val2->data.f;
            break;
        case M_LANG_VAL_TYPE_STRING:
            val1->type = M_LANG_VAL_TYPE_STRING;
            if ((val1->data.s = mln_string_pool_dup(ctx->pool, val2->data.s)) == NULL) {
                return -1;
            }
            break;
        case M_LANG_VAL_TYPE_OBJECT:
            val1->type = M_LANG_VAL_TYPE_OBJECT;
            val1->data.obj = val2->data.obj;
            ++(val1->data.obj->ref);
            break;
        case M_LANG_VAL_TYPE_FUNC:
            val1->type = M_LANG_VAL_TYPE_FUNC;
            if ((val1->data.func = mln_lang_func_detail_dup(ctx, val2->data.func)) == NULL) {
                return -1;
            }
            break;
        case M_LANG_VAL_TYPE_ARRAY:
            val1->type = M_LANG_VAL_TYPE_ARRAY;
            val1->data.array = val2->data.array;
            ++(val1->data.array->ref);
            break;
        default:
            ASSERT(0);
            return -1;
    }
    return 0;
})

MLN_FUNC(, int, mln_lang_var_value_set_string_ref, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t *dest, mln_lang_var_t *src), (ctx, dest, src), \
{
    /*
     * Note:
     * ref string cannot be used in the case that string data can be modified.
     */
    ASSERT(dest != NULL && dest->val != NULL && src != NULL && src->val != NULL);
    mln_lang_val_t *val1 = dest->val;
    mln_lang_val_t *val2 = src->val;
    mln_lang_val_data_free(val1);
    switch (val2->type) {
        case M_LANG_VAL_TYPE_NIL:
            val1->type = M_LANG_VAL_TYPE_NIL;
            val1->data.s = NULL;
            break;
        case M_LANG_VAL_TYPE_INT:
            val1->type = M_LANG_VAL_TYPE_INT;
            val1->data.i = val2->data.i;
            break;
        case M_LANG_VAL_TYPE_BOOL:
            val1->type = M_LANG_VAL_TYPE_BOOL;
            val1->data.b = val2->data.b;
            break;
        case M_LANG_VAL_TYPE_REAL:
            val1->type = M_LANG_VAL_TYPE_REAL;
            val1->data.f = val2->data.f;
            break;
        case M_LANG_VAL_TYPE_STRING:
            val1->type = M_LANG_VAL_TYPE_STRING;
            val1->data.s = mln_string_ref(val2->data.s);
            break;
        case M_LANG_VAL_TYPE_OBJECT:
            val1->type = M_LANG_VAL_TYPE_OBJECT;
            val1->data.obj = val2->data.obj;
            ++(val1->data.obj->ref);
            break;
        case M_LANG_VAL_TYPE_FUNC:
            val1->type = M_LANG_VAL_TYPE_FUNC;
            if ((val1->data.func = mln_lang_func_detail_dup(ctx, val2->data.func)) == NULL) {
                return -1;
            }
            break;
        case M_LANG_VAL_TYPE_ARRAY:
            val1->type = M_LANG_VAL_TYPE_ARRAY;
            val1->data.array = val2->data.array;
            ++(val1->data.array->ref);
            break;
        default:
            ASSERT(0);
            return -1;
    }
    return 0;
})

MLN_FUNC_VOID(, void, mln_lang_var_set_int, (mln_lang_var_t *var, mln_s64_t i), (var, i), {
    ASSERT(var != NULL && var->val != NULL);
    mln_lang_val_t *val = var->val;
    mln_lang_val_data_free(val);
    val->type = M_LANG_VAL_TYPE_INT;
    val->data.i = i;
})

MLN_FUNC_VOID(, void, mln_lang_var_set_real, (mln_lang_var_t *var, double r), (var, r), {
    ASSERT(var != NULL && var->val != NULL);
    mln_lang_val_t *val = var->val;
    mln_lang_val_data_free(val);
    val->type = M_LANG_VAL_TYPE_REAL;
    val->data.f = r;
})

MLN_FUNC_VOID(, void, mln_lang_var_set_string, (mln_lang_var_t *var, mln_string_t *s), (var, s), {
    ASSERT(var != NULL && var->val != NULL);
    mln_lang_val_t *val = var->val;
    mln_lang_val_data_free(val);
    val->type = M_LANG_VAL_TYPE_STRING;
    val->data.s = s;
})


MLN_FUNC(static inline, int, mln_lang_funcdef_args_get, \
         (mln_lang_ctx_t *ctx, mln_lang_exp_t *exp, mln_array_t *arr, int is_closure), \
         (ctx, exp, arr, is_closure), \
{
    mln_lang_exp_t *scan;
    mln_lang_var_t *var, **v;
    mln_lang_factor_t *factor;
    mln_lang_spec_t *spec;
    mln_lang_symbol_node_t *sym;
    mln_lang_var_type_t type = M_LANG_VAR_NORMAL;
    for (scan = exp; scan != NULL; scan = scan->next) {
        if (scan->jump == NULL) {
            mln_lang_generate_jump_ptr(scan, M_LSNT_EXP);
        }
        if (scan->type == M_LSNT_SPEC) {
            spec = (mln_lang_spec_t *)(scan->jump);
            if (spec->op == M_SPEC_REFER) {
                spec = spec->data.spec;
                type = M_LANG_VAR_REFER;
            }
            if (spec->op != M_SPEC_FACTOR) {
                return -1;
            }

            factor = spec->data.factor;
        } else if (scan->type == M_LSNT_FACTOR) {
            factor = (mln_lang_factor_t *)(scan->jump);
        } else {
            return -1;
        }
        if (factor->type != M_FACTOR_ID) {
            return -1;
        }

        if ((v = (mln_lang_var_t **)mln_array_push(arr)) == NULL) {
            return -1;
        }
        if ((*v = __mln_lang_var_new(ctx, factor->data.s_id, type, NULL, NULL)) == NULL) {
            return -1;
        }
        if (is_closure) {
            sym = mln_lang_symbol_node_id_search(ctx, factor->data.s_id);
            if (sym != NULL) {
                if (sym->type != M_LANG_SYMBOL_VAR) {
                    return -1;
                }
                if ((var = mln_lang_var_transform(ctx, sym->data.var, *v)) == NULL) {
                    return -1;
                }
                __mln_lang_var_free(*v);
                *v = var;
            }
        }
    }

    return 0;
})


MLN_FUNC(, mln_lang_func_detail_t *, mln_lang_func_detail_new, \
         (mln_lang_ctx_t *ctx, mln_lang_func_type_t type, void *data, \
          mln_lang_exp_t *exp, mln_lang_exp_t *closure), \
         (ctx, type, data, exp, closure), \
{
    return __mln_lang_func_detail_new(ctx, type, data, exp, closure);
})

MLN_FUNC(static inline, mln_lang_func_detail_t *, __mln_lang_func_detail_new, \
         (mln_lang_ctx_t *ctx, mln_lang_func_type_t type, void *data, \
          mln_lang_exp_t *exp, mln_lang_exp_t *closure), \
         (ctx, type, data, exp, closure), \
{
    mln_lang_func_detail_t *lfd;
    if ((lfd = (mln_lang_func_detail_t *)mln_alloc_m(ctx->pool, sizeof(mln_lang_func_detail_t))) == NULL) {
        return NULL;
    }
    lfd->exp = exp;

    if (mln_array_pool_init(&(lfd->args), \
                            (array_free)mln_lang_var_pfree, \
                            sizeof(mln_lang_var_t *), \
                            M_LANG_ARRAY_PREALLOC, \
                            ctx->pool, \
                            (array_pool_alloc_handler)mln_alloc_m, \
                            (array_pool_free_handler)mln_alloc_free) < 0)
    {
        mln_alloc_free(lfd);
        return NULL;
    }
    if (mln_array_pool_init(&(lfd->closure), \
                            (array_free)mln_lang_var_pfree, \
                            sizeof(mln_lang_var_t *), \
                            M_LANG_ARRAY_PREALLOC, \
                            ctx->pool, \
                            (array_pool_alloc_handler)mln_alloc_m, \
                            (array_pool_free_handler)mln_alloc_free) < 0)
    {
        mln_array_destroy(&(lfd->args));
        mln_alloc_free(lfd);
        return NULL;
    }
    lfd->type = type;
    switch (type) {
        case M_FUNC_INTERNAL:
            lfd->data.process = (mln_lang_internal)data;
            break;
        default:
            lfd->data.stm = (mln_lang_stm_t *)data;
            break;
    }
    if (exp != NULL) {
        if (mln_lang_funcdef_args_get(ctx, exp, &(lfd->args), 0) < 0) {
            __mln_lang_func_detail_free(lfd);
            return NULL;
        }
    }
    if (closure != NULL) {
        if (mln_lang_funcdef_args_get(ctx, closure, &(lfd->closure), 1) < 0) {
            __mln_lang_func_detail_free(lfd);
            return NULL;
        }
    }
    return lfd;
})

MLN_FUNC_VOID(, void, mln_lang_func_detail_free, (mln_lang_func_detail_t *lfd), (lfd), {
    __mln_lang_func_detail_free(lfd);
})

MLN_FUNC_VOID(static inline, void, __mln_lang_func_detail_free, \
              (mln_lang_func_detail_t *lfd), (lfd), \
{
    if (lfd == NULL) return;
    mln_array_destroy(&lfd->args);
    mln_array_destroy(&lfd->closure);
    mln_alloc_free(lfd);
})

MLN_FUNC(static inline, mln_lang_func_detail_t *, mln_lang_func_detail_dup, \
         (mln_lang_ctx_t *ctx, mln_lang_func_detail_t *func), (ctx, func), \
{
    mln_lang_func_detail_t *ret;
    mln_lang_var_t **var, *v;
    mln_u8ptr_t data;
    mln_size_t i;
    switch (func->type) {
        case M_FUNC_INTERNAL:
            data = (mln_u8ptr_t)(func->data.process);
            break;
        default:
            data = (mln_u8ptr_t)(func->data.stm);
            break;
    }
    ret = __mln_lang_func_detail_new(ctx, func->type, data, NULL, NULL);
    if (ret == NULL) return NULL;

    for (i = 0; i < mln_array_nelts(&(func->args)); ++i) {
        if ((var = (mln_lang_var_t **)mln_array_push(&(ret->args))) == NULL) {
            __mln_lang_func_detail_free(ret);
            return NULL;
        }
        v = ((mln_lang_var_t **)mln_array_elts(&(func->args)))[i];
        if ((*var = __mln_lang_var_dup(ctx, v)) == NULL) {
            __mln_lang_func_detail_free(ret);
            return NULL;
        }
    }
    for (i = 0; i < mln_array_nelts(&(func->closure)); ++i) {
        if ((var = (mln_lang_var_t **)mln_array_push(&(ret->closure))) == NULL) {
            __mln_lang_func_detail_free(ret);
            return NULL;
        }
        v = ((mln_lang_var_t **)mln_array_elts(&(func->closure)))[i];
        if ((*var = __mln_lang_var_dup(ctx, v)) == NULL) {
            __mln_lang_func_detail_free(ret);
            return NULL;
        }
    }
    return ret;
})

MLN_FUNC(, int, mln_lang_func_detail_arg_append, \
         (mln_lang_func_detail_t *func, mln_lang_var_t *var), (func, var), \
{
    mln_lang_var_t **v;

    if ((v = (mln_lang_var_t **)mln_array_push(&(func->args))) == NULL) {
        return -1;
    }
    *v = var;
    return 0;
})


MLN_FUNC(static inline, mln_lang_object_t *, mln_lang_object_new, \
         (mln_lang_ctx_t *ctx, mln_lang_set_detail_t *in_set), (ctx, in_set), \
{
    struct mln_rbtree_attr rbattr;
    mln_lang_object_t *obj;
    if ((obj = (mln_lang_object_t *)mln_alloc_m(ctx->pool, sizeof(mln_lang_object_t))) == NULL) {
        return NULL;
    }
    obj->in_set = in_set;
    if (in_set != NULL) ++(in_set->ref);
    rbattr.pool = ctx->pool;
    rbattr.pool_alloc = (rbtree_pool_alloc_handler)mln_alloc_m;
    rbattr.pool_free = (rbtree_pool_free_handler)mln_alloc_free;
    rbattr.cmp = mln_lang_var_name_cmp;
    rbattr.data_free = mln_lang_var_free;
    if ((obj->members = mln_rbtree_new(&rbattr)) == NULL) {
        mln_alloc_free(obj);
        return NULL;
    }
    obj->ref = 0;

    if (in_set != NULL) {
        struct mln_lang_scan_s ls;
        ls.cnt = NULL;
        ls.tree = obj->members;
        ls.tree2 = NULL;
        ls.ctx = ctx;
        if (mln_rbtree_iterate(in_set->members, mln_lang_set_member_iterate_handler, &ls) < 0) {
            mln_lang_object_free(obj);
            return NULL;
        }
    }
    obj->gc_item = NULL;
    obj->ctx = ctx;

    if (mln_lang_gc_item_new(ctx->pool, ctx->gc, M_GC_OBJ, obj) < 0) {
        mln_lang_object_free(obj);
        obj = NULL;
    }

    return obj;
})

MLN_FUNC_VOID(static inline, void, mln_lang_object_free, (mln_lang_object_t *obj), (obj), {
    if (obj == NULL) return;
    if (obj->ref > 1) {
        ASSERT(obj->gc_item);
        if (obj->gc_item->gc != NULL) {
            mln_gc_suspect(obj->gc_item->gc, obj->gc_item);
        }
        --(obj->ref);
        return;
    }
    if (obj->members != NULL) mln_rbtree_free(obj->members);
    if (obj->in_set != NULL) mln_lang_set_detail_free(obj->in_set);
    if (obj->gc_item != NULL) {
        if (obj->gc_item->gc != NULL)
            mln_gc_remove(obj->gc_item->gc, obj->gc_item, obj->ctx->gc);
        mln_lang_gc_item_free_immediatly(obj->gc_item);
    }
    mln_alloc_free(obj);
})

MLN_FUNC(, mln_lang_val_t *, mln_lang_val_new, \
         (mln_lang_ctx_t *ctx, mln_s32_t type, void *data), (ctx, type, data), \
{
    return __mln_lang_val_new(ctx, type, data);
})

MLN_FUNC(static inline, mln_lang_val_t *, __mln_lang_val_new, \
         (mln_lang_ctx_t *ctx, mln_s32_t type, void *data), (ctx, type, data), \
{
    mln_lang_val_t *val;
    if ((val = (mln_lang_val_t *)mln_alloc_m(ctx->pool, sizeof(mln_lang_val_t))) == NULL) {
        return NULL;
    }
    val->prev = val->next = NULL;
    switch (type) {
        case M_LANG_VAL_TYPE_NIL:
            break;
        case M_LANG_VAL_TYPE_INT:
            val->data.i = *(mln_s64_t *)data;
            break;
        case M_LANG_VAL_TYPE_BOOL:
            val->data.b = *(mln_u8ptr_t)data;
            break;
        case M_LANG_VAL_TYPE_REAL:
            val->data.f = *(double *)data;
            break;
        case M_LANG_VAL_TYPE_STRING:
            val->data.s = (mln_string_t *)data;
            break;
        case M_LANG_VAL_TYPE_OBJECT:
            val->data.obj = (mln_lang_object_t *)data;
            ++(val->data.obj->ref);
            break;
        case M_LANG_VAL_TYPE_FUNC:
            val->data.func = (mln_lang_func_detail_t *)data;
            break;
        case M_LANG_VAL_TYPE_ARRAY:
            val->data.array = (mln_lang_array_t *)data;
            ++(val->data.array->ref);
            break;
        case M_LANG_VAL_TYPE_CALL:
            val->data.call = (mln_lang_funccall_val_t *)data;
            break;
        default:
            ASSERT(0);
            mln_alloc_free(val);
            return NULL;
    }
    val->type = type;
    val->ref = 0;
    val->udata = NULL;
    val->func = NULL;
    val->not_modify = 0;
    return val;
})

MLN_FUNC_VOID(, void, mln_lang_val_free, (mln_lang_val_t *val), (val), {
    __mln_lang_val_free(val);
})

MLN_FUNC_VOID(static inline, void, __mln_lang_val_free, (void *data), (data), {
    if (data == NULL) return;
    mln_lang_val_t *val = (mln_lang_val_t *)data;
    if (val->ref-- > 1) return;
    if (val->udata != NULL && val->udata != val) {
        mln_lang_val_free(val->udata);
        val->udata = NULL;
    }
    if (val->func != NULL) {
        __mln_lang_func_detail_free(val->func);
        val->func = NULL;
    }
    mln_lang_val_data_free(val);
    mln_alloc_free(val);
})

MLN_FUNC_VOID(static inline, void, mln_lang_val_data_free, (mln_lang_val_t *val), (val), {
    switch (val->type) {
        case M_LANG_VAL_TYPE_OBJECT:
            if (val->data.obj != NULL) {
                mln_lang_object_free(val->data.obj);
                val->data.obj = NULL;
            }
            break;
        case M_LANG_VAL_TYPE_FUNC:
            if (val->data.func != NULL) {
                __mln_lang_func_detail_free(val->data.func);
                val->data.func = NULL;
            }
            break;
        case M_LANG_VAL_TYPE_ARRAY:
            if (val->data.array != NULL) {
                __mln_lang_array_free(val->data.array);
                val->data.array = NULL;
            }
            break;
        case M_LANG_VAL_TYPE_STRING:
            if (val->data.s != NULL) {
                mln_string_free(val->data.s);
                val->data.s = NULL;
            }
            break;
        case M_LANG_VAL_TYPE_CALL:
            if (val->data.call != NULL) {
                __mln_lang_funccall_val_free(val->data.call);
                val->data.call = NULL;
            }
            break;
        default:
            val->data.i = 0;
            break;
    }
    val->type = M_LANG_VAL_TYPE_NIL;
})

MLN_FUNC(static inline, mln_lang_val_t *, mln_lang_val_dup, \
         (mln_lang_ctx_t *ctx, mln_lang_val_t *val), (ctx, val), \
{
    mln_u8ptr_t data;
    mln_u32_t type = val->type;
    switch (type) {
        case M_LANG_VAL_TYPE_NIL:
            data = NULL;
            break;
        case M_LANG_VAL_TYPE_INT:
            data = (mln_u8ptr_t)&(val->data.i);
            break;
        case M_LANG_VAL_TYPE_BOOL:
            data = (mln_u8ptr_t)&(val->data.b);
            break;
        case M_LANG_VAL_TYPE_REAL:
            data = (mln_u8ptr_t)&(val->data.f);
            break;
        case M_LANG_VAL_TYPE_STRING:
            data = (mln_u8ptr_t)mln_string_ref(val->data.s);
            break;
        case M_LANG_VAL_TYPE_OBJECT:
            data = (mln_u8ptr_t)(val->data.obj);
            break;
        case M_LANG_VAL_TYPE_FUNC:
            if ((data = (mln_u8ptr_t)mln_lang_func_detail_dup(ctx, val->data.func)) == NULL) {
                return NULL;
            }
            break;
        case M_LANG_VAL_TYPE_ARRAY:
            data = (mln_u8ptr_t)(val->data.array);
            break;
        default:
            return NULL;
    }
    mln_lang_val_t *ret = __mln_lang_val_new(ctx, type, data);
    if (ret == NULL) {
        switch (type) {
            case M_LANG_VAL_TYPE_STRING:
                mln_string_free((mln_string_t *)data);
                break;
            case M_LANG_VAL_TYPE_FUNC:
                __mln_lang_func_detail_free((mln_lang_func_detail_t *)data);
                break;
            default:
                break;
        }
    }
    return ret;
})

MLN_FUNC(static, int, mln_lang_val_cmp, \
         (const void *data1, const void *data2), (data1, data2), \
{
    int ret;
    mln_lang_val_t *val1 = (mln_lang_val_t *)data1;
    mln_lang_val_t *val2 = (mln_lang_val_t *)data2;
    if (val1->type != val2->type) {
        return val1->type - val2->type;
    }
    switch (val1->type) {
        case M_LANG_VAL_TYPE_NIL:
            ret = 0;
            break;
        case M_LANG_VAL_TYPE_INT:
            if (val1->data.i > val2->data.i) ret = 1;
            else if (val1->data.i < val2->data.i) ret = -1;
            else ret = 0;
            break;
        case M_LANG_VAL_TYPE_BOOL:
            ASSERT(val1->data.b <= 1 && val2->data.b <= 1);
            ret = (int)(val1->data.b) - (int)(val2->data.b);
            break;
        case M_LANG_VAL_TYPE_REAL:
            if (val1->data.f > val2->data.f) ret = 1;
            else if (val1->data.f < val2->data.f) ret = -1;
            else ret = 0;
            break;
        case M_LANG_VAL_TYPE_STRING:
            ret = mln_string_strcmp(val1->data.s, val2->data.s);
            break;
        case M_LANG_VAL_TYPE_OBJECT:
            if (val1->data.obj > val2->data.obj) ret = 1;
            else if (val1->data.obj < val2->data.obj) ret = -1;
            else ret = 0;
            break;
        case M_LANG_VAL_TYPE_FUNC:
            if (val1->data.func->type > val2->data.func->type) {
                ret = 1;
            } else if (val1->data.func->type < val2->data.func->type) {
                ret = -1;
            } else {
                mln_lang_func_type_t type = val1->data.func->type;
                mln_lang_func_detail_t *f1 = val1->data.func;
                mln_lang_func_detail_t *f2 = val2->data.func;
                switch (type) {
                    case M_FUNC_INTERNAL:
                        if ((mln_uptr_t)(f1->data.process) > (mln_uptr_t)(f2->data.process)) ret = 1;
                        else if ((mln_uptr_t)(f1->data.process) < (mln_uptr_t)(f2->data.process)) ret = -1;
                        else ret = 0;
                        break;
                    default:
                        if (f1->data.stm > f2->data.stm) ret = 1;
                        else if (f1->data.stm < f2->data.stm) ret = -1;
                        else ret = 0;
                        break;
                }
            }
            break;
        case M_LANG_VAL_TYPE_ARRAY:
            if (val1->data.array > val2->data.array) ret = 1;
            else if (val1->data.array < val2->data.array) ret = -1;
            else ret = 0;
            break;
        default:
            mln_log(error, "No such type %x\n", val1->type);
            abort();
    }
    return ret;
})


MLN_FUNC(, mln_lang_array_t *, mln_lang_array_new, (mln_lang_ctx_t *ctx), (ctx), {
    return __mln_lang_array_new(ctx);
})

MLN_FUNC(static inline, mln_lang_array_t *, __mln_lang_array_new, (mln_lang_ctx_t *ctx), (ctx), {
    mln_lang_array_t *la;
    struct mln_rbtree_attr rbattr;
    if ((la = (mln_lang_array_t *)mln_alloc_m(ctx->pool, sizeof(mln_lang_array_t))) == NULL) {
        return NULL;
    }
    rbattr.pool = ctx->pool;
    rbattr.pool_alloc = (rbtree_pool_alloc_handler)mln_alloc_m;
    rbattr.pool_free = (rbtree_pool_free_handler)mln_alloc_free;
    rbattr.cmp = mln_lang_array_elem_index_cmp;
    rbattr.data_free = mln_lang_array_elem_free;
    if ((la->elems_index = mln_rbtree_new(&rbattr)) == NULL) {
        mln_alloc_free(la);
        return NULL;
    }
    rbattr.cmp = mln_lang_array_elem_key_cmp;
    rbattr.data_free = NULL;
    if ((la->elems_key = mln_rbtree_new(&rbattr)) == NULL) {
        mln_rbtree_free(la->elems_index);
        mln_alloc_free(la);
        return NULL;
    }
    la->index = 0;
    la->ref = 0;
    la->gc_item = NULL;
    la->ctx = ctx;

    if (mln_lang_gc_item_new(ctx->pool, ctx->gc, M_GC_ARRAY, la) < 0) {
        __mln_lang_array_free(la);
        la = NULL;
    }
    return la;
})

MLN_FUNC_VOID(, void, mln_lang_array_free, (mln_lang_array_t *array), (array), {
    __mln_lang_array_free(array);
})

MLN_FUNC_VOID(static inline, void, __mln_lang_array_free, (mln_lang_array_t *array), (array), {
    if (array == NULL) return;
    if (array->ref > 1) {
        ASSERT(array->gc_item);
        if (array->gc_item->gc != NULL)
            mln_gc_suspect(array->gc_item->gc, array->gc_item);
        --(array->ref);
        return;
    }
    if (array->elems_key != NULL) mln_rbtree_free(array->elems_key);
    if (array->elems_index != NULL) mln_rbtree_free(array->elems_index);

    if (array->gc_item != NULL) {
        if (array->gc_item->gc != NULL)
            mln_gc_remove(array->gc_item->gc, array->gc_item, array->ctx->gc);
        mln_lang_gc_item_free_immediatly(array->gc_item);
    }
    mln_alloc_free(array);
})

MLN_FUNC(static, int, mln_lang_array_elem_index_cmp, \
         (const void *data1, const void *data2), (data1, data2), \
{
    mln_lang_array_elem_t *elem1 = (mln_lang_array_elem_t *)data1;
    mln_lang_array_elem_t *elem2 = (mln_lang_array_elem_t *)data2;
    if (elem1->index > elem2->index) return 1;
    if (elem1->index < elem2->index) return -1;
    return 0;
})

MLN_FUNC(static, int, mln_lang_array_elem_key_cmp, \
         (const void *data1, const void *data2), (data1, data2), \
{
    mln_lang_array_elem_t *elem1 = (mln_lang_array_elem_t *)data1;
    mln_lang_array_elem_t *elem2 = (mln_lang_array_elem_t *)data2;
    return mln_lang_var_cmp(elem1->key, elem2->key);
})

MLN_FUNC(static inline, mln_lang_array_elem_t *, mln_lang_array_elem_new, \
         (mln_alloc_t *pool, mln_lang_var_t *key, mln_lang_var_t *val, mln_u64_t index), \
         (pool, key, val, index), \
{
    mln_lang_array_elem_t *elem;
    if ((elem = (mln_lang_array_elem_t *)mln_alloc_m(pool, sizeof(mln_lang_array_elem_t))) == NULL) {
        return NULL;
    }
    elem->index = index;
    elem->key = key;
    elem->value = val;
    return elem;
})

MLN_FUNC_VOID(static inline, void, mln_lang_array_elem_free, (void *data), (data), {
    if (data == NULL) return;
    mln_lang_array_elem_t *elem = (mln_lang_array_elem_t *)data;
    if (elem->key != NULL) __mln_lang_var_free(elem->key);
    if (elem->value != NULL) __mln_lang_var_free(elem->value);
    mln_alloc_free(elem);
})

MLN_FUNC(, mln_lang_var_t *, mln_lang_array_get, \
         (mln_lang_ctx_t *ctx, mln_lang_array_t *array, mln_lang_var_t *key), \
         (ctx, array, key), \
{
    return __mln_lang_array_get(ctx, array, key);
})

MLN_FUNC(static inline, mln_lang_var_t *, __mln_lang_array_get, \
         (mln_lang_ctx_t *ctx, mln_lang_array_t *array, mln_lang_var_t *key), \
         (ctx, array, key), \
{
    mln_s32_t type;
    mln_lang_var_t *ret;

    if (key != NULL && (type = mln_lang_var_val_type_get(key)) != M_LANG_VAL_TYPE_NIL) {
        if (type == M_LANG_VAL_TYPE_INT) {
            if ((ret = mln_lang_array_get_int(ctx, array, key)) == NULL) {
                return NULL;
            }
        } else {
            if ((ret = mln_lang_array_get_other(ctx, array, key)) == NULL) {
                return NULL;
            }
        }
    } else {
        if ((ret = mln_lang_array_get_nil(ctx, array)) == NULL) {
            return NULL;
        }
    }
    return ret;
})

MLN_FUNC(static inline, mln_lang_var_t *, mln_lang_array_get_int, \
         (mln_lang_ctx_t *ctx, mln_lang_array_t *array, mln_lang_var_t *key), \
         (ctx, array, key), \
{
    mln_rbtree_t *tree;
    mln_rbtree_node_t *rn;
    mln_lang_var_t *ret, *nil;
    mln_lang_array_elem_t *elem;
    if ((nil = __mln_lang_var_create_nil(ctx, NULL)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        return NULL;
    }
    if ((elem = mln_lang_array_elem_new(ctx->pool, NULL, nil, key->val->data.i)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        __mln_lang_var_free(nil);
        return NULL;
    }
    tree = array->elems_index;
    rn = mln_rbtree_search(tree, elem);
    if (!mln_rbtree_null(rn, tree)) {
        mln_lang_array_elem_free(elem);
        ret = ((mln_lang_array_elem_t *)mln_rbtree_node_data_get(rn))->value;
    } else {
        if ((rn = mln_rbtree_node_new(tree, elem)) == NULL) {
            __mln_lang_errmsg(ctx, "No memory.");
            mln_lang_array_elem_free(elem);
            return NULL;
        }
        mln_rbtree_insert(tree, rn);
        if (array->index <= key->val->data.i)
            array->index = key->val->data.i + 1;
        ret = elem->value;
    }
    return ret;
})

MLN_FUNC(static inline, mln_lang_var_t *, mln_lang_array_get_other, \
         (mln_lang_ctx_t *ctx, mln_lang_array_t *array, mln_lang_var_t *key), \
         (ctx, array, key), \
{
    mln_rbtree_t *tree;
    mln_rbtree_node_t *rn;
    mln_lang_var_t *ret, *nil, *k;
    mln_lang_array_elem_t *elem;
    if ((nil = __mln_lang_var_create_nil(ctx, NULL)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        return NULL;
    }
    if ((k = mln_lang_var_dup_with_val(ctx, key)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        __mln_lang_var_free(nil);
        return NULL;
    }
    if ((elem = mln_lang_array_elem_new(ctx->pool, k, nil, array->index)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        __mln_lang_var_free(k);
        __mln_lang_var_free(nil);
        return NULL;
    }
    tree = array->elems_key;
    rn = mln_rbtree_search(tree, elem);
    if (!mln_rbtree_null(rn, tree)) {
        mln_lang_array_elem_free(elem);
        ret = ((mln_lang_array_elem_t *)mln_rbtree_node_data_get(rn))->value;
    } else {
        if ((rn = mln_rbtree_node_new(tree, elem)) == NULL) {
            __mln_lang_errmsg(ctx, "No memory.");
            mln_lang_array_elem_free(elem);
            return NULL;
        }
        mln_rbtree_insert(tree, rn);
        tree = array->elems_index;
        if ((rn = mln_rbtree_node_new(tree, elem)) == NULL) {
            __mln_lang_errmsg(ctx, "No memory.");
            mln_lang_array_elem_free(elem);
            return NULL;
        }
        mln_rbtree_insert(tree, rn);
        ++(array->index);
        ret = elem->value;
    }
    return ret;
})

MLN_FUNC(static inline, mln_lang_var_t *, mln_lang_array_get_nil, \
         (mln_lang_ctx_t *ctx, mln_lang_array_t *array), (ctx, array), \
{
    mln_rbtree_node_t *rn;
    mln_lang_var_t *nil;
    mln_lang_array_elem_t *elem;
    if ((nil = __mln_lang_var_create_nil(ctx, NULL)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        return NULL;
    }
    if ((elem = mln_lang_array_elem_new(ctx->pool, NULL, nil, array->index)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        __mln_lang_var_free(nil);
        return NULL;
    }
    if ((rn = mln_rbtree_node_new(array->elems_index, elem)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        mln_lang_array_elem_free(elem);
        return NULL;
    }
    mln_rbtree_insert(array->elems_index, rn);
    ++(array->index);
    return elem->value;
})

MLN_FUNC(, int, mln_lang_array_elem_exist, \
         (mln_lang_array_t *array, mln_lang_var_t *key), (array, key), \
{
    mln_rbtree_t *tree;
    mln_rbtree_node_t *rn;
    mln_lang_var_t *nil, *k;
    mln_lang_array_elem_t *elem;
    mln_lang_ctx_t *ctx = array->ctx;
    if ((nil = __mln_lang_var_create_nil(ctx, NULL)) == NULL) {
        return 0;
    }
    if (mln_lang_var_val_type_get(key) == M_LANG_VAL_TYPE_INT) {
        if ((elem = mln_lang_array_elem_new(ctx->pool, NULL, nil, key->val->data.i)) == NULL) {
            __mln_lang_var_free(nil);
            return 0;
        }
        tree = array->elems_index;
        rn = mln_rbtree_search(tree, elem);
        mln_lang_array_elem_free(elem);
        if (mln_rbtree_null(rn, tree)) {
            return 0;
        }
        return 1;
    }
    if ((k = mln_lang_var_dup_with_val(ctx, key)) == NULL) {
        __mln_lang_var_free(nil);
        return 0;
    }
    if ((elem = mln_lang_array_elem_new(ctx->pool, k, nil, array->index)) == NULL) {
        __mln_lang_var_free(k);
        __mln_lang_var_free(nil);
        return 0;
    }
    tree = array->elems_key;
    rn = mln_rbtree_search(tree, elem);
    mln_lang_array_elem_free(elem);
    if (mln_rbtree_null(rn, tree)) {
        return 0;
    }
    return 1;
})


MLN_FUNC(static inline, mln_lang_funccall_val_t *, __mln_lang_funccall_val_new, \
         (mln_alloc_t *pool, mln_string_t *name), (pool, name), \
{
    mln_lang_funccall_val_t *func;

    if ((func = (mln_lang_funccall_val_t *)mln_alloc_m(pool, sizeof(mln_lang_funccall_val_t))) == NULL) {
        return NULL;
    }
    if (name != NULL) {
        func->name = mln_string_ref(name);
    } else {
        func->name = NULL;
    }
    func->prototype = NULL;
    func->object = NULL;

    if (mln_array_pool_init(&func->args, \
                            (array_free)mln_lang_var_pfree, \
                            sizeof(mln_lang_var_t *), \
                            M_LANG_ARRAY_PREALLOC, \
                            pool, \
                            (array_pool_alloc_handler)mln_alloc_m, \
                            (array_pool_free_handler)mln_alloc_free) < 0)
    {
        mln_alloc_free(func);
        return NULL;
    }
    return func;
})

MLN_FUNC(, mln_lang_funccall_val_t *, mln_lang_funccall_val_new, \
         (mln_alloc_t *pool, mln_string_t *name), (pool, name), \
{
    return __mln_lang_funccall_val_new(pool, name);
})

MLN_FUNC_VOID(, void, mln_lang_funccall_val_free, (mln_lang_funccall_val_t *func), (func), {
    __mln_lang_funccall_val_free(func);
})

MLN_FUNC_VOID(static inline, void, __mln_lang_funccall_val_free, \
              (mln_lang_funccall_val_t *func), (func), \
{
    if (func == NULL) return;
    if (func->name != NULL) {
        mln_string_free(func->name);
    }
    mln_array_destroy(&func->args);
    if (func->object != NULL) __mln_lang_val_free(func->object);
    mln_alloc_free(func);
})

MLN_FUNC(inline, int, mln_lang_funccall_val_add_arg, \
        (mln_lang_funccall_val_t *func, mln_lang_var_t *var), (func, var), \
{
    mln_lang_var_t **v;

    if ((v = (mln_lang_var_t **)mln_array_push(&func->args)) == NULL)
        return -1;
    *v = var;
    return 0;
})

MLN_FUNC_VOID(inline, void, mln_lang_funccall_val_object_add, \
              (mln_lang_funccall_val_t *func, mln_lang_val_t *obj_val), (func, obj_val), \
{
    ASSERT(obj_val != NULL && func->object == NULL);
    ++(obj_val->ref);
    func->object = obj_val;
})

MLN_FUNC(, int, mln_lang_funccall_val_operator, \
         (mln_lang_ctx_t *ctx, mln_string_t *name, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, name, ret, op1, op2), \
{
    mln_lang_symbol_node_t *sym;

    if (mln_lang_funccall_val_operator_overload_test(ctx, name)) {
        return 0;
    }

    if ((sym = __mln_lang_symbol_node_search(ctx, name, 0)) != NULL \
        && sym->type == M_LANG_SYMBOL_VAR \
        && mln_lang_var_val_type_get(sym->data.var) == M_LANG_VAL_TYPE_FUNC)
    {
        mln_lang_funccall_val_t *call = __mln_lang_funccall_val_new(ctx->pool, name);
        if (call == NULL) {
            __mln_lang_errmsg(ctx, "No memory.");
            return -1;
        }
        *ret = mln_lang_var_create_call(ctx, call);
        if (*ret == NULL) {
            __mln_lang_errmsg(ctx, "No memory.");
            __mln_lang_funccall_val_free(call);
            return -1;
        }
        mln_lang_funccall_val_add_arg(call, mln_lang_var_ref(op1));
        if (op2 != NULL) mln_lang_funccall_val_add_arg(call, mln_lang_var_ref(op2));
        call->prototype = mln_lang_var_val_get(sym->data.var)->data.func;
        return 1;
    }
    return 0;
})

MLN_FUNC(, int, mln_lang_funccall_val_obj_operator, \
         (mln_lang_ctx_t *ctx, mln_lang_var_t *obj, mln_string_t *name, \
          mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2), \
         (ctx, obj, name, ret, op1, op2), \
{
    mln_lang_var_t *var;
    mln_lang_symbol_node_t *sym;

    if (mln_lang_funccall_val_operator_overload_test(ctx, name)) {
        return 0;
    }

    if ((var = __mln_lang_set_member_search(mln_lang_var_val_get(obj)->data.obj->members, name)) != NULL \
        && mln_lang_var_val_type_get(var) == M_LANG_VAL_TYPE_FUNC)
    {
        mln_lang_funccall_val_t *call = __mln_lang_funccall_val_new(ctx->pool, name);
        if (call == NULL) {
            __mln_lang_errmsg(ctx, "No memory.");
            return -1;
        }
        *ret = mln_lang_var_create_call(ctx, call);
        if (*ret == NULL) {
            __mln_lang_errmsg(ctx, "No memory.");
            __mln_lang_funccall_val_free(call);
            return -1;
        }
        mln_lang_funccall_val_add_arg(call, mln_lang_var_ref(op1));
        if (op2 != NULL) mln_lang_funccall_val_add_arg(call, mln_lang_var_ref(op2));
        call->prototype = mln_lang_var_val_get(var)->data.func;
        mln_lang_funccall_val_object_add(call, obj->val);
        return 1;
    }

    if ((sym = __mln_lang_symbol_node_search(ctx, name, 0)) != NULL \
        && sym->type == M_LANG_SYMBOL_VAR \
        && mln_lang_var_val_type_get(sym->data.var) == M_LANG_VAL_TYPE_FUNC)
    {
        mln_lang_funccall_val_t *call = __mln_lang_funccall_val_new(ctx->pool, name);
        if (call == NULL) {
            __mln_lang_errmsg(ctx, "No memory.");
            return -1;
        }
        *ret = mln_lang_var_create_call(ctx, call);
        if (*ret == NULL) {
            __mln_lang_errmsg(ctx, "No memory.");
            __mln_lang_funccall_val_free(call);
            return -1;
        }
        mln_lang_funccall_val_add_arg(call, mln_lang_var_ref(op1));
        if (op2 != NULL) mln_lang_funccall_val_add_arg(call, mln_lang_var_ref(op2));
        call->prototype = mln_lang_var_val_get(sym->data.var)->data.func;
        return 1;
    }
    return 0;
})

MLN_FUNC(static inline, int, mln_lang_funccall_val_operator_overload_test, \
         (mln_lang_ctx_t *ctx, mln_string_t *name), (ctx, name), \
{
    mln_lang_scope_t *scope = mln_lang_scope_top(ctx);

    for (; mln_lang_scope_in(ctx, scope); --scope) {
        if (scope->name != NULL && !mln_string_strcmp(scope->name, name))
            return 1;
    }

    return 0;
})


MLN_FUNC_VOID(, void, mln_lang_errmsg, (mln_lang_ctx_t *ctx, char *msg), (ctx, msg), {
    __mln_lang_errmsg(ctx, msg);
})

MLN_FUNC_VOID(static, void, __mln_lang_errmsg, (mln_lang_ctx_t *ctx, char *msg), (ctx, msg), {
    mln_u64_t line = 0;
    mln_string_t *filename = NULL, nilname = mln_string(" ");
    mln_lang_stack_node_t *node;
    if ((node = mln_lang_stack_top(ctx)) != NULL) {
        switch (node->type) {
            case M_LSNT_STM:
                filename = node->data.stm->file;
                line = node->data.stm->line;
                break;
            case M_LSNT_FUNCDEF:
                filename = node->data.funcdef->file;
                line = node->data.funcdef->line;
                break;
            case M_LSNT_SET:
                filename = node->data.set->file;
                line = node->data.set->line;
                break;
            case M_LSNT_SETSTM:
                filename = node->data.set_stm->file;
                line = node->data.set_stm->line;
                break;
            case M_LSNT_BLOCK:
                filename = node->data.block->file;
                line = node->data.block->line;
                break;
            case M_LSNT_WHILE:
                filename = node->data.w->file;
                line = node->data.w->line;
                break;
            case M_LSNT_SWITCH:
                filename = node->data.sw->file;
                line = node->data.sw->line;
                break;
            case M_LSNT_SWITCHSTM:
                filename = node->data.sw_stm->file;
                line = node->data.sw_stm->line;
                break;
            case M_LSNT_FOR:
                filename = node->data.f->file;
                line = node->data.f->line;
                break;
            case M_LSNT_IF:
                filename = node->data.i->file;
                line = node->data.i->line;
                break;
            case M_LSNT_EXP:
                filename = node->data.exp->file;
                line = node->data.exp->line;
                break;
            case M_LSNT_ASSIGN:
                filename = node->data.assign->file;
                line = node->data.assign->line;
                break;
            case M_LSNT_LOGICLOW:
                filename = node->data.logiclow->file;
                line = node->data.logiclow->line;
                break;
            case M_LSNT_LOGICHIGH:
                filename = node->data.logichigh->file;
                line = node->data.logichigh->line;
                break;
            case M_LSNT_RELATIVELOW:
                filename = node->data.relativelow->file;
                line = node->data.relativelow->line;
                break;
            case M_LSNT_RELATIVEHIGH:
                filename = node->data.relativehigh->file;
                line = node->data.relativehigh->line;
                break;
            case M_LSNT_MOVE:
                filename = node->data.move->file;
                line = node->data.move->line;
                break;
            case M_LSNT_ADDSUB:
                filename = node->data.addsub->file;
                line = node->data.addsub->line;
                break;
            case M_LSNT_MULDIV:
                filename = node->data.muldiv->file;
                line = node->data.muldiv->line;
                break;
            case M_LSNT_NOT:
                filename = node->data.not->file;
                line = node->data.not->line;
                break;
            case M_LSNT_SUFFIX:
                filename = node->data.suffix->file;
                line = node->data.suffix->line;
                break;
            case M_LSNT_LOCATE:
                filename = node->data.locate->file;
                line = node->data.locate->line;
                break;
            case M_LSNT_SPEC:
                filename = node->data.spec->file;
                line = node->data.spec->line;
                break;
            case M_LSNT_FACTOR:
                filename = node->data.factor->file;
                line = node->data.factor->line;
                break;
            default: /* M_LSNT_ELEMLIST */
                filename = node->data.elemlist->file;
                line = node->data.elemlist->line;
                break;
        }
    }
    if (filename == NULL) {
        filename = (ctx->filename == NULL)? &nilname: ctx->filename;
    }
    mln_log(none, "%S:%I: %s\n", filename, line, msg);
})

/*
 * stack handlers
 */
MLN_FUNC_VOID(static, void, mln_lang_stack_handler_stm, (mln_lang_ctx_t *ctx), (ctx), {
    mln_lang_stack_node_t *node = mln_lang_stack_top(ctx);
    mln_lang_stm_t *stm = node->data.stm;

    if (node->step == 0) {
        ++(node->step);
again:
        if (stm->type == M_STM_LABEL) goto goon1;
        if (stm->jump == NULL)
            mln_lang_generate_jump_ptr(stm, M_LSNT_STM);
        if ((node = mln_lang_stack_push(ctx, (mln_lang_stack_node_type_t)(stm->jump_type), stm->jump)) == NULL) {
            __mln_lang_errmsg(ctx, "Stack is full.");
            ctx->quit = 1;
            return;
        }
        mln_lang_stack_map[node->type](ctx);
        return;
    } else {
goon1:
        if (!ctx->ret_flag && stm->next != NULL) {
            stm = stm->next;
            node->data.stm = stm;
            goto again;
        }
        if (ctx->ret_flag) ctx->ret_flag = 0;
        else mln_lang_ctx_reset_ret_var(ctx);
        mln_lang_stack_node_free(mln_lang_stack_pop(ctx));
    }
})

MLN_FUNC_VOID(static, void, mln_lang_stack_handler_funcdef, (mln_lang_ctx_t *ctx), (ctx), {
    mln_lang_stack_node_t *node = mln_lang_stack_top(ctx);
    mln_lang_funcdef_t *funcdef = node->data.funcdef;
    mln_lang_val_t *val;
    mln_lang_var_t *var;
    mln_lang_func_detail_t *func;
    mln_lang_set_detail_t *in_set = mln_lang_ctx_get_class(ctx);
    if ((func = __mln_lang_func_detail_new(ctx, \
                                         M_FUNC_EXTERNAL, \
                                         funcdef->stm, \
                                         funcdef->args, \
                                         funcdef->closure)) == NULL)
    {
        __mln_lang_errmsg(ctx, "Parse function definition failed.");
        ctx->quit = 1;
        return;
    }
    if ((val = __mln_lang_val_new(ctx, M_LANG_VAL_TYPE_FUNC, func)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        __mln_lang_func_detail_free(func);
        ctx->quit = 1;
        return;
    }
    if (funcdef->name->len > 18 && funcdef->name->data[0] == '_' && funcdef->name->data[1] == '_') {
        switch (funcdef->name->data[2]) {
            case 'a':
                ctx->op_array_flag = 1;
                break;
            case 'b':
                ctx->op_bool_flag = 1;
                break;
            case 'f':
                ctx->op_func_flag = 1;
                break;
            case 'i':
                ctx->op_int_flag = 1;
                break;
            case 'n':
                ctx->op_nil_flag = 1;
                break;
            case 'o':
                ctx->op_obj_flag = 1;
                break;
            case 'r':
                ctx->op_real_flag = 1;
                break;
            case 's':
                ctx->op_str_flag = 1;
                break;
        }
    }
    if ((var = __mln_lang_var_new(ctx, funcdef->name, M_LANG_VAR_NORMAL, val, in_set)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        __mln_lang_val_free(val);
        ctx->quit = 1;
        return;
    }
    if (in_set != NULL) {
        if (mln_lang_set_member_add(ctx->pool, in_set->members, var) < 0) {
            __mln_lang_errmsg(ctx, "Add member failed. No memory.");
            __mln_lang_var_free(var);
            ctx->quit = 1;
            return;
        }
    } else {
        if (__mln_lang_symbol_node_join(ctx, M_LANG_SYMBOL_VAR, var) < 0) {
            __mln_lang_var_free(var);
            ctx->quit = 1;
            return;
        }
    }
    mln_lang_stack_node_free(mln_lang_stack_pop(ctx));
})

MLN_FUNC_VOID(static, void, mln_lang_stack_handler_set, (mln_lang_ctx_t *ctx), (ctx), {
    mln_lang_stack_node_t *node = mln_lang_stack_top(ctx);
    mln_lang_set_t *set = node->data.set;
    mln_lang_set_detail_t *s_detail;
    mln_lang_scope_t *scope;
    if (node->step == 0) {
        if ((s_detail = mln_lang_set_detail_new(ctx->pool, set->name)) == NULL) {
            __mln_lang_errmsg(ctx, "No memory.");
            ctx->quit = 1;
            return;
        }
        ++(s_detail->ref);
        if (__mln_lang_symbol_node_join(ctx, M_LANG_SYMBOL_SET, s_detail) < 0) {
            mln_lang_set_detail_self_free(s_detail);
            ctx->quit = 1;
            return;
        }
        if (set->stm != NULL) {
            node->step = 1;
            if ((scope = mln_lang_scope_push(ctx, s_detail->name, M_LANG_SCOPE_TYPE_SET, NULL, NULL)) == NULL) {
                __mln_lang_errmsg(ctx, "Scope stack is full.");
                ctx->quit = 1;
                return;
            }
            if ((node = mln_lang_stack_push(ctx, M_LSNT_SETSTM, set->stm)) == NULL) {
                __mln_lang_errmsg(ctx, "Stack is full.");
                ctx->quit = 1;
                return;
            }
            mln_lang_stack_map[node->type](ctx);
            return;
        } else {
            mln_lang_stack_node_free(mln_lang_stack_pop(ctx));
        }
    } else {
        mln_lang_scope_pop(ctx);
        mln_lang_stack_node_free(mln_lang_stack_pop(ctx));
    }
})

MLN_FUNC_VOID(static, void, mln_lang_stack_handler_setstm, (mln_lang_ctx_t *ctx), (ctx), {
    mln_lang_stack_node_t *node = mln_lang_stack_top(ctx);
    mln_lang_setstm_t *ls = node->data.set_stm;
    mln_lang_var_t *var;
    mln_lang_val_t *val;
    mln_lang_set_detail_t *in_set;

    if (node->step == 0) {
        node->step = 1;
again:
        if (ls->type == M_SETSTM_VAR) {
            in_set = mln_lang_ctx_get_class(ctx);
            if ((val = __mln_lang_val_new(ctx, M_LANG_VAL_TYPE_NIL, NULL)) == NULL) {
                __mln_lang_errmsg(ctx, "No memory.");
                ctx->quit = 1;
                return;
            }
            if ((var = __mln_lang_var_new(ctx, ls->data.var, M_LANG_VAR_NORMAL, val, in_set)) == NULL) {
                __mln_lang_errmsg(ctx, "No memory.");
                __mln_lang_val_free(val);
                ctx->quit = 1;
                return;
            }
            if (in_set == NULL) {
                if (__mln_lang_symbol_node_join(ctx, M_LANG_SYMBOL_VAR, var) < 0) {
                    __mln_lang_errmsg(ctx, "No memory.");
                    __mln_lang_var_free(var);
                    ctx->quit = 1;
                    return;
                }
            } else {
                if (mln_lang_set_member_add(ctx->pool, in_set->members, var) < 0) {
                    __mln_lang_errmsg(ctx, "No memory.");
                    __mln_lang_var_free(var);
                    ctx->quit = 1;
                    return;
                }
            }
            goto goon1;
        } else {
            if ((node = mln_lang_stack_push(ctx, M_LSNT_FUNCDEF, ls->data.func)) == NULL) {
                __mln_lang_errmsg(ctx, "Stack is full.");
                ctx->quit = 1;
                return;
            }
            mln_lang_stack_map[node->type](ctx);
            return;
        }
    } else {
goon1:
        if (ls->next != NULL) {
            ls = ls->next;
            node->data.set_stm = ls;
            goto again;
        }
        mln_lang_stack_node_free(mln_lang_stack_pop(ctx));
    }
})

MLN_FUNC_VOID(static, void, mln_lang_stack_handler_block, (mln_lang_ctx_t *ctx), (ctx), {
    mln_lang_stack_node_t *node = mln_lang_stack_top(ctx);
    mln_lang_block_t *block = node->data.block;
    if (node->step == 0) {
        mln_lang_ctx_reset_ret_var(ctx);
        node->step = block->type!=M_BLOCK_RETURN? M_LANG_STEP_OUT: 1;
        switch (block->type) {
            case M_BLOCK_EXP:
                if (block->data.exp == NULL) goto nil;
                if (mln_lang_stack_handler_block_exp(ctx, block) < 0) {
                    ctx->quit = 1;
                    return;
                }
                return;
            case M_BLOCK_STM:
                if (block->data.stm == NULL) goto nil;
                if (mln_lang_stack_handler_block_stm(ctx, block) < 0) {
                    ctx->quit = 1;
                    return;
                }
                return;
            case M_BLOCK_CONTINUE:
                if (mln_lang_stack_handler_block_continue(ctx, block) < 0) {
                    ctx->quit = 1;
                    return;
                }
                break;
            case M_BLOCK_BREAK:
                if (mln_lang_stack_handler_block_break(ctx, block) < 0) {
                    ctx->quit = 1;
                    return;
                }
                break;
            case M_BLOCK_RETURN:
                if (mln_lang_stack_handler_block_return(ctx, block) < 0) {
                    ctx->quit = 1;
                    return;
                }
                return;
            case M_BLOCK_GOTO:
                if (mln_lang_stack_handler_block_goto(ctx, block) < 0) {
                    ctx->quit = 1;
                    return;
                }
                return;
            default:
                if (mln_lang_stack_handler_block_if(ctx, block) < 0) {
                    ctx->quit = 1;
                    return;
                }
                return;
        }
    } else {
nil:
        if (block->type != M_BLOCK_RETURN) {
            mln_lang_ctx_reset_ret_var(ctx);
            mln_lang_stack_node_free(mln_lang_stack_pop(ctx));
        } else {
            if (mln_lang_met_return(ctx) < 0) {
                ctx->quit = 1;
                return;
            }
        }
    }
})

MLN_FUNC(static inline, int, mln_lang_met_return, (mln_lang_ctx_t *ctx), (ctx), {
    mln_lang_stack_node_t *node, *last;
    mln_lang_scope_t *scope = mln_lang_scope_top(ctx);
    ASSERT(scope->type == M_LANG_SCOPE_TYPE_FUNC);
    last = mln_lang_stack_pop(ctx);
    ASSERT(last);
    while (1) {
        node = mln_lang_stack_top(ctx);
        if (node == NULL || scope->cur_stack == node) {
            mln_lang_stack_withdraw(ctx);
            ASSERT(last->type == M_LSNT_STM);
            last->step = 1;
            break;
        }
        mln_lang_stack_node_free(last);
        last = mln_lang_stack_pop(ctx);
    }
    if (ctx->ret_var == NULL) {
        mln_lang_var_t *ret_var;
        if ((ret_var = __mln_lang_var_create_nil(ctx, NULL)) == NULL) {
            __mln_lang_errmsg(ctx, "No memory.");
            return -1;
        }
        __mln_lang_ctx_set_ret_var(ctx, ret_var);
    }
    ctx->ret_flag = 1;
    return 0;
})

MLN_FUNC(static inline, int, mln_lang_stack_handler_block_exp, \
         (mln_lang_ctx_t *ctx, mln_lang_block_t *block), (ctx, block), \
{
    mln_lang_stack_node_t *node;
    if (block->jump == NULL)
        mln_lang_generate_jump_ptr(block, M_LSNT_BLOCK);
    if ((node = mln_lang_stack_push(ctx, (mln_lang_stack_node_type_t)(block->jump_type), block->jump)) == NULL) {
        __mln_lang_errmsg(ctx, "Stack is full.");
        return -1;
    }
    mln_lang_stack_map[node->type](ctx);
    return 0;
})

MLN_FUNC(static inline, int, mln_lang_stack_handler_block_stm, \
         (mln_lang_ctx_t *ctx, mln_lang_block_t *block), (ctx, block), \
{
    mln_lang_stack_node_t *node;
    if ((node = mln_lang_stack_push(ctx, M_LSNT_STM, block->data.stm)) == NULL) {
        __mln_lang_errmsg(ctx, "Stack is full.");
        return -1;
    }
    mln_lang_stack_map[node->type](ctx);
    return 0;
})

MLN_FUNC(static inline, int, mln_lang_stack_handler_block_continue, \
         (mln_lang_ctx_t *ctx, mln_lang_block_t *block), (ctx, block), \
{
    mln_lang_stack_node_t *node = mln_lang_withdraw_to_loop(ctx);
    if (node == NULL) {
        __mln_lang_errmsg(ctx, "Invalid 'continue' usage.");
        return -1;
    }
    return 0;
})

MLN_FUNC(static inline, int, mln_lang_stack_handler_block_break, \
         (mln_lang_ctx_t *ctx, mln_lang_block_t *block), (ctx, block), \
{
    mln_lang_stack_node_t *node = mln_lang_withdraw_break_loop(ctx);
    if (node == NULL) {
        __mln_lang_errmsg(ctx, "Invalid 'break' usage.");
        return -1;
    }
    node->step = 1;
    return 0;
})

MLN_FUNC(static inline, int, mln_lang_stack_handler_block_return, \
         (mln_lang_ctx_t *ctx, mln_lang_block_t *block), (ctx, block), \
{
    mln_lang_ctx_reset_ret_var(ctx);
    if (block->data.exp == NULL) {
        mln_lang_var_t *ret_var;
        if ((ret_var = mln_lang_var_create_nil(ctx, NULL)) == NULL) {
            __mln_lang_errmsg(ctx, "No memory.");
            return -1;
        }
        __mln_lang_ctx_set_ret_var(ctx, ret_var);
        return 0;
    }
    mln_lang_stack_node_t *node;
    if ((node = mln_lang_stack_push(ctx, M_LSNT_EXP, block->data.exp)) == NULL) {
        __mln_lang_errmsg(ctx, "Stack is full.");
        return -1;
    }
    mln_lang_stack_map[node->type](ctx);
    return 0;
})

MLN_FUNC(static inline, int, mln_lang_stack_handler_block_goto, \
         (mln_lang_ctx_t *ctx, mln_lang_block_t *block), (ctx, block), \
{
    mln_string_t *pos = block->data.pos;
    mln_lang_stack_node_t *node;
    mln_lang_stm_t *stm;
    ASSERT(mln_lang_scope_top(ctx)->type == M_LANG_SCOPE_TYPE_FUNC);
    mln_lang_ctx_reset_ret_var(ctx);

    for (stm = mln_lang_scope_top(ctx)->entry; stm != NULL; stm = stm->next) {
        if (stm->type == M_STM_LABEL && !mln_string_strcmp(stm->data.pos, pos)) {
            stm = stm->next;
            break;
        }
    }
    if (stm == NULL) {
        __mln_lang_errmsg(ctx, "Invalid label.");
        return -1;
    }

    while (1) {
        node = mln_lang_stack_top(ctx);
        if (node == mln_lang_scope_top(ctx)->cur_stack) {
            break;
        }
        mln_lang_stack_node_free(mln_lang_stack_pop(ctx));
    }

    if ((node = mln_lang_stack_push(ctx, M_LSNT_STM, stm)) == NULL) {
        __mln_lang_errmsg(ctx, "Stack is full.");
        return -1;
    }
    mln_lang_stack_map[node->type](ctx);
    return 0;
})

MLN_FUNC(static inline, int, mln_lang_stack_handler_block_if, \
         (mln_lang_ctx_t *ctx, mln_lang_block_t *block), (ctx, block), \
{
    mln_lang_stack_node_t *node;
    if ((node = mln_lang_stack_push(ctx, M_LSNT_IF, block->data.i)) == NULL) {
        __mln_lang_errmsg(ctx, "Stack is full.");
        return -1;
    }
    mln_lang_stack_map[node->type](ctx);
    return 0;
})

MLN_FUNC(static inline, mln_lang_stack_node_t *, mln_lang_withdraw_to_loop, \
         (mln_lang_ctx_t *ctx), (ctx), \
{
    mln_lang_ctx_reset_ret_var(ctx);
    mln_lang_stack_node_t *node;
    while (1) {
        node = mln_lang_stack_top(ctx);
        if (node == NULL) {
            __mln_lang_errmsg(ctx, "break or continue not in loop statement.");
            return NULL;
        }
        if (node->type == M_LSNT_WHILE || \
            node->type == M_LSNT_SWITCH || \
            node->type == M_LSNT_FOR)
        {
            break;
        }
        mln_lang_stack_node_free(mln_lang_stack_pop(ctx));
    }
    return node;
})

MLN_FUNC(static inline, mln_lang_stack_node_t *, mln_lang_withdraw_break_loop, \
         (mln_lang_ctx_t *ctx), (ctx), \
{
    mln_lang_ctx_reset_ret_var(ctx);
    mln_lang_stack_node_t *node;
    mln_lang_stm_t *stm;
    while (1) {
        node = mln_lang_stack_top(ctx);
        if (node == NULL) {
            __mln_lang_errmsg(ctx, "break or continue not in loop statement.");
            return NULL;
        }
        if (node->type != M_LSNT_STM) {
            mln_lang_stack_node_free(mln_lang_stack_pop(ctx));
            continue;
        }
        stm = node->data.stm;
        if (stm->type != M_STM_SWITCH && stm->type != M_STM_WHILE && stm->type != M_STM_FOR) {
            mln_lang_stack_node_free(mln_lang_stack_pop(ctx));
            continue;
        }
        break;
    }
    return node;
})

MLN_FUNC(static inline, int, mln_lang_withdraw_until_func, \
         (mln_lang_ctx_t *ctx), (ctx), \
{
    mln_lang_stack_node_t *node;
    mln_lang_scope_t *scope = mln_lang_scope_top(ctx);
    ASSERT(scope->type == M_LANG_SCOPE_TYPE_FUNC);
    while (1) {
        node = mln_lang_stack_top(ctx);
        if (node == NULL) break;
        if (scope->cur_stack == node) {
            break;
        }
        mln_lang_stack_node_free(mln_lang_stack_pop(ctx));
    }
    mln_lang_scope_pop(ctx);
    if (ctx->ret_var == NULL) {
        mln_lang_var_t *ret_var;
        if ((ret_var = __mln_lang_var_create_nil(ctx, NULL)) == NULL) {
            __mln_lang_errmsg(ctx, "No memory.");
            return -1;
        }
        __mln_lang_ctx_set_ret_var(ctx, ret_var);
    }
    return 0;
})

MLN_FUNC_VOID(static, void, mln_lang_stack_handler_while, (mln_lang_ctx_t *ctx), (ctx), {
    mln_lang_stack_node_t *node = mln_lang_stack_top(ctx);
    mln_lang_while_t *w = node->data.w;
    if (node->step == 0) {
        node->step = 1;
        if (w->condition == NULL) {
            mln_lang_var_t *var;
            if ((var = __mln_lang_var_create_true(ctx, NULL)) == NULL) {
                __mln_lang_errmsg(ctx, "No memory.");
                ctx->quit = 1;
                return;
            }
            __mln_lang_ctx_set_ret_var(ctx, var);
            goto goon1;
        } else {
            if ((node = mln_lang_stack_push(ctx, M_LSNT_EXP, w->condition)) == NULL) {
                __mln_lang_errmsg(ctx, "Stack is full.");
                ctx->quit = 1;
                return;
            }
            mln_lang_stack_map[node->type](ctx);
            return;
        }
    } else {
goon1:
        if (__mln_lang_condition_is_true(ctx->ret_var)) {
            node->step = 0;
            if (w->blockstm != NULL) {
                if ((node = mln_lang_stack_push(ctx, M_LSNT_BLOCK, w->blockstm)) == NULL) {
                    __mln_lang_errmsg(ctx, "Stack is full.");
                    ctx->quit = 1;
                    return;
                }
                mln_lang_stack_map[node->type](ctx);
                return;
            }
        } else {
            mln_lang_ctx_reset_ret_var(ctx);
            mln_lang_stack_node_free(mln_lang_stack_pop(ctx));
        }
    }
})

MLN_FUNC(, int, mln_lang_condition_is_true, (mln_lang_var_t *var), (var), {
    return __mln_lang_condition_is_true(var);
})

MLN_FUNC(static inline, int, __mln_lang_condition_is_true, (mln_lang_var_t *var), (var), {
    ASSERT(var != NULL);
    mln_lang_val_t *val;
    if ((val = var->val) == NULL) return 0;
    switch (val->type) {
        case M_LANG_VAL_TYPE_NIL:
            break;
        case M_LANG_VAL_TYPE_INT:
            if (val->data.i) return 1;
            break;
        case M_LANG_VAL_TYPE_BOOL:
            if (val->data.b) return 1;
            break;
        case M_LANG_VAL_TYPE_REAL:
            if (val->data.f < -2.2204460492503131E-16 || val->data.f > 2.2204460492503131E-16) return 1;
            break;
        case M_LANG_VAL_TYPE_STRING:
            if (val->data.s != NULL && val->data.s->len) return 1;
            break;
        case M_LANG_VAL_TYPE_OBJECT:
            if (val->data.obj != NULL) return 1;
            break;
        case M_LANG_VAL_TYPE_FUNC:
            if (val->data.func != NULL) return 1;
            break;
        case M_LANG_VAL_TYPE_ARRAY:
            if (val->data.array != NULL && mln_rbtree_node_num(val->data.array->elems_index) > 0) return 1;
            break;
        default:
            mln_log(error, "shouldn't be here. %X\n", val->type);
            abort();
    }
    return 0;
})

MLN_FUNC_VOID(static, void, mln_lang_stack_handler_switch, (mln_lang_ctx_t *ctx), (ctx), {
    mln_lang_stack_node_t *node = mln_lang_stack_top(ctx);
    mln_lang_switch_t *sw = node->data.sw;
    if (node->step == 0) {
        node->step = sw->switchstm == NULL? 2: 1;
        if (sw->condition == NULL) {
            __mln_lang_ctx_set_ret_var(ctx, NULL);
        } else {
            if ((node = mln_lang_stack_push(ctx, M_LSNT_EXP, sw->condition)) == NULL) {
                __mln_lang_errmsg(ctx, "Stack is full.");
                ctx->quit = 1;
                return;
            }
            mln_lang_stack_map[node->type](ctx);
            return;
        }
    } else if (node->step == 1) {
        node->step = 2;
        mln_lang_stack_node_get_ctx_ret_var(node, ctx);
        if ((node = mln_lang_stack_push(ctx, M_LSNT_SWITCHSTM, sw->switchstm)) == NULL) {
            __mln_lang_errmsg(ctx, "Stack is full.");
            ctx->quit = 1;
            return;
        }
        mln_lang_stack_map[node->type](ctx);
        return;
    } else {
        mln_lang_stack_node_free(mln_lang_stack_pop(ctx));
    }
})

MLN_FUNC_VOID(static, void, mln_lang_stack_handler_switchstm, (mln_lang_ctx_t *ctx), (ctx), {
    mln_lang_var_t *res = NULL;
    mln_lang_stack_node_t *node = mln_lang_stack_top(ctx);
    mln_lang_switchstm_t *sw_stm = node->data.sw_stm;
    if (node->step == 0) {
        node->step = 1;
        mln_lang_ctx_reset_ret_var(ctx);
        node = mln_lang_stack_pop(ctx);
        mln_lang_stack_node_t *sw_node = mln_lang_stack_top(ctx);
        mln_lang_stack_withdraw(ctx);
        ASSERT(sw_node != NULL && sw_node->type == M_LSNT_SWITCH);
        if (sw_node->ret_var != NULL && sw_stm->factor != NULL) {
            if ((node = mln_lang_stack_push(ctx, M_LSNT_FACTOR, sw_stm->factor)) == NULL) {
                __mln_lang_errmsg(ctx, "Stack is full.");
                ctx->quit = 1;
                return;
            }
            mln_lang_stack_map[node->type](ctx);
            return;
        } else {
            if (sw_stm->stm == NULL) {
                node->step = 4;
                goto goon4;
            } else {
                node->step = 3;
                goto goon3;
            }
        }
    } else if (node->step == 1) {
        node = mln_lang_stack_pop(ctx);
        mln_lang_stack_node_t *sw_node = mln_lang_stack_top(ctx);
        mln_lang_stack_withdraw(ctx);
        ASSERT(sw_node != NULL && sw_node->type == M_LSNT_SWITCH);
        mln_lang_op handler;
        mln_lang_method_t *method;
        method = mln_lang_methods[mln_lang_var_val_type_get(sw_node->ret_var)];
        if (method == NULL) {
            __mln_lang_errmsg(ctx, "Operation NOT support.");
            ctx->quit = 1;
            return;
        }
        handler = method->equal_handler;
        if (handler == NULL) {
            __mln_lang_errmsg(ctx, "Operation NOT support.");
            ctx->quit = 1;
            return;
        }
        if (handler(ctx, &res, sw_node->ret_var, ctx->ret_var) < 0) {
            ctx->quit = 1;
            return;
        }
        __mln_lang_ctx_set_ret_var(ctx, res);
        if (res->val->type == M_LANG_VAL_TYPE_CALL) {
            node->call = 1;
again:
            node->step = 2;
            if (mln_lang_stack_handler_funccall_run(ctx, node, res->val->data.call) < 0) {
                ctx->quit = 1;
                return;
            }
        } else {
            node->step = 2;
            goto goon2;
        }
    } else if (node->step == 2) {
goon2:
        node = mln_lang_stack_pop(ctx);
        mln_lang_stack_node_t *sw_node = mln_lang_stack_top(ctx);
        mln_lang_stack_withdraw(ctx);
        if (node->call) {
            if (mln_lang_withdraw_until_func(ctx) < 0) {
                ctx->quit = 1;
                return;
            }
            res = ctx->ret_var;
            if (ctx->ret_var->val->type == M_LANG_VAL_TYPE_CALL) goto again;
        }
        if (__mln_lang_condition_is_true(ctx->ret_var)) {
            mln_lang_stack_node_reset_ret_val(sw_node);
            node->step = 3;
            goto goon3;
        } else {
            node->step = 4;
            goto goon4;
        }
        mln_lang_ctx_reset_ret_var(ctx);
    } else if (node->step == 3) {
goon3:
        ASSERT(ctx->ret_var == NULL && sw_stm->stm != NULL);
        node->step = 4;
        if ((node = mln_lang_stack_push(ctx, M_LSNT_STM, sw_stm->stm)) == NULL) {
            __mln_lang_errmsg(ctx, "Stack is full.");
            ctx->quit = 1;
            return;
        }
        mln_lang_stack_map[node->type](ctx);
        return;
    } else {
goon4:
        mln_lang_stack_node_free(mln_lang_stack_pop(ctx));
        if (sw_stm->next != NULL) {
            if ((node = mln_lang_stack_push(ctx, M_LSNT_SWITCHSTM, sw_stm->next)) == NULL) {
                __mln_lang_errmsg(ctx, "Stack is full.");
                ctx->quit = 1;
                return;
            }
            mln_lang_stack_map[node->type](ctx);
            return;
        }
    }
})

MLN_FUNC_VOID(static, void, mln_lang_stack_handler_for, (mln_lang_ctx_t *ctx), (ctx), {
    mln_lang_stack_node_t *node = mln_lang_stack_top(ctx);
    mln_lang_for_t *f = node->data.f;

    if (node->step == 0) {
        node->step = 1;
        if (f->init_exp != NULL) {
            if ((node = mln_lang_stack_push(ctx, M_LSNT_EXP, f->init_exp)) == NULL) {
                __mln_lang_errmsg(ctx, "Stack is full.");
                ctx->quit = 1;
                return;
            }
            mln_lang_stack_map[node->type](ctx);
            return;
        } else {
            goto goon1;
        }
    } else if (node->step == 1) {
goon1:
        node->step = 2;
        if (f->condition != NULL) {
            if ((node = mln_lang_stack_push(ctx, M_LSNT_EXP, f->condition)) == NULL) {
                __mln_lang_errmsg(ctx, "Stack is full.");
                ctx->quit = 1;
                return;
            }
            mln_lang_stack_map[node->type](ctx);
            return;
        } else {
            goto goon2;
        }
    } else if (node->step == 2) {
goon2:
        if (ctx->ret_var == NULL || __mln_lang_condition_is_true(ctx->ret_var)) {
            node->step = 3;
            if ((node = mln_lang_stack_push(ctx, M_LSNT_BLOCK, f->blockstm)) == NULL) {
                __mln_lang_errmsg(ctx, "Stack is full.");
                ctx->quit = 1;
                return;
            }
            mln_lang_stack_map[node->type](ctx);
            return;
        } else {
            mln_lang_stack_node_free(mln_lang_stack_pop(ctx));
        }
    } else {
        node->step = 1;
        if (f->mod_exp != NULL) {
            if ((node = mln_lang_stack_push(ctx, M_LSNT_EXP, f->mod_exp)) == NULL) {
                __mln_lang_errmsg(ctx, "Stack is full.");
                ctx->quit = 1;
                return;
            }
            mln_lang_stack_map[node->type](ctx);
            return;
        } else {
            goto goon1;
        }
    }
})

MLN_FUNC_VOID(static, void, mln_lang_stack_handler_if, (mln_lang_ctx_t *ctx), (ctx), {
    mln_lang_stack_node_t *node = mln_lang_stack_top(ctx);
    mln_lang_if_t *i = node->data.i;

    if (node->step == 0) {
        node->step = 1;
        mln_lang_ctx_reset_ret_var(ctx);
        if (i->condition != NULL) {
            if ((node = mln_lang_stack_push(ctx, M_LSNT_EXP, i->condition)) == NULL) {
                __mln_lang_errmsg(ctx, "Stack is full.");
                ctx->quit = 1;
                return;
            }
            mln_lang_stack_map[node->type](ctx);
            return;
        } else {
            goto goon1;
        }
    } else {
goon1:
        mln_lang_stack_node_free(mln_lang_stack_pop(ctx));
        if (!__mln_lang_condition_is_true(ctx->ret_var)) {
            if (i->elsestm != NULL) {
                if ((node = mln_lang_stack_push(ctx, M_LSNT_BLOCK, i->elsestm)) == NULL) {
                    __mln_lang_errmsg(ctx, "Stack is full.");
                    ctx->quit = 1;
                    return;
                }
            } else {
                node = NULL;
            }
        } else {
            if ((node = mln_lang_stack_push(ctx, M_LSNT_BLOCK, i->blockstm)) == NULL) {
                __mln_lang_errmsg(ctx, "Stack is full.");
                ctx->quit = 1;
                return;
            }
        }
        if (node != NULL) {
            mln_lang_stack_map[node->type](ctx);
            return;
        }
        mln_lang_ctx_reset_ret_var(ctx);
    }
})

MLN_FUNC_VOID(static inline, void, mln_lang_generate_jump_ptr, \
              (void *ptr, mln_lang_stack_node_type_t type), (ptr, type), \
{
    switch (type) {
        case M_LSNT_STM:
        {
            mln_lang_stm_t *stm = (mln_lang_stm_t *)ptr;
            switch (stm->type) {
                case M_STM_BLOCK:
                    if (stm->data.block->type == M_BLOCK_EXP) {
                        if (stm->data.block->jump == NULL)
                            mln_lang_generate_jump_ptr(stm->data.block, M_LSNT_BLOCK);
                        stm->jump = stm->data.block->jump;
                        stm->jump_type = stm->data.block->jump_type;
                    } else {
                        stm->jump = stm->data.block;
                        stm->jump_type = M_LSNT_BLOCK;
                    }
                    break;
                case M_STM_FUNC:
                    stm->jump = stm->data.func;
                    stm->jump_type = M_LSNT_FUNCDEF;
                    break;
                case M_STM_SET:
                    stm->jump = stm->data.setdef;
                    stm->jump_type = M_LSNT_SET;
                    break;
                case M_STM_LABEL:
                    break;
                case M_STM_SWITCH:
                    stm->jump = stm->data.sw;
                    stm->jump_type = M_LSNT_SWITCH;
                    break;
                case M_STM_WHILE:
                    stm->jump = stm->data.w;
                    stm->jump_type = M_LSNT_WHILE;
                    break;
                default:
                    stm->jump = stm->data.f;
                    stm->jump_type = M_LSNT_FOR;
                    break;
            }
            break;
        }
        case M_LSNT_BLOCK:
        {
            mln_lang_block_t *block = (mln_lang_block_t *)ptr;
            ASSERT(block->type == M_BLOCK_EXP);
            if (block->data.exp->jump == NULL)
                mln_lang_generate_jump_ptr(block->data.exp, M_LSNT_EXP);
            block->jump = block->data.exp->jump;
            block->jump_type = block->data.exp->type;
            break;
        }
        case M_LSNT_EXP:
        {
            mln_lang_exp_t *exp = (mln_lang_exp_t *)ptr;
            if (exp->assign->op != M_ASSIGN_NONE) {
                exp->jump = exp->assign;
                exp->type = M_LSNT_ASSIGN;
            } else {
                if (exp->assign->jump == NULL)
                    mln_lang_generate_jump_ptr(exp->assign, M_LSNT_ASSIGN);
                exp->jump = exp->assign->jump;
                exp->type = exp->assign->type;
            }
            break;
        }
        case M_LSNT_ASSIGN:
        {
            mln_lang_assign_t *a = (mln_lang_assign_t *)ptr;
            if (a->left->op != M_LOGICLOW_NONE) {
                a->jump = a->left;
                a->type = M_LSNT_LOGICLOW;
            } else {
                if (a->left->jump == NULL)
                    mln_lang_generate_jump_ptr(a->left, M_LSNT_LOGICLOW);
                a->jump = a->left->jump;
                a->type = a->left->type;
            }
            break;
        }
        case M_LSNT_LOGICLOW:
        {
            mln_lang_logiclow_t *l = (mln_lang_logiclow_t *)ptr;
            if (l->left->op != M_LOGICHIGH_NONE) {
                l->jump = l->left;
                l->type = M_LSNT_LOGICHIGH;
            } else {
                if (l->left->jump == NULL)
                    mln_lang_generate_jump_ptr(l->left, M_LSNT_LOGICHIGH);
                l->jump = l->left->jump;
                l->type = l->left->type;
            }
            break;
        }
        case M_LSNT_LOGICHIGH:
        {
            mln_lang_logichigh_t *l = (mln_lang_logichigh_t *)ptr;
            if (l->left->op != M_RELATIVELOW_NONE) {
                l->jump = l->left;
                l->type = M_LSNT_RELATIVELOW;
            } else {
                if (l->left->jump == NULL)
                    mln_lang_generate_jump_ptr(l->left, M_LSNT_RELATIVELOW);
                l->jump = l->left->jump;
                l->type = l->left->type;
            }
            break;
        }
        case M_LSNT_RELATIVELOW:
        {
            mln_lang_relativelow_t *r = (mln_lang_relativelow_t *)ptr;
            if (r->left->op != M_RELATIVEHIGH_NONE) {
                r->jump = r->left;
                r->type = M_LSNT_RELATIVEHIGH;
            } else {
                if (r->left->jump == NULL)
                    mln_lang_generate_jump_ptr(r->left, M_LSNT_RELATIVEHIGH);
                r->jump = r->left->jump;
                r->type = r->left->type;
            }
            break;
        }
        case M_LSNT_RELATIVEHIGH:
        {
            mln_lang_relativehigh_t *r = (mln_lang_relativehigh_t *)ptr;
            if (r->left->op != M_MOVE_NONE) {
                r->jump = r->left;
                r->type = M_LSNT_MOVE;
            } else {
                if (r->left->jump == NULL)
                    mln_lang_generate_jump_ptr(r->left, M_LSNT_MOVE);
                r->jump = r->left->jump;
                r->type = r->left->type;
            }
            break;
        }
        case M_LSNT_MOVE:
        {
            mln_lang_move_t *m = (mln_lang_move_t *)ptr;
            if (m->left->op != M_ADDSUB_NONE) {
                m->jump = m->left;
                m->type = M_LSNT_ADDSUB;
            } else {
                if (m->left->jump == NULL)
                    mln_lang_generate_jump_ptr(m->left, M_LSNT_ADDSUB);
                m->jump = m->left->jump;
                m->type = m->left->type;
            }
            break;
        }
        case M_LSNT_ADDSUB:
        {
            mln_lang_addsub_t *a = (mln_lang_addsub_t *)ptr;
            if (a->left->op != M_MULDIV_NONE) {
                a->jump = a->left;
                a->type = M_LSNT_MULDIV;
            } else {
                if (a->left->jump == NULL)
                    mln_lang_generate_jump_ptr(a->left, M_LSNT_MULDIV);
                a->jump = a->left->jump;
                a->type = a->left->type;
            }
            break;
        }
        case M_LSNT_MULDIV:
        {
            mln_lang_muldiv_t *m = (mln_lang_muldiv_t *)ptr;
            if (m->left->op == M_NOT_NONE) {
                if (m->left->jump == NULL)
                    mln_lang_generate_jump_ptr(m->left, M_LSNT_NOT);
                m->jump = m->left->jump;
                m->type = m->left->type;
            } else {
                m->jump = m->left;
                m->type = M_LSNT_NOT;
            }
            break;
        }
        case M_LSNT_NOT:
        {
            mln_lang_not_t *n = (mln_lang_not_t *)ptr;
            if (n->op != M_NOT_NONE) {
                ASSERT(n->op == M_NOT_NOT);
                n->jump = n->right.not;
                n->type = M_LSNT_NOT;
            } else {
                if (n->right.suffix->op != M_SUFFIX_NONE) {
                    n->jump = n->right.suffix;
                    n->type = M_LSNT_SUFFIX;
                } else {
                    if (n->right.suffix->jump == NULL)
                        mln_lang_generate_jump_ptr(n->right.suffix, M_LSNT_SUFFIX);
                    n->jump = n->right.suffix->jump;
                    n->type = n->right.suffix->type;
                }
            }
            break;
        }
        case M_LSNT_SUFFIX:
        {
            mln_lang_suffix_t *s = (mln_lang_suffix_t *)ptr;
            if (s->left->op != M_LOCATE_NONE) {
                s->jump = s->left;
                s->type = M_LSNT_LOCATE;
            } else {
                if (s->left->jump == NULL)
                    mln_lang_generate_jump_ptr(s->left, M_LSNT_LOCATE);
                s->jump = s->left->jump;
                s->type = s->left->type;
            }
            break;
        }
        case M_LSNT_LOCATE:
        {
            mln_lang_locate_t *l = (mln_lang_locate_t *)ptr;
            if (l->left->op != M_SPEC_FACTOR) {
                l->jump = l->left;
                l->type = M_LSNT_SPEC;
            } else {
                l->jump = l->left->data.factor;
                l->type = M_LSNT_FACTOR;
            }
            break;
        }
        default:
            break;
    }
})

MLN_FUNC_VOID(static, void, mln_lang_stack_handler_exp, (mln_lang_ctx_t *ctx), (ctx), {
    mln_lang_stack_node_t *node = mln_lang_stack_top(ctx);
    mln_lang_exp_t *exp = node->data.exp;
    if (node->step == 0) {
        node->step = 1;
again:
        mln_lang_ctx_reset_ret_var(ctx);
        if (exp->jump == NULL)
            mln_lang_generate_jump_ptr(exp, M_LSNT_EXP);
        if (exp->next == NULL) {
            if (exp->type == M_LSNT_FACTOR) {
                mln_lang_stack_node_free(mln_lang_stack_pop(ctx));
            } else {
                node->step = M_LANG_STEP_OUT;
            }
        }
        if ((node = mln_lang_stack_push(ctx, (mln_lang_stack_node_type_t)(exp->type), exp->jump)) == NULL) {
            __mln_lang_errmsg(ctx, "Stack is full.");
            ctx->quit = 1;
            return;
        }
        mln_lang_stack_map[node->type](ctx);
        return;
    } else {
        if (exp->next != NULL) {
            node->data.exp = exp->next;
            exp = exp->next;
            node->step = 1;
            goto again;
        } else {
            mln_lang_stack_node_free(mln_lang_stack_pop(ctx));
            mln_lang_stack_popuntil(ctx);
        }
    }
})

MLN_FUNC_VOID(static, void, mln_lang_stack_handler_assign, (mln_lang_ctx_t *ctx), (ctx), {
    mln_lang_var_t *res = NULL;
    mln_lang_stack_node_t *node = mln_lang_stack_top(ctx);
    mln_lang_assign_t *assign = node->data.assign;
    if (node->step == 0) {
        mln_lang_ctx_reset_ret_var(ctx);
        if (assign->op == M_ASSIGN_NONE) node->step = M_LANG_STEP_OUT;
        else node->step = 1;
        if (assign->jump == NULL)
            mln_lang_generate_jump_ptr(assign, M_LSNT_ASSIGN);
        if (assign->type == M_LSNT_FACTOR && assign->op == M_ASSIGN_NONE) {
            mln_lang_stack_node_free(mln_lang_stack_pop(ctx));
            mln_lang_stack_popuntil(ctx);
        }
        if ((node = mln_lang_stack_push(ctx, (mln_lang_stack_node_type_t)(assign->type), assign->jump)) == NULL) {
            __mln_lang_errmsg(ctx, "Stack is full.");
            ctx->quit = 1;
            return;
        }
        mln_lang_stack_map[node->type](ctx);
        return;
    } else if (node->step == 1) {
        node->step = 2;
        ASSERT(assign->right != NULL);
        mln_lang_stack_node_get_ctx_ret_var(node, ctx);
        if ((node = mln_lang_stack_push(ctx, M_LSNT_ASSIGN, assign->right)) == NULL) {
            __mln_lang_errmsg(ctx, "Stack is full.");
            ctx->quit = 1;
            return;
        }
        mln_lang_stack_map[node->type](ctx);
        return;
    } else if (node->step == 2) {
        node->step = 3;
        mln_lang_op handler = NULL;
        mln_lang_method_t *method;
        if (assign->op != M_ASSIGN_NONE) {
            /*left may be undefined, so must based on right type.*/
            method = mln_lang_methods[mln_lang_var_val_type_get(ctx->ret_var)];
            if (method == NULL) {
                __mln_lang_errmsg(ctx, "Operation NOT support.");
                ctx->quit = 1;
                return;
            }
            switch (assign->op) {
                case M_ASSIGN_EQUAL:
                    handler = method->assign_handler;
                    break;
                case M_ASSIGN_PLUSEQ:
                    handler = method->pluseq_handler;
                    break;
                case M_ASSIGN_SUBEQ:
                    handler = method->subeq_handler;
                    break;
                case M_ASSIGN_LMOVEQ:
                    handler = method->lmoveq_handler;
                    break;
                case M_ASSIGN_RMOVEQ:
                    handler = method->rmoveq_handler;
                    break;
                case M_ASSIGN_MULEQ:
                    handler = method->muleq_handler;
                    break;
                case M_ASSIGN_DIVEQ:
                    handler = method->diveq_handler;
                    break;
                case M_ASSIGN_OREQ:
                    handler = method->oreq_handler;
                    break;
                case M_ASSIGN_ANDEQ:
                    handler = method->andeq_handler;
                    break;
                case M_ASSIGN_XOREQ:
                    handler = method->xoreq_handler;
                    break;
                case M_ASSIGN_MODEQ:
                    handler = method->modeq_handler;
                    break;
                default:
                    break;
            }
        }
        if (handler != NULL) {
            if (mln_lang_val_not_modify_isset(node->ret_var->val)) {
                __mln_lang_errmsg(ctx, "Operand cannot be changed.");
                ctx->quit = 1;
                return;
            }
            if (handler(ctx, &res, node->ret_var, ctx->ret_var) < 0) {
                ctx->quit = 1;
                return;
            }
            __mln_lang_ctx_set_ret_var(ctx, res);
            if (res->val->type == M_LANG_VAL_TYPE_CALL) {
                node->call = 1;
again:
                if (mln_lang_stack_handler_funccall_run(ctx, node, res->val->data.call) < 0) {
                    ctx->quit = 1;
                    return;
                }
            } else {
                goto goon3;
            }
        } else {
            if (assign->op == M_ASSIGN_NONE) {
                mln_lang_ctx_get_node_ret_val(ctx, node);
            } else {
                __mln_lang_errmsg(ctx, "Operation NOT support.");
                ctx->quit = 1;
                return;
            }
            goto goon3;
        }
    } else if (node->step == 3) {
        if (node->call) {
            if (mln_lang_withdraw_until_func(ctx) < 0) {
                ctx->quit = 1;
                return;
            }
            res = ctx->ret_var;
            if (ctx->ret_var->val->type == M_LANG_VAL_TYPE_CALL) goto again;
            node->call = 0;
        }
goon3:
        node->step = 4;
        if (node->ret_var != NULL && node->ret_var->val->func != NULL) {
            if (mln_lang_watch_func_build(ctx, \
                                         node, \
                                         node->ret_var->val->func, \
                                         node->ret_var->val->udata, \
                                         node->ret_var->val) < 0)
            {
                ctx->quit = 1;
                return;
            }
            mln_lang_stack_node_get_ctx_ret_var(node, ctx);
            mln_lang_ctx_reset_ret_var(ctx);
        } else {
            goto goon4;
        }
    } else if (node->step == 4) {
        if (node->call) {
            if (mln_lang_withdraw_until_func(ctx) < 0) {
                ctx->quit = 1;
                return;
            }
            node->call = 0;
        }
        mln_lang_ctx_get_node_ret_val(ctx, node);
goon4:
        mln_lang_stack_node_free(mln_lang_stack_pop(ctx));
        mln_lang_stack_popuntil(ctx);
    } else {
        mln_lang_stack_node_free(mln_lang_stack_pop(ctx));
        mln_lang_stack_popuntil(ctx);
    }
})

MLN_FUNC_VOID(static, void, mln_lang_stack_handler_logiclow, (mln_lang_ctx_t *ctx), (ctx), {
    mln_lang_stack_node_t *node = mln_lang_stack_top(ctx);
    mln_lang_logiclow_t *logiclow = node->data.logiclow;
    if (node->step == 0) {
        mln_lang_stack_node_reset_ret_val(node);
        mln_lang_ctx_reset_ret_var(ctx);
        if (logiclow->op == M_LOGICLOW_NONE) node->step = M_LANG_STEP_OUT;
        else node->step = 1;
        if (logiclow->jump == NULL)
            mln_lang_generate_jump_ptr(logiclow, M_LSNT_LOGICLOW);
        if (logiclow->type == M_LSNT_FACTOR && logiclow->op == M_LOGICLOW_NONE) {
            mln_lang_stack_node_free(mln_lang_stack_pop(ctx));
            mln_lang_stack_popuntil(ctx);
        }
        if ((node = mln_lang_stack_push(ctx, (mln_lang_stack_node_type_t)(logiclow->type), logiclow->jump)) == NULL) {
            __mln_lang_errmsg(ctx, "Stack is full.");
            ctx->quit = 1;
            return;
        }
        mln_lang_stack_map[node->type](ctx);
        return;
    } else if (node->step == 1) {
        node->step = 2;
        ASSERT(logiclow->right != NULL);
        if (__mln_lang_condition_is_true(ctx->ret_var)) {
            if (logiclow->op == M_LOGICLOW_AND) {
                if ((node = mln_lang_stack_push(ctx, M_LSNT_LOGICLOW, logiclow->right)) == NULL) {
                    __mln_lang_errmsg(ctx, "Stack is full.");
                    ctx->quit = 1;
                    return;
                }
                mln_lang_stack_map[node->type](ctx);
                return;
            }
        } else {
again:
            if (logiclow->op == M_LOGICLOW_OR) {
                if ((node = mln_lang_stack_push(ctx, M_LSNT_LOGICLOW, logiclow->right)) == NULL) {
                    __mln_lang_errmsg(ctx, "Stack is full.");
                    ctx->quit = 1;
                    return;
                }
                mln_lang_stack_map[node->type](ctx);
                return;
            } else {
                logiclow = logiclow->right;
                if (logiclow != NULL) {
                    goto again;
                }
            }
        }
    } else {
        mln_lang_stack_node_free(mln_lang_stack_pop(ctx));
        mln_lang_stack_popuntil(ctx);
    }
})

MLN_FUNC_VOID(static, void, mln_lang_stack_handler_logichigh, (mln_lang_ctx_t *ctx), (ctx), {
    mln_lang_stack_node_t *node = mln_lang_stack_top(ctx);
    mln_lang_logichigh_t *ll, *logichigh = (mln_lang_logichigh_t *)(node->pos);
    mln_lang_var_t *res = NULL;
    if (node->step == 0) {
        mln_lang_stack_node_reset_ret_val(node);
        mln_lang_ctx_reset_ret_var(ctx);
        if (logichigh->op == M_LOGICHIGH_NONE) node->step = M_LANG_STEP_OUT;
        else node->step = 1;
        if (logichigh->jump == NULL)
            mln_lang_generate_jump_ptr(logichigh, M_LSNT_LOGICHIGH);
        if (logichigh->type == M_LSNT_FACTOR && logichigh->op == M_LOGICHIGH_NONE) {
            mln_lang_stack_node_free(mln_lang_stack_pop(ctx));
            mln_lang_stack_popuntil(ctx);
        }
        if ((node = mln_lang_stack_push(ctx, (mln_lang_stack_node_type_t)(logichigh->type), logichigh->jump)) == NULL) {
            __mln_lang_errmsg(ctx, "Stack is full.");
            ctx->quit = 1;
            return;
        }
        mln_lang_stack_map[node->type](ctx);
        return;
    } else if (node->step == 1) {
        node->step = 2;
        ASSERT(logichigh->right != NULL);
        ASSERT(node->pos != NULL);
        mln_lang_stack_node_get_ctx_ret_var(node, ctx);
        goto goon2;
    } else if (node->step == 2) {
goon2:
        node->step = 3;
        ll = logichigh->right;
        if ((node = mln_lang_stack_push(ctx, M_LSNT_RELATIVELOW, ll->left)) == NULL) {
            __mln_lang_errmsg(ctx, "Stack is full.");
            ctx->quit = 1;
            return;
        }
        mln_lang_stack_map[node->type](ctx);
        return;
    } else if (node->step == 3) {
        node->step = 4;
        mln_lang_op handler = NULL;
        mln_lang_method_t *method;
        method = mln_lang_methods[mln_lang_var_val_type_get(node->ret_var)];
        if (method == NULL) {
            __mln_lang_errmsg(ctx, "Operation NOT support.");
            ctx->quit = 1;
            return;
        }
        switch (logichigh->op) {
            case M_LOGICHIGH_OR:
                handler = method->cor_handler;
                break;
            case M_LOGICHIGH_AND:
                handler = method->cand_handler;
                break;
            case M_LOGICHIGH_XOR:
                handler = method->cxor_handler;
                break;
            default:
                ASSERT(0);
                break;
        }
        if (handler == NULL) {
            __mln_lang_errmsg(ctx, "Operation NOT support.");
            ctx->quit = 1;
            return;
        }
        if (handler(ctx, &res, node->ret_var, ctx->ret_var) < 0) {
            ctx->quit = 1;
            return;
        }
        mln_lang_stack_node_get_ctx_ret_var(node, ctx);
        __mln_lang_ctx_set_ret_var(ctx, res);
        if (res->val->type == M_LANG_VAL_TYPE_CALL) {
again:
            node->call = 1;
            if (mln_lang_stack_handler_funccall_run(ctx, node, res->val->data.call) < 0) {
                ctx->quit = 1;
                return;
            }
        } else {
            goto goon4;
        }
    } else if (node->step == 4) {
        if (node->call) {
            if (mln_lang_withdraw_until_func(ctx) < 0) {
                ctx->quit = 1;
                return;
            }
            node->call = 0;
            res = ctx->ret_var;
            if (res->val->type == M_LANG_VAL_TYPE_CALL) goto again;
        }
goon4:
        node->pos = logichigh->right;
        if (node->pos != NULL && logichigh->right->op != M_LOGICHIGH_NONE) {
            node->step = 2;
            mln_lang_stack_node_get_ctx_ret_var(node, ctx);
        } else {
            mln_lang_stack_node_free(mln_lang_stack_pop(ctx));
            mln_lang_stack_popuntil(ctx);
        }
    } else {
        mln_lang_stack_node_free(mln_lang_stack_pop(ctx));
        mln_lang_stack_popuntil(ctx);
    }
})

MLN_FUNC_VOID(static, void, mln_lang_stack_handler_relativelow, (mln_lang_ctx_t *ctx), (ctx), {
    mln_lang_stack_node_t *node = mln_lang_stack_top(ctx);
    mln_lang_relativelow_t *tmp, *relativelow = (mln_lang_relativelow_t *)(node->pos);
    mln_lang_var_t *res = NULL;
    if (node->step == 0) {
        mln_lang_stack_node_reset_ret_val(node);
        mln_lang_ctx_reset_ret_var(ctx);
        if (relativelow->op == M_RELATIVELOW_NONE) node->step = M_LANG_STEP_OUT;
        else node->step = 1;
        if (relativelow->jump == NULL)
            mln_lang_generate_jump_ptr(relativelow, M_LSNT_RELATIVELOW);
        if (relativelow->type == M_LSNT_FACTOR && relativelow->op == M_RELATIVELOW_NONE) {
            mln_lang_stack_node_free(mln_lang_stack_pop(ctx));
            mln_lang_stack_popuntil(ctx);
        }
        if ((node = mln_lang_stack_push(ctx, (mln_lang_stack_node_type_t)(relativelow->type), relativelow->jump)) == NULL) {
            __mln_lang_errmsg(ctx, "Stack is full.");
            ctx->quit = 1;
            return;
        }
        mln_lang_stack_map[node->type](ctx);
        return;
    } else if (node->step == 1) {
        node->step = 2;
        ASSERT(relativelow->right != NULL);
        ASSERT(node->pos != NULL);
        mln_lang_stack_node_get_ctx_ret_var(node, ctx);
        goto goon2;
    } else if (node->step == 2) {
goon2:
        node->step = 3;
        tmp = relativelow->right;
        if ((node = mln_lang_stack_push(ctx, M_LSNT_RELATIVEHIGH, tmp->left)) == NULL) {
            __mln_lang_errmsg(ctx, "Stack is full.");
            ctx->quit = 1;
            return;
        }
        mln_lang_stack_map[node->type](ctx);
        return;
    } else if (node->step == 3) {
        node->step = 4;
        mln_lang_op handler = NULL;
        mln_lang_method_t *method;
        method = mln_lang_methods[mln_lang_var_val_type_get(node->ret_var)];
        if (method == NULL) {
            __mln_lang_errmsg(ctx, "Operation NOT support.");
            ctx->quit = 1;
            return;
        }
        switch (relativelow->op) {
            case M_RELATIVELOW_EQUAL:
                handler = method->equal_handler;
                break;
            case M_RELATIVELOW_NEQUAL:
                handler = method->nonequal_handler;
                break;
            default:
                ASSERT(0);
                break;
        }
        if (handler == NULL) {
            __mln_lang_errmsg(ctx, "Operation NOT support.");
            ctx->quit = 1;
            return;
        }
        if (handler(ctx, &res, node->ret_var, ctx->ret_var) < 0) {
            ctx->quit = 1;
            return;
        }
        mln_lang_stack_node_get_ctx_ret_var(node, ctx);
        __mln_lang_ctx_set_ret_var(ctx, res);
        if (res->val->type == M_LANG_VAL_TYPE_CALL) {
again:
            node->call = 1;
            if (mln_lang_stack_handler_funccall_run(ctx, node, res->val->data.call) < 0) {
                ctx->quit = 1;
                return;
            }
        } else {
            goto goon4;
        }
    } else if (node->step == 4) {
        if (node->call) {
            if (mln_lang_withdraw_until_func(ctx) < 0) {
                ctx->quit = 1;
                return;
            }
            node->call = 0;
            res = ctx->ret_var;
            if (res->val->type == M_LANG_VAL_TYPE_CALL) goto again;
        }
goon4:
        node->pos = relativelow->right;
        if (node->pos != NULL && relativelow->right->op != M_RELATIVELOW_NONE) {
            node->step = 2;
            mln_lang_stack_node_get_ctx_ret_var(node, ctx);
        } else {
            mln_lang_stack_node_free(mln_lang_stack_pop(ctx));
            mln_lang_stack_popuntil(ctx);
        }
    } else {
        mln_lang_stack_node_free(mln_lang_stack_pop(ctx));
        mln_lang_stack_popuntil(ctx);
    }
})

MLN_FUNC_VOID(static, void, mln_lang_stack_handler_relativehigh, (mln_lang_ctx_t *ctx), (ctx), {
    mln_lang_stack_node_t *node = mln_lang_stack_top(ctx);
    mln_lang_relativehigh_t *tmp, *relativehigh = (mln_lang_relativehigh_t *)(node->pos);
    mln_lang_var_t *res = NULL;
    if (node->step == 0) {
        mln_lang_stack_node_reset_ret_val(node);
        mln_lang_ctx_reset_ret_var(ctx);
        if (relativehigh->op == M_RELATIVEHIGH_NONE) node->step = M_LANG_STEP_OUT;
        else node->step = 1;
        if (relativehigh->jump == NULL)
            mln_lang_generate_jump_ptr(relativehigh, M_LSNT_RELATIVEHIGH);
        if (relativehigh->type == M_LSNT_FACTOR && relativehigh->op == M_RELATIVEHIGH_NONE) {
            mln_lang_stack_node_free(mln_lang_stack_pop(ctx));
            mln_lang_stack_popuntil(ctx);
        }
        if ((node = mln_lang_stack_push(ctx, (mln_lang_stack_node_type_t)(relativehigh->type), relativehigh->jump)) == NULL) {
            __mln_lang_errmsg(ctx, "Stack is full.");
            ctx->quit = 1;
            return;
        }
        mln_lang_stack_map[node->type](ctx);
        return;
    } else if (node->step == 1) {
        node->step = 2;
        ASSERT(relativehigh->right != NULL);
        ASSERT(node->pos != NULL);
        mln_lang_stack_node_get_ctx_ret_var(node, ctx);
        goto goon2;
    } else if (node->step == 2) {
goon2:
        node->step = 3;
        tmp = relativehigh->right;
        if ((node = mln_lang_stack_push(ctx, M_LSNT_MOVE, tmp->left)) == NULL) {
            __mln_lang_errmsg(ctx, "Stack is full.");
            ctx->quit = 1;
            return;
        }
        mln_lang_stack_map[node->type](ctx);
        return;
    } else if (node->step == 3) {
        node->step = 4;
        mln_lang_op handler = NULL;
        mln_lang_method_t *method;
        method = mln_lang_methods[mln_lang_var_val_type_get(node->ret_var)];
        if (method == NULL) {
            __mln_lang_errmsg(ctx, "Operation NOT support.");
            ctx->quit = 1;
            return;
        }
        switch (relativehigh->op) {
            case M_RELATIVEHIGH_LESS:
                handler = method->less_handler;
                break;
            case M_RELATIVEHIGH_LESSEQ:
                handler = method->lesseq_handler;
                break;
            case M_RELATIVEHIGH_GREATER:
                handler = method->grea_handler;
                break;
            case M_RELATIVEHIGH_GREATEREQ:
                handler = method->greale_handler;
                break;
            default:
                ASSERT(0);
                break;
        }
        if (handler == NULL) {
            __mln_lang_errmsg(ctx, "Operation NOT support.");
            ctx->quit = 1;
            return;
        }
        if (handler(ctx, &res, node->ret_var, ctx->ret_var) < 0) {
            ctx->quit = 1;
            return;
        }
        mln_lang_stack_node_get_ctx_ret_var(node, ctx);
        __mln_lang_ctx_set_ret_var(ctx, res);
        if (res->val->type == M_LANG_VAL_TYPE_CALL) {
again:
            node->call = 1;
            if (mln_lang_stack_handler_funccall_run(ctx, node, res->val->data.call) < 0) {
                ctx->quit = 1;
                return;
            }
        } else {
            goto goon4;
        }
    } else if (node->step == 4) {
        if (node->call) {
            if (mln_lang_withdraw_until_func(ctx) < 0) {
                ctx->quit = 1;
                return;
            }
            node->call = 0;
            res = ctx->ret_var;
            if (res->val->type == M_LANG_VAL_TYPE_CALL) goto again;
        }
goon4:
        node->pos = relativehigh->right;
        if (node->pos != NULL && relativehigh->right->op != M_RELATIVEHIGH_NONE) {
            node->step = 2;
            mln_lang_stack_node_get_ctx_ret_var(node, ctx);
        } else {
            mln_lang_stack_node_free(mln_lang_stack_pop(ctx));
            mln_lang_stack_popuntil(ctx);
        }
    } else {
        mln_lang_stack_node_free(mln_lang_stack_pop(ctx));
        mln_lang_stack_popuntil(ctx);
    }
})

MLN_FUNC_VOID(static, void, mln_lang_stack_handler_move, (mln_lang_ctx_t *ctx), (ctx), {
    mln_lang_stack_node_t *node = mln_lang_stack_top(ctx);
    mln_lang_move_t *tmp, *move = (mln_lang_move_t *)(node->pos);
    mln_lang_var_t *res = NULL;
    if (node->step == 0) {
        mln_lang_stack_node_reset_ret_val(node);
        mln_lang_ctx_reset_ret_var(ctx);
        if (move->op == M_MOVE_NONE) node->step = M_LANG_STEP_OUT;
        else node->step = 1;
        if (move->jump == NULL)
            mln_lang_generate_jump_ptr(move, M_LSNT_MOVE);
        if (move->type == M_LSNT_FACTOR && move->op == M_MOVE_NONE) {
            mln_lang_stack_node_free(mln_lang_stack_pop(ctx));
            mln_lang_stack_popuntil(ctx);
        }
        if ((node = mln_lang_stack_push(ctx, (mln_lang_stack_node_type_t)(move->type), move->jump)) == NULL) {
            __mln_lang_errmsg(ctx, "Stack is full.");
            ctx->quit = 1;
            return;
        }
        mln_lang_stack_map[node->type](ctx);
        return;
    } else if (node->step == 1) {
        node->step = 2;
        ASSERT(move->right != NULL);
        ASSERT(node->pos != NULL);
        mln_lang_stack_node_get_ctx_ret_var(node, ctx);
        goto goon2;
    } else if (node->step == 2) {
goon2:
        node->step = 3;
        tmp = move->right;
        if ((node = mln_lang_stack_push(ctx, M_LSNT_ADDSUB, tmp->left)) == NULL) {
            __mln_lang_errmsg(ctx, "Stack is full.");
            ctx->quit = 1;
            return;
        }
        mln_lang_stack_map[node->type](ctx);
        return;
    } else if (node->step == 3) {
        node->step = 4;
        mln_lang_op handler = NULL;
        mln_lang_method_t *method;
        method = mln_lang_methods[mln_lang_var_val_type_get(node->ret_var)];
        if (method == NULL) {
            __mln_lang_errmsg(ctx, "Operation NOT support.");
            ctx->quit = 1;
            return;
        }
        switch (move->op) {
            case M_MOVE_LMOVE:
                handler = method->lmov_handler;
                break;
            case M_MOVE_RMOVE:
                handler = method->rmov_handler;
                break;
            default:
                ASSERT(0);
                break;
        }
        if (handler == NULL) {
            __mln_lang_errmsg(ctx, "Operation NOT support.");
            ctx->quit = 1;
            return;
        }
        if (handler(ctx, &res, node->ret_var, ctx->ret_var) < 0) {
            ctx->quit = 1;
            return;
        }
        mln_lang_stack_node_get_ctx_ret_var(node, ctx);
        __mln_lang_ctx_set_ret_var(ctx, res);
        if (res->val->type == M_LANG_VAL_TYPE_CALL) {
again:
            node->call = 1;
            if (mln_lang_stack_handler_funccall_run(ctx, node, res->val->data.call) < 0) {
                ctx->quit = 1;
                return;
            }
        } else {
            goto goon4;
        }
    } else if (node->step == 4) {
        if (node->call) {
            if (mln_lang_withdraw_until_func(ctx) < 0) {
                ctx->quit = 1;
                return;
            }
            node->call = 0;
            res = ctx->ret_var;
            if (res->val->type == M_LANG_VAL_TYPE_CALL) goto again;
        }
goon4:
        node->pos = move->right;
        if (node->pos != NULL && move->right->op != M_MOVE_NONE) {
            node->step = 2;
            mln_lang_stack_node_get_ctx_ret_var(node, ctx);
        } else {
            mln_lang_stack_node_free(mln_lang_stack_pop(ctx));
            mln_lang_stack_popuntil(ctx);
        }
    } else {
        mln_lang_stack_node_free(mln_lang_stack_pop(ctx));
        mln_lang_stack_popuntil(ctx);
    }
})

MLN_FUNC_VOID(static, void, mln_lang_stack_handler_addsub, (mln_lang_ctx_t *ctx), (ctx), {
    mln_lang_stack_node_t *node = mln_lang_stack_top(ctx);
    mln_lang_addsub_t *tmp, *addsub = (mln_lang_addsub_t *)(node->pos);
    mln_lang_var_t *res = NULL;
    if (node->step == 0) {
        mln_lang_stack_node_reset_ret_val(node);
        mln_lang_ctx_reset_ret_var(ctx);
        if (addsub->op == M_ADDSUB_NONE) node->step = M_LANG_STEP_OUT;
        else node->step = 1;
        if (addsub->jump == NULL)
            mln_lang_generate_jump_ptr(addsub, M_LSNT_ADDSUB);
        if (addsub->type == M_LSNT_FACTOR && addsub->op == M_ADDSUB_NONE) {
            mln_lang_stack_node_free(mln_lang_stack_pop(ctx));
            mln_lang_stack_popuntil(ctx);
        }
        if ((node = mln_lang_stack_push(ctx, (mln_lang_stack_node_type_t)(addsub->type), addsub->jump)) == NULL) {
            __mln_lang_errmsg(ctx, "Stack is full.");
            ctx->quit = 1;
            return;
        }
        mln_lang_stack_map[node->type](ctx);
        return;
    } else if (node->step == 1) {
        node->step = 2;
        ASSERT(addsub->right != NULL);
        ASSERT(node->pos != NULL);
        mln_lang_stack_node_get_ctx_ret_var(node, ctx);
        goto goon2;
    } else if (node->step == 2) {
goon2:
        node->step = 3;
        tmp = addsub->right;
        if ((node = mln_lang_stack_push(ctx, M_LSNT_MULDIV, tmp->left)) == NULL) {
            __mln_lang_errmsg(ctx, "Stack is full.");
            ctx->quit = 1;
            return;
        }
        mln_lang_stack_map[node->type](ctx);
        return;
    } else if (node->step == 3) {
        node->step = 4;
        mln_lang_op handler = NULL;
        mln_lang_method_t *method;
        method = mln_lang_methods[mln_lang_var_val_type_get(node->ret_var)];
        if (method == NULL) {
            __mln_lang_errmsg(ctx, "Operation NOT support.");
            ctx->quit = 1;
            return;
        }
        switch (addsub->op) {
            case M_ADDSUB_PLUS:
                handler = method->plus_handler;
                break;
            case M_ADDSUB_SUB:
                handler = method->sub_handler;
                break;
            default:
                ASSERT(0);
                break;
        }
        if (handler == NULL) {
            __mln_lang_errmsg(ctx, "Operation NOT support.");
            ctx->quit = 1;
            return;
        }
        if (handler(ctx, &res, node->ret_var, ctx->ret_var) < 0) {
            ctx->quit = 1;
            return;
        }
        mln_lang_stack_node_get_ctx_ret_var(node, ctx);
        __mln_lang_ctx_set_ret_var(ctx, res);
        if (res->val->type == M_LANG_VAL_TYPE_CALL) {
again:
            node->call = 1;
            if (mln_lang_stack_handler_funccall_run(ctx, node, res->val->data.call) < 0) {
                ctx->quit = 1;
                return;
            }
        } else {
            goto goon4;
        }
    } else if (node->step == 4) {
        if (node->call) {
            if (mln_lang_withdraw_until_func(ctx) < 0) {
                ctx->quit = 1;
                return;
            }
            node->call = 0;
            res = ctx->ret_var;
            if (res->val->type == M_LANG_VAL_TYPE_CALL) goto again;
        }
goon4:
        node->pos = addsub->right;
        if (node->pos != NULL && addsub->right->op != M_ADDSUB_NONE) {
            node->step = 2;
            mln_lang_stack_node_get_ctx_ret_var(node, ctx);
        } else {
            mln_lang_stack_node_free(mln_lang_stack_pop(ctx));
            mln_lang_stack_popuntil(ctx);
        }
    } else {
        mln_lang_stack_node_free(mln_lang_stack_pop(ctx));
        mln_lang_stack_popuntil(ctx);
    }
})

MLN_FUNC_VOID(static, void, mln_lang_stack_handler_muldiv, (mln_lang_ctx_t *ctx), (ctx), {
    mln_lang_stack_node_t *node = mln_lang_stack_top(ctx);
    mln_lang_muldiv_t *tmp, *muldiv = (mln_lang_muldiv_t *)(node->pos);
    mln_lang_var_t *res = NULL;
    if (node->step == 0) {
        mln_lang_stack_node_reset_ret_val(node);
        mln_lang_ctx_reset_ret_var(ctx);
        if (muldiv->op == M_MULDIV_NONE) node->step = M_LANG_STEP_OUT;
        else node->step = 1;
        if (muldiv->jump == NULL)
            mln_lang_generate_jump_ptr(muldiv, M_LSNT_MULDIV);
        if (muldiv->type == M_LSNT_FACTOR && muldiv->op == M_MULDIV_NONE) {
            mln_lang_stack_node_free(mln_lang_stack_pop(ctx));
            mln_lang_stack_popuntil(ctx);
        }
        if ((node = mln_lang_stack_push(ctx, (mln_lang_stack_node_type_t)(muldiv->type), muldiv->jump)) == NULL) {
            __mln_lang_errmsg(ctx, "Stack is full.");
            ctx->quit = 1;
            return;
        }
        mln_lang_stack_map[node->type](ctx);
        return;
    } else if (node->step == 1) {
        node->step = 2;
        ASSERT(muldiv->right != NULL);
        ASSERT(node->pos != NULL);
        mln_lang_stack_node_get_ctx_ret_var(node, ctx);
        goto goon2;
    } else if (node->step == 2) {
goon2:
        node->step = 3;
        tmp = muldiv->right;
        if ((node = mln_lang_stack_push(ctx, M_LSNT_NOT, tmp->left)) == NULL) {
            __mln_lang_errmsg(ctx, "Stack is full.");
            ctx->quit = 1;
            return;
        }
        mln_lang_stack_map[node->type](ctx);
        return;
    } else if (node->step == 3) {
        node->step = 4;
        mln_lang_op handler = NULL;
        mln_lang_method_t *method;
        method = mln_lang_methods[mln_lang_var_val_type_get(node->ret_var)];
        if (method == NULL) {
            __mln_lang_errmsg(ctx, "Operation NOT support.");
            ctx->quit = 1;
            return;
        }
        switch (muldiv->op) {
            case M_MULDIV_MUL:
                handler = method->mul_handler;
                break;
            case M_MULDIV_DIV:
                handler = method->div_handler;
                break;
            case M_MULDIV_MOD:
                handler = method->mod_handler;
                break;
            default:
                ASSERT(0);
                break;
        }
        if (handler == NULL) {
            __mln_lang_errmsg(ctx, "Operation NOT support.");
            ctx->quit = 1;
            return;
        }
        if (handler(ctx, &res, node->ret_var, ctx->ret_var) < 0) {
            ctx->quit = 1;
            return;
        }
        mln_lang_stack_node_get_ctx_ret_var(node, ctx);
        __mln_lang_ctx_set_ret_var(ctx, res);
        if (res->val->type == M_LANG_VAL_TYPE_CALL) {
again:
            node->call = 1;
            if (mln_lang_stack_handler_funccall_run(ctx, node, res->val->data.call) < 0) {
                ctx->quit = 1;
                return;
            }
        } else {
            goto goon4;
        }
    } else if (node->step == 4) {
        if (node->call) {
            if (mln_lang_withdraw_until_func(ctx) < 0) {
                ctx->quit = 1;
                return;
            }
            node->call = 0;
            res = ctx->ret_var;
            if (res->val->type == M_LANG_VAL_TYPE_CALL) goto again;
        }
goon4:
        node->pos = muldiv->right;
        if (node->pos != NULL && muldiv->right->op != M_MULDIV_NONE) {
            node->step = 2;
            mln_lang_stack_node_get_ctx_ret_var(node, ctx);
        } else {
            mln_lang_stack_node_free(mln_lang_stack_pop(ctx));
            mln_lang_stack_popuntil(ctx);
        }
    } else {
        mln_lang_stack_node_free(mln_lang_stack_pop(ctx));
        mln_lang_stack_popuntil(ctx);
    }
})

MLN_FUNC_VOID(static, void, mln_lang_stack_handler_not, (mln_lang_ctx_t *ctx), (ctx), {
    mln_lang_var_t *res = NULL;
    mln_lang_stack_node_t *node = mln_lang_stack_top(ctx);
    mln_lang_not_t *not = node->data.not;

    if (node->step == 0) {
        mln_lang_stack_node_reset_ret_val(node);
        mln_lang_ctx_reset_ret_var(ctx);
        node->step = not->op == M_NOT_NONE? M_LANG_STEP_OUT: 1;
        if (not->jump == NULL)
            mln_lang_generate_jump_ptr(not, M_LSNT_NOT);
        if ((node = mln_lang_stack_push(ctx, (mln_lang_stack_node_type_t)(not->type), not->jump)) == NULL) {
            __mln_lang_errmsg(ctx, "Stack is full.");
            ctx->quit = 1;
            return;
        }
        mln_lang_stack_map[node->type](ctx);
        return;
    } else if (node->step == 1) {
        node->step = 2;
        mln_lang_op handler = NULL;
        mln_lang_method_t *method;
        method = mln_lang_methods[mln_lang_var_val_type_get(ctx->ret_var)];
        if (method == NULL) {
            __mln_lang_errmsg(ctx, "Operation NOT support.");
            ctx->quit = 1;
            return;
        }
        switch (not->op) {
            case M_NOT_NOT:
                handler = method->not_handler;
                break;
            default:
                break;
        }
        if (handler != NULL) {
            if (handler(ctx, &res, ctx->ret_var, NULL) < 0) {
                ctx->quit = 1;
                return;
            }
            __mln_lang_ctx_set_ret_var(ctx, res);
            if (res->val->type == M_LANG_VAL_TYPE_CALL) {
                node->call = 1;
again:
                if (mln_lang_stack_handler_funccall_run(ctx, node, res->val->data.call) < 0) {
                    ctx->quit = 1;
                    return;
                }
            } else {
                goto out;
            }
        }
    } else if (node->step == 2) {
        if (node->call) {
            if (mln_lang_withdraw_until_func(ctx) < 0) {
                ctx->quit = 1;
                return;
            }
            res = ctx->ret_var;
            if (ctx->ret_var->val->type == M_LANG_VAL_TYPE_CALL) goto again;
        }
        goto out;
    } else {
out:
        mln_lang_stack_node_free(mln_lang_stack_pop(ctx));
        mln_lang_stack_popuntil(ctx);
    }
})

MLN_FUNC_VOID(static, void, mln_lang_stack_handler_suffix, (mln_lang_ctx_t *ctx), (ctx), {
    mln_lang_var_t *res = NULL;
    mln_lang_stack_node_t *node = mln_lang_stack_top(ctx);
    mln_lang_suffix_t *suffix = node->data.suffix;
    if (node->step == 0) {
        mln_lang_stack_node_reset_ret_val(node);
        mln_lang_ctx_reset_ret_var(ctx);
        node->step = suffix->op == M_SUFFIX_NONE? M_LANG_STEP_OUT: 1;
        if (suffix->jump == NULL)
            mln_lang_generate_jump_ptr(suffix, M_LSNT_SUFFIX);
        if (suffix->type == M_LSNT_FACTOR && suffix->op == M_SUFFIX_NONE) {
            mln_lang_stack_node_free(mln_lang_stack_pop(ctx));
            mln_lang_stack_popuntil(ctx);
        }
        if ((node = mln_lang_stack_push(ctx, (mln_lang_stack_node_type_t)(suffix->type), suffix->jump)) == NULL) {
            __mln_lang_errmsg(ctx, "Stack is full.");
            ctx->quit = 1;
            return;
        }
        mln_lang_stack_map[node->type](ctx);
        return;
    } else if (node->step == 1) {
        node->step = 2;
        mln_lang_op handler = NULL;
        mln_lang_method_t *method;
        method = mln_lang_methods[mln_lang_var_val_type_get(ctx->ret_var)];
        if (method == NULL) {
            __mln_lang_errmsg(ctx, "Operation NOT support.");
            ctx->quit = 1;
            return;
        }
        switch (suffix->op) {
            case M_SUFFIX_INC:
                handler = method->sinc_handler;
                break;
            case M_SUFFIX_DEC:
                handler = method->sdec_handler;
                break;
            default:
                break;
        }
        if (handler != NULL) {
            if (mln_lang_val_not_modify_isset(ctx->ret_var->val)) {
                __mln_lang_errmsg(ctx, "Operand cannot be changed.");
                ctx->quit = 1;
                return;
            }
            if (handler(ctx, &res, ctx->ret_var, NULL) < 0) {
                ctx->quit = 1;
                return;
            }
            mln_lang_stack_node_get_ctx_ret_var(node, ctx);
            __mln_lang_ctx_set_ret_var(ctx, res);
            if (res->val->type == M_LANG_VAL_TYPE_CALL) {
                node->call = 1;
again:
                if (mln_lang_stack_handler_funccall_run(ctx, node, res->val->data.call) < 0) {
                    ctx->quit = 1;
                    return;
                }
            } else {
                goto goon2;
            }
        } else if (suffix->op != M_SUFFIX_NONE) {
            __mln_lang_errmsg(ctx, "Operation NOT support.");
            ctx->quit = 1;
            return;
        }
    } else if (node->step == 2) {
        if (node->call) {
            if (mln_lang_withdraw_until_func(ctx) < 0) {
                ctx->quit = 1;
                return;
            }
            res = ctx->ret_var;
            if (ctx->ret_var->val->type == M_LANG_VAL_TYPE_CALL) goto again;
        }
goon2:
        node->step = 3;
        if (node->ret_var != NULL && node->ret_var->val->func != NULL) {
            if (mln_lang_watch_func_build(ctx, \
                                         node, \
                                         node->ret_var->val->func, \
                                         node->ret_var->val->udata, \
                                         node->ret_var->val) < 0)
            {
                ctx->quit = 1;
                return;
            }
            mln_lang_stack_node_get_ctx_ret_var(node, ctx);
            mln_lang_ctx_reset_ret_var(ctx);
        } else {
            goto out;
        }
    } else if (node->step == 3) {
        if (node->call) {
            if (mln_lang_withdraw_until_func(ctx) < 0) {
                ctx->quit = 1;
                return;
            }
            node->call = 0;
        }
        mln_lang_ctx_get_node_ret_val(ctx, node);
        mln_lang_stack_node_free(mln_lang_stack_pop(ctx));
        mln_lang_stack_popuntil(ctx);
    } else {
out:
        mln_lang_stack_node_free(mln_lang_stack_pop(ctx));
        mln_lang_stack_popuntil(ctx);
    }
})

MLN_FUNC_VOID(static, void, mln_lang_stack_handler_locate, (mln_lang_ctx_t *ctx), (ctx), {
    mln_lang_var_t *res = NULL;
    mln_lang_stack_node_t *cur, *node = mln_lang_stack_top(ctx), tmp;
    mln_lang_locate_t *locate = node->data.locate;
    if (node->step == 0) {
        mln_lang_stack_node_reset_ret_val(node);
        mln_lang_ctx_reset_ret_var(ctx);
        switch (locate->op) {
            case M_LOCATE_NONE:
                node->step = M_LANG_STEP_OUT;
                break;
            case M_LOCATE_PROPERTY:
                node->step = 4;
                break;
            case M_LOCATE_INDEX:
                node->step = 1;
                break;
            default:
                node->step = 5;
                break;
        }
        if (locate->jump == NULL)
            mln_lang_generate_jump_ptr(locate, M_LSNT_LOCATE);
        if (locate->type == M_LSNT_FACTOR && locate->op == M_LOCATE_NONE) {
            mln_lang_stack_node_free(mln_lang_stack_pop(ctx));
            mln_lang_stack_popuntil(ctx);
        }
        if ((node = mln_lang_stack_push(ctx, (mln_lang_stack_node_type_t)(locate->type), locate->jump)) == NULL) {
            __mln_lang_errmsg(ctx, "Stack is full.");
            ctx->quit = 1;
            return;
        }
        mln_lang_stack_map[node->type](ctx);
        return;
    } else if (node->step == 1) {
        node->step = 2;
        ASSERT(locate->op == M_LOCATE_INDEX);
        mln_lang_stack_node_get_ctx_ret_var(node, ctx);
        if (locate->right.exp == NULL) {
            mln_lang_var_t *ret_var;
            if ((ret_var = __mln_lang_var_create_nil(ctx, NULL)) == NULL) {
                __mln_lang_errmsg(ctx, "No memory.");
                ctx->quit = 1;
                return;
            }
            __mln_lang_ctx_set_ret_var(ctx, ret_var);
        } else {
            if ((node = mln_lang_stack_push(ctx, M_LSNT_EXP, locate->right.exp)) == NULL) {
                __mln_lang_errmsg(ctx, "Stack is full.");
                ctx->quit = 1;
                return;
            }
            mln_lang_stack_map[node->type](ctx);
            return;
        }
    } else if (node->step == 2) {
        mln_lang_method_t *method;
        method = mln_lang_methods[mln_lang_var_val_type_get(node->ret_var)];
        if (method == NULL) {
            __mln_lang_errmsg(ctx, "Operation NOT support.");
            ctx->quit = 1;
            return;
        }
        mln_lang_op handler = method->index_handler;
        if (handler == NULL) {
            __mln_lang_errmsg(ctx, "Operation NOT support.");
            ctx->quit = 1;
            return;
        }
        if (handler(ctx, &res, node->ret_var, ctx->ret_var) < 0) {
            ctx->quit = 1;
            return;
        }
        __mln_lang_ctx_set_ret_var(ctx, res);
        if (res->val->type == M_LANG_VAL_TYPE_CALL) {
again_index:
            node->call = 1;
            node->step = 3;
            if (mln_lang_stack_handler_funccall_run(ctx, node, res->val->data.call) < 0) {
                ctx->quit = 1;
                return;
            }
        } else {
            node->step = 7;
            goto goon7;
        }
    } else if (node->step == 3) {
        if (node->call) {
            if (mln_lang_withdraw_until_func(ctx) < 0) {
                ctx->quit = 1;
                return;
            }
            node->call = 0;
            res = ctx->ret_var;
            if (ctx->ret_var->val->type == M_LANG_VAL_TYPE_CALL) goto again_index;
        }
        node->step = 7;
        goto goon7;
    } else if (node->step == 4) {
        ASSERT(locate->op == M_LOCATE_PROPERTY);
        if (node->ret_var2 != NULL) {
            __mln_lang_var_free(node->ret_var2);
            node->ret_var2 = NULL;
        }
        node->ret_var2 = ctx->ret_var;
        mln_lang_method_t *method;
        method = mln_lang_methods[mln_lang_var_val_type_get(ctx->ret_var)];
        if (method == NULL || method->property_handler == NULL) {
            __mln_lang_errmsg(ctx, "Operation NOT support.");
            node->ret_var2 = NULL;
            ctx->quit = 1;
            return;
        }
        mln_lang_var_t *res = NULL;
        mln_lang_var_t *var;
        if ((var = mln_lang_var_create_string(ctx, locate->right.id, NULL)) == NULL) {
            __mln_lang_errmsg(ctx, "No memory.");
            node->ret_var2 = NULL;
            ctx->quit = 1;
            return;
        }
        if (method->property_handler(ctx, &res, ctx->ret_var, var) < 0) {
            __mln_lang_var_free(var);
            node->ret_var2 = NULL;
            ctx->quit = 1;
            return;
        }
        __mln_lang_var_free(var);
        ctx->ret_var = NULL;
        __mln_lang_ctx_set_ret_var(ctx, res);
        if (res->val->type == M_LANG_VAL_TYPE_CALL) {
again_property:
            res = ctx->ret_var;
            node->call = 1;
            node->step = 6;
            if (mln_lang_stack_handler_funccall_run(ctx, node, res->val->data.call) < 0) {
                ctx->quit = 1;
                return;
            }
        } else {
            node->step = 7;
            goto goon7;
        }
    } else if (node->step == 5) {
        mln_lang_var_t *ret_var;
        mln_lang_funccall_val_t *funccall;
        if (mln_lang_var_val_type_get(ctx->ret_var) != M_LANG_VAL_TYPE_FUNC) {
            mln_string_t *s = NULL;
            if (mln_lang_var_val_type_get(ctx->ret_var) == M_LANG_VAL_TYPE_STRING) {
                s = mln_lang_var_val_get(ctx->ret_var)->data.s;
            } else if (mln_lang_var_val_type_get(ctx->ret_var) == M_LANG_VAL_TYPE_NIL && ctx->ret_var->name != NULL) {
                s = ctx->ret_var->name;
            } else {
                __mln_lang_errmsg(ctx, "Operation NOT support.");
                ctx->quit = 1;
                return;
            }
            if ((funccall = __mln_lang_funccall_val_new(ctx->pool, s)) == NULL) {
                __mln_lang_errmsg(ctx, "No memory.");
                ctx->quit = 1;
                return;
            }
            funccall->prototype = NULL;
            if (node->ret_var2 != NULL) {
                mln_lang_funccall_val_object_add(funccall, mln_lang_var_val_get(node->ret_var2));
                __mln_lang_var_free(node->ret_var2);
                node->ret_var2 = NULL;
            }
            if ((ret_var = __mln_lang_var_create_call(ctx, funccall)) == NULL) {
                __mln_lang_errmsg(ctx, "No memory.");
                __mln_lang_funccall_val_free(funccall);
                ctx->quit = 1;
                return;
            }
            node->ret_var = ret_var;
            mln_lang_ctx_reset_ret_var(ctx);
        } else {
            if ((funccall = __mln_lang_funccall_val_new(ctx->pool, ctx->ret_var->name)) == NULL) {
                __mln_lang_errmsg(ctx, "No memory.");
                ctx->quit = 1;
                return;
            }
            funccall->prototype = ctx->ret_var->val->data.func;
            if (node->ret_var2 != NULL) {
                mln_lang_funccall_val_object_add(funccall, mln_lang_var_val_get(node->ret_var2));
                __mln_lang_var_free(node->ret_var2);
                node->ret_var2 = NULL;
            }
            if ((ret_var = __mln_lang_var_create_call(ctx, funccall)) == NULL) {
                __mln_lang_errmsg(ctx, "No memory.");
                __mln_lang_funccall_val_free(funccall);
                ctx->quit = 1;
                return;
            }
            node->ret_var = ret_var;
            mln_lang_ctx_reset_ret_var(ctx);
        }
        node->pos = locate->right.exp;
        node->step = 8;
    } else if (node->step == 8) {
        if (ctx->ret_var != NULL) {
            mln_lang_funccall_val_add_arg(node->ret_var->val->data.call, mln_lang_var_ref(ctx->ret_var));
            mln_lang_ctx_reset_ret_var(ctx);
        }
        ASSERT(ctx->ret_var == NULL);
        if (node->pos != NULL) {
            mln_lang_exp_t *exp = (mln_lang_exp_t *)(node->pos);
            node->pos = exp->next;
            if ((node = mln_lang_stack_push(ctx, M_LSNT_ASSIGN, exp->assign)) == NULL) {
                __mln_lang_errmsg(ctx, "Stack is full.");
                ctx->quit = 1;
                return;
            }
            mln_lang_stack_map[node->type](ctx);
            return;
        } else {
            node->call = 1;
            node->step = 6;
            if (mln_lang_stack_handler_funccall_run(ctx, node, node->ret_var->val->data.call) < 0) {
                ctx->quit = 1;
                return;
            }
        }
    } else if (node->step == 6) {
        if (node->call) {
            if (mln_lang_withdraw_until_func(ctx) < 0) {
                ctx->quit = 1;
                return;
            }
            node->call = 0;
            res = ctx->ret_var;
            if (ctx->ret_var->val->type == M_LANG_VAL_TYPE_CALL) goto again_property;
        }
        node->step = 7;
        goto goon7;
    } else if (node->step == 7) {
goon7:
        cur = mln_lang_stack_pop(ctx);
        tmp = *cur;
        if (locate->next != NULL) {
            if ((node = mln_lang_stack_push(ctx, M_LSNT_LOCATE, locate->next)) == NULL) {
                __mln_lang_errmsg(ctx, "Stack is full.");
                mln_lang_stack_node_free(&tmp);
                ctx->quit = 1;
                return;
            }
            if (locate->next->op == M_LOCATE_NONE) node->step = 7;
            else if (locate->next->op == M_LOCATE_PROPERTY) node->step = 4;
            else if (locate->next->op == M_LOCATE_INDEX) node->step = 1;
            else {
                node->step = 5;
                if (tmp.ret_var2 != NULL) {
                    node->ret_var2 = tmp.ret_var2;
                    tmp.ret_var2 = NULL;
                }
            }
        } else {
            node = NULL;
        }
        mln_lang_stack_node_free(&tmp);
        if (node != NULL) {
            mln_lang_stack_map[node->type](ctx);
            return;
        }
    } else {
        mln_lang_stack_node_free(mln_lang_stack_pop(ctx));
        mln_lang_stack_popuntil(ctx);
    }
})

static inline int
mln_lang_stack_handler_funccall_run(mln_lang_ctx_t *ctx, mln_lang_stack_node_t *node, mln_lang_funccall_val_t *funccall)
{
    mln_lang_scope_t *scope;
    mln_lang_func_detail_t *prototype = funccall->prototype;
    mln_lang_var_t *var = NULL, *newvar, *scan;
    mln_string_t _this = mln_string("this");
    mln_size_t i, j, n, m;
    mln_s32_t type;
    mln_string_t *funcname = funccall->name;
    mln_lang_array_t *args_array;
    mln_lang_symbol_node_t *sym;

    if (prototype == NULL) {
        if (funccall->object == NULL) {
            while (1) {
                sym = __mln_lang_symbol_node_search(ctx, funcname, 0);
                if (sym == NULL || sym->type != M_LANG_SYMBOL_VAR) {
                    __mln_lang_errmsg(ctx, "Invalid function, No such prototype.");
                    return -1;
                }
                var = sym->data.var;
                ASSERT(sym->data.var != NULL);
                if ((type = mln_lang_var_val_type_get(var)) == M_LANG_VAL_TYPE_FUNC) {
                    prototype = funccall->prototype = var->val->data.func;
                    break;
                } else if (type != M_LANG_VAL_TYPE_STRING) {
                    __mln_lang_errmsg(ctx, "Invalid function, No such prototype.");
                    return -1;
                }
                funcname = var->val->data.s;
            }
        } else {/*maybe never get in, not support b.a="foo";b.a() to call b.foo*/
            while (1) {
                if ((var = __mln_lang_set_member_search(funccall->object->data.obj->members, \
                                                        funcname)) == NULL)
                {
                    sym = __mln_lang_symbol_node_search(ctx, funcname, 0);
                    if (sym == NULL || sym->type != M_LANG_SYMBOL_VAR) {
                        __mln_lang_errmsg(ctx, "Invalid function, No such prototype.");
                        return -1;
                    }
                    ASSERT(sym->data.var != NULL);
                    if (mln_lang_var_val_type_get(sym->data.var) != M_LANG_VAL_TYPE_STRING) {
                        __mln_lang_errmsg(ctx, "Invalid function, No such prototype.");
                        return -1;
                    }
                    funcname = sym->data.var->val->data.s;
                    continue;
                }
                if ((type = mln_lang_var_val_type_get(var)) == M_LANG_VAL_TYPE_FUNC) {
                    prototype = funccall->prototype = var->val->data.func;
                    break;
                } else if (type != M_LANG_VAL_TYPE_STRING) {
                    __mln_lang_errmsg(ctx, "Invalid function, No such prototype.");
                    return -1;
                }
                funcname = var->val->data.s;
            }
        }
    }

    if ((scope = mln_lang_scope_push(ctx, funcname, M_LANG_SCOPE_TYPE_FUNC, node, NULL)) == NULL) {
        __mln_lang_errmsg(ctx, "Scope stack is full.");
        return -1;
    }

    if (funccall->object != NULL) {
        if ((newvar = __mln_lang_var_new(ctx, &_this, M_LANG_VAR_NORMAL, funccall->object, NULL)) == NULL) {
            __mln_lang_errmsg(ctx, "No memory.");
            return -1;
        }
        if (__mln_lang_symbol_node_join(ctx, M_LANG_SYMBOL_VAR, newvar) < 0) {
            __mln_lang_var_free(newvar);
            return -1;
        }
    }
    n = mln_array_nelts(&(prototype->args));
    m = mln_array_nelts(&(funccall->args));
    for (i = 0, j = 0; i < n; ++i) {
        scan = ((mln_lang_var_t **)mln_array_elts(&(prototype->args)))[i];
        var = j >= m? NULL: (((mln_lang_var_t **)mln_array_elts(&funccall->args))[j]);

        if (var == NULL) {
            if ((newvar = __mln_lang_var_create_nil(ctx, scan->name)) == NULL) {
                __mln_lang_errmsg(ctx, "No memory.");
                return -1;
            }
        } else {
            if (!var->ref) {/* name must be NULL*/
                newvar = mln_lang_var_ref(var);
                newvar->name = mln_string_ref(scan->name);
            } else {
                if ((newvar = mln_lang_var_transform(ctx, var, scan)) == NULL) {
                    __mln_lang_errmsg(ctx, "No memory.");
                    return -1;
                }
            }
            ++j;
        }
        if (__mln_lang_symbol_node_join(ctx, M_LANG_SYMBOL_VAR, newvar) < 0) {
            __mln_lang_var_free(newvar);
            return -1;
        }
    }
    if (j < m) {
        if ((args_array = mln_lang_funccall_run_build_args(ctx)) == NULL) {
            __mln_lang_errmsg(ctx, "No memory.");
            return -1;
        }
        for (; j < m; ++j) {
            var = ((mln_lang_var_t **)mln_array_elts(&funccall->args))[j];
            if (mln_lang_funcall_run_add_args(ctx, args_array, var) < 0) {
                __mln_lang_errmsg(ctx, "No memory.");
                return -1;
            }
        }
    }
    n = mln_array_nelts(&(prototype->closure));
    for (i = 0; i < n; ++i) {
        scan = ((mln_lang_var_t **)mln_array_elts(&(prototype->closure)))[i];
        newvar = mln_lang_var_ref(scan);
        if (__mln_lang_symbol_node_join(ctx, M_LANG_SYMBOL_VAR, newvar) < 0) {
            __mln_lang_var_free(newvar);
            return -1;
        }
    }
    mln_lang_ctx_reset_ret_var(ctx);
    if (prototype->type == M_FUNC_INTERNAL) {
        if (prototype->data.process == NULL) {
            __mln_lang_errmsg(ctx, "Not implemented.");
            return -1;
        }
#if !defined(MSVC)
        pthread_mutex_lock(&ctx->lang->lock);
#endif
        if ((var = prototype->data.process(ctx)) == NULL) {
#if !defined(MSVC)
            pthread_mutex_unlock(&ctx->lang->lock);
#endif
            return -1;
        }
        __mln_lang_ctx_set_ret_var(ctx, var);
#if !defined(MSVC)
        pthread_mutex_unlock(&ctx->lang->lock);
#endif
    } else {
        if (prototype->data.stm != NULL) {
            scope->entry = prototype->data.stm;
            if ((node = mln_lang_stack_push(ctx, M_LSNT_STM, prototype->data.stm)) == NULL) {
                __mln_lang_errmsg(ctx, "Stack is full.");
                return -1;
            }
        }
    }
    return 0;
}

MLN_FUNC(static inline, int, mln_lang_funcall_run_add_args, \
         (mln_lang_ctx_t *ctx, mln_lang_array_t *array, mln_lang_var_t *arg), \
         (ctx, array, arg), \
{
    mln_lang_var_t *var;
    if ((var = __mln_lang_array_get(ctx, array, NULL)) == NULL) {
        return -1;
    }
    if (mln_lang_var_value_set_string_ref(ctx, var, arg) < 0) {
        return -1;
    }
    return 0;
})

MLN_FUNC(static inline, mln_lang_array_t *, mln_lang_funccall_run_build_args, \
         (mln_lang_ctx_t *ctx), (ctx), \
{
    mln_string_t name = mln_string("args");
    mln_lang_array_t *array;
    mln_lang_val_t *val;
    mln_lang_var_t *var;

    if ((array = __mln_lang_array_new(ctx)) == NULL) {
        return NULL;
    }
    if ((val = __mln_lang_val_new(ctx, M_LANG_VAL_TYPE_ARRAY, array)) == NULL) {
        __mln_lang_array_free(array);
        return NULL;
    }
    if ((var = __mln_lang_var_new(ctx, &name, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        __mln_lang_val_free(val);
        return NULL;
    }
    if (__mln_lang_symbol_node_join(ctx, M_LANG_SYMBOL_VAR, var) < 0) {
        __mln_lang_var_free(var);
        return NULL;
    }
    return array;
})

MLN_FUNC_VOID(static, void, mln_lang_stack_handler_spec, (mln_lang_ctx_t *ctx), (ctx), {
    mln_lang_stack_node_t *node = mln_lang_stack_top(ctx);
    mln_lang_spec_t *spec = node->data.spec;
    mln_lang_var_t *ret_var;
    if (spec->op == M_SPEC_REFER) {
        __mln_lang_errmsg(ctx, "'&' Not allowed.");
        ctx->quit = 1;
        return;
    }
    if (node->step == 0) {
        node->step = 1;
        mln_u8ptr_t data = NULL;
        mln_lang_stack_node_type_t type;
        mln_lang_stack_node_reset_ret_val(node);
        mln_lang_ctx_reset_ret_var(ctx);
        switch (spec->op) {
            case M_SPEC_NEGATIVE:
            case M_SPEC_REVERSE:
            case M_SPEC_REFER:
            case M_SPEC_INC:
            case M_SPEC_DEC:
                data = (mln_u8ptr_t)(spec->data.spec);
                type = M_LSNT_SPEC;
                break;
            case M_SPEC_NEW:
                if (mln_lang_stack_handler_spec_new(ctx, spec->data.set_name) < 0) {
                    ctx->quit = 1;
                    return;
                }
                node->step = 2;
                break;
            case M_SPEC_PARENTH:
                if (spec->data.exp == NULL) {
                    if ((ret_var = __mln_lang_var_create_nil(ctx, NULL)) == NULL) {
                        __mln_lang_errmsg(ctx, "No memory.");
                        ctx->quit = 1;
                        return;
                    }
                } else {
                    data = (mln_u8ptr_t)(spec->data.exp);
                    type = M_LSNT_EXP;
                }
                node->step = 2;
                break;
            default: /* M_SPEC_FACTOR */
                data = (mln_u8ptr_t)(spec->data.factor);
                type = M_LSNT_FACTOR;
                node->step = M_LANG_STEP_OUT;
                break;
        }
        if (data != NULL) {
            if (spec->op == M_SPEC_FACTOR) {
                mln_lang_stack_node_free(mln_lang_stack_pop(ctx));
                mln_lang_stack_popuntil(ctx);
            }
            if ((node = mln_lang_stack_push(ctx, type, data)) == NULL) {
                __mln_lang_errmsg(ctx, "Stack is full.");
                ctx->quit = 1;
                return;
            }
            mln_lang_stack_map[node->type](ctx);
            return;
        } else {
            ASSERT(spec->op == M_SPEC_NEW || spec->op == M_SPEC_PARENTH);
        }
    } else if (node->step == 1) {
        node->step = 2;
        ret_var = ctx->ret_var;
        ASSERT(ret_var != NULL);
        if (ret_var->val->type == M_LANG_VAL_TYPE_CALL) {
again:
            node->call = 1;
            if (mln_lang_stack_handler_funccall_run(ctx, node, ret_var->val->data.call) < 0) {
                ctx->quit = 1;
                return;
            }
        } else {
            mln_lang_op handler = NULL;
            mln_lang_method_t *method;
            method = mln_lang_methods[mln_lang_var_val_type_get(ret_var)];
            if (method == NULL) {
                __mln_lang_errmsg(ctx, "Operation NOT support.");
                ctx->quit = 1;
                return;
            }
            switch (spec->op) {
                case M_SPEC_NEGATIVE:
                    handler = method->negative_handler;
                    break;
                case M_SPEC_REVERSE:
                    handler = method->reverse_handler;
                    break;
                case M_SPEC_INC:
                    handler = method->pinc_handler;
                    break;
                case M_SPEC_DEC:
                    handler = method->pdec_handler;
                    break;
                default:
                    ASSERT(0);
                    break;
            }
            if (handler == NULL) {
                __mln_lang_errmsg(ctx, "Operation NOT support.");
                ctx->quit = 1;
                return;
            }
            ret_var = NULL;
            if ((spec->op == M_SPEC_INC || spec->op == M_SPEC_DEC) && mln_lang_val_not_modify_isset(ctx->ret_var->val)) {
                __mln_lang_errmsg(ctx, "Operand cannot be changed.");
                ctx->quit = 1;
                return;
            }
            if (handler(ctx, &ret_var, ctx->ret_var, NULL) < 0) {
                ctx->quit = 1;
                return;
            }
            mln_lang_stack_node_get_ctx_ret_var(node, ctx);
            __mln_lang_ctx_set_ret_var(ctx, ret_var);
            if (ret_var->val->type == M_LANG_VAL_TYPE_CALL) {
                goto again;
            } else {
                goto goon2;
            }
        }
    } else if (node->step == 2) {
        if (node->call) {
            if (mln_lang_withdraw_until_func(ctx) < 0) {
                ctx->quit = 1;
                return;
            }
            node->call = 0;
            ret_var = ctx->ret_var;
            if (ctx->ret_var->val->type == M_LANG_VAL_TYPE_CALL) goto again;
        }
goon2:
        node->step = 3;
        if ((spec->op == M_SPEC_INC || spec->op == M_SPEC_DEC) && \
            node->ret_var != NULL && \
            node->ret_var->val->func != NULL)
        {
            if (mln_lang_watch_func_build(ctx, \
                                         node, \
                                         node->ret_var->val->func, \
                                         node->ret_var->val->udata, \
                                         node->ret_var->val) < 0)
            {
                ctx->quit = 1;
                return;
            }
            mln_lang_stack_node_get_ctx_ret_var(node, ctx);
            mln_lang_ctx_reset_ret_var(ctx);
        } else {
            goto out;
        }
    } else if (node->step == 3) {
        if (node->call) {
            if (mln_lang_withdraw_until_func(ctx) < 0) {
                ctx->quit = 1;
                return;
            }
            node->call = 0;
        }
        mln_lang_ctx_get_node_ret_val(ctx, node);
        mln_lang_stack_node_free(mln_lang_stack_pop(ctx));
        mln_lang_stack_popuntil(ctx);
    } else {
out:
        mln_lang_stack_node_free(mln_lang_stack_pop(ctx));
        mln_lang_stack_popuntil(ctx);
    }
})

MLN_FUNC(static inline, int, mln_lang_stack_handler_spec_new, \
         (mln_lang_ctx_t *ctx, mln_string_t *name), (ctx, name), \
{
    mln_lang_var_t *var;
    mln_lang_val_t *val;
    mln_lang_object_t *obj;
    mln_lang_symbol_node_t *sym;

again:
    sym = __mln_lang_symbol_node_search(ctx, name, 0);
    if (sym == NULL) {
        sym = mln_lang_symbol_node_id_search(ctx, name);
        if (sym == NULL) goto err;
        if (sym->type != M_LANG_SYMBOL_VAR) goto err;
        if (mln_lang_var_val_type_get(sym->data.var) != M_LANG_VAL_TYPE_STRING) goto err;
        if ((name = sym->data.var->val->data.s) == NULL) goto err;
        goto again;
err:
        if ((var = __mln_lang_var_create_nil(ctx, NULL)) == NULL) {
            __mln_lang_errmsg(ctx, "No memory.");
            return -1;
        }
        __mln_lang_ctx_set_ret_var(ctx, var);
        return 0;
    } else {
        if (sym->type == M_LANG_SYMBOL_SET) goto goon;
        sym = mln_lang_symbol_node_id_search(ctx, name);
        if (sym == NULL) goto err;
        if (sym->type != M_LANG_SYMBOL_VAR) goto err;
        if (mln_lang_var_val_type_get(sym->data.var) != M_LANG_VAL_TYPE_STRING) goto err;
        if ((name = sym->data.var->val->data.s) == NULL) goto err;
        goto again;
    }

goon:
    if ((obj = mln_lang_object_new(ctx, sym->data.set)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if ((val = __mln_lang_val_new(ctx, M_LANG_VAL_TYPE_OBJECT, obj)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        mln_lang_object_free(obj);
        return -1;
    }
    if ((var = __mln_lang_var_new(ctx, NULL, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        __mln_lang_val_free(val);
        return -1;
    }
    __mln_lang_ctx_set_ret_var(ctx, var);
    return 0;
})

MLN_FUNC_VOID(static, void, mln_lang_stack_handler_factor, (mln_lang_ctx_t *ctx), (ctx), {
    mln_lang_stack_node_t *node = mln_lang_stack_top(ctx);
    mln_lang_factor_t *factor = node->data.factor;
    mln_string_t *name;
    mln_lang_var_t *var;
    if (node->step == 0) {
        node->step = 1;
        switch (factor->type) {
            case M_FACTOR_ID:
            {
                mln_lang_symbol_node_t *sym;
                if ((sym = mln_lang_symbol_node_id_search(ctx, factor->data.s_id)) != NULL) {
                    if (sym->type == M_LANG_SYMBOL_VAR) {
                        __mln_lang_ctx_set_ret_var(ctx, mln_lang_var_ref(sym->data.var));
                    } else {/*M_LANG_SYMBOL_SET*/
                        __mln_lang_errmsg(ctx, "Invalid token. Token is a SET name, not a value or function.");
                        ctx->quit = 1;
                        return;
                    }
                } else {
                    if ((name = mln_string_pool_dup(ctx->pool, factor->data.s_id)) == NULL) {
                        __mln_lang_errmsg(ctx, "No memory.");
                        ctx->quit = 1;
                        return;
                    }
                    if ((var = __mln_lang_var_create_nil(ctx, name)) == NULL) {
                        __mln_lang_errmsg(ctx, "No memory.");
                        mln_string_free(name);
                        ctx->quit = 1;
                        return;
                    }
                    mln_string_free(name);
                    if (__mln_lang_symbol_node_join(ctx, M_LANG_SYMBOL_VAR, var) < 0) {
                        __mln_lang_errmsg(ctx, "No memory.");
                        __mln_lang_var_free(var);
                        ctx->quit = 1;
                        return;
                    }
                    __mln_lang_ctx_set_ret_var(ctx, mln_lang_var_ref(var));
                }
                break;
            }
            case M_FACTOR_INT:
                if ((var = __mln_lang_var_create_int(ctx, factor->data.i, NULL)) == NULL) {
                    __mln_lang_errmsg(ctx, "No memory.");
                    ctx->quit = 1;
                    return;
                }
                __mln_lang_ctx_set_ret_var(ctx, var);
                break;
            case M_FACTOR_BOOL:
                if ((var = __mln_lang_var_create_bool(ctx, factor->data.b, NULL)) == NULL) {
                    __mln_lang_errmsg(ctx, "No memory.");
                    ctx->quit = 1;
                    return;
                }
                __mln_lang_ctx_set_ret_var(ctx, var);
                break;
            case M_FACTOR_STRING:
                if ((var = mln_lang_var_create_string(ctx, factor->data.s_id, NULL)) == NULL) {
                    __mln_lang_errmsg(ctx, "No memory.");
                    ctx->quit = 1;
                    return;
                }
                __mln_lang_ctx_set_ret_var(ctx, var);
                break;
            case M_FACTOR_REAL:
                if ((var = __mln_lang_var_create_real(ctx, factor->data.f, NULL)) == NULL) {
                    __mln_lang_errmsg(ctx, "No memory.");
                    ctx->quit = 1;
                    return;
                }
                __mln_lang_ctx_set_ret_var(ctx, var);
                break;
            case M_FACTOR_ARRAY:
                if ((var = __mln_lang_var_create_array(ctx, NULL)) == NULL) {
                    __mln_lang_errmsg(ctx, "No memory.");
                    ctx->quit = 1;
                    return;
                }
                mln_lang_stack_node_set_ret_var(node, var);
                if (factor->data.array != NULL) {
                    if ((node = mln_lang_stack_push(ctx, M_LSNT_ELEMLIST, factor->data.array)) == NULL) {
                        __mln_lang_errmsg(ctx, "Stack is full.");
                        ctx->quit = 1;
                        return;
                    }
                }
                break;
            default:
                if ((var = __mln_lang_var_create_nil(ctx, NULL)) == NULL) {
                    __mln_lang_errmsg(ctx, "No memory.");
                    ctx->quit = 1;
                    return;
                }
                __mln_lang_ctx_set_ret_var(ctx, var);
                break;
        }
        if (factor->type != M_FACTOR_ARRAY) {
            mln_lang_stack_node_free(mln_lang_stack_pop(ctx));
            mln_lang_stack_popuntil(ctx);
        }
    } else {
        if (factor->type == M_FACTOR_ARRAY) {
            mln_lang_ctx_get_node_ret_val(ctx, node);
        }
        ASSERT(ctx->ret_var != NULL);
        mln_lang_stack_node_free(mln_lang_stack_pop(ctx));
        mln_lang_stack_popuntil(ctx);
    }
})

MLN_FUNC_VOID(static, void, mln_lang_stack_handler_elemlist, (mln_lang_ctx_t *ctx), (ctx), {
    mln_lang_stack_node_t *node = mln_lang_stack_top(ctx);
    mln_lang_elemlist_t *elem = node->data.elemlist;
    if (node->step == 0) {
        node->step = 1;
        mln_lang_stack_node_reset_ret_val(node);
        mln_lang_ctx_reset_ret_var(ctx);
        ASSERT(node->ret_var == NULL);
        if (elem->key != NULL) {
            if ((node = mln_lang_stack_push(ctx, M_LSNT_ASSIGN, elem->key)) == NULL) {
                __mln_lang_errmsg(ctx, "Stack is full.");
                ctx->quit = 1;
                return;
            }
            mln_lang_stack_map[node->type](ctx);
            return;
        }
    } else if (node->step == 1) {
        node->step = 2;
        if (ctx->ret_var != NULL) {
            mln_lang_stack_node_get_ctx_ret_var(node, ctx);
        }
        if ((node = mln_lang_stack_push(ctx, M_LSNT_ASSIGN, elem->val)) == NULL) {
            __mln_lang_errmsg(ctx, "Stack is full.");
            ctx->quit = 1;
            return;
        }
        mln_lang_stack_map[node->type](ctx);
        return;
    } else if (node->step == 2) {
        mln_lang_var_t *key, *val;
        mln_lang_stack_node_t *array_node;
        mln_lang_array_t *array;

        node = mln_lang_stack_pop(ctx);
        array_node = mln_lang_stack_top(ctx);
        ASSERT(array_node != NULL);
        mln_lang_stack_withdraw(ctx);

        ASSERT(ctx->ret_var != NULL);
        ASSERT(array_node->ret_var->val != NULL);
        ASSERT(array_node->ret_var->val->type == M_LANG_VAL_TYPE_ARRAY);
        array = array_node->ret_var->val->data.array;
        ASSERT(array != NULL);
        key = node->ret_var;
        if ((val = __mln_lang_array_get(ctx, array, key)) == NULL) {
            ctx->quit = 1;
            return;
        }
        if (mln_lang_var_value_set_string_ref(ctx, val, ctx->ret_var) < 0) {
            __mln_lang_errmsg(ctx, "No memory.");
            ctx->quit = 1;
            return;
        }
        mln_lang_stack_node_free(mln_lang_stack_pop(ctx));
        mln_lang_ctx_reset_ret_var(ctx);
        if (elem->next != NULL) {
            if ((node = mln_lang_stack_push(ctx, M_LSNT_ELEMLIST, elem->next)) == NULL) {
                __mln_lang_errmsg(ctx, "Stack is full.");
                ctx->quit = 1;
                return;
            }
            mln_lang_stack_map[node->type](ctx);
            return;
        }
    }
})


MLN_FUNC(static, int, mln_lang_dump_symbol, \
         (mln_lang_ctx_t *ctx, void *key, void *val, void *udata), \
         (ctx, key, val, udata), \
{
    char *title = NULL;
    mln_lang_symbol_node_t *sym = (mln_lang_symbol_node_t *)val;
    struct mln_rbtree_attr rbattr;
    mln_rbtree_t *check;

    rbattr.pool = ctx->pool;
    rbattr.pool_alloc = (rbtree_pool_alloc_handler)mln_alloc_m;
    rbattr.pool_free = (rbtree_pool_free_handler)mln_alloc_free;
    rbattr.cmp = mln_lang_dump_check_cmp;
    rbattr.data_free = NULL;
    if ((check = mln_rbtree_new(&rbattr)) == NULL) {
        return -1;
    }

    switch (sym->type) {
        case M_LANG_SYMBOL_VAR:
            title = "Var";
            break;
        case M_LANG_SYMBOL_SET:
            title = "Set";
            break;
        default:
            title = "Label";
            break;
    }
    mln_log(none, "  %S <%s>", sym->symbol, title);
    switch (sym->type) {
        case M_LANG_SYMBOL_VAR:
            mln_lang_dump_var(sym->data.var, 4, check);
            break;
        default: /*M_LANG_SYMBOL_SET*/
            mln_lang_dump_set(sym->data.set);
            break;
    }
    mln_rbtree_free(check);
    return 0;
})

MLN_FUNC(static, int, mln_lang_dump_check_cmp, \
         (const void *addr1, const void *addr2), (addr1, addr2), \
{
    return (mln_s8ptr_t)addr1 - (mln_s8ptr_t)addr2;
})

#define blank(); {int tmpi; for (tmpi = 0; tmpi < cnt; tmpi++) mln_log(none, " ");}
MLN_FUNC_VOID(static, void, mln_lang_dump_var, \
              (mln_lang_var_t *var, int cnt, mln_rbtree_t *check), (var, cnt, check), \
{
    blank();
    mln_log(none, "%s", var->type == M_LANG_VAR_NORMAL?"Normal":"Refer");
    if (var->name != NULL) {
        mln_log(none, "  Alias name: %S", var->name);
    }
    if (var->val == NULL) {
        mln_log(none, "  No value.\n");
        return;
    }
    if (var->in_set != NULL) {
        mln_log(none, "  In Set '%S'  setRef: %l", var->in_set->name, var->in_set->ref);
    }
    mln_log(none, "  valueRef: %u, udata <0x%X>, func <0x%X>, not_modify: %s\n", \
            var->val->ref, \
            var->val->udata, \
            var->val->func, \
            var->val->not_modify? "true": "false");
    mln_s32_t type = mln_lang_var_val_type_get(var);
    switch (type) {
        case M_LANG_VAL_TYPE_NIL:
            blank();
            mln_log(none, "<NIL>\n");
            break;
        case M_LANG_VAL_TYPE_INT:
            blank();
            mln_log(none, "<INT> %i\n", var->val->data.i);
            break;
        case M_LANG_VAL_TYPE_BOOL:
            blank();
            mln_log(none, "<BOOL> %s\n", var->val->data.b?"true":"false");
            break;
        case M_LANG_VAL_TYPE_REAL:
            blank();
            mln_log(none, "<REAL> %f\n", var->val->data.f);
            break;
        case M_LANG_VAL_TYPE_STRING:
            blank();
            mln_log(none, "<STRING> '%S'\n", var->val->data.s);
            break;
        case M_LANG_VAL_TYPE_OBJECT:
            mln_lang_dump_object(var->val->data.obj, cnt, check);
            break;
        case M_LANG_VAL_TYPE_FUNC:
            mln_lang_dump_function(var->val->data.func, cnt);
            break;
        case M_LANG_VAL_TYPE_ARRAY:
            mln_lang_dump_array(var->val->data.array, cnt, check);
            break;
        default: /*M_LANG_VAL_TYPE_CALL:*/
            blank();
            mln_log(none, "<CALL>\n");
            ASSERT(var->val->data.call != NULL);
            if (var->val->data.call->prototype != NULL) {
                mln_lang_dump_function(var->val->data.call->prototype, cnt + 2);
            }
            break;
    }
})

MLN_FUNC_VOID(static, void, mln_lang_dump_object, \
              (mln_lang_object_t *obj, int cnt, mln_rbtree_t *check), (obj, cnt, check), \
{
    mln_rbtree_node_t *rn;
    struct mln_lang_scan_s ls;
    ls.cnt = &cnt;
    ls.tree = check;
    ls.tree2 = NULL;
    ls.ctx = NULL;

    blank();
    if (obj->in_set != NULL) {
        mln_log(none, "<OBJECT><%X> In Set '%S'  setRef: %I objRef: %I\n", \
                obj, obj->in_set->name, obj->in_set->ref, obj->ref);
    } else {
        mln_log(none, "<OBJECT><%X> In Set '<anonymous>'  setRef: <unknown> objRef: %I\n", obj, obj->ref);
    }

    if (check != NULL) {
        rn = mln_rbtree_search(check, obj);
        if (!mln_rbtree_null(rn, check)) {
            return;
        }
        if ((rn = mln_rbtree_node_new(check, obj)) == NULL) return;
        mln_rbtree_insert(check, rn);
    }
    mln_rbtree_iterate(obj->members, mln_lang_dump_var_iterate_handler, &ls);
})

MLN_FUNC(static, int, mln_lang_dump_var_iterate_handler, \
         (mln_rbtree_node_t *node, void *udata), (node, udata), \
{
    struct mln_lang_scan_s *ls = (struct mln_lang_scan_s *)udata;
    mln_lang_dump_var((mln_lang_var_t *)mln_rbtree_node_data_get(node), *(ls->cnt) + 2, ls->tree);
    return 0;
})

MLN_FUNC_VOID(static, void, mln_lang_dump_function, \
              (mln_lang_func_detail_t *func, int cnt), (func, cnt), \
{
    mln_lang_var_t *var;
    mln_size_t i, n;
    mln_string_t *file, cfunc = mln_string("<C Function>"), unknown = mln_string("<Empty Function>");
    mln_u64_t line = 0;
    file = &cfunc;

    blank();

    if (func->type == M_FUNC_EXTERNAL) {
        if (func->data.stm != NULL) {
            file = func->data.stm->file;
            line = func->data.stm->line;
        } else {
            file = &unknown;
        }
    }

    n = mln_array_nelts(&func->args);
    mln_log(none, "<FUNCTION> %S:%I NARGS:%I\n", file, line, (mln_u64_t)n);
    for (i = 0; i < n; ++i) {
        var = ((mln_lang_var_t **)mln_array_elts(&func->args))[i];
        mln_lang_dump_var(var, cnt + 2, NULL);
    }
})

MLN_FUNC_VOID(static, void, mln_lang_dump_set, (mln_lang_set_detail_t *set), (set), {
    int cnt = 4;
    mln_log(none, "    Name: %S  ref:%I\n", set->name, set->ref);
    mln_rbtree_iterate(set->members, mln_lang_dump_var_iterate_handler, &cnt);
})

MLN_FUNC_VOID(static, void, mln_lang_dump_array, \
              (mln_lang_array_t *array, int cnt, mln_rbtree_t *check), (array, cnt, check), \
{
    mln_rbtree_node_t *rn;
    int tmp = cnt + 2;
    struct mln_lang_scan_s ls;
    ls.cnt = &tmp;
    ls.tree = check;
    ls.tree2 = NULL;
    ls.ctx = NULL;
    blank();
    mln_log(none, "<ARRAY><%X>\n", array);
    if (check != NULL) {
        rn = mln_rbtree_search(check, array);
        if (!mln_rbtree_null(rn, check)) {
            return;
        }
        if ((rn = mln_rbtree_node_new(check, array)) == NULL) return;
        mln_rbtree_insert(check, rn);
    }
    blank();
    mln_log(none, "ALL ELEMENTS:\n");
    mln_rbtree_iterate(array->elems_index, mln_lang_dump_array_elem, &ls);
    blank();
    mln_log(none, "KEY ELEMENTS:\n");
    mln_rbtree_iterate(array->elems_key, mln_lang_dump_array_elem, &ls);
    blank();
    mln_log(none, "Refs: %I\n", array->ref);
})

MLN_FUNC(static, int, mln_lang_dump_array_elem, \
         (mln_rbtree_node_t *node, void *udata), (node, udata), \
{
    mln_lang_array_elem_t *elem = (mln_lang_array_elem_t *)mln_rbtree_node_data_get(node);
    struct mln_lang_scan_s *ls = (struct mln_lang_scan_s *)udata;
    int cnt = *(ls->cnt);
    blank();
    mln_log(none, "Index: %I\n", elem->index);
    if (elem->key != NULL) {
        blank();
        mln_log(none, "Key:\n");
        mln_lang_dump_var(elem->key, cnt + 2, ls->tree);
    }
    blank();
    mln_log(none, "Value:\n");
    mln_lang_dump_var(elem->value, cnt + 2, ls->tree);
    return 0;
})


MLN_FUNC(static, int, mln_lang_func_watch, (mln_lang_ctx_t *ctx), (ctx), {
    mln_lang_val_t *val;
    mln_lang_var_t *var;
    mln_lang_func_detail_t *func;
    mln_string_t funcname = mln_string("Watch");
    mln_string_t v1 = mln_string("var");
    mln_string_t v2 = mln_string("func");
    mln_string_t v3 = mln_string("data");
    if ((func = __mln_lang_func_detail_new(ctx, M_FUNC_INTERNAL, (void *)mln_lang_func_watch_process, NULL, NULL)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if ((val = __mln_lang_val_new(ctx, M_LANG_VAL_TYPE_NIL, NULL)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        __mln_lang_func_detail_free(func);
        return -1;
    }
    if ((var = __mln_lang_var_new(ctx, &v1, M_LANG_VAR_REFER, val, NULL)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        __mln_lang_val_free(val);
        __mln_lang_func_detail_free(func);
        return -1;
    }
    mln_lang_func_detail_arg_append(func, var);
    if ((val = __mln_lang_val_new(ctx, M_LANG_VAL_TYPE_NIL, NULL)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        __mln_lang_func_detail_free(func);
        return -1;
    }
    if ((var = __mln_lang_var_new(ctx, &v2, M_LANG_VAR_REFER, val, NULL)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        __mln_lang_val_free(val);
        __mln_lang_func_detail_free(func);
        return -1;
    }
    mln_lang_func_detail_arg_append(func, var);
    if ((val = __mln_lang_val_new(ctx, M_LANG_VAL_TYPE_NIL, NULL)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        __mln_lang_func_detail_free(func);
        return -1;
    }
    if ((var = __mln_lang_var_new(ctx, &v3, M_LANG_VAR_REFER, val, NULL)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        __mln_lang_val_free(val);
        __mln_lang_func_detail_free(func);
        return -1;
    }
    mln_lang_func_detail_arg_append(func, var);
    if ((val = __mln_lang_val_new(ctx, M_LANG_VAL_TYPE_FUNC, func)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        __mln_lang_func_detail_free(func);
        return -1;
    }
    if ((var = __mln_lang_var_new(ctx, &funcname, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        __mln_lang_val_free(val);
        return -1;
    }
    if (__mln_lang_symbol_node_join(ctx, M_LANG_SYMBOL_VAR, var) < 0) {
        __mln_lang_errmsg(ctx, "No memory.");
        __mln_lang_var_free(var);
        return -1;
    }
    return 0;
})

MLN_FUNC(static, mln_lang_var_t *, mln_lang_func_watch_process, (mln_lang_ctx_t *ctx), (ctx), {
    mln_lang_var_t *var;
    mln_string_t v1 = mln_string("var");
    mln_string_t v2 = mln_string("func");
    mln_string_t v3 = mln_string("data");
    mln_lang_symbol_node_t *sym;
    mln_lang_val_t *val1;
    mln_lang_func_detail_t *func, *dup;
    if ((sym = __mln_lang_symbol_node_search(ctx, &v1, 1)) == NULL) {
        ASSERT(0);
        __mln_lang_errmsg(ctx, "Argument missing.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);
    val1 = sym->data.var->val;

    if ((sym = __mln_lang_symbol_node_search(ctx, &v2, 1)) == NULL) {
        ASSERT(0);
        __mln_lang_errmsg(ctx, "Argument missing.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);
    if (sym->data.var->val->type != M_LANG_VAL_TYPE_FUNC) {
        if ((var = mln_lang_var_create_false(ctx, NULL)) == NULL) {
            __mln_lang_errmsg(ctx, "No memory.");
            return NULL;
        }
    } else {
        mln_lang_val_t *val3;
        func = sym->data.var->val->data.func;

        if ((sym = __mln_lang_symbol_node_search(ctx, &v3, 1)) == NULL) {
            ASSERT(0);
            __mln_lang_errmsg(ctx, "Argument missing.");
            return NULL;
        }
        ASSERT(sym->type == M_LANG_SYMBOL_VAR);
        val3 = sym->data.var->val;

        if ((dup = mln_lang_func_detail_dup(ctx, func)) == NULL) {
            __mln_lang_errmsg(ctx, "No memory.");
            return NULL;
        }

        if (val1->func != NULL) mln_lang_func_detail_free(val1->func);
        if (val1->udata != NULL) __mln_lang_val_free(val1->udata);
        val1->func = dup;
        val1->udata = val3;
        ++(val3->ref);

        if ((var = mln_lang_var_create_true(ctx, NULL)) == NULL) {
            __mln_lang_errmsg(ctx, "No memory.");
            return NULL;
        }
    }
    return var;
})

MLN_FUNC(static, int, mln_lang_func_unwatch, (mln_lang_ctx_t *ctx), (ctx), {
    mln_lang_val_t *val;
    mln_lang_var_t *var;
    mln_lang_func_detail_t *func;
    mln_string_t funcname = mln_string("Unwatch");
    mln_string_t v1 = mln_string("var");
    if ((func = __mln_lang_func_detail_new(ctx, M_FUNC_INTERNAL, (void *)mln_lang_func_unwatch_process, NULL, NULL)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if ((val = __mln_lang_val_new(ctx, M_LANG_VAL_TYPE_NIL, NULL)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        __mln_lang_func_detail_free(func);
        return -1;
    }
    if ((var = __mln_lang_var_new(ctx, &v1, M_LANG_VAR_REFER, val, NULL)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        __mln_lang_val_free(val);
        __mln_lang_func_detail_free(func);
        return -1;
    }
    mln_lang_func_detail_arg_append(func, var);
    if ((val = __mln_lang_val_new(ctx, M_LANG_VAL_TYPE_FUNC, func)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        __mln_lang_func_detail_free(func);
        return -1;
    }
    if ((var = __mln_lang_var_new(ctx, &funcname, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        __mln_lang_val_free(val);
        return -1;
    }
    if (__mln_lang_symbol_node_join(ctx, M_LANG_SYMBOL_VAR, var) < 0) {
        __mln_lang_errmsg(ctx, "No memory.");
        __mln_lang_var_free(var);
        return -1;
    }
    return 0;
})

MLN_FUNC(static, mln_lang_var_t *, mln_lang_func_unwatch_process, (mln_lang_ctx_t *ctx), (ctx), {
    mln_lang_var_t *var;
    mln_string_t v1 = mln_string("var");
    mln_lang_symbol_node_t *sym;
    mln_lang_val_t *val1;
    if ((sym = __mln_lang_symbol_node_search(ctx, &v1, 1)) == NULL) {
        ASSERT(0);
        __mln_lang_errmsg(ctx, "Argument missing.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);
    val1 = sym->data.var->val;
    if (val1->func != NULL) mln_lang_func_detail_free(val1->func);
    if (val1->udata != NULL) __mln_lang_val_free(val1->udata);
    val1->func = NULL;
    val1->udata = NULL;
    if ((var = mln_lang_var_create_nil(ctx, NULL)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        return NULL;
    }
    return var;
})

MLN_FUNC(static, int, mln_lang_watch_func_build, \
         (mln_lang_ctx_t *ctx, mln_lang_stack_node_t *node, mln_lang_func_detail_t *funcdef, mln_lang_val_t *udata, mln_lang_val_t *newval), \
         (ctx, node, funcdef, udata, newval), \
{
    mln_lang_funccall_val_t *func;
    mln_lang_var_t *var;

    if ((func = __mln_lang_funccall_val_new(ctx->pool, NULL)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    func->prototype = funcdef;
    if ((var = __mln_lang_var_new(ctx, NULL, M_LANG_VAR_REFER, newval, NULL)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        __mln_lang_funccall_val_free(func);
        return -1;
    }
    mln_lang_funccall_val_add_arg(func, var);
    if ((var = __mln_lang_var_new(ctx, NULL, M_LANG_VAR_REFER, udata, NULL)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        __mln_lang_funccall_val_free(func);
        return -1;
    }
    mln_lang_funccall_val_add_arg(func, var);

    if (mln_lang_stack_handler_funccall_run(ctx, node, func) < 0) {
        __mln_lang_funccall_val_free(func);
        return -1;
    }
    node->call = 1;
    __mln_lang_funccall_val_free(func);
    return 0;
})

MLN_FUNC(static, int, mln_lang_func_dump, (mln_lang_ctx_t *ctx), (ctx), {
    mln_lang_val_t *val;
    mln_lang_var_t *var;
    mln_lang_func_detail_t *func;
    mln_string_t funcname = mln_string("Dump");
    mln_string_t v1 = mln_string("var");
    if ((func = __mln_lang_func_detail_new(ctx, M_FUNC_INTERNAL, (void *)mln_lang_func_dump_process, NULL, NULL)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if ((val = __mln_lang_val_new(ctx, M_LANG_VAL_TYPE_NIL, NULL)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        __mln_lang_func_detail_free(func);
        return -1;
    }
    if ((var = __mln_lang_var_new(ctx, &v1, M_LANG_VAR_REFER, val, NULL)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        __mln_lang_val_free(val);
        __mln_lang_func_detail_free(func);
        return -1;
    }
    mln_lang_func_detail_arg_append(func, var);
    if ((val = __mln_lang_val_new(ctx, M_LANG_VAL_TYPE_FUNC, func)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        __mln_lang_func_detail_free(func);
        return -1;
    }
    if ((var = __mln_lang_var_new(ctx, &funcname, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        __mln_lang_val_free(val);
        return -1;
    }
    if (__mln_lang_symbol_node_join(ctx, M_LANG_SYMBOL_VAR, var) < 0) {
        __mln_lang_errmsg(ctx, "No memory.");
        __mln_lang_var_free(var);
        return -1;
    }
    return 0;
})

MLN_FUNC(static, mln_lang_var_t *, mln_lang_func_dump_process, (mln_lang_ctx_t *ctx), (ctx), {
    mln_lang_var_t *var;
    mln_string_t v1 = mln_string("var");
    mln_lang_symbol_node_t *sym;
    if ((sym = __mln_lang_symbol_node_search(ctx, &v1, 1)) == NULL) {
        ASSERT(0);
        __mln_lang_errmsg(ctx, "Argument missing.");
        return NULL;
    }
    (void)mln_lang_dump_symbol(ctx, NULL, sym, NULL);
    if ((var = mln_lang_var_create_nil(ctx, NULL)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        return NULL;
    }
    return var;
})

MLN_FUNC(static, int, mln_lang_internal_func_installer, (mln_lang_ctx_t *ctx), (ctx), {
    if (mln_lang_func_resource_register(ctx) < 0) return -1;
    if (mln_lang_func_import_handler(ctx) < 0) return -1;
    if (mln_lang_func_dump(ctx) < 0) return -1;
    if (mln_lang_func_stack(ctx) < 0) return -1;
    if (mln_lang_func_watch(ctx) < 0) return -1;
    if (mln_lang_func_unwatch(ctx) < 0) return -1;
    if (mln_lang_func_eval(ctx) < 0) return -1;
    if (mln_lang_func_kill(ctx) < 0) return -1;
    if (mln_lang_func_pipe(ctx) < 0) return -1;
    return 0;
})

MLN_FUNC(static, int, mln_lang_func_resource_register, (mln_lang_ctx_t *ctx), (ctx), {
    mln_rbtree_t *tree;
    struct mln_rbtree_attr rbattr;

    if ((tree = (mln_rbtree_t *)mln_lang_resource_fetch(ctx->lang, "import")) == NULL) {
        rbattr.pool = ctx->lang->pool;
        rbattr.pool_alloc = (rbtree_pool_alloc_handler)mln_alloc_m;
        rbattr.pool_free = (rbtree_pool_free_handler)mln_alloc_free;
        rbattr.cmp = (rbtree_cmp)mln_lang_func_import_cmp;
        rbattr.data_free = (rbtree_free_data)mln_lang_func_import_free;
        if ((tree = mln_rbtree_new(&rbattr)) == NULL) {
            mln_lang_errmsg(ctx, "No memory.");
            return -1;
        }
        if (mln_lang_resource_register(ctx->lang, "import", tree, (mln_lang_resource_free)mln_rbtree_free) < 0) {
            mln_rbtree_free(tree);
            mln_lang_errmsg(ctx, "No memory.");
            return -1;
        }
    }

    if ((tree = (mln_rbtree_t *)mln_lang_ctx_resource_fetch(ctx, "import")) == NULL) {
        rbattr.pool = ctx->pool;
        rbattr.pool_alloc = (rbtree_pool_alloc_handler)mln_alloc_m;
        rbattr.pool_free = (rbtree_pool_free_handler)mln_alloc_free;
        rbattr.cmp = (rbtree_cmp)mln_lang_func_ctx_import_cmp;
        rbattr.data_free = (rbtree_free_data)mln_lang_func_ctx_import_free;
        if ((tree = mln_rbtree_new(&rbattr)) == NULL) {
            mln_lang_errmsg(ctx, "No memory.");
            return -1;
        }
        if (mln_lang_ctx_resource_register(ctx, "import", tree, (mln_lang_resource_free)mln_rbtree_free) < 0) {
            mln_rbtree_free(tree);
            mln_lang_errmsg(ctx, "No memory.");
            return -1;
        }
    }

    return 0;
})

MLN_FUNC(static, int, mln_lang_func_stack, (mln_lang_ctx_t *ctx), (ctx), {
    mln_lang_val_t *val;
    mln_lang_var_t *var;
    mln_lang_func_detail_t *func;
    mln_string_t funcname = mln_string("Stack");
    if ((func = __mln_lang_func_detail_new(ctx, M_FUNC_INTERNAL, (void *)mln_lang_func_stack_process, NULL, NULL)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if ((val = __mln_lang_val_new(ctx, M_LANG_VAL_TYPE_FUNC, func)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        __mln_lang_func_detail_free(func);
        return -1;
    }
    if ((var = __mln_lang_var_new(ctx, &funcname, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        __mln_lang_val_free(val);
        return -1;
    }
    if (__mln_lang_symbol_node_join(ctx, M_LANG_SYMBOL_VAR, var) < 0) {
        __mln_lang_errmsg(ctx, "No memory.");
        __mln_lang_var_free(var);
        return -1;
    }
    return 0;
})

MLN_FUNC(static, mln_lang_var_t *, mln_lang_func_stack_process, (mln_lang_ctx_t *ctx), (ctx), {
    mln_lang_var_t *var;
    mln_string_t *name, anony = mln_string("<anonymous>");
    mln_string_t *file;
    mln_u64_t line;
    mln_lang_scope_t *s = mln_lang_scope_top(ctx);
    int i;


    for (i = 1, --s; s >= mln_lang_scope_base(ctx); --s, ++i) {
        name = s->name == NULL? &anony: s->name;
        file = s->entry == NULL? &anony: s->entry->file;
        line = s->entry == NULL? 0: s->entry->line;
        mln_log(none, "%d: %S %S:%I\n", i, name, file, line);
    }

    if ((var = mln_lang_var_create_nil(ctx, NULL)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        return NULL;
    }
    return var;
})

MLN_FUNC(static, mln_lang_import_t *, mln_lang_func_import_new, \
         (mln_lang_ctx_t *ctx, mln_string_t *name, void *handle), (ctx, name, handle), \
{
    mln_lang_import_t *i;

    if ((i = (mln_lang_import_t *)mln_alloc_m(ctx->lang->pool, sizeof(mln_lang_import_t))) == NULL) {
        return NULL;
    }
    if ((i->name = mln_string_pool_dup(ctx->lang->pool, name)) == NULL) {
        mln_alloc_free(i);
        return NULL;
    }
    i->ref = 0;
    i->count = MLN_LANG_IMPORT_FREE_COUNT;
    i->handle = handle;
    i->node = NULL;
    i->lang = ctx->lang;

    return i;
})

MLN_FUNC(static, int, mln_lang_func_import_cmp, \
         (const mln_lang_import_t *i1, const mln_lang_import_t *i2), (i1, i2), \
{
    return mln_string_strcmp(i1->name, i2->name);
})

static void mln_lang_func_import_free(mln_lang_import_t *i)
{
    import_destroy_t destroy;

    if (i == NULL) return;

#if defined(MSVC)
    destroy = (import_destroy_t)GetProcAddress(i->handle, "destroy");
#else
    destroy = (import_destroy_t)dlsym(i->handle, "destroy");
#endif
    if (destroy != NULL) {
        destroy(i->lang);
    }

    if (i->name != NULL) mln_string_free(i->name);
    if (i->handle != NULL) {
#if defined(MSVC)
        FreeLibrary((HMODULE)(i->handle));
#else
        dlclose(i->handle);
#endif
    }
    mln_alloc_free(i);
}

MLN_FUNC(static, mln_lang_ctx_import_t *, mln_lang_func_ctx_import_new, \
         (mln_lang_ctx_t *ctx, mln_lang_import_t *i), (ctx, i), \
{
    mln_lang_ctx_import_t *ci;

    if ((ci = (mln_lang_ctx_import_t *)mln_alloc_m(ctx->pool, sizeof(mln_lang_ctx_import_t))) == NULL) {
        return NULL;
    }
    ci->i = i;
    ci->ctx = ctx;
    ++(ci->i->ref);

    return ci;
})

MLN_FUNC(static, int, mln_lang_func_ctx_import_cmp, \
         (mln_lang_ctx_import_t *ci1, mln_lang_ctx_import_t *ci2), (ci1, ci2), \
{
    return mln_string_strcmp(ci1->i->name, ci2->i->name);
})

MLN_FUNC_VOID(static, void, mln_lang_func_ctx_import_free, (mln_lang_ctx_import_t *ci), (ci), {
    mln_rbtree_t *tree;

    if (ci == NULL) return;
    mln_lang_import_t *i = ci->i;
    mln_lang_ctx_t *ctx = ci->ctx;
    mln_alloc_free(ci);

    if ((i->ref)-- > 1) return;
    if ((i->count)-- > 0) return;

    if ((tree = (mln_rbtree_t *)mln_lang_resource_fetch(ctx->lang, "import")) != NULL) {
        mln_rbtree_delete(tree, i->node);
        mln_rbtree_node_free(tree, i->node);
    }
})

MLN_FUNC(static, int, mln_lang_func_import_handler, (mln_lang_ctx_t *ctx), (ctx), {
    mln_lang_val_t *val;
    mln_lang_var_t *var;
    mln_lang_func_detail_t *func;
    mln_string_t funcname = mln_string("Import");
    mln_string_t v1 = mln_string("name");
    if ((func = mln_lang_func_detail_new(ctx, M_FUNC_INTERNAL, (void *)mln_lang_func_import_process, NULL, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if ((val = mln_lang_val_new(ctx, M_LANG_VAL_TYPE_NIL, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_func_detail_free(func);
        return -1;
    }
    if ((var = mln_lang_var_new(ctx, &v1, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_val_free(val);
        mln_lang_func_detail_free(func);
        return -1;
    }
    mln_lang_func_detail_arg_append(func, var);
    if ((val = mln_lang_val_new(ctx, M_LANG_VAL_TYPE_FUNC, func)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_func_detail_free(func);
        return -1;
    }
    if ((var = mln_lang_var_new(ctx, &funcname, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_val_free(val);
        return -1;
    }
    if (mln_lang_symbol_node_join(ctx, M_LANG_SYMBOL_VAR, var) < 0) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_var_free(var);
        return -1;
    }
    return 0;
})

static mln_lang_var_t *mln_lang_func_import_process(mln_lang_ctx_t *ctx)
{
    mln_lang_var_t *ret_var;
    mln_string_t v1 = mln_string("name");
    mln_lang_ctx_import_t ci, *pci;
    mln_lang_symbol_node_t *sym;
    mln_lang_import_t i, *pi;
    mln_string_t *name;
    mln_rbtree_node_t *rn, *ctx_rn;
    mln_rbtree_t *tree, *ctx_tree;
    char path[1024];
    char tmp_path[1024];
    char *melang_dy_path;
    import_init_t init;
    void *handle;
    int n;

    if ((sym = mln_lang_symbol_node_search(ctx, &v1, 1)) == NULL) {
        ASSERT(0);
        mln_lang_errmsg(ctx, "Argument 1 missing.");
        return NULL;
    }
    if (sym->type != M_LANG_SYMBOL_VAR || mln_lang_var_val_type_get(sym->data.var) != M_LANG_VAL_TYPE_STRING) {
        mln_lang_errmsg(ctx, "Invalid type of argument 1.");
        return NULL;
    }
    name = mln_lang_var_val_get(sym->data.var)->data.s;

    i.name = name;
    tree = (mln_rbtree_t *)mln_lang_resource_fetch(ctx->lang, "import");
    rn = mln_rbtree_search(tree, &i);
    if (!mln_rbtree_null(rn, tree)) {
        pi = (mln_lang_import_t *)mln_rbtree_node_data_get(rn);
    } else {
#if defined(MSVC)
        if (name->len > 1 && name->data[1] == ':') {
            n = snprintf(path, sizeof(path)-1, "%s.dll", (char *)(name->data));
#else
        if (name->data[0] == '/') {
            n = snprintf(path, sizeof(path)-1, "%s.so", (char *)(name->data));
#endif
            path[n] = 0;
        } else {
            if (name->len > 0 && name->data[0] != '.') {
#if defined(MSVC)
                n = snprintf(path, sizeof(path)-1, "%s.dll", (char *)(name->data));
#else
                n = snprintf(path, sizeof(path)-1, "./%s.so", (char *)(name->data));
#endif
            } else {
#if defined(MSVC)
                n = snprintf(path, sizeof(path)-1, "%s.dll", (char *)(name->data));
#else
                n = snprintf(path, sizeof(path)-1, "%s.so", (char *)(name->data));
#endif
            }
            path[n] = 0;
#if defined(MSVC)
            if (!_access(path, 0)) {
#else
            if (!access(path, F_OK)) {
#endif
                /* do nothing */
            } else if ((melang_dy_path = getenv("MELANG_DYNAMIC_PATH")) != NULL) {
                char *end = strchr(melang_dy_path, ';');
                int found = 0;
                while (end != NULL) {
                    *end = 0;
                    n = snprintf(tmp_path, sizeof(tmp_path)-1, "%s/%s", melang_dy_path, path);
#if defined(MSVC)
                    if (!_access(tmp_path, 0)) {
#else
                    if (!access(tmp_path, F_OK)) {
#endif
                        memcpy(path, tmp_path, n);
                        path[n] = 0;
                        found = 1;
                        break;
                    }
                    tmp_path[n] = 0;
                    melang_dy_path = end + 1;
                    end = strchr(melang_dy_path, ';');
                }
                if (!found) {
                    if (*melang_dy_path) {
                        n = snprintf(tmp_path, sizeof(tmp_path)-1, "%s/%s", melang_dy_path, path);
                        memcpy(path, tmp_path, n);
                        path[n] = 0;
                    } else {
                        goto goon;
                    }
                }
            } else {
goon:
                n = snprintf(tmp_path, sizeof(tmp_path)-1, "%s/%s", mln_path_melang_dylib(), path);
                memcpy(path, tmp_path, n);
                path[n] = 0;
            }
        }

#if defined(MSVC)
        handle = LoadLibrary(TEXT(path));
#else
        handle = dlopen(path, RTLD_LAZY);
#endif
        if (handle == NULL) {
            n = snprintf(tmp_path, sizeof(tmp_path)-1, "Load dynamic library [%s] failed.", path);
            tmp_path[n] = 0;
            mln_lang_errmsg(ctx, tmp_path);
            return NULL;
        }

        if ((pi = mln_lang_func_import_new(ctx, name, handle)) == NULL) {
#if defined(MSVC)
            FreeLibrary((HMODULE)(pi->handle));
#else
            dlclose(pi->handle);
#endif
            mln_lang_errmsg(ctx, "No memory.");
            return NULL;
        }
        if ((rn = mln_rbtree_node_new(tree, pi)) == NULL) {
            mln_lang_func_import_free(pi);
            mln_lang_errmsg(ctx, "No memory.");
            return NULL;
        }
        mln_rbtree_insert(tree, rn);
        pi->node = rn;
    }

    ctx_tree = (mln_rbtree_t *)mln_lang_ctx_resource_fetch(ctx, "import");
    ci.i = pi;
    ctx_rn = mln_rbtree_search(ctx_tree, &ci);
    if (mln_rbtree_null(ctx_rn, ctx_tree)) {
        if ((pci = mln_lang_func_ctx_import_new(ctx, pi)) == NULL) {
            if (!pi->ref) {
                mln_rbtree_delete(tree, rn);
                mln_rbtree_node_free(tree, rn);
            }
            mln_lang_errmsg(ctx, "No memory.");
            return NULL;
        }
        if ((ctx_rn = mln_rbtree_node_new(ctx_tree, pci)) == NULL) {
            mln_lang_func_ctx_import_free(pci);
            mln_lang_errmsg(ctx, "No memory.");
            return NULL;
        }
        mln_rbtree_insert(ctx_tree, ctx_rn);
    }

#if defined(MSVC)
    init = (import_init_t)GetProcAddress(pi->handle, "init");
#else
    init = (import_init_t)dlsym(pi->handle, "init");
#endif
    if (init == NULL) {
        n = snprintf(tmp_path, sizeof(tmp_path)-1, "No 'init' found in dynamic library [%s].", path);
        tmp_path[n] = 0;
        mln_lang_errmsg(ctx, tmp_path);
        return NULL;
    }

    if ((ret_var = init(ctx, mln_conf())) == NULL) {
        n = snprintf(tmp_path, sizeof(tmp_path)-1, "Init dynamic library [%s] failed.", path);
        tmp_path[n] = 0;
        mln_lang_errmsg(ctx, tmp_path);
        return NULL;
    }

    return ret_var;
}

MLN_FUNC(static, int, mln_lang_func_eval, (mln_lang_ctx_t *ctx), (ctx), {
    mln_lang_val_t *val;
    mln_lang_var_t *var;
    mln_lang_func_detail_t *func;
    mln_string_t funcname = mln_string("Eval");
    mln_string_t v1 = mln_string("val");
    mln_string_t v2 = mln_string("data");
    mln_string_t v3 = mln_string("in_string");
    mln_string_t v4 = mln_string("alias");
    if ((func = mln_lang_func_detail_new(ctx, M_FUNC_INTERNAL, (void *)mln_lang_func_eval_process, NULL, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if ((val = mln_lang_val_new(ctx, M_LANG_VAL_TYPE_NIL, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_func_detail_free(func);
        return -1;
    }
    if ((var = mln_lang_var_new(ctx, &v1, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_val_free(val);
        mln_lang_func_detail_free(func);
        return -1;
    }
    mln_lang_func_detail_arg_append(func, var);
    if ((val = mln_lang_val_new(ctx, M_LANG_VAL_TYPE_NIL, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_func_detail_free(func);
        return -1;
    }
    if ((var = mln_lang_var_new(ctx, &v2, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_val_free(val);
        mln_lang_func_detail_free(func);
        return -1;
    }
    mln_lang_func_detail_arg_append(func, var);
    if ((val = mln_lang_val_new(ctx, M_LANG_VAL_TYPE_NIL, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_func_detail_free(func);
        return -1;
    }
    if ((var = mln_lang_var_new(ctx, &v3, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_val_free(val);
        mln_lang_func_detail_free(func);
        return -1;
    }
    mln_lang_func_detail_arg_append(func, var);
    if ((val = mln_lang_val_new(ctx, M_LANG_VAL_TYPE_NIL, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_func_detail_free(func);
        return -1;
    }
    if ((var = mln_lang_var_new(ctx, &v4, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_val_free(val);
        mln_lang_func_detail_free(func);
        return -1;
    }
    mln_lang_func_detail_arg_append(func, var);
    if ((val = mln_lang_val_new(ctx, M_LANG_VAL_TYPE_FUNC, func)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_func_detail_free(func);
        return -1;
    }
    if ((var = mln_lang_var_new(ctx, &funcname, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_val_free(val);
        return -1;
    }
    if (mln_lang_symbol_node_join(ctx, M_LANG_SYMBOL_VAR, var) < 0) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_var_free(var);
        return -1;
    }
    return 0;
})

MLN_FUNC(static, mln_lang_var_t *, mln_lang_func_eval_process, (mln_lang_ctx_t *ctx), (ctx), {
    mln_lang_var_t *ret_var = NULL;
    mln_string_t v1 = mln_string("val");
    mln_string_t v2 = mln_string("data");
    mln_string_t v3 = mln_string("in_string");
    mln_string_t v4 = mln_string("alias");
    mln_string_t data_name = mln_string("EVAL_DATA"), *dup, *alias, *path, tmp;
    mln_lang_symbol_node_t *sym;
    mln_lang_val_t *val1, *val2;
    mln_s32_t type, type3;
    mln_u32_t job_type;
    mln_u8ptr_t data;
    mln_lang_ctx_t *newctx;
    char buf[1024], *p;
    int n;
    /*arg1*/
    if ((sym = mln_lang_symbol_node_search(ctx, &v1, 1)) == NULL) {
        ASSERT(0);
        mln_lang_errmsg(ctx, "Argument 1 missing.");
        return NULL;
    }
    if (sym->type != M_LANG_SYMBOL_VAR) {
        mln_lang_errmsg(ctx, "Invalid type of argument 1.");
        return NULL;
    }
    if (mln_lang_var_val_type_get(sym->data.var) == M_LANG_VAL_TYPE_NIL) {
        return mln_lang_func_eval_process_list(ctx);
    }
    if (mln_lang_var_val_type_get(sym->data.var) != M_LANG_VAL_TYPE_STRING) {
        mln_lang_errmsg(ctx, "Invalid type of argument 1.");
        return NULL;
    }
    val1 = mln_lang_var_val_get(sym->data.var);
    /*arg2*/
    if ((sym = mln_lang_symbol_node_search(ctx, &v2, 1)) == NULL) {
        ASSERT(0);
        mln_lang_errmsg(ctx, "Argument 2 missing.");
        return NULL;
    }
    if (sym->type != M_LANG_SYMBOL_VAR) {
        mln_lang_errmsg(ctx, "Invalid type of argument 2.");
        return NULL;
    }
    val2 = mln_lang_var_val_get(sym->data.var);
    type = mln_lang_var_val_type_get(sym->data.var);
    if (type < M_LANG_VAL_TYPE_NIL || type >= M_LANG_VAL_TYPE_OBJECT) {
        mln_lang_errmsg(ctx, "Invalid type of argument 2, only support nil, int, real, boolean, string.");
        return NULL;
    }
    /*arg3*/
    if ((sym = mln_lang_symbol_node_search(ctx, &v3, 1)) == NULL) {
        ASSERT(0);
        mln_lang_errmsg(ctx, "Argument 3 missing.");
        return NULL;
    }
    if (sym->type != M_LANG_SYMBOL_VAR) {
        mln_lang_errmsg(ctx, "Invalid type of argument 3.");
        return NULL;
    }
    type3 = mln_lang_var_val_type_get(sym->data.var);
    if (type3 == M_LANG_VAL_TYPE_BOOL && mln_lang_var_val_get(sym->data.var)->data.b) {
        job_type = M_INPUT_T_BUF;
    } else {
        job_type = M_INPUT_T_FILE;
    }
    /*arg4*/
    if ((sym = mln_lang_symbol_node_search(ctx, &v4, 1)) == NULL) {
        ASSERT(0);
        mln_lang_errmsg(ctx, "Argument 4 missing.");
        return NULL;
    }
    if (sym->type != M_LANG_SYMBOL_VAR) {
        mln_lang_errmsg(ctx, "Invalid type of argument 4.");
        return NULL;
    }
    if (mln_lang_var_val_type_get(sym->data.var) == M_LANG_VAL_TYPE_NIL) {
        alias = NULL;
    } else if (mln_lang_var_val_type_get(sym->data.var) == M_LANG_VAL_TYPE_STRING) {
        alias = mln_lang_var_val_get(sym->data.var)->data.s;
    } else {
        mln_lang_errmsg(ctx, "Invalid type of argument 4.");
        return NULL;
    }
    path = val1->data.s;
    if (path->len && path->data[0] == '@' && ctx->filename != NULL) {
        if ((p = strrchr((const char *)(ctx->filename->data), '/')) == NULL) {
            if (path->len > 1 && path->data[1] == '/') {
                tmp.data = path->data + 2;
                tmp.len = path->len - 2;
            } else {
                tmp.data = path->data + 1;
                tmp.len = path->len - 1;
            }
        } else {
            n = sizeof(buf) - 1;
            if (n > p - (char *)(ctx->filename->data)) {
                n = p - (char *)(ctx->filename->data);
            }
            memcpy(buf, ctx->filename->data, n);
            n += snprintf(buf + n, sizeof(buf) - n - 1, "/%s", (char *)(&(path->data[1])));
            buf[n] = 0;
            mln_string_nset(&tmp, buf, n);
        }
        path = &tmp;
    }
    /*create job ctx*/
    if ((newctx = __mln_lang_job_new(ctx->lang, alias, job_type, path, NULL, NULL)) == NULL) {
        if ((ret_var = mln_lang_var_create_false(ctx, NULL)) == NULL) {
            mln_lang_errmsg(ctx, "No memory.");
            return NULL;
        }
        return ret_var;
    }
    switch (type) {
        case M_LANG_VAL_TYPE_NIL:
            data = NULL;
            break;
        case M_LANG_VAL_TYPE_INT:
            data = (mln_u8ptr_t)&(val2->data.i);
            break;
        case M_LANG_VAL_TYPE_BOOL:
            data = (mln_u8ptr_t)&(val2->data.b);
            break;
        case M_LANG_VAL_TYPE_REAL:
            data = (mln_u8ptr_t)&(val2->data.f);
            break;
        case M_LANG_VAL_TYPE_STRING:
            if ((data = (mln_u8ptr_t)mln_string_pool_dup(newctx->pool, val2->data.s)) == NULL) {
                mln_lang_errmsg(ctx, "No memory.");
                __mln_lang_job_free(newctx);
                return NULL;
            }
            break;
        default:
            mln_lang_errmsg(ctx, "Invalid type of argument 2");
            ASSERT(0);
            return NULL;
    }
    dup = mln_string_pool_dup(newctx->pool, &data_name);
    if (dup == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        if (type == M_LANG_VAL_TYPE_STRING) mln_string_free((mln_string_t *)data);
        __mln_lang_job_free(newctx);
        return NULL;
    }
    if (mln_lang_ctx_global_var_add(newctx, dup, data, type) < 0) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_string_free(dup);
        if (type == M_LANG_VAL_TYPE_STRING) mln_string_free((mln_string_t *)data);
        __mln_lang_job_free(newctx);
        return NULL;
    }
    mln_string_free(dup);
    if ((ret_var = mln_lang_var_create_nil(ctx, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        __mln_lang_job_free(newctx);
        return NULL;
    }
    return ret_var;
})

MLN_FUNC(static inline, mln_lang_var_t *, mln_lang_func_eval_process_list, \
         (mln_lang_ctx_t *ctx), (ctx), \
{
    mln_lang_ctx_t *c;
    mln_lang_var_t *ret_var, *var, *key;
    mln_lang_array_t *arr;

    if ((ret_var = mln_lang_var_create_array(ctx, NULL)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        return NULL;
    }
    arr = mln_lang_var_val_get(ret_var)->data.array;

    for (c = ctx->lang->run_head; c != NULL; c = c->next) {
        if (c->alias == NULL) continue;

        if ((key = mln_lang_var_create_ref_string(ctx, c->alias, NULL)) == NULL) {
            __mln_lang_var_free(ret_var);
            __mln_lang_errmsg(ctx, "No memory.");
            return NULL;
        }
        if ((var = mln_lang_array_get(ctx, arr, key)) == NULL) {
            __mln_lang_var_free(key);
            __mln_lang_var_free(ret_var);
            __mln_lang_errmsg(ctx, "No memory.");
            return NULL;
        }
        __mln_lang_var_free(key);

        mln_lang_var_set_string(var, mln_string_ref(c->alias));
    }

    for (c = ctx->lang->wait_head; c != NULL; c = c->next) {
        if (c->alias == NULL) continue;

        if ((key = mln_lang_var_create_ref_string(ctx, c->alias, NULL)) == NULL) {
            __mln_lang_var_free(ret_var);
            __mln_lang_errmsg(ctx, "No memory.");
            return NULL;
        }
        if ((var = mln_lang_array_get(ctx, arr, key)) == NULL) {
            __mln_lang_var_free(key);
            __mln_lang_var_free(ret_var);
            __mln_lang_errmsg(ctx, "No memory.");
            return NULL;
        }
        __mln_lang_var_free(key);

        mln_lang_var_set_string(var, mln_string_ref(c->alias));
    }

    return ret_var;
})

MLN_FUNC(static, int, mln_lang_func_kill, (mln_lang_ctx_t *ctx), (ctx), {
    mln_lang_val_t *val;
    mln_lang_var_t *var;
    mln_lang_func_detail_t *func;
    mln_string_t funcname = mln_string("Kill");
    mln_string_t v1 = mln_string("alias");
    if ((func = mln_lang_func_detail_new(ctx, M_FUNC_INTERNAL, (void *)mln_lang_func_kill_process, NULL, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if ((val = mln_lang_val_new(ctx, M_LANG_VAL_TYPE_NIL, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_func_detail_free(func);
        return -1;
    }
    if ((var = mln_lang_var_new(ctx, &v1, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_val_free(val);
        mln_lang_func_detail_free(func);
        return -1;
    }
    mln_lang_func_detail_arg_append(func, var);
    if ((val = mln_lang_val_new(ctx, M_LANG_VAL_TYPE_FUNC, func)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_func_detail_free(func);
        return -1;
    }
    if ((var = mln_lang_var_new(ctx, &funcname, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_val_free(val);
        return -1;
    }
    if (mln_lang_symbol_node_join(ctx, M_LANG_SYMBOL_VAR, var) < 0) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_var_free(var);
        return -1;
    }
    return 0;
})

MLN_FUNC(static, mln_lang_var_t *, mln_lang_func_kill_process, (mln_lang_ctx_t *ctx), (ctx), {
    mln_lang_var_t *ret_var = NULL;
    mln_string_t v1 = mln_string("alias");
    mln_lang_symbol_node_t *sym;
    mln_lang_ctx_t *killed_ctx, tmp;
    mln_rbtree_node_t *rn;

    if ((sym = mln_lang_symbol_node_search(ctx, &v1, 1)) == NULL) {
        ASSERT(0);
        mln_lang_errmsg(ctx, "Argument 1 missing.");
        return NULL;
    }
    if (sym->type != M_LANG_SYMBOL_VAR || mln_lang_var_val_type_get(sym->data.var) != M_LANG_VAL_TYPE_STRING) {
        mln_lang_errmsg(ctx, "Invalid type of argument 1.");
        return NULL;
    }
    tmp.alias = mln_lang_var_val_get(sym->data.var)->data.s;

    rn = mln_rbtree_search(ctx->lang->alias_set, &tmp);
    if (!mln_rbtree_null(rn, ctx->lang->alias_set)) {
        killed_ctx = (mln_lang_ctx_t *)mln_rbtree_node_data_get(rn);
        __mln_lang_job_free(killed_ctx);
    }
    if ((ret_var = mln_lang_var_create_nil(ctx, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return NULL;
    }
    return ret_var;
})


/*
 * mln_lang_gc_item_t
 */
MLN_FUNC(static inline, int, mln_lang_gc_item_new, \
         (mln_alloc_t *pool, mln_gc_t *gc, mln_lang_gc_type_t type, void *data), \
         (pool, gc, type, data), \
{
    mln_lang_gc_item_t *gi;
    if ((gi = (mln_lang_gc_item_t *)mln_alloc_m(pool, sizeof(mln_lang_gc_item_t))) == NULL) {
        return -1;
    }
    gi->gc = gc;
    gi->type = type;
    switch (type) {
        case M_GC_OBJ:
            gi->data.obj = (mln_lang_object_t *)data;
            gi->data.obj->gc_item = gi;
            break;
        default:
            gi->data.array = (mln_lang_array_t *)data;
            gi->data.array->gc_item = gi;
            break;
    }
    gi->gc_data = NULL;
    if (mln_gc_add(gc, gi) < 0) {
        mln_alloc_free(gi);
        return -1;
    }
    return 0;
})

MLN_FUNC_VOID(static, void, mln_lang_gc_item_free, (mln_lang_gc_item_t *gc_item), (gc_item), {
    if (gc_item == NULL) return;
    switch (gc_item->type) {
        case M_GC_OBJ:
            ASSERT(gc_item->data.obj->ref <= 1);
            gc_item->data.obj->gc_item = NULL;
            mln_lang_object_free(gc_item->data.obj);
            break;
        default:
            ASSERT(gc_item->data.array->ref <= 1);
            gc_item->data.array->gc_item = NULL;
            __mln_lang_array_free(gc_item->data.array);
            break;
    }
    mln_alloc_free(gc_item);
})

MLN_FUNC_VOID(static inline, void, mln_lang_gc_item_free_immediatly, \
              (mln_lang_gc_item_t *gc_item), (gc_item), \
{
    if (gc_item == NULL) return;
    mln_alloc_free(gc_item);
})

MLN_FUNC(static, void *, mln_lang_gc_item_getter, (mln_lang_gc_item_t *gc_item), (gc_item), {
    return gc_item->gc_data;
})

MLN_FUNC_VOID(static, void, mln_lang_gc_item_setter, \
              (mln_lang_gc_item_t *gc_item, void *gc_data), (gc_item, gc_data), \
{
    gc_item->gc_data = gc_data;
})

MLN_FUNC_VOID(static, void, mln_lang_gc_item_member_setter, \
              (mln_gc_t *gc, mln_lang_gc_item_t *gc_item), (gc, gc_item), \
{
    struct mln_lang_gc_setter_s lgs;
    struct mln_rbtree_attr rbattr;

    rbattr.pool = gc_item->type == M_GC_OBJ? gc_item->data.obj->ctx->pool: gc_item->data.array->ctx->pool;
    rbattr.pool_alloc = (rbtree_pool_alloc_handler)mln_alloc_m;
    rbattr.pool_free = (rbtree_pool_free_handler)mln_alloc_free;
    rbattr.cmp = mln_lang_gc_setter_cmp;
    rbattr.data_free = NULL;

    if ((lgs.visited = mln_rbtree_new(&rbattr)) == NULL) {
        mln_log(error, "No memory.\n");
        abort();
    }
    lgs.gc = gc;
    mln_lang_gc_item_member_setter_recursive(&lgs, gc_item);
    mln_rbtree_free(lgs.visited);
})

MLN_FUNC(static, int, mln_lang_gc_setter_cmp, \
         (const void *data1, const void *data2), (data1, data2), \
{
    return (mln_size_t)data1 - (mln_size_t)data2;
})

MLN_FUNC_VOID(static, void, mln_lang_gc_item_member_setter_recursive, \
              (struct mln_lang_gc_setter_s *lgs, mln_lang_gc_item_t *gc_item), \
              (lgs, gc_item), \
{
    mln_rbtree_t *t;
    mln_rbtree_node_t *node;

    node = mln_rbtree_search(lgs->visited, gc_item);
    if (!mln_rbtree_null(node, lgs->visited)) {
        return;
    }
    if ((node = mln_rbtree_node_new(lgs->visited, gc_item)) == NULL) {
        mln_log(error, "No memory.\n");
        abort();
    }
    mln_rbtree_insert(lgs->visited, node);

    switch (gc_item->type) {
        case M_GC_OBJ:
            t = gc_item->data.obj->members;
            mln_rbtree_iterate(t, mln_lang_gc_item_member_setter_obj_iterate_handler, lgs);
            break;
        default:
            t = gc_item->data.array->elems_index;
            mln_rbtree_iterate(t, mln_lang_gc_item_member_setter_array_iterate_handler, lgs);
            break;
    }
})

MLN_FUNC(static, int, mln_lang_gc_item_member_setter_obj_iterate_handler, \
         (mln_rbtree_node_t *node, void *udata), (node, udata), \
{
    mln_lang_val_t *val;
    mln_lang_var_t *var = (mln_lang_var_t *)mln_rbtree_node_data_get(node);
    struct mln_lang_gc_setter_s *lgs = (struct mln_lang_gc_setter_s *)udata;
    mln_s32_t type = mln_lang_var_val_type_get(var);
    val = mln_lang_var_val_get(var);
    if (type == M_LANG_VAL_TYPE_OBJECT) {
        mln_gc_collect_add(lgs->gc, val->data.obj->gc_item);
        mln_lang_gc_item_member_setter_recursive(lgs, val->data.obj->gc_item);
    } else if (type == M_LANG_VAL_TYPE_ARRAY) {
        mln_gc_collect_add(lgs->gc, val->data.array->gc_item);
        mln_lang_gc_item_member_setter_recursive(lgs, val->data.array->gc_item);
    }
    return 0;
})

MLN_FUNC(static, int, mln_lang_gc_item_member_setter_array_iterate_handler, \
         (mln_rbtree_node_t *node, void *udata), (node, udata), \
{
    mln_lang_val_t *val;
    mln_lang_array_elem_t *elem = (mln_lang_array_elem_t *)mln_rbtree_node_data_get(node);
    struct mln_lang_gc_setter_s *lgs = (struct mln_lang_gc_setter_s *)udata;
    mln_s32_t type;
    if (elem->key != NULL) {
        type = mln_lang_var_val_type_get(elem->key);
        val = mln_lang_var_val_get(elem->key);
        if (type == M_LANG_VAL_TYPE_OBJECT) {
            mln_gc_collect_add(lgs->gc, val->data.obj->gc_item);
            mln_lang_gc_item_member_setter_recursive(lgs, val->data.obj->gc_item);
        } else if (type == M_LANG_VAL_TYPE_ARRAY) {
            mln_gc_collect_add(lgs->gc, val->data.array->gc_item);
            mln_lang_gc_item_member_setter_recursive(lgs, val->data.array->gc_item);
        }
    }
    if (elem->value != NULL) {
        type = mln_lang_var_val_type_get(elem->value);
        val = mln_lang_var_val_get(elem->value);
        if (type == M_LANG_VAL_TYPE_OBJECT) {
            mln_gc_collect_add(lgs->gc, val->data.obj->gc_item);
            mln_lang_gc_item_member_setter_recursive(lgs, val->data.obj->gc_item);
        } else if (type == M_LANG_VAL_TYPE_ARRAY) {
            mln_gc_collect_add(lgs->gc, val->data.array->gc_item);
            mln_lang_gc_item_member_setter_recursive(lgs, val->data.array->gc_item);
        }
    }
    return 0;
})

MLN_FUNC_VOID(static, void, mln_lang_gc_item_move_handler, \
              (mln_gc_t *dest_gc, mln_lang_gc_item_t *gc_item), (dest_gc, gc_item), \
{
    gc_item->gc = dest_gc;
})

MLN_FUNC_VOID(static, void, mln_lang_gc_item_root_setter, \
              (mln_gc_t *gc, mln_lang_ctx_t *ctx), (gc, ctx), \
{
    if (ctx == NULL) return;
    mln_s32_t type;
    mln_lang_val_t *val;
    mln_lang_hash_bucket_t *hb, *end;
    mln_lang_symbol_node_t *sym;

    for (hb = ctx->symbols->bucket, end = ctx->symbols->bucket+ctx->symbols->len; hb < end; ++hb) {
        for (sym = hb->tail; sym != NULL; sym = sym->prev) {
            if (sym->type != M_LANG_SYMBOL_VAR) continue;
            type = mln_lang_var_val_type_get(sym->data.var);
            val = mln_lang_var_val_get(sym->data.var);
            if (type == M_LANG_VAL_TYPE_OBJECT) {
                mln_gc_collect_add(gc, val->data.obj->gc_item);
            } else if (type == M_LANG_VAL_TYPE_ARRAY) {
                mln_gc_collect_add(gc, val->data.array->gc_item);
            }
        }
    }
})

MLN_FUNC_VOID(static, void, mln_lang_gc_item_clean_searcher, \
              (mln_gc_t *gc, mln_lang_gc_item_t *gc_item), (gc, gc_item), \
{
    mln_rbtree_t *t;
    struct mln_lang_gc_scan_s gs;
    switch (gc_item->type) {
        case M_GC_OBJ:
            t = gc_item->data.obj->members;
            gs.tree = t;
            gs.gc = gc;
            mln_rbtree_iterate(t, mln_lang_gc_item_clean_searcher_obj_iterate_handler, &gs);
            break;
        default:
            t = gc_item->data.array->elems_index;
            gs.tree = t;
            gs.gc = gc;
            mln_rbtree_iterate(t, mln_lang_gc_item_clean_searcher_array_iterate_handler, &gs);
            break;
    }
})

MLN_FUNC(static, int, mln_lang_gc_item_clean_searcher_obj_iterate_handler, \
         (mln_rbtree_node_t *node, void *udata), (node, udata), \
{
    mln_lang_val_t *val;
    mln_lang_var_t *var = (mln_lang_var_t *)mln_rbtree_node_data_get(node);
    struct mln_lang_gc_scan_s *gs = (struct mln_lang_gc_scan_s *)udata;
    mln_s32_t type = mln_lang_var_val_type_get(var);
    val = mln_lang_var_val_get(var);
    if (type == M_LANG_VAL_TYPE_OBJECT) {
        if (mln_gc_clean_add(gs->gc, val->data.obj->gc_item) < 0) {
            if (val->data.obj->ref > 0) {
                --(val->data.obj->ref);
            }
            val->data.obj = NULL;
            mln_rbtree_delete(gs->tree, node);
            mln_rbtree_node_free(gs->tree, node);
        }
    } else if (type == M_LANG_VAL_TYPE_ARRAY) {
        if (mln_gc_clean_add(gs->gc, val->data.array->gc_item) < 0) {
            if (val->data.array->ref > 0) {
                --(val->data.array->ref);
            }
            val->data.array = NULL;
            mln_rbtree_delete(gs->tree, node);
            mln_rbtree_node_free(gs->tree, node);
        }
    }
    return 0;
})

MLN_FUNC(static, int, mln_lang_gc_item_clean_searcher_array_iterate_handler, \
         (mln_rbtree_node_t *node, void *udata), (node, udata), \
{
    mln_lang_val_t *val;
    mln_lang_array_elem_t *elem = (mln_lang_array_elem_t *)mln_rbtree_node_data_get(node);
    struct mln_lang_gc_scan_s *gs = (struct mln_lang_gc_scan_s *)udata;
    mln_s32_t type;
    int need_to_free = 0;

    if (elem->key != NULL) {
        type = mln_lang_var_val_type_get(elem->key);
        val = mln_lang_var_val_get(elem->key);
        if (type == M_LANG_VAL_TYPE_OBJECT) {
            if (mln_gc_clean_add(gs->gc, val->data.obj->gc_item) < 0) {
                if (val->data.obj->ref > 0) {
                    --(val->data.obj->ref);
                }
                val->data.obj = NULL;
                need_to_free = 1;
            }
        } else if (type == M_LANG_VAL_TYPE_ARRAY) {
            if (mln_gc_clean_add(gs->gc, val->data.array->gc_item) < 0) {
                if (val->data.array->ref > 0) {
                    --(val->data.array->ref);
                }
                val->data.array = NULL;
                need_to_free = 1;
            }
        }
    }
    if (elem->value != NULL) {
        type = mln_lang_var_val_type_get(elem->value);
        val = mln_lang_var_val_get(elem->value);
        if (type == M_LANG_VAL_TYPE_OBJECT) {
            if (mln_gc_clean_add(gs->gc, val->data.obj->gc_item) < 0) {
                if (val->data.obj->ref > 0) {
                    --(val->data.obj->ref);
                }
                val->data.obj = NULL;
                need_to_free = 1;
            }
        } else if (type == M_LANG_VAL_TYPE_ARRAY) {
            if (mln_gc_clean_add(gs->gc, val->data.array->gc_item) < 0) {
                if (val->data.array->ref > 0) {
                    --(val->data.array->ref);
                }
                val->data.array = NULL;
                need_to_free = 1;
            }
        }
    }
    if (need_to_free) {
        mln_rbtree_delete(gs->tree, node);
        mln_rbtree_node_free(gs->tree, node);
    }
    return 0;
})

MLN_FUNC_VOID(static, void, mln_lang_gc_item_free_handler, (mln_lang_gc_item_t *gc_item), (gc_item), {
    gc_item->gc = NULL;
})


MLN_FUNC(, int, mln_lang_ctx_resource_register, \
         (mln_lang_ctx_t *ctx, char *name, void *data, mln_lang_resource_free free_handler), \
         (ctx, name, data, free_handler), \
{
    mln_rbtree_node_t *rn;
    mln_string_t tmp;
    mln_lang_resource_t *lr = (mln_lang_resource_t *)mln_alloc_m(ctx->pool, sizeof(mln_lang_resource_t));
    if (lr == NULL) return -1;
    mln_string_set(&tmp, name);
    if ((lr->name = mln_string_pool_dup(ctx->pool, &tmp)) == NULL) {
        mln_alloc_free(lr);
        return -1;
    }
    lr->data = data;
    lr->free_handler = free_handler;
    if ((rn = mln_rbtree_node_new(ctx->resource_set, lr)) == NULL) {
        mln_lang_ctx_resource_free_handler(lr);
        return -1;
    }
    mln_rbtree_insert(ctx->resource_set, rn);
    return 0;
})

MLN_FUNC_VOID(static, void, mln_lang_ctx_resource_free_handler, (mln_lang_resource_t *lr), (lr), {
    if (lr == NULL) return;
    if (lr->free_handler != NULL) lr->free_handler(lr->data);
    mln_string_free(lr->name);
    mln_alloc_free(lr);
})

MLN_FUNC(, void *, mln_lang_ctx_resource_fetch, (mln_lang_ctx_t *ctx, char *name), (ctx, name), {
    mln_rbtree_node_t *rn;
    mln_lang_resource_t lr;
    mln_string_t s;

    mln_string_set(&s, name);
    lr.name = &s;
    rn = mln_rbtree_search(ctx->resource_set, &lr);
    if (mln_rbtree_null(rn, ctx->resource_set)) return NULL;
    return ((mln_lang_resource_t *)mln_rbtree_node_data_get(rn))->data;
})


MLN_FUNC(static, int, mln_lang_resource_cmp, \
         (const mln_lang_resource_t *lr1, const mln_lang_resource_t *lr2), (lr1, lr2), \
{
    return mln_string_strcmp(lr1->name, lr2->name);
})

MLN_FUNC(, int, mln_lang_resource_register, \
         (mln_lang_t *lang, char *name, void *data, mln_lang_resource_free free_handler), \
         (lang, name, data, free_handler), \
{
    mln_rbtree_node_t *rn;
    mln_string_t tmp;
    mln_lang_resource_t *lr = (mln_lang_resource_t *)malloc(sizeof(mln_lang_resource_t));
    if (lr == NULL) return -1;
    mln_string_set(&tmp, name);
    if ((lr->name = mln_string_dup(&tmp)) == NULL) {
        free(lr);
        return -1;
    }
    lr->data = data;
    lr->free_handler = free_handler;
    if ((rn = mln_rbtree_node_new(lang->resource_set, lr)) == NULL) {
        mln_lang_resource_free_handler(lr);
        return -1;
    }
    mln_rbtree_insert(lang->resource_set, rn);
    return 0;
})

MLN_FUNC_VOID(static, void, mln_lang_resource_free_handler, (mln_lang_resource_t *lr), (lr), {
    if (lr == NULL) return;
    if (lr->free_handler != NULL) lr->free_handler(lr->data);
    mln_string_free(lr->name);
    free(lr);
})

MLN_FUNC_VOID(, void, mln_lang_resource_cancel, (mln_lang_t *lang, char *name), (lang, name), {
    mln_rbtree_node_t *rn;
    mln_lang_resource_t lr;
    mln_string_t s;

    mln_string_set(&s, name);
    lr.name = &s;
    rn = mln_rbtree_search(lang->resource_set, &lr);
    if (mln_rbtree_null(rn, lang->resource_set)) return;
    mln_rbtree_delete(lang->resource_set, rn);
    mln_rbtree_node_free(lang->resource_set, rn);
})

MLN_FUNC(, void *, mln_lang_resource_fetch, (mln_lang_t *lang, char *name), (lang, name), {
    mln_rbtree_node_t *rn;
    mln_lang_resource_t lr;
    mln_string_t s;

    mln_string_set(&s, name);
    lr.name = &s;
    rn = mln_rbtree_search(lang->resource_set, &lr);
    if (mln_rbtree_null(rn, lang->resource_set)) return NULL;
    return ((mln_lang_resource_t *)mln_rbtree_node_data_get(rn))->data;
})

/*
 * hash table for melang
 */
MLN_FUNC(static inline, mln_lang_hash_t *, mln_lang_hash_new, (mln_lang_ctx_t *ctx), (ctx), {
    mln_lang_hash_t *h = (mln_lang_hash_t *)mln_alloc_m(ctx->pool, sizeof(mln_lang_hash_t));
    if (h == NULL) return NULL;
    if ((h->bucket = (mln_lang_hash_bucket_t *)mln_alloc_c(ctx->pool, sizeof(mln_lang_hash_bucket_t)*M_LANG_SYMBOL_TABLE_LEN)) == NULL) {
         mln_alloc_free(h);
         return NULL;
    }
    h->len = M_LANG_SYMBOL_TABLE_LEN;
    return h;
})

MLN_FUNC_VOID(static inline, void, mln_lang_hash_free, (mln_lang_hash_t *h), (h), {
    mln_lang_hash_bucket_t *hb, *end;
    mln_lang_symbol_node_t *sym;

    for (hb = h->bucket, end = h->bucket+h->len; hb < end; ++hb) {
        while ((sym = hb->tail) != NULL) {
            mln_lang_sym_chain_del(&hb->head, &hb->tail, sym);
            sym->ctx = NULL;
            mln_lang_symbol_node_free(sym);
        }
    }
    mln_alloc_free(h->bucket);
    mln_alloc_free(h);
})

MLN_FUNC(static inline, mln_lang_hash_bucket_t *, mln_lang_hash_get_bucket, \
         (mln_lang_hash_t *h, mln_lang_symbol_node_t *sym), (h, sym), \
{
    mln_u64_t idx = 0, i, n = sym->symbol->len;
    mln_u8ptr_t p = sym->symbol->data;
    for (i = 0; i < n; ++i) {
        idx += p[i];
    }
    return &h->bucket[(idx * sym->layer) % h->len];
})

/*
 * pipe for communication between C code and script
 */
static mln_lang_ctx_pipe_t *mln_lang_ctx_pipe_new(mln_lang_ctx_t *ctx)
{
    mln_lang_ctx_pipe_t *p;

    if ((p = (mln_lang_ctx_pipe_t *)mln_alloc_m(ctx->pool, sizeof(mln_lang_ctx_pipe_t))) == NULL) {
        return NULL;
    }
#if !defined(MSVC)
    if (pthread_mutex_init(&p->lock, NULL)) {
        mln_alloc_free(p);
        return NULL;
    }
#endif
    p->ctx = ctx;
    if (mln_array_pool_init(&p->list, \
                            (array_free)mln_array_destroy, \
                            sizeof(mln_array_t), \
                            MLN_LANG_PIPE_LIST_NALLOC, \
                            ctx->pool, \
                            (array_pool_alloc_handler)mln_alloc_m, \
                            (array_pool_free_handler)mln_alloc_free) < 0)
    {
#if !defined(MSVC)
        pthread_mutex_destroy(&p->lock);
#endif
        mln_alloc_free(p);
        return NULL;
    }
    p->recv_handler = NULL;
    p->subscribed = 0;

    return p;
}

static void mln_lang_ctx_pipe_free(mln_lang_ctx_pipe_t *p)
{
    if (p == NULL) return;

#if !defined(MSVC)
    pthread_mutex_lock(&p->lock);
#endif

    mln_array_destroy(&p->list);

#if !defined(MSVC)
    pthread_mutex_unlock(&p->lock);

    pthread_mutex_destroy(&p->lock);
#endif
    mln_alloc_free(p);
}

MLN_FUNC(static inline, int, mln_lang_ctx_pipe_elem_init, \
         (mln_lang_ctx_pipe_elem_t *pe, int type, void *value), (pe, type, value), \
{
    pe->type = type;
    switch(type) {
        case M_LANG_VAL_TYPE_INT:
            pe->data.i = *((mln_s64_t *)value);
            break;
        case M_LANG_VAL_TYPE_REAL:
            pe->data.r = *((double *)value);
            break;
        case M_LANG_VAL_TYPE_STRING:
            pe->data.s = mln_string_dup((mln_string_t *)value);
            if (pe->data.s == NULL) return -1;
            break;
        default:
            return -1;
    }

    return 0;
})

MLN_FUNC_VOID(static, void, mln_lang_ctx_pipe_elem_destroy, (mln_lang_ctx_pipe_elem_t *pe), (pe), {
    if (pe != NULL && pe->type == M_LANG_VAL_TYPE_STRING && pe->data.s != NULL)
        mln_string_free(pe->data.s);
})

MLN_FUNC(static, int, mln_lang_func_pipe, (mln_lang_ctx_t *ctx), (ctx), {
    mln_lang_val_t *val;
    mln_lang_var_t *var;
    mln_lang_func_detail_t *func;
    mln_string_t funcname = mln_string("Pipe");
    mln_string_t v1 = mln_string("op");
    mln_string_t v2 = mln_string("data");
    if ((func = mln_lang_func_detail_new(ctx, M_FUNC_INTERNAL, (void *)mln_lang_func_pipe_process, NULL, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if ((val = mln_lang_val_new(ctx, M_LANG_VAL_TYPE_NIL, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_func_detail_free(func);
        return -1;
    }
    if ((var = mln_lang_var_new(ctx, &v1, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_val_free(val);
        mln_lang_func_detail_free(func);
        return -1;
    }
    mln_lang_func_detail_arg_append(func, var);
    if ((val = mln_lang_val_new(ctx, M_LANG_VAL_TYPE_NIL, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_func_detail_free(func);
        return -1;
    }
    if ((var = mln_lang_var_new(ctx, &v2, M_LANG_VAR_REFER, val, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_val_free(val);
        mln_lang_func_detail_free(func);
        return -1;
    }
    mln_lang_func_detail_arg_append(func, var);
    if ((val = mln_lang_val_new(ctx, M_LANG_VAL_TYPE_FUNC, func)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_func_detail_free(func);
        return -1;
    }
    if ((var = mln_lang_var_new(ctx, &funcname, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_val_free(val);
        return -1;
    }
    if (mln_lang_symbol_node_join(ctx, M_LANG_SYMBOL_VAR, var) < 0) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_var_free(var);
        return -1;
    }
    return 0;
})

static mln_lang_var_t *mln_lang_func_pipe_process(mln_lang_ctx_t *ctx)
{
    mln_lang_var_t *ret_var = NULL;
    mln_lang_var_t *var = NULL;
    mln_string_t v1 = mln_string("op");
    mln_string_t v2 = mln_string("data");
    mln_string_t op_sub = mln_string("subscribe");
    mln_string_t op_unsub = mln_string("unsubscribe");
    mln_string_t op_recv = mln_string("recv");
    mln_string_t op_send = mln_string("send");
    mln_lang_symbol_node_t *sym;
    mln_lang_ctx_pipe_t *p;
    mln_lang_ctx_pipe_recv_cb_t cb;
    mln_string_t *op;
    int rc = 0;

    /*arg1*/
    if ((sym = mln_lang_symbol_node_search(ctx, &v1, 1)) == NULL) {
        ASSERT(0);
        mln_lang_errmsg(ctx, "Argument 1 missing.");
        return NULL;
    }
    if (sym->type != M_LANG_SYMBOL_VAR || mln_lang_var_val_type_get(sym->data.var) != M_LANG_VAL_TYPE_STRING) {
        mln_lang_errmsg(ctx, "Invalid type of argument 1.");
        return NULL;
    }
    op = mln_lang_var_val_get(sym->data.var)->data.s;

    /*arg2*/
    if ((sym = mln_lang_symbol_node_search(ctx, &v2, 1)) == NULL) {
        ASSERT(0);
        mln_lang_errmsg(ctx, "Argument 2 missing.");
        return NULL;
    }
    if (sym->type != M_LANG_SYMBOL_VAR) {
        mln_lang_errmsg(ctx, "Invalid type of argument 2.");
        return NULL;
    }
    var = sym->data.var;

    if (!mln_string_strcmp(op, &op_sub)) {
        if ((p = (mln_lang_ctx_pipe_t *)mln_lang_ctx_resource_fetch(ctx, "pipe")) == NULL) {
            if ((p = mln_lang_ctx_pipe_new(ctx)) == NULL) {
                mln_lang_errmsg(ctx, "No memory.");
                return NULL;
            }
            if (mln_lang_ctx_resource_register(ctx, "pipe", p, (mln_lang_resource_free)mln_lang_ctx_pipe_free) < 0) {
                mln_lang_ctx_pipe_free(p);
                mln_lang_errmsg(ctx, "No memory.");
                return NULL;
            }
        }
#if !defined(MSVC)
        pthread_mutex_lock(&p->lock);
#endif
        p->subscribed = 1;
#if !defined(MSVC)
        pthread_mutex_unlock(&p->lock);
#endif
        ret_var = mln_lang_var_create_nil(ctx, NULL);
    } else if (!mln_string_strcmp(op, &op_unsub)) {
        if ((p = (mln_lang_ctx_pipe_t *)mln_lang_ctx_resource_fetch(ctx, "pipe")) != NULL) {
#if !defined(MSVC)
            pthread_mutex_lock(&p->lock);
#endif
            p->subscribed = 0;
#if !defined(MSVC)
            pthread_mutex_unlock(&p->lock);
#endif
        }
        ret_var = mln_lang_var_create_nil(ctx, NULL);
    } else if (!mln_string_strcmp(op, &op_recv)) {
        if ((p = (mln_lang_ctx_pipe_t *)mln_lang_ctx_resource_fetch(ctx, "pipe")) != NULL) {
#if !defined(MSVC)
            pthread_mutex_lock(&p->lock);
#endif
            ret_var = mln_lang_func_pipe_process_array_generate(ctx, p);
#if !defined(MSVC)
            pthread_mutex_unlock(&p->lock);
#endif
        } else {
            ret_var = mln_lang_var_create_false(ctx, NULL);
        }
    } else if (!mln_string_strcmp(op, &op_send)) {
        if ((p = (mln_lang_ctx_pipe_t *)mln_lang_ctx_resource_fetch(ctx, "pipe")) != NULL) {
#if !defined(MSVC)
            pthread_mutex_lock(&p->lock);
#endif
            cb = p->recv_handler;
#if !defined(MSVC)
            pthread_mutex_unlock(&p->lock);
#endif
            if (cb != NULL) {
                rc = cb(ctx, mln_lang_var_val_get(var));
            }
            ret_var = !rc? mln_lang_var_create_true(ctx, NULL): mln_lang_var_create_false(ctx, NULL);
        } else {
            ret_var = mln_lang_var_create_false(ctx, NULL);
        }
    } else {
        mln_lang_errmsg(ctx, "Invalid argument 2.");
        return NULL;
    }

    if (ret_var == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
    }
    return ret_var;
}

MLN_FUNC(static inline, mln_lang_var_t *, mln_lang_func_pipe_process_array_generate, \
         (mln_lang_ctx_t *ctx, mln_lang_ctx_pipe_t *pipe), (ctx, pipe), \
{
    mln_lang_var_t *ret_var, *var, *v;
    mln_lang_array_t *arr, *a;
    mln_array_t *pa, *paend;
    mln_lang_ctx_pipe_elem_t *pe, *peend;

    if ((ret_var = mln_lang_var_create_array(ctx, NULL)) == NULL) {
        return NULL;
    }
    arr = mln_lang_var_val_get(ret_var)->data.array;

    pa = paend = (mln_array_t *)mln_array_elts(&pipe->list);
    paend += mln_array_nelts(&pipe->list);
    for (; pa < paend; ++pa) {
        if ((var = mln_lang_var_create_array(ctx, NULL)) == NULL) {
            mln_lang_var_free(ret_var);
            return NULL;
        }
        if ((v = mln_lang_array_get(ctx, arr, NULL)) == NULL) {
            mln_lang_var_free(ret_var);
            mln_lang_var_free(var);
            return NULL;
        }
        mln_lang_var_assign(v, mln_lang_var_val_get(var));
        mln_lang_var_free(var);
        a = mln_lang_var_val_get(v)->data.array;

        pe = peend = (mln_lang_ctx_pipe_elem_t *)mln_array_elts(pa);
        peend += mln_array_nelts(pa);
        for (; pe < peend; ++pe) {
            if ((v = mln_lang_array_get(ctx, a, NULL)) == NULL) {
                mln_lang_var_free(ret_var);
                return NULL;
            }
            switch (pe->type) {
                case M_LANG_VAL_TYPE_INT:
                    mln_lang_var_set_int(v, pe->data.i);
                    break;
                case M_LANG_VAL_TYPE_REAL:
                    mln_lang_var_set_real(v, pe->data.r);
                    break;
                case M_LANG_VAL_TYPE_STRING:
                {
                    mln_string_t *s = mln_string_pool_dup(ctx->pool, pe->data.s);
                    if (s == NULL) {
                        mln_lang_var_free(ret_var);
                        return NULL;
                    }
                    mln_lang_var_set_string(v, s);
                    break;
                }
                default:
                    mln_lang_var_free(ret_var);
                    return NULL;
            }
        }
    }
    mln_array_reset(&pipe->list);

    return ret_var;
})

int mln_lang_ctx_pipe_send(mln_lang_ctx_t *ctx, char *fmt, ...)
{
    int rc;
    va_list arg;

    va_start(arg, fmt);
    rc = mln_lang_ctx_pipe_do_send(ctx, fmt, arg);
    va_end(arg);

    return rc;
}

static inline int mln_lang_ctx_pipe_do_send(mln_lang_ctx_t *ctx, char *fmt, va_list arg)
{
    mln_lang_ctx_pipe_t *p;
    mln_lang_ctx_pipe_elem_t *pe;
    mln_array_t *a = NULL;

    if ((p = (mln_lang_ctx_pipe_t *)mln_lang_ctx_resource_fetch(ctx, "pipe")) == NULL)
        return 0;

    if (p->ctx != ctx) return -1;

#if !defined(MSVC)
    pthread_mutex_lock(&p->lock);
#endif
    if (!p->subscribed) {
#if !defined(MSVC)
        pthread_mutex_unlock(&p->lock);
#endif
        return -1;
    }

    if ((a = (mln_array_t *)mln_array_push(&p->list)) == NULL) {
#if !defined(MSVC)
        pthread_mutex_unlock(&p->lock);
#endif
        return -1;
    }
    if (mln_array_pool_init(a, \
                            (array_free)mln_lang_ctx_pipe_elem_destroy, \
                            sizeof(mln_lang_ctx_pipe_elem_t), \
                            MLN_LANG_PIPE_ELEM_NALLOC, \
                            ctx->pool, \
                            (array_pool_alloc_handler)mln_alloc_m, \
                            (array_pool_free_handler)mln_alloc_free) < 0)
    {
        mln_array_pop(&p->list);
#if !defined(MSVC)
        pthread_mutex_unlock(&p->lock);
#endif
        return -1;
    }

    while (*fmt) {
        if ((pe = (mln_lang_ctx_pipe_elem_t *)mln_array_push(a)) == NULL) {
            mln_array_pop(&p->list);
#if !defined(MSVC)
            pthread_mutex_unlock(&p->lock);
#endif
            return -1;
        }
        pe->data.s= NULL;
        switch(*fmt) {
            case 'i':
            {
                mln_s64_t tmp = va_arg(arg, mln_s64_t);
                mln_lang_ctx_pipe_elem_init(pe, M_LANG_VAL_TYPE_INT, &tmp);
                break;
            }
            case 's':
            {
                char *str = va_arg(arg, char *);
                mln_string_t tmp;
                mln_string_set(&tmp, str);
                if (mln_lang_ctx_pipe_elem_init(pe, M_LANG_VAL_TYPE_STRING, &tmp) < 0) {
                    mln_array_pop(&p->list);
#if !defined(MSVC)
                    pthread_mutex_unlock(&p->lock);
#endif
                    return -1;
                }
                break;
            }
            case 'S':
            {
                mln_string_t *tmp = va_arg(arg, mln_string_t *);
                if (mln_lang_ctx_pipe_elem_init(pe, M_LANG_VAL_TYPE_STRING, tmp) < 0) {
                    mln_array_pop(&p->list);
#if !defined(MSVC)
                    pthread_mutex_unlock(&p->lock);
#endif
                    return -1;
                }
                break;
            }
            case 'r':
            {
                double tmp = va_arg(arg, double);
                mln_lang_ctx_pipe_elem_init(pe, M_LANG_VAL_TYPE_REAL, &tmp);
                break;
            }
            default:
                mln_array_pop(&p->list);
#if !defined(MSVC)
                pthread_mutex_unlock(&p->lock);
#endif
                return -1;
        }
        ++fmt;
    }
#if !defined(MSVC)
    pthread_mutex_unlock(&p->lock);
#endif

    return 0;

}

int mln_lang_ctx_pipe_recv_handler_set(mln_lang_ctx_t *ctx, mln_lang_ctx_pipe_recv_cb_t recv_handler)
{
    mln_lang_ctx_pipe_t *p;

    if ((p = (mln_lang_ctx_pipe_t *)mln_lang_ctx_resource_fetch(ctx, "pipe")) == NULL) {
        if ((p = mln_lang_ctx_pipe_new(ctx)) == NULL) {
            return -1;
        }
        if (mln_lang_ctx_resource_register(ctx, "pipe", p, (mln_lang_resource_free)mln_lang_ctx_pipe_free) < 0) {
            mln_lang_ctx_pipe_free(p);
            return -1;
        }
    }

    if (p->ctx != ctx) return -1;

#if !defined(MSVC)
    pthread_mutex_lock(&p->lock);
#endif
    p->recv_handler = recv_handler;
#if !defined(MSVC)
    pthread_mutex_unlock(&p->lock);
#endif

    return 0;
}

