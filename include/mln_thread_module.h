
/*
 * Copyright (C) Niklaus F.Schen.
 */
#ifndef __MLN_THREAD_MODULE_H
#define __MLN_THREAD_MODULE_H
typedef int (*tmain)(int, char **);

typedef struct {
    char    *alias;
    tmain    thread_main;
} mln_thread_module_t;

extern int haha_main(int argc, char **argv);
extern int hello_main(int argc, char **argv);
extern void *mln_get_module_entrance(char *alise);
#endif
