
/*
 * Copyright (C) Niklaus F.Schen.
 */

#include "mln_error.h"
#include <stdio.h>

mln_string_t *mln_error_filenames = NULL;
mln_string_t *mln_error_errmsgs = NULL;
mln_size_t    mln_error_nfile = 0;
mln_size_t    mln_error_nmsg = 0;

void mln_error_init(mln_string_t *filenames, mln_string_t *errmsgs, mln_size_t nfile, mln_size_t nmsg)
{
    mln_error_filenames = filenames;
    mln_error_errmsgs = errmsgs;
    mln_error_nfile = nfile;
    mln_error_nmsg = nmsg;
}

char *mln_error_string(int err, void *buf, mln_size_t len)
{
    int i = 0, line = 0;
    char *b = (char *)buf, *f = "Unkown File", *l = "Unknown Line", *c = "Unknown Code";
    char intstr[8] = {0};

    if (err > 0) {
        i = snprintf(b, len - 1, "Invalid error code");
    } else if (err < 0) {
        err = -err;
        i = (err >> 22) & 0x1ff;
        line = (err >> 8) & 0x3fff;
        err &= 0xff;
        snprintf(intstr, sizeof(intstr) - 1, "%d", line);

        i = snprintf(b, len - 1, "%s:%s:%s", \
                (i == 0x1ff? f: (char *)(mln_error_filenames[i].data)), \
                (line == 0x3fff? l: intstr), \
                ((err == 0xff || mln_error_errmsgs == NULL)? c: (char *)(mln_error_errmsgs[err].data)));
    } else {
        i = snprintf(b, len - 1, "Success");
    }
    b[i] = 0;

    return b;
}

