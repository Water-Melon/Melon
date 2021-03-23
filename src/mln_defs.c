
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

#if defined(WINNT)
/*
 * Copy and modify from github
 *
 * Niklaus
 */
/*
 * fork.c
 * Experimental fork() on Windows.  Requires NT 6 subsystem or
 * newer.
 *
 * Copyright (c) 2012 William Pitcock <nenolod@dereferenced.org>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * This software is provided 'as is' and without any warranty, express or
 * implied.  In no event shall the authors be liable for any damages arising
 * from the use of this software.
 */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winnt.h>
#include <winternl.h>
#include <stdio.h>
#include <errno.h>
#include <assert.h>
#include <process.h>

typedef struct _SECTION_IMAGE_INFORMATION {
    PVOID EntryPoint;
    ULONG StackZeroBits;
    ULONG StackReserved;
    ULONG StackCommit;
    ULONG ImageSubsystem;
    WORD SubSystemVersionLow;
    WORD SubSystemVersionHigh;
    ULONG Unknown1;
    ULONG ImageCharacteristics;
    ULONG ImageMachineType;
    ULONG Unknown2[3];
} SECTION_IMAGE_INFORMATION, *PSECTION_IMAGE_INFORMATION;

typedef struct _RTL_USER_PROCESS_INFORMATION {
    ULONG Size;
    HANDLE Process;
    HANDLE Thread;
    CLIENT_ID ClientId;
    SECTION_IMAGE_INFORMATION ImageInformation;
} RTL_USER_PROCESS_INFORMATION, *PRTL_USER_PROCESS_INFORMATION;

#define RTL_CLONE_PROCESS_FLAGS_CREATE_SUSPENDED    0x00000001
#define RTL_CLONE_PROCESS_FLAGS_INHERIT_HANDLES        0x00000002
#define RTL_CLONE_PROCESS_FLAGS_NO_SYNCHRONIZE        0x00000004

#define RTL_CLONE_PARENT                0
#define RTL_CLONE_CHILD                    297

typedef NTSTATUS (*RtlCloneUserProcess_f)(ULONG ProcessFlags,
    PSECURITY_DESCRIPTOR ProcessSecurityDescriptor /* optional */,
    PSECURITY_DESCRIPTOR ThreadSecurityDescriptor /* optional */,
    HANDLE DebugPort /* optional */,
    PRTL_USER_PROCESS_INFORMATION ProcessInformation);

pid_t fork(void)
{
    HMODULE mod;
    RtlCloneUserProcess_f clone_p;
    RTL_USER_PROCESS_INFORMATION process_info;
    NTSTATUS result;

    mod = GetModuleHandle("ntdll.dll");
    if (!mod)
        return -ENOSYS;

    clone_p = (RtlCloneUserProcess_f)GetProcAddress(mod, "RtlCloneUserProcess");
    if (clone_p == NULL)
        return -ENOSYS;

    /* lets do this */
    result = clone_p(RTL_CLONE_PROCESS_FLAGS_CREATE_SUSPENDED | RTL_CLONE_PROCESS_FLAGS_INHERIT_HANDLES, NULL, NULL, NULL, &process_info);

    if (result == RTL_CLONE_PARENT)
    {
        HANDLE hp, ht;
        DWORD pi, ti;
        pi = (DWORD)process_info.ClientId.UniqueProcess;
        ti = (DWORD)process_info.ClientId.UniqueThread;

        assert(hp = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pi));
        assert(ht = OpenThread(THREAD_ALL_ACCESS, FALSE, ti));

        ResumeThread(ht);
        CloseHandle(ht);
        CloseHandle(hp);
        return (pid_t)pi;
    }
    else if (result == RTL_CLONE_CHILD)
    {
        /* fix stdio */
        AllocConsole();
        return 0;
    }
    else
        return -1;

    /* NOTREACHED */
    return -1;
}
#endif

