
/*
 * Copyright (C) Niklaus F.Schen.
 */

#include "mln_expr.h"
#include "mln_lex.h"

static mln_string_t keywords[] = {
    mln_string("true"),
    mln_string("false"),
    mln_string("null"),
    mln_string("if"),
    mln_string("then"),
    mln_string("else"),
    mln_string("fi"),
    mln_string("loop"),
    mln_string("do"),
    mln_string("end"),
    mln_string(NULL)
};

MLN_DEFINE_TOKEN_TYPE_AND_STRUCT(static, \
                                 mln_expr, \
                                 EXPR, \
                                 EXPR_TK_TRUE, \
                                 EXPR_TK_FALSE, \
                                 EXPR_TK_NULL, \
                                 EXPR_TK_IF, \
                                 EXPR_TK_THEN, \
                                 EXPR_TK_ELSE, \
                                 EXPR_TK_FI, \
                                 EXPR_TK_LOOP, \
                                 EXPR_TK_DO, \
                                 EXPR_TK_END, \
                                 EXPR_TK_STRING);
MLN_DEFINE_TOKEN(static, \
                 mln_expr, \
                 EXPR, \
                 {EXPR_TK_TRUE, "EXPR_TK_TRUE"}, \
                 {EXPR_TK_FALSE, "EXPR_TK_FALSE"}, \
                 {EXPR_TK_NULL, "EXPR_TK_NULL"}, \
                 {EXPR_TK_IF, "EXPR_TK_IF"}, \
                 {EXPR_TK_THEN, "EXPR_TK_THEN"}, \
                 {EXPR_TK_ELSE, "EXPR_TK_ELSE"}, \
                 {EXPR_TK_FI, "EXPR_TK_FI"}, \
                 {EXPR_TK_LOOP, "EXPR_TK_LOOP"}, \
                 {EXPR_TK_DO, "EXPR_TK_DO"}, \
                 {EXPR_TK_END, "EXPR_TK_END"}, \
                 {EXPR_TK_STRING, "EXPR_TK_STRING"});

static inline int
mln_expr_parse_if(mln_lex_t *lex, mln_expr_cb_t cb, void *data, mln_expr_val_t *ret, int *eof, mln_expr_struct_t **next);
static inline int
mln_expr_parse_loop(mln_lex_t *lex, mln_expr_cb_t cb, void *data, mln_expr_val_t *ret, int *eof, mln_expr_struct_t **next);

MLN_FUNC(static inline, int, mln_get_char, (mln_lex_t *lex, char c), (lex, c), {
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
})

MLN_FUNC(static, mln_expr_struct_t *, mln_expr_dblq_handler, (mln_lex_t *lex, void *data), (lex, data), {
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
        if (mln_get_char(lex, c) < 0) return NULL;
    }
    return mln_expr_new(lex, EXPR_TK_STRING);
})

MLN_FUNC(static, mln_expr_struct_t *, mln_expr_sglq_handler, (mln_lex_t *lex, void *data), (lex, data), {
    mln_lex_result_clean(lex);
    char c;
    while ( 1 ) {
        c = mln_lex_getchar(lex);
        if (c == MLN_ERR) return NULL;
        if (c == MLN_EOF) {
            mln_lex_error_set(lex, MLN_LEX_EINVEOF);
            return NULL;
        }
        if (c == '\'') break;
        if (mln_get_char(lex, c) < 0) return NULL;
    }
    return mln_expr_new(lex, EXPR_TK_STRING);
})

static inline int mln_expr_val_init(mln_expr_val_t *v, mln_expr_struct_t *token)
{
    char num[1024];
    mln_size_t len = token->text->len >= sizeof(num)-1? sizeof(num)-1: token->text->len;
    memcpy(num, token->text->data, len);
    num[len] = 0;

    switch (token->type) {
        case EXPR_TK_OCT:
            v->type = mln_expr_type_int;
#if defined(MSVC) || defined(i386) || defined(__arm__) || defined(__wasm__)
            sscanf(num, "%llo", &(v->data.i));
#else
            sscanf(num, "%lo", &(v->data.i));
#endif
            break;
        case EXPR_TK_DEC:
            v->type = mln_expr_type_int;
#if defined(MSVC) || defined(i386) || defined(__arm__) || defined(__wasm__)
        sscanf(num, "%lld", &(v->data.i));
#else
        sscanf(num, "%ld", &(v->data.i));
#endif
            break;
        case EXPR_TK_HEX:
            v->type = mln_expr_type_int;
#if defined(MSVC) || defined(i386) || defined(__arm__) || defined(__wasm__)
            sscanf(num, "%llx", &(v->data.i));
#else
            sscanf(num, "%lx", &(v->data.i));
#endif
            break;
        case EXPR_TK_REAL:
            v->type = mln_expr_type_real;
            sscanf(num, "%lf", &(v->data.r));
            break;
        case EXPR_TK_STRING:
            v->type = mln_expr_type_string;
            v->data.s = mln_string_ref(token->text);
            break;
        case EXPR_TK_TRUE:
            v->type = mln_expr_type_bool;
            v->data.b = 1;
            break;
        case EXPR_TK_FALSE:
            v->type = mln_expr_type_bool;
            v->data.b = 0;
            break;
        case EXPR_TK_NULL:
            v->type = mln_expr_type_null;
            break;
        default:
            return -1;
    }
    v->free = NULL;

    return 0;
}

MLN_FUNC(, mln_expr_val_t *, mln_expr_val_new, (mln_expr_typ_t type, void *data, mln_expr_udata_free free), (type, data, free), {
    mln_expr_val_t *ev;

    if ((ev = (mln_expr_val_t *)malloc(sizeof(mln_expr_val_t))) == NULL) return NULL;

    ev->type = type;
    switch (type) {
        case mln_expr_type_null:
            ev->free = NULL;
            break;
        case mln_expr_type_bool:
            ev->data.b = *((mln_u8ptr_t)data);
            ev->free = NULL;
            break;
        case mln_expr_type_int:
            ev->data.i = *((mln_s64_t *)data);
            ev->free = NULL;
            break;
        case mln_expr_type_real:
            ev->data.r = *((double *)data);
            ev->free = NULL;
            break;
        case mln_expr_type_string:
            ev->data.s = mln_string_ref((mln_string_t *)data);
            ev->free = free;
            break;
        default: /* mln_expr_type_udata */
            ev->data.u = data;
            ev->free = free;
            break;
    }
    return ev;
})

MLN_FUNC_VOID(, void, mln_expr_val_free, (mln_expr_val_t *ev), (ev), {
    if (ev == NULL) return;

    if (ev->type == mln_expr_type_string) {
        if (ev->free != NULL) ev->free(ev->data.s);
        else mln_string_free(ev->data.s);
    } else if (ev->type == mln_expr_type_udata) {
        if (ev->free != NULL) ev->free(ev->data.u);
    }

    free(ev);
})

MLN_FUNC_VOID(static, void, mln_expr_val_destroy, (mln_expr_val_t *ev), (ev), {
    if (ev->type == mln_expr_type_string) {
        if (ev->free != NULL) ev->free(ev->data.s);
        else mln_string_free(ev->data.s);
    } else if (ev->type == mln_expr_type_udata) {
        if (ev->free != NULL) ev->free(ev->data.u);
    }
})

MLN_FUNC(, mln_expr_val_t *, mln_expr_val_dup, (mln_expr_val_t *val), (val), {
    mln_expr_val_t *v;

    if ((v = (mln_expr_val_t *)malloc(sizeof(mln_expr_val_t))) == NULL) return NULL;
    v->type = val->type;
    switch (val->type) {
        case mln_expr_type_null:
            break;
        case mln_expr_type_bool:
            v->data.b = val->data.b;
            break;
        case mln_expr_type_int:
            v->data.i = val->data.i;
            break;
        case mln_expr_type_real:
            v->data.r = val->data.r;
            break;
        case mln_expr_type_string:
            v->data.s = mln_string_ref(val->data.s);
            v->free = val->free;
            break;
        default: /* mln_expr_type_udata */
            v->data.u = val->data.u;
            v->free = val->free;
            val->free = NULL;
            break;
    }
    return v;
})

MLN_FUNC_VOID(, void, mln_expr_val_copy, (mln_expr_val_t *dest, mln_expr_val_t *src), (dest, src), {
    if (src == NULL) return;

    dest->type = src->type;
    switch (src->type) {
        case mln_expr_type_null:
            break;
        case mln_expr_type_bool:
            dest->data.b = src->data.b;
            break;
        case mln_expr_type_int:
            dest->data.i = src->data.i;
            break;
        case mln_expr_type_real:
            dest->data.r = src->data.r;
            break;
        case mln_expr_type_string:
            dest->data.s = mln_string_ref(src->data.s);
            dest->free = src->free;
            break;
        default: /* mln_expr_type_udata */
            dest->data.u = src->data.u;
            dest->free = src->free;
            src->free = NULL;
            break;
    }
})

MLN_FUNC(static inline, int, mln_expr_parse, \
         (mln_lex_t *lex, mln_expr_cb_t cb, void *data, mln_expr_val_t *ret, int *eof, mln_expr_struct_t **next), \
         (lex, cb, data, ret, eof, next), \
{
    int rc;
    enum mln_expr_enum type;
    mln_expr_struct_t *name, *tk;
    mln_array_t arr;
    mln_expr_val_t *v;
    mln_string_t *tmp, *namespace = NULL;

again:
    if ((name = *next) != NULL) {
        *next = NULL;
    } else {
        if ((name = mln_expr_token(lex)) == NULL) {
            if (name != NULL) mln_expr_free(name);
            if (namespace != NULL) mln_string_free(namespace);
            return MLN_EXPR_RET_ERR;
        }
    }
    if ((type = name->type) == EXPR_TK_EOF) {
        *eof = 1;
        if (name != NULL) mln_expr_free(name);
        if (namespace != NULL) mln_string_free(namespace);
        return MLN_EXPR_RET_OK;
    }
    if (type == EXPR_TK_SPACE || type == EXPR_TK_COLON) {
        if (name != NULL) mln_expr_free(name);
        goto again;
    }
    if (type == EXPR_TK_COMMA) {
        if (name != NULL) mln_expr_free(name);
        if (namespace != NULL) mln_string_free(namespace);
        return MLN_EXPR_RET_COMMA;
    }
    if (type == EXPR_TK_RPAR) {
        if (name != NULL) mln_expr_free(name);
        if (namespace != NULL) mln_string_free(namespace);
        return MLN_EXPR_RET_RPAR;
    }
    if (type == EXPR_TK_IF) {
        if (name != NULL) mln_expr_free(name);
        if (namespace != NULL) mln_string_free(namespace);
        return mln_expr_parse_if(lex, cb, data, ret, eof, next);
    }
    if (type == EXPR_TK_LOOP) {
        if (name != NULL) mln_expr_free(name);
        if (namespace != NULL) mln_string_free(namespace);
        return mln_expr_parse_loop(lex, cb, data, ret, eof, next);
    }
    if (type != EXPR_TK_ID) {
        rc = mln_expr_val_init(ret, name);
        if (name != NULL) mln_expr_free(name);
        if (namespace != NULL) mln_string_free(namespace);
        return rc < 0? MLN_EXPR_RET_ERR: MLN_EXPR_RET_OK;
    }

again2:
    if ((tk = mln_expr_token(lex)) == NULL) {
        if (name != NULL) mln_expr_free(name);
        if (namespace != NULL) mln_string_free(namespace);
        return MLN_EXPR_RET_ERR;
    }
    if ((type = tk->type) == EXPR_TK_SPACE) {
        mln_expr_free(tk);
        goto again2;
    }
    if (type == EXPR_TK_COLON) {
        tmp = mln_string_pool_concat(mln_lex_get_pool(lex), namespace, name->text, tk->text);
        mln_expr_free(tk);
        if (name != NULL) mln_expr_free(name);
        if (namespace != NULL) mln_string_free(namespace);
        namespace = tmp;
        name = NULL;
        goto again;
    }
    if (type != EXPR_TK_LPAR) {
        v = cb(namespace, name->text, 0, NULL, data);
        mln_expr_val_copy(ret, v);
        mln_expr_val_free(v);
        if (name != NULL) mln_expr_free(name);
        if (namespace != NULL) mln_string_free(namespace);
        if ((type = tk->type) == EXPR_TK_EOF || type == EXPR_TK_COMMA) {
            mln_expr_free(tk);
        } else {
            *next = tk;
        }
        return v == NULL? MLN_EXPR_RET_ERR: MLN_EXPR_RET_OK;
    }
    mln_expr_free(tk);

    if (mln_array_init(&arr, (array_free)mln_expr_val_destroy, sizeof(mln_expr_val_t), MLN_EXPR_DEFAULT_ARGS) < 0) {
        if (name != NULL) mln_expr_free(name);
        if (namespace != NULL) mln_string_free(namespace);
        return MLN_EXPR_RET_ERR;
    }

    v = NULL;
    while (1) {
        if (v == NULL && (v = (mln_expr_val_t *)mln_array_push(&arr)) == NULL) {
            if (name != NULL) mln_expr_free(name);
            if (namespace != NULL) mln_string_free(namespace);
            mln_array_destroy(&arr);
            return MLN_EXPR_RET_ERR;
        }
        v->type = mln_expr_type_null;
        rc = mln_expr_parse(lex, cb, data, v, eof, next);
        if (rc == MLN_EXPR_RET_ERR) {
            if (name != NULL) mln_expr_free(name);
            if (namespace != NULL) mln_string_free(namespace);
            mln_array_destroy(&arr);
            return MLN_EXPR_RET_ERR;
        }
        if (rc == MLN_EXPR_RET_RPAR) {
            mln_array_pop(&arr);
            break;
        } else if (rc == MLN_EXPR_RET_OK) {
            v = NULL;
            if (*eof) {
                if (name != NULL) mln_expr_free(name);
                if (namespace != NULL) mln_string_free(namespace);
                mln_array_destroy(&arr);
                return MLN_EXPR_RET_ERR;
            }
        }
    }

    v = cb(namespace, name->text, 1, &arr, data);
    mln_expr_val_copy(ret, v);
    mln_expr_val_free(v);
    if (name != NULL) mln_expr_free(name);
    if (namespace != NULL) mln_string_free(namespace);
    mln_array_destroy(&arr);
    return v == NULL? MLN_EXPR_RET_ERR: MLN_EXPR_RET_OK;
})

MLN_FUNC(static inline, int, mln_expr_parse_if, \
         (mln_lex_t *lex, mln_expr_cb_t cb, void *data, mln_expr_val_t *ret, int *eof, mln_expr_struct_t **next), \
         (lex, cb, data, ret, eof, next), \
{
    enum mln_expr_enum type;
    int rc, is_true = 1, count = 0;
    mln_expr_struct_t *tk;
    mln_expr_val_t v;

    v.type = mln_expr_type_null;
    if ((rc = mln_expr_parse(lex, cb, data, &v, eof, next)) != MLN_EXPR_RET_OK) {
        mln_expr_val_destroy(&v);
        return rc;
    }
    if (*eof) {
        mln_expr_val_destroy(&v);
        return MLN_EXPR_RET_ERR;
    }

    switch (v.type) {
        case mln_expr_type_null:
            is_true = 0;
            break;
        case mln_expr_type_bool:
            is_true = v.data.b;
            break;
        case mln_expr_type_int:
            is_true = v.data.i? 1: 0;
            break;
        case mln_expr_type_real:
            is_true = v.data.r == 0.0? 0: 1;
            break;
        case mln_expr_type_string:
            is_true = (v.data.s != NULL && v.data.s->len)? 1: 0;
            break;
        default: /* mln_expr_type_udata */
            is_true = v.data.u != NULL? 1: 0;
            break;
    }
    mln_expr_val_destroy(&v);

    /* then */
again:
    if (*next != NULL) {
        tk = *next;
        *next = NULL;
    } else {
        if ((tk = mln_expr_token(lex)) == NULL) {
            return MLN_EXPR_RET_ERR;
        }
    }
    type = tk->type;
    mln_expr_free(tk);
    if (type == EXPR_TK_SPACE) {
        goto again;
    }
    if (type != EXPR_TK_THEN) {
        return MLN_EXPR_RET_ERR;
    }

    if (is_true) {
        while (1) {
            if (*next != NULL) {
                tk = *next;
                *next = NULL;
            } else {
                if ((tk = mln_expr_token(lex)) == NULL) {
                    return MLN_EXPR_RET_ERR;
                }
            }
            if ((type = tk->type) == EXPR_TK_ELSE) {
                mln_expr_free(tk);
                while (1) {
                    if ((tk = mln_expr_token(lex)) == NULL) {
                        return MLN_EXPR_RET_ERR;
                    }
                    type = tk->type;
                    mln_expr_free(tk);
                    if (type == EXPR_TK_IF) {
                        ++count;
                    } else if (type == EXPR_TK_EOF) {
                        return MLN_EXPR_RET_ERR;
                    } else if (type == EXPR_TK_FI) {
                        if (count-- == 0) break;
                    }
                }
                break;
            } else if (type == EXPR_TK_FI) {
                mln_expr_free(tk);
                break;
            } else {
                *next = tk;
                v.type = mln_expr_type_null;
                if ((rc = mln_expr_parse(lex, cb, data, &v, eof, next)) != MLN_EXPR_RET_OK) {
                    mln_expr_val_destroy(&v);
                    return rc;
                }
                if (*eof) {
                    mln_expr_val_destroy(&v);
                    return MLN_EXPR_RET_ERR;
                }

                if (ret != NULL) mln_expr_val_destroy(ret);
                mln_expr_val_copy(ret, &v);
                mln_expr_val_destroy(&v);
            }
        }
    } else {
        while (1) {
            if (*next != NULL) {
                tk = *next;
                *next = NULL;
            } else {
lp:
                if ((tk = mln_expr_token(lex)) == NULL) {
                    return MLN_EXPR_RET_ERR;
                }
            }

            type = tk->type;
            mln_expr_free(tk);
            if (type == EXPR_TK_IF) {
                ++count;
            } else if (type == EXPR_TK_EOF) {
                return MLN_EXPR_RET_ERR;
            } else if (type == EXPR_TK_FI) {
                --count;
            } else if (type == EXPR_TK_ELSE) {
                if (!count) break;
            }
            goto lp;
        }

        while (1) {
            if (*next != NULL) {
                tk = *next;
                *next = NULL;
            } else {
                if ((tk = mln_expr_token(lex)) == NULL) {
                    return MLN_EXPR_RET_ERR;
                }
            }
            if (tk->type == EXPR_TK_FI) {
                break;
            }
            *next = tk;
            v.type = mln_expr_type_null;
            if ((rc = mln_expr_parse(lex, cb, data, &v, eof, next)) != MLN_EXPR_RET_OK) {
                mln_expr_val_destroy(&v);
                return rc;
            }
            if (*eof) {
                mln_expr_val_destroy(&v);
                return MLN_EXPR_RET_ERR;
            }

            if (ret != NULL) mln_expr_val_destroy(ret);
            mln_expr_val_copy(ret, &v);
            mln_expr_val_destroy(&v);
        }
    }

    return MLN_EXPR_RET_OK;
})

MLN_FUNC(static inline, int, mln_expr_parse_loop, \
         (mln_lex_t *lex, mln_expr_cb_t cb, void *data, mln_expr_val_t *ret, int *eof, mln_expr_struct_t **next), \
         (lex, cb, data, ret, eof, next), \
{
    enum mln_expr_enum type;
    int rc, is_true = 1, count = 0;
    mln_expr_struct_t *tk;
    mln_expr_val_t v;
    mln_lex_off_t off = mln_lex_snapshot_record(lex);

begin:
    v.type = mln_expr_type_null;
    if ((rc = mln_expr_parse(lex, cb, data, &v, eof, next)) != MLN_EXPR_RET_OK) {
        mln_expr_val_destroy(&v);
        return rc;
    }
    if (*eof) {
        mln_expr_val_destroy(&v);
        return MLN_EXPR_RET_ERR;
    }

    switch (v.type) {
        case mln_expr_type_null:
            is_true = 0;
            break;
        case mln_expr_type_bool:
            is_true = v.data.b;
            break;
        case mln_expr_type_int:
            is_true = v.data.i? 1: 0;
            break;
        case mln_expr_type_real:
            is_true = v.data.r == 0.0? 0: 1;
            break;
        case mln_expr_type_string:
            is_true = (v.data.s != NULL && v.data.s->len)? 1: 0;
            break;
        default: /* mln_expr_type_udata */
            is_true = v.data.u != NULL? 1: 0;
            break;
    }
    mln_expr_val_destroy(&v);

    /* do */
again:
    if (*next != NULL) {
        tk = *next;
        *next = NULL;
    } else {
        if ((tk = mln_expr_token(lex)) == NULL) {
            return MLN_EXPR_RET_ERR;
        }
    }
    type = tk->type;
    mln_expr_free(tk);
    if (type == EXPR_TK_SPACE) {
        goto again;
    }
    if (type != EXPR_TK_DO) {
        return MLN_EXPR_RET_ERR;
    }

    if (is_true) {
        while (1) {
            if (*next != NULL) {
                tk = *next;
                *next = NULL;
            } else {
                if ((tk = mln_expr_token(lex)) == NULL) {
                    return MLN_EXPR_RET_ERR;
                }
            }
            if (tk->type == EXPR_TK_END) {
                break;
            }
            *next = tk;
            v.type = mln_expr_type_null;
            if ((rc = mln_expr_parse(lex, cb, data, &v, eof, next)) != MLN_EXPR_RET_OK) {
                mln_expr_val_destroy(&v);
                return rc;
            }
            if (*eof) {
                mln_expr_val_destroy(&v);
                return MLN_EXPR_RET_ERR;
            }

            if (ret != NULL) mln_expr_val_destroy(ret);
            mln_expr_val_copy(ret, &v);
            mln_expr_val_destroy(&v);
        }
        mln_lex_snapshot_apply(lex, off);
        goto begin;
    } else {
        while (1) {
            if (*next != NULL) {
                tk = *next;
                *next = NULL;
            } else {
lp:
                if ((tk = mln_expr_token(lex)) == NULL) {
                    return MLN_EXPR_RET_ERR;
                }
            }

            type = tk->type;
            mln_expr_free(tk);
            if (type == EXPR_TK_LOOP) {
                ++count;
            } else if (type == EXPR_TK_EOF) {
                return MLN_EXPR_RET_ERR;
            } else if (type == EXPR_TK_END) {
                if (count-- == 0) break;
            }
            goto lp;
        }
    }

    return MLN_EXPR_RET_OK;
})

MLN_FUNC(, mln_expr_val_t *, mln_expr_run, (mln_string_t *exp, mln_expr_cb_t cb, void *data), (exp, cb, data), {
    mln_lex_t *lex = NULL;
    struct mln_lex_attr lattr;
    mln_lex_hooks_t hooks;
    mln_alloc_t *pool;
    mln_expr_val_t *ret = NULL, v;
    int eof = 0;
    mln_expr_struct_t *next = NULL;

    memset(&hooks, 0, sizeof(hooks));
    hooks.dblq_handler = (lex_hook)mln_expr_dblq_handler;
    hooks.sglq_handler = (lex_hook)mln_expr_sglq_handler;

    if ((pool = lattr.pool = mln_alloc_init(NULL)) == NULL) {
        return NULL;
    }
    lattr.keywords = keywords;
    lattr.hooks = &hooks;
    lattr.preprocess = 0;
    lattr.padding = 0;
    lattr.type = M_INPUT_T_BUF;
    lattr.data = exp;
    lattr.env = NULL;

    mln_lex_init_with_hooks(mln_expr, lex, &lattr);
    if (lex == NULL) {
        mln_alloc_destroy(pool);
        return NULL;
    }

    while (1) {
        v.type = mln_expr_type_null;
        if (mln_expr_parse(lex, cb, data, &v, &eof, &next) != MLN_EXPR_RET_OK) {
            if (ret != NULL) mln_expr_val_free(ret);
            ret = NULL;
            mln_expr_val_destroy(&v);
            break;
        }

        if (eof) {
            mln_expr_val_destroy(&v);
            if (ret == NULL) ret = mln_expr_val_new(mln_expr_type_null, NULL, NULL);
            break;
        }

        if (ret != NULL) mln_expr_val_free(ret);
        if ((ret = (mln_expr_val_t *)malloc(sizeof(mln_expr_val_t))) != NULL) {
            mln_expr_val_copy(ret, &v);
        }
        mln_expr_val_destroy(&v);
    }

    if (next != NULL) mln_expr_free(next);
    mln_lex_destroy(lex);
    mln_alloc_destroy(pool);

    return ret;
})

MLN_FUNC(, mln_expr_val_t *, mln_expr_run_file, (mln_string_t *path, mln_expr_cb_t cb, void *data), (path, cb, data), {
    mln_lex_t *lex = NULL;
    struct mln_lex_attr lattr;
    mln_lex_hooks_t hooks;
    mln_alloc_t *pool;
    mln_expr_val_t *ret = NULL, v;
    int eof = 0;
    mln_expr_struct_t *next = NULL;

    memset(&hooks, 0, sizeof(hooks));
    hooks.dblq_handler = (lex_hook)mln_expr_dblq_handler;
    hooks.sglq_handler = (lex_hook)mln_expr_sglq_handler;

    if ((pool = lattr.pool = mln_alloc_init(NULL)) == NULL) {
        return NULL;
    }
    lattr.keywords = keywords;
    lattr.hooks = &hooks;
    lattr.preprocess = 1;
    lattr.padding = 0;
    lattr.type = M_INPUT_T_FILE;
    lattr.data = path;
    lattr.env = NULL;

    mln_lex_init_with_hooks(mln_expr, lex, &lattr);
    if (lex == NULL) {
        mln_alloc_destroy(pool);
        return NULL;
    }

    while (1) {
        v.type = mln_expr_type_null;
        if (mln_expr_parse(lex, cb, data, &v, &eof, &next) != MLN_EXPR_RET_OK) {
            if (ret != NULL) mln_expr_val_free(ret);
            ret = NULL;
            mln_expr_val_destroy(&v);
            break;
        }

        if (eof) {
            mln_expr_val_destroy(&v);
            if (ret == NULL) ret = mln_expr_val_new(mln_expr_type_null, NULL, NULL);
            break;
        }

        if (ret != NULL) mln_expr_val_free(ret);
        if ((ret = (mln_expr_val_t *)malloc(sizeof(mln_expr_val_t))) != NULL) {
            mln_expr_val_copy(ret, &v);
        }
        mln_expr_val_destroy(&v);
    }

    if (next != NULL) mln_expr_free(next);
    mln_lex_destroy(lex);
    mln_alloc_destroy(pool);

    return ret;
})

