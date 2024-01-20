#pragma once

#include <common.h>

#define SYSCALL_READ        0
#define SYSCALL_WRITE       1
#define SYSCALL_OPEN        2
#define SYSCALL_CLOSE       3
#define SYSCALL_EXIT        4
#define SYSCALL_MKDIR       5
#define SYSCALL_FTELL       6
#define SYSCALL_LSEEK       7
#define SYSCALL_OPENDIR     8
#define SYSCALL_READDIR     9
#define SYSCALL_CLOSEDIR    10
#define SYSCALL_RAISE       11
#define SYSCALL_SIGNAL      12

/// @brief Stop the current process.
/// @param status Status to stop with.
/// @return Status.
void sys_exit(int status);

/// @brief Initialize syscalls.
void syscalls_init();