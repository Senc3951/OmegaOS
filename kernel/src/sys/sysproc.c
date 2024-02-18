#include <sys/scheduler.h>
#include <io/io.h>
#include <assert.h>
#include <panic.h>
#include <logger.h>

void sys_exit(int status)
{
    UNUSED(status);
    
    __CLI();

    Process_t *current = currentProcess();
    assert(current->id != ROOT_PID);
    
    // Delete the process
    LOG_PROC("sys_exit with status %d\n", status);
    process_delete(current);
    
    // Execute another process
    __STI();
    yield(NULL);
    
    panic("Unreachable");
}