#include <string.h>
#include <assert.h>
#include "mln_path.h"

char conf_path[] = "/tmp/a.conf";

static char *return_conf_path(void)
{
    return conf_path;
}

int main(void)
{
    mln_path_hook_set(m_p_conf, return_conf_path);
    assert(!strcmp(mln_path_conf(), conf_path));
    return 0;
}
