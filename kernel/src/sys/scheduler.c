#include <sys/scheduler.h>
#include <mem/heap.h>
#include <misc/queue.h>
#include <io/io.h>
#include <assert.h>

#define SWITCH_PROCESS(process) ({      \
    process->status = PROCESS_RUNNING;  \
    vmm_switchTable(process->pml4);     \
    x64_context_switch(&process->ctx);  \
})

extern void x64_context_switch(Context_t *ctx);

Process_t *_CurrentProcess = NULL;

static Queue_t *g_readyQueue = NULL;

static int getNextID()
{
    static int id = ROOT_PID;
    return id++;
}

static Process_t *getNextProcess()
{
    Process_t *next = queue_deqeueue(g_readyQueue);
    if (next->id == ROOT_PID && g_readyQueue->count >= 1)
    {
        queue_enqueue(g_readyQueue, next);
        return queue_deqeueue(g_readyQueue);
    }
    
    return next;
}

void scheduler_init(Process_t *init)
{
    assert(g_readyQueue = queue_create());
    
    _CurrentProcess = init;
    scheduler_add(init);
}

void scheduler_add(Process_t *process)
{
    process->id = getNextID();
    process->status = PROCESS_PENDING;
    
    queue_enqueue(g_readyQueue, process);
}

void scheduler_remove(Process_t *process)
{
    assert(process->id != ROOT_PID);
    queue_remove(g_readyQueue, process);
}

void yield()
{
    _CurrentProcess = getNextProcess();
    SWITCH_PROCESS(_CurrentProcess);
}

void yield_cs(InterruptStack_t *stack)
{
    assert(_CurrentProcess->status == PROCESS_RUNNING);
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
    _CurrentProcess->status = PROCESS_PENDING;
    
    queue_enqueue(g_readyQueue, _CurrentProcess);
    yield();
}