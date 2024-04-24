#include "mln_class.h"
#include <stdio.h>

typedef void (*func_t)(void *);
static void _constructor(void *o, int a, func_t func);
static void _destructor(void *o);

class(F, _constructor, _destructor, {
    int a;
    func_t f;
});

static void fcall(void *o)
{
    printf("in function call\n");
}

static void _constructor(void *o, int a, func_t func)
{
    printf("constructor\n");
    F *f = (F *)o;
    f->a = a;
    f->f = func;
}

static void _destructor(void *o)
{
    printf("destructor\n");
}

int main(void)
{
     F *f = new(F, 1, fcall);
     printf("%d\n", f->a);
     f->f(f);
     delete(f);
     return 0;
}

