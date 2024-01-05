#pragma once

#include <mem/vmm.h>
#include <fs/vfs.h>
#include <misc/tree.h>

#define MAX_PROCESS_NAME    50
#define MAX_PROCESS_COUNT   50

#define PROCESS_PENDING     0
#define PROCESS_RUNNING     1

/// @brief Context of a process.
typedef struct CONTEXT
{
    uint64_t rip, cs, rsp, rflags, ss;
    struct
    {
        uint64_t rax, rbx, rcx, rdx, rdi, rsi, rbp;
        uint64_t r8, r9, r10, r11, r12, r13, r14, r15;
    };
    uint64_t stackButtom, stackSize;
} Context_t;

/// @brief Information of a process. 
typedef struct PROCESS
{
    int id;
    int status;
    Context_t ctx;
    PageTable_t *pml4;
    TreeNode_t *treeNode;
    char name[MAX_PROCESS_NAME];
    char cwd[FS_MAX_PATH];
#define parent_proc treeNode->parent->value
} Process_t;

/// @brief Initialize processes.
/// @return Init process.
Process_t *process_init();

/// @brief Create a process.
/// @param name Name of the process.
/// @param entry Entry point of the process.
/// @return Created process.
Process_t *process_create(const char *name, void *entry);

/// @brief Delete a process.
/// @param process Process to delete.
void process_delete(Process_t *process);