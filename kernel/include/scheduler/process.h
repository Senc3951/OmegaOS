#pragma once

#include <mem/vmm.h>
#include <fs/vfs.h>

#define MAX_PROCESS_NAME    50
#define MAX_PROCESS_COUNT   50

/// @brief Registers stored when a process is switched.
typedef struct REGISTERS
{
    uint64_t rsp;
    uint64_t rbp;
    uint64_t rip;
} Registers_t;

/// @brief Information of a process. 
typedef struct PROCESS
{
    int id;
    Registers_t regs;
    PageTable_t *pml4;
    char name[MAX_PROCESS_NAME];
    char cwd[FS_MAX_PATH];
    struct PROCESS *next;
    struct PROCESS *prev;
} Process_t;

/// @brief Initialize the processes.
void process_init();

/// @brief Get the next process.
/// @return The next process.
Process_t *process_getNext();

/// @brief Create a process.
/// @param name Name of the process.
/// @param entry Entry point of the process.
/// @param stack Stack of the process.
/// @param stackSize Size of the stack.
/// @return Process created.
Process_t *process_create(const char *name, void *entry, void *stack, const size_t stackSize);

/// @brief Current running process.
extern Process_t *_CurrentProcess;