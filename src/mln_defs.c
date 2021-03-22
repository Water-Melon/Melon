
/*
 * Copyright (C) Niklaus F.Schen.
 */
#include "mln_types.h"
#if defined(WINNT)
#include <windows.h>
#include <stdio.h>

int socketpair(int d, int type, int protocol, int *sv)
{
    static int count;
    char buf[64];
    HANDLE fd;
    DWORD dwMode;
    (void)d; (void)type; (void)protocol;
    sprintf(buf, "\\\\.\\pipe\\levent-%d", count++);
    /* Create a duplex pipe which will behave like a socket pair */
    fd = CreateNamedPipe(buf, PIPE_ACCESS_DUPLEX, PIPE_TYPE_BYTE | PIPE_NOWAIT,
        PIPE_UNLIMITED_INSTANCES, 4096, 4096, 0, NULL);
    if (fd == INVALID_HANDLE_VALUE)
        return (-1);
    sv[0] = (int)fd;

    fd = CreateFile(buf, GENERIC_READ|GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (fd == INVALID_HANDLE_VALUE)
        return (-1);
    dwMode = PIPE_NOWAIT;
    SetNamedPipeHandleState(fd, &dwMode, NULL, NULL);
    sv[1] = (int)fd;

    return (0);
}
#endif

int mln_socketpair_read(int fd, void *buf, size_t len)
{
#if defined(WINNT)
    DWORD n;
    int rc = ReadFile((HANDLE)fd, buf, len, &n, NULL);
    return rc? -1: n;
#else
    return read(fd, buf, len);
#endif
}

int mln_socketpair_write(int fd, void *buf, size_t len)
{
#if defined(WINNT)
    DWORD n;
    int rc = WriteFile((HANDLE)fd, buf, len, &n, NULL);
    return rc? -1: n;
#else
    return write(fd, buf, len);
#endif
}

int mln_socketpair_close(int fd)
{
#if defined(WINNT)
    return CloseHandle((HANDLE)fd);
#else
    return close(fd);
#endif
}


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

