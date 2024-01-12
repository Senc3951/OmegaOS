#pragma once

#include <common.h>

#define SYSCALL_READ    0
#define SYSCALL_WRITE   1
#define SYSCALL_OPEN    2
#define SYSCALL_CLOSE   3
#define SYSCALL_EXIT    4

#define STDIN   0
#define STDOUT  1
#define STDERR  2

/// @brief Stop the current process.
/// @param status Status to stop with.
/// @return Status.
void sys_exit(int status);

/// @brief Initialize syscalls.
void syscalls_init();