#include <stdio.h>
#include "mln_md5.h"

int main(int argc, char *argv[])
{
    mln_md5_t m;
    char text[] = "Hello";
    char output[33] = {0};

    mln_md5_init(&m);
    mln_md5_calc(&m, (mln_u8ptr_t)text, sizeof(text)-1, 1);
    mln_md5_tostring(&m, output, sizeof(output));
    printf("%s\n", output);

    return 0;
}

