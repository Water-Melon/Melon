#include <stdio.h>
#include "mln_file.h"

int main(int argc, char *argv[])
{
    mln_fileset_t *fset;
    mln_file_t *file;

    fset = mln_fileset_init(100);
    if (fset == NULL) {
        fprintf(stderr, "fset init failed\n");
        return -1;
    }

    file = mln_file_open(fset, "a.c");//打开本文件
    if (file == NULL) {
        fprintf(stderr, "file open failed\n");
        return -1;
    }
    printf("filename: %s size:%lu\n", file->file_path->data, (unsigned long)(file->size));

    mln_fileset_destroy(fset);

    return 0;
}

