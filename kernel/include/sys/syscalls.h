#pragma once

#include <common.h>

#define SYSCALL_READ    0
#define SYSCALL_WRITE   1
#define SYSCALL_EXIT    2

/// @brief Stop the current process.
/// @param status Status to stop with.
void sys_exit(int status);

/// @brief Initialize syscalls.
void syscalls_init();