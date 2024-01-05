#include <sys/syscalls.h>
#include <sys/scheduler.h>
#include <dev/ps2/kbd.h>
#include <gui/screen.h>
#include <arch/isr.h>
#include <io/io.h>
#include <assert.h>
#include <logger.h>

#define STDIN   0
#define STDOUT  1
#define STDERR  2

void sys_exit(int status)
{
    __CLI();
    LOG("Terminating [%s:%d] with status %d\n", _CurrentProcess->name, _CurrentProcess->id, status);
    
    scheduler_remove(_CurrentProcess);
    process_delete(_CurrentProcess);
    
    __STI();
    yield();
}

static ssize_t sys_read(uint32_t fd, char *buf, size_t count)
{
    switch (fd)
    {
        case STDIN:
            for (size_t i = 0; i < count; i++)
                buf[i] = ps2_kbd_getc();
            
            return count;
        case STDOUT:
        case STDERR:
            return 0;
        default:
            LOG("TODO sys_read\n");
            return -1;
    }
}

static ssize_t sys_write(uint32_t fd, const char *buf, size_t count)
{
    LOG("sys_write: %u %p %u\n",fd,buf,count);
    switch (fd)
    {
        case STDIN:
        case STDERR:
            for (size_t i = 0; i < count; i++)
                screen_putc(buf[i]);
            
            return count;
        case STDOUT:
            return 0;
        default:
            LOG("TODO sys_write\n");
            return -1;
    }
}

typedef uint64_t (*syscall_func_t)(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t);
static syscall_func_t g_syscalls[] = 
{
    [SYSCALL_READ] = (syscall_func_t)(uint64_t)sys_read,
    [SYSCALL_WRITE] = (syscall_func_t)(uint64_t)sys_write,
    [SYSCALL_EXIT]  = (syscall_func_t)(uint64_t)sys_exit
};

#define SYSCALL_COUNT (sizeof(g_syscalls) / sizeof(*g_syscalls))

static void syscallHandler(InterruptStack_t *stack)
{
    uint64_t num = stack->rax;
    if (num >= SYSCALL_COUNT)
        sys_exit(0);
    
    long ret = g_syscalls[num](stack->rdi, stack->rsi, stack->rdx, stack->r10, stack->r8);
    stack->rax = ret;
}

void syscalls_init()
{
    for (uint32_t i = 0; i < SYSCALL_COUNT; i++)
        assert(g_syscalls[i]);
    
    assert(isr_registerHandler(SYSCALL, syscallHandler));
}