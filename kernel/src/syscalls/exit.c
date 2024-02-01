#include <sys/scheduler.h>
#include <io/io.h>
#include <assert.h>
#include <panic.h>
#include <logger.h>

void sys_exit(int status)
{
    UNUSED(status);
    
    __CLI();
    assert(_CurrentProcess->id != ROOT_PID);
    
    // Delete the process
    LOG_PROC("sys_exit with status %d\n", status);
    process_delete(_CurrentProcess);
    
    // Execute another process
    __STI();
    yield(NULL);
    
    panic("Unreachable");
}