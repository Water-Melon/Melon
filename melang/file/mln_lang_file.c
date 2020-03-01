
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

static inline void mln_lang_file_open_getPrio(mln_s64_t prio, mode_t *mode);
static int mln_lang_file_open_getOp(mln_string_t *op);
static int mln_lang_file_addFD(mln_lang_ctx_t *ctx, mln_lang_set_detail_t *set);
static int mln_lang_file_addErrno(mln_lang_ctx_t *ctx, mln_lang_set_detail_t *set);
static int mln_lang_file_addOpen(mln_lang_ctx_t *ctx, mln_lang_set_detail_t *set);
static int mln_lang_file_addLseek(mln_lang_ctx_t *ctx, mln_lang_set_detail_t *set);
static int mln_lang_file_addRead(mln_lang_ctx_t *ctx, mln_lang_set_detail_t *set);
static int mln_lang_file_addWrite(mln_lang_ctx_t *ctx, mln_lang_set_detail_t *set);
static int mln_lang_file_addClose(mln_lang_ctx_t *ctx, mln_lang_set_detail_t *set);
static int mln_lang_file_addErrmsg(mln_lang_ctx_t *ctx, mln_lang_set_detail_t *set);
static int mln_lang_file_addSize(mln_lang_ctx_t *ctx, mln_lang_set_detail_t *set);
static mln_lang_retExp_t *mln_lang_open_process(mln_lang_ctx_t *ctx);
static mln_lang_retExp_t *mln_lang_lseek_process(mln_lang_ctx_t *ctx);
static mln_lang_retExp_t *mln_lang_read_process(mln_lang_ctx_t *ctx);
static mln_lang_retExp_t *mln_lang_write_process(mln_lang_ctx_t *ctx);
static mln_lang_retExp_t *mln_lang_close_process(mln_lang_ctx_t *ctx);
static mln_lang_retExp_t *mln_lang_errmsg_process(mln_lang_ctx_t *ctx);
static mln_lang_retExp_t *mln_lang_size_process(mln_lang_ctx_t *ctx);
static int mln_lang_file_setErrno(mln_lang_ctx_t *ctx, int err);

int mln_lang_file(mln_lang_ctx_t *ctx)
{
    mln_string_t setname = mln_string("MFile");
    mln_lang_set_detail_t *set;

    if ((set = mln_lang_set_detail_new(ctx->pool, &setname)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    ++(set->ref);
    if (mln_lang_symbolNode_join(ctx, M_LANG_SYMBOL_SET, set) < 0) {
        mln_lang_set_detail_freeSelf(set);
        return -1;
    }

    if (mln_lang_file_addFD(ctx, set) < 0) {
        return -1;
    }
    if (mln_lang_file_addErrno(ctx, set) < 0) {
        return -1;
    }
    if (mln_lang_file_addOpen(ctx, set) < 0) {
        return -1;
    }
    if (mln_lang_file_addLseek(ctx, set) < 0) {
        return -1;
    }
    if (mln_lang_file_addRead(ctx, set) < 0) {
        return -1;
    }
    if (mln_lang_file_addWrite(ctx, set) < 0) {
        return -1;
    }
    if (mln_lang_file_addClose(ctx, set) < 0) {
        return -1;
    }
    if (mln_lang_file_addErrmsg(ctx, set) < 0) {
        return -1;
    }
    if (mln_lang_file_addSize(ctx, set) < 0) {
        return -1;
    }
    return 0;
}

static int mln_lang_file_addFD(mln_lang_ctx_t *ctx, mln_lang_set_detail_t *set)
{
    int fd = -1;
    mln_lang_val_t *val;
    mln_lang_var_t *var;
    mln_string_t varname = mln_string("fd");

    if ((val = mln_lang_val_new(ctx->pool, M_LANG_VAL_TYPE_INT, &fd)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if ((var = mln_lang_var_new(ctx->pool, &varname, M_LANG_VAR_NORMAL, val, set)) == NULL) {
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

static int mln_lang_file_addErrno(mln_lang_ctx_t *ctx, mln_lang_set_detail_t *set)
{
    int err = 0;
    mln_lang_val_t *val;
    mln_lang_var_t *var;
    mln_string_t varname = mln_string("errno");

    if ((val = mln_lang_val_new(ctx->pool, M_LANG_VAL_TYPE_INT, &err)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if ((var = mln_lang_var_new(ctx->pool, &varname, M_LANG_VAR_NORMAL, val, set)) == NULL) {
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

static int mln_lang_file_addOpen(mln_lang_ctx_t *ctx, mln_lang_set_detail_t *set)
{
    mln_lang_val_t *val;
    mln_lang_var_t *var;
    mln_lang_func_detail_t *func;
    mln_string_t funcname = mln_string("open");
    mln_string_t v1 = mln_string("path"), v2 = mln_string("op"), v3 = mln_string("prio");

    if ((func = mln_lang_func_detail_new(ctx->pool, M_FUNC_INTERNAL, mln_lang_open_process, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if ((val = mln_lang_val_new(ctx->pool, M_LANG_VAL_TYPE_NIL, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_func_detail_free(func);
        return -1;
    }
    if ((var = mln_lang_var_new(ctx->pool, &v1, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_val_free(val);
        mln_lang_func_detail_free(func);
        return -1;
    }
    mln_lang_var_chain_add(&(func->args_head), &(func->args_tail), var);
    ++func->nargs;
    if ((val = mln_lang_val_new(ctx->pool, M_LANG_VAL_TYPE_NIL, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_func_detail_free(func);
        return -1;
    }
    if ((var = mln_lang_var_new(ctx->pool, &v2, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_val_free(val);
        mln_lang_func_detail_free(func);
        return -1;
    }
    mln_lang_var_chain_add(&(func->args_head), &(func->args_tail), var);
    ++func->nargs;
    if ((val = mln_lang_val_new(ctx->pool, M_LANG_VAL_TYPE_NIL, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_func_detail_free(func);
        return -1;
    }
    if ((var = mln_lang_var_new(ctx->pool, &v3, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_val_free(val);
        mln_lang_func_detail_free(func);
        return -1;
    }
    mln_lang_var_chain_add(&(func->args_head), &(func->args_tail), var);
    ++func->nargs;

    if ((val = mln_lang_val_new(ctx->pool, M_LANG_VAL_TYPE_FUNC, func)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_func_detail_free(func);
        return -1;
    }
    if ((var = mln_lang_var_new(ctx->pool, &funcname, M_LANG_VAR_NORMAL, val, set)) == NULL) {
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

static mln_lang_retExp_t *mln_lang_open_process(mln_lang_ctx_t *ctx)
{
    mln_s32_t type, op;
    mln_lang_val_t *val1, *val2, *val3, *val;
    mln_string_t _this = mln_string("this");
    mln_string_t v1 = mln_string("path");
    mln_string_t v2 = mln_string("op");
    mln_string_t v3 = mln_string("prio");
    mln_string_t v = mln_string("fd");
    mln_string_t typename = mln_string("MFile");
    mln_lang_symbolNode_t *sym;
    mln_lang_var_t *var;
    mln_s8ptr_t path;
    mln_lang_retExp_t *retExp;
    mode_t mode;

    if ((sym = mln_lang_symbolNode_search(ctx, &_this, 1)) == NULL) {
        ASSERT(0);
        mln_lang_errmsg(ctx, "Lack of 'this'.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);

    type = mln_lang_var_getValType(sym->data.var);
    val = sym->data.var->val;
    if (type != M_LANG_VAL_TYPE_OBJECT) {
        mln_lang_errmsg(ctx, "'this' not object.");
        return NULL;
    }
    if (val->data.obj->inSet == NULL || mln_string_strcmp(val->data.obj->inSet->name, &typename)) {
        mln_lang_errmsg(ctx, "Invalid set type, MFile object required.");
        return NULL;
    }
    if ((var = mln_lang_set_member_search(val->data.obj->members, &v)) == NULL) {
        mln_lang_errmsg(ctx, "Lack of member 'fd'.");
        return NULL;
    }
    if (mln_lang_var_getValType(var) != M_LANG_VAL_TYPE_INT) {
        mln_lang_errmsg(ctx, "Invalid type of 'fd'.");
        return NULL;
    }
    val = var->val;

    /*arg1*/
    if ((sym = mln_lang_symbolNode_search(ctx, &v1, 1)) == NULL) {
        ASSERT(0);
        mln_lang_errmsg(ctx, "Argument missing.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);
    type = mln_lang_var_getValType(sym->data.var);
    val1 = sym->data.var->val;
    if (type != M_LANG_VAL_TYPE_STRING) {
        mln_lang_errmsg(ctx, "'path' demand string.");
        return NULL;
    }

    /*arg2*/
    if ((sym = mln_lang_symbolNode_search(ctx, &v2, 1)) == NULL) {
        ASSERT(0);
        mln_lang_errmsg(ctx, "Argument missing.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);
    type = mln_lang_var_getValType(sym->data.var);
    val2 = sym->data.var->val;
    if (type != M_LANG_VAL_TYPE_STRING) {
        mln_lang_errmsg(ctx, "'op' demand string.");
        return NULL;
    }

    /*arg3*/
    if ((sym = mln_lang_symbolNode_search(ctx, &v3, 1)) == NULL) {
        ASSERT(0);
        mln_lang_errmsg(ctx, "Argument missing.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);
    type = mln_lang_var_getValType(sym->data.var);
    val3 = sym->data.var->val;
    if (type != M_LANG_VAL_TYPE_INT) {
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
    if ((op = mln_lang_file_open_getOp(val2->data.s)) < 0) {
        mln_lang_errmsg(ctx, "Invalid 'op'.");
        free(path);
        return NULL;
    }
    memset(&mode, 0, sizeof(mode));
    mln_lang_file_open_getPrio(val3->data.i, &mode);
    val->data.i = open(path, op, mode);
    if (mln_lang_file_setErrno(ctx, errno) < 0) {
        free(path);
        return NULL;
    }
    free(path);
    if (val->data.i < 0) {
        retExp = mln_lang_retExp_createTmpFalse(ctx->pool, NULL);
    } else {
        mln_lang_val_setNotModify(val);
        retExp = mln_lang_retExp_createTmpTrue(ctx->pool, NULL);
    }
    if (retExp == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return NULL;
    }
    return retExp;
}

static inline void mln_lang_file_open_getPrio(mln_s64_t prio, mode_t *mode)
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

static int mln_lang_file_open_getOp(mln_string_t *op)
{
    int flags = 0, r = 0, w = 0, a = 0;
    mln_u8ptr_t p, pend;
    for (p = op->data, pend = op->data+op->len; p < pend; ++p) {
        if (*p == (mln_u8_t)'r') {
            r = 1;
        } else if (*p == (mln_u8_t)'w') {
            w = 1;
        } else if (*p == (mln_u8_t)'a') {
            a = 1;
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
    }
    return flags;
}

static int mln_lang_file_addLseek(mln_lang_ctx_t *ctx, mln_lang_set_detail_t *set)
{
    mln_lang_val_t *val;
    mln_lang_var_t *var;
    mln_lang_func_detail_t *func;
    mln_string_t funcname = mln_string("lseek");
    mln_string_t v1 = mln_string("offset"), v2 = mln_string("whence");

    if ((func = mln_lang_func_detail_new(ctx->pool, M_FUNC_INTERNAL, mln_lang_lseek_process, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if ((val = mln_lang_val_new(ctx->pool, M_LANG_VAL_TYPE_NIL, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_func_detail_free(func);
        return -1;
    }
    if ((var = mln_lang_var_new(ctx->pool, &v1, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_val_free(val);
        mln_lang_func_detail_free(func);
        return -1;
    }
    mln_lang_var_chain_add(&(func->args_head), &(func->args_tail), var);
    ++func->nargs;
    if ((val = mln_lang_val_new(ctx->pool, M_LANG_VAL_TYPE_NIL, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_func_detail_free(func);
        return -1;
    }
    if ((var = mln_lang_var_new(ctx->pool, &v2, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_val_free(val);
        mln_lang_func_detail_free(func);
        return -1;
    }
    mln_lang_var_chain_add(&(func->args_head), &(func->args_tail), var);
    ++func->nargs;

    if ((val = mln_lang_val_new(ctx->pool, M_LANG_VAL_TYPE_FUNC, func)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_func_detail_free(func);
        return -1;
    }
    if ((var = mln_lang_var_new(ctx->pool, &funcname, M_LANG_VAR_NORMAL, val, set)) == NULL) {
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

static mln_lang_retExp_t *mln_lang_lseek_process(mln_lang_ctx_t *ctx)
{
    mln_s32_t t, whence = 0;
    mln_lang_val_t *val1, *val2, *val;
    mln_string_t _this = mln_string("this");
    mln_string_t v1 = mln_string("offset");
    mln_string_t v2 = mln_string("whence");
    mln_string_t v = mln_string("fd");
    mln_string_t typename = mln_string("MFile");
    mln_string_t *tmp;
    mln_lang_symbolNode_t *sym;
    mln_lang_var_t *var;
    mln_lang_retExp_t *retExp;

    if ((sym = mln_lang_symbolNode_search(ctx, &_this, 1)) == NULL) {
        ASSERT(0);
        mln_lang_errmsg(ctx, "Lack of 'this'.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);

    t = mln_lang_var_getValType(sym->data.var);
    val = sym->data.var->val;
    if (t != M_LANG_VAL_TYPE_OBJECT) {
        mln_lang_errmsg(ctx, "'this' not object.");
        return NULL;
    }
    if (val->data.obj->inSet == NULL || mln_string_strcmp(val->data.obj->inSet->name, &typename)) {
        mln_lang_errmsg(ctx, "Invalid set type, MFile object required.");
        return NULL;
    }
    if ((var = mln_lang_set_member_search(val->data.obj->members, &v)) == NULL) {
        mln_lang_errmsg(ctx, "Lack of member 'fd'.");
        return NULL;
    }
    if (mln_lang_var_getValType(var) != M_LANG_VAL_TYPE_INT) {
        mln_lang_errmsg(ctx, "Invalid type of 'fd'.");
        return NULL;
    }
    val = var->val;

    /*arg1*/
    if ((sym = mln_lang_symbolNode_search(ctx, &v1, 1)) == NULL) {
        ASSERT(0);
        mln_lang_errmsg(ctx, "Argument missing.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);
    t = mln_lang_var_getValType(sym->data.var);
    val1 = sym->data.var->val;
    if (t != M_LANG_VAL_TYPE_INT) {
        mln_lang_errmsg(ctx, "'offset' demand integer.");
        return NULL;
    }
    if (val1->data.i < 0) {
        if (mln_lang_file_setErrno(ctx, EBADF) < 0) {
            return NULL;
        }
        if ((retExp = mln_lang_retExp_createTmpFalse(ctx->pool, NULL)) == NULL) {
            mln_lang_errmsg(ctx, "No memory.");
            return NULL;
        }
        return retExp;
    }

    /*arg2*/
    if ((sym = mln_lang_symbolNode_search(ctx, &v2, 1)) == NULL) {
        ASSERT(0);
        mln_lang_errmsg(ctx, "Argument missing.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);
    t = mln_lang_var_getValType(sym->data.var);
    val2 = sym->data.var->val;
    if (t != M_LANG_VAL_TYPE_STRING) {
        mln_lang_errmsg(ctx, "'whence' demand string.");
        return NULL;
    }
    if ((tmp = val2->data.s) == NULL) {
        mln_lang_errmsg(ctx, "Invalid argument.");
        return NULL;
    }
    if (!mln_string_constStrcmp(tmp, "begin")) {
        whence = SEEK_SET;
    } else if (!mln_string_constStrcmp(tmp, "current")) {
        whence = SEEK_CUR;
    } else if (!mln_string_constStrcmp(tmp, "end")) {
        whence = SEEK_END;
    } else {
        mln_lang_errmsg(ctx, "Invalid argument 'whence'.");
        return NULL;
    }

    t = lseek(val->data.i, val1->data.i, whence);
    if (mln_lang_file_setErrno(ctx, errno) < 0) {
        return NULL;
    }
    if (t < 0) {
        retExp = mln_lang_retExp_createTmpFalse(ctx->pool, NULL);
    } else {
        retExp = mln_lang_retExp_createTmpTrue(ctx->pool, NULL);
    }
    if (retExp == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return NULL;
    }
    return retExp;
}

static int mln_lang_file_addRead(mln_lang_ctx_t *ctx, mln_lang_set_detail_t *set)
{
    mln_lang_val_t *val;
    mln_lang_var_t *var;
    mln_lang_func_detail_t *func;
    mln_string_t funcname = mln_string("read");
    mln_string_t v1 = mln_string("nbytes");

    if ((func = mln_lang_func_detail_new(ctx->pool, M_FUNC_INTERNAL, mln_lang_read_process, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if ((val = mln_lang_val_new(ctx->pool, M_LANG_VAL_TYPE_NIL, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_func_detail_free(func);
        return -1;
    }
    if ((var = mln_lang_var_new(ctx->pool, &v1, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_val_free(val);
        mln_lang_func_detail_free(func);
        return -1;
    }
    mln_lang_var_chain_add(&(func->args_head), &(func->args_tail), var);
    ++func->nargs;

    if ((val = mln_lang_val_new(ctx->pool, M_LANG_VAL_TYPE_FUNC, func)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_func_detail_free(func);
        return -1;
    }
    if ((var = mln_lang_var_new(ctx->pool, &funcname, M_LANG_VAR_NORMAL, val, set)) == NULL) {
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

static mln_lang_retExp_t *mln_lang_read_process(mln_lang_ctx_t *ctx)
{
    mln_s32_t t;
    ssize_t n;
    mln_lang_val_t *val1, *val;
    mln_string_t _this = mln_string("this");
    mln_string_t v1 = mln_string("nbytes");
    mln_string_t v = mln_string("fd");
    mln_string_t typename = mln_string("MFile");
    mln_string_t tmp;
    mln_lang_symbolNode_t *sym;
    mln_lang_var_t *var;
    mln_lang_retExp_t *retExp;
    mln_u8ptr_t buf;

    if ((sym = mln_lang_symbolNode_search(ctx, &_this, 1)) == NULL) {
        ASSERT(0);
        mln_lang_errmsg(ctx, "Lack of 'this'.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);

    t = mln_lang_var_getValType(sym->data.var);
    val = sym->data.var->val;
    if (t != M_LANG_VAL_TYPE_OBJECT) {
        mln_lang_errmsg(ctx, "'this' not object.");
        return NULL;
    }
    if (val->data.obj->inSet == NULL || mln_string_strcmp(val->data.obj->inSet->name, &typename)) {
        mln_lang_errmsg(ctx, "Invalid set type, MFile object required.");
        return NULL;
    }
    if ((var = mln_lang_set_member_search(val->data.obj->members, &v)) == NULL) {
        mln_lang_errmsg(ctx, "Lack of member 'fd'.");
        return NULL;
    }
    if (mln_lang_var_getValType(var) != M_LANG_VAL_TYPE_INT) {
        mln_lang_errmsg(ctx, "Invalid type of 'fd'.");
        return NULL;
    }
    val = var->val;
    if (val->data.i < 0) {
        if (mln_lang_file_setErrno(ctx, EBADF) < 0) {
            return NULL;
        }
        if ((retExp = mln_lang_retExp_createTmpFalse(ctx->pool, NULL)) == NULL) {
            mln_lang_errmsg(ctx, "No memory.");
            return NULL;
        }
        return retExp;
    }

    /*arg1*/
    if ((sym = mln_lang_symbolNode_search(ctx, &v1, 1)) == NULL) {
        ASSERT(0);
        mln_lang_errmsg(ctx, "Argument missing.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);
    if (mln_lang_var_getValType(sym->data.var) != M_LANG_VAL_TYPE_INT) {
        mln_lang_errmsg(ctx, "'nbytes' demand integer.");
        return NULL;
    }
    val1 = sym->data.var->val;
    if (val1->data.i <= 0) {
        if ((retExp = mln_lang_retExp_createTmpNil(ctx->pool, NULL)) == NULL) {
            mln_lang_errmsg(ctx, "No memory.");
            return NULL;
        }
        return retExp;
    }

    if ((buf = (mln_u8ptr_t)malloc(val1->data.i)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return NULL;
    }
    n = read(val->data.i, buf, val1->data.i);
    if (mln_lang_file_setErrno(ctx, errno) < 0) {
        free(buf);
        return NULL;
    }
    if (n < 0) {
        free(buf);
        retExp = mln_lang_retExp_createTmpFalse(ctx->pool, NULL);
    } else {
        mln_string_nSet(&tmp, buf, n);
        retExp = mln_lang_retExp_createTmpString(ctx->pool, &tmp, NULL);
        free(buf);
    }
    if (retExp == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return NULL;
    }
    return retExp;
}

static int mln_lang_file_addWrite(mln_lang_ctx_t *ctx, mln_lang_set_detail_t *set)
{
    mln_lang_val_t *val;
    mln_lang_var_t *var;
    mln_lang_func_detail_t *func;
    mln_string_t funcname = mln_string("write");
    mln_string_t v1 = mln_string("buf");

    if ((func = mln_lang_func_detail_new(ctx->pool, M_FUNC_INTERNAL, mln_lang_write_process, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if ((val = mln_lang_val_new(ctx->pool, M_LANG_VAL_TYPE_NIL, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_func_detail_free(func);
        return -1;
    }
    if ((var = mln_lang_var_new(ctx->pool, &v1, M_LANG_VAR_NORMAL, val, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_val_free(val);
        mln_lang_func_detail_free(func);
        return -1;
    }
    mln_lang_var_chain_add(&(func->args_head), &(func->args_tail), var);
    ++func->nargs;

    if ((val = mln_lang_val_new(ctx->pool, M_LANG_VAL_TYPE_FUNC, func)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_func_detail_free(func);
        return -1;
    }
    if ((var = mln_lang_var_new(ctx->pool, &funcname, M_LANG_VAR_NORMAL, val, set)) == NULL) {
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

static mln_lang_retExp_t *mln_lang_write_process(mln_lang_ctx_t *ctx)
{
    mln_s32_t t;
    ssize_t n;
    mln_lang_val_t *val1, *val;
    mln_string_t _this = mln_string("this");
    mln_string_t v1 = mln_string("buf");
    mln_string_t v = mln_string("fd");
    mln_string_t typename = mln_string("MFile");
    mln_lang_symbolNode_t *sym;
    mln_lang_var_t *var;
    mln_lang_retExp_t *retExp;

    if ((sym = mln_lang_symbolNode_search(ctx, &_this, 1)) == NULL) {
        ASSERT(0);
        mln_lang_errmsg(ctx, "Lack of 'this'.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);

    t = mln_lang_var_getValType(sym->data.var);
    val = sym->data.var->val;
    if (t != M_LANG_VAL_TYPE_OBJECT) {
        mln_lang_errmsg(ctx, "'this' not object.");
        return NULL;
    }
    if (val->data.obj->inSet == NULL || mln_string_strcmp(val->data.obj->inSet->name, &typename)) {
        mln_lang_errmsg(ctx, "Invalid set type, MFile object required.");
        return NULL;
    }
    if ((var = mln_lang_set_member_search(val->data.obj->members, &v)) == NULL) {
        mln_lang_errmsg(ctx, "Lack of member 'fd'.");
        return NULL;
    }
    if (mln_lang_var_getValType(var) != M_LANG_VAL_TYPE_INT) {
        mln_lang_errmsg(ctx, "Invalid type of 'fd'.");
        return NULL;
    }
    val = var->val;
    if (val->data.i < 0) {
        if (mln_lang_file_setErrno(ctx, EBADF) < 0) {
            return NULL;
        }
        if ((retExp = mln_lang_retExp_createTmpFalse(ctx->pool, NULL)) == NULL) {
            mln_lang_errmsg(ctx, "No memory.");
            return NULL;
        }
        return retExp;
    }

    /*arg1*/
    if ((sym = mln_lang_symbolNode_search(ctx, &v1, 1)) == NULL) {
        ASSERT(0);
        mln_lang_errmsg(ctx, "Argument missing.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);
    if (mln_lang_var_getValType(sym->data.var) != M_LANG_VAL_TYPE_STRING) {
        mln_lang_errmsg(ctx, "'buf' demand string.");
        return NULL;
    }
    val1 = sym->data.var->val;
    if (val1->data.s == NULL) {
        mln_lang_errmsg(ctx, "Invalid argument.");
        return NULL;
    }

    n = write(val->data.i, val1->data.s->data, val1->data.s->len);
    if (mln_lang_file_setErrno(ctx, errno) < 0) {
        return NULL;
    }
    if (n < 0) {
        retExp = mln_lang_retExp_createTmpFalse(ctx->pool, NULL);
    } else {
        retExp = mln_lang_retExp_createTmpInt(ctx->pool, n, NULL);
    }
    if (retExp == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return NULL;
    }
    return retExp;
}

static int mln_lang_file_addClose(mln_lang_ctx_t *ctx, mln_lang_set_detail_t *set)
{
    mln_lang_val_t *val;
    mln_lang_var_t *var;
    mln_lang_func_detail_t *func;
    mln_string_t funcname = mln_string("close");

    if ((func = mln_lang_func_detail_new(ctx->pool, M_FUNC_INTERNAL, mln_lang_close_process, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if ((val = mln_lang_val_new(ctx->pool, M_LANG_VAL_TYPE_FUNC, func)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_func_detail_free(func);
        return -1;
    }
    if ((var = mln_lang_var_new(ctx->pool, &funcname, M_LANG_VAR_NORMAL, val, set)) == NULL) {
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

static mln_lang_retExp_t *mln_lang_close_process(mln_lang_ctx_t *ctx)
{
    mln_s32_t type;
    mln_lang_val_t *val;
    mln_string_t _this = mln_string("this");
    mln_string_t v = mln_string("fd");
    mln_string_t typename = mln_string("MFile");
    mln_lang_symbolNode_t *sym;
    mln_lang_var_t *var;
    mln_lang_retExp_t *retExp;

    if ((sym = mln_lang_symbolNode_search(ctx, &_this, 1)) == NULL) {
        ASSERT(0);
        mln_lang_errmsg(ctx, "Lack of 'this'.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);

    type = mln_lang_var_getValType(sym->data.var);
    val = sym->data.var->val;
    if (type != M_LANG_VAL_TYPE_OBJECT) {
        mln_lang_errmsg(ctx, "'this' not object.");
        return NULL;
    }
    if (val->data.obj->inSet == NULL || mln_string_strcmp(val->data.obj->inSet->name, &typename)) {
        mln_lang_errmsg(ctx, "Invalid set type, MFile object required.");
        return NULL;
    }
    if ((var = mln_lang_set_member_search(val->data.obj->members, &v)) == NULL) {
        mln_lang_errmsg(ctx, "Lack of member 'fd'.");
        return NULL;
    }
    if (mln_lang_var_getValType(var) != M_LANG_VAL_TYPE_INT) {
        mln_lang_errmsg(ctx, "Invalid type of 'fd'.");
        return NULL;
    }
    val = var->val;
    if (val->data.i < 0) {
        if (mln_lang_file_setErrno(ctx, EBADF) < 0) {
            return NULL;
        }
        if ((retExp = mln_lang_retExp_createTmpFalse(ctx->pool, NULL)) == NULL) {
            mln_lang_errmsg(ctx, "No memory.");
            return NULL;
        }
        return retExp;
    }
    close(val->data.i);
    val->data.i = -1;
    if (mln_lang_file_setErrno(ctx, errno) < 0) {
        return NULL;
    }

    if ((retExp = mln_lang_retExp_createTmpNil(ctx->pool, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return NULL;
    }
    return retExp;
}

static int mln_lang_file_addErrmsg(mln_lang_ctx_t *ctx, mln_lang_set_detail_t *set)
{
    mln_lang_val_t *val;
    mln_lang_var_t *var;
    mln_lang_func_detail_t *func;
    mln_string_t funcname = mln_string("errmsg");

    if ((func = mln_lang_func_detail_new(ctx->pool, M_FUNC_INTERNAL, mln_lang_errmsg_process, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if ((val = mln_lang_val_new(ctx->pool, M_LANG_VAL_TYPE_FUNC, func)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_func_detail_free(func);
        return -1;
    }
    if ((var = mln_lang_var_new(ctx->pool, &funcname, M_LANG_VAR_NORMAL, val, set)) == NULL) {
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

static mln_lang_retExp_t *mln_lang_errmsg_process(mln_lang_ctx_t *ctx)
{
    mln_s32_t t;
    mln_lang_val_t *val;
    mln_string_t _this = mln_string("this");
    mln_string_t v = mln_string("errno");
    mln_string_t typename = mln_string("MFile");
    mln_lang_symbolNode_t *sym;
    mln_lang_var_t *var;
    mln_lang_retExp_t *retExp;
    mln_string_t tmp;
    mln_s8_t msg[512];

    if ((sym = mln_lang_symbolNode_search(ctx, &_this, 1)) == NULL) {
        ASSERT(0);
        mln_lang_errmsg(ctx, "Lack of 'this'.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);

    t = mln_lang_var_getValType(sym->data.var);
    val = sym->data.var->val;
    if (t != M_LANG_VAL_TYPE_OBJECT) {
        mln_lang_errmsg(ctx, "'this' not object.");
        return NULL;
    }
    if (val->data.obj->inSet == NULL || mln_string_strcmp(val->data.obj->inSet->name, &typename)) {
        mln_lang_errmsg(ctx, "Invalid set type, MFile object required.");
        return NULL;
    }
    if ((var = mln_lang_set_member_search(val->data.obj->members, &v)) == NULL) {
        mln_lang_errmsg(ctx, "Lack of member 'errno'.");
        return NULL;
    }
    val = var->val;

    t = snprintf(msg, sizeof(msg)-1, "%s", strerror(val->data.i));
    msg[t] = 0;
    mln_string_nSet(&tmp, msg, t);
    if ((retExp = mln_lang_retExp_createTmpString(ctx->pool, &tmp, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return NULL;
    }
    return retExp;
}

static int mln_lang_file_setErrno(mln_lang_ctx_t *ctx, int err)
{
    mln_s32_t type;
    mln_lang_val_t *val;
    mln_string_t _this = mln_string("this");
    mln_string_t v = mln_string("errno");
    mln_lang_symbolNode_t *sym;
    mln_lang_var_t *var;

    if ((sym = mln_lang_symbolNode_search(ctx, &_this, 1)) == NULL) {
        ASSERT(0);
        mln_lang_errmsg(ctx, "Lack of 'this'.");
        return -1;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);

    type = mln_lang_var_getValType(sym->data.var);
    val = sym->data.var->val;
    if (type != M_LANG_VAL_TYPE_OBJECT) {
        mln_lang_errmsg(ctx, "'this' not object.");
        return -1;
    }
    if ((var = mln_lang_set_member_search(val->data.obj->members, &v)) == NULL) {
        mln_lang_errmsg(ctx, "Lack of member 'errno'.");
        return -1;
    }
    if (mln_lang_var_getValType(var) != M_LANG_VAL_TYPE_INT) {
        mln_lang_errmsg(ctx, "Invalid type of 'errno'.");
        return -1;
    }
    var->val->data.i = err;
    return 0;
}

static int mln_lang_file_addSize(mln_lang_ctx_t *ctx, mln_lang_set_detail_t *set)
{
    mln_lang_val_t *val;
    mln_lang_var_t *var;
    mln_lang_func_detail_t *func;
    mln_string_t funcname = mln_string("size");

    if ((func = mln_lang_func_detail_new(ctx->pool, M_FUNC_INTERNAL, mln_lang_size_process, NULL)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    if ((val = mln_lang_val_new(ctx->pool, M_LANG_VAL_TYPE_FUNC, func)) == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_func_detail_free(func);
        return -1;
    }
    if ((var = mln_lang_var_new(ctx->pool, &funcname, M_LANG_VAR_NORMAL, val, set)) == NULL) {
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

static mln_lang_retExp_t *mln_lang_size_process(mln_lang_ctx_t *ctx)
{
    mln_s32_t t;
    mln_lang_val_t *val;
    mln_string_t _this = mln_string("this");
    mln_string_t v = mln_string("fd");
    mln_string_t typename = mln_string("MFile");
    mln_lang_symbolNode_t *sym;
    mln_lang_var_t *var;
    mln_lang_retExp_t *retExp;
    struct stat st;

    if ((sym = mln_lang_symbolNode_search(ctx, &_this, 1)) == NULL) {
        ASSERT(0);
        mln_lang_errmsg(ctx, "Lack of 'this'.");
        return NULL;
    }
    ASSERT(sym->type == M_LANG_SYMBOL_VAR);

    t = mln_lang_var_getValType(sym->data.var);
    val = sym->data.var->val;
    if (t != M_LANG_VAL_TYPE_OBJECT) {
        mln_lang_errmsg(ctx, "'this' not object.");
        return NULL;
    }
    if (val->data.obj->inSet == NULL || mln_string_strcmp(val->data.obj->inSet->name, &typename)) {
        mln_lang_errmsg(ctx, "Invalid set type, MFile object required.");
        return NULL;
    }
    if ((var = mln_lang_set_member_search(val->data.obj->members, &v)) == NULL) {
        mln_lang_errmsg(ctx, "Lack of member 'fd'.");
        return NULL;
    }
    if (mln_lang_var_getValType(var) != M_LANG_VAL_TYPE_INT) {
        mln_lang_errmsg(ctx, "Invalid type of 'fd'.");
        return NULL;
    }
    val = var->val;
    if (val->data.i < 0) {
        if (mln_lang_file_setErrno(ctx, EBADF) < 0) {
            return NULL;
        }
        if ((retExp = mln_lang_retExp_createTmpFalse(ctx->pool, NULL)) == NULL) {
            mln_lang_errmsg(ctx, "No memory.");
            return NULL;
        }
        return retExp;
    }

    memset(&st, 0, sizeof(st));
    t = fstat(val->data.i, &st);
    if (mln_lang_file_setErrno(ctx, errno) < 0) {
        return NULL;
    }
    if (t < 0) {
        if ((retExp = mln_lang_retExp_createTmpFalse(ctx->pool, NULL)) == NULL) {
            mln_lang_errmsg(ctx, "No memory.");
            return NULL;
        }
    } else {
        if ((retExp = mln_lang_retExp_createTmpInt(ctx->pool, st.st_size, NULL)) == NULL) {
            mln_lang_errmsg(ctx, "No memory.");
            return NULL;
        }
    }
    return retExp;
}

