#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "mln_file.h"
#include <assert.h>

int main(int argc, char *argv[])
{
    mln_fileset_t *fset;
    mln_file_t *file;

    fset = mln_fileset_init(100);
    assert(fset != NULL);

    int fd = open("/tmp/aaa", O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
    assert(fd >= 0);
    int n = write(fd, "Hello", 5);
    assert(n == 5);
    close(fd);

    file = mln_file_open(fset, "/tmp/aaa");
    assert(file != NULL);

    printf("filename: %s size:%lu\n", file->file_path->data, (unsigned long)(file->size));

    mln_fileset_destroy(fset);

    return 0;
}

