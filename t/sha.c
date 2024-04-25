#include <stdio.h>
#include "mln_sha.h"

int main(int argc, char *argv[])
{
    mln_sha256_t s;
    char text[1024] = {0};

    mln_sha256_init(&s);
    mln_sha256_calc(&s, (mln_u8ptr_t)"Hello", sizeof("Hello")-1, 1);
    mln_sha256_tostring(&s, text, sizeof(text)-1);
    printf("%s\n", text);

    return 0;
}
