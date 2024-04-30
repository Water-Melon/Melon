#include <stdio.h>
#include "mln_regexp.h"

int main(int argc, char *argv[])
{
    mln_reg_match_result_t *res = NULL;
    mln_string_t text = mln_string("Hello world");
    mln_string_t exp = mln_string("(.*e(ll)o)");
    mln_string_t *s;
    int i, n;

    if ((res = mln_reg_match_result_new(1)) == NULL) {
        fprintf(stderr, "new match result failed.\n");
        return -1;
    }

    n = mln_reg_match(&exp, &text, res);
    printf("matched: %d\n", n);

    s = mln_reg_match_result_get(res);
    for (i = 0; i < n; ++i) {
        write(STDOUT_FILENO, s[i].data, s[i].len);
        write(STDOUT_FILENO, "\n", 1);
    }

    mln_reg_match_result_free(res);
    return 0;
}

