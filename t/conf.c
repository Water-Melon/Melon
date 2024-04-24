#include <stdio.h>
#include "mln_conf.h"

int main(int argc, char *argv[])
{
    mln_conf_t *cf;
    mln_conf_domain_t *cd;
    mln_conf_cmd_t *cc;
    mln_conf_item_t *ci;

    if (mln_conf_load() < 0) {
        fprintf(stderr, "Load configuration failed.\n");
        return -1;
    }

    cf = mln_conf();
    cd = cf->search(cf, "main");
    cc = cd->search(cd, "framework");
    if (cc == NULL) {
        fprintf(stderr, "framework not found.\n");
        return -1;
    }
    ci = cc->search(cc, 1);
    if (ci->type == CONF_BOOL && !ci->val.b) {
        printf("framework off\n");
    } else if (ci->type == CONF_STR) {
        printf("framework %s\n", ci->val.s->data);
    } else {
        fprintf(stderr, "Invalid framework value.\n");
    }
    mln_conf_free();
    return 0;
}

