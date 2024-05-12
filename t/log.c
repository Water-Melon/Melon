#include "mln_log.h"
#include <assert.h>

int main(int argc, char *argv[])
{
    mln_log(debug, "This will be outputted to stderr\n");
    mln_conf_load();
    assert(mln_log_init(NULL) == 0);
    mln_log(debug, "This will be outputted to stderr and log file\n");
    mln_log_destroy();
    mln_conf_free();
    return 0;
}

