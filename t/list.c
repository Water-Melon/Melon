#include "mln_list.h"
#include "mln_utils.h"
#include <stdlib.h>

typedef struct {
    int        val;
    mln_list_t node;
} test_t;

int main(void)
{
    int i;
    test_t *t, *fr;
    mln_list_t sentinel = mln_list_null();

    for (i = 0; i < 3; ++i) {
        t = (test_t *)calloc(1, sizeof(*t));
        if (t == NULL)
            return -1;
        mln_list_add(&sentinel, &t->node);
        t->val = i;
    }
    for (t = mln_container_of(mln_list_head(&sentinel), test_t, node); \
         t != NULL; \
         t = mln_container_of(mln_list_next(&t->node), test_t, node))
    {
        printf("%d\n", t->val);
    }
    for (t = mln_container_of(mln_list_head(&sentinel), test_t, node); t != NULL; ) {
        fr = t;
        t = mln_container_of(mln_list_next(&t->node), test_t, node);
        free(fr);
    }
    return 0;
}

