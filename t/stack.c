#include "mln_stack.h"
#include <stdlib.h>
#include <assert.h>

typedef struct {
    void *data1;
    void *data2;
} data_t;

static void *copy(data_t *d, void *data)
{
    data_t *dup;
    assert((dup = (data_t *)malloc(sizeof(data_t))) != NULL);
    *dup = *d;
    return dup;
}

int main(void)
{
    int i;
    data_t *d;
    mln_stack_t *st1, *st2;

    assert((st1 = mln_stack_init((stack_free)free, (stack_copy)copy)) != NULL);

    for (i = 0; i < 3; ++i) {
        assert((d = (data_t *)malloc(sizeof(data_t))) != NULL);
        assert(mln_stack_push(st1, d) == 0);
    }

    assert((st2 = mln_stack_dup(st1, NULL)) != NULL);

    assert((d = mln_stack_pop(st1)) != NULL);
    free(d);

    mln_stack_destroy(st1);
    mln_stack_destroy(st2);

    return 0;
}
