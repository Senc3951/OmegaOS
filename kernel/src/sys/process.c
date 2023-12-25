#include <sys/process.h>
#include <sys/scheduler.h>
#include <mem/heap.h>
#include <io/io.h>
#include <libc/string.h>

Process_t *process_create(const char *name, void *entry, void *stack, const size_t stackSize)
{
    __CLI();
    
    Process_t *process = (Process_t *)kmalloc(sizeof(Process_t));
    process->prev = process->next = NULL;
    process->pml4 = NULL;   // TODO
    strncpy(process->name, name, MAX_PROCESS_NAME);
    strncpy(process->cwd, FS_PATH_SEPERATOR_STR, FS_MAX_PATH);
    
    memset(&process->regs, 0, sizeof(process->regs));
    process->regs.rip = (uint64_t)entry;
    process->regs.rsp = process->regs.rbp = (uint64_t)stack + stackSize;
    scheduler_addProcess(process);
    
    __STI();
    return process;
}