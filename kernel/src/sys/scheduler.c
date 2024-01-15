#include <sys/scheduler.h>
#include <mem/heap.h>
#include <misc/queue.h>
#include <io/io.h>
#include <assert.h>
#include <panic.h>
#include <logger.h>

#define SWITCH_PROCESS(process) ({      \
    vmm_switchTable(process->pml4);     \
    x64_context_switch(&process->ctx);  \
})

extern void x64_context_switch(Context_t *ctx);

Process_t *_CurrentProcess = NULL, *_IdleProcess = NULL;
static Queue_t **g_processQueues = NULL;

static Process_t *getNextProcess()
{
    // Search for the last index because the priorities are opposite in order
    for (int i = PROCESS_PRIORITIES_COUNT - 1; i >= 0; i--)
    {
        Queue_t *q = g_processQueues[i];
        if (q->count > 0)
            return queue_deqeueue(q);
    }
        
    panic("No processes in scheduler");
}

void scheduler_init()
{
    // Create the process queues
    assert(g_processQueues = (Queue_t **)kmalloc(sizeof(Queue_t *) * PROCESS_PRIORITIES_COUNT));
    for (uint16_t i = 0; i < PROCESS_PRIORITIES_COUNT; i++)
       assert(g_processQueues[i] = queue_create());
    
    scheduler_add(_IdleProcess);
    LOG("Scheduler initialized\n");
}

void scheduler_add(Process_t *process)
{
    LOG("[REMOVE] priority of %s/%u is %u\n", process->name, process->id, process->priority);
    assert(process->priority >= 0 && process->priority < PROCESS_PRIORITIES_COUNT);
    queue_enqueue(g_processQueues[process->priority], process);
}

void scheduler_remove(Process_t *process)
{
    assert(process->priority >= 0 && process->priority < PROCESS_PRIORITIES_COUNT);
    queue_remove(g_processQueues[process->priority], process);
}

void yield()
{
    _CurrentProcess = getNextProcess();
    SWITCH_PROCESS(_CurrentProcess);
}

void yield_cs(InterruptStack_t *stack)
{
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
    
    queue_enqueue(g_processQueues[_CurrentProcess->priority], _CurrentProcess);
    yield();
}