
/*
 * Copyright (C) Niklaus F.Schen.
 */
#include "mln_lang.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <netdb.h>
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
#include "mln_melang.h"

#ifdef __DEBUG__
#include <assert.h>
#define ASSERT(x) assert(x)
#else
#define ASSERT(x);
#endif

struct mln_lang_scan_s {
    mln_rbtree_t     *tree;
    mln_rbtree_t     *tree2;
    mln_lang_ctx_t   *ctx;
};

struct mln_lang_gc_scan_s {
    mln_rbtree_t     *tree;
    mln_gc_t         *gc;
};

MLN_CHAIN_FUNC_DECLARE(mln_lang_ctx, \
                       mln_lang_ctx_t, \
                       static inline void, \
                       __NONNULL3(1,2,3));
MLN_CHAIN_FUNC_DEFINE(mln_lang_ctx, \
                      mln_lang_ctx_t, \
                      static inline void, \
                      prev, \
                      next);
MLN_CHAIN_FUNC_DECLARE(mln_lang_scope, \
                       mln_lang_scope_t, \
                       static inline void, \
                       __NONNULL3(1,2,3));
MLN_CHAIN_FUNC_DEFINE(mln_lang_scope, \
                      mln_lang_scope_t, \
                      static inline void, \
                      prev, \
                      next);
MLN_CHAIN_FUNC_DEFINE(mln_lang_var, \
                      mln_lang_var_t, \
                      void, \
                      prev, \
                      next);
static inline int __mln_lang_run(mln_lang_t *lang);
static void mln_lang_run_handler(mln_event_t *ev, int fd, void *data);
static inline mln_lang_ctx_t *
mln_lang_ctx_new(mln_lang_t *lang, void *data, mln_string_t *filename, mln_u32_t type, mln_string_t *content);
static inline void mln_lang_ctx_free(mln_lang_ctx_t *ctx);
static inline void mln_lang_ctx_resetRetExp(mln_lang_ctx_t *ctx);
static inline void
mln_lang_ctx_getRetExpFromNode(mln_lang_ctx_t *ctx, mln_lang_stack_node_t *node);
static inline mln_lang_set_detail_t *
mln_lang_ctx_getClass(mln_lang_ctx_t *ctx);
static inline mln_lang_retExp_t *
__mln_lang_retExp_new(mln_alloc_t *pool, mln_lang_retExp_type_t type, void *data);
static inline void __mln_lang_retExp_free(mln_lang_retExp_t *retExp);
static inline mln_lang_retExp_t *
__mln_lang_retExp_createTmpNil(mln_alloc_t *pool, mln_string_t *name);
static inline mln_lang_retExp_t *
__mln_lang_retExp_createTmpObj(mln_lang_ctx_t *ctx, mln_lang_set_detail_t *inSet, mln_string_t *name);
static inline mln_lang_retExp_t *
__mln_lang_retExp_createTmpTrue(mln_alloc_t *pool, mln_string_t *name);
static inline mln_lang_stack_node_t *
mln_lang_stack_node_new(mln_alloc_t *pool, mln_lang_stack_node_type_t type, void *data);
static inline void __mln_lang_stack_node_free(void *data);
static inline void
mln_lang_stack_node_getRetExpFromCTX(mln_lang_stack_node_t *node, mln_lang_ctx_t *ctx);
static inline void
mln_lang_stack_node_resetRetExp(mln_lang_stack_node_t *node);
static inline void
mln_lang_stack_node_setRetExp(mln_lang_stack_node_t *node, mln_lang_retExp_t *retExp);
static inline mln_lang_scope_t *
mln_lang_scope_new(mln_lang_ctx_t *ctx, \
                   mln_string_t *name, \
                   mln_lang_scope_type_t type, \
                   mln_lang_stack_node_t *cur_stack);
static void mln_lang_scope_free(mln_lang_scope_t *scope);
static inline mln_gc_t *mln_lang_scope_getCurrentGC(mln_lang_ctx_t *ctx);
static int mln_lang_symbolNode_cmp(const void *data1, const void *data2);
static inline mln_lang_symbolNode_t *
mln_lang_symbolNode_new(mln_lang_ctx_t *ctx, mln_string_t *symbol, mln_lang_symbolType_t type, void *data);
static void mln_lang_symbolNode_free(void *data);
static inline int __mln_lang_symbolNode_join(mln_lang_ctx_t *ctx, mln_lang_symbolType_t type, void *data);
static inline mln_lang_symbolNode_t *
__mln_lang_symbolNode_search(mln_lang_ctx_t *ctx, mln_string_t *name, int local);
static inline mln_lang_symbolNode_t *
mln_lang_symbolNode_idSearch(mln_lang_ctx_t *ctx, mln_string_t *name);
static inline mln_lang_label_t *
mln_lang_label_new(mln_alloc_t *pool, mln_string_t *label, mln_lang_stm_t *stm);
static inline void mln_lang_label_free(mln_lang_label_t *label);
static inline mln_lang_var_t *
__mln_lang_set_member_search(mln_rbtree_t *members, mln_string_t *name);
static int mln_lang_set_member_scan(mln_rbtree_node_t *node, void *rn_data, void *udata);
static inline mln_lang_var_t *
__mln_lang_var_new(mln_alloc_t *pool, \
                 mln_string_t *name, \
                 mln_lang_var_type_t type, \
                 mln_lang_val_t *val, \
                 mln_lang_set_detail_t *inSet);
static inline void __mln_lang_var_free(void *data);
static inline mln_lang_var_t *
__mln_lang_var_dup(mln_lang_ctx_t *ctx, mln_lang_var_t *var);
static inline mln_lang_var_t *
mln_lang_var_dupWithVal(mln_lang_ctx_t *ctx, mln_lang_var_t *var);
static int mln_lang_var_cmpForElemCmp(const void *data1, const void *data2);
static int mln_lang_var_cmpName(const void *data1, const void *data2);
static inline mln_lang_var_t *
mln_lang_var_transform(mln_lang_ctx_t *ctx, mln_lang_var_t *realvar, mln_lang_var_t *defvar);
static inline mln_lang_var_t *
__mln_lang_var_convert(mln_alloc_t *pool, mln_lang_var_t *var);
static inline void __mln_lang_var_assign(mln_lang_var_t *var, mln_lang_val_t *val);
static inline mln_s32_t __mln_lang_var_getValType(mln_lang_var_t *var);
static inline int
__mln_lang_var_setValue(mln_lang_ctx_t *ctx, mln_lang_var_t *dest, mln_lang_var_t *src);
static inline int
mln_lang_funcdef_getArgs(mln_alloc_t *ctx, \
                         mln_lang_exp_t *exp, \
                         mln_lang_var_t **head, \
                         mln_lang_var_t **tail, \
                         mln_size_t *nargs);
static inline mln_lang_func_detail_t *
__mln_lang_func_detail_new(mln_alloc_t *pool, \
                         mln_lang_funcType_t type, \
                         void *data, \
                         mln_lang_exp_t *exp);
static inline void __mln_lang_func_detail_free(mln_lang_func_detail_t *lfd);
static inline mln_lang_func_detail_t *
mln_lang_func_detail_dup(mln_lang_ctx_t *ctx, mln_lang_func_detail_t *func);
static inline mln_lang_object_t *
mln_lang_object_new(mln_lang_ctx_t *ctx, mln_lang_set_detail_t *inSet);
static inline void mln_lang_object_free(mln_lang_object_t *obj);
static inline mln_lang_object_t *
mln_lang_object_dup(mln_lang_ctx_t *ctx, mln_lang_object_t *src);
static inline mln_lang_val_t *
__mln_lang_val_new(mln_alloc_t *pool, mln_s32_t type, void *data);
static void __mln_lang_val_free(void *data);
static inline void mln_lang_val_freeData(mln_lang_val_t *val);
static inline mln_lang_val_t *
mln_lang_val_dup(mln_lang_ctx_t *ctx, mln_lang_val_t *val);
static int mln_lang_val_cmp(const void *data1, const void *data2);
static inline mln_lang_array_t *__mln_lang_array_new(mln_lang_ctx_t *ctx);
static inline void __mln_lang_array_free(mln_lang_array_t *array);
static inline mln_lang_array_t *
mln_lang_array_dup(mln_lang_ctx_t *ctx, mln_lang_array_t *array);
static int mln_lang_array_scan_dup(mln_rbtree_node_t *node, void *rn_data, void *udata);
static int mln_lang_array_elem_indexCmp(const void *data1, const void *data2);
static int mln_lang_array_elem_keyCmp(const void *data1, const void *data2);
static inline mln_lang_array_elem_t *
mln_lang_array_elem_new(mln_alloc_t *pool, mln_lang_var_t *key, mln_lang_var_t *val, mln_u64_t index);
static inline void mln_lang_array_elem_free(void *data);
static inline mln_lang_array_elem_t *
mln_lang_array_elem_dup(mln_lang_ctx_t *ctx, mln_lang_array_elem_t *lae);
static inline mln_lang_var_t *
__mln_lang_array_getAndNew(mln_lang_ctx_t *ctx, mln_lang_array_t *array, mln_lang_var_t *key);
static inline mln_lang_var_t *
mln_lang_array_getAndNew_int(mln_lang_ctx_t *ctx, mln_lang_array_t *array, mln_lang_var_t *key);
static inline mln_lang_var_t *
mln_lang_array_getAndNew_other(mln_lang_ctx_t *ctx, mln_lang_array_t *array, mln_lang_var_t *key);
static inline mln_lang_var_t *
mln_lang_array_getAndNew_nil(mln_lang_ctx_t *ctx, mln_lang_array_t *array);
static inline mln_lang_funccall_val_t *__mln_lang_funccall_val_new(mln_alloc_t *pool, mln_string_t *name);
static inline void __mln_lang_funccall_val_free(mln_lang_funccall_val_t *func);
static inline void mln_lang_funccall_val_addArg(mln_lang_funccall_val_t *func, mln_lang_var_t *var);
static void __mln_lang_errmsg(mln_lang_ctx_t *ctx, char *msg);
static void mln_lang_stack_handler_stm(mln_lang_ctx_t *ctx);
static void mln_lang_stack_handler_funcdef(mln_lang_ctx_t *ctx);
static void mln_lang_stack_handler_set(mln_lang_ctx_t *ctx);
static void mln_lang_stack_handler_setstm(mln_lang_ctx_t *ctx);
static void mln_lang_stack_handler_block(mln_lang_ctx_t *ctx);
static inline int mln_lang_metReturn(mln_lang_ctx_t *ctx);
static inline int mln_lang_stack_handler_block_exp(mln_lang_ctx_t *ctx, mln_lang_block_t *block);
static inline int mln_lang_stack_handler_block_stm(mln_lang_ctx_t *ctx, mln_lang_block_t *block);
static inline int mln_lang_stack_handler_block_continue(mln_lang_ctx_t *ctx, mln_lang_block_t *block);
static inline int mln_lang_stack_handler_block_break(mln_lang_ctx_t *ctx, mln_lang_block_t *block);
static inline int mln_lang_stack_handler_block_return(mln_lang_ctx_t *ctx, mln_lang_block_t *block);
static inline int mln_lang_stack_handler_block_goto(mln_lang_ctx_t *ctx, mln_lang_block_t *block);
static inline int mln_lang_stack_handler_block_if(mln_lang_ctx_t *ctx, mln_lang_block_t *block);
static inline mln_lang_stack_node_t *mln_lang_withdrawToLoop(mln_lang_ctx_t *ctx);
static inline mln_lang_stack_node_t *mln_lang_withdrawBreakLoop(mln_lang_ctx_t *ctx);
static inline int mln_lang_withdrawUntilFunc(mln_lang_ctx_t *ctx);
static void mln_lang_stack_handler_while(mln_lang_ctx_t *ctx);
static inline int __mln_lang_condition_isTrue(mln_lang_var_t *var);
static void mln_lang_stack_handler_switch(mln_lang_ctx_t *ctx);
static void mln_lang_stack_handler_switchstm(mln_lang_ctx_t *ctx);
static void mln_lang_stack_handler_for(mln_lang_ctx_t *ctx);
static void mln_lang_stack_handler_if(mln_lang_ctx_t *ctx);
static void mln_lang_stack_handler_exp(mln_lang_ctx_t *ctx);
static void mln_lang_stack_handler_assign(mln_lang_ctx_t *ctx);
static void mln_lang_stack_handler_logicLow(mln_lang_ctx_t *ctx);
static void mln_lang_stack_handler_logicHigh(mln_lang_ctx_t *ctx);
static void mln_lang_stack_handler_relativeLow(mln_lang_ctx_t *ctx);
static void mln_lang_stack_handler_relativeHigh(mln_lang_ctx_t *ctx);
static void mln_lang_stack_handler_move(mln_lang_ctx_t *ctx);
static void mln_lang_stack_handler_addsub(mln_lang_ctx_t *ctx);
static void mln_lang_stack_handler_muldiv(mln_lang_ctx_t *ctx);
static void mln_lang_stack_handler_suffix(mln_lang_ctx_t *ctx);
static void mln_lang_stack_handler_locate(mln_lang_ctx_t *ctx);
static int mln_lang_stack_handler_funccall_run(mln_lang_ctx_t *ctx, \
                                               mln_lang_stack_node_t *node, \
                                               mln_lang_funccall_val_t *funccall);
static void mln_lang_stack_handler_spec(mln_lang_ctx_t *ctx);
static inline int mln_lang_stack_handler_spec_new(mln_lang_ctx_t *ctx, mln_string_t *name);
static void mln_lang_stack_handler_factor(mln_lang_ctx_t *ctx);
static inline int
mln_lang_stack_handler_factor_bool(mln_lang_ctx_t *ctx, mln_lang_factor_t *factor);
static inline int
mln_lang_stack_handler_factor_string(mln_lang_ctx_t *ctx, mln_lang_factor_t *factor);
static inline int
mln_lang_stack_handler_factor_int(mln_lang_ctx_t *ctx, mln_lang_factor_t *factor);
static inline int
mln_lang_stack_handler_factor_real(mln_lang_ctx_t *ctx, mln_lang_factor_t *factor);
static inline int
mln_lang_stack_handler_factor_array(mln_lang_ctx_t *ctx, mln_lang_factor_t *factor);
static inline int
mln_lang_stack_handler_factor_id(mln_lang_ctx_t *ctx, mln_lang_factor_t *factor);
static inline int
mln_lang_stack_handler_factor_nil(mln_lang_ctx_t *ctx, mln_lang_factor_t *factor);
static void mln_lang_stack_handler_elemlist(mln_lang_ctx_t *ctx);
static void mln_lang_stack_handler_funccall(mln_lang_ctx_t *ctx);

static int mln_lang_dump_symbol(mln_rbtree_node_t *node, void *rn_data, void *udata);
static void mln_lang_dump_var(mln_lang_var_t *var, int cnt);
static void mln_lang_dump_set(mln_lang_set_detail_t *set);
static void mln_lang_dump_object(mln_lang_object_t *obj, int cnt);
static int mln_lang_dump_var_scan(mln_rbtree_node_t *node, void *rn_data, void *udata);
static void mln_lang_dump_function(mln_lang_func_detail_t *func, int cnt);
static void mln_lang_dump_array(mln_lang_array_t *array, int cnt);
static int mln_lang_dump_array_elem(mln_rbtree_node_t *node, void *rn_data, void *udata);

static int mln_lang_msg_cmp(const void *data1, const void *data2);
static mln_lang_msg_t *__mln_lang_msg_new(mln_lang_ctx_t *ctx, mln_string_t *name);
static void __mln_lang_msg_free(void *data);
static void mln_lang_msg_scriptRecver(mln_event_t *ev, int fd, void *data);
static void mln_lang_msg_cRecver(mln_event_t *ev, int fd, void *data);
static int mln_lang_msg_func_new(mln_lang_ctx_t *ctx);
static int mln_lang_msg_func_free(mln_lang_ctx_t *ctx);
static int mln_lang_msg_func_recv(mln_lang_ctx_t *ctx);
static int mln_lang_msg_func_send(mln_lang_ctx_t *ctx);
static int mln_lang_msg_func_dump(mln_lang_ctx_t *ctx);
static int mln_lang_func_watch(mln_lang_ctx_t *ctx);
static int mln_lang_func_unwatch(mln_lang_ctx_t *ctx);
static int mln_lang_msg_installer(mln_lang_ctx_t *ctx);
static mln_lang_retExp_t *mln_lang_msg_func_new_process(mln_lang_ctx_t *ctx);
static mln_lang_retExp_t *mln_lang_msg_func_free_process(mln_lang_ctx_t *ctx);
static mln_lang_retExp_t *mln_lang_msg_func_recv_process(mln_lang_ctx_t *ctx);
static mln_lang_retExp_t *mln_lang_msg_func_send_process(mln_lang_ctx_t *ctx);
static mln_lang_retExp_t *mln_lang_msg_func_dump_process(mln_lang_ctx_t *ctx);
static mln_lang_retExp_t *mln_lang_func_watch_process(mln_lang_ctx_t *ctx);
static mln_lang_retExp_t *mln_lang_func_unwatch_process(mln_lang_ctx_t *ctx);
static int
mln_lang_watch_funcBuild(mln_lang_ctx_t *ctx, mln_lang_stack_node_t *node, mln_lang_func_detail_t *funcdef, mln_lang_val_t *udata, mln_lang_val_t *new);
static inline int
mln_lang_gc_item_new(mln_alloc_t *pool, mln_gc_t *gc, mln_lang_gcType_t type, void *data);
static void mln_lang_gc_item_free(mln_lang_gc_item_t *gcItem);
static inline void mln_lang_gc_item_freeImmediatly(mln_lang_gc_item_t *gcItem);
static void *mln_lang_gc_item_getter(mln_lang_gc_item_t *gcItem);
static void mln_lang_gc_item_setter(mln_lang_gc_item_t *gcItem, void *gcData);
static void mln_lang_gc_item_memberSetter(mln_gc_t *gc, mln_lang_gc_item_t *gcItem);
static int mln_lang_gc_item_memberSetter_objScanner(mln_rbtree_node_t *node, void *rn_data, void *udata);
static int mln_lang_gc_item_memberSetter_arrayScanner(mln_rbtree_node_t *node, void *rn_data, void *udata);
static void mln_lang_gc_item_moveHandler(mln_gc_t *destGC, mln_lang_gc_item_t *gcItem);
static void mln_lang_gc_item_rootSetter(mln_gc_t *gc, mln_lang_scope_t *scope);
static int mln_lang_gc_item_rootSetterScanner(mln_rbtree_node_t *node, void *rn_data, void *udata);
static void mln_lang_gc_item_cleanSearcher(mln_gc_t *gc, mln_lang_gc_item_t *gcItem);
static int mln_lang_gc_item_cleanSearcher_objScanner(mln_rbtree_node_t *node, void *rn_data, void *udata);
static int mln_lang_gc_item_cleanSearcher_arrayScanner(mln_rbtree_node_t *node, void *rn_data, void *udata);
static void mln_lang_gc_item_freeHandler(mln_lang_gc_item_t *gcItem);
static mln_lang_tcp_t *
mln_lang_tcp_new(mln_lang_ctx_t *ctx, int sockfd, const char *ip, mln_u16_t port, mln_string_t *msgName, mln_s32_t timeout);
static void mln_lang_tcp_free(mln_lang_tcp_t *tcp);
static int mln_lang_tcp_cmp (const void *data1, const void *data2);
static void mln_lang_tcp_node_free(void *data);
static int mln_lang_tcp_connect(mln_lang_ctx_t *ctx);
static int mln_lang_tcp_listen(mln_lang_ctx_t *ctx);
static int mln_lang_tcp_send(mln_lang_ctx_t *ctx);
static int mln_lang_tcp_recv(mln_lang_ctx_t *ctx);
static int mln_lang_tcp_close(mln_lang_ctx_t *ctx);
static int mln_lang_tcp_shutdown(mln_lang_ctx_t *ctx);
static mln_lang_retExp_t *mln_lang_tcp_close_process(mln_lang_ctx_t *ctx);
static mln_lang_retExp_t *mln_lang_tcp_shutdown_process(mln_lang_ctx_t *ctx);
static mln_lang_retExp_t *mln_lang_tcp_recv_process(mln_lang_ctx_t *ctx);
static mln_lang_retExp_t *mln_lang_tcp_send_process(mln_lang_ctx_t *ctx);
static void mln_lang_tcp_send_handler(mln_event_t *ev, int fd, void *data);
static mln_lang_retExp_t *mln_lang_tcp_connect_process(mln_lang_ctx_t *ctx);
static mln_lang_retExp_t *mln_lang_tcp_listen_process(mln_lang_ctx_t *ctx);
static void mln_lang_tcp_connect_handler(mln_event_t *ev, int fd, void *data);
static void mln_lang_tcp_timeout_handler(mln_event_t *ev, int fd, void *data);
static void mln_lang_tcp_accept_handler(mln_event_t *ev, int fd, void *data);
static void mln_lang_tcp_accept_timer_handler(mln_event_t *ev, void *data);
static void mln_lang_tcp_recv_handler(mln_event_t *ev, int fd, void *data);
static void mln_lang_tcp_setMsg(mln_string_t *type, mln_lang_tcp_t *target, mln_lang_tcp_t *from);
static int mln_lang_tcp_getinfo(const struct sockaddr *addr, char *ip, mln_u16_t *port);
static inline int __mln_lang_tcp_msgExisted(mln_lang_array_t *arr, mln_string_t *type, mln_lang_tcp_t *target);
static int mln_lang_tcp_msgExisted_scan(mln_rbtree_node_t *node, void *rn_data, void *udata);
static void mln_lang_resource_free_handler(mln_lang_resource_t *lr);
static int mln_lang_resource_cmp(const mln_lang_resource_t *lr1, const mln_lang_resource_t *lr2);


mln_lang_method_t *mln_lang_methods[] = {
&mln_lang_nil_oprs,
&mln_lang_int_oprs,
&mln_lang_bool_oprs,
&mln_lang_real_oprs,
&mln_lang_str_oprs,
&mln_lang_obj_oprs,
&mln_lang_func_oprs,
&mln_lang_array_oprs
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
    mln_lang_stack_handler_logicLow,
    mln_lang_stack_handler_logicHigh,
    mln_lang_stack_handler_relativeLow,
    mln_lang_stack_handler_relativeHigh,
    mln_lang_stack_handler_move,
    mln_lang_stack_handler_addsub,
    mln_lang_stack_handler_muldiv,
    mln_lang_stack_handler_suffix,
    mln_lang_stack_handler_locate,
    mln_lang_stack_handler_spec,
    mln_lang_stack_handler_factor,
    mln_lang_stack_handler_elemlist,
    mln_lang_stack_handler_funccall
};

mln_lang_t *mln_lang_new(mln_alloc_t *pool, mln_event_t *ev)
{
    int fds[2];
    mln_lang_t *lang;
    struct timeval tv;
    if ((lang = (mln_lang_t *)mln_alloc_m(pool, sizeof(mln_lang_t))) == NULL) {
        return NULL;
    }
    lang->ev = ev;
    lang->pool = pool;
    lang->run_head = lang->run_tail = NULL;
    lang->blocked_head = lang->blocked_tail = NULL;
    lang->wait_head = lang->wait_tail = NULL;
    lang->ctx_cur = NULL;
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, fds) < 0) {
        mln_alloc_free(lang);
        return NULL;
    }
    lang->fd_notInUsed = fds[0];
    lang->fd_signal = fds[1];
    gettimeofday(&tv, NULL);
    lang->lastTime = tv.tv_sec*1000000 + tv.tv_usec;
    if ((lang->shift_table = mln_lang_parserGenerate()) == NULL) {
        mln_lang_free(lang);
        return NULL;
    }
    return lang;
}

void mln_lang_free(mln_lang_t *lang)
{
    if (lang == NULL) return;
    mln_lang_ctx_t *ctx;
    while ((ctx = lang->run_head) != NULL) {
        mln_lang_ctx_chain_del(&(lang->run_head), &(lang->run_tail), ctx);
        mln_lang_ctx_free(ctx);
    }
    while ((ctx = lang->blocked_head) != NULL) {
        mln_lang_ctx_chain_del(&(lang->blocked_head), &(lang->blocked_tail), ctx);
        mln_lang_ctx_free(ctx);
    }
    while ((ctx = lang->wait_head) != NULL) {
        mln_lang_ctx_chain_del(&(lang->wait_head), &(lang->wait_tail), ctx);
        mln_lang_ctx_free(ctx);
    }
    mln_event_set_fd(lang->ev, lang->fd_signal, M_EV_CLR, M_EV_UNLIMITED, NULL, NULL);
    if (lang->fd_notInUsed >= 0) close(lang->fd_notInUsed);
    if (lang->fd_signal >= 0) close(lang->fd_signal);
    if (lang->shift_table != NULL) mln_lang_parserDestroy(lang->shift_table);
    mln_alloc_free(lang);
}

int mln_lang_run(mln_lang_t *lang)
{
    return __mln_lang_run(lang);
}

static inline int __mln_lang_run(mln_lang_t *lang)
{
    struct timeval tv;
    if (lang->ctx_cur != NULL) {
        mln_lang_stack_node_t *node;
        if ((node = (mln_lang_stack_node_t *)mln_stack_top(lang->ctx_cur->run_stack)) == NULL) {
            if (lang->ctx_cur->ref)
                mln_lang_ctx_chain_del(&(lang->wait_head), &(lang->wait_tail), lang->ctx_cur);
            else if (lang->ctx_cur->step > 0)
                mln_lang_ctx_chain_del(&(lang->run_head), &(lang->run_tail), lang->ctx_cur);
            else
                mln_lang_ctx_chain_del(&(lang->blocked_head), &(lang->blocked_tail), lang->ctx_cur);
            if (lang->ctx_cur != NULL && lang->ctx_cur->return_handler != NULL) {
                lang->ctx_cur->return_handler(lang->ctx_cur);
            }
            mln_lang_ctx_free(lang->ctx_cur);
            lang->ctx_cur = NULL;
        } else {
            if (lang->ctx_cur->ref || lang->ctx_cur->step <= 0) {
                lang->ctx_cur = NULL;
            } else {
                gettimeofday(&tv, NULL);
                mln_u64_t now = tv.tv_sec*1000000 + tv.tv_usec;
                if (now < lang->lastTime + M_LANG_HEARTBEAT_US) goto ok;
                lang->lastTime = now;
                if (--(lang->ctx_cur->step) > 0) {
                    goto ok;
                }
                mln_lang_ctx_chain_del(&(lang->run_head), &(lang->run_tail), lang->ctx_cur);
                mln_lang_ctx_chain_add(&(lang->blocked_head), &(lang->blocked_tail), lang->ctx_cur);
                lang->ctx_cur = NULL;
            }
        }
    }
    gettimeofday(&tv, NULL);
    lang->lastTime = tv.tv_sec*1000000 + tv.tv_usec;
    if (lang->run_head == NULL && lang->blocked_head != NULL) {
        mln_lang_ctx_t *scan = lang->blocked_head;
        for (; scan != NULL; scan = scan->next) {
            scan->step = M_LANG_DEFAULT_STEP;
        }
        lang->run_head = lang->blocked_head;
        lang->run_tail = lang->blocked_tail;
        lang->blocked_head = lang->blocked_tail = NULL;
    }
    lang->ctx_cur = lang->run_head;

    if (lang->ctx_cur != NULL) {
        mln_lang_scope_t *scope = lang->ctx_cur->scope_tail;
        for (; scope != NULL; scope = scope->prev) {
            if (scope->type == M_LANG_SCOPE_TYPE_FUNC)
                break;
        }
        ASSERT(scope && scope->gc);
        mln_gc_collect(scope->gc, scope);
    }

    if (lang->ctx_cur != NULL) {
ok:
        if (mln_event_set_fd(lang->ev, \
                             lang->fd_signal, \
                             M_EV_SEND|M_EV_ONESHOT, \
                             M_EV_UNLIMITED, \
                             lang->ctx_cur, \
                             mln_lang_run_handler) < 0)
        {
            return -1;
        }
    } else {
        mln_event_set_fd(lang->ev, lang->fd_signal, M_EV_CLR, M_EV_UNLIMITED, NULL, NULL);
    }
    return 0;
}

static void mln_lang_run_handler(mln_event_t *ev, int fd, void *data)
{
    mln_lang_ctx_t *ctx = (mln_lang_ctx_t *)data;
    ASSERT(!ctx->ref);
    mln_lang_stack_node_t *node;
    if ((node = (mln_lang_stack_node_t *)mln_stack_top(ctx->run_stack)) == NULL) {
        if (ctx != NULL && ctx->return_handler != NULL) {
            ctx->return_handler(ctx);
        }
        mln_lang_job_free(ctx);
        return;
    }
    mln_lang_stack_map[node->type](ctx);
}


static inline mln_lang_ctx_t *
mln_lang_ctx_new(mln_lang_t *lang, void *data, mln_string_t *filename, mln_u32_t type, mln_string_t *content)
{
    mln_lang_ctx_t *ctx;
    struct mln_rbtree_attr rbattr;
    struct mln_stack_attr stattr;
    melang_installer *installer, *end;

    if ((ctx = (mln_lang_ctx_t *)mln_alloc_m(lang->pool, sizeof(mln_lang_ctx_t))) == NULL) {
        return NULL;
    }
    ctx->lang = lang;
    if ((ctx->pool = mln_alloc_init()) == NULL) {
        mln_alloc_free(ctx);
        return NULL;
    }
    if ((ctx->fset = mln_fileset_init(M_LANG_MAX_OPENFILE)) == NULL) {
        mln_alloc_destroy(ctx->pool);
        mln_alloc_free(ctx);
        return NULL;
    }
    ctx->data = data;
    if ((ctx->stm = (mln_lang_stm_t *)mln_lang_ast_generate(ctx->pool, lang->shift_table, content, type)) == NULL) {
        mln_fileset_destroy(ctx->fset);
        mln_alloc_destroy(ctx->pool);
        mln_alloc_free(ctx);
        return NULL;
    }
    stattr.free_handler = mln_lang_stack_node_free;
    stattr.copy_handler = NULL;
    if ((ctx->run_stack = mln_stack_init(&stattr)) == NULL) {
        mln_lang_ast_free(ctx->stm);
        mln_fileset_destroy(ctx->fset);
        mln_alloc_destroy(ctx->pool);
        mln_alloc_free(ctx);
        return NULL;
    }
    ctx->scope_head = ctx->scope_tail = NULL;
    ctx->ref = 0;
    ctx->step = M_LANG_DEFAULT_STEP;
    ctx->filename = NULL;
    rbattr.cmp = mln_lang_msg_cmp;
    rbattr.data_free = __mln_lang_msg_free;
    if ((ctx->msg_map = mln_rbtree_init(&rbattr)) == NULL) {
        mln_stack_destroy(ctx->run_stack);
        mln_lang_ast_free(ctx->stm);
        mln_fileset_destroy(ctx->fset);
        mln_alloc_destroy(ctx->pool);
        mln_alloc_free(ctx);
        return NULL;
    }
    rbattr.cmp = mln_lang_tcp_cmp;
    rbattr.data_free = mln_lang_tcp_node_free;
    if ((ctx->tcp_set = mln_rbtree_init(&rbattr)) == NULL) {
        mln_rbtree_destroy(ctx->msg_map);
        mln_stack_destroy(ctx->run_stack);
        mln_lang_ast_free(ctx->stm);
        mln_fileset_destroy(ctx->fset);
        mln_alloc_destroy(ctx->pool);
        mln_alloc_free(ctx);
        return NULL;
    }
    rbattr.cmp = (rbtree_cmp)mln_lang_resource_cmp;
    rbattr.data_free = (rbtree_free_data)mln_lang_resource_free_handler;
    if ((ctx->resource_set = mln_rbtree_init(&rbattr)) == NULL) {
        mln_rbtree_destroy(ctx->tcp_set);
        mln_rbtree_destroy(ctx->msg_map);
        mln_stack_destroy(ctx->run_stack);
        mln_lang_ast_free(ctx->stm);
        mln_fileset_destroy(ctx->fset);
        mln_alloc_destroy(ctx->pool);
        mln_alloc_free(ctx);
        return NULL;
    }

    ctx->retExp = NULL;
    ctx->return_handler = NULL;
    ctx->prev = ctx->next = NULL;

    mln_lang_stack_node_t *node = mln_lang_stack_node_new(ctx->pool, M_LSNT_STM, ctx->stm);
    if (node == NULL) {
        mln_lang_ctx_free(ctx);
        return NULL;
    }
    if (mln_stack_push(ctx->run_stack, node) < 0) {
        mln_lang_stack_node_free(node);
        mln_lang_ctx_free(ctx);
        return NULL;
    }

    mln_lang_scope_t *outer_scope;
    if ((outer_scope = mln_lang_scope_new(ctx, NULL, M_LANG_SCOPE_TYPE_FUNC, NULL)) == NULL) {
        mln_lang_ctx_free(ctx);
        return NULL;
    }
    mln_lang_scope_chain_add(&(ctx->scope_head), &(ctx->scope_tail), outer_scope);

    if (filename != NULL) {
        if ((ctx->filename = mln_string_pool_dup(ctx->pool, filename)) == NULL) {
            mln_lang_ctx_free(ctx);
            return NULL;
        }
    }

    if (mln_lang_msg_installer(ctx) < 0) {
        mln_lang_ctx_free(ctx);
        return NULL;
    }
    end = mln_melang_installers + M_N_INSTALLER;
    for (installer = mln_melang_installers; installer < end; ++installer) {
        if ((*installer)(ctx) < 0) {
            mln_lang_ctx_free(ctx);
            return NULL;
        }
    }
    return ctx;
}

static inline void mln_lang_ctx_free(mln_lang_ctx_t *ctx)
{
    if (ctx == NULL) return;
    mln_lang_scope_t *scope;
    if (ctx->retExp != NULL) __mln_lang_retExp_free(ctx->retExp);
    if (ctx->filename != NULL) mln_string_pool_free(ctx->filename);
    if (ctx->resource_set != NULL) mln_rbtree_destroy(ctx->resource_set);
    if (ctx->tcp_set != NULL) mln_rbtree_destroy(ctx->tcp_set);
    if (ctx->msg_map != NULL) mln_rbtree_destroy(ctx->msg_map);
    if (ctx->run_stack != NULL) mln_stack_destroy(ctx->run_stack);
    while ((scope = ctx->scope_head) != NULL) {
        mln_lang_scope_chain_del(&(ctx->scope_head), &(ctx->scope_tail), scope);
        mln_lang_scope_free(scope);
    }
    if (ctx->stm != NULL) mln_lang_ast_free(ctx->stm);
    mln_fileset_destroy(ctx->fset);
    mln_alloc_destroy(ctx->pool);
    mln_alloc_free(ctx);
}

static inline void mln_lang_ctx_resetRetExp(mln_lang_ctx_t *ctx)
{
    if (ctx->retExp == NULL) return;
    __mln_lang_retExp_free(ctx->retExp);
    ctx->retExp = NULL;
}

void mln_lang_ctx_setRetExp(mln_lang_ctx_t *ctx, mln_lang_retExp_t *retExp)
{
    mln_lang_ctx_resetRetExp(ctx);
    ctx->retExp = retExp;
}

static inline void
mln_lang_ctx_getRetExpFromNode(mln_lang_ctx_t *ctx, mln_lang_stack_node_t *node)
{
    if (ctx->retExp != NULL) __mln_lang_retExp_free(ctx->retExp);
    ctx->retExp = node->retExp;
    node->retExp = NULL;
}

static inline mln_lang_set_detail_t *
mln_lang_ctx_getClass(mln_lang_ctx_t *ctx)
{
    mln_string_t *name = NULL;
    ASSERT(ctx->scope_tail != NULL);
    if (ctx->scope_tail->type != M_LANG_SCOPE_TYPE_SET) return NULL;
    name = ctx->scope_tail->name;
    ASSERT(ctx->scope_tail->prev != NULL);
    mln_rbtree_t *tree = ctx->scope_tail->prev->symbols;
    mln_rbtree_node_t *rn;
    mln_lang_symbolNode_t sym;
    sym.symbol = name;
    rn = mln_rbtree_search(tree, tree->root, &sym);
    ASSERT(!mln_rbtree_null(rn, tree));
    ASSERT(((mln_lang_symbolNode_t *)(rn->data))->type == M_LANG_SYMBOL_SET);
    return ((mln_lang_symbolNode_t *)(rn->data))->data.set;
}

int mln_lang_ctx_addGlobalVar(mln_lang_ctx_t *ctx, mln_string_t *name, void *val, mln_u32_t type)
{
    mln_lang_val_t *v;
    mln_lang_var_t *var;
    if ((v = __mln_lang_val_new(ctx->pool, type, val)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if ((var = __mln_lang_var_new(ctx->pool, name, M_LANG_VAR_NORMAL, v, NULL)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        __mln_lang_val_free(v);
        return -1;
    }
    if (mln_lang_symbolNode_join(ctx, M_LANG_SYMBOL_VAR, var) < 0) {
        __mln_lang_errmsg(ctx, "No memory.");
        __mln_lang_var_free(var);
        return -1;
    }
    return 0;
}

mln_lang_ctx_t *
mln_lang_job_new(mln_lang_t *lang, \
                 mln_u32_t type, \
                 mln_string_t *data, \
                 void *udata, \
                 mln_lang_return_handler handler)
{
    mln_lang_ctx_t *ctx = mln_lang_ctx_new(lang, udata, type==M_INPUT_T_FILE? data: NULL, type, data);
    if (ctx == NULL) return NULL;
    ctx->return_handler = handler;
    mln_lang_ctx_chain_add(&(lang->run_head), &(lang->run_tail), ctx);
    if (__mln_lang_run(lang) < 0) {
        mln_lang_ctx_chain_del(&(lang->run_head), &(lang->run_tail), ctx);
        mln_lang_ctx_free(ctx);
        return NULL;
    }
    return ctx;
}

void mln_lang_job_free(mln_lang_ctx_t *ctx)
{
    if (ctx == NULL) return;
    mln_lang_t *lang = ctx->lang;
    if (lang->ctx_cur == ctx) lang->ctx_cur = NULL;
    if (ctx->ref)
        mln_lang_ctx_chain_del(&(lang->wait_head), &(lang->wait_tail), ctx);
    else if (ctx->step > 0)
        mln_lang_ctx_chain_del(&(lang->run_head), &(lang->run_tail), ctx);
    else
        mln_lang_ctx_chain_del(&(lang->blocked_head), &(lang->blocked_tail), ctx);
    mln_lang_ctx_free(ctx);
    if (lang->ctx_cur == NULL) (void)__mln_lang_run(lang);
}


mln_lang_retExp_t *mln_lang_retExp_new(mln_alloc_t *pool, mln_lang_retExp_type_t type, void *data)
{
    return __mln_lang_retExp_new(pool, type, data);
}

static inline mln_lang_retExp_t *
__mln_lang_retExp_new(mln_alloc_t *pool, mln_lang_retExp_type_t type, void *data)
{
    mln_lang_retExp_t *retExp;
    if ((retExp = (mln_lang_retExp_t *)mln_alloc_m(pool, sizeof(mln_lang_retExp_t))) == NULL) {
        return NULL;
    }
    retExp->type = type;
    switch (type) {
        case M_LANG_RETEXP_VAR:
            retExp->data.var = (mln_lang_var_t *)data;
            break;
        default:
            retExp->data.func = (mln_lang_funccall_val_t *)data;
            break;
    }
    return retExp;
}

void mln_lang_retExp_free(mln_lang_retExp_t *retExp)
{
    return __mln_lang_retExp_free(retExp);
}

static inline void __mln_lang_retExp_free(mln_lang_retExp_t *retExp)
{
    if (retExp == NULL) return;
    switch (retExp->type) {
        case M_LANG_RETEXP_VAR:
            if (retExp->data.var != NULL)
                __mln_lang_var_free(retExp->data.var);
            break;
        default:
            if (retExp->data.func != NULL)
                __mln_lang_funccall_val_free(retExp->data.func);
            break;
    }
    mln_alloc_free(retExp);
}

mln_lang_retExp_t *mln_lang_retExp_createTmpNil(mln_alloc_t *pool, mln_string_t *name)
{
    return __mln_lang_retExp_createTmpNil(pool, name);
}

static inline mln_lang_retExp_t *
__mln_lang_retExp_createTmpNil(mln_alloc_t *pool, mln_string_t *name)
{
    mln_lang_val_t *val;
    mln_lang_var_t *var;
    mln_lang_retExp_t *retExp;
    if ((val = __mln_lang_val_new(pool, M_LANG_VAL_TYPE_NIL, NULL)) == NULL) {
        return NULL;
    }
    if ((var = __mln_lang_var_new(pool, name, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        __mln_lang_val_free(val);
        return NULL;
    }
    if ((retExp = __mln_lang_retExp_new(pool, M_LANG_RETEXP_VAR, var)) == NULL) {
        __mln_lang_var_free(var);
        return NULL;
    }
    return retExp;
}

mln_lang_retExp_t *mln_lang_retExp_createTmpObj(mln_lang_ctx_t *ctx, mln_lang_set_detail_t *inSet, mln_string_t *name)
{
    return __mln_lang_retExp_createTmpObj(ctx, inSet, name);
}

static inline mln_lang_retExp_t *
__mln_lang_retExp_createTmpObj(mln_lang_ctx_t *ctx, mln_lang_set_detail_t *inSet, mln_string_t *name)
{
    mln_lang_val_t *val;
    mln_lang_var_t *var;
    mln_lang_retExp_t *retExp;
    mln_lang_object_t *obj;
    if ((obj = mln_lang_object_new(ctx, inSet)) == NULL) {
        return NULL;
    }
    if ((val = __mln_lang_val_new(ctx->pool, M_LANG_VAL_TYPE_OBJECT, obj)) == NULL) {
        mln_lang_object_free(obj);
        return NULL;
    }
    if ((var = __mln_lang_var_new(ctx->pool, NULL, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        __mln_lang_val_free(val);
        return NULL;
    }
    if ((retExp = __mln_lang_retExp_new(ctx->pool, M_LANG_RETEXP_VAR, var)) == NULL) {
        __mln_lang_var_free(var);
        return NULL;
    }
    return retExp;
}

mln_lang_retExp_t *mln_lang_retExp_createTmpTrue(mln_alloc_t *pool, mln_string_t *name)
{
    return __mln_lang_retExp_createTmpTrue(pool, name);
}

static inline mln_lang_retExp_t *
__mln_lang_retExp_createTmpTrue(mln_alloc_t *pool, mln_string_t *name)
{
    mln_lang_val_t *val;
    mln_lang_var_t *var;
    mln_lang_retExp_t *retExp;
    mln_u8_t t = 1;
    if ((val = __mln_lang_val_new(pool, M_LANG_VAL_TYPE_BOOL, &t)) == NULL) {
        return NULL;
    }
    if ((var = __mln_lang_var_new(pool, name, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        __mln_lang_val_free(val);
        return NULL;
    }
    if ((retExp = __mln_lang_retExp_new(pool, M_LANG_RETEXP_VAR, var)) == NULL) {
        __mln_lang_var_free(var);
        return NULL;
    }
    return retExp;
}

mln_lang_retExp_t *mln_lang_retExp_createTmpFalse(mln_alloc_t *pool, mln_string_t *name)
{
    mln_lang_val_t *val;
    mln_lang_var_t *var;
    mln_lang_retExp_t *retExp;
    mln_u8_t t = 0;
    if ((val = __mln_lang_val_new(pool, M_LANG_VAL_TYPE_BOOL, &t)) == NULL) {
        return NULL;
    }
    if ((var = __mln_lang_var_new(pool, name, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        __mln_lang_val_free(val);
        return NULL;
    }
    if ((retExp = __mln_lang_retExp_new(pool, M_LANG_RETEXP_VAR, var)) == NULL) {
        __mln_lang_var_free(var);
        return NULL;
    }
    return retExp;
}

mln_lang_retExp_t *mln_lang_retExp_createTmpInt(mln_alloc_t *pool, mln_s64_t off, mln_string_t *name)
{
    mln_lang_val_t *val;
    mln_lang_var_t *var;
    mln_lang_retExp_t *retExp;
    if ((val = __mln_lang_val_new(pool, M_LANG_VAL_TYPE_INT, &off)) == NULL) {
        return NULL;
    }
    if ((var = __mln_lang_var_new(pool, name, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        __mln_lang_val_free(val);
        return NULL;
    }
    if ((retExp = __mln_lang_retExp_new(pool, M_LANG_RETEXP_VAR, var)) == NULL) {
        __mln_lang_var_free(var);
        return NULL;
    }
    return retExp;
}

mln_lang_retExp_t *mln_lang_retExp_createTmpReal(mln_alloc_t *pool, double f, mln_string_t *name)
{
    mln_lang_val_t *val;
    mln_lang_var_t *var;
    mln_lang_retExp_t *retExp;
    if ((val = __mln_lang_val_new(pool, M_LANG_VAL_TYPE_REAL, &f)) == NULL) {
        return NULL;
    }
    if ((var = __mln_lang_var_new(pool, name, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        __mln_lang_val_free(val);
        return NULL;
    }
    if ((retExp = __mln_lang_retExp_new(pool, M_LANG_RETEXP_VAR, var)) == NULL) {
        __mln_lang_var_free(var);
        return NULL;
    }
    return retExp;
}

mln_lang_retExp_t *mln_lang_retExp_createTmpBool(mln_alloc_t *pool, mln_u8_t b, mln_string_t *name)
{
    mln_lang_val_t *val;
    mln_lang_var_t *var;
    mln_lang_retExp_t *retExp;
    if ((val = __mln_lang_val_new(pool, M_LANG_VAL_TYPE_BOOL, &b)) == NULL) {
        return NULL;
    }
    if ((var = __mln_lang_var_new(pool, name, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        __mln_lang_val_free(val);
        return NULL;
    }
    if ((retExp = __mln_lang_retExp_new(pool, M_LANG_RETEXP_VAR, var)) == NULL) {
        __mln_lang_var_free(var);
        return NULL;
    }
    return retExp;
}

mln_lang_retExp_t *mln_lang_retExp_createTmpString(mln_alloc_t *pool, mln_string_t *s, mln_string_t *name)
{
    mln_lang_val_t *val;
    mln_lang_var_t *var;
    mln_lang_retExp_t *retExp;
    mln_string_t *dup;
    if ((dup = mln_string_pool_dup(pool, s)) == NULL) {
        return NULL;
    }
    if ((val = __mln_lang_val_new(pool, M_LANG_VAL_TYPE_STRING, dup)) == NULL) {
        mln_string_pool_free(dup);
        return NULL;
    }
    if ((var = __mln_lang_var_new(pool, name, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        __mln_lang_val_free(val);
        return NULL;
    }
    if ((retExp = __mln_lang_retExp_new(pool, M_LANG_RETEXP_VAR, var)) == NULL) {
        __mln_lang_var_free(var);
        return NULL;
    }
    return retExp;
}

mln_lang_retExp_t *mln_lang_retExp_createTmpArray(mln_lang_ctx_t *ctx, mln_string_t *name)
{
    mln_lang_val_t *val;
    mln_lang_var_t *var;
    mln_lang_retExp_t *retExp;
    mln_lang_array_t *array;
    if ((array = __mln_lang_array_new(ctx)) == NULL) {
        return NULL;
    }
    if ((val = __mln_lang_val_new(ctx->pool, M_LANG_VAL_TYPE_ARRAY, array)) == NULL) {
        __mln_lang_array_free(array);
        return NULL;
    }
    if ((var = __mln_lang_var_new(ctx->pool, name, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        __mln_lang_val_free(val);
        return NULL;
    }
    if ((retExp = __mln_lang_retExp_new(ctx->pool, M_LANG_RETEXP_VAR, var)) == NULL) {
        __mln_lang_var_free(var);
        return NULL;
    }
    return retExp;
}

static inline mln_lang_stack_node_t *
mln_lang_stack_node_new(mln_alloc_t *pool, mln_lang_stack_node_type_t type, void *data)
{
    mln_lang_stack_node_t *sn;
    if ((sn = (mln_lang_stack_node_t *)mln_alloc_m(pool, sizeof(mln_lang_stack_node_t))) == NULL) {
        return NULL;
    }
    sn->type = type;
    sn->pos = NULL;
    switch (type) {
        case M_LSNT_STM:
            sn->data.stm = (mln_lang_stm_t *)data;
            break;
        case M_LSNT_FUNCDEF:
            sn->data.funcdef = (mln_lang_funcdef_t *)data;
            break;
        case M_LSNT_SET:
            sn->data.set = (mln_lang_set_t *)data;
            break;
        case M_LSNT_SETSTM:
            sn->data.set_stm = (mln_lang_setstm_t *)data;
            break;
        case M_LSNT_BLOCK:
            sn->data.block = (mln_lang_block_t *)data;
            break;
        case M_LSNT_WHILE:
            sn->data.w = (mln_lang_while_t *)data;
            break;
        case M_LSNT_SWITCH:
            sn->data.sw = (mln_lang_switch_t *)data;
            break;
        case M_LSNT_SWITCHSTM:
            sn->data.sw_stm = (mln_lang_switchstm_t *)data;
            break;
        case M_LSNT_FOR:
            sn->data.f = (mln_lang_for_t *)data;
            break;
        case M_LSNT_IF:
            sn->data.i = (mln_lang_if_t *)data;
            break;
        case M_LSNT_EXP:
            sn->data.exp = (mln_lang_exp_t *)data;
            break;
        case M_LSNT_ASSIGN:
            sn->data.assign = (mln_lang_assign_t *)data;
            break;
        case M_LSNT_LOGICLOW:
            sn->data.logicLow = (mln_lang_logicLow_t *)data;
            break;
        case M_LSNT_LOGICHIGH:
            sn->data.logicHigh = (mln_lang_logicHigh_t *)data;
            sn->pos = sn->data.logicHigh;
            break;
        case M_LSNT_RELATIVELOW:
            sn->data.relativeLow = (mln_lang_relativeLow_t *)data;
            sn->pos = sn->data.relativeLow;
            break;
        case M_LSNT_RELATIVEHIGH:
            sn->data.relativeHigh = (mln_lang_relativeHigh_t *)data;
            sn->pos = sn->data.relativeHigh;
            break;
        case M_LSNT_MOVE:
            sn->data.move = (mln_lang_move_t *)data;
            sn->pos = sn->data.move;
            break;
        case M_LSNT_ADDSUB:
            sn->data.addsub = (mln_lang_addsub_t *)data;
            sn->pos = sn->data.addsub;
            break;
        case M_LSNT_MULDIV:
            sn->data.muldiv = (mln_lang_muldiv_t *)data;
            sn->pos = sn->data.muldiv;
            break;
        case M_LSNT_SUFFIX:
            sn->data.suffix = (mln_lang_suffix_t *)data;
            break;
        case M_LSNT_LOCATE:
            sn->data.locate = (mln_lang_locate_t *)data;
            break;
        case M_LSNT_SPEC:
            sn->data.spec = (mln_lang_spec_t *)data;
            break;
        case M_LSNT_FACTOR:
            sn->data.factor = (mln_lang_factor_t *)data;
            break;
        case M_LSNT_ELEMLIST:
            sn->data.elemlist = (mln_lang_elemlist_t *)data;
            break;
        default:/*M_LSNT_FUNCCALL*/
            sn->data.funccall = (mln_lang_funccall_t *)data;
            sn->pos = sn->data.funccall->args;
            break;
    }
    sn->retExp = NULL;
    sn->retExp2 = NULL;
    sn->step = 0;
    sn->call = 0;
    return sn;
}

void mln_lang_stack_node_free(void *data)
{
    __mln_lang_stack_node_free(data);
}

static inline void __mln_lang_stack_node_free(void *data)
{
    if (data == NULL) return;
    mln_lang_stack_node_t *node = (mln_lang_stack_node_t *)data;
    if (node->retExp != NULL) __mln_lang_retExp_free(node->retExp);
    if (node->retExp2 != NULL) __mln_lang_retExp_free(node->retExp2);
    mln_alloc_free(node);
}

static inline void
mln_lang_stack_node_getRetExpFromCTX(mln_lang_stack_node_t *node, mln_lang_ctx_t *ctx)
{
    if (node->retExp != NULL) __mln_lang_retExp_free(node->retExp);
    node->retExp = ctx->retExp;
    ctx->retExp = NULL;
}

static inline void
mln_lang_stack_node_resetRetExp(mln_lang_stack_node_t *node)
{
    if (node->retExp == NULL) return;
    __mln_lang_retExp_free(node->retExp);
    node->retExp = NULL;
}

static inline void
mln_lang_stack_node_setRetExp(mln_lang_stack_node_t *node, mln_lang_retExp_t *retExp)
{
    mln_lang_stack_node_resetRetExp(node);
    node->retExp = retExp;
}


static inline mln_lang_scope_t *
mln_lang_scope_new(mln_lang_ctx_t *ctx, \
                   mln_string_t *name, \
                   mln_lang_scope_type_t type, \
                   mln_lang_stack_node_t *cur_stack)
{
    mln_lang_scope_t *scope;
    struct mln_rbtree_attr rbattr;
    if ((scope = (mln_lang_scope_t *)mln_alloc_m(ctx->pool, sizeof(mln_lang_scope_t))) == NULL) {
        return NULL;
    }
    scope->type = type;
    if (name != NULL) {
        if ((scope->name = mln_string_pool_dup(ctx->pool, name)) == NULL) {
            mln_alloc_free(scope);
            return NULL;
        }
    } else {
        scope->name = NULL;
    }

    if (type == M_LANG_SCOPE_TYPE_FUNC) {
        struct mln_gc_attr gcattr;
        gcattr.pool = ctx->pool;
        gcattr.itemGetter = (gcItemGetter)mln_lang_gc_item_getter;
        gcattr.itemSetter = (gcItemSetter)mln_lang_gc_item_setter;
        gcattr.itemFreer = (gcItemFreer)mln_lang_gc_item_free;
        gcattr.memberSetter = (gcMemberSetter)mln_lang_gc_item_memberSetter;
        gcattr.moveHandler = (gcMoveHandler)mln_lang_gc_item_moveHandler;
        gcattr.rootSetter = (gcRootSetter)mln_lang_gc_item_rootSetter;
        gcattr.cleanSearcher = (gcCleanSearcher)mln_lang_gc_item_cleanSearcher;
        gcattr.freeHandler = (gcFreeHandler)mln_lang_gc_item_freeHandler;
        if ((scope->gc = mln_gc_new(&gcattr)) == NULL) {
            if (scope->name != NULL) mln_string_pool_free(scope->name);
            mln_alloc_free(scope);
            return NULL;
        }
    } else {
        scope->gc = NULL;
    }

    rbattr.cmp = mln_lang_symbolNode_cmp;
    rbattr.data_free = mln_lang_symbolNode_free;
    if ((scope->symbols = mln_rbtree_init(&rbattr)) == NULL) {
        if (scope->gc != NULL) mln_gc_free(scope->gc);
        if (scope->name != NULL) mln_string_pool_free(scope->name);
        mln_alloc_free(scope);
        return NULL;
    }
    scope->ctx = ctx;
    scope->cur_stack = cur_stack;
    scope->prev = scope->next = NULL;
    return scope;
}

static void mln_lang_scope_free(mln_lang_scope_t *scope)
{
    if (scope == NULL) return;
    mln_gc_t *cur = mln_lang_scope_getCurrentGC(scope->ctx);
    if (scope->gc != NULL) {
        mln_gc_collect(scope->gc, scope);
        if (cur != NULL) {
            mln_gc_merge(cur, scope->gc);
            mln_gc_free(scope->gc);
        }
    }
    if (scope->name != NULL) mln_string_pool_free(scope->name);
    if (scope->symbols != NULL) mln_rbtree_destroy(scope->symbols);
    if (cur == NULL) {
        mln_gc_collect(scope->gc, NULL);
        mln_gc_free(scope->gc);
    }
    mln_alloc_free(scope);
}

static inline mln_gc_t *
mln_lang_scope_getCurrentGC(mln_lang_ctx_t *ctx)
{
    mln_lang_scope_t *scope = ctx->scope_tail;
    for (; scope != NULL; scope = scope->prev) {
        if (scope->type == M_LANG_SCOPE_TYPE_FUNC)
            break;
    }
    if (scope == NULL) return NULL;
    return scope->gc;
}


static int mln_lang_symbolNode_cmp(const void *data1, const void *data2)
{
    mln_lang_symbolNode_t *ls1 = (mln_lang_symbolNode_t *)data1;
    mln_lang_symbolNode_t *ls2 = (mln_lang_symbolNode_t *)data2;
    return mln_string_strcmp(ls1->symbol, ls2->symbol);
}

static inline mln_lang_symbolNode_t *
mln_lang_symbolNode_new(mln_lang_ctx_t *ctx, mln_string_t *symbol, mln_lang_symbolType_t type, void *data)
{
    mln_lang_symbolNode_t *ls;
    if ((ls = (mln_lang_symbolNode_t *)mln_alloc_m(ctx->pool, sizeof(mln_lang_symbolNode_t))) == NULL) {
        return NULL;
    }
    ls->symbol = symbol;
    ls->ctx = ctx;
    ls->type = type;
    switch (type) {
        case M_LANG_SYMBOL_VAR:
            ls->data.var = (mln_lang_var_t *)data;
            break;
        case M_LANG_SYMBOL_SET:
            ls->data.set = (mln_lang_set_detail_t *)data;
            break;
        default:
            ls->data.label = (mln_lang_label_t *)data;
            break;
    }
    return ls;
}

static void mln_lang_symbolNode_free(void *data)
{
    if (data == NULL) return;
    mln_lang_symbolNode_t *ls = (mln_lang_symbolNode_t *)data;
    switch (ls->type) {
        case M_LANG_SYMBOL_VAR:
            if (ls->data.var != NULL) __mln_lang_var_free(ls->data.var);
            break;
        case M_LANG_SYMBOL_SET:
            if (ls->data.set != NULL) mln_lang_set_detail_freeSelf(ls->data.set);
            break;
        default:
            if (ls->data.label != NULL) mln_lang_label_free(ls->data.label);
            break;
    }
    mln_alloc_free(ls);
}

int mln_lang_symbolNode_join(mln_lang_ctx_t *ctx, mln_lang_symbolType_t type, void *data)
{
    return __mln_lang_symbolNode_join(ctx, type, data);
}

static inline int __mln_lang_symbolNode_join(mln_lang_ctx_t *ctx, mln_lang_symbolType_t type, void *data)
{
    mln_lang_symbolNode_t *symbol;
    mln_string_t *name;
    mln_rbtree_t *tree = ctx->scope_tail->symbols;
    mln_rbtree_node_t *rn;
    switch (type) {
        case M_LANG_SYMBOL_VAR:
            name = ((mln_lang_var_t *)data)->name;
            break;
        case M_LANG_SYMBOL_SET:
            name = ((mln_lang_set_detail_t *)data)->name;
            break;
        default:
            name = ((mln_lang_label_t *)data)->label;
            break;
    }
    if ((symbol = mln_lang_symbolNode_new(ctx, name, type, data)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    rn = mln_rbtree_search(tree, tree->root, symbol);
    if (!mln_rbtree_null(rn, tree)) {
        mln_lang_symbolNode_free(rn->data);
        rn->data = symbol;
        return 0;
    }
    if ((rn = mln_rbtree_node_new(tree, symbol)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        mln_lang_symbolNode_free(symbol);
        return -1;
    }
    mln_rbtree_insert(tree, rn);
    return 0;
}

mln_lang_symbolNode_t *mln_lang_symbolNode_search(mln_lang_ctx_t *ctx, mln_string_t *name, int local)
{
    return __mln_lang_symbolNode_search(ctx, name, local);
}

static inline mln_lang_symbolNode_t *
__mln_lang_symbolNode_search(mln_lang_ctx_t *ctx, mln_string_t *name, int local)
{
    mln_lang_symbolNode_t sym;
    mln_rbtree_node_t *rn;
    sym.symbol = name;
    mln_lang_scope_t *scope = ctx->scope_tail;
    for (; scope != NULL; scope = scope->prev) {
        rn = mln_rbtree_search(scope->symbols, scope->symbols->root, &sym);
        if (!mln_rbtree_null(rn, scope->symbols)) {
            return (mln_lang_symbolNode_t *)(rn->data);
        }
        if (local) break;
    }
    return NULL;
}

static inline mln_lang_symbolNode_t *
mln_lang_symbolNode_idSearch(mln_lang_ctx_t *ctx, mln_string_t *name)
{
    mln_lang_symbolNode_t sym;
    mln_rbtree_node_t *rn;
    sym.symbol = name;
    mln_lang_scope_t *scope = ctx->scope_tail;
    if (scope == NULL) return NULL;
    rn = mln_rbtree_search(scope->symbols, scope->symbols->root, &sym);
    if (!mln_rbtree_null(rn, scope->symbols)) {
        return (mln_lang_symbolNode_t *)(rn->data);
    }
    return NULL;
}


static inline mln_lang_label_t *
mln_lang_label_new(mln_alloc_t *pool, mln_string_t *label, mln_lang_stm_t *stm)
{
    mln_lang_label_t *ll;
    if ((ll = (mln_lang_label_t *)mln_alloc_m(pool, sizeof(mln_lang_label_t))) == NULL) {
        return NULL;
    }
    if ((ll->label = mln_string_pool_dup(pool, label)) == NULL) {
        mln_alloc_free(ll);
        return NULL;
    }
    ll->stm = stm;
    return ll;
}

static inline void mln_lang_label_free(mln_lang_label_t *label)
{
    if (label == NULL) return;
    if (label->label != NULL) mln_string_pool_free(label->label);
    mln_alloc_free(label);
}


mln_lang_set_detail_t *
mln_lang_set_detail_new(mln_alloc_t *pool, mln_string_t *name)
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
    rbattr.cmp = mln_lang_var_cmpName;
    rbattr.data_free = mln_lang_var_free;
    if ((lcd->members = mln_rbtree_init(&rbattr)) == NULL) {
        mln_alloc_free(lcd);
        return NULL;
    }
    lcd->ref = 0;
    return lcd;
}

void mln_lang_set_detail_free(mln_lang_set_detail_t *c)
{
    if (c == NULL) return;
    if (c->ref-- > 1) return;
    if (c->name != NULL) mln_string_pool_free(c->name);
    if (c->members != NULL) mln_rbtree_destroy(c->members);
    mln_alloc_free(c);
}

void mln_lang_set_detail_freeSelf(mln_lang_set_detail_t *c)
{
    if (c == NULL) return;
    if (c->members != NULL) {
        mln_rbtree_destroy(c->members);
        c->members = NULL;
    }
    if (c->ref-- > 1) return;
    if (c->name != NULL) mln_string_pool_free(c->name);
    mln_alloc_free(c);
}


int mln_lang_set_member_add(mln_alloc_t *pool, mln_rbtree_t *members, mln_lang_var_t *var)
{
    mln_rbtree_node_t *rn;
    rn = mln_rbtree_search(members, members->root, var);
    if (!mln_rbtree_null(rn, members)) {
        mln_lang_var_t *tmp = (mln_lang_var_t *)(rn->data);
        __mln_lang_var_assign(tmp, var->val);
        __mln_lang_var_free(var);
        return 0;
    }
    if ((rn = mln_rbtree_node_new(members, var)) == NULL) {
        return -1;
    }
    mln_rbtree_insert(members, rn);
    return 0;
}

mln_lang_var_t *mln_lang_set_member_search(mln_rbtree_t *members, mln_string_t *name)
{
    return __mln_lang_set_member_search(members, name);
}

static inline mln_lang_var_t *
__mln_lang_set_member_search(mln_rbtree_t *members, mln_string_t *name)
{
    mln_rbtree_node_t *rn;
    mln_lang_var_t var;
    var.name = name;
    rn = mln_rbtree_search(members, members->root, &var);
    if (mln_rbtree_null(rn, members)) return NULL;
    return (mln_lang_var_t *)(rn->data);
}

static int mln_lang_set_member_scan(mln_rbtree_node_t *node, void *rn_data, void *udata)
{
    mln_lang_var_t *var, *lv = (mln_lang_var_t *)rn_data;
    struct mln_lang_scan_s *ls = (struct mln_lang_scan_s *)udata;
    mln_rbtree_t *tree = ls->tree;
    mln_rbtree_node_t *rn;

    rn = mln_rbtree_search(tree, tree->root, lv);
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
}

int mln_lang_object_add_member(mln_lang_ctx_t *ctx, mln_lang_object_t *obj, mln_lang_var_t *var)
{
    mln_lang_var_t *dup;
    mln_rbtree_node_t *rn;

    rn = mln_rbtree_search(obj->members, obj->members->root, var);
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
}


mln_lang_var_t *mln_lang_var_new(mln_alloc_t *pool, \
                                 mln_string_t *name, \
                                 mln_lang_var_type_t type, \
                                 mln_lang_val_t *val, \
                                 mln_lang_set_detail_t *inSet)
{
    return __mln_lang_var_new(pool, name, type, val, inSet);
}

static inline mln_lang_var_t *
__mln_lang_var_new(mln_alloc_t *pool, \
                   mln_string_t *name, \
                   mln_lang_var_type_t type, \
                   mln_lang_val_t *val, \
                   mln_lang_set_detail_t *inSet)
{
    mln_lang_var_t *var;
    if ((var = (mln_lang_var_t *)mln_alloc_m(pool, sizeof(mln_lang_var_t))) == NULL) {
        return NULL;
    }
    var->type = type;
    if (name != NULL) {
        if ((var->name = mln_string_pool_dup(pool, name)) == NULL) {
            mln_alloc_free(var);
            return NULL;
        }
    } else {
        var->name = NULL;
    }
    var->val = val;
    if (val != NULL) ++(val->ref);
    var->inSet = inSet;
    if (inSet != NULL) ++(inSet->ref);
    var->prev = var->next = NULL;
    return var;
}

void mln_lang_var_free(void *data)
{
    return __mln_lang_var_free(data);
}

static inline void __mln_lang_var_free(void *data)
{
    if (data == NULL) return;
    mln_lang_var_t *var = (mln_lang_var_t *)data;
    if (var->name != NULL) mln_string_pool_free(var->name);
    if (var->val != NULL) __mln_lang_val_free(var->val);
    if (var->inSet != NULL) mln_lang_set_detail_free(var->inSet);
    mln_alloc_free(var);
}

mln_lang_var_t *mln_lang_var_dup(mln_lang_ctx_t *ctx, mln_lang_var_t *var)
{
    return __mln_lang_var_dup(ctx, var);
}

static inline mln_lang_var_t *
__mln_lang_var_dup(mln_lang_ctx_t *ctx, mln_lang_var_t *var)
{
    mln_lang_var_t *ret;
    mln_lang_val_t *val = var->val;
    if (var->type == M_LANG_VAR_NORMAL && var->val != NULL) {
        if ((val = mln_lang_val_dup(ctx, var->val)) == NULL) {
            return NULL;
        }
    }
    if ((ret = __mln_lang_var_new(ctx->pool, var->name, var->type, val, var->inSet)) == NULL) {
        if (var->type == M_LANG_VAR_NORMAL && var->val != NULL) {
            __mln_lang_val_free(val);
        }
        return NULL;
    }
    return ret;
}

static inline mln_lang_var_t *
mln_lang_var_dupWithVal(mln_lang_ctx_t *ctx, mln_lang_var_t *var)
{
    mln_lang_var_t *ret;
    mln_lang_val_t *val;
    if ((val = mln_lang_val_dup(ctx, var->val)) == NULL) {
        return NULL;
    }
    if ((ret = __mln_lang_var_new(ctx->pool, var->name, M_LANG_VAR_NORMAL, val, var->inSet)) == NULL) {
        __mln_lang_val_free(val);
        return NULL;
    }
    return ret;
}

static int mln_lang_var_cmpForElemCmp(const void *data1, const void *data2)
{
    mln_lang_var_t *v1 = (mln_lang_var_t *)data1;
    mln_lang_var_t *v2 = (mln_lang_var_t *)data2;
    if (v1 == NULL && v2 != NULL) return -1;
    else if (v1 != NULL && v2 == NULL) return 1;
    if (v1->val == NULL && v2->val != NULL) return -1;
    else if (v1->val != NULL && v2->val == NULL) return 1;
    return mln_lang_val_cmp(v1->val, v2->val);
}

static int mln_lang_var_cmpName(const void *data1, const void *data2)
{
    mln_lang_var_t *v1 = (mln_lang_var_t *)data1;
    mln_lang_var_t *v2 = (mln_lang_var_t *)data2;
    return mln_string_strcmp(v1->name, v2->name);
}

static inline mln_lang_var_t *
mln_lang_var_transform(mln_lang_ctx_t *ctx, mln_lang_var_t *realvar, mln_lang_var_t *defvar)
{
    mln_lang_var_t *var;
    if ((var = (mln_lang_var_t *)mln_alloc_m(ctx->pool, sizeof(mln_lang_var_t))) == NULL) {
        return NULL;
    }
    var->type = defvar->type;
    if (defvar->name != NULL) {
        if ((var->name = mln_string_pool_dup(ctx->pool, defvar->name)) == NULL) {
            mln_alloc_free(var);
            return NULL;
        }
    } else {
        var->name = NULL;
    }
    if (var->type == M_LANG_VAR_NORMAL) {
        if ((var->val = mln_lang_val_dup(ctx, realvar->val)) == NULL) {
            if (var->name != NULL) mln_string_pool_free(var->name);
            mln_alloc_free(var);
            return NULL;
        }
    } else {
        var->val = realvar->val;
    }
    ASSERT(var->val != NULL);
    ++(var->val->ref);
    var->inSet = defvar->inSet;
    if (var->inSet != NULL) ++(var->inSet->ref);
    var->prev = var->next = NULL;
    return var;
}

mln_lang_var_t *mln_lang_var_convert(mln_alloc_t *pool, mln_lang_var_t *var)
{
    return __mln_lang_var_convert(pool, var);
}

static inline mln_lang_var_t *
__mln_lang_var_convert(mln_alloc_t *pool, mln_lang_var_t *var)
{
    mln_lang_var_t *ret;
    if ((ret = (mln_lang_var_t *)mln_alloc_m(pool, sizeof(mln_lang_var_t))) == NULL) {
        return NULL;
    }
    ret->type = M_LANG_VAR_NORMAL;
    ret->name = NULL;
    if (var->name != NULL && (ret->name = mln_string_pool_dup(pool, var->name)) == NULL) {
        mln_alloc_free(ret);
        return NULL;
    }
    ret->val = var->val;
    ++(ret->val->ref);
    ret->inSet = var->inSet;
    ret->prev = ret->next = NULL;
    if (ret->inSet != NULL) ++(ret->inSet->ref);
    return ret;
}

void mln_lang_var_assign(mln_lang_var_t *var, mln_lang_val_t *val)
{
    return __mln_lang_var_assign(var, val);
}

static inline void __mln_lang_var_assign(mln_lang_var_t *var, mln_lang_val_t *val)
{
    ASSERT(val != NULL);
    if (val != NULL) ++(val->ref);
    if (var->val != NULL) __mln_lang_val_free(var->val);
    var->val = val;
}

mln_s32_t mln_lang_var_getValType(mln_lang_var_t *var)
{
    return __mln_lang_var_getValType(var);
}

static inline mln_s32_t __mln_lang_var_getValType(mln_lang_var_t *var)
{
    ASSERT(var && var->val != NULL);
    ASSERT(var->val->type >= 0 && var->val->type <= M_LANG_VAL_TYPE_ARRAY);
    return var->val->type;
}

mln_s64_t mln_lang_var_toInt(mln_lang_var_t *var)
{
    ASSERT(var != NULL && var->val != NULL);
    mln_s64_t i = 0;
    mln_lang_val_t *val = var->val;
    switch (val->type) {
        case M_LANG_VAL_TYPE_NIL:
        case M_LANG_VAL_TYPE_OBJECT:
        case M_LANG_VAL_TYPE_FUNC:
        case M_LANG_VAL_TYPE_ARRAY:
            break;
        case M_LANG_VAL_TYPE_INT:
            i = val->data.i;
            break;
        case M_LANG_VAL_TYPE_BOOL:
            i = val->data.b? 1: 0;
            break;
        case M_LANG_VAL_TYPE_REAL:
            i = val->data.f;
            break;
        case M_LANG_VAL_TYPE_STRING:
        {
            mln_string_t *s = val->data.s;
            mln_u8ptr_t buf = (mln_u8ptr_t)malloc(s->len + 1);
            if (buf == NULL) break;
            memcpy(buf, s->data, s->len);
            buf[s->len] = 0;
#if defined(i386) || defined(MLN_ARM32)
            sscanf((char *)buf, "%lld", &i);
#else
            sscanf((char *)buf, "%ld", &i);
#endif
            free(buf);
            break;
        }
        default:
            ASSERT(0);
            break;
    }
    return i;
}

double mln_lang_var_toReal(mln_lang_var_t *var)
{
    ASSERT(var != NULL && var->val != NULL);
    double r = 0;
    mln_lang_val_t *val = var->val;
    switch (val->type) {
        case M_LANG_VAL_TYPE_NIL:
        case M_LANG_VAL_TYPE_OBJECT:
        case M_LANG_VAL_TYPE_FUNC:
        case M_LANG_VAL_TYPE_ARRAY:
            break;
        case M_LANG_VAL_TYPE_INT:
            r = (double)val->data.i;
            break;
        case M_LANG_VAL_TYPE_BOOL:
            r = val->data.b? 1.0: 0;
            break;
        case M_LANG_VAL_TYPE_REAL:
            r = val->data.f;
            break;
        case M_LANG_VAL_TYPE_STRING:
        {
            mln_string_t *s = val->data.s;
            mln_u8ptr_t buf = (mln_u8ptr_t)malloc(s->len + 1);
            if (buf == NULL) break;
            memcpy(buf, s->data, s->len);
            buf[s->len] = 0;
            sscanf((char *)buf, "%lf", &r);
            free(buf);
            break;
        }
        default:
            ASSERT(0);
            break;
    }
    return r;
}

mln_string_t *mln_lang_var_toString(mln_alloc_t *pool, mln_lang_var_t *var)
{
    ASSERT(var != NULL && var->val != NULL);
    char buf[1024] = {0};
    mln_lang_val_t *val = var->val;
    int n = 0;
    switch (val->type) {
        case M_LANG_VAL_TYPE_NIL:
            n = snprintf(buf, sizeof(buf)-1, "NIL");
            break;
        case M_LANG_VAL_TYPE_OBJECT:
            n = snprintf(buf, sizeof(buf)-1, "Object");
            break;
        case M_LANG_VAL_TYPE_FUNC:
            n = snprintf(buf, sizeof(buf)-1, "Function");
            break;
        case M_LANG_VAL_TYPE_ARRAY:
            n = snprintf(buf, sizeof(buf)-1, "Array");
            break;
        case M_LANG_VAL_TYPE_INT:
#if defined(i386) || defined(MLN_ARM32)
            n = snprintf(buf, sizeof(buf)-1, "%lld", val->data.i);
#else
            n = snprintf(buf, sizeof(buf)-1, "%ld", val->data.i);
#endif
            break;
        case M_LANG_VAL_TYPE_BOOL:
            n = snprintf(buf, sizeof(buf)-1, "%s", val->data.b?"True":"False");
            break;
        case M_LANG_VAL_TYPE_REAL:
            n = snprintf(buf, sizeof(buf)-1, "%lf", val->data.f);
            break;
        case M_LANG_VAL_TYPE_STRING:
            return mln_string_pool_dup(pool, val->data.s);
        default:
            ASSERT(0);
            break;
    }
    mln_string_t tmp;
    mln_string_nSet(&tmp, buf, n);
    return mln_string_pool_dup(pool, &tmp);
}

int mln_lang_var_setValue(mln_lang_ctx_t *ctx, mln_lang_var_t *dest, mln_lang_var_t *src)
{
    return __mln_lang_var_setValue(ctx, dest, src);
}

static inline int
__mln_lang_var_setValue(mln_lang_ctx_t *ctx, mln_lang_var_t *dest, mln_lang_var_t *src)
{
    ASSERT(dest != NULL && dest->val != NULL && src != NULL && src->val != NULL);
    mln_lang_val_t *val1 = dest->val;
    mln_lang_val_t *val2 = src->val;
    mln_lang_val_freeData(val1);
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
}

void mln_lang_var_setInt(mln_lang_var_t *var, mln_s64_t i)
{
    ASSERT(var != NULL && var->val != NULL);
    mln_lang_val_t *val = var->val;
    mln_lang_val_freeData(val);
    val->type = M_LANG_VAL_TYPE_INT;
    val->data.i = i;
}

void mln_lang_var_setReal(mln_lang_var_t *var, double r)
{
    ASSERT(var != NULL && var->val != NULL);
    mln_lang_val_t *val = var->val;
    mln_lang_val_freeData(val);
    val->type = M_LANG_VAL_TYPE_REAL;
    val->data.f = r;
}

void mln_lang_var_setString(mln_lang_var_t *var, mln_string_t *s)
{
    ASSERT(var != NULL && var->val != NULL);
    mln_lang_val_t *val = var->val;
    mln_lang_val_freeData(val);
    val->type = M_LANG_VAL_TYPE_STRING;
    val->data.s = s;
}


static inline int
mln_lang_funcdef_getArgs(mln_alloc_t *pool, \
                         mln_lang_exp_t *exp, \
                         mln_lang_var_t **head, \
                         mln_lang_var_t **tail, \
                         mln_size_t *nargs)
{
    mln_size_t n = 0;
    mln_lang_exp_t *scan;
    mln_lang_var_t *var;
    mln_lang_assign_t *assign;
    mln_lang_logicLow_t *logicLow;
    mln_lang_logicHigh_t *logicHigh;
    mln_lang_relativeLow_t *relativeLow;
    mln_lang_relativeHigh_t *relativeHigh;
    mln_lang_move_t *move;
    mln_lang_addsub_t *addsub;
    mln_lang_muldiv_t *muldiv;
    mln_lang_suffix_t *suffix;
    mln_lang_locate_t *locate;
    mln_lang_spec_t *spec;
    mln_lang_factor_t *factor;
    mln_lang_var_type_t type = M_LANG_VAR_NORMAL;
    for (scan = exp; scan != NULL; ++n, scan = scan->next) {
        assign = scan->assign;
        if (assign->op != M_ASSIGN_NONE) goto err;
        logicLow = assign->left;
        if (logicLow->op != M_LOGICLOW_NONE) goto err;
        logicHigh = logicLow->left;
        if (logicHigh->op != M_LOGICHIGH_NONE) goto err;
        relativeLow = logicHigh->left;
        if (relativeLow->op != M_RELATIVELOW_NONE) goto err;
        relativeHigh = relativeLow->left;
        if (relativeHigh->op != M_RELATIVEHIGH_NONE) goto err;
        move = relativeHigh->left;
        if (move->op != M_MOVE_NONE) goto err;
        addsub = move->left;
        if (addsub->op != M_ADDSUB_NONE) goto err;
        muldiv = addsub->left;
        if (muldiv->op != M_MULDIV_NONE) goto err;
        suffix = muldiv->left;
        if (suffix->op != M_SUFFIX_NONE) goto err;
        locate = suffix->left;
        if (locate->op != M_LOCATE_NONE) goto err;
        spec = locate->left;
        if (spec->op == M_SPEC_REFER) {
            spec = spec->data.spec;
            type = M_LANG_VAR_REFER;
        }
        if (spec->op != M_SPEC_FACTOR) goto err;
        factor = spec->data.factor;
        if (factor->type != M_FACTOR_ID) goto err;

        if ((var = __mln_lang_var_new(pool, factor->data.s_id, type, NULL, NULL)) == NULL) {
            while ((var = *head) != NULL) {
                mln_lang_var_chain_del(head, tail, var);
                __mln_lang_var_free(var);
            }
            return -1;
        }
        mln_lang_var_chain_add(head, tail, var);
    }

    *nargs = n;
    return 0;

err:
    while ((var = *head) != NULL) {
        mln_lang_var_chain_del(head, tail, var);
        __mln_lang_var_free(var);
    }
    mln_log(error, "Invalid argument.");
    return -1;
}


mln_lang_func_detail_t *mln_lang_func_detail_new(mln_alloc_t *pool, \
                                                 mln_lang_funcType_t type, \
                                                 void *data, \
                                                 mln_lang_exp_t *exp)
{
    return __mln_lang_func_detail_new(pool, type, data, exp);
}

static inline mln_lang_func_detail_t *
__mln_lang_func_detail_new(mln_alloc_t *pool, \
                           mln_lang_funcType_t type, \
                           void *data, \
                           mln_lang_exp_t *exp)
{
    mln_lang_func_detail_t *lfd;
    if ((lfd = (mln_lang_func_detail_t *)mln_alloc_m(pool, sizeof(mln_lang_func_detail_t))) == NULL) {
        return NULL;
    }
    lfd->exp = exp;
    lfd->args_head = NULL;
    lfd->args_tail = NULL;
    lfd->nargs = 0;
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
        if (mln_lang_funcdef_getArgs(pool, exp, &(lfd->args_head), &(lfd->args_tail), &(lfd->nargs)) < 0) {
            __mln_lang_func_detail_free(lfd);
            return NULL;
        }
    }
    return lfd;
}

void mln_lang_func_detail_free(mln_lang_func_detail_t *lfd)
{
    return __mln_lang_func_detail_free(lfd);
}

static inline void __mln_lang_func_detail_free(mln_lang_func_detail_t *lfd)
{
    if (lfd == NULL) return;
    mln_lang_var_t *arg;
    while ((arg = lfd->args_head) != NULL) {
        mln_lang_var_chain_del(&(lfd->args_head), &(lfd->args_tail), arg);
        __mln_lang_var_free(arg);
    }
    mln_alloc_free(lfd);
}

static inline mln_lang_func_detail_t *
mln_lang_func_detail_dup(mln_lang_ctx_t *ctx, mln_lang_func_detail_t *func)
{
    mln_lang_func_detail_t *ret;
    mln_lang_var_t *var, *newvar;
    mln_u8ptr_t data;
    switch (func->type) {
        case M_FUNC_INTERNAL:
            data = (mln_u8ptr_t)(func->data.process);
            break;
        default:
            data = (mln_u8ptr_t)(func->data.stm);
            break;
    }
    ret = __mln_lang_func_detail_new(ctx->pool, \
                                     func->type, \
                                     data, \
                                     NULL);
    for (var = func->args_head; var != NULL; var = var->next, ++(ret->nargs)) {
        if ((newvar = __mln_lang_var_dup(ctx, var)) == NULL) {
            __mln_lang_func_detail_free(ret);
            return NULL;
        }
        mln_lang_var_chain_add(&(ret->args_head), &(ret->args_tail), newvar);
    }
    return ret;
}


static inline mln_lang_object_t *
mln_lang_object_new(mln_lang_ctx_t *ctx, mln_lang_set_detail_t *inSet)
{
    struct mln_rbtree_attr rbattr;
    mln_lang_object_t *obj;
    if ((obj = (mln_lang_object_t *)mln_alloc_m(ctx->pool, sizeof(mln_lang_object_t))) == NULL) {
        return NULL;
    }
    obj->inSet = inSet;
    if (inSet != NULL) ++(inSet->ref);
    rbattr.cmp = mln_lang_var_cmpName;
    rbattr.data_free = mln_lang_var_free;
    if ((obj->members = mln_rbtree_init(&rbattr)) == NULL) {
        mln_alloc_free(obj);
        return NULL;
    }
    obj->ref = 0;

    if (inSet != NULL) {
        struct mln_lang_scan_s ls;
        ls.tree = obj->members;
        ls.tree2 = NULL;
        ls.ctx = ctx;
        if (mln_rbtree_scan_all(inSet->members, mln_lang_set_member_scan, &ls) < 0) {
            mln_lang_object_free(obj);
            return NULL;
        }
    }
    obj->gcItem = NULL;
    obj->ctx = ctx;

    mln_gc_t *gc = mln_lang_scope_getCurrentGC(ctx);
    ASSERT(gc);
    if (mln_lang_gc_item_new(ctx->pool, gc, M_GC_OBJ, obj) < 0) {
        mln_lang_object_free(obj);
        obj = NULL;
    }

    return obj;
}

static inline void mln_lang_object_free(mln_lang_object_t *obj)
{
    if (obj == NULL) return;
    if (obj->ref > 1) {
        ASSERT(obj->gcItem);
        if (obj->gcItem->gc != NULL) {
            mln_gc_suspect(obj->gcItem->gc, obj->gcItem);
        }
        --(obj->ref);
        return;
    }
    if (obj->members != NULL) mln_rbtree_destroy(obj->members);
    if (obj->inSet != NULL) mln_lang_set_detail_free(obj->inSet);
    if (obj->gcItem != NULL) {
        mln_gc_t *cur = mln_lang_scope_getCurrentGC(obj->ctx);
        if (obj->gcItem->gc != NULL)
            mln_gc_remove(obj->gcItem->gc, obj->gcItem, cur);
        mln_lang_gc_item_freeImmediatly(obj->gcItem);
    }
    mln_alloc_free(obj);
}

static inline mln_lang_object_t *
mln_lang_object_dup(mln_lang_ctx_t *ctx, mln_lang_object_t *src)
{
    struct mln_rbtree_attr rbattr;
    mln_lang_object_t *obj;
    if ((obj = (mln_lang_object_t *)mln_alloc_m(ctx->pool, sizeof(mln_lang_object_t))) == NULL) {
        return NULL;
    }
    obj->inSet = src->inSet;
    if (obj->inSet != NULL) ++(obj->inSet->ref);
    rbattr.cmp = mln_lang_var_cmpName;
    rbattr.data_free = mln_lang_var_free;
    if ((obj->members = mln_rbtree_init(&rbattr)) == NULL) {
        mln_alloc_free(obj);
        return NULL;
    }
    obj->ref = 0;

    struct mln_lang_scan_s ls;
    ls.tree = obj->members;
    ls.tree2 = NULL;
    ls.ctx = ctx;
    if (mln_rbtree_scan_all(src->members, mln_lang_set_member_scan, &ls) < 0) {
        mln_lang_object_free(obj);
        return NULL;
    }
    obj->gcItem = NULL;
    obj->ctx = ctx;

    mln_gc_t *gc = mln_lang_scope_getCurrentGC(ctx);
    ASSERT(gc);
    if (mln_lang_gc_item_new(ctx->pool, gc, M_GC_OBJ, obj) < 0) {
        mln_lang_object_free(obj);
        obj = NULL;
    }
    return obj;
}


mln_lang_val_t *mln_lang_val_new(mln_alloc_t *pool, mln_s32_t type, void *data)
{
    return __mln_lang_val_new(pool, type, data);
}

static inline mln_lang_val_t *
__mln_lang_val_new(mln_alloc_t *pool, mln_s32_t type, void *data)
{
    mln_lang_val_t *val;
    if ((val = (mln_lang_val_t *)mln_alloc_m(pool, sizeof(mln_lang_val_t))) == NULL) {
        return NULL;
    }
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
        default:
            ASSERT(0);
            mln_alloc_free(val);
            return NULL;
    }
    val->type = type;
    val->ref = 0;
    val->udata = NULL;
    val->func = NULL;
    val->notModify = 0;
    return val;
}

void mln_lang_val_free(mln_lang_val_t *val)
{
    return __mln_lang_val_free(val);
}

static void __mln_lang_val_free(void *data)
{
    if (data == NULL) return;
    mln_lang_val_t *val = (mln_lang_val_t *)data;
    if (val->ref-- > 1) return;
    if (val->udata != val && val->udata != NULL) __mln_lang_val_free(val->udata);
    if (val->func != NULL) __mln_lang_func_detail_free(val->func);
    mln_lang_val_freeData(val);
    mln_alloc_free(val);
}

static inline void mln_lang_val_freeData(mln_lang_val_t *val)
{
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
                mln_string_pool_free(val->data.s);
                val->data.s = NULL;
            }
            break;
        default:
            break;
    }
    val->type = M_LANG_VAL_TYPE_NIL;
}

static inline mln_lang_val_t *
mln_lang_val_dup(mln_lang_ctx_t *ctx, mln_lang_val_t *val)
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
            if ((data = (mln_u8ptr_t)mln_string_pool_dup(ctx->pool, val->data.s)) == NULL) {
                return NULL;
            }
            break;
        case M_LANG_VAL_TYPE_OBJECT:
            if ((data = (mln_u8ptr_t)mln_lang_object_dup(ctx, val->data.obj)) == NULL) {
                return NULL;
            }
            ++(((mln_lang_object_t *)data)->ref);
            break;
        case M_LANG_VAL_TYPE_FUNC:
            if ((data = (mln_u8ptr_t)mln_lang_func_detail_dup(ctx, val->data.func)) == NULL) {
                return NULL;
            }
            break;
        case M_LANG_VAL_TYPE_ARRAY:
            if ((data = (mln_u8ptr_t)mln_lang_array_dup(ctx, val->data.array)) == NULL) {
                return NULL;
            }
            ++(((mln_lang_array_t *)data)->ref);
            break;
        default:
            return NULL;
    }
    mln_lang_val_t *ret = __mln_lang_val_new(ctx->pool, type, data);
    if (ret == NULL) {
        switch (type) {
            case M_LANG_VAL_TYPE_STRING:
                mln_string_pool_free((mln_string_t *)data);
                break;
            case M_LANG_VAL_TYPE_OBJECT:
                --(((mln_lang_object_t *)data)->ref);
                mln_lang_object_free((mln_lang_object_t *)data);
                break;
            case M_LANG_VAL_TYPE_FUNC:
                __mln_lang_func_detail_free((mln_lang_func_detail_t *)data);
                break;
            case M_LANG_VAL_TYPE_ARRAY:
                --(((mln_lang_array_t *)data)->ref);
                __mln_lang_array_free((mln_lang_array_t *)data);
                break;
            default:
                break;
        }
    }
    return ret;
}

static int mln_lang_val_cmp(const void *data1, const void *data2)
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
                mln_lang_funcType_t type = val1->data.func->type;
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
}


mln_lang_array_t *mln_lang_array_new(mln_lang_ctx_t *ctx)
{
    return __mln_lang_array_new(ctx);
}

static inline mln_lang_array_t *__mln_lang_array_new(mln_lang_ctx_t *ctx)
{
    mln_lang_array_t *la;
    struct mln_rbtree_attr rbattr;
    if ((la = (mln_lang_array_t *)mln_alloc_m(ctx->pool, sizeof(mln_lang_array_t))) == NULL) {
        return NULL;
    }
    rbattr.cmp = mln_lang_array_elem_indexCmp;
    rbattr.data_free = mln_lang_array_elem_free;
    if ((la->elems_index = mln_rbtree_init(&rbattr)) == NULL) {
        mln_alloc_free(la);
        return NULL;
    }
    rbattr.cmp = mln_lang_array_elem_keyCmp;
    rbattr.data_free = NULL;
    if ((la->elems_key = mln_rbtree_init(&rbattr)) == NULL) {
        mln_rbtree_destroy(la->elems_index);
        mln_alloc_free(la);
        return NULL;
    }
    la->index = 0;
    la->ref = 0;
    la->gcItem = NULL;
    la->ctx = ctx;

    mln_gc_t *gc = mln_lang_scope_getCurrentGC(ctx);
    ASSERT(gc);
    if (mln_lang_gc_item_new(ctx->pool, gc, M_GC_ARRAY, la) < 0) {
        __mln_lang_array_free(la);
        la = NULL;
    }
    return la;
}

void mln_lang_array_free(mln_lang_array_t *array)
{
    __mln_lang_array_free(array);
}

static inline void __mln_lang_array_free(mln_lang_array_t *array)
{
    if (array == NULL) return;
    if (array->ref > 1) {
        ASSERT(array->gcItem);
        if (array->gcItem->gc != NULL)
            mln_gc_suspect(array->gcItem->gc, array->gcItem);
        --(array->ref);
        return;
    }
    if (array->elems_key != NULL) mln_rbtree_destroy(array->elems_key);
    if (array->elems_index != NULL) mln_rbtree_destroy(array->elems_index);

    if (array->gcItem != NULL) {
        mln_gc_t *cur = mln_lang_scope_getCurrentGC(array->ctx);
        if (array->gcItem->gc != NULL)
            mln_gc_remove(array->gcItem->gc, array->gcItem, cur);
        mln_lang_gc_item_freeImmediatly(array->gcItem);
    }
    mln_alloc_free(array);
}

static inline mln_lang_array_t *
mln_lang_array_dup(mln_lang_ctx_t *ctx, mln_lang_array_t *array)
{
    mln_lang_array_t *ret;
    struct mln_lang_scan_s ls;
    if ((ret = __mln_lang_array_new(ctx)) == NULL) return NULL;
    ls.tree = ret->elems_index;
    ls.tree2 = ret->elems_key;
    ls.ctx = ctx;
    if (mln_rbtree_scan_all(array->elems_index, mln_lang_array_scan_dup, &ls) < 0) {
        __mln_lang_array_free(ret);
        return NULL;
    }
    ret->index = array->index;
    return ret;
}

static int mln_lang_array_scan_dup(mln_rbtree_node_t *node, void *rn_data, void *udata)
{
    mln_rbtree_node_t *rn;
    struct mln_lang_scan_s *ls = (struct mln_lang_scan_s *)udata;
    mln_lang_array_elem_t *lae = (mln_lang_array_elem_t *)rn_data;
    mln_lang_array_elem_t *newlae = mln_lang_array_elem_dup(ls->ctx, lae);
    if (newlae == NULL) return -1;
    if ((rn = mln_rbtree_node_new(ls->tree, newlae)) == NULL) {
        mln_lang_array_elem_free(newlae);
        return -1;
    }
    mln_rbtree_insert(ls->tree, rn);
    if (newlae->key != NULL) {
        if ((rn = mln_rbtree_node_new(ls->tree2, newlae)) == NULL) {
            return -1;
        }
        mln_rbtree_insert(ls->tree2, rn);
    }
    return 0;
}


static int mln_lang_array_elem_indexCmp(const void *data1, const void *data2)
{
    mln_lang_array_elem_t *elem1 = (mln_lang_array_elem_t *)data1;
    mln_lang_array_elem_t *elem2 = (mln_lang_array_elem_t *)data2;
    if (elem1->index > elem2->index) return 1;
    if (elem1->index < elem2->index) return -1;
    return 0;
}

static int mln_lang_array_elem_keyCmp(const void *data1, const void *data2)
{
    mln_lang_array_elem_t *elem1 = (mln_lang_array_elem_t *)data1;
    mln_lang_array_elem_t *elem2 = (mln_lang_array_elem_t *)data2;
    return mln_lang_var_cmpForElemCmp(elem1->key, elem2->key);
}

static inline mln_lang_array_elem_t *
mln_lang_array_elem_new(mln_alloc_t *pool, mln_lang_var_t *key, mln_lang_var_t *val, mln_u64_t index)
{
    mln_lang_array_elem_t *elem;
    if ((elem = (mln_lang_array_elem_t *)mln_alloc_m(pool, sizeof(mln_lang_array_elem_t))) == NULL) {
        return NULL;
    }
    elem->index = index;
    elem->key = key;
    elem->value = val;
    return elem;
}

static inline void mln_lang_array_elem_free(void *data)
{
    if (data == NULL) return;
    mln_lang_array_elem_t *elem = (mln_lang_array_elem_t *)data;
    if (elem->key != NULL) __mln_lang_var_free(elem->key);
    if (elem->value != NULL) __mln_lang_var_free(elem->value);
    mln_alloc_free(elem);
}

static inline mln_lang_array_elem_t *
mln_lang_array_elem_dup(mln_lang_ctx_t *ctx, mln_lang_array_elem_t *lae)
{
    mln_lang_array_elem_t *dup;
    mln_lang_var_t *key = NULL, *val;
    if (lae->key != NULL) {
        if ((key = mln_lang_var_dupWithVal(ctx, lae->key)) == NULL) return NULL;
    }
    if ((val = __mln_lang_var_dup(ctx, lae->value)) == NULL) {
        if (key != NULL) __mln_lang_var_free(key);
        return NULL;
    }
    if ((dup = mln_lang_array_elem_new(ctx->pool, key, val, lae->index)) == NULL) {
        __mln_lang_var_free(val);
        if (key != NULL) __mln_lang_var_free(key);
        return NULL;
    }
    return dup;
}

mln_lang_var_t *
mln_lang_array_getAndNew(mln_lang_ctx_t *ctx, mln_lang_array_t *array, mln_lang_var_t *key)
{
    return __mln_lang_array_getAndNew(ctx, array, key);
}

static inline mln_lang_var_t *
__mln_lang_array_getAndNew(mln_lang_ctx_t *ctx, mln_lang_array_t *array, mln_lang_var_t *key)
{
    mln_s32_t type;
    mln_lang_var_t *ret;

    if (key != NULL && (type = __mln_lang_var_getValType(key)) != M_LANG_VAL_TYPE_NIL) {
        if (type == M_LANG_VAL_TYPE_INT) {
            if ((ret = mln_lang_array_getAndNew_int(ctx, array, key)) == NULL) {
                return NULL;
            }
        } else {
            if ((ret = mln_lang_array_getAndNew_other(ctx, array, key)) == NULL) {
                return NULL;
            }
        }
    } else {
        if ((ret = mln_lang_array_getAndNew_nil(ctx, array)) == NULL) {
            return NULL;
        }
    }
    return ret;
}

static inline mln_lang_var_t *
mln_lang_array_getAndNew_int(mln_lang_ctx_t *ctx, mln_lang_array_t *array, mln_lang_var_t *key)
{
    mln_rbtree_t *tree;
    mln_rbtree_node_t *rn;
    mln_lang_var_t *ret, *nil;
    mln_lang_array_elem_t *elem;
    mln_lang_val_t *val;
    if ((val = __mln_lang_val_new(ctx->pool, M_LANG_VAL_TYPE_NIL, NULL)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        return NULL;
    }
    if ((nil = __mln_lang_var_new(ctx->pool, NULL, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        __mln_lang_val_free(val);
        return NULL;
    }
    if ((elem = mln_lang_array_elem_new(ctx->pool, NULL, nil, key->val->data.i)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        __mln_lang_var_free(nil);
        return NULL;
    }
    tree = array->elems_index;
    rn = mln_rbtree_search(tree, tree->root, elem);
    if (!mln_rbtree_null(rn, tree)) {
        mln_lang_array_elem_free(elem);
        ret = ((mln_lang_array_elem_t *)(rn->data))->value;
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
}

static inline mln_lang_var_t *
mln_lang_array_getAndNew_other(mln_lang_ctx_t *ctx, mln_lang_array_t *array, mln_lang_var_t *key)
{
    mln_rbtree_t *tree;
    mln_rbtree_node_t *rn;
    mln_lang_var_t *ret, *nil, *k;
    mln_lang_array_elem_t *elem;
    mln_lang_val_t *val;
    if ((val = __mln_lang_val_new(ctx->pool, M_LANG_VAL_TYPE_NIL, NULL)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        return NULL;
    }
    if ((nil = __mln_lang_var_new(ctx->pool, NULL, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        __mln_lang_val_free(val);
        return NULL;
    }
    if ((k = mln_lang_var_dupWithVal(ctx, key)) == NULL) {
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
    rn = mln_rbtree_search(tree, tree->root, elem);
    if (!mln_rbtree_null(rn, tree)) {
        mln_lang_array_elem_free(elem);
        ret = ((mln_lang_array_elem_t *)(rn->data))->value;
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
}

static inline mln_lang_var_t *
mln_lang_array_getAndNew_nil(mln_lang_ctx_t *ctx, mln_lang_array_t *array)
{
    mln_rbtree_node_t *rn;
    mln_lang_var_t *nil;
    mln_lang_array_elem_t *elem;
    mln_lang_val_t *val;
    if ((val = __mln_lang_val_new(ctx->pool, M_LANG_VAL_TYPE_NIL, NULL)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        return NULL;
    }
    if ((nil = __mln_lang_var_new(ctx->pool, NULL, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        __mln_lang_val_free(val);
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
}

int mln_lang_array_elem_exist(mln_lang_array_t *array, mln_lang_var_t *key)
{
    mln_lang_val_t *val;
    mln_rbtree_t *tree;
    mln_rbtree_node_t *rn;
    mln_lang_var_t *nil, *k;
    mln_lang_array_elem_t *elem;
    mln_lang_ctx_t *ctx = array->ctx;
    if ((val = __mln_lang_val_new(ctx->pool, M_LANG_VAL_TYPE_NIL, NULL)) == NULL) {
        return 0;
    }
    if ((nil = __mln_lang_var_new(ctx->pool, NULL, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        __mln_lang_val_free(val);
        return 0;
    }
    if (mln_lang_var_getValType(key) == M_LANG_VAL_TYPE_INT) {
        if ((elem = mln_lang_array_elem_new(ctx->pool, NULL, nil, key->val->data.i)) == NULL) {
            __mln_lang_var_free(nil);
            return 0;
        }
        tree = array->elems_index;
        rn = mln_rbtree_search(tree, tree->root, elem);
        mln_lang_array_elem_free(elem);
        if (mln_rbtree_null(rn, tree)) {
            return 0;
        }
        return 1;
    }
    if ((k = mln_lang_var_dupWithVal(ctx, key)) == NULL) {
        __mln_lang_var_free(nil);
        return 0;
    }
    if ((elem = mln_lang_array_elem_new(ctx->pool, k, nil, array->index)) == NULL) {
        __mln_lang_var_free(k);
        __mln_lang_var_free(nil);
        return 0;
    }
    tree = array->elems_key;
    rn = mln_rbtree_search(tree, tree->root, elem);
    mln_lang_array_elem_free(elem);
    if (mln_rbtree_null(rn, tree)) {
        return 0;
    }
    return 1;
}


mln_lang_funccall_val_t *mln_lang_funccall_val_new(mln_alloc_t *pool, mln_string_t *name)
{
    return __mln_lang_funccall_val_new(pool, name);
}

static inline mln_lang_funccall_val_t *__mln_lang_funccall_val_new(mln_alloc_t *pool, mln_string_t *name)
{
    mln_lang_funccall_val_t *func;
    if ((func = (mln_lang_funccall_val_t *)mln_alloc_m(pool, sizeof(mln_lang_funccall_val_t))) == NULL) {
        return NULL;
    }
    if (name != NULL) {
        if ((func->name = mln_string_pool_dup(pool, name)) == NULL) {
            mln_alloc_free(func);
            return NULL;
        }
    } else {
        func->name = NULL;
    }
    func->prototype = NULL;
    func->object = NULL;
    func->args_head = func->args_tail = NULL;
    func->nargs = 0;
    return func;
}

void mln_lang_funccall_val_free(mln_lang_funccall_val_t *func)
{
    return __mln_lang_funccall_val_free(func);
}

static inline void __mln_lang_funccall_val_free(mln_lang_funccall_val_t *func)
{
    if (func == NULL) return;
    mln_lang_var_t *var;
    if (func->name != NULL) mln_string_pool_free(func->name);
    while ((var = func->args_head) != NULL) {
        mln_lang_var_chain_del(&(func->args_head), &(func->args_tail), var);
        __mln_lang_var_free(var);
    }
    if (func->object != NULL) __mln_lang_val_free(func->object);
    mln_alloc_free(func);
}

static inline void mln_lang_funccall_val_addArg(mln_lang_funccall_val_t *func, mln_lang_var_t *var)
{
    mln_lang_var_chain_add(&(func->args_head), &(func->args_tail), var);
    ++(func->nargs);
}

void mln_lang_funccall_val_addObject(mln_lang_funccall_val_t *func, mln_lang_val_t *obj_val)
{
    ASSERT(obj_val != NULL && func->object == NULL);
    ++(obj_val->ref);
    func->object = obj_val;
}


void mln_lang_errmsg(mln_lang_ctx_t *ctx, char *msg)
{
    return __mln_lang_errmsg(ctx, msg);
}

static void __mln_lang_errmsg(mln_lang_ctx_t *ctx, char *msg)
{
    mln_u64_t line = 0;
    mln_string_t *filename, nilname = mln_string(" ");
    mln_lang_stack_node_t *node;
    if ((node = (mln_lang_stack_node_t *)mln_stack_top(ctx->run_stack)) != NULL) {
        switch (node->type) {
            case M_LSNT_STM:
                line = node->data.stm->line; break;
            case M_LSNT_FUNCDEF:
                line = node->data.funcdef->line; break;
            case M_LSNT_SET:
                line = node->data.set->line; break;
            case M_LSNT_SETSTM:
                line = node->data.set_stm->line; break;
            case M_LSNT_BLOCK:
                line = node->data.block->line; break;
            case M_LSNT_WHILE:
                line = node->data.w->line; break;
            case M_LSNT_SWITCH:
                line = node->data.sw->line; break;
            case M_LSNT_SWITCHSTM:
                line = node->data.sw_stm->line; break;
            case M_LSNT_FOR:
                line = node->data.f->line; break;
            case M_LSNT_IF:
                line = node->data.i->line; break;
            case M_LSNT_EXP:
                line = node->data.exp->line; break;
            case M_LSNT_ASSIGN:
                line = node->data.assign->line; break;
            case M_LSNT_LOGICLOW:
                line = node->data.logicLow->line; break;
            case M_LSNT_LOGICHIGH:
                line = node->data.logicHigh->line; break;
            case M_LSNT_RELATIVELOW:
                line = node->data.relativeLow->line; break;
            case M_LSNT_RELATIVEHIGH:
                line = node->data.relativeHigh->line; break;
            case M_LSNT_MOVE:
                line = node->data.move->line; break;
            case M_LSNT_ADDSUB:
                line = node->data.addsub->line; break;
            case M_LSNT_MULDIV:
                line = node->data.muldiv->line; break;
            case M_LSNT_SUFFIX:
                line = node->data.suffix->line; break;
            case M_LSNT_LOCATE:
                line = node->data.locate->line; break;
            case M_LSNT_SPEC:
                line = node->data.spec->line; break;
            case M_LSNT_FACTOR:
                line = node->data.factor->line; break;
            case M_LSNT_ELEMLIST:
                line = node->data.elemlist->line; break;
            default:
                line = node->data.funccall->line; break;
        }
    }
    if (ctx->filename == NULL) filename = &nilname;
    filename = ctx->filename;
    mln_log(error, "%S:%I: %s\n", filename, line, msg);
}

/*
 * stack handlers
 */
static void mln_lang_stack_handler_stm(mln_lang_ctx_t *ctx)
{
    mln_lang_stack_node_t *node = (mln_lang_stack_node_t *)mln_stack_top(ctx->run_stack);
    mln_lang_stm_t *stm = node->data.stm;
in:
    if (node->step == 0) {
        ++(node->step);
        mln_u8ptr_t data;
        mln_lang_stack_node_type_t type;
        switch (stm->type) {
            case M_STM_BLOCK:
                data = (mln_u8ptr_t)(stm->data.block);
                type = M_LSNT_BLOCK;
                break;
            case M_STM_FUNC:
                data = (mln_u8ptr_t)(stm->data.func);
                type = M_LSNT_FUNCDEF;
                break;
            case M_STM_SET:
                data = (mln_u8ptr_t)(stm->data.setdef);
                type = M_LSNT_SET;
                break;
            case M_STM_LABEL:
            {
                mln_lang_label_t *ll = mln_lang_label_new(ctx->pool, stm->data.pos, stm->next);
                if (ll == NULL) {
                    __mln_lang_errmsg(ctx, "No memory.");
                    mln_lang_job_free(ctx);
                    return;
                }
                if (__mln_lang_symbolNode_join(ctx, M_LANG_SYMBOL_LABEL, ll) < 0) {
                    mln_lang_label_free(ll);
                    mln_lang_job_free(ctx);
                    return;
                }
                goto in;
            }
            case M_STM_SWITCH:
                data = (mln_u8ptr_t)(stm->data.sw);
                type = M_LSNT_SWITCH;
                break;
            case M_STM_WHILE:
                data = (mln_u8ptr_t)(stm->data.w);
                type = M_LSNT_WHILE;
                break;
            default:
                data = (mln_u8ptr_t)(stm->data.f);
                type = M_LSNT_FOR;
                break;
        }
        if ((node = mln_lang_stack_node_new(ctx->pool, type, data)) == NULL) {
            __mln_lang_errmsg(ctx, "No memory.");
            mln_lang_job_free(ctx);
            return;
        }
        if (mln_stack_push(ctx->run_stack, node) < 0) {
            __mln_lang_errmsg(ctx, "No memory.");
            __mln_lang_stack_node_free(node);
            mln_lang_job_free(ctx);
            return;
        }
    } else {
        __mln_lang_stack_node_free(mln_stack_pop(ctx->run_stack));
        if (ctx->retExp == NULL && stm->next != NULL) {
            if ((node = mln_lang_stack_node_new(ctx->pool, M_LSNT_STM, stm->next)) == NULL) {
                __mln_lang_errmsg(ctx, "No memory.");
                mln_lang_job_free(ctx);
                return;
            }
            if (mln_stack_push(ctx->run_stack, node) < 0) {
                __mln_lang_errmsg(ctx, "No memory.");
                __mln_lang_stack_node_free(node);
                mln_lang_job_free(ctx);
                return;
            }
        }
    }
    (void)__mln_lang_run(ctx->lang);
}

static void mln_lang_stack_handler_funcdef(mln_lang_ctx_t *ctx)
{
    mln_lang_stack_node_t *node = (mln_lang_stack_node_t *)mln_stack_top(ctx->run_stack);
    mln_lang_funcdef_t *funcdef = node->data.funcdef;
    mln_lang_val_t *val;
    mln_lang_var_t *var;
    mln_lang_func_detail_t *func;
    mln_lang_set_detail_t *inSet = mln_lang_ctx_getClass(ctx);
    if ((func = __mln_lang_func_detail_new(ctx->pool, \
                                         M_FUNC_EXTERNAL, \
                                         funcdef->stm, \
                                         funcdef->args)) == NULL)
    {
        __mln_lang_errmsg(ctx, "Parse function definition failed.");
        mln_lang_job_free(ctx);
        return;
    }
    if ((val = __mln_lang_val_new(ctx->pool, M_LANG_VAL_TYPE_FUNC, func)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        __mln_lang_func_detail_free(func);
        mln_lang_job_free(ctx);
        return;
    }
    if ((var = __mln_lang_var_new(ctx->pool, funcdef->name, M_LANG_VAR_NORMAL, val, inSet)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        __mln_lang_val_free(val);
        mln_lang_job_free(ctx);
        return;
    }
    if (inSet != NULL) {
        if (mln_lang_set_member_add(ctx->pool, inSet->members, var) < 0) {
            __mln_lang_errmsg(ctx, "Add member failed. No memory.");
            __mln_lang_var_free(var);
            mln_lang_job_free(ctx);
            return;
        }
    } else {
        if (__mln_lang_symbolNode_join(ctx, M_LANG_SYMBOL_VAR, var) < 0) {
            __mln_lang_var_free(var);
            mln_lang_job_free(ctx);
            return;
        }
    }
    __mln_lang_stack_node_free(mln_stack_pop(ctx->run_stack));
    (void)__mln_lang_run(ctx->lang);
}

static void mln_lang_stack_handler_set(mln_lang_ctx_t *ctx)
{
    mln_lang_stack_node_t *node = (mln_lang_stack_node_t *)mln_stack_top(ctx->run_stack);
    mln_lang_set_t *set = node->data.set;
    mln_lang_set_detail_t *s_detail;
    mln_lang_scope_t *scope;
    if (node->step == 0) {
        if ((s_detail = mln_lang_set_detail_new(ctx->pool, set->name)) == NULL) {
            __mln_lang_errmsg(ctx, "No memory.");
            mln_lang_job_free(ctx);
            return;
        }
        ++(s_detail->ref);
        if (__mln_lang_symbolNode_join(ctx, M_LANG_SYMBOL_SET, s_detail) < 0) {
            mln_lang_set_detail_freeSelf(s_detail);
            mln_lang_job_free(ctx);
            return;
        }
        if (set->stm != NULL) {
            node->step = 1;
            if ((scope = mln_lang_scope_new(ctx, s_detail->name, M_LANG_SCOPE_TYPE_SET, NULL)) == NULL) {
                __mln_lang_errmsg(ctx, "No memory.");
                mln_lang_job_free(ctx);
                return;
            }
            mln_lang_scope_chain_add(&(ctx->scope_head), &(ctx->scope_tail), scope);
            if ((node = mln_lang_stack_node_new(ctx->pool, M_LSNT_SETSTM, set->stm)) == NULL) {
                __mln_lang_errmsg(ctx, "No memory.");
                mln_lang_job_free(ctx);
                return;
            }
            if (mln_stack_push(ctx->run_stack, node) < 0) {
                __mln_lang_errmsg(ctx, "No memory.");
                __mln_lang_stack_node_free(node);
                mln_lang_job_free(ctx);
                return;
            }
        } else {
            __mln_lang_stack_node_free(mln_stack_pop(ctx->run_stack));
        }
    } else {
        scope = ctx->scope_tail;
        mln_lang_scope_chain_del(&(ctx->scope_head), &(ctx->scope_tail), scope);
        mln_lang_scope_free(scope);
        __mln_lang_stack_node_free(mln_stack_pop(ctx->run_stack));
    }
    (void)__mln_lang_run(ctx->lang);
}

static void mln_lang_stack_handler_setstm(mln_lang_ctx_t *ctx)
{
    mln_lang_stack_node_t *node = (mln_lang_stack_node_t *)mln_stack_top(ctx->run_stack);
    mln_lang_setstm_t *ls = node->data.set_stm;
    mln_lang_var_t *var;
    mln_lang_val_t *val;
    if (node->step == 0) {
        node->step = 1;
        if (ls->type == M_SETSTM_VAR) {
            mln_lang_set_detail_t *inSet = mln_lang_ctx_getClass(ctx);
            if ((val = __mln_lang_val_new(ctx->pool, M_LANG_VAL_TYPE_NIL, NULL)) == NULL) {
                __mln_lang_errmsg(ctx, "No memory.");
                mln_lang_job_free(ctx);
                return;
            }
            if ((var = __mln_lang_var_new(ctx->pool, ls->data.var, M_LANG_VAR_NORMAL, val, inSet)) == NULL) {
                __mln_lang_errmsg(ctx, "No memory.");
                __mln_lang_val_free(val);
                mln_lang_job_free(ctx);
                return;
            }
            if (inSet == NULL) {
                if (__mln_lang_symbolNode_join(ctx, M_LANG_SYMBOL_VAR, var) < 0) {
                    __mln_lang_errmsg(ctx, "No memory.");
                    __mln_lang_var_free(var);
                    mln_lang_job_free(ctx);
                    return;
                }
            } else {
                if (mln_lang_set_member_add(ctx->pool, inSet->members, var) < 0) {
                    __mln_lang_errmsg(ctx, "No memory.");
                    __mln_lang_var_free(var);
                    mln_lang_job_free(ctx);
                    return;
                }
            }
        } else {
            if ((node = mln_lang_stack_node_new(ctx->pool, M_LSNT_FUNCDEF, ls->data.func)) == NULL) {
                __mln_lang_errmsg(ctx, "No memory.");
                mln_lang_job_free(ctx);
                return;
            }
            if (mln_stack_push(ctx->run_stack, node) < 0) {
                __mln_lang_errmsg(ctx, "No memory.");
                __mln_lang_stack_node_free(node);
                mln_lang_job_free(ctx);
                return;
            }
        }
    } else {
        __mln_lang_stack_node_free(mln_stack_pop(ctx->run_stack));
        if (ls->next != NULL) {
            if ((node = mln_lang_stack_node_new(ctx->pool, M_LSNT_SETSTM, ls->next)) == NULL) {
                __mln_lang_errmsg(ctx, "No memory.");
                mln_lang_job_free(ctx);
                return;
            }
            if (mln_stack_push(ctx->run_stack, node) < 0) {
                __mln_lang_errmsg(ctx, "No memory.");
                __mln_lang_stack_node_free(node);
                mln_lang_job_free(ctx);
                return;
            }
        }
    }
    (void)__mln_lang_run(ctx->lang);
}

static void mln_lang_stack_handler_block(mln_lang_ctx_t *ctx)
{
    mln_lang_stack_node_t *node = (mln_lang_stack_node_t *)mln_stack_top(ctx->run_stack);
    mln_lang_block_t *block = node->data.block;
    if (node->step == 0) {
        mln_lang_ctx_resetRetExp(ctx);
        ++(node->step);
        switch (block->type) {
            case M_BLOCK_EXP:
                if (block->data.exp == NULL) goto nil;
                if (mln_lang_stack_handler_block_exp(ctx, block) < 0) {
                    mln_lang_job_free(ctx);
                    return;
                }
                break;
            case M_BLOCK_STM:
                if (block->data.stm == NULL) goto nil;
                if (mln_lang_stack_handler_block_stm(ctx, block) < 0) {
                    mln_lang_job_free(ctx);
                    return;
                }
                break;
            case M_BLOCK_CONTINUE:
                if (mln_lang_stack_handler_block_continue(ctx, block) < 0) {
                    mln_lang_job_free(ctx);
                    return;
                }
                break;
            case M_BLOCK_BREAK:
                if (mln_lang_stack_handler_block_break(ctx, block) < 0) {
                    mln_lang_job_free(ctx);
                    return;
                }
                break;
            case M_BLOCK_RETURN:
                if (mln_lang_stack_handler_block_return(ctx, block) < 0) {
                    mln_lang_job_free(ctx);
                    return;
                }
                break;
            case M_BLOCK_GOTO:
                if (mln_lang_stack_handler_block_goto(ctx, block) < 0) {
                    mln_lang_job_free(ctx);
                    return;
                }
                break;
            default:
                if (mln_lang_stack_handler_block_if(ctx, block) < 0) {
                    mln_lang_job_free(ctx);
                    return;
                }
                break;
        }
    } else {
nil:
        if (block->type != M_BLOCK_RETURN) {
            mln_lang_ctx_resetRetExp(ctx);
            __mln_lang_stack_node_free(mln_stack_pop(ctx->run_stack));
        } else {
            if (mln_lang_metReturn(ctx) < 0) {
                mln_lang_job_free(ctx);
                return;
            }
        }
    }
    (void)__mln_lang_run(ctx->lang);
}

static inline int mln_lang_metReturn(mln_lang_ctx_t *ctx)
{
    mln_lang_stack_node_t *node, *last;
    ASSERT(ctx->scope_tail->type == M_LANG_SCOPE_TYPE_FUNC);
    mln_lang_scope_t *scope = ctx->scope_tail;
    last = (mln_lang_stack_node_t *)mln_stack_pop(ctx->run_stack);
    ASSERT(last);
    while (1) {
        node = (mln_lang_stack_node_t *)mln_stack_top(ctx->run_stack);
        if (node == NULL || scope->cur_stack == node) {
            if (mln_stack_push(ctx->run_stack, last) < 0) {
                __mln_lang_errmsg(ctx, "No memory.");
                __mln_lang_stack_node_free(last);
                return -1;
            }
            ASSERT(last->type == M_LSNT_STM);
            last->step = 1;
            break;
        }
        __mln_lang_stack_node_free(last);
        last = mln_stack_pop(ctx->run_stack);
    }
    if (ctx->retExp == NULL) {
        mln_lang_retExp_t *retExp;
        if ((retExp = __mln_lang_retExp_createTmpNil(ctx->pool, NULL)) == NULL) {
            __mln_lang_errmsg(ctx, "No memory.");
            return -1;
        }
        mln_lang_ctx_setRetExp(ctx, retExp);
    } else {
        mln_lang_var_t *var;
        mln_lang_retExp_t *retExp;
        if ((var = __mln_lang_var_dup(ctx, ctx->retExp->data.var)) == NULL) {
            __mln_lang_errmsg(ctx, "No memory.");
            return -1;
        }
        if ((retExp = __mln_lang_retExp_new(ctx->pool, M_LANG_RETEXP_VAR, var)) == NULL) {
            __mln_lang_errmsg(ctx, "No memory.");
            __mln_lang_var_free(var);
            return -1;
        }
        mln_lang_ctx_setRetExp(ctx, retExp);
    }
    return 0;
}

static inline int mln_lang_stack_handler_block_exp(mln_lang_ctx_t *ctx, mln_lang_block_t *block)
{
    mln_lang_stack_node_t *node;
    if ((node = mln_lang_stack_node_new(ctx->pool, M_LSNT_EXP, block->data.exp)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if (mln_stack_push(ctx->run_stack, node) < 0) {
        __mln_lang_errmsg(ctx, "No memory.");
        __mln_lang_stack_node_free(node);
        return -1;
    }
    return 0;
}

static inline int mln_lang_stack_handler_block_stm(mln_lang_ctx_t *ctx, mln_lang_block_t *block)
{
    mln_lang_stack_node_t *node;
    if ((node = mln_lang_stack_node_new(ctx->pool, M_LSNT_STM, block->data.stm)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if (mln_stack_push(ctx->run_stack, node) < 0) {
        __mln_lang_errmsg(ctx, "No memory.");
        __mln_lang_stack_node_free(node);
        return -1;
    }
    return 0;
}

static inline int mln_lang_stack_handler_block_continue(mln_lang_ctx_t *ctx, mln_lang_block_t *block)
{
    mln_lang_stack_node_t *node = mln_lang_withdrawToLoop(ctx);
    if (node == NULL) {
        __mln_lang_errmsg(ctx, "Invalid 'continue' usage.");
        return -1;
    }
    return 0;
}

static inline int mln_lang_stack_handler_block_break(mln_lang_ctx_t *ctx, mln_lang_block_t *block)
{
    mln_lang_stack_node_t *node = mln_lang_withdrawBreakLoop(ctx);
    if (node == NULL) {
        __mln_lang_errmsg(ctx, "Invalid 'break' usage.");
        return -1;
    }
    node->step = 1;
    return 0;
}

static inline int mln_lang_stack_handler_block_return(mln_lang_ctx_t *ctx, mln_lang_block_t *block)
{
    mln_lang_ctx_resetRetExp(ctx);
    if (block->data.exp == NULL) {
        mln_lang_retExp_t *retExp;
        if ((retExp = mln_lang_retExp_createTmpNil(ctx->pool, NULL)) == NULL) {
            __mln_lang_errmsg(ctx, "No memory.");
            return -1;
        }
        mln_lang_ctx_setRetExp(ctx, retExp);
        return 0;
    }
    mln_lang_stack_node_t *node;
    if ((node = mln_lang_stack_node_new(ctx->pool, M_LSNT_EXP, block->data.exp)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if (mln_stack_push(ctx->run_stack, node) < 0) {
        __mln_lang_errmsg(ctx, "No memory.");
        __mln_lang_stack_node_free(node);
        return -1;
    }
    return 0;
}

static inline int mln_lang_stack_handler_block_goto(mln_lang_ctx_t *ctx, mln_lang_block_t *block)
{
    mln_string_t *pos = block->data.pos;
    mln_lang_symbolNode_t *sym;
    mln_lang_stack_node_t *node;
    mln_lang_stm_t *stm;
    ASSERT(ctx->scope_tail->type == M_LANG_SCOPE_TYPE_FUNC);
    mln_lang_ctx_resetRetExp(ctx);
    if ((sym = __mln_lang_symbolNode_search(ctx, pos, 1)) != NULL) {
        if (sym->type != M_LANG_SYMBOL_LABEL) {
            __mln_lang_errmsg(ctx, "Invalid a label.");
            return -1;
        }
        stm = sym->data.label->stm;
    } else {
        while (1) {
            node = (mln_lang_stack_node_t *)mln_stack_pop(ctx->run_stack);
            ASSERT(node != NULL);
            if (node->type == M_LSNT_STM) {
                stm = node->data.stm->next;
                __mln_lang_stack_node_free(node);
                break;
            }
            __mln_lang_stack_node_free(node);
        }
        for (; stm != NULL; stm = stm->next) {
            if (stm->type == M_STM_LABEL && !mln_string_strcmp(stm->data.pos, pos)) {
                mln_lang_label_t *l;
                stm = stm->next;
                if ((l = mln_lang_label_new(ctx->pool, pos, stm)) == NULL) {
                    __mln_lang_errmsg(ctx, "No memory.");
                    return -1;
                }
                if (__mln_lang_symbolNode_join(ctx, M_LANG_SYMBOL_LABEL, l) < 0) {
                    __mln_lang_errmsg(ctx, "No memory.");
                    mln_lang_label_free(l);
                    return -1;
                }
                break;
            }
        }
        if (stm == NULL) {
            __mln_lang_errmsg(ctx, "Invalid a label.");
            return -1;
        }
    }
    if ((node = mln_lang_stack_node_new(ctx->pool, M_LSNT_STM, stm)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if (mln_stack_push(ctx->run_stack, node) < 0) {
        __mln_lang_errmsg(ctx, "No memory.");
        __mln_lang_stack_node_free(node);
        return -1;
    }
    return 0;
}

static inline int mln_lang_stack_handler_block_if(mln_lang_ctx_t *ctx, mln_lang_block_t *block)
{
    mln_lang_stack_node_t *node;
    if ((node = mln_lang_stack_node_new(ctx->pool, M_LSNT_IF, block->data.i)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if (mln_stack_push(ctx->run_stack, node) < 0) {
        __mln_lang_errmsg(ctx, "No memory.");
        __mln_lang_stack_node_free(node);
        return -1;
    }
    return 0;
}

static inline mln_lang_stack_node_t *mln_lang_withdrawToLoop(mln_lang_ctx_t *ctx)
{
    mln_lang_ctx_resetRetExp(ctx);
    mln_lang_stack_node_t *node;
    while (1) {
        node = (mln_lang_stack_node_t *)mln_stack_top(ctx->run_stack);
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
        __mln_lang_stack_node_free(mln_stack_pop(ctx->run_stack));
    }
    return node;
}

static inline mln_lang_stack_node_t *mln_lang_withdrawBreakLoop(mln_lang_ctx_t *ctx)
{
    mln_lang_ctx_resetRetExp(ctx);
    mln_lang_stack_node_t *node;
    mln_lang_stm_t *stm;
    while (1) {
        node = (mln_lang_stack_node_t *)mln_stack_top(ctx->run_stack);
        if (node == NULL) {
            __mln_lang_errmsg(ctx, "break or continue not in loop statement.");
            return NULL;
        }
        if (node->type != M_LSNT_STM) {
            __mln_lang_stack_node_free(mln_stack_pop(ctx->run_stack));
            continue;
        }
        stm = node->data.stm;
        if (stm->type != M_STM_SWITCH && stm->type != M_STM_WHILE && stm->type != M_STM_FOR) {
            __mln_lang_stack_node_free(mln_stack_pop(ctx->run_stack));
            continue;
        }
        break;
    }
    return node;
}

static inline int mln_lang_withdrawUntilFunc(mln_lang_ctx_t *ctx)
{
    mln_lang_stack_node_t *node;
    ASSERT(ctx->scope_tail->type == M_LANG_SCOPE_TYPE_FUNC);
    mln_lang_scope_t *scope = ctx->scope_tail;
    while (1) {
        node = (mln_lang_stack_node_t *)mln_stack_top(ctx->run_stack);
        if (node == NULL) break;
        if (scope->cur_stack == node) {
            break;
        }
        __mln_lang_stack_node_free(mln_stack_pop(ctx->run_stack));
    }
    mln_lang_scope_chain_del(&(ctx->scope_head), &(ctx->scope_tail), scope);
    mln_lang_scope_free(scope);
    if (ctx->retExp == NULL) {
        mln_lang_retExp_t *retExp;
        if ((retExp = __mln_lang_retExp_createTmpNil(ctx->pool, NULL)) == NULL) {
            __mln_lang_errmsg(ctx, "No memory.");
            return -1;
        }
        mln_lang_ctx_setRetExp(ctx, retExp);
    }
    return 0;
}

static void mln_lang_stack_handler_while(mln_lang_ctx_t *ctx)
{
    mln_lang_stack_node_t *node = (mln_lang_stack_node_t *)mln_stack_top(ctx->run_stack);
    mln_lang_while_t *w = node->data.w;
    mln_lang_retExp_t *retExp;
    if (node->step == 0) {
        node->step = 1;
        if (w->condition == NULL) {
            if ((retExp = __mln_lang_retExp_createTmpTrue(ctx->pool, NULL)) == NULL) {
                __mln_lang_errmsg(ctx, "No memory.");
                mln_lang_job_free(ctx);
                return;
            }
            mln_lang_ctx_setRetExp(ctx, retExp);
        } else {
            if ((node = mln_lang_stack_node_new(ctx->pool, M_LSNT_EXP, w->condition)) == NULL) {
                __mln_lang_errmsg(ctx, "No memory.");
                mln_lang_job_free(ctx);
                return;
            }
            if (mln_stack_push(ctx->run_stack, node) < 0) {
                __mln_lang_errmsg(ctx, "No memory.");
                __mln_lang_stack_node_free(node);
                mln_lang_job_free(ctx);
                return;
            }
        }
    } else if (node->step == 1) {
        retExp = ctx->retExp;
        ASSERT(retExp->type == M_LANG_RETEXP_VAR);
        if (__mln_lang_condition_isTrue(retExp->data.var)) {
            node->step = 0;
            if (w->blockstm != NULL) {
                if ((node = mln_lang_stack_node_new(ctx->pool, M_LSNT_BLOCK, w->blockstm)) == NULL) {
                    __mln_lang_errmsg(ctx, "No memory.");
                    mln_lang_job_free(ctx);
                    return;
                }
                if (mln_stack_push(ctx->run_stack, node) < 0) {
                    __mln_lang_errmsg(ctx, "No memory.");
                    __mln_lang_stack_node_free(node);
                    mln_lang_job_free(ctx);
                    return;
                }
            }
        } else {
            node->step = 2;
        }
    } else {
        mln_lang_ctx_resetRetExp(ctx);
        __mln_lang_stack_node_free(mln_stack_pop(ctx->run_stack));
    }
    (void)__mln_lang_run(ctx->lang);
}

int mln_lang_condition_isTrue(mln_lang_var_t *var)
{
    return __mln_lang_condition_isTrue(var);
}

static inline int __mln_lang_condition_isTrue(mln_lang_var_t *var)
{
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
            if (val->data.array != NULL && val->data.array->elems_index->nr_node > 0) return 1;
            break;
        default:
            mln_log(error, "shouldn't be here. %X\n", val->type);
            abort();
    }
    return 0;
}

static void mln_lang_stack_handler_switch(mln_lang_ctx_t *ctx)
{
    mln_lang_stack_node_t *node = (mln_lang_stack_node_t *)mln_stack_top(ctx->run_stack);
    mln_lang_switch_t *sw = node->data.sw;
    if (node->step == 0) {
        node->step = sw->switchstm == NULL? 2: 1;
        if (sw->condition == NULL) {
            mln_lang_ctx_setRetExp(ctx, NULL);
        } else {
            if ((node = mln_lang_stack_node_new(ctx->pool, M_LSNT_EXP, sw->condition)) == NULL) {
                __mln_lang_errmsg(ctx, "No memory.");
                mln_lang_job_free(ctx);
                return;
            }
            if (mln_stack_push(ctx->run_stack, node) < 0) {
                __mln_lang_errmsg(ctx, "No memory.");
                __mln_lang_stack_node_free(node);
                mln_lang_job_free(ctx);
                return;
            }
        }
    } else if (node->step == 1) {
        node->step = 2;
        mln_lang_stack_node_getRetExpFromCTX(node, ctx);
        if ((node = mln_lang_stack_node_new(ctx->pool, M_LSNT_SWITCHSTM, sw->switchstm)) == NULL) {
            __mln_lang_errmsg(ctx, "No memory.");
            mln_lang_job_free(ctx);
            return;
        }
        if (mln_stack_push(ctx->run_stack, node) < 0) {
            __mln_lang_errmsg(ctx, "No memory.");
            __mln_lang_stack_node_free(node);
            mln_lang_job_free(ctx);
            return;
        }
    } else {
        __mln_lang_stack_node_free(mln_stack_pop(ctx->run_stack));
    }
    (void)__mln_lang_run(ctx->lang);
}

static void mln_lang_stack_handler_switchstm(mln_lang_ctx_t *ctx)
{
    mln_lang_retExp_t *res = NULL;
    mln_lang_stack_node_t *node = (mln_lang_stack_node_t *)mln_stack_top(ctx->run_stack);
    mln_lang_switchstm_t *sw_stm = node->data.sw_stm;
    if (node->step == 0) {
        node->step = 1;
        mln_lang_ctx_resetRetExp(ctx);
        node = (mln_lang_stack_node_t *)mln_stack_pop(ctx->run_stack);
        mln_lang_stack_node_t *sw_node = (mln_lang_stack_node_t *)mln_stack_top(ctx->run_stack);
        if (mln_stack_push(ctx->run_stack, node) < 0) {
            __mln_lang_errmsg(ctx, "No memory.");
            __mln_lang_stack_node_free(node);
            mln_lang_job_free(ctx);
            return;
        }
        ASSERT(sw_node != NULL && sw_node->type == M_LSNT_SWITCH);
        if (sw_node->retExp != NULL && sw_stm->factor != NULL) {
            if ((node = mln_lang_stack_node_new(ctx->pool, M_LSNT_FACTOR, sw_stm->factor)) == NULL) {
                __mln_lang_errmsg(ctx, "No memory.");
                mln_lang_job_free(ctx);
                return;
            }
            if (mln_stack_push(ctx->run_stack, node) < 0) {
                __mln_lang_errmsg(ctx, "No memory.");
                __mln_lang_stack_node_free(node);
                mln_lang_job_free(ctx);
                return;
            }
        } else {
            node->step = sw_stm->stm==NULL? 4: 3;
        }
    } else if (node->step == 1) {
        node = (mln_lang_stack_node_t *)mln_stack_pop(ctx->run_stack);
        mln_lang_stack_node_t *sw_node = (mln_lang_stack_node_t *)mln_stack_top(ctx->run_stack);
        if (mln_stack_push(ctx->run_stack, node) < 0) {
            __mln_lang_errmsg(ctx, "No memory.");
            __mln_lang_stack_node_free(node);
            mln_lang_job_free(ctx);
            return;
        }
        ASSERT(sw_node != NULL && sw_node->type == M_LSNT_SWITCH);
        ASSERT(ctx->retExp->type == M_LANG_RETEXP_VAR);
        mln_lang_op handler;
        mln_lang_method_t *method;
        method = mln_lang_methods[__mln_lang_var_getValType(sw_node->retExp->data.var)];
        if (method == NULL) {
            __mln_lang_errmsg(ctx, "Operation NOT support.");
            mln_lang_job_free(ctx);
            return;
        }
        handler = method->equal_handler;
        if (handler == NULL) {
            __mln_lang_errmsg(ctx, "Operation NOT support.");
            mln_lang_job_free(ctx);
            return;
        }
        if (handler(ctx, &res, sw_node->retExp, ctx->retExp) < 0) {
            mln_lang_job_free(ctx);
            return;
        }
        mln_lang_ctx_setRetExp(ctx, res);
        if (res->type == M_LANG_RETEXP_FUNC) {
            node->call = 1;
again:
            node->step = 2;
            if (mln_lang_stack_handler_funccall_run(ctx, node, res->data.func) < 0) {
                mln_lang_job_free(ctx);
                return;
            }
        } else {
            node->step = 2;
        }
    } else if (node->step == 2) {
        ASSERT(ctx->retExp != NULL && ctx->retExp->type == M_LANG_RETEXP_VAR);
        node = (mln_lang_stack_node_t *)mln_stack_pop(ctx->run_stack);
        mln_lang_stack_node_t *sw_node = (mln_lang_stack_node_t *)mln_stack_top(ctx->run_stack);
        if (mln_stack_push(ctx->run_stack, node) < 0) {
            __mln_lang_errmsg(ctx, "No memory.");
            __mln_lang_stack_node_free(node);
            mln_lang_job_free(ctx);
            return;
        }
        if (node->call) {
            if (mln_lang_withdrawUntilFunc(ctx) < 0) {
                mln_lang_job_free(ctx);
                return;
            }
            res = ctx->retExp;
            if (ctx->retExp->type == M_LANG_RETEXP_FUNC) goto again;
        }
        if (__mln_lang_condition_isTrue(ctx->retExp->data.var)) {
            mln_lang_stack_node_resetRetExp(sw_node);
            node->step = 3;
        } else {
            node->step = 4;
        }
        mln_lang_ctx_resetRetExp(ctx);
    } else if (node->step == 3) {
        ASSERT(ctx->retExp == NULL && sw_stm->stm != NULL);
        node->step = 4;
        if ((node = mln_lang_stack_node_new(ctx->pool, M_LSNT_STM, sw_stm->stm)) == NULL) {
            __mln_lang_errmsg(ctx, "No memory.");
            mln_lang_job_free(ctx);
            return;
        }
        if (mln_stack_push(ctx->run_stack, node) < 0) {
            __mln_lang_errmsg(ctx, "No memory.");
            __mln_lang_stack_node_free(node);
            mln_lang_job_free(ctx);
            return;
        }
    } else {
        __mln_lang_stack_node_free(mln_stack_pop(ctx->run_stack));
        if (sw_stm->next != NULL) {
            if ((node = mln_lang_stack_node_new(ctx->pool, M_LSNT_SWITCHSTM, sw_stm->next)) == NULL) {
                __mln_lang_errmsg(ctx, "No memory.");
                mln_lang_job_free(ctx);
                return;
            }
            if (mln_stack_push(ctx->run_stack, node) < 0) {
                __mln_lang_errmsg(ctx, "No memory.");
                __mln_lang_stack_node_free(node);
                mln_lang_job_free(ctx);
                return;
            }
        }
    }
    (void)__mln_lang_run(ctx->lang);
}

static void mln_lang_stack_handler_for(mln_lang_ctx_t *ctx)
{
    mln_lang_stack_node_t *node = (mln_lang_stack_node_t *)mln_stack_top(ctx->run_stack);
    mln_lang_for_t *f = node->data.f;
    if (node->step == 0) {
        mln_lang_ctx_resetRetExp(ctx);
        node->step = 1;
        if (f->init_exp != NULL) {
            if ((node = mln_lang_stack_node_new(ctx->pool, M_LSNT_EXP, f->init_exp)) == NULL) {
                __mln_lang_errmsg(ctx, "No memory.");
                mln_lang_job_free(ctx);
                return;
            }
            if (mln_stack_push(ctx->run_stack, node) < 0) {
                __mln_lang_errmsg(ctx, "No memory.");
                __mln_lang_stack_node_free(node);
                mln_lang_job_free(ctx);
                return;
            }
        }
    } else if (node->step == 1) {
        mln_lang_ctx_resetRetExp(ctx);
        node->step = 2;
        if (f->condition != NULL) {
            if ((node = mln_lang_stack_node_new(ctx->pool, M_LSNT_EXP, f->condition)) == NULL) {
                __mln_lang_errmsg(ctx, "No memory.");
                mln_lang_job_free(ctx);
                return;
            }
            if (mln_stack_push(ctx->run_stack, node) < 0) {
                __mln_lang_errmsg(ctx, "No memory.");
                __mln_lang_stack_node_free(node);
                mln_lang_job_free(ctx);
                return;
            }
        }
    } else if (node->step == 2) {
        mln_lang_retExp_t *retExp = ctx->retExp;
        if (retExp != NULL) ASSERT(retExp->type == M_LANG_RETEXP_VAR);
        if (retExp == NULL || __mln_lang_condition_isTrue(retExp->data.var)) {
            node->step = 3;
            if ((node = mln_lang_stack_node_new(ctx->pool, M_LSNT_BLOCK, f->blockstm)) == NULL) {
                __mln_lang_errmsg(ctx, "No memory.");
                mln_lang_job_free(ctx);
                return;
            }
            if (mln_stack_push(ctx->run_stack, node) < 0) {
                __mln_lang_errmsg(ctx, "No memory.");
                __mln_lang_stack_node_free(node);
                mln_lang_job_free(ctx);
                return;
            }
        } else {
            node->step = 4;
        }
        mln_lang_ctx_resetRetExp(ctx);
    } else if (node->step == 3) {
        mln_lang_ctx_resetRetExp(ctx);
        node->step = 1;
        if (f->mod_exp != NULL) {
            if ((node = mln_lang_stack_node_new(ctx->pool, M_LSNT_EXP, f->mod_exp)) == NULL) {
                __mln_lang_errmsg(ctx, "No memory.");
                mln_lang_job_free(ctx);
                return;
            }
            if (mln_stack_push(ctx->run_stack, node) < 0) {
                __mln_lang_errmsg(ctx, "No memory.");
                __mln_lang_stack_node_free(node);
                mln_lang_job_free(ctx);
                return;
            }
        }
    } else {
        mln_lang_ctx_resetRetExp(ctx);
        __mln_lang_stack_node_free(mln_stack_pop(ctx->run_stack));
    }
    (void)__mln_lang_run(ctx->lang);
}

static void mln_lang_stack_handler_if(mln_lang_ctx_t *ctx)
{
    mln_lang_stack_node_t *node = (mln_lang_stack_node_t *)mln_stack_top(ctx->run_stack);
    mln_lang_if_t *i = node->data.i;
    if (node->step == 0) {
        node->step = 1;
        mln_lang_ctx_resetRetExp(ctx);
        if (i->condition != NULL) {
            if ((node = mln_lang_stack_node_new(ctx->pool, M_LSNT_EXP, i->condition)) == NULL) {
                __mln_lang_errmsg(ctx, "No memory.");
                mln_lang_job_free(ctx);
                return;
            }
            if (mln_stack_push(ctx->run_stack, node) < 0) {
                __mln_lang_errmsg(ctx, "No memory.");
                __mln_lang_stack_node_free(node);
                mln_lang_job_free(ctx);
                return;
            }
        }
    } else if (node->step == 1) {
        node->step = 2;
        mln_lang_retExp_t *retExp = ctx->retExp;
        if (retExp != NULL) ASSERT(retExp->type == M_LANG_RETEXP_VAR);
        if (!__mln_lang_condition_isTrue(retExp->data.var)) {
            if (i->elsestm != NULL) {
                if ((node = mln_lang_stack_node_new(ctx->pool, M_LSNT_BLOCK, i->elsestm)) == NULL) {
                    __mln_lang_errmsg(ctx, "No memory.");
                    mln_lang_job_free(ctx);
                    return;
                }
                if (mln_stack_push(ctx->run_stack, node) < 0) {
                    __mln_lang_errmsg(ctx, "No memory.");
                    __mln_lang_stack_node_free(node);
                    mln_lang_job_free(ctx);
                    return;
                }
            }
        } else {
            if ((node = mln_lang_stack_node_new(ctx->pool, M_LSNT_BLOCK, i->blockstm)) == NULL) {
                __mln_lang_errmsg(ctx, "No memory.");
                mln_lang_job_free(ctx);
                return;
            }
            if (mln_stack_push(ctx->run_stack, node) < 0) {
                __mln_lang_errmsg(ctx, "No memory.");
                __mln_lang_stack_node_free(node);
                mln_lang_job_free(ctx);
                return;
            }
        }
        mln_lang_ctx_resetRetExp(ctx);
    } else {
        __mln_lang_stack_node_free(mln_stack_pop(ctx->run_stack));
    }
    (void)__mln_lang_run(ctx->lang);
}

static void mln_lang_stack_handler_exp(mln_lang_ctx_t *ctx)
{
    mln_lang_stack_node_t *node = (mln_lang_stack_node_t *)mln_stack_top(ctx->run_stack);
    mln_lang_exp_t *exp = node->data.exp;
    if (node->step == 0) {
        mln_lang_ctx_resetRetExp(ctx);
        node->step = 1;
        if ((node = mln_lang_stack_node_new(ctx->pool, M_LSNT_ASSIGN, exp->assign)) == NULL) {
            __mln_lang_errmsg(ctx, "No memory.");
            mln_lang_job_free(ctx);
            return;
        }
        if (mln_stack_push(ctx->run_stack, node) < 0) {
            __mln_lang_errmsg(ctx, "No memory.");
            __mln_lang_stack_node_free(node);
            mln_lang_job_free(ctx);
            return;
        }
    } else {
        __mln_lang_stack_node_free(mln_stack_pop(ctx->run_stack));
        if (exp->next != NULL) {
            if ((node = mln_lang_stack_node_new(ctx->pool, M_LSNT_EXP, exp->next)) == NULL) {
                __mln_lang_errmsg(ctx, "No memory.");
                mln_lang_job_free(ctx);
                return;
            }
            if (mln_stack_push(ctx->run_stack, node) < 0) {
                __mln_lang_errmsg(ctx, "No memory.");
                __mln_lang_stack_node_free(node);
                mln_lang_job_free(ctx);
                return;
            }
        }
    }
    (void)__mln_lang_run(ctx->lang);
}

static void mln_lang_stack_handler_assign(mln_lang_ctx_t *ctx)
{
    mln_lang_retExp_t *res = NULL;
    mln_lang_stack_node_t *node = (mln_lang_stack_node_t *)mln_stack_top(ctx->run_stack);
    mln_lang_assign_t *assign = node->data.assign;
    if (node->step == 0) {
        mln_lang_ctx_resetRetExp(ctx);
        if (assign->op == M_ASSIGN_NONE) node->step = 3;
        else node->step = 1;
        if ((node = mln_lang_stack_node_new(ctx->pool, M_LSNT_LOGICLOW, assign->left)) == NULL) {
            __mln_lang_errmsg(ctx, "No memory.");
            mln_lang_job_free(ctx);
            return;
        }
        if (mln_stack_push(ctx->run_stack, node) < 0) {
            __mln_lang_errmsg(ctx, "No memory.");
            __mln_lang_stack_node_free(node);
            mln_lang_job_free(ctx);
            return;
        }
    } else if (node->step == 1) {
        node->step = 2;
        ASSERT(assign->right != NULL);
        ASSERT(ctx->retExp->type == M_LANG_RETEXP_VAR);
        mln_lang_stack_node_getRetExpFromCTX(node, ctx);
        if ((node = mln_lang_stack_node_new(ctx->pool, M_LSNT_ASSIGN, assign->right)) == NULL) {
            __mln_lang_errmsg(ctx, "No memory.");
            mln_lang_job_free(ctx);
            return;
        }
        if (mln_stack_push(ctx->run_stack, node) < 0) {
            __mln_lang_errmsg(ctx, "No memory.");
            __mln_lang_stack_node_free(node);
            mln_lang_job_free(ctx);
            return;
        }
    } else if (node->step == 2) {
        node->step = 3;
        mln_lang_op handler = NULL;
        mln_lang_method_t *method;
        ASSERT(ctx->retExp->type == M_LANG_RETEXP_VAR);
        if (assign->op != M_ASSIGN_NONE) {
            ASSERT(node->retExp->type == M_LANG_RETEXP_VAR);
            /*left may be undefined, so must based on right type.*/
            method = mln_lang_methods[__mln_lang_var_getValType(ctx->retExp->data.var)];
            if (method == NULL) {
                __mln_lang_errmsg(ctx, "Operation NOT support.");
                mln_lang_job_free(ctx);
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
            if (mln_lang_val_issetNotModify(node->retExp->data.var->val)) {
                __mln_lang_errmsg(ctx, "Operand cannot be changed.");
                mln_lang_job_free(ctx);
                return;
            }
            if (handler(ctx, &res, node->retExp, ctx->retExp) < 0) {
                mln_lang_job_free(ctx);
                return;
            }
            mln_lang_ctx_setRetExp(ctx, res);
            if (res->type == M_LANG_RETEXP_FUNC) {
                node->call = 1;
again:
                if (mln_lang_stack_handler_funccall_run(ctx, node, res->data.func) < 0) {
                    mln_lang_job_free(ctx);
                    return;
                }
            }
        } else {
            if (assign->op == M_ASSIGN_NONE) {
                mln_lang_ctx_getRetExpFromNode(ctx, node);
            } else {
                __mln_lang_errmsg(ctx, "Operation NOT support.");
                mln_lang_job_free(ctx);
                return;
            }
        }
    } else if (node->step == 3) {
        if (node->call) {
            if (mln_lang_withdrawUntilFunc(ctx) < 0) {
                mln_lang_job_free(ctx);
                return;
            }
            res = ctx->retExp;
            if (ctx->retExp->type == M_LANG_RETEXP_FUNC) goto again;
        }
        node->step = 4;
        if (node->retExp != NULL && node->retExp->type == M_LANG_RETEXP_VAR && node->retExp->data.var->val->func != NULL) {
            if (mln_lang_watch_funcBuild(ctx, \
                                         node, \
                                         node->retExp->data.var->val->func, \
                                         node->retExp->data.var->val->udata, \
                                         node->retExp->data.var->val) < 0)
            {
                mln_lang_job_free(ctx);
                return;
            }
        }
        mln_lang_stack_node_getRetExpFromCTX(node, ctx);
        mln_lang_ctx_resetRetExp(ctx);
    } else {
        if (node->call) {
            if (mln_lang_withdrawUntilFunc(ctx) < 0) {
                mln_lang_job_free(ctx);
                return;
            }
            node->call = 0;
        }
        mln_lang_ctx_getRetExpFromNode(ctx, node);
        __mln_lang_stack_node_free(mln_stack_pop(ctx->run_stack));
    }
    (void)__mln_lang_run(ctx->lang);
}

static void mln_lang_stack_handler_logicLow(mln_lang_ctx_t *ctx)
{
    mln_lang_stack_node_t *node = (mln_lang_stack_node_t *)mln_stack_top(ctx->run_stack);
    mln_lang_logicLow_t *logicLow = node->data.logicLow;
    if (node->step == 0) {
        mln_lang_stack_node_resetRetExp(node);
        mln_lang_ctx_resetRetExp(ctx);
        if (logicLow->op == M_LOGICLOW_NONE) node->step = 2;
        else node->step = 1;
        if ((node = mln_lang_stack_node_new(ctx->pool, M_LSNT_LOGICHIGH, logicLow->left)) == NULL) {
            __mln_lang_errmsg(ctx, "No memory.");
            mln_lang_job_free(ctx);
            return;
        }
        if (mln_stack_push(ctx->run_stack, node) < 0) {
            __mln_lang_errmsg(ctx, "No memory.");
            __mln_lang_stack_node_free(node);
            mln_lang_job_free(ctx);
            return;
        }
    } else if (node->step == 1) {
        node->step = 2;
        ASSERT(logicLow->right != NULL);
        ASSERT(ctx->retExp->type == M_LANG_RETEXP_VAR);
        if (__mln_lang_condition_isTrue(ctx->retExp->data.var)) {
            if (logicLow->op == M_LOGICLOW_AND) {
                if ((node = mln_lang_stack_node_new(ctx->pool, M_LSNT_LOGICLOW, logicLow->right)) == NULL) {
                    __mln_lang_errmsg(ctx, "No memory.");
                    mln_lang_job_free(ctx);
                    return;
                }
                if (mln_stack_push(ctx->run_stack, node) < 0) {
                    __mln_lang_errmsg(ctx, "No memory.");
                    __mln_lang_stack_node_free(node);
                    mln_lang_job_free(ctx);
                    return;
                }
            }
        } else {
            if (logicLow->op == M_LOGICLOW_OR) {
                if ((node = mln_lang_stack_node_new(ctx->pool, M_LSNT_LOGICLOW, logicLow->right)) == NULL) {
                    __mln_lang_errmsg(ctx, "No memory.");
                    mln_lang_job_free(ctx);
                    return;
                }
                if (mln_stack_push(ctx->run_stack, node) < 0) {
                    __mln_lang_errmsg(ctx, "No memory.");
                    __mln_lang_stack_node_free(node);
                    mln_lang_job_free(ctx);
                    return;
                }
            }
        }
    } else {
        __mln_lang_stack_node_free(mln_stack_pop(ctx->run_stack));
    }
    (void)__mln_lang_run(ctx->lang);
}

static void mln_lang_stack_handler_logicHigh(mln_lang_ctx_t *ctx)
{
    mln_lang_stack_node_t *node = (mln_lang_stack_node_t *)mln_stack_top(ctx->run_stack);
    mln_lang_logicHigh_t *logicHigh = (mln_lang_logicHigh_t *)(node->pos);
    mln_lang_retExp_t *res = NULL;
    if (node->step == 0) {
        mln_lang_stack_node_resetRetExp(node);
        mln_lang_ctx_resetRetExp(ctx);
        if (logicHigh->op == M_LOGICHIGH_NONE) node->step = 4;
        else node->step = 1;
        if ((node = mln_lang_stack_node_new(ctx->pool, M_LSNT_RELATIVELOW, logicHigh->left)) == NULL) {
            __mln_lang_errmsg(ctx, "No memory.");
            mln_lang_job_free(ctx);
            return;
        }
        if (mln_stack_push(ctx->run_stack, node) < 0) {
            __mln_lang_errmsg(ctx, "No memory.");
            __mln_lang_stack_node_free(node);
            mln_lang_job_free(ctx);
            return;
        }
    } else if (node->step == 1) {
        node->step = 2;
        ASSERT(logicHigh->right != NULL);
        ASSERT(ctx->retExp->type == M_LANG_RETEXP_VAR);
        ASSERT(node->pos != NULL);
        mln_lang_stack_node_getRetExpFromCTX(node, ctx);
    } else if (node->step == 2) {
        node->step = 3;
        mln_lang_logicHigh_t *ll = logicHigh->right;
        if ((node = mln_lang_stack_node_new(ctx->pool, M_LSNT_RELATIVELOW, ll->left)) == NULL) {
            __mln_lang_errmsg(ctx, "No memory.");
            mln_lang_job_free(ctx);
            return;
        }
        if (mln_stack_push(ctx->run_stack, node) < 0) {
            __mln_lang_errmsg(ctx, "No memory.");
            __mln_lang_stack_node_free(node);
            mln_lang_job_free(ctx);
            return;
        }
    } else if (node->step == 3) {
        node->step = 4;
        mln_lang_op handler = NULL;
        ASSERT(node->retExp->type == M_LANG_RETEXP_VAR);
        ASSERT(ctx->retExp->type == M_LANG_RETEXP_VAR);
        mln_lang_method_t *method;
        method = mln_lang_methods[__mln_lang_var_getValType(node->retExp->data.var)];
        if (method == NULL) {
            __mln_lang_errmsg(ctx, "Operation NOT support.");
            mln_lang_job_free(ctx);
            return;
        }
        switch (logicHigh->op) {
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
            mln_lang_job_free(ctx);
            return;
        }
        if (handler(ctx, &res, node->retExp, ctx->retExp) < 0) {
            mln_lang_job_free(ctx);
            return;
        }
        mln_lang_stack_node_setRetExp(node, res);
        if (res->type == M_LANG_RETEXP_FUNC) {
again:
            node->call = 1;
            if (mln_lang_stack_handler_funccall_run(ctx, node, res->data.func) < 0) {
                mln_lang_job_free(ctx);
                return;
            }
        }
    } else {
        if (node->call) {
            if (mln_lang_withdrawUntilFunc(ctx) < 0) {
                mln_lang_job_free(ctx);
                return;
            }
            node->call = 0;
            mln_lang_stack_node_getRetExpFromCTX(node, ctx);
            res = node->retExp;
            if (node->retExp->type == M_LANG_RETEXP_FUNC) goto again;
        }
        node->pos = logicHigh->right;
        if (node->pos != NULL && logicHigh->right->op != M_LOGICHIGH_NONE) {
            node->step = 2;
        } else {
            if (node->retExp != NULL) mln_lang_ctx_getRetExpFromNode(ctx, node);
            __mln_lang_stack_node_free(mln_stack_pop(ctx->run_stack));
        }
    }
    (void)__mln_lang_run(ctx->lang);
}

static void mln_lang_stack_handler_relativeLow(mln_lang_ctx_t *ctx)
{
    mln_lang_stack_node_t *node = (mln_lang_stack_node_t *)mln_stack_top(ctx->run_stack);
    mln_lang_relativeLow_t *relativeLow = (mln_lang_relativeLow_t *)(node->pos);
    mln_lang_retExp_t *res = NULL;
    if (node->step == 0) {
        mln_lang_stack_node_resetRetExp(node);
        mln_lang_ctx_resetRetExp(ctx);
        if (relativeLow->op == M_RELATIVELOW_NONE) node->step = 4;
        else node->step = 1;
        if ((node = mln_lang_stack_node_new(ctx->pool, M_LSNT_RELATIVEHIGH, relativeLow->left)) == NULL) {
            __mln_lang_errmsg(ctx, "No memory.");
            mln_lang_job_free(ctx);
            return;
        }
        if (mln_stack_push(ctx->run_stack, node) < 0) {
            __mln_lang_errmsg(ctx, "No memory.");
            __mln_lang_stack_node_free(node);
            mln_lang_job_free(ctx);
            return;
        }
    } else if (node->step == 1) {
        node->step = 2;
        ASSERT(relativeLow->right != NULL);
        ASSERT(ctx->retExp->type == M_LANG_RETEXP_VAR);
        ASSERT(node->pos != NULL);
        mln_lang_stack_node_getRetExpFromCTX(node, ctx);
    } else if (node->step == 2) {
        node->step = 3;
        mln_lang_relativeLow_t *tmp = relativeLow->right;
        if ((node = mln_lang_stack_node_new(ctx->pool, M_LSNT_RELATIVEHIGH, tmp->left)) == NULL) {
            __mln_lang_errmsg(ctx, "No memory.");
            mln_lang_job_free(ctx);
            return;
        }
        if (mln_stack_push(ctx->run_stack, node) < 0) {
            __mln_lang_errmsg(ctx, "No memory.");
            __mln_lang_stack_node_free(node);
            mln_lang_job_free(ctx);
            return;
        }
    } else if (node->step == 3) {
        node->step = 4;
        mln_lang_op handler = NULL;
        ASSERT(node->retExp->type == M_LANG_RETEXP_VAR);
        ASSERT(ctx->retExp->type == M_LANG_RETEXP_VAR);
        mln_lang_method_t *method;
        method = mln_lang_methods[__mln_lang_var_getValType(node->retExp->data.var)];
        if (method == NULL) {
            __mln_lang_errmsg(ctx, "Operation NOT support.");
            mln_lang_job_free(ctx);
            return;
        }
        switch (relativeLow->op) {
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
            mln_lang_job_free(ctx);
            return;
        }
        if (handler(ctx, &res, node->retExp, ctx->retExp) < 0) {
            mln_lang_job_free(ctx);
            return;
        }
        mln_lang_stack_node_setRetExp(node, res);
        if (res->type == M_LANG_RETEXP_FUNC) {
again:
            node->call = 1;
            if (mln_lang_stack_handler_funccall_run(ctx, node, res->data.func) < 0) {
                mln_lang_job_free(ctx);
                return;
            }
        }
    } else {
        if (node->call) {
            if (mln_lang_withdrawUntilFunc(ctx) < 0) {
                mln_lang_job_free(ctx);
                return;
            }
            node->call = 0;
            mln_lang_stack_node_getRetExpFromCTX(node, ctx);
            res = node->retExp;
            if (node->retExp->type == M_LANG_RETEXP_FUNC) goto again;
        }
        node->pos = relativeLow->right;
        if (node->pos != NULL && relativeLow->right->op != M_RELATIVELOW_NONE) {
            node->step = 2;
        } else {
            if (node->retExp != NULL) mln_lang_ctx_getRetExpFromNode(ctx, node);
            __mln_lang_stack_node_free(mln_stack_pop(ctx->run_stack));
        }
    }
    (void)__mln_lang_run(ctx->lang);
}

static void mln_lang_stack_handler_relativeHigh(mln_lang_ctx_t *ctx)
{
    mln_lang_stack_node_t *node = (mln_lang_stack_node_t *)mln_stack_top(ctx->run_stack);
    mln_lang_relativeHigh_t *relativeHigh = (mln_lang_relativeHigh_t *)(node->pos);
    mln_lang_retExp_t *res = NULL;
    if (node->step == 0) {
        mln_lang_stack_node_resetRetExp(node);
        mln_lang_ctx_resetRetExp(ctx);
        if (relativeHigh->op == M_RELATIVEHIGH_NONE) node->step = 4;
        else node->step = 1;
        if ((node = mln_lang_stack_node_new(ctx->pool, M_LSNT_MOVE, relativeHigh->left)) == NULL) {
            __mln_lang_errmsg(ctx, "No memory.");
            mln_lang_job_free(ctx);
            return;
        }
        if (mln_stack_push(ctx->run_stack, node) < 0) {
            __mln_lang_errmsg(ctx, "No memory.");
            __mln_lang_stack_node_free(node);
            mln_lang_job_free(ctx);
            return;
        }
    } else if (node->step == 1) {
        node->step = 2;
        ASSERT(relativeHigh->right != NULL);
        ASSERT(ctx->retExp->type == M_LANG_RETEXP_VAR);
        ASSERT(node->pos != NULL);
        mln_lang_stack_node_getRetExpFromCTX(node, ctx);
    } else if (node->step == 2) {
        node->step = 3;
        mln_lang_relativeHigh_t *tmp = relativeHigh->right;
        if ((node = mln_lang_stack_node_new(ctx->pool, M_LSNT_MOVE, tmp->left)) == NULL) {
            __mln_lang_errmsg(ctx, "No memory.");
            mln_lang_job_free(ctx);
            return;
        }
        if (mln_stack_push(ctx->run_stack, node) < 0) {
            __mln_lang_errmsg(ctx, "No memory.");
            __mln_lang_stack_node_free(node);
            mln_lang_job_free(ctx);
            return;
        }
    } else if (node->step == 3) {
        node->step = 4;
        mln_lang_op handler = NULL;
        ASSERT(node->retExp->type == M_LANG_RETEXP_VAR);
        ASSERT(ctx->retExp->type == M_LANG_RETEXP_VAR);
        mln_lang_method_t *method;
        method = mln_lang_methods[__mln_lang_var_getValType(node->retExp->data.var)];
        if (method == NULL) {
            __mln_lang_errmsg(ctx, "Operation NOT support.");
            mln_lang_job_free(ctx);
            return;
        }
        switch (relativeHigh->op) {
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
            mln_lang_job_free(ctx);
            return;
        }
        if (handler(ctx, &res, node->retExp, ctx->retExp) < 0) {
            mln_lang_job_free(ctx);
            return;
        }
        mln_lang_stack_node_setRetExp(node, res);
        if (res->type == M_LANG_RETEXP_FUNC) {
again:
            node->call = 1;
            if (mln_lang_stack_handler_funccall_run(ctx, node, res->data.func) < 0) {
                mln_lang_job_free(ctx);
                return;
            }
        }
    } else {
        if (node->call) {
            if (mln_lang_withdrawUntilFunc(ctx) < 0) {
                mln_lang_job_free(ctx);
                return;
            }
            node->call = 0;
            mln_lang_stack_node_getRetExpFromCTX(node, ctx);
            res = node->retExp;
            if (node->retExp->type == M_LANG_RETEXP_FUNC) goto again;
        }
        node->pos = relativeHigh->right;
        if (node->pos != NULL && relativeHigh->right->op != M_RELATIVEHIGH_NONE) {
            node->step = 2;
        } else {
            if (node->retExp != NULL) mln_lang_ctx_getRetExpFromNode(ctx, node);
            __mln_lang_stack_node_free(mln_stack_pop(ctx->run_stack));
        }
    }
    (void)__mln_lang_run(ctx->lang);
}

static void mln_lang_stack_handler_move(mln_lang_ctx_t *ctx)
{
    mln_lang_stack_node_t *node = (mln_lang_stack_node_t *)mln_stack_top(ctx->run_stack);
    mln_lang_move_t *move = (mln_lang_move_t *)(node->pos);
    mln_lang_retExp_t *res = NULL;
    if (node->step == 0) {
        mln_lang_stack_node_resetRetExp(node);
        mln_lang_ctx_resetRetExp(ctx);
        if (move->op == M_MOVE_NONE) node->step = 4;
        else node->step = 1;
        if ((node = mln_lang_stack_node_new(ctx->pool, M_LSNT_ADDSUB, move->left)) == NULL) {
            __mln_lang_errmsg(ctx, "No memory.");
            mln_lang_job_free(ctx);
            return;
        }
        if (mln_stack_push(ctx->run_stack, node) < 0) {
            __mln_lang_errmsg(ctx, "No memory.");
            __mln_lang_stack_node_free(node);
            mln_lang_job_free(ctx);
            return;
        }
    } else if (node->step == 1) {
        node->step = 2;
        ASSERT(move->right != NULL);
        ASSERT(ctx->retExp->type == M_LANG_RETEXP_VAR);
        ASSERT(node->pos != NULL);
        mln_lang_stack_node_getRetExpFromCTX(node, ctx);
    } else if (node->step == 2) {
        node->step = 3;
        mln_lang_move_t *tmp = move->right;
        if ((node = mln_lang_stack_node_new(ctx->pool, M_LSNT_ADDSUB, tmp->left)) == NULL) {
            __mln_lang_errmsg(ctx, "No memory.");
            mln_lang_job_free(ctx);
            return;
        }
        if (mln_stack_push(ctx->run_stack, node) < 0) {
            __mln_lang_errmsg(ctx, "No memory.");
            __mln_lang_stack_node_free(node);
            mln_lang_job_free(ctx);
            return;
        }
    } else if (node->step == 3) {
        node->step = 4;
        mln_lang_op handler = NULL;
        ASSERT(node->retExp->type == M_LANG_RETEXP_VAR);
        ASSERT(ctx->retExp->type == M_LANG_RETEXP_VAR);
        mln_lang_method_t *method;
        method = mln_lang_methods[__mln_lang_var_getValType(node->retExp->data.var)];
        if (method == NULL) {
            __mln_lang_errmsg(ctx, "Operation NOT support.");
            mln_lang_job_free(ctx);
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
            mln_lang_job_free(ctx);
            return;
        }
        if (handler(ctx, &res, node->retExp, ctx->retExp) < 0) {
            mln_lang_job_free(ctx);
            return;
        }
        mln_lang_stack_node_setRetExp(node, res);
        if (res->type == M_LANG_RETEXP_FUNC) {
again:
            node->call = 1;
            if (mln_lang_stack_handler_funccall_run(ctx, node, res->data.func) < 0) {
                mln_lang_job_free(ctx);
                return;
            }
        }
    } else {
        if (node->call) {
            if (mln_lang_withdrawUntilFunc(ctx) < 0) {
                mln_lang_job_free(ctx);
                return;
            }
            node->call = 0;
            mln_lang_stack_node_getRetExpFromCTX(node, ctx);
            res = node->retExp;
            if (node->retExp->type == M_LANG_RETEXP_FUNC) goto again;
        }
        node->pos = move->right;
        if (node->pos != NULL && move->right->op != M_MOVE_NONE) {
            node->step = 2;
        } else {
            if (node->retExp != NULL) mln_lang_ctx_getRetExpFromNode(ctx, node);
            __mln_lang_stack_node_free(mln_stack_pop(ctx->run_stack));
        }
    }
    (void)__mln_lang_run(ctx->lang);
}

static void mln_lang_stack_handler_addsub(mln_lang_ctx_t *ctx)
{
    mln_lang_stack_node_t *node = (mln_lang_stack_node_t *)mln_stack_top(ctx->run_stack);
    mln_lang_addsub_t *addsub = (mln_lang_addsub_t *)(node->pos);
    mln_lang_retExp_t *res = NULL;
    if (node->step == 0) {
        mln_lang_stack_node_resetRetExp(node);
        mln_lang_ctx_resetRetExp(ctx);
        if (addsub->op == M_ADDSUB_NONE) node->step = 4;
        else node->step = 1;
        if ((node = mln_lang_stack_node_new(ctx->pool, M_LSNT_MULDIV, addsub->left)) == NULL) {
            __mln_lang_errmsg(ctx, "No memory.");
            mln_lang_job_free(ctx);
            return;
        }
        if (mln_stack_push(ctx->run_stack, node) < 0) {
            __mln_lang_errmsg(ctx, "No memory.");
            __mln_lang_stack_node_free(node);
            mln_lang_job_free(ctx);
            return;
        }
    } else if (node->step == 1) {
        node->step = 2;
        ASSERT(addsub->right != NULL);
        ASSERT(ctx->retExp->type == M_LANG_RETEXP_VAR);
        ASSERT(node->pos != NULL);
        mln_lang_stack_node_getRetExpFromCTX(node, ctx);
    } else if (node->step == 2) {
        node->step = 3;
        mln_lang_addsub_t *tmp = addsub->right;
        if ((node = mln_lang_stack_node_new(ctx->pool, M_LSNT_MULDIV, tmp->left)) == NULL) {
            __mln_lang_errmsg(ctx, "No memory.");
            mln_lang_job_free(ctx);
            return;
        }
        if (mln_stack_push(ctx->run_stack, node) < 0) {
            __mln_lang_errmsg(ctx, "No memory.");
            __mln_lang_stack_node_free(node);
            mln_lang_job_free(ctx);
            return;
        }
    } else if (node->step == 3) {
        node->step = 4;
        mln_lang_op handler = NULL;
        ASSERT(node->retExp->type == M_LANG_RETEXP_VAR);
        ASSERT(ctx->retExp->type == M_LANG_RETEXP_VAR);
        mln_lang_method_t *method;
        method = mln_lang_methods[__mln_lang_var_getValType(node->retExp->data.var)];
        if (method == NULL) {
            __mln_lang_errmsg(ctx, "Operation NOT support.");
            mln_lang_job_free(ctx);
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
            mln_lang_job_free(ctx);
            return;
        }
        if (handler(ctx, &res, node->retExp, ctx->retExp) < 0) {
            mln_lang_job_free(ctx);
            return;
        }
        mln_lang_stack_node_setRetExp(node, res);
        if (res->type == M_LANG_RETEXP_FUNC) {
again:
            node->call = 1;
            if (mln_lang_stack_handler_funccall_run(ctx, node, res->data.func) < 0) {
                mln_lang_job_free(ctx);
                return;
            }
        }
    } else {
        if (node->call) {
            if (mln_lang_withdrawUntilFunc(ctx) < 0) {
                mln_lang_job_free(ctx);
                return;
            }
            node->call = 0;
            mln_lang_stack_node_getRetExpFromCTX(node, ctx);
            res = node->retExp;
            if (node->retExp->type == M_LANG_RETEXP_FUNC) goto again;
        }
        node->pos = addsub->right;
        if (node->pos != NULL && addsub->right->op != M_ADDSUB_NONE) {
            node->step = 2;
        } else {
            if (node->retExp != NULL) mln_lang_ctx_getRetExpFromNode(ctx, node);
            __mln_lang_stack_node_free(mln_stack_pop(ctx->run_stack));
        }
    }
    (void)__mln_lang_run(ctx->lang);
}

static void mln_lang_stack_handler_muldiv(mln_lang_ctx_t *ctx)
{
    mln_lang_stack_node_t *node = (mln_lang_stack_node_t *)mln_stack_top(ctx->run_stack);
    mln_lang_muldiv_t *muldiv = (mln_lang_muldiv_t *)(node->pos);
    mln_lang_retExp_t *res = NULL;
    if (node->step == 0) {
        mln_lang_stack_node_resetRetExp(node);
        mln_lang_ctx_resetRetExp(ctx);
        if (muldiv->op == M_MULDIV_NONE) node->step = 4;
        else node->step = 1;
        if ((node = mln_lang_stack_node_new(ctx->pool, M_LSNT_SUFFIX, muldiv->left)) == NULL) {
            __mln_lang_errmsg(ctx, "No memory.");
            mln_lang_job_free(ctx);
            return;
        }
        if (mln_stack_push(ctx->run_stack, node) < 0) {
            __mln_lang_errmsg(ctx, "No memory.");
            __mln_lang_stack_node_free(node);
            mln_lang_job_free(ctx);
            return;
        }
    } else if (node->step == 1) {
        node->step = 2;
        ASSERT(muldiv->right != NULL);
        ASSERT(ctx->retExp->type == M_LANG_RETEXP_VAR);
        ASSERT(node->pos != NULL);
        mln_lang_stack_node_getRetExpFromCTX(node, ctx);
    } else if (node->step == 2) {
        node->step = 3;
        mln_lang_muldiv_t *tmp = muldiv->right;
        if ((node = mln_lang_stack_node_new(ctx->pool, M_LSNT_SUFFIX, tmp->left)) == NULL) {
            __mln_lang_errmsg(ctx, "No memory.");
            mln_lang_job_free(ctx);
            return;
        }
        if (mln_stack_push(ctx->run_stack, node) < 0) {
            __mln_lang_errmsg(ctx, "No memory.");
            __mln_lang_stack_node_free(node);
            mln_lang_job_free(ctx);
            return;
        }
    } else if (node->step == 3) {
        node->step = 4;
        mln_lang_op handler = NULL;
        ASSERT(node->retExp->type == M_LANG_RETEXP_VAR);
        ASSERT(ctx->retExp->type == M_LANG_RETEXP_VAR);
        mln_lang_method_t *method;
        method = mln_lang_methods[__mln_lang_var_getValType(node->retExp->data.var)];
        if (method == NULL) {
            __mln_lang_errmsg(ctx, "Operation NOT support.");
            mln_lang_job_free(ctx);
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
            mln_lang_job_free(ctx);
            return;
        }
        if (handler(ctx, &res, node->retExp, ctx->retExp) < 0) {
            mln_lang_job_free(ctx);
            return;
        }
        mln_lang_stack_node_setRetExp(node, res);
        if (res->type == M_LANG_RETEXP_FUNC) {
again:
            node->call = 1;
            if (mln_lang_stack_handler_funccall_run(ctx, node, res->data.func) < 0) {
                mln_lang_job_free(ctx);
                return;
            }
        }
    } else {
        if (node->call) {
            if (mln_lang_withdrawUntilFunc(ctx) < 0) {
                mln_lang_job_free(ctx);
                return;
            }
            node->call = 0;
            mln_lang_stack_node_getRetExpFromCTX(node, ctx);
            res = node->retExp;
            if (node->retExp->type == M_LANG_RETEXP_FUNC) goto again;
        }
        node->pos = muldiv->right;
        if (node->pos != NULL && muldiv->right->op != M_MULDIV_NONE) {
            node->step = 2;
        } else {
            if (node->retExp != NULL) mln_lang_ctx_getRetExpFromNode(ctx, node);
            __mln_lang_stack_node_free(mln_stack_pop(ctx->run_stack));
        }
    }
    (void)__mln_lang_run(ctx->lang);
}

static void mln_lang_stack_handler_suffix(mln_lang_ctx_t *ctx)
{
    mln_lang_retExp_t *res = NULL;
    mln_lang_stack_node_t *node = (mln_lang_stack_node_t *)mln_stack_top(ctx->run_stack);
    mln_lang_suffix_t *suffix = node->data.suffix;
    if (node->step == 0) {
        mln_lang_stack_node_resetRetExp(node);
        mln_lang_ctx_resetRetExp(ctx);
        node->step = suffix->op == M_SUFFIX_NONE? 2: 1;
        if ((node = mln_lang_stack_node_new(ctx->pool, M_LSNT_LOCATE, suffix->left)) == NULL) {
            __mln_lang_errmsg(ctx, "No memory.");
            mln_lang_job_free(ctx);
            return;
        }
        if (mln_stack_push(ctx->run_stack, node) < 0) {
            __mln_lang_errmsg(ctx, "No memory.");
            __mln_lang_stack_node_free(node);
            mln_lang_job_free(ctx);
            return;
        }
    } else if (node->step == 1) {
        node->step = 2;
        mln_lang_op handler = NULL;
        ASSERT(ctx->retExp != NULL && ctx->retExp->type == M_LANG_RETEXP_VAR);
        mln_lang_method_t *method;
        method = mln_lang_methods[__mln_lang_var_getValType(ctx->retExp->data.var)];
        if (method == NULL) {
            __mln_lang_errmsg(ctx, "Operation NOT support.");
            mln_lang_job_free(ctx);
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
            if (mln_lang_val_issetNotModify(ctx->retExp->data.var->val)) {
                __mln_lang_errmsg(ctx, "Operand cannot be changed.");
                mln_lang_job_free(ctx);
                return;
            }
            if (handler(ctx, &res, ctx->retExp, NULL) < 0) {
                mln_lang_job_free(ctx);
                return;
            }
            mln_lang_stack_node_getRetExpFromCTX(node, ctx);
            mln_lang_ctx_setRetExp(ctx, res);
            if (res->type == M_LANG_RETEXP_FUNC) {
                node->call = 1;
again:
                if (mln_lang_stack_handler_funccall_run(ctx, node, res->data.func) < 0) {
                    mln_lang_job_free(ctx);
                    return;
                }
            }
        } else if (suffix->op != M_SUFFIX_NONE) {
            __mln_lang_errmsg(ctx, "Operation NOT support.");
            mln_lang_job_free(ctx);
            return;
        }
    } else if (node->step == 2) {
        if (node->call) {
            if (mln_lang_withdrawUntilFunc(ctx) < 0) {
                mln_lang_job_free(ctx);
                return;
            }
            res = ctx->retExp;
            if (ctx->retExp->type == M_LANG_RETEXP_FUNC) goto again;
        }
        node->step = 3;
        if (node->retExp != NULL && node->retExp->type == M_LANG_RETEXP_VAR && node->retExp->data.var->val->func != NULL) {
            if (mln_lang_watch_funcBuild(ctx, \
                                         node, \
                                         node->retExp->data.var->val->func, \
                                         node->retExp->data.var->val->udata, \
                                         node->retExp->data.var->val) < 0)
            {
                mln_lang_job_free(ctx);
                return;
            }
        }
        mln_lang_stack_node_getRetExpFromCTX(node, ctx);
        mln_lang_ctx_resetRetExp(ctx);
    } else {
        if (node->call) {
            if (mln_lang_withdrawUntilFunc(ctx) < 0) {
                mln_lang_job_free(ctx);
                return;
            }
            node->call = 0;
        }
        mln_lang_ctx_getRetExpFromNode(ctx, node);
        __mln_lang_stack_node_free(mln_stack_pop(ctx->run_stack));
    }
    (void)__mln_lang_run(ctx->lang);
}

static void mln_lang_stack_handler_locate(mln_lang_ctx_t *ctx)
{
    mln_lang_retExp_t *res = NULL;
    mln_lang_stack_node_t *node = (mln_lang_stack_node_t *)mln_stack_top(ctx->run_stack);
    mln_lang_locate_t *locate = node->data.locate;
    if (node->step == 0) {
        mln_lang_stack_node_resetRetExp(node);
        mln_lang_ctx_resetRetExp(ctx);
        if (locate->op == M_LOCATE_NONE) node->step = 7;
        else if (locate->op == M_LOCATE_PROPERTY) node->step = 4;
        else if (locate->op == M_LOCATE_INDEX) node->step = 1;
        else node->step = 5;
        if ((node = mln_lang_stack_node_new(ctx->pool, M_LSNT_SPEC, locate->left)) == NULL) {
            __mln_lang_errmsg(ctx, "No memory.");
            mln_lang_job_free(ctx);
            return;
        }
        if (mln_stack_push(ctx->run_stack, node) < 0) {
            __mln_lang_errmsg(ctx, "No memory.");
            __mln_lang_stack_node_free(node);
            mln_lang_job_free(ctx);
            return;
        }
    } else if (node->step == 1) {
        node->step = 2;
        ASSERT(locate->op == M_LOCATE_INDEX);
        ASSERT(ctx->retExp != NULL && ctx->retExp->type == M_LANG_RETEXP_VAR);
        mln_lang_stack_node_getRetExpFromCTX(node, ctx);
        if (locate->right.exp == NULL) {
            mln_lang_retExp_t *retExp;
            if ((retExp = __mln_lang_retExp_createTmpNil(ctx->pool, NULL)) == NULL) {
                __mln_lang_errmsg(ctx, "No memory.");
                mln_lang_job_free(ctx);
                return;
            }
            mln_lang_ctx_setRetExp(ctx, retExp);
        } else {
            if ((node = mln_lang_stack_node_new(ctx->pool, M_LSNT_EXP, locate->right.exp)) == NULL) {
                __mln_lang_errmsg(ctx, "No memory.");
                mln_lang_job_free(ctx);
                return;
            }
            if (mln_stack_push(ctx->run_stack, node) < 0) {
                __mln_lang_errmsg(ctx, "No memory.");
                __mln_lang_stack_node_free(node);
                mln_lang_job_free(ctx);
                return;
            }
        }
    } else if (node->step == 2) {
        ASSERT(ctx->retExp != NULL && ctx->retExp->type == M_LANG_RETEXP_VAR);
        mln_lang_method_t *method;
        method = mln_lang_methods[__mln_lang_var_getValType(node->retExp->data.var)];
        if (method == NULL) {
            __mln_lang_errmsg(ctx, "Operation NOT support.");
            mln_lang_job_free(ctx);
            return;
        }
        mln_lang_op handler = method->index_handler;
        if (handler == NULL) {
            __mln_lang_errmsg(ctx, "Operation NOT support.");
            mln_lang_job_free(ctx);
            return;
        }
        if (handler(ctx, &res, node->retExp, ctx->retExp) < 0) {
            mln_lang_job_free(ctx);
            return;
        }
        mln_lang_ctx_setRetExp(ctx, res);
        if (res->type == M_LANG_RETEXP_FUNC) {
again_index:
            node->call = 1;
            node->step = 3;
            if (mln_lang_stack_handler_funccall_run(ctx, node, res->data.func) < 0) {
                mln_lang_job_free(ctx);
                return;
            }
        } else {
            node->step = 3;
        }
    } else if (node->step == 3) {
        if (node->call) {
            if (mln_lang_withdrawUntilFunc(ctx) < 0) {
                mln_lang_job_free(ctx);
                return;
            }
            node->call = 0;
            res = ctx->retExp;
            if (ctx->retExp->type == M_LANG_RETEXP_FUNC) goto again_index;
        }
        node->step = 7;
    } else if (node->step == 4) {
        ASSERT(locate->op == M_LOCATE_PROPERTY);
        ASSERT(ctx->retExp != NULL && ctx->retExp->type == M_LANG_RETEXP_VAR);
        if (node->retExp2 != NULL) {
            mln_lang_retExp_free(node->retExp2);
            node->retExp2 = NULL;
        }
        node->retExp2 = ctx->retExp;
        mln_lang_method_t *method;
        method = mln_lang_methods[__mln_lang_var_getValType(ctx->retExp->data.var)];
        if (method == NULL || method->property_handler == NULL) {
            __mln_lang_errmsg(ctx, "Operation NOT support.");
            node->retExp2 = NULL;
            mln_lang_job_free(ctx);
            return;
        }
        mln_string_t *id;
        mln_lang_retExp_t *res = NULL, *retExp;
        mln_lang_val_t *val;
        mln_lang_var_t *var;
        if ((id = mln_string_pool_dup(ctx->pool, locate->right.id)) == NULL) {
            __mln_lang_errmsg(ctx, "No memory.");
            node->retExp2 = NULL;
            mln_lang_job_free(ctx);
            return;
        }
        if ((val = __mln_lang_val_new(ctx->pool, M_LANG_VAL_TYPE_STRING, id)) == NULL) {
            __mln_lang_errmsg(ctx, "No memory.");
            node->retExp2 = NULL;
            mln_string_pool_free(id);
            mln_lang_job_free(ctx);
            return;
        }
        if ((var = __mln_lang_var_new(ctx->pool, NULL, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
            __mln_lang_errmsg(ctx, "No memory.");
            node->retExp2 = NULL;
            __mln_lang_val_free(val);
            mln_lang_job_free(ctx);
            return;
        }
        if ((retExp = __mln_lang_retExp_new(ctx->pool, M_LANG_RETEXP_VAR, var)) == NULL) {
            __mln_lang_errmsg(ctx, "No memory.");
            node->retExp2 = NULL;
            __mln_lang_var_free(var);
            mln_lang_job_free(ctx);
            return;
        }
        if (method->property_handler(ctx, &res, ctx->retExp, retExp) < 0) {
            __mln_lang_retExp_free(retExp);
            node->retExp2 = NULL;
            mln_lang_job_free(ctx);
            return;
        }
        __mln_lang_retExp_free(retExp);
        ctx->retExp = NULL;
        mln_lang_ctx_setRetExp(ctx, res);
        if (res->type == M_LANG_RETEXP_FUNC) {
again_property:
            res = ctx->retExp;
            node->call = 1;
            node->step = 6;
            if (mln_lang_stack_handler_funccall_run(ctx, node, res->data.func) < 0) {
                mln_lang_job_free(ctx);
                return;
            }
        } else {
            node->step = 6;
        }
    } else if (node->step == 5) {
        mln_lang_retExp_t *retExp;
        mln_lang_funccall_val_t *funccall;
        if (mln_lang_var_getValType(ctx->retExp->data.var) != M_LANG_VAL_TYPE_FUNC) {
            mln_string_t *s = NULL;
            if (mln_lang_var_getValType(ctx->retExp->data.var) == M_LANG_VAL_TYPE_STRING) {
                s = mln_lang_var_getVal(mln_lang_retExp_getVar(ctx->retExp))->data.s;
            } else if (mln_lang_var_getValType(ctx->retExp->data.var) == M_LANG_VAL_TYPE_NIL && ctx->retExp->data.var->name != NULL) {
                s = ctx->retExp->data.var->name;
            } else {
                __mln_lang_errmsg(ctx, "Operation Not support.");
                mln_lang_job_free(ctx);
                return;
            }
            if ((funccall = __mln_lang_funccall_val_new(ctx->pool, s)) == NULL) {
                __mln_lang_errmsg(ctx, "No memory.");
                mln_lang_job_free(ctx);
                return;
            }
            funccall->prototype = NULL;
            if (node->retExp2 != NULL) {
                mln_lang_funccall_val_addObject(funccall, mln_lang_var_getVal(mln_lang_retExp_getVar(node->retExp2)));
                mln_lang_retExp_free(node->retExp2);
                node->retExp2 = NULL;
            }
            if ((retExp = __mln_lang_retExp_new(ctx->pool, M_LANG_RETEXP_FUNC, funccall)) == NULL) {
                __mln_lang_errmsg(ctx, "No memory.");
                __mln_lang_funccall_val_free(funccall);
                mln_lang_job_free(ctx);
                return;
            }
            node->retExp = retExp;
            mln_lang_ctx_resetRetExp(ctx);
        } else {
            if ((funccall = __mln_lang_funccall_val_new(ctx->pool, NULL)) == NULL) {
                __mln_lang_errmsg(ctx, "No memory.");
                mln_lang_job_free(ctx);
                return;
            }
            funccall->prototype = ctx->retExp->data.var->val->data.func;
            if (node->retExp2 != NULL) {
                mln_lang_funccall_val_addObject(funccall, mln_lang_var_getVal(mln_lang_retExp_getVar(node->retExp2)));
                mln_lang_retExp_free(node->retExp2);
                node->retExp2 = NULL;
            }
            if ((retExp = __mln_lang_retExp_new(ctx->pool, M_LANG_RETEXP_FUNC, funccall)) == NULL) {
                __mln_lang_errmsg(ctx, "No memory.");
                __mln_lang_funccall_val_free(funccall);
                mln_lang_job_free(ctx);
                return;
            }
            node->retExp = retExp;
            node->retExp2 = ctx->retExp;
            ctx->retExp = NULL;
        }
        node->pos = locate->right.exp;
        node->step = 8;
    } else if (node->step == 8) {
        if (ctx->retExp != NULL) {
            mln_lang_var_t *var;
            ASSERT(ctx->retExp->type == M_LANG_RETEXP_VAR);
            if ((var = __mln_lang_var_convert(ctx->pool, ctx->retExp->data.var)) == NULL) {
                __mln_lang_errmsg(ctx, "No memory.");
                mln_lang_job_free(ctx);
                return;
            }
            mln_lang_funccall_val_addArg(node->retExp->data.func, var);
            mln_lang_ctx_resetRetExp(ctx);
        }
        ASSERT(ctx->retExp == NULL);
        if (node->pos != NULL) {
            mln_lang_exp_t *exp = (mln_lang_exp_t *)(node->pos);
            node->pos = exp->next;
            if ((node = mln_lang_stack_node_new(ctx->pool, M_LSNT_ASSIGN, exp->assign)) == NULL) {
                __mln_lang_errmsg(ctx, "No memory.");
                mln_lang_job_free(ctx);
                return;
            }
            if (mln_stack_push(ctx->run_stack, node) < 0) {
                __mln_lang_errmsg(ctx, "No memory.");
                __mln_lang_stack_node_free(node);
                mln_lang_job_free(ctx);
                return;
            }
        } else {
            node->call = 1;
            node->step = 6;
            if (mln_lang_stack_handler_funccall_run(ctx, node, node->retExp->data.func) < 0) {
                mln_lang_job_free(ctx);
                return;
            }
            mln_lang_retExp_free(node->retExp2);
            node->retExp2 = NULL;
        }
    } else if (node->step == 6) {
        if (node->call) {
            if (mln_lang_withdrawUntilFunc(ctx) < 0) {
                mln_lang_job_free(ctx);
                return;
            }
            node->call = 0;
            res = ctx->retExp;
            if (ctx->retExp->type == M_LANG_RETEXP_FUNC) goto again_property;
        }
        node->step = 7;
    } else {
        mln_lang_stack_node_t *cur = mln_stack_pop(ctx->run_stack);
        if (locate->next != NULL) {
            if ((node = mln_lang_stack_node_new(ctx->pool, M_LSNT_LOCATE, locate->next)) == NULL) {
                __mln_lang_errmsg(ctx, "No memory.");
                __mln_lang_stack_node_free(cur);
                mln_lang_job_free(ctx);
                return;
            }
            if (mln_stack_push(ctx->run_stack, node) < 0) {
                __mln_lang_errmsg(ctx, "No memory.");
                __mln_lang_stack_node_free(node);
                __mln_lang_stack_node_free(cur);
                mln_lang_job_free(ctx);
                return;
            }
            if (locate->next->op == M_LOCATE_NONE) node->step = 7;
            else if (locate->next->op == M_LOCATE_PROPERTY) node->step = 4;
            else if (locate->next->op == M_LOCATE_INDEX) node->step = 1;
            else {
                node->step = 5;
                if (cur->retExp2 != NULL) {
                    node->retExp2 = cur->retExp2;
                    cur->retExp2 = NULL;
                }
            }
        }
        __mln_lang_stack_node_free(cur);
    }
    (void)__mln_lang_run(ctx->lang);
}

static int mln_lang_stack_handler_funccall_run(mln_lang_ctx_t *ctx, \
                                               mln_lang_stack_node_t *node, \
                                               mln_lang_funccall_val_t *funccall)
{
    mln_lang_scope_t *scope;
    mln_lang_func_detail_t *prototype = funccall->prototype;
    mln_lang_var_t *var = NULL, *scan, *newvar;
    mln_string_t _this = mln_string("this");
    mln_s32_t type;
    mln_string_t *funcname = funccall->name;

    if (prototype == NULL) {
        mln_lang_symbolNode_t *sym;
        if (funccall->object == NULL) {
            while (1) {
                if ((sym = __mln_lang_symbolNode_search(ctx, funcname, 0)) == NULL) {
                    __mln_lang_errmsg(ctx, "Invalid function, No such prototype.");
                    return -1;
                }
                if (sym->type != M_LANG_SYMBOL_VAR) {
                    __mln_lang_errmsg(ctx, "Invalid function, No such prototype.");
                    return -1;
                }
                ASSERT(sym->data.var != NULL);
                if ((type = __mln_lang_var_getValType(sym->data.var)) == M_LANG_VAL_TYPE_FUNC) {
                    prototype = funccall->prototype = sym->data.var->val->data.func;
                    break;
                }
                if (type != M_LANG_VAL_TYPE_STRING) {
                    __mln_lang_errmsg(ctx, "Invalid function, No such prototype.");
                    return -1;
                }
                funcname = sym->data.var->val->data.s;
            }
        } else {/*maybe never get in, not support b.a="foo";b.a() to call b.foo*/
            while (1) {
                if ((var = __mln_lang_set_member_search(funccall->object->data.obj->members, \
                                                        funcname)) == NULL)
                {
                    if ((sym = __mln_lang_symbolNode_search(ctx, funcname, 0)) == NULL) {
                        __mln_lang_errmsg(ctx, "Invalid function, No such prototype.");
                        return -1;
                    }
                    if (sym->type != M_LANG_SYMBOL_VAR) {
                        __mln_lang_errmsg(ctx, "Invalid function, No such prototype.");
                        return -1;
                    }
                    ASSERT(sym->data.var != NULL);
                    if (type != M_LANG_VAL_TYPE_STRING) {
                        __mln_lang_errmsg(ctx, "Invalid function, No such prototype.");
                        return -1;
                    }
                    funcname = sym->data.var->val->data.s;
                    continue;
                }
                if ((type = __mln_lang_var_getValType(var)) == M_LANG_VAL_TYPE_FUNC) {
                    prototype = funccall->prototype = var->val->data.func;
                    break;
                }
                if (type != M_LANG_VAL_TYPE_STRING) {
                    __mln_lang_errmsg(ctx, "Invalid function, No such prototype.");
                    return -1;
                }
                funcname = var->val->data.s;
            }
        }
    }

    if ((scope = mln_lang_scope_new(ctx, \
                                    NULL, \
                                    M_LANG_SCOPE_TYPE_FUNC, \
                                    node)) == NULL)
    {
        __mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    mln_lang_scope_chain_add(&(ctx->scope_head), &(ctx->scope_tail), scope);

    if (funccall->object != NULL) {
        if ((newvar = __mln_lang_var_new(ctx->pool, &_this, M_LANG_VAR_NORMAL, funccall->object, NULL)) == NULL) {
            __mln_lang_errmsg(ctx, "No memory.");
            return -1;
        }
        if (__mln_lang_symbolNode_join(ctx, M_LANG_SYMBOL_VAR, newvar) < 0) {
            __mln_lang_var_free(newvar);
            return -1;
        }
    }
    for (scan = prototype->args_head, var = funccall->args_head; \
         scan != NULL; \
         scan = scan->next)
    {
        if (var == NULL) {
            mln_lang_retExp_t *tmp;
            if ((tmp = __mln_lang_retExp_createTmpNil(ctx->pool, scan->name)) == NULL) {
                __mln_lang_errmsg(ctx, "No memory.");
                return -1;
            }
            newvar = tmp->data.var;
            tmp->data.var = NULL;
            mln_lang_retExp_free(tmp);
        } else {
            if ((newvar = mln_lang_var_transform(ctx, var, scan)) == NULL) {
                __mln_lang_errmsg(ctx, "No memory.");
                return -1;
            }
            var = var->next;
        }
        if (__mln_lang_symbolNode_join(ctx, M_LANG_SYMBOL_VAR, newvar) < 0) {
            __mln_lang_var_free(newvar);
            return -1;
        }
    }
    mln_lang_ctx_resetRetExp(ctx);
    if (prototype->type == M_FUNC_INTERNAL) {
        mln_lang_retExp_t *retExp = NULL;
        if (prototype->data.process == NULL) {
            __mln_lang_errmsg(ctx, "Not implemented.");
            return -1;
        }
        if ((retExp = prototype->data.process(ctx)) == NULL) {
            return -1;
        }
        mln_lang_ctx_setRetExp(ctx, retExp);
    } else {
        if (prototype->data.stm != NULL) {
            if ((node = mln_lang_stack_node_new(ctx->pool, M_LSNT_STM, prototype->data.stm)) == NULL) {
                __mln_lang_errmsg(ctx, "No memory.");
                return -1;
            }
            if (mln_stack_push(ctx->run_stack, node) < 0) {
                __mln_lang_errmsg(ctx, "No memory.");
                __mln_lang_stack_node_free(node);
                return -1;
            }
        }
    }
    return 0;
}

static void mln_lang_stack_handler_spec(mln_lang_ctx_t *ctx)
{
    mln_lang_stack_node_t *node = (mln_lang_stack_node_t *)mln_stack_top(ctx->run_stack);
    mln_lang_spec_t *spec = node->data.spec;
    mln_lang_retExp_t *retExp;
    if (spec->op == M_SPEC_REFER) {
        __mln_lang_errmsg(ctx, "'&' Not allowed.");
        mln_lang_job_free(ctx);
        return;
    }
    if (node->step == 0) {
        node->step = 1;
        mln_u8ptr_t data = NULL;
        mln_lang_stack_node_type_t type;
        mln_lang_stack_node_resetRetExp(node);
        mln_lang_ctx_resetRetExp(ctx);
        switch (spec->op) {
            case M_SPEC_NEGATIVE:
            case M_SPEC_REVERSE:
            case M_SPEC_NOT:
            case M_SPEC_REFER:
            case M_SPEC_INC:
            case M_SPEC_DEC:
                data = (mln_u8ptr_t)(spec->data.spec);
                type = M_LSNT_SPEC;
                break;
            case M_SPEC_NEW:
                if (mln_lang_stack_handler_spec_new(ctx, spec->data.setName) < 0) {
                    mln_lang_job_free(ctx);
                    return;
                }
                node->step = 2;
                break;
            case M_SPEC_PARENTH:
                if (spec->data.exp == NULL) {
                    if ((retExp = __mln_lang_retExp_createTmpNil(ctx->pool, NULL)) == NULL) {
                        __mln_lang_errmsg(ctx, "No memory.");
                        mln_lang_job_free(ctx);
                        return;
                    }
                } else {
                    data = (mln_u8ptr_t)(spec->data.exp);
                    type = M_LSNT_EXP;
                }
                node->step = 2;
                break;
            case M_SPEC_FACTOR:
                data = (mln_u8ptr_t)(spec->data.factor);
                type = M_LSNT_FACTOR;
                node->step = 2;
                break;
            default: /*M_SPEC_FUNC:*/
                data = (mln_u8ptr_t)(spec->data.func);
                type = M_LSNT_FUNCCALL;
                break;
        }
        if (data != NULL) {
            if ((node = mln_lang_stack_node_new(ctx->pool, type, data)) == NULL) {
                __mln_lang_errmsg(ctx, "No memory.");
                mln_lang_job_free(ctx);
                return;
            }
            if (mln_stack_push(ctx->run_stack, node) < 0) {
                __mln_lang_errmsg(ctx, "No memory.");
                __mln_lang_stack_node_free(node);
                mln_lang_job_free(ctx);
                return;
            }
        } else {
            ASSERT(spec->op == M_SPEC_NEW || spec->op == M_SPEC_PARENTH);
        }
    } else if (node->step == 1) {
        node->step = 2;
        retExp = ctx->retExp;
        ASSERT(retExp != NULL);
        if (retExp->type == M_LANG_RETEXP_FUNC) {
again:
            node->call = 1;
            if (mln_lang_stack_handler_funccall_run(ctx, node, retExp->data.func) < 0) {
                mln_lang_job_free(ctx);
                return;
            }
        } else {
            mln_lang_op handler = NULL;
            mln_lang_method_t *method;
            method = mln_lang_methods[__mln_lang_var_getValType(retExp->data.var)];
            if (method == NULL) {
                __mln_lang_errmsg(ctx, "Operation NOT support.");
                mln_lang_job_free(ctx);
                return;
            }
            switch (spec->op) {
                case M_SPEC_NEGATIVE:
                    handler = method->negative_handler;
                    break;
                case M_SPEC_REVERSE:
                    handler = method->reverse_handler;
                    break;
                case M_SPEC_NOT:
                    handler = method->not_handler;
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
            if (handler != NULL) {
                retExp = NULL;
                if ((spec->op == M_SPEC_INC || spec->op == M_SPEC_DEC) && mln_lang_val_issetNotModify(ctx->retExp->data.var->val)) {
                    __mln_lang_errmsg(ctx, "Operand cannot be changed.");
                    mln_lang_job_free(ctx);
                    return;
                }
                if (handler(ctx, &retExp, ctx->retExp, NULL) < 0) {
                    mln_lang_job_free(ctx);
                    return;
                }
                mln_lang_stack_node_getRetExpFromCTX(node, ctx);
                mln_lang_ctx_setRetExp(ctx, retExp);
                if (retExp->type == M_LANG_RETEXP_FUNC) {
                    goto again;
                }
            } else {
                __mln_lang_errmsg(ctx, "Operation NOT support.");
                mln_lang_job_free(ctx);
                return;
            }
        }
    } else if (node->step == 2) {
        if (node->call) {
            if (mln_lang_withdrawUntilFunc(ctx) < 0) {
                mln_lang_job_free(ctx);
                return;
            }
            node->call = 0;
            retExp = ctx->retExp;
            if (ctx->retExp->type == M_LANG_RETEXP_FUNC) goto again;
        }
        node->step = 3;
        if ((spec->op == M_SPEC_INC || spec->op == M_SPEC_DEC) && \
            node->retExp != NULL && \
            node->retExp->type == M_LANG_RETEXP_VAR && \
            node->retExp->data.var->val->func != NULL)
        {
            if (mln_lang_watch_funcBuild(ctx, \
                                         node, \
                                         node->retExp->data.var->val->func, \
                                         node->retExp->data.var->val->udata, \
                                         node->retExp->data.var->val) < 0)
            {
                mln_lang_job_free(ctx);
                return;
            }
        }
        mln_lang_stack_node_getRetExpFromCTX(node, ctx);
        mln_lang_ctx_resetRetExp(ctx);
    } else {
        if (node->call) {
            if (mln_lang_withdrawUntilFunc(ctx) < 0) {
                mln_lang_job_free(ctx);
                return;
            }
            node->call = 0;
        }
        mln_lang_ctx_getRetExpFromNode(ctx, node);
        __mln_lang_stack_node_free(mln_stack_pop(ctx->run_stack));
    }
    (void)__mln_lang_run(ctx->lang);
}

static inline int mln_lang_stack_handler_spec_new(mln_lang_ctx_t *ctx, mln_string_t *name)
{
    mln_lang_var_t *var;
    mln_lang_val_t *val;
    mln_lang_retExp_t *retExp;
    mln_lang_object_t *obj;
    mln_lang_symbolNode_t *sym;
    char tmp[512] = {0};
    char msg[1024] = {0};
    mln_size_t len;

    while (1) {
        sym = __mln_lang_symbolNode_search(ctx, name, 0);
        if (sym == NULL) {
err:
            len = name->len>(sizeof(tmp)-1)? name->len: sizeof(tmp)-1;
            memcpy(tmp, name->data, len);
            snprintf(msg, sizeof(msg)-1, "Invalid Set name '%s'.", tmp);
            __mln_lang_errmsg(ctx, msg);
            return -1;
        } else {
            if (sym->type == M_LANG_SYMBOL_SET) break;
            if (sym->type != M_LANG_SYMBOL_VAR) goto err;
            if (mln_lang_var_getValType(sym->data.var) != M_LANG_VAL_TYPE_STRING) goto err;
            if ((name = sym->data.var->val->data.s) == NULL) goto err;
        }
    }

    if ((obj = mln_lang_object_new(ctx, sym->data.set)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if ((val = __mln_lang_val_new(ctx->pool, M_LANG_VAL_TYPE_OBJECT, obj)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        mln_lang_object_free(obj);
        return -1;
    }
    if ((var = __mln_lang_var_new(ctx->pool, NULL, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        __mln_lang_val_free(val);
        return -1;
    }
    if ((retExp = __mln_lang_retExp_new(ctx->pool, M_LANG_RETEXP_VAR, var)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        __mln_lang_var_free(var);
        return -1;
    }
    mln_lang_ctx_setRetExp(ctx, retExp);
    return 0;
}

static void mln_lang_stack_handler_factor(mln_lang_ctx_t *ctx)
{
    mln_lang_stack_node_t *node = (mln_lang_stack_node_t *)mln_stack_top(ctx->run_stack);
    mln_lang_factor_t *factor = node->data.factor;
    if (node->step == 0) {
        node->step = 1;
        switch (factor->type) {
            case M_FACTOR_BOOL:
                if (mln_lang_stack_handler_factor_bool(ctx, factor) < 0) {
                    mln_lang_job_free(ctx);
                    return;
                }
                break;
            case M_FACTOR_STRING:
                if (mln_lang_stack_handler_factor_string(ctx, factor) < 0) {
                    mln_lang_job_free(ctx);
                    return;
                }
                break;
            case M_FACTOR_INT:
                if (mln_lang_stack_handler_factor_int(ctx, factor) < 0) {
                    mln_lang_job_free(ctx);
                    return;
                }
                break;
            case M_FACTOR_REAL:
                if (mln_lang_stack_handler_factor_real(ctx, factor) < 0) {
                    mln_lang_job_free(ctx);
                    return;
                }
                break;
            case M_FACTOR_ARRAY:
                if (mln_lang_stack_handler_factor_array(ctx, factor) < 0) {
                    mln_lang_job_free(ctx);
                    return;
                }
                break;
            case M_FACTOR_ID:
                if (mln_lang_stack_handler_factor_id(ctx, factor) < 0) {
                    mln_lang_job_free(ctx);
                    return;
                }
                break;
            default:
                if (mln_lang_stack_handler_factor_nil(ctx, factor) < 0) {
                    mln_lang_job_free(ctx);
                    return;
                }
                break;
        }
    } else {
        if (factor->type == M_FACTOR_ARRAY) {
            mln_lang_ctx_getRetExpFromNode(ctx, node);
        }
        ASSERT(ctx->retExp != NULL && ctx->retExp->type == M_LANG_RETEXP_VAR);
        __mln_lang_stack_node_free(mln_stack_pop(ctx->run_stack));
    }
    (void)__mln_lang_run(ctx->lang);
}

static inline int
mln_lang_stack_handler_factor_bool(mln_lang_ctx_t *ctx, mln_lang_factor_t *factor)
{
    mln_lang_var_t *var;
    mln_lang_val_t *val;
    mln_lang_retExp_t *retExp;

    if ((val = __mln_lang_val_new(ctx->pool, M_LANG_VAL_TYPE_BOOL, &(factor->data.b))) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if ((var = __mln_lang_var_new(ctx->pool, NULL, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        __mln_lang_val_free(val);
        return -1;
    }
    if ((retExp = __mln_lang_retExp_new(ctx->pool, M_LANG_RETEXP_VAR, var)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        __mln_lang_var_free(var);
        return -1;
    }
    mln_lang_ctx_setRetExp(ctx, retExp);
    return 0;
}

static inline int
mln_lang_stack_handler_factor_string(mln_lang_ctx_t *ctx, mln_lang_factor_t *factor)
{
    mln_lang_var_t *var;
    mln_lang_val_t *val;
    mln_lang_retExp_t *retExp;
    mln_string_t *s;

    if ((s = mln_string_pool_dup(ctx->pool, factor->data.s_id)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if ((val = __mln_lang_val_new(ctx->pool, M_LANG_VAL_TYPE_STRING, s)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        mln_string_pool_free(s);
        return -1;
    }
    if ((var = __mln_lang_var_new(ctx->pool, NULL, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        __mln_lang_val_free(val);
        return -1;
    }
    if ((retExp = __mln_lang_retExp_new(ctx->pool, M_LANG_RETEXP_VAR, var)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        __mln_lang_var_free(var);
        return -1;
    }
    mln_lang_ctx_setRetExp(ctx, retExp);
    return 0;
}

static inline int
mln_lang_stack_handler_factor_int(mln_lang_ctx_t *ctx, mln_lang_factor_t *factor)
{
    mln_lang_var_t *var;
    mln_lang_val_t *val;
    mln_lang_retExp_t *retExp;

    if ((val = __mln_lang_val_new(ctx->pool, M_LANG_VAL_TYPE_INT, &(factor->data.i))) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if ((var = __mln_lang_var_new(ctx->pool, NULL, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        __mln_lang_val_free(val);
        return -1;
    }
    if ((retExp = __mln_lang_retExp_new(ctx->pool, M_LANG_RETEXP_VAR, var)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        __mln_lang_var_free(var);
        return -1;
    }
    mln_lang_ctx_setRetExp(ctx, retExp);
    return 0;
}

static inline int
mln_lang_stack_handler_factor_real(mln_lang_ctx_t *ctx, mln_lang_factor_t *factor)
{
    mln_lang_var_t *var;
    mln_lang_val_t *val;
    mln_lang_retExp_t *retExp;

    if ((val = __mln_lang_val_new(ctx->pool, M_LANG_VAL_TYPE_REAL, &(factor->data.f))) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if ((var = __mln_lang_var_new(ctx->pool, NULL, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        __mln_lang_val_free(val);
        return -1;
    }
    if ((retExp = __mln_lang_retExp_new(ctx->pool, M_LANG_RETEXP_VAR, var)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        __mln_lang_var_free(var);
        return -1;
    }
    mln_lang_ctx_setRetExp(ctx, retExp);
    return 0;
}

static inline int
mln_lang_stack_handler_factor_array(mln_lang_ctx_t *ctx, mln_lang_factor_t *factor)
{
    mln_lang_stack_node_t *node = (mln_lang_stack_node_t *)mln_stack_top(ctx->run_stack);
    mln_lang_var_t *var;
    mln_lang_val_t *val;
    mln_lang_retExp_t *retExp;
    mln_lang_array_t *array;

    if ((array = __mln_lang_array_new(ctx)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if ((val = __mln_lang_val_new(ctx->pool, M_LANG_VAL_TYPE_ARRAY, array)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        __mln_lang_array_free(array);
        return -1;
    }
    if ((var = __mln_lang_var_new(ctx->pool, NULL, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        __mln_lang_val_free(val);
        return -1;
    }
    if ((retExp = __mln_lang_retExp_new(ctx->pool, M_LANG_RETEXP_VAR, var)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        __mln_lang_var_free(var);
        return -1;
    }
    mln_lang_stack_node_setRetExp(node, retExp);
    if (factor->data.array != NULL) {
        if ((node = mln_lang_stack_node_new(ctx->pool, M_LSNT_ELEMLIST, factor->data.array)) == NULL) {
            __mln_lang_errmsg(ctx, "No memory.");
            return -1;
        }
        if (mln_stack_push(ctx->run_stack, node) < 0) {
            __mln_lang_errmsg(ctx, "No memory.");
            __mln_lang_stack_node_free(node);
            return -1;
        }
    }
    return 0;
}

static inline int
mln_lang_stack_handler_factor_id(mln_lang_ctx_t *ctx, mln_lang_factor_t *factor)
{
    mln_lang_val_t *val;
    mln_lang_var_t *var;
    mln_lang_symbolNode_t *sym;
    mln_lang_retExp_t *retExp;

    if ((sym = mln_lang_symbolNode_idSearch(ctx, factor->data.s_id)) != NULL) {
        if (sym->type == M_LANG_SYMBOL_VAR) {
            if ((var = __mln_lang_var_convert(ctx->pool, sym->data.var)) == NULL) {
                __mln_lang_errmsg(ctx, "No memory.");
                return -1;
            }
            if ((retExp = __mln_lang_retExp_new(ctx->pool, M_LANG_RETEXP_VAR, var)) == NULL) {
                __mln_lang_errmsg(ctx, "No memory.");
                __mln_lang_var_free(var);
                return -1;
            }
        } else if (sym->type == M_LANG_SYMBOL_SET) {
            __mln_lang_errmsg(ctx, "Invalid token. Token is a SET name, not a value or function.");
            return -1;
        } else {/*M_LANG_SYMBOL_LABEL*/
            __mln_lang_errmsg(ctx, "Invalid token. Token is a Label, not a value or function.");
            return -1;
        }
        mln_lang_ctx_setRetExp(ctx, retExp);
        return 0;
    }

    if ((val = __mln_lang_val_new(ctx->pool, M_LANG_VAL_TYPE_NIL, NULL)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if ((var = __mln_lang_var_new(ctx->pool, factor->data.s_id, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        __mln_lang_val_free(val);
        return -1;
    }
    if (__mln_lang_symbolNode_join(ctx, M_LANG_SYMBOL_VAR, var) < 0) {
        __mln_lang_errmsg(ctx, "No memory.");
        __mln_lang_var_free(var);
        return -1;
    }
    if ((var = __mln_lang_var_convert(ctx->pool, var)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if ((retExp = __mln_lang_retExp_new(ctx->pool, M_LANG_RETEXP_VAR, var)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    mln_lang_ctx_setRetExp(ctx, retExp);
    return 0;
}

static inline int
mln_lang_stack_handler_factor_nil(mln_lang_ctx_t *ctx, mln_lang_factor_t *factor)
{
    mln_lang_retExp_t *retExp;
    if ((retExp = __mln_lang_retExp_createTmpNil(ctx->pool, NULL)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    mln_lang_ctx_setRetExp(ctx, retExp);
    return 0;
}

static void mln_lang_stack_handler_elemlist(mln_lang_ctx_t *ctx)
{
    mln_lang_stack_node_t *node = (mln_lang_stack_node_t *)mln_stack_top(ctx->run_stack);
    mln_lang_elemlist_t *elem = node->data.elemlist;
    if (node->step == 0) {
        node->step = 1;
        mln_lang_stack_node_resetRetExp(node);
        mln_lang_ctx_resetRetExp(ctx);
        ASSERT(node->retExp == NULL);
        if (elem->key != NULL) {
            if ((node = mln_lang_stack_node_new(ctx->pool, M_LSNT_ASSIGN, elem->key)) == NULL) {
                __mln_lang_errmsg(ctx, "No memory.");
                mln_lang_job_free(ctx);
                return;
            }
            if (mln_stack_push(ctx->run_stack, node) < 0) {
                __mln_lang_errmsg(ctx, "No memory.");
                __mln_lang_stack_node_free(node);
                mln_lang_job_free(ctx);
                return;
            }
        }
    } else if (node->step == 1) {
        node->step = 2;
        if (ctx->retExp != NULL) {
            ASSERT(ctx->retExp->type == M_LANG_RETEXP_VAR);
            mln_lang_stack_node_getRetExpFromCTX(node, ctx);
        }
        if ((node = mln_lang_stack_node_new(ctx->pool, M_LSNT_ASSIGN, elem->val)) == NULL) {
            __mln_lang_errmsg(ctx, "No memory.");
            mln_lang_job_free(ctx);
            return;
        }
        if (mln_stack_push(ctx->run_stack, node) < 0) {
            __mln_lang_errmsg(ctx, "No memory.");
            __mln_lang_stack_node_free(node);
            mln_lang_job_free(ctx);
            return;
        }
    } else if (node->step == 2) {
        mln_lang_var_t *key, *val;
        mln_lang_stack_node_t *array_node;
        mln_lang_array_t *array;
        node->step = 3;
        node = (mln_lang_stack_node_t *)mln_stack_pop(ctx->run_stack);
        array_node = (mln_lang_stack_node_t *)mln_stack_top(ctx->run_stack);
        ASSERT(array_node != NULL);
        if (mln_stack_push(ctx->run_stack, node) < 0) {
            __mln_lang_errmsg(ctx, "No memory.");
            __mln_lang_stack_node_free(node);
            mln_lang_job_free(ctx);
            return;
        }

        ASSERT(ctx->retExp != NULL && ctx->retExp->type == M_LANG_RETEXP_VAR);
        ASSERT(array_node->retExp->data.var->val != NULL);
        ASSERT(array_node->retExp->data.var->val->type == M_LANG_VAL_TYPE_ARRAY);
        array = array_node->retExp->data.var->val->data.array;
        ASSERT(array != NULL);
        key = node->retExp == NULL? NULL: node->retExp->data.var;
        if ((val = __mln_lang_array_getAndNew(ctx, array, key)) == NULL) {
            mln_lang_job_free(ctx);
            return;
        }
        if (__mln_lang_var_setValue(ctx, val, ctx->retExp->data.var) < 0) {
            __mln_lang_errmsg(ctx, "No memory.");
            mln_lang_job_free(ctx);
            return;
        }
    } else {
        __mln_lang_stack_node_free(mln_stack_pop(ctx->run_stack));
        mln_lang_ctx_resetRetExp(ctx);
        if (elem->next != NULL) {
            if ((node = mln_lang_stack_node_new(ctx->pool, M_LSNT_ELEMLIST, elem->next)) == NULL) {
                __mln_lang_errmsg(ctx, "No memory.");
                mln_lang_job_free(ctx);
                return;
            }
            if (mln_stack_push(ctx->run_stack, node) < 0) {
                __mln_lang_errmsg(ctx, "No memory.");
                __mln_lang_stack_node_free(node);
                mln_lang_job_free(ctx);
                return;
            }
        }
    }
    (void)__mln_lang_run(ctx->lang);
}

static void mln_lang_stack_handler_funccall(mln_lang_ctx_t *ctx)
{
    mln_lang_stack_node_t *node = (mln_lang_stack_node_t *)mln_stack_top(ctx->run_stack);
    mln_lang_funccall_t *lf = node->data.funccall;
    if (node->step == 0) {
        mln_lang_funccall_val_t *funccall;
        mln_lang_retExp_t *retExp;
        node->step = 1;
        if ((funccall = __mln_lang_funccall_val_new(ctx->pool, lf->name)) == NULL) {
            __mln_lang_errmsg(ctx, "No memory.");
            mln_lang_job_free(ctx);
            return;
        }
        if ((retExp = __mln_lang_retExp_new(ctx->pool, M_LANG_RETEXP_FUNC, funccall)) == NULL) {
            __mln_lang_errmsg(ctx, "No memory.");
            __mln_lang_funccall_val_free(funccall);
            mln_lang_job_free(ctx);
            return;
        }
        mln_lang_stack_node_setRetExp(node, retExp);
        mln_lang_ctx_resetRetExp(ctx);
    } else if (node->step == 1) {
        if (ctx->retExp != NULL) {
            mln_lang_var_t *var;
            ASSERT(node->retExp != NULL && node->retExp->type == M_LANG_RETEXP_FUNC);
            ASSERT(ctx->retExp->type == M_LANG_RETEXP_VAR);
            if ((var = __mln_lang_var_convert(ctx->pool, ctx->retExp->data.var)) == NULL) {
                __mln_lang_errmsg(ctx, "No memory.");
                mln_lang_job_free(ctx);
                return;
            }
            mln_lang_funccall_val_addArg(node->retExp->data.func, var);
            mln_lang_ctx_resetRetExp(ctx);
        }
        ASSERT(ctx->retExp == NULL);
        if (node->pos != NULL) {
            mln_lang_exp_t *exp = (mln_lang_exp_t *)(node->pos);
            node->pos = exp->next;
            if ((node = mln_lang_stack_node_new(ctx->pool, M_LSNT_ASSIGN, exp->assign)) == NULL) {
                __mln_lang_errmsg(ctx, "No memory.");
                mln_lang_job_free(ctx);
                return;
            }
            if (mln_stack_push(ctx->run_stack, node) < 0) {
                __mln_lang_errmsg(ctx, "No memory.");
                __mln_lang_stack_node_free(node);
                mln_lang_job_free(ctx);
                return;
            }
        } else {
            node->step = 2;
        }
    } else {
        mln_lang_ctx_getRetExpFromNode(ctx, node);
        __mln_lang_stack_node_free(mln_stack_pop(ctx->run_stack));
    }
    (void)__mln_lang_run(ctx->lang);
}

/*
 * debug
 */
void mln_lang_dump(mln_lang_ctx_t *ctx)
{
    mln_log(none, "Lang Debug!\n");
    mln_lang_scope_t *scope;
    char *title = NULL;
    for (scope = ctx->scope_tail; scope != NULL; scope = scope->prev) {
        if (scope->type == M_LANG_SCOPE_TYPE_SET) title = "Set scope";
        else title = "Function scope";
        mln_log(none, "%s\n", title);
        mln_rbtree_scan_all(scope->symbols, mln_lang_dump_symbol, NULL);
    }
}

static int mln_lang_dump_symbol(mln_rbtree_node_t *node, void *rn_data, void *udata)
{
    mln_lang_symbolNode_t *sym = (mln_lang_symbolNode_t *)rn_data;
    char *title = NULL;
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
            mln_lang_dump_var(sym->data.var, 4);
            break;
        case M_LANG_SYMBOL_SET:
            mln_lang_dump_set(sym->data.set);
            break;
        default:
            mln_log(none, "    \"%S\" pos: %X\n", \
                    sym->data.label->label, (mln_uptr_t)(sym->data.label->stm));
            break;
    }
    return 0;
}

#define blank(); {int tmpi; for (tmpi = 0; tmpi < cnt; tmpi++) mln_log(none, " ");}
static void mln_lang_dump_var(mln_lang_var_t *var, int cnt)
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
    if (var->inSet != NULL) {
        mln_log(none, "  In Set '%S'  setRef: %l", var->inSet->name, var->inSet->ref);
    }
    mln_log(none, "  valueRef: %u, udata <0x%X>, func <0x%X>, notModify: %s\n", \
            var->val->ref, \
            var->val->udata, \
            var->val->func, \
            var->val->notModify? "true": "false");
    mln_s32_t type = __mln_lang_var_getValType(var);
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
            mln_log(none, "<BOOL> %s\n", var->val->data.b?"True":"False");
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
            mln_lang_dump_object(var->val->data.obj, cnt);
            break;
        case M_LANG_VAL_TYPE_FUNC:
            mln_lang_dump_function(var->val->data.func, cnt);
            break;
        default: /*M_LANG_VAL_TYPE_ARRAY:*/
            mln_lang_dump_array(var->val->data.array, cnt);
            break;
    }
}

static void mln_lang_dump_object(mln_lang_object_t *obj, int cnt)
{
    blank();
    mln_log(none, "<OBJECT> In Set '%S'  setRef: %I objRef: %I\n", \
            obj->inSet->name, obj->inSet->ref, obj->ref);
    mln_rbtree_scan_all(obj->members, mln_lang_dump_var_scan, &cnt);
}

static int mln_lang_dump_var_scan(mln_rbtree_node_t *node, void *rn_data, void *udata)
{
    mln_lang_dump_var((mln_lang_var_t *)rn_data, *(int *)udata+2);
    return 0;
}

static void mln_lang_dump_function(mln_lang_func_detail_t *func, int cnt)
{
    mln_lang_var_t *var;
    blank();
    mln_log(none, "<FUNCTION> NARGS:%I\n", (mln_u64_t)func->nargs);
    for (var = func->args_head; var != NULL; var = var->next) {
        mln_lang_dump_var(var, cnt+2);
    }
}

static void mln_lang_dump_set(mln_lang_set_detail_t *set)
{
    int cnt = 4;
    mln_log(none, "    Name: %S  ref:%I\n", set->name, set->ref);
    mln_rbtree_scan_all(set->members, mln_lang_dump_var_scan, &cnt);
}

static void mln_lang_dump_array(mln_lang_array_t *array, int cnt)
{
    int tmp = cnt + 2;
    blank();
    mln_log(none, "<ARRAY>\n");
    blank();
    mln_log(none, "ALL ELEMENTS:\n");
    mln_rbtree_scan_all(array->elems_index, mln_lang_dump_array_elem, &tmp);
    blank();
    mln_log(none, "KEY ELEMENTS:\n");
    mln_rbtree_scan_all(array->elems_key, mln_lang_dump_array_elem, &tmp);
    blank();
    mln_log(none, "Refs: %I\n", array->ref);
}

static int mln_lang_dump_array_elem(mln_rbtree_node_t *node, void *rn_data, void *udata)
{
    mln_lang_array_elem_t *elem = (mln_lang_array_elem_t *)rn_data;
    int cnt = *(int *)udata;
    blank();
    mln_log(none, "Index: %I\n", elem->index);
    if (elem->key != NULL) {
        blank();
        mln_log(none, "Key:\n");
        mln_lang_dump_var(elem->key, cnt+2);
    }
    blank();
    mln_log(none, "Value:\n");
    mln_lang_dump_var(elem->value, cnt+2);
    return 0;
}

/*
 * msg
 */
static int mln_lang_msg_cmp(const void *data1, const void *data2)
{
    mln_lang_msg_t *lm1 = (mln_lang_msg_t *)data1;
    mln_lang_msg_t *lm2 = (mln_lang_msg_t *)data2;
    return mln_string_strcmp(lm1->name, lm2->name);
}

static mln_lang_msg_t *__mln_lang_msg_new(mln_lang_ctx_t *ctx, mln_string_t *name)
{
    int fds[2];
    mln_lang_msg_t *lm = (mln_lang_msg_t *)mln_alloc_m(ctx->pool, sizeof(mln_lang_msg_t));
    if (lm == NULL) return NULL;
    lm->ctx = ctx;
    if ((lm->name = mln_string_pool_dup(ctx->pool, name)) == NULL) {
        mln_alloc_free(lm);
        return NULL;
    }
    lm->script_val = lm->c_val = NULL;
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, fds) < 0) {
        mln_string_pool_free(lm->name);
        mln_alloc_free(lm);
        return NULL;
    }
    lm->script_fd = fds[0];
    lm->c_fd = fds[1];
    lm->c_handler = NULL;
    lm->script_read = lm->c_read = 1;
    lm->script_wait = 0;
    return lm;
}

int mln_lang_msg_new(mln_lang_ctx_t *ctx, mln_string_t *name)
{
    mln_lang_msg_t *lm, tmp;
    mln_rbtree_node_t *rn;
    tmp.name = name;
    rn = mln_rbtree_search(ctx->msg_map, ctx->msg_map->root, &tmp);
    if (!mln_rbtree_null(rn, ctx->msg_map)) return -1;
    if ((lm = __mln_lang_msg_new(ctx, name)) == NULL) return -1;
    if ((rn = mln_rbtree_node_new(ctx->msg_map, lm)) == NULL) {
        __mln_lang_msg_free(lm);
        return -1;
    }
    mln_rbtree_insert(ctx->msg_map, rn);
    return 0;
}

static void __mln_lang_msg_free(void *data)
{
    if (data == NULL) return;
    mln_lang_msg_t *lm = (mln_lang_msg_t *)data;
    if (lm->name != NULL) mln_string_pool_free(lm->name);
    if (lm->script_val != NULL) __mln_lang_val_free(lm->script_val);
    if (lm->c_val != NULL) __mln_lang_val_free(lm->c_val);
    mln_event_set_fd(lm->ctx->lang->ev, lm->script_fd, M_EV_CLR, M_EV_UNLIMITED, NULL, NULL);
    mln_event_set_fd(lm->ctx->lang->ev, lm->c_fd, M_EV_CLR, M_EV_UNLIMITED, NULL, NULL);
    if (lm->script_fd >= 0) close(lm->script_fd);
    if (lm->c_fd >= 0) close(lm->c_fd);
    mln_alloc_free(lm);
}

void mln_lang_msg_free(mln_lang_ctx_t *ctx, mln_string_t *name)
{
    mln_lang_msg_t tmp;
    mln_rbtree_node_t *rn;
    tmp.name = name;
    rn = mln_rbtree_search(ctx->msg_map, ctx->msg_map->root, &tmp);
    if (mln_rbtree_null(rn, ctx->msg_map)) return;
    mln_rbtree_delete(ctx->msg_map, rn);
    mln_rbtree_node_free(ctx->msg_map, rn);
}

void mln_lang_msg_setHandler(mln_lang_ctx_t *ctx, mln_string_t *name, mln_msg_c_handler handler)
{
    mln_lang_msg_t tmp;
    mln_rbtree_node_t *rn;
    tmp.name = name;
    rn = mln_rbtree_search(ctx->msg_map, ctx->msg_map->root, &tmp);
    if (mln_rbtree_null(rn, ctx->msg_map)) return;
    ((mln_lang_msg_t *)(rn->data))->c_handler = handler;
}

int mln_lang_msg_sendMsg(mln_lang_ctx_t *ctx, mln_string_t *name, mln_lang_val_t *val, int isC)
{
    mln_lang_msg_t tmp, *lm;
    mln_rbtree_node_t *rn;
    tmp.name = name;
    rn = mln_rbtree_search(ctx->msg_map, ctx->msg_map->root, &tmp);
    if (mln_rbtree_null(rn, ctx->msg_map)) return -1;
    lm = (mln_lang_msg_t *)(rn->data);
    if (isC) {
        if (!lm->script_read) return -1;
        if (lm->script_val != NULL) __mln_lang_val_free(lm->script_val);
        lm->script_val = val;
        ++(val->ref);
        lm->script_read = 0;
        if (lm->script_wait && \
            mln_event_set_fd(ctx->lang->ev, \
                             lm->script_fd, \
                             M_EV_SEND|M_EV_ONESHOT, \
                             M_EV_UNLIMITED, \
                             lm, \
                             mln_lang_msg_scriptRecver) < 0)
        {
            return -1;
        }
    } else {
        if (!lm->c_read) return -1;
        if (mln_event_set_fd(ctx->lang->ev, \
                             lm->c_fd, \
                             M_EV_SEND|M_EV_ONESHOT, \
                             M_EV_UNLIMITED, \
                             lm, \
                             mln_lang_msg_cRecver) < 0)
        {
            return -1;
        }
        if (lm->c_val != NULL) __mln_lang_val_free(lm->c_val);
        lm->c_val = val;
        ++(val->ref);
        lm->c_read = 0;
    }
    return 0;
}

static void mln_lang_msg_scriptRecver(mln_event_t *ev, int fd, void *data)
{
    mln_lang_retExp_t *retExp;
    mln_lang_var_t *var;
    mln_lang_val_t *val;
    mln_lang_msg_t *lm = (mln_lang_msg_t *)data;
    mln_lang_ctx_t *ctx = lm->ctx;
    ASSERT(lm->script_wait);
    lm->script_wait = 0;
    val = lm->script_val;
    lm->script_val = NULL;
    --(val->ref);
    if ((var = __mln_lang_var_new(ctx->pool, NULL, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        __mln_lang_val_free(val);
        return;
    }
    if ((retExp = __mln_lang_retExp_new(ctx->pool, M_LANG_RETEXP_VAR, var)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        __mln_lang_var_free(var);
        return;
    }
    mln_lang_ctx_setRetExp(ctx, retExp);
    lm->script_read = 1;
    --(ctx->ref);
    mln_lang_ctx_chain_del(&(ctx->lang->wait_head), &(ctx->lang->wait_tail), ctx);
    if (ctx->step > 0)
        mln_lang_ctx_chain_add(&(ctx->lang->run_head), &(ctx->lang->run_tail), ctx);
    else
        mln_lang_ctx_chain_add(&(ctx->lang->blocked_head), &(ctx->lang->blocked_tail), ctx);
    __mln_lang_run(ctx->lang);
}

static void mln_lang_msg_cRecver(mln_event_t *ev, int fd, void *data)
{
    mln_lang_msg_t *lm = (mln_lang_msg_t *)data;
    ASSERT(!lm->c_read);
    if (lm->c_handler == NULL) return;
    if (lm->c_handler(lm->ctx, lm->c_val) < 0) {
        /* <0 -- lm already been freed.*/
        return;
    }
    lm->c_read = 1;
}

static int mln_lang_msg_func_new(mln_lang_ctx_t *ctx)
{
    mln_lang_val_t *val;
    mln_lang_var_t *var;
    mln_lang_func_detail_t *func;
    mln_string_t funcname = mln_string("mln_msgNew");
    mln_string_t name = mln_string("name");
    if ((func = mln_lang_func_detail_new(ctx->pool, M_FUNC_INTERNAL, mln_lang_msg_func_new_process, NULL)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if ((val = __mln_lang_val_new(ctx->pool, M_LANG_VAL_TYPE_NIL, NULL)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        __mln_lang_func_detail_free(func);
        return -1;
    }
    if ((var = __mln_lang_var_new(ctx->pool, &name, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        __mln_lang_val_free(val);
        __mln_lang_func_detail_free(func);
        return -1;
    }
    func->args_head = func->args_tail = var;
    func->nargs = 1;
    if ((val = __mln_lang_val_new(ctx->pool, M_LANG_VAL_TYPE_FUNC, func)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        __mln_lang_func_detail_free(func);
        return -1;
    }
    if ((var = __mln_lang_var_new(ctx->pool, &funcname, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        __mln_lang_val_free(val);
        return -1;
    }
    if (__mln_lang_symbolNode_join(ctx, M_LANG_SYMBOL_VAR, var) < 0) {
        __mln_lang_errmsg(ctx, "No memory.");
        __mln_lang_var_free(var);
        return -1;
    }
    return 0;
}

static mln_lang_retExp_t *mln_lang_msg_func_new_process(mln_lang_ctx_t *ctx)
{
    mln_lang_retExp_t *retExp;
    mln_lang_var_t *var;
    mln_string_t name = mln_string("name");
    mln_lang_symbolNode_t *sym;
    if ((sym = __mln_lang_symbolNode_search(ctx, &name, 1)) == NULL) {
        ASSERT(0);
        __mln_lang_errmsg(ctx, "Argument missing.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);
    var = sym->data.var;
    if (__mln_lang_var_getValType(var) != M_LANG_VAL_TYPE_STRING) {
        __mln_lang_errmsg(ctx, "Argument Not string.");
        return NULL;
    }
    if (mln_lang_msg_new(ctx, var->val->data.s) < 0) {
        if ((retExp = mln_lang_retExp_createTmpFalse(ctx->pool, NULL)) == NULL) {
            __mln_lang_errmsg(ctx, "No memory.");
            return NULL;
        }
    } else {
        if ((retExp = mln_lang_retExp_createTmpTrue(ctx->pool, NULL)) == NULL) {
            __mln_lang_errmsg(ctx, "No memory.");
            return NULL;
        }
    }
    return retExp;
}

static int mln_lang_msg_func_free(mln_lang_ctx_t *ctx)
{
    mln_lang_val_t *val;
    mln_lang_var_t *var;
    mln_lang_func_detail_t *func;
    mln_string_t funcname = mln_string("mln_msgFree");
    mln_string_t name = mln_string("name");
    if ((func = mln_lang_func_detail_new(ctx->pool, M_FUNC_INTERNAL, mln_lang_msg_func_free_process, NULL)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if ((val = __mln_lang_val_new(ctx->pool, M_LANG_VAL_TYPE_NIL, NULL)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        __mln_lang_func_detail_free(func);
        return -1;
    }
    if ((var = __mln_lang_var_new(ctx->pool, &name, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        __mln_lang_val_free(val);
        __mln_lang_func_detail_free(func);
        return -1;
    }
    func->args_head = func->args_tail = var;
    func->nargs = 1;
    if ((val = __mln_lang_val_new(ctx->pool, M_LANG_VAL_TYPE_FUNC, func)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        __mln_lang_func_detail_free(func);
        return -1;
    }
    if ((var = __mln_lang_var_new(ctx->pool, &funcname, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        __mln_lang_val_free(val);
        return -1;
    }
    if (__mln_lang_symbolNode_join(ctx, M_LANG_SYMBOL_VAR, var) < 0) {
        __mln_lang_errmsg(ctx, "No memory.");
        __mln_lang_var_free(var);
        return -1;
    }
    return 0;
}

static mln_lang_retExp_t *mln_lang_msg_func_free_process(mln_lang_ctx_t *ctx)
{
    mln_lang_retExp_t *retExp;
    mln_lang_var_t *var;
    mln_string_t name = mln_string("name");
    mln_lang_symbolNode_t *sym;
    if ((sym = __mln_lang_symbolNode_search(ctx, &name, 1)) == NULL) {
        ASSERT(0);
        __mln_lang_errmsg(ctx, "Argument missing.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);
    var = sym->data.var;
    if (__mln_lang_var_getValType(var) != M_LANG_VAL_TYPE_STRING) {
        __mln_lang_errmsg(ctx, "Argument Not string.");
        return NULL;
    }
    mln_lang_msg_free(ctx, var->val->data.s);
    if ((retExp = mln_lang_retExp_createTmpNil(ctx->pool, NULL)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        return NULL;
    }
    return retExp;
}

static int mln_lang_msg_func_recv(mln_lang_ctx_t *ctx)
{
    mln_lang_val_t *val;
    mln_lang_var_t *var;
    mln_lang_func_detail_t *func;
    mln_string_t funcname = mln_string("mln_msgRecv");
    mln_string_t name = mln_string("name");
    if ((func = mln_lang_func_detail_new(ctx->pool, M_FUNC_INTERNAL, mln_lang_msg_func_recv_process, NULL)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if ((val = __mln_lang_val_new(ctx->pool, M_LANG_VAL_TYPE_NIL, NULL)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        __mln_lang_func_detail_free(func);
        return -1;
    }
    if ((var = __mln_lang_var_new(ctx->pool, &name, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        __mln_lang_val_free(val);
        __mln_lang_func_detail_free(func);
        return -1;
    }
    mln_lang_var_chain_add(&(func->args_head), &(func->args_tail), var);
    func->nargs = 1;
    if ((val = __mln_lang_val_new(ctx->pool, M_LANG_VAL_TYPE_FUNC, func)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        __mln_lang_func_detail_free(func);
        return -1;
    }
    if ((var = __mln_lang_var_new(ctx->pool, &funcname, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        __mln_lang_val_free(val);
        return -1;
    }
    if (__mln_lang_symbolNode_join(ctx, M_LANG_SYMBOL_VAR, var) < 0) {
        __mln_lang_errmsg(ctx, "No memory.");
        __mln_lang_var_free(var);
        return -1;
    }
    return 0;
}

static mln_lang_retExp_t *mln_lang_msg_func_recv_process(mln_lang_ctx_t *ctx)
{
    mln_lang_retExp_t *retExp;
    mln_lang_var_t *var;
    mln_string_t name = mln_string("name");
    mln_lang_val_t *val;
    mln_lang_msg_t *lm, tmp;
    mln_rbtree_node_t *rn;
    mln_lang_symbolNode_t *sym;
    if ((sym = __mln_lang_symbolNode_search(ctx, &name, 1)) == NULL) {
        ASSERT(0);
        __mln_lang_errmsg(ctx, "Argument missing.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);
    var = sym->data.var;
    if (__mln_lang_var_getValType(var) != M_LANG_VAL_TYPE_STRING) {
        __mln_lang_errmsg(ctx, "Argument Not string.");
        return NULL;
    }

    tmp.name = var->val->data.s;
    rn = mln_rbtree_search(ctx->msg_map, ctx->msg_map->root, &tmp);
    if (mln_rbtree_null(rn, ctx->msg_map)) {
        if ((retExp = mln_lang_retExp_createTmpFalse(ctx->pool, NULL)) == NULL) {
            __mln_lang_errmsg(ctx, "No memory.");
            return NULL;
        }
        return retExp;
    }
    lm = (mln_lang_msg_t *)(rn->data);
    if (lm->script_read) {
        if (!lm->script_wait) {
            lm->script_wait = 1;
            ++(ctx->ref);
            mln_lang_ctx_chain_del(&(ctx->lang->run_head), &(ctx->lang->run_tail), ctx);
            mln_lang_ctx_chain_add(&(ctx->lang->wait_head), &(ctx->lang->wait_tail), ctx);
        }
        if ((retExp = __mln_lang_retExp_createTmpNil(ctx->pool, NULL)) == NULL) {
            __mln_lang_errmsg(ctx, "No memory.");
            return NULL;
        }
    } else {
        val = lm->script_val;
        lm->script_val = NULL;
        --(val->ref);
        lm->script_read = 1;
        if ((var = __mln_lang_var_new(lm->ctx->pool, NULL, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
            __mln_lang_errmsg(ctx, "No memory.");
            __mln_lang_val_free(val);
            return NULL;
        }
        if ((retExp = __mln_lang_retExp_new(lm->ctx->pool, M_LANG_RETEXP_VAR, var)) == NULL) {
            __mln_lang_errmsg(ctx, "No memory.");
            __mln_lang_var_free(var);
            return NULL;
        }
    }
    return retExp;
}

static int mln_lang_msg_func_send(mln_lang_ctx_t *ctx)
{
    mln_lang_val_t *val;
    mln_lang_var_t *var;
    mln_lang_func_detail_t *func;
    mln_string_t funcname = mln_string("mln_msgSend");
    mln_string_t name = mln_string("name");
    mln_string_t data = mln_string("var");
    if ((func = mln_lang_func_detail_new(ctx->pool, M_FUNC_INTERNAL, mln_lang_msg_func_send_process, NULL)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if ((val = __mln_lang_val_new(ctx->pool, M_LANG_VAL_TYPE_NIL, NULL)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        __mln_lang_func_detail_free(func);
        return -1;
    }
    if ((var = __mln_lang_var_new(ctx->pool, &name, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        __mln_lang_val_free(val);
        __mln_lang_func_detail_free(func);
        return -1;
    }
    mln_lang_var_chain_add(&(func->args_head), &(func->args_tail), var);
    func->nargs = 1;
    if ((val = __mln_lang_val_new(ctx->pool, M_LANG_VAL_TYPE_NIL, NULL)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        __mln_lang_func_detail_free(func);
        return -1;
    }
    if ((var = __mln_lang_var_new(ctx->pool, &data, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        __mln_lang_val_free(val);
        __mln_lang_func_detail_free(func);
        return -1;
    }
    mln_lang_var_chain_add(&(func->args_head), &(func->args_tail), var);
    func->nargs = 2;
    if ((val = __mln_lang_val_new(ctx->pool, M_LANG_VAL_TYPE_FUNC, func)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        __mln_lang_func_detail_free(func);
        return -1;
    }
    if ((var = __mln_lang_var_new(ctx->pool, &funcname, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        __mln_lang_val_free(val);
        return -1;
    }
    if (__mln_lang_symbolNode_join(ctx, M_LANG_SYMBOL_VAR, var) < 0) {
        __mln_lang_errmsg(ctx, "No memory.");
        __mln_lang_var_free(var);
        return -1;
    }
    return 0;
}

static mln_lang_retExp_t *mln_lang_msg_func_send_process(mln_lang_ctx_t *ctx)
{
    mln_lang_retExp_t *retExp;
    mln_lang_var_t *var;
    mln_string_t name = mln_string("name");
    mln_string_t data = mln_string("var");
    mln_string_t *s;
    mln_lang_symbolNode_t *sym;
    if ((sym = __mln_lang_symbolNode_search(ctx, &name, 1)) == NULL) {
        ASSERT(0);
        __mln_lang_errmsg(ctx, "Argument missing.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);
    var = sym->data.var;
    if (__mln_lang_var_getValType(var) != M_LANG_VAL_TYPE_STRING) {
        __mln_lang_errmsg(ctx, "Argument Not string.");
        return NULL;
    }
    s = var->val->data.s;

    if ((sym = __mln_lang_symbolNode_search(ctx, &data, 1)) == NULL) {
        ASSERT(0);
        __mln_lang_errmsg(ctx, "Argument missing.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);
    var = sym->data.var;

    if (mln_lang_msg_sendMsg(ctx, s, var->val, 0) < 0) {
        if ((retExp = mln_lang_retExp_createTmpFalse(ctx->pool, NULL)) == NULL) {
            __mln_lang_errmsg(ctx, "No memory.");
            return NULL;
        }
    } else {
        if ((retExp = mln_lang_retExp_createTmpTrue(ctx->pool, NULL)) == NULL) {
            __mln_lang_errmsg(ctx, "No memory.");
            return NULL;
        }
    }
    return retExp;
}

static int mln_lang_func_watch(mln_lang_ctx_t *ctx)
{
    mln_lang_val_t *val;
    mln_lang_var_t *var;
    mln_lang_func_detail_t *func;
    mln_string_t funcname = mln_string("mln_watch");
    mln_string_t v1 = mln_string("var");
    mln_string_t v2 = mln_string("func");
    mln_string_t v3 = mln_string("data");
    if ((func = mln_lang_func_detail_new(ctx->pool, M_FUNC_INTERNAL, mln_lang_func_watch_process, NULL)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if ((val = __mln_lang_val_new(ctx->pool, M_LANG_VAL_TYPE_NIL, NULL)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        __mln_lang_func_detail_free(func);
        return -1;
    }
    if ((var = __mln_lang_var_new(ctx->pool, &v1, M_LANG_VAR_REFER, val, NULL)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        __mln_lang_val_free(val);
        __mln_lang_func_detail_free(func);
        return -1;
    }
    mln_lang_var_chain_add(&(func->args_head), &(func->args_tail), var);
    func->nargs = 1;
    if ((val = __mln_lang_val_new(ctx->pool, M_LANG_VAL_TYPE_NIL, NULL)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        __mln_lang_func_detail_free(func);
        return -1;
    }
    if ((var = __mln_lang_var_new(ctx->pool, &v2, M_LANG_VAR_REFER, val, NULL)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        __mln_lang_val_free(val);
        __mln_lang_func_detail_free(func);
        return -1;
    }
    mln_lang_var_chain_add(&(func->args_head), &(func->args_tail), var);
    func->nargs = 2;
    if ((val = __mln_lang_val_new(ctx->pool, M_LANG_VAL_TYPE_NIL, NULL)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        __mln_lang_func_detail_free(func);
        return -1;
    }
    if ((var = __mln_lang_var_new(ctx->pool, &v3, M_LANG_VAR_REFER, val, NULL)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        __mln_lang_val_free(val);
        __mln_lang_func_detail_free(func);
        return -1;
    }
    mln_lang_var_chain_add(&(func->args_head), &(func->args_tail), var);
    func->nargs = 3;
    if ((val = __mln_lang_val_new(ctx->pool, M_LANG_VAL_TYPE_FUNC, func)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        __mln_lang_func_detail_free(func);
        return -1;
    }
    if ((var = __mln_lang_var_new(ctx->pool, &funcname, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        __mln_lang_val_free(val);
        return -1;
    }
    if (__mln_lang_symbolNode_join(ctx, M_LANG_SYMBOL_VAR, var) < 0) {
        __mln_lang_errmsg(ctx, "No memory.");
        __mln_lang_var_free(var);
        return -1;
    }
    return 0;
}

static mln_lang_retExp_t *mln_lang_func_watch_process(mln_lang_ctx_t *ctx)
{
    mln_lang_retExp_t *retExp;
    mln_string_t v1 = mln_string("var");
    mln_string_t v2 = mln_string("func");
    mln_string_t v3 = mln_string("data");
    mln_lang_symbolNode_t *sym;
    mln_lang_val_t *val1;
    mln_lang_func_detail_t *func, *dup;
    if ((sym = __mln_lang_symbolNode_search(ctx, &v1, 1)) == NULL) {
        ASSERT(0);
        __mln_lang_errmsg(ctx, "Argument missing.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);
    val1 = sym->data.var->val;

    if ((sym = __mln_lang_symbolNode_search(ctx, &v2, 1)) == NULL) {
        ASSERT(0);
        __mln_lang_errmsg(ctx, "Argument missing.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);
    if (sym->data.var->val->type != M_LANG_VAL_TYPE_FUNC) {
        if ((retExp = mln_lang_retExp_createTmpFalse(ctx->pool, NULL)) == NULL) {
            __mln_lang_errmsg(ctx, "No memory.");
            return NULL;
        }
    } else {
        mln_lang_val_t *val3;
        func = sym->data.var->val->data.func;

        if ((sym = __mln_lang_symbolNode_search(ctx, &v3, 1)) == NULL) {
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

        if ((retExp = mln_lang_retExp_createTmpTrue(ctx->pool, NULL)) == NULL) {
            __mln_lang_errmsg(ctx, "No memory.");
            return NULL;
        }
    }
    return retExp;
}

static int mln_lang_func_unwatch(mln_lang_ctx_t *ctx)
{
    mln_lang_val_t *val;
    mln_lang_var_t *var;
    mln_lang_func_detail_t *func;
    mln_string_t funcname = mln_string("mln_unwatch");
    mln_string_t v1 = mln_string("var");
    if ((func = mln_lang_func_detail_new(ctx->pool, M_FUNC_INTERNAL, mln_lang_func_unwatch_process, NULL)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if ((val = __mln_lang_val_new(ctx->pool, M_LANG_VAL_TYPE_NIL, NULL)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        __mln_lang_func_detail_free(func);
        return -1;
    }
    if ((var = __mln_lang_var_new(ctx->pool, &v1, M_LANG_VAR_REFER, val, NULL)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        __mln_lang_val_free(val);
        __mln_lang_func_detail_free(func);
        return -1;
    }
    mln_lang_var_chain_add(&(func->args_head), &(func->args_tail), var);
    func->nargs = 1;
    if ((val = __mln_lang_val_new(ctx->pool, M_LANG_VAL_TYPE_FUNC, func)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        __mln_lang_func_detail_free(func);
        return -1;
    }
    if ((var = __mln_lang_var_new(ctx->pool, &funcname, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        __mln_lang_val_free(val);
        return -1;
    }
    if (__mln_lang_symbolNode_join(ctx, M_LANG_SYMBOL_VAR, var) < 0) {
        __mln_lang_errmsg(ctx, "No memory.");
        __mln_lang_var_free(var);
        return -1;
    }
    return 0;
}

static mln_lang_retExp_t *mln_lang_func_unwatch_process(mln_lang_ctx_t *ctx)
{
    mln_lang_retExp_t *retExp;
    mln_string_t v1 = mln_string("var");
    mln_lang_symbolNode_t *sym;
    mln_lang_val_t *val1;
    if ((sym = __mln_lang_symbolNode_search(ctx, &v1, 1)) == NULL) {
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
    if ((retExp = mln_lang_retExp_createTmpNil(ctx->pool, NULL)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        return NULL;
    }
    return retExp;
}

static int
mln_lang_watch_funcBuild(mln_lang_ctx_t *ctx, mln_lang_stack_node_t *node, mln_lang_func_detail_t *funcdef, mln_lang_val_t *udata, mln_lang_val_t *new)
{
    mln_lang_funccall_val_t *func;
    mln_lang_var_t *var;

    if ((func = __mln_lang_funccall_val_new(ctx->pool, NULL)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    func->prototype = funcdef;
    if ((var = __mln_lang_var_new(ctx->pool, NULL, M_LANG_VAR_REFER, new, NULL)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        __mln_lang_funccall_val_free(func);
        return -1;
    }
    mln_lang_var_chain_add(&(func->args_head), &(func->args_tail), var);
    func->nargs = 1;
    if ((var = __mln_lang_var_new(ctx->pool, NULL, M_LANG_VAR_REFER, udata, NULL)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        __mln_lang_funccall_val_free(func);
        return -1;
    }
    mln_lang_var_chain_add(&(func->args_head), &(func->args_tail), var);
    func->nargs = 2;

    if (mln_lang_stack_handler_funccall_run(ctx, node, func) < 0) {
        __mln_lang_funccall_val_free(func);
        return -1;
    }
    node->call = 1;
    __mln_lang_funccall_val_free(func);
    return 0;
}

static int mln_lang_msg_func_dump(mln_lang_ctx_t *ctx)
{
    mln_lang_val_t *val;
    mln_lang_var_t *var;
    mln_lang_func_detail_t *func;
    mln_string_t funcname = mln_string("mln_dump");
    mln_string_t v1 = mln_string("var");
    if ((func = mln_lang_func_detail_new(ctx->pool, M_FUNC_INTERNAL, mln_lang_msg_func_dump_process, NULL)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if ((val = __mln_lang_val_new(ctx->pool, M_LANG_VAL_TYPE_NIL, NULL)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        __mln_lang_func_detail_free(func);
        return -1;
    }
    if ((var = __mln_lang_var_new(ctx->pool, &v1, M_LANG_VAR_REFER, val, NULL)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        __mln_lang_val_free(val);
        __mln_lang_func_detail_free(func);
        return -1;
    }
    mln_lang_var_chain_add(&(func->args_head), &(func->args_tail), var);
    func->nargs = 1;
    if ((val = __mln_lang_val_new(ctx->pool, M_LANG_VAL_TYPE_FUNC, func)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        __mln_lang_func_detail_free(func);
        return -1;
    }
    if ((var = __mln_lang_var_new(ctx->pool, &funcname, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        __mln_lang_val_free(val);
        return -1;
    }
    if (__mln_lang_symbolNode_join(ctx, M_LANG_SYMBOL_VAR, var) < 0) {
        __mln_lang_errmsg(ctx, "No memory.");
        __mln_lang_var_free(var);
        return -1;
    }
    return 0;
}

static mln_lang_retExp_t *mln_lang_msg_func_dump_process(mln_lang_ctx_t *ctx)
{
    mln_lang_retExp_t *retExp;
    mln_string_t v1 = mln_string("var");
    mln_lang_symbolNode_t *sym;
    if ((sym = __mln_lang_symbolNode_search(ctx, &v1, 1)) == NULL) {
        ASSERT(0);
        __mln_lang_errmsg(ctx, "Argument missing.");
        return NULL;
    }
    (void)mln_lang_dump_symbol(NULL, sym, NULL);
    if ((retExp = mln_lang_retExp_createTmpNil(ctx->pool, NULL)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        return NULL;
    }
    return retExp;
}

static int mln_lang_msg_installer(mln_lang_ctx_t *ctx)
{
    if (mln_lang_msg_func_new(ctx) < 0) return -1;
    if (mln_lang_msg_func_free(ctx) < 0) return -1;
    if (mln_lang_msg_func_recv(ctx) < 0) return -1;
    if (mln_lang_msg_func_send(ctx) < 0) return -1;
    if (mln_lang_msg_func_dump(ctx) < 0) return -1;
    if (mln_lang_func_watch(ctx) < 0) return -1;
    if (mln_lang_func_unwatch(ctx) < 0) return -1;
    if (mln_lang_tcp_connect(ctx) < 0) return -1;
    if (mln_lang_tcp_listen(ctx) < 0) return -1;
    if (mln_lang_tcp_send(ctx) < 0) return -1;
    if (mln_lang_tcp_recv(ctx) < 0) return -1;
    if (mln_lang_tcp_close(ctx) < 0) return -1;
    if (mln_lang_tcp_shutdown(ctx) < 0) return -1;
    return 0;
}

/*
 * mln_lang_gc_item_t
 */
static inline int
mln_lang_gc_item_new(mln_alloc_t *pool, mln_gc_t *gc, mln_lang_gcType_t type, void *data)
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
            gi->data.obj->gcItem = gi;
            break;
        default:
            gi->data.array = (mln_lang_array_t *)data;
            gi->data.array->gcItem = gi;
            break;
    }
    gi->gcData = NULL;
    if (mln_gc_add(gc, gi) < 0) {
        mln_alloc_free(gi);
        return -1;
    }
    return 0;
}

static void mln_lang_gc_item_free(mln_lang_gc_item_t *gcItem)
{
    if (gcItem == NULL) return;
    switch (gcItem->type) {
        case M_GC_OBJ:
            ASSERT(gcItem->data.obj->ref <= 1);
            gcItem->data.obj->gcItem = NULL;
            mln_lang_object_free(gcItem->data.obj);
            break;
        default:
            ASSERT(gcItem->data.array->ref <= 1);
            gcItem->data.array->gcItem = NULL;
            __mln_lang_array_free(gcItem->data.array);
            break;
    }
    mln_alloc_free(gcItem);
}

static inline void mln_lang_gc_item_freeImmediatly(mln_lang_gc_item_t *gcItem)
{
    if (gcItem == NULL) return;
    mln_alloc_free(gcItem);
}

static void *mln_lang_gc_item_getter(mln_lang_gc_item_t *gcItem)
{
    return gcItem->gcData;
}

static void mln_lang_gc_item_setter(mln_lang_gc_item_t *gcItem, void *gcData)
{
    gcItem->gcData = gcData;
}

static void mln_lang_gc_item_memberSetter(mln_gc_t *gc, mln_lang_gc_item_t *gcItem)
{
    mln_rbtree_t *t;
    switch (gcItem->type) {
        case M_GC_OBJ:
            t = gcItem->data.obj->members;
            mln_rbtree_scan_all(t, mln_lang_gc_item_memberSetter_objScanner, gc);
            break;
        default:
            t = gcItem->data.array->elems_index;
            mln_rbtree_scan_all(t, mln_lang_gc_item_memberSetter_arrayScanner, gc);
            break;
    }
}

static int mln_lang_gc_item_memberSetter_objScanner(mln_rbtree_node_t *node, void *rn_data, void *udata)
{
    mln_lang_val_t *val;
    mln_lang_var_t *var = (mln_lang_var_t *)rn_data;
    mln_gc_t *gc = (mln_gc_t *)udata;
    mln_s32_t type = __mln_lang_var_getValType(var);
    val = mln_lang_var_getVal(var);
    if (type == M_LANG_VAL_TYPE_OBJECT) {
        mln_gc_addForCollect(gc, val->data.obj->gcItem);
    } else if (type == M_LANG_VAL_TYPE_ARRAY) {
        mln_gc_addForCollect(gc, val->data.array->gcItem);
    }
    return 0;
}

static int mln_lang_gc_item_memberSetter_arrayScanner(mln_rbtree_node_t *node, void *rn_data, void *udata)
{
    mln_lang_val_t *val;
    mln_lang_array_elem_t *elem = (mln_lang_array_elem_t *)rn_data;
    mln_gc_t *gc = (mln_gc_t *)udata;
    mln_s32_t type;
    if (elem->key != NULL) {
        type = __mln_lang_var_getValType(elem->key);
        val = mln_lang_var_getVal(elem->key);
        if (type == M_LANG_VAL_TYPE_OBJECT) {
            mln_gc_addForCollect(gc, val->data.obj->gcItem);
        } else if (type == M_LANG_VAL_TYPE_ARRAY) {
            mln_gc_addForCollect(gc, val->data.array->gcItem);
        }
    }
    if (elem->value != NULL) {
        type = __mln_lang_var_getValType(elem->value);
        val = mln_lang_var_getVal(elem->value);
        if (type == M_LANG_VAL_TYPE_OBJECT) {
            mln_gc_addForCollect(gc, val->data.obj->gcItem);
        } else if (type == M_LANG_VAL_TYPE_ARRAY) {
            mln_gc_addForCollect(gc, val->data.array->gcItem);
        }
    }
    return 0;
}

static void mln_lang_gc_item_moveHandler(mln_gc_t *destGC, mln_lang_gc_item_t *gcItem)
{
    gcItem->gc = destGC;
}

static void mln_lang_gc_item_rootSetter(mln_gc_t *gc, mln_lang_scope_t *scope)
{
    if (scope == NULL) return;
    ASSERT(scope->gc == gc);
    mln_rbtree_scan_all(scope->symbols, mln_lang_gc_item_rootSetterScanner, gc);
    if (scope->ctx->scope_head != scope && scope->ctx->scope_head != NULL) {
        mln_rbtree_scan_all(scope->ctx->scope_head->symbols, mln_lang_gc_item_rootSetterScanner, gc);
    }
}

static int mln_lang_gc_item_rootSetterScanner(mln_rbtree_node_t *node, void *rn_data, void *udata)
{
    mln_s32_t type;
    mln_lang_val_t *val;
    mln_gc_t *gc = (mln_gc_t *)udata;
    mln_lang_symbolNode_t *sym = (mln_lang_symbolNode_t *)rn_data;
    if (sym->type != M_LANG_SYMBOL_VAR) return 0;
    type = __mln_lang_var_getValType(sym->data.var);
    val = mln_lang_var_getVal(sym->data.var);
    if (type == M_LANG_VAL_TYPE_OBJECT) {
        mln_gc_addForCollect(gc, val->data.obj->gcItem);
    } else if (type == M_LANG_VAL_TYPE_ARRAY) {
        mln_gc_addForCollect(gc, val->data.array->gcItem);
    }
    return 0;
}

static void mln_lang_gc_item_cleanSearcher(mln_gc_t *gc, mln_lang_gc_item_t *gcItem)
{
    mln_rbtree_t *t;
    struct mln_lang_gc_scan_s gs;
    switch (gcItem->type) {
        case M_GC_OBJ:
            t = gcItem->data.obj->members;
            gs.tree = t;
            gs.gc = gc;
            mln_rbtree_scan_all(t, mln_lang_gc_item_cleanSearcher_objScanner, &gs);
            break;
        default:
            t = gcItem->data.array->elems_index;
            gs.tree = t;
            gs.gc = gc;
            mln_rbtree_scan_all(t, mln_lang_gc_item_cleanSearcher_arrayScanner, &gs);
            break;
    }
}

static int mln_lang_gc_item_cleanSearcher_objScanner(mln_rbtree_node_t *node, void *rn_data, void *udata)
{
    mln_lang_val_t *val;
    mln_lang_var_t *var = (mln_lang_var_t *)rn_data;
    struct mln_lang_gc_scan_s *gs = (struct mln_lang_gc_scan_s *)udata;
    mln_s32_t type = __mln_lang_var_getValType(var);
    val = mln_lang_var_getVal(var);
    if (type == M_LANG_VAL_TYPE_OBJECT) {
        if (mln_gc_addForClean(gs->gc, val->data.obj->gcItem) < 0) {
            if (val->data.obj->ref > 0) {
                --(val->data.obj->ref);
            }
            val->data.obj = NULL;
            mln_rbtree_delete(gs->tree, node);
            mln_rbtree_node_free(gs->tree, node);
        }
    } else if (type == M_LANG_VAL_TYPE_ARRAY) {
        if (mln_gc_addForClean(gs->gc, val->data.array->gcItem) < 0) {
            if (val->data.array->ref > 0) {
                --(val->data.array->ref);
            }
            val->data.array = NULL;
            mln_rbtree_delete(gs->tree, node);
            mln_rbtree_node_free(gs->tree, node);
        }
    }
    return 0;
}

static int mln_lang_gc_item_cleanSearcher_arrayScanner(mln_rbtree_node_t *node, void *rn_data, void *udata)
{
    mln_lang_val_t *val;
    mln_lang_array_elem_t *elem = (mln_lang_array_elem_t *)rn_data;
    struct mln_lang_gc_scan_s *gs = (struct mln_lang_gc_scan_s *)udata;
    mln_s32_t type;
    int needToFree = 0;

    if (elem->key != NULL) {
        type = __mln_lang_var_getValType(elem->key);
        val = mln_lang_var_getVal(elem->key);
        if (type == M_LANG_VAL_TYPE_OBJECT) {
            if (mln_gc_addForClean(gs->gc, val->data.obj->gcItem) < 0) {
                if (val->data.obj->ref > 0) {
                    --(val->data.obj->ref);
                }
                val->data.obj = NULL;
                needToFree = 1;
            }
        } else if (type == M_LANG_VAL_TYPE_ARRAY) {
            if (mln_gc_addForClean(gs->gc, val->data.array->gcItem) < 0) {
                if (val->data.array->ref > 0) {
                    --(val->data.array->ref);
                }
                val->data.array = NULL;
                needToFree = 1;
            }
        }
    }
    if (elem->value != NULL) {
        type = __mln_lang_var_getValType(elem->value);
        val = mln_lang_var_getVal(elem->value);
        if (type == M_LANG_VAL_TYPE_OBJECT) {
            if (mln_gc_addForClean(gs->gc, val->data.obj->gcItem) < 0) {
                if (val->data.obj->ref > 0) {
                    --(val->data.obj->ref);
                }
                val->data.obj = NULL;
                needToFree = 1;
            }
        } else if (type == M_LANG_VAL_TYPE_ARRAY) {
            if (mln_gc_addForClean(gs->gc, val->data.array->gcItem) < 0) {
                if (val->data.array->ref > 0) {
                    --(val->data.array->ref);
                }
                val->data.array = NULL;
                needToFree = 1;
            }
        }
    }
    if (needToFree) {
        mln_rbtree_delete(gs->tree, node);
        mln_rbtree_node_free(gs->tree, node);
    }
    return 0;
}

static void mln_lang_gc_item_freeHandler(mln_lang_gc_item_t *gcItem)
{
    gcItem->gc = NULL;
}


/*
 * tcp connection
 */
static mln_lang_tcp_t *
mln_lang_tcp_new(mln_lang_ctx_t *ctx, int sockfd, const char *ip, mln_u16_t port, mln_string_t *msgName, mln_s32_t timeout)
{
    mln_lang_tcp_t *tcp;
    if ((tcp = (mln_lang_tcp_t *)mln_alloc_m(ctx->pool, sizeof(mln_lang_tcp_t))) == NULL) {
        return NULL;
    }
    tcp->ctx = ctx;
    if (mln_tcp_conn_init(&(tcp->connection), sockfd) < 0) {
        mln_alloc_free(tcp);
        return NULL;
    }
    if ((tcp->msgName = mln_string_pool_dup(ctx->pool, msgName)) == NULL) {
        mln_tcp_conn_destroy(&(tcp->connection));
        mln_alloc_free(tcp);
        return NULL;
    }
    if ((tcp->ip = mln_string_pool_new(ctx->pool, ip)) == NULL) {
        mln_string_pool_free(tcp->msgName);
        mln_tcp_conn_destroy(&(tcp->connection));
        mln_alloc_free(tcp);
        return NULL;
    }
    tcp->port = port;
    tcp->send = 1;
    tcp->recv = 1;
    tcp->readyClosed = 0;
    tcp->timeout = timeout;
    return tcp;
}

static void mln_lang_tcp_free(mln_lang_tcp_t *tcp)
{
    if (tcp == NULL) return;
    close(mln_tcp_conn_get_fd(&(tcp->connection)));
    mln_tcp_conn_destroy(&(tcp->connection));
    if (tcp->ip != NULL) mln_string_pool_free(tcp->ip);
    if (tcp->msgName != NULL) mln_string_pool_free(tcp->msgName);
    mln_alloc_free(tcp);
}

static int mln_lang_tcp_cmp(const void *data1, const void *data2)
{
    mln_lang_tcp_t *tcp1 = (mln_lang_tcp_t *)data1;
    mln_lang_tcp_t *tcp2 = (mln_lang_tcp_t *)data2;
    int fd1 = mln_tcp_conn_get_fd(&(tcp1->connection));
    int fd2 = mln_tcp_conn_get_fd(&(tcp2->connection));
    if (fd1 > fd2) return 1;
    if (fd1 == fd2) return 0;
    return -1;
}

static void mln_lang_tcp_node_free(void *data)
{
    mln_lang_tcp_t *tcp = (mln_lang_tcp_t *)data;
    if (tcp == NULL) return;
    int fd = mln_tcp_conn_get_fd(&(tcp->connection));
    mln_event_set_fd(tcp->ctx->lang->ev, fd, M_EV_CLR, M_EV_UNLIMITED, NULL, NULL);
    mln_lang_tcp_free(tcp);
}

static int mln_lang_tcp_connect(mln_lang_ctx_t *ctx)
{
    mln_lang_val_t *val;
    mln_lang_var_t *var;
    mln_lang_func_detail_t *func;
    mln_string_t funcname = mln_string("mln_tcpConnect");
    mln_string_t v1 = mln_string("host");
    mln_string_t v2 = mln_string("service");
    mln_string_t v3 = mln_string("msgName");
    mln_string_t v4 = mln_string("timeout");
    if ((func = mln_lang_func_detail_new(ctx->pool, M_FUNC_INTERNAL, mln_lang_tcp_connect_process, NULL)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if ((val = __mln_lang_val_new(ctx->pool, M_LANG_VAL_TYPE_NIL, NULL)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        __mln_lang_func_detail_free(func);
        return -1;
    }
    if ((var = __mln_lang_var_new(ctx->pool, &v1, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        __mln_lang_val_free(val);
        __mln_lang_func_detail_free(func);
        return -1;
    }
    mln_lang_var_chain_add(&(func->args_head), &(func->args_tail), var);
    func->nargs = 1;
    if ((val = __mln_lang_val_new(ctx->pool, M_LANG_VAL_TYPE_NIL, NULL)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        __mln_lang_func_detail_free(func);
        return -1;
    }
    if ((var = __mln_lang_var_new(ctx->pool, &v2, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        __mln_lang_val_free(val);
        __mln_lang_func_detail_free(func);
        return -1;
    }
    mln_lang_var_chain_add(&(func->args_head), &(func->args_tail), var);
    func->nargs = 2;
    if ((val = __mln_lang_val_new(ctx->pool, M_LANG_VAL_TYPE_NIL, NULL)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        __mln_lang_func_detail_free(func);
        return -1;
    }
    if ((var = __mln_lang_var_new(ctx->pool, &v3, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        __mln_lang_val_free(val);
        __mln_lang_func_detail_free(func);
        return -1;
    }
    mln_lang_var_chain_add(&(func->args_head), &(func->args_tail), var);
    func->nargs = 3;
    if ((val = __mln_lang_val_new(ctx->pool, M_LANG_VAL_TYPE_NIL, NULL)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        __mln_lang_func_detail_free(func);
        return -1;
    }
    if ((var = __mln_lang_var_new(ctx->pool, &v4, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        __mln_lang_val_free(val);
        __mln_lang_func_detail_free(func);
        return -1;
    }
    mln_lang_var_chain_add(&(func->args_head), &(func->args_tail), var);
    func->nargs = 4;
    if ((val = __mln_lang_val_new(ctx->pool, M_LANG_VAL_TYPE_FUNC, func)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        __mln_lang_func_detail_free(func);
        return -1;
    }
    if ((var = __mln_lang_var_new(ctx->pool, &funcname, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        __mln_lang_val_free(val);
        return -1;
    }
    if (__mln_lang_symbolNode_join(ctx, M_LANG_SYMBOL_VAR, var) < 0) {
        __mln_lang_errmsg(ctx, "No memory.");
        __mln_lang_var_free(var);
        return -1;
    }
    return 0;
}

static mln_lang_retExp_t *mln_lang_tcp_connect_process(mln_lang_ctx_t *ctx)
{
    int fd;
    mln_s32_t timeout;
    mln_u16_t port = 0;
    char host[128] = {0}, service[32] = {0}, ip[128] = {0};
    mln_lang_val_t *val;
    mln_lang_tcp_t *tcp;
    mln_string_t *msgName;
    mln_rbtree_node_t *rn;
    struct addrinfo addr, *res = NULL;
    mln_lang_retExp_t *retExp;
    mln_lang_symbolNode_t *sym;
    mln_string_t v1 = mln_string("host");
    mln_string_t v2 = mln_string("service");
    mln_string_t v3 = mln_string("msgName");
    mln_string_t v4 = mln_string("timeout");

    if ((sym = __mln_lang_symbolNode_search(ctx, &v1, 1)) == NULL) {
        ASSERT(0);
        __mln_lang_errmsg(ctx, "Argument missing.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);
    if (mln_lang_var_getValType(sym->data.var) != M_LANG_VAL_TYPE_STRING) {
        __mln_lang_errmsg(ctx, "Invalid type of argument 1.");
        return NULL;
    }
    val = sym->data.var->val;
    if (val->data.s->len > sizeof(host)-1) {
        __mln_lang_errmsg(ctx, "Invalid host.");
        return NULL;
    }
    memcpy(host, val->data.s->data, val->data.s->len);

    if ((sym = __mln_lang_symbolNode_search(ctx, &v2, 1)) == NULL) {
        ASSERT(0);
        __mln_lang_errmsg(ctx, "Argument missing.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);
    if (mln_lang_var_getValType(sym->data.var) != M_LANG_VAL_TYPE_STRING) {
        __mln_lang_errmsg(ctx, "Invalid type of argument 2.");
        return NULL;
    }
    val = sym->data.var->val;
    if (val->data.s->len > sizeof(service)-1) {
        __mln_lang_errmsg(ctx, "Invalid service.");
        return NULL;
    }
    memcpy(service, val->data.s->data, val->data.s->len);

    if ((sym = __mln_lang_symbolNode_search(ctx, &v3, 1)) == NULL) {
        ASSERT(0);
        __mln_lang_errmsg(ctx, "Argument missing.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);
    if (mln_lang_var_getValType(sym->data.var) != M_LANG_VAL_TYPE_STRING) {
        __mln_lang_errmsg(ctx, "Invalid type of argument 3.");
        return NULL;
    }
    val = sym->data.var->val;
    msgName = val->data.s;

    if ((sym = __mln_lang_symbolNode_search(ctx, &v4, 1)) == NULL) {
        ASSERT(0);
        __mln_lang_errmsg(ctx, "Argument missing.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);
    if (mln_lang_var_getValType(sym->data.var) == M_LANG_VAL_TYPE_NIL) {
        timeout = M_EV_UNLIMITED;
    } else if (mln_lang_var_getValType(sym->data.var) == M_LANG_VAL_TYPE_INT) {
        val = sym->data.var->val;
        timeout = val->data.i;
    } else {
        __mln_lang_errmsg(ctx, "Invalid type of argument 4.");
        return NULL;
    }

    memset(&addr, 0, sizeof(addr));
    addr.ai_flags = AI_PASSIVE;
    addr.ai_family = AF_UNSPEC;
    addr.ai_socktype = SOCK_STREAM;
    addr.ai_protocol = IPPROTO_IP;
    if (getaddrinfo(host, service, &addr, &res) != 0 || res == NULL) {
        if ((retExp = mln_lang_retExp_createTmpFalse(ctx->pool, NULL)) == NULL) {
            __mln_lang_errmsg(ctx, "No memory.");
            return NULL;
        }
        return retExp;
    }
    if ((fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) < 0) {
        freeaddrinfo(res);
        if ((retExp = mln_lang_retExp_createTmpFalse(ctx->pool, NULL)) == NULL) {
            __mln_lang_errmsg(ctx, "No memory.");
            return NULL;
        }
        return retExp;
    }

    if (mln_lang_tcp_getinfo(res->ai_addr, ip, &port) < 0) {
        freeaddrinfo(res);
        if ((retExp = mln_lang_retExp_createTmpFalse(ctx->pool, NULL)) == NULL) {
            __mln_lang_errmsg(ctx, "No memory.");
            return NULL;
        }
        return retExp;
    }
    if ((tcp = mln_lang_tcp_new(ctx, fd, ip, port, msgName, timeout)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        freeaddrinfo(res);
        close(fd);
        return NULL;
    }
    if ((rn = mln_rbtree_node_new(ctx->tcp_set, tcp)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        freeaddrinfo(res);
        mln_lang_tcp_free(tcp);
        return NULL;
    }
    mln_rbtree_insert(ctx->tcp_set, rn);

    if (mln_event_set_fd(ctx->lang->ev, \
                         fd, \
                         M_EV_SEND|M_EV_RECV|M_EV_NONBLOCK|M_EV_ONESHOT, \
                         M_EV_UNLIMITED, \
                         tcp, \
                         mln_lang_tcp_connect_handler) < 0)
    {
        __mln_lang_errmsg(ctx, "No memory.");
        freeaddrinfo(res);
        mln_rbtree_delete(ctx->tcp_set, rn);
        mln_rbtree_node_free(ctx->tcp_set, rn);
        return NULL;
    }

    if (connect(fd, res->ai_addr, res->ai_addrlen) < 0 && errno != EINPROGRESS) {
        freeaddrinfo(res);
        mln_event_set_fd(ctx->lang->ev, fd, M_EV_CLR, M_EV_UNLIMITED, NULL, NULL);
        mln_rbtree_delete(ctx->tcp_set, rn);
        mln_rbtree_node_free(ctx->tcp_set, rn);
        if ((retExp = mln_lang_retExp_createTmpFalse(ctx->pool, NULL)) == NULL) {
            __mln_lang_errmsg(ctx, "No memory.");
            return NULL;
        }
        return retExp;
    }
    freeaddrinfo(res);

    if ((retExp = mln_lang_retExp_createTmpTrue(ctx->pool, NULL)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        mln_event_set_fd(ctx->lang->ev, fd, M_EV_CLR, M_EV_UNLIMITED, NULL, NULL);
        mln_rbtree_delete(ctx->tcp_set, rn);
        mln_rbtree_node_free(ctx->tcp_set, rn);
        return NULL;
    }
    return retExp;
}

static int mln_lang_tcp_getinfo(const struct sockaddr *addr, char *ip, mln_u16_t *port)
{
    char *numeric_addr = NULL;
    char addr_buf[128] = {0};

    if (AF_INET == addr->sa_family) {
        numeric_addr = (char *)(&((struct sockaddr_in*)addr)->sin_addr);
        *port = ntohs(((struct sockaddr_in *)addr)->sin_port);
    } else if (AF_INET6 == addr->sa_family) {
        numeric_addr = (char *)(&((struct sockaddr_in6*)addr)->sin6_addr);
        *port = ntohs(((struct sockaddr_in6 *)addr)->sin6_port);
    } else {
        return -1;
    }
    if (inet_ntop(addr->sa_family, numeric_addr, addr_buf, sizeof(addr_buf)) == NULL)
        return -1;
    memcpy(ip, addr_buf, sizeof(addr_buf));
    return 0;
}

static void mln_lang_tcp_connect_handler(mln_event_t *ev, int fd, void *data)
{
    mln_lang_tcp_t *tcp = (mln_lang_tcp_t *)data;
    int err = 0;
    socklen_t len = sizeof(err);
    mln_string_t errtype = mln_string("tcp_error");
    mln_string_t succtype = mln_string("tcp_connect");

    if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &len) < 0) {
        tcp->readyClosed = 1;
        mln_event_set_fd(ev, fd, M_EV_CLR, M_EV_UNLIMITED, NULL, NULL);
        mln_lang_tcp_setMsg(&errtype, tcp, NULL);
    } else {
        if (err) {
            if (err != EINPROGRESS) {
                tcp->readyClosed = 1;
                mln_event_set_fd(ev, fd, M_EV_CLR, M_EV_UNLIMITED, NULL, NULL);
                mln_lang_tcp_setMsg(&errtype, tcp, NULL);
            } else {
                mln_event_set_fd(ev, fd, M_EV_SEND|M_EV_RECV|M_EV_NONBLOCK|M_EV_ONESHOT, M_EV_UNLIMITED, tcp, mln_lang_tcp_connect_handler);
            }
        } else {
            mln_event_set_fd(ev, fd, M_EV_RECV|M_EV_NONBLOCK, tcp->timeout, tcp, mln_lang_tcp_recv_handler);
            if (tcp->timeout != M_EV_UNLIMITED) {
                mln_event_set_fd_timeout_handler(ev, fd, tcp, mln_lang_tcp_timeout_handler);
            }
            mln_lang_tcp_setMsg(&succtype, tcp, NULL);
        }
    }
}

struct mln_lang_tcp_msg_udata {
    mln_string_t   *type;
    mln_lang_tcp_t *target;
};

static void mln_lang_tcp_setMsg(mln_string_t *type, mln_lang_tcp_t *target, mln_lang_tcp_t *from)
{
    mln_lang_ctx_t *ctx = target->ctx;
    mln_rbtree_node_t *rn;
    mln_lang_msg_t *lm, tmp;
    mln_lang_array_t *inarr, *tmparr;
    mln_lang_val_t val;
    mln_lang_var_t var, *arr_val;
    mln_string_t stype = mln_string("type");
    mln_string_t sdata = mln_string("data");
    mln_string_t sfrom = mln_string("from");
    mln_string_t sip = mln_string("ip");
    mln_string_t sport = mln_string("port");
    mln_string_t sfd = mln_string("fd");

    if ((inarr = __mln_lang_array_new(ctx)) == NULL) {
        mln_log(error, "No memory.\n");
        exit(1);
    }
    /*type*/
    var.type = M_LANG_VAR_NORMAL;
    var.name = NULL;
    var.val = &val;
    var.inSet = NULL;
    var.prev = var.next = NULL;
    val.data.s = &stype;
    val.type = M_LANG_VAL_TYPE_STRING;
    val.ref = 1;
    if ((arr_val = __mln_lang_array_getAndNew(ctx, inarr, &var)) == NULL) {
        mln_log(error, "No memory.\n");
        exit(1);
    }
    val.data.s = type;
    if (__mln_lang_var_setValue(ctx, arr_val, &var) < 0) {
        mln_log(error, "No memory.\n");
        exit(1);
    }

    /*data*/
    val.data.s = &sdata;
    if ((arr_val = __mln_lang_array_getAndNew(ctx, inarr, &var)) == NULL) {
        mln_log(error, "No memory.\n");
        exit(1);
    }
    if ((tmparr = __mln_lang_array_new(ctx)) == NULL) {
        mln_log(error, "No memory.\n");
        exit(1);
    }
    val.data.array = tmparr;
    val.type = M_LANG_VAL_TYPE_ARRAY;
    if (__mln_lang_var_setValue(ctx, arr_val, &var) < 0) {
        mln_log(error, "No memory.\n");
        exit(1);
    }
    /*data -- ip*/
    val.data.s = &sip;
    val.type = M_LANG_VAL_TYPE_STRING;
    if ((arr_val = __mln_lang_array_getAndNew(ctx, tmparr, &var)) == NULL) {
        mln_log(error, "No memory.\n");
        exit(1);
    }
    val.data.s = target->ip;
    if (__mln_lang_var_setValue(ctx, arr_val, &var) < 0) {
        mln_log(error, "No memory.\n");
        exit(1);
    }
    /*data -- port*/
    val.data.s = &sport;
    if ((arr_val = __mln_lang_array_getAndNew(ctx, tmparr, &var)) == NULL) {
        mln_log(error, "No memory.\n");
        exit(1);
    }
    val.data.i = target->port;
    val.type = M_LANG_VAL_TYPE_INT;
    if (__mln_lang_var_setValue(ctx, arr_val, &var) < 0) {
        mln_log(error, "No memory.\n");
        exit(1);
    }
    /*data -- fd*/
    val.data.s = &sfd;
    val.type = M_LANG_VAL_TYPE_STRING;
    if ((arr_val = __mln_lang_array_getAndNew(ctx, tmparr, &var)) == NULL) {
        mln_log(error, "No memory.\n");
        exit(1);
    }
    val.data.i = mln_tcp_conn_get_fd(&(target->connection));
    val.type = M_LANG_VAL_TYPE_INT;
    if (__mln_lang_var_setValue(ctx, arr_val, &var) < 0) {
        mln_log(error, "No memory.\n");
        exit(1);
    }

    /*from*/
    if (from != NULL) {
        val.data.s = &sfrom;
        val.type = M_LANG_VAL_TYPE_STRING;
        if ((arr_val = __mln_lang_array_getAndNew(ctx, inarr, &var)) == NULL) {
            mln_log(error, "No memory.\n");
            exit(1);
        }
        if ((tmparr = __mln_lang_array_new(ctx)) == NULL) {
            mln_log(error, "No memory.\n");
            exit(1);
        }
        val.data.array = tmparr;
        val.type = M_LANG_VAL_TYPE_ARRAY;
        if (__mln_lang_var_setValue(ctx, arr_val, &var) < 0) {
            mln_log(error, "No memory.\n");
            exit(1);
        }
        /*from -- ip*/
        val.data.s = &sip;
        val.type = M_LANG_VAL_TYPE_STRING;
        if ((arr_val = __mln_lang_array_getAndNew(ctx, tmparr, &var)) == NULL) {
            mln_log(error, "No memory.\n");
            exit(1);
        }
        val.data.s = from->ip;
        if (__mln_lang_var_setValue(ctx, arr_val, &var) < 0) {
            mln_log(error, "No memory.\n");
            exit(1);
        }
        /*from -- port*/
        val.data.s = &sport;
        if ((arr_val = __mln_lang_array_getAndNew(ctx, tmparr, &var)) == NULL) {
            mln_log(error, "No memory.\n");
            exit(1);
        }
        val.data.i = from->port;
        val.type = M_LANG_VAL_TYPE_INT;
        if (__mln_lang_var_setValue(ctx, arr_val, &var) < 0) {
            mln_log(error, "No memory.\n");
            exit(1);
        }
        /*from -- fd*/
        val.data.s = &sfd;
        val.type = M_LANG_VAL_TYPE_STRING;
        if ((arr_val = __mln_lang_array_getAndNew(ctx, tmparr, &var)) == NULL) {
            mln_log(error, "No memory.\n");
            exit(1);
        }
        val.data.i = mln_tcp_conn_get_fd(&(from->connection));
        val.type = M_LANG_VAL_TYPE_INT;
        if (__mln_lang_var_setValue(ctx, arr_val, &var) < 0) {
            mln_log(error, "No memory.\n");
            exit(1);
        }
    }

    /*msg*/
    tmp.name = target->msgName;
    rn = mln_rbtree_search(ctx->msg_map, ctx->msg_map->root, &tmp);
    if (mln_rbtree_null(rn, ctx->msg_map)) {
        mln_log(error, "No such message name <%S>.\n", target->msgName);
        exit(1);
    }
    lm = (mln_lang_msg_t *)(rn->data);
    if (!lm->script_read && lm->script_val != NULL && lm->script_val->type == M_LANG_VAL_TYPE_ARRAY) {
        /*check existence*/
        struct mln_lang_tcp_msg_udata tmu;
        tmu.type = type;
        tmu.target = target;
        if (mln_rbtree_scan_all(lm->script_val->data.array->elems_index, mln_lang_tcp_msgExisted_scan, &tmu) < 0) {
            __mln_lang_array_free(inarr);
            return;
        }
        if ((arr_val = __mln_lang_array_getAndNew(ctx, lm->script_val->data.array, NULL)) == NULL) {
            mln_log(error, "No memory.\n");
            exit(1);
        }
        val.data.array = inarr;
        val.type = M_LANG_VAL_TYPE_ARRAY;
        if (__mln_lang_var_setValue(ctx, arr_val, &var) < 0) {
            mln_log(error, "No memory.\n");
            exit(1);
        }
    } else {
        mln_lang_val_t *newval;
        if ((tmparr = mln_lang_array_new(ctx)) == NULL) {
            mln_log(error, "No memory.\n");
            exit(1);
        }
        if ((arr_val = __mln_lang_array_getAndNew(ctx, tmparr, NULL)) == NULL) {
            mln_log(error, "No memory.\n");
            exit(1);
        }
        val.data.array = inarr;
        val.type = M_LANG_VAL_TYPE_ARRAY;
        if (__mln_lang_var_setValue(ctx, arr_val, &var) < 0) {
            mln_log(error, "No memory.\n");
            exit(1);
        }
        if ((newval = __mln_lang_val_new(ctx->pool, M_LANG_VAL_TYPE_ARRAY, tmparr)) == NULL) {
            mln_log(error, "No memory.\n");
            exit(1);
        }
        if (mln_lang_msg_sendMsg(ctx, target->msgName, newval, 1) < 0) {
            mln_log(error, "Send message from C level failed.\n");
            exit(1);
        }
    }
}

static int mln_lang_tcp_msgExisted_scan(mln_rbtree_node_t *node, void *rn_data, void *udata)
{
    struct mln_lang_tcp_msg_udata *tmu = (struct mln_lang_tcp_msg_udata *)udata;
    mln_lang_array_elem_t *elem = (mln_lang_array_elem_t *)rn_data;
    if (elem->value->val->type != M_LANG_VAL_TYPE_ARRAY) {
        return 0;
    }
    return __mln_lang_tcp_msgExisted(elem->value->val->data.array, tmu->type, tmu->target);
}

static inline int __mln_lang_tcp_msgExisted(mln_lang_array_t *arr, mln_string_t *type, mln_lang_tcp_t *target)
{
    mln_lang_ctx_t *ctx = target->ctx;
    mln_lang_val_t val;
    mln_lang_var_t var, *arr_val;
    mln_string_t stype = mln_string("type");
    mln_string_t sdata = mln_string("data");
    mln_string_t sip = mln_string("ip");
    mln_string_t sport = mln_string("port");
    mln_string_t sfd = mln_string("fd");

    var.type = M_LANG_VAR_NORMAL;
    var.name = NULL;
    var.val = &val;
    var.inSet = NULL;
    var.prev = var.next = NULL;
    val.data.s = &stype;
    val.type = M_LANG_VAL_TYPE_STRING;
    val.ref = 1;
    if ((arr_val = __mln_lang_array_getAndNew(ctx, arr, &var)) == NULL) {
        return 0;
    }
    if (arr_val->val->type != M_LANG_VAL_TYPE_STRING || mln_string_strcmp(type, arr_val->val->data.s)) {
        return 0;
    }

    val.data.s = &sdata;
    if ((arr_val = __mln_lang_array_getAndNew(ctx, arr, &var)) == NULL) {
        return 0;
    }
    if (arr_val->val->type != M_LANG_VAL_TYPE_ARRAY) {
        return 0;
    }
    arr = arr_val->val->data.array;

    val.data.s = &sip;
    if ((arr_val = __mln_lang_array_getAndNew(ctx, arr, &var)) == NULL) {
        return 0;
    }
    if (arr_val->val->type != M_LANG_VAL_TYPE_STRING || mln_string_strcmp(target->ip, arr_val->val->data.s)) {
        return 0;
    }

    val.data.s = &sport;
    if ((arr_val = __mln_lang_array_getAndNew(ctx, arr, &var)) == NULL) {
        return 0;
    }
    if (arr_val->val->type != M_LANG_VAL_TYPE_INT || target->port != arr_val->val->data.i) {
        return 0;
    }

    val.data.s = &sfd;
    if ((arr_val = __mln_lang_array_getAndNew(ctx, arr, &var)) == NULL) {
        return 0;
    }
    if (arr_val->val->type != M_LANG_VAL_TYPE_INT || mln_tcp_conn_get_fd(&(target->connection)) != arr_val->val->data.i) {
        return 0;
    }
    return -1;
}

static void mln_lang_tcp_recv_handler(mln_event_t *ev, int fd, void *data)
{
    int rc;
    mln_lang_tcp_t *tcp = (mln_lang_tcp_t *)data;
    mln_tcp_conn_t *connection = &(tcp->connection);
    mln_string_t errtype = mln_string("tcp_error");
    mln_string_t closetype = mln_string("tcp_close");
    mln_string_t succtype = mln_string("tcp_recv");

    rc = mln_tcp_conn_recv(connection, M_C_TYPE_MEMORY);
    if (rc == M_C_ERROR) {
        tcp->readyClosed = 1;
        mln_event_set_fd(ev, fd, M_EV_CLR, M_EV_UNLIMITED, NULL, NULL);
        mln_lang_tcp_setMsg(&errtype, tcp, NULL);
    } else if (rc == M_C_CLOSED) {
        tcp->readyClosed = 1;
        mln_event_set_fd(ev, fd, M_EV_CLR, M_EV_UNLIMITED, NULL, NULL);
        if (mln_tcp_conn_get_head(connection, M_C_RECV) != NULL) {
            mln_lang_tcp_setMsg(&succtype, tcp, NULL);
        }
        mln_lang_tcp_setMsg(&closetype, tcp, NULL);
    } else {
        if (mln_tcp_conn_get_head(connection, M_C_SEND) != NULL) {
            mln_event_set_fd(ev, fd, M_EV_RECV|M_EV_NONBLOCK, M_EV_UNLIMITED, tcp, mln_lang_tcp_recv_handler);
            mln_event_set_fd(ev, fd, M_EV_SEND|M_EV_APPEND|M_EV_ONESHOT|M_EV_NONBLOCK, M_EV_UNLIMITED, tcp, mln_lang_tcp_send_handler);
        } else {
            mln_event_set_fd(ev, fd, M_EV_RECV|M_EV_NONBLOCK, tcp->timeout, tcp, mln_lang_tcp_recv_handler);
            if (tcp->timeout != M_EV_UNLIMITED) {
                mln_event_set_fd_timeout_handler(ev, fd, tcp, mln_lang_tcp_timeout_handler);
            }
        }
        mln_lang_tcp_setMsg(&succtype, tcp, NULL);
    }
}

static int mln_lang_tcp_listen(mln_lang_ctx_t *ctx)
{
    mln_lang_val_t *val;
    mln_lang_var_t *var;
    mln_lang_func_detail_t *func;
    mln_string_t funcname = mln_string("mln_tcpListen");
    mln_string_t v1 = mln_string("host");
    mln_string_t v2 = mln_string("service");
    mln_string_t v3 = mln_string("msgName");
    mln_string_t v4 = mln_string("timeout");
    if ((func = mln_lang_func_detail_new(ctx->pool, M_FUNC_INTERNAL, mln_lang_tcp_listen_process, NULL)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if ((val = __mln_lang_val_new(ctx->pool, M_LANG_VAL_TYPE_NIL, NULL)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        __mln_lang_func_detail_free(func);
        return -1;
    }
    if ((var = __mln_lang_var_new(ctx->pool, &v1, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        __mln_lang_val_free(val);
        __mln_lang_func_detail_free(func);
        return -1;
    }
    mln_lang_var_chain_add(&(func->args_head), &(func->args_tail), var);
    func->nargs = 1;
    if ((val = __mln_lang_val_new(ctx->pool, M_LANG_VAL_TYPE_NIL, NULL)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        __mln_lang_func_detail_free(func);
        return -1;
    }
    if ((var = __mln_lang_var_new(ctx->pool, &v2, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        __mln_lang_val_free(val);
        __mln_lang_func_detail_free(func);
        return -1;
    }
    mln_lang_var_chain_add(&(func->args_head), &(func->args_tail), var);
    func->nargs = 2;
    if ((val = __mln_lang_val_new(ctx->pool, M_LANG_VAL_TYPE_NIL, NULL)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        __mln_lang_func_detail_free(func);
        return -1;
    }
    if ((var = __mln_lang_var_new(ctx->pool, &v3, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        __mln_lang_val_free(val);
        __mln_lang_func_detail_free(func);
        return -1;
    }
    mln_lang_var_chain_add(&(func->args_head), &(func->args_tail), var);
    func->nargs = 3;
    if ((val = __mln_lang_val_new(ctx->pool, M_LANG_VAL_TYPE_NIL, NULL)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        __mln_lang_func_detail_free(func);
        return -1;
    }
    if ((var = __mln_lang_var_new(ctx->pool, &v4, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        __mln_lang_val_free(val);
        __mln_lang_func_detail_free(func);
        return -1;
    }
    mln_lang_var_chain_add(&(func->args_head), &(func->args_tail), var);
    func->nargs = 4;
    if ((val = __mln_lang_val_new(ctx->pool, M_LANG_VAL_TYPE_FUNC, func)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        __mln_lang_func_detail_free(func);
        return -1;
    }
    if ((var = __mln_lang_var_new(ctx->pool, &funcname, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        __mln_lang_val_free(val);
        return -1;
    }
    if (__mln_lang_symbolNode_join(ctx, M_LANG_SYMBOL_VAR, var) < 0) {
        __mln_lang_errmsg(ctx, "No memory.");
        __mln_lang_var_free(var);
        return -1;
    }
    return 0;
}

static mln_lang_retExp_t *mln_lang_tcp_listen_process(mln_lang_ctx_t *ctx)
{
    int fd, opt = 1;
    mln_u16_t port = 0;
    mln_s32_t timeout;
    mln_lang_val_t *val;
    mln_lang_tcp_t *tcp;
    struct addrinfo addr, *res = NULL;
    mln_string_t *msgName;
    mln_rbtree_node_t *rn;
    mln_lang_retExp_t *retExp;
    mln_lang_symbolNode_t *sym;
    mln_string_t v1 = mln_string("host");
    mln_string_t v2 = mln_string("service");
    mln_string_t v3 = mln_string("msgName");
    mln_string_t v4 = mln_string("timeout");
    char ip[128] = {0}, host[128] = {0}, service[64] = {0};

    if ((sym = __mln_lang_symbolNode_search(ctx, &v1, 1)) == NULL) {
        ASSERT(0);
        __mln_lang_errmsg(ctx, "Argument missing.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);
    if (mln_lang_var_getValType(sym->data.var) != M_LANG_VAL_TYPE_STRING) {
        __mln_lang_errmsg(ctx, "Invalid type of argument 1.");
        return NULL;
    }
    val = sym->data.var->val;
    if (val->data.s->len > sizeof(host)-1) {
        __mln_lang_errmsg(ctx, "Invalid host.");
        return NULL;
    }
    memcpy(host, val->data.s->data, val->data.s->len);

    if ((sym = __mln_lang_symbolNode_search(ctx, &v2, 1)) == NULL) {
        ASSERT(0);
        __mln_lang_errmsg(ctx, "Argument missing.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);
    if (mln_lang_var_getValType(sym->data.var) != M_LANG_VAL_TYPE_STRING) {
        __mln_lang_errmsg(ctx, "Invalid type of argument 2.");
        return NULL;
    }
    val = sym->data.var->val;
    if (val->data.s->len > sizeof(service)-1) {
        __mln_lang_errmsg(ctx, "Invalid service.");
        return NULL;
    }
    memcpy(service, val->data.s->data, val->data.s->len);

    if ((sym = __mln_lang_symbolNode_search(ctx, &v3, 1)) == NULL) {
        ASSERT(0);
        __mln_lang_errmsg(ctx, "Argument missing.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);
    if (mln_lang_var_getValType(sym->data.var) != M_LANG_VAL_TYPE_STRING) {
        __mln_lang_errmsg(ctx, "Invalid type of argument 3.");
        return NULL;
    }
    val = sym->data.var->val;
    msgName = val->data.s;

    if ((sym = __mln_lang_symbolNode_search(ctx, &v4, 1)) == NULL) {
        ASSERT(0);
        __mln_lang_errmsg(ctx, "Argument missing.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);
    if (mln_lang_var_getValType(sym->data.var) == M_LANG_VAL_TYPE_NIL) {
        timeout = M_EV_UNLIMITED;
    } else if (mln_lang_var_getValType(sym->data.var) == M_LANG_VAL_TYPE_INT) {
        val = sym->data.var->val;
        timeout = val->data.i;
    } else {
        __mln_lang_errmsg(ctx, "Invalid type of argument 4.");
        return NULL;
    }

    addr.ai_flags = AI_PASSIVE;
    addr.ai_family = AF_UNSPEC;
    addr.ai_socktype = SOCK_STREAM;
    addr.ai_protocol = IPPROTO_IP;
    if (getaddrinfo(host, service, &addr, &res) != 0 || res == NULL) {
        if ((retExp = mln_lang_retExp_createTmpFalse(ctx->pool, NULL)) == NULL) {
            __mln_lang_errmsg(ctx, "No memory.");
            return NULL;
        }
        return retExp;
    }

    if ((fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) < 0) {
        freeaddrinfo(res);
        if ((retExp = mln_lang_retExp_createTmpFalse(ctx->pool, NULL)) == NULL) {
            __mln_lang_errmsg(ctx, "No memory.");
            return NULL;
        }
        return retExp;
    }
    if (mln_lang_tcp_getinfo(res->ai_addr, ip, &port) < 0) {
        freeaddrinfo(res);
        if ((retExp = mln_lang_retExp_createTmpFalse(ctx->pool, NULL)) == NULL) {
            __mln_lang_errmsg(ctx, "No memory.");
            return NULL;
        }
        return retExp;
    }
    if ((tcp = mln_lang_tcp_new(ctx, fd, ip, port, msgName, timeout)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        freeaddrinfo(res);
        close(fd);
        return NULL;
    }
    if ((rn = mln_rbtree_node_new(ctx->tcp_set, tcp)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        freeaddrinfo(res);
        mln_lang_tcp_free(tcp);
        return NULL;
    }
    mln_rbtree_insert(ctx->tcp_set, rn);

    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        freeaddrinfo(res);
        mln_rbtree_delete(ctx->tcp_set, rn);
        mln_rbtree_node_free(ctx->tcp_set, rn);
        if ((retExp = mln_lang_retExp_createTmpFalse(ctx->pool, NULL)) == NULL) {
            __mln_lang_errmsg(ctx, "No memory.");
            return NULL;
        }
        return retExp;
    }
    if (bind(fd, res->ai_addr, res->ai_addrlen) < 0) {
        freeaddrinfo(res);
        mln_rbtree_delete(ctx->tcp_set, rn);
        mln_rbtree_node_free(ctx->tcp_set, rn);
        if ((retExp = mln_lang_retExp_createTmpFalse(ctx->pool, NULL)) == NULL) {
            __mln_lang_errmsg(ctx, "No memory.");
            return NULL;
        }
        return retExp;
    }
    freeaddrinfo(res);
    if (listen(fd, 32767) < 0) {
        mln_rbtree_delete(ctx->tcp_set, rn);
        mln_rbtree_node_free(ctx->tcp_set, rn);
        if ((retExp = mln_lang_retExp_createTmpFalse(ctx->pool, NULL)) == NULL) {
            __mln_lang_errmsg(ctx, "No memory.");
            return NULL;
        }
        return retExp;
    }
    if (mln_event_set_fd(ctx->lang->ev, \
                         fd, \
                         M_EV_RECV|M_EV_NONBLOCK|M_EV_ONESHOT, \
                         M_EV_UNLIMITED, \
                         tcp, 
                         mln_lang_tcp_accept_handler) < 0)
    {
        __mln_lang_errmsg(ctx, "No memory.");
        mln_rbtree_delete(ctx->tcp_set, rn);
        mln_rbtree_node_free(ctx->tcp_set, rn);
        return NULL;
    }

    if ((retExp = mln_lang_retExp_createTmpTrue(ctx->pool, NULL)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        mln_event_set_fd(ctx->lang->ev, fd, M_EV_CLR, M_EV_UNLIMITED, NULL, NULL);
        mln_rbtree_delete(ctx->tcp_set, rn);
        mln_rbtree_node_free(ctx->tcp_set, rn);
        return NULL;
    }
    return retExp;
}

static void mln_lang_tcp_timeout_handler(mln_event_t *ev, int fd, void *data)
{
    mln_string_t type = mln_string("tcp_timeout");
    mln_lang_tcp_t *tcp = (mln_lang_tcp_t *)data;
    tcp->readyClosed = 1;
    mln_event_set_fd(ev, fd, M_EV_CLR, M_EV_UNLIMITED, NULL, NULL);
    mln_lang_tcp_setMsg(&type, tcp, NULL);
}

static void mln_lang_tcp_accept_handler(mln_event_t *ev, int fd, void *data)
{
    char ip[128];
    int connfd, again = 0;
    mln_u16_t port;
    struct sockaddr addr;
    socklen_t len;
    mln_lang_tcp_t *tcp, *tmp;
    mln_rbtree_node_t *rn;
    mln_string_t errtype = mln_string("tcp_error");
    mln_string_t succtype = mln_string("tcp_accept");

    tcp = (mln_lang_tcp_t *)data;
    while (1) {
        memset(&addr, 0, sizeof(addr));
        memset(ip, 0, sizeof(ip));
        len = sizeof(addr);
        if ((connfd = accept(fd, &addr, &len)) < 0) {
            if (errno == EINTR || errno == ECONNABORTED) continue;
            if (errno == EAGAIN) {
                again = 1;
                break;
            }
            if (errno == EMFILE || errno == ENFILE) break;
            tcp->readyClosed = 1;
            mln_event_set_fd(ev, fd, M_EV_CLR, M_EV_UNLIMITED, NULL, NULL);
            mln_lang_tcp_setMsg(&errtype, tcp, NULL);
            return;
        }
        if (mln_lang_tcp_getinfo(&addr, ip, &port) < 0) {
            close(connfd);
            break;
        }
        if ((tmp = mln_lang_tcp_new(tcp->ctx, connfd, ip, port, tcp->msgName, tcp->timeout)) == NULL) {
            close(connfd);
            break;
        }
        if ((rn = mln_rbtree_node_new(tcp->ctx->tcp_set, tmp)) == NULL) {
            mln_lang_tcp_free(tmp);
            break;
        }
        mln_rbtree_insert(tcp->ctx->tcp_set, rn);
        if (mln_event_set_fd(ev, \
                             connfd, \
                             M_EV_RECV|M_EV_NONBLOCK, \
                             tmp->timeout, \
                             tmp, \
                             mln_lang_tcp_recv_handler) < 0)
        {
            mln_rbtree_delete(tcp->ctx->tcp_set, rn);
            mln_rbtree_node_free(tcp->ctx->tcp_set, rn);
            break;
        }
        if (tmp->timeout != M_EV_UNLIMITED) {
            mln_event_set_fd_timeout_handler(ev, connfd, tmp, mln_lang_tcp_timeout_handler);
        }
        mln_lang_tcp_setMsg(&succtype, tmp, tcp);
    }
    if (again) {
        mln_event_set_fd(ev, fd, M_EV_RECV|M_EV_NONBLOCK|M_EV_ONESHOT, M_EV_UNLIMITED, data, mln_lang_tcp_accept_handler);
    } else {
        if (mln_event_set_timer(ev, 100, data, mln_lang_tcp_accept_timer_handler) < 0) {
            mln_event_set_fd(ev, fd, M_EV_RECV|M_EV_NONBLOCK|M_EV_ONESHOT, M_EV_UNLIMITED, data, mln_lang_tcp_accept_handler);
        }
    }
}

static void mln_lang_tcp_accept_timer_handler(mln_event_t *ev, void *data)
{
    mln_lang_tcp_t *tcp = (mln_lang_tcp_t *)data;
    int fd = mln_tcp_conn_get_fd(&(tcp->connection));
    mln_event_set_fd(ev, fd, M_EV_RECV|M_EV_NONBLOCK|M_EV_ONESHOT, M_EV_UNLIMITED, data, mln_lang_tcp_accept_handler);
}

static int mln_lang_tcp_send(mln_lang_ctx_t *ctx)
{
    mln_lang_val_t *val;
    mln_lang_var_t *var;
    mln_lang_func_detail_t *func;
    mln_string_t funcname = mln_string("mln_tcpSend");
    mln_string_t v1 = mln_string("sockfd");
    mln_string_t v2 = mln_string("buf");
    if ((func = mln_lang_func_detail_new(ctx->pool, M_FUNC_INTERNAL, mln_lang_tcp_send_process, NULL)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if ((val = __mln_lang_val_new(ctx->pool, M_LANG_VAL_TYPE_NIL, NULL)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        __mln_lang_func_detail_free(func);
        return -1;
    }
    if ((var = __mln_lang_var_new(ctx->pool, &v1, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        __mln_lang_val_free(val);
        __mln_lang_func_detail_free(func);
        return -1;
    }
    mln_lang_var_chain_add(&(func->args_head), &(func->args_tail), var);
    func->nargs = 1;
    if ((val = __mln_lang_val_new(ctx->pool, M_LANG_VAL_TYPE_NIL, NULL)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        __mln_lang_func_detail_free(func);
        return -1;
    }
    if ((var = __mln_lang_var_new(ctx->pool, &v2, M_LANG_VAR_REFER, val, NULL)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        __mln_lang_val_free(val);
        __mln_lang_func_detail_free(func);
        return -1;
    }
    mln_lang_var_chain_add(&(func->args_head), &(func->args_tail), var);
    func->nargs = 2;
    if ((val = __mln_lang_val_new(ctx->pool, M_LANG_VAL_TYPE_FUNC, func)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        __mln_lang_func_detail_free(func);
        return -1;
    }
    if ((var = __mln_lang_var_new(ctx->pool, &funcname, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        __mln_lang_val_free(val);
        return -1;
    }
    if (__mln_lang_symbolNode_join(ctx, M_LANG_SYMBOL_VAR, var) < 0) {
        __mln_lang_errmsg(ctx, "No memory.");
        __mln_lang_var_free(var);
        return -1;
    }
    return 0;
}

static mln_lang_retExp_t *mln_lang_tcp_send_process(mln_lang_ctx_t *ctx)
{
    int fd;
    mln_buf_t *b;
    mln_chain_t *c;
    mln_u8ptr_t buf;
    mln_lang_val_t *val;
    mln_rbtree_node_t *rn;
    mln_lang_tcp_t tmp, *tcp;
    mln_lang_retExp_t *retExp;
    mln_lang_symbolNode_t *sym;
    mln_string_t v1 = mln_string("sockfd");
    mln_string_t v2 = mln_string("buf");

    if ((sym = __mln_lang_symbolNode_search(ctx, &v1, 1)) == NULL) {
        ASSERT(0);
        __mln_lang_errmsg(ctx, "Argument missing.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);
    if (mln_lang_var_getValType(sym->data.var) != M_LANG_VAL_TYPE_INT) {
        __mln_lang_errmsg(ctx, "Invalid type of argument 1.");
        return NULL;
    }
    val = sym->data.var->val;
    fd = val->data.i;
    mln_tcp_conn_set_fd(&(tmp.connection), fd);
    rn = mln_rbtree_search(ctx->tcp_set, ctx->tcp_set->root, &tmp);
    if (mln_rbtree_null(rn, ctx->tcp_set)) {
        if ((retExp = mln_lang_retExp_createTmpFalse(ctx->pool, NULL)) == NULL) {
            __mln_lang_errmsg(ctx, "No memory.");
            return NULL;
        }
    } else {
        tcp = (mln_lang_tcp_t *)(rn->data);

        if ((sym = __mln_lang_symbolNode_search(ctx, &v2, 1)) == NULL) {
            ASSERT(0);
            __mln_lang_errmsg(ctx, "Argument missing.");
            return NULL;
        }
        ASSERT(sym->type == M_LANG_SYMBOL_VAR);
        if (mln_lang_var_getValType(sym->data.var) != M_LANG_VAL_TYPE_STRING) {
            __mln_lang_errmsg(ctx, "Invalid type of argument 2.");
            return NULL;
        }
        val = sym->data.var->val;

        if (!tcp->send || tcp->readyClosed) {
            if ((retExp = mln_lang_retExp_createTmpFalse(ctx->pool, NULL)) == NULL) {
                __mln_lang_errmsg(ctx, "No memory.");
                return NULL;
            }
        } else {
            if (val->data.s->len) {
                if ((c = mln_chain_new(mln_tcp_conn_get_pool(&(tcp->connection)))) == NULL) {
                    __mln_lang_errmsg(ctx, "No memory.");
                    return NULL;
                }
                if ((b = mln_buf_new(mln_tcp_conn_get_pool(&(tcp->connection)))) == NULL) {
                    __mln_lang_errmsg(ctx, "No memory.");
                    mln_chain_pool_release(c);
                    return NULL;
                }
                c->buf = b;
                if ((buf = (mln_u8ptr_t)mln_alloc_m(mln_tcp_conn_get_pool(&(tcp->connection)), val->data.s->len)) == NULL) {
                    __mln_lang_errmsg(ctx, "No memory.");
                    mln_chain_pool_release(c);
                    return NULL;
                }
                memcpy(buf, val->data.s->data, val->data.s->len);
                b->left_pos = b->pos = b->start = buf;
                b->last = b->end = buf + val->data.s->len;
                b->in_memory = 1;
                b->last_buf = 1;
                mln_tcp_conn_append(&(tcp->connection), c, M_C_SEND);
                if (mln_event_set_fd(ctx->lang->ev, \
                                     fd, \
                                     M_EV_SEND|M_EV_APPEND|M_EV_ONESHOT|M_EV_NONBLOCK, \
                                     M_EV_UNLIMITED, \
                                     tcp, \
                                     mln_lang_tcp_send_handler) < 0)
                {
                    __mln_lang_errmsg(ctx, "No memory.");
                    return NULL;
                }
            }
            if ((retExp = mln_lang_retExp_createTmpTrue(ctx->pool, NULL)) == NULL) {
                __mln_lang_errmsg(ctx, "No memory.");
                return NULL;
            }
        }
    }
    return retExp;
}

static void mln_lang_tcp_send_handler(mln_event_t *ev, int fd, void *data)
{
    mln_lang_tcp_t *tcp = (mln_lang_tcp_t *)data;
    mln_tcp_conn_t *connection = &(tcp->connection);
    mln_string_t errtype = mln_string("tcp_error");
    mln_string_t closetype = mln_string("tcp_close");
    mln_rbtree_node_t *rn;
    int rc;

    rc = mln_tcp_conn_send(connection);
    if (rc == M_C_FINISH || rc == M_C_NOTYET) {
        mln_chain_pool_release_all(mln_tcp_conn_remove(connection, M_C_SENT));
        if (mln_tcp_conn_get_head(connection, M_C_SEND) != NULL) {
            mln_event_set_fd(ev, fd, M_EV_SEND|M_EV_APPEND|M_EV_ONESHOT|M_EV_NONBLOCK, M_EV_UNLIMITED, tcp, mln_lang_tcp_send_handler);
        } else {
            if (tcp->readyClosed) {
                mln_event_set_fd(ev, fd, M_EV_CLR, M_EV_UNLIMITED, NULL, NULL);
                rn = mln_rbtree_search(tcp->ctx->tcp_set, tcp->ctx->tcp_set->root, tcp);
                ASSERT(!mln_rbtree_null(rn, tcp->ctx->tcp_set));
                mln_rbtree_delete(tcp->ctx->tcp_set, rn);
                mln_rbtree_node_free(tcp->ctx->tcp_set, rn);
                return;
            }
            mln_event_set_fd(ev, fd, M_EV_RECV|M_EV_NONBLOCK, tcp->timeout, tcp, mln_lang_tcp_recv_handler);
            if (tcp->timeout != M_EV_UNLIMITED) {
                mln_event_set_fd_timeout_handler(ev, fd, tcp, mln_lang_tcp_timeout_handler);
            }
        }
    } else if (rc == M_C_ERROR) {
        if (tcp->readyClosed) {
            mln_event_set_fd(ev, fd, M_EV_CLR, M_EV_UNLIMITED, NULL, NULL);
            rn = mln_rbtree_search(tcp->ctx->tcp_set, tcp->ctx->tcp_set->root, tcp);
            ASSERT(!mln_rbtree_null(rn, tcp->ctx->tcp_set));
            mln_rbtree_delete(tcp->ctx->tcp_set, rn);
            mln_rbtree_node_free(tcp->ctx->tcp_set, rn);
            return;
        }
        tcp->readyClosed = 1;
        mln_event_set_fd(ev, fd, M_EV_CLR, M_EV_UNLIMITED, NULL, NULL);
        mln_lang_tcp_setMsg(&errtype, tcp, NULL);
    } else {
        if (tcp->readyClosed) {
            mln_event_set_fd(ev, fd, M_EV_CLR, M_EV_UNLIMITED, NULL, NULL);
            rn = mln_rbtree_search(tcp->ctx->tcp_set, tcp->ctx->tcp_set->root, tcp);
            ASSERT(!mln_rbtree_null(rn, tcp->ctx->tcp_set));
            mln_rbtree_delete(tcp->ctx->tcp_set, rn);
            mln_rbtree_node_free(tcp->ctx->tcp_set, rn);
            return;
        }
        tcp->readyClosed = 1;
        mln_event_set_fd(ev, fd, M_EV_CLR, M_EV_UNLIMITED, NULL, NULL);
        mln_lang_tcp_setMsg(&closetype, tcp, NULL);
    }
}

static int mln_lang_tcp_recv(mln_lang_ctx_t *ctx)
{
    mln_lang_val_t *val;
    mln_lang_var_t *var;
    mln_lang_func_detail_t *func;
    mln_string_t funcname = mln_string("mln_tcpRecv");
    mln_string_t v1 = mln_string("sockfd");
    if ((func = mln_lang_func_detail_new(ctx->pool, M_FUNC_INTERNAL, mln_lang_tcp_recv_process, NULL)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if ((val = __mln_lang_val_new(ctx->pool, M_LANG_VAL_TYPE_NIL, NULL)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        __mln_lang_func_detail_free(func);
        return -1;
    }
    if ((var = __mln_lang_var_new(ctx->pool, &v1, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        __mln_lang_val_free(val);
        __mln_lang_func_detail_free(func);
        return -1;
    }
    mln_lang_var_chain_add(&(func->args_head), &(func->args_tail), var);
    func->nargs = 1;
    if ((val = __mln_lang_val_new(ctx->pool, M_LANG_VAL_TYPE_FUNC, func)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        __mln_lang_func_detail_free(func);
        return -1;
    }
    if ((var = __mln_lang_var_new(ctx->pool, &funcname, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        __mln_lang_val_free(val);
        return -1;
    }
    if (__mln_lang_symbolNode_join(ctx, M_LANG_SYMBOL_VAR, var) < 0) {
        __mln_lang_errmsg(ctx, "No memory.");
        __mln_lang_var_free(var);
        return -1;
    }
    return 0;
}

static mln_lang_retExp_t *mln_lang_tcp_recv_process(mln_lang_ctx_t *ctx)
{
    mln_lang_val_t *val;
    mln_lang_retExp_t *retExp;
    mln_string_t v1 = mln_string("sockfd");
    mln_lang_symbolNode_t *sym;
    mln_lang_tcp_t tmp, *tcp;
    mln_rbtree_node_t *rn;

    if ((sym = __mln_lang_symbolNode_search(ctx, &v1, 1)) == NULL) {
        ASSERT(0);
        __mln_lang_errmsg(ctx, "Argument missing.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);
    if (mln_lang_var_getValType(sym->data.var) != M_LANG_VAL_TYPE_INT) {
        __mln_lang_errmsg(ctx, "Invalid type of argument 1.");
        return NULL;
    }
    val = sym->data.var->val;
    mln_tcp_conn_set_fd(&(tmp.connection), val->data.i);
    rn = mln_rbtree_search(ctx->tcp_set, ctx->tcp_set->root, &tmp);
    if (mln_rbtree_null(rn, ctx->tcp_set)) {
        if ((retExp = mln_lang_retExp_createTmpFalse(ctx->pool, NULL)) == NULL) {
            __mln_lang_errmsg(ctx, "No memory.");
            return NULL;
        }
    } else {
        mln_size_t len = 0;
        tcp = (mln_lang_tcp_t *)(rn->data);
        mln_chain_t *c = mln_tcp_conn_get_head(&(tcp->connection), M_C_RECV);
        for (; c != NULL; c = c->next) {
            len += mln_buf_size(c->buf);
        }
        if (!len) {
            mln_chain_pool_release_all(mln_tcp_conn_remove(&(tcp->connection), M_C_RECV));
            if (!tcp->recv || tcp->readyClosed) {
                if ((retExp = mln_lang_retExp_createTmpFalse(ctx->pool, NULL)) == NULL) {
                    __mln_lang_errmsg(ctx, "No memory.");
                    return NULL;
                }
            } else {
                if ((retExp = mln_lang_retExp_createTmpNil(ctx->pool, NULL)) == NULL) {
                    __mln_lang_errmsg(ctx, "No memory.");
                    return NULL;
                }
            }
        } else {
            mln_u8ptr_t buf, p;
            mln_size_t l;
            mln_string_t tmp;
            if ((buf = (mln_u8ptr_t)mln_alloc_m(ctx->pool, len)) == NULL) {
                __mln_lang_errmsg(ctx, "No memory.");
                return NULL;
            }
            p = buf;
            for (c = mln_tcp_conn_get_head(&(tcp->connection), M_C_RECV); c != NULL; c = c->next) {
                l = mln_buf_size(c->buf);
                if (!l) continue;
                memcpy(p, c->buf->pos, l);
                p += l;
            }
            mln_string_nSet(&tmp, buf, len);
            if ((retExp = mln_lang_retExp_createTmpString(ctx->pool, &tmp, NULL)) == NULL) {
                __mln_lang_errmsg(ctx, "No memory.");
                mln_alloc_free(buf);
                return NULL;
            }
            mln_alloc_free(buf);
            mln_chain_pool_release_all(mln_tcp_conn_remove(&(tcp->connection), M_C_RECV));
        }
    }
    return retExp;
}

static int mln_lang_tcp_close(mln_lang_ctx_t *ctx)
{
    mln_lang_val_t *val;
    mln_lang_var_t *var;
    mln_lang_func_detail_t *func;
    mln_string_t funcname = mln_string("mln_tcpClose");
    mln_string_t v1 = mln_string("sockfd");
    if ((func = mln_lang_func_detail_new(ctx->pool, M_FUNC_INTERNAL, mln_lang_tcp_close_process, NULL)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if ((val = __mln_lang_val_new(ctx->pool, M_LANG_VAL_TYPE_NIL, NULL)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        __mln_lang_func_detail_free(func);
        return -1;
    }
    if ((var = __mln_lang_var_new(ctx->pool, &v1, M_LANG_VAR_REFER, val, NULL)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        __mln_lang_val_free(val);
        __mln_lang_func_detail_free(func);
        return -1;
    }
    mln_lang_var_chain_add(&(func->args_head), &(func->args_tail), var);
    func->nargs = 1;
    if ((val = __mln_lang_val_new(ctx->pool, M_LANG_VAL_TYPE_FUNC, func)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        __mln_lang_func_detail_free(func);
        return -1;
    }
    if ((var = __mln_lang_var_new(ctx->pool, &funcname, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        __mln_lang_val_free(val);
        return -1;
    }
    if (__mln_lang_symbolNode_join(ctx, M_LANG_SYMBOL_VAR, var) < 0) {
        __mln_lang_errmsg(ctx, "No memory.");
        __mln_lang_var_free(var);
        return -1;
    }
    return 0;
}

static mln_lang_retExp_t *mln_lang_tcp_close_process(mln_lang_ctx_t *ctx)
{
    mln_lang_val_t *val;
    mln_lang_retExp_t *retExp;
    mln_string_t v1 = mln_string("sockfd");
    mln_lang_symbolNode_t *sym;
    mln_lang_tcp_t tmp, *tcp;
    mln_rbtree_node_t *rn;

    if ((sym = __mln_lang_symbolNode_search(ctx, &v1, 1)) == NULL) {
        ASSERT(0);
        __mln_lang_errmsg(ctx, "Argument missing.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);
    if (mln_lang_var_getValType(sym->data.var) != M_LANG_VAL_TYPE_INT) {
        __mln_lang_errmsg(ctx, "Invalid type of argument 1.");
        return NULL;
    }
    val = sym->data.var->val;
    mln_tcp_conn_set_fd(&(tmp.connection), val->data.i);
    rn = mln_rbtree_search(ctx->tcp_set, ctx->tcp_set->root, &tmp);
    if (mln_rbtree_null(rn, ctx->tcp_set)) {
        if ((retExp = mln_lang_retExp_createTmpFalse(ctx->pool, NULL)) == NULL) {
            __mln_lang_errmsg(ctx, "No memory.");
            return NULL;
        }
    } else {
        tcp = (mln_lang_tcp_t *)(rn->data);
        if (mln_tcp_conn_send_empty(&(tcp->connection))) {
            mln_rbtree_delete(ctx->tcp_set, rn);
            mln_rbtree_node_free(ctx->tcp_set, rn);
            val->data.i = -1;
        } else {
            tcp->readyClosed = 1;
        }
        if ((retExp = mln_lang_retExp_createTmpTrue(ctx->pool, NULL)) == NULL) {
            __mln_lang_errmsg(ctx, "No memory.");
            return NULL;
        }
    }
    return retExp;
}

static int mln_lang_tcp_shutdown(mln_lang_ctx_t *ctx)
{
    mln_lang_val_t *val;
    mln_lang_var_t *var;
    mln_lang_func_detail_t *func;
    mln_string_t funcname = mln_string("mln_tcpShutdown");
    mln_string_t v1 = mln_string("sockfd");
    mln_string_t v2 = mln_string("type");
    if ((func = mln_lang_func_detail_new(ctx->pool, M_FUNC_INTERNAL, mln_lang_tcp_shutdown_process, NULL)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if ((val = __mln_lang_val_new(ctx->pool, M_LANG_VAL_TYPE_NIL, NULL)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        __mln_lang_func_detail_free(func);
        return -1;
    }
    if ((var = __mln_lang_var_new(ctx->pool, &v1, M_LANG_VAR_REFER, val, NULL)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        __mln_lang_val_free(val);
        __mln_lang_func_detail_free(func);
        return -1;
    }
    mln_lang_var_chain_add(&(func->args_head), &(func->args_tail), var);
    func->nargs = 1;
    if ((val = __mln_lang_val_new(ctx->pool, M_LANG_VAL_TYPE_NIL, NULL)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        __mln_lang_func_detail_free(func);
        return -1;
    }
    if ((var = __mln_lang_var_new(ctx->pool, &v2, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        __mln_lang_val_free(val);
        __mln_lang_func_detail_free(func);
        return -1;
    }
    mln_lang_var_chain_add(&(func->args_head), &(func->args_tail), var);
    func->nargs = 2;
    if ((val = __mln_lang_val_new(ctx->pool, M_LANG_VAL_TYPE_FUNC, func)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        __mln_lang_func_detail_free(func);
        return -1;
    }
    if ((var = __mln_lang_var_new(ctx->pool, &funcname, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        __mln_lang_errmsg(ctx, "No memory.");
        __mln_lang_val_free(val);
        return -1;
    }
    if (__mln_lang_symbolNode_join(ctx, M_LANG_SYMBOL_VAR, var) < 0) {
        __mln_lang_errmsg(ctx, "No memory.");
        __mln_lang_var_free(var);
        return -1;
    }
    return 0;
}

static mln_lang_retExp_t *mln_lang_tcp_shutdown_process(mln_lang_ctx_t *ctx)
{
    mln_lang_retExp_t *retExp;
    mln_string_t v1 = mln_string("sockfd");
    mln_string_t v2 = mln_string("type");
    mln_lang_symbolNode_t *sym;
    mln_lang_tcp_t tmp, *tcp;
    mln_rbtree_node_t *rn;
    int fd;
    mln_string_t *type;
    mln_lang_val_t *val;
    mln_string_t type_send = mln_string("send");
    mln_string_t type_recv = mln_string("recv");

    if ((sym = __mln_lang_symbolNode_search(ctx, &v1, 1)) == NULL) {
        ASSERT(0);
        __mln_lang_errmsg(ctx, "Argument missing.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);
    if (mln_lang_var_getValType(sym->data.var) != M_LANG_VAL_TYPE_INT) {
        __mln_lang_errmsg(ctx, "Invalid type of argument 1.");
        return NULL;
    }
    val = sym->data.var->val;
    fd = val->data.i;

    if ((sym = __mln_lang_symbolNode_search(ctx, &v2, 1)) == NULL) {
        ASSERT(0);
        __mln_lang_errmsg(ctx, "Argument missing.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);
    if (mln_lang_var_getValType(sym->data.var) != M_LANG_VAL_TYPE_STRING) {
        __mln_lang_errmsg(ctx, "Invalid type of argument 2.");
        return NULL;
    }
    type = sym->data.var->val->data.s;

    mln_tcp_conn_set_fd(&(tmp.connection), fd);
    rn = mln_rbtree_search(ctx->tcp_set, ctx->tcp_set->root, &tmp);
    if (mln_rbtree_null(rn, ctx->tcp_set)) {
        if ((retExp = mln_lang_retExp_createTmpFalse(ctx->pool, NULL)) == NULL) {
            __mln_lang_errmsg(ctx, "No memory.");
            return NULL;
        }
    } else {
        tcp = (mln_lang_tcp_t *)(rn->data);
        if (!mln_string_strcmp(type, &type_send)) {
            if (tcp->send) {
                if (tcp->recv) {
                    tcp->send = 0;
                } else {
                    if (mln_tcp_conn_send_empty(&(tcp->connection))) {
                        mln_rbtree_delete(ctx->tcp_set, rn);
                        mln_rbtree_node_free(ctx->tcp_set, rn);
                        val->data.i = -1;
                    } else {
                        tcp->readyClosed = 1;
                    }
                }
                if ((retExp = mln_lang_retExp_createTmpTrue(ctx->pool, NULL)) == NULL) {
                    __mln_lang_errmsg(ctx, "No memory.");
                    return NULL;
                }
            } else {
                if ((retExp = mln_lang_retExp_createTmpFalse(ctx->pool, NULL)) == NULL) {
                    __mln_lang_errmsg(ctx, "No memory.");
                    return NULL;
                }
            }
        } else if (!mln_string_strcmp(type, &type_recv)) {
            if (tcp->recv) {
                if (tcp->send) {
                    tcp->recv = 0;
                } else {
                    if (mln_tcp_conn_send_empty(&(tcp->connection))) {
                        mln_rbtree_delete(ctx->tcp_set, rn);
                        mln_rbtree_node_free(ctx->tcp_set, rn);
                        val->data.i = -1;
                    } else {
                        tcp->readyClosed = 1;
                    }
                }
                if ((retExp = mln_lang_retExp_createTmpTrue(ctx->pool, NULL)) == NULL) {
                    __mln_lang_errmsg(ctx, "No memory.");
                    return NULL;
                }
            } else {
                if ((retExp = mln_lang_retExp_createTmpFalse(ctx->pool, NULL)) == NULL) {
                    __mln_lang_errmsg(ctx, "No memory.");
                    return NULL;
                }
            }
        } else {
            if ((retExp = mln_lang_retExp_createTmpFalse(ctx->pool, NULL)) == NULL) {
                __mln_lang_errmsg(ctx, "No memory.");
                return NULL;
            }
        }
    }
    return retExp;
}

int mln_lang_resource_register(mln_lang_ctx_t *ctx, char *name, void *data, mln_lang_resource_free free_handler)
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
        mln_lang_resource_free_handler(lr);
        return -1;
    }
    mln_rbtree_insert(ctx->resource_set, rn);
    return 0;
}

static void mln_lang_resource_free_handler(mln_lang_resource_t *lr)
{
    if (lr == NULL) return;
    if (lr->free_handler != NULL) lr->free_handler(lr->data);
    mln_string_pool_free(lr->name);
    mln_alloc_free(lr);
}

static int mln_lang_resource_cmp(const mln_lang_resource_t *lr1, const mln_lang_resource_t *lr2)
{
    return mln_string_strcmp(lr1->name, lr2->name);
}

