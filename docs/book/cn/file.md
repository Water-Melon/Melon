## 文件集合



### 头文件

```c
#include "mln_file.h"
```



### 模块名

`file`



### 相关结构

```c
typedef struct mln_file_s {//文件结构
    mln_string_t      *file_path;//文件路径                     
    int                fd;//文件描述符
    mln_u32_t          is_tmp:1;//是否是临时文件
    time_t             mtime;//文件内容的修改时间
    time_t             ctime;//文件内容及文件名、文件位置、权限的修改时间
    time_t             atime;//文件的访问时间
    size_t             size;//文件大小
    size_t             refer_cnt;//本文件被引用次数，即在别的地方使用文件集合相关函数打开了同一路径的文件
    mln_fileset_t     *fset;//所属文件集合结构指针
    ...                                                                                                            
} mln_file_t;
```



### 函数/宏



#### mln_fileset_init

```c
mln_fileset_t *mln_fileset_init(mln_size_t max_file); 
```

描述：创建文件集合结构。参数`max_file`表示文件集合内可承载的文件结构最大值。当打开一个新文件导致总数大于该值时，会将集合中空闲的文件结构释放，以维持总数不超过该值。若无闲置文件结构，则不做任何处理，也不会影响本次及后续打开新文件的操作。

返回值：成功则返回文件集合指针，否则返回`NULL`



#### mln_fileset_destroy

```c
void mln_fileset_destroy(mln_fileset_t *fs);
```

描述：销毁文件集合，并关闭集合内所有文件描述符。

返回值：无



#### mln_file_open

```c
mln_file_t *mln_file_open(mln_fileset_t *fs, const char *filepath);
```

描述：在文件集合`fs`上打开`filepath`指定的文件。

返回值：成功则返回文件结构指针，否则返回`NULL`



#### mln_file_close

```c
void mln_file_close(mln_file_t *pfile);
```

描述：关闭文件`pfile`。

返回值：无



#### mln_file_tmp_open

```c
mln_file_t *mln_file_tmp_open(mln_alloc_t *pool);
```

描述：打开一个临时文件。临时文件仅在程序运行期间存在，但不会出现在文件系统中。当程序退出后，相应文件亦不会存在于文件系统中。

返回值：成功则返回文件结构指针，否则返回`NULL`



#### mln_file_fd

```c
mln_file_fd(pfile)
```

描述：获取`mln_file_t`类型的`pfile`文件结构中的文件描述符。

返回值：文件描述符值



### 示例

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

