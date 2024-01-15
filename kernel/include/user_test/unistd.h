#pragma once

#include <user_test/syscall.h>

typedef long ssize_t;

#define stdin       0
#define stdout      1
#define stderr      2

#define SEEK_SET    0
#define SEEK_CUR    1
#define SEEK_END    2

inline ssize_t read(uint32_t fd, void *buf, size_t count)
{
    return SYSCALL_3(SYSCALL_READ, fd, (uint64_t)buf, count);
}

inline ssize_t write(uint32_t fd, const void *buf, size_t count)
{
    return SYSCALL_3(SYSCALL_WRITE, fd, (uint64_t)buf, count);
}

inline uint32_t open(const char *path, int flags, int mode)
{
    return SYSCALL_3(SYSCALL_OPEN, (uint64_t)path, flags, mode);
}

inline int close(uint32_t fd)
{
    return SYSCALL_1(SYSCALL_CLOSE, fd);
}

inline void exit(int status)
{
    SYSCALL_1(SYSCALL_EXIT, status);
}

inline int mkdir(const char *filename, uint32_t attr)
{
    return SYSCALL_2(SYSCALL_MKDIR, (uint64_t)filename, attr);
}

inline long ftell(uint32_t fd)
{
    return SYSCALL_1(SYSCALL_FTELL, fd);
}

inline int lseek(uint32_t fd, long offset, int whence)
{
    return SYSCALL_3(SYSCALL_LSEEK, fd, offset, whence);
}