
/*
 * Copyright (C) Niklaus F.Schen.
 */
#include "file/mln_lang_file.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>

#ifdef __DEBUG__
#include <assert.h>
#define ASSERT(x) assert(x)
#else
#define ASSERT(x);
#endif

static inline void mln_lang_file_open_get_prio(mln_s64_t prio, mode_t *mode);
static int mln_lang_file_open_get_op(mln_string_t *op);
static int mln_lang_file_add_fd(mln_lang_ctx_t *ctx, mln_lang_set_detail_t *set);
static int mln_lang_file_add_errno(mln_lang_ctx_t *ctx, mln_lang_set_detail_t *set);
static int mln_lang_file_add_open(mln_lang_ctx_t *ctx, mln_lang_set_detail_t *set);
static int mln_lang_file_add_lseek(mln_lang_ctx_t *ctx, mln_lang_set_detail_t *set);
static int mln_lang_file_add_read(mln_lang_ctx_t *ctx, mln_lang_set_detail_t *set);
static int mln_lang_file_add_write(mln_lang_ctx_t *ctx, mln_lang_set_detail_t *set);
static int mln_lang_file_add_close(mln_lang_ctx_t *ctx, mln_lang_set_detail_t *set);
static int mln_lang_file_add_errmsg(mln_lang_ctx_t *ctx, mln_lang_set_detail_t *set);
static int mln_lang_file_add_size(mln_lang_ctx_t *ctx, mln_lang_set_detail_t *set);
static mln_lang_var_t *mln_lang_open_process(mln_lang_ctx_t *ctx);
static mln_lang_var_t *mln_lang_lseek_process(mln_lang_ctx_t *ctx);
static mln_lang_var_t *mln_lang_read_process(mln_lang_ctx_t *ctx);
static mln_lang_var_t *mln_lang_write_process(mln_lang_ctx_t *ctx);
static mln_lang_var_t *mln_lang_close_process(mln_lang_ctx_t *ctx);
static mln_lang_var_t *mln_lang_errmsg_process(mln_lang_ctx_t *ctx);
static mln_lang_var_t *mln_lang_size_process(mln_lang_ctx_t *ctx);
static int mln_lang_file_set_errno(mln_lang_ctx_t *ctx, int err);
static int mln_lang_file_fd_cmp(int fd1, int fd2);

int mln_lang_file(mln_lang_ctx_t *ctx)
{
    mln_string_t setname = mln_string("File");
    mln_lang_set_detail_t *set;
    mln_rbtree_t *tree;
    struct mln_rbtree_attr rbattr;

    rbattr.pool = ctx->pool;
    rbattr.pool_alloc = (rbtree_pool_alloc_handler)mln_alloc_m;
    rbattr.pool_free = (rbtree_pool_free_handler)mln_alloc_free;
    rbattr.cmp = (rbtree_cmp)mln_lang_file_fd_cmp;
    rbattr.data_free = (rbtree_free_data)close;
    rbattr.cache = 0;
    if ((tree = mln_rbtree_init(&rbattr)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if (mln_lang_ctx_resource_register(ctx, "file_fd", tree, (mln_lang_resource_free)mln_rbtree_destroy) < 0) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_rbtree_destroy(tree);
        return -1;
    }

    if ((set = mln_lang_set_detail_new(ctx->pool, &setname)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    ++(set->ref);
    if (mln_lang_symbol_node_join(ctx, M_LANG_SYMBOL_SET, set) < 0) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_set_detail_self_free(set);
        return -1;
    }

    if (mln_lang_file_add_fd(ctx, set) < 0) {
        return -1;
    }
    if (mln_lang_file_add_errno(ctx, set) < 0) {
        return -1;
    }
    if (mln_lang_file_add_open(ctx, set) < 0) {
        return -1;
    }
    if (mln_lang_file_add_lseek(ctx, set) < 0) {
        return -1;
    }
    if (mln_lang_file_add_read(ctx, set) < 0) {
        return -1;
    }
    if (mln_lang_file_add_write(ctx, set) < 0) {
        return -1;
    }
    if (mln_lang_file_add_close(ctx, set) < 0) {
        return -1;
    }
    if (mln_lang_file_add_errmsg(ctx, set) < 0) {
        return -1;
    }
    if (mln_lang_file_add_size(ctx, set) < 0) {
        return -1;
    }
    return 0;
}

static int mln_lang_file_fd_cmp(int fd1, int fd2)
{
    return fd1 - fd2;
}

static int mln_lang_file_add_fd(mln_lang_ctx_t *ctx, mln_lang_set_detail_t *set)
{
    mln_s64_t fd = -1;
    mln_lang_val_t *val;
    mln_lang_var_t *var;
    mln_string_t varname = mln_string("fd");

    if ((val = mln_lang_val_new(ctx, M_LANG_VAL_TYPE_INT, &fd)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if ((var = mln_lang_var_new(ctx, &varname, M_LANG_VAR_NORMAL, val, set)) == NULL) {
        mln_lang_val_free(val);
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if (mln_lang_set_member_add(ctx->pool, set->members, var) < 0) {
        mln_lang_var_free(var);
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    return 0;
}

static int mln_lang_file_add_errno(mln_lang_ctx_t *ctx, mln_lang_set_detail_t *set)
{
    int err = 0;
    mln_lang_val_t *val;
    mln_lang_var_t *var;
    mln_string_t varname = mln_string("errno");

    if ((val = mln_lang_val_new(ctx, M_LANG_VAL_TYPE_INT, &err)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if ((var = mln_lang_var_new(ctx, &varname, M_LANG_VAR_NORMAL, val, set)) == NULL) {
        mln_lang_val_free(val);
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if (mln_lang_set_member_add(ctx->pool, set->members, var) < 0) {
        mln_lang_var_free(var);
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    return 0;
}

static int mln_lang_file_add_open(mln_lang_ctx_t *ctx, mln_lang_set_detail_t *set)
{
    mln_lang_val_t *val;
    mln_lang_var_t *var;
    mln_lang_func_detail_t *func;
    mln_string_t funcname = mln_string("open");
    mln_string_t v1 = mln_string("path"), v2 = mln_string("op"), v3 = mln_string("prio");

    if ((func = mln_lang_func_detail_new(ctx, M_FUNC_INTERNAL, mln_lang_open_process, NULL, NULL)) == NULL) {
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
    mln_lang_var_chain_add(&(func->args_head), &(func->args_tail), var);
    ++func->nargs;
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
    mln_lang_var_chain_add(&(func->args_head), &(func->args_tail), var);
    ++func->nargs;
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
    mln_lang_var_chain_add(&(func->args_head), &(func->args_tail), var);
    ++func->nargs;

    if ((val = mln_lang_val_new(ctx, M_LANG_VAL_TYPE_FUNC, func)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_func_detail_free(func);
        return -1;
    }
    if ((var = mln_lang_var_new(ctx, &funcname, M_LANG_VAR_NORMAL, val, set)) == NULL) {
        mln_lang_val_free(val);
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if (mln_lang_set_member_add(ctx->pool, set->members, var) < 0) {
        mln_lang_var_free(var);
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }

    return 0;
}

static mln_lang_var_t *mln_lang_open_process(mln_lang_ctx_t *ctx)
{
    mln_s32_t type, op, prio;
    mln_lang_val_t *val1, *val2, *val;
    mln_string_t _this = mln_string("this");
    mln_string_t v1 = mln_string("path");
    mln_string_t v2 = mln_string("op");
    mln_string_t v3 = mln_string("prio");
    mln_string_t v = mln_string("fd");
    mln_string_t typename = mln_string("File");
    mln_lang_symbol_node_t *sym;
    mln_lang_var_t *var;
    mln_s8ptr_t path;
    mln_rbtree_t *tree;
    mln_rbtree_node_t *rn;
    mln_lang_var_t *ret_var;
    mode_t mode;

    if ((sym = mln_lang_symbol_node_search(ctx, &_this, 1)) == NULL) {
        ASSERT(0);
        mln_lang_errmsg(ctx, "Lack of 'this'.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);

    type = mln_lang_var_val_type_get(sym->data.var);
    val = sym->data.var->val;
    if (type != M_LANG_VAL_TYPE_OBJECT) {
        mln_lang_errmsg(ctx, "'this' not object.");
        return NULL;
    }
    if (val->data.obj->in_set == NULL || val->data.obj->in_set->name == NULL || mln_string_strcmp(val->data.obj->in_set->name, &typename)) {
        mln_lang_errmsg(ctx, "Invalid set type, File object required.");
        return NULL;
    }
    if ((var = mln_lang_set_member_search(val->data.obj->members, &v)) == NULL) {
        mln_lang_errmsg(ctx, "Lack of member 'fd'.");
        return NULL;
    }
    if (mln_lang_var_val_type_get(var) != M_LANG_VAL_TYPE_INT) {
        mln_lang_errmsg(ctx, "Invalid type of 'fd'.");
        return NULL;
    }
    val = var->val;

    /*arg1*/
    if ((sym = mln_lang_symbol_node_search(ctx, &v1, 1)) == NULL) {
        ASSERT(0);
        mln_lang_errmsg(ctx, "Argument missing.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);
    type = mln_lang_var_val_type_get(sym->data.var);
    val1 = sym->data.var->val;
    if (type != M_LANG_VAL_TYPE_STRING) {
        mln_lang_errmsg(ctx, "'path' demand string.");
        return NULL;
    }

    /*arg2*/
    if ((sym = mln_lang_symbol_node_search(ctx, &v2, 1)) == NULL) {
        ASSERT(0);
        mln_lang_errmsg(ctx, "Argument missing.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);
    type = mln_lang_var_val_type_get(sym->data.var);
    val2 = sym->data.var->val;
    if (type != M_LANG_VAL_TYPE_STRING) {
        mln_lang_errmsg(ctx, "'op' demand string.");
        return NULL;
    }

    /*arg3*/
    if ((sym = mln_lang_symbol_node_search(ctx, &v3, 1)) == NULL) {
        ASSERT(0);
        mln_lang_errmsg(ctx, "Argument missing.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);
    type = mln_lang_var_val_type_get(sym->data.var);
    if (type == M_LANG_VAL_TYPE_NIL) {
        prio = 0644;
    } else if (type == M_LANG_VAL_TYPE_INT) {
        prio = sym->data.var->val->data.i;
    } else {
        mln_lang_errmsg(ctx, "'prio' demand integer.");
        return NULL;
    }

    if (val1->data.s == NULL || val2->data.s == NULL) {
        mln_lang_errmsg(ctx, "Invalid argument.");
        return NULL;
    }

    if ((path = (mln_s8ptr_t)malloc(val1->data.s->len + 1)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return NULL;
    }
    memcpy(path, val1->data.s->data, val1->data.s->len);
    path[val1->data.s->len] = 0;
    if ((op = mln_lang_file_open_get_op(val2->data.s)) < 0) {
        mln_lang_errmsg(ctx, "Invalid 'op'.");
        free(path);
        return NULL;
    }
    memset(&mode, 0, sizeof(mode));
    mln_lang_file_open_get_prio(prio, &mode);
    val->data.i = open(path, op, mode);
    if (mln_lang_file_set_errno(ctx, errno) < 0) {
        free(path);
        return NULL;
    }
    free(path);

    tree = mln_lang_ctx_resource_fetch(ctx, "file_fd");
#if defined(WIN32)
    int fd = val->data.i;
    if ((rn = mln_rbtree_node_new(tree, (void *)fd)) == NULL) {
#else
    if ((rn = mln_rbtree_node_new(tree, (void *)(val->data.i))) == NULL) {
#endif
        mln_lang_errmsg(ctx, "No memory.");
        close(val->data.i);
        return NULL;
    }
    mln_rbtree_insert(tree, rn);

    if (val->data.i < 0) {
        ret_var = mln_lang_var_create_false(ctx, NULL);
    } else {
        mln_lang_val_not_modify_set(val);
        ret_var = mln_lang_var_create_true(ctx, NULL);
    }
    if (ret_var == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return NULL;
    }
    return ret_var;
}

static inline void mln_lang_file_open_get_prio(mln_s64_t prio, mode_t *mode)
{
    mode_t m = 0;
    if (prio & 0x1) m |= S_IXOTH;
    if (prio & 0x2) m |= S_IWOTH;
    if (prio & 0x4) m |= S_IROTH;
    if (prio & 0x8) m |= S_IXGRP;
    if (prio & 0x10) m |= S_IWGRP;
    if (prio & 0x20) m |= S_IRGRP;
    if (prio & 0x40) m |= S_IXUSR;
    if (prio & 0x80) m |= S_IWUSR;
    if (prio & 0x100) m |= S_IRUSR;
    *mode = m;
}

static int mln_lang_file_open_get_op(mln_string_t *op)
{
    int flags = 0, r = 0, w = 0, a = 0, notrunc = 0;
    mln_u8ptr_t p, pend;
    for (p = op->data, pend = op->data+op->len; p < pend; ++p) {
        if (*p == (mln_u8_t)'r') {
            r = 1;
        } else if (*p == (mln_u8_t)'w') {
            w = 1;
        } else if (*p == (mln_u8_t)'a') {
            a = 1;
        } else if (*p == (mln_u8_t)'+') {
            notrunc = 1;
        } else {
            return -1;
        }
    }
    if (r && w) {
        flags |= (O_RDWR|O_CREAT|O_TRUNC);
    } else {
        if (r) {
            flags |= O_RDONLY;
        } else if (w) {
            flags |= (O_WRONLY|O_CREAT|O_TRUNC);
        } else {
            flags |= (O_RDWR|O_CREAT|O_TRUNC);
        }
    }
    if (a) {
        flags &= (~(int)O_TRUNC);
        flags |= O_APPEND;
    } else if (notrunc) {
        flags &= (~(int)O_TRUNC);
        flags &= (~(int)O_APPEND);
    }
    return flags;
}

static int mln_lang_file_add_lseek(mln_lang_ctx_t *ctx, mln_lang_set_detail_t *set)
{
    mln_lang_val_t *val;
    mln_lang_var_t *var;
    mln_lang_func_detail_t *func;
    mln_string_t funcname = mln_string("lseek");
    mln_string_t v1 = mln_string("offset"), v2 = mln_string("whence");

    if ((func = mln_lang_func_detail_new(ctx, M_FUNC_INTERNAL, mln_lang_lseek_process, NULL, NULL)) == NULL) {
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
    mln_lang_var_chain_add(&(func->args_head), &(func->args_tail), var);
    ++func->nargs;
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
    mln_lang_var_chain_add(&(func->args_head), &(func->args_tail), var);
    ++func->nargs;

    if ((val = mln_lang_val_new(ctx, M_LANG_VAL_TYPE_FUNC, func)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_func_detail_free(func);
        return -1;
    }
    if ((var = mln_lang_var_new(ctx, &funcname, M_LANG_VAR_NORMAL, val, set)) == NULL) {
        mln_lang_val_free(val);
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if (mln_lang_set_member_add(ctx->pool, set->members, var) < 0) {
        mln_lang_var_free(var);
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }

    return 0;
}

static mln_lang_var_t *mln_lang_lseek_process(mln_lang_ctx_t *ctx)
{
    mln_s32_t t, whence = 0;
    mln_lang_val_t *val1, *val2, *val;
    mln_string_t _this = mln_string("this");
    mln_string_t v1 = mln_string("offset");
    mln_string_t v2 = mln_string("whence");
    mln_string_t v = mln_string("fd");
    mln_string_t typename = mln_string("File");
    mln_string_t *tmp;
    mln_lang_symbol_node_t *sym;
    mln_lang_var_t *var;
    mln_lang_var_t *ret_var;

    if ((sym = mln_lang_symbol_node_search(ctx, &_this, 1)) == NULL) {
        ASSERT(0);
        mln_lang_errmsg(ctx, "Lack of 'this'.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);

    t = mln_lang_var_val_type_get(sym->data.var);
    val = sym->data.var->val;
    if (t != M_LANG_VAL_TYPE_OBJECT) {
        mln_lang_errmsg(ctx, "'this' not object.");
        return NULL;
    }
    if (val->data.obj->in_set == NULL || val->data.obj->in_set->name == NULL || mln_string_strcmp(val->data.obj->in_set->name, &typename)) {
        mln_lang_errmsg(ctx, "Invalid set type, File object required.");
        return NULL;
    }
    if ((var = mln_lang_set_member_search(val->data.obj->members, &v)) == NULL) {
        mln_lang_errmsg(ctx, "Lack of member 'fd'.");
        return NULL;
    }
    if (mln_lang_var_val_type_get(var) != M_LANG_VAL_TYPE_INT) {
        mln_lang_errmsg(ctx, "Invalid type of 'fd'.");
        return NULL;
    }
    val = var->val;

    /*arg1*/
    if ((sym = mln_lang_symbol_node_search(ctx, &v1, 1)) == NULL) {
        ASSERT(0);
        mln_lang_errmsg(ctx, "Argument missing.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);
    t = mln_lang_var_val_type_get(sym->data.var);
    val1 = sym->data.var->val;
    if (t != M_LANG_VAL_TYPE_INT) {
        mln_lang_errmsg(ctx, "'offset' demand integer.");
        return NULL;
    }
    if (val1->data.i < 0) {
        if (mln_lang_file_set_errno(ctx, EBADF) < 0) {
            return NULL;
        }
        if ((ret_var = mln_lang_var_create_false(ctx, NULL)) == NULL) {
            mln_lang_errmsg(ctx, "No memory.");
            return NULL;
        }
        return ret_var;
    }

    /*arg2*/
    if ((sym = mln_lang_symbol_node_search(ctx, &v2, 1)) == NULL) {
        ASSERT(0);
        mln_lang_errmsg(ctx, "Argument missing.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);
    t = mln_lang_var_val_type_get(sym->data.var);
    val2 = sym->data.var->val;
    if (t != M_LANG_VAL_TYPE_STRING) {
        mln_lang_errmsg(ctx, "'whence' demand string.");
        return NULL;
    }
    if ((tmp = val2->data.s) == NULL) {
        mln_lang_errmsg(ctx, "Invalid argument.");
        return NULL;
    }
    if (!mln_string_const_strcmp(tmp, "begin")) {
        whence = SEEK_SET;
    } else if (!mln_string_const_strcmp(tmp, "current")) {
        whence = SEEK_CUR;
    } else if (!mln_string_const_strcmp(tmp, "end")) {
        whence = SEEK_END;
    } else {
        mln_lang_errmsg(ctx, "Invalid argument 'whence'.");
        return NULL;
    }

    t = lseek(val->data.i, val1->data.i, whence);
    if (mln_lang_file_set_errno(ctx, errno) < 0) {
        return NULL;
    }
    if (t < 0) {
        ret_var = mln_lang_var_create_false(ctx, NULL);
    } else {
        ret_var = mln_lang_var_create_true(ctx, NULL);
    }
    if (ret_var == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return NULL;
    }
    return ret_var;
}

static int mln_lang_file_add_read(mln_lang_ctx_t *ctx, mln_lang_set_detail_t *set)
{
    mln_lang_val_t *val;
    mln_lang_var_t *var;
    mln_lang_func_detail_t *func;
    mln_string_t funcname = mln_string("read");
    mln_string_t v1 = mln_string("nbytes");

    if ((func = mln_lang_func_detail_new(ctx, M_FUNC_INTERNAL, mln_lang_read_process, NULL, NULL)) == NULL) {
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
    mln_lang_var_chain_add(&(func->args_head), &(func->args_tail), var);
    ++func->nargs;

    if ((val = mln_lang_val_new(ctx, M_LANG_VAL_TYPE_FUNC, func)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_func_detail_free(func);
        return -1;
    }
    if ((var = mln_lang_var_new(ctx, &funcname, M_LANG_VAR_NORMAL, val, set)) == NULL) {
        mln_lang_val_free(val);
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if (mln_lang_set_member_add(ctx->pool, set->members, var) < 0) {
        mln_lang_var_free(var);
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }

    return 0;
}

static mln_lang_var_t *mln_lang_read_process(mln_lang_ctx_t *ctx)
{
    mln_s32_t t;
    ssize_t n;
    mln_lang_val_t *val1, *val;
    mln_string_t _this = mln_string("this");
    mln_string_t v1 = mln_string("nbytes");
    mln_string_t v = mln_string("fd");
    mln_string_t typename = mln_string("File");
    mln_string_t tmp;
    mln_lang_symbol_node_t *sym;
    mln_lang_var_t *var;
    mln_lang_var_t *ret_var;
    mln_u8ptr_t buf;

    if ((sym = mln_lang_symbol_node_search(ctx, &_this, 1)) == NULL) {
        ASSERT(0);
        mln_lang_errmsg(ctx, "Lack of 'this'.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);

    t = mln_lang_var_val_type_get(sym->data.var);
    val = sym->data.var->val;
    if (t != M_LANG_VAL_TYPE_OBJECT) {
        mln_lang_errmsg(ctx, "'this' not object.");
        return NULL;
    }
    if (val->data.obj->in_set == NULL || val->data.obj->in_set->name == NULL || mln_string_strcmp(val->data.obj->in_set->name, &typename)) {
        mln_lang_errmsg(ctx, "Invalid set type, File object required.");
        return NULL;
    }
    if ((var = mln_lang_set_member_search(val->data.obj->members, &v)) == NULL) {
        mln_lang_errmsg(ctx, "Lack of member 'fd'.");
        return NULL;
    }
    if (mln_lang_var_val_type_get(var) != M_LANG_VAL_TYPE_INT) {
        mln_lang_errmsg(ctx, "Invalid type of 'fd'.");
        return NULL;
    }
    val = var->val;
    if (val->data.i < 0) {
        if (mln_lang_file_set_errno(ctx, EBADF) < 0) {
            return NULL;
        }
        if ((ret_var = mln_lang_var_create_false(ctx, NULL)) == NULL) {
            mln_lang_errmsg(ctx, "No memory.");
            return NULL;
        }
        return ret_var;
    }

    /*arg1*/
    if ((sym = mln_lang_symbol_node_search(ctx, &v1, 1)) == NULL) {
        ASSERT(0);
        mln_lang_errmsg(ctx, "Argument missing.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);
    if (mln_lang_var_val_type_get(sym->data.var) != M_LANG_VAL_TYPE_INT) {
        mln_lang_errmsg(ctx, "'nbytes' demand integer.");
        return NULL;
    }
    val1 = sym->data.var->val;
    if (val1->data.i <= 0) {
        if ((ret_var = mln_lang_var_create_nil(ctx, NULL)) == NULL) {
            mln_lang_errmsg(ctx, "No memory.");
            return NULL;
        }
        return ret_var;
    }

    if ((buf = (mln_u8ptr_t)malloc(val1->data.i)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return NULL;
    }
    n = read(val->data.i, buf, val1->data.i);
    if (mln_lang_file_set_errno(ctx, errno) < 0) {
        free(buf);
        return NULL;
    }
    if (n < 0) {
        free(buf);
        ret_var = mln_lang_var_create_false(ctx, NULL);
    } else {
        mln_string_nset(&tmp, buf, n);
        ret_var = mln_lang_var_create_string(ctx, &tmp, NULL);
        free(buf);
    }
    if (ret_var == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return NULL;
    }
    return ret_var;
}

static int mln_lang_file_add_write(mln_lang_ctx_t *ctx, mln_lang_set_detail_t *set)
{
    mln_lang_val_t *val;
    mln_lang_var_t *var;
    mln_lang_func_detail_t *func;
    mln_string_t funcname = mln_string("write");
    mln_string_t v1 = mln_string("buf");

    if ((func = mln_lang_func_detail_new(ctx, M_FUNC_INTERNAL, mln_lang_write_process, NULL, NULL)) == NULL) {
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
    mln_lang_var_chain_add(&(func->args_head), &(func->args_tail), var);
    ++func->nargs;

    if ((val = mln_lang_val_new(ctx, M_LANG_VAL_TYPE_FUNC, func)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_func_detail_free(func);
        return -1;
    }
    if ((var = mln_lang_var_new(ctx, &funcname, M_LANG_VAR_NORMAL, val, set)) == NULL) {
        mln_lang_val_free(val);
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if (mln_lang_set_member_add(ctx->pool, set->members, var) < 0) {
        mln_lang_var_free(var);
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }

    return 0;
}

static mln_lang_var_t *mln_lang_write_process(mln_lang_ctx_t *ctx)
{
    mln_s32_t t;
    ssize_t n;
    mln_lang_val_t *val1, *val;
    mln_string_t _this = mln_string("this");
    mln_string_t v1 = mln_string("buf");
    mln_string_t v = mln_string("fd");
    mln_string_t typename = mln_string("File");
    mln_lang_symbol_node_t *sym;
    mln_lang_var_t *var;
    mln_lang_var_t *ret_var;

    if ((sym = mln_lang_symbol_node_search(ctx, &_this, 1)) == NULL) {
        ASSERT(0);
        mln_lang_errmsg(ctx, "Lack of 'this'.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);

    t = mln_lang_var_val_type_get(sym->data.var);
    val = sym->data.var->val;
    if (t != M_LANG_VAL_TYPE_OBJECT) {
        mln_lang_errmsg(ctx, "'this' not object.");
        return NULL;
    }
    if (val->data.obj->in_set == NULL || val->data.obj->in_set->name == NULL || mln_string_strcmp(val->data.obj->in_set->name, &typename)) {
        mln_lang_errmsg(ctx, "Invalid set type, File object required.");
        return NULL;
    }
    if ((var = mln_lang_set_member_search(val->data.obj->members, &v)) == NULL) {
        mln_lang_errmsg(ctx, "Lack of member 'fd'.");
        return NULL;
    }
    if (mln_lang_var_val_type_get(var) != M_LANG_VAL_TYPE_INT) {
        mln_lang_errmsg(ctx, "Invalid type of 'fd'.");
        return NULL;
    }
    val = var->val;
    if (val->data.i < 0) {
        if (mln_lang_file_set_errno(ctx, EBADF) < 0) {
            return NULL;
        }
        if ((ret_var = mln_lang_var_create_false(ctx, NULL)) == NULL) {
            mln_lang_errmsg(ctx, "No memory.");
            return NULL;
        }
        return ret_var;
    }

    /*arg1*/
    if ((sym = mln_lang_symbol_node_search(ctx, &v1, 1)) == NULL) {
        ASSERT(0);
        mln_lang_errmsg(ctx, "Argument missing.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);
    if (mln_lang_var_val_type_get(sym->data.var) != M_LANG_VAL_TYPE_STRING) {
        mln_lang_errmsg(ctx, "'buf' demand string.");
        return NULL;
    }
    val1 = sym->data.var->val;
    if (val1->data.s == NULL) {
        mln_lang_errmsg(ctx, "Invalid argument.");
        return NULL;
    }

    n = write(val->data.i, val1->data.s->data, val1->data.s->len);
    if (mln_lang_file_set_errno(ctx, errno) < 0) {
        return NULL;
    }
    if (n < 0) {
        ret_var = mln_lang_var_create_false(ctx, NULL);
    } else {
        ret_var = mln_lang_var_create_int(ctx, n, NULL);
    }
    if (ret_var == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return NULL;
    }
    return ret_var;
}

static int mln_lang_file_add_close(mln_lang_ctx_t *ctx, mln_lang_set_detail_t *set)
{
    mln_lang_val_t *val;
    mln_lang_var_t *var;
    mln_lang_func_detail_t *func;
    mln_string_t funcname = mln_string("close");

    if ((func = mln_lang_func_detail_new(ctx, M_FUNC_INTERNAL, mln_lang_close_process, NULL, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if ((val = mln_lang_val_new(ctx, M_LANG_VAL_TYPE_FUNC, func)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_func_detail_free(func);
        return -1;
    }
    if ((var = mln_lang_var_new(ctx, &funcname, M_LANG_VAR_NORMAL, val, set)) == NULL) {
        mln_lang_val_free(val);
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if (mln_lang_set_member_add(ctx->pool, set->members, var) < 0) {
        mln_lang_var_free(var);
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }

    return 0;
}

static mln_lang_var_t *mln_lang_close_process(mln_lang_ctx_t *ctx)
{
    mln_s32_t type;
    mln_lang_val_t *val;
    mln_string_t _this = mln_string("this");
    mln_string_t v = mln_string("fd");
    mln_string_t typename = mln_string("File");
    mln_lang_symbol_node_t *sym;
    mln_lang_var_t *var;
    mln_lang_var_t *ret_var;
    mln_rbtree_t *tree;
    mln_rbtree_node_t *rn;

    if ((sym = mln_lang_symbol_node_search(ctx, &_this, 1)) == NULL) {
        ASSERT(0);
        mln_lang_errmsg(ctx, "Lack of 'this'.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);

    type = mln_lang_var_val_type_get(sym->data.var);
    val = sym->data.var->val;
    if (type != M_LANG_VAL_TYPE_OBJECT) {
        mln_lang_errmsg(ctx, "'this' not object.");
        return NULL;
    }
    if (val->data.obj->in_set == NULL || val->data.obj->in_set->name == NULL || mln_string_strcmp(val->data.obj->in_set->name, &typename)) {
        mln_lang_errmsg(ctx, "Invalid set type, File object required.");
        return NULL;
    }
    if ((var = mln_lang_set_member_search(val->data.obj->members, &v)) == NULL) {
        mln_lang_errmsg(ctx, "Lack of member 'fd'.");
        return NULL;
    }
    if (mln_lang_var_val_type_get(var) != M_LANG_VAL_TYPE_INT) {
        mln_lang_errmsg(ctx, "Invalid type of 'fd'.");
        return NULL;
    }
    val = var->val;
    if (val->data.i < 0) {
        if (mln_lang_file_set_errno(ctx, EBADF) < 0) {
            return NULL;
        }
        if ((ret_var = mln_lang_var_create_false(ctx, NULL)) == NULL) {
            mln_lang_errmsg(ctx, "No memory.");
            return NULL;
        }
        return ret_var;
    }
    tree = mln_lang_ctx_resource_fetch(ctx, "file_fd");
#if defined(WIN32)
    int fd = val->data.i;
    rn = mln_rbtree_search(tree, tree->root, (void *)fd);
#else
    rn = mln_rbtree_search(tree, tree->root, (void *)(val->data.i));
#endif
    if (mln_rbtree_null(rn, tree)) {
        mln_lang_errmsg(ctx, "Invalid file descriptor.");
        return NULL;
    }
    mln_rbtree_delete(tree, rn);
    mln_rbtree_node_free(tree, rn);
    val->data.i = -1;
    if (mln_lang_file_set_errno(ctx, errno) < 0) {
        return NULL;
    }

    if ((ret_var = mln_lang_var_create_nil(ctx, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return NULL;
    }
    return ret_var;
}

static int mln_lang_file_add_errmsg(mln_lang_ctx_t *ctx, mln_lang_set_detail_t *set)
{
    mln_lang_val_t *val;
    mln_lang_var_t *var;
    mln_lang_func_detail_t *func;
    mln_string_t funcname = mln_string("errmsg");

    if ((func = mln_lang_func_detail_new(ctx, M_FUNC_INTERNAL, mln_lang_errmsg_process, NULL, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if ((val = mln_lang_val_new(ctx, M_LANG_VAL_TYPE_FUNC, func)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_func_detail_free(func);
        return -1;
    }
    if ((var = mln_lang_var_new(ctx, &funcname, M_LANG_VAR_NORMAL, val, set)) == NULL) {
        mln_lang_val_free(val);
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if (mln_lang_set_member_add(ctx->pool, set->members, var) < 0) {
        mln_lang_var_free(var);
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }

    return 0;
}

static mln_lang_var_t *mln_lang_errmsg_process(mln_lang_ctx_t *ctx)
{
    mln_s32_t t;
    mln_lang_val_t *val;
    mln_string_t _this = mln_string("this");
    mln_string_t v = mln_string("errno");
    mln_string_t typename = mln_string("File");
    mln_lang_symbol_node_t *sym;
    mln_lang_var_t *var;
    mln_lang_var_t *ret_var;
    mln_string_t tmp;
    mln_s8_t msg[512];

    if ((sym = mln_lang_symbol_node_search(ctx, &_this, 1)) == NULL) {
        ASSERT(0);
        mln_lang_errmsg(ctx, "Lack of 'this'.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);

    t = mln_lang_var_val_type_get(sym->data.var);
    val = sym->data.var->val;
    if (t != M_LANG_VAL_TYPE_OBJECT) {
        mln_lang_errmsg(ctx, "'this' not object.");
        return NULL;
    }
    if (val->data.obj->in_set == NULL || val->data.obj->in_set->name == NULL || mln_string_strcmp(val->data.obj->in_set->name, &typename)) {
        mln_lang_errmsg(ctx, "Invalid set type, File object required.");
        return NULL;
    }
    if ((var = mln_lang_set_member_search(val->data.obj->members, &v)) == NULL) {
        mln_lang_errmsg(ctx, "Lack of member 'errno'.");
        return NULL;
    }
    val = var->val;

    t = snprintf(msg, sizeof(msg)-1, "%s", strerror(val->data.i));
    msg[t] = 0;
    mln_string_nset(&tmp, msg, t);
    if ((ret_var = mln_lang_var_create_string(ctx, &tmp, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return NULL;
    }
    return ret_var;
}

static int mln_lang_file_set_errno(mln_lang_ctx_t *ctx, int err)
{
    mln_s32_t type;
    mln_lang_val_t *val;
    mln_string_t _this = mln_string("this");
    mln_string_t v = mln_string("errno");
    mln_lang_symbol_node_t *sym;
    mln_lang_var_t *var;

    if ((sym = mln_lang_symbol_node_search(ctx, &_this, 1)) == NULL) {
        ASSERT(0);
        mln_lang_errmsg(ctx, "Lack of 'this'.");
        return -1;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);

    type = mln_lang_var_val_type_get(sym->data.var);
    val = sym->data.var->val;
    if (type != M_LANG_VAL_TYPE_OBJECT) {
        mln_lang_errmsg(ctx, "'this' not object.");
        return -1;
    }
    if ((var = mln_lang_set_member_search(val->data.obj->members, &v)) == NULL) {
        mln_lang_errmsg(ctx, "Lack of member 'errno'.");
        return -1;
    }
    if (mln_lang_var_val_type_get(var) != M_LANG_VAL_TYPE_INT) {
        mln_lang_errmsg(ctx, "Invalid type of 'errno'.");
        return -1;
    }
    var->val->data.i = err;
    return 0;
}

static int mln_lang_file_add_size(mln_lang_ctx_t *ctx, mln_lang_set_detail_t *set)
{
    mln_lang_val_t *val;
    mln_lang_var_t *var;
    mln_lang_func_detail_t *func;
    mln_string_t funcname = mln_string("size");

    if ((func = mln_lang_func_detail_new(ctx, M_FUNC_INTERNAL, mln_lang_size_process, NULL, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if ((val = mln_lang_val_new(ctx, M_LANG_VAL_TYPE_FUNC, func)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_func_detail_free(func);
        return -1;
    }
    if ((var = mln_lang_var_new(ctx, &funcname, M_LANG_VAR_NORMAL, val, set)) == NULL) {
        mln_lang_val_free(val);
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if (mln_lang_set_member_add(ctx->pool, set->members, var) < 0) {
        mln_lang_var_free(var);
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }

    return 0;
}

static mln_lang_var_t *mln_lang_size_process(mln_lang_ctx_t *ctx)
{
    mln_s32_t t;
    mln_lang_val_t *val;
    mln_string_t _this = mln_string("this");
    mln_string_t v = mln_string("fd");
    mln_string_t typename = mln_string("File");
    mln_lang_symbol_node_t *sym;
    mln_lang_var_t *var;
    mln_lang_var_t *ret_var;
    struct stat st;

    if ((sym = mln_lang_symbol_node_search(ctx, &_this, 1)) == NULL) {
        ASSERT(0);
        mln_lang_errmsg(ctx, "Lack of 'this'.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);

    t = mln_lang_var_val_type_get(sym->data.var);
    val = sym->data.var->val;
    if (t != M_LANG_VAL_TYPE_OBJECT) {
        mln_lang_errmsg(ctx, "'this' not object.");
        return NULL;
    }
    if (val->data.obj->in_set == NULL || val->data.obj->in_set->name == NULL || mln_string_strcmp(val->data.obj->in_set->name, &typename)) {
        mln_lang_errmsg(ctx, "Invalid set type, File object required.");
        return NULL;
    }
    if ((var = mln_lang_set_member_search(val->data.obj->members, &v)) == NULL) {
        mln_lang_errmsg(ctx, "Lack of member 'fd'.");
        return NULL;
    }
    if (mln_lang_var_val_type_get(var) != M_LANG_VAL_TYPE_INT) {
        mln_lang_errmsg(ctx, "Invalid type of 'fd'.");
        return NULL;
    }
    val = var->val;
    if (val->data.i < 0) {
        if (mln_lang_file_set_errno(ctx, EBADF) < 0) {
            return NULL;
        }
        if ((ret_var = mln_lang_var_create_false(ctx, NULL)) == NULL) {
            mln_lang_errmsg(ctx, "No memory.");
            return NULL;
        }
        return ret_var;
    }

    memset(&st, 0, sizeof(st));
    t = fstat(val->data.i, &st);
    if (mln_lang_file_set_errno(ctx, errno) < 0) {
        return NULL;
    }
    if (t < 0) {
        if ((ret_var = mln_lang_var_create_false(ctx, NULL)) == NULL) {
            mln_lang_errmsg(ctx, "No memory.");
            return NULL;
        }
    } else {
        if ((ret_var = mln_lang_var_create_int(ctx, st.st_size, NULL)) == NULL) {
            mln_lang_errmsg(ctx, "No memory.");
            return NULL;
        }
    }
    return ret_var;
}

