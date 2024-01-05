#include <sys/process.h>
#include <sys/scheduler.h>
#include <arch/gdt.h>
#include <mem/heap.h>
#include <misc/tree.h>
#include <assert.h>
#include <io/io.h>
#include <libc/string.h>

#define USER_RFLAGS 0x202

#define INIT_PROCESS_NAME   "init"
#define INIT_STACK_SIZE     PAGE_SIZE

static Tree_t *g_processTree = NULL;

static void init()
{
    while (1)
        __HALT();
}

static Process_t *createProcess(const char *name, PageTable_t *addressSpace, void *entry, uint64_t stackSize, const uint64_t cs, const uint64_t ds)
{    
    Process_t *process = (Process_t *)kmalloc(sizeof(Process_t));
    assert(process && addressSpace);
    process->pml4 = addressSpace;
    strncpy(process->name, name, MAX_PROCESS_NAME);
    strncpy(process->cwd, FS_PATH_SEPERATOR_STR, FS_MAX_PATH);
        
    memset(&process->ctx, 0, sizeof(process->ctx));
    process->ctx.rip = (uint64_t)entry;
    process->ctx.cs = cs;
    process->ctx.rsp = process->ctx.rbp = USER_STACK_START + stackSize;
    process->ctx.stackButtom = USER_STACK_START;
    process->ctx.rflags = USER_RFLAGS;
    process->ctx.ss = ds;
    process->ctx.stackSize = stackSize;
    
    return process;
}

Process_t *process_init()
{
    assert(g_processTree = tree_create());

    PageTable_t *pml4 = vmm_createAddressSpace(_KernelPML4);
    vmm_createPages(pml4, (void *)USER_STACK_START, INIT_STACK_SIZE / PAGE_SIZE, VMM_USER_ATTRIBUTES);
    _InitProcess = createProcess(INIT_PROCESS_NAME, pml4, init, INIT_STACK_SIZE, GDT_KERNEL_CS, GDT_KERNEL_DS);
    
    tree_set_root(g_processTree, init);
    _InitProcess->treeNode = g_processTree->root;
    
    return _InitProcess;
}

Process_t *process_create(const char *name, void *entry)
{
    PageTable_t *pml4 = vmm_createAddressSpace(_InitProcess->pml4);
    vmm_createPages(pml4, (void *)USER_STACK_START, USER_STACK_SIZE / PAGE_SIZE, VMM_USER_ATTRIBUTES);
    
    Process_t *proc = createProcess(name, pml4, entry, USER_STACK_SIZE, GDT_USER_CS, GDT_USER_DS);
    proc->treeNode = tree_create_node(proc);
    tree_insert(g_processTree, g_processTree->root, proc->treeNode);
    scheduler_add(proc);
    
    return proc;
}

void process_delete(Process_t *process)
{
    Process_t *parentProccess = (Process_t *)(process->parent_proc);
    assert(process->treeNode && parentProccess && parentProccess->pml4);
    
    vmm_switchTable(parentProccess->pml4);
    vmm_destroyAddressSpace(parentProccess->pml4, process->pml4);
    
    tree_remove(g_processTree, process->treeNode);
    kfree(process);
}