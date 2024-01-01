#include <sys/process.h>
#include <sys/scheduler.h>
#include <arch/gdt.h>
#include <mem/heap.h>
#include <assert.h>
#include <io/io.h>
#include <libc/string.h>

#define USER_RFLAGS 0x202

#define INIT_PROCESS_NAME   "init"
#define INIT_STACK_SIZE     (1 * _KB)

extern void x64_init_proccess();

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
    process->ctx.rflags = USER_RFLAGS;
    process->ctx.ss = ds;
    process->ctx.stackSize = stackSize;
    scheduler_add(process);
    
    return process;
}

Process_t *process_createInit()
{
    vmm_createPages(_KernelPML4, (void *)USER_STACK_START, INIT_STACK_SIZE / PAGE_SIZE, VMM_USER_ATTRIBUTES);    
    return createProcess(INIT_PROCESS_NAME, _KernelPML4, x64_init_proccess, USER_STACK_START, INIT_STACK_SIZE, GDT_KERNEL_CS, GDT_KERNEL_DS);
}

Process_t *process_create(const char *name, void *entry)
{
    PageTable_t *pml4 = vmm_createAddressSpace();
    vmm_createPages(pml4, (void *)USER_STACK_START, USER_STACK_SIZE / PAGE_SIZE, VMM_USER_ATTRIBUTES);
    
    return createProcess(name, pml4, entry, USER_STACK_START, USER_STACK_SIZE, GDT_USER_CS, GDT_USER_DS);
}

void process_delete(Process_t *process)
{
    vmm_unmapPages(process->pml4, (void *)(process->ctx.rsp - process->ctx.stackSize), process->ctx.stackSize / PAGE_SIZE);
    vmm_unmapPage(_KernelPML4, process->pml4);
    kfree(process);
}