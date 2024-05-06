#include <stdio.h>
#include <assert.h>
#include "mln_regexp.h"

int main(int argc, char *argv[])
{
    mln_reg_match_result_t *res = NULL;
    mln_string_t text = mln_string("dabcdeadcbef");
    mln_string_t exp = mln_string("a.c.e");
    mln_string_t *s;
    int i, n;

    assert((res = mln_reg_match_result_new(1)) != NULL);

    n = mln_reg_match(&exp, &text, res);
    assert(n);
    printf("%d matched\n", n);

    s = mln_reg_match_result_get(res);
    for (i = 0; i < n; ++i) {
        write(STDOUT_FILENO, s[i].data, s[i].len);
        write(STDOUT_FILENO, "\n", 1);
    }

    mln_reg_match_result_free(res);

    assert(!mln_reg_equal(&exp, &text));

    return 0;
}
