#pragma once

#include <arch/isr.h>
#include <sys/process.h>

/// @brief Spawn the init process.
void spawnInit();

/// @brief Add a process to the scheduler.
/// @param process Process to add.
void scheduler_addProcess(Process_t *process);

void scheduler_remove(Process_t *process);

/// @brief Switch a process.
/// @param stack Stack before switching the process.
void yield(InterruptStack_t *stack);

extern Process_t *_CurrentProcess;  /* Current running process. */
extern Process_t *_InitProcess;     /* Init process. */