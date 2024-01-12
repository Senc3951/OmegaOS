#include <sys/process.h>
#include <sys/scheduler.h>
#include <arch/gdt.h>
#include <mem/heap.h>
#include <misc/tree.h>
#include <assert.h>
#include <libc/string.h>

#define USER_RFLAGS 0x202

#define INIT_PROCESS_NAME   "init"

static Tree_t *g_processTree = NULL;

extern void x64_idle();

Process_t *process_init()
{
    assert(g_processTree = tree_create());
    Process_t *init = process_createInit();
    
    tree_set_root(g_processTree, init);
    init->treeNode = g_processTree->root;

    return init;
}

Process_t *createProcess(const char *name, PageTable_t *addressSpace, void *entry, uint64_t stack, const uint64_t stackSize, const uint64_t cs, const uint64_t ds)
{    
    Process_t *process = (Process_t *)kmalloc(sizeof(Process_t));
    assert(process && addressSpace);
    process->pml4 = addressSpace;
    strncpy(process->name, name, MAX_PROCESS_NAME);
    strncpy(process->cwd, FS_PATH_SEPERATOR_STR, FS_MAX_PATH);
        
    memset(&process->ctx, 0, sizeof(process->ctx));
    process->ctx.rip = (uint64_t)entry;
    process->ctx.cs = cs;
    process->ctx.rsp = process->ctx.rbp = stack + stackSize;
    process->ctx.stackButtom = stack;
    process->ctx.rflags = USER_RFLAGS;
    process->ctx.ss = ds;
    process->ctx.stackSize = stackSize;
    
    return process;
}

Process_t *process_createInit()
{
    PageTable_t *pml4 = vmm_createAddressSpace();    
    return createProcess(INIT_PROCESS_NAME, pml4, x64_idle, USER_STACK_START, 0, GDT_KERNEL_CS, GDT_KERNEL_DS);
}

Process_t *process_create(const char *name, void *entry)
{
    PageTable_t *pml4 = vmm_createAddressSpace();
    vmm_createPages(pml4, (void *)USER_STACK_START, USER_STACK_SIZE / PAGE_SIZE, VMM_USER_ATTRIBUTES);
    
    Process_t *proc = createProcess(name, pml4, entry, USER_STACK_START, USER_STACK_SIZE, GDT_USER_CS, GDT_USER_DS);
    proc->treeNode = tree_create_node(proc);
    tree_insert(g_processTree, g_processTree->root, proc->treeNode);
    scheduler_add(proc);
    
    return proc;
}

void process_delete(Process_t *process)
{
    assert(process->treeNode);
    tree_remove(g_processTree, process->treeNode);
    
    vmm_unmapPages(process->pml4, (void *)(process->ctx.stackButtom), process->ctx.stackSize / PAGE_SIZE);
    vmm_unmapPage(_KernelPML4, process->pml4);
    kfree(process);
}