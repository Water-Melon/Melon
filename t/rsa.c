#include <stdio.h>
#include <stdlib.h>
#include "mln_rsa.h"

int main(int argc, char *argv[])
{
    char s[] = "Hello";
    mln_string_t tmp, *cipher = NULL, *text = NULL;
    mln_rsa_key_t *pub = NULL, *pri = NULL;

    pub = mln_rsa_key_new();
    pri = mln_rsa_key_new();
    if (pri == NULL || pub == NULL) {
        fprintf(stderr, "new pub/pri key failed\n");
        goto failed;
    }

    if (mln_rsa_key_generate(pub, pri, 128) < 0) {
        fprintf(stderr, "key generate failed\n");
        goto failed;
    }

    mln_string_set(&tmp, s);
    cipher = mln_RSAESPKCS1V15_public_encrypt(pub, &tmp);
    if (cipher == NULL) {
        fprintf(stderr, "pub key encrypt failed\n");
        goto failed;
    }
    write(STDOUT_FILENO, cipher->data, cipher->len);
    write(STDOUT_FILENO, "\n", 1);

    text = mln_RSAESPKCS1V15_private_decrypt(pri, cipher);
    if (text == NULL) {
        fprintf(stderr, "pri key decrypt failed\n");
        goto failed;
    }
    write(STDOUT_FILENO, text->data, text->len);
    write(STDOUT_FILENO, "\n", 1);

failed:
    if (pub != NULL) mln_rsa_key_free(pub);
    if (pri != NULL) mln_rsa_key_free(pri);
    if (cipher != NULL) mln_RSAESPKCS1V15_free(cipher);
    if (text != NULL) mln_RSAESPKCS1V15_free(text);

    return 0;
}

