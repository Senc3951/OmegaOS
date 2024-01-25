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

static bool createStandardFiles(Process_t *process)
{
    if (process_add_file(process, createStdinNode()) < 0)
        return false;
    if (process_add_file(process, createStdoutNode()) < 0)
        return false;
    if (process_add_file(process, createStderrNode()) < 0)
        return false;
    
    return true;
}

static Process_t *createProcess(const char *name, PageTable_t *addressSpace, void *entry, const ProcessPriority_t priority, void *stackButtom, uint64_t stackSize, const uint64_t cs, const uint64_t ds)
{
    LOG("[REMOVE] Creating process %s with entry %p, priority: %u\n", name, entry, priority);
    Process_t *process = (Process_t *)kmalloc(sizeof(Process_t));
    if (!process)
    {
        return NULL;
        LOG("[REMOVE] Failed creating process %s with entry %p\n", name, entry);
    }
    
    process->pml4 = addressSpace;
    strncpy(process->name, name, MAX_PROCESS_NAME);
    memset(process->cwd, 0, FS_MAX_PATH);
    strncpy(process->cwd, FS_PATH_SEPERATOR_STR, strlen(FS_PATH_SEPERATOR_STR));
    
    // Basic information and registers
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
    
    proc_dump(process, "createProcess_Start");
    
    // Signal queue
    memset(process->sigtb, 0, sizeof(process->sigtb));
    if (!(process->sigQueue = queue_create()))
    {
        process_delete(process);
        return NULL;
    } 

    // Open files
    if (!(process->fdt = list_create()))
    {
        process_delete(process);
        return NULL;
    }
    
    // stdin, stdout, stderr
    if (!createStandardFiles(process))
    {
        process_delete(process);
        return NULL;
    }
    
    LOG("Created process `%s` with id %u. entry at %p, stack at %p - %p\n", process->name, process->id, process->ctx.rip, process->ctx.stackButtom, process->ctx.stackButtom + stackSize);
    proc_dump(process, "createProcess_Final");
    
    return process;
}

void proc_dump(Process_t *p, char *n)
{
    LOG("-------- DUMPING PROCESS %s/%u from function %s -------\n", p->name, p->id, n);
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
    LOG("[REMOVE] creating address space for process %s\n", name);
    PageTable_t *pml4 = vmm_createAddressSpace(_IdleProcess->pml4);
    if (!pml4)
    {
        LOG("[REMOVE] failed creating pml4\n");
        return NULL;
    }
    LOG("[REMOVE] creating stack for process %s\n", name);
    if (!vmm_createPages(pml4, (void *)USER_STACK_START, USER_STACK_SIZE / PAGE_SIZE, VMM_USER_ATTRIBUTES))
    {
        LOG("[REMOVE] failed creating stack\n");
        vmm_destroyAddressSpace(_IdleProcess->pml4, pml4);
        return NULL;
    }
    
    // Create the process    
    Process_t *process = createProcess(name, pml4, entry, priority, (void *)USER_STACK_START, USER_STACK_SIZE, GDT_USER_CS, GDT_USER_DS);
    if (!process)
        return NULL;
    
    // Insert the process into the process tree
    if (!(process->treeNode = tree_create_node(process)))
    {
        process_delete(process);
        return NULL;
    }
    tree_insert(g_processTree, g_processTree->root, process->treeNode);
    
    // Notify the scheduler about the process
    scheduler_add(process);
    proc_dump(process, "process_create_Final");
    
    return process;
}

void process_delete(Process_t *process)
{
    // Remove the process from the process tree
    Process_t *parentProcess = NULL;
    if (process->treeNode)
    {    
        parentProcess = (Process_t *)(process->parent_proc);
        assert(parentProcess);
        tree_remove(g_processTree, process->treeNode);
    }
    
    // Delete the page table and switch to kernel one
    if (process->pml4)
    {
        assert(parentProcess);
        vmm_switchTable(_KernelPML4);
        vmm_destroyAddressSpace(parentProcess->pml4, process->pml4);    
    }
    
    // Close all files
    if (process->fdt)
    {
        foreach(node, process->fdt) {
            vfs_close((VfsNode_t *)node->value);
        }
        list_destroy(process->fdt);
        list_free(process->fdt);  
        kfree(process->fdt);
    }

    // Close all signals
    if (process->sigQueue)
    {
        signal_t *sig;
        while ((sig = (signal_t *)queue_deqeueue(process->sigQueue)))
            kfree(sig);
        kfree(process->sigQueue);
    }
    
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