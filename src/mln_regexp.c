
/*
 * Copyright (C) Niklaus F.Schen.
 */

#include <stdio.h> 
#include <stdlib.h>
#include <string.h>
#include "mln_regexp.h"
#include "mln_func.h"

static int mln_match_star(char *mregexp, int mreglen, \
                          char *regexp, char *text, \
                          int reglen, int textlen, \
                          mln_reg_match_result_t *matches);
static int mln_match_here(unsigned int flag, \
                          char *regexp, char *text, \
                          int reglen, int textlen, \
                          mln_reg_match_result_t *matches);
static int mln_match_plus(char *mregexp, int mreglen, \
                          char *regexp, char *text, \
                          int reglen, int textlen, \
                          mln_reg_match_result_t *matches);
static int mln_match_question(char *mregexp, int mreglen, \
                              char *regexp, char *text, \
                              int reglen, int textlen, \
                              mln_reg_match_result_t *matches);
static int mln_match_brace(char *mregexp, int mreglen, \
                           char *regexp, char *text, \
                           int reglen, int textlen, \
                           int min, int max, \
                           mln_reg_match_result_t *matches);
static inline int
mln_match_square(char *regexp, int reglen, char **text, int *textlen, mln_reg_match_result_t *matches);
static inline void
mln_match_get_limit(char *regexp, int reglen, int *min, int *max);
static inline int mln_get_char(unsigned int flag, char *s, int len);
static inline int mln_get_length(char *s, int len);
static inline int
mln_process_or(unsigned int flag, \
               char **regexp, int *reglen, \
               char **text,   int *textlen, \
               mln_reg_match_result_t *matches);
static int
mln_or_return_val(char **regexp, int *reglen, char *rexp, int rlen, int rv);
static inline void
mln_adjust_or_pos(unsigned int flag, char **rexp, int *rlen);

MLN_FUNC(static, int, mln_match_here, \
         (unsigned int flag, char *regexp, char *text, \
          int reglen, int textlen, mln_reg_match_result_t *matches), \
         (flag, regexp, text, reglen, textlen, matches), \
{
    int steplen, count, c_0, len_0, c_n, len_n, ret;

again:
    c_0 = mln_get_char(flag, regexp, reglen);
    len_0 = mln_get_length(regexp, reglen);
    steplen = len_0;

    if (reglen < 1) {
        return textlen;
    }

    if (!(flag & M_REGEXP_SPECIAL_MASK)) {
        if (!(flag & M_REGEXP_MASK_OR)) {
            ret = mln_process_or(flag, &regexp, &reglen, &text, &textlen, matches);
            if (ret < 0) {
                return -1;
            } else if (ret > 0) {
                goto again;
            }
        }

        if (c_0 == M_REGEXP_LPAR) {
            int c, len = reglen;
            count = 0;
            while (len > 0) {
                c = mln_get_char(flag, regexp+(reglen-len), len);
                if (c == M_REGEXP_LPAR) ++count;
                if (c == M_REGEXP_RPAR && --count == 0) break;
                len -= mln_get_length(regexp+(reglen-len), len);
            }
            if (len <= 0) {
                return -1;
            }
            steplen = reglen - len + len_0;
            if (reglen - len == len_0) {
                regexp += (len_0 << 1);
                reglen -= (len_0 << 1);
                goto again;
            }
        }

        if (c_0 == M_REGEXP_LSQUAR) {
            int c, len = reglen;
            count = 0;
            while (len > 0) {
                c = mln_get_char(flag, regexp+(reglen-len), len);
                if (c == M_REGEXP_LSQUAR) ++count;
                if (c == M_REGEXP_RSQUAR && --count == 0) break;
                len -= mln_get_length(regexp+(reglen-len), len);
            }
            if (len <= 0) {
                return -1;
            }
            steplen = reglen - len + len_0;
            if (reglen - len == len_0) {
                regexp += (len_0 << 1);
                reglen -= (len_0 << 1);
                goto again;
            }
        }

        if (steplen < reglen) {
            c_n = mln_get_char(flag, regexp+steplen, reglen-steplen);
            len_n = mln_get_length(regexp+steplen, reglen-steplen);

            if (c_n == M_REGEXP_STAR) {
                return mln_match_star(regexp, steplen, \
                                      regexp+steplen+len_n, text, \
                                      reglen-steplen-len_n, textlen, \
                                      matches);
            }
            if (c_n == M_REGEXP_PLUS) {
                return mln_match_plus(regexp, steplen, \
                                      regexp+steplen+len_n, text, \
                                      reglen-steplen-len_n, textlen, \
                                      matches);
            }
            if (c_n == M_REGEXP_QUES) {
                return mln_match_question(regexp, steplen, \
                                          regexp+steplen+len_n, text, \
                                          reglen-steplen-len_n, textlen, \
                                          matches);
            }
            if (c_n == M_REGEXP_LBRACE) {
                int part = 1, min = 0, max = 0, existent = 0;
                int c, len = reglen, l = mln_get_length(regexp+steplen, reglen-steplen);
                count = 0;
                while (len > steplen) {
                    c = mln_get_char(flag, regexp+steplen+(reglen-len), reglen-steplen-(reglen-len));
                    if (c == ',') {
                        ++part;
                        len -= mln_get_length(regexp+steplen+(reglen-len), reglen-steplen-(reglen-len));
                        continue;
                    }
                    if (c == M_REGEXP_LBRACE) {
                        ++count;
                        len -= mln_get_length(regexp+steplen+(reglen-len), reglen-steplen-(reglen-len));
                        continue;
                    }
                    if (c == M_REGEXP_RBRACE) {
                        if (--count == 0) break;
                        len -= mln_get_length(regexp+steplen+(reglen-len), reglen-steplen-(reglen-len));
                        continue;
                    }
                    if (!mln_isdigit(c)) {
                        return -1;
                    }
                    existent = 1;
                    len -= mln_get_length(regexp+steplen+(reglen-len), reglen-steplen-(reglen-len));
                }
                if (len <= steplen || !existent || part > 2) {
                    return -1;
                }
                mln_match_get_limit(regexp+steplen+l, reglen-len-l, &min, &max);
                if (max > 0 && min > max) {
                    return -1;
                }
                return mln_match_brace(regexp, steplen, \
                                       regexp+steplen+(reglen-len)+l, text, \
                                       len-steplen-l, textlen, \
                                       min, max, matches);
            }
        }

        if (c_0 == M_REGEXP_XOR && (flag & M_REGEXP_MASK_NEW)) {
            regexp += len_0;
            reglen -= len_0;
            goto again;
        }
        flag &= ~M_REGEXP_MASK_NEW;
        if (c_0 == M_REGEXP_DOLL) {
            return textlen < 1? textlen: -1;
        }
        if (flag & M_REGEXP_MASK_SQUARE && reglen > len_0 && textlen > 0) {
            int sub_c, sub_len;
            sub_c = mln_get_char(flag, regexp+len_0, reglen-len_0);
            sub_len = mln_get_length(regexp+len_0, reglen-len_0);
            if (sub_c == M_REGEXP_SUB && reglen > len_0+sub_len) {
                sub_c = mln_get_char(flag, regexp+len_0+sub_len, reglen-len_0-sub_len);
                sub_len = mln_get_length(regexp+len_0+sub_len, reglen-len_0-sub_len);
                if (text[0] >= c_0 && text[0] <= sub_c) {
                    return --textlen;
                }
                return -1;
            }
        }
        if (c_0 == M_REGEXP_NUM && textlen > 0) {
            if (!mln_isdigit(*text)) {
                return -1;
            }
            ++text;
            --textlen;
            regexp += len_0;
            reglen -= len_0;
            goto again;
        }
        if (c_0 == M_REGEXP_NOT_NUM && textlen > 0) {
            if (mln_isdigit(*text)) {
                return -1;
            }
            ++text;
            --textlen;
            regexp += len_0;
            reglen -= len_0;
            goto again;
        }
        if (c_0 == M_REGEXP_ALPHA && textlen > 0) {
            if (!mln_isalpha(*text)) {
                return -1;
            }
            ++text;
            --textlen;
            regexp += len_0;
            reglen -= len_0;
            goto again;
        }
        if (c_0 == M_REGEXP_NOT_ALPHA && textlen > 0) {
            if (mln_isalpha(*text)) {
                return -1;
            }
            ++text;
            --textlen;
            regexp += len_0;
            reglen -= len_0;
            goto again;
        }
        if (c_0 == M_REGEXP_WHITESPACE && textlen > 0) {
            if (!mln_iswhitespace(*text)) {
                return -1;
            }
            ++text;
            --textlen;
            regexp += len_0;
            reglen -= len_0;
            goto again;
        }
        if (c_0 == M_REGEXP_NOT_WHITESPACE && textlen > 0) {
            if (mln_iswhitespace(*text)) {
                return -1;
            }
            ++text;
            --textlen;
            regexp += len_0;
            reglen -= len_0;
            goto again;
        }
        if (c_0 == M_REGEXP_DOT && textlen > 0) {
            regexp += len_0;
            ++text;
            reglen -= len_0;
            --textlen;
            goto again;
        }
    }

    if (c_0 == M_REGEXP_LSQUAR) {
        if (mln_match_square(regexp, steplen, &text, &textlen, matches) < 0) {
            return -1;
        }
        regexp += steplen;
        reglen -= steplen;
        goto again;
    }

    if (c_0 == M_REGEXP_LPAR) {
        int left = mln_match_here(M_REGEXP_MASK_NEW, regexp+len_0, text, steplen-(len_0<<1), textlen, matches);
        if (left < 0) {
            return -1;
        }
        if (matches != NULL) {
            mln_string_t *s;
            if ((s = (mln_string_t *)mln_array_push(matches)) == NULL) {
                return -1;
            }
            mln_string_nset(s, text, textlen - left);
        }
        regexp += steplen;
        reglen -= steplen;
        text += (textlen - left);
        textlen = left;
        goto again;
    }

    if (textlen > 0 && c_0 == (unsigned char)(text[0])) {
        if (flag & M_REGEXP_SPECIAL_MASK) return textlen;
        regexp += len_0;
        ++text;
        reglen -= len_0;
        --textlen;
        goto again;
    }

    return -1;
})

MLN_FUNC(static inline, int, mln_process_or, \
         (unsigned int flag, char **regexp, int *reglen, \
          char **text,   int *textlen, mln_reg_match_result_t *matches), \
         (flag, regexp, reglen, text, textlen, matches), \
{
    char *rexp = *regexp;
    int rlen = *reglen;
    int c, len, left, count, ret, last = 0;
    int loop = 0, rv = 0, match_len;

again:
    c = mln_get_char(flag, rexp, rlen);
    len = mln_get_length(rexp, rlen);

    left = rlen;
    count = 0;
    if (c == M_REGEXP_LSQUAR) {
        while (left > 0) {
            c = mln_get_char(flag, rexp+(rlen-left), left);
            left -= mln_get_length(rexp+(rlen-left), left);
            if (c == M_REGEXP_LSQUAR) ++count;
            if (c == M_REGEXP_RSQUAR && --count == 0) break;
        }
    } else if (c == M_REGEXP_LPAR) {
        while (left > 0) {
            c = mln_get_char(flag, rexp+(rlen-left), left);
            left -= mln_get_length(rexp+(rlen-left), left);
            if (c == M_REGEXP_LPAR) ++count;
            if (c == M_REGEXP_RPAR && --count == 0) break;
        }
    } else {
        if (last == 1 && (c == M_REGEXP_OR || rlen <= 0)) {
            if (rlen > 0) {
                rexp += len;
                rlen -= len;
            }
            mln_adjust_or_pos(flag, &rexp, &rlen);
            return mln_or_return_val(regexp, reglen, rexp, rlen, rv);
        }
        left -= len;
    }

    last = 0;

    if (left <= 0) {
        if (loop == 0) return mln_or_return_val(regexp, reglen, rexp, rlen, rv);
        loop = 0;
        match_len = rlen;
        goto match;
    }

    c = mln_get_char(flag, rexp+(rlen-left), left);
    len = mln_get_length(rexp+(rlen-left), left);
    if (c == M_REGEXP_STAR || c == M_REGEXP_QUES || c == M_REGEXP_PLUS) {
        left -= len;
        c = mln_get_char(flag, rexp+(rlen-left), left);
        len = mln_get_length(rexp+(rlen-left), left);
    } else if (flag & M_REGEXP_MASK_SQUARE && c == M_REGEXP_SUB) {
        left -= len;
        left -= mln_get_length(rexp+(rlen-left), left);
        c = mln_get_char(flag, rexp+(rlen-left), left);
        len = mln_get_length(rexp+(rlen-left), left);
    } else if (c == M_REGEXP_LBRACE) {
        while (left > 0) {
            c = mln_get_char(flag, rexp+(rlen-left), left);
            left -= mln_get_length(rexp+(rlen-left), left);
            if (c == M_REGEXP_LBRACE) ++count;
            if (c == M_REGEXP_RBRACE && --count == 0) break;
        }
        c = mln_get_char(flag, rexp+(rlen-left), left);
        len = mln_get_length(rexp+(rlen-left), left);
    }

    if (c != M_REGEXP_OR) {
        if (loop == 0) return mln_or_return_val(regexp, reglen, rexp, rlen, rv);
        loop = 0;
        match_len = left > 0? rlen-left: rlen;
        goto match;
    }

    last = loop = rv = 1;
    match_len = rlen - left;

match:
    ret = mln_match_here(flag|M_REGEXP_MASK_OR, rexp, *text, match_len, *textlen, matches);

    rexp += match_len;
    rlen -= match_len;
    if (left > 0 && c == M_REGEXP_OR) {
        rexp += len;
        rlen -= len;
    }

    if (ret >=0 ) {
        mln_adjust_or_pos(flag, &rexp, &rlen);
        (*text) += (*textlen - ret);
        (*textlen) = ret;
        return mln_or_return_val(regexp, reglen, rexp, rlen, rv);
    }

    if (loop) goto again;
    return mln_or_return_val(regexp, reglen, rexp, rlen, -1);
})

MLN_FUNC(static, int, mln_or_return_val, \
         (char **regexp, int *reglen, char *rexp, int rlen, int rv), \
         (regexp, reglen, rexp, rlen, rv), \
{
    *regexp = rexp;
    *reglen = rlen;
    return rv;
})

MLN_FUNC_VOID(static inline, void, mln_adjust_or_pos, \
              (unsigned int flag, char **rexp, int *rlen), \
              (flag, rexp, rlen), \
{
    int c, len, count;

again:
    if (*rlen == 0) return;

    c = mln_get_char(flag, *rexp, *rlen);
    len = mln_get_length(*rexp, *rlen);

    count = 0;
    if (c == M_REGEXP_LSQUAR) {
        while (*rlen > 0) {
            c = mln_get_char(flag, *rexp, *rlen);
            len = mln_get_length(*rexp, *rlen);
            (*rexp) += len;
            (*rlen) -= len;
            if (c == M_REGEXP_LSQUAR) ++count;
            if (c == M_REGEXP_RSQUAR && --count == 0) break;
        }
    } else if (c == M_REGEXP_LPAR) {
        while (*rlen > 0) {
            c = mln_get_char(flag, *rexp, *rlen);
            len = mln_get_length(*rexp, *rlen);
            (*rexp) += len;
            (*rlen) -= len;
            if (c == M_REGEXP_LPAR) ++count;
            if (c == M_REGEXP_RPAR && --count == 0) break;
        }
    } else {
        (*rexp) += len;
        (*rlen) -= len;
    }

    if (*rlen <= 0) return;

    c = mln_get_char(flag, *rexp, *rlen);
    len = mln_get_length(*rexp, *rlen);

    if (c == M_REGEXP_STAR || c == M_REGEXP_QUES || c == M_REGEXP_PLUS) {
        (*rexp) += len;
        (*rlen) -= len;
        if (*rlen <= 0) return;
        c = mln_get_char(flag, *rexp, *rlen);
        len = mln_get_length(*rexp, *rlen);
    } else if (flag & M_REGEXP_MASK_SQUARE && c == M_REGEXP_SUB) {
        (*rexp) += len;
        (*rlen) -= len;
        if (*rlen <= 0) return;
        len = mln_get_length(*rexp, *rlen);
        (*rexp) += len;
        (*rlen) -= len;
        if (*rlen <= 0) return;
        c = mln_get_char(flag, *rexp, *rlen);
        len = mln_get_length(*rexp, *rlen);
    } else if (c == M_REGEXP_LBRACE) {
        while (*rlen > 0) {
            c = mln_get_char(flag, *rexp, *rlen);
            len = mln_get_length(*rexp, *rlen);
            (*rexp) += len;
            (*rlen) -= len;
            if (c == M_REGEXP_LPAR) ++count;
            if (c == M_REGEXP_RPAR && --count == 0) break;
        }
        if (*rlen <= 0) return;
        c = mln_get_char(flag, *rexp, *rlen);
        len = mln_get_length(*rexp, *rlen);
    }

    if (c == M_REGEXP_OR) {
        (*rexp) += len;
        (*rlen) -= len;
        goto again;
    }
})

MLN_FUNC(static inline, int, mln_match_square, \
         (char *regexp, int reglen, char **text, int *textlen, mln_reg_match_result_t *matches), \
         (regexp, reglen, text, textlen, matches), \
{
    int c, len, reverse = 0, count, left, steplen;
    int end_c, tmp_c, tmp_len;

    /*jump off '['*/
    len = mln_get_length(regexp, reglen);
    regexp += len;
    reglen -= len;

    c = mln_get_char(M_REGEXP_MASK_SQUARE, regexp, reglen);
    len = mln_get_length(regexp, reglen);
    if (c == M_REGEXP_XOR) {
        reverse = 1;
        regexp += len;
        reglen -= len;
    }

again:
    while (reglen > 0) {
        c = mln_get_char(M_REGEXP_MASK_SQUARE, regexp, reglen);
        len = mln_get_length(regexp, reglen);

        if (c == M_REGEXP_RSQUAR) break;

        switch (c) {
            case M_REGEXP_LBRACE:
                end_c = M_REGEXP_RBRACE;
                break;
            case M_REGEXP_LPAR:
                end_c = M_REGEXP_RPAR;
                break;
            case M_REGEXP_LSQUAR:
                end_c = M_REGEXP_RSQUAR;
                break;
            default:
                if (c > M_REGEXP_SEPARATOR) {
                    regexp += len;
                    reglen -= len;
                    goto again;
                }
                end_c = 0;
                break;
        }

        if (end_c == 0) {
            steplen = len;
            if (reglen > len) {
                int sub_c, sub_len;
                sub_c = mln_get_char(M_REGEXP_MASK_SQUARE, regexp+len, reglen-len);
                sub_len = mln_get_length(regexp+len, reglen-len);
                if (sub_c == M_REGEXP_SUB && reglen > len+sub_len) {
                    steplen += (sub_len + mln_get_length(regexp+len+sub_len, reglen-len-sub_len));
                }
            }
        } else {
            steplen = -1;
            count = 0;
            tmp_len = reglen;
            while (tmp_len > 1) {
                tmp_c = mln_get_char(M_REGEXP_MASK_SQUARE, regexp+(reglen-tmp_len), tmp_len);
                tmp_len -= mln_get_length(regexp+(reglen-tmp_len), tmp_len);
                if (tmp_c == c) ++count;
                if (tmp_c == end_c && --count == 0) {
                    steplen = reglen - tmp_len;
                    break;
                }
            }
            if (steplen < 0) return -1;
            if (steplen == (len << 1)) {
                regexp += steplen;
                reglen = tmp_len;
                continue;
            }
        }

        if (*textlen <= 0) return -1;
        left = mln_match_here(M_REGEXP_MASK_SQUARE, regexp, *text, steplen, *textlen, matches);
        if (left >= 0) {
            if (!reverse) {
                (*text) += (*textlen - left);
                *textlen = left;
                return left;
            } else {
                return -1;
            }
        }

        regexp += steplen;
        reglen -= steplen;
    }
 
    if (reverse) {
        if (*textlen <= 0) return -1;
        --(*textlen);
        ++(*text);
        return *textlen;
    }
    if (*textlen <= 0) return *textlen;
    return -1;
})

MLN_FUNC_VOID(static inline, void, mln_match_get_limit, \
              (char *regexp, int reglen, int *min, int *max), \
              (regexp, reglen, min, max), \
{
    int val;
    char *p, *end = regexp + reglen;

    for (val = 0, p = regexp; p < end && *p != ','; ++p) {
        val += (val * 10 + (*p - '0'));
    }
    *min = *max = val;

    if (p >= end) return;

    *max = -1;
    for (val = 0, ++p; p < end; ++p) {
        val += (val * 10 + (*p - '0'));
    }
    if (val == 0) return;

    *max = val;
})

MLN_FUNC(static, int, mln_match_star, \
         (char *mregexp, int mreglen, char *regexp, char *text, \
          int reglen, int textlen, mln_reg_match_result_t *matches), \
         (mregexp, mreglen, regexp, text, reglen, textlen, matches), \
{
    int ret;
    char dot = (char)M_REGEXP_DOT;
    char *record_text = NULL;
    int record_len = -1;

    if (textlen <= 0) return textlen;

    if (mreglen > 1) {
        int found = 0;
again:
        ret = mln_match_here(0, mregexp, text, mreglen, textlen, matches);
        if (ret < 0) {
            if (reglen <= 0) return found? textlen: ret;
            ret =  mln_match_here(0, regexp, text, reglen, textlen, matches);
            if (found) {
                return ret < 0? textlen: ret;
            } else {
                return ret;
            }
        } else {
            found = 1;
            text += (textlen - ret);
            textlen = ret;
            if (textlen > 0) goto again;
            if (reglen > 0) return mln_match_here(0, regexp, text, reglen, textlen, matches);
            return 0;
        }
    }

    while (textlen > 0 && \
           (mln_match_here(M_REGEXP_STAR, mregexp, text, mreglen, textlen, matches) >= 0 || \
            mln_match_here(M_REGEXP_STAR, mregexp, &dot, mreglen, 1, matches) >= 0))
    {
        ++text;
        --textlen;
        if (reglen > 0) {
            if (mln_match_here(0, regexp, text, reglen, textlen, NULL) >= 0) {
                record_text = text;
                record_len = textlen;
            }
        }
    }

    if (reglen > 0) {
        if (record_text != NULL) {
            text = record_text;
            textlen = record_len;
        }
        return mln_match_here(0, regexp, text, reglen, textlen, matches);
    }

    return textlen;
})

MLN_FUNC(static, int, mln_match_plus, \
         (char *mregexp, int mreglen, char *regexp, char *text, \
          int reglen, int textlen, mln_reg_match_result_t *matches), \
         (mregexp, mreglen, regexp, text, reglen, textlen, matches), \
{
    int ret, found = 0;
    char dot = (char)M_REGEXP_DOT;

    if (mreglen > 1) {
again:
        ret = mln_match_here(0, mregexp, text, mreglen, textlen, matches);
        if (ret < 0) {
            if (found == 0) return ret;
            return mln_match_here(0, regexp, text, reglen, textlen, matches);
        } else {
            found = 1;
            text += (textlen - ret);
            textlen = ret;
            if (textlen > 0) goto again;
            if (reglen > 0) return mln_match_here(0, regexp, text, reglen, textlen, matches);
            return 0;
        }
    }

    while (textlen > 0 && \
           (mln_match_here(M_REGEXP_PLUS, mregexp, text, mreglen, textlen, matches) >= 0 || \
            mln_match_here(M_REGEXP_PLUS, mregexp, &dot, mreglen, 1, matches) >= 0))
    {
        found = 1;
        ++text;
        --textlen;
    }
    if (found) {
        if (textlen > 0)
            return mln_match_here(0, regexp, text, reglen, textlen, matches);
        return textlen;
    }

    return -1;
})

MLN_FUNC(static, int, mln_match_question, \
         (char *mregexp, int mreglen, char *regexp, char *text, \
          int reglen, int textlen, mln_reg_match_result_t *matches), \
         (mregexp, mreglen, regexp, text, reglen, textlen, matches), \
{
    int ret;

    if (mreglen > 1) {
        ret = mln_match_here(0, mregexp, text, mreglen, textlen, matches);
        if (ret >= 0) {
            text += (textlen - ret);
            textlen = ret;
        }
        return mln_match_here(0, regexp, text, reglen, textlen, matches);
    }

    if (mln_match_here(M_REGEXP_QUES, mregexp, text, mreglen, textlen, matches) >= 0)
        return mln_match_here(0, regexp, text+1, reglen, textlen-1, matches);
    return mln_match_here(0, regexp, text, reglen, textlen, matches);
})

MLN_FUNC(static, int, mln_match_brace, \
         (char *mregexp, int mreglen, char *regexp, char *text, \
          int reglen, int textlen, int min, int max, mln_reg_match_result_t *matches), \
         (mregexp, mreglen, regexp, text, reglen, textlen, min, max, matches), \
{
    int ret, found = 0;
    char dot = (char)M_REGEXP_DOT;

    if (mreglen > 1) {
again:
        ret = mln_match_here(0, mregexp, text, mreglen, textlen, matches);
        if (ret < 0) {
            if (reglen <= 0 || found < min) return ret;
            return mln_match_here(0, regexp, text, reglen, textlen, matches);
        } else {
            ++found;
            text += (textlen - ret);
            textlen = ret;
            if (textlen > 0 && (max < 0 || found < max)) goto again;
            if (textlen <= 0 && reglen <= 0) return 0;
            return mln_match_here(0, regexp, text, reglen, textlen, matches);
        }
    }

    while (textlen > 0 && \
           (mln_match_here(M_REGEXP_LBRACE, mregexp, text, mreglen, textlen, matches) >= 0 || \
            mln_match_here(M_REGEXP_LBRACE, mregexp, &dot, mreglen, 1, matches) >= 0))
    {
        ++found;
        ++text;
        --textlen;
        if (max > 0 && found >= max) break;
    }
    if (found >= min) {
        if (textlen > 0 || reglen > 0)
            return mln_match_here(0, regexp, text, reglen, textlen, matches);
        return textlen;
    }

    return -1;
})

MLN_FUNC(static inline, int, mln_get_char, (unsigned int flag, char *s, int len), (flag, s, len), {
    if (len <= 0) return 0;

    if (s[0] == '\\' && len > 1) {
        switch (s[1]) {
            case 'n':
                return '\n';
            case 't':
                return '\t';
            case '-':
                return '-';
            case '|':
                return '|';
            case '\\':
                return '\\';
            case 'w':
                return M_REGEXP_ALPHA;
            case 'W':
                return M_REGEXP_NOT_ALPHA;
            case 'd':
                return M_REGEXP_NUM;
            case 'D':
                return M_REGEXP_NOT_NUM;
            case 's':
                return M_REGEXP_WHITESPACE;
            case 'S':
                return M_REGEXP_NOT_WHITESPACE;
            default:
                return s[1];
        }
    }

    switch (s[0]) {
        case '{':
            return M_REGEXP_LBRACE;
        case '}':
            return M_REGEXP_RBRACE;
        case '(':
            return M_REGEXP_LPAR;
        case ')':
            return M_REGEXP_RPAR;
        case '[':
            return M_REGEXP_LSQUAR;
        case ']':
            return M_REGEXP_RSQUAR;
        case '^':
            return M_REGEXP_XOR;
        case '*':
            return M_REGEXP_STAR;
        case '$':
            return M_REGEXP_DOLL;
        case '.':
            return M_REGEXP_DOT;
        case '?':
            return M_REGEXP_QUES;
        case '+':
            return M_REGEXP_PLUS;
        case '-':
            if (flag & M_REGEXP_MASK_SQUARE)
                return M_REGEXP_SUB;
            return '-';
        case '|':
            return M_REGEXP_OR;
        default:
            break;
    }

    return s[0];
})

MLN_FUNC(static inline, int, mln_get_length, (char *s, int len), (s, len), {
    if (s[0] == '\\' && len > 1) {
        switch (s[1]) {
            case 'n':
            case 't':
            case '-':
            case '|':
            case 'w':
            case 'W':
            case 'd':
            case 'D':
            case 's':
            case 'S':
            case '\\':
            default:
                return 2;
        }
    }

    return 1;
})

MLN_FUNC(, int, mln_reg_match, \
         (mln_string_t *exp, mln_string_t *text, mln_reg_match_result_t *matches), \
         (exp, text, matches), \
{
    int ret;
    mln_string_t *s;
    mln_u8ptr_t p = text->data;
    mln_size_t len = text->len;

again:
    ret = mln_match_here(M_REGEXP_MASK_NEW, (char *)(exp->data), (char *)p, exp->len, len, matches);
    if (ret < 0) {
        if (exp->len && exp->data[0] != '^' && len > 0) {
            ++p, --len;
            goto again;
        }
        return mln_array_nelts(matches);
    }
    if (len - ret > 0) {
        if ((s = (mln_string_t *)mln_array_push(matches)) == NULL) {
            return -1;
        }
        mln_string_nset(s, p, len - ret);

        if (ret >= exp->len) {
            p += (len - ret);
            len = ret;
            goto again;
        }
    }
    return mln_array_nelts(matches);
})

MLN_FUNC(, int, mln_reg_equal, (mln_string_t *exp, mln_string_t *text), (exp, text), {
    int ret;
    mln_u8ptr_t p = text->data;
    mln_size_t len = text->len;

again:
    ret = mln_match_here(M_REGEXP_MASK_NEW, (char *)(exp->data), (char *)p, exp->len, len, NULL);
    if (ret < 0) {
        if (exp->len && exp->data[0] != '^' && len > 0) {
            ++p, --len;
            goto again;
        }
        return ret;
    }

    return ret && exp->len && exp->data[exp->len - 1] != '$'? 0: ret;
})

