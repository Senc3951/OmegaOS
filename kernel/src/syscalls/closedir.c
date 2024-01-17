#include <sys/scheduler.h>
#include <fs/vfs.h>
#include <mem/heap.h>
#include <logger.h>

void sys_closedir(DIR *dirp)
{
    uint32_t fd = dirp->fd;
    LOG_PROC("sys_closedir directory %u\n", fd);
    
    process_close_file(_CurrentProcess, fd);
    kfree(dirp);
}