#include <stdio.h>
#include <stdlib.h>
#include "mln_string.h"
#include "mln_aes.h"

int main(int argc, char *argv[])
{
    mln_aes_t a;
    char p[] = "1234567890123456";
    mln_string_t s;

    if (mln_aes_init(&a, (mln_u8ptr_t)"abcdefghijklmnop", M_AES_128) < 0) {
        fprintf(stderr, "aes init failed\n");
        return -1;
    }

    mln_string_set(&s, p);
    if (mln_aes_encrypt(&a, s.data) < 0) {
        fprintf(stderr, "aes encrypt failed\n");
        return -1;
    }
    write(STDOUT_FILENO, s.data, s.len);
    write(STDOUT_FILENO, "\n", 1);

    if (mln_aes_decrypt(&a, s.data) < 0) {
        fprintf(stderr, "aes decrypt failed\n");
        return -1;
    }
    write(STDOUT_FILENO, s.data, s.len);
    write(STDOUT_FILENO, "\n", 1);

    return 0;
}
