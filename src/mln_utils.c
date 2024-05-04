
/*
 * Copyright (C) Niklaus F.Schen.
 */
#if defined(MSVC)
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

#include "mln_types.h"


#if defined(__GNUC__) && (__GNUC__ >= 4 && __GNUC_MINOR__ > 1)

void spin_lock(void *lock)
{
    long *l = (long *)lock;
    while (!(__sync_bool_compare_and_swap(l, 0, 1)))
        ;
}

void spin_unlock(void *lock)
{
    long *l = (long *)lock;
    __sync_bool_compare_and_swap(l, 1, 0);
}

int spin_trylock(void *lock)
{
    long *l = (long *)lock;
    return !__sync_bool_compare_and_swap(l, 0, 1);
}

#elif defined(i386) || defined(__x86_64)

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

#if defined(MLN_C99) && defined(__linux__)
#include <time.h>
#include <stdio.h>
void usleep(unsigned long usec)
{
    struct timespec req;
    req.tv_sec = usec / 1000000;
    req.tv_nsec = (usec % 1000000) * 1000;
    nanosleep(&req, NULL);
}
#endif

#if defined(MSVC)
#define EPOCH 116444736000000000ULL
int gettimeofday(struct timeval *tv, void *tz)
{
    FILETIME ft;
    mln_u64_t tmpres = 0;

    if (tv != NULL) {
        GetSystemTimeAsFileTime(&ft);
        tmpres |= ft.dwHighDateTime;
        tmpres <<= 32;
        tmpres |= ft.dwLowDateTime;
        tmpres /= 10;
        tmpres -= EPOCH;
        tv->tv_sec = tmpres / 1000000;
        tv->tv_usec = tmpres % 1000000;
    }

    return 0;
}

DIR* opendir(const char* path)
{
    DIR* dir = (DIR*)malloc(sizeof(DIR));
    if (dir == NULL) {
        return NULL;
    }

    char searchPath[MAX_PATH];
    snprintf(searchPath, MAX_PATH, "%s\\*", path);
    dir->hFind = FindFirstFileA(searchPath, &dir->findFileData);
    if (dir->hFind == INVALID_HANDLE_VALUE) {
        free(dir);
        return NULL;
    }

    dir->path = _strdup(path);
    InitializeCriticalSection(&dir->lock);
    return dir;
}

struct dirent* readdir(DIR* dirp)
{
    if (dirp == NULL || dirp->hFind == INVALID_HANDLE_VALUE) {
        return NULL;
    }

    EnterCriticalSection(&dirp->lock);
    static struct dirent entry;
    if (FindNextFileA(dirp->hFind, &dirp->findFileData) != 0) {
        strncpy(entry.d_name, dirp->findFileData.cFileName, sizeof(entry.d_name));
        entry.d_name[sizeof(entry.d_name) - 1] = '\0';
        LeaveCriticalSection(&dirp->lock);
        return &entry;
    }

    LeaveCriticalSection(&dirp->lock);
    return NULL;
}

int closedir(DIR* dirp)
{
    if (dirp == NULL || dirp->hFind == INVALID_HANDLE_VALUE) {
        return -1;
    }

    EnterCriticalSection(&dirp->lock);
    FindClose(dirp->hFind);
    free(dirp->path);
    LeaveCriticalSection(&dirp->lock);
    DeleteCriticalSection(&dirp->lock);
    free(dirp);
    return 0;
}
#endif

