## Span



The span component in Melon is used to measure C language function consumption. This module needs to be used together with the `function` module, so it is also necessary to enable the function template through defining macro `MLN_FUNC_FLAG`, thereby enabling function consumption tracing.

Currently supported consumption are as follows:

- time consumption



### Header file

```c
#include "mln_span.h"
```



### Module

`span`



### Functions/Macros



#### mln_span_start

```c
mln_span_start();
```

Description: Start the measurement. This macro function sets the global variables used by the module and only tracks the functions called by the thread that calls this function during the measurement.

**Note**: This macro function needs to be called in the same scope as the `mln_span_stop` macro function, or the call stack depth of the `mln_span_stop` should be deeper than the call stack depth of the `mln_span_start`. For example:

> If there are three functions: `main`, `foo`, and `bar`, and their calling relationships are as follows:
>
> ```c
> void bar(void)
> {
> }
> void foo(void)
> {
>   bar();
> }
> int main(void)
> {
>   foo();
>   return 0;
> }
> ```
>
> If `mln_span_start` is called before the `bar` call in `foo`, then `mln_span_stop` should be called after `mln_span_start` in `foo` or within the `bar` function, but not in `main`.

Not following the above rules may lead to a situation where memory leaks could occur.

Return value: none



#### mln_span_stop

```c
mln_span_stop();
```

Description: Stop the measurement. This macro function destroys some global variables within the module but retains the structure containing the measured values for this measurement. The timing of calling this macro function follows the description in `mln_span_start`.

Return value: None



#### mln_span_release

```c
mln_span_release();
```

Description: Release the structure containing the measured values for this measurement. **Note**: This macro function should be called after `mln_span_stop`, otherwise, it may lead to memory access exceptions.

Return value: None



#### mln_span_move

```c
mln_span_move();
```

Description: Retrieve the structure containing the measured values for this measurement and set the global pointer pointing to this structure to `NULL`. **Note**: This macro function should be called after `mln_span_stop`, otherwise, unpredictable errors may occur.

Return value: Pointer to `mln_span_t`



#### mln_span_dump

```c
void mln_span_dump(mln_span_dump_cb_t cb, void *data);

typedef void (*mln_span_dump_cb_t)(mln_span_t *s, int level, void *data);
```

Description: Use a user-defined output function and auxiliary data to output the current measurement data. The meanings of the parameters of `mln_span_dump_cb_t` are as follows:

- `s`: Pointer to the current traversed span node.

- `level`: The current level of the span in the call stack (relative to the function call where `mln_span_start` is called).

- `data`: User-defined data.

Return value: None.


#### mln_span_free

```c
void mln_span_free(mln_span_t *s);
```

Description: Free the memory allocated for the `mln_span_t` structure.

Return value: None



#### mln_span_file

```c
mln_span_file(s);
```

Description: Get the filename of the specified span's location.

Return value: `char *` pointer to the filename.



#### mln_span_func

```c
mln_span_func(s);
```

Description: Get the function name referred to by the specified span.

Return value: `char *` pointer to the function name.



#### mln_span_line

```c
mln_span_line(s);
```

Description: Get the line number of the file referred to by the specified span.

Return value: `int` representing the line number.



#### mln_span_time_cost

```c
mln_span_time_cost(s);
```

Description: Get the duration of the measurement taken by the specified span, in microseconds.

Return value: `mln_u64_t` representing the duration in microseconds.



### Example

This is a multi-threading example demonstrating the usage of the `mln_span` interfaces and its runtime behavior in a multi-threaded environment.

```c
//a.c
#include <pthread.h>
#include "mln_span.h"
#include "mln_func.h"
#include "mln_log.h"

static void mln_span_dump_callback(mln_span_t *s, int level, void *data)
{
    int i;
    for (i = 0; i < level * 2; ++i) {
        mln_log(none, " ");
    }

    mln_log(none, "| %s at %s:%d takes %U (us)\n", \
            mln_span_func(s), mln_span_file(s), mln_span_line(s), mln_span_time_cost(s));
}

MLN_FUNC(, int, abc, (int a, int b), (a, b), {
    return a + b;
})

MLN_FUNC(static, int, bcd, (int a, int b), (a, b), {
    return abc(a, b) + abc(a, b);
})

MLN_FUNC(static, int, cde, (int a, int b), (a, b), {
    return bcd(a, b) + bcd(a, b);
})

void *pentry(void *args)
{
    int i;
    mln_span_start();
    for (i = 0; i < 10; ++i) {
        cde(i, i + 1);
    }

    mln_span_stop();
    mln_span_dump(mln_span_dump_callback, NULL);
    mln_span_release();
    return NULL;
}

int main(void)
{
    int i;
    pthread_t pth;


    pthread_create(&pth, NULL, pentry, NULL);

    for (i = 0; i < 10; ++i) {
        bcd(i, i + 1);
    }

    pthread_join(pth, NULL);

    return 0;
}
```

Compile the program:

```bash
cc -o a a.c -I /usr/local/melon/include/ -L /usr/local/melon/lib/ -lmelon -DMLN_FUNC_FLAG -lpthread
```

After execution, you should see the following output:

```
| pentry at a.c:32 takes 16 (us)
  | cde at a.c:25 takes 2 (us)
    | bcd at a.c:21 takes 1 (us)
      | abc at a.c:17 takes 1 (us)
      | abc at a.c:17 takes 0 (us)
    | bcd at a.c:21 takes 0 (us)
      | abc at a.c:17 takes 0 (us)
      | abc at a.c:17 takes 0 (us)
  | cde at a.c:25 takes 3 (us)
    | bcd at a.c:21 takes 1 (us)
      | abc at a.c:17 takes 0 (us)
      | abc at a.c:17 takes 1 (us)
    | bcd at a.c:21 takes 0 (us)
      | abc at a.c:17 takes 0 (us)
      | abc at a.c:17 takes 0 (us)
  | cde at a.c:25 takes 1 (us)
    | bcd at a.c:21 takes 1 (us)
      | abc at a.c:17 takes 0 (us)
      | abc at a.c:17 takes 0 (us)
    | bcd at a.c:21 takes 0 (us)
      | abc at a.c:17 takes 0 (us)
      | abc at a.c:17 takes 0 (us)
  | cde at a.c:25 takes 1 (us)
    | bcd at a.c:21 takes 1 (us)
      | abc at a.c:17 takes 1 (us)
      | abc at a.c:17 takes 0 (us)
    | bcd at a.c:21 takes 0 (us)
      | abc at a.c:17 takes 0 (us)
      | abc at a.c:17 takes 0 (us)
  | cde at a.c:25 takes 1 (us)
    | bcd at a.c:21 takes 1 (us)
      | abc at a.c:17 takes 0 (us)
      | abc at a.c:17 takes 0 (us)
    | bcd at a.c:21 takes 0 (us)
      | abc at a.c:17 takes 0 (us)
      | abc at a.c:17 takes 0 (us)
  | cde at a.c:25 takes 1 (us)
    | bcd at a.c:21 takes 1 (us)
      | abc at a.c:17 takes 0 (us)
      | abc at a.c:17 takes 0 (us)
    | bcd at a.c:21 takes 0 (us)
      | abc at a.c:17 takes 0 (us)
      | abc at a.c:17 takes 0 (us)
  | cde at a.c:25 takes 3 (us)
    | bcd at a.c:21 takes 2 (us)
      | abc at a.c:17 takes 0 (us)
      | abc at a.c:17 takes 0 (us)
    | bcd at a.c:21 takes 1 (us)
      | abc at a.c:17 takes 0 (us)
      | abc at a.c:17 takes 0 (us)
  | cde at a.c:25 takes 1 (us)
    | bcd at a.c:21 takes 0 (us)
      | abc at a.c:17 takes 0 (us)
      | abc at a.c:17 takes 0 (us)
    | bcd at a.c:21 takes 1 (us)
      | abc at a.c:17 takes 0 (us)
      | abc at a.c:17 takes 0 (us)
  | cde at a.c:25 takes 1 (us)
    | bcd at a.c:21 takes 0 (us)
      | abc at a.c:17 takes 0 (us)
      | abc at a.c:17 takes 0 (us)
    | bcd at a.c:21 takes 1 (us)
      | abc at a.c:17 takes 1 (us)
      | abc at a.c:17 takes 0 (us)
  | cde at a.c:25 takes 1 (us)
    | bcd at a.c:21 takes 0 (us)
      | abc at a.c:17 takes 0 (us)
      | abc at a.c:17 takes 0 (us)
    | bcd at a.c:21 takes 1 (us)
      | abc at a.c:17 takes 0 (us)
      | abc at a.c:17 takes 0 (us)
```

