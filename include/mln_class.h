
/*
 * Copyright (C) Niklaus F.Schen.
 */
#ifndef __MLN_CLASS_H
#define __MLN_CLASS_H

#include <stdlib.h>

#if !defined(MSVC)

#define new(type, ...) ({\
    type *_o = (type *)malloc(sizeof(type));\
    if (_o != NULL) {\
        _o->_constructor = type##_constructor;\
        _o->_destructor = type##_destructor;\
        type##_constructor(_o, ##__VA_ARGS__);\
    }\
    _o;\
})
#define delete(o, ...) ({\
    if (o != NULL) {\
        o->_destructor(o, ##__VA_ARGS__);\
        free(o);\
    }\
})
#define class(type, constructor, destructor, ...); \
    typedef struct type type; \
    struct type {\
        typeof(constructor) *_constructor;\
        typeof(destructor) *_destructor;\
        struct __VA_ARGS__;\
    };\
    typeof(constructor) *type##_constructor = constructor;\
    typeof(destructor) *type##_destructor = destructor;

#endif

#endif


#if 0

/*
 * Here is an example
 */

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
#endif

