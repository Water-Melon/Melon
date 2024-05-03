
/*
 * Copyright (C) Niklaus F.Schen.
 */
#ifndef __MLN_ERROR_H
#define __MLN_ERROR_H

#include "mln_utils.h"
#include "mln_string.h"

typedef void (*mln_error_cb_t)(int, void *);

extern mln_string_t   *mln_error_filenames;
extern mln_string_t   *mln_error_errmsgs;
extern mln_size_t      mln_error_nfile;
extern mln_size_t      mln_error_nmsg;
extern void           *mln_error_udata;
extern mln_error_cb_t  mln_error_callback;

#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

#if defined(MSVC)
#define RET(code) do {\
    int r, l = __LINE__ >= 0x3fff? 0x3fff: (__LINE__ & 0x3fff);\
    int c = ((code) < 0 || (code) >= 0xff)? 0xff: (((code) & 0xff) >= mln_error_nmsg? 0xff: ((code) & 0xff));\
    if (c > 0) {\
        if (mln_error_filenames == NULL) {\
            r = 0x1ff;\
        } else {\
            mln_string_t tmp = mln_string(__FILENAME__);\
            for (r = 0; r < mln_error_nfile; ++r) {\
                if (!mln_string_strcmp(&tmp, &mln_error_filenames[r]))\
                    break;\
            }\
            if (r >= 0x1ff || r >= mln_error_nfile) {\
                r = 0x1ff;\
            }\
        }\
        r = (r << 22) | (l << 8) | c;\
        r = -r;\
    } else {\
        r = 0;\
    }\
    if (mln_error_callback != NULL) mln_error_callback(r, mln_error_udata);\
    (code) = r;\
} while (0)
#else
#define RET(code) ({\
    int r, l = __LINE__ >= 0x3fff? 0x3fff: (__LINE__ & 0x3fff);\
    int c = ((code) < 0 || (code) >= 0xff)? 0xff: (((code) & 0xff) >= mln_error_nmsg? 0xff: ((code) & 0xff));\
    if (c > 0) {\
        if (mln_error_filenames == NULL) {\
            r = 0x1ff;\
        } else {\
            mln_string_t tmp;\
            mln_string_set(&tmp, __FILENAME__);\
            for (r = 0; r < mln_error_nfile; ++r) {\
                if (!mln_string_strcmp(&tmp, &mln_error_filenames[r]))\
                    break;\
            }\
            if (r >= 0x1ff || r >= mln_error_nfile) {\
                r = 0x1ff;\
            }\
        }\
        r = (r << 22) | (l << 8) | c;\
        r = -r;\
    } else {\
        r = 0;\
    }\
    if (mln_error_callback != NULL) mln_error_callback(r, mln_error_udata);\
    r;\
})
#endif

#define CODE(r)   ((-(r)) & 0xff)

extern void mln_error_init(mln_string_t *filenames, mln_string_t *errmsgs, mln_size_t nfile, mln_size_t nmsg);
extern char *mln_error_string(int err, void *buf, mln_size_t len);
extern void mln_error_callback_set(mln_error_cb_t cb, void *udata);

#endif

