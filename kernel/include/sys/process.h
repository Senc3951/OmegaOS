#pragma once

#include <mem/vmm.h>
#include <fs/vfs.h>
#include <misc/tree.h>

#define MAX_PROCESS_NAME            50
#define MAX_PROCESS_COUNT           50
#define PROCESS_PRIORITIES_COUNT    (PriorityInteractive + 1)

/// @brief Types of process priorities.
typedef enum PROCESS_PRIORITY
{
    PriorityIdle,
    PriorityLow,
    PriorityHigh,
    PriorityInteractive
} ProcessPriority_t;

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

typedef struct DESCRIPTOR_TABLE
{
    VfsNode_t **nodes;
    int size;
    int capacity;
} FileDescriptorTable_t;

/// @brief Information of a process. 
typedef struct PROCESS
{
    int id;
    ProcessPriority_t priority;
    Context_t ctx;
    PageTable_t *pml4;
    TreeNode_t *treeNode;
    FileDescriptorTable_t *fdt;
    char name[MAX_PROCESS_NAME];
    char cwd[FS_MAX_PATH];
#define parent_proc treeNode->parent->value
} Process_t;

/// @brief Initialize processes.
/// @return Idle process.
Process_t *process_init();

/// @brief Create a process.
/// @param name Name of the process.
/// @param entry Entry point of the process.
/// @param priority Priority of the process.
/// @return Created process.
Process_t *process_create(const char *name, void *entry, const ProcessPriority_t priority);

/// @brief Delete a process.
/// @param process Process to delete.
void process_delete(Process_t *process);