
/*
 * Copyright (C) Niklaus F.Schen.
 */

#include "mln_expr.h"
#include "mln_func.h"
#include "mln_lex.h"
#include <stdlib.h>
#include <string.h>

/* =========================================================================
 * Lex-based tokenizer (used by mln_expr_run_file)
 * ========================================================================= */

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

/* Escape character lookup table */
static const char mln_expr_escape_tbl[256] = {
    ['\"'] = '\"',
    ['\''] = '\'',
    ['n']  = '\n',
    ['t']  = '\t',
    ['b']  = '\b',
    ['a']  = '\a',
    ['f']  = '\f',
    ['r']  = '\r',
    ['v']  = '\v',
    ['\\'] = '\\',
};

/* =========================================================================
 * Value helper functions
 * ========================================================================= */
static inline void mln_expr_val_move(mln_expr_val_t *dest, mln_expr_val_t *src)
{
    *dest = *src;
    src->type = mln_expr_type_null;
}

static inline void mln_expr_val_cleanup(mln_expr_val_t *ev)
{
    if (ev->type == mln_expr_type_string) {
        if (ev->free != NULL) ev->free(ev->data.s);
        else mln_string_free(ev->data.s);
    } else if (ev->type == mln_expr_type_udata) {
        if (ev->free != NULL) ev->free(ev->data.u);
    }
}

static inline void mln_expr_val_replace(mln_expr_val_t *dest, mln_expr_val_t *src)
{
    mln_expr_val_cleanup(dest);
    *dest = *src;
    src->type = mln_expr_type_null;
}

static inline int mln_expr_val_is_true(mln_expr_val_t *v)
{
    switch (v->type) {
        case mln_expr_type_null:
            return 0;
        case mln_expr_type_bool:
            return v->data.b;
        case mln_expr_type_int:
            return v->data.i? 1: 0;
        case mln_expr_type_real:
            return v->data.r == 0.0? 0: 1;
        case mln_expr_type_string:
            return (v->data.s != NULL && v->data.s->len)? 1: 0;
        default: /* mln_expr_type_udata */
            return v->data.u != NULL? 1: 0;
    }
}

/* =========================================================================
 * Public value API
 * ========================================================================= */
MLN_FUNC(, mln_expr_val_t *, mln_expr_val_new, (mln_expr_typ_t type, void *data, mln_expr_udata_free free), (type, data, free), {
    mln_expr_val_t *ev;
    if ((ev = (mln_expr_val_t *)malloc(sizeof(mln_expr_val_t))) == NULL) return NULL;
    ev->type = type;
    switch (type) {
        case mln_expr_type_null: ev->free = NULL; break;
        case mln_expr_type_bool: ev->data.b = *((mln_u8ptr_t)data); ev->free = NULL; break;
        case mln_expr_type_int: ev->data.i = *((mln_s64_t *)data); ev->free = NULL; break;
        case mln_expr_type_real: ev->data.r = *((double *)data); ev->free = NULL; break;
        case mln_expr_type_string: ev->data.s = mln_string_ref((mln_string_t *)data); ev->free = free; break;
        default: ev->data.u = data; ev->free = free; break;
    }
    return ev;
})

MLN_FUNC_VOID(, void, mln_expr_val_free, (mln_expr_val_t *ev), (ev), {
    if (ev == NULL) return;
    mln_expr_val_cleanup(ev);
    free(ev);
})

MLN_FUNC(, mln_expr_val_t *, mln_expr_val_dup, (mln_expr_val_t *val), (val), {
    mln_expr_val_t *v;
    if ((v = (mln_expr_val_t *)malloc(sizeof(mln_expr_val_t))) == NULL) return NULL;
    v->type = val->type;
    switch (val->type) {
        case mln_expr_type_null: break;
        case mln_expr_type_bool: v->data.b = val->data.b; break;
        case mln_expr_type_int: v->data.i = val->data.i; break;
        case mln_expr_type_real: v->data.r = val->data.r; break;
        case mln_expr_type_string: v->data.s = mln_string_ref(val->data.s); v->free = val->free; break;
        default: v->data.u = val->data.u; v->free = val->free; val->free = NULL; break;
    }
    return v;
})

MLN_FUNC_VOID(, void, mln_expr_val_copy, (mln_expr_val_t *dest, mln_expr_val_t *src), (dest, src), {
    if (src == NULL) return;
    dest->type = src->type;
    switch (src->type) {
        case mln_expr_type_null: break;
        case mln_expr_type_bool: dest->data.b = src->data.b; break;
        case mln_expr_type_int: dest->data.i = src->data.i; break;
        case mln_expr_type_real: dest->data.r = src->data.r; break;
        case mln_expr_type_string: dest->data.s = mln_string_ref(src->data.s); dest->free = src->free; break;
        default: dest->data.u = src->data.u; dest->free = src->free; src->free = NULL; break;
    }
})

/* =========================================================================
 * Fast-path direct scanner for mln_expr_run (buffer input only).
 * Bypasses lex/pool machinery for significant speedup.
 * ========================================================================= */

enum expr_ft {
    FT_EOF = 0, FT_ID, FT_DEC, FT_OCT, FT_HEX, FT_REAL,
    FT_STRING, FT_TRUE, FT_FALSE, FT_NULL,
    FT_IF, FT_THEN, FT_ELSE, FT_FI, FT_LOOP, FT_DO, FT_END,
    FT_LPAR, FT_RPAR, FT_COMMA, FT_COLON,
    FT_ERR
};

typedef struct {
    mln_u8ptr_t text;
    mln_size_t  len;
    enum expr_ft type;
    mln_u8_t    heap;
} expr_ft_t;

typedef struct {
    mln_u8ptr_t buf;
    mln_u8ptr_t pos;
    mln_u8ptr_t end;
} expr_scan_t;

#define EXPR_IS_ALPHA(c) (((c) >= 'a' && (c) <= 'z') || ((c) >= 'A' && (c) <= 'Z') || (c) == '_')
#define EXPR_IS_DIGIT(c) ((c) >= '0' && (c) <= '9')
#define EXPR_IS_ALNUM(c) (EXPR_IS_ALPHA(c) || EXPR_IS_DIGIT(c))
#define EXPR_IS_HEX(c) (EXPR_IS_DIGIT(c) || ((c) >= 'a' && (c) <= 'f') || ((c) >= 'A' && (c) <= 'F'))
#define EXPR_IS_OCT(c) ((c) >= '0' && (c) <= '7')

static inline void expr_ft_free(expr_ft_t *t)
{
    if (t->heap && t->text != NULL) {
        free(t->text);
        t->text = NULL;
        t->heap = 0;
    }
}

/* Transfer ownership of token from src to dest (move semantics). */
static inline void expr_ft_move(expr_ft_t *dest, expr_ft_t *src)
{
    *dest = *src;
    src->type = FT_EOF;
    src->heap = 0;
    src->text = NULL;
    src->len = 0;
}

MLN_FUNC(static inline, int, expr_scan_string, \
         (expr_scan_t *s, expr_ft_t *tok, char quote), (s, tok, quote), \
{
    mln_u8ptr_t start = s->pos;
    int has_escape = 0;
    while (s->pos < s->end) {
        mln_u8_t c = *s->pos;
        if (c == (mln_u8_t)quote) {
            if (!has_escape) {
                tok->text = start;
                tok->len = (mln_size_t)(s->pos - start);
                tok->type = FT_STRING;
                tok->heap = 0;
                s->pos++;
                return 0;
            }
            break;
        }
        if (c == '\\') { has_escape = 1; s->pos++; if (s->pos >= s->end) return -1; }
        s->pos++;
    }
    if (s->pos >= s->end) return -1;
    {
        mln_size_t raw_len = (mln_size_t)(s->pos - start);
        mln_u8ptr_t buf = (mln_u8ptr_t)malloc(raw_len);
        mln_u8ptr_t p, dst;
        if (buf == NULL) return -1;
        dst = buf;
        for (p = start; p < s->pos; p++) {
            if (*p == '\\') {
                p++;
                char rep = mln_expr_escape_tbl[*p];
                if (rep == 0) { free(buf); return -1; }
                *dst++ = (mln_u8_t)rep;
            } else {
                *dst++ = *p;
            }
        }
        tok->text = buf;
        tok->len = (mln_size_t)(dst - buf);
        tok->type = FT_STRING;
        tok->heap = 1;
        s->pos++;
    }
    return 0;
})

MLN_FUNC_VOID(static inline, void, expr_scan_next, (expr_scan_t *s, expr_ft_t *tok), (s, tok), {
    tok->heap = 0;
    while (s->pos < s->end) {
        mln_u8_t c = *s->pos;
        if (c != ' ' && c != '\t' && c != '\r' && c != '\n') break;
        s->pos++;
    }
    if (s->pos >= s->end) {
        tok->type = FT_EOF; tok->text = NULL; tok->len = 0; return;
    }
    mln_u8_t c = *s->pos;
    switch (c) {
        case '(':  tok->type = FT_LPAR;  tok->text = s->pos++; tok->len = 1; return;
        case ')':  tok->type = FT_RPAR;  tok->text = s->pos++; tok->len = 1; return;
        case ',':  tok->type = FT_COMMA; tok->text = s->pos++; tok->len = 1; return;
        case ':':  tok->type = FT_COLON; tok->text = s->pos++; tok->len = 1; return;
        default: break;
    }
    if (c == '"' || c == '\'') {
        s->pos++;
        if (expr_scan_string(s, tok, (char)c) < 0) { tok->type = FT_ERR; tok->text = NULL; tok->len = 0; }
        return;
    }
    if (EXPR_IS_DIGIT(c)) {
        mln_u8ptr_t start = s->pos;
        if (c == '0' && s->pos + 1 < s->end) {
            mln_u8_t nc = s->pos[1];
            if (nc == 'x' || nc == 'X') {
                s->pos += 2;
                if (s->pos >= s->end || !EXPR_IS_HEX(*s->pos)) {
                    tok->type = FT_ERR; tok->text = start; tok->len = (mln_size_t)(s->pos - start); return;
                }
                while (s->pos < s->end && EXPR_IS_HEX(*s->pos)) s->pos++;
                tok->type = FT_HEX; tok->text = start; tok->len = (mln_size_t)(s->pos - start); return;
            }
            if (EXPR_IS_OCT(nc)) {
                s->pos++;
                while (s->pos < s->end && EXPR_IS_OCT(*s->pos)) s->pos++;
                if (s->pos < s->end && *s->pos == '.') goto parse_real;
                tok->type = FT_OCT; tok->text = start; tok->len = (mln_size_t)(s->pos - start); return;
            }
        }
        while (s->pos < s->end && EXPR_IS_DIGIT(*s->pos)) s->pos++;
        if (s->pos < s->end && *s->pos == '.') {
parse_real:
            s->pos++;
            while (s->pos < s->end && EXPR_IS_DIGIT(*s->pos)) s->pos++;
            tok->type = FT_REAL; tok->text = start; tok->len = (mln_size_t)(s->pos - start); return;
        }
        tok->type = FT_DEC; tok->text = start; tok->len = (mln_size_t)(s->pos - start); return;
    }
    if (EXPR_IS_ALPHA(c)) {
        mln_u8ptr_t start = s->pos;
        s->pos++;
        while (s->pos < s->end && EXPR_IS_ALNUM(*s->pos)) s->pos++;
        tok->text = start;
        tok->len = (mln_size_t)(s->pos - start);
        switch (tok->len) {
            case 2:
                if (tok->text[0] == 'i' && tok->text[1] == 'f') { tok->type = FT_IF; return; }
                if (tok->text[0] == 'f' && tok->text[1] == 'i') { tok->type = FT_FI; return; }
                if (tok->text[0] == 'd' && tok->text[1] == 'o') { tok->type = FT_DO; return; }
                break;
            case 3:
                if (memcmp(tok->text, "end", 3) == 0) { tok->type = FT_END; return; }
                break;
            case 4:
                if (memcmp(tok->text, "true", 4) == 0) { tok->type = FT_TRUE; return; }
                if (memcmp(tok->text, "else", 4) == 0) { tok->type = FT_ELSE; return; }
                if (memcmp(tok->text, "loop", 4) == 0) { tok->type = FT_LOOP; return; }
                if (memcmp(tok->text, "then", 4) == 0) { tok->type = FT_THEN; return; }
                if (memcmp(tok->text, "null", 4) == 0) { tok->type = FT_NULL; return; }
                break;
            case 5:
                if (memcmp(tok->text, "false", 5) == 0) { tok->type = FT_FALSE; return; }
                break;
        }
        tok->type = FT_ID; return;
    }
    s->pos++;
    tok->type = FT_ERR; tok->text = NULL; tok->len = 0;
})

static inline int
expr_fast_parse_if(expr_scan_t *s, mln_expr_cb_t cb, void *data, mln_expr_val_t *ret, int *eof, expr_ft_t *next);
static inline int
expr_fast_parse_loop(expr_scan_t *s, mln_expr_cb_t cb, void *data, mln_expr_val_t *ret, int *eof, expr_ft_t *next);

MLN_FUNC(static inline, int, expr_fast_parse, \
         (expr_scan_t *s, mln_expr_cb_t cb, void *data, mln_expr_val_t *ret, int *eof, expr_ft_t *next), \
         (s, cb, data, ret, eof, next), \
{
    expr_ft_t tok;
    mln_expr_val_t *v;
    mln_string_t name_str, *ns_str = NULL, *ns_cat;
    char *endp;

again:
    if (next->type != FT_EOF) { expr_ft_move(&tok, next); }
    else expr_scan_next(s, &tok);

    switch (tok.type) {
        case FT_EOF:
            *eof = 1;
            if (ns_str != NULL) mln_string_free(ns_str);
            return MLN_EXPR_RET_OK;
        case FT_COMMA:
            if (ns_str != NULL) mln_string_free(ns_str);
            return MLN_EXPR_RET_COMMA;
        case FT_RPAR:
            if (ns_str != NULL) mln_string_free(ns_str);
            return MLN_EXPR_RET_RPAR;
        case FT_IF:
            if (ns_str != NULL) mln_string_free(ns_str);
            return expr_fast_parse_if(s, cb, data, ret, eof, next);
        case FT_LOOP:
            if (ns_str != NULL) mln_string_free(ns_str);
            return expr_fast_parse_loop(s, cb, data, ret, eof, next);
        case FT_TRUE:
            ret->type = mln_expr_type_bool; ret->data.b = 1; ret->free = NULL;
            if (ns_str != NULL) mln_string_free(ns_str);
            return MLN_EXPR_RET_OK;
        case FT_FALSE:
            ret->type = mln_expr_type_bool; ret->data.b = 0; ret->free = NULL;
            if (ns_str != NULL) mln_string_free(ns_str);
            return MLN_EXPR_RET_OK;
        case FT_NULL:
            ret->type = mln_expr_type_null; ret->free = NULL;
            if (ns_str != NULL) mln_string_free(ns_str);
            return MLN_EXPR_RET_OK;
        case FT_DEC: case FT_OCT: case FT_HEX:
        {
            char num[32];
            int base = (tok.type == FT_DEC) ? 10 : (tok.type == FT_OCT) ? 8 : 16;
            mln_size_t len = tok.len >= sizeof(num)-1? sizeof(num)-1: tok.len;
            memcpy(num, tok.text, len); num[len] = 0;
            ret->type = mln_expr_type_int;
            ret->data.i = (mln_s64_t)strtoll(num, &endp, base);
            ret->free = NULL;
            if (ns_str != NULL) mln_string_free(ns_str);
            return MLN_EXPR_RET_OK;
        }
        case FT_REAL:
        {
            char num[64];
            mln_size_t len = tok.len >= sizeof(num)-1? sizeof(num)-1: tok.len;
            memcpy(num, tok.text, len); num[len] = 0;
            ret->type = mln_expr_type_real;
            ret->data.r = strtod(num, &endp);
            ret->free = NULL;
            if (ns_str != NULL) mln_string_free(ns_str);
            return MLN_EXPR_RET_OK;
        }
        case FT_STRING:
        {
            mln_string_t tmp;
            mln_string_nset(&tmp, tok.text, tok.len);
            mln_string_t *sd = mln_string_dup(&tmp);
            expr_ft_free(&tok);
            if (sd == NULL) { if (ns_str) mln_string_free(ns_str); return MLN_EXPR_RET_ERR; }
            ret->type = mln_expr_type_string; ret->data.s = sd; ret->free = NULL;
            if (ns_str != NULL) mln_string_free(ns_str);
            return MLN_EXPR_RET_OK;
        }
        case FT_ID: break;
        default:
            if (ns_str != NULL) mln_string_free(ns_str);
            return MLN_EXPR_RET_ERR;
    }

    mln_string_nset(&name_str, tok.text, tok.len);
    expr_scan_next(s, &tok);

    if (tok.type == FT_COLON) {
        /* Build namespace as: concat(ns_str, name_str, ":") = ns_str + ":" + name_str */
        mln_string_t sep = mln_string(":");
        ns_cat = mln_string_concat(ns_str, &name_str, &sep);
        if (ns_str != NULL) mln_string_free(ns_str);
        if (ns_cat == NULL) return MLN_EXPR_RET_ERR;
        ns_str = ns_cat;
        goto again;
    }

    if (tok.type != FT_LPAR) {
        v = cb(ns_str, &name_str, 0, NULL, data);
        if (v == NULL) { if (ns_str) mln_string_free(ns_str); expr_ft_free(&tok); return MLN_EXPR_RET_ERR; }
        mln_expr_val_move(ret, v); free(v);
        if (ns_str != NULL) mln_string_free(ns_str);
        if (tok.type == FT_EOF || tok.type == FT_COMMA) return MLN_EXPR_RET_OK;
        *next = tok;
        return MLN_EXPR_RET_OK;
    }

    /* Function call */
    {
        mln_array_t arr;
        int rc;
        if (mln_array_init(&arr, (array_free)mln_expr_val_cleanup, sizeof(mln_expr_val_t), MLN_EXPR_DEFAULT_ARGS) < 0) {
            if (ns_str) mln_string_free(ns_str); return MLN_EXPR_RET_ERR;
        }
        while (1) {
            mln_expr_val_t *elem;
            MLN_ARRAY_PUSH(&arr, elem);
            if (elem == NULL) { if (ns_str) mln_string_free(ns_str); mln_array_destroy(&arr); return MLN_EXPR_RET_ERR; }
            elem->type = mln_expr_type_null;
            rc = expr_fast_parse(s, cb, data, elem, eof, next);
            if (rc == MLN_EXPR_RET_ERR) { if (ns_str) mln_string_free(ns_str); mln_array_destroy(&arr); return MLN_EXPR_RET_ERR; }
            if (rc == MLN_EXPR_RET_RPAR) { MLN_ARRAY_POP(&arr); break; }
            else if (rc == MLN_EXPR_RET_OK) {
                if (*eof) { if (ns_str) mln_string_free(ns_str); mln_array_destroy(&arr); return MLN_EXPR_RET_ERR; }
            }
        }
        v = cb(ns_str, &name_str, 1, &arr, data);
        if (v == NULL) { if (ns_str) mln_string_free(ns_str); mln_array_destroy(&arr); return MLN_EXPR_RET_ERR; }
        mln_expr_val_move(ret, v); free(v);
        if (ns_str) mln_string_free(ns_str);
        mln_array_destroy(&arr);
        return MLN_EXPR_RET_OK;
    }
})

MLN_FUNC(static inline, int, expr_fast_parse_if, \
         (expr_scan_t *s, mln_expr_cb_t cb, void *data, mln_expr_val_t *ret, int *eof, expr_ft_t *next), \
         (s, cb, data, ret, eof, next), \
{
    int rc, is_true, count = 0;
    expr_ft_t tok;
    mln_expr_val_t v;

    v.type = mln_expr_type_null;
    if ((rc = expr_fast_parse(s, cb, data, &v, eof, next)) != MLN_EXPR_RET_OK) { mln_expr_val_cleanup(&v); return rc; }
    if (*eof) { mln_expr_val_cleanup(&v); return MLN_EXPR_RET_ERR; }
    is_true = mln_expr_val_is_true(&v);
    mln_expr_val_cleanup(&v);

    if (next->type != FT_EOF) { expr_ft_move(&tok, next); } else expr_scan_next(s, &tok);
    if (tok.type != FT_THEN) return MLN_EXPR_RET_ERR;

    if (is_true) {
        while (1) {
            if (next->type != FT_EOF) { expr_ft_move(&tok, next); } else expr_scan_next(s, &tok);
            if (tok.type == FT_ELSE) {
                while (1) {
                    expr_scan_next(s, &tok);
                    if (tok.type == FT_IF) ++count;
                    else if (tok.type == FT_EOF) return MLN_EXPR_RET_ERR;
                    else if (tok.type == FT_FI) { if (count-- == 0) break; }
                    expr_ft_free(&tok);
                }
                break;
            } else if (tok.type == FT_FI) {
                break;
            } else {
                *next = tok;
                v.type = mln_expr_type_null;
                if ((rc = expr_fast_parse(s, cb, data, &v, eof, next)) != MLN_EXPR_RET_OK) { mln_expr_val_cleanup(&v); return rc; }
                if (*eof) { mln_expr_val_cleanup(&v); return MLN_EXPR_RET_ERR; }
                mln_expr_val_replace(ret, &v);
            }
        }
    } else {
        int found_else = 0;
        while (1) {
            expr_scan_next(s, &tok);
            if (tok.type == FT_IF) ++count;
            else if (tok.type == FT_EOF) return MLN_EXPR_RET_ERR;
            else if (tok.type == FT_FI) { if (count-- == 0) break; }
            else if (tok.type == FT_ELSE) { if (!count) { found_else = 1; break; } }
            expr_ft_free(&tok);
        }
        if (found_else) {
            while (1) {
                if (next->type != FT_EOF) { expr_ft_move(&tok, next); } else expr_scan_next(s, &tok);
                if (tok.type == FT_FI) break;
                *next = tok;
                v.type = mln_expr_type_null;
                if ((rc = expr_fast_parse(s, cb, data, &v, eof, next)) != MLN_EXPR_RET_OK) { mln_expr_val_cleanup(&v); return rc; }
                if (*eof) { mln_expr_val_cleanup(&v); return MLN_EXPR_RET_ERR; }
                mln_expr_val_replace(ret, &v);
            }
        }
    }
    return MLN_EXPR_RET_OK;
})

MLN_FUNC(static inline, int, expr_fast_parse_loop, \
         (expr_scan_t *s, mln_expr_cb_t cb, void *data, mln_expr_val_t *ret, int *eof, expr_ft_t *next), \
         (s, cb, data, ret, eof, next), \
{
    int rc, is_true, count;
    expr_ft_t tok;
    mln_expr_val_t v;
    mln_u8ptr_t loop_start = s->pos;

begin:
    v.type = mln_expr_type_null;
    if ((rc = expr_fast_parse(s, cb, data, &v, eof, next)) != MLN_EXPR_RET_OK) { mln_expr_val_cleanup(&v); return rc; }
    if (*eof) { mln_expr_val_cleanup(&v); return MLN_EXPR_RET_ERR; }
    is_true = mln_expr_val_is_true(&v);
    mln_expr_val_cleanup(&v);

    if (next->type != FT_EOF) { expr_ft_move(&tok, next); } else expr_scan_next(s, &tok);
    if (tok.type != FT_DO) return MLN_EXPR_RET_ERR;

    if (is_true) {
        while (1) {
            if (next->type != FT_EOF) { expr_ft_move(&tok, next); } else expr_scan_next(s, &tok);
            if (tok.type == FT_END) break;
            *next = tok;
            v.type = mln_expr_type_null;
            if ((rc = expr_fast_parse(s, cb, data, &v, eof, next)) != MLN_EXPR_RET_OK) { mln_expr_val_cleanup(&v); return rc; }
            if (*eof) { mln_expr_val_cleanup(&v); return MLN_EXPR_RET_ERR; }
            mln_expr_val_replace(ret, &v);
        }
        s->pos = loop_start;
        next->type = FT_EOF;
        goto begin;
    } else {
        count = 0;
        while (1) {
            if (next->type != FT_EOF) { expr_ft_move(&tok, next); } else expr_scan_next(s, &tok);
            if (tok.type == FT_LOOP) ++count;
            else if (tok.type == FT_EOF) return MLN_EXPR_RET_ERR;
            else if (tok.type == FT_END) { if (count-- == 0) break; }
            expr_ft_free(&tok);
        }
    }
    return MLN_EXPR_RET_OK;
})

MLN_FUNC(, mln_expr_val_t *, mln_expr_run, (mln_string_t *exp, mln_expr_cb_t cb, void *data), (exp, cb, data), {
    expr_scan_t scan;
    mln_expr_val_t v, *ret_val;
    int eof = 0, has_result = 0;
    expr_ft_t next;

    scan.buf = exp->data;
    scan.pos = exp->data;
    scan.end = exp->data + exp->len;
    next.type = FT_EOF;
    next.heap = 0;

    v.type = mln_expr_type_null;
    while (1) {
        mln_expr_val_t tmp;
        tmp.type = mln_expr_type_null;
        if (expr_fast_parse(&scan, cb, data, &tmp, &eof, &next) != MLN_EXPR_RET_OK) {
            mln_expr_val_cleanup(&tmp); mln_expr_val_cleanup(&v); has_result = 0; break;
        }
        if (eof) { mln_expr_val_cleanup(&tmp); break; }
        mln_expr_val_replace(&v, &tmp);
        has_result = 1;
    }
    expr_ft_free(&next);
    if (!has_result) {
        if (eof) return mln_expr_val_new(mln_expr_type_null, NULL, NULL);
        return NULL;
    }
    if ((ret_val = (mln_expr_val_t *)malloc(sizeof(mln_expr_val_t))) == NULL) { mln_expr_val_cleanup(&v); return NULL; }
    mln_expr_val_move(ret_val, &v);
    return ret_val;
})

/* =========================================================================
 * Lex-based parsers for mln_expr_run_file
 * ========================================================================= */

MLN_FUNC(static inline, int, mln_get_char, (mln_lex_t *lex, char c), (lex, c), {
    if (c == '\\') {
        char n;
        if ((n = mln_lex_getchar(lex)) == MLN_ERR) return -1;
        char replacement = mln_expr_escape_tbl[(mln_u8_t)n];
        if (replacement == 0) { mln_lex_error_set(lex, MLN_LEX_EINVCHAR); return -1; }
        if (mln_lex_putchar(lex, replacement) == MLN_ERR) return -1;
    } else {
        if (mln_lex_putchar(lex, c) == MLN_ERR) return -1;
    }
    return 0;
})

MLN_FUNC(static, mln_expr_struct_t *, mln_expr_dblq_handler, (mln_lex_t *lex, void *data), (lex, data), {
    mln_lex_result_clean(lex);
    char c;
    while (1) {
        c = mln_lex_getchar(lex);
        if (c == MLN_ERR) return NULL;
        if (c == MLN_EOF) { mln_lex_error_set(lex, MLN_LEX_EINVEOF); return NULL; }
        if (c == '\"') break;
        if (mln_get_char(lex, c) < 0) return NULL;
    }
    return mln_expr_new(lex, EXPR_TK_STRING);
})

MLN_FUNC(static, mln_expr_struct_t *, mln_expr_sglq_handler, (mln_lex_t *lex, void *data), (lex, data), {
    mln_lex_result_clean(lex);
    char c;
    while (1) {
        c = mln_lex_getchar(lex);
        if (c == MLN_ERR) return NULL;
        if (c == MLN_EOF) { mln_lex_error_set(lex, MLN_LEX_EINVEOF); return NULL; }
        if (c == '\'') break;
        if (mln_get_char(lex, c) < 0) return NULL;
    }
    return mln_expr_new(lex, EXPR_TK_STRING);
})

static inline int mln_expr_parse_if(mln_lex_t *lex, mln_expr_cb_t cb, void *data, mln_expr_val_t *ret, int *eof, mln_expr_struct_t **next);
static inline int mln_expr_parse_loop(mln_lex_t *lex, mln_expr_cb_t cb, void *data, mln_expr_val_t *ret, int *eof, mln_expr_struct_t **next);

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
    char *endp;

again:
    if ((name = *next) != NULL) { *next = NULL; }
    else { if ((name = mln_expr_token(lex)) == NULL) { if (namespace) mln_string_free(namespace); return MLN_EXPR_RET_ERR; } }
    if ((type = name->type) == EXPR_TK_EOF) { *eof = 1; mln_expr_free(name); if (namespace) mln_string_free(namespace); return MLN_EXPR_RET_OK; }
    if (type == EXPR_TK_SPACE || type == EXPR_TK_COLON) { mln_expr_free(name); goto again; }
    if (type == EXPR_TK_COMMA) { mln_expr_free(name); if (namespace) mln_string_free(namespace); return MLN_EXPR_RET_COMMA; }
    if (type == EXPR_TK_RPAR) { mln_expr_free(name); if (namespace) mln_string_free(namespace); return MLN_EXPR_RET_RPAR; }
    if (type == EXPR_TK_IF) { mln_expr_free(name); if (namespace) mln_string_free(namespace); return mln_expr_parse_if(lex, cb, data, ret, eof, next); }
    if (type == EXPR_TK_LOOP) { mln_expr_free(name); if (namespace) mln_string_free(namespace); return mln_expr_parse_loop(lex, cb, data, ret, eof, next); }
    if (type != EXPR_TK_ID) {
        switch (type) {
            case EXPR_TK_DEC: case EXPR_TK_OCT: case EXPR_TK_HEX:
            {
                int base = (type == EXPR_TK_DEC) ? 10 : (type == EXPR_TK_OCT) ? 8 : 16;
                char num[32];
                mln_size_t len = name->text->len >= sizeof(num)-1? sizeof(num)-1: name->text->len;
                memcpy(num, name->text->data, len); num[len] = 0;
                ret->type = mln_expr_type_int; ret->data.i = (mln_s64_t)strtoll(num, &endp, base); ret->free = NULL;
                break;
            }
            case EXPR_TK_REAL:
            {
                char num[64];
                mln_size_t len = name->text->len >= sizeof(num)-1? sizeof(num)-1: name->text->len;
                memcpy(num, name->text->data, len); num[len] = 0;
                ret->type = mln_expr_type_real; ret->data.r = strtod(num, &endp); ret->free = NULL;
                break;
            }
            case EXPR_TK_STRING: ret->type = mln_expr_type_string; ret->data.s = mln_string_ref(name->text); ret->free = NULL; break;
            case EXPR_TK_TRUE: ret->type = mln_expr_type_bool; ret->data.b = 1; ret->free = NULL; break;
            case EXPR_TK_FALSE: ret->type = mln_expr_type_bool; ret->data.b = 0; ret->free = NULL; break;
            case EXPR_TK_NULL: ret->type = mln_expr_type_null; ret->free = NULL; break;
            default: mln_expr_free(name); if (namespace) mln_string_free(namespace); return MLN_EXPR_RET_ERR;
        }
        mln_expr_free(name); if (namespace) mln_string_free(namespace);
        return MLN_EXPR_RET_OK;
    }

again2:
    if ((tk = mln_expr_token(lex)) == NULL) { mln_expr_free(name); if (namespace) mln_string_free(namespace); return MLN_EXPR_RET_ERR; }
    if ((type = tk->type) == EXPR_TK_SPACE) { mln_expr_free(tk); goto again2; }
    if (type == EXPR_TK_COLON) {
        tmp = mln_string_pool_concat(mln_lex_get_pool(lex), namespace, name->text, tk->text);
        mln_expr_free(tk); mln_expr_free(name); if (namespace) mln_string_free(namespace);
        namespace = tmp; name = NULL; goto again;
    }
    if (type != EXPR_TK_LPAR) {
        v = cb(namespace, name->text, 0, NULL, data);
        if (v == NULL) { mln_expr_free(name); mln_expr_free(tk); if (namespace) mln_string_free(namespace); return MLN_EXPR_RET_ERR; }
        mln_expr_val_move(ret, v); free(v);
        mln_expr_free(name); if (namespace) mln_string_free(namespace);
        if ((type = tk->type) == EXPR_TK_EOF || type == EXPR_TK_COMMA) { mln_expr_free(tk); } else { *next = tk; }
        return MLN_EXPR_RET_OK;
    }
    mln_expr_free(tk);
    if (mln_array_init(&arr, (array_free)mln_expr_val_cleanup, sizeof(mln_expr_val_t), MLN_EXPR_DEFAULT_ARGS) < 0) {
        mln_expr_free(name); if (namespace) mln_string_free(namespace); return MLN_EXPR_RET_ERR;
    }
    v = NULL;
    while (1) {
        mln_expr_val_t *elem;
        MLN_ARRAY_PUSH(&arr, elem);
        if (elem == NULL) { mln_expr_free(name); if (namespace) mln_string_free(namespace); mln_array_destroy(&arr); return MLN_EXPR_RET_ERR; }
        elem->type = mln_expr_type_null;
        rc = mln_expr_parse(lex, cb, data, elem, eof, next);
        if (rc == MLN_EXPR_RET_ERR) { mln_expr_free(name); if (namespace) mln_string_free(namespace); mln_array_destroy(&arr); return MLN_EXPR_RET_ERR; }
        if (rc == MLN_EXPR_RET_RPAR) { MLN_ARRAY_POP(&arr); break; }
        else if (rc == MLN_EXPR_RET_OK) { v = NULL; if (*eof) { mln_expr_free(name); if (namespace) mln_string_free(namespace); mln_array_destroy(&arr); return MLN_EXPR_RET_ERR; } }
    }
    v = cb(namespace, name->text, 1, &arr, data);
    if (v == NULL) { mln_expr_free(name); if (namespace) mln_string_free(namespace); mln_array_destroy(&arr); return MLN_EXPR_RET_ERR; }
    mln_expr_val_move(ret, v); free(v);
    mln_expr_free(name); if (namespace) mln_string_free(namespace);
    mln_array_destroy(&arr);
    return MLN_EXPR_RET_OK;
})

MLN_FUNC(static inline, int, mln_expr_parse_if, \
         (mln_lex_t *lex, mln_expr_cb_t cb, void *data, mln_expr_val_t *ret, int *eof, mln_expr_struct_t **next), \
         (lex, cb, data, ret, eof, next), \
{
    enum mln_expr_enum type;
    int rc, is_true, count = 0;
    mln_expr_struct_t *tk;
    mln_expr_val_t v;

    v.type = mln_expr_type_null;
    if ((rc = mln_expr_parse(lex, cb, data, &v, eof, next)) != MLN_EXPR_RET_OK) { mln_expr_val_cleanup(&v); return rc; }
    if (*eof) { mln_expr_val_cleanup(&v); return MLN_EXPR_RET_ERR; }
    is_true = mln_expr_val_is_true(&v);
    mln_expr_val_cleanup(&v);

lex_if_again:
    if (*next != NULL) { tk = *next; *next = NULL; } else { if ((tk = mln_expr_token(lex)) == NULL) return MLN_EXPR_RET_ERR; }
    type = tk->type; mln_expr_free(tk);
    if (type == EXPR_TK_SPACE) goto lex_if_again;
    if (type != EXPR_TK_THEN) return MLN_EXPR_RET_ERR;

    if (is_true) {
        while (1) {
            if (*next != NULL) { tk = *next; *next = NULL; } else { if ((tk = mln_expr_token(lex)) == NULL) return MLN_EXPR_RET_ERR; }
            if ((type = tk->type) == EXPR_TK_ELSE) {
                mln_expr_free(tk);
                while (1) { if ((tk = mln_expr_token(lex)) == NULL) return MLN_EXPR_RET_ERR; type = tk->type; mln_expr_free(tk); if (type == EXPR_TK_IF) ++count; else if (type == EXPR_TK_EOF) return MLN_EXPR_RET_ERR; else if (type == EXPR_TK_FI) { if (count-- == 0) break; } }
                break;
            } else if (type == EXPR_TK_FI) { mln_expr_free(tk); break; }
            else {
                *next = tk;
                v.type = mln_expr_type_null;
                if ((rc = mln_expr_parse(lex, cb, data, &v, eof, next)) != MLN_EXPR_RET_OK) { mln_expr_val_cleanup(&v); return rc; }
                if (*eof) { mln_expr_val_cleanup(&v); return MLN_EXPR_RET_ERR; }
                mln_expr_val_replace(ret, &v);
            }
        }
    } else {
        int found_else = 0;
        while (1) {
            if (*next != NULL) { tk = *next; *next = NULL; } else {
lex_if_lp: if ((tk = mln_expr_token(lex)) == NULL) return MLN_EXPR_RET_ERR; }
            type = tk->type; mln_expr_free(tk);
            if (type == EXPR_TK_IF) ++count; else if (type == EXPR_TK_EOF) return MLN_EXPR_RET_ERR; else if (type == EXPR_TK_FI) { if (count-- == 0) break; } else if (type == EXPR_TK_ELSE) { if (!count) { found_else = 1; break; } }
            goto lex_if_lp;
        }
        if (found_else) {
            while (1) {
                if (*next != NULL) { tk = *next; *next = NULL; } else { if ((tk = mln_expr_token(lex)) == NULL) return MLN_EXPR_RET_ERR; }
                if (tk->type == EXPR_TK_FI) break;
                *next = tk;
                v.type = mln_expr_type_null;
                if ((rc = mln_expr_parse(lex, cb, data, &v, eof, next)) != MLN_EXPR_RET_OK) { mln_expr_val_cleanup(&v); return rc; }
                if (*eof) { mln_expr_val_cleanup(&v); return MLN_EXPR_RET_ERR; }
                mln_expr_val_replace(ret, &v);
            }
        }
    }
    return MLN_EXPR_RET_OK;
})

MLN_FUNC(static inline, int, mln_expr_parse_loop, \
         (mln_lex_t *lex, mln_expr_cb_t cb, void *data, mln_expr_val_t *ret, int *eof, mln_expr_struct_t **next), \
         (lex, cb, data, ret, eof, next), \
{
    enum mln_expr_enum type;
    int rc, is_true, count;
    mln_expr_struct_t *tk;
    mln_expr_val_t v;
    mln_string_t *loop_text = NULL;

    if (lex->cur != NULL && lex->cur->type == M_INPUT_T_BUF) {
        mln_string_t tmp;
        mln_string_nset(&tmp, lex->cur->pos, lex->cur->buf_len - (mln_size_t)(lex->cur->pos - lex->cur->buf));
        loop_text = mln_string_dup(&tmp);
    }

lex_loop_begin:
    {
        mln_lex_off_t off = mln_lex_snapshot_record(lex);
        v.type = mln_expr_type_null;
        if ((rc = mln_expr_parse(lex, cb, data, &v, eof, next)) != MLN_EXPR_RET_OK) { mln_expr_val_cleanup(&v); if (loop_text) mln_string_free(loop_text); return rc; }
        if (*eof) { mln_expr_val_cleanup(&v); if (loop_text) mln_string_free(loop_text); return MLN_EXPR_RET_ERR; }
        is_true = mln_expr_val_is_true(&v);
        mln_expr_val_cleanup(&v);

lex_loop_do:
        if (*next != NULL) { tk = *next; *next = NULL; } else { if ((tk = mln_expr_token(lex)) == NULL) { if (loop_text) mln_string_free(loop_text); return MLN_EXPR_RET_ERR; } }
        type = tk->type; mln_expr_free(tk);
        if (type == EXPR_TK_SPACE) goto lex_loop_do;
        if (type != EXPR_TK_DO) { if (loop_text) mln_string_free(loop_text); return MLN_EXPR_RET_ERR; }

        if (is_true) {
            while (1) {
                if (*next != NULL) { tk = *next; *next = NULL; } else { if ((tk = mln_expr_token(lex)) == NULL) { if (loop_text) mln_string_free(loop_text); return MLN_EXPR_RET_ERR; } }
                if (tk->type == EXPR_TK_END) { mln_expr_free(tk); break; }
                *next = tk;
                v.type = mln_expr_type_null;
                if ((rc = mln_expr_parse(lex, cb, data, &v, eof, next)) != MLN_EXPR_RET_OK) { mln_expr_val_cleanup(&v); if (loop_text) mln_string_free(loop_text); return rc; }
                if (*eof) { mln_expr_val_cleanup(&v); if (loop_text) mln_string_free(loop_text); return MLN_EXPR_RET_ERR; }
                mln_expr_val_replace(ret, &v);
            }
            mln_lex_snapshot_apply(lex, off);
            if (lex->cur == NULL && loop_text != NULL) {
                if (mln_lex_push_input_buf_stream(lex, loop_text) < 0) { mln_string_free(loop_text); return MLN_EXPR_RET_ERR; }
            }
            goto lex_loop_begin;
        } else {
            count = 0;
            while (1) {
                if (*next != NULL) { tk = *next; *next = NULL; } else {
lex_loop_skip: if ((tk = mln_expr_token(lex)) == NULL) { if (loop_text) mln_string_free(loop_text); return MLN_EXPR_RET_ERR; } }
                type = tk->type; mln_expr_free(tk);
                if (type == EXPR_TK_LOOP) ++count; else if (type == EXPR_TK_EOF) { if (loop_text) mln_string_free(loop_text); return MLN_EXPR_RET_ERR; }
                else if (type == EXPR_TK_END) { if (count-- == 0) break; }
                goto lex_loop_skip;
            }
        }
    }
    if (loop_text) mln_string_free(loop_text);
    return MLN_EXPR_RET_OK;
})

MLN_FUNC(, mln_expr_val_t *, mln_expr_run_file, (mln_string_t *path, mln_expr_cb_t cb, void *data), (path, cb, data), {
    mln_lex_t *lex = NULL;
    struct mln_lex_attr lattr;
    mln_lex_hooks_t hooks;
    mln_alloc_t *pool;
    mln_expr_val_t *ret;
    mln_expr_val_t v;
    int eof = 0, has_result = 0;
    mln_expr_struct_t *next = NULL;

    memset(&hooks, 0, sizeof(hooks));
    hooks.dblq_handler = (lex_hook)mln_expr_dblq_handler;
    hooks.sglq_handler = (lex_hook)mln_expr_sglq_handler;

    if ((pool = lattr.pool = mln_alloc_init(NULL, 0)) == NULL) return NULL;
    lattr.keywords = keywords; lattr.hooks = &hooks; lattr.preprocess = 1; lattr.padding = 0;
    lattr.type = M_INPUT_T_FILE; lattr.data = path; lattr.env = NULL;
    mln_lex_init_with_hooks(mln_expr, lex, &lattr);
    if (lex == NULL) { mln_alloc_destroy(pool); return NULL; }

    v.type = mln_expr_type_null;
    while (1) {
        mln_expr_val_t tmp;
        tmp.type = mln_expr_type_null;
        if (mln_expr_parse(lex, cb, data, &tmp, &eof, &next) != MLN_EXPR_RET_OK) { mln_expr_val_cleanup(&tmp); mln_expr_val_cleanup(&v); has_result = 0; break; }
        if (eof) { mln_expr_val_cleanup(&tmp); break; }
        mln_expr_val_replace(&v, &tmp);
        has_result = 1;
    }
    if (next != NULL) mln_expr_free(next);
    mln_lex_destroy(lex);
    mln_alloc_destroy(pool);
    if (!has_result) { if (eof) return mln_expr_val_new(mln_expr_type_null, NULL, NULL); return NULL; }
    if ((ret = (mln_expr_val_t *)malloc(sizeof(mln_expr_val_t))) == NULL) { mln_expr_val_cleanup(&v); return NULL; }
    mln_expr_val_move(ret, &v);
    return ret;
})
