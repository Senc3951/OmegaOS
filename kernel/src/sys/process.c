#include <sys/process.h>
#include <sys/scheduler.h>
#include <arch/gdt.h>
#include <mem/heap.h>
#include <assert.h>
#include <io/io.h>
#include <libc/string.h>

#define USER_RFLAGS 0x202

#define INIT_PROCESS_NAME   "init"
#define INIT_PROCESS_STACK  (1 * _KB)

extern void x64_init_proccess();

Process_t *createProcess(const char *name, PageTable_t *addressSpace, void *entry, void *stack, const uint64_t stackSize, const uint64_t cs, const uint64_t ds)
{    
    Process_t *process = (Process_t *)kmalloc(sizeof(Process_t));
    assert(process && addressSpace);
    process->pml4 = addressSpace;
    strncpy(process->name, name, MAX_PROCESS_NAME);
    strncpy(process->cwd, FS_PATH_SEPERATOR_STR, FS_MAX_PATH);
    
    memset(&process->ctx, 0, sizeof(process->ctx));
    process->ctx.cr3 = (uint64_t)process->pml4;
    process->ctx.rip = (uint64_t)entry;
    process->ctx.cs = cs;
    process->ctx.rsp = process->ctx.rbp = (uint64_t)stack + stackSize;
    process->ctx.rflags = USER_RFLAGS;
    process->ctx.ss = ds;
    scheduler_add(process);
    
    return process;
}

Process_t *process_createInit()
{
    void *stack = kmalloc(INIT_PROCESS_STACK);
    assert(stack);

    return createProcess(INIT_PROCESS_NAME, _KernelPML4, x64_init_proccess, stack, INIT_PROCESS_STACK, GDT_KERNEL_CS, GDT_KERNEL_DS);
}

Process_t *process_create(const char *name, void *entry, void *stack, const uint64_t stackSize)
{
    return createProcess(name, vmm_createAddressSpace(), entry, stack, stackSize, GDT_USER_CS, GDT_USER_DS);
}

void process_delete(Process_t *process)
{
    kfree((void *)(process->ctx.rsp - process->ctx.stackSize));
}