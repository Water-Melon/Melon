## Bignum Computing



### Header file

```c
#include "mln_bignum.h"
```



### Module

`bignum`



### Structures

```c
typedef struct {
    mln_u32_t tag;//positive and negative flag
    mln_u32_t length;//The number of elements of data used by the current large number value
    mln_u64_t data[M_BIGNUM_SIZE];//value of big number
} mln_bignum_t;

#define M_BIGNUM_POSITIVE 0 //positive
#define M_BIGNUM_NEGATIVE 1 //negative

#define M_BIGNUM_SIZE     257
```

The implementation of large numbers in Melon is fixed-length, that is, large numbers have an upper limit, and currently supports a maximum of 2048 bits.



### Functions/Macros



#### mln_bignum_new

```c
mln_bignum_t *mln_bignum_new(void);
```

Description: Create and initialize the big number structure `mln_bignum_t` allocated by `malloc`.

Return value: If successful, return a pointer to a big number structure, otherwise return `NULL`



#### mln_bignum_pool_new

```c
mln_bignum_t *mln_bignum_pool_new(mln_alloc_t *pool);
```

Description: Create and initialize the big number structure `mln_bignum_t`, which is allocated from the memory pool specified by `pool`.

Return value: If successful, return a pointer to a big number structure, otherwise return `NULL`



#### mln_bignum_free

```c
void mln_bignum_free(mln_bignum_t *bn);
```

Description: Free the big number structure `bn`, which should have been allocated by `mln_bignum_new`.

Return value: none



#### mln_bignum_pool_free

```c
void mln_bignum_pool_free(mln_bignum_t *bn);
```

Description: Free the big number structure `bn`, which should have been allocated by `mln_bignum_pool_new`.

Return value: none



#### mln_bignum_dup

```c
mln_bignum_t *mln_bignum_dup(mln_bignum_t *bn);
```

Description: Completely copy a large number structure `bn`, the copy is allocated by `malloc`.

Return value: If successful, return a pointer to a big number structure, otherwise return `NULL`



#### mln_bignum_pool_dup

```c
mln_bignum_t *mln_bignum_pool_dup(mln_alloc_t *pool, mln_bignum_t *bn);
```

Description: Make a complete copy of the large number structure `bn`, and the copy allocates memory on the memory pool specified by `pool`.

Return value: If successful, return a pointer to a big number structure, otherwise return `NULL`



#### mln_bignum_assign

```c
int mln_bignum_assign(mln_bignum_t *bn, mln_s8ptr_t sval, mln_u32_t len);
```

Description: Assign a big number in string form represented by `sval` and `len` to the big number structure `bn`.

Return value: return `0` if successful, otherwise return `-1`



#### mln_bignum_add

```c
void mln_bignum_add(mln_bignum_t *dest, mln_bignum_t *src);
```

Description: Add big numbers, the result will be put into `dest`.

Return value: none



#### mln_bignum_sub

```c
void mln_bignum_sub(mln_bignum_t *dest, mln_bignum_t *src);
```

Description: Large number subtraction, minuend `dest`, subtrahend `src`, the calculation result will be put into `dest`.

Return value: none



#### mln_bignum_mul

```c
void mln_bignum_mul(mln_bignum_t *dest, mln_bignum_t *src);
```

Description: Multiplication of big numbers, the result of the calculation will be put into `dest`.

Return value: none



#### mln_bignum_div

```c
int mln_bignum_div(mln_bignum_t *dest, mln_bignum_t *src, mln_bignum_t *quotient);
```

Description: Large number division, the dividend `dest`, the divisor `src`, the remainder will be put in `dest`, if `quotient` is not empty, the quotient will be put in it.

Return value: return `0` if successful, otherwise return `-1`



#### mln_bignum_pwr

```c
int mln_bignum_pwr(mln_bignum_t *dest, mln_bignum_t *exponent, mln_bignum_t *mod);
```

Description: Big number exponentiation, computes the `exponent` power of `dest`. If `mod` is not empty, the result is modulo `mod`. The final calculation result is put into `dest`.

Return value: return `0` if successful, otherwise return `-1`



#### mln_bignum_compare

```c
int mln_bignum_compare(mln_bignum_t *bn1, mln_bignum_t *bn2);
```

Description: The comparison of signed numeric values.

return value:

-  `1` - `bn1` > `bn2`
- `-1` -` bn1` < `bn2`
-  `0` - `bn1` = `bn2`



#### mln_bignum_abs_compare

```c
int mln_bignum_abs_compare(mln_bignum_t *bn1, mln_bignum_t *bn2);
```

Description: Absolute value comparison.

return value:

-  `1` - `bn1` > `bn2`
- `-1` -` bn1` < `bn2`
-  `0` - `bn1` = `bn2`



#### mln_bignum_bit_test

```c
int mln_bignum_bit_test(mln_bignum_t *bn, mln_u32_t index);
```

Description: Check if the bit is 1 in the big number `bn` specified by `index`.

Return value: return `1` if it is 1, otherwise return `0`



#### mln_bignum_left_shift

```c
void mln_bignum_left_shift(mln_bignum_t *bn, mln_u32_t n);
```

Description: Shift a large number `bn` left by `n` bits.

Return value: none



#### mln_bignum_right_shift

```c
void mln_bignum_right_shift(mln_bignum_t *bn, mln_u32_t n);
```

Description: Shift a large number `bn` right by `n` bits.

Return value: none



#### mln_bignum_prime

```c
int mln_bignum_prime(mln_bignum_t *res, mln_u32_t bitwidth);
```

Description: Computes a big prime number of `bitwidth` bits and writes the result to `res`.

Return value: return `0` if successful, otherwise return `-1`



#### mln_bignum_extend_eulid

```c
int mln_bignum_extend_eulid(mln_bignum_t *a, mln_bignum_t *b, mln_bignum_t *x, mln_bignum_t *y);
```

Description: A version of the Extended Euclidean Algorithm for big numbers.

Return value: return `0` if successful, otherwise return `-1`



#### mln_bignum_i2osp

```c
int mln_bignum_i2osp(mln_bignum_t *n, mln_u8ptr_t buf, mln_size_t len);
```

Description: Write the binary value of `n` to the memory specified by `buf` and `len`.

Return value: return `0` if successful, otherwise return `-1`



#### mln_bignum_os2ip

```c
int mln_bignum_os2ip(mln_bignum_t *n, mln_u8ptr_t buf, mln_size_t len);
```

Description: Assigns `n` the binary value in memory pointed to by `buf` and `len`. Opposite to `mln_bignum_i2osp`.

Return value: return `0` if successful, otherwise return `-1`



#### mln_bignum_tostring

```c
mln_string_t *mln_bignum_tostring(mln_bignum_t *n);
```

Description: Convert a big number n to a decimal string format. Do not forget to free the returned string by `mln_string_free`.

Return value: return the string if successful, otherwise return `NULL`



#### mln_bignum_positive

```c
mln_bignum_positive(pbn)
```

Description: Set the big number `pbn` of `mln_bignum_t*` to a positive number.

Return value: none



#### mln_bignum_negative

```c
mln_bignum_negative(pbn)
```

Description: Set the big number `pbn` of `mln_bignum_t*` to a negative number.

Return value: none



#### mln_bignum_is_positive

```c
mln_bignum_is_positive(pbn)
```

Description: Determine whether the big number `pbn` of `mln_bignum_t *` is positive.

Return value: return `non-0` for regular, otherwise return `0`



#### mln_bignum_is_negative

```c
mln_bignum_is_negative(pbn)
```

Description: Determine whether the big number `pbn` of `mln_bignum_t *` is negative.

Return value: if negative, return `not 0`, otherwise return `0`



#### mln_bignum_get_length

```c
mln_bignum_get_length(pbn)
```

Description: Get the number of array elements in `data` that the value of the big number `pbn` of `mln_bignum_t *` occupies.

Return value: the number of elements



#### mln_bignum_zero

```s
mln_bignum_zero()
```

Description: Returns a big number with a value of `0`.

Return value: constant with a big value of `0`



### Example

```c
#include "mln_bignum.h"

int main(int argc, char *argv[])
{
    mln_string_t *s;
    mln_bignum_t n1, n2 = mln_bignum_zero();

    mln_bignum_init(n1); //same as mln_bignum_zero

    mln_bignum_assign(&n1, "10", 2);
    mln_bignum_assign(&n2, "30", 2);

    mln_bignum_pwr(&n1, &n2, NULL);
    s = mln_bignum_tostring(&n1);
    write(STDOUT_FILENO, s->data, s->len);
    write(STDOUT_FILENO, "\n", 1);
    mln_string_free(s);

    return 0;
}
```

