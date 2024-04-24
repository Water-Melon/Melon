#include "mln_bignum.h"

int main(int argc, char *argv[])
{
    mln_string_t *s;
    mln_bignum_t n1, n2 = mln_bignum_zero();

    mln_bignum_init(n1); //same as mln_bignum_zero

    mln_bignum_assign(&n1, "10", 2);
    mln_bignum_assign(&n2, "30", 2);

    mln_bignum_pwr(&n1, &n2, NULL);
    s = mln_bignum_tostring(&n1);
    write(STDOUT_FILENO, s->data, s->len);
    write(STDOUT_FILENO, "\n", 1);
    mln_string_free(s);

    return 0;
}
