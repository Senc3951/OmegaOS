#pragma once

#include <sys/signal.h>
#include <mem/vmm.h>
#include <fs/vfs.h>
#include <misc/queue.h>
#include <misc/tree.h>

#define MAX_PROCESS_NAME            50
#define MAX_PROCESS_COUNT           50
#define PROCESS_PRIORITIES_COUNT    (PriorityInteractive + 1)

/// @brief Types of process priorities.
typedef enum PROCESS_PRIORITY
{
    PriorityIdle = 0,
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

/// @brief Information of a process. 
typedef struct PROCESS
{
    int id;
    short priority;
    Context_t ctx;
    PageTable_t *pml4;
    TreeNode_t *treeNode;
    list_t *fdt;
    uint64_t sigtb[SIG_COUNT];
    Queue_t *sigQueue;
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

void proc_dump(Process_t *p, char *n);

/// @brief Delete a process.
/// @param process Process to delete.
void process_delete(Process_t *process);

/// @brief Add a file to the list of open files.
/// @param process Process to add the file to.
/// @param node File to add.
/// @return fd of the added file, -ENOMEM if an error occurred.
int process_add_file(Process_t *process, VfsNode_t *node);

/// @brief Close a file.
/// @param process Process to close the file to.
/// @param fd File descriptor of the file.
/// @return True if successfully closed the file, False, otherwise.
bool process_close_file(Process_t *process, const uint32_t fd);