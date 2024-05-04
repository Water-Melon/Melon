
/*
 * Copyright (C) Niklaus F.Schen.
 */
#ifndef __MLN_LANG_H
#define __MLN_LANG_H
#include "mln_event.h"
#include "mln_types.h"
#include "mln_conf.h"
#include "mln_connection.h"
#include "mln_file.h"
#include "mln_rbtree.h"
#include "mln_lang_ast.h"
#include "mln_utils.h"
#include "mln_gc.h"
#include "mln_alloc.h"
#include "mln_array.h"

#define M_LANG_ARRAY_PREALLOC      32
#define M_LANG_CACHE_COUNT         65535
#define M_LANG_SYMBOL_TABLE_LEN    371
#define M_LANG_STEP_OUT            -1
#define M_LANG_RUN_STACK_LEN       1024
#define M_LANG_SCOPE_LEN           1024
#define M_LANG_MAX_OPENFILE        67
#define M_LANG_DEFAULT_STEP        1700
#define M_LANG_HEARTBEAT_US        50000
#define MLN_LANG_PIPE_LIST_NALLOC  1024
#define MLN_LANG_PIPE_ELEM_NALLOC  6
#define MLN_LANG_IMPORT_FREE_COUNT 65535
#define MLN_LANG_STM_CACHE_USEC    30000000

#define M_LANG_VAL_TYPE_NIL       0
#define M_LANG_VAL_TYPE_INT       1
#define M_LANG_VAL_TYPE_BOOL      2
#define M_LANG_VAL_TYPE_REAL      3
#define M_LANG_VAL_TYPE_STRING    4
#define M_LANG_VAL_TYPE_OBJECT    5
#define M_LANG_VAL_TYPE_FUNC      6
#define M_LANG_VAL_TYPE_ARRAY     7
#define M_LANG_VAL_TYPE_CALL      8

typedef struct mln_lang_funccall_val_s  mln_lang_funccall_val_t;
typedef struct mln_lang_val_s           mln_lang_val_t;
typedef struct mln_lang_object_s        mln_lang_object_t;
typedef struct mln_lang_func_detail_s   mln_lang_func_detail_t;
typedef struct mln_lang_var_s           mln_lang_var_t;
typedef struct mln_lang_set_detail_s    mln_lang_set_detail_t;
typedef struct mln_lang_symbol_node_s   mln_lang_symbol_node_t;
typedef struct mln_lang_scope_s         mln_lang_scope_t;
typedef struct mln_lang_stack_node_s    mln_lang_stack_node_t;
typedef struct mln_lang_ctx_s           mln_lang_ctx_t;
typedef struct mln_lang_s               mln_lang_t;
typedef struct mln_lang_array_s         mln_lang_array_t;
typedef struct mln_lang_array_elem_s    mln_lang_array_elem_t;
typedef struct mln_lang_methods_s       mln_lang_method_t;
typedef struct mln_lang_resource_s      mln_lang_resource_t;
typedef struct mln_lang_ast_cache_s     mln_lang_ast_cache_t;
typedef struct mln_lang_hash_s          mln_lang_hash_t;
typedef struct mln_lang_hash_bucket_s   mln_lang_hash_bucket_t;
typedef struct mln_lang_ctx_pipe_elem_s mln_lang_ctx_pipe_elem_t;

typedef int (*mln_lang_run_ctl_t)(mln_lang_t *);
typedef void (*mln_lang_stack_handler)(mln_lang_ctx_t *);
typedef int (*mln_lang_op)(mln_lang_ctx_t *, mln_lang_var_t **, mln_lang_var_t *, mln_lang_var_t *);
typedef mln_lang_var_t *(*mln_lang_internal) (mln_lang_ctx_t *);
typedef int (*mln_msg_c_handler)(mln_lang_ctx_t *, const mln_lang_val_t *);
typedef void (*mln_lang_return_handler)(mln_lang_ctx_t *);
typedef void (*mln_lang_resource_free)(void *data);
/* import init */
typedef mln_lang_var_t *(* import_init_t)(mln_lang_ctx_t *, mln_conf_t *cf);
typedef void (* import_destroy_t)(mln_lang_t *);
/* pipe */
typedef int (*mln_lang_ctx_pipe_recv_cb_t)(mln_lang_ctx_t *, mln_lang_val_t *);


struct mln_lang_hash_s {
    mln_lang_hash_bucket_t          *bucket;
    mln_size_t                       len;
};

struct mln_lang_hash_bucket_s {
    mln_lang_symbol_node_t          *head;
    mln_lang_symbol_node_t          *tail;
};

struct mln_lang_ast_cache_s {
    mln_lang_stm_t                  *stm;
    mln_string_t                    *code;
    mln_u64_t                        ref:63;
    mln_u64_t                        expire:1;
    mln_u64_t                        timestamp;
    struct mln_lang_ast_cache_s     *prev;
    struct mln_lang_ast_cache_s     *next;
};

struct mln_lang_s {
    mln_event_t                     *ev;
    mln_alloc_t                     *pool;
    mln_lang_ctx_t                  *run_head;
    mln_lang_ctx_t                  *run_tail;
    mln_lang_ctx_t                  *wait_head;
    mln_lang_ctx_t                  *wait_tail;
    mln_rbtree_t                    *resource_set;
    mln_u64_t                        wait:62;
    mln_u64_t                        quit:1;
    mln_u64_t                        cache:1;
    void                            *shift_table;
    mln_lang_ast_cache_t            *cache_head;
    mln_lang_ast_cache_t            *cache_tail;
    mln_lang_run_ctl_t               signal;
    mln_lang_run_ctl_t               clear;
    ev_fd_handler                    launcher;
    mln_rbtree_t                    *alias_set;
#if !defined(MSVC)
    pthread_mutex_t                  lock;
#endif
};

typedef enum {
    M_LSNT_STM = 0,
    M_LSNT_FUNCDEF,
    M_LSNT_SET,
    M_LSNT_SETSTM,
    M_LSNT_BLOCK,
    M_LSNT_WHILE,
    M_LSNT_SWITCH,
    M_LSNT_SWITCHSTM,
    M_LSNT_FOR,
    M_LSNT_IF,
    M_LSNT_EXP,
    M_LSNT_ASSIGN,
    M_LSNT_LOGICLOW,
    M_LSNT_LOGICHIGH,
    M_LSNT_RELATIVELOW,
    M_LSNT_RELATIVEHIGH,
    M_LSNT_MOVE,
    M_LSNT_ADDSUB,
    M_LSNT_MULDIV,
    M_LSNT_NOT,
    M_LSNT_SUFFIX,
    M_LSNT_LOCATE,
    M_LSNT_SPEC,
    M_LSNT_FACTOR,
    M_LSNT_ELEMLIST
} mln_lang_stack_node_type_t;

struct mln_lang_stack_node_s {
    mln_lang_ctx_t                  *ctx;
    mln_lang_stack_node_type_t       type;
    mln_u32_t                        step:31;
    mln_u32_t                        call:1;
    union {
        mln_lang_stm_t          *stm;
        mln_lang_funcdef_t      *funcdef;
        mln_lang_set_t          *set;
        mln_lang_setstm_t       *set_stm;
        mln_lang_block_t        *block;
        mln_lang_while_t        *w;
        mln_lang_switch_t       *sw;
        mln_lang_switchstm_t    *sw_stm;
        mln_lang_for_t          *f;
        mln_lang_if_t           *i;
        mln_lang_exp_t          *exp;
        mln_lang_assign_t       *assign;
        mln_lang_logiclow_t     *logiclow;
        mln_lang_logichigh_t    *logichigh;
        mln_lang_relativelow_t  *relativelow;
        mln_lang_relativehigh_t *relativehigh;
        mln_lang_move_t         *move;
        mln_lang_addsub_t       *addsub;
        mln_lang_muldiv_t       *muldiv;
        mln_lang_not_t          *not;
        mln_lang_suffix_t       *suffix;
        mln_lang_locate_t       *locate;
        mln_lang_spec_t         *spec;
        mln_lang_factor_t       *factor;
        mln_lang_elemlist_t     *elemlist;
    } data;
    mln_lang_var_t                  *ret_var;
    mln_lang_var_t                  *ret_var2;/* only used to store object temporarily in locate production */
    void                            *pos;
};

typedef enum {
    M_LANG_SCOPE_TYPE_SET = 0,
    M_LANG_SCOPE_TYPE_FUNC
} mln_lang_scope_type_t;

struct mln_lang_scope_s {
    mln_lang_scope_type_t            type;
    mln_string_t                    *name;
    mln_lang_ctx_t                  *ctx;
    mln_lang_stack_node_t           *cur_stack;
    mln_lang_stm_t                  *entry;
    mln_uauto_t                      layer;
    mln_lang_symbol_node_t          *sym_head;
    mln_lang_symbol_node_t          *sym_tail;
};

struct mln_lang_ctx_s {
    mln_lang_t                      *lang;
    mln_alloc_t                     *pool;
    mln_fileset_t                   *fset;
    void                            *data;
    mln_lang_stm_t                  *stm;
    mln_lang_stack_node_t            run_stack[M_LANG_RUN_STACK_LEN + 1];
    mln_lang_stack_node_t           *run_stack_top;
    mln_lang_scope_t                 scopes[M_LANG_SCOPE_LEN + 1];
    mln_lang_scope_t                *scope_top;
    mln_u64_t                        ref;
    mln_string_t                    *filename;
    mln_rbtree_t                    *resource_set;
    mln_lang_var_t                  *ret_var;
    mln_lang_return_handler          return_handler;
    mln_lang_ast_cache_t            *cache;
    mln_gc_t                        *gc;
    mln_lang_hash_t                 *symbols;
    struct mln_lang_ctx_s           *prev;
    struct mln_lang_ctx_s           *next;
    mln_lang_symbol_node_t          *sym_head;
    mln_lang_symbol_node_t          *sym_tail;
#if !defined(MSVC)
    pthread_t                        owner;
#endif
    mln_string_t                    *alias;
    mln_u32_t                        sym_count:16;
    mln_u32_t                        ret_flag:1;
    /*flags for operator overloading*/
    mln_u32_t                        op_array_flag:1;
    mln_u32_t                        op_bool_flag:1;
    mln_u32_t                        op_func_flag:1;
    mln_u32_t                        op_int_flag:1;
    mln_u32_t                        op_nil_flag:1;
    mln_u32_t                        op_obj_flag:1;
    mln_u32_t                        op_real_flag:1;
    mln_u32_t                        op_str_flag:1;
    mln_u32_t                        quit:1;
    mln_u32_t                        padding:6;
};

struct mln_lang_resource_s {
    mln_string_t                    *name;
    void                            *data;
    mln_lang_resource_free           free_handler;
};

typedef enum {
    M_LANG_SYMBOL_VAR = 0,
    M_LANG_SYMBOL_SET
} mln_lang_symbol_type_t;

struct mln_lang_symbol_node_s {
    mln_string_t                    *symbol;
    mln_lang_ctx_t                  *ctx;
    mln_lang_symbol_type_t           type;
    union {
        mln_lang_var_t          *var;
        mln_lang_set_detail_t   *set;
    } data;
    mln_uauto_t                      layer;
    mln_lang_hash_bucket_t          *bucket;
    struct mln_lang_symbol_node_s   *prev;
    struct mln_lang_symbol_node_s   *next;
    struct mln_lang_symbol_node_s   *scope_prev;
    struct mln_lang_symbol_node_s   *scope_next;
};

struct mln_lang_set_detail_s {
    mln_string_t                    *name;
    mln_rbtree_t                    *members;
    mln_u64_t                        ref;
};

typedef enum {
    M_LANG_VAR_NORMAL = 0,
    M_LANG_VAR_REFER
} mln_lang_var_type_t;

struct mln_lang_var_s {
    mln_lang_var_type_t              type;
    mln_string_t                    *name;
    mln_lang_val_t                  *val;
    mln_lang_set_detail_t           *in_set;
    mln_lang_var_t                  *prev;
    mln_lang_var_t                  *next;
    mln_uauto_t                      ref;
};

typedef enum {
    M_FUNC_INTERNAL = 0,
    M_FUNC_EXTERNAL
} mln_lang_func_type_t;

struct mln_lang_func_detail_s {
    mln_lang_exp_t                  *exp;
    mln_array_t                      args;
    mln_array_t                      closure;
    mln_lang_func_type_t             type;
    union {
        mln_lang_internal        process;
        mln_lang_stm_t          *stm;
    } data;
};

typedef enum {
    M_GC_OBJ = 0,
    M_GC_ARRAY
} mln_lang_gc_type_t;

typedef struct {
    mln_gc_t                        *gc;
    mln_lang_gc_type_t               type;
    union {
        mln_lang_object_t       *obj;
        mln_lang_array_t        *array;
    } data;
    void                            *gc_data;
} mln_lang_gc_item_t;

struct mln_lang_object_s {
    mln_lang_set_detail_t           *in_set;
    mln_rbtree_t                    *members;
    mln_u64_t                        ref;
    mln_lang_gc_item_t              *gc_item;
    mln_lang_ctx_t                  *ctx;
};

struct mln_lang_val_s {
    struct mln_lang_val_s           *prev;
    struct mln_lang_val_s           *next;
    union {
        mln_s64_t                i;
        mln_u8_t                 b;
        double                   f;
        mln_string_t            *s;
        mln_lang_object_t       *obj;
        mln_lang_func_detail_t  *func;
        mln_lang_array_t        *array;
        mln_lang_funccall_val_t *call;
    } data;
    mln_s32_t                        type;
    mln_u32_t                        ref;
    mln_lang_val_t                  *udata;
    mln_lang_func_detail_t          *func;
    mln_u32_t                        not_modify:1;
};

struct mln_lang_funccall_val_s {
    mln_string_t                    *name;
    mln_lang_func_detail_t          *prototype;
    mln_lang_val_t                  *object;
    mln_array_t                      args;
};

struct mln_lang_array_s {
    mln_rbtree_t                    *elems_index;
    mln_rbtree_t                    *elems_key;
    mln_u64_t                        index;
    mln_u64_t                        ref;
    mln_lang_gc_item_t              *gc_item;
    mln_lang_ctx_t                  *ctx;
};

struct mln_lang_array_elem_s {
    mln_u64_t                        index;
    mln_lang_var_t                  *key;
    mln_lang_var_t                  *value;
};

struct mln_lang_methods_s {
    mln_lang_op                      assign_handler;
    mln_lang_op                      pluseq_handler;
    mln_lang_op                      subeq_handler;
    mln_lang_op                      lmoveq_handler;
    mln_lang_op                      rmoveq_handler;
    mln_lang_op                      muleq_handler;
    mln_lang_op                      diveq_handler;
    mln_lang_op                      oreq_handler;
    mln_lang_op                      andeq_handler;
    mln_lang_op                      xoreq_handler;
    mln_lang_op                      modeq_handler;
    mln_lang_op                      cor_handler;
    mln_lang_op                      cand_handler;
    mln_lang_op                      cxor_handler;
    mln_lang_op                      equal_handler;
    mln_lang_op                      nonequal_handler;
    mln_lang_op                      less_handler;
    mln_lang_op                      lesseq_handler;
    mln_lang_op                      grea_handler;
    mln_lang_op                      greale_handler;
    mln_lang_op                      lmov_handler;
    mln_lang_op                      rmov_handler;
    mln_lang_op                      plus_handler;
    mln_lang_op                      sub_handler;
    mln_lang_op                      mul_handler;
    mln_lang_op                      div_handler;
    mln_lang_op                      mod_handler;
    mln_lang_op                      sdec_handler;
    mln_lang_op                      sinc_handler;
    mln_lang_op                      index_handler;
    mln_lang_op                      property_handler;
    mln_lang_op                      negative_handler;
    mln_lang_op                      reverse_handler;
    mln_lang_op                      not_handler;
    mln_lang_op                      pinc_handler;
    mln_lang_op                      pdec_handler;
};

typedef struct {
    mln_string_t                    *name;
    mln_size_t                       ref;
    mln_size_t                       count;
    void                            *handle;
    mln_rbtree_node_t               *node;
    mln_lang_t                      *lang;
} mln_lang_import_t;

typedef struct {
    mln_lang_import_t               *i;
    mln_lang_ctx_t                  *ctx;
} mln_lang_ctx_import_t;

typedef struct {
    mln_lang_ctx_t                  *ctx;
    mln_array_t                      list;
#if !defined(MSVC)
    pthread_mutex_t                  lock;
#endif
    mln_lang_ctx_pipe_recv_cb_t      recv_handler;
    mln_u32_t                        subscribed:1;
    mln_u32_t                        padding:31;
} mln_lang_ctx_pipe_t;

struct mln_lang_ctx_pipe_elem_s {
    int                              type;
    union {
        mln_s64_t                    i;
        double                       r;
        mln_string_t                *s;
    } data;
};

extern mln_lang_method_t *mln_lang_methods[];


#define mln_lang_ctx_is_quit(ctx)    ((ctx)->quit)
#if !defined(MSVC)
#define mln_lang_mutex_lock(lang)    pthread_mutex_lock(&(lang)->lock)
#define mln_lang_mutex_unlock(lang)  pthread_mutex_unlock(&(lang)->lock)
#endif
#define mln_lang_task_empty(lang)    ((lang)->run_head == NULL && (lang)->wait_head == NULL)
#define mln_lang_signal_get(lang)    ((lang)->signal)
#define mln_lang_event_get(lang)     ((lang)->ev)
#define mln_lang_launcher_get(lang)  ((lang)->launcher)
#define mln_lang_cache_set(lang)     ((lang)->cache = 1)
#define mln_lang_ctx_data_get(ctx)   ((ctx)->data)
#define mln_lang_ctx_data_set(ctx,d) ((ctx)->data = (d))
extern void mln_lang_errmsg(mln_lang_ctx_t *ctx, char *msg) __NONNULL2(1,2);
extern mln_lang_t *mln_lang_new(mln_event_t *ev, mln_lang_run_ctl_t signal, mln_lang_run_ctl_t clear) __NONNULL3(1,2,3);
extern void mln_lang_free(mln_lang_t *lang);
extern mln_lang_ctx_t *
mln_lang_job_new(mln_lang_t *lang, \
                 mln_string_t *alias, \
                 mln_u32_t type, \
                 mln_string_t *data, \
                 void *udata, \
                 mln_lang_return_handler handler) __NONNULL2(1,4);
extern void mln_lang_job_free(mln_lang_ctx_t *ctx);
extern void mln_lang_funccall_val_object_add(mln_lang_funccall_val_t *func, mln_lang_val_t *obj_val) __NONNULL2(1,2);
/*
 * Note:
 * 'name' must be in heap or global memory. It will be crashed in stack.
 */
extern int mln_lang_ctx_global_var_add(mln_lang_ctx_t *ctx, mln_string_t *name, void *val, mln_u32_t type) __NONNULL2(1,2);
extern mln_lang_var_t *mln_lang_var_create_call(mln_lang_ctx_t *ctx, mln_lang_funccall_val_t *call);
extern mln_lang_var_t *mln_lang_var_create_nil(mln_lang_ctx_t *ctx, mln_string_t *name) __NONNULL1(1);
extern mln_lang_var_t *mln_lang_var_create_obj(mln_lang_ctx_t *ctx, mln_lang_set_detail_t *in_set, mln_string_t *name) __NONNULL1(1);
extern mln_lang_var_t *mln_lang_var_create_true(mln_lang_ctx_t *ctx, mln_string_t *name) __NONNULL1(1);
extern mln_lang_var_t *mln_lang_var_create_false(mln_lang_ctx_t *ctx, mln_string_t *name) __NONNULL1(1);
extern mln_lang_var_t *mln_lang_var_create_int(mln_lang_ctx_t *ctx, mln_s64_t off, mln_string_t *name) __NONNULL1(1);
extern mln_lang_var_t *mln_lang_var_create_real(mln_lang_ctx_t *ctx, double f, mln_string_t *name) __NONNULL1(1);
extern mln_lang_var_t *mln_lang_var_create_bool(mln_lang_ctx_t *ctx, mln_u8_t b, mln_string_t *name) __NONNULL1(1);
extern mln_lang_var_t *mln_lang_var_create_ref_string(mln_lang_ctx_t *ctx, mln_string_t *s, mln_string_t *name) __NONNULL2(1,2);
extern mln_lang_var_t *mln_lang_var_create_string(mln_lang_ctx_t *ctx, mln_string_t *s, mln_string_t *name) __NONNULL2(1,2);
extern mln_lang_var_t *mln_lang_var_create_array(mln_lang_ctx_t *ctx, mln_string_t *name) __NONNULL1(1);
extern mln_lang_symbol_node_t *mln_lang_symbol_node_search(mln_lang_ctx_t *ctx, mln_string_t *name, int local) __NONNULL2(1,2);
/* Note end*/
extern int mln_lang_symbol_node_join(mln_lang_ctx_t *ctx, mln_lang_symbol_type_t type, void *data) __NONNULL2(1,3);
extern int mln_lang_symbol_node_upper_join(mln_lang_ctx_t *ctx, mln_lang_symbol_type_t type, void *data) __NONNULL2(1,3);
extern mln_lang_var_t *mln_lang_var_new(mln_lang_ctx_t *ctx, \
                                        mln_string_t *name, \
                                        mln_lang_var_type_t type, \
                                        mln_lang_val_t *val, \
                                        mln_lang_set_detail_t *in_set) __NONNULL1(1);
extern void mln_lang_var_free(void *data);
extern int mln_lang_var_cmp(const void *data1, const void *data2);
#define mln_lang_var_ref(var) (++(var)->ref, (var))
#define mln_lang_var_val_get(var) ((var)->val)
#define mln_lang_var_val_type_get(var) ((var)->val->type)
#define mln_lang_val_not_modify_set(val) ((val)->not_modify = 1)
#define mln_lang_val_not_modify_isset(val) ((val)->not_modify)
#define mln_lang_val_not_modify_reset(val) ((val)->not_modify = 0)
extern void mln_lang_var_set_int(mln_lang_var_t *var, mln_s64_t i) __NONNULL1(1);
extern void mln_lang_var_set_real(mln_lang_var_t *var, double r) __NONNULL1(1);
extern void mln_lang_var_set_string(mln_lang_var_t *var, mln_string_t *s) __NONNULL2(1,2);
extern mln_lang_var_t *mln_lang_var_dup(mln_lang_ctx_t *ctx, mln_lang_var_t *var) __NONNULL2(1,2);
extern void mln_lang_var_assign(mln_lang_var_t *var, mln_lang_val_t *val) __NONNULL2(1,2);
extern int mln_lang_var_value_set(mln_lang_ctx_t *ctx, mln_lang_var_t *dest, mln_lang_var_t *src) __NONNULL3(1,2,3);
extern int mln_lang_var_value_set_string_ref(mln_lang_ctx_t *ctx, mln_lang_var_t *dest, mln_lang_var_t *src) __NONNULL1(1);
extern mln_lang_func_detail_t *
mln_lang_func_detail_new(mln_lang_ctx_t *ctx, \
                         mln_lang_func_type_t type, \
                         void *data, \
                         mln_lang_exp_t *exp, \
                         mln_lang_exp_t *closure) __NONNULL2(1,3);
extern void mln_lang_func_detail_free(mln_lang_func_detail_t *lfd);
extern int mln_lang_func_detail_arg_append(mln_lang_func_detail_t *func, mln_lang_var_t *var) __NONNULL2(1,2);
extern mln_lang_val_t *mln_lang_val_new(mln_lang_ctx_t *ctx, mln_s32_t type, void *data) __NONNULL1(1);
extern void mln_lang_val_free(mln_lang_val_t *val);
extern int mln_lang_condition_is_true(mln_lang_var_t *var) __NONNULL1(1);
extern mln_lang_var_t *mln_lang_set_member_search(mln_rbtree_t *members, mln_string_t *name) __NONNULL2(1,2);
extern mln_lang_funccall_val_t *mln_lang_funccall_val_new(mln_alloc_t *pool, mln_string_t *name) __NONNULL1(1);
extern void mln_lang_funccall_val_free(mln_lang_funccall_val_t *func);
extern int mln_lang_funccall_val_add_arg(mln_lang_funccall_val_t *func, mln_lang_var_t *var);
extern int mln_lang_funccall_val_operator(mln_lang_ctx_t *ctx, mln_string_t *name, mln_lang_var_t **ret, mln_lang_var_t *op1, mln_lang_var_t *op2) __NONNULL4(1,2,3,4);
extern int mln_lang_funccall_val_obj_operator(mln_lang_ctx_t *ctx, \
                                              mln_lang_var_t *obj, \
                                              mln_string_t *name, \
                                              mln_lang_var_t **ret, \
                                              mln_lang_var_t *op1, \
                                              mln_lang_var_t *op2) __NONNULL5(1,2,3,4,5);
extern mln_lang_var_t *
mln_lang_array_get(mln_lang_ctx_t *ctx, mln_lang_array_t *array, mln_lang_var_t *key) __NONNULL2(1,2);
extern mln_lang_set_detail_t *
mln_lang_set_detail_new(mln_alloc_t *pool, mln_string_t *name) __NONNULL2(1,2);
extern void mln_lang_set_detail_free(mln_lang_set_detail_t *c);
extern void mln_lang_set_detail_self_free(mln_lang_set_detail_t *c);
extern int
mln_lang_set_member_add(mln_alloc_t *pool, mln_rbtree_t *members, mln_lang_var_t *var) __NONNULL3(1,2,3);
extern int
mln_lang_object_add_member(mln_lang_ctx_t *ctx, mln_lang_object_t *obj, mln_lang_var_t *var) __NONNULL3(1,2,3);
extern mln_lang_array_t *mln_lang_array_new(mln_lang_ctx_t *ctx) __NONNULL1(1);
extern void mln_lang_array_free(mln_lang_array_t *array);
extern int mln_lang_array_elem_exist(mln_lang_array_t *array, mln_lang_var_t *key) __NONNULL2(1,2);
extern int mln_lang_ctx_resource_register(mln_lang_ctx_t *ctx, char *name, void *data, mln_lang_resource_free free_handler) __NONNULL2(1,2);
extern void *mln_lang_ctx_resource_fetch(mln_lang_ctx_t *ctx, char *name) __NONNULL2(1,2);
extern void mln_lang_ctx_set_ret_var(mln_lang_ctx_t *ctx, mln_lang_var_t *var) __NONNULL1(1);
extern void mln_lang_ctx_suspend(mln_lang_ctx_t *ctx) __NONNULL1(1);
extern void mln_lang_ctx_continue(mln_lang_ctx_t *ctx) __NONNULL1(1);
extern int mln_lang_resource_register(mln_lang_t *lang, char *name, void *data, mln_lang_resource_free free_handler) __NONNULL2(1,2);
extern void mln_lang_resource_cancel(mln_lang_t *lang, char *name) __NONNULL2(1,2);
extern void *mln_lang_resource_fetch(mln_lang_t *lang, char *name) __NONNULL2(1,2);
extern int mln_lang_ctx_pipe_send(mln_lang_ctx_t *ctx, char *fmt, ...);
extern int mln_lang_ctx_pipe_recv_handler_set(mln_lang_ctx_t *ctx, mln_lang_ctx_pipe_recv_cb_t recv_handler) __NONNULL1(1);

#endif
