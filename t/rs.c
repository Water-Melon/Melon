#include <stdio.h>
#include <assert.h>
#include "mln_rs.h"

#define COL  10
#define ROW  10
#define K    2

int main(int argc, char *argv[])
{
    int i, j;
    mln_rs_result_t *res, *dres;
    uint8_t origin[COL * ROW] = "0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789";
    uint8_t *err[ROW + K] = {0};

    assert((res = mln_rs_encode(origin, COL, ROW, K)) != NULL);

    printf("res->num=%ld, res->len=%ld\n", res->num, res->len);

    for (i = 0; i < mln_rs_result_get_num(res); ++i) {
        err[i] = mln_rs_result_get_data_by_index(res, i);
    }
    err[0] = NULL;
    err[1] = NULL;

    assert((dres = mln_rs_decode(err,COL, ROW, K)) != NULL);

    for (i = 0; i < mln_rs_result_get_num(dres); ++i) {
        for (j = 0; j < COL; ++j) {
            printf("%c", mln_rs_result_get_data_by_index(dres, i)[j]);
        }
        printf("\n");
    }

    mln_rs_result_free(res);
    mln_rs_result_free(dres);
    return 0;
}
