#pragma once

#include <mem/vmm.h>
#include <fs/vfs.h>

#define MAX_PROCESS_NAME    50
#define MAX_PROCESS_COUNT   50

#define PROCESS_STATUS_PENDING  0
#define PROCESS_STATUS_RUNNING  1
#define PROCESS_STATUS_STOPPED  2

/// @brief Context of a process.
typedef struct CONTEXT
{
    uint64_t rip, cs, rsp, rflags, ss;
    struct
    {
        uint64_t rax, rbx, rcx, rdx, rdi, rsi, rbp;
        uint64_t r8, r9, r10, r11, r12, r13, r14, r15;
    };
    uint64_t stackSize;
    uint64_t cr3;
} Context_t;

/// @brief Information of a process. 
typedef struct PROCESS
{
    int id;
    int status;
    Context_t ctx;
    PageTable_t *pml4;
    char name[MAX_PROCESS_NAME];
    char cwd[FS_MAX_PATH];
} Process_t;

/// @brief Create the init process.
/// @return Init process.
Process_t *process_createInit();

/// @brief Create a process.
/// @param name Name of the process.
/// @param entry Entry point of the process.
/// @param stack Stack of the process.
/// @param stackSize Size of the stack.
/// @return Created process.
Process_t *process_create(const char *name, void *entry, void *stack, const size_t stackSize);

/// @brief Delete a process.
/// @param process Process to delete.
void process_delete(Process_t *process);