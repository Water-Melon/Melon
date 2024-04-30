#include <stdio.h>
#include <stdlib.h>
#include "mln_rc.h"

int main(int argc, char *argv[])
{
    mln_u8_t s[256] = {0};
    mln_u8_t text[] = "Hello";

    mln_rc4_init(s, (mln_u8ptr_t)"this is a key", sizeof("this is a key")-1);
    mln_rc4_calc(s, text, sizeof(text)-1);
    printf("%s\n", text);
    mln_rc4_calc(s, text, sizeof(text)-1);
    printf("%s\n", text);

    return 0;
}

