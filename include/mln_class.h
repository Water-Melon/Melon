
/*
 * Copyright (C) Niklaus F.Schen.
 */
#ifndef __MLN_CLASS_H
#define __MLN_CLASS_H

#include <stdlib.h>

#define class(type, constructor, destructor, properties, ...); \
    typedef struct type type; \
    __VA_ARGS__ \
    struct type properties;\
    int type##_init(type *this) constructor \
    int type##_destroy(type *this) destructor \
    type *new##type() {\
        type *ptr = (type *)malloc(sizeof(*ptr));\
        if (ptr == NULL) return NULL;\
        if (type##_init(ptr)) {\
            free(ptr);\
            return NULL;\
        }\
        return ptr;\
    }\
    void del##type(type *ptr) {\
        if (ptr == NULL) return;\
        if (!type##_destroy(ptr)) free(ptr);\
    }

#endif

/*
 * Example
 * It defines a class named Foo.
 * And must be given constructor, destructor and structure properties and methods.
 * The rest things are optional. Commonly, they are declarations.
 * The object will be allocated via malloc.
 *
 * #include <stdio.h>
 * class(Foo, {
 *     printf("constructor\n");
 *     this->a = 1;
 *     this->method = foo_method;
 *     return 0;
 * },{
 *     printf("destructor\n");
 *     return 0;
 * },{
 *     int a;
 *     method method;
 * },
 * typedef void (*method)(Foo *);
 * void foo_method(Foo *this);
 * );
 * 
 * void foo_method(Foo *this)
 * {
 *   printf("foo method: %d\n", this->a);
 * }
 * 
 * int main(void)
 * {
 *     Foo *f = newFoo();
 *     f->method(f);
 *     delFoo(f);
 *     return 0;
 * }
 */
