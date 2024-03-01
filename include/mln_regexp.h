
/*
 * Copyright (C) Niklaus F.Schen.
 */

#ifndef __MLN_REGEXP_H
#define __MLN_REGEXP_H

#include "mln_string.h"
#include "mln_array.h"

typedef mln_array_t mln_reg_match_result_t;

#define M_REGEXP_MASK_SQUARE  ((unsigned int)0x00800000)
#define M_REGEXP_MASK_OR      ((unsigned int)0x01000000)
#define M_REGEXP_MASK_NEW     ((unsigned int)0x02000000)
#define M_REGEXP_SPECIAL_MASK ((unsigned int)0x0000ffff)

#define M_REGEXP_ALPHA          128
#define M_REGEXP_NOT_ALPHA      129
#define M_REGEXP_NUM            130
#define M_REGEXP_NOT_NUM        131
#define M_REGEXP_WHITESPACE     132
#define M_REGEXP_NOT_WHITESPACE 133
#define M_REGEXP_SEPARATOR      160

#define M_REGEXP_LBRACE         161
#define M_REGEXP_RBRACE         162
#define M_REGEXP_LPAR           163
#define M_REGEXP_RPAR           164
#define M_REGEXP_LSQUAR         165
#define M_REGEXP_RSQUAR         166
#define M_REGEXP_XOR            167
#define M_REGEXP_STAR           168
#define M_REGEXP_DOLL           169
#define M_REGEXP_DOT            170
#define M_REGEXP_QUES           171
#define M_REGEXP_PLUS           172
#define M_REGEXP_SUB            173
#define M_REGEXP_OR             174


#define mln_reg_match_result_new(prealloc) mln_array_new(NULL, sizeof(mln_string_t), (prealloc))
#define mln_reg_match_result_free(res)  mln_array_free(res)
#define mln_reg_match_result_get(res)   (mln_string_t *)mln_array_elts(res)

extern int mln_reg_match(mln_string_t *exp, mln_string_t *text, mln_reg_match_result_t *matches) __NONNULL3(1,2,3);
extern int mln_reg_equal(mln_string_t *exp, mln_string_t *text) __NONNULL2(1,2);

#endif

