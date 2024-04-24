#include "mln_error.h"

#define OK    0 //0是个特殊值，表示一切正常，由0生成的返回值就是0，不会增加额外信息
#define NMEM  1 //由使用者自行定义，但顺序必须与errs数组给出的错误信息顺序一致

int main(void)
{
    char msg[1024];
    mln_string_t files[] = {
        mln_string("a.c"),
    };
    mln_string_t errs[] = {
        mln_string("Success"),
        mln_string("No memory"),
    };
    mln_error_init(files, errs, sizeof(files)/sizeof(mln_string_t), sizeof(errs)/sizeof(mln_string_t));
    printf("%x %d [%s]\n", RET(OK), CODE(RET(OK)), mln_error_string(RET(OK), msg, sizeof(msg)));
    printf("%x %d [%s]\n", RET(NMEM), CODE(RET(NMEM)), mln_error_string(RET(NMEM), msg, sizeof(msg)));
    printf("%x %d [%s]\n", RET(2), CODE(RET(2)), mln_error_string(RET(2), msg, sizeof(msg)));
    printf("%x %d [%s]\n", RET(0xff), CODE(RET(0xff)), mln_error_string(RET(0xff), msg, sizeof(msg)));
    return 0;
}

