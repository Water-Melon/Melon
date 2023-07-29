## Reed Solomon Error Correcting Code



### Header file

```c
#include "mln_rs.h"
```



### Functions/Macros



#### mln_rs_encode

```c
mln_rs_result_t *mln_rs_encode(uint8_t *data_vector, size_t len, size_t n, size_t k);
```

Description: Generate error correction codes. Parameter meaning:

- `data_vector` stores several packets. Since the packets must be of the same length, a one-dimensional pointer is directly passed in here to improve processing performance.
- `len` The length of each piece of data.
- How many pieces of data in `n`.
- How many segments of `k` error correction codes need to be generated.

The generated result will contain the original data in the error correction code, and the error correction code will be stored after the data.

Return value: return `mln_rs_result_t` structure pointer if successful, otherwise return `NULL`



#### mln_rs_decode

```c
mln_rs_result_t *mln_rs_decode(uint8_t **data_vector, size_t len, size_t n, size_t k);
```

Description: Use error correction codes for error correction. Parameter meaning:

- `data_vector` An array of messages including error correction codes. Note: This is a two-dimensional array. **Note 1**: The order of the packets in the data area must be the same as when they were generated, and `NULL` is given to the missing packets, but the order cannot be disordered. **Note 2**: When the number of missing data is more than `k`, it cannot be repaired.
- `len` The length of each piece of data.
- `n` The total number of pieces of data in the original.
- `k` How many segments of the original error correction code were generated.

Return value: return `mln_rs_result_t` structure pointer if successful, otherwise return `NULL`



#### mln_rs_result_free

```c
void mln_rs_result_free(mln_rs_result_t *result);
```

Description: Frees the result structure produced by the above function.

Return value: none



#### mln_rs_result_get_num

```c
mln_rs_result_get_num(_presult)
```

Description: Get the number of processing results in `_presult` of class behavior `mln_rs_result_t`.

Return value: the number of processing results



#### mln_rs_result_get_data_by_index

```c
mln_rs_result_get_data_by_index(_presult,index)
```

Description: Get the data subscripted `index` in `_presult` of class behavior `mln_rs_result_t`, `index` starts from 0.

Return value: result data memory address



### Example

```c
#include <stdio.h>
#include <stdlib.h>
#include "mln_core.h"
#include "mln_log.h"
#include "mln_string.h"
#include "mln_rs.h"

int main(int argc, char *argv[])
{
    mln_rs_result_t *res, *dres;
    char origin[] = "AAABBBCCCDDD";
    uint8_t *err[6] = {0};
    mln_string_t tmp;
    struct mln_core_attr cattr;

    cattr.argc = argc;
    cattr.argv = argv;
    cattr.global_init = NULL;
    cattr.main_thread = NULL;
    cattr.master_process = NULL;
    cattr.worker_process = NULL;
    if (mln_core_init(&cattr) < 0) {
        fprintf(stderr, "init failed\n");
        return -1;
    }

    res = mln_rs_encode((uint8_t *)origin, 3, 4, 2);
    if (res == NULL) {
        mln_log(error, "rs encode failed.\n");
        return -1;
    }

    err[0] = NULL;
    err[1] = NULL;
    err[2] = (uint8_t *)origin+6;
    err[3] = (uint8_t *)origin+9;
    err[4] = mln_rs_result_get_data_by_index(res, 4);
    err[5] = mln_rs_result_get_data_by_index(res, 5);

    dres = mln_rs_decode(err, 3, 4, 2);
    if (dres == NULL) {
        mln_log(error, "rs decode failed.\n");
        return -1;
    }

    mln_string_nset(&tmp, mln_rs_result_get_data_by_index(dres, 1), 3);
    mln_log(debug, "%S\n", &tmp);

    mln_rs_result_free(res);
    mln_rs_result_free(dres);
    return 0;
}
```

