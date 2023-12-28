#include <sys/scheduler.h>
#include <mem/heap.h>
#include <io/io.h>
#include <assert.h>
#include <libc/string.h>

#define SWITCH_PROCESS(process) ({                          \
    process->status = PROCESS_STATUS_RUNNING;               \
    x64_switch_processes(&process->ctx, process->ctx.cr3);  \
})

extern void x64_switch_processes(Context_t *ctx, uint64_t cr3);

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
        if (curr->status == PROCESS_STATUS_PENDING)
            return curr;
    } while (true);
}

void spawnInit()
{    
    _CurrentProcess = _InitProcess = process_createInit();
    _InitProcess->id = ROOT_PID;
    
    SWITCH_PROCESS(_InitProcess);
}

void scheduler_add(Process_t *process)
{
    process->id = ++g_nextID;
    g_processes[g_processCount++] = process;
}

void scheduler_remove(Process_t *process)
{
    assert(process->status != PROCESS_STATUS_STOPPED);
    process->status = PROCESS_STATUS_STOPPED;
}

void yield()
{
    _CurrentProcess = getNextProcess();
    SWITCH_PROCESS(_CurrentProcess);
}

void yield_cs(InterruptStack_t *stack)
{
    assert(_CurrentProcess->status == PROCESS_STATUS_RUNNING);

    _CurrentProcess->ctx.rip = stack->rip;
    _CurrentProcess->ctx.cs = stack->cs;
    _CurrentProcess->ctx.rsp = stack->rsp;
    _CurrentProcess->ctx.rflags = stack->rflags;
    _CurrentProcess->ctx.ss = stack->ds;
    _CurrentProcess->ctx.rax = stack->rax;
    _CurrentProcess->ctx.rbx = stack->rbx;
    _CurrentProcess->ctx.rcx = stack->rcx;
    _CurrentProcess->ctx.rdx = stack->rdx;
    _CurrentProcess->ctx.rdi = stack->rdi;
    _CurrentProcess->ctx.rsi = stack->rsi;
    _CurrentProcess->ctx.rbp = stack->rbp;
    _CurrentProcess->ctx.r8 = stack->r8;
    _CurrentProcess->ctx.r9 = stack->r9;
    _CurrentProcess->ctx.r10 = stack->r10;
    _CurrentProcess->ctx.r11 = stack->r11;
    _CurrentProcess->ctx.r12 = stack->r12;
    _CurrentProcess->ctx.r13 = stack->r13;
    _CurrentProcess->ctx.r14 = stack->r14;
    _CurrentProcess->ctx.r15 = stack->r15;
    
    _CurrentProcess->status = PROCESS_STATUS_PENDING;
    yield();
}