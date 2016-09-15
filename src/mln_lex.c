
/*
 * Copyright (C) Niklaus F.Schen.
 */
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <ctype.h>
#include "mln_log.h"
#include "mln_string.h"
#include "mln_lex.h"

/*
 * error information
 */
char mln_lex_errmsg[][MLN_LEX_ERRMSG_LEN] = {
    "Succeed.",
    "No memory.",
    "Invalid character.",
    "Invalid hexadecimal.",
    "Invalid decimal.",
    "Invalid octal number.",
    "Invalid floating number.",
    "Read file error.",
    "Invalid end of file."
};

char error_msg_err[] = "No memory to store error message.";

mln_lex_t *mln_lex_init(struct mln_lex_attr *attr)
{
    mln_lex_t *lex = (mln_lex_t *)malloc(sizeof(mln_lex_t));
    if (lex == NULL) {
        mln_log(error, "No memory.\n");
        return NULL;
    }

    lex->filename = NULL;
    lex->keywords = attr->keywords;
    if (attr->hooks != NULL)
        memcpy(&(lex->hooks), attr->hooks, sizeof(mln_lex_hooks_t));
    else
        memset(&(lex->hooks), 0, sizeof(mln_lex_hooks_t));
    lex->file_buf = NULL;
    lex->file_cur_ptr = NULL;
    lex->result_buf = NULL;
    lex->result_cur_ptr = NULL;
    lex->err_msg = NULL;
    lex->file_buflen = MLN_DEFAULT_BUFLEN;
    lex->result_buflen = MLN_DEFAULT_BUFLEN;
    lex->error = MLN_LEX_SUCCEED;
    lex->line = 1;
    lex->fd = -1;
    if (attr->input_type == mln_lex_file) {
        lex->filename = mln_string_new(attr->input.filename);
        if (lex->filename == NULL) {
            mln_log(error, "No memory.\n");
            free(lex);
            return NULL;
        }
    } else {
        lex->file_buf = attr->input.file_buf;
        lex->file_cur_ptr = lex->file_buf;
        lex->file_buflen = strlen(lex->file_buf);
    }
    if (lex->filename == NULL && lex->file_buf == NULL) {
        mln_log(error, "Invalid attributes.\n");
        if (lex->filename != NULL) mln_string_free(lex->filename);
        free(lex);
        return NULL;
    }
    if (lex->filename == NULL) return lex;

    lex->fd = open((char *)(lex->filename->data), O_RDONLY);
    if (lex->fd < 0) {
        mln_log(error, "Open file \"%s\" error. %s\n", \
                (char *)(lex->filename->data), strerror(errno));
        mln_string_free(lex->filename);
        free(lex);
        return NULL;
    }
    return lex;
}

void mln_lex_destroy(mln_lex_t *lex)
{
    if (lex == NULL) return;
    if (lex->filename != NULL)
        mln_string_free(lex->filename);
    if (lex->fd >= 0 && lex->file_buf != NULL)
        free(lex->file_buf);
    if (lex->result_buf != NULL)
        free(lex->result_buf);
    if (lex->err_msg != NULL)
        free(lex->err_msg);
    if (lex->fd >= 0) close(lex->fd);
    free(lex);
}

char *mln_lex_strerror(mln_lex_t *lex)
{
    if (lex->err_msg != NULL) free(lex->err_msg);
    int len = (lex->fd<0?0:lex->filename->len) + strlen(mln_lex_errmsg[lex->error]);
    len += ((lex->error == MLN_LEX_EREAD)?strlen(strerror(errno)):0);
    len += (lex->result_cur_ptr - lex->result_buf + 1);
    len += 256;
    lex->err_msg = (mln_s8ptr_t)malloc(len+1);
    if (lex->err_msg == NULL) return error_msg_err;

    int n = 0;
    if (lex->fd >= 0)
        n += snprintf(lex->err_msg + n, len - n, "%s:", (char *)(lex->filename->data));
    n += snprintf(lex->err_msg + n, len - n, "%d: %s", lex->line, mln_lex_errmsg[lex->error]);
    if (lex->result_buf[0] != 0)
        n += snprintf(lex->err_msg + n, len - n, " \"%s\"", lex->result_buf);
    if (lex->error == MLN_LEX_EREAD)
        snprintf(lex->err_msg + n, len - n, ". %s", strerror(errno));
    return lex->err_msg;
}

char mln_lex_getAChar(mln_lex_t *lex)
{
    if (lex->fd < 0 && \
        (lex->file_buf == NULL || \
         *(lex->file_cur_ptr) == 0))
    {
        lex->file_cur_ptr++;
        return MLN_EOF;
    }
    if (lex->file_buf == NULL) {
        lex->file_buf = (mln_s8ptr_t)malloc(lex->file_buflen+1);
        if (lex->file_buf == NULL) {
            lex->error = MLN_LEX_ENMEM;
            return MLN_ERR;
        }
        lex->file_cur_ptr = lex->file_buf + lex->file_buflen;
        *(lex->file_cur_ptr) = 0;
    }
    int n;
    if (*(lex->file_cur_ptr) == 0) {
lp:     n = read(lex->fd, lex->file_buf, lex->file_buflen);
        if (n < 0) {
            if (errno == EINTR) goto lp;
            lex->error = MLN_LEX_EREAD;
            return MLN_ERR;
        } else if (n == 0) {
            return MLN_EOF;
        }
        lex->file_buf[n] = 0;
        lex->file_cur_ptr = lex->file_buf;
    }
    char c = *(lex->file_cur_ptr)++;
    if (c <= 0) {
        lex->error = MLN_LEX_EINVCHAR;
        return MLN_ERR;
    }
    return c;
}

void mln_lex_stepBack(mln_lex_t *lex)
{
    if (lex->file_cur_ptr <= lex->file_buf) {
        mln_log(error, "Lexer crashed.\n");
        abort();
    }
    lex->file_cur_ptr--;
}

int mln_lex_putAChar(mln_lex_t *lex, char c)
{
    if (lex->result_buf == NULL) {
        lex->result_buf = (mln_s8ptr_t)malloc(lex->result_buflen+1);
        if (lex->result_buf == NULL) {
            lex->error = MLN_LEX_ENMEM;
            return MLN_ERR;
        }
        lex->result_cur_ptr = lex->result_buf;
    }
    if (lex->result_cur_ptr >= lex->result_buf+lex->result_buflen) {
        mln_s32_t len = lex->result_buflen + 1;
        lex->result_buflen += (len>>1);
        mln_s8ptr_t tmp = lex->result_buf;
        lex->result_buf = (mln_s8ptr_t)realloc(tmp, lex->result_buflen+1);
        if (lex->result_buf == NULL) {
            lex->result_buf = tmp;
            lex->error = MLN_LEX_ENMEM;
            return MLN_ERR;
        }
        lex->result_cur_ptr = lex->result_buf + len - 1;
    }
    *(lex->result_cur_ptr)++ = c;
    *(lex->result_cur_ptr) = 0;
    return 0;
}

int mln_lex_isLetter(char c)
{
    if (c == '_' || isalpha(c))
        return 1;
    return 0;
}

int mln_lex_isOct(char c)
{
    if (c >= '0' && c < '8')
        return 1;
    return 0;
}

int mln_lex_isHex(char c)
{
    if (isdigit(c) || \
        (c >= 'a' && c <= 'f') || \
        (c >= 'A' && c <= 'F'))
        return 1;
    return 0;
}
