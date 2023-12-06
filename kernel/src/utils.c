#include <utils.h>
#include <gui/screen.h>

void dumpStack(InterruptStack_t *stack)
{
    screen_puts("\nDumping Stack\n");
    kprintf("RAX = 0x%p, RBX = 0x%p, RCX = 0x%p, RDX = 0x%p, RSI = 0x%p, RDI = 0x%p\n", 
        stack->rax, stack->rbx, stack->rcx, stack->rdx, stack->rsi, stack->rdi);
    kprintf("R8  = 0x%p, R9  = 0x%p, R10 = 0x%p, R11 = 0x%p, R12 = 0x%p, R13 = 0x%p, R14 = 0x%p, R15 = 0x%p\n",
        stack->r8, stack->r9, stack->r10, stack->r11, stack->r12, stack->r13, stack->r14, stack->r15);
    kprintf("CS  = 0x%p, DS  = 0x%p, SS  = 0x%p, RIP = 0x%p, RSP = 0x%p, RFL = 0x%p\n",
        stack->cs, stack->ds, stack->ss, stack->rip, stack->rsp, stack->rflags);
    kprintf("CR0 = 0x%p, CR2 = 0x%p, CR3 = 0x%p\n", stack->cr0, stack->cr2, stack->cr3);
}