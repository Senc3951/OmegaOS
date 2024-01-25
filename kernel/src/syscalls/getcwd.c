#include <fs/vfs.h>
#include <logger.h>
#include <libc/string.h>

char *sys_getcwd(char *buf, size_t size)
{
    LOG_PROC("sys_getcwd to %p with size %lu\n", buf, size);
    if (!buf || !size)
        return NULL;
    
    return strncpy(buf, _CurrentProcess->cwd, size);        
}