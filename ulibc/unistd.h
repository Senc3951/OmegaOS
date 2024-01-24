#pragma once

#include <syscall.h>

#define MAX_PATH    256

typedef struct dirent
{
    uint32_t d_ino;
    char d_name[MAX_PATH];
} dirent;

typedef struct DIR
{
    uint32_t d_fd;
    size_t d_currentEntry;
} DIR;

inline ssize_t read(uint32_t fd, void *buf, size_t count)
{
    return SYSCALL_3(SYSCALL_READ, fd, (uint64_t)buf, count);
}

inline ssize_t write(uint32_t fd, const void *buf, size_t count)
{
    return SYSCALL_3(SYSCALL_WRITE, fd, (uint64_t)buf, count);
}

inline int open(const char *path, int flags, int mode)
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

inline DIR *opendir(const char *name)
{
    uint64_t ret = SYSCALL_1(SYSCALL_OPENDIR, (uint64_t)name);
    return (DIR *)ret;
}

inline struct dirent *readdir(DIR *dirp)
{
    uint64_t ret = SYSCALL_1(SYSCALL_READDIR, (uint64_t)dirp);
    return (struct dirent *)ret;
}

inline void closedir(DIR *dirp)
{
    SYSCALL_1(SYSCALL_CLOSEDIR, (uint64_t)dirp);
}

inline int raise(int sig)
{
    return SYSCALL_1(SYSCALL_RAISE, sig);
}

inline int signal(int signum, uint64_t handler)
{
    return SYSCALL_2(SYSCALL_SIGNAL, signum, handler);
}

inline int chdir(const char *path)
{
    return SYSCALL_1(SYSCALL_CHDIR, (uint64_t)path);
}

inline char *getcwd(char *buf, size_t size)
{
    uint64_t ret = SYSCALL_2(SYSCALL_GETCWD, (uint64_t)buf, size);
    return (char *)ret;
}