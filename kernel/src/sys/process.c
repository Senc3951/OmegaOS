#include <sys/process.h>
#include <sys/scheduler.h>
#include <arch/atomic.h>
#include <arch/gdt.h>
#include <mem/heap.h>
#include <misc/tree.h>
#include <assert.h>
#include <io/io.h>
#include <libc/string.h>
#include <logger.h>

#define USER_RFLAGS         0x202
#define INIT_PROCESS_NAME   "init"

static Tree_t *g_processTree = NULL;

static int getNextID()
{
    static int id = ROOT_PID;
    return x64_atomic_add((uint64_t *)&id, 1);
}

static Process_t *createProcess(const char *name, PageTable_t *addressSpace, void *entry, const ProcessPriority_t priority, void *stackButtom, uint64_t stackSize, const uint64_t cs, const uint64_t ds)
{
    Process_t *process = (Process_t *)kmalloc(sizeof(Process_t));
    if (!process)
        return NULL;
    
    process->pml4 = addressSpace;
    strncpy(process->name, name, MAX_PROCESS_NAME);
    strncpy(process->cwd, FS_PATH_SEPERATOR_STR, FS_MAX_PATH);
        
    memset(&process->ctx, 0, sizeof(process->ctx));
    process->ctx.rip = (uint64_t)entry;
    process->ctx.cs = cs;
    process->ctx.rsp = (uint64_t)stackButtom + stackSize;
    process->ctx.stackButtom = (uint64_t)stackButtom;
    process->ctx.rflags = USER_RFLAGS;
    process->ctx.ss = ds;
    process->ctx.stackSize = stackSize;
    process->priority = priority;
    process->id = getNextID();
    
    assert(process->fdt = (FileDescriptorTable_t *)kmalloc(sizeof(FileDescriptorTable_t)));
    process->fdt->size = 0;
    process->fdt->capacity = 1;
    assert(process->fdt->nodes = (VfsNode_t **)kmalloc(sizeof(VfsNode_t *)));
    
    LOG("Created process `%s` with id %u. entry at %p, stack at %p - %p\n", process->name, process->id, process->ctx.rip, process->ctx.stackButtom, process->ctx.stackButtom + stackSize);
    return process;
}

Process_t *process_init()
{
    extern void x64_idle();

    // Create the process tree
    assert(g_processTree = tree_create());
    
    // Create the idle process
    assert(_IdleProcess = createProcess(INIT_PROCESS_NAME, _KernelPML4, x64_idle, PriorityIdle, 0, 0, GDT_KERNEL_CS, GDT_KERNEL_DS));
    
    // Insert idle process as root process
    tree_set_root(g_processTree, _IdleProcess);
    _IdleProcess->treeNode = g_processTree->root;
    
    return _IdleProcess;
}

Process_t *process_create(const char *name, void *entry, const ProcessPriority_t priority)
{
    // Create a page table for the process and map the stack
    PageTable_t *pml4 = vmm_createAddressSpace(_IdleProcess->pml4);
    assert(pml4);
    assert(vmm_createPages(pml4, (void *)USER_STACK_START, USER_STACK_SIZE / PAGE_SIZE, VMM_USER_ATTRIBUTES));
    
    // Create the process and insert into the process tree
    Process_t *process = createProcess(name, pml4, entry, priority, (void *)USER_STACK_START, USER_STACK_SIZE, GDT_USER_CS, GDT_USER_DS);
    assert(process);
    assert(process->treeNode = tree_create_node(process));
    tree_insert(g_processTree, g_processTree->root, process->treeNode);
    
    // Insert the process as ready to schedule
    scheduler_add(process);
    
    return process;
}

void process_delete(Process_t *process)
{
    // Remove the process from the process tree
    assert(process->treeNode);
    Process_t *parentProcess = (Process_t *)(process->parent_proc);
    assert(parentProcess);
    tree_remove(g_processTree, process->treeNode);
    
    // Delete the page table and switch to kernel one
    assert(process->pml4 && parentProcess->pml4);
    vmm_switchTable(_KernelPML4);
    //vmm_destroyAddressSpace(parentProcess->pml4, process->pml4);
    
    // Close open files
    for (uint32_t i = 0; i < process->fdt->size; i++)
    {
        VfsNode_t *node = process->fdt->nodes[i];
        vfs_close(node);
        kfree(node);
    }
    
    kfree(process->fdt);
    kfree(process);
}