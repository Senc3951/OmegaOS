#pragma once

#include <common.h>

#define SYSCALL_EXIT 0

/// @brief Stop the current process.
/// @param status Status to stop with.
/// @return Status.
int sys_exit(int status);

/// @brief Initialize syscalls.
void syscalls_init();