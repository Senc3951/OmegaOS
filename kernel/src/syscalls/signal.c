#include <sys/scheduler.h>
#include <errno.h>
#include <logger.h>

int sys_signal(int signum, uint64_t handler)
{
    LOG_PROC("sys_signal signal %d with handler %p\n", signum, handler);
    if (signum > SIG_COUNT || signum == 0)
        return EPERM;
    
    _CurrentProcess->sigtb[signum - 1] = handler;
    return ENOER;
}