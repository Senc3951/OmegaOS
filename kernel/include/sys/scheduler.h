#pragma once

#include <arch/isr.h>
#include <sys/process.h>

#define PROC_FILE_AT(fd)    ((VfsNode_t *)(list_find_index(currentProcess()->fdt, fd)->value))

/// @brief Initialize the scheduler.
void scheduler_init();

/// @brief Add a process to the scheduler.
/// @param process Process to add.
void scheduler_add(Process_t *process);

/// @brief Remove a process from the scheduler.
/// @param process Process to remove.
void scheduler_remove(Process_t *process);

/// @brief Switch a process.
/// @param process Process to switch to.
void yield(Process_t *process);

/// @brief Switch a process.
/// @param stack Stack before switching the process.
/// @return New process.
Process_t *dispatch(InterruptStack_t *stack);

/// @brief Get the current process.
/// @return Current process.
Process_t *currentProcess();