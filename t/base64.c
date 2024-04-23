#include <stdio.h>
#include <stdlib.h>
#include "mln_string.h"
#include "mln_base64.h"

int main(int argc, char *argv[])
{
    mln_string_t text = mln_string("Hello");
    mln_string_t tmp;
    mln_u8ptr_t p1, p2;
    mln_uauto_t len1, len2;

    if (mln_base64_encode(text.data, text.len, &p1, &len1) < 0) {
        fprintf(stderr, "encode failed\n");
        return -1;
    }
    mln_string_nset(&tmp, p1, len1);
    write(STDOUT_FILENO, tmp.data, tmp.len);
    write(STDOUT_FILENO, "\n", 1);

    if (mln_base64_decode(p1, len1, &p2, &len2) < 0) {
        fprintf(stderr, "decode failed\n");
        return -1;
    }
    mln_string_nset(&tmp, p2, len2);
    write(STDOUT_FILENO, tmp.data, tmp.len);
    write(STDOUT_FILENO, "\n", 1);

    mln_base64_free(p1);
    mln_base64_free(p2);

    return 0;
}
