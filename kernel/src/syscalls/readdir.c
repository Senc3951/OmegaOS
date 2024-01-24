#include <sys/scheduler.h>
#include <fs/vfs.h>
#include <logger.h>

struct dirent *sys_readdir(DIR *dirp)
{
    uint32_t fd = dirp->fd;
    LOG_PROC("sys_readdir directory %u (index %u)\n", fd, dirp->currentEntry);
    
    if (!dirp)
        return NULL;
    if (fd >= _CurrentProcess->fdt->length)
        return NULL;
    
    VfsNode_t *node = PROC_FILE_AT(fd);
    return vfs_readdir(node, dirp->currentEntry++);
}