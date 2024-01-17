#include <sys/scheduler.h>
#include <fs/vfs.h>
#include <logger.h>

struct dirent *sys_readdir(DIR *dirp)
{
    uint32_t fd = dirp->fd;
    LOG_PROC("sys_readdir directory %u (index %u)\n", fd, dirp->i);

    if (fd >= _CurrentProcess->fdt->size)
        return NULL;
    
    VfsNode_t *node = _CurrentProcess->fdt->nodes[fd];
    return vfs_readdir(node, dirp->i++);
}