#include <sys/scheduler.h>
#include <mem/heap.h>
#include <io/io.h>
#include <assert.h>
#include <libc/string.h>
#include <logger.h>

#define SWITCH_PROCESS(process) { x64_switch_processes(GDT_USER_CS, GDT_USER_DS, &process->regs); }

extern void x64_switch_processes(uint64_t cs, uint64_t ds, PRegisters_t *regs);
extern void x64_idle_proccess();

Process_t *_CurrentProcess = NULL, *_InitProcess = NULL;

static Process_t *g_processes[MAX_PROCESS_COUNT];
static int g_processCount = 0, g_processIndex = 0;
static int g_nextID = ROOT_PID;

static Process_t *getNextProcess()
{
    Process_t *curr;
    do
    {
        if (g_processIndex >= g_processCount)
            g_processIndex = 0;
        
        curr = g_processes[g_processIndex++];
        if (curr->id != 0)
            return curr;
    } while (true);
}

void spawnInit()
{
    void *initStack = kmalloc(INIT_PROC_STACK_SIZE);
    assert(initStack);
    
    _CurrentProcess = _InitProcess = process_create("init", x64_idle_proccess, initStack, INIT_PROC_STACK_SIZE);
    _InitProcess->id = ROOT_PID;
    SWITCH_PROCESS(_InitProcess);
}

void scheduler_addProcess(Process_t *process)
{
    process->id = ++g_nextID;
    g_processes[g_processCount++] = process;
}

void scheduler_remove(Process_t *process)
{
    process->id = 0;
    _CurrentProcess = getNextProcess();
    SWITCH_PROCESS(_CurrentProcess);
}

void yield(InterruptStack_t *stack)
{
    _CurrentProcess->regs.rsp = stack->rsp;
    _CurrentProcess->regs.rbp = stack->rbp;
    _CurrentProcess->regs.rip = stack->rip;
    _CurrentProcess->regs.rflags = stack->rflags;
    _CurrentProcess->regs.rax = stack->rax;
    _CurrentProcess->regs.rbx = stack->rbx;
    _CurrentProcess->regs.rcx = stack->rcx;
    _CurrentProcess->regs.rdx = stack->rdx;
    _CurrentProcess->regs.rdi = stack->rdi;
    _CurrentProcess->regs.rsi = stack->rsi;
    
    _CurrentProcess = getNextProcess();
    LOG("%s\n", _CurrentProcess->name);
    SWITCH_PROCESS(_CurrentProcess);
}