#include <sys/process.h>
#include <sys/scheduler.h>
#include <arch/atomic.h>
#include <arch/gdt.h>
#include <fs/std.h>
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
    process->priority = (short)priority;
    process->id = getNextID();

    memset(process->sigtb, 0, sizeof(process->sigtb));
    assert(process->sigQueue = queue_create());
    
    assert(process->fdt = list_create());
    assert(process_add_file(process, createStdinNode()) >= 0);
    assert(process_add_file(process, createStdoutNode()) >= 0);
    assert(process_add_file(process, createStderrNode()) >= 0);
        
    LOG("Created process `%s` with id %u. entry at %p, stack at %p - %p\n", process->name, process->id, process->ctx.rip, process->ctx.stackButtom, process->ctx.stackButtom + stackSize);
    return process;
}

void proc_dump(Process_t *p)
{
    LOG("-------- DUMPING PROCESS %s/%u -------\n", p->name, p->id);
    LOG("CWD: `%s`, PML4: %p, Priority: %d, TreeNode: %p\n", p->cwd, p->pml4, p->priority, p->treeNode);
    LOG("Stack: %p, Stack Size: %u, Entry: %p\n\n", p->ctx.stackButtom, p->ctx.stackSize, p->ctx.rip);
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
    proc_dump(process);
    assert(process);
    assert(process->treeNode = tree_create_node(process));
    tree_insert(g_processTree, g_processTree->root, process->treeNode);
    
    // Insert the process as ready to schedule
    scheduler_add(process);
    proc_dump(process);
    
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
    vmm_destroyAddressSpace(parentProcess->pml4, process->pml4);
    
    // Close all files
    foreach(node, process->fdt) {
        vfs_close((VfsNode_t *)node->value);
    }
    list_destroy(process->fdt);
    list_free(process->fdt);  
    kfree(process->fdt);

    // Close all signals
    signal_t *sig;
    while ((sig = (signal_t *)queue_deqeueue(process->sigQueue)))
        kfree(sig);
    kfree(process->sigQueue);
    
    kfree(process);
}

int process_add_file(Process_t *process, VfsNode_t *node)
{
    int fd = list_insert(process->fdt, node);
    return fd >= 0 ? fd : -ENOMEM;
}

bool process_close_file(Process_t *process, const uint32_t fd)
{
    node_t *node = list_find_index(process->fdt, fd);
    if (!node)
        return false;
    
    VfsNode_t *vfsNode = (VfsNode_t *)(node->value);
    vfs_close(vfsNode);
    
    return true;
}