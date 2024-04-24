#include <stdio.h>
#include "mln_des.h"

int main(int argc, char *argv[])
{
    mln_3des_t d;
    mln_u8_t text[9] = {0};
    mln_u8_t cipher[9] = {0};

    mln_3des_init(&d, 0xffff, 0xff120000);
    mln_3des_buf(&d, (mln_u8ptr_t)"Hi Tom!!", 11, cipher, sizeof(cipher), 0, 1);
    mln_3des_buf(&d, cipher, sizeof(cipher)-1, text, sizeof(text), 0, 0);
    printf("%s\n", text);

    return 0;
}
