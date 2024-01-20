#include <sys/scheduler.h>
#include <mem/heap.h>
#include <logger.h>

int sys_raise(int sig)
{
    LOG_PROC("sys_raise with signal %d\n", sig);
    if (sig > SIG_COUNT || sig == 0)
        return EPERM;

    signal_t *signal = (signal_t *)kmalloc(sizeof(signal_t));
    if (!signal)
        return ENOMEM;
    
    signal->num = sig;
    signal->handler = _CurrentProcess->sigtb[sig - 1];
    
    queue_enqueue(_CurrentProcess->sigQueue, signal);
    return ENOER;
}