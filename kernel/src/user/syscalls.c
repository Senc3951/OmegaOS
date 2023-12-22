#include <user/syscalls.h>
#include <arch/gdt.h>
#include <arch/isr.h>
#include <mem/heap.h>
#include <assert.h>
#include <logger.h>

int sys_exit(int code)
{
    LOG("returned: %u\n",code);
    return code;
}

int sys_test(int a)
{
    LOG("syscall 0!: %d\n", a);
    return 1;
}

typedef long (*syscall_func_t)(int64_t, int64_t, int64_t, int64_t, int64_t);
static syscall_func_t g_syscalls[] = 
{
    (syscall_func_t)(uint64_t)sys_exit,
    (syscall_func_t)(uint64_t)sys_test
};

#define SYSCALL_COUNT (sizeof(g_syscalls) / sizeof(*g_syscalls))

static void syscallHandler(InterruptStack_t *stack)
{
    uint64_t num = stack->rax;
    assert(num < SYSCALL_COUNT && g_syscalls[num]);
        
    long ret = g_syscalls[num](stack->rdi, stack->rsi, stack->rdx, stack->rcx, stack->rbx);
    stack->rax = ret;
}

void syscalls_init()
{
    for (uint32_t i = 0; i < SYSCALL_COUNT; i++)
        assert(g_syscalls[i]);
    
    assert(isr_registerHandler(SYSCALL, false, syscallHandler));
}