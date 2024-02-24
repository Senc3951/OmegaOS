#include <sys/scheduler.h>
#include <arch/apic/apic.h>
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
    
    scheduler_add(currentProcess());
    LOG("Scheduler initialized\n");
}

void scheduler_add(Process_t *process)
{
    assert(process->priority >= 0 && process->priority < PROCESS_PRIORITIES_COUNT);
    queue_enqueue(g_processQueues[process->priority], process);
}

void scheduler_remove(Process_t *process)
{
    assert(process->priority >= 0 && process->priority < PROCESS_PRIORITIES_COUNT);
    queue_remove(g_processQueues[process->priority], process);
}

void yield(Process_t *process)
{
    CoreContext_t *core = currentCPU();
    if (process)
        core->currentProcess = process;
    else
        core->currentProcess = getNextProcess();
    
    SWITCH_PROCESS(core->currentProcess);
}

Process_t *dispatch(InterruptStack_t *stack)
{
    Process_t *current = currentProcess();
    current->ctx.rip = stack->rip;
    current->ctx.cs = stack->cs;
    current->ctx.rsp = stack->rsp;
    current->ctx.rflags = stack->rflags;
    current->ctx.ss = stack->ds;
    current->ctx.rax = stack->rax;
    current->ctx.rbx = stack->rbx;
    current->ctx.rcx = stack->rcx;
    current->ctx.rdx = stack->rdx;
    current->ctx.rdi = stack->rdi;
    current->ctx.rsi = stack->rsi;
    current->ctx.rbp = stack->rbp;
    current->ctx.r8 = stack->r8;
    current->ctx.r9 = stack->r9;
    current->ctx.r10 = stack->r10;
    current->ctx.r11 = stack->r11;
    current->ctx.r12 = stack->r12;
    current->ctx.r13 = stack->r13;
    current->ctx.r14 = stack->r14;
    current->ctx.r15 = stack->r15;   
    
    queue_enqueue(g_processQueues[current->priority], current);
    return getNextProcess();
}

Process_t *currentProcess()
{
    return currentCPU()->currentProcess;
}