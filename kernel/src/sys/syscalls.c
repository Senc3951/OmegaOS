#include <sys/syscalls.h>
#include <sys/scheduler.h>
#include <arch/isr.h>
#include <io/io.h>
#include <assert.h>
#include <logger.h>

extern ssize_t sys_read(uint32_t fd, void *buf, size_t count);
extern ssize_t sys_write(uint32_t fd, const void *buf, size_t count);
extern uint32_t sys_open(const char *path, int flags, int mode);
extern int sys_close(uint32_t fd);

typedef uint64_t (*syscall_func_t)(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t);
static syscall_func_t g_syscalls[] = 
{
    [SYSCALL_READ]  = (syscall_func_t)(uint64_t)sys_read,
    [SYSCALL_WRITE] = (syscall_func_t)(uint64_t)sys_write,
    [SYSCALL_OPEN]  = (syscall_func_t)(uint64_t)sys_open,
    [SYSCALL_CLOSE] = (syscall_func_t)(uint64_t)sys_close,
    [SYSCALL_EXIT]  = (syscall_func_t)(uint64_t)sys_exit
};

#define SYSCALL_COUNT (sizeof(g_syscalls) / sizeof(*g_syscalls))

static void syscallHandler(InterruptStack_t *stack)
{
    uint64_t num = stack->rax;
    if (num >= SYSCALL_COUNT)
    {
        LOG_PROC("Terminating process because received an invalid syscall (%llu)\n", num);
        sys_exit(0);
    }
        
    stack->rax = g_syscalls[num](stack->rdi, stack->rsi, stack->rdx, stack->r10, stack->r8, stack->r9);
}

void syscalls_init()
{
    assert(isr_registerHandler(SYSCALL, syscallHandler));
}