## Hash table



### Header file

```c
#include "mln_hash.h"
```



### Module

`hash`



### Structure

```c
struct mln_hash_s {
    void                    *pool;//memory pool, it's an option, NULL means memory pool not activated
    hash_pool_alloc_handler  pool_alloc;//allocation function of memory pool
    hash_pool_free_handler   pool_free;//free function of memory pool
    hash_calc_handler        hash;//callback to calculate the bucket index
    hash_cmp_handler         cmp;//comparision function for comparing nodes in the same bucket
    hash_free_handler        key_freer;//key free function
    hash_free_handler        val_freer;//value free function
    mln_hash_mgr_t          *tbl;//buckets
    mln_u64_t                len;//bucket length
    mln_u32_t                nr_nodes;//number of nodes
    mln_u32_t                threshold;//bucket expansion Threshold
    mln_u32_t                expandable:1;//expansion flag
    mln_u32_t                calc_prime:1;//prime flag for calculating bucket length as a prime number
};
```

Here, we mainly focus on `len`, because when calculating the belonging bucket, the value is usually modulo.



### Functions



#### mln_hash_init

```c
int mln_hash_init(mln_hash_t *h, struct mln_hash_attr *attr);

struct mln_hash_attr {
    void                    *pool;
    hash_pool_alloc_handler  pool_alloc;
    hash_pool_free_handler   pool_free;
    hash_calc_handler        hash;
    hash_cmp_handler         cmp;
    hash_free_handler        key_freer;
    hash_free_handler        val_freer;
    mln_u64_t                len_base;
    mln_u32_t                expandable:1;
    mln_u32_t                calc_prime:1;
};

typedef mln_u64_t (*hash_calc_handler)(mln_hash_t *, void *);
typedef int  (*hash_cmp_handler) (mln_hash_t *, void *, void *);
typedef void (*hash_free_handler)(void *);
typedef void *(*hash_pool_alloc_handler)(void *, mln_size_t);
typedef void (*hash_pool_free_handler)(void *);
```

Description:

This function is used to initialize the hash table structure `h`.

The `pool` of parameter `attr` is optional. If you want to allocate from the memory pool, assign a value, otherwise set `NULL` and allocate it by `malloc`. That is, the pool structure is given by the caller.

`pool_alloc` and `pool_free` are function pointers for memory allocation and release of the pool `pool`.

`key_freer` and `val_freer` are also optional parameters. If you need to release resources at the same time when the table entry is removed or the hash table is destroyed, set it, otherwise set it to `NULL`. Release the structure whose function parameter is `key` or `value`.

The return value of `hash` is a 64-bit integer offset, and the offset should not be greater than or equal to the bucket length, so the modulo operation should be performed within this function.

`cmp` return value: `0` means different, `non-0` means the same.

The hash table supports automatic expansion of the bucket length based on the number of elements, but it is recommended to treat this option with caution, because the bucket length expansion will be accompanied by node migration, which will cause corresponding calculation and time overhead, so use it with caution.

It is recommended that the hash table bucket length be a prime number, because taking the modulo of a prime number will relatively evenly place elements into different buckets, preventing some bucket linked lists from being too long.

Return value: Returns `0` if successful, otherwise returns `-1`



#### mln_hash_init_fast

```c
int mln_hash_init_fast(mln_hash_t *h, hash_calc_handler hash, hash_cmp_handler cmp, hash_free_handler key_freer,
                       hash_free_handler val_freer, mln_u64_t base_len, mln_u32_t expandable, mln_u32_t calc_prime);
```

Description: This function is used to initialize the hash table structure `h`. The meaning of the parameters is the same as that of `mln_hash_init`, but it is passed directly as the function parameters.

Return value: Returns `0` if successful, otherwise returns `-1`



#### mln_hash_new

```c
mln_hash_t *mln_hash_new(struct mln_hash_attr *attr);

struct mln_hash_attr {
    void                    *pool;
    hash_pool_alloc_handler  pool_alloc;
    hash_pool_free_handler   pool_free;
    hash_calc_handler        hash;
    hash_cmp_handler         cmp;
    hash_free_handler        key_freer;
    hash_free_handler        val_freer;
    mln_u64_t                len_base;
    mln_u32_t                expandable:1;
    mln_u32_t                calc_prime:1;
};

typedef mln_u64_t (*hash_calc_handler)(mln_hash_t *, void *);
typedef int  (*hash_cmp_handler) (mln_hash_t *, void *, void *);
typedef void (*hash_free_handler)(void *);
typedef void *(*hash_pool_alloc_handler)(void *, mln_size_t);
typedef void (*hash_pool_free_handler)(void *);
```

Description:

This function is used to initialize the hash table structure.

The `pool` of parameter `attr` is optional. If you want to allocate from the memory pool, assign a value, otherwise set `NULL` and allocate it by `malloc`. That is, the pool structure is given by the caller.

`pool_alloc` and `pool_free` are function pointers for memory allocation and release of the pool `pool`.

`key_freer` and `val_freer` are also optional parameters. If you need to release resources at the same time when the table entry is removed or the hash table is destroyed, set it, otherwise set it to `NULL`. Release the structure whose function parameter is `key` or `value`.

The return value of `hash` is a 64-bit integer offset, and the offset should not be greater than or equal to the bucket length, so the modulo operation should be performed within this function.

`cmp` return value: `0` means different, `non-0` means the same.

The hash table supports automatic expansion of the bucket length based on the number of elements, but it is recommended to treat this option with caution, because the bucket length expansion will be accompanied by node migration, which will cause corresponding calculation and time overhead, so use it with caution.

It is recommended that the hash table bucket length be a prime number, because taking the modulo of a prime number will relatively evenly place elements into different buckets, preventing some bucket linked lists from being too long.

Return value: If successful, returns the hash table structure pointer, otherwise `NULL`



#### mln_hash_new_fast

```c
mln_hash_t *mln_hash_new_fast(hash_calc_handler hash, hash_cmp_handler cmp, hash_free_handler key_freer, hash_free_handler val_freer,
                              mln_u64_t base_len, mln_u32_t expandable, mln_u32_t calc_prime);
```

Description: This function is used to initialize the hash table structure. The meaning of the parameters is the same as that of `mln_hash_new`, except that it is passed as function parameters.

Return value: If successful, returns the hash table structure pointer, otherwise `NULL`



#### mln_hash_destroy

```c
void mln_hash_destroy(mln_hash_t *h, mln_hash_flag_t flg);

typedef enum mln_hash_flag {
    M_HASH_F_NONE,
    M_HASH_F_VAL,
    M_HASH_F_KEY,
    M_HASH_F_KV
} mln_hash_flag_t;
```

Description:

Destroy the hash table.

The parameter `flg` is used to indicate whether to release the resources occupied by the `key`value` in the hash table when it is released. It is divided into:

- No need to release
- Only release value resources
- Only release key resources
- Release key and value resources at the same time

Return value: None



#### mln_hash_free

```c
void mln_hash_free(mln_hash_t *h, mln_hash_flag_t flg);

typedef enum mln_hash_flag {
    M_HASH_F_NONE,
    M_HASH_F_VAL,
    M_HASH_F_KEY,
    M_HASH_F_KV
} mln_hash_flag_t;
```

Description:

Destroy and free the hash table.

The parameter `flg` is used to indicate whether to release the resources occupied by the `key`value` in the hash table when it is released. It is divided into:

- No need to release
- Only release value resources
- Only release key resources
- Release key and value resources at the same time

Return value: None



#### mln_hash_search

```c
void *mln_hash_search(mln_hash_t *h, void *key);
```

Description: Find the corresponding value in the hash table `h` according to `key`.

Return value: If successful, return `value` corresponding to `key`, otherwise return `NULL`



#### mln_hash_search_iterator

```c
void *mln_hash_search_iterator(mln_hash_t *h, void *key, int **ctx);
```

Description:

Since the hash table allows multiple items with the same `key` to exist in the table, this query iterator is provided, and only one table item is queried at a time.

`ctx` is a secondary pointer that takes the address of a pointer type variable, which is used to store the location of this search.

Note that this function uses some restrictions, and elements cannot be deleted during query, otherwise it may cause illegal memory access.

Return value: the `value` structure associated with `key`



#### mln_hash_update

```c
int mln_hash_update(mln_hash_t *h, void *key, void *val);
```

Description: Replace `key` with `value` for entries with the same `key` in the hash table `h`.

It should be noted that `key` and `val` are secondary pointers, so you need to take the address of the key and value structures of the pointer type again when calling.

return value:

Returns `0` on success, `-1` otherwise. Moreover, the original key and value will be assigned to `key` and `val`, so it must be noted that these two parameters are secondary pointers, but the declaration is given to `void *`.



#### mln_hash_insert

```c
int mln_hash_insert(mln_hash_t *h, void *key, void *val);
```

Description: Insert `key` and `val` into a hash table.

Return value: return `0` on success, otherwise return `-1`



#### mln_hash_remove

```c
void mln_hash_remove(mln_hash_t *h, void *key, mln_hash_flag_t flg);

typedef enum mln_hash_flag {
    M_HASH_F_NONE,
    M_HASH_F_VAL,
    M_HASH_F_KEY,
    M_HASH_F_KV
} mln_hash_flag_t;
```

Description: Delete the item corresponding to `key` in the hash table. `flg` is used to indicate whether to release the resources occupied by `key`value` in the hash table when it is released. It is divided into:

- no release required
- only release value resources
- only release key resources
- Release key and value resources at the same time

Return value: none



#### mln_hash_iterate

```c
int mln_hash_iterate(mln_hash_t *h, hash_iterate_handler handler, void *udata);

typedef int (*hash_iterate_handler)(mln_hash_t * /*h*/, void * /*key*/, void * /*val*/, void *);
```

Description: Traverse all entries in the hash table.

`handler` is a table entry access function, its first parameter is the hash structure `h`, the second one is `key`, the third parameter is `val`, and the last parameter is the third parameter `udata` of `mln_hash_iterate`.

return value:

- `mln_hash_iterate`: returns `0` on success, `-1` on failure
- `handler` processing function: return `0` normally, if you want to interrupt the traversal, return `-1`



#### mln_hash_change_value

```c
void *mln_hash_change_value(mln_hash_t *h, void *key, void *new_value);
```

Description: Replace the original value corresponding to `key` with `new_value`, and return the original value.

Return value: If `key` does not exist, return `NULL`, otherwise return the corresponding value.



#### mln_hash_key_exist

```c
int mln_hash_key_exist(mln_hash_t *h, void *key);
```

Description: Check if there is an entry with key `key` in hash table `h`.

Return value: returns `1` if exists, otherwise returns `0`



#### mln_hash_reset

```c
void mln_hash_reset(mln_hash_t *h, mln_hash_flag_t flg);

typedef enum mln_hash_flag {
    M_HASH_F_NONE,
    M_HASH_F_VAL,
    M_HASH_F_KEY,
    M_HASH_F_KV
} mln_hash_flag_t;
```

Description: Reset all entries in the hash table and release. The release will be processed according to `flg`.

Return value: none



### Example

```c
#include <stdio.h>
#include <stdlib.h>
#include "mln_hash.h"

typedef struct mln_hash_test_s {
    int    key;
    int    val;
} mln_hash_test_t;

static mln_u64_t calc_handler(mln_hash_t *h, void *key)
{
    return *((int *)key) % h->len;
}

static int cmp_handler(mln_hash_t *h, void *key1, void *key2)
{
    return !(*((int *)key1) - *((int *)key2));
}

static void free_handler(void *val)
{
    free(val);
}

int main(int argc, char *argv[])
{
    mln_hash_t *h;
    struct mln_hash_attr hattr;
    mln_hash_test_t *item, *ret;

    hattr.pool = NULL;
    hattr.pool_alloc = NULL;
    hattr.pool_free = NULL;
    hattr.hash = calc_handler;
    hattr.cmp = cmp_handler;
    hattr.key_freer = NULL;
    hattr.val_freer = free_handler;
    hattr.len_base = 97;
    hattr.expandable = 0;
    hattr.calc_prime = 0;

    if ((h = mln_hash_new(&hattr)) == NULL) {
        fprintf(stderr, "Hash init failed.\n");
        return -1;
    }

    item = (mln_hash_test_t *)malloc(sizeof(mln_hash_test_t));
    if (item == NULL) {
        fprintf(stderr, "malloc failed.\n");
        return -1;
    }
    item->key = 1;
    item->val = 10;
    if (mln_hash_insert(h, &(item->key), item) < 0) {
        fprintf(stderr, "insert failed.\n");
        return -1;
    }

    ret = mln_hash_search(h, &(item->key));
    printf("%p %p\n", ret, item);

    mln_hash_free(h, M_HASH_F_VAL);

    return 0;
}
```
