#include <scheduler/scheduler.h>
#include <scheduler/process.h>
#include <io/io.h>
#include <assert.h>

extern void x64_switch_processes(Registers_t *regs);

void switchProcess(Process_t *proc)
{
    __CLI();
    
    //x64_switch_processes(&proc->regs);
    asm volatile("      \
        mov %0, %%rsp;  \
        mov %1, %%rbp;  \
        sti;            \
        jmp *%2;        "
        : : "r"(proc->regs.rsp), "r"(proc->regs.rbp), "r"(proc->regs.rip));
}

void scheduler_init()
{
    assert(_CurrentProcess);
    switchProcess(_CurrentProcess);
}

void yield(InterruptStack_t *stack)
{
    // Store old information
    _CurrentProcess->regs.rsp = stack->rsp;
    _CurrentProcess->regs.rbp = stack->rbp;
    _CurrentProcess->regs.rip = stack->rip;
    
    _CurrentProcess = process_getNext();
    switchProcess(_CurrentProcess);
}