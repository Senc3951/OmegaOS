#pragma once

#include <mem/vmm.h>
#include <fs/vfs.h>

#define MAX_PROCESS_NAME    50
#define MAX_PROCESS_COUNT   50

/// @brief Registers stored of a process.
typedef struct PROCESS_REGISTERS
{
    uint64_t rsp, rbp, rip, rflags;
    uint64_t rax, rbx, rcx, rdx, rdi, rsi;
} PRegisters_t;

/// @brief Information of a process. 
typedef struct PROCESS
{
    int id;
    PRegisters_t regs;
    PageTable_t *pml4;
    char name[MAX_PROCESS_NAME];
    char cwd[FS_MAX_PATH];
    struct PROCESS *next;
    struct PROCESS *prev;
} Process_t;

/// @brief Create a process.
/// @param name Name of the process.
/// @param entry Entry point of the process.
/// @param stack Stack of the process.
/// @param stackSize Size of the stack.
/// @return Created process.
Process_t *process_create(const char *name, void *entry, void *stack, const size_t stackSize);