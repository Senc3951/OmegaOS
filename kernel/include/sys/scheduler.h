#pragma once

#include <arch/isr.h>
#include <sys/process.h>

#define PROC_FILE_AT(fd)    ((VfsNode_t *)(list_find_index(_CurrentProcess->fdt, fd)->value))

/// @brief Initialize the scheduler.
void scheduler_init();

/// @brief Add a process to the scheduler.
/// @param process Process to add.
void scheduler_add(Process_t *process);

/// @brief Remove a process from the scheduler.
/// @param process Process to remove.
void scheduler_remove(Process_t *process);

/// @brief Switch a process.
void yield();

/// @brief Switch a process.
/// @param stack Stack before switching the process.
void yield_cs(InterruptStack_t *stack);

extern Process_t *_CurrentProcess;  /* Current running process. */
extern Process_t *_IdleProcess;     /* Init process. */