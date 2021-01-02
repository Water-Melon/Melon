
/*
 * Copyright (C) Niklaus F.Schen.
 */

#ifndef __MLN_REGEXP_H
#define __MLN_REGEXP_H

#include "mln_string.h"

#define M_REGEXP_MASK_SQUARE  ((unsigned int)0x00800000)
#define M_REGEXP_MASK_OR      ((unsigned int)0x01000000)
#define M_REGEXP_MASK_NEW     ((unsigned int)0x02000000)
#define M_REGEXP_SPECIAL_MASK ((unsigned int)0x0000ffff)

#define M_REGEXP_ALPHA        128
#define M_REGEXP_NUM          129
#define M_REGEXP_NOT_NUM      130
#define M_REGEXP_SEPARATOR    160

#define M_REGEXP_LBRACE       161
#define M_REGEXP_RBRACE       162
#define M_REGEXP_LPAR         163
#define M_REGEXP_RPAR         164
#define M_REGEXP_LSQUAR       165
#define M_REGEXP_RSQUAR       166
#define M_REGEXP_XOR          167
#define M_REGEXP_STAR         168
#define M_REGEXP_DOLL         169
#define M_REGEXP_DOT          170
#define M_REGEXP_QUES         171
#define M_REGEXP_PLUS         172
#define M_REGEXP_SUB          173
#define M_REGEXP_OR           174

typedef struct mln_reg_match_s {
    mln_string_t            data;
    struct mln_reg_match_s *prev;
    struct mln_reg_match_s *next;
} mln_reg_match_t;

extern int mln_reg_match(mln_string_t *exp, mln_string_t *text, mln_reg_match_t **head, mln_reg_match_t **tail);
extern int mln_reg_equal(mln_string_t *exp, mln_string_t *text);
extern void mln_reg_match_result_free(mln_reg_match_t *results);

#endif

