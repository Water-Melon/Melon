
/*
 * Copyright (C) Niklaus F.Schen.
 */
#ifndef __MLN_LANG_STRING_H
#define __MLN_LANG_STRING_H

#include "mln_lang.h"
#include "mln_regexp.h"

typedef struct mln_lang_string_pos_s {
    struct mln_lang_string_pos_s *prev;
    struct mln_lang_string_pos_s *next;
    mln_lang_array_elem_t        *elem;
    mln_s64_t                     off;
} mln_lang_string_pos_t;

struct mln_lang_string_replace_s {
    mln_lang_ctx_t               *ctx;
    mln_string_t                 *s;
    mln_lang_string_pos_t        *head;
    mln_lang_string_pos_t        *tail;
};

extern int mln_lang_string(mln_lang_ctx_t *ctx);

#endif

