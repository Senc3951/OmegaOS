#include <scheduler/process.h>
#include <mem/heap.h>
#include <io/io.h>
#include <assert.h>
#include <libc/string.h>

Process_t *_CurrentProcess = NULL;

static Process_t g_processes[MAX_PROCESS_COUNT];
static int g_processCount = 0, g_processIndex = 0;
static int g_nextID = ROOT_PID;

void process_init()
{
    // Create the init process
    void *stack = kmalloc(IDLE_PROC_STACK_SIZE);
    assert(stack);
    
    extern void x64_idle_proccess();
    _CurrentProcess = process_create("init", x64_idle_proccess, stack, IDLE_PROC_STACK_SIZE);
}

Process_t *process_getNext()
{
    if (g_processIndex >= g_processCount)
        g_processIndex = 0;
    
    return &g_processes[g_processIndex++];
}

Process_t *process_create(const char *name, void *entry, void *stack, const size_t stackSize)
{
    assert(g_processCount < MAX_PROCESS_COUNT);
    __CLI();
    
    Process_t *process = &g_processes[g_processCount++];
    process->id = g_nextID++;
    process->prev = process->next = NULL;
    process->pml4 = NULL;   // TODO
    strncpy(process->name, name, MAX_PROCESS_NAME);
    strncpy(process->cwd, FS_PATH_SEPERATOR_STR, FS_MAX_PATH);
    
    memset(&process->regs, 0, sizeof(process->regs));
    process->regs.rip = (uint64_t)entry;
    process->regs.rsp = process->regs.rbp = (uint64_t)stack + stackSize;
    
    __STI();
    return process;
}