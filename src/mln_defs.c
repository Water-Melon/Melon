
/*
 * Copyright (C) Niklaus F.Schen.
 */
#include "mln_types.h"
#if defined(WINNT)
#include <stdio.h>
#include <winsock2.h>
#include <windows.h>
int pipe(int fds[2])
{
    int namelen, fd, fd1, fd2;
    struct sockaddr_in name;
    memset(&name, 0, sizeof(name));
    name.sin_family = AF_INET;
    name.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    namelen = sizeof(name);
    fd1 = fd2 = -1;
    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) goto clean;
    if (bind(fd, (struct sockaddr *)&name, namelen) < 0) goto clean;
    if (listen(fd, 5) < 0) goto clean;
    if (getsockname(fd, (struct sockaddr *)&name, &namelen) < 0) goto clean;
    fd1 = socket(AF_INET, SOCK_STREAM, 0);
    if (fd1 < 0) goto clean;
    if (connect(fd1, (struct sockaddr *)&name, namelen) < 0) goto clean;
    fd2 = accept(fd, (struct sockaddr *)&name, &namelen);
    if (fd2 < 0) goto clean;
    if (closesocket(fd) < 0) goto clean;
    fds[0] = fd1;
    fds[1] = fd2;
    return 0;

clean:
    if (fd >= 0) closesocket(fd);
    if (fd1 >= 0) closesocket(fd1);
    if (fd2 >= 0) closesocket(fd2);
    return -1;
}

int socketpair(int domain, int type, int protocol, int sv[2])
{
    return pipe(sv);
}
#endif


#if defined(i386) || defined(__x86_64)
#define barrier() asm volatile ("": : :"memory")
#define cpu_relax() asm volatile ("pause\n": : :"memory")
static inline unsigned long xchg(void *ptr, unsigned long x)
{
    __asm__ __volatile__("xchg %0, %1"
                         :"=r" (x)
                         :"m" (*(volatile long *)ptr), "0"(x)
                         :"memory");
    return x;
}

void spin_lock(void *lock)
{
    unsigned long *l = (unsigned long *)lock;
    while (1) {
        if (!xchg(l, 1)) return;
        while (*l) cpu_relax();
    }
}

void spin_unlock(void *lock)
{
    unsigned long *l = (unsigned long *)lock;
    barrier();
    *l = 0;
}

int spin_trylock(void *lock)
{
    return !xchg(lock, 1);
}
#endif

