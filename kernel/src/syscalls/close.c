#include <sys/scheduler.h>
#include <fs/vfs.h>
#include <logger.h>

int sys_close(uint32_t fd)
{
    LOG_PROC("sys_close file %u\n", fd);
    if (fd >= _CurrentProcess->fdt->size)
        return -ENOENT;
    
    vfs_close(_CurrentProcess->fdt->nodes[fd]);
    return ENOER;
}