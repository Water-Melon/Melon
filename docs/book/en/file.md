## File Set



### Header file

```c
#include "mln_file.h"
```



### Module

`file`



### Structures

```c
typedef struct mln_file_s {
    mln_string_t      *file_path;//file path                     
    int                fd;//file descriptor
    mln_u32_t          is_tmp:1;//is temp file
    time_t             mtime;//modification time
    time_t             ctime;//change time
    time_t             atime;//access time
    size_t             size;//file size
    size_t             refer_cnt;//reference count of this file
    mln_fileset_t     *fset;//file set that this file belongs to
    ...                                                                                                            
} mln_file_t;
struct mln_fileset_s {
    mln_alloc_t       *pool; //file set 
    mln_rbtree_t      *reg_file_tree;
    mln_file_t        *reg_free_head;
    mln_file_t        *reg_free_tail;
    mln_size_t         max_file;
};
```



### Functions/Macros



#### mln_fileset_init

```c
mln_fileset_t *mln_fileset_init(mln_size_t max_file); 
```

Description: Create a file set structure. `max_file`indicates how many files can be contained in this file set. When we open a new file and the file number is greater than this capacity, file set will release unused file cached by file set. If there is no unused file in the file set, opening a new file won't be prevented.

Return value: A file set pointer returned on sucess, otherwise `NULL` returned



#### mln_fileset_destroy

```c
void mln_fileset_destroy(mln_fileset_t *fs);
```

Description: Destroy a file set and close all files in it.

Return value: none



#### mln_file_open

```c
mln_file_t *mln_file_open(mln_fileset_t *fs, const char *filepath);
```

Description: Open the file specified by `filepath` on the file set `fs`.

Return value: file pointer returned on success, otherwise `NULL` returned



#### mln_file_close

```c
void mln_file_close(mln_file_t *pfile);
```

Description: close file `pfile`。

Return value: none



#### mln_file_tmp_open

```c
mln_file_t *mln_file_tmp_open(mln_alloc_t *pool);
```

Description: Open a temporary file. This file can be accessed during the runtime and it never exist in file system. 

Return value: file pointer returned on success, otherwise `NULL` returned



#### mln_file_fd

```c
mln_file_fd(pfile)
```

Description: Get the file descriptor of file `pfile`.

Return value: the file descriptor



### Example

```c
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
```

