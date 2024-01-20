#include <sys/syscalls.h>
#include <sys/scheduler.h>
#include <arch/isr.h>
#include <io/io.h>
#include <assert.h>
#include <logger.h>

extern ssize_t sys_read(uint32_t fd, void *buf, size_t count);
extern ssize_t sys_write(uint32_t fd, const void *buf, size_t count);
extern int sys_open(const char *path, int flags, int mode);
extern int sys_close(uint32_t fd);
extern int sys_mkdir(const char *name, uint32_t attr);
extern long sys_ftell(uint32_t fd);
extern int sys_lseek(uint32_t fd, long offset, int whence);
extern DIR *sys_opendir(const char *name);
extern struct dirent *sys_readdir(DIR *dirp);
extern void sys_closedir(DIR *dirp);
extern int sys_raise(int sig);
extern int sys_signal(int signum, uint64_t handler);

typedef uint64_t (*syscall_func_t)(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t);
static syscall_func_t g_syscalls[] = 
{
    [SYSCALL_READ]      = (syscall_func_t)(uint64_t)sys_read,
    [SYSCALL_WRITE]     = (syscall_func_t)(uint64_t)sys_write,
    [SYSCALL_OPEN]      = (syscall_func_t)(uint64_t)sys_open,
    [SYSCALL_CLOSE]     = (syscall_func_t)(uint64_t)sys_close,
    [SYSCALL_EXIT]      = (syscall_func_t)(uint64_t)sys_exit,
    [SYSCALL_MKDIR]     = (syscall_func_t)(uint64_t)sys_mkdir,
    [SYSCALL_FTELL]     = (syscall_func_t)(uint64_t)sys_ftell,
    [SYSCALL_LSEEK]     = (syscall_func_t)(uint64_t)sys_lseek,
    [SYSCALL_OPENDIR]   = (syscall_func_t)(uint64_t)sys_opendir,
    [SYSCALL_READDIR]   = (syscall_func_t)(uint64_t)sys_readdir,
    [SYSCALL_CLOSEDIR]  = (syscall_func_t)(uint64_t)sys_closedir,
    [SYSCALL_RAISE]     = (syscall_func_t)(uint64_t)sys_raise,
    [SYSCALL_SIGNAL]    = (syscall_func_t)(uint64_t)sys_signal
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